#include "Declaration.h"
#include "LibraryImporter.h"
#include "ManifestJson.h"

#include <juce_audio_formats/juce_audio_formats.h>

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

static void escreverWav(const juce::File& f, double freq)
{
    f.getParentDirectory().createDirectory();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> out = std::make_unique<juce::FileOutputStream>(f);
    auto w = wav.createWriterFor(out, juce::AudioFormatWriterOptions {}
                                          .withSampleRate(48000.0).withNumChannels(1).withBitsPerSample(16));
    juce::AudioBuffer<float> b(1, 4800);
    for (int i = 0; i < 4800; ++i)
        b.setSample(0, i, 0.2f * static_cast<float>(std::sin(2.0 * 3.14159265358979 * freq * i / 48000.0)));
    w->writeFromAudioSampleBuffer(b, 0, 4800);
}

static bool problemaEm(const DeclarationResult& r, const std::string& campo)
{
    for (const auto& p : r.problems)
        if (p.rfind(campo, 0) == 0)
            return true;
    return false;
}

int main()
{
    const auto base = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("trioio-declaracao-" + juce::String(juce::Time::currentTimeMillis()));
    const auto lib = base.getChildFile("lib");
    const auto D = base.getChildFile("dados").getFullPathName().toStdString();
    escreverWav(lib.getChildFile("a.wav"), 300.0);
    escreverWav(lib.getChildFile("b.wav"), 400.0);
    (void) scanLibrary(lib.getFullPathName().toStdString(), D, {}, nullptr, "2026-10-05T12:00:00Z");

    const auto dir = juce::File(juce::String(referenceManifestDir(D)));
    auto arquivos = dir.findChildFiles(juce::File::findFiles, false, "*.json");
    arquivos.sort();
    CHECK(arquivos.size() == 2);
    const auto idA = arquivos[0].getFileNameWithoutExtension().toStdString();
    const auto idB = arquivos[1].getFileNameWithoutExtension().toStdString();
    const auto ler = [&](const std::string& id) {
        return *fromJson(dir.getChildFile(juce::String(id) + ".json").loadFileAsString().toStdString()).manifest;
    };
    const auto texto = [&](const std::string& id) {
        return dir.getChildFile(juce::String(id) + ".json").loadFileAsString();
    };

    // 1. declaração incompleta: nada gravado, lista do que falta
    {
        const auto antes = texto(idA);
        AuthorizationRequest q;
        q.id = idA;
        q.basis = AuthorizationBasis::ownWork;
        q.scope = static_cast<UseSet>(Use::derive);
        q.declaredBy = "Performer";
        const auto r = authorize(D, q, "2026-10-05T12:01:00Z");  // origem ainda não verificada
        CHECK(!r.written);
        CHECK(problemaEm(r, "origin.category"));
        CHECK(texto(idA) == antes);
    }

    // 2. obra própria, liberada para derivar: o escopo fecha pela cadeia
    AuthorizationRequest ok;
    ok.id = idA;
    ok.origin = OriginCategory::authorial;
    ok.basis = AuthorizationBasis::ownWork;
    ok.scope = static_cast<UseSet>(Use::derive);
    ok.declaredBy = "Performer";
    {
        const auto r = authorize(D, ok, "2026-10-05T12:02:00Z");
        CHECK(r.written && r.problems.empty());
        const auto m = ler(idA);
        CHECK(m.authorization.status == AuthorizationStatus::authorized);
        CHECK(has(m.authorization.scope, Use::listen) && has(m.authorization.scope, Use::analyze));
        CHECK(permits(m, Use::derive));
        CHECK(!permits(m, Use::publishDerivative));
        CHECK(m.history.back().event == "autorizado");
    }

    // 3. revogar: precisa de quem; depois não ouve mais
    CHECK(problemaEm(revoke(D, idA, "", "", "2026-10-05T12:03:00Z"), "declaredBy"));
    {
        const auto r = revoke(D, idA, "Performer", "mudei de ideia", "2026-10-05T12:04:00Z");
        CHECK(r.written);
        const auto m = ler(idA);
        CHECK(m.authorization.status == AuthorizationStatus::revoked);
        CHECK(!permits(m, Use::listen));
        CHECK(m.history.back().event == "revogado" && m.history.back().note == "mudei de ideia");
    }

    // 4. recusar só o que está pendente
    CHECK(problemaEm(refuse(D, idA, "Performer", "", "2026-10-05T12:05:00Z"), "authorization.status"));

    // 5. voltar a autorizar fica registrado como redeclaração
    {
        const auto r = authorize(D, ok, "2026-10-05T12:06:00Z");
        CHECK(r.written);
        CHECK(ler(idA).history.back().event == "autorização redeclarada");
    }

    // 6. gerado por IA sem evidência: recusado pela validação, disco intacto
    {
        const auto antes = texto(idA);
        AuthorizationRequest ia = ok;
        ia.origin = OriginCategory::aiGenerated;
        ia.basis = AuthorizationBasis::serviceTerms;
        ia.toolName = "serviço X";
        const auto r = authorize(D, ia, "2026-10-05T12:07:00Z");
        CHECK(!r.written);
        CHECK(problemaEm(r, "authorization.evidence"));
        CHECK(texto(idA) == antes);
        ia.evidence = "termos de 2026-09-30, docs-locais/termos-x.pdf";
        CHECK(authorize(D, ia, "2026-10-05T12:08:00Z").written);
        CHECK(ler(idA).origin.toolName == "serviço X");
    }

    // 7. id desconhecido
    {
        AuthorizationRequest q = ok;
        q.id = "ref-0000000000000000";
        const auto r = authorize(D, q, "2026-10-05T12:09:00Z");
        CHECK(!r.written && !r.problems.empty());
    }

    // 8. pendente recusado, depois autorizado
    {
        CHECK(refuse(D, idB, "Performer", "não é meu", "2026-10-05T12:10:00Z").written);
        CHECK(ler(idB).authorization.status == AuthorizationStatus::refused);
        CHECK(!permits(ler(idB), Use::listen));
        AuthorizationRequest q = ok;
        q.id = idB;
        q.origin = OriginCategory::licensed;
        q.basis = AuthorizationBasis::license;
        q.license = "CC-BY-4.0";
        q.evidence = "https://exemplo.org/b";
        CHECK(authorize(D, q, "2026-10-05T12:11:00Z").written);
        CHECK(ler(idB).history.back().event == "autorização redeclarada");
    }

    // 9. nomes do JSON para a linha de comando
    {
        OriginCategory o {};
        AuthorizationBasis b {};
        Use u {};
        CHECK(parseOrigin("aiGenerated", o) && o == OriginCategory::aiGenerated);
        CHECK(parseBasis("serviceTerms", b) && b == AuthorizationBasis::serviceTerms);
        CHECK(parseUse("publishDerivative", u) && u == Use::publishDerivative);
        CHECK(!parseUse("tudo", u));
    }

    base.deleteRecursively();
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("declaração: todas as checagens passaram");
    return EXIT_SUCCESS;
}
