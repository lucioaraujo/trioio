#include "Declaration.h"
#include "LibraryImporter.h"
#include "ManifestJson.h"
#include "Response.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>

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

// referência com quatro caracteres alternados a cada 2 s (como em ChainTests)
static void escreverReferencia(const juce::File& f)
{
    constexpr double SR = 48000.0;
    f.getParentDirectory().createDirectory();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> out = std::make_unique<juce::FileOutputStream>(f);
    auto w = wav.createWriterFor(out, juce::AudioFormatWriterOptions {}.withSampleRate(SR).withNumChannels(1)
                                          .withBitsPerSample(32)
                                          .withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
    const int n = static_cast<int>(SR * 16.0);
    juce::AudioBuffer<float> b(1, n);
    std::uint32_t x = 9;
    for (int i = 0; i < n; ++i)
    {
        const double t = i / SR;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        const double r = x / 4294967295.0 * 2.0 - 1.0;
        double s = 0.0;
        switch (static_cast<int>(t / 2.0) % 4)
        {
            case 0: s = 0.02 * std::sin(2.0 * 3.14159265358979 * 150.0 * t); break;
            case 1: s = 0.6 * r * (std::fmod(t, 0.125) < 0.02 ? 1.0 : 0.05); break;
            case 2: s = 0.15 * std::sin(2.0 * 3.14159265358979 * 600.0 * t) * (std::fmod(t, 0.5) < 0.1 ? 1.0 : 0.3); break;
            case 3: s = 0.4 * r * (std::fmod(t, 0.25) < 0.02 ? 1.0 : 0.1) + 0.1 * std::sin(2.0 * 3.14159265358979 * 2000.0 * t); break;
        }
        b.setSample(0, i, static_cast<float>(s));
    }
    w->writeFromAudioSampleBuffer(b, 0, n);
}

static UseSet usos(std::initializer_list<Use> l)
{
    UseSet s = 0;
    for (auto u : l)
        s = s | u;
    return s;
}

int main()
{
    const auto base = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("trioio-resposta-" + juce::String(juce::Time::currentTimeMillis()));
    const auto lib = base.getChildFile("lib");
    const auto D = base.getChildFile("dados").getFullPathName().toStdString();
    escreverReferencia(lib.getChildFile("ref.wav"));
    (void) scanLibrary(lib.getFullPathName().toStdString(), D, {}, nullptr, "2026-10-05T14:00:00Z");
    const auto id = juce::File(juce::String(referenceManifestDir(D))).findChildFiles(juce::File::findFiles, false, "*.json")[0]
                        .getFileNameWithoutExtension().toStdString();
    const auto autorizar = [&](UseSet u) {
        AuthorizationRequest q;
        q.id = id;
        q.origin = OriginCategory::authorial;
        q.basis = AuthorizationBasis::ownWork;
        q.scope = u;
        q.declaredBy = "Performer";
        CHECK(authorize(D, q, "2026-10-05T14:01:00Z").written);
    };
    const auto antesLib = [&] {
        juce::StringArray r;
        for (const auto& e : juce::RangedDirectoryIterator(lib, true, "*", juce::File::findFilesAndDirectories))
            r.add(e.getFile().getFullPathName() + "@" + juce::String(e.getFileSize()));
        return r.joinIntoString("|");
    }();

    // 1. só `analyze`: responder é derivar, e derivar não foi autorizado
    autorizar(usos({ Use::analyze }));
    ResponseRequest q;
    q.id = id;
    q.mode = trioio::strategy::Mode::follow;
    q.seed = 7;
    q.score = false;  // §1–5: o motor de energia (a primeira cadeia); a partitura em §6
    {
        const auto r = respond(D, q, "2026-10-05T14:02:00Z");
        CHECK(!r.ok && !r.problems.empty());
    }

    // 2. com `derive`: seguir correlaciona, contrapor anticorrelaciona
    autorizar(usos({ Use::derive }));
    const auto seguir = respond(D, q, "2026-10-05T14:03:00Z");
    for (const auto& p : seguir.problems)
        std::fprintf(stderr, "  %s\n", p.c_str());
    CHECK(seguir.ok);
    std::printf("seguir:    energia %+.2f  densidade %+.2f  brilho %+.2f\n", seguir.energyCorrelation,
                seguir.densityCorrelation, seguir.brightnessCorrelation);
    CHECK(seguir.energyCorrelation > 0.6f && seguir.brightnessCorrelation > 0.6f && seguir.densityCorrelation > 0.3f);
    q.mode = trioio::strategy::Mode::counter;
    const auto contra = respond(D, q, "2026-10-05T14:04:00Z");
    std::printf("contrapor: energia %+.2f  densidade %+.2f  brilho %+.2f\n", contra.energyCorrelation,
                contra.densityCorrelation, contra.brightnessCorrelation);
    CHECK(contra.ok);
    CHECK(contra.energyCorrelation < -0.6f && contra.brightnessCorrelation < -0.6f && contra.densityCorrelation < -0.3f);

    // 3. o que foi gravado: WAV estéreo de mesma duração, decisões, manifesto válido
    {
        const juce::File wav(juce::String::fromUTF8(seguir.wavPath.c_str()));
        CHECK(wav.existsAsFile());
        CHECK(wav.isAChildOf(juce::File(juce::String::fromUTF8(D.c_str()))));
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd(fm.createReaderFor(wav));
        CHECK(rd && rd->numChannels == 2 && std::abs(rd->lengthInSamples / rd->sampleRate - 16.0) < 0.01);

        const auto dec = juce::JSON::parse(juce::File(juce::String::fromUTF8(seguir.decisionsPath.c_str())));
        CHECK(dec["schemaVersion"].toString() == "trioio.decisoes/1");
        CHECK(dec["steps"].size() == 32);
        CHECK(dec["referenceId"].toString() == juce::String(id));

        const auto dm = juce::File(juce::String(derivationManifestDir(D))).getChildFile(juce::String(seguir.derivationId) + ".json");
        const auto m = fromJson(dm.loadFileAsString().toStdString()).manifest;
        CHECK(m.has_value());
        if (m)
        {
            CHECK(validate(*m).empty());
            CHECK(m->kind == ManifestKind::derivation && m->derivation->from[0] == id);
            CHECK(m->derivation->strategy == Strategy::follow && m->derivation->seed == 7);
            CHECK(m->authorization.status == AuthorizationStatus::pending);
            // publicar exige publishDerivative na referência
            const auto refM = *fromJson(juce::File(juce::String(referenceManifestDir(D))).getChildFile(juce::String(id) + ".json")
                                           .loadFileAsString().toStdString()).manifest;
            CHECK(!canPublishDerivation(*m, { refM }));
            autorizar(usos({ Use::publishDerivative }));
            const auto refP = *fromJson(juce::File(juce::String(referenceManifestDir(D))).getChildFile(juce::String(id) + ".json")
                                           .loadFileAsString().toStdString()).manifest;
            CHECK(canPublishDerivation(*m, { refP }));
        }
    }

    // 4. determinismo: mesmo modo e seed → mesmo áudio (mesmo id de derivação);
    //    outro seed → outro áudio
    {
        q.mode = trioio::strategy::Mode::follow;
        const auto de_novo = respond(D, q, "2026-10-05T14:05:00Z");
        CHECK(de_novo.ok && de_novo.derivationId == seguir.derivationId);
        q.seed = 8;
        const auto outro = respond(D, q, "2026-10-05T14:06:00Z");
        CHECK(outro.ok && outro.derivationId != seguir.derivationId);
    }

    // 6. motor partitura: WAV + MIDI + forma; determinístico
    {
        ResponseRequest ps;
        ps.id = id;
        ps.mode = trioio::strategy::Mode::counter;
        ps.seed = 3;
        const auto r = respond(D, ps, "2026-10-05T14:07:00Z");
        for (const auto& p : r.problems)
            std::fprintf(stderr, "  %s\n", p.c_str());
        CHECK(r.ok);
        CHECK(!r.formMarks.empty());
        const juce::File mid(juce::String::fromUTF8(r.midiPath.c_str()));
        CHECK(mid.existsAsFile());
        juce::FileInputStream in(mid);
        juce::MidiFile mf;
        CHECK(in.openedOk() && mf.readFrom(in));
        int notas = 0;
        for (int t = 0; t < mf.getNumTracks(); ++t)
            for (const auto* ev : *mf.getTrack(t))
                notas += ev->message.isNoteOn();
        const auto dec = juce::JSON::parse(juce::File(juce::String::fromUTF8(r.decisionsPath.c_str())));
        std::printf("partitura: %d notas no MIDI, %d na partitura\n", notas, dec["notes"].size());
        CHECK(notas > 10 && notas == dec["notes"].size());
        CHECK(dec["engine"].toString() == "partitura");
        const auto de_novo = respond(D, ps, "2026-10-05T14:08:00Z");
        CHECK(de_novo.ok && de_novo.derivationId == r.derivationId);
        ps.pitch = trioio::composition::PitchSource::modal;
        ps.time = trioio::composition::TimeMode::free;
        const auto alt = respond(D, ps, "2026-10-05T14:09:00Z");
        CHECK(alt.ok && alt.derivationId != r.derivationId);
    }

    // 7. linhas por região de frequência (docs/LINHAS.md): um stem por
    //    linha, a soma dos stems é a resposta, cada nota sabe a sua linha,
    //    o manifesto diz os cortes; determinístico
    {
        ResponseRequest pl;
        pl.id = id;
        pl.mode = trioio::strategy::Mode::follow;
        pl.seed = 5;
        pl.lineCuts = { 300.0, 1500.0 };
        pl.lineModes = { trioio::strategy::Mode::follow, trioio::strategy::Mode::transfigure };  // a terceira: `mode`
        const auto r = respond(D, pl, "2026-10-05T14:10:00Z");
        for (const auto& p : r.problems)
            std::fprintf(stderr, "  %s\n", p.c_str());
        CHECK(r.ok);
        CHECK(r.stemPaths.size() == 3);
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> mix(fm.createReaderFor(juce::File(juce::String::fromUTF8(r.wavPath.c_str()))));
        CHECK(mix != nullptr);
        if (mix && r.stemPaths.size() == 3)
        {
            const int n = static_cast<int>(mix->lengthInSamples);
            juce::AudioBuffer<float> m(2, n), sum(2, n), b(2, n);
            mix->read(&m, 0, n, 0, true, true);
            sum.clear();
            for (const auto& st : r.stemPaths)
            {
                std::unique_ptr<juce::AudioFormatReader> rd(fm.createReaderFor(juce::File(juce::String::fromUTF8(st.c_str()))));
                CHECK(rd != nullptr && rd->lengthInSamples == mix->lengthInSamples);
                if (!rd)
                    continue;
                rd->read(&b, 0, n, 0, true, true);
                for (int c = 0; c < 2; ++c)
                    sum.addFrom(c, 0, b, c, 0, n);
            }
            float err = 0.0f;
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < n; ++i)
                    err = std::max(err, std::abs(sum.getSample(c, i) - m.getSample(c, i)));
            std::printf("linhas: soma dos stems − resposta = %.2g · pico %.2f\n", static_cast<double>(err),
                        static_cast<double>(m.getMagnitude(0, n)));
            CHECK(err < 1e-5f);                     // 24 bits: a soma é a resposta
            CHECK(m.getMagnitude(0, n) <= 0.892f);  // o teto de −1 dBFS
        }
        const auto dec = juce::JSON::parse(juce::File(juce::String::fromUTF8(r.decisionsPath.c_str())));
        CHECK(dec["lines"].size() == 3);
        CHECK(dec["lines"][1]["mode"].toString() == "transfigurar");
        CHECK(dec["lines"][2]["mode"].toString() == "seguir");
        CHECK(dec["lines"][0]["pulse"].toString() == juce::String::fromUTF8("peça"));
        int semLinha = 0;
        for (int i = 0; i < dec["notes"].size(); ++i)
            semLinha += !dec["notes"][i].hasProperty("line");
        CHECK(semLinha == 0 && dec["notes"].size() > 10);
        const auto man = fromJson(juce::File(juce::String(derivationManifestDir(D))).getChildFile(juce::String(r.derivationId) + ".json")
                                      .loadFileAsString().toStdString());
        CHECK(man.manifest && man.manifest->derivation
              && man.manifest->derivation->declaredRelation.find("3 linhas por região de frequência") != std::string::npos);
        const auto de_novo = respond(D, pl, "2026-10-05T14:11:00Z");
        CHECK(de_novo.ok && de_novo.derivationId == r.derivationId);
        pl.lineCuts = { 10.0 };  // nenhum corte válido
        CHECK(!respond(D, pl, "2026-10-05T14:12:00Z").ok);
    }

    // 5. a Biblioteca não foi tocada
    {
        juce::StringArray r;
        for (const auto& e : juce::RangedDirectoryIterator(lib, true, "*", juce::File::findFilesAndDirectories))
            r.add(e.getFile().getFullPathName() + "@" + juce::String(e.getFileSize()));
        CHECK(r.joinIntoString("|") == antesLib);
    }

    base.deleteRecursively();
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("resposta: todas as checagens passaram");
    return EXIT_SUCCESS;
}
