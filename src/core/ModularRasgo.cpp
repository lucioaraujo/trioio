#include "ModularRasgo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>

namespace trioio::composition
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

struct Rng
{
    std::uint64_t s;
    double next()  // 0..1
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return static_cast<double>(s >> 11) / 9007199254740992.0;
    }
};

double midiHz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

// o envelope do próprio som (seguidor: ataque 1 ms, soltura 60 ms), 0..1
std::vector<float> follower(const std::vector<float>& x, double sr)
{
    std::vector<float> e(x.size());
    const double a = std::exp(-1.0 / (0.001 * sr)), r = std::exp(-1.0 / (0.06 * sr));
    double v = 0.0, peak = 1e-9;
    for (std::size_t i = 0; i < x.size(); ++i)
    {
        const double ax = std::abs(x[i]);
        v = ax > v ? a * v + (1.0 - a) * ax : r * v + (1.0 - r) * ax;
        e[i] = static_cast<float>(v);
        peak = std::max(peak, v);
    }
    for (auto& z : e)
        z = static_cast<float>(z / peak);
    return e;
}

float peakOf(const std::vector<float>& x)
{
    float p = 1e-9f;
    for (float v : x)
        p = std::max(p, std::abs(v));
    return p;
}

// o par de Hilbert (duas cascatas de passa-tudos; coeficientes de O.
// Niemitalo) para o deslocador de frequência
struct Allpass2
{
    double c, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    double run(double x)
    {
        const double y = c * (x + y2) - x2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }
};
} // namespace

const char* patchName(Patch p)
{
    switch (p)
    {
        case Patch::none: return "nenhum";
        case Patch::fold: return "dobra";
        case Patch::ring: return "anel inarmônico";
        case Patch::shift: return "deslocamento de frequência";
        case Patch::chaosGate: return "portão caótico";
        case Patch::stutter: return "gagueira granular";
        case Patch::sympathetic: return "ressonância simpática";
    }
    return "?";
}

void applyPatch(std::vector<float>& x, const Note& n, double sr, std::uint64_t seed)
{
    const auto p = static_cast<Patch>(n.patch);
    const double d = std::clamp(static_cast<double>(n.patchDepth), 0.0, 1.0);
    if (p == Patch::none || d <= 0.0 || x.empty())
        return;
    Rng rng { seed | 1 };
    const double f0 = midiHz(n.pitch);
    const float pk = peakOf(x);
    switch (p)
    {
        case Patch::fold:
        {
            // a dobra cresce com o próprio envelope: o ataque se rasga, a
            // cauda volta ao corpo
            const auto env = follower(x, sr);
            for (std::size_t i = 0; i < x.size(); ++i)
            {
                const double g = 1.0 + 9.0 * d * env[i];
                const double y = std::sin(0.5 * kPi * g * x[i] / pk) * pk;
                x[i] = static_cast<float>((1.0 - d) * x[i] + d * y);
            }
            break;
        }
        case Patch::ring:
        {
            const double fr = f0 * std::max(0.2, static_cast<double>(n.patchRatio));
            double ph = 0.0;
            for (auto& v : x)
            {
                ph += 2.0 * kPi * fr / sr;
                v = static_cast<float>((1.0 - d) * v + d * v * std::sin(ph));
            }
            break;
        }
        case Patch::shift:
        {
            static const double ca[4] = { 0.6923878, 0.9360654322959, 0.9882295226860, 0.9987488452737 };
            static const double cb[4] = { 0.4021921162426, 0.8561710882420, 0.9722909545651, 0.9952884791278 };
            std::array<Allpass2, 4> A, B;
            for (int k = 0; k < 4; ++k)
            {
                A[k].c = ca[k] * ca[k];
                B[k].c = cb[k] * cb[k];
            }
            const double hz = (rng.next() < 0.5 ? -1.0 : 1.0) * (25.0 + 250.0 * d * rng.next());
            double ph = 0.0, prevA = 0.0;
            for (auto& v : x)
            {
                double a = v, b = v;
                for (int k = 0; k < 4; ++k)
                {
                    a = A[k].run(a);
                    b = B[k].run(b);
                }
                const double re = prevA;  // o ramo A atrasado de uma amostra
                prevA = a;
                ph += 2.0 * kPi * hz / sr;
                const double y = re * std::cos(ph) - b * std::sin(ph);
                v = static_cast<float>((1.0 - d) * v + d * y);
            }
            break;
        }
        case Patch::chaosGate:
        {
            // o registrador de 8 bits do Rungler: a cada batida do relógio, o
            // bit novo é o XOR do último com um oscilador lento; os três bits
            // de cima dão o nível do novo golpe do vactrol
            const double rate = 5.0 + 16.0 * d;
            std::uint8_t reg = static_cast<std::uint8_t>(rng.next() * 255.0) | 1;
            double clock = 0.0, osc = 0.0, vac = 1.0, lp1 = 0.0, lp2 = 0.0;
            const double tau = 0.06 + 0.15 * (1.0 - d);
            for (auto& v : x)
            {
                clock += rate / sr;
                osc += (rate * 0.37) / sr;
                if (clock >= 1.0)
                {
                    clock -= 1.0;
                    const int bit = ((reg >> 7) & 1) ^ (osc - std::floor(osc) > 0.5 ? 1 : 0);
                    reg = static_cast<std::uint8_t>((reg << 1) | bit);
                    vac = std::max(vac, 0.25 + 0.75 * ((reg >> 5) & 7) / 7.0);
                }
                vac *= std::exp(-1.0 / (tau * sr));
                const double fc = 150.0 + 9000.0 * vac * vac;
                const double a = 1.0 - std::exp(-2.0 * kPi * fc / sr);
                lp1 += a * (v - lp1);
                lp2 += a * (lp1 - lp2);
                v = static_cast<float>((1.0 - d) * v + d * lp2 * vac * 1.6);
            }
            break;
        }
        case Patch::stutter:
        {
            // grãos de 25–90 ms (janela de Hann, meia sobreposição); cada um
            // pode vir de outro lugar da nota ou repetir o anterior
            const std::vector<float> src = x;
            std::fill(x.begin(), x.end(), 0.0f);
            const auto g = static_cast<std::size_t>((0.025 + 0.065 * rng.next()) * sr);
            std::size_t from = 0;
            for (std::size_t at = 0; at + g < src.size(); at += g / 2)
            {
                const double u = rng.next();
                if (u < 0.45 * d)
                    from = static_cast<std::size_t>(rng.next() * static_cast<double>(src.size() - g));
                else if (u < 0.75 * d)
                {
                    // repete o grão de antes (a gagueira)
                }
                else
                    from = at;
                for (std::size_t k = 0; k < g; ++k)
                {
                    const double w = 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(k) / static_cast<double>(g));
                    x[at + k] += static_cast<float>(w * src[from + k]);
                }
            }
            break;
        }
        case Patch::sympathetic:
        {
            const double fr = f0 * std::max(0.3, static_cast<double>(n.patchRatio));
            const auto D = std::max<std::size_t>(2, static_cast<std::size_t>(sr / fr));
            std::vector<double> line(D, 0.0);
            std::size_t w = 0;
            double lp = 0.0;
            const double fb = 0.6 + 0.35 * d;
            // a corda que não existe soa além do fim da nota
            x.resize(x.size() + static_cast<std::size_t>(0.6 * sr), 0.0f);
            for (auto& v : x)
            {
                const double y = line[w];
                lp = 0.7 * lp + 0.3 * y;
                line[w] = v + fb * lp;
                w = (w + 1) % D;
                v = static_cast<float>(v + 0.6 * d * y);
            }
            break;
        }
        case Patch::none: break;
    }
    // o nível de volta ao do corpo (o ScoreSynth ainda nivela a nota)
    const float pk2 = peakOf(x);
    for (auto& v : x)
        v *= pk / pk2;
}

std::string applyModularRasgo(Score& score, float amount, const TensionCurve& curve, const std::vector<double>& sectionStarts,
                              std::uint64_t seed)
{
    const double a = std::clamp(static_cast<double>(amount), 0.0, 1.0);
    if (a <= 0.0 || score.notes.empty())
        return {};
    Rng rng { seed * 0x9E3779B97F4A7C15ull + 0x2A5Bull };
    std::vector<double> starts = sectionStarts;
    if (starts.empty() || starts.front() > 0.0)
        starts.insert(starts.begin(), 0.0);
    starts.push_back(1e9);
    // as razões inarmônicas: do corpo espectral da referência, quando há
    std::vector<float> ratios;
    for (const auto& sp : score.spectra)
        for (float r : sp.ratios)
            if (r > 1.05f && std::abs(r - std::round(r)) > 0.08f)
                ratios.push_back(r);
    if (ratios.empty())
        ratios = { 1.414f, 2.76f, 0.707f, 3.17f, 1.618f, 2.24f };
    // as quimeras pela família do instrumento
    const auto chimeraFor = [&](Timbre t) {
        switch (t)
        {
            case Timbre::bars: case Timbre::metal: case Timbre::ride: case Timbre::crash: case Timbre::piano:
            case Timbre::agogo: case Timbre::triangle:
                return Timbre::bowedPlate;
            case Timbre::drum: case Timbre::membrane: case Timbre::surdo: case Timbre::conga: case Timbre::bongo:
            case Timbre::tom: case Timbre::tamborim: case Timbre::repique: case Timbre::zabumba:
                return Timbre::blownSkin;
            default:
                return rng.next() < 0.5 ? Timbre::sungString : (rng.next() < 0.5 ? Timbre::bowedPlate : Timbre::blownSkin);
        }
    };
    std::map<std::string, int> count;
    int chimeras = 0;
    for (std::size_t s = 0; s + 1 < starts.size(); ++s)
    {
        const double t0 = starts[s], t1 = starts[s + 1];
        const double tension = curve.value.empty() ? 0.5 : curve.at(std::min(score.duration, 0.5 * (t0 + std::min(t1, score.duration))));
        for (int v : { 0, 2, 3, 5, 6, 1, 4 })
        {
            const double pv = a * (v == 1 || v == 4 ? 0.5 : 1.0);
            if (rng.next() >= pv)
                continue;
            const auto patch = static_cast<Patch>(1 + static_cast<int>(rng.next() * (kPatches - 1 - 1e-9)));
            const float depth = static_cast<float>(std::clamp(a * (0.35 + 0.9 * tension), 0.1, 1.0));
            const float ratio = ratios[static_cast<std::size_t>(rng.next() * ratios.size()) % ratios.size()];
            const bool chimera = (v == 0 || v == 2 || v == 3 || v == 5) && rng.next() < a * 0.3;
            Timbre chim = Timbre::sungString;
            bool any = false;
            for (auto& n : score.notes)
                if (n.voice == v && n.time >= t0 && n.time < t1)
                {
                    if (chimera)
                    {
                        if (!any)
                            chim = chimeraFor(n.timbre);
                        n.timbre = chim;
                    }
                    n.patch = static_cast<std::uint8_t>(patch);
                    n.patchDepth = depth;
                    n.patchRatio = ratio;
                    any = true;
                }
            if (any)
            {
                ++count[patchName(patch)];
                chimeras += chimera ? 1 : 0;
            }
        }
    }
    if (count.empty())
        return {};
    std::string line = "rasgo modular (" + std::to_string(static_cast<int>(std::lround(a * 100))) + " %):";
    for (const auto& [name, c] : count)
        line += " " + name + " ×" + std::to_string(c) + ",";
    if (chimeras > 0)
        line += " " + std::to_string(chimeras) + " quimera(s),";
    line.pop_back();
    return line;
}

} // namespace trioio::composition
