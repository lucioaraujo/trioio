#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace antitotem
{
class NoisePalette
{
public:
    enum class Colour : unsigned char { white, pink, brown, blue, violet, bit };
    // seed 0 would stick xorshift at 0 forever, same caution as
    // SimpleSequencer::seedRandom - reject it in favour of 1.
    explicit NoisePalette(std::uint32_t seed = 0x6d2b79f5U) noexcept
        : seedValue(seed != 0U ? seed : 1U), state(seedValue) {}
    void reset() noexcept { state = seedValue; pink = brown = previous = 0.0f; }
    void setColour(Colour value) noexcept { colour = value; }
    [[nodiscard]] float tick() noexcept
    {
        state ^= state << 13U; state ^= state >> 17U; state ^= state << 5U;
        const auto white = static_cast<float>(state) * (1.0f / 2147483648.0f) - 1.0f;
        const auto blue = std::clamp((white - previous) * 0.7f, -1.0f, 1.0f);
        previous = white;
        pink = std::clamp(pink * 0.985f + white * 0.07f, -1.0f, 1.0f);
        brown = std::clamp(brown + white * 0.018f, -1.0f, 1.0f);
        if (colour == Colour::pink) return pink;
        if (colour == Colour::brown) return brown;
        if (colour == Colour::blue) return blue;
        if (colour == Colour::violet) return std::clamp((blue - pink) * 0.8f, -1.0f, 1.0f);
        if (colour == Colour::bit) return (state & 0x10000U) != 0U ? 1.0f : -1.0f;
        return white;
    }

private:
    std::uint32_t seedValue;
    std::uint32_t state;
    float pink = 0.0f, brown = 0.0f, previous = 0.0f;
    Colour colour = Colour::white;
};

class SampleHold
{
public:
    void prepare(double value) noexcept
    {
        sampleRate = std::max(100.0, value);
        // A very short charge time models the hold capacitor and prevents a
        // full-scale discontinuity from becoming a digital click.
        slewCoefficient = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate * 0.00035)));
        reset();
    }
    void reset() noexcept { elapsed = 0.0; held = slewed = 0.0f; }
    void setRate(float hertz) noexcept { rate = std::clamp(hertz, 0.05f, 120.0f); }
    [[nodiscard]] float process(float input) noexcept
    {
        if (++elapsed >= sampleRate / rate) { elapsed = 0.0; held = input; }
        slewed += (held - slewed) * slewCoefficient;
        return slewed;
    }

private:
    double sampleRate = 48000.0, elapsed = 0.0;
    float rate = 7.0f, held = 0.0f, slewed = 0.0f, slewCoefficient = 0.05f;
};
} // namespace antitotem
