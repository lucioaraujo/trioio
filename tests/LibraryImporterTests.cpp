#include "LibraryImporter.h"
#include "ManifestJson.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>

using namespace trioio::provenance;
using namespace trioio::library;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static juce::String s8(const char* t) { return juce::String::fromUTF8(t); }

static void escreverWav(const juce::File& f, double freq, double sr = 48000.0, int canais = 2)
{
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> out = std::make_unique<juce::FileOutputStream>(f);
    auto w = wav.createWriterFor(out, juce::AudioFormatWriterOptions {}
                                          .withSampleRate(sr).withNumChannels(canais).withBitsPerSample(24));
    const int n = static_cast<int>(sr * 0.2);
    juce::AudioBuffer<float> b(canais, n);
    for (int c = 0; c < canais; ++c)
        for (int i = 0; i < n; ++i)
            b.setSample(c, i, 0.25f * static_cast<float>(std::sin(2.0 * 3.14159265358979 * freq * i / sr)));
    w->writeFromAudioSampleBuffer(b, 0, n);
}

static void escreverMidi(const juce::File& f)
{
    f.getParentDirectory().createDirectory();
    juce::MidiMessageSequence seq;
    seq.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0.0);
    seq.addEvent(juce::MidiMessage::noteOff(1, 60), 960.0);
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote(480);
    mf.addTrack(seq);
    juce::FileOutputStream out(f);
    mf.writeTo(out);
}

// estado da Biblioteca: caminho -> hash do conteúdo + data de modificação
// o retrato da Biblioteca: as entradas, e de cada arquivo o conteúdo e a data
// lida do próprio arquivo. A data das PASTAS fica de fora: no NTFS ela vem do
// índice do diretório pai, atualizado de forma preguiçosa, e mudava sem
// nenhuma escrita (a CI do Windows, 2026-10-19)
static std::map<juce::String, juce::String> retrato(const juce::File& raiz)
{
    std::map<juce::String, juce::String> r;
    for (const auto& e : juce::RangedDirectoryIterator(raiz, true, "*", juce::File::findFilesAndDirectories))
        r[e.getFile().getFullPathName()] = e.isDirectory() ? juce::String("dir")
            : juce::SHA256(e.getFile()).toHexString() + "@" + juce::String(e.getFile().getLastModificationTime().toMilliseconds());
    return r;
}

// o que mudou entre dois retratos (para a falha dizer onde)
static bool igual(const std::map<juce::String, juce::String>& a, const std::map<juce::String, juce::String>& b)
{
    if (a == b)
        return true;
    for (const auto& [k, v] : a)
        if (!b.count(k) || b.at(k) != v)
            std::fprintf(stderr, "  mudou/sumiu: %s (%s → %s)\n", k.toRawUTF8(), v.toRawUTF8(), b.count(k) ? b.at(k).toRawUTF8() : "—");
    for (const auto& [k, v] : b)
        if (!a.count(k))
            std::fprintf(stderr, "  apareceu: %s\n", k.toRawUTF8());
    return false;
}

static std::map<std::string, Manifest> manifestos(const juce::File& dados)
{
    std::map<std::string, Manifest> r;
    for (const auto& f : juce::File(juce::String(referenceManifestDir(dados.getFullPathName().toStdString())))
                             .findChildFiles(juce::File::findFiles, false, "*.json"))
        if (auto m = fromJson(f.loadFileAsString().toStdString()).manifest)
            r[m->id] = *m;
    return r;
}

static const Manifest* porCaminho(const std::map<std::string, Manifest>& ms, const juce::File& f)
{
    for (const auto& [id, m] : ms)
        if (m.identity.canonicalPath == f.getFullPathName().toStdString())
            return &m;
    return nullptr;
}

static void gravar(const juce::File& dados, const Manifest& m)
{
    juce::File(juce::String(referenceManifestDir(dados.getFullPathName().toStdString())))
        .getChildFile(juce::String(m.id) + ".json").replaceWithText(juce::String::fromUTF8(toJson(m).c_str()));
}

int main()
{
    const auto base = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("trioio-importador-" + juce::String(juce::Time::currentTimeMillis()));
    const auto lib = base.getChildFile(s8("Biblioteca Música"));
    const auto dados = base.getChildFile("dados");
    const auto L = lib.getFullPathName().toStdString();
    const auto D = dados.getFullPathName().toStdString();

    const auto a = lib.getChildFile(s8("referencias/autorais/órbita.wav"));
    const auto b = lib.getChildFile("referencias/b.wav");
    const auto s = lib.getChildFile("stems/voz.wav");
    const auto mid = lib.getChildFile("referencias/tema.mid");
    escreverWav(a, 440.0);
    escreverWav(b, 550.0, 44100.0, 1);
    escreverWav(s, 660.0);
    escreverMidi(mid);
    lib.getChildFile("notas.txt").replaceWithText("ignorado");
    escreverWav(lib.getChildFile(".oculto.wav"), 100.0);
    escreverWav(lib.getChildFile("sessoes/s1/saidas/out.wav"), 200.0);
    lib.getChildFile("quebrado.wav").replaceWithText("isto não é um wav");

    // 1. primeira varredura: quatro pendentes, nada escrito na Biblioteca
    const auto antes = retrato(lib);
    auto r = scanLibrary(L, D, {}, nullptr, "2026-10-05T10:00:00Z");
    CHECK(r.errors.empty());
    CHECK(r.registered == 4);
    CHECK(r.skipped.size() == 1);  // quebrado.wav
    CHECK(igual(antes, retrato(lib)));
    auto ms = manifestos(dados);
    CHECK(ms.size() == 4);
    for (const auto& [id, m] : ms)
    {
        CHECK(validate(m).empty());
        CHECK(m.authorization.status == AuthorizationStatus::pending);
        CHECK(m.origin.category == OriginCategory::thirdPartyUnverified);
        CHECK(permits(m, Use::listen));
        CHECK(!permits(m, Use::analyze));
        CHECK(m.identity.contentSha256.size() == 64);
    }
    const auto* ma = porCaminho(ms, a);
    CHECK(ma && ma->identity.sampleRate == 48000 && ma->identity.channels == 2);
    CHECK(ma && std::abs(ma->identity.durationSeconds - 0.2) < 1e-6);
    const auto* mb = porCaminho(ms, b);
    CHECK(mb && mb->identity.sampleRate == 44100 && mb->identity.channels == 1);
    CHECK(porCaminho(ms, s) && porCaminho(ms, s)->material == MaterialType::stem);
    CHECK(porCaminho(ms, mid) && porCaminho(ms, mid)->material == MaterialType::midi);
    CHECK(porCaminho(ms, mid) && porCaminho(ms, mid)->identity.durationSeconds > 0.5);
    const std::string idA = ma ? ma->id : "";
    const std::string idB = mb ? mb->id : "";

    // 2. nova varredura sem mudança: nada regravado
    std::map<juce::String, juce::String> jsonAntes;
    for (const auto& f : juce::File(juce::String(referenceManifestDir(D))).findChildFiles(juce::File::findFiles, false, "*"))
        jsonAntes[f.getFileName()] = f.loadFileAsString();
    r = scanLibrary(L, D, {}, nullptr, "2026-10-05T10:05:00Z");
    CHECK(r.unchanged == 4 && r.registered == 0);
    for (const auto& [nome, texto] : jsonAntes)
        CHECK(juce::File(juce::String(referenceManifestDir(D))).getChildFile(nome).loadFileAsString() == texto);

    // 3. autorizar, depois mover e renomear: a autorização acompanha o conteúdo
    {
        auto m = ms[idA];
        m.origin.category = OriginCategory::authorial;
        m.authorization.status = AuthorizationStatus::authorized;
        m.authorization.basis = AuthorizationBasis::ownWork;
        m.authorization.scope = Use::listen | Use::analyze | Use::derive;
        m.authorization.declaredBy = "Performer";
        m.authorization.declaredAt = "2026-10-05T10:06:00Z";
        m.history.push_back({ "2026-10-05T10:06:00Z", "autorizado", "Performer", "" });
        gravar(dados, m);
    }
    const auto a2 = lib.getChildFile(s8("referencias/autorais/2026/órbita (final).wav"));
    a2.getParentDirectory().createDirectory();
    CHECK(a.moveFileTo(a2));
    r = scanLibrary(L, D, {}, nullptr, "2026-10-05T10:10:00Z");
    CHECK(r.relocated == 1 && r.unchanged == 3 && r.registered == 0);
    ms = manifestos(dados);
    CHECK(ms[idA].identity.canonicalPath == a2.getFullPathName().toStdString());
    CHECK(ms[idA].authorization.status == AuthorizationStatus::authorized);
    CHECK(permits(ms[idA], Use::derive));

    // 4. cópia idêntica: conta como duplicata, registra uma vez só
    const auto bCopia = lib.getChildFile(s8("referencias/b cópia.wav"));
    CHECK(b.copyFileTo(bCopia));
    r = scanLibrary(L, D, {}, nullptr, "2026-10-05T10:15:00Z");
    CHECK(r.duplicates == 1 && r.registered == 0);
    const auto eventos = manifestos(dados)[idB].history.size();
    r = scanLibrary(L, D, {}, nullptr, "2026-10-05T10:16:00Z");
    CHECK(manifestos(dados)[idB].history.size() == eventos);

    // 5. sumiu: indisponível (nem ouvir); voltou: reencontrado
    CHECK(b.deleteFile() && bCopia.deleteFile());
    r = scanLibrary(L, D, {}, nullptr, "2026-10-05T10:20:00Z");
    CHECK(r.missing == 1);
    CHECK(manifestos(dados)[idB].analysis == AnalysisState::unavailable);
    CHECK(!permits(manifestos(dados)[idB], Use::listen));
    escreverWav(b, 550.0, 44100.0, 1);
    r = scanLibrary(L, D, {}, nullptr, "2026-10-05T10:25:00Z");
    CHECK(r.reappeared == 1 && r.registered == 0);
    CHECK(manifestos(dados)[idB].analysis == AnalysisState::none);

    // 6. outro conteúdo no mesmo caminho: a autorização NÃO passa adiante
    escreverWav(a2, 880.0);
    r = scanLibrary(L, D, {}, nullptr, "2026-10-05T10:30:00Z");
    CHECK(r.replaced == 1 && r.registered == 1);
    ms = manifestos(dados);
    CHECK(ms[idA].analysis == AnalysisState::unavailable);
    CHECK(!permits(ms[idA], Use::derive));
    const auto* novo = porCaminho(ms, a2);
    CHECK(novo && novo->id != idA);
    CHECK(novo && novo->authorization.status == AuthorizationStatus::pending);
    CHECK(novo && novo->relations.size() == 1 && novo->relations[0].type == "version_of"
          && novo->relations[0].target == idA);

    // 7. raiz de dados dentro da Biblioteca: recusado, nada escrito
    {
        const auto dentro = lib.getChildFile("dados-trioio");
        const auto antes2 = retrato(lib);
        r = scanLibrary(L, dentro.getFullPathName().toStdString(), {}, nullptr, "2026-10-05T10:35:00Z");
        CHECK(!r.errors.empty() && r.registered == 0);
        CHECK(igual(antes2, retrato(lib)));
    }

    // 8. manifesto ilegível: relatado, nunca sobrescrito, o arquivo é pulado
    {
        const auto idS = porCaminho(ms, s)->id;
        const auto arq = juce::File(juce::String(referenceManifestDir(D))).getChildFile(juce::String(idS) + ".json");
        arq.replaceWithText("{ estragado");
        r = scanLibrary(L, D, {}, nullptr, "2026-10-05T10:40:00Z");
        CHECK(!r.errors.empty());
        CHECK(arq.loadFileAsString() == "{ estragado");
        bool pulou = false;
        for (const auto& p : r.skipped)
            pulou = pulou || p.find("voz.wav") != std::string::npos;
        CHECK(pulou);
    }

    // 9. cancelado antes de começar: nada gravado
    {
        const auto dados2 = base.getChildFile("dados2");
        std::atomic<bool> cancelar { true };
        r = scanLibrary(L, dados2.getFullPathName().toStdString(), {}, &cancelar, "2026-10-05T10:45:00Z");
        CHECK(r.cancelled);
        CHECK(manifestos(dados2).empty());
    }

    // 10. limite de tamanho
    {
        ImportLimits lim;
        lim.maxFileBytes = 1000;
        r = scanLibrary(L, base.getChildFile("dados3").getFullPathName().toStdString(), lim, nullptr,
                        "2026-10-05T10:50:00Z");
        CHECK(r.registered == 1);  // só o MIDI cabe
    }

    // 11. um arquivo só ("trazer gravação…"): registra só ele; a pasta vizinha
    //     não é varrida e nada de outra varredura é dado como ausente
    {
        const auto lib4 = base.getChildFile("lib4");
        const auto x1 = lib4.getChildFile("x1.wav"), x2 = lib4.getChildFile("x2.wav");
        escreverWav(x1, 410.0);
        escreverWav(x2, 420.0);
        const auto dados4 = base.getChildFile("dados4");
        const auto D4 = dados4.getFullPathName().toStdString();
        r = scanLibrary(lib4.getFullPathName().toStdString(), D4, {}, nullptr, "2026-10-05T11:00:00Z");
        CHECK(r.registered == 2);
        r = scanLibrary(x1.getFullPathName().toStdString(), D4, {}, nullptr, "2026-10-05T11:05:00Z");
        CHECK(r.errors.empty() && r.registered == 0 && r.unchanged == 1 && r.missing == 0);
        const auto novo = base.getChildFile(s8("solta/nova gravação.wav"));
        escreverWav(novo, 330.0);
        escreverWav(base.getChildFile("solta/vizinha.wav"), 990.0);
        r = scanLibrary(novo.getFullPathName().toStdString(), D4, {}, nullptr, "2026-10-05T11:10:00Z");
        CHECK(r.errors.empty() && r.registered == 1 && r.missing == 0);
        const auto m4 = manifestos(dados4);
        CHECK(m4.size() == 3);  // as 2 da Biblioteca e a nova; a vizinha não
        CHECK(porCaminho(m4, novo) != nullptr);
        for (const auto& [id, m] : m4)
            CHECK(m.analysis != AnalysisState::unavailable);
        // um arquivo que não é áudio: pulado, nada gravado
        r = scanLibrary(lib.getChildFile("notas.txt").getFullPathName().toStdString(), D4, {}, nullptr, "2026-10-05T11:15:00Z");
        CHECK(r.registered == 0 && r.skipped.size() == 1);
        // a raiz de dados pode ficar ao lado do arquivo (não é "dentro da Biblioteca")
        const auto vizinho = dados4.getChildFile("referencia-ao-lado.wav");
        escreverWav(vizinho, 220.0);
        r = scanLibrary(vizinho.getFullPathName().toStdString(), D4, {}, nullptr, "2026-10-05T11:20:00Z");
        CHECK(r.errors.empty() && r.registered == 1);
    }

    base.deleteRecursively();
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("importador: todas as checagens passaram");
    return EXIT_SUCCESS;
}
