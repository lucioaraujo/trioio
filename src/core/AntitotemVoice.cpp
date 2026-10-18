#include "AntitotemVoice.h"

#include "CmosVoice.h"

#include <algorithm>
#include <cmath>

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
} // namespace

std::vector<float> renderAntitotemNote(const Note& n, double sr, std::uint64_t seed)
{
    if (n.timbre != Timbre::cmos || n.duration <= 0.0)
        return {};
    std::uint64_t rs = seed;
    antitotem::CmosVoice v;
    v.prepare(sr);
    v.reset();
    v.setOscillatorCore(static_cast<antitotem::CmosVoice::OscillatorCore>(static_cast<int>(unit(rs) * 2.999)));
    // A, B e C na nota (B e C levemente afastados: os batimentos do CMOS);
    // o OSC4 (divisor 4020) desligado; o OSC5 (PLL 4046) uma oitava acima,
    // caçando a nota
    const float levels[5] = { 1.0f, 0.55f, 0.4f, 0.0f, static_cast<float>(0.2 + 0.3 * unit(rs)) };
    const float ratios[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 2.0f };
    for (std::size_t o = 0; o < 5; ++o)
    {
        v.setOscillatorLevel(o, levels[o]);
        v.setOscillatorRatio(o, ratios[o]);
        v.setOscillatorShape(o, static_cast<float>(unit(rs) * 3.0));
    }
    v.setFeedbackAmount(static_cast<float>(0.1 + 0.3 * unit(rs)));
    // A voz recebe a altura como "tensão" 0..1: f = 52 Hz × 2^(4,3 × cv) ×
    // alimentação, com alimentação = 0,46 + 0,78 × energia
    // (CmosVoice::tickStereo). O Trioio converte a nota para essa escala.
    const float energy = 0.7f;
    v.setEnergy(energy);
    const double supply = 0.46 + 0.78 * energy;
    const double tail = 0.12;
    const auto frames = static_cast<std::size_t>((n.duration + tail) * sr);
    std::vector<float> out(frames, 0.0f);
    double dcIn = 0.0, dcOut = 0.0;
    for (std::size_t i = 0; i < frames; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double m = n.pitchAt(t);
        const double hz = 440.0 * std::pow(2.0, (m - 69.0) / 12.0);
        const double cv = std::clamp(std::log2(hz / (52.0 * supply)) / 4.3, 0.0, 1.0);
        const auto f = v.tickStereo(static_cast<float>(cv));
        const double x = 0.5 * (f.left + f.right);
        dcOut = x - dcIn + 0.9995 * dcOut;
        dcIn = x;
        const double env = std::min(1.0, t / 0.01) * (t > n.duration ? std::max(0.0, 1.0 - (t - n.duration) / tail) : 1.0);
        out[i] = std::isfinite(dcOut) ? static_cast<float>(dcOut * env) : 0.0f;
    }
    return out;
}

} // namespace trioio
