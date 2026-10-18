#include "Groove.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trioio::composition
{
using perception::kMaxStepsPerBar;
using perception::kStepsPerBeat;
using perception::Onset;
using perception::Pulse;

namespace
{
constexpr float kMinSwing = 0.03f;     // fração do tempo
constexpr double kMinOffset = 0.008;   // segundos
constexpr int kMinCount = 4;
constexpr float kMinPulseConfidence = 0.25f;

float median(std::vector<float> v)
{
    if (v.empty())
        return 0.0f;
    const auto mid = v.begin() + static_cast<std::ptrdiff_t>(v.size() / 2);
    std::nth_element(v.begin(), mid, v.end());
    return *mid;
}

// a deformação dentro do tempo: 0 → 0, 0,5 → 0,5 + s8, 1 → 1, linear em cada
// metade; e, dentro de cada metade, o mesmo para a semicolcheia (s16)
double warp(double frac, double s8, double s16)
{
    const auto half = [s16](double u) {  // u em [0, 1) dentro da metade
        const double m = 0.5 + s16;
        return u < 0.5 ? u / 0.5 * m : m + (u - 0.5) / 0.5 * (1.0 - m);
    };
    const double mid = 0.5 + s8;
    return frac < 0.5 ? half(frac / 0.5) * mid : mid + half((frac - 0.5) / 0.5) * (1.0 - mid);
}
} // namespace

GrooveProfile measureGroove(const std::vector<Onset>& onsets, const Pulse& pulse, double t0, double t1)
{
    GrooveProfile g;
    g.accent.fill(1.0f);
    // sem pulso confiável não há grade para medir o balanço (a mesma régua das
    // linhas, core/Lines.h)
    if (pulse.period <= 0.0 || pulse.confidence < kMinPulseConfidence)
        return g;
    g.barSteps = std::clamp(pulse.beatsPerBar, 1, kMaxStepsPerBar / kStepsPerBeat) * kStepsPerBeat;
    std::vector<const Onset*> in;
    for (const auto& o : onsets)
        if (o.time >= t0 && o.time < t1)
            in.push_back(&o);
    g.onsets = static_cast<int>(in.size());
    if (in.size() < 16)
        return g;

    // 1. o swing. Não é a mediana de uma janela larga: com ataques soltos (o
    //    tempo expressivo de uma gravação) a mediana de 0,40–0,71 cai no meio
    //    da janela (~0,555) e inventava 30 % de swing em toda gravação. O swing
    //    existe quando os contratempos se CONCENTRAM na região ternária
    //    (0,58–0,72 do tempo), em clara maioria sobre a região reta (0,42–0,58)
    std::vector<float> straight8, swung8, straight16, swung16;
    for (const auto* o : in)
    {
        const double x = (o->time - pulse.phase) / pulse.period;
        const double frac = x - std::floor(x);
        if (frac >= 0.42 && frac < 0.58)
            straight8.push_back(static_cast<float>(frac));
        else if (frac >= 0.58 && frac < 0.72)
            swung8.push_back(static_cast<float>(frac));
        // as semicolcheias da primeira metade (0,25 reto; ~0,29–0,34 com swing)
        else if (frac >= 0.21 && frac < 0.285)
            straight16.push_back(static_cast<float>(frac / 0.5));
        else if (frac >= 0.285 && frac < 0.36)
            swung16.push_back(static_cast<float>(frac / 0.5));
    }
    if (swung8.size() * 6 >= in.size() && swung8.size() * 2 >= straight8.size() * 3)
        g.swing8 = std::clamp(median(swung8) - 0.5f, 0.0f, 0.25f);
    if (swung16.size() * 8 >= in.size() && swung16.size() * 2 >= straight16.size() * 3)
        g.swing16 = std::clamp(median(swung16) - 0.5f, 0.0f, 0.25f);
    if (g.swing8 < kMinSwing)
        g.swing8 = 0.0f;
    if (g.swing16 < kMinSwing)
        g.swing16 = 0.0f;

    // 2. o microtempo: o resíduo de cada ataque em relação à grade já com o
    //    swing, descontada a deriva lenta (a mediana dos resíduos em ±4
    //    tempos); a mediana por semicolcheia do compasso
    struct Hit { double t; int pos; double res; float s; };
    std::vector<Hit> hits;
    for (const auto* o : in)
    {
        const double x = (o->time - pulse.phase) / pulse.period;
        const double beat = std::floor(x);
        // a semicolcheia mais próxima na grade deformada
        int best = 0;
        double bestD = 1e9, bestT = 0.0;
        for (int k = 0; k <= kStepsPerBeat; ++k)
        {
            const double t = pulse.phase + (beat + warp(static_cast<double>(k) / kStepsPerBeat, g.swing8, g.swing16)) * pulse.period;
            if (std::abs(o->time - t) < bestD)
            {
                bestD = std::abs(o->time - t);
                best = k;
                bestT = t;
            }
        }
        const long step = static_cast<long>(beat) * kStepsPerBeat + best;
        const int pos = static_cast<int>(((step % g.barSteps) + g.barSteps) % g.barSteps);
        hits.push_back({ o->time, pos, o->time - bestT, o->strength });
    }
    std::vector<float> res(hits.size());
    for (std::size_t i = 0; i < hits.size(); ++i)
    {
        std::vector<float> near;
        for (const auto& h : hits)
            if (std::abs(h.t - hits[i].t) <= 4.0 * pulse.period)
                near.push_back(static_cast<float>(h.res));
        res[i] = static_cast<float>(hits[i].res) - median(near);
    }
    std::array<std::vector<float>, kMaxStepsPerBar> byPos, strength;
    float meanS = 0.0f;
    for (std::size_t i = 0; i < hits.size(); ++i)
    {
        byPos[static_cast<std::size_t>(hits[i].pos)].push_back(res[i]);
        strength[static_cast<std::size_t>(hits[i].pos)].push_back(hits[i].s);
        meanS += hits[i].s;
    }
    meanS /= static_cast<float>(hits.size());
    for (int p = 0; p < g.barSteps; ++p)
    {
        const auto i = static_cast<std::size_t>(p);
        g.count[i] = static_cast<int>(byPos[i].size());
        if (g.count[i] >= kMinCount)
        {
            const float m = median(byPos[i]);
            g.offset[i] = std::abs(m) >= kMinOffset ? std::clamp(m, -0.06f, 0.06f) : 0.0f;
            float s = 0.0f;
            for (float v : strength[i])
                s += v;
            s /= static_cast<float>(strength[i].size());
            // o acento: comprimido (raiz) e limitado — molda, não substitui a dinâmica
            g.accent[i] = meanS > 0.0f ? std::clamp(std::sqrt(s / meanS), 0.8f, 1.2f) : 1.0f;
        }
    }
    g.valid = true;
    return g;
}

std::string applyGroove(Score& score, const GrooveProfile& g, const Pulse& pulse,
                        const std::vector<std::pair<double, double>>& spans, double amount)
{
    if (!g.valid || pulse.period <= 0.0 || amount <= 0.0 || spans.empty())
        return {};
    const bool anyOffset = std::any_of(g.offset.begin(), g.offset.begin() + g.barSteps, [](float o) { return o != 0.0f; });
    const bool anyAccent = std::any_of(g.accent.begin(), g.accent.begin() + g.barSteps, [](float a) { return std::abs(a - 1.0f) > 0.05f; });
    if (g.swing8 == 0.0f && g.swing16 == 0.0f && !anyOffset && !anyAccent)
        return {};
    const double s8 = g.swing8 * amount, s16 = g.swing16 * amount;
    int moved = 0;
    for (auto& n : score.notes)
    {
        const bool inside = std::any_of(spans.begin(), spans.end(), [&](const auto& sp) { return n.time >= sp.first && n.time < sp.second; });
        if (!inside)
            continue;
        const double x = (n.time - pulse.phase) / pulse.period * kStepsPerBeat;
        const double k = std::round(x);
        if (std::abs(x - k) > 0.05)
            continue;  // fora da grade (tempo livre, ornamento, objeto do olho)
        const long step = static_cast<long>(k);
        const long beat = static_cast<long>(std::floor(static_cast<double>(step) / kStepsPerBeat));
        const int inBeat = static_cast<int>(step - beat * kStepsPerBeat);
        const int pos = static_cast<int>(((step % g.barSteps) + g.barSteps) % g.barSteps);
        const double t = pulse.phase
                       + (static_cast<double>(beat) + warp(static_cast<double>(inBeat) / kStepsPerBeat, s8, s16)) * pulse.period
                       + g.offset[static_cast<std::size_t>(pos)] * amount;
        const double end = n.time + n.duration;
        n.time = std::max(0.0, t);
        n.duration = std::max(0.02, end - n.time);  // o fim fica onde estava
        n.velocity = std::clamp(n.velocity * static_cast<float>(1.0 + (g.accent[static_cast<std::size_t>(pos)] - 1.0) * amount), 0.0f, 1.0f);
        ++moved;
    }
    if (moved == 0)
        return {};
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) { return a.time < b.time; });
    float maxOff = 0.0f;
    int late = -1;
    for (int p = 0; p < g.barSteps; ++p)
        if (std::abs(g.offset[static_cast<std::size_t>(p)]) > std::abs(maxOff))
        {
            maxOff = g.offset[static_cast<std::size_t>(p)];
            late = p;
        }
    char line[256];
    std::snprintf(line, sizeof line,
                  "groove da referência (%.0f %%): swing das colcheias %.0f %% (contratempo em %.2f do tempo)%s%s; %d notas na grade com o balanço",
                  100.0 * amount, 100.0 * g.swing8 / 0.1667, 0.5 + s8,
                  g.swing16 != 0.0f ? ", das semicolcheias também" : "",
                  late >= 0 ? (", microtempo até " + std::to_string(static_cast<int>(std::lround(maxOff * 1000.0))) + " ms na semicolcheia "
                               + std::to_string(late + 1) + " do compasso").c_str() : "",
                  moved);
    return line;
}

std::string applyRubato(Score& score, const std::vector<std::pair<double, double>>& freeSpans, double depth)
{
    if (depth <= 0.0 || freeSpans.empty())
        return {};
    // o arco: w(u) = (u + β((u − ½)³ + ⅛)) / (1 + β/4); w(0)=0, w(1)=1, a
    // derivada (o tempo local) mínima no meio e máxima nas pontas, razão 1 + 3β/4
    const double beta = 0.4 * depth;
    const auto warp = [beta](double u) {
        return (u + beta * ((u - 0.5) * (u - 0.5) * (u - 0.5) + 0.125)) / (1.0 + 0.25 * beta);
    };
    int phrases = 0, moved = 0;
    for (const auto& span : freeSpans)
    {
        // as frases do motivo dentro da seção livre
        std::vector<double> starts;
        for (const auto& n : score.notes)
            if (n.voice == 0 && n.time >= span.first && n.time < span.second)
                starts.push_back(n.time);
        std::sort(starts.begin(), starts.end());
        if (starts.size() < 4)
            continue;
        std::vector<std::pair<double, double>> phr;
        double a = starts.front(), last = starts.front();
        for (std::size_t i = 1; i < starts.size(); ++i)
        {
            if (starts[i] - last > 0.6)
            {
                phr.push_back({ a, last });
                a = starts[i];
            }
            last = starts[i];
        }
        phr.push_back({ a, last });
        for (const auto& [p0, p1] : phr)
        {
            if (p1 - p0 < 1.0)
                continue;  // frase curta demais para um arco
            const double len = p1 - p0;
            for (auto& n : score.notes)
            {
                if (n.time < p0 || n.time > p1)
                    continue;
                const double u = (n.time - p0) / len;
                const double end = std::min(1.0, (n.time + n.duration - p0) / len);
                const double t = p0 + warp(u) * len;
                const double e = end < 1.0 ? p0 + warp(end) * len : n.time + n.duration;
                n.duration = std::max(0.03, e - t);
                n.time = t;
                ++moved;
            }
            ++phrases;
        }
    }
    if (phrases == 0)
        return {};
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) { return a.time < b.time; });
    char line[200];
    std::snprintf(line, sizeof line, "rubato nas seções de tempo livre: o arco de frase (Todd 1992) em %d frases, pontas %.0f %% mais lentas que o meio (%d notas)",
                  phrases, 100.0 * 0.75 * beta, moved);
    return line;
}

} // namespace trioio::composition
