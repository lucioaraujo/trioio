#include "AquorbiumVoice.h"

#include "core/BiomaGranularStriker.h"
#include "core/BiomaModalResonator.h"
#include "core/BiomaPhysicalString.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace trioio
{
using composition::Note;
using composition::Timbre;

namespace
{
double unit(std::uint64_t& s)
{
    s += 0x9E3779B97F4A7C15ull;
    std::uint64_t x = s;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return static_cast<double>((x ^ (x >> 31)) >> 11) / 9007199254740992.0;
}
double midiHz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

// Os três corpos do Aquorbium são golpeados: `excite` no início da nota e o
// corpo decai sozinho; a cauda termina num fade curto.
std::vector<float> ring(double sr, double total, const std::function<float()>& tick)
{
    const auto frames = static_cast<std::size_t>(total * sr);
    std::vector<float> out(frames, 0.0f);
    for (std::size_t i = 0; i < frames; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const float v = tick();
        out[i] = std::isfinite(v) ? static_cast<float>(v * std::min(1.0, (total - t) / 0.05)) : 0.0f;
    }
    return out;
}
} // namespace

std::vector<float> renderAquorbiumNote(const Note& n, double sr, std::uint64_t seed)
{
    if (n.duration <= 0.0)
        return {};
    std::uint64_t rs = seed;
    const auto s32 = static_cast<std::uint32_t>(unit(rs) * 4294967295.0);
    const auto hz = static_cast<float>(midiHz(n.pitch));
    switch (n.timbre)
    {
        case Timbre::biomaModal:
        {
            aquorbium::BiomaModalResonator m;
            m.prepare(sr);
            m.setSeed(s32);
            m.reset();
            aquorbium::BiomaModalResonator::Parameters p;
            p.fundamentalHz = hz;
            p.material = static_cast<float>(unit(rs));
            p.decaySeconds = static_cast<float>(std::clamp(n.duration * 1.5, 0.4, 3.0));
            p.brightness = 0.3f + 0.5f * n.velocity;
            p.hardness = static_cast<float>(0.3 + 0.5 * unit(rs));
            p.strikePosition = static_cast<float>(0.1 + 0.8 * unit(rs));
            m.setParameters(p);
            m.excite(n.velocity);
            return ring(sr, n.duration + 2.0, [&] { return m.tick(); });
        }
        case Timbre::biomaString:
        {
            aquorbium::BiomaPhysicalString s;
            s.prepare(sr);
            s.setSeed(s32);
            s.reset();
            aquorbium::BiomaPhysicalString::Parameters p;
            p.fundamentalHz = hz;
            p.decaySeconds = static_cast<float>(std::clamp(n.duration * 1.6, 0.5, 3.5));
            p.brightness = 0.35f + 0.45f * n.velocity;
            p.hardness = static_cast<float>(0.3 + 0.5 * unit(rs));
            p.pluckPosition = static_cast<float>(0.08 + 0.35 * unit(rs));
            s.setParameters(p);
            s.excite(n.velocity);
            return ring(sr, n.duration + 1.8, [&] { return s.tick(); });
        }
        case Timbre::striker:
        {
            // o centro das partículas é a nota; notas longas são golpeadas de
            // novo a cada ~0,35 s, como um rufar de partículas
            aquorbium::BiomaGranularStriker g;
            g.prepare(sr);
            g.setSeed(s32);
            g.reset();
            aquorbium::BiomaGranularStriker::Parameters p;
            p.centerHz = hz;
            p.grainDurationMs = static_cast<float>(8.0 + 20.0 * unit(rs));
            p.particleDensity = static_cast<float>(0.3 + 0.6 * unit(rs));
            p.spectralSpread = static_cast<float>(0.2 + 0.5 * unit(rs));
            p.resonance = static_cast<float>(0.3 + 0.6 * unit(rs));
            p.irregularity = static_cast<float>(0.2 + 0.7 * unit(rs));
            g.setParameters(p);
            const auto every = static_cast<std::size_t>(0.35 * sr);
            const auto until = static_cast<std::size_t>(n.duration * sr);
            std::size_t i = 0;
            return ring(sr, n.duration + 0.6, [&] {
                if (i < until && i % every == 0)
                    g.excite(n.velocity * (i == 0 ? 1.0f : 0.6f));
                ++i;
                return g.tick();
            });
        }
        default:
            break;
    }
    return {};
}

} // namespace trioio
