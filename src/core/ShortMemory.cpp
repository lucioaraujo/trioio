#include "ShortMemory.h"

#include <algorithm>
#include <cmath>

namespace trioio::perception
{

ShortMemory::ShortMemory(double seconds, double hopSeconds)
    : seconds_(std::max(0.1, seconds)), hop_(std::max(1e-3, hopSeconds))
{
    ring_.resize(static_cast<std::size_t>(std::ceil(seconds_ / hop_)) + 1);
    // até 30 ataques por segundo (o rufo mais denso que a escuta separa)
    onsetRing_.resize(static_cast<std::size_t>(std::ceil(seconds_ * 30.0)) + 1);
}

void ShortMemory::push(const Frame& f) noexcept
{
    ring_[head_] = f;
    head_ = (head_ + 1) % ring_.size();
    frames_ = std::min(frames_ + 1, ring_.size());
}

void ShortMemory::pushOnset(const Onset& o) noexcept
{
    onsetRing_[onsetHead_] = o;
    onsetHead_ = (onsetHead_ + 1) % onsetRing_.size();
    onsets_ = std::min(onsets_ + 1, onsetRing_.size());
}

void ShortMemory::clear() noexcept
{
    head_ = frames_ = onsetHead_ = onsets_ = 0;
}

double ShortMemory::newestTime() const noexcept
{
    if (frames_ == 0)
        return 0.0;
    return ring_[(head_ + ring_.size() - 1) % ring_.size()].time;
}

std::vector<Frame> ShortMemory::recent(double window) const
{
    std::vector<Frame> out;
    if (frames_ == 0)
        return out;
    const double from = newestTime() - std::min(window, seconds_);
    for (std::size_t k = 0; k < frames_; ++k)
    {
        const auto& f = ring_[(head_ + ring_.size() - frames_ + k) % ring_.size()];
        if (f.time > from - 1e-9)
            out.push_back(f);
    }
    return out;
}

std::vector<Onset> ShortMemory::recentOnsets(double window) const
{
    std::vector<Onset> out;
    const double from = newestTime() - std::min(window, seconds_);
    for (std::size_t k = 0; k < onsets_; ++k)
    {
        const auto& o = onsetRing_[(onsetHead_ + onsetRing_.size() - onsets_ + k) % onsetRing_.size()];
        if (o.time > from - 1e-9)
            out.push_back(o);
    }
    return out;
}

ShortMemory::Summary ShortMemory::summary(double window) const
{
    Summary s;
    const auto fr = recent(window);
    if (fr.empty())
        return s;
    double db = 0.0, cen = 0.0, sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    int sounding = 0, pitched = 0;
    std::vector<float> f0;
    for (const auto& f : fr)
    {
        if (f.silent)
            continue;
        ++sounding;
        db += f.rmsDb;
        cen += f.centroidHz;
        s.peakDb = std::max(s.peakDb, f.rmsDb);
        sx += f.time;
        sy += f.rmsDb;
        sxx += f.time * f.time;
        sxy += f.time * f.rmsDb;
        if (f.f0Hz > 0.0f && f.periodicity >= 0.6f)
        {
            ++pitched;
            f0.push_back(f.f0Hz);
        }
    }
    s.silentShare = 1.0f - static_cast<float>(sounding) / static_cast<float>(fr.size());
    if (sounding > 0)
    {
        s.meanDb = static_cast<float>(db / sounding);
        s.centroidHz = static_cast<float>(cen / sounding);
        s.pitchedShare = static_cast<float>(pitched) / static_cast<float>(sounding);
        const double den = sounding * sxx - sx * sx;
        if (sounding >= 3 && std::abs(den) > 1e-12)
            s.trendDbPerSecond = static_cast<float>((sounding * sxy - sx * sy) / den);
    }
    if (!f0.empty())
    {
        std::nth_element(f0.begin(), f0.begin() + static_cast<std::ptrdiff_t>(f0.size() / 2), f0.end());
        s.medianF0Hz = f0[f0.size() / 2];
    }
    const double span = std::max(hop_, fr.back().time - fr.front().time + hop_);
    s.onsetsPerSecond = static_cast<float>(static_cast<double>(recentOnsets(window).size()) / span);
    return s;
}

} // namespace trioio::perception
