#include "Mosaic.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace trioio::composition
{
using namespace trioio::perception;

namespace
{
struct Rng
{
    std::uint64_t s;
    double uniform()
    {
        s += 0x9E3779B97F4A7C15ull;
        std::uint64_t x = s;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return static_cast<double>((x ^ (x >> 31)) >> 11) / 9007199254740992.0;
    }
};

// os três descritores pesquisáveis, normalizados pela extensão do acervo
using Point = std::array<double, 3>;  // energia, brilho, ruído

struct Unit
{
    double start = 0.0, duration = 0.0;
    float pitch = 60.0f;
    Point raw {}, norm {};
};

Point describe(const Summary& s) { return { s.energy, s.brightness, s.noisiness }; }

// sem altura definida (YIN), o pico espectral mais forte perto do ataque
float unitPitch(const std::vector<Frame>& frames, double t)
{
    const float p = pitchAt(frames, t + 0.04);
    if (std::isfinite(p))
        return p;
    const Frame* best = nullptr;
    for (const auto& f : frames)
        if (f.time >= t && f.time < t + 0.12 && !f.silent && f.peaksHz[0] > 0.0f)
        {
            best = &f;
            break;
        }
    if (!best)
        return std::nanf("");
    return std::clamp(69.0f + 12.0f * std::log2(best->peaksHz[0] / 440.0f), 36.0f, 96.0f);
}

const char* modeTarget(strategy::Mode m)
{
    switch (m)
    {
        case strategy::Mode::counter: return "energia e ruído opostos";
        case strategy::Mode::transfigure: return "brilho invertido";
        case strategy::Mode::interrupt: return "em fragmentos";
        case strategy::Mode::follow: break;
    }
    return "o momento da referência";
}
} // namespace

void descriptorMosaic(Score& score, const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                      const Options& opt)
{
    const float u = std::clamp(opt.unpredictability, 0.0f, 1.0f);
    if (!opt.mosaic || u <= 0.0f || score.duration < 10.0)
        return;

    // 1. o acervo: cada ataque com som e altura vira uma unidade
    std::vector<Unit> corpus;
    for (std::size_t i = 0; i < onsets.size(); ++i)
    {
        const double t = onsets[i].time;
        const double next = i + 1 < onsets.size() ? onsets[i + 1].time : t + 0.6;
        const double d = std::clamp(next - t, 0.08, 1.2);
        const auto s = summarize(frames, onsets, t, t + d);
        const float p = unitPitch(frames, t);
        if (!hasTimbre(s) || !std::isfinite(p))
            continue;
        corpus.push_back({ t, d, p, describe(s), {} });
    }
    if (corpus.size() < 8)
        return;
    Point lo { 1, 1, 1 }, hi { 0, 0, 0 };
    for (const auto& c : corpus)
        for (std::size_t k = 0; k < 3; ++k)
        {
            lo[k] = std::min(lo[k], c.raw[k]);
            hi[k] = std::max(hi[k], c.raw[k]);
        }
    const auto normalize = [&](const Point& r) {
        Point n;
        for (std::size_t k = 0; k < 3; ++k)
            n[k] = hi[k] - lo[k] > 1e-4 ? std::clamp((r[k] - lo[k]) / (hi[k] - lo[k]), 0.0, 1.0) : 0.5;
        return n;
    };
    for (auto& c : corpus)
        c.norm = normalize(c.raw);

    // a família: o corpo de origem (espectral); proibido, o aditivo; proibidos
    // os dois, a primeira permitida
    Timbre timbre = Timbre::additive;
    for (Timbre t : { score.spectra.empty() ? Timbre::additive : Timbre::spectral, Timbre::additive })
        if (!opt.constraints.noTimbre[static_cast<std::size_t>(t)])
        {
            timbre = t;
            break;
        }
    if (opt.constraints.noTimbre[static_cast<std::size_t>(timbre)])
        for (int k = 0; k < kTimbreCount; ++k)
            if (!opt.constraints.noTimbre[static_cast<std::size_t>(k)])
            {
                timbre = static_cast<Timbre>(k);
                break;
            }

    // 2. os lugares ocupados pela outra memória (oráculo, voice 3)
    std::vector<std::pair<double, double>> used;
    for (const auto& n : score.notes)
        if (n.voice == 3)
        {
            if (!used.empty() && n.time < used.back().second + 1.5)
                used.back().second = std::max(used.back().second, n.time + n.duration);
            else
                used.push_back({ n.time, n.time + n.duration });
        }

    Rng rng { opt.seed * 0x9FB21C651E98DF25ull + 0xCA7A };
    const int passages = 1 + (u > 0.5f ? 1 : 0);
    const int choices = 1 + static_cast<int>(std::lround(4.0 * u));  // entre as k mais próximas
    const bool fragments = opt.mode == strategy::Mode::interrupt;
    // os eixos que o modo inverte; pesam o dobro na distância: sem isso, num
    // acervo em que energia, brilho e ruído andam juntos, o eixo invertido
    // perde para os outros dois e "transfigurar" vira "seguir"
    const std::array<bool, 3> inverted { opt.mode == strategy::Mode::counter, opt.mode == strategy::Mode::transfigure,
                                         opt.mode == strategy::Mode::counter };
    std::vector<Note> added;
    for (int k = 0; k < passages; ++k)
    {
        const double span = 5.0 + rng.uniform() * 5.0;
        double t0 = -1.0;
        for (int tries = 0; tries < 12 && t0 < 0.0; ++tries)
        {
            const double c = 0.08 * score.duration + rng.uniform() * (0.84 * score.duration - span);
            if (c < 0.0)
                break;
            if (std::none_of(used.begin(), used.end(), [&](const auto& r) { return c < r.second + 1.0 && c + span > r.first - 1.0; }))
                t0 = c;
        }
        if (t0 < 0.0)
            continue;
        used.push_back({ t0, t0 + span });

        // 3. passo a passo: o alvo do momento, as unidades mais próximas
        int written = 0, previous = -1, step = 0;
        double t = t0, drift = 0.0;
        std::vector<std::pair<double, int>> cost(corpus.size());
        while (t < t0 + span && t < score.duration - 0.5)
        {
            const auto now = summarize(frames, onsets, t - 0.75, t + 0.75);
            if (!hasTimbre(now))
            {
                t += 0.25;  // a referência cala: o mosaico também
                continue;
            }
            Point target = normalize(describe(now));
            for (std::size_t q = 0; q < 3; ++q)
                if (inverted[q])
                    target[q] = 1.0 - target[q];
            for (std::size_t c = 0; c < corpus.size(); ++c)
            {
                const auto& unit = corpus[c];
                double d = 0.0;
                for (std::size_t q = 0; q < 3; ++q)
                    d += (inverted[q] ? 4.0 : 1.0) * (unit.norm[q] - target[q]) * (unit.norm[q] - target[q]);
                d = std::sqrt(d);
                if (previous >= 0)  // a emenda: o salto de altura desde a unidade anterior
                    d += 0.12 * std::min(1.0, std::abs(unit.pitch - corpus[static_cast<std::size_t>(previous)].pitch) / 12.0);
                if (static_cast<int>(c) == previous)
                    d += 10.0;  // não repete a mesma unidade em seguida
                cost[c] = { d, static_cast<int>(c) };
            }
            const auto kth = cost.begin() + std::min<std::ptrdiff_t>(choices, static_cast<std::ptrdiff_t>(cost.size()));
            std::partial_sort(cost.begin(), kth, cost.end());
            const int pick = cost[static_cast<std::size_t>(rng.uniform() * static_cast<double>(kth - cost.begin()))].second;
            const auto& unit = corpus[static_cast<std::size_t>(pick)];
            Note n;
            n.time = t;
            n.duration = unit.duration * (fragments ? 0.5 : 0.9);
            n.pitch = unit.pitch;
            n.velocity = static_cast<float>(std::clamp(0.25 + 0.5 * unit.norm[0], 0.2, 0.8));
            n.voice = 5;
            n.timbre = timbre;
            n.source = unit.start;
            if (n.time + n.duration < score.duration - 0.2)
            {
                added.push_back(n);
                ++written;
            }
            drift += std::abs(unit.start - t);
            previous = pick;
            t += unit.duration;
            if (fragments && ++step % 3 == 0)
                t += 0.6;  // o respiro entre fragmentos
        }
        if (written == 0)
            continue;
        char text[220];
        std::snprintf(text, sizeof text,
                      "memória (mosaico): %d unidades da referência escolhidas pelo som · alvo: %s · "
                      "vindas em média de %.0f s de distância",
                      written, modeTarget(opt.mode), drift / written);
        score.marks.push_back({ t0, text });
    }
    if (added.empty())
        return;
    score.notes.insert(score.notes.end(), added.begin(), added.end());
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) {
        return a.time < b.time || (a.time == b.time && a.voice < b.voice);
    });
    std::stable_sort(score.marks.begin(), score.marks.end(), [](const Mark& a, const Mark& b) { return a.time < b.time; });
}

} // namespace trioio::composition
