#include "core/BiomaGranularStriker.h"

#include "core/Numeric.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace aquorbium
{
namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float twoPi = 6.28318530717958647692f;

float interpolate(float left, float right, float amount) noexcept
{
    return left + (right - left) * amount;
}

float softLimit(float value) noexcept
{
    return value / (1.0f + std::abs(value));
}
}

void BiomaGranularStriker::prepare(double value) noexcept
{
    sampleRate = numeric::sampleRate(value);
    setParameters(generativeParameters);
    reset();
}

void BiomaGranularStriker::setSeed(std::uint32_t value) noexcept
{
    seed = value == 0 ? 1u : value;
    reset();
}

void BiomaGranularStriker::reset() noexcept
{
    randomState = seed;
    silenceParticles();
    particlesInLastStrike = 0;
    currentOutput = 0.0f;
    excitationCount = 0;
    stolenParticleCount = 0;
}

void BiomaGranularStriker::setParameters(const Parameters& value) noexcept
{
    const auto maximumCenter = static_cast<float>(sampleRate * 0.42);
    generativeParameters.centerHz = numeric::clampFinite(
        value.centerHz, 20.0f, maximumCenter, generativeParameters.centerHz);
    generativeParameters.grainDurationMs = numeric::clampFinite(
        value.grainDurationMs, 2.0f, 40.0f, generativeParameters.grainDurationMs);
    generativeParameters.particleDensity = numeric::unit(
        value.particleDensity, generativeParameters.particleDensity);
    generativeParameters.spectralSpread = numeric::unit(
        value.spectralSpread, generativeParameters.spectralSpread);
    generativeParameters.resonance = numeric::unit(
        value.resonance, generativeParameters.resonance);
    generativeParameters.irregularity = numeric::unit(
        value.irregularity, generativeParameters.irregularity);
    updateEffectiveParameters();
}

void BiomaGranularStriker::setPerformanceIntent(
    const PerformanceIntent& value) noexcept
{
    performance.influence = numeric::unit(value.influence, performance.influence);
    performance.grainDurationMs = numeric::clampFinite(
        value.grainDurationMs, 2.0f, 40.0f, performance.grainDurationMs);
    performance.particleDensity = numeric::unit(
        value.particleDensity, performance.particleDensity);
    performance.spectralSpread = numeric::unit(
        value.spectralSpread, performance.spectralSpread);
    performance.resonance = numeric::unit(value.resonance, performance.resonance);
    performance.irregularity = numeric::unit(
        value.irregularity, performance.irregularity);
    updateEffectiveParameters();
}

void BiomaGranularStriker::updateEffectiveParameters() noexcept
{
    const auto amount = performance.influence;
    effectiveParameters.centerHz = generativeParameters.centerHz;
    effectiveParameters.grainDurationMs = interpolate(
        generativeParameters.grainDurationMs, performance.grainDurationMs, amount);
    effectiveParameters.particleDensity = interpolate(
        generativeParameters.particleDensity, performance.particleDensity, amount);
    effectiveParameters.spectralSpread = interpolate(
        generativeParameters.spectralSpread, performance.spectralSpread, amount);
    effectiveParameters.resonance = interpolate(
        generativeParameters.resonance, performance.resonance, amount);
    effectiveParameters.irregularity = interpolate(
        generativeParameters.irregularity, performance.irregularity, amount);
}

void BiomaGranularStriker::excite(float amount) noexcept
{
    if (!std::isfinite(amount) || amount <= 0.0f)
        return;
    const auto boundedAmount = std::clamp(amount, 0.0f, 1.0f);
    const auto span = maximumParticlesPerStrike - minimumParticlesPerStrike;
    const auto particleCount = minimumParticlesPerStrike
        + static_cast<std::size_t>(std::lround(
            effectiveParameters.particleDensity * static_cast<float>(span)));
    particlesInLastStrike = std::min(particleCount, maximumParticlesPerStrike);
    for (std::size_t index = 0; index < particlesInLastStrike; ++index)
        configureParticle(particles[acquireParticle()], boundedAmount,
                          particlesInLastStrike);
    ++excitationCount;
}

std::size_t BiomaGranularStriker::acquireParticle() noexcept
{
    for (std::size_t index = 0; index < particles.size(); ++index)
        if (!particles[index].active)
            return index;

    std::size_t quietest = 0;
    auto quietestEnergy = std::numeric_limits<float>::max();
    for (std::size_t index = 0; index < particles.size(); ++index)
    {
        const auto energy = particles[index].envelope * particles[index].amplitude;
        if (energy < quietestEnergy)
        {
            quietest = index;
            quietestEnergy = energy;
        }
    }
    ++stolenParticleCount;
    return quietest;
}

void BiomaGranularStriker::configureParticle(Particle& particle, float amount,
                                             std::size_t particleCount) noexcept
{
    const auto durationVariation = 0.65f + nextRandomUnit() * 0.70f;
    const auto durationMs = std::clamp(
        effectiveParameters.grainDurationMs * durationVariation, 2.0f, 40.0f);
    const auto durationSamples = static_cast<std::uint32_t>(std::max(
        1.0, std::round(static_cast<double>(durationMs) * sampleRate * 0.001)));
    const auto scatterMs = effectiveParameters.grainDurationMs
        * (0.15f + effectiveParameters.irregularity * 2.5f);
    const auto maximumDelay = static_cast<std::uint32_t>(std::max(
        0.0, std::round(static_cast<double>(scatterMs) * sampleRate * 0.001)));
    const auto bipolar = nextRandomUnit() * 2.0f - 1.0f;
    const auto octaveOffset = bipolar * effectiveParameters.spectralSpread * 2.0f;
    const auto maximumFrequency = static_cast<float>(sampleRate * 0.42);
    const auto frequency = std::clamp(
        effectiveParameters.centerHz * std::exp2(octaveOffset), 20.0f,
        maximumFrequency);
    const auto q = 20.0f + effectiveParameters.resonance
        * effectiveParameters.resonance * 80.0f;
    const auto omega = twoPi * frequency / static_cast<float>(sampleRate);
    const auto sine = std::sin(omega);
    const auto cosine = std::cos(omega);
    const auto alpha = sine / (2.0f * q);
    const auto inverseA0 = 1.0f / (1.0f + alpha);

    particle.b0 = alpha * inverseA0;
    particle.b2 = -particle.b0;
    particle.a1 = -2.0f * cosine * inverseA0;
    particle.a2 = (1.0f - alpha) * inverseA0;
    particle.z1 = 0.0f;
    particle.z2 = 0.0f;
    particle.envelope = 1.0f;
    particle.envelopeDecay = std::exp(
        std::log(0.001f) / static_cast<float>(durationSamples));
    particle.amplitude = amount * (0.72f + nextRandomUnit() * 0.28f)
        / std::sqrt(static_cast<float>(particleCount));
    particle.delaySamples = maximumDelay == 0
        ? 0u : static_cast<std::uint32_t>(nextRandomUnit()
            * static_cast<float>(maximumDelay + 1u));
    particle.remainingSamples = durationSamples;
    particle.active = true;
}

float BiomaGranularStriker::tick() noexcept
{
    float sum = 0.0f;
    for (auto& particle : particles)
    {
        if (!particle.active)
            continue;
        if (particle.delaySamples > 0)
        {
            --particle.delaySamples;
            continue;
        }

        const auto input = nextNoise() * particle.envelope;
        const auto filtered = particle.b0 * input + particle.z1;
        particle.z1 = -particle.a1 * filtered + particle.z2;
        particle.z2 = particle.b2 * input - particle.a2 * filtered;
        sum += filtered * particle.amplitude;
        particle.envelope *= particle.envelopeDecay;
        if (--particle.remainingSamples == 0)
            particle.active = false;
    }

    currentOutput = softLimit(sum * 7.5f);
    if (!std::isfinite(currentOutput))
    {
        silenceParticles();
        currentOutput = 0.0f;
    }
    return currentOutput;
}

BiomaGranularStriker::Telemetry BiomaGranularStriker::telemetry() const noexcept
{
    Telemetry result;
    result.parameters = effectiveParameters;
    result.particlesInLastStrike = particlesInLastStrike;
    result.lastOutput = currentOutput;
    result.excitationCount = excitationCount;
    result.stolenParticleCount = stolenParticleCount;
    for (const auto& particle : particles)
    {
        if (!particle.active)
            continue;
        ++result.activeParticles;
        if (particle.delaySamples > 0)
            ++result.scheduledParticles;
        result.particleEnergy += particle.envelope * particle.amplitude;
    }
    result.active = result.activeParticles > 0;
    return result;
}

float BiomaGranularStriker::nextRandomUnit() noexcept
{
    randomState ^= randomState << 13u;
    randomState ^= randomState >> 17u;
    randomState ^= randomState << 5u;
    return static_cast<float>(randomState & 0x00ffffffu)
        / static_cast<float>(0x01000000u);
}

float BiomaGranularStriker::nextNoise() noexcept
{
    return nextRandomUnit() * 2.0f - 1.0f;
}

void BiomaGranularStriker::silenceParticles() noexcept
{
    particles.fill({});
}
}
