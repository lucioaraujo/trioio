#include "Mashup.h"

#include <sstream>

namespace trioio::composition
{

bool parseMashupLayers(const std::string& text, unsigned& out)
{
    out = 0;
    std::istringstream in(text);
    std::string w;
    while (std::getline(in, w, ','))
    {
        w.erase(0, w.find_first_not_of(" \t"));
        w.erase(w.find_last_not_of(" \t") + 1);
        if (w == "harmonia") out |= kMashHarmony;
        else if (w == "ritmo") out |= kMashRhythm;
        else if (w == "memoria" || w == "memória") out |= kMashMemory;
        else if (w == "corpo") out |= kMashBody;
        else if (w == "tudo") out |= kMashHarmony | kMashRhythm | kMashMemory | kMashBody;
        else if (!w.empty()) return false;
    }
    return out != 0;
}

std::string mashupLayersName(unsigned l)
{
    std::string s;
    const auto add = [&](const char* n) { s += (s.empty() ? "" : ", ") + std::string(n); };
    if (l & kMashHarmony) add("harmonia");
    if (l & kMashRhythm) add("ritmo");
    if (l & kMashMemory) add("memória");
    if (l & kMashBody) add("corpo");
    return s;
}

void remapTimeline(std::vector<perception::Frame>& frames, std::vector<perception::Onset>& onsets, double from, double to)
{
    if (from <= 0.0 || to <= 0.0)
        return;
    const double k = to / from;
    for (auto& f : frames)
        f.time *= k;
    for (auto& o : onsets)
        o.time *= k;
}

} // namespace trioio::composition
