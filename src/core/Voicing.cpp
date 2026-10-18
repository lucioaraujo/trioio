#include "Voicing.h"

#include <algorithm>
#include <cmath>

namespace trioio::composition
{
namespace
{
// a altura do grau da escala mais perto de `p`, até `reach` semitons: só os
// desvios microtonais são corrigidos; uma nota fora da escala por um semitom
// é cor do voicing (a quarta Dó–Fá numa pentatônica sem Fá continua quarta)
float snapToScale(float p, const MachineScale* s, float reach = 0.6f)
{
    if (!s || s->cents.empty())
        return p;
    const double rel = 100.0 * p - s->tonic;
    double best = rel, bestD = 1e18;
    for (double c : s->cents)
    {
        const double m0 = std::round((rel - c) / s->period);
        for (double m = m0 - 1; m <= m0 + 1; m += 1.0)
        {
            const double cand = c + m * s->period;
            if (std::abs(cand - rel) < bestD) { bestD = std::abs(cand - rel); best = cand; }
        }
    }
    return bestD <= 100.0 * reach ? static_cast<float>((s->tonic + best) / 100.0) : p;
}

float wrap12(float x) { return std::fmod(std::fmod(x, 12.0f) + 12.0f, 12.0f); }
} // namespace

bool parseVoicing(const std::string& n, Voicing& out)
{
    if (n == "tercas" || n == "terças") out = Voicing::thirds;
    else if (n == "quartal") out = Voicing::quartal;
    else if (n == "sowhat") out = Voicing::soWhat;
    else if (n == "planing") out = Voicing::planing;
    else if (n == "bitonal") out = Voicing::bitonal;
    else if (n == "aberto") out = Voicing::open;
    else if (n == "cluster") out = Voicing::cluster;
    else return false;
    return true;
}

const char* voicingName(Voicing v)
{
    switch (v)
    {
        case Voicing::thirds: return "terças";
        case Voicing::quartal: return "quartal (McCoy Tyner)";
        case Voicing::soWhat: return "So What (Evans, Davis)";
        case Voicing::planing: return "planing paralelo (Debussy)";
        case Voicing::bitonal: return "bitonal (Stravinsky, Milhaud)";
        case Voicing::open: return "quintas abertas (bordão)";
        case Voicing::cluster: return "cluster (Cowell, Ligeti)";
    }
    return "?";
}

std::vector<float> voiceChord(Voicing v, float root, const std::vector<float>& chordPcs, const MachineScale* scale,
                              std::vector<float>& shape)
{
    std::vector<float> out;
    const auto stack = [&](std::initializer_list<float> offs, bool snap) {
        for (float o : offs)
            out.push_back(snap ? snapToScale(root + o, scale) : root + o);
    };
    switch (v)
    {
        case Voicing::quartal: stack({ 0, 5, 10 }, true); break;
        case Voicing::soWhat: stack({ 0, 5, 10, 15, 19 }, true); break;
        case Voicing::open: stack({ 0, 7, 14 }, true); break;
        case Voicing::bitonal: stack({ 0, 4, 7, 6, 10, 13 }, false); break;
        case Voicing::cluster:
        {
            if (scale && scale->cents.size() >= 3)
            {
                // três graus vizinhos a partir do grau da fundamental
                const double rel = 100.0 * root - scale->tonic;
                const auto n = static_cast<int>(scale->cents.size());
                int k0 = 0;
                double bd = 1e18;
                for (int k = 0; k < n; ++k)
                {
                    double d = std::fmod(std::abs(rel - scale->cents[static_cast<std::size_t>(k)]), scale->period);
                    d = std::min(d, scale->period - d);
                    if (d < bd) { bd = d; k0 = k; }
                }
                const double octave = std::floor((rel - scale->cents[static_cast<std::size_t>(k0)]) / scale->period + 0.5);
                for (int j = 0; j < 3; ++j)
                {
                    const int idx = k0 + j;
                    const double c = scale->cents[static_cast<std::size_t>(idx % n)] + scale->period * (octave + idx / n);
                    out.push_back(static_cast<float>((scale->tonic + c) / 100.0));
                }
            }
            else
                stack({ 0, 1, 2 }, false);
            break;
        }
        case Voicing::planing:
        {
            // a forma do primeiro acorde, em paralelo
            if (shape.empty())
            {
                float last = -1.0f;
                for (float pc : chordPcs)
                {
                    float o = wrap12(pc - chordPcs.front());
                    while (o <= last) o += 12.0f;
                    shape.push_back(o);
                    last = o;
                }
            }
            for (float o : shape)
                out.push_back(root + o);
            break;
        }
        case Voicing::thirds:
        default:
        {
            float last = root - 1.0f;
            for (float pc : chordPcs)
            {
                float p = root + wrap12(pc - chordPcs.front());
                while (p <= last) p += 12.0f;
                out.push_back(p);
                last = p;
            }
            break;
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace trioio::composition
