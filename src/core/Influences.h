#pragma once

// A árvore de influências (docs/INFLUENCIAS.md; data/influencias/). Não é
// um catálogo de gêneros: é uma árvore ABERTA (classes, subclasses, maneiras
// de fazer…), de onde cada resposta tira parâmetros, cruza galhos distantes
// e muta, para que a música não se repita nem se feche num rótulo — "nos
// nutrimos com informações para criarmos algo novo" (o autor, 2026-10-18).
//
// - Os traços são TENDÊNCIAS, nunca regras: "presença:percussão 0.85" é a
//   chance de a percussão estar; nos eixos (dança, peso, ruído, rasgo,
//   repetição), o valor de 0 a 1.
// - O CONCEITO de cada nó vem primeiro ("os instrumentos rasgo precisam dos
//   conceitos"); os filhos herdam os traços dos pais.
// - A NOVIDADE: a polinização (um galho distante, com peso menor), a
//   mutação (um traço de um terceiro galho) e a memória do que já saiu (o
//   muito usado fica raro, o "mercado" do Studio reescrito).
// - A REPETIÇÃO, quando o conceito a pede ("se a repetição for necessária
//   para exprimir uma música que requer a repetição"): o eixo repetição
//   alto faz a memória ceder e a polinização e a mutação diminuírem.

#include "Composer.h"
#include "Dramaturgy.h"

#include <map>
#include <string>
#include <vector>

namespace trioio::composition
{

struct InfluenceTrait
{
    std::string key;  // "presença:percussão", "conjunto:rock", "eixo:dança"…
    float value = 0.0f;
};

struct InfluenceNode
{
    std::string path;     // "rock/pesado/lento"
    std::string conceito, context, source;  // (conceito: "concept" é palavra do C++20)
    std::vector<std::string> aliases;       // "também:" os outros nomes ("coco de roda", "samba de coco")
    std::vector<InfluenceTrait> traits;
};

// as palavras do texto que um nome de nó com mais de uma palavra consumiu
// ("coco de roda" → coco, de, roda): ficam fora da rede e da leitura avulsa
[[nodiscard]] std::vector<std::string> influenceAliasWords(const std::vector<InfluenceNode>& archive, const std::string& text);

[[nodiscard]] std::vector<InfluenceNode> parseInfluenceArchive(const std::string& text, std::vector<std::string>& errors);

// o nó pelo caminho inteiro ou pelo fim dele ("pesado" acha "rock/pesado");
// nullptr se nenhum
[[nodiscard]] const InfluenceNode* findInfluence(const std::vector<InfluenceNode>& archive, const std::string& name);

// os traços de um nó com os dos ancestrais (o filho muda o que herda)
[[nodiscard]] std::vector<InfluenceTrait> inheritedTraits(const std::vector<InfluenceNode>& archive, const InfluenceNode& node);

struct InfluenceDraw
{
    std::vector<std::string> nodes;      // os nós que entraram (pedidos e polinizados)
    std::map<std::string, float> axes;   // dança, peso, ruído, rasgo, repetição
    std::string ensemble, rockStyle, harmony, mood;  // os nomes escolhidos ("" = nenhum)
    std::string scale, cycle;
    std::string narrative;  // a gramática narrativa (Dramaturgy.h)
    // o galho distante para a virada (o polinizado, ou o segundo pedido):
    // o nó, os seus eixos e os seus timbres (a família de maior tendência por papel)
    std::string twistNode;
    std::map<std::string, float> twistAxes;
    std::map<std::string, std::string> twistTimbres;
    std::map<std::string, bool> presence;  // papel → presente
    std::map<std::string, std::string> timbres;  // papel → família de timbre ("motivo" → "sax")
    std::vector<std::string> used;       // as chaves sorteadas (para a memória)
    std::vector<std::string> lines;      // a forma: o que entrou e por quê
};

// Sorteia as influências de uma resposta: `requested` (caminhos ou fins de
// caminho; vazio = um nó escolhido pela raridade), a semente, a
// imprevisibilidade e a memória de uso (chave → vezes).
// `strength` (0..1, a "força das influências", o análogo do Style Influence
// do Suno e do guidance dos modelos): 1 = as tendências do arquivo inteiras;
// menos = as escolhas acontecem menos e os eixos voltam ao meio (a escuta da
// referência pesa mais). `exclude`: os papéis que o autor tirou ("percussão",
// "baixo"…), o análogo do Exclude: a exclusão é dele, não uma tendência.
[[nodiscard]] InfluenceDraw drawInfluences(const std::vector<InfluenceNode>& archive, const std::vector<std::string>& requested,
                                           std::uint64_t seed, float unpredictability, const std::map<std::string, int>& usage,
                                           float strength = 1.0f, const std::vector<std::string>& exclude = {});

// O mapa (a busca pelo não ocupado; J. Lehman e K. O. Stanley, "Abandoning
// objectives: Evolution through the search for novelty alone", Evolutionary
// Computation 19(2), 2011): cada resposta com influências é um ponto (os
// eixos dança, peso, ruído e rasgo; o conjunto e a harmonia escolhidos).
// Entre os sorteios candidatos, fica o mais longe do que já foi feito (a
// distância ao vizinho mais perto); se o conceito pede a repetição
// (repetição ≥ 0,6), a busca cede e fica o primeiro. Devolve o índice e
// preenche `distance`.
struct InfluencePoint
{
    float dance = 0.5f, weight = 0.5f, noise = 0.5f, rasgo = 0.5f;
    std::string ensemble, harmony;
};
[[nodiscard]] InfluencePoint influencePoint(const InfluenceDraw& d);
[[nodiscard]] double influenceDistance(const InfluencePoint& a, const InfluencePoint& b);
[[nodiscard]] std::size_t pickNovelDraw(const std::vector<InfluenceDraw>& candidates, const std::vector<InfluencePoint>& past,
                                        double* distance = nullptr);

// As palavras do autor que nomeiam nós da árvore ("maracatu", "post-rock",
// "minimalismo", "jazz/livre"): cada palavra que é um segmento de um caminho
// (sem distinguir maiúsculas) dá o nó mais raso que a tem. Devolve os
// caminhos, sem repetir.
[[nodiscard]] std::vector<std::string> influencesFromWords(const std::vector<InfluenceNode>& archive, const std::string& text);

// Antes de compor: o que o autor não escolheu vem do sorteio (o conjunto, o
// subestilo, a harmonia, o humor, a escala, o ciclo; o rasgo e o ruído
// puxam o rasgo modular). Devolve as linhas.
std::vector<std::string> applyInfluenceOptions(Options& opt, const InfluenceDraw& draw);

// Depois das figuras do estilo: as presenças (uma voz pode não estar: a
// tendência, não a regra; o motivo sempre fica) e os eixos de caráter, que
// valem para qualquer conjunto:
//   dança    — acima de 0,5, a percussão mais acentuada e o baixo e a
//              harmonia mais curtos (o groove); abaixo, a percussão rareia e
//              as notas se alongam (o introspectivo);
//   peso     — a força, e o baixo e a harmonia uma oitava abaixo (pesado) ou
//              acima (leve);
//   ruído    — (pelo rasgo modular, em applyInfluenceOptions).
// Com `curve` (a forma da peça: a gramática, o gesto ou a curva do modo), os
// eixos dança, peso e densidade SEGUEM a forma: sobem no clímax e descem
// nos vales (±0,2 em torno do valor da influência).
std::string applyCharacter(Score& score, const std::map<std::string, float>& axes, std::uint64_t seed,
                           const TensionCurve* curve = nullptr);
// Com a gramática ki-shō-ten-ketsu (`narrative`), a VIRADA (50 a 75 % da
// peça) traz o galho distante: os timbres e os eixos do nó polinizado (ou do
// segundo pedido) valem só nesse trecho.
std::string applyInfluenceScore(Score& score, const InfluenceDraw& draw, std::uint64_t seed, const TensionCurve* curve = nullptr,
                                const std::string& narrative = {});

} // namespace trioio::composition
