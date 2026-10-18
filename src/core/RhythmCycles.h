#pragma once

// Os ciclos rítmicos de cada cultura (docs/RITMO.md, "Ciclos das culturas";
// data/ritmos/arquivo_trioio_ritmos.txt, gerado por
// tools/gerar_arquivo_de_ritmos.py): iqa' árabes, tala hindustani e
// carnático, usul turco e aksak, claves e linhas-guia, a colotomia do
// gamelão, o compás do flamenco e o shuffle. Cada ciclo cita a fonte.
//
// Quando a escala da seção (pelo comportamento, campo "ritmo") ou a opção
// --ciclo pedem um ciclo, a camada de percussão (voz 4) daquele trecho toca
// o ciclo, ancorado no pulso: dum/dha no grave, tak/tin no agudo, golpes
// leves, gongs da colotomia, clave em madeira; acentos no sam e nos tempos
// fortes; o khālī sem grave.

#include "Composer.h"
#include "ScaleArchive.h"
#include "ScaleMachine.h"
#include "Structure.h"

#include <memory>
#include <string>
#include <vector>

namespace trioio::composition
{

struct RhythmCycle
{
    std::string name, family, source, note;
    double unitBeats = 1.0;       // tempo = 1; colcheia = 1/2; colcheia-ternária = 1/3; semicolcheia = 1/4
    std::string strokes;          // um símbolo por unidade: D T k B x G N P t -
    std::vector<int> accents;     // posições fortes (a partir de 1)
    std::vector<int> empties;     // khālī
};

// O arquivo de ciclos ("= nome", "família:", "unidade:", "golpes:",
// "acentos:", "vazios:", "fonte:", "nota:"). Blocos ilegíveis vão para `errors`.
[[nodiscard]] std::vector<RhythmCycle> parseRhythmArchive(const std::string& text, std::vector<std::string>& errors);

// Reconhecer o ciclo de uma referência (docs/LEITURA_E_GABARITO.md): os
// ataques de [t0, t1) são dobrados sobre cada ciclo do arquivo, em cada
// deslocamento inicial e em três níveis do pulso (×½, ×1, ×2). Por posição,
// a força média e a parte GRAVE (centroide abaixo da mediana dos ataques:
// o dum contra o tak). Pontuação: correlação da força com o desenho do
// ciclo + ½ da correlação grave/agudo com os golpes D/B/G/N contra T/k/t.
struct RankedCycle
{
    const RhythmCycle* cycle = nullptr;
    double score = 0.0;
    int offset = 0;          // a posição do ciclo que cai na fase do pulso
    double levelScale = 1.0;  // o nível do pulso usado (½, 1, 2); negativo = o ciclo em m pulsos
    bool graveUsed = false;   // o ciclo tem golpes graves/agudos (D/T); as claves (x) não somam esse termo
    // claro o bastante para dizer "é este": 0,6 com o termo grave/agudo, 0,4
    // sem ele (os acertos sintéticos ficam em 0,85–1,0 e 0,45–0,67)
    [[nodiscard]] bool clear() const noexcept { return score >= (graveUsed ? 0.6 : 0.4); }
};
[[nodiscard]] std::vector<RankedCycle> rankCycles(const std::vector<RhythmCycle>& archive,
                                                  const std::vector<perception::Frame>& frames,
                                                  const std::vector<perception::Onset>& onsets,
                                                  const perception::Pulse& pulse, double t0, double t1, int count);

// O ciclo pelo nome (exato, ou o único que contém o termo; sem caixa).
[[nodiscard]] const RhythmCycle* findCycle(const std::vector<RhythmCycle>& archive, const std::string& query);

// Toca os ciclos: `perSection[k]` (nullptr = deixa a percussão como está)
// para o trecho [starts[k], ends[k]). Devolve a linha da forma.
[[nodiscard]] std::string cultureRhythm(Score& score, const std::vector<std::pair<double, double>>& spans,
                                        const std::vector<const RhythmCycle*>& perSection, const perception::Pulse& pulse,
                                        const Options& options);

// A melodia chega no ciclo (o sam do tala, o gong do gamelão, o dum do
// iqa'): em cada volta, a nota da voz 0 mais perto do tempo principal (o 1.º
// acento do ciclo) passa a começar nele, na nota de apoio da escala da seção
// (ou na tônica), com acento. Nos tala, a seção fecha com um TIHAI: uma
// frase curta três vezes, a última nota exatamente no sam (M. Clayton, Time
// in Indian Music, 2000). Devolve a linha da forma.
[[nodiscard]] std::string alignMelodyToCycles(Score& score, const std::vector<SectionScale>& sections,
                                              const std::vector<const RhythmCycle*>& cycles,
                                              const std::vector<const ScaleBehavior*>& behaviors,
                                              const perception::Pulse& pulse);

} // namespace trioio::composition
