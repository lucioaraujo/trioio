#include "LibraryImporter.h"

#include "AudioSource.h"
#include "ManifestJson.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

#include <algorithm>
#include <map>
#include <memory>
#include <set>

namespace trioio::library
{
using namespace trioio::provenance;

namespace
{
juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.data(), static_cast<int>(s.size())); }

const juce::StringArray kMidiExt { "mid", "midi" };

// O que a varredura descobriu sobre um arquivo, antes de decidir o manifesto.
struct Found
{
    std::string path;
    std::string id;
    MaterialType material = MaterialType::audio;
    TechnicalIdentity identity;
    std::vector<std::pair<std::string, std::string>> metadata;
};

struct Known
{
    Manifest manifest;
    juce::File file;
    bool dirty = false;
};

bool hasHistory(const Manifest& m, const std::string& event, const std::string& note)
{
    return std::any_of(m.history.begin(), m.history.end(),
                       [&](const HistoryEvent& h) { return h.event == event && h.note == note; });
}

void addEvent(Known& k, const std::string& at, const std::string& event, const std::string& note)
{
    k.manifest.history.push_back({ at, event, "importador", note });
    k.manifest.updatedAt = at;
    k.dirty = true;
}

// "stems" em qualquer nível entre a raiz e o arquivo
bool insideFolder(const juce::File& f, const juce::File& root, const juce::String& name)
{
    for (auto p = f.getParentDirectory(); p != root && p.isAChildOf(root); p = p.getParentDirectory())
        if (p.getFileName().equalsIgnoreCase(name))
            return true;
    return false;
}

bool writeAtomically(const juce::File& target, const Manifest& m)
{
    juce::TemporaryFile tmp(target);
    if (!tmp.getFile().replaceWithText(u8(toJson(m)), false, false, "\n"))
        return false;
    return tmp.overwriteTargetFileWithTemporary();
}
} // namespace

std::string referenceManifestDir(const std::string& dataRoot)
{
    return juce::File(u8(dataRoot)).getChildFile("indice").getChildFile("referencias")
        .getFullPathName().toStdString();
}

ScanReport scanLibrary(const std::string& libraryRoot, const std::string& dataRoot,
                       const ImportLimits& limits, const std::atomic<bool>* cancel,
                       const std::string& nowArg)
{
    ScanReport rep;
    const std::string now = nowArg.empty()
        ? juce::Time::getCurrentTime().toISO8601(true).toStdString() : nowArg;
    const auto cancelled = [&] { return cancel && cancel->load(std::memory_order_relaxed); };

    const juce::File root(u8(libraryRoot));
    const juce::File data(u8(dataRoot));
    // a Biblioteca pode ser um arquivo só ("trazer gravação…" na interface):
    // registra só ele, e nada da pasta vizinha é tocado nem dado como ausente
    const bool single = root.existsAsFile();
    if (!single && !root.isDirectory())
    {
        rep.errors.push_back("Biblioteca não encontrada: " + libraryRoot);
        return rep;
    }
    if (!single && (data == root || data.isAChildOf(root)))
    {
        rep.errors.push_back("a raiz de dados não pode ficar dentro da Biblioteca: o importador só lê a Biblioteca");
        return rep;
    }
    const juce::File refsDir(u8(referenceManifestDir(dataRoot)));
    if (!refsDir.createDirectory())
    {
        rep.errors.push_back("não foi possível criar " + refsDir.getFullPathName().toStdString());
        return rep;
    }

    // ---- manifestos já existentes ---------------------------------------
    std::map<std::string, Known> known;
    std::set<std::string> blockedIds;  // arquivo de manifesto ilegível: nunca sobrescrever
    for (const auto& f : refsDir.findChildFiles(juce::File::findFiles, false, "*.json"))
    {
        const auto read = fromJson(f.loadFileAsString().toStdString());
        if (!read.manifest)
        {
            blockedIds.insert(f.getFileNameWithoutExtension().toStdString());
            for (const auto& e : read.errors)
                rep.errors.push_back(f.getFileName().toStdString() + ": " + e);
            continue;
        }
        known[read.manifest->id] = { *read.manifest, f, false };
    }

    // ---- varredura (só leitura) ------------------------------------------
    juce::Array<juce::File> files;
    if (single)
    {
        const auto ext = root.getFileExtension().trimCharactersAtStart(".").toLowerCase();
        if (audioExtensions().contains(ext) || kMidiExt.contains(ext))
            files.add(root);
        else
            rep.skipped.push_back(root.getFullPathName().toStdString() + ": não é áudio nem MIDI");
    }
    else
    for (const auto& entry : juce::RangedDirectoryIterator(root, true, "*", juce::File::findFiles))
    {
        const auto f = entry.getFile();
        if (f.getFileName().startsWithChar('.') || insideFolder(f, root, "sessoes"))
            continue;  // ocultos; saídas do próprio TRIOIO são registradas pela sessão
        const auto ext = f.getFileExtension().trimCharactersAtStart(".").toLowerCase();
        if (audioExtensions().contains(ext) || kMidiExt.contains(ext))
            files.add(f);
    }
    std::sort(files.begin(), files.end(),
              [](const juce::File& a, const juce::File& b) { return a.getFullPathName() < b.getFullPathName(); });
    if (files.size() > limits.maxFiles)
    {
        rep.errors.push_back("mais de " + std::to_string(limits.maxFiles)
                             + " arquivos; só os primeiros foram lidos");
        files.removeRange(limits.maxFiles, files.size());
    }

    std::vector<Found> found;
    for (const auto& f : files)
    {
        if (cancelled())
        {
            rep.cancelled = true;
            return rep;  // nada gravado
        }
        const auto path = f.getFullPathName().toStdString();
        const auto size = static_cast<std::uint64_t>(f.getSize());
        if (size == 0 || size > limits.maxFileBytes)
        {
            rep.skipped.push_back(path + ": tamanho fora do limite");
            continue;
        }

        Found x;
        x.path = path;
        x.identity.canonicalPath = path;
        x.identity.sizeBytes = size;
        x.identity.modifiedAt = f.getLastModificationTime().toISO8601(true).toStdString();
        x.identity.format = f.getFileExtension().trimCharactersAtStart(".").toLowerCase().toStdString();

        const auto ext = u8(x.identity.format);
        if (kMidiExt.contains(ext))
        {
            x.material = MaterialType::midi;
            juce::FileInputStream in(f);
            juce::MidiFile midi;
            if (!in.openedOk() || !midi.readFrom(in))
            {
                rep.skipped.push_back(path + ": MIDI ilegível");
                continue;
            }
            midi.convertTimestampTicksToSeconds();
            x.identity.durationSeconds = midi.getLastTimestamp();
        }
        else
        {
            x.material = insideFolder(f, root, "stems") ? MaterialType::stem : MaterialType::audio;
            // WAV, AIFF, FLAC, Ogg e MP3 pelo JUCE; os demais pelo ffprobe
            // (AudioSource.h), sem decodificar o arquivo
            const auto info = probeAudio(f);
            if (!info.ok)
            {
                rep.skipped.push_back(path + ": formato de áudio não lido" + (info.error.empty() ? "" : " (" + info.error + ")"));
                continue;
            }
            x.identity.sampleRate = static_cast<std::uint32_t>(info.sampleRate);
            x.identity.channels = info.channels;
            x.identity.durationSeconds = info.durationSeconds;
            x.metadata = info.metadata;
            if (info.via == "ffmpeg")
                x.metadata.emplace_back("decodificador", "ffmpeg");
            if (!info.warning.empty())
                x.metadata.emplace_back("aviso", info.warning);
        }

        juce::FileInputStream in(f);
        if (!in.openedOk())
        {
            rep.skipped.push_back(path + ": não abriu para leitura");
            continue;
        }
        const auto hex = juce::SHA256(in, -1).toHexString().toStdString();
        x.identity.contentSha256 = hex;
        x.id = "ref-" + hex.substr(0, 16);
        if (blockedIds.count(x.id))
        {
            rep.skipped.push_back(path + ": o manifesto deste conteúdo está ilegível; corrigir à mão antes");
            continue;
        }
        found.push_back(std::move(x));
    }

    // ---- reconciliação ---------------------------------------------------
    std::map<std::string, std::vector<const Found*>> byId;
    std::map<std::string, std::string> idAtPath;
    for (const auto& x : found)
    {
        byId[x.id].push_back(&x);
        idAtPath[x.path] = x.id;
    }

    for (auto& [id, list] : byId)
    {
        auto it = known.find(id);
        if (it == known.end())
        {
            const Found& x = *list.front();
            Manifest m;
            m.id = id;
            m.createdAt = now;
            m.updatedAt = now;
            m.material = x.material;
            m.identity = x.identity;
            m.metadataImported = x.metadata;
            m.history = { { now, "registrado", "importador", "encontrado na Biblioteca; origem e autorização a declarar" } };
            // O caminho guardava outro conteúdo: é uma nova versão, não herda nada.
            for (const auto& [kid, k] : known)
                if (k.manifest.kind == ManifestKind::reference && k.manifest.identity.canonicalPath == x.path
                    && !byId.count(kid))  // se o conteúdo antigo só mudou de lugar, não é versão
                    m.relations.push_back({ "version_of", kid });
            known[id] = { std::move(m), refsDir.getChildFile(u8(id) + ".json"), true };
            ++rep.registered;
            rep.duplicates += static_cast<int>(list.size()) - 1;
            continue;
        }

        Known& k = it->second;
        const auto& oldPath = k.manifest.identity.canonicalPath;
        const auto here = std::find_if(list.begin(), list.end(), [&](const Found* x) { return x->path == oldPath; });
        const bool wasMissing = k.manifest.analysis == AnalysisState::unavailable;
        if (here != list.end())
        {
            if (wasMissing)
            {
                k.manifest.analysis = AnalysisState::none;
                addEvent(k, now, "reencontrado", oldPath);
                ++rep.reappeared;
            }
            else
                ++rep.unchanged;
        }
        else
        {
            const Found& x = *list.front();
            addEvent(k, now, "novo caminho", oldPath + " -> " + x.path);
            k.manifest.identity.canonicalPath = x.path;
            k.manifest.identity.modifiedAt = x.identity.modifiedAt;
            if (wasMissing)
                k.manifest.analysis = AnalysisState::none;
            ++rep.relocated;
        }
        for (const auto* x : list)
            if (x->path != k.manifest.identity.canonicalPath)
            {
                ++rep.duplicates;
                if (!hasHistory(k.manifest, "cópia idêntica", x->path))
                    addEvent(k, now, "cópia idêntica", x->path);
            }
    }

    // conhecidos desta Biblioteca que não apareceram
    for (auto& [id, k] : known)
    {
        if (k.manifest.kind != ManifestKind::reference || byId.count(id))
            continue;
        const juce::File old(u8(k.manifest.identity.canonicalPath));
        if (single || !old.isAChildOf(root))
            continue;  // de outra Biblioteca: não é desta varredura
        if (k.manifest.analysis == AnalysisState::unavailable)
            continue;  // já registrado
        k.manifest.analysis = AnalysisState::unavailable;
        if (idAtPath.count(k.manifest.identity.canonicalPath))
        {
            addEvent(k, now, "conteúdo substituído", "o caminho agora guarda " + idAtPath[k.manifest.identity.canonicalPath]);
            ++rep.replaced;
        }
        else
        {
            addEvent(k, now, "ausente", k.manifest.identity.canonicalPath);
            ++rep.missing;
        }
    }

    if (cancelled())
    {
        rep.cancelled = true;
        return rep;
    }
    for (auto& [id, k] : known)
        if (k.dirty && !writeAtomically(k.file, k.manifest))
            rep.errors.push_back("falha ao gravar " + k.file.getFullPathName().toStdString());
    return rep;
}

ManagedCopyResult copyReferenceToManagedFolder(const std::string& dataRoot,
    const std::string& referenceId, const std::string& sourcePath)
{
    ManagedCopyResult result;
    const juce::File source(u8(sourcePath));
    const auto dir = juce::File(u8(dataRoot)).getChildFile("fontes");
    if (!source.existsAsFile() || source.getSize() <= 0 || source.getSize() > 4ll * 1024 * 1024 * 1024) {
        result.error = "fonte ausente ou acima do limite de 4 GiB"; return result;
    }
    if (source.isAChildOf(dir)) { result.ok = true; result.path = sourcePath; return result; }
    if (!dir.createDirectory() || dir.getBytesFreeOnVolume() <= source.getSize() + 256ll * 1024 * 1024) {
        result.error = "espaço livre insuficiente ou pasta indisponível"; return result;
    }
    const auto target = dir.getNonexistentChildFile(source.getFileNameWithoutExtension(), source.getFileExtension());
    const auto partial = dir.getNonexistentChildFile(u8(referenceId) + "-partial", ".tmp");
    if (!source.copyFileTo(partial) || !partial.moveFileTo(target)) {
        (void) partial.deleteFile(); result.error = "falha ao copiar a fonte"; return result;
    }
    const auto report = scanLibrary(target.getFullPathName().toStdString(), dataRoot);
    if (!report.errors.empty() || report.relocated + report.unchanged + report.reappeared + report.duplicates == 0) {
        (void) target.deleteFile(); result.error = "a cópia não corresponde a uma referência registrada"; return result;
    }
    result.ok = true; result.path = target.getFullPathName().toStdString(); return result;
}

} // namespace trioio::library
