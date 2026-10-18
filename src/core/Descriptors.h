#pragma once

// Percepção, primeira camada: descritores mínimos de um sinal mono.
// Especificação: docs/DESCRITORES.md.
//
// Mede, não decide: energia, brilho, ruído e ataques ao longo do tempo. O que
// fazer com isso é trabalho do Contexto e da Estratégia (docs/TRIOIO_CEREBRO.md §3).
//
// Thread de análise, nunca a de áudio: o construtor aloca; push() e finish()
// só crescem os vetores de resultado. Determinístico e independente do
// tamanho de bloco entregue a push().

#include <array>
#include <complex>
#include <cstddef>
#include <vector>

namespace trioio::perception
{

// Sobe quando um número produzido muda; o cache de análise usa esta versão.
// (O resumo não entra no cache — é recalculado dos quadros —, então mudar
// só o resumo não exige subir a versão.)
// v2 (2026-10-05): croma e picos espectrais por quadro.
// v3 (2026-10-05): altura fundamental (YIN) e periodicidade.
// v4 (2026-10-08): planura entre 50 Hz e 10 kHz, piso de −80 dB (áudio com perda).
// v5 (2026-10-10): separação harmônico/percussivo (HPSS) no offline: fluxo
//     percussivo por quadro e croma só da parte harmônica.
// v6 (2026-10-10): croma do grave (45–260 Hz) por quadro, para os acordes; e
//     a altura predominante por saliência harmônica (melodia na mistura).
// v7 (2026-10-10): fluxo por banda (grave < 200 Hz, agudo > 2 kHz), para
//     os ataques por banda (bandOnsets).
// v8 (2026-10-11): os 3 candidatos de altura predominante por quadro e a
//     linha da melodia rastreada (trackMelody) em melodyHz.
inline constexpr int kAnalyzerVersion = 8;
inline constexpr int kMelodyCandidates = 3;
inline constexpr int kPeaks = 4;

struct AnalyzerConfig
{
    int frameSize = 2048;          // potência de 2
    int hopSize = 1024;
    float silenceDb = -70.0f;      // abaixo disto o quadro é silêncio
    double minOnsetGapSeconds = 0.05;
    bool hpss = true;  // separação harmônico/percussivo no offline (v5); false = como na v4, para comparar
};

struct Frame
{
    double time = 0.0;        // centro do quadro, em segundos
    float rmsDb = -120.0f;    // dBFS
    float centroidHz = 0.0f;  // 0 em silêncio
    float spreadHz = 0.0f;
    float flatness = 0.0f;    // 0 = tonal, 1 = ruído branco; 0 em silêncio
    float flux = 0.0f;        // fluxo espectral log, só subidas
    bool silent = true;
    // classes de altura (0 = Dó … 11 = Si), soma 1 quando há som; 110 Hz..5 kHz
    std::array<float, 12> chroma {};
    // os picos espectrais mais fortes, em Hz (interpolação parabólica), 0 = nenhum
    std::array<float, kPeaks> peaksHz {};
    // altura fundamental provável (YIN, 50–1000 Hz), 0 = sem altura definida
    float f0Hz = 0.0f;
    float periodicity = 0.0f;  // 0..1: 1 = periódico puro
    // fluxo só da parte PERCUSSIVA (HPSS, Analyzer offline); 0 = sem
    // separação (escuta ao vivo, caches antigos): quem usa cai no `flux`
    float percFlux = 0.0f;
    // classes de altura do GRAVE (picos entre 45 e 260 Hz, os mais graves
    // pesando mais), soma 1; zeros sem grave. É o baixo que distingue Dó6
    // de Lám7 (as mesmas quatro notas) e as inversões.
    std::array<float, 12> bassChroma {};
    // altura PREDOMINANTE por saliência harmônica (soma de harmônicos sobre
    // os picos do espectro longo; J. Salamon e E. Gómez, IEEE TASLP 2012;
    // A. Klapuri, ISMIR 2006): acha a voz principal dentro da mistura, onde
    // o YIN (f0Hz) se perde. 100–1000 Hz; 0 = nada. `melodySalience` é o
    // pico da saliência ÷ a soma das magnitudes dos picos usados (≈ 0..3).
    // Não decide se há melodia ou só acompanhamento (o MELODIA faz isso por
    // contornos): é a altura mais saliente do quadro.
    float melodyHz = 0.0f;
    float melodySalience = 0.0f;
    // o fluxo só da banda grave (< 200 Hz: bumbo, dum, baixo) e só da aguda
    // (> 2 kHz: chimbal, tak, clave), média por bin; para os ataques por banda
    float fluxLow = 0.0f, fluxHigh = 0.0f;
    // os picos de saliência do quadro (os kMelodyCandidates maiores, do maior
    // para o menor; 0 = nenhum): a melodia é escolhida entre eles pela
    // continuidade (trackMelody), não só pelo maior
    std::array<float, kMelodyCandidates> melodyCandHz {};
    std::array<float, kMelodyCandidates> melodyCandSalience {};
};

// A linha da melodia: um caminho de Viterbi pelos candidatos de cada quadro
// (o MELODIA escolhe contornos; aqui, uma versão enxuta): cada passo custa
// `jump` por semitom saltado, e cada candidato ganha `height` por oitava
// acima de Dó4 (a melodia costuma ficar acima do acompanhamento: no
// autogabarito, os erros eram quase todos uma ou duas oitavas abaixo). O
// silêncio reinicia a linha. Devolve a altura escolhida por quadro (0 =
// nenhuma) e a saliência dela.
struct MelodyTrackParams
{
    double jump = 0.08, height = 0.25;
};
void trackMelody(const std::vector<Frame>& frames, std::vector<float>& hz, std::vector<float>& salience,
                 const MelodyTrackParams& params = {});

// Um ataque achado por banda (bandOnsets): grave, agudo ou os dois juntos.
struct BandOnset
{
    double time = 0.0;
    float strength = 0.0f;  // o maior das bandas, cada uma normalizada pelo seu p90
    bool low = false, high = false;
    // o fluxo de cada banda no ataque, ÷ o p90 da banda: grave quando lowLevel
    // domina (um tambor grave tem clique de ataque, um tak tem ruído largo:
    // os dois acendem as duas bandas)
    float lowLevel = 0.0f, highLevel = 0.0f;
    [[nodiscard]] bool grave() const noexcept { return lowLevel > highLevel; }
};

// Os ataques de cada banda (grave e aguda) escolhidos pela regra de sempre
// (pico local acima de 1,5 × a mediana local + 0,1 × o p90 da banda) e
// juntados: ataques a até 30 ms viram um só. Na seleção pelo fluxo inteiro,
// um golpe agudo logo depois de um grave some atrás dele (achado no shuffle
// sintético, 2026-10-10); por banda, os dois ficam.
[[nodiscard]] std::vector<BandOnset> bandOnsets(const std::vector<Frame>& frames, double minGapSeconds = 0.05);

struct Onset
{
    double time = 0.0;
    float strength = 0.0f;  // fluxo acima do limiar local
};

// Regra de ataque, a mesma no offline e ao vivo: pico local do fluxo
// (±kOnsetPeakHalfWidth quadros) acima de kOnsetMedianScale × a mediana
// local + kOnsetOffset.
inline constexpr int kOnsetPeakHalfWidth = 3;
inline constexpr int kOnsetMedianHalfWidth = 10;
inline constexpr float kOnsetMedianScale = 1.5f;
inline constexpr float kOnsetOffset = 0.02f;

// Janela longa para harmonia: 8192 amostras (≈170 ms, 5,9 Hz por bin a
// 48 kHz). Com 2048, Lá 220 e Dó 261 caem a menos de 2 bins e se fundem.
inline constexpr int kHarmonyFrameSize = 8192;

// Separação harmônico/percussivo por filtros de mediana (D. Fitzgerald,
// "Harmonic/Percussive Separation using Median Filtering", DAFx 2010):
// num espectrograma, o que é harmônico se estende no TEMPO (linha
// horizontal) e o que é percussivo se estende na FREQUÊNCIA (linha
// vertical). A mediana no tempo estima a parte harmônica; a mediana na
// frequência (±≈ 190 Hz), a percussiva; a máscara suave de Wiener (p = 2)
// divide cada bin entre as duas. Centrada: só no offline, que pode olhar à
// frente. Duas larguras no tempo:
//   - janela curta (fluxo percussivo): ±kHpssHalfFrames quadros (≈ ±180 ms);
//   - janela longa (croma harmônico): ±kHpssHalfFramesLong (≈ ±0,5 s). A
//     janela de 170 ms espalha um golpe curto por ~10 quadros; com ±8, um
//     tom afinado passava por "harmônico" (achado de 2026-10-10).
inline constexpr int kHpssHalfFrames = 8;
inline constexpr int kHpssHalfFramesLong = 24;
// O croma harmônico só substitui o croma quando a peça TEM percussão: numa
// melodia sozinha, a mediana longa apaga as notas rápidas, e a harmonia
// tirada da linha se perde (achado de 2026-10-10, ComposerTests §8). A
// decisão é por peça, pela fração percussiva da magnitude (ver
// Analyzer::percussiveShare). Medido em 2026-10-10: melodia pinçada 0,08,
// pad 0,01, voz sozinha (vocalise) 0,31, faixa com banda (Suno) 0,51,
// bateria + pad 0,76. O limiar 0,40 separa a voz sozinha da banda;
// provisório até haver gabaritos (docs/LEITURA_E_GABARITO.md).
inline constexpr float kPercussiveShareForHarmonicChroma = 0.40f;
inline constexpr double kHpssHalfHz = 190.0;

// O croma pelos picos de um espectro longo (kHarmonyFrameSize): cada pico
// entre 110 Hz e 5 kHz, acima de 1 % do maior, soma sua magnitude à classe
// de altura da frequência interpolada. Soma 1 quando há picos; zeros senão.
void chromaFromSpectrum(const std::vector<double>& magLong, double sampleRate, std::array<float, 12>& chroma) noexcept;
// A altura predominante de um espectro longo (ver Frame::melodyHz). `work`
// tem kSalienceBins posições (pré-alocado: sem alocação aqui).
inline constexpr int kSalienceBins = 400;  // 10 cents de 100 Hz a 1000 Hz
void predominantPitch(const std::vector<double>& magLong, double sampleRate, std::vector<double>& work,
                      float& hz, float& salience) noexcept;
// O mesmo, devolvendo os kMelodyCandidates máximos locais da saliência (a
// pelo menos 1 semitom um do outro), do maior para o menor.
void predominantCandidates(const std::vector<double>& magLong, double sampleRate, std::vector<double>& work,
                           std::array<float, kMelodyCandidates>& hz, std::array<float, kMelodyCandidates>& salience) noexcept;

// O croma do grave: picos entre 45 e 260 Hz acima de 5 % do maior pico
// dessa faixa, cada um pesando magnitude × 130 Hz / frequência.
void bassChromaFromSpectrum(const std::vector<double>& magLong, double sampleRate, std::array<float, 12>& chroma) noexcept;

// Magnitudes de uma FFT radix-2 (Cooley–Tukey) com janela de Hann.
// Aloca só no construtor.
class Spectrum
{
public:
    explicit Spectrum(std::size_t size);
    void magnitudes(const float* samples, std::vector<double>& out) noexcept;  // out: size/2 + 1
    [[nodiscard]] std::size_t size() const noexcept { return window_.size(); }

private:
    std::vector<float> window_;
    std::vector<std::complex<double>> fft_;
    std::vector<std::complex<double>> twiddles_;
    std::vector<std::size_t> bitrev_;
};

// O cálculo de UM quadro, compartilhado pela análise offline e pela escuta
// ao vivo. Aloca só no construtor; analyze() não aloca.
// Contrato: chamadas consecutivas avançam `hopSize` amostras (a primeira
// recebe o quadro inteiro); assim o histórico da janela longa se mantém.
class FrameAnalyzer
{
public:
    FrameAnalyzer(double sampleRate, const AnalyzerConfig& config);
    // `samples` tem config.frameSize amostras; `time` é o centro do quadro.
    Frame analyze(const float* samples, double time) noexcept;
    // os espectros do último quadro (o longo só vale se o quadro tinha som)
    [[nodiscard]] const std::vector<double>& shortMagnitudes() const noexcept { return mag_; }
    [[nodiscard]] const std::vector<double>& longMagnitudes() const noexcept { return magLong_; }
    [[nodiscard]] bool longValid() const noexcept { return longValid_; }

private:
    bool longValid_ = false;
    double sampleRate_;
    AnalyzerConfig config_;
    Spectrum short_;
    Spectrum long_;
    std::vector<float> history_;  // últimas kHarmonyFrameSize amostras
    std::vector<double> mag_;
    std::vector<double> magLong_;
    std::vector<double> prevLogMag_;
    bool havePrev_ = false;
    // YIN: últimas 4096 amostras reduzidas a ~12 kHz
    int decim_ = 1;
    std::vector<double> yinX_;
    std::vector<double> yinD_;
    std::vector<double> salience_;  // kSalienceBins
};

class Analyzer
{
public:
    explicit Analyzer(double sampleRate, AnalyzerConfig config = {});

    void push(const float* mono, std::size_t count);
    // Fecha o último quadro (com zeros) e marca os ataques. Chamar uma vez.
    void finish();

    [[nodiscard]] const std::vector<Frame>& frames() const noexcept { return frames_; }
    [[nodiscard]] const std::vector<Onset>& onsets() const noexcept { return onsets_; }
    [[nodiscard]] double sampleRate() const noexcept { return sampleRate_; }
    [[nodiscard]] const AnalyzerConfig& config() const noexcept { return config_; }
    // HPSS: a fração da magnitude que é percussiva nos quadros com som
    // (−1 sem separação), e se o croma passou a ser o harmônico
    [[nodiscard]] float percussiveShare() const noexcept { return percussiveShare_; }
    [[nodiscard]] bool harmonicChroma() const noexcept { return harmonicChroma_; }

private:
    void analyzeFrame();
    void pickOnsets();
    void separate(std::size_t j);  // HPSS do quadro j (precisa de j ± kHpssHalfFrames no anel)

    double sampleRate_;
    AnalyzerConfig config_;
    FrameAnalyzer frameAnalyzer_;
    std::vector<float> buffer_;
    std::size_t filled_ = 0;
    std::size_t consumed_ = 0;  // amostras já deixadas para trás (início do quadro atual)
    std::size_t pushed_ = 0;
    bool finished_ = false;
    std::vector<Frame> frames_;
    std::vector<Onset> onsets_;
    // HPSS: anel com os espectros dos últimos 2·kHpssHalfFrames + 1 quadros
    std::vector<std::vector<double>> shortRing_, longRing_;
    std::vector<char> longOk_;
    std::vector<double> prevPercLog_, masked_, column_;
    std::size_t separated_ = 0;
    std::vector<std::array<float, 12>> harmonic_;  // o croma harmônico de cada quadro (zeros = sem)
    std::vector<std::array<float, 12>> harmonicBass_;
    double percSum_ = 0.0, magSum_ = 0.0;
    float percussiveShare_ = -1.0f;
    bool harmonicChroma_ = false;
};

// Resumo de um trecho [t0, t1). Escalas 0..1 em docs/DESCRITORES.md §3.
struct Summary
{
    double start = 0.0;
    double end = 0.0;
    float meanRmsDb = -120.0f;   // média em potência, silêncio incluído
    float energy = 0.0f;         // (meanRmsDb + 60) / 60, limitado a 0..1
    float dynamicRangeDb = 0.0f; // p95 − p10 dos quadros com som
    float onsetsPerSecond = 0.0f;
    float density = 0.0f;        // r / (r + 4): 4 ataques/s → 0,5
    float centroidHz = 0.0f;     // média geométrica (em oitavas) dos quadros com som
    float brightness = 0.0f;     // log2(c/100) / log2(80), limitado a 0..1
    float noisiness = 0.0f;      // planura média dos quadros com som
    float activeShare = 0.0f;    // fração de quadros com som
    int onsetCount = 0;
};

[[nodiscard]] Summary summarize(const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                                double t0, double t1);

// Janelas consecutivas de `seconds` (a última pode ser menor).
[[nodiscard]] std::vector<Summary> segment(const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                                           double duration, double seconds);

// Um trecho com som em ao menos 20 % do tempo: só então brilho e ruído
// significam algo (silêncio não tem timbre).
[[nodiscard]] bool hasTimbre(const Summary& s) noexcept;

// Distância entre dois resumos nas quatro escalas 0..1 (energia, densidade,
// brilho, ruído): base do "onde a música muda" do relatório. Se um dos dois
// não tem timbre, só energia e densidade contam.
[[nodiscard]] float contrast(const Summary& a, const Summary& b) noexcept;

// Correlação de Pearson de uma escala entre duas linhas do tempo, só nos
// trechos com timbre nas duas e pulando o primeiro (subida dos suavizadores).
// 0 se houver menos de 3 trechos comparáveis.
[[nodiscard]] float correlation(const std::vector<Summary>& a, const std::vector<Summary>& b,
                                float Summary::*scale);

} // namespace trioio::perception
