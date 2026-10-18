#include "Strategy.h"

#include <algorithm>
#include <cmath>

namespace trioio::strategy
{
namespace
{
float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

// Normaliza uma escala pela faixa da própria peça: contrapor uma peça que só
// varia entre 0,55 e 0,65 de energia precisa inverter ESSA faixa para soar.
struct Range
{
    float lo = 0.0f, hi = 1.0f;
    float norm(float v) const { return hi - lo > 1e-4f ? clamp01((v - lo) / (hi - lo)) : 0.5f; }
};

template <typename F>
Range rangeOf(const std::vector<perception::Summary>& r, F get)
{
    Range out { 1.0f, 0.0f };
    bool any = false;
    for (const auto& s : r)
        if (perception::hasTimbre(s))
        {
            out.lo = std::min(out.lo, get(s));
            out.hi = std::max(out.hi, get(s));
            any = true;
        }
    return any ? out : Range {};
}

std::uint64_t mix(std::uint64_t x)  // splitmix64: um número por trecho, sem estado
{
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}
} // namespace

std::vector<Step> plan(const std::vector<perception::Summary>& ref, Mode mode, std::uint64_t seed)
{
    std::vector<Step> out;
    out.reserve(ref.size());
    const auto E = rangeOf(ref, [](const auto& s) { return s.energy; });
    const auto Dn = rangeOf(ref, [](const auto& s) { return s.density; });
    const auto B = rangeOf(ref, [](const auto& s) { return s.brightness; });

    // pontos de corte do "interromper": trechos que começam uma grande mudança
    std::vector<bool> cut(ref.size(), false);
    if (mode == Mode::interrupt)
    {
        std::vector<std::pair<float, std::size_t>> c;
        for (std::size_t i = 1; i < ref.size(); ++i)
            c.push_back({ perception::contrast(ref[i - 1], ref[i]), i });
        std::sort(c.begin(), c.end(), [](const auto& a, const auto& b) {
            return a.first > b.first || (a.first == b.first && a.second < b.second);
        });
        const std::size_t cuts = std::max<std::size_t>(1, ref.size() / 12);
        for (std::size_t k = 0; k < std::min(cuts, c.size()); ++k)
            cut[c[k].second] = true;
    }

    for (std::size_t i = 0; i < ref.size(); ++i)
    {
        const auto& r = ref[i];
        const bool sounding = perception::hasTimbre(r);
        // variação pequena e determinística por trecho: o seed muda a resposta
        const float jitter = static_cast<float>(mix(seed ^ (i * 0x100000001B3ull)) % 1000) / 1000.0f - 0.5f;
        BrainDecision d;
        d.memory = 0.5f;
        d.autonomy = 0.3f;
        d.confidence = r.activeShare;
        d.mutation = 0.1f + 0.1f * jitter;

        switch (mode)
        {
            case Mode::follow:
                d.energy = sounding ? r.energy : 0.0f;
                d.density = r.density;
                d.brightness = r.brightness;
                d.harmonicity = 1.0f - r.noisiness;
                d.transientRetention = 0.8f;
                d.spatialActivity = 0.3f;
                break;
            case Mode::transfigure:  // mesma forma de energia e tempo, outra matéria
                d.energy = sounding ? r.energy : 0.0f;
                d.density = r.density;
                d.brightness = 1.0f - r.brightness;
                d.harmonicity = clamp01(0.35f + r.noisiness);  // tonal vira granuloso; ruído vira tom
                d.transientRetention = 0.3f;
                d.spatialActivity = 0.7f;
                d.mutation = 0.35f + 0.1f * jitter;
                break;
            case Mode::counter:  // o avesso, dentro da faixa da própria peça
                d.energy = sounding ? 0.25f + 0.6f * (1.0f - E.norm(r.energy)) : 0.6f;
                d.density = 0.1f + 0.7f * (1.0f - Dn.norm(r.density));
                d.brightness = 0.1f + 0.8f * (1.0f - B.norm(r.brightness));
                d.harmonicity = r.noisiness > 0.3f ? 0.9f : 0.3f;
                d.transientRetention = 0.5f;
                d.spatialActivity = 0.5f;
                break;
            case Mode::interrupt:  // segue, mas corta onde a peça muda e volta com força
            {
                const bool afterCut = i > 0 && cut[i - 1];
                d.energy = cut[i] ? 0.0f : afterCut ? 0.95f : (sounding ? r.energy : 0.0f);
                d.density = afterCut ? 0.85f : r.density;
                d.brightness = afterCut ? clamp01(r.brightness + 0.3f) : r.brightness;
                d.harmonicity = 1.0f - r.noisiness;
                d.transientRetention = afterCut ? 1.0f : 0.7f;
                d.spatialActivity = afterCut ? 0.9f : 0.3f;
                break;
            }
        }
        d.revision = i + 1;
        out.push_back({ r.start, r.end, sanitizeDecision(d) });
    }
    return out;
}

std::string describe(Mode mode)
{
    switch (mode)
    {
        case Mode::follow: return "segue a referência: mesma energia, densidade, brilho e harmonicidade, com matéria própria";
        case Mode::transfigure: return "mantém a curva de energia e a densidade; inverte o brilho e troca tom por ruído e ruído por tom";
        case Mode::counter: return "contrapõe a referência: energia, densidade e brilho invertidos dentro da faixa da própria peça";
        case Mode::interrupt: return "segue a referência e corta nos pontos de maior mudança, voltando com um bloco denso";
    }
    return {};
}

bool parseMode(const std::string& name, Mode& out)
{
    for (const auto m : { Mode::follow, Mode::transfigure, Mode::counter, Mode::interrupt })
        if (name == modeName(m))
        {
            out = m;
            return true;
        }
    return false;
}

const char* modeName(Mode mode)
{
    switch (mode)
    {
        case Mode::follow: return "seguir";
        case Mode::transfigure: return "transfigurar";
        case Mode::counter: return "contrapor";
        case Mode::interrupt: return "interromper";
    }
    return "?";
}

} // namespace trioio::strategy
