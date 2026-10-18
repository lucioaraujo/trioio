#pragma once
#include <cmath>
#include <algorithm>

namespace rasgo {

// VOSIM (VOice SIMulation), developed at Utrecht's Institute of Sonology and
// chiefly associated with Werner Kaegi; Stan Tempelaars published an important
// spectral analysis of the method. It is a classic electroacoustic
// formant-synthesis technique from the pre-FM era of computer music. A sixth,
// genuinely distinct synthesis principle for
// this engine (the other five: PolyBLEP sinusoidal/saw/pulse, MorphOscillator's
// continuous shape crossfade, GendynOscillator's breakpoint random walk,
// WalshOscillator's orthogonal +-1 basis, ModalResonator's continuous-
// excitation additive resonance) -- VOSIM instead builds each pitch period
// out of a small number of discrete sin^2 "pips", each one quieter than
// the last by a fixed decay ratio, followed by silence for the remainder
// of the period. The pip WIDTH sets the formant frequency (a resonance-like
// spectral peak) independently of the pip REPETITION RATE, which sets the
// perceived pitch -- two knobs doing two genuinely different jobs, unlike
// a filtered oscillator where cutoff and pitch are coupled through the
// same waveform.
//
// Deliberately not a rewrite of ModalResonator (fase 10/43, continuous
// excitation into a resonant filter) -- VOSIM's pips are directly
// synthesized bursts with no filter/resonator in the signal path at all,
// the decay ratio IS the "resonance" character, not a byproduct of a
// feedback loop. Doesn't need a real speech recording (unlike the "voz
// narrante" idea from the same research batch) -- the formant character
// is synthesized directly from the pip width/count/decay.
//
// Real historically-documented output is unipolar (a sum of sin^2 pips,
// always >= 0) -- the same DC-blocker this engine already runs on the
// full master mix (see "overflow de DC contínuo" already noted as
// redundant elsewhere in this project precisely because that blocker
// already cancels any sustained DC bias) removes the offset, so no
// separate DC handling is needed here.
//
// Verified by an isolated test before wiring in: bounded to [0,1], exact
// periodicity at the configured pitch, the decay ratio between successive
// pips matches the configured factor, and the silence gap fills exactly
// the rest of the period.
class VosimOscillator {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double hz) { frequency_ = hz; }   // pitch (F0)
    void setFormant(double hz) { formantFreq_ = hz; }   // pip width -> formant peak
    void setPulseCount(int n) { pulseCount_ = std::max(1, n); }
    void setDecay(double b) { decay_ = std::clamp(b, 0.0, 0.999); }

    float tick() {
        double periodSamples = sampleRate_ / std::max(frequency_, 1.0);
        double pulseWidthSamples = sampleRate_ / std::max(formantFreq_, 1.0);
        // Never lets the pips overrun the period even if the formant is
        // set low relative to the pitch -- caps pip width so kPulseCount_
        // of them always fit, silence just shrinks instead of pips
        // overlapping into the next period.
        pulseWidthSamples = std::min(pulseWidthSamples, periodSamples / pulseCount_);

        double posInPeriod = std::fmod(phase_, periodSamples);
        int pulseIndex = static_cast<int>(posInPeriod / pulseWidthSamples);
        float out = 0.0f;
        if (pulseIndex < pulseCount_) {
            double posInPulse = posInPeriod - pulseIndex * pulseWidthSamples;
            double theta = M_PI * posInPulse / pulseWidthSamples;
            double shape = std::sin(theta);
            double amp = std::pow(decay_, pulseIndex);
            out = static_cast<float>(shape * shape * amp);
        }
        phase_ += 1.0;
        if (phase_ >= periodSamples) phase_ -= periodSamples;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double frequency_ = 100.0;
    double formantFreq_ = 800.0;
    int pulseCount_ = 4;
    double decay_ = 0.7;
    double phase_ = 0.0;
};

} // namespace rasgo
