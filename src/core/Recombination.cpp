#include "Recombination.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trioio::composition
{

bool parseMixTechnique(const std::string& n, MixTechnique& out)
{
    if (n == "colagem") out = MixTechnique::collage;
    else if (n == "hibrido" || n == "híbrido") out = MixTechnique::hybrid;
    else if (n == "camadas") out = MixTechnique::layers;
    else if (n == "transicao" || n == "transição") out = MixTechnique::transition;
    else return false;
    return true;
}

const char* mixTechniqueName(MixTechnique t)
{
    switch (t)
    {
        case MixTechnique::collage: return "colagem";
        case MixTechnique::hybrid: return "híbrido";
        case MixTechnique::layers: return "camadas";
        case MixTechnique::transition: return "transição";
    }
    return "?";
}

MachineScale machineFromPick(const ArchivePick& p)
{
    MachineScale m;
    m.tonic = 100.0 * p.choice.tonic;
    for (float s : p.choice.steps)
        m.cents.push_back(100.0 * s);
    m.period = 100.0 * p.period;
    m.name = p.choice.name;
    return m;
}

ArchivePick hybridPick(const ArchivePick& low, const ArchivePick& high)
{
    ArchivePick h = low;
    std::vector<float> steps;
    for (float s : low.choice.steps)
        if (s < 6.9f)
            steps.push_back(s);
    for (float s : high.choice.steps)
        if (s >= 6.9f && s < low.period - 0.05f)
            steps.push_back(s);
    std::sort(steps.begin(), steps.end());
    steps.erase(std::unique(steps.begin(), steps.end(), [](float a, float b) { return std::abs(a - b) < 0.05f; }), steps.end());
    h.choice.steps = steps;
    h.choice.name = "híbrido: " + low.choice.name + " embaixo + " + high.choice.name + " em cima";
    h.source = low.source + " | " + high.source;
    // o comportamento de baixo, o ornamento de cima (enxerto)
    h.behavior.up.clear();    // as formas de subida e descida eram de uma escala só
    h.behavior.down.clear();
    if (!high.behavior.ornament.empty())
        h.behavior.ornament = high.behavior.ornament;
    h.behavior.source = low.behavior.source.empty() ? high.behavior.source : low.behavior.source;
    return h;
}

std::string layerSecondScale(Score& score, const std::vector<SectionScale>& sections, const ArchivePick& other)
{
    if (sections.empty())
        return {};
    const auto o = machineFromPick(other);
    int moved = 0;
    for (auto& n : score.notes)
    {
        if (n.voice != 2 && n.voice != 6)
            continue;
        const SectionScale* sc = &sections.front();
        for (const auto& x : sections)
            if (n.time + 1e-6 >= x.start)
                sc = &x;
        // a escala `other` sobre a tônica da seção (o mesmo centro)
        const double T = sc->scale.tonic, P = o.period;
        const double rel = 100.0 * n.pitch - T;
        double best = rel, bestD = 1e18;
        for (double c : o.cents)
        {
            const double m0 = std::round((rel - c) / P);
            for (double m = m0 - 1; m <= m0 + 1; m += 1.0)
            {
                const double cand = c + m * P;
                if (std::abs(cand - rel) < bestD) { bestD = std::abs(cand - rel); best = cand; }
            }
        }
        const float p = static_cast<float>((T + best) / 100.0);
        moved += std::abs(p - n.pitch) > 0.01f;
        n.pitch = p;
    }
    char text[400];
    std::snprintf(text, sizeof text,
                  "mistura (camadas): a segunda voz e a harmonia em %s, sobre o centro da melodia (%d notas movidas) · "
                  "polimodalidade (Bartók; Stravinsky)",
                  other.choice.name.c_str(), moved);
    return text;
}

namespace
{
// a altura mais perto de `pitch` num conjunto de alturas absolutas (cents 0..1200)
float snapAbs(float pitch, const std::vector<double>& absCents)
{
    if (absCents.empty())
        return pitch;
    const double here = 100.0 * pitch;
    double best = here, bestD = 1e18;
    for (double c : absCents)
    {
        const double m0 = std::round((here - c) / 1200.0);
        for (double m = m0 - 1; m <= m0 + 1; m += 1.0)
        {
            const double cand = c + 1200.0 * m;
            if (std::abs(cand - here) < bestD) { bestD = std::abs(cand - here); best = cand; }
        }
    }
    return static_cast<float>(best / 100.0);
}

std::vector<double> absolute(const MachineScale& s)
{
    std::vector<double> out;
    for (double c : s.cents)
        out.push_back(std::fmod(std::fmod(s.tonic + c, 1200.0) + 1200.0, 1200.0));
    return out;
}
} // namespace

bool parseSeam(const std::string& n, Seam& out)
{
    if (n == "corte") out = Seam::cut;
    else if (n == "pivo" || n == "pivô") out = Seam::pivot;
    else if (n == "ponte") out = Seam::bridge;
    else if (n == "sobreposicao" || n == "sobreposição") out = Seam::overlap;
    else return false;
    return true;
}

const char* seamName(Seam s)
{
    switch (s)
    {
        case Seam::cut: return "corte";
        case Seam::pivot: return "pivô por notas comuns";
        case Seam::bridge: return "ponte pela escala intermediária";
        case Seam::overlap: return "sobreposição (crossfade)";
    }
    return "?";
}

std::string seamSections(Score& score, const std::vector<SectionScale>& sections, Seam seam, double barLength)
{
    if (seam == Seam::cut || sections.size() < 2)
        return {};
    const double bar = barLength >= 1.0 && barLength <= 6.0 ? barLength : 2.0;
    int seams = 0, moved = 0, held = 0, copied = 0, commonTotal = 0;
    std::vector<Note> added;
    for (std::size_t k = 0; k + 1 < sections.size(); ++k)
    {
        const auto& a = sections[k];
        const auto& b = sections[k + 1];
        const double edge = b.start;
        const auto absA = absolute(a.scale), absB = absolute(b.scale);
        switch (seam)
        {
            case Seam::pivot:
            {
                std::vector<double> common;
                for (double x : absA)
                    if (std::any_of(absB.begin(), absB.end(), [&](double y) {
                            const double d = std::abs(x - y);
                            return std::min(d, 1200.0 - d) <= 15.0;
                        }))
                        common.push_back(x);
                commonTotal += static_cast<int>(common.size());
                if (common.empty())
                    common = { std::fmod(b.scale.tonic, 1200.0) };  // sem comuns: a tônica de quem entra
                for (auto& n : score.notes)
                    if (n.voice == 0 && n.time >= edge - bar && n.time < edge)
                    {
                        const float p = snapAbs(n.pitch, common);
                        moved += std::abs(p - n.pitch) > 0.01f;
                        n.pitch = p;
                    }
                // a nota comum sustentada que atravessa a fronteira
                Note h;
                h.time = edge - bar;
                h.duration = 2.0 * bar;
                h.pitch = snapAbs(57.0f, common);
                h.velocity = 0.35f;
                h.voice = 6;
                h.timbre = Timbre::breath;
                if (h.time >= 0.0 && h.time + h.duration < score.duration - 0.05)
                {
                    added.push_back(h);
                    ++held;
                }
                break;
            }
            case Seam::bridge:
            {
                const auto mid = interpolateScales(a.scale, b.scale, 0.5);
                const auto absM = absolute(mid);
                for (auto& n : score.notes)
                    if ((n.voice == 0 || n.voice == 2) && n.time >= edge - bar && n.time < edge + bar)
                    {
                        const float p = snapAbs(n.pitch, absM);
                        moved += std::abs(p - n.pitch) > 0.01f;
                        n.pitch = p;
                    }
                break;
            }
            case Seam::overlap:
            {
                for (const auto& n : score.notes)
                    if ((n.voice == 4 || n.voice == 6) && n.time >= edge - bar && n.time < edge)
                    {
                        Note c = n;
                        c.time += bar;
                        c.velocity *= 0.6f;
                        if (c.time + c.duration < score.duration - 0.05)
                        {
                            added.push_back(c);
                            ++copied;
                        }
                    }
                break;
            }
            case Seam::cut: break;
        }
        ++seams;
    }
    score.notes.insert(score.notes.end(), added.begin(), added.end());
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& x, const Note& y) {
        return x.time < y.time || (x.time == y.time && x.voice < y.voice);
    });
    char text[300];
    std::snprintf(text, sizeof text, "costuras (%s): %d fronteiras · %d notas movidas, %d notas comuns sustentadas, %d golpes e acordes sobrepostos%s",
                  seamName(seam), seams, moved, held, copied,
                  seam == Seam::pivot ? (" · " + std::to_string(commonTotal) + " notas comuns ao todo").c_str() : "");
    return text;
}

} // namespace trioio::composition
