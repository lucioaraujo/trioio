#include "AppModel.h"

#include "DataFiles.h"

#include "Analysis.h"
#include "LibraryImporter.h"
#include "ManifestJson.h"
#include "Reading.h"
#include "Response.h"
#include "ScaleArchive.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trioio::app
{
using namespace trioio::provenance;

namespace
{
juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.data(), static_cast<int>(s.size())); }

const char* statusName(const Manifest& m)
{
    if (m.analysis == AnalysisState::unavailable)
        return "indisponível";
    switch (m.authorization.status)
    {
        case AuthorizationStatus::pending: return "pendente";
        case AuthorizationStatus::authorized: return "autorizado";
        case AuthorizationStatus::refused: return "recusado";
        case AuthorizationStatus::revoked: return "revogado";
    }
    return "?";
}

const char* originName(OriginCategory c)
{
    switch (c)
    {
        case OriginCategory::authorial: return "autoral";
        case OriginCategory::performerCapture: return "captura";
        case OriginCategory::otherInstrument: return "outro instrumento";
        case OriginCategory::aiGenerated: return "IA";
        case OriginCategory::licensed: return "licenciado";
        case OriginCategory::publicDomain: return "domínio público";
        case OriginCategory::thirdPartyUnverified: return "origem desconhecida";
    }
    return "?";
}

const char* modeName(Strategy s)
{
    switch (s)
    {
        case Strategy::follow: return "seguir";
        case Strategy::transfigure: return "transfigurar";
        case Strategy::counter: return "contrapor";
        case Strategy::interrupt: return "interromper";
    }
    return "?";
}

// o trecho de `text` entre `from` e o próximo " · " (ou o fim)
std::string field(const std::string& text, const std::string& from)
{
    const auto a = text.find(from);
    if (a == std::string::npos)
        return {};
    const auto start = a + from.size();
    const auto b = text.find(" · ", start);
    return text.substr(start, b == std::string::npos ? std::string::npos : b - start);
}

const std::vector<composition::ArchiveScale>& languages()
{
    static const std::vector<composition::ArchiveScale> all = [] {
        std::vector<composition::ArchiveScale> out;
        const juce::File f = library::trioioDataFile("escalas/arquivo_trioio.txt");
        if (f.existsAsFile())
        {
            std::vector<std::string> errors;
            for (auto& s : composition::parseScaleArchive(f.loadFileAsString().toStdString(), errors))
                if (composition::isLanguageScale(s))
                    out.push_back(std::move(s));
        }
        return out;
    }();
    return all;
}
} // namespace

void AppModel::setDataRoot(const std::string& root) { root_ = root; }

std::vector<ReferenceItem> AppModel::references() const
{
    std::vector<ReferenceItem> out;
    if (root_.empty())
        return out;
    const juce::File dir(u8(library::referenceManifestDir(root_)));
    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.json");
    files.sort();
    for (const auto& f : files)
    {
        const auto read = library::fromJson(f.loadFileAsString().toStdString());
        if (!read.manifest)
            continue;
        const auto& m = *read.manifest;
        ReferenceItem r;
        r.id = m.id;
        r.name = juce::File(u8(m.identity.canonicalPath)).getFileName().toStdString();
        r.path = m.identity.canonicalPath;
        r.status = statusName(m);
        r.origin = originName(m.origin.category);
        r.duration = m.identity.durationSeconds;
        r.canDerive = permits(m, Use::derive);
        out.push_back(std::move(r));
    }
    return out;
}

std::vector<ResponseItem> AppModel::responsesFor(const std::string& referenceId) const
{
    std::vector<ResponseItem> out;
    if (root_.empty())
        return out;
    const juce::File dir(u8(library::derivationManifestDir(root_)));
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.json"))
    {
        const auto read = library::fromJson(f.loadFileAsString().toStdString());
        if (!read.manifest || !read.manifest->derivation)
            continue;
        const auto& d = *read.manifest->derivation;
        if (std::find(d.from.begin(), d.from.end(), referenceId) == d.from.end())
            continue;
        ResponseItem r;
        r.derivationId = read.manifest->id;
        r.sessionId = d.sessionId;
        r.mode = modeName(d.strategy);
        r.seed = d.seed;
        const auto session = juce::File(u8(root_)).getChildFile("sessoes").getChildFile(u8(d.sessionId));
        if (!session.isDirectory())
            continue;  // a sessão foi limpa (os testes não julgados): o manifesto fica no índice como registro
        r.decisionsPath = session.getChildFile(u8(d.decisionsRef.empty() ? std::string("decisoes.json") : d.decisionsRef))
                              .getFullPathName().toStdString();
        const auto wavs = session.getChildFile("saidas").findChildFiles(juce::File::findFiles, false, "*.wav");
        if (!wavs.isEmpty())
            r.wavPath = wavs[0].getFullPathName().toStdString();
        out.push_back(std::move(r));
    }
    // a sessão começa com a data ("sessao-AAAAMMDDhhmmss-…"): a mais nova primeiro
    std::sort(out.begin(), out.end(), [](const ResponseItem& a, const ResponseItem& b) { return a.sessionId > b.sessionId; });
    return out;
}

ReferenceView AppModel::loadReference(const std::string& referenceId) const
{
    ReferenceView v;
    const auto a = library::analyzeReference(root_, referenceId);
    if (!a.ok)
    {
        v.problem = a.problems.empty() ? "análise indisponível" : a.problems.front();
        return v;
    }
    v.ok = true;
    v.duration = a.duration;
    const auto n = static_cast<std::size_t>(std::ceil(a.duration / v.energyStep));
    v.energy.assign(n, 0.0f);
    std::vector<int> count(n, 0);
    for (const auto& f : a.frames)
    {
        const auto i = static_cast<std::size_t>(f.time / v.energyStep);
        if (i < n)
        {
            v.energy[i] += std::clamp((f.rmsDb + 60.0f) / 60.0f, 0.0f, 1.0f);
            ++count[i];
        }
    }
    for (std::size_t i = 0; i < n; ++i)
        if (count[i] > 0)
            v.energy[i] /= static_cast<float>(count[i]);
    const auto r = composition::readReference(a.frames, a.onsets, a.duration);
    v.chords = r.chords;
    v.bpm = r.pulse.bpm;
    v.beatsPerBar = r.pulse.beatsPerBar;
    v.key = r.key.name();
    if (!languages().empty())
    {
        const auto top = composition::rankArchive(languages(), r.structuralField, 1, 5, 9);
        if (!top.empty())
            v.language = top.front().scale->name + " sobre " + composition::chordFromIndex(top.front().tonic * 3).name();
    }
    return v;
}

ResponseView AppModel::loadResponse(const ResponseItem& item) const
{
    ResponseView v;
    const juce::File f(u8(item.decisionsPath));
    if (!f.existsAsFile())
    {
        v.problem = "decisoes.json não encontrado";
        return v;
    }
    const auto dj = juce::JSON::parse(f);
    if (const auto* notes = dj["notes"].getArray())
        for (const auto& n : *notes)
        {
            NoteView nv { static_cast<double>(n["time"]), static_cast<double>(n["duration"]),
                          static_cast<float>(static_cast<double>(n["pitch"])), static_cast<int>(n["voice"]) };
            v.duration = std::max(v.duration, nv.time + nv.duration);
            v.notes.push_back(nv);
        }
    std::vector<std::pair<double, std::string>> marks;
    if (const auto* form = dj["form"].getArray())
        for (const auto& m : *form)
            marks.push_back({ static_cast<double>(m["time"]), m["text"].toString().toStdString() });
    const std::string secPrefix = "seção ", eyePrefix = "Terceiro Olho · ";
    for (const auto& [t, text] : marks)
    {
        if (text.rfind(secPrefix, 0) == 0)
        {
            SectionView s;
            s.start = t;
            const auto colon = text.find("): ");
            std::string rest = colon == std::string::npos ? text : text.substr(colon + 3);
            std::string role = rest.substr(0, rest.find(" · "));
            std::string scale = field(rest, "campo: ");
            if (const auto c = scale.find(':'); c != std::string::npos)
                scale = scale.substr(0, c);  // "maqam Hijaz (…) sobre Lá: Lá Sib…" → o nome
            if (const auto p = scale.find(" ("); p != std::string::npos && scale.find(" sobre ") != std::string::npos)
                scale = scale.substr(0, p) + scale.substr(scale.find(" sobre "));
            s.label = role + (scale.empty() ? "" : " · " + scale);
            v.sections.push_back(s);
        }
        else if (text.rfind(eyePrefix, 0) == 0)
        {
            EyeView e;
            e.time = t;
            const auto body = text.substr(eyePrefix.size());
            e.what = body.substr(0, body.find(" · "));
            if (const auto por = e.what.find(" por "); por != std::string::npos)
                e.duration = std::atof(e.what.c_str() + por + 5);
            e.stays = field(body, "fica: ");
            v.eye.push_back(e);
        }
        else if (t <= 0.01)
            v.header.push_back(text);
    }
    for (std::size_t i = 0; i < v.sections.size(); ++i)
        v.sections[i].end = i + 1 < v.sections.size() ? v.sections[i + 1].start : v.duration;
    if (const auto* game = dj["game"].getArray())
        for (const auto& r : *game)
        {
            // o texto revelado (a regra apócrifa aparece com a origem inventada:
            // é o jogo; a verdade fica no decisoes.json)
            auto text = r["text"].toString().toStdString();
            const std::string prefix = "jogo · ";
            if (text.rfind(prefix, 0) == 0)
                text = text.substr(prefix.size());
            char tail[96];
            std::snprintf(tail, sizeof tail, " · disparou %d×, mudou %d notas", static_cast<int>(r["fired"]), static_cast<int>(r["changed"]));
            // os instantes dos disparos: o trecho a julgar para julgar a regra
            std::string at;
            if (const auto* fa = r["firedAt"].getArray())
                for (const auto& t : *fa)
                {
                    const int sec = static_cast<int>(static_cast<double>(t));
                    char buf[16];
                    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
                    at += (at.empty() ? " (em " : ", ") + std::string(buf);
                }
            if (!at.empty())
                at += ")";
            v.gameRules.push_back(text + tail + at);
        }
    v.ok = !v.notes.empty() || !v.sections.empty();
    if (!v.ok)
        v.problem = "resposta sem partitura (motor de energia)";
    return v;
}

ReferenceView demoReference()
{
    ReferenceView v;
    v.ok = true;
    v.duration = 138.0;
    for (int i = 0; i < 552; ++i)
        v.energy.push_back(0.55f + 0.2f * std::sin(i * 0.11f) + 0.1f * std::sin(i * 0.7f));
    const int prog[] = { 7 * 3 + 1, 2 * 3, 7 * 3 + 1, 0 * 3 + 1, 10 * 3, 3 * 3, 2 * 3 };
    for (int k = 0; k < 46; ++k)
        v.chords.push_back({ k * 3.0, k * 3.0 + 3.0, composition::chordFromIndex(prog[k % 7]), 0.8f });
    v.bpm = 84.9;
    v.key = "Sol menor";
    v.language = "maqam Hijaz sobre Ré";
    return v;
}

ResponseView demoResponse()
{
    ResponseView v;
    v.ok = true;
    v.duration = 138.0;
    v.header = { "pulso da referência: 84.9 BPM, compasso de 4 tempos", "escala do arquivo: maqam Hijaz sobre Ré" };
    const double starts[] = { 0, 20, 41, 62, 79, 112, 138 };
    const char* labels[] = { "apresentação · maqam Hijaz sobre Ré", "desenvolvimento · maqam Nahawand sobre Sol",
                             "desenvolvimento · maqam Hijaz sobre Ré", "desenvolvimento · maqam Nahawand sobre Sol",
                             "desenvolvimento · maqam Hijaz sobre Ré", "recapitulação · maqam Hijaz sobre Ré" };
    for (int i = 0; i < 6; ++i)
        v.sections.push_back({ starts[i], starts[i + 1], labels[i] });
    v.eye = { { 8.4, 8.6, "objeto sonoro: corpo da referência por 8.6 s", "o corpo da referência vira o timbre" },
              { 78.4, 9.0, "regra oculta: eco em cânone por 9.0 s", "" },
              { 127.6, 2.1, "objeto sonoro: massa de glissandos por 2.1 s", "" } };
    for (int i = 0; i < 300; ++i)
    {
        const double t = i * 0.46;
        v.notes.push_back({ t, 0.4, static_cast<float>(62 + 7 * std::sin(i * 0.37) + 3 * std::sin(i * 1.3)), 0 });
        if (i % 4 == 0)
            v.notes.push_back({ t, 1.8, static_cast<float>(43 + (i / 8) % 5), 1 });
    }
    return v;
}

} // namespace trioio::app
