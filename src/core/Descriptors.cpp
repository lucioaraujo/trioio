#include "Descriptors.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace trioio::perception
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

float toDb(double power) { return static_cast<float>(10.0 * std::log10(std::max(power, 1e-12))); }

float percentile(std::vector<float> v, double p)
{
    if (v.empty())
        return 0.0f;
    const auto k = static_cast<std::size_t>(std::clamp(p, 0.0, 1.0) * static_cast<double>(v.size() - 1) + 0.5);
    std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(k), v.end());
    return v[k];
}
} // namespace

Spectrum::Spectrum(std::size_t n)
{
    window_.resize(n);
    for (std::size_t i = 0; i < n; ++i)  // Hann periódica
        window_[i] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(i) / static_cast<double>(n)));
    fft_.resize(n);
    twiddles_.resize(n / 2);
    for (std::size_t i = 0; i < n / 2; ++i)
        twiddles_[i] = std::polar(1.0, -2.0 * kPi * static_cast<double>(i) / static_cast<double>(n));
    bitrev_.resize(n);
    std::size_t bits = 0;
    while ((std::size_t { 1 } << bits) < n)
        ++bits;
    for (std::size_t i = 0; i < n; ++i)
    {
        std::size_t r = 0;
        for (std::size_t b = 0; b < bits; ++b)
            r |= ((i >> b) & 1u) << (bits - 1 - b);
        bitrev_[i] = r;
    }
}

void Spectrum::magnitudes(const float* samples, std::vector<double>& out) noexcept
{
    const auto n = window_.size();
    for (std::size_t i = 0; i < n; ++i)
        fft_[bitrev_[i]] = { static_cast<double>(samples[i] * window_[i]), 0.0 };
    for (std::size_t len = 2; len <= n; len <<= 1)
    {
        const auto half = len / 2;
        const auto step = n / len;
        for (std::size_t s = 0; s < n; s += len)
            for (std::size_t k = 0; k < half; ++k)
            {
                const auto t = twiddles_[k * step] * fft_[s + k + half];
                fft_[s + k + half] = fft_[s + k] - t;
                fft_[s + k] += t;
            }
    }
    for (std::size_t k = 0; k <= n / 2; ++k)
        out[k] = std::abs(fft_[k]);
}

FrameAnalyzer::FrameAnalyzer(double sampleRate, const AnalyzerConfig& config)
    : sampleRate_(sampleRate), config_(config),
      short_(static_cast<std::size_t>(config.frameSize)), long_(static_cast<std::size_t>(kHarmonyFrameSize))
{
    const auto n = static_cast<std::size_t>(config_.frameSize);
    history_.assign(static_cast<std::size_t>(kHarmonyFrameSize), 0.0f);
    mag_.resize(n / 2 + 1);
    magLong_.resize(static_cast<std::size_t>(kHarmonyFrameSize) / 2 + 1);
    prevLogMag_.assign(n / 2 + 1, 0.0);
    decim_ = std::max(1, static_cast<int>(std::lround(sampleRate_ / 12000.0)));
    yinX_.assign(4096 / static_cast<std::size_t>(decim_), 0.0);
    yinD_.assign(yinX_.size(), 0.0);
    salience_.assign(static_cast<std::size_t>(kSalienceBins), 0.0);
}

// croma pelos PICOS (não por todos os bins): o vazamento da janela
// espalharia um Lá de 220 Hz por Sol# e Lá#.
void chromaFromSpectrum(const std::vector<double>& magLong, double sampleRate, std::array<float, 12>& out) noexcept
{
    out.fill(0.0f);
    const auto binsLong = magLong.size();
    const double binHzLong = sampleRate / static_cast<double>(kHarmonyFrameSize);
    double maxMag = 0.0;
    for (std::size_t k = 1; k < binsLong; ++k)
        maxMag = std::max(maxMag, magLong[k]);
    double chromaTotal = 0.0;
    std::array<double, 12> chroma {};
    for (std::size_t k = 2; k + 1 < binsLong; ++k)
    {
        const double a = magLong[k - 1], b = magLong[k], c = magLong[k + 1];
        if (b <= a || b < c || b < 0.01 * maxMag || b <= 0.0)
            continue;
        const double den = a - 2.0 * b + c;
        const double hz = (static_cast<double>(k) + (den != 0.0 ? 0.5 * (a - c) / den : 0.0)) * binHzLong;
        if (hz < 110.0 || hz > 5000.0)
            continue;
        const double midi = 69.0 + 12.0 * std::log2(hz / 440.0);
        chroma[static_cast<std::size_t>(((std::lround(midi) % 12) + 12) % 12)] += b;
        chromaTotal += b;
    }
    if (chromaTotal > 0.0)
        for (std::size_t i = 0; i < 12; ++i)
            out[i] = static_cast<float>(chroma[i] / chromaTotal);
}

namespace
{
// a função de saliência (Salamon e Gómez 2012, eq. 2–3) em `work`;
// devolve a soma das magnitudes dos picos usados (0 = nada)
double salienceFunction(const std::vector<double>& magLong, double sampleRate, std::vector<double>& work) noexcept
{
    constexpr int kMaxPeaks = 30, kHarmonics = 20;
    constexpr double kAlpha = 0.8, kLowHz = 100.0;
    const auto bins = magLong.size();
    const double binHz = sampleRate / static_cast<double>(kHarmonyFrameSize);
    // 1. os picos (250 Hz–5 kHz) acima de −40 dB do maior: os kMaxPeaks mais
    //    fortes. Abaixo de 250 Hz os picos não votam: o baixo (110–150 Hz)
    //    vencia a melodia (33 % de acerto na faixa de teste; 91 % assim). A
    //    melodia grave continua achada pelos harmônicos (a soma acha a
    //    fundamental ausente): Lá3–Sol4 com 86 % (ReadingTests, 2026-10-10).
    const auto kLo = std::max<std::size_t>(2, static_cast<std::size_t>(std::floor(250.0 / binHz)));
    const auto kHi = std::min(bins - 2, static_cast<std::size_t>(std::ceil(5000.0 / binHz)));
    double maxMag = 0.0;
    for (std::size_t k = kLo; k <= kHi; ++k)
        maxMag = std::max(maxMag, magLong[k]);
    if (maxMag <= 0.0)
        return 0.0;
    std::array<std::pair<double, double>, kMaxPeaks> peaks {};  // (magnitude, Hz)
    int count = 0;
    for (std::size_t k = kLo; k <= kHi; ++k)
    {
        const double a = magLong[k - 1], b = magLong[k], d = magLong[k + 1];
        if (b <= a || b < d || b < 0.01 * maxMag)
            continue;
        const double den = a - 2.0 * b + d;
        const double f = (static_cast<double>(k) + (den != 0.0 ? 0.5 * (a - d) / den : 0.0)) * binHz;
        if (count < kMaxPeaks)
            peaks[static_cast<std::size_t>(count++)] = { b, f };
        else
        {
            auto weakest = std::min_element(peaks.begin(), peaks.end());
            if (b > weakest->first)
                *weakest = { b, f };
        }
    }
    // 2. saliência: cada pico vota em f/h (h = 1..20) com peso α^(h−1),
    //    espalhado ±1 semitom por cos² (Salamon e Gómez 2012, eq. 2–3)
    std::fill(work.begin(), work.end(), 0.0);
    const int n = static_cast<int>(work.size());
    double used = 0.0;
    for (int p = 0; p < count; ++p)
    {
        const auto [mag, f] = peaks[static_cast<std::size_t>(p)];
        used += mag;
        double weight = 1.0;
        for (int hN = 1; hN <= kHarmonics; ++hN, weight *= kAlpha)
        {
            const double cand = f / hN;
            if (cand < kLowHz * 0.94)
                break;
            const double pos = 120.0 * std::log2(cand / kLowHz);  // em décimos de semitom
            const int c0 = static_cast<int>(std::floor(pos)) - 10, c1 = static_cast<int>(std::ceil(pos)) + 10;
            for (int b = std::max(0, c0); b <= std::min(n - 1, c1); ++b)
            {
                const double d = std::abs(b - pos) / 10.0;  // em semitons
                if (d < 1.0)
                {
                    const double g = std::cos(0.5 * 3.14159265358979323846 * d);
                    work[static_cast<std::size_t>(b)] += mag * weight * g * g;
                }
            }
        }
    }
    return used;
}
} // namespace

void predominantPitch(const std::vector<double>& magLong, double sampleRate, std::vector<double>& work,
                      float& hz, float& salience) noexcept
{
    hz = 0.0f;
    salience = 0.0f;
    const double used = salienceFunction(magLong, sampleRate, work);
    const auto best = std::max_element(work.begin(), work.end());
    if (used <= 0.0 || *best <= 0.0)
        return;
    hz = static_cast<float>(100.0 * std::exp2(static_cast<double>(best - work.begin()) / 120.0));
    salience = static_cast<float>(*best / used);
}

void predominantCandidates(const std::vector<double>& magLong, double sampleRate, std::vector<double>& work,
                           std::array<float, kMelodyCandidates>& hz, std::array<float, kMelodyCandidates>& salience) noexcept
{
    hz.fill(0.0f);
    salience.fill(0.0f);
    const auto n = work.size();
    const double used = salienceFunction(magLong, sampleRate, work);
    if (used <= 0.0)
        return;
    // máximos locais, os maiores primeiro, a ≥ 10 bins (1 semitom) um do outro
    std::array<std::pair<double, int>, kMelodyCandidates> top {};
    int count = 0;
    for (std::size_t i = 1; i + 1 < n; ++i)
    {
        if (work[i] <= 0.0 || work[i] < work[i - 1] || work[i] < work[i + 1])
            continue;
        bool far = true;
        for (int k = 0; k < count; ++k)
            if (std::abs(top[static_cast<std::size_t>(k)].second - static_cast<int>(i)) < 10)
            {
                far = false;
                if (work[i] > top[static_cast<std::size_t>(k)].first)
                    top[static_cast<std::size_t>(k)] = { work[i], static_cast<int>(i) };
                break;
            }
        if (!far)
            continue;
        if (count < kMelodyCandidates)
            top[static_cast<std::size_t>(count++)] = { work[i], static_cast<int>(i) };
        else
        {
            auto weakest = std::min_element(top.begin(), top.end());
            if (work[i] > weakest->first)
                *weakest = { work[i], static_cast<int>(i) };
        }
    }
    for (int x = 0; x < count; ++x)  // do maior para o menor (3 elementos)
        for (int y = x + 1; y < count; ++y)
            if (top[static_cast<std::size_t>(y)].first > top[static_cast<std::size_t>(x)].first)
                std::swap(top[static_cast<std::size_t>(x)], top[static_cast<std::size_t>(y)]);
    for (int k = 0; k < count; ++k)
    {
        hz[static_cast<std::size_t>(k)] = static_cast<float>(100.0 * std::exp2(top[static_cast<std::size_t>(k)].second / 120.0));
        salience[static_cast<std::size_t>(k)] = static_cast<float>(top[static_cast<std::size_t>(k)].first / used);
    }
}

void trackMelody(const std::vector<Frame>& frames, std::vector<float>& hz, std::vector<float>& sal,
                 const MelodyTrackParams& par)
{
    const auto n = frames.size();
    hz.assign(n, 0.0f);
    sal.assign(n, 0.0f);
    constexpr int K = kMelodyCandidates;
    const auto midi = [](float f) { return 69.0 + 12.0 * std::log2(f / 440.0); };
    std::size_t i = 0;
    while (i < n)
    {
        // um trecho com som e candidatos
        while (i < n && (frames[i].silent || frames[i].melodyCandHz[0] <= 0.0f))
            ++i;
        std::size_t j = i;
        while (j < n && !frames[j].silent && frames[j].melodyCandHz[0] > 0.0f)
            ++j;
        if (j > i)
        {
            const auto m = j - i;
            std::vector<std::array<double, K>> score(m);
            std::vector<std::array<int, K>> back(m);
            const auto emit = [&](std::size_t f, int k) {
                const auto& fr = frames[i + f];
                const float s = fr.melodyCandSalience[static_cast<std::size_t>(k)];
                if (fr.melodyCandHz[static_cast<std::size_t>(k)] <= 0.0f || s <= 0.0f)
                    return -1e9;
                return std::log(s / fr.melodyCandSalience[0]) + par.height * (midi(fr.melodyCandHz[static_cast<std::size_t>(k)]) - 60.0) / 12.0;
            };
            for (int k = 0; k < K; ++k)
                score[0][static_cast<std::size_t>(k)] = emit(0, k);
            for (std::size_t f = 1; f < m; ++f)
                for (int k = 0; k < K; ++k)
                {
                    const double e = emit(f, k);
                    double best = -1e18;
                    int arg = 0;
                    for (int p = 0; p < K; ++p)
                    {
                        const float hp = frames[i + f - 1].melodyCandHz[static_cast<std::size_t>(p)];
                        const float hk = frames[i + f].melodyCandHz[static_cast<std::size_t>(k)];
                        if (hp <= 0.0f || hk <= 0.0f)
                            continue;
                        const double v = score[f - 1][static_cast<std::size_t>(p)] - par.jump * std::abs(midi(hk) - midi(hp));
                        if (v > best) { best = v; arg = p; }
                    }
                    score[f][static_cast<std::size_t>(k)] = (best < -1e17 ? -1e9 : best) + e;
                    back[f][static_cast<std::size_t>(k)] = arg;
                }
            int k = static_cast<int>(std::max_element(score[m - 1].begin(), score[m - 1].end()) - score[m - 1].begin());
            for (std::size_t f = m; f-- > 0;)
            {
                hz[i + f] = frames[i + f].melodyCandHz[static_cast<std::size_t>(k)];
                sal[i + f] = frames[i + f].melodyCandSalience[static_cast<std::size_t>(k)];
                if (f > 0)
                    k = back[f][static_cast<std::size_t>(k)];
            }
        }
        i = j;
    }
}

void bassChromaFromSpectrum(const std::vector<double>& magLong, double sampleRate, std::array<float, 12>& out) noexcept
{
    out.fill(0.0f);
    const auto bins = magLong.size();
    const double binHz = sampleRate / static_cast<double>(kHarmonyFrameSize);
    const auto kLo = std::max<std::size_t>(2, static_cast<std::size_t>(std::floor(45.0 / binHz)));
    const auto kHi = std::min(bins - 2, static_cast<std::size_t>(std::ceil(260.0 / binHz)));
    double maxMag = 0.0;
    for (std::size_t k = kLo; k <= kHi; ++k)
        maxMag = std::max(maxMag, magLong[k]);
    if (maxMag <= 0.0)
        return;
    std::array<double, 12> c {};
    double total = 0.0;
    for (std::size_t k = kLo; k <= kHi; ++k)
    {
        const double a = magLong[k - 1], b = magLong[k], d = magLong[k + 1];
        if (b <= a || b < d || b < 0.05 * maxMag)
            continue;
        const double den = a - 2.0 * b + d;
        const double hz = (static_cast<double>(k) + (den != 0.0 ? 0.5 * (a - d) / den : 0.0)) * binHz;
        if (hz < 45.0 || hz > 260.0)
            continue;
        const double midi = 69.0 + 12.0 * std::log2(hz / 440.0);
        const double w = b * 130.0 / hz;
        c[static_cast<std::size_t>(((std::lround(midi) % 12) + 12) % 12)] += w;
        total += w;
    }
    if (total > 0.0)
        for (std::size_t i = 0; i < 12; ++i)
            out[i] = static_cast<float>(c[i] / total);
}

Analyzer::Analyzer(double sampleRate, AnalyzerConfig config)
    : sampleRate_(sampleRate), config_(config), frameAnalyzer_(sampleRate, config)
{
    buffer_.assign(static_cast<std::size_t>(config_.frameSize), 0.0f);
    const auto ring = static_cast<std::size_t>(2 * std::max(kHpssHalfFrames, kHpssHalfFramesLong) + 1);
    shortRing_.assign(ring, std::vector<double>(static_cast<std::size_t>(config_.frameSize) / 2 + 1, 0.0));
    longRing_.assign(ring, std::vector<double>(static_cast<std::size_t>(kHarmonyFrameSize) / 2 + 1, 0.0));
    longOk_.assign(ring, 0);
    prevPercLog_.assign(static_cast<std::size_t>(config_.frameSize) / 2 + 1, 0.0);
    masked_.assign(static_cast<std::size_t>(kHarmonyFrameSize) / 2 + 1, 0.0);
    column_.reserve(512);
}

namespace
{
double median(std::vector<double>& v)
{
    if (v.empty())
        return 0.0;
    const auto mid = v.begin() + static_cast<std::ptrdiff_t>(v.size() / 2);
    std::nth_element(v.begin(), mid, v.end());
    return *mid;
}
} // namespace

void Analyzer::separate(std::size_t j)
{
    const auto ring = shortRing_.size();
    const auto half = static_cast<std::size_t>(kHpssHalfFrames);
    std::size_t lo = j >= half ? j - half : 0;
    std::size_t hi = std::min(frames_.size() - 1, j + half);
    Frame& f = frames_[j];
    const auto slot = [&](std::size_t i) { return i % ring; };

    // 1. curto: a parte percussiva e o seu fluxo
    {
        const auto& cur = shortRing_[slot(j)];
        const auto bins = cur.size();
        const double binHz = sampleRate_ / static_cast<double>(config_.frameSize);
        const auto fw = static_cast<std::size_t>(std::max(1.0, std::round(kHpssHalfHz / binHz)));
        double flux = 0.0;
        for (std::size_t k = 0; k < bins; ++k)
        {
            column_.clear();
            for (std::size_t i = lo; i <= hi; ++i)
                column_.push_back(shortRing_[slot(i)][k]);
            const double H = median(column_);
            column_.clear();
            for (std::size_t q = (k >= fw ? k - fw : 0); q <= std::min(bins - 1, k + fw); ++q)
                column_.push_back(cur[q]);
            const double P = median(column_);
            const double mask = P * P / (H * H + P * P + 1e-18);
            if (!f.silent)
            {
                percSum_ += cur[k] * mask;
                magSum_ += cur[k];
            }
            const double lm = std::log1p(100.0 * cur[k] * mask);
            if (j > 0)
                flux += std::max(0.0, lm - prevPercLog_[k]);
            prevPercLog_[k] = lm;
        }
        f.percFlux = f.silent ? 0.0f : static_cast<float>(flux / static_cast<double>(bins));
    }

    // 2. longo: o croma só da parte harmônica (110 Hz–5 kHz, com folga para
    //    a interpolação dos picos)
    const auto halfLong = static_cast<std::size_t>(kHpssHalfFramesLong);
    lo = j >= halfLong ? j - halfLong : 0;
    hi = std::min(frames_.size() - 1, j + halfLong);
    if (!f.silent && longOk_[slot(j)])
    {
        const auto& cur = longRing_[slot(j)];
        const auto bins = cur.size();
        const double binHz = sampleRate_ / static_cast<double>(kHarmonyFrameSize);
        const auto kLo = static_cast<std::size_t>(std::max(2.0, std::floor(40.0 / binHz)));
        const auto kHi = std::min(bins - 1, static_cast<std::size_t>(std::ceil(5200.0 / binHz)));
        std::fill(masked_.begin(), masked_.end(), 0.0);
        for (std::size_t k = kLo; k <= kHi; ++k)
        {
            column_.clear();
            for (std::size_t i = lo; i <= hi; ++i)
                column_.push_back(longOk_[slot(i)] ? longRing_[slot(i)][k] : 0.0);
            const double H = median(column_);
            // o harmônico é o que se SUSTENTA: a própria mediana no tempo
            // (limitada pelo valor atual). A máscara de Wiener deixava passar
            // golpes curtos com altura (um tom afinado é estreito na
            // frequência, então também não é "percussivo"); a mediana pura os
            // tira. O preço, apagar notas rápidas, é a razão da decisão por
            // peça em finish().
            masked_[k] = std::min(cur[k], H);
        }
        if (harmonic_.size() <= j)
        {
            harmonic_.resize(j + 1, std::array<float, 12> {});
            harmonicBass_.resize(j + 1, std::array<float, 12> {});
        }
        chromaFromSpectrum(masked_, sampleRate_, harmonic_[j]);
        bassChromaFromSpectrum(masked_, sampleRate_, harmonicBass_[j]);
    }
}

void Analyzer::push(const float* mono, std::size_t count)
{
    const auto n = static_cast<std::size_t>(config_.frameSize);
    const auto hop = static_cast<std::size_t>(config_.hopSize);
    pushed_ += count;
    while (count > 0)
    {
        const auto take = std::min(count, n - filled_);
        std::copy(mono, mono + take, buffer_.begin() + static_cast<std::ptrdiff_t>(filled_));
        filled_ += take;
        mono += take;
        count -= take;
        if (filled_ == n)
        {
            analyzeFrame();
            std::copy(buffer_.begin() + static_cast<std::ptrdiff_t>(hop), buffer_.end(), buffer_.begin());
            filled_ = n - hop;
            consumed_ += hop;
        }
    }
}

void Analyzer::finish()
{
    if (finished_)
        return;
    finished_ = true;
    const auto n = static_cast<std::size_t>(config_.frameSize);
    const auto hop = static_cast<std::size_t>(config_.hopSize);
    // Fecha quadros enquanto houver amostra real que nenhum quadro cobriu inteira.
    while (consumed_ + n - hop < pushed_ || frames_.empty())
    {
        std::fill(buffer_.begin() + static_cast<std::ptrdiff_t>(filled_), buffer_.end(), 0.0f);
        filled_ = n;
        analyzeFrame();
        std::copy(buffer_.begin() + static_cast<std::ptrdiff_t>(hop), buffer_.end(), buffer_.begin());
        filled_ = n - hop;
        consumed_ += hop;
        if (pushed_ == 0)
            break;
    }
    while (config_.hpss && separated_ < frames_.size())
        separate(separated_++);
    // a linha da melodia pelos candidatos (offline: olha a peça inteira)
    {
        std::vector<float> mh, ms;
        trackMelody(frames_, mh, ms);
        for (std::size_t j = 0; j < frames_.size(); ++j)
            if (mh[j] > 0.0f)
            {
                frames_[j].melodyHz = mh[j];
                frames_[j].melodySalience = ms[j];
            }
    }
    if (config_.hpss && magSum_ > 0.0)
    {
        percussiveShare_ = static_cast<float>(percSum_ / magSum_);
        harmonicChroma_ = percussiveShare_ >= kPercussiveShareForHarmonicChroma;
        if (harmonicChroma_)
            for (std::size_t j = 0; j < frames_.size() && j < harmonic_.size(); ++j)
            {
                float total = 0.0f;
                for (float x : harmonic_[j])
                    total += x;
                if (total > 0.0f)  // sem picos sustentados, fica o croma do espectro inteiro
                    frames_[j].chroma = harmonic_[j];
                // o grave sustentado: sem o bumbo (pode ficar vazio: sem baixo)
                frames_[j].bassChroma = harmonicBass_[j];
            }
    }
    pickOnsets();
}

Frame FrameAnalyzer::analyze(const float* samples, double time) noexcept
{
    const auto n = static_cast<std::size_t>(config_.frameSize);
    const auto bins = n / 2 + 1;

    Frame f;
    f.time = time;

    double power = 0.0;
    for (std::size_t i = 0; i < n; ++i)
        power += static_cast<double>(samples[i]) * samples[i];
    f.rmsDb = toDb(power / static_cast<double>(n));
    f.silent = f.rmsDb <= config_.silenceDb;

    short_.magnitudes(samples, mag_);
    longValid_ = false;

    // histórico da janela longa: entra o trecho novo (o quadro inteiro na
    // primeira chamada, depois só o último salto)
    const auto fresh = havePrev_ ? static_cast<std::size_t>(config_.hopSize) : n;
    std::copy(history_.begin() + static_cast<std::ptrdiff_t>(fresh), history_.end(), history_.begin());
    std::copy(samples + (n - fresh), samples + n, history_.end() - static_cast<std::ptrdiff_t>(fresh));

    // fluxo: subidas do log-magnitude (log(1 + 100·|X|)), média por bin; e
    // o mesmo só na banda grave (< 200 Hz) e só na aguda (> 2 kHz)
    double flux = 0.0, fLow = 0.0, fHigh = 0.0;
    const double binHzShort = sampleRate_ / static_cast<double>(n);
    const auto kLowEnd = static_cast<std::size_t>(200.0 / binHzShort);
    const auto kHighStart = static_cast<std::size_t>(2000.0 / binHzShort);
    for (std::size_t k = 0; k < bins; ++k)
    {
        const double lm = std::log1p(100.0 * mag_[k]);
        if (havePrev_)
        {
            const double up = std::max(0.0, lm - prevLogMag_[k]);
            flux += up;
            if (k >= 1 && k <= kLowEnd) fLow += up;
            if (k >= kHighStart) fHigh += up;
        }
        prevLogMag_[k] = lm;
    }
    if (!f.silent)
    {
        f.fluxLow = static_cast<float>(fLow / static_cast<double>(std::max<std::size_t>(1, kLowEnd)));
        f.fluxHigh = static_cast<float>(fHigh / static_cast<double>(std::max<std::size_t>(1, bins - kHighStart)));
    }
    havePrev_ = true;
    f.flux = f.silent ? 0.0f : static_cast<float>(flux / static_cast<double>(bins));

    if (!f.silent)
    {
        const double binHz = sampleRate_ / static_cast<double>(n);
        double sumMag = 0.0, sumFMag = 0.0, sumPow = 0.0, sumLogPow = 0.0, peakPow = 0.0;
        for (std::size_t k = 1; k < bins; ++k)  // sem o DC
        {
            const double m = mag_[k];
            const double hz = static_cast<double>(k) * binHz;
            sumMag += m;
            sumFMag += hz * m;
            peakPow = std::max(peakPow, m * m);
        }
        // Planura entre 50 Hz e 10 kHz, com piso de −80 dB do pico do quadro.
        // Codecs com perda (MP3, AAC, Ogg) zeram o agudo acima de ~16 kHz e
        // abrem buracos no espectro; na média geométrica um zero derruba a
        // planura a ~0, e ruído aparecia como tom (achado em 2026-10-08, ao
        // aceitar MP3). Ruído branco continua em ≈ 0,56 nesta faixa.
        const auto kLo = std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(50.0 / binHz)));
        const auto kHi = std::min(bins - 1, static_cast<std::size_t>(10000.0 / binHz));
        const double floorPow = peakPow * 1e-8 + 1e-20;
        for (std::size_t k = kLo; k <= kHi; ++k)
        {
            const double p = std::max(mag_[k] * mag_[k], floorPow);
            sumPow += p;
            sumLogPow += std::log(p);
        }
        const double c = sumMag > 0.0 ? sumFMag / sumMag : 0.0;
        double var = 0.0;
        for (std::size_t k = 1; k < bins; ++k)
        {
            const double d = static_cast<double>(k) * binHz - c;
            var += d * d * mag_[k];
        }
        const auto count = static_cast<double>(kHi >= kLo ? kHi - kLo + 1 : 1);
        f.centroidHz = static_cast<float>(c);
        f.spreadHz = static_cast<float>(sumMag > 0.0 ? std::sqrt(var / sumMag) : 0.0);
        f.flatness = static_cast<float>(std::clamp(std::exp(sumLogPow / count) / (sumPow / count), 0.0, 1.0));

        // Altura fundamental por YIN (de Cheveigné e Kawahara, 2002) sobre as
        // últimas 4096 amostras reduzidas a ~12 kHz: 85 ms cobrem 4 períodos
        // de 50 Hz. Diferença quadrática, média cumulativa normalizada,
        // primeiro mínimo abaixo de 0,15 e interpolação parabólica.
        {
            const auto m = yinX_.size();
            const auto D = static_cast<std::size_t>(decim_);
            const std::size_t base = history_.size() - m * D;
            for (std::size_t i = 0; i < m; ++i)
            {
                double acc = 0.0;
                for (std::size_t j = 0; j < D; ++j)
                    acc += history_[base + i * D + j];
                yinX_[i] = acc / static_cast<double>(D);
            }
            const double rate = sampleRate_ / static_cast<double>(D);
            const auto tauMin = static_cast<std::size_t>(std::floor(rate / 1000.0));
            const auto tauMax = std::min(m / 3, static_cast<std::size_t>(std::ceil(rate / 50.0)));
            const std::size_t W = m - tauMax;
            yinD_[0] = 1.0;
            double running = 0.0;
            for (std::size_t tau = 1; tau <= tauMax; ++tau)
            {
                double d = 0.0;
                for (std::size_t j = 0; j < W; ++j)
                {
                    const double diff = yinX_[j] - yinX_[j + tau];
                    d += diff * diff;
                }
                running += d;
                yinD_[tau] = running > 0.0 ? d * static_cast<double>(tau) / running : 1.0;
            }
            std::size_t best = 0;
            for (std::size_t tau = std::max<std::size_t>(tauMin, 2); tau + 1 <= tauMax; ++tau)
                if (yinD_[tau] < 0.15 && yinD_[tau] <= yinD_[tau + 1])
                {
                    best = tau;
                    break;
                }
            if (best == 0)  // nenhum abaixo do limiar: o mínimo global, se for razoável
            {
                double mn = 1e9;
                for (std::size_t tau = std::max<std::size_t>(tauMin, 2); tau + 1 <= tauMax; ++tau)
                    if (yinD_[tau] < mn)
                    {
                        mn = yinD_[tau];
                        best = tau;
                    }
                if (mn > 0.35)
                    best = 0;
            }
            if (best > 1 && best < tauMax)
            {
                const double a = yinD_[best - 1], b = yinD_[best], c = yinD_[best + 1];
                const double den = a - 2.0 * b + c;
                const double tau = static_cast<double>(best) + (den > 0.0 ? 0.5 * (a - c) / den : 0.0);
                f.f0Hz = static_cast<float>(rate / tau);
                f.periodicity = static_cast<float>(std::clamp(1.0 - b, 0.0, 1.0));
            }
        }

        // Picos e croma: janela longa (kHarmonyFrameSize), que separa semitons graves.
        long_.magnitudes(history_.data(), magLong_);
        longValid_ = true;
        const auto binsLong = magLong_.size();
        const double binHzLong = sampleRate_ / static_cast<double>(kHarmonyFrameSize);
        // picos: máximos locais acima de 1 % do maior, os kPeaks mais fortes
        double maxMag = 0.0;
        for (std::size_t k = 1; k < binsLong; ++k)
            maxMag = std::max(maxMag, magLong_[k]);
        std::array<std::pair<double, double>, kPeaks> best {};  // (magnitude, Hz)
        for (std::size_t k = 2; k + 1 < binsLong; ++k)
        {
            const double a = magLong_[k - 1], b = magLong_[k], c = magLong_[k + 1];
            if (b <= a || b < c || b < 0.01 * maxMag)
                continue;
            const double den = a - 2.0 * b + c;
            const double delta = den != 0.0 ? 0.5 * (a - c) / den : 0.0;  // −0,5..0,5 bin
            const double hz = (static_cast<double>(k) + delta) * binHzLong;
            auto weakest = std::min_element(best.begin(), best.end());
            if (b > weakest->first)
                *weakest = { b, hz };
        }
        std::sort(best.begin(), best.end(), [](const auto& x, const auto& y) { return x.first > y.first; });

        chromaFromSpectrum(magLong_, sampleRate_, f.chroma);
        bassChromaFromSpectrum(magLong_, sampleRate_, f.bassChroma);
        predominantCandidates(magLong_, sampleRate_, salience_, f.melodyCandHz, f.melodyCandSalience);
        f.melodyHz = f.melodyCandHz[0];
        f.melodySalience = f.melodyCandSalience[0];
        for (std::size_t i = 0; i < static_cast<std::size_t>(kPeaks); ++i)
            f.peaksHz[i] = static_cast<float>(best[i].second);
    }
    return f;
}

void Analyzer::analyzeFrame()
{
    const double n = static_cast<double>(config_.frameSize);
    frames_.push_back(frameAnalyzer_.analyze(buffer_.data(), (static_cast<double>(consumed_) + n / 2.0) / sampleRate_));
    const auto s = (frames_.size() - 1) % shortRing_.size();
    shortRing_[s] = frameAnalyzer_.shortMagnitudes();
    longOk_[s] = frameAnalyzer_.longValid() ? 1 : 0;
    if (longOk_[s])
        longRing_[s] = frameAnalyzer_.longMagnitudes();
    // separa cada quadro quando os seus kHpssHalfFrames seguintes chegaram
    while (config_.hpss && separated_ + static_cast<std::size_t>(std::max(kHpssHalfFrames, kHpssHalfFramesLong)) < frames_.size())
        separate(separated_++);
}

// Ataque = pico local do fluxo (±3 quadros) acima de 1,5 × a mediana local
// (±10 quadros) + 0,02, respeitando o intervalo mínimo. Offline: olha para
// frente e para trás; a versão ao vivo virá com a janela curta.
void Analyzer::pickOnsets()
{
    const auto count = frames_.size();
    if (count < 3)
        return;
    const double frameSeconds = static_cast<double>(config_.hopSize) / sampleRate_;
    const auto gap = static_cast<std::ptrdiff_t>(std::ceil(config_.minOnsetGapSeconds / frameSeconds));
    std::vector<float> local;
    local.reserve(2 * kOnsetMedianHalfWidth + 1);
    std::ptrdiff_t last = -gap - 1;
    for (std::size_t i = 1; i < count; ++i)
    {
        const float v = frames_[i].flux;
        if (v <= 0.0f)
            continue;
        bool peak = true;
        const auto pw = static_cast<std::size_t>(kOnsetPeakHalfWidth);
        for (std::size_t j = (i >= pw ? i - pw : 0); j <= std::min(count - 1, i + pw) && peak; ++j)
            if (j != i && (frames_[j].flux > v || (frames_[j].flux == v && j < i)))
                peak = false;
        if (!peak)
            continue;
        local.clear();
        const auto mw = static_cast<std::size_t>(kOnsetMedianHalfWidth);
        for (std::size_t j = (i >= mw ? i - mw : 0); j <= std::min(count - 1, i + mw); ++j)
            local.push_back(frames_[j].flux);
        const float threshold = kOnsetMedianScale * percentile(local, 0.5) + kOnsetOffset;
        const auto ii = static_cast<std::ptrdiff_t>(i);
        if (v > threshold && ii - last >= gap)
        {
            onsets_.push_back({ frames_[i].time, v - threshold });
            last = ii;
        }
    }
}

std::vector<BandOnset> bandOnsets(const std::vector<Frame>& frames, double minGapSeconds)
{
    std::vector<BandOnset> out;
    if (frames.size() < 3)
        return out;
    const double hop = frames[1].time - frames[0].time;
    const auto gap = static_cast<std::ptrdiff_t>(std::ceil(minGapSeconds / hop));
    struct Pick { double t; float s; int band; };
    std::vector<Pick> picks;
    float norm[2] = { 0.0f, 0.0f };
    for (int band = 0; band < 2; ++band)
    {
        std::vector<float> v(frames.size());
        for (std::size_t i = 0; i < frames.size(); ++i)
            v[i] = band == 0 ? frames[i].fluxLow : frames[i].fluxHigh;
        const float p90 = percentile(v, 0.9);
        norm[band] = p90;
        if (p90 <= 0.0f)
            continue;
        std::vector<float> local;
        std::ptrdiff_t last = -gap - 1;
        const auto count = v.size();
        for (std::size_t i = 1; i < count; ++i)
        {
            const float x = v[i];
            if (x <= 0.0f)
                continue;
            bool peak = true;
            const auto pw = static_cast<std::size_t>(kOnsetPeakHalfWidth);
            for (std::size_t j = (i >= pw ? i - pw : 0); j <= std::min(count - 1, i + pw) && peak; ++j)
                if (j != i && (v[j] > x || (v[j] == x && j < i)))
                    peak = false;
            if (!peak)
                continue;
            local.clear();
            const auto mw = static_cast<std::size_t>(kOnsetMedianHalfWidth);
            for (std::size_t j = (i >= mw ? i - mw : 0); j <= std::min(count - 1, i + mw); ++j)
                local.push_back(v[j]);
            const float threshold = kOnsetMedianScale * percentile(local, 0.5) + 0.1f * p90;
            const auto ii = static_cast<std::ptrdiff_t>(i);
            if (x > threshold && ii - last >= gap)
            {
                picks.push_back({ frames[i].time, (x - threshold) / p90, band });
                last = ii;
            }
        }
    }
    std::sort(picks.begin(), picks.end(), [](const Pick& a, const Pick& b) { return a.t < b.t; });
    // o nível de cada banda no ataque: o maior fluxo da banda a ±1 quadro
    const auto levelAt = [&](double t, int band) {
        const auto c = static_cast<long>(std::lround((t - frames.front().time) / hop));
        float m = 0.0f;
        for (long i = c - 1; i <= c + 1; ++i)
            if (i >= 0 && i < static_cast<long>(frames.size()))
                m = std::max(m, band == 0 ? frames[static_cast<std::size_t>(i)].fluxLow : frames[static_cast<std::size_t>(i)].fluxHigh);
        return norm[band] > 0.0f ? m / norm[band] : 0.0f;
    };
    for (const auto& p : picks)
    {
        if (!out.empty() && p.t - out.back().time <= 0.03)
        {
            auto& o = out.back();
            o.strength = std::max(o.strength, p.s);
            (p.band == 0 ? o.low : o.high) = true;
            continue;
        }
        BandOnset o;
        o.time = p.t;
        o.strength = p.s;
        (p.band == 0 ? o.low : o.high) = true;
        out.push_back(o);
    }
    for (auto& o : out)
    {
        o.lowLevel = levelAt(o.time, 0);
        o.highLevel = levelAt(o.time, 1);
    }
    return out;
}

Summary summarize(const std::vector<Frame>& frames, const std::vector<Onset>& onsets, double t0, double t1)
{
    Summary s;
    s.start = t0;
    s.end = t1;
    double power = 0.0, centroid = 0.0, flat = 0.0;
    std::size_t all = 0, active = 0;
    std::vector<float> levels;
    for (const auto& f : frames)
    {
        if (f.time < t0 || f.time >= t1)
            continue;
        ++all;
        power += std::pow(10.0, f.rmsDb / 10.0);
        if (f.silent)
            continue;
        ++active;
        centroid += std::log2(std::max(f.centroidHz, 1.0f));  // média em oitavas
        flat += f.flatness;
        levels.push_back(f.rmsDb);
    }
    for (const auto& o : onsets)
        if (o.time >= t0 && o.time < t1)
            ++s.onsetCount;
    if (all == 0)
        return s;
    s.meanRmsDb = toDb(power / static_cast<double>(all));
    s.energy = std::clamp((s.meanRmsDb + 60.0f) / 60.0f, 0.0f, 1.0f);
    s.activeShare = static_cast<float>(active) / static_cast<float>(all);
    const double span = std::max(t1 - t0, 1e-9);
    s.onsetsPerSecond = static_cast<float>(s.onsetCount / span);
    s.density = s.onsetsPerSecond / (s.onsetsPerSecond + 4.0f);
    if (active > 0)
    {
        s.centroidHz = static_cast<float>(std::exp2(centroid / static_cast<double>(active)));
        s.brightness = s.centroidHz > 0.0f
            ? std::clamp(static_cast<float>(std::log2(s.centroidHz / 100.0) / std::log2(80.0)), 0.0f, 1.0f) : 0.0f;
        s.noisiness = static_cast<float>(flat / static_cast<double>(active));
        s.dynamicRangeDb = percentile(levels, 0.95) - percentile(levels, 0.10);
    }
    return s;
}

std::vector<Summary> segment(const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                             double duration, double seconds)
{
    std::vector<Summary> out;
    if (seconds <= 0.0 || duration <= 0.0)
        return out;
    for (double t = 0.0; t < duration - 1e-9; t += seconds)
        out.push_back(summarize(frames, onsets, t, std::min(t + seconds, duration)));
    return out;
}

bool hasTimbre(const Summary& s) noexcept { return s.activeShare >= 0.2f; }

float correlation(const std::vector<Summary>& a, const std::vector<Summary>& b, float Summary::*scale)
{
    std::vector<double> x, y;
    for (std::size_t i = 1; i < std::min(a.size(), b.size()); ++i)
        if (hasTimbre(a[i]) && hasTimbre(b[i]))
        {
            x.push_back(a[i].*scale);
            y.push_back(b[i].*scale);
        }
    if (x.size() < 3)
        return 0.0f;
    const double mx = std::accumulate(x.begin(), x.end(), 0.0) / static_cast<double>(x.size());
    const double my = std::accumulate(y.begin(), y.end(), 0.0) / static_cast<double>(y.size());
    double sxy = 0.0, sxx = 0.0, syy = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i)
    {
        sxy += (x[i] - mx) * (y[i] - my);
        sxx += (x[i] - mx) * (x[i] - mx);
        syy += (y[i] - my) * (y[i] - my);
    }
    return sxx > 0.0 && syy > 0.0 ? static_cast<float>(sxy / std::sqrt(sxx * syy)) : 0.0f;
}

float contrast(const Summary& a, const Summary& b) noexcept
{
    const float de = a.energy - b.energy;
    const float dd = a.density - b.density;
    if (!hasTimbre(a) || !hasTimbre(b))  // trecho quase mudo: brilho e ruído não dizem nada
        return std::sqrt((de * de + dd * dd) / 2.0f);
    const float db = a.brightness - b.brightness;
    const float dn = a.noisiness - b.noisiness;
    return std::sqrt((de * de + dd * dd + db * db + dn * dn) / 4.0f);
}

} // namespace trioio::perception
