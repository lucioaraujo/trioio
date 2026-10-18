#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace antitotem
{
// Antitotem-specific CMOS-inspired voice. PolyBLEP keeps the continuously
// variable saw/pulse components controlled at high frequencies.
class CmosVoice
{
public:
    struct StereoFrame { float left = 0.0f, right = 0.0f; };
    // These are original digital studies of circuit behaviours, not component emulations.
    enum class OscillatorCore : unsigned char { schmittPulse, functionForms, unbufferedDrift };
    enum class FeedbackSignal : unsigned char { direct = 1, rectified = 2, capacitive = 4, pulse = 8, transistor = 16, reflux = 32 };

    void prepare(double value) noexcept { sampleRate = std::max(8000.0, value); }
    void reset() noexcept { phaseA = phaseB = phaseC = phaseD = phaseE = 0.0f; previousOutput = capacitor = 0.0f; proximityFilterState.fill(0.0f); }
    void setFeedbackAmount(float value) noexcept { feedbackAmount = std::clamp(value, 0.0f, 0.72f); }
    void setFeedbackSignal(FeedbackSignal value) noexcept { feedbackConnections = static_cast<unsigned char>(value); }
    void setFeedbackConnections(unsigned char value) noexcept { feedbackConnections = value & 0x3fU; }
    void setOscillatorLevel(std::size_t oscillator, float value) noexcept
    {
        if (oscillator < oscillatorLevels.size()) oscillatorLevels[oscillator] = std::clamp(value, 0.0f, 1.0f);
    }
    void setOscillatorShape(std::size_t oscillator, float value) noexcept
    {
        if (oscillator < oscillatorShapes.size()) oscillatorShapes[oscillator] = std::clamp(value, 0.0f, 3.0f);
    }
    void setOscillatorRatio(std::size_t oscillator, float value) noexcept
    {
        if (oscillator >= oscillatorRatios.size()) return;
        // OSC4 (index 3) is a 4093-relaxation/4020-divider study, not a
        // voice like A/B/C: it needs real sub-clock range (deep division),
        // not just the 0.125x-4x span that keeps the other three audible.
        // OSC5 (index 4) is the 4046 PLL/VCO study - it stays in the same
        // audible 0.125x-4x span as A/B/C since it is meant to be heard,
        // with its own sub-clock (OSC4) rather than an audible ratio being
        // what gives the module its instability.
        oscillatorRatios[oscillator] = oscillator == 3 ? std::clamp(value, 0.03125f, 4.0f)
                                                        : std::clamp(value, 0.125f, 4.0f);
    }
    void setOscillatorPan(std::size_t oscillator, float value) noexcept
    {
        if (oscillator < oscillatorPans.size()) oscillatorPans[oscillator] = std::clamp(value, -1.0f, 1.0f);
    }
    // Y: proximity/materiality (DESIGN.md's "espacialidade como topologia").
    // 0 is dry/close (identical to the sound before this control existed);
    // raising it blends in a duller, filtered version of this one
    // oscillator - the "profundidade de filtro" the design calls for, but
    // per voice, not the single shared VCF further downstream in
    // SimpleSequencer.
    void setOscillatorProximity(std::size_t oscillator, float value) noexcept
    {
        if (oscillator < oscillatorProximity.size()) oscillatorProximity[oscillator] = std::clamp(value, 0.0f, 1.0f);
    }
    // Z: orbit/height. 0 leaves the oscillator exactly where its pan (X)
    // puts it, static, as before. Raising it lets this one voice drift
    // slowly in pitch and circulate around its own base pan position - an
    // autonomous slow LFO per oscillator, not a static offset, matching
    // "diferença de fase, deriva, circulação" rather than a third static
    // coordinate.
    void setOscillatorOrbit(std::size_t oscillator, float value) noexcept
    {
        if (oscillator < oscillatorOrbit.size()) oscillatorOrbit[oscillator] = std::clamp(value, 0.0f, 1.0f);
    }
    void setEnergy(float value) noexcept { energy = std::clamp(value, 0.0f, 1.0f); }
    void setOscillatorCore(OscillatorCore value) noexcept { core = value; }

    [[nodiscard]] StereoFrame tickStereo(float voltage, float externalFeedback = 0.0f) noexcept
    {
        const auto cv = std::clamp(voltage, 0.0f, 1.0f);
        const auto supply = 0.46f + energy * 0.78f;
        const auto railWander = (1.0f - energy) * 0.035f * std::sin(phaseC * twoPi);
        const auto fundamental = 52.0f * std::pow(2.0f, cv * 4.3f) * supply;
        // Z (orbit): a slow, per-oscillator autonomous LFO, silent at 0
        // (oscillatorOrbit's default). A different rate per oscillator
        // index (not a shared LFO) so raising orbit on more than one voice
        // still reads as independent drift, not synchronized wobble. Used
        // just below to nudge each oscillator's own pitch, and again
        // further down to circulate its effective pan.
        std::array<float, 5> orbitLfo {};
        for (std::size_t i = 0; i < orbitLfo.size(); ++i)
        {
            orbitLfo[i] = std::sin(orbitPhase[i]);
            const auto orbitRate = (0.05f + 0.03f * static_cast<float>(i)) / static_cast<float>(sampleRate);
            orbitPhase[i] += twoPi * orbitRate;
            if (orbitPhase[i] > twoPi) orbitPhase[i] -= twoPi;
        }
        const auto rateA = fundamental * oscillatorRatios[0] * (1.0f + oscillatorOrbit[0] * 0.006f * orbitLfo[0]) / static_cast<float>(sampleRate);
        const auto rateB = fundamental * oscillatorRatios[1] * (1.0f + railWander) * (1.0f + oscillatorOrbit[1] * 0.006f * orbitLfo[1]) / static_cast<float>(sampleRate);
        const auto rateC = fundamental * oscillatorRatios[2] * (1.0f - railWander) * (1.0f + oscillatorOrbit[2] * 0.006f * orbitLfo[2]) / static_cast<float>(sampleRate);
        // OSC4: same fundamental, but oscillatorRatios[3]'s much wider range
        // (see setOscillatorRatio) lets it land anywhere from a slow
        // divided-clock pulse to an audible buzz, matching a 4093 relaxation
        // oscillator run through a 4020 divider rather than a fourth voice
        // tuned like A/B/C.
        const auto rateD = fundamental * oscillatorRatios[3] * (1.0f + oscillatorOrbit[3] * 0.006f * orbitLfo[3]) / static_cast<float>(sampleRate);
        // OSC5 study: a 4046 PLL's VCO is not free-running like A/B/C - a
        // phase detector compares it against a reference (OSC A here) and
        // bends its rate to chase that reference, producing the "hunting"
        // instability real PLLs show when not fully locked. phaseError is
        // that detector output; pllLockGain is the loop-filter gain.
        const auto rateE = fundamental * oscillatorRatios[4] * (1.0f + oscillatorOrbit[4] * 0.006f * orbitLfo[4]) / static_cast<float>(sampleRate);
        auto phaseError = phaseE - phaseA;
        phaseError -= std::round(phaseError);
        const auto pllCorrection = std::clamp(phaseError * pllLockGain, -0.9f, 0.9f);
        const auto lockedRateE = rateE * (1.0f - pllCorrection);
        const auto feedback = feedbackSample() + std::clamp(externalFeedback, -1.0f, 1.0f);
        const auto modulator = std::sin(phaseB * twoPi);
        const auto oscillatorA = waveform(phaseA + modulator * (0.04f + cv * 0.16f) + feedback * feedbackAmount * 0.11f,
                                          rateA, oscillatorShapes[0]);
        const auto oscillatorB = waveform(phaseB, rateB, oscillatorShapes[1]);
        const auto oscillatorC = waveform(phaseC, rateC, oscillatorShapes[2]);
        // A Schmitt-NAND relaxation oscillator's threshold hysteresis is not
        // perfectly symmetric; the divider stage after it just passes that
        // duty cycle through cleanly, so this models the asymmetry rather
        // than the RC charge curve a full circuit simulation would need.
        // FORMA drives OSC4's duty-cycle asymmetry directly (0..3 maps to
        // roughly -0.42..+0.42) instead of being a dead control that
        // schmittRelaxation ignores - a relaxation/divider oscillator's
        // natural "shape" parameter is its pulse width, not a sine/saw/
        // square blend like A/B/C.
        const auto dutyAsymmetry = (oscillatorShapes[3] / 3.0f - 0.5f) * 0.84f + (energy - 0.5f) * 0.16f;
        const auto oscillatorD = schmittRelaxation(phaseD, rateD, dutyAsymmetry);
        // OSC5 study: the PLL's own VCO waveform (vcoE) is fed into an
        // LM13600-style OTA used as a four-quadrant multiplier against OSC A
        // rather than mixed with it - true ring modulation, giving the
        // sum/difference "heterodyne" beating tones the module is named for.
        const auto vcoE = waveform(phaseE, lockedRateE, oscillatorShapes[4]);
        // Multiplying two [-1,1] signals is always quieter than either one
        // alone (that is what a ring-mod product is), so OSC5 reads as much
        // weaker than A/B/C at the same MIX position unless compensated -
        // an output-stage trim after the OTA, same as a real ring-mod
        // circuit would need to match unity-ish gain against the other buses.
        const auto oscillatorE = std::clamp(vcoE * oscillatorA * 2.1f, -1.0f, 1.0f);
        const std::array<float, 5> raw { oscillatorA, oscillatorB, oscillatorC, oscillatorD, oscillatorE };
        float colouredLeft = feedback * feedbackAmount * 0.22f;
        float colouredRight = colouredLeft;
        for (std::size_t i = 0; i < raw.size(); ++i)
        {
            // Y (proximity): a gentle one-pole lowpass per oscillator,
            // blended in as proximity rises. At 0 (default) this is a
            // no-op - processed[i] reduces exactly to raw[i].
            proximityFilterState[i] += (raw[i] - proximityFilterState[i]) * 0.06f;
            const auto processed = raw[i] * (1.0f - oscillatorProximity[i]) + proximityFilterState[i] * oscillatorProximity[i];
            // Z (orbit): circulates the effective pan around the oscillator's
            // own base position instead of sitting still. At 0 (default)
            // this is also a no-op - pan reduces exactly to oscillatorPans[i].
            const auto pan = std::clamp(oscillatorPans[i] + oscillatorOrbit[i] * 0.5f * orbitLfo[i], -1.0f, 1.0f);
            colouredLeft += processed * oscillatorLevels[i] * std::sqrt(0.5f * (1.0f - pan));
            colouredRight += processed * oscillatorLevels[i] * std::sqrt(0.5f * (1.0f + pan));
        }

        const auto shapeCore = [this, oscillatorA, oscillatorB, oscillatorC, feedback] (float coloured)
        {
            // Three deliberately distinct studies: switching threshold, continuous
            // function forms, and asymmetric unbuffered-inverter-like drift. Each
            // one reads raw oscillator waveforms (not the level-scaled sum in
            // `coloured`) for its own character, so each such read is itself
            // scaled by that oscillator's own MIX level - otherwise MIX=0 could
            // not silence it, as found live, 26 ago. 2026 (40106/4069UB kept
            // sounding with every oscillator MIX at 0).
            if (core == OscillatorCore::schmittPulse)
            {
                const auto threshold = oscillatorA + oscillatorB * 0.34f + feedback * 0.12f;
                const auto abPresence = std::max(oscillatorLevels[0], oscillatorLevels[1]);
                return coloured * 0.72f + (threshold >= 0.0f ? 0.36f : -0.36f) * abPresence;
            }
            if (core == OscillatorCore::functionForms)
                return std::sin(coloured * 1.57079632679f);
            const auto asymmetry = 0.15f + (1.0f - energy) * 0.21f;
            return std::tanh((coloured + oscillatorC * asymmetry * oscillatorLevels[2] + feedback * 0.18f) * 1.48f);
        };

        colouredLeft = shapeCore(colouredLeft);
        colouredRight = shapeCore(colouredRight);

        advance(phaseA, rateA);
        advance(phaseB, rateB);
        advance(phaseC, rateC);
        advance(phaseD, rateD);
        advance(phaseE, lockedRateE);
        const auto scale = (0.38f + energy * 0.78f + cv * 0.32f);
        const auto outputGain = 0.42f + energy * 0.58f;
        StereoFrame output { std::tanh(colouredLeft * scale) * outputGain,
                             std::tanh(colouredRight * scale) * outputGain };
        const auto mono = (output.left + output.right) * 0.5f;
        capacitor += (mono - capacitor) * (0.0015f + cv * 0.006f);
        previousOutput = mono;
        return output;
    }

    [[nodiscard]] float tick(float voltage, float externalFeedback = 0.0f) noexcept
    {
        const auto stereo = tickStereo(voltage, externalFeedback);
        return (stereo.left + stereo.right) * 0.5f;
    }

private:
    static void advance(float& phase, float increment) noexcept
    {
        phase += increment;
        phase -= std::floor(phase);
    }
    [[nodiscard]] static float polyBlep(float time, float increment) noexcept
    {
        if (increment <= 0.0f) return 0.0f;
        if (time < increment)
        {
            const auto x = time / increment;
            return x + x - x * x - 1.0f;
        }
        if (time > 1.0f - increment)
        {
            const auto x = (time - 1.0f) / increment;
            return x * x + x + x + 1.0f;
        }
        return 0.0f;
    }
    [[nodiscard]] static float waveform(float phase, float increment, float shape) noexcept
    {
        phase -= std::floor(phase);
        const auto sine = std::sin(phase * twoPi);
        const auto triangle = 1.0f - 4.0f * std::abs(phase - 0.5f);
        const auto saw = 2.0f * phase - 1.0f - polyBlep(phase, increment);
        const auto width = 0.5f;
        auto square = phase < width ? 1.0f : -1.0f;
        square += polyBlep(phase, increment);
        auto edge = phase + 1.0f - width;
        edge -= std::floor(edge);
        square -= polyBlep(edge, increment);
        const auto position = std::clamp(shape, 0.0f, 3.0f);
        const auto segment = static_cast<int>(position);
        const auto blend = position - static_cast<float>(segment);
        const auto next = segment == 0 ? triangle : (segment == 1 ? saw : square);
        const auto current = segment == 0 ? sine : (segment == 1 ? triangle : saw);
        return current + (next - current) * blend;
    }
    // OSC4 study: a 4093 Schmitt-NAND relaxation oscillator's charge/discharge
    // thresholds are not perfectly symmetric, and a 4020 divider downstream
    // only ever passes through the clean digital transitions - so this is a
    // duty-cycle-shifted square wave (asymmetry driven by ENERGIA, matching
    // the rest of the engine's convention of energy shaping character), not
    // an RC relaxation simulation.
    [[nodiscard]] static float schmittRelaxation(float phase, float increment, float asymmetry) noexcept
    {
        phase -= std::floor(phase);
        const auto width = std::clamp(0.5f + asymmetry, 0.08f, 0.92f);
        auto square = phase < width ? 1.0f : -1.0f;
        square += polyBlep(phase, increment);
        auto edge = phase + 1.0f - width;
        edge -= std::floor(edge);
        square -= polyBlep(edge, increment);
        return square;
    }
    [[nodiscard]] float feedbackSample() const noexcept
    {
        float sum = 0.0f;
        int count = 0;
        const auto add = [&] (unsigned char flag, float signal) {
            if ((feedbackConnections & flag) != 0) { sum += signal; ++count; }
        };
        add(static_cast<unsigned char>(FeedbackSignal::direct), previousOutput);
        add(static_cast<unsigned char>(FeedbackSignal::rectified), std::abs(previousOutput) * 2.0f - 1.0f);
        add(static_cast<unsigned char>(FeedbackSignal::capacitive), capacitor);
        add(static_cast<unsigned char>(FeedbackSignal::pulse), previousOutput >= 0.0f ? 1.0f : -1.0f);
        // Original digital studies of material routes: soft asymmetric gain and
        // an AC-like return against the stored capacitor state.
        add(static_cast<unsigned char>(FeedbackSignal::transistor), std::tanh(previousOutput * 2.2f + capacitor * 0.42f));
        add(static_cast<unsigned char>(FeedbackSignal::reflux), previousOutput - capacitor * 0.82f);
        return count > 0 ? sum / static_cast<float>(count) : 0.0f;
    }

    static constexpr float twoPi = 6.28318530717958647692f;
    // Loop-filter gain for the OSC5 (4046 PLL) phase-detector correction -
    // low enough that mismatched ratios "hunt" instead of snapping to a
    // hard 1:1 lock, which would fight the FREQ ratio control.
    static constexpr float pllLockGain = 0.35f;
    double sampleRate = 48000.0;
    float phaseA = 0.0f, phaseB = 0.0f, phaseC = 0.0f, phaseD = 0.0f, phaseE = 0.0f;
    float previousOutput = 0.0f, capacitor = 0.0f, feedbackAmount = 0.26f;
    // Index 3 is OSC4 (4093/4020 study), index 4 is OSC5 (4046/LM13600
    // study). Both default to silent so existing patches/tests that only
    // know about fewer oscillators are unaffected until raised explicitly.
    std::array<float, 5> oscillatorLevels { 0.62f, 0.38f, 0.28f, 0.0f, 0.0f };
    std::array<float, 5> oscillatorShapes { 0.0f, 2.0f, 3.0f, 0.0f, 1.0f };
    std::array<float, 5> oscillatorRatios { 1.0f, 0.73f, 1.51f, 0.25f, 0.87f };
    std::array<float, 5> oscillatorPans { -0.58f, 0.0f, 0.58f, 0.0f, -0.24f };
    // Y and Z (see setOscillatorProximity/setOscillatorOrbit above) both
    // default to 0 - every existing patch/test that never touches them
    // hears exactly the same output as before these controls existed.
    std::array<float, 5> oscillatorProximity { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> oscillatorOrbit { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 5> proximityFilterState { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    // Staggered start phases (i * 2pi/5), not all zero: with orbit raised on
    // more than one oscillator, a shared start phase would circulate them
    // in lockstep instead of independently.
    std::array<float, 5> orbitPhase { 0.0f, 1.25663706f, 2.51327412f, 3.76991118f, 5.02654824f };
    float energy = 0.72f;
    OscillatorCore core = OscillatorCore::functionForms;
    unsigned char feedbackConnections = static_cast<unsigned char>(FeedbackSignal::capacitive);
};
} // namespace antitotem
