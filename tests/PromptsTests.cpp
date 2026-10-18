// O prompts.json (src/library/Prompts.h): as três camadas separadas; a
// tradução revisável; o manifesto marcado; e apagar devolve a referência a
// "sem prompt".
#include "LibraryImporter.h"
#include "ManifestJson.h"
#include "Prompts.h"
#include "Ensemble.h"

#include <juce_core/juce_core.h>

#include <cstdio>
#include <cstdlib>

using namespace trioio::library;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

int main()
{
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("trioio_prompts_" + juce::String(juce::Random().nextInt64()));
    root.createDirectory();
    const std::string dados = root.getFullPathName().toStdString();
    trioio::provenance::Manifest m;
    m.id = "ref-teste";
    const juce::File mdir(juce::String(referenceManifestDir(dados)));
    mdir.createDirectory();
    mdir.getChildFile("ref-teste.json").replaceWithText(juce::String::fromUTF8(toJson(m).c_str()));
    const auto manifesto = [&] { return fromJson(mdir.getChildFile("ref-teste.json").loadFileAsString().toStdString()); };

    std::vector<std::string> pr;
    CHECK(providePrompt(dados, "ref-teste", "dreamy lo-fi jazz, 80 bpm, female vocals", "gerador", "2026-10-18T10:00:00", pr));
    PromptRecord r;
    CHECK(loadPrompt(dados, "ref-teste", r));
    CHECK(r.hasOriginal && r.original == "dreamy lo-fi jazz, 80 bpm, female vocals" && r.source == "gerador");
    CHECK(r.declared.fields.ensemble == static_cast<int>(trioio::composition::Ensemble::jazz) && r.declared.fields.bpm == 80.0f);
    CHECK(!r.declared.revised && !r.hasInferred && !r.declared.matches.empty());
    CHECK(manifesto().manifest->promptOriginalProvided && manifesto().manifest->promptsRef == "indice/prompts/ref-teste.json");

    // a revisão muda a tradução, não o original
    CHECK(reviseIntent(dados, "ref-teste", "energia=0.3, humor=serena, estilo=tango", "2026-10-18T10:05:00", pr));
    CHECK(loadPrompt(dados, "ref-teste", r));
    CHECK(r.declared.revised && r.declared.fields.energy == 0.3f && r.declared.fields.ensemble == static_cast<int>(trioio::composition::Ensemble::tango));
    CHECK(r.original == "dreamy lo-fi jazz, 80 bpm, female vocals" && r.history.size() == 2);
    // o que não se entende é recusado, sem gravar
    std::vector<std::string> p2;
    CHECK(!reviseIntent(dados, "ref-teste", "cor=azul", "x", p2) && !p2.empty());

    // a camada ouvida é gravada à parte
    r.hasInferred = true;
    r.inferred.energy = 0.9f;
    CHECK(savePrompt(dados, r, pr));
    PromptRecord r2;
    CHECK(loadPrompt(dados, "ref-teste", r2) && r2.hasInferred && r2.inferred.energy == 0.9f && r2.declared.fields.energy == 0.3f);
    // os campos de 2026-10-18 sobrevivem à ida e volta; os não ditos ficam −1
    CHECK(r2.declared.fields.freedom < 0.0f && r2.declared.fields.narrative < 0);
    r2.declared.fields.freedom = 0.8f;
    r2.declared.fields.narrative = 3;
    r2.declared.fields.rockStyle = 2;
    CHECK(savePrompt(dados, r2, pr));
    PromptRecord r3;
    CHECK(loadPrompt(dados, "ref-teste", r3) && r3.declared.fields.freedom > 0.79f && r3.declared.fields.narrative == 3
          && r3.declared.fields.rockStyle == 2 && r3.declared.fields.dance < 0.0f);

    // apagar: sem prompt, manifesto desmarcado
    CHECK(removePrompt(dados, "ref-teste", pr));
    CHECK(!loadPrompt(dados, "ref-teste", r2) && !manifesto().manifest->promptOriginalProvided);
    root.deleteRecursively();
    for (const auto& p : pr)
        std::fprintf(stderr, "problema: %s\n", p.c_str());
    if (falhas || !pr.empty())
        return EXIT_FAILURE;
    std::puts("prompts: todas as checagens passaram");
    return EXIT_SUCCESS;
}
