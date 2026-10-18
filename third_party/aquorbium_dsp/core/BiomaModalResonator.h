#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace aquorbium
{
class BiomaModalResonator final
{
public:
    static constexpr std::size_t modeCount = 4;

    struct Parameters
    {
        float fundamentalHz = 110.0f;
        float material = 0.5f;
        float decaySeconds = 1.2f;
        float brightness = 0.5f;
        float hardness = 0.5f;
        float strikePosition = 0.5f;
    };

    struct PerformanceIntent
    {
        float influence = 0.0f;
        float material = 0.5f;
        float decaySeconds = 1.2f;
        float brightness = 0.5f;
        float hardness = 0.5f;
        float strikePosition = 0.5f;
    };

    struct Telemetry
    {
        Parameters parameters {};
        std::array<float, modeCount> modeFrequencies {};
        std::array<float, modeCount> poleRadii {};
        float modalEnergy = 0.0f;
        float exciterEnergy = 0.0f;
        float lastOutput = 0.0f;
        std::uint64_t excitationCount = 0;
        bool active = false;
    };

    void prepare(double sampleRate) noexcept;
    void setSeed(std::uint32_t value) noexcept;
    void reset() noexcept;
    void setParameters(const Parameters& value) noexcept;
    void setPerformanceIntent(const PerformanceIntent& value) noexcept;
    void excite(float amount) noexcept;
    [[nodiscard]] float tick() noexcept;
    [[nodiscard]] Telemetry telemetry() const noexcept;
    [[nodiscard]] const Parameters& parameters() const noexcept { return currentParameters; }
    [[nodiscard]] const PerformanceIntent& performanceIntent() const noexcept
    {
        return performance;
    }

private:
    struct Mode
    {
        float real = 0.0f;
        float imaginary = 0.0f;
        float rotationCos = 1.0f;
        float rotationSin = 0.0f;
        float radius = 0.0f;
        float inputGain = 0.0f;
        float frequency = 0.0f;
    };

    void updateCoefficients() noexcept;
    void updateEffectiveParameters() noexcept;
    [[nodiscard]] float nextNoise() noexcept;
    void silenceState() noexcept;

    double sampleRate = 48000.0;
    std::uint32_t seed = 0x7a43c219u;
    std::uint32_t randomState = 0x7a43c219u;
    Parameters generativeParameters {};
    Parameters currentParameters {};
    PerformanceIntent performance {};
    std::array<Mode, modeCount> modes {};
    float burstEnvelope = 0.0f;
    float burstDecay = 0.0f;
    float exciterLowpass = 0.0f;
    float currentModalEnergy = 0.0f;
    float currentOutput = 0.0f;
    std::uint64_t excitationCount = 0;
};
}
