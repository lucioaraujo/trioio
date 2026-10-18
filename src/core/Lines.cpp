#include "Lines.h"

#include <algorithm>
#include <cmath>

namespace trioio::composition
{

LineValidity assessLine(double lowHz, double highHz, double sampleRate, const Reading& line, const Reading& whole,
                        double onsetsPerSecond)
{
    LineValidity v;
    const double lo = lowHz, hi = highHz > 0.0 ? highHz : sampleRate * 0.5;
    v.pulse = line.pulse.confidence >= 0.25f && onsetsPerSecond >= 0.5;
    const double ratio = whole.pulse.bpm > 0.0 ? line.pulse.bpm / whole.pulse.bpm : 0.0;
    v.agrees = std::abs(ratio - 1.0) <= 0.04 || std::abs(ratio - 0.5) <= 0.02 || std::abs(ratio - 2.0) <= 0.08;
    v.melody = line.pitchedShare >= 0.25f && lo < 2000.0 && hi > 400.0;
    v.bass = hi <= 300.0;
    v.chords = lo < 1500.0 && hi > 300.0 && !line.chords.empty();
    return v;
}

std::string borrowPitch(std::vector<perception::Frame>& lineFrames, const std::vector<perception::Frame>& whole,
                        const LineValidity& v)
{
    if (v.melody && v.chords)
        return "nada";
    const std::size_t n = std::min(lineFrames.size(), whole.size());
    for (std::size_t i = 0; i < n; ++i)
    {
        auto& f = lineFrames[i];
        const auto& w = whole[i];
        f.chroma = w.chroma;
        f.melodyHz = w.melodyHz;
        f.melodySalience = w.melodySalience;
        f.melodyCandHz = w.melodyCandHz;
        f.melodyCandSalience = w.melodyCandSalience;
        f.f0Hz = w.f0Hz;
        f.periodicity = w.periodicity;
        f.peaksHz = w.peaksHz;
        if (!v.bass)
            f.bassChroma = w.bassChroma;
    }
    return v.bass ? "o croma, a melodia e o YIN da peça (as fundamentais são da linha)"
                  : "as alturas da peça (croma, melodia, YIN, picos)";
}

void bandRegister(double lowHz, double highHz, float& lowMidi, float& highMidi)
{
    const auto midi = [](double hz) { return static_cast<float>(69.0 + 12.0 * std::log2(hz / 440.0)); };
    lowMidi = midi(std::max(lowHz, 32.7));  // Dó1
    highMidi = std::min(108.0f, midi(highHz > 0.0 ? highHz : std::max(lowHz, 32.7) * 4.0));
    if (highMidi - lowMidi < 24.0f)
        lowMidi = highMidi - 24.0f;
}

void foldIntoRegister(Score& score, float lowMidi, float highMidi)
{
    for (auto& n : score.notes)
    {
        if (n.voice == 4)
            continue;
        while (n.pitch < lowMidi)
            n.pitch += 12.0f;
        while (n.pitch > highMidi && n.pitch - 12.0f >= lowMidi)
            n.pitch -= 12.0f;
    }
}

} // namespace trioio::composition
