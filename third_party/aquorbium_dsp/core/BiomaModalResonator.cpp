#include "core/BiomaModalResonator.h"

#include "core/Numeric.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace aquorbium
{
namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float twoPi = 6.28318530717958647692f;
constexpr std::array<float, BiomaModalResonator::modeCount> harmonicRatios {
    1.0f, 2.0f, 3.0f, 4.0f
};
constexpr std::array<float, BiomaModalResonator::modeCount> membraneRatios {
    1.0f, 1.5933f, 2.1355f, 2.2954f
};
constexpr std::array<float, BiomaModalResonator::modeCount> darkModeWeights {
    1.0f, 0.44f, 0.20f, 0.11f
};
constexpr std::array<float, BiomaModalResonator::modeCount> brightModeWeights {
    0.78f, 0.68f, 0.52f, 0.38f
};

float interpolate(float left, float right, float amount) noexcept
{
    return left + (right - left) * amount;
}
}

void BiomaModalResonator::prepare(double value) noexcept
{
    sampleRate = numeric::sampleRate(value);
    updateCoefficients();
    reset();
}

void BiomaModalResonator::setSeed(std::uint32_t value) noexcept
{
    seed = value == 0 ? 1u : value;
    reset();
}

void BiomaModalResonator::reset() noexcept
{
    randomState = seed;
    burstEnvelope = 0.0f;
    exciterLowpass = 0.0f;
    currentModalEnergy = 0.0f;
    currentOutput = 0.0f;
    excitationCount = 0;
    for (auto& mode : modes)
    {
        mode.real = 0.0f;
        mode.imaginary = 0.0f;
    }
}

void BiomaModalResonator::setParameters(const Parameters& value) noexcept
{
    const auto maximumFundamental = static_cast<float>(sampleRate * 0.45 / 4.0);
    generativeParameters.fundamentalHz = numeric::clampFinite(
        value.fundamentalHz, 10.0f, maximumFundamental,
        generativeParameters.fundamentalHz);
    generativeParameters.material = numeric::unit(value.material, generativeParameters.material);
    generativeParameters.decaySeconds = numeric::clampFinite(
        value.decaySeconds, 0.04f, 8.0f, generativeParameters.decaySeconds);
    generativeParameters.brightness = numeric::unit(
        value.brightness, generativeParameters.brightness);
    generativeParameters.hardness = numeric::unit(value.hardness, generativeParameters.hardness);
    generativeParameters.strikePosition = numeric::clampFinite(
        value.strikePosition, 0.02f, 0.98f, generativeParameters.strikePosition);
    updateEffectiveParameters();
}

void BiomaModalResonator::setPerformanceIntent(const PerformanceIntent& value) noexcept
{
    performance.influence = numeric::unit(value.influence, performance.influence);
    performance.material = numeric::unit(value.material, performance.material);
    performance.decaySeconds = numeric::clampFinite(
        value.decaySeconds, 0.04f, 8.0f, performance.decaySeconds);
    performance.brightness = numeric::unit(value.brightness, performance.brightness);
    performance.hardness = numeric::unit(value.hardness, performance.hardness);
    performance.strikePosition = numeric::clampFinite(
        value.strikePosition, 0.02f, 0.98f, performance.strikePosition);
}

void BiomaModalResonator::updateEffectiveParameters() noexcept
{
    const auto amount = performance.influence;
    currentParameters.fundamentalHz = generativeParameters.fundamentalHz;
    currentParameters.material = interpolate(generativeParameters.material,
                                             performance.material, amount);
    currentParameters.decaySeconds = interpolate(generativeParameters.decaySeconds,
                                                 performance.decaySeconds, amount);
    currentParameters.brightness = interpolate(generativeParameters.brightness,
                                               performance.brightness, amount);
    currentParameters.hardness = interpolate(generativeParameters.hardness,
                                             performance.hardness, amount);
    currentParameters.strikePosition = interpolate(generativeParameters.strikePosition,
                                                   performance.strikePosition, amount);
    updateCoefficients();
}

void BiomaModalResonator::excite(float amount) noexcept
{
    if (!std::isfinite(amount) || amount <= 0.0f)
        return;
    // Performance changes are committed at a physical impact boundary. This
    // avoids moving poles in the middle of a ringing tail while still making
    // every continuous intent value available to the next gesture.
    updateEffectiveParameters();
    burstEnvelope = std::clamp(burstEnvelope + std::min(amount, 1.0f), 0.0f, 1.0f);
    ++excitationCount;
}

float BiomaModalResonator::tick() noexcept
{
    auto excitation = 0.0f;
    if (burstEnvelope > 0.0f)
    {
        const auto white = nextNoise();
        const auto lowpassCoefficient = 0.025f + currentParameters.hardness * 0.42f;
        exciterLowpass += (white - exciterLowpass) * lowpassCoefficient;
        const auto coloured = interpolate(exciterLowpass, white,
                                           currentParameters.hardness * 0.82f);
        excitation = coloured * burstEnvelope * 0.018f;
        burstEnvelope *= burstDecay;
        if (burstEnvelope < 1.0e-6f)
            burstEnvelope = 0.0f;
    }

    auto output = 0.0f;
    auto energySquared = 0.0f;
    for (auto& mode : modes)
    {
        const auto previousReal = mode.real;
        const auto previousImaginary = mode.imaginary;
        mode.real = mode.radius
                * (mode.rotationCos * previousReal - mode.rotationSin * previousImaginary)
            + excitation * mode.inputGain;
        mode.imaginary = mode.radius
            * (mode.rotationSin * previousReal + mode.rotationCos * previousImaginary);
        output += mode.real;
        energySquared += mode.real * mode.real + mode.imaginary * mode.imaginary;
    }

    constexpr float maximumEnergySquared = 1.0f;
    if (energySquared > maximumEnergySquared)
    {
        const auto scale = std::sqrt(maximumEnergySquared / energySquared);
        for (auto& mode : modes)
        {
            mode.real *= scale;
            mode.imaginary *= scale;
        }
        output *= scale;
        energySquared = maximumEnergySquared;
    }

    if (!std::isfinite(output) || !std::isfinite(energySquared))
    {
        silenceState();
        return 0.0f;
    }

    if (burstEnvelope == 0.0f && energySquared < 1.0e-18f)
    {
        silenceState();
        return 0.0f;
    }

    currentModalEnergy = std::sqrt(energySquared);
    // With four modes and a unit Euclidean state norm, Cauchy-Schwarz bounds
    // the raw real sum by two. This gain therefore keeps the public sample
    // strictly below unity without clipping the modal state.
    currentOutput = output * 0.45f;
    return currentOutput;
}

BiomaModalResonator::Telemetry BiomaModalResonator::telemetry() const noexcept
{
    Telemetry result;
    result.parameters = currentParameters;
    result.modalEnergy = currentModalEnergy;
    result.exciterEnergy = burstEnvelope;
    result.lastOutput = currentOutput;
    result.excitationCount = excitationCount;
    result.active = burstEnvelope > 0.0f || currentModalEnergy > 1.0e-7f;
    for (std::size_t index = 0; index < modeCount; ++index)
    {
        result.modeFrequencies[index] = modes[index].frequency;
        result.poleRadii[index] = modes[index].radius;
    }
    return result;
}

void BiomaModalResonator::updateCoefficients() noexcept
{
    const auto hardness = currentParameters.hardness;
    const auto burstSeconds = 0.0007f + (1.0f - hardness) * (1.0f - hardness) * 0.0143f;
    burstDecay = std::exp(std::log(1.0e-4f)
                          / (burstSeconds * static_cast<float>(sampleRate)));

    auto gainSum = 0.0f;
    std::array<float, modeCount> gains {};
    for (std::size_t index = 0; index < modeCount; ++index)
    {
        const auto ratio = interpolate(harmonicRatios[index], membraneRatios[index],
                                       currentParameters.material);
        const auto frequency = std::clamp(currentParameters.fundamentalHz * ratio,
                                          10.0f, static_cast<float>(sampleRate * 0.45));
        const auto highFrequencyDamping = 0.20f
            + (1.0f - currentParameters.brightness) * 0.85f;
        const auto decay = currentParameters.decaySeconds
            / (1.0f + static_cast<float>(index) * highFrequencyDamping);
        const auto radius = std::exp(-1.0f / (decay * static_cast<float>(sampleRate)));
        const auto angle = twoPi * frequency / static_cast<float>(sampleRate);
        auto& mode = modes[index];
        mode.frequency = frequency;
        mode.radius = std::clamp(radius, 0.0f, 0.9999995f);
        mode.rotationCos = std::cos(angle);
        mode.rotationSin = std::sin(angle);

        const auto spectralWeight = interpolate(darkModeWeights[index],
                                                brightModeWeights[index],
                                                currentParameters.brightness);
        const auto strikeShape = 0.18f + 0.82f * std::abs(std::sin(
            pi * static_cast<float>(index + 1) * currentParameters.strikePosition));
        gains[index] = spectralWeight * strikeShape;
        gainSum += gains[index];
    }
    const auto inverseGain = gainSum > 0.0f ? 1.0f / gainSum : 0.0f;
    for (std::size_t index = 0; index < modeCount; ++index)
        modes[index].inputGain = gains[index] * inverseGain;
}

float BiomaModalResonator::nextNoise() noexcept
{
    randomState = randomState * 1664525u + 1013904223u;
    const auto unit = static_cast<float>((randomState >> 8u) & 0x00ffffffu)
        / 16777215.0f;
    return unit * 2.0f - 1.0f;
}

void BiomaModalResonator::silenceState() noexcept
{
    for (auto& mode : modes)
    {
        mode.real = 0.0f;
        mode.imaginary = 0.0f;
    }
    burstEnvelope = 0.0f;
    exciterLowpass = 0.0f;
    currentModalEnergy = 0.0f;
    currentOutput = 0.0f;
}
}
