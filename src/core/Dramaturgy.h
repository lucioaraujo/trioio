#pragma once

// A dramaturgia da peça (docs/AUDITORIA_2026-10-14.md §6, item 3;
// docs/COMPOSICAO.md "Dramaturgia"): uma curva de tensão para a peça inteira
// que molda a resposta depois de composta — a dinâmica, a densidade das vozes
// de enfeite e um silêncio composto antes do clímax. Antes, a forma vinha só
// das seções da referência e cada seção recebia uma operação: havia
// variedade, mas pouca direção.
//
// A curva, por modo:
//   - seguir: a energia da referência, suavizada (o arco que ela já tem);
//   - transfigurar: um arco imposto, clímax perto da seção áurea (0,62);
//   - contrapor: a curva da referência invertida;
//   - interromper: a curva da referência com quedas bruscas (cortes).
//
// Fontes: a curva de tensão como forma (F. Lerdahl e C. Krumhansl, "Modeling
// tonal tension", Music Perception 24(4), 2007; E. Farbood, "A parametric,
// temporal model of musical tension", Music Perception 29(4), 2012); o
// clímax na seção áurea e a respiração antes dele (R. Howat, Debussy in
// Proportion, Cambridge UP, 1983).

#include "Composer.h"
#include "Structure.h"

#include <string>
#include <vector>

namespace trioio::composition
{

struct TensionCurve
{
    double step = 0.5;        // segundos por ponto
    std::vector<float> value; // 0..1
    double climax = -1.0;     // instante do clímax (s); < 0 = sem clímax claro
    std::string shape;        // "a energia da referência", "arco", …
    [[nodiscard]] float at(double t) const;
};

[[nodiscard]] TensionCurve tensionCurve(const std::vector<perception::Frame>& frames, double duration, strategy::Mode mode,
                                        std::uint64_t seed);

// As GRAMÁTICAS NARRATIVAS (2026-10-18; o conceito das "gramáticas
// plugáveis" do Rasgo Synth Studio, reescrito aqui, sem código copiado):
// a forma da peça como uma curva de tensão com nome, que pode MISTURAR duas
// gramáticas ao longo da peça (a primeira no começo, a segunda no fim).
//   freytag          — exposição, ascensão, clímax, queda, resolução
//                      (G. Freytag, Die Technik des Dramas, 1863)
//   kishotenketsu    — ki, shō, TEN, ketsu: apresentação, desenvolvimento,
//                      a VIRADA (um material que não decorre do anterior) e a
//                      conclusão que reconcilia (a estrutura de quatro partes
//                      do Leste Asiático, sem autor único)
//   ondas            — três ondas que crescem, cada uma mais alta
//   plato            — o patamar que sobe devagar (para a repetição)
//   arco             — o arco com clímax perto da seção áurea
[[nodiscard]] bool isNarrativeGrammar(const std::string& name);
// `names`: uma gramática, ou duas separadas por "+" ("freytag+plato")
[[nodiscard]] TensionCurve narrativeCurve(const std::string& names, double duration);
// Os eventos da gramática na partitura: no ki-shō-ten-ketsu, a virada (de 50
// a 75 % da peça): o motivo sobe uma oitava e troca de timbre com o
// contracanto, e a percussão rareia pela metade. Devolve a linha da forma.
std::string applyNarrativeEvents(Score& score, const std::string& names, std::uint64_t seed);

// Os GESTOS (do olho, os efeitos) também seguem a curva: o nível de cada
// um pela tensão no seu meio (0,85–1,15×; com a curva explícita, 0,6–1,4× e
// metade do caminho até o nível da curva). Antes, só as notas a seguiam, e
// os gestos, tão altos quanto o motivo e constantes, mascaravam a forma
// (medido em 2026-10-18 pelos stems). Depois do olho. Devolve quantos.
int shapeGestures(Score& score, const TensionCurve& curve);

// A curva desenhada pelo autor (Options::gesture): os pontos (posição 0..1,
// intensidade 0..1), interpolados; o clímax é o ponto mais alto, se ele se
// destaca (≥ 0,2 acima da mediana).
[[nodiscard]] TensionCurve gestureCurve(const std::vector<std::pair<float, float>>& points, double duration);
// "0:0.2, 0.6:1, 1:0.3" → pontos; falso se não entender
[[nodiscard]] bool parseGesture(const std::string& text, std::vector<std::pair<float, float>>& out);

// Molda a partitura: a força das notas (0,85–1,15× pela tensão), as vozes de
// enfeite (eco, memória, mosaico) rareiam nos vales, e — em peças de 60 s ou
// mais com clímax claro — uma respiração de um tempo logo antes do clímax.
// O motivo, o baixo e a percussão nunca são cortados pela densidade.
// Devolve a linha da forma.
// `breath` (opcional) recebe a janela da respiração; (−1, −1) se não houver.
std::string applyDramaturgy(Score& score, const TensionCurve& curve, const perception::Pulse& pulse, std::uint64_t seed,
                            std::pair<double, double>* breath = nullptr);

} // namespace trioio::composition
