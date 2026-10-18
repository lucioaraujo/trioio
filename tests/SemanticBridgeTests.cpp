// A ponte semântica (src/library/SemanticBridge.h) sem rede: a lista de
// fontes, os formatos das respostas e a memória (o que já foi consultado
// volta sem rede, e sem fontes nada é inventado).
#include "SemanticBridge.h"

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
    // a lista: comentários, pesos, línguas; erros por linha
    {
        std::vector<std::string> errors;
        const auto s = parseSemanticSources("# comentário\n"
                                            "traducao wikidata-lexema https://www.wikidata.org/w/api.php linguas=pt,fr\n"
                                            "associacao datamuse https://api.datamuse.com/words?rel_jjb={en} 0.5\n"
                                            "associacao inventado https://exemplo.org/{en}\n"
                                            "associacao datamuse http://sem-tls.org/{en}\n"
                                            "traducao\n",
                                            errors);
        CHECK(s.size() == 2 && errors.size() == 3);
        CHECK(s[0].languages.size() == 2 && s[0].languages[1] == "fr");
        CHECK(s[1].weight > 0.49 && s[1].weight < 0.51);
        // a lista do repositório lê sem erro
        std::vector<std::string> e2;
        const auto repo = parseSemanticSources(
            juce::File(TRIOIO_SOURCE_DIR "/data/semantica/fontes.txt").loadFileAsString().toStdString(), e2);
        CHECK(e2.empty() && repo.size() >= 2);
    }
    // os formatos
    {
        const auto d = parseAssociationResponse("datamuse", R"([{"word":"Gentle","score":2000},{"word":"cold","score":500}])");
        CHECK(d.size() == 2 && d[0].first == "gentle" && d[0].second > 0.99 && d[1].second < 0.3);
        const auto c = parseAssociationResponse(
            "conceptnet", R"({"related":[{"@id":"/c/en/heavy_rain","weight":0.8},{"@id":"/c/en/drizzle","weight":0.6}]})");
        CHECK(c.size() == 2 && c[0].first == "heavy rain" && c[1].second > 0.59);
        CHECK(parseAssociationResponse("datamuse", "<html>502</html>").empty());
    }
    // a memória: o que foi consultado volta sem rede; sem fontes, nada se inventa
    {
        const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("trioio_semantica_" + juce::String(juce::Random().nextInt64()));
        root.getChildFile("memoria").createDirectory();
        (void) root.getChildFile("memoria").getChildFile("semantica.json").replaceWithText(
            R"({"chuva":{"english":"rain","via":"Wikidata L648257 (pt) → L3325","associations":[["gentle",1.0],["steady",0.7]]}})");
        const std::string dados = root.getFullPathName().toStdString();
        const auto c = cachedSemantic(dados, "  Chuva ");
        CHECK(c.ok && c.cached && c.english == "rain" && c.associations.size() == 2);
        CHECK(!cachedSemantic(dados, "saudade").ok);
        const auto r = lookupSemantic(dados, { "chuva", "saudade" }, {}, 500);
        CHECK(r.size() == 1 && r[0].word == "chuva" && r[0].cached);
        // a palavra que as fontes não conhecem: lembrada, não se pergunta de novo
        (void) root.getChildFile("memoria").getChildFile("semantica.json").replaceWithText(
            "{\"xyzqw\":{\"unknown\":true,\"when\":\"" + juce::Time::getCurrentTime().toISO8601(true) + "\"}}");
        std::vector<std::string> e3;
        const auto fake = parseSemanticSources("associacao datamuse https://invalid.invalid/{en}\n", e3);
        const auto u = lookupSemantic(dados, { "xyzqw" }, fake, 500);
        CHECK(u.size() == 1 && !u[0].ok && u[0].cached && u[0].error.find("não conhecem") != std::string::npos);
        root.deleteRecursively();
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("ponte semântica: todas as checagens passaram");
    return EXIT_SUCCESS;
}
