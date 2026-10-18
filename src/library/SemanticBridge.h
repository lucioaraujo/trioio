#pragma once

// A ponte semântica (docs/PROMPTS_E_INTENCOES.md, "A ponte semântica pela
// rede"): a palavra que o léxico do Trioio não leu é consultada em serviços
// PÚBLICOS na internet, e o Trioio calcula a partir das respostas. Nada de
// léxico grande embutido no app (o autor, 2026-10-18: "não gostaria de
// deixar o app muito pesado com um recurso de texto"; "o app pode ter uma
// lista de sites que prestam esse serviço ... busca e retorno rápido, sem
// ocupar muito tempo do trabalho de composição ... ágil, prático e não
// tornar o uso do app monolítico"):
//
//   - a LISTA de serviços é um arquivo de texto editável
//     (data/semantica/fontes.txt): o autor tira, troca ou acrescenta fontes
//     sem recompilar; nenhum serviço é obrigatório;
//   - os serviços de um papel são consultados em PARALELO, dentro de UM
//     tempo limite para tudo (padrão 4 s; a primeira consulta de uma palavra leva 1–3 s); o que não respondeu a tempo
//     fica de fora e a resposta segue;
//   - o que já foi consultado fica em <dados>/memoria/semantica.json: a
//     segunda vez é instantânea e sem rede;
//   - o app consulta ENQUANTO o autor escreve (em segundo plano): quando ele
//     pede a resposta, as palavras normalmente já estão na memória.
//
// O cálculo é do Trioio (core/Intent.h, intentFromAssociations): os
// serviços só dão palavras (a tradução, as associações); quem decide os
// campos é o léxico. Opcional e desligada por padrão (as palavras vão a um
// serviço externo). Nunca na thread de áudio.

#include <string>
#include <utility>
#include <vector>

namespace trioio::library
{

struct SemanticSource
{
    std::string role;    // traducao | associacao
    std::string format;  // wikidata-lexema | datamuse | conceptnet
    std::string url;     // o modelo, com {palavra} {en} {lingua}
    std::vector<std::string> languages;  // wikidata-lexema
    double weight = 1.0;
};

// lê fontes.txt (as linhas com erro vão para `errors`)
[[nodiscard]] std::vector<SemanticSource> parseSemanticSources(const std::string& text, std::vector<std::string>& errors);
// a lista em uso: data/semantica/fontes.txt (TRIOIO_DATA_DIR, junto do app…)
[[nodiscard]] std::vector<SemanticSource> defaultSemanticSources();

struct SemanticLookup
{
    std::string word;      // a palavra do autor
    std::string english;   // o conceito em inglês
    std::string via;       // de onde veio (a memória, o lexema do Wikidata, as fontes)
    std::vector<std::pair<std::string, double>> associations;  // (palavra, peso 0..1)
    bool ok = false;
    bool cached = false;
    int millis = 0;
    std::string error;
};

// Só a memória (instantâneo, sem rede): ok = false se a palavra não foi consultada.
[[nodiscard]] SemanticLookup cachedSemantic(const std::string& dataRoot, const std::string& word);

// Consulta as palavras (em paralelo, as que não estão na memória) dentro de
// `budgetMs` para TUDO; guarda as respostas na memória.
[[nodiscard]] std::vector<SemanticLookup> lookupSemantic(const std::string& dataRoot, const std::vector<std::string>& words,
                                                         const std::vector<SemanticSource>& sources, int budgetMs = 4000);

// Os formatos (expostos para os testes, que não usam a rede)
[[nodiscard]] std::vector<std::pair<std::string, double>> parseAssociationResponse(const std::string& format, const std::string& body);

} // namespace trioio::library
