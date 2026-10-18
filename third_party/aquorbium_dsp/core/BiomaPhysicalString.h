#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace aquorbium
{
class BiomaPhysicalString final
{
public:
    static constexpr std::size_t delayCapacity = 16384;
    static_assert((delayCapacity & (delayCapacity - 1)) == 0,
                  "Physical String ring capacity must remain a power of two");

    struct Parameters
    {
        float fundamentalHz = 110.0f;
        float decaySeconds = 1.6f;
        float brightness = 0.58f;
        float hardness = 0.52f;
        float pluckPosition = 0.22f;
    };

    struct PerformanceIntent
    {
        float influence = 0.0f;
        float decaySeconds = 1.6f;
        float brightness = 0.58f;
        float hardness = 0.52f;
        float pluckPosition = 0.22f;
    };

    struct Telemetry
    {
        Parameters parameters {};
        float effectiveDelaySamples = 0.0f;
        float fractionalDelay = 0.0f;
        float loopGain = 0.0f;
        float lossBlend = 0.0f;
        float stringEnergy = 0.0f;
        float excitationEnergy = 0.0f;
        float lastOutput = 0.0f;
        std::uint32_t integerDelaySamples = 0;
        std::uint32_t pluckDelaySamples = 0;
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
    [[nodiscard]] const Parameters& parameters() const noexcept
    {
        return currentParameters;
    }
    [[nodiscard]] const PerformanceIntent& performanceIntent() const noexcept
    {
        return performance;
    }

private:
    void updateEffectiveParameters() noexcept;
    void configureLoop() noexcept;
    [[nodiscard]] float nextNoise() noexcept;
    [[nodiscard]] static float energyBarrier(float value) noexcept;

    double sampleRate = 48000.0;
    std::uint32_t seed = 0x29b7a43du;
    std::uint32_t randomState = 0x29b7a43du;
    Parameters generativeParameters {};
    Parameters currentParameters {};
    PerformanceIntent performance {};

    std::array<float, delayCapacity> delayLine {};
    std::array<float, delayCapacity> excitationHistory {};
    std::size_t writeIndex = 0;
    std::size_t excitationWriteIndex = 0;
    std::uint32_t integerDelay = 436;
    std::uint32_t pluckDelay = 96;
    float fractionalDelay = 0.0f;
    float fractionalCoefficient = 0.0f;
    bool fractionalBypass = true;
    float fractionalInputPrevious = 0.0f;
    float fractionalOutputPrevious = 0.0f;
    float lossBlend = 0.2f;
    float lossInputPrevious = 0.0f;
    float loopGain = 0.99f;

    std::uint32_t excitationSamplesRemaining = 0;
    std::uint32_t excitationSamplesTotal = 0;
    std::uint32_t excitationSampleIndex = 0;
    float excitationEnvelope = 0.0f;
    float exciterLowpass = 0.0f;
    float exciterLowpassSecondary = 0.0f;
    float exciterCoefficient = 0.2f;
    float currentStringEnergy = 0.0f;
    float currentExcitationEnergy = 0.0f;
    float dcInputPrevious = 0.0f;
    float dcOutputPrevious = 0.0f;
    float dcCoefficient = 0.9987f;
    float currentOutput = 0.0f;
    std::uint32_t quietSamples = 0;
    std::uint64_t excitationCount = 0;
    bool dormant = true;
};
}
