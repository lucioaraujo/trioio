#include "WindowPolicy.h"

#include <algorithm>
#include <cmath>

namespace trioio::app
{

Regime classify(int width, int height) noexcept
{
    if (width < kCompactW || height < kCompactH)
        return Regime::emergency;
    if (width < kFullW)
        return Regime::compact;
    return Regime::full;
}

const char* regimeName(Regime r) noexcept
{
    switch (r)
    {
        case Regime::full: return "completo";
        case Regime::compact: return "compacto";
        case Regime::emergency: return "emergência";
    }
    return "?";
}

Rect initialBounds(const Rect& area, double fraction) noexcept
{
    if (area.empty())
        return { 0, 0, kCompactW, kCompactH };
    int w = static_cast<int>(std::lround(area.w * fraction));
    int h = static_cast<int>(std::lround(area.h * fraction));
    w = std::min(w, kCanvasW);
    h = std::min(h, kCanvasH);
    w = std::min(std::max(w, kMinW), area.w);
    h = std::min(std::max(h, kMinH), area.h);
    return { area.x + (area.w - w) / 2, area.y + (area.h - h) / 2, w, h };
}

namespace
{
int overlap(int a0, int a1, int b0, int b1) noexcept { return std::max(0, std::min(a1, b1) - std::max(a0, b0)); }
} // namespace

Rect revalidate(const Rect& saved, const std::vector<Rect>& areas) noexcept
{
    if (areas.empty())
        return saved.empty() ? Rect { 0, 0, kCompactW, kCompactH } : saved;
    if (!saved.empty())
        for (const auto& a : areas)
        {
            const long visible = static_cast<long>(overlap(saved.x, saved.x + saved.w, a.x, a.x + a.w))
                               * overlap(saved.y, saved.y + saved.h, a.y, a.y + a.h);
            const bool titleVisible = saved.y >= a.y && saved.y + 40 <= a.y + a.h
                                   && overlap(saved.x, saved.x + saved.w, a.x, a.x + a.w) >= 100;
            if (2 * visible >= static_cast<long>(saved.w) * saved.h && titleVisible)
            {
                Rect r = saved;
                r.w = std::min(std::max(r.w, std::min(kMinW, a.w)), a.w);
                r.h = std::min(std::max(r.h, std::min(kMinH, a.h)), a.h);
                r.x = std::clamp(r.x, a.x, a.x + a.w - r.w);
                r.y = std::clamp(r.y, a.y, a.y + a.h - r.h);
                return r;
            }
        }
    return initialBounds(areas.front());
}

} // namespace trioio::app
