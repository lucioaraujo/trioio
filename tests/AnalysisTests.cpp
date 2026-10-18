#include "Analysis.h"
#include "Declaration.h"
#include "LibraryImporter.h"
#include "ManifestJson.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>

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

constexpr double SR = 48000.0;

// canais iguais: o mono (média) é o próprio sinal
static void escreverWav(const juce::File& f, double seg, int canais, const std::function<float(double)>& sinal)
{
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> out = std::make_unique<juce::FileOutputStream>(f);
    auto w = wav.createWriterFor(out, juce::AudioFormatWriterOptions {}
                                          .withSampleRate(SR).withNumChannels(canais).withBitsPerSample(32)
                                          .withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
    const int n = static_cast<int>(SR * seg);
    juce::AudioBuffer<float> b(canais, n);
    for (int i = 0; i < n; ++i)
        for (int c = 0; c < canais; ++c)
            b.setSample(c, i, sinal(i / SR));
    w->writeFromAudioSampleBuffer(b, 0, n);
}

static std::string idDe(const std::string& D, const juce::File& audio)
{
    for (const auto& f : juce::File(juce::String(referenceManifestDir(D))).findChildFiles(juce::File::findFiles, false, "*.json"))
        if (auto m = fromJson(f.loadFileAsString().toStdString()).manifest)
            if (m->identity.canonicalPath == audio.getFullPathName().toStdString())
                return m->id;
    return {};
}

static void liberar(const std::string& D, const std::string& id)
{
    AuthorizationRequest q;
    q.id = id;
    q.origin = OriginCategory::authorial;
    q.basis = AuthorizationBasis::ownWork;
    q.scope = static_cast<UseSet>(Use::analyze);
    q.declaredBy = "Performer";
    (void) authorize(D, q, "2026-10-05T13:00:00Z");
}

int main()
{
    const auto base = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("trioio-analise-" + juce::String(juce::Time::currentTimeMillis()));
    const auto lib = base.getChildFile("lib");
    const auto D = base.getChildFile("dados").getFullPathName().toStdString();
    const auto L = lib.getFullPathName().toStdString();

    const auto peca = lib.getChildFile("peca.wav");  // 3 s de seno, 3 s de ruído, estéreo
    std::uint32_t x = 1;
    escreverWav(peca, 6.0, 2, [&](double t) {
        if (t < 3.0)
            return static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979 * 440.0 * t));
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        return static_cast<float>(0.3 * (x / 4294967295.0 * 2.0 - 1.0));
    });
    const auto midi = lib.getChildFile("tema.mid");
    {
        juce::MidiMessageSequence seq;
        seq.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0.0);
        seq.addEvent(juce::MidiMessage::noteOff(1, 60), 480.0);
        juce::MidiFile mf;
        mf.setTicksPerQuarterNote(480);
        mf.addTrack(seq);
        juce::FileOutputStream out(midi);
        mf.writeTo(out);
    }
    (void) scanLibrary(L, D, {}, nullptr, "2026-10-05T12:59:00Z");
    const auto id = idDe(D, peca);
    CHECK(!id.empty());
    const auto manifestoArq = juce::File(juce::String(referenceManifestDir(D))).getChildFile(juce::String(id) + ".json");
    auto ler = [&] { return *fromJson(manifestoArq.loadFileAsString().toStdString()).manifest; };
    const auto cache = juce::File(juce::String(analysisCachePath(D, ler().identity.contentSha256)));

    // 1. pendente: o portão barra, nada lido nem gravado
    {
        const auto r = analyzeReference(D, id);
        CHECK(!r.ok && !r.problems.empty());
        CHECK(!cache.exists());
    }

    // 2. liberado: analisa, grava cache, manifesto vira `cached`
    liberar(D, id);
    const auto a = analyzeReference(D, id, {}, nullptr, "2026-10-05T13:01:00Z");
    CHECK(a.ok && !a.fromCache);
    CHECK(cache.existsAsFile());
    CHECK(ler().analysis == AnalysisState::cached);
    CHECK(ler().history.back().event == "analisado");
    CHECK(std::abs(a.duration - 6.0) < 1e-9);
    const auto s0 = trioio::perception::summarize(a.frames, a.onsets, 0.5, 2.5);
    CHECK(std::abs(s0.meanRmsDb - (-9.03f)) < 0.1f);  // estéreo igual → mesmo nível do mono

    // 3. de novo: vem do cache, idêntico quadro a quadro; o manifesto não muda
    {
        const auto antes = manifestoArq.loadFileAsString();
        const auto b = analyzeReference(D, id);
        CHECK(b.ok && b.fromCache);
        CHECK(b.frames.size() == a.frames.size() && b.onsets.size() == a.onsets.size());
        bool iguais = true;
        for (std::size_t i = 0; iguais && i < a.frames.size(); ++i)
            iguais = std::abs(a.frames[i].time - b.frames[i].time) < 1e-12 && a.frames[i].rmsDb == b.frames[i].rmsDb
                  && a.frames[i].centroidHz == b.frames[i].centroidHz && a.frames[i].spreadHz == b.frames[i].spreadHz
                  && a.frames[i].flatness == b.frames[i].flatness && a.frames[i].flux == b.frames[i].flux
                  && a.frames[i].silent == b.frames[i].silent && a.frames[i].chroma == b.frames[i].chroma
                  && a.frames[i].peaksHz == b.frames[i].peaksHz && a.frames[i].f0Hz == b.frames[i].f0Hz
                  && a.frames[i].periodicity == b.frames[i].periodicity;
        for (std::size_t i = 0; iguais && i < a.onsets.size(); ++i)
            iguais = std::abs(a.onsets[i].time - b.onsets[i].time) < 1e-12 && a.onsets[i].strength == b.onsets[i].strength;
        CHECK(iguais);
        CHECK(manifestoArq.loadFileAsString() == antes);
    }

    // 4. o relatório aponta a mudança no segundo 3. Do seno de 440 Hz para o
    //    ruído branco o brilho sobe mais (≈0,3 → 1,0) que o ruído (0 → 0,56):
    //    a maior diferença é o brilho.
    {
        const auto txt = formatReport(id, a);
        std::fputs(txt.c_str(), stdout);
        CHECK(txt.find("onde muda mais:\n  0:03  fica mais brilhante") != std::string::npos);
        CHECK(txt.find("com som     100 %") != std::string::npos);  // linha inteira, sem corte
    }

    // 4b. entrar no silêncio não é "escurecer"
    {
        trioio::perception::Analyzer an(SR);
        std::vector<float> v(static_cast<std::size_t>(SR * 6.0), 0.0f);
        for (std::size_t i = 0; i < v.size() / 2; ++i)
            v[i] = static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979 * 2000.0 * static_cast<double>(i) / SR));
        an.push(v.data(), v.size());
        an.finish();
        AnalysisResult r;
        r.sampleRate = SR;
        r.duration = 6.0;
        r.frames = an.frames();
        r.onsets = an.onsets();
        const auto txt = formatReport("silêncio", r);
        CHECK(txt.find("0:03  o som se cala") != std::string::npos);
        CHECK(txt.find("escurece") == std::string::npos);
    }

    // 5. arquivo alterado sem nova varredura: análise nova recusada
    {
        cache.deleteFile();
        escreverWav(peca, 6.0, 2, [](double) { return 0.1f; });
        const auto r = analyzeReference(D, id);
        CHECK(!r.ok);
        CHECK(!cache.exists());
    }

    // 6. revogado: barrado mesmo com cache presente
    {
        (void) revoke(D, id, "Performer", "", "2026-10-05T13:05:00Z");
        cache.replaceWithText("{}");
        const auto r = analyzeReference(D, id);
        CHECK(!r.ok && !r.fromCache);
    }

    // 7. MIDI: dito com clareza, não analisado como áudio
    {
        const auto idm = idDe(D, midi);
        liberar(D, idm);
        const auto r = analyzeReference(D, idm);
        CHECK(!r.ok && !r.problems.empty() && r.problems[0].rfind("MIDI", 0) == 0);
    }

    // 8. cancelado: nada gravado
    {
        const auto outro = lib.getChildFile("outro.wav");
        escreverWav(outro, 2.0, 1, [](double t) { return static_cast<float>(0.2 * std::sin(6.0 * t)); });
        (void) scanLibrary(L, D, {}, nullptr, "2026-10-05T13:10:00Z");
        const auto ido = idDe(D, outro);
        liberar(D, ido);
        std::atomic<bool> parar { true };
        const auto r = analyzeReference(D, ido, {}, &parar);
        CHECK(!r.ok);
        auto mo = fromJson(juce::File(juce::String(referenceManifestDir(D))).getChildFile(juce::String(ido) + ".json")
                               .loadFileAsString().toStdString()).manifest;
        CHECK(mo && !juce::File(juce::String(analysisCachePath(D, mo->identity.contentSha256))).exists());
        CHECK(mo && mo->analysis != AnalysisState::cached);
    }

    // 9. vários formatos: MP3, FLAC, Ogg e Opus pelo JUCE; M4A pelo ffmpeg
    //    (no macOS, pelo CoreAudio).
    //    O ffmpeg do teste também codifica as cópias; sem ele, o trecho é pulado.
    {
        juce::ChildProcess v;
        const bool temFfmpeg = v.start("ffmpeg -version") && v.readAllProcessOutput().startsWith("ffmpeg");
        if (!temFfmpeg)
            std::puts("9. pulado: ffmpeg não encontrado");
        else
        {
            const auto lib2 = base.getChildFile("formatos");
            const auto fonte = base.getChildFile("fonte.wav");  // fora da biblioteca
            std::uint32_t y = 7;
            escreverWav(fonte, 4.0, 2, [&](double t) {
                if (t < 2.0)
                    return static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979 * 440.0 * t));
                y ^= y << 13; y ^= y >> 17; y ^= y << 5;
                return static_cast<float>(0.3 * (y / 4294967295.0 * 2.0 - 1.0));
            });
            lib2.createDirectory();
            struct Fmt { const char* ext; const char* codec; bool ffmpeg; };
            const Fmt fmts[] = { { "mp3", "libmp3lame", false }, { "flac", "flac", false }, { "ogg", "libvorbis", false },
                                 { "opus", "libopus", false },
#if JUCE_MAC
                                 { "m4a", "aac", false } };
#else
                                 { "m4a", "aac", true } };
#endif
            for (const auto& f : fmts)
            {
                juce::ChildProcess c;
                c.start(juce::StringArray { "ffmpeg", "-v", "error", "-nostdin", "-y", "-i", fonte.getFullPathName(),
                                            "-c:a", f.codec, lib2.getChildFile(juce::String("x.") + f.ext).getFullPathName() });
                c.waitForProcessToFinish(60000);
            }
            // extensões enganosas, como o gravador de celular que salva MP4/AAC
            // como ".mp3": o formato é lido pelo conteúdo
            // (codificados de novo, com outros ajustes: cópias idênticas teriam o
            // mesmo conteúdo e seriam o mesmo material na Biblioteca)
            for (const auto& a : { juce::StringArray { "-c:a", "aac", "-b:a", "96k", "-f", "mp4", "falso_aac.mp3" },
                                   juce::StringArray { "-c:a", "flac", "-compression_level", "0", "-f", "flac", "falso_flac.wav" } })
            {
                juce::StringArray cmd { "ffmpeg", "-v", "error", "-nostdin", "-y", "-i", fonte.getFullPathName() };
                for (int k = 0; k + 1 < a.size(); ++k)
                    cmd.add(a[k]);
                cmd.add(lib2.getChildFile(a[a.size() - 1]).getFullPathName());
                juce::ChildProcess c;
                c.start(cmd);
                c.waitForProcessToFinish(60000);
            }
            const auto D2 = base.getChildFile("dados2").getFullPathName().toStdString();
            const auto rep = scanLibrary(lib2.getFullPathName().toStdString(), D2, {}, nullptr, "2026-10-08T19:00:00Z");
            for (const auto& sk : rep.skipped)
                std::printf("  pulado: %s\n", sk.c_str());
            CHECK(rep.skipped.empty());
            const char* td = std::getenv("TMPDIR");
            const auto tmp = td && *td ? juce::File(juce::CharPointer_UTF8(td)) : juce::File::getSpecialLocation(juce::File::tempDirectory);
            for (const auto& f : fmts)
            {
                const auto arq = lib2.getChildFile(juce::String("x.") + f.ext);
                const auto i = idDe(D2, arq);
                CHECK(!i.empty());
                if (i.empty())
                    continue;
                liberar(D2, i);
                const auto antes = juce::SHA256(arq).toHexString();
                const auto r = analyzeReference(D2, i, {}, nullptr, "2026-10-08T19:01:00Z");
                const auto m = *fromJson(juce::File(juce::String(referenceManifestDir(D2))).getChildFile(juce::String(i) + ".json")
                                             .loadFileAsString().toStdString()).manifest;
                const auto s = trioio::perception::summarize(r.frames, r.onsets, 0.5, 1.5);
                const auto z = trioio::perception::summarize(r.frames, r.onsets, 2.5, 3.5);
                std::printf("  %-4s via %-6s · %.3f s · seno %.1f dB · ruído planura %.2f\n", f.ext,
                            f.ffmpeg ? "ffmpeg" : "JUCE", r.duration, s.meanRmsDb, z.noisiness);
                CHECK(r.ok);
                CHECK(std::abs(r.duration - 4.0) < 0.1);           // MP3/AAC acrescentam um pouco de silêncio
                CHECK(std::abs(s.meanRmsDb - (-9.03f)) < 1.0f);    // o nível sobrevive à compressão
                CHECK(z.noisiness > 0.3f);
                CHECK((m.history.back().note.find("ffmpeg") != std::string::npos) == f.ffmpeg);
                CHECK(juce::SHA256(arq).toHexString() == antes);   // o original intacto
            }
            for (const char* nome : { "falso_aac.mp3", "falso_flac.wav" })
            {
                const auto arq = lib2.getChildFile(nome);
                const auto i = idDe(D2, arq);
                CHECK(!i.empty());
                if (i.empty())
                    continue;
                liberar(D2, i);
                const auto r = analyzeReference(D2, i, {}, nullptr, "2026-10-08T19:03:00Z");
                const auto m = *fromJson(juce::File(juce::String(referenceManifestDir(D2))).getChildFile(juce::String(i) + ".json")
                                             .loadFileAsString().toStdString()).manifest;
                std::string aviso;
                for (const auto& [k, v] : m.metadataImported)
                    if (k == "aviso") aviso = v;
                std::printf("  %s · %.3f s · aviso: %s\n", nome, r.duration, aviso.c_str());
                CHECK(r.ok && std::abs(r.duration - 4.0) < 0.1);
                CHECK(aviso.find(std::string(nome) == "falso_aac.mp3" ? "MP4/AAC, não .mp3" : "FLAC, não .wav") != std::string::npos);
            }
            // o WAV temporário do ffmpeg não fica para trás
            CHECK(tmp.findChildFiles(juce::File::findFiles, false, "trioio-decodificado*").isEmpty());
#if !defined(_WIN32) && !JUCE_MAC
            // sem o ffmpeg, o M4A e o falso .mp3 são pulados com o motivo; MP3 continua lido
            setenv("TRIOIO_FFPROBE", "/nao/existe/ffprobe", 1);
            const auto D3 = base.getChildFile("dados3").getFullPathName().toStdString();
            const auto rep3 = scanLibrary(lib2.getFullPathName().toStdString(), D3, {}, nullptr, "2026-10-08T19:02:00Z");
            unsetenv("TRIOIO_FFPROBE");
            int semFfmpeg = 0;
            for (const auto& sk : rep3.skipped)
            {
                std::printf("  sem ffmpeg: %s\n", sk.c_str());
                semFfmpeg += sk.find("ffmpeg") != std::string::npos;
            }
            CHECK(semFfmpeg == 2 && !idDe(D3, lib2.getChildFile("x.mp3")).empty());
#endif
        }
    }

    base.deleteRecursively();
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("análise: todas as checagens passaram");
    return EXIT_SUCCESS;
}
