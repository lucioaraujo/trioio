#pragma once

// A leitura de uma referência e a sua avaliação contra um gabarito do autor
// (docs/LEITURA_E_GABARITO.md). Mede, não decide.
//
// - Tonalidade global: perfis de Krumhansl e Kessler (C. L. Krumhansl, E. J.
//   Kessler, Psychological Review 89, 1982; C. L. Krumhansl, Cognitive
//   Foundations of Musical Pitch, Oxford UP, 1990), correlacionados com o
//   croma médio da peça.
// - Leitura: pulso, tempos, tonalidade, acordes, seções (com letras na forma
//   por repetição) e a cobertura de altura definida (quanto da peça tem uma
//   melodia legível).
// - Gabarito: o que o AUTOR ouve, num arquivo de texto próprio. As métricas
//   seguem as convenções públicas de avaliação em MIR (MIREX; C. Raffel et
//   al., "mir_eval", ISMIR 2014), reimplementadas aqui: tempos com ±70 ms
//   (S. Dixon, JNMR 36, 2007), tonalidade com acerto parcial para quinta,
//   relativa e homônima, acordes pela fração do tempo, fronteiras de seção
//   com ±3 s.

#include "Harmony.h"
#include "Structure.h"

#include <string>
#include <utility>
#include <vector>

namespace trioio::composition
{

struct Key
{
    int tonic = -1;  // 0..11, −1 = sem leitura
    bool minor = false;
    float correlation = 0.0f;  // Pearson com o perfil, −1..1
    int secondTonic = -1;      // a segunda melhor (a dúvida, quando perto)
    bool secondMinor = false;
    float secondCorrelation = 0.0f;
    [[nodiscard]] std::string name() const;  // "Sol menor", "Ré maior"
};

// A tonalidade pelo croma médio de [t0, t1).
[[nodiscard]] Key estimateKey(const std::vector<perception::Frame>& frames, double t0, double t1);
// A tonalidade pelas notas dos acordes lidos, cada tríade pesada pela
// duração (a fundamental conta um pouco mais). −1 se não houver acordes.
[[nodiscard]] Key keyFromChords(const std::vector<ChordSpan>& chords);
// A tonalidade de um croma qualquer (12 pesos).
[[nodiscard]] Key keyFromChroma(const std::array<float, 12>& chroma);

struct Reading
{
    double duration = 0.0;
    perception::Pulse pulse;
    perception::Downbeat downbeat;  // o 1 do compasso (o grave dominante e a mudança de acorde)
    std::vector<double> beats;  // os tempos lidos: fase + k × período
    Key key;        // pelos acordes lidos (as tríades pesadas pela duração); senão pelo croma
    Key chromaKey;  // pelo croma bruto: os harmônicos (o 5.º de Lá é Dó#) puxam para o maior
    std::vector<ChordSpan> chords;
    std::vector<double> boundaries;  // começo de cada seção depois da primeira
    std::vector<int> labels;         // letra de cada seção (forma por repetição); vazio = por contraste
    float pitchedShare = 0.0f;       // quadros com som que têm altura definida (YIN, periodicidade ≥ 0,6)
    float bassAgreement = -1.0f;
    // a altura predominante (saliência harmônica) nos quadros com som: os
    // percentis 10, 50 e 90 em MIDI (0 = sem leitura)
    float melodyP10 = 0.0f, melodyP50 = 0.0f, melodyP90 = 0.0f;
    // o campo de alturas que ESTRUTURA a peça: as notas dos acordes lidos
    // (por duração) e as da melodia predominante, cada metade somando 0,5.
    // O croma bruto tem um pouco de tudo, e uma escala cromática o vestia.
    std::array<float, 12> structuralField {};
    // os ataques com altura para o compositor (pitchAt): com a altura
    // predominante como reserva, e só pelo YIN (como antes da v6)
    float onsetsPitched = 0.0f, onsetsPitchedYin = 0.0f;     // quadros com grave em que a nota mais forte do grave é a do croma (−1 = sem grave)
};

// `tempoScale` 0,5 / 2 = a escuta do autor sobre o nível do pulso (--andamento).
[[nodiscard]] Reading readReference(const std::vector<perception::Frame>& frames,
                                    const std::vector<perception::Onset>& onsets, double duration,
                                    bool byRepetition = false, double tempoScale = 1.0,
                                    perception::PulseEnvelope envelope = perception::PulseEnvelope::bands);

// Com que frequência a nota mais forte do grave é a nota mais forte do croma
// inteiro, nos quadros com grave. Numa voz sozinha o "grave" é a própria
// melodia e coincide quase sempre; numa banda o baixo segue os acordes.
// −1 se não há grave.
[[nodiscard]] float bassAgreement(const std::vector<perception::Frame>& frames);

// ---- gabarito ------------------------------------------------------------------

struct Gabarito
{
    std::string reference;   // ref-…
    std::string source;      // "escuta do autor, 2026-10-10"
    std::vector<double> beats;
    double bpm = 0.0;        // andamento declarado (ou a mediana das batidas)
    int beatsPerBar = 0;
    bool hasKey = false;
    int keyTonic = 0;
    bool keyMinor = false;
    std::vector<std::pair<double, int>> chords;  // (início, índice em chordVocabulary ou −1 = sem acorde)
    std::vector<std::pair<double, std::string>> sections;  // (início, letra)
    double end = 0.0;        // fim declarado (0 = a duração da leitura)
};

// Os 36 acordes que o gabarito e a leitura compartilham: maior, menor e
// diminuto sobre as 12 classes.
[[nodiscard]] Chord chordFromIndex(int index);
[[nodiscard]] int chordIndex(const Chord& c);

// "Solm", "Sol menor", "Gm", "F#", "Si°", "Bdim", "N" (sem acorde).
// −2 = ilegível, −1 = sem acorde, 0..35 = o acorde.
[[nodiscard]] int parseChordName(const std::string& text);
// "Sol menor", "Solm", "Gm", "Ré maior", "D". false se ilegível.
[[nodiscard]] bool parseKeyName(const std::string& text, int& tonic, bool& minor);
// "83.4", "1:23.4", "1:23". false se ilegível.
[[nodiscard]] bool parseClock(const std::string& text, double& seconds);

// O formato está em docs/LEITURA_E_GABARITO.md. Erros vão para `errors`,
// com o número da linha.
[[nodiscard]] Gabarito parseGabarito(const std::string& text, std::vector<std::string>& errors);

struct Evaluation
{
    // andamento: lido ÷ gabarito; "certo" (±4 %), "metade", "dobro", "terço",
    // "triplo" ou "outro"
    double tempoRatio = 0.0;
    std::string tempoVerdict;
    bool hasBeats = false;
    double beatF = 0.0, beatPrecision = 0.0, beatRecall = 0.0;  // ±70 ms
    bool hasKey = false;
    double keyScore = 0.0;  // 1 certa · 0,5 quinta · 0,3 relativa · 0,2 homônima · 0
    std::string keyRelation;
    bool hasChords = false;
    double chordOverlap = 0.0;  // fração do tempo com o mesmo acorde
    bool hasSections = false;
    double boundaryF = 0.0, boundaryPrecision = 0.0, boundaryRecall = 0.0;  // ±3 s
};

[[nodiscard]] Evaluation evaluate(const Reading& reading, const Gabarito& gabarito);

// ---- teste por transformação (equivariância) ------------------------------------
// Tocar a gravação `rate` vezes mais rápido (rate = 2^(k/12)) sobe as alturas
// k semitons e o andamento × rate. Uma escuta correta se transforma junto. Não
// mede acerto (uma leitura constante seria "coerente"), mas pega o que é
// frágil nas gravações reais, sem gabarito.

// Reamostragem por janela de Lanczos (a = 8) com corte antialiasing:
// y(t) = x(t · rate).
void resampleSpeed(const std::vector<float>& x, double rate, std::vector<float>& y);

struct Equivariance
{
    double tempoRatio = 0.0;    // BPM transformado ÷ original ÷ rate: 1 = coerente
    bool keyOk = false;         // a tonalidade subiu k e manteve o modo
    double chordAgree = 0.0;    // fração do tempo com o acorde transposto em k
    double melodyAgree = 0.0;   // fração dos quadros com a altura predominante + k (±50 cents)
    double boundaryF = 0.0;     // as fronteiras de seção no tempo ÷ rate (±3 s)
};
[[nodiscard]] Equivariance compareTransformed(const Reading& original, const std::vector<perception::Frame>& originalFrames,
                                              const Reading& transformed, const std::vector<perception::Frame>& transformedFrames,
                                              double rate, int semitones);

// ---- autogabarito: a partitura do próprio Trioio como resposta exata ------------
// (docs/LEITURA_E_GABARITO.md, "Autogabarito"). O Trioio compõe, sintetiza e
// escuta o próprio áudio; a partitura diz o que de fato soou:
//   - andamento: score.bpm;
//   - acordes: as notas de cada tempo (sem a voz 4, a percussão), pesadas
//     pela duração × intensidade, pelo molde das 36 tríades (N se nenhuma
//     veste);
//   - tonalidade: Krumhansl e Kessler sobre as notas da peça inteira;
//   - seções: as marcas "seção N".
[[nodiscard]] Gabarito gabaritoFromScore(const Score& score);

// A melodia lida contra a voz 0 da partitura, nos quadros em que uma nota
// da voz 0 soa (sem os 60 ms das pontas): a fração com a altura
// predominante a ±50 cents (pitch) e na mesma classe de altura (chroma, sem
// a oitava). −1 se não há quadro comparável.
struct MelodyScore
{
    double pitch = -1.0, chroma = -1.0;
    double octaveBelow = 0.0, octaveAbove = 0.0;  // fração com a classe certa uma ou mais oitavas abaixo / acima
    int frames = 0;
};
[[nodiscard]] MelodyScore melodyAgainstScore(const std::vector<perception::Frame>& frames, const Score& score);

// F-measure de dois conjuntos de instantes com tolerância (casamento guloso,
// cada referência casa no máximo uma vez).
struct Match
{
    double precision = 0.0, recall = 0.0, f = 0.0;
};
[[nodiscard]] Match matchTimes(const std::vector<double>& estimated, const std::vector<double>& reference,
                               double tolerance);

} // namespace trioio::composition
