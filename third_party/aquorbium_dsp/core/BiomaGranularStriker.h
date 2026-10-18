#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace aquorbium
{
class BiomaGranularStriker final
{
public:
    static constexpr std::size_t particleCapacity = 24;
    static constexpr std::size_t minimumParticlesPerStrike = 2;
    static constexpr std::size_t maximumParticlesPerStrike = 12;

    struct Parameters
    {
        float centerHz = 440.0f;
        float grainDurationMs = 12.0f;
        float particleDensity = 0.5f;
        float spectralSpread = 0.45f;
        float resonance = 0.5f;
        float irregularity = 0.5f;
    };

    struct PerformanceIntent
    {
        float influence = 0.0f;
        float grainDurationMs = 12.0f;
        float particleDensity = 0.5f;
        float spectralSpread = 0.45f;
        float resonance = 0.5f;
        float irregularity = 0.5f;
    };

    struct Telemetry
    {
        Parameters parameters {};
        std::size_t activeParticles = 0;
        std::size_t scheduledParticles = 0;
        std::size_t particlesInLastStrike = 0;
        float particleEnergy = 0.0f;
        float lastOutput = 0.0f;
        std::uint64_t excitationCount = 0;
        std::uint64_t stolenParticleCount = 0;
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
    [[nodiscard]] const Parameters& parameters() const noexcept { return effectiveParameters; }
    [[nodiscard]] const PerformanceIntent& performanceIntent() const noexcept
    {
        return performance;
    }

private:
    struct Particle
    {
        float b0 = 0.0f;
        float b2 = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
        float z1 = 0.0f;
        float z2 = 0.0f;
        float envelope = 0.0f;
        float envelopeDecay = 0.0f;
        float amplitude = 0.0f;
        std::uint32_t delaySamples = 0;
        std::uint32_t remainingSamples = 0;
        bool active = false;
    };

    void updateEffectiveParameters() noexcept;
    [[nodiscard]] std::size_t acquireParticle() noexcept;
    void configureParticle(Particle& particle, float amount,
                           std::size_t particleCount) noexcept;
    [[nodiscard]] float nextRandomUnit() noexcept;
    [[nodiscard]] float nextNoise() noexcept;
    void silenceParticles() noexcept;

    double sampleRate = 48000.0;
    std::uint32_t seed = 0x37a4d91bu;
    std::uint32_t randomState = 0x37a4d91bu;
    Parameters generativeParameters {};
    Parameters effectiveParameters {};
    PerformanceIntent performance {};
    std::array<Particle, particleCapacity> particles {};
    std::size_t particlesInLastStrike = 0;
    float currentOutput = 0.0f;
    std::uint64_t excitationCount = 0;
    std::uint64_t stolenParticleCount = 0;
};
}
