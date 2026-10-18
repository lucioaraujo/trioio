#include "FilmEffects.h"

#include <algorithm>
#include <cmath>

namespace trioio::composition
{

std::string applyFilmEffects(Score& score, const TensionCurve& curve, const std::vector<double>& sectionStarts,
                             float brightness, std::uint64_t seed)
{
    const double dur = score.duration;
    if (dur < 10.0)
        return {};
    std::uint64_t s = seed * 0x9E3779B97F4A7C15ull + 0xF11Bull;
    const auto rnd = [&] {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return static_cast<double>(s >> 11) / 9007199254740992.0;
    };
    int whooshes = 0, ambiences = 0;
    std::vector<GestureKind> kinds;
    bool climax = false, thunder = false;
    const auto add = [&](GestureKind k, double t, double d, float level, float lo = 48.0f, float pan = 0.0f, float panEnd = 0.0f,
                         float density = 10.0f) {
        Gesture g;
        g.kind = k;
        g.time = std::max(0.0, t);
        g.duration = std::min(d, dur - g.time);
        if (g.duration <= 0.1)
            return;
        g.level = level;
        g.lo = g.loEnd = lo;
        g.hi = g.hiEnd = lo + 12.0f;
        g.pan = pan;
        g.panEnd = panEnd;
        g.density = g.densityEnd = density;
        g.seed = s ^ static_cast<std::uint64_t>(t * 1000.0);
        score.gestures.push_back(g);
    };
    // 1. o clímax: o riser antes, o impacto e o braam nele
    if (curve.climax > 6.0 && curve.climax < dur - 1.0)
    {
        const double c = curve.climax;
        add(GestureKind::riser, c - 4.0, 4.0, 0.55f, 48.0f);
        add(GestureKind::impact, c, 3.0, 0.7f);
        add(GestureKind::braam, c, std::min(4.5, dur - c), 0.5f, 31.0f);
        climax = true;
    }
    // 2. os whooshes nas mudanças de seção (longe do clímax)
    double last = -1e9;
    for (double t : sectionStarts)
    {
        if (t < 4.0 || t > dur - 2.0 || t - last < 20.0 || (climax && std::abs(t - curve.climax) < 6.0))
            continue;
        const bool lr = rnd() < 0.5;
        add(GestureKind::whoosh, t - 0.8, 1.2, 0.45f, 48.0f, lr ? -0.9f : 0.9f, lr ? 0.9f : -0.9f);
        last = t;
        ++whooshes;
    }
    // 3. as ambiências nos trechos rarefeitos
    if (!curve.value.empty())
    {
        double start = -1.0;
        for (std::size_t i = 0; i <= curve.value.size(); ++i)
        {
            const double t = static_cast<double>(i) * curve.step;
            const bool low = i < curve.value.size() && curve.value[i] < 0.3f;
            if (low && start < 0.0)
                start = t;
            if ((!low || i == curve.value.size()) && start >= 0.0)
            {
                const double len = std::min(t, dur) - start;
                if (len >= 8.0 && ambiences < 3)
                {
                    // pelo brilho, e entre duas por faixa pela semente: escura →
                    // vento ou fogo; média → água ou cidade; clara → chuva ou multidão
                    const bool alt = rnd() < 0.4;
                    const GestureKind k = brightness > 0.6f ? (alt ? GestureKind::crowd : GestureKind::rain)
                                        : brightness < 0.35f ? (alt ? GestureKind::fire : GestureKind::wind)
                                                             : (alt ? GestureKind::city : GestureKind::water);
                    kinds.push_back(k);
                    add(k, start, len, k == GestureKind::water ? 0.4f : 0.35f, 48.0f, 0.0f, 0.0f,
                        k == GestureKind::rain ? 600.0f : 90.0f);
                    if (k == GestureKind::rain && !thunder && len >= 12.0)
                    {
                        add(GestureKind::thunder, start + len * (0.4 + 0.3 * rnd()), 6.0, 0.5f);
                        thunder = true;
                    }
                    ++ambiences;
                }
                start = -1.0;
            }
        }
    }
    std::stable_sort(score.gestures.begin(), score.gestures.end(), [](const Gesture& a, const Gesture& b) { return a.time < b.time; });
    if (!climax && whooshes == 0 && ambiences == 0)
        return {};
    std::string line = "efeitos de cinema:";
    if (climax)
        line += " riser, impacto e braam no clímax;";
    if (whooshes > 0)
        line += " " + std::to_string(whooshes) + " whoosh nas mudanças de seção;";
    if (ambiences > 0)
    {
        std::string names;
        for (auto k : kinds)
        {
            const char* nm = k == GestureKind::rain ? "chuva" : k == GestureKind::crowd ? "multidão" : k == GestureKind::wind ? "vento"
                           : k == GestureKind::fire ? "fogo" : k == GestureKind::city ? "cidade" : "água";
            if (names.find(nm) == std::string::npos)
                names += (names.empty() ? "" : ", ") + std::string(nm);
        }
        line += std::string(" ") + std::to_string(ambiences) + " ambiência(s) nos trechos rarefeitos (" + names
                + (thunder ? ", com trovão" : "") + ");";
    }
    line.pop_back();
    return line;
}

} // namespace trioio::composition
