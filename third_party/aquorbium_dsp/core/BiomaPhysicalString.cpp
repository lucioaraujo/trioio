#include "core/BiomaPhysicalString.h"

#include "core/Numeric.h"

#include <algorithm>
#include <cmath>

namespace aquorbium
{
namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr std::size_t delayMask = BiomaPhysicalString::delayCapacity - 1;

float interpolate(float left, float right, float amount) noexcept
{
    return left + (right - left) * amount;
}
}

void BiomaPhysicalString::prepare(double value) noexcept
{
    sampleRate = numeric::sampleRate(value);
    dcCoefficient = static_cast<float>(
        std::exp(-2.0 * static_cast<double>(pi) * 10.0 / sampleRate));
    updateEffectiveParameters();
    reset();
}

void BiomaPhysicalString::setSeed(std::uint32_t value) noexcept
{
    seed = value == 0 ? 1u : value;
    reset();
}

void BiomaPhysicalString::reset() noexcept
{
    delayLine.fill(0.0f);
    excitationHistory.fill(0.0f);
    writeIndex = 0;
    excitationWriteIndex = 0;
    randomState = seed;
    fractionalInputPrevious = 0.0f;
    fractionalOutputPrevious = 0.0f;
    lossInputPrevious = 0.0f;
    excitationSamplesRemaining = 0;
    excitationSamplesTotal = 0;
    excitationSampleIndex = 0;
    excitationEnvelope = 0.0f;
    exciterLowpass = 0.0f;
    exciterLowpassSecondary = 0.0f;
    currentStringEnergy = 0.0f;
    currentExcitationEnergy = 0.0f;
    dcInputPrevious = 0.0f;
    dcOutputPrevious = 0.0f;
    currentOutput = 0.0f;
    quietSamples = 0;
    excitationCount = 0;
    dormant = true;
}

void BiomaPhysicalString::setParameters(const Parameters& value) noexcept
{
    const auto minimumFrequency = std::max(
        20.0f, static_cast<float>(sampleRate / static_cast<double>(delayCapacity - 4)));
    const auto maximumFrequency = std::min(
        6000.0f, static_cast<float>(sampleRate * 0.22));
    generativeParameters.fundamentalHz = numeric::clampFinite(
        value.fundamentalHz, minimumFrequency, maximumFrequency,
        generativeParameters.fundamentalHz);
    generativeParameters.decaySeconds = numeric::clampFinite(
        value.decaySeconds, 0.04f, 12.0f, generativeParameters.decaySeconds);
    generativeParameters.brightness = numeric::unit(
        value.brightness, generativeParameters.brightness);
    generativeParameters.hardness = numeric::unit(
        value.hardness, generativeParameters.hardness);
    generativeParameters.pluckPosition = numeric::clampFinite(
        value.pluckPosition, 0.04f, 0.5f, generativeParameters.pluckPosition);

    if (dormant)
        updateEffectiveParameters();
}

void BiomaPhysicalString::setPerformanceIntent(
    const PerformanceIntent& value) noexcept
{
    performance.influence = numeric::unit(value.influence, performance.influence);
    performance.decaySeconds = numeric::clampFinite(
        value.decaySeconds, 0.04f, 12.0f, performance.decaySeconds);
    performance.brightness = numeric::unit(value.brightness, performance.brightness);
    performance.hardness = numeric::unit(value.hardness, performance.hardness);
    performance.pluckPosition = numeric::clampFinite(
        value.pluckPosition, 0.04f, 0.5f, performance.pluckPosition);
}

void BiomaPhysicalString::excite(float amount) noexcept
{
    if (!std::isfinite(amount) || amount <= 0.0f)
        return;

    // Material changes are committed at the event boundary. Energy already
    // circulating in the guide is never silently retuned by a GUI snapshot.
    updateEffectiveParameters();
    excitationEnvelope = std::clamp(
        excitationEnvelope + std::min(amount, 1.0f), 0.0f, 1.0f);
    excitationSamplesTotal = static_cast<std::uint32_t>(std::max(
        2.0f, std::round(static_cast<float>(integerDelay) + fractionalDelay)));
    excitationSamplesRemaining = excitationSamplesTotal;
    excitationSampleIndex = 0;
    quietSamples = 0;
    dormant = false;
    ++excitationCount;
}

float BiomaPhysicalString::tick() noexcept
{
    if (dormant && excitationSamplesRemaining == 0)
        return 0.0f;

    auto excitation = 0.0f;
    if (excitationSamplesRemaining > 0 && excitationEnvelope > 0.0f)
    {
        const auto white = nextNoise();
        exciterLowpass += (white - exciterLowpass) * exciterCoefficient;
        exciterLowpassSecondary += (exciterLowpass - exciterLowpassSecondary)
            * exciterCoefficient;
        const auto coloured = interpolate(
            exciterLowpassSecondary, exciterLowpass,
            currentParameters.hardness * 0.75f);

        const auto delayedIndex = (excitationWriteIndex + delayCapacity - pluckDelay)
            & delayMask;
        const auto delayedExcitation = excitationHistory[delayedIndex];
        excitationHistory[excitationWriteIndex] = coloured;
        excitationWriteIndex = (excitationWriteIndex + 1) & delayMask;

        const auto positionShape = coloured - delayedExcitation * 0.92f;
        const auto progress = (static_cast<float>(excitationSampleIndex) + 0.5f)
            / static_cast<float>(excitationSamplesTotal);
        const auto windowSine = std::sin(pi * progress);
        const auto window = windowSine * windowSine;
        excitation = positionShape * excitationEnvelope * window * 0.075f;
        ++excitationSampleIndex;
        --excitationSamplesRemaining;
        if (excitationSamplesRemaining == 0)
        {
            excitationSamplesRemaining = 0;
            excitationSamplesTotal = 0;
            excitationSampleIndex = 0;
            excitationEnvelope = 0.0f;
        }
    }

    const auto readIndex = (writeIndex + delayCapacity - integerDelay) & delayMask;
    const auto delayed = delayLine[readIndex];
    auto fractionallyDelayed = delayed;
    if (!fractionalBypass)
    {
        fractionallyDelayed = fractionalCoefficient * delayed
            + fractionalInputPrevious
            - fractionalCoefficient * fractionalOutputPrevious;
        fractionalInputPrevious = delayed;
        fractionalOutputPrevious = fractionallyDelayed;
    }

    const auto lossy = (1.0f - lossBlend) * fractionallyDelayed
        + lossBlend * lossInputPrevious;
    lossInputPrevious = fractionallyDelayed;
    auto nextState = energyBarrier(lossy * loopGain + excitation);
    if (std::abs(nextState) < 1.0e-15f)
        nextState = 0.0f;
    delayLine[writeIndex] = nextState;
    writeIndex = (writeIndex + 1) & delayMask;

    currentStringEnergy += (std::abs(nextState) - currentStringEnergy) * 0.0015f;
    if (currentStringEnergy < 1.0e-12f && excitationSamplesRemaining == 0)
        currentStringEnergy = 0.0f;
    currentExcitationEnergy = std::abs(excitation);

    // A pickup primarily hears the circulating guide. A small contact path
    // preserves articulation without exposing the stochastic exciter itself.
    const auto rawOutput = fractionallyDelayed + excitation * 0.04f;
    auto dcBlocked = rawOutput - dcInputPrevious + dcCoefficient * dcOutputPrevious;
    dcInputPrevious = rawOutput;
    dcOutputPrevious = dcBlocked;
    if (!std::isfinite(dcBlocked) || !std::isfinite(currentStringEnergy))
    {
        reset();
        return 0.0f;
    }
    if (std::abs(dcBlocked) < 1.0e-15f)
        dcBlocked = 0.0f;
    currentOutput = dcBlocked * 0.38f;

    if (excitationSamplesRemaining == 0
        && std::abs(nextState) < 1.0e-12f
        && std::abs(fractionallyDelayed) < 1.0e-12f)
    {
        ++quietSamples;
        if (quietSamples > integerDelay + 2u)
        {
            fractionalInputPrevious = 0.0f;
            fractionalOutputPrevious = 0.0f;
            lossInputPrevious = 0.0f;
            dcInputPrevious = 0.0f;
            dcOutputPrevious = 0.0f;
            currentStringEnergy = 0.0f;
            currentExcitationEnergy = 0.0f;
            currentOutput = 0.0f;
            dormant = true;
            return 0.0f;
        }
    }
    else
    {
        quietSamples = 0;
    }
    return currentOutput;
}

BiomaPhysicalString::Telemetry BiomaPhysicalString::telemetry() const noexcept
{
    return {
        currentParameters,
        static_cast<float>(integerDelay) + fractionalDelay + lossBlend,
        fractionalDelay,
        loopGain,
        lossBlend,
        currentStringEnergy,
        currentExcitationEnergy,
        currentOutput,
        integerDelay,
        pluckDelay,
        excitationCount,
        !dormant
    };
}

void BiomaPhysicalString::updateEffectiveParameters() noexcept
{
    const auto amount = performance.influence;
    currentParameters.fundamentalHz = generativeParameters.fundamentalHz;
    currentParameters.decaySeconds = interpolate(
        generativeParameters.decaySeconds, performance.decaySeconds, amount);
    currentParameters.brightness = interpolate(
        generativeParameters.brightness, performance.brightness, amount);
    currentParameters.hardness = interpolate(
        generativeParameters.hardness, performance.hardness, amount);
    currentParameters.pluckPosition = interpolate(
        generativeParameters.pluckPosition, performance.pluckPosition, amount);
    configureLoop();
}

void BiomaPhysicalString::configureLoop() noexcept
{
    const auto rate = static_cast<float>(sampleRate);
    const auto desiredDelay = rate / currentParameters.fundamentalHz;
    const auto darkness = 1.0f - currentParameters.brightness;
    lossBlend = 0.025f + darkness * darkness * 0.475f;

    const auto compensatedDelay = std::clamp(
        desiredDelay - lossBlend, 2.0f,
        static_cast<float>(delayCapacity - 2));
    integerDelay = static_cast<std::uint32_t>(std::floor(compensatedDelay));
    fractionalDelay = compensatedDelay - static_cast<float>(integerDelay);
    fractionalBypass = fractionalDelay < 1.0e-4f;
    fractionalCoefficient = fractionalBypass
        ? 0.0f : (1.0f - fractionalDelay) / (1.0f + fractionalDelay);

    const auto decayCycles = currentParameters.decaySeconds
        * currentParameters.fundamentalHz;
    loopGain = std::clamp(
        std::exp(std::log(0.001f) / std::max(0.001f, decayCycles)),
        0.0f, 0.999999f);
    exciterCoefficient = 0.02f
        + currentParameters.hardness * currentParameters.hardness * 0.65f;
    pluckDelay = static_cast<std::uint32_t>(std::clamp(
        std::round(desiredDelay * currentParameters.pluckPosition),
        1.0f, static_cast<float>(delayCapacity - 1)));
}

float BiomaPhysicalString::nextNoise() noexcept
{
    randomState = randomState * 1664525u + 1013904223u;
    const auto unit = static_cast<float>((randomState >> 8u) & 0x00ffffffu)
        / 16777215.0f;
    return unit * 2.0f - 1.0f;
}

float BiomaPhysicalString::energyBarrier(float value) noexcept
{
    constexpr float linearLimit = 0.88f;
    const auto magnitude = std::abs(value);
    if (magnitude <= linearLimit)
        return value;
    const auto excess = (magnitude - linearLimit) / (1.0f - linearLimit);
    const auto bounded = linearLimit + (1.0f - linearLimit) * std::tanh(excess);
    return std::copysign(bounded, value);
}
}
