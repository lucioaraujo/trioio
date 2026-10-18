#pragma once

// Escuta da estrutura: pulso, grade rítmica, campo harmônico e seções de uma
// referência, a partir dos quadros e ataques de Descriptors. É o material do
// compositor (docs/COMPOSICAO.md). Mede, não decide.

#include "Descriptors.h"

#include <array>
#include <vector>

namespace trioio::perception
{

struct Pulse
{
    double bpm = 0.0;
    double period = 0.0;     // segundos por tempo
    double phase = 0.0;      // instante do primeiro tempo, 0..period
    float confidence = 0.0f; // 0..1: quanto o pulso se destaca do resto das periodicidades
    double barPeriod = 0.0;  // ciclo do compasso medido (repetição do padrão), 0 = não achado
    int beatsPerBar = 4;     // 2 a 7: medido pelo ciclo do compasso; 4 quando não há acento de compasso
};

// Andamento entre 50 e 200 BPM por autocorrelação do fluxo, com preferência
// perceptiva em torno de 100 BPM (log-gaussiana de uma oitava; Parncutt
// 1994, London 2004: o pulso de contagem fica entre 600 e 700 ms).
// A fase vem da média circular dos ataques no período (a datação dos ataques
// é mais fina que o quadro); sem ataques, do fluxo.
// O envelope que o pulso escuta: as bandas grave + aguda (padrão desde a
// v7) ou o fluxo percussivo/inteiro (como antes), para comparar.
enum class PulseEnvelope { bands, flux };
[[nodiscard]] Pulse estimatePulse(const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                                  double t0, double t1, PulseEnvelope envelope = PulseEnvelope::bands);

// O tempo forte (o 1 do compasso): entre os `beatsPerBar` deslocamentos
// possíveis da grade, o que soma mais, nos seus tempos, a banda GRAVE (o
// bumbo e o baixo marcam o 1) e a MUDANÇA de acorde (a distância do croma
// para o tempo anterior: a harmonia costuma mudar no 1). Ver M. Davies e M.
// Plumbley, "A spectral difference approach to downbeat extraction in
// musical audio", EUSIPCO 2006; G. Peeters e H. Papadopoulos, IEEE TASLP
// 2011. Devolve o instante de um tempo forte (no primeiro compasso) e a
// confiança (0..1: a vantagem do melhor sobre o segundo).
struct Downbeat
{
    double time = 0.0;
    int offsetBeats = 0;  // quantos tempos depois de pulse.phase
    float confidence = 0.0f;
};
[[nodiscard]] Downbeat estimateDownbeat(const std::vector<Frame>& frames, const Pulse& pulse, double t0, double t1);

// O compasso: 3 ou 4 tempos (2 e 4 são o mesmo metro, agrupado diferente).
// Por tempo, o grave dominante e o croma; para m = 2, 3, 4, a correlação do
// grave no atraso de m tempos mais ½ da semelhança do croma no mesmo atraso.
// 3 só vence se superar o 2 e o 4 por `margin`. Devolve os tempos por
// compasso e a confiança (a vantagem do vencedor).
struct Meter
{
    int beatsPerBar = 4;
    float confidence = 0.0f;
    double score2 = 0.0, score3 = 0.0, score4 = 0.0;
};
[[nodiscard]] Meter estimateMeter(const std::vector<Frame>& frames, const Pulse& pulse, double t0, double t1,
                                  double margin = 0.05);

inline constexpr int kStepsPerBeat = 4;
inline constexpr int kMaxBeatsPerBar = 8;
inline constexpr int kMaxStepsPerBar = kStepsPerBeat * kMaxBeatsPerBar;

struct Bar
{
    double start = 0.0;
    int steps = 16;  // passos deste compasso: tempos × 4
    std::array<float, kMaxStepsPerBar> hits {};  // força do ataque em cada semicolcheia (0 = nada)
    [[nodiscard]] int count() const noexcept;
};

// Os ataques da referência quantizados na grade do pulso: compassos de
// pulse.beatsPerBar tempos com 4 passos por tempo.
[[nodiscard]] std::vector<Bar> rhythmGrid(const std::vector<Onset>& onsets, const Pulse& pulse,
                                          double t0, double t1);

// Altura provável em `t`, em MIDI com fração: mediana do f0 dos quadros a
// ±40 ms com periodicidade ≥ 0,6. NaN se não houver altura definida.
[[nodiscard]] float pitchAt(const std::vector<Frame>& frames, double t);
// o piso de saliência para a altura predominante substituir o YIN em pitchAt
inline constexpr float kMelodySalienceForPitch = 0.15f;

using PitchField = std::array<float, 12>;  // peso de cada classe de altura, soma 1

// Croma médio dos quadros com som, ponderado pela potência.
[[nodiscard]] PitchField harmonicField(const std::vector<Frame>& frames, double t0, double t1);

struct Section
{
    double start = 0.0;
    double end = 0.0;
    Pulse pulse;
    PitchField field {};
    Summary summary;
};

// A forma por repetição (docs/COMPOSICAO.md, "Forma por repetição"):
//   1. a matriz de autossimilaridade de 1 s (croma e timbre);
//   2. a novidade de Foote: um núcleo em xadrez de `kernelSeconds` corre pela
//      diagonal; as fronteiras são os picos (J. Foote, "Automatic audio
//      segmentation using a measure of audio novelty", ICME 2000);
//   3. os segmentos parecidos recebem a mesma letra (A, B, A…): o que
//      VOLTA, como a estrofe e o refrão (M. Müller, Fundamentals of Music
//      Processing, Springer, 2015, cap. 4).
// `labels[k]` é a letra da seção k (0 = A, 1 = B…).
// a semelhança entre os segmentos da última chamada (z médio na diagonal),
// para diagnóstico e calibração; e o limiar de "mesma letra"
extern std::vector<std::vector<double>> lastSimilarity;
inline constexpr double kRepetitionThreshold = 0.8;
[[nodiscard]] std::vector<Section> findSectionsByRepetition(const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                                                            double duration, std::vector<int>& labels,
                                                            double minSeconds = 8.0, double kernelSeconds = 6.0);

// Seções por novidade: compara os `context` segundos antes e depois de cada
// ponto (energia, densidade, brilho, ruído e croma) e corta nos picos, com
// pelo menos `minSeconds` por seção.
[[nodiscard]] std::vector<Section> findSections(const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                                                double duration, double minSeconds = 8.0, double context = 4.0);

} // namespace trioio::perception
