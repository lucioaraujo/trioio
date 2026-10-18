#include <array>
#include "Structure.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace trioio::perception
{
namespace
{
double hopOf(const std::vector<Frame>& f) { return f.size() > 1 ? f[1].time - f[0].time : 0.0; }
}

Pulse estimatePulse(const std::vector<Frame>& frames, const std::vector<Onset>& onsets, double t0, double t1,
                    PulseEnvelope envelope)
{
    Pulse p;
    const double hop = hopOf(frames);
    if (hop <= 0.0)
        return p;
    // o envelope do pulso: o fluxo da parte PERCUSSIVA quando o analisador
    // separou (v5, offline); senão o fluxo inteiro
    bool separated = false;
    for (const auto& f : frames)
        if (f.percFlux > 0.0f)
        {
            separated = true;
            break;
        }
    // o envelope do pulso: o fluxo da banda GRAVE mais o da AGUDA, cada um
    // ÷ o seu p90 (analisador v7). Bumbo e chimbal, dum e tak, entram com o
    // mesmo peso, e o que fica no meio (o pad, a voz) não puxa o pulso. Na
    // bancada de andamentos: 92/120 certos e 6 saltos de nível, contra 68 e
    // 21 com o fluxo percussivo (2026-10-10). Uma banda sem atividade fica de
    // fora; sem as duas, volta o fluxo percussivo (ou o inteiro).
    std::vector<double> e;
    {
        std::vector<float> lo, hi, fl;
        for (const auto& f : frames)
            if (f.time >= t0 && f.time < t1)
            {
                lo.push_back(f.fluxLow);
                hi.push_back(f.fluxHigh);
                fl.push_back(separated ? f.percFlux : f.flux);
            }
        const auto q90 = [](std::vector<float> v) {
            if (v.empty())
                return 0.0f;
            std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(v.size() * 9 / 10), v.end());
            return v[v.size() * 9 / 10];
        };
        const float nl = q90(lo), nh = q90(hi), nf = q90(fl);
        // "sem atividade": o p90 da banda abaixo de 5 % do p90 do fluxo inteiro
        const bool bands = envelope == PulseEnvelope::bands;
        const bool useLow = bands && nl > 0.05f * nf && nl > 0.0f, useHigh = bands && nh > 0.05f * nf && nh > 0.0f;
        for (std::size_t i = 0; i < fl.size(); ++i)
            e.push_back(useLow || useHigh ? (useLow ? lo[i] / nl : 0.0) + (useHigh ? hi[i] / nh : 0.0) : fl[i]);
    }
    // Suaviza o envelope (triangular, duas passadas de 1-2-1): o fluxo é um
    // pico de um quadro, e um período que não cai em número inteiro de
    // quadros (0,5 s = 23,4) perderia a autocorrelação para um múltiplo que
    // por acaso cai quase inteiro (achado: metrônomo a 120 lido como 97,6).
    for (int pass = 0; pass < 2 && e.size() > 2; ++pass)
    {
        std::vector<double> s(e.size());
        for (std::size_t i = 0; i < e.size(); ++i)
            s[i] = 0.5 * e[i] + 0.25 * (e[i > 0 ? i - 1 : i] + e[i + 1 < e.size() ? i + 1 : i]);
        e.swap(s);
    }
    const double mean = e.empty() ? 0.0 : std::accumulate(e.begin(), e.end(), 0.0) / static_cast<double>(e.size());
    for (auto& v : e)
        v -= mean;  // sem a média, a autocorrelação mede só a periodicidade
    const auto lagMin = static_cast<std::size_t>(std::floor(60.0 / 200.0 / hop));
    const auto lagMax = static_cast<std::size_t>(std::ceil(60.0 / 50.0 / hop));
    if (e.size() < lagMax * 3)
        return p;

    const auto ac = [&](std::size_t lag) {
        double s = 0.0;
        for (std::size_t i = 0; i + lag < e.size(); ++i)
            s += e[i] * e[i + lag];
        return s / static_cast<double>(e.size() - lag);
    };
    const double ac0 = ac(0);
    if (ac0 <= 0.0)
        return p;
    std::vector<double> a(lagMax + 2, 0.0);
    for (std::size_t L = lagMin > 0 ? lagMin - 1 : 0; L <= lagMax + 1; ++L)
        a[L] = ac(L);
    std::size_t best = lagMin;
    double bestScore = -1e300, sum = 0.0;
    for (std::size_t L = lagMin; L <= lagMax; ++L)
    {
        const double bpm = 60.0 / (static_cast<double>(L) * hop);
        // a preferência perceptiva pelo pulso de contagem: centrada em 100
        // BPM (600 ms). A saliência do pulso é máxima entre 600 e 700 ms
        // (R. Parncutt, "A perceptual model of pulse salience and metrical
        // accent in musical rhythms", Music Perception 11(4), 1994; J. London,
        // Hearing in Time, 2004). Centrada em 120, empatava 86 e 172, e a
        // faixa do Suno era lida a 172 quando o autor conta a 86
        // (2026-10-09).
        const double prior = std::exp(-0.5 * std::pow(std::log2(bpm / 100.0), 2.0));
        const double score = a[L] * prior;
        sum += a[L];
        if (score > bestScore)
        {
            bestScore = score;
            best = L;
        }
    }
    // atraso fracionário por interpolação parabólica
    double lag = static_cast<double>(best);
    if (best > 0 && best + 1 < a.size())
    {
        const double den = a[best - 1] - 2.0 * a[best] + a[best + 1];
        if (den < 0.0)
            lag += 0.5 * (a[best - 1] - a[best + 1]) / den;
    }
    p.period = lag * hop;
    p.bpm = 60.0 / p.period;
    const double meanAc = sum / static_cast<double>(lagMax - lagMin + 1);
    p.confidence = static_cast<float>(std::clamp((a[best] - meanAc) / (ac0 - meanAc + 1e-12), 0.0, 1.0));

    // Ciclo do compasso: a periodicidade longa (1,2–6 s) em que o padrão se
    // repete. Se o tempo não divide o compasso em partes inteiras (2, 3, 4,
    // 6, 8), a leitura do tempo era de um agrupamento interno: um 3+3+2 a
    // 120 BPM soa como semínima pontuada (80 BPM), mas se repete a cada 2 s.
    // Nesse caso o tempo passa a ser o compasso ÷ 4.
    {
        const auto bMin = static_cast<std::size_t>(std::floor(1.2 / hop));
        const auto bMax = std::min(e.size() / 3, static_cast<std::size_t>(std::ceil(6.0 / hop)));
        // máximo com os vizinhos (±1 quadro): um período que não cai num
        // número inteiro de quadros (0,5 s = 23,4 quadros) não pode parecer
        // mais fraco que um múltiplo que por acaso cai quase inteiro
        const auto acNear = [&](std::size_t L) { return std::max({ ac(L - 1), ac(L), ac(L + 1) }); };
        std::vector<double> bac;
        double bMaxVal = 0.0;
        for (std::size_t L = bMin; L <= bMax; ++L)
        {
            bac.push_back(acNear(L));
            bMaxVal = std::max(bMaxVal, bac.back());
        }
        // o MENOR atraso com força perto da máxima: num pulso regular todo
        // múltiplo é periódico, e o compasso é o primeiro
        std::size_t bBest = 0;
        double bVal = 0.0;
        for (std::size_t i = 0; i < bac.size(); ++i)
            if (bac[i] >= 0.9 * bMaxVal)
            {
                bBest = bMin + i;
                bVal = bac[i];
                break;
            }
        // só corrige o tempo se o compasso for CLARAMENTE mais periódico que
        // ele (num metrônomo os dois empatam e o tempo fica)
        if (bBest > 0 && bVal > 0.3 * ac0 && bVal > 1.25 * acNear(best))
        {
            p.barPeriod = static_cast<double>(bBest) * hop;
            const double ratio = p.barPeriod / p.period;
            const double nearest = std::round(ratio);
            const bool whole = std::abs(ratio - nearest) < 0.12 * nearest / 4.0 + 0.08 && nearest >= 2 && nearest <= 8;
            if (whole)
            {
                // o compasso tem acento próprio: quantos tempos ele tem
                const int n = static_cast<int>(nearest);
                // 2 a 7 tempos; 8 é frase de dois compassos de 4. Um 0-6-8 em
                // 12 passos é ambíguo entre 3/4 (3 × 0,5 s) e 6/8 (2 × 0,75 s):
                // as duas leituras dão o mesmo ciclo de compasso, que é o que a
                // composição usa.
                p.beatsPerBar = n == 8 ? 4 : n;
            }
            else
            {
                p.period = p.barPeriod / 4.0;
                p.bpm = 60.0 / p.period;
                // a grade agora se apoia no compasso: a confiança é a dele
                p.confidence = std::max(p.confidence, static_cast<float>(std::clamp(
                    (bVal - meanAc) / (ac0 - meanAc + 1e-12), 0.0, 1.0)));
            }
        }
    }

    // fase: o deslocamento cuja grade de tempos soma mais fluxo
    double bestPhase = 0.0, bestSum = -1e300;
    const auto steps = static_cast<int>(std::ceil(p.period / hop));
    for (int s = 0; s < steps; ++s)
    {
        const double ph = s * hop;
        double tot = 0.0;
        for (double t = ph; t < (t1 - t0); t += p.period)
        {
            const auto i = static_cast<std::size_t>(std::lround(t / hop));
            if (i < e.size())
                tot += e[i];
        }
        if (tot > bestSum)
        {
            bestSum = tot;
            bestPhase = ph;
        }
    }
    // e[i] corresponde ao quadro de tempo t0 + (primeiro quadro) + i·hop
    double firstTime = t0;
    for (const auto& f : frames)
        if (f.time >= t0)
        {
            firstTime = f.time;
            break;
        }
    p.phase = std::fmod(firstTime + bestPhase, p.period);

    // (A fase pela média circular dos ataques foi retirada em 2026-10-11:
    // os ataques fora do tempo, como o bumbo antecipado da bossa e o chimbal
    // em tercina do shuffle, puxavam a média, e a fase saía errada em 0/15
    // faixas da bancada nos dois estilos. A grade que soma mais envelope,
    // agora das bandas, acerta 15/15 no shuffle; ver
    // docs/LEITURA_E_GABARITO.md.)

    // Refinamento: período e fase juntos, por regressão dos ataques contra o
    // índice do tempo mais próximo. A autocorrelação erra ~0,5 % no período,
    // o que em 20 s vira ~100 ms de deriva e estraga a fase.
    for (int pass = 0; pass < 3; ++pass)
    {
        double sw = 0, sk = 0, st = 0, skk = 0, skt = 0;
        int used = 0;
        for (const auto& o : onsets)
        {
            if (o.time < t0 || o.time >= t1)
                continue;
            const double k = std::round((o.time - p.phase) / p.period);
            if (std::abs(o.time - (p.phase + k * p.period)) > p.period / 4.0)
                continue;  // fora do tempo (contratempo, ornamento): não puxa a grade
            const double w = o.strength;
            sw += w; sk += w * k; st += w * o.time; skk += w * k * k; skt += w * k * o.time;
            ++used;
        }
        const double den = sw * skk - sk * sk;
        if (used < 4 || den <= 0.0)
            break;
        const double b = (sw * skt - sk * st) / den;
        const double a = (st - b * sk) / sw;
        if (b <= 0.0 || std::abs(b - p.period) > 0.05 * p.period)
            break;  // a regressão não pode trocar de andamento, só afinar
        p.period = b;
        p.bpm = 60.0 / b;
        p.phase = std::fmod(std::fmod(a, b) + b, b);
    }
    // o compasso de 3: quando o ciclo do compasso não foi medido (o 4 é só
    // o padrão), o grave e a harmonia decidem entre 3 e 4 (estimateMeter;
    // na bancada, a valsa de 0/16 a 13/16, sem perder nenhum 4/4)
    if (p.beatsPerBar == 4 && p.barPeriod <= 0.0 && p.period > 0.0)
        if (const auto m = estimateMeter(frames, p, t0, t1); m.beatsPerBar == 3)
            p.beatsPerBar = 3;
    return p;
}

Downbeat estimateDownbeat(const std::vector<Frame>& frames, const Pulse& pulse, double t0, double t1)
{
    Downbeat d;
    d.time = pulse.phase;
    const int bpb = std::max(1, pulse.beatsPerBar);
    if (pulse.period <= 0.0 || frames.size() < 2 || bpb < 2)
        return d;
    const double hop = frames[1].time - frames[0].time;
    const auto idx = [&](double t) {
        return std::clamp(static_cast<long>(std::lround((t - frames.front().time) / hop)), 0L, static_cast<long>(frames.size()) - 1);
    };
    std::vector<float> lows, highs;
    for (const auto& f : frames)
        if (f.time >= t0 && f.time < t1)
        {
            lows.push_back(f.fluxLow);
            highs.push_back(f.fluxHigh);
        }
    if (lows.empty())
        return d;
    const auto q90 = [](std::vector<float>& v) {
        std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(v.size() * 9 / 10), v.end());
        return std::max(1e-9f, v[v.size() * 9 / 10]);
    };
    const float p90 = q90(lows), p90h = q90(highs);
    // por tempo: o grave no ataque (±40 ms) e a mudança de croma
    std::vector<double> lowAt, change;
    std::array<double, 12> prev {};
    bool havePrev = false;
    const double first = pulse.phase + std::ceil((t0 - pulse.phase) / pulse.period - 1e-9) * pulse.period;
    for (double t = first; t + pulse.period <= t1; t += pulse.period)
    {
        // o grave DOMINANTE: o bumbo é grave sem agudo; a caixa (ruído largo)
        // acende as duas bandas e não deve contar como o 1
        float lo = 0.0f, hi = 0.0f;
        for (long i = idx(t - 0.04); i <= idx(t + 0.04); ++i)
        {
            lo = std::max(lo, frames[static_cast<std::size_t>(i)].fluxLow);
            hi = std::max(hi, frames[static_cast<std::size_t>(i)].fluxHigh);
        }
        lowAt.push_back(std::clamp(static_cast<double>(lo / p90 - hi / p90h), 0.0, 2.0));
        std::array<double, 12> c {};
        for (long i = idx(t); i < idx(t + pulse.period); ++i)
            if (!frames[static_cast<std::size_t>(i)].silent)
                for (std::size_t k = 0; k < 12; ++k)
                    c[k] += frames[static_cast<std::size_t>(i)].chroma[k];
        double dot = 0.0, na = 0.0, nb = 0.0;
        for (std::size_t k = 0; k < 12; ++k) { dot += c[k] * prev[k]; na += c[k] * c[k]; nb += prev[k] * prev[k]; }
        change.push_back(havePrev && na > 0.0 && nb > 0.0 ? 1.0 - dot / std::sqrt(na * nb) : 0.0);
        prev = c;
        havePrev = true;
    }
    if (lowAt.size() < static_cast<std::size_t>(2 * bpb))
        return d;
    // normaliza a mudança de croma pela sua média (0 se não há harmonia)
    double meanChange = 0.0;
    for (double x : change) meanChange += x;
    meanChange /= static_cast<double>(change.size());
    std::vector<double> score(static_cast<std::size_t>(bpb), 0.0);
    std::vector<int> count(static_cast<std::size_t>(bpb), 0);
    for (std::size_t k = 0; k < lowAt.size(); ++k)
    {
        const auto b = k % static_cast<std::size_t>(bpb);
        score[b] += lowAt[k] + (meanChange > 1e-3 ? change[k] / meanChange : 0.0);
        ++count[b];
    }
    for (std::size_t b = 0; b < score.size(); ++b)
        score[b] /= std::max(1, count[b]);
    std::size_t best = 0, second = 1;
    for (std::size_t b = 0; b < score.size(); ++b)
        if (score[b] > score[best]) best = b;
    second = best == 0 ? 1 : 0;
    for (std::size_t b = 0; b < score.size(); ++b)
        if (b != best && score[b] > score[second]) second = b;
    d.offsetBeats = static_cast<int>(best);
    d.time = first + static_cast<double>(best) * pulse.period;
    d.confidence = score[best] > 0.0 ? static_cast<float>(std::clamp((score[best] - score[second]) / score[best], 0.0, 1.0)) : 0.0f;
    return d;
}

Meter estimateMeter(const std::vector<Frame>& frames, const Pulse& pulse, double t0, double t1, double margin)
{
    Meter m;
    if (pulse.period <= 0.0 || frames.size() < 2)
        return m;
    const double hop = frames[1].time - frames[0].time;
    const auto idx = [&](double t) {
        return std::clamp(static_cast<long>(std::lround((t - frames.front().time) / hop)), 0L, static_cast<long>(frames.size()) - 1);
    };
    std::vector<float> lows, highs;
    for (const auto& f : frames)
        if (f.time >= t0 && f.time < t1) { lows.push_back(f.fluxLow); highs.push_back(f.fluxHigh); }
    if (lows.empty())
        return m;
    const auto q90 = [](std::vector<float>& v) {
        std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(v.size() * 9 / 10), v.end());
        return std::max(1e-9f, v[v.size() * 9 / 10]);
    };
    const float pl = q90(lows), ph = q90(highs);
    std::vector<double> low;
    std::vector<std::array<double, 12>> chroma;
    const double first = pulse.phase + std::ceil((t0 - pulse.phase) / pulse.period - 1e-9) * pulse.period;
    for (double t = first; t + pulse.period <= t1; t += pulse.period)
    {
        float lo = 0.0f, hi = 0.0f;
        for (long i = idx(t - 0.04); i <= idx(t + 0.04); ++i)
        {
            lo = std::max(lo, frames[static_cast<std::size_t>(i)].fluxLow);
            hi = std::max(hi, frames[static_cast<std::size_t>(i)].fluxHigh);
        }
        low.push_back(std::clamp(static_cast<double>(lo / pl - hi / ph), 0.0, 2.0));
        std::array<double, 12> c {};
        for (long i = idx(t); i < idx(t + pulse.period); ++i)
            if (!frames[static_cast<std::size_t>(i)].silent)
                for (std::size_t k = 0; k < 12; ++k)
                    c[k] += frames[static_cast<std::size_t>(i)].chroma[k];
        chroma.push_back(c);
    }
    if (low.size() < 16)
        return m;
    const auto corrAt = [&](std::size_t lag) {
        double ma = 0.0, mb = 0.0;
        const auto n = low.size() - lag;
        for (std::size_t k = 0; k < n; ++k) { ma += low[k]; mb += low[k + lag]; }
        ma /= static_cast<double>(n); mb /= static_cast<double>(n);
        double sab = 0.0, saa = 0.0, sbb = 0.0;
        for (std::size_t k = 0; k < n; ++k)
        {
            sab += (low[k] - ma) * (low[k + lag] - mb);
            saa += (low[k] - ma) * (low[k] - ma);
            sbb += (low[k + lag] - mb) * (low[k + lag] - mb);
        }
        return saa > 1e-12 && sbb > 1e-12 ? sab / std::sqrt(saa * sbb) : 0.0;
    };
    const auto chromaAt = [&](std::size_t lag) {
        double sum = 0.0;
        int n = 0;
        for (std::size_t k = 0; k + lag < chroma.size(); ++k)
        {
            double dot = 0.0, na = 0.0, nb = 0.0;
            for (std::size_t q = 0; q < 12; ++q)
            {
                dot += chroma[k][q] * chroma[k + lag][q];
                na += chroma[k][q] * chroma[k][q];
                nb += chroma[k + lag][q] * chroma[k + lag][q];
            }
            if (na > 0.0 && nb > 0.0) { sum += dot / std::sqrt(na * nb); ++n; }
        }
        return n > 0 ? sum / n : 0.0;
    };
    // a semelhança do croma conta só o que passa da do tempo vizinho (a
    // harmonia se repete no compasso, não a cada tempo)
    const double c1 = chromaAt(1);
    m.score2 = corrAt(2) + 0.5 * (chromaAt(2) - c1);
    m.score3 = corrAt(3) + 0.5 * (chromaAt(3) - c1);
    m.score4 = corrAt(4) + 0.5 * (chromaAt(4) - c1);
    const double duple = std::max(m.score2, m.score4);
    if (m.score3 > duple + margin)
    {
        m.beatsPerBar = 3;
        m.confidence = static_cast<float>(std::clamp(m.score3 - duple, 0.0, 1.0));
    }
    else
    {
        m.beatsPerBar = 4;
        m.confidence = static_cast<float>(std::clamp(duple - m.score3, 0.0, 1.0));
    }
    return m;
}

int Bar::count() const noexcept
{
    return static_cast<int>(std::count_if(hits.begin(), hits.end(), [](float h) { return h > 0.0f; }));
}

std::vector<Bar> rhythmGrid(const std::vector<Onset>& onsets, const Pulse& pulse, double t0, double t1)
{
    std::vector<Bar> bars;
    if (pulse.period <= 0.0)
        return bars;
    const double step = pulse.period / kStepsPerBeat;
    const int stepsPerBar = pulse.beatsPerBar * kStepsPerBeat;
    const double barLen = pulse.period * pulse.beatsPerBar;
    // primeiro compasso: o primeiro tempo da grade a partir de t0
    const double first = pulse.phase + std::ceil((t0 - pulse.phase) / pulse.period - 1e-9) * pulse.period;
    float maxStrength = 0.0f;
    for (const auto& o : onsets)
        maxStrength = std::max(maxStrength, o.strength);
    for (double b = first; b + barLen <= t1 + 1e-9; b += barLen)
    {
        Bar bar;
        bar.start = b;
        bar.steps = stepsPerBar;
        for (const auto& o : onsets)
        {
            if (o.time < b - step / 2 || o.time >= b + barLen - step / 2)
                continue;
            const auto k = static_cast<int>(std::lround((o.time - b) / step));
            if (k >= 0 && k < stepsPerBar)
                bar.hits[static_cast<std::size_t>(k)] = std::max(bar.hits[static_cast<std::size_t>(k)],
                                                                 maxStrength > 0 ? 0.2f + 0.8f * o.strength / maxStrength : 1.0f);
        }
        bars.push_back(bar);
    }
    return bars;
}

float pitchAt(const std::vector<Frame>& frames, double t)
{
    const double hop = hopOf(frames);
    if (frames.empty() || hop <= 0.0)
        return std::nanf("");
    std::vector<float> v;
    const auto c = static_cast<long>(std::lround((t - frames.front().time) / hop));
    for (long i = c - 2; i <= c + 2; ++i)
        if (i >= 0 && i < static_cast<long>(frames.size()))
        {
            const auto& f = frames[static_cast<std::size_t>(i)];
            if (!f.silent && f.f0Hz > 0.0f && f.periodicity >= 0.6f)
                v.push_back(69.0f + 12.0f * std::log2(f.f0Hz / 440.0f));
        }
    // numa mistura (voz + banda + bateria) o YIN se perde: a altura
    // PREDOMINANTE (saliência harmônica, analisador v6) toma o lugar quando
    // o YIN não dá ao menos 2 dos 5 quadros (docs/LEITURA_E_GABARITO.md)
    if (v.size() < 2)
    {
        v.clear();
        for (long i = c - 2; i <= c + 2; ++i)
            if (i >= 0 && i < static_cast<long>(frames.size()))
            {
                const auto& f = frames[static_cast<std::size_t>(i)];
                if (!f.silent && f.melodyHz > 0.0f && f.melodySalience >= kMelodySalienceForPitch)
                    v.push_back(69.0f + 12.0f * std::log2(f.melodyHz / 440.0f));
            }
        if (v.size() < 2)
            return std::nanf("");
    }
    std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(v.size() / 2), v.end());
    return v[v.size() / 2];
}

PitchField harmonicField(const std::vector<Frame>& frames, double t0, double t1)
{
    PitchField f {};
    double total = 0.0;
    for (const auto& fr : frames)
    {
        if (fr.time < t0 || fr.time >= t1 || fr.silent)
            continue;
        const double w = std::pow(10.0, fr.rmsDb / 10.0);
        for (std::size_t i = 0; i < 12; ++i)
            f[i] += static_cast<float>(w * fr.chroma[i]);
        total += w;
    }
    if (total > 0.0)
    {
        float s = 0.0f;
        for (const float v : f)
            s += v;
        if (s > 0.0f)
            for (auto& v : f)
                v /= s;
    }
    return f;
}

std::vector<Section> findSections(const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                                  double duration, double minSeconds, double context)
{
    // vetor de características por segundo
    const auto win = segment(frames, onsets, duration, 1.0);
    const std::size_t n = win.size();
    std::vector<std::array<float, 16>> feat(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        const auto& s = win[i];
        const auto field = harmonicField(frames, s.start, s.end);
        feat[i][0] = s.energy;
        feat[i][1] = s.density;
        feat[i][2] = hasTimbre(s) ? s.brightness : 0.0f;
        feat[i][3] = hasTimbre(s) ? s.noisiness : 0.0f;
        for (std::size_t c = 0; c < 12; ++c)
            feat[i][4 + c] = field[c];
    }
    const auto ctx = static_cast<std::size_t>(std::max(1.0, context));
    std::vector<float> novelty(n, 0.0f);
    for (std::size_t i = ctx; i + ctx <= n; ++i)
    {
        std::array<float, 16> before {}, after {};
        for (std::size_t k = 0; k < ctx; ++k)
            for (std::size_t j = 0; j < 16; ++j)
            {
                before[j] += feat[i - 1 - k][j] / static_cast<float>(ctx);
                after[j] += feat[i + k][j] / static_cast<float>(ctx);
            }
        float d = 0.0f;
        for (std::size_t j = 0; j < 4; ++j)
            d += (before[j] - after[j]) * (before[j] - after[j]);
        float dc = 0.0f;  // diferença de campo harmônico, mesma ordem de grandeza
        for (std::size_t j = 4; j < 16; ++j)
            dc += std::abs(before[j] - after[j]);
        novelty[i] = std::sqrt(d) + 0.5f * dc;
    }
    // picos acima de média + 1 desvio, com espaçamento mínimo, do mais forte ao mais fraco
    const float mean = std::accumulate(novelty.begin(), novelty.end(), 0.0f) / static_cast<float>(std::max<std::size_t>(n, 1));
    float var = 0.0f;
    for (const float v : novelty)
        var += (v - mean) * (v - mean);
    const float threshold = mean + std::sqrt(var / static_cast<float>(std::max<std::size_t>(n, 1)));
    std::vector<std::size_t> cand;
    for (std::size_t i = 1; i + 1 < n; ++i)
        if (novelty[i] > threshold && novelty[i] >= novelty[i - 1] && novelty[i] >= novelty[i + 1])
            cand.push_back(i);
    std::sort(cand.begin(), cand.end(), [&](std::size_t a, std::size_t b) { return novelty[a] > novelty[b]; });
    std::vector<double> cuts;
    for (const auto i : cand)
    {
        const double t = static_cast<double>(i);
        if (t < minSeconds || duration - t < minSeconds)
            continue;
        if (std::all_of(cuts.begin(), cuts.end(), [&](double c) { return std::abs(c - t) >= minSeconds; }))
            cuts.push_back(t);
    }
    std::sort(cuts.begin(), cuts.end());
    cuts.insert(cuts.begin(), 0.0);
    cuts.push_back(duration);

    std::vector<Section> out;
    for (std::size_t i = 0; i + 1 < cuts.size(); ++i)
    {
        Section s;
        s.start = cuts[i];
        s.end = cuts[i + 1];
        s.pulse = estimatePulse(frames, onsets, s.start, s.end);
        s.field = harmonicField(frames, s.start, s.end);
        s.summary = summarize(frames, onsets, s.start, s.end);
        out.push_back(s);
    }
    return out;
}

std::vector<std::vector<double>> lastSimilarity;

std::vector<Section> findSectionsByRepetition(const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                                              double duration, std::vector<int>& labels, double minSeconds,
                                              double kernelSeconds)
{
    labels.clear();
    // 1. um vetor por segundo: o croma (normalizado) e o timbre (metade do peso)
    const auto win = segment(frames, onsets, duration, 1.0);
    const std::size_t n = win.size();
    std::vector<std::array<double, 16>> f(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        const auto& s = win[i];
        const auto field = harmonicField(frames, s.start, s.end);
        double norm = 0.0;
        for (float c : field)
            norm += static_cast<double>(c) * c;
        norm = std::sqrt(norm);
        for (std::size_t c = 0; c < 12; ++c)
            f[i][c] = norm > 0.0 ? field[c] / norm : 0.0;
        f[i][12] = 0.5 * s.energy;
        f[i][13] = 0.5 * s.density;
        f[i][14] = 0.5 * (hasTimbre(s) ? s.brightness : 0.0f);
        f[i][15] = 0.5 * (hasTimbre(s) ? s.noisiness : 0.0f);
    }
    const auto cosine = [](const std::array<double, 16>& a, const std::array<double, 16>& b) {
        double d = 0.0, na = 0.0, nb = 0.0;
        for (std::size_t j = 0; j < 16; ++j)
        {
            d += a[j] * b[j];
            na += a[j] * a[j];
            nb += b[j] * b[j];
        }
        return na > 0.0 && nb > 0.0 ? d / std::sqrt(na * nb) : 0.0;
    };
    // 2. a matriz de autossimilaridade e a novidade de Foote (núcleo em
    //    xadrez, afinado por uma gaussiana)
    std::vector<std::vector<double>> S(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = i; j < n; ++j)
            S[i][j] = S[j][i] = cosine(f[i], f[j]);
    const int w = std::max(2, static_cast<int>(std::lround(kernelSeconds)));
    std::vector<double> nov(n, 0.0);
    for (int i = w; i + w <= static_cast<int>(n); ++i)
    {
        double acc = 0.0;
        for (int a = -w; a < w; ++a)
            for (int b = -w; b < w; ++b)
            {
                const double sign = (a < 0) == (b < 0) ? 1.0 : -1.0;
                const double g = std::exp(-0.5 * ((a + 0.5) * (a + 0.5) + (b + 0.5) * (b + 0.5)) / (0.25 * w * w));
                acc += sign * g * S[static_cast<std::size_t>(i + a)][static_cast<std::size_t>(i + b)];
            }
        nov[static_cast<std::size_t>(i)] = acc;
    }
    double mean = 0.0, var = 0.0;
    for (double v : nov) mean += v;
    mean /= static_cast<double>(std::max<std::size_t>(n, 1));
    for (double v : nov) var += (v - mean) * (v - mean);
    const double threshold = mean + 0.5 * std::sqrt(var / static_cast<double>(std::max<std::size_t>(n, 1)));
    std::vector<std::size_t> cand;
    for (std::size_t i = 1; i + 1 < n; ++i)
        if (nov[i] > threshold && nov[i] >= nov[i - 1] && nov[i] >= nov[i + 1])
            cand.push_back(i);
    std::sort(cand.begin(), cand.end(), [&](std::size_t a, std::size_t b) { return nov[a] > nov[b]; });
    std::vector<double> cuts;
    for (const auto i : cand)
    {
        const double t = static_cast<double>(i);
        if (t < minSeconds || duration - t < minSeconds)
            continue;
        if (std::all_of(cuts.begin(), cuts.end(), [&](double c) { return std::abs(c - t) >= minSeconds; }))
            cuts.push_back(t);
    }
    std::sort(cuts.begin(), cuts.end());
    cuts.insert(cuts.begin(), 0.0);
    cuts.push_back(duration);
    // 3. as letras. A repetição aparece como DIAGONAL na matriz (o refrão
    //    volta segundo a segundo, na mesma ordem); o perfil médio não basta,
    //    numa canção no mesmo tom e timbre todos os trechos têm a mesma
    //    média (achado no Suno: tudo "A"). A matriz é normalizada pela
    //    média e pelo desvio da peça (z), e dois segmentos são comparados
    //    pela média de z ao longo da diagonal, com folga de ±2 s.
    double sMean = 0.0, sVar = 0.0;
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            sMean += S[i][j];
    sMean /= static_cast<double>(std::max<std::size_t>(n * n, 1));
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            sVar += (S[i][j] - sMean) * (S[i][j] - sMean);
    const double sSd = std::sqrt(sVar / static_cast<double>(std::max<std::size_t>(n * n, 1))) + 1e-9;
    const auto diagonal = [&](std::size_t x0, std::size_t x1, std::size_t y0, std::size_t y1) {
        double best = -1e9;
        const int len = static_cast<int>(std::min(x1 - x0, y1 - y0));
        for (int d = -2; d <= 2; ++d)
        {
            double acc = 0.0;
            int count = 0;
            for (int i = 0; i < len; ++i)
            {
                const int xi = static_cast<int>(x0) + i, yi = static_cast<int>(y0) + i + d;
                if (yi < static_cast<int>(y0) || yi >= static_cast<int>(y1) || xi >= static_cast<int>(n) || yi >= static_cast<int>(n))
                    continue;
                acc += (S[static_cast<std::size_t>(xi)][static_cast<std::size_t>(yi)] - sMean) / sSd;
                ++count;
            }
            if (count >= 4)
                best = std::max(best, acc / count);
        }
        return best;
    };
    std::vector<std::pair<std::size_t, std::size_t>> seg;
    for (std::size_t k = 0; k + 1 < cuts.size(); ++k)
        seg.push_back({ static_cast<std::size_t>(cuts[k]), std::min(n, static_cast<std::size_t>(cuts[k + 1])) });
    lastSimilarity.assign(seg.size(), std::vector<double>(seg.size(), 0.0));
    for (std::size_t x = 0; x < seg.size(); ++x)
        for (std::size_t y = 0; y < seg.size(); ++y)
            lastSimilarity[x][y] = diagonal(seg[x].first, seg[x].second, seg[y].first, seg[y].second);
    std::vector<std::size_t> firstOf;  // o segmento que inaugurou cada letra
    for (std::size_t k = 0; k < seg.size(); ++k)
    {
        int label = -1;
        double best = kRepetitionThreshold;
        for (std::size_t l = 0; l < firstOf.size(); ++l)
        {
            const double c = lastSimilarity[k][firstOf[l]];
            if (c > best)
            {
                best = c;
                label = static_cast<int>(l);
            }
        }
        if (label < 0)
        {
            label = static_cast<int>(firstOf.size());
            firstOf.push_back(k);
        }
        labels.push_back(label);
    }
    std::vector<Section> out;
    for (std::size_t i = 0; i + 1 < cuts.size(); ++i)
    {
        Section s;
        s.start = cuts[i];
        s.end = cuts[i + 1];
        s.pulse = estimatePulse(frames, onsets, s.start, s.end);
        s.field = harmonicField(frames, s.start, s.end);
        s.summary = summarize(frames, onsets, s.start, s.end);
        out.push_back(s);
    }
    return out;
}

} // namespace trioio::perception
