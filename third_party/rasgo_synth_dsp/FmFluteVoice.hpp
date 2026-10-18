#pragma once
#include <cmath>
#include "Envelope.hpp"

namespace rasgo {

// Real 2-operator FM synthesis (Chowning, 1973 -- the technique behind the
// Yamaha DX7's classic "flute"/"pipe" patches): a sine carrier's phase is
// modulated by a sine modulator, output = sin(phaseC + index*sin(phaseM)).
// A genuinely new synthesis principle for this engine -- distinct from the
// bidirectional cross-FM already between osc1/osc2 (a coupling between two
// full voices, not a dedicated carrier/modulator instrument), and from
// every other principle already here (subtractive Oscillator, breakpoint
// Gendyn, crossfade Morph, +-1 orthogonal Walsh, additive Modal, VOSIM).
//
// What makes this specifically read as "flute" rather than a generic FM
// bell/bass: modulator ratio near 1:1 (flute's own spectrum is close to
// harmonic, not the inharmonic ratios a bell/bass patch would use), and a
// real acoustic detail -- a flute's breath noise/chiff is loudest right at
// the attack and settles as the tone speaks, so the modulation INDEX itself
// is enveloped (high then decaying), not held constant like a static FM
// bell. Amplitude has its own separate, slower envelope -- the two
// envelopes decaying at different rates is what makes the timbre visibly
// brighten-then-mellow during a single note, not just get quieter.
class FmFluteVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        ampEnv_.setSampleRate(sr);
        indexEnv_.setSampleRate(sr);
    }

    // modulatorRatio: near 1.0 for a flute-like near-harmonic spectrum
    // (varied slightly per piece, not hardcoded, so every piece's flute
    // isn't bit-identical in timbre). peakIndex: how bright/breathy the
    // attack chiff gets. sustainIndex: how pure the tone settles to.
    void configure(double modulatorRatio, double peakIndex, double sustainIndex,
                   double ampAttack, double ampRelease) {
        modulatorRatio_ = modulatorRatio;
        peakIndex_ = peakIndex;
        sustainIndex_ = sustainIndex;
        // Index envelope's own decay is deliberately much faster than the
        // amplitude envelope's -- the chiff is a brief onset transient,
        // not something that should still be audible mid-note.
        indexEnv_.setADSR(0.006, 0.10, sustainIndex_ / std::max(peakIndex_, 0.0001), 0.05);
        ampEnv_.setADSR(ampAttack, 0.05, 1.0, ampRelease);
    }

    void setFrequency(double hz) { carrierHz_ = hz; }
    void setGain(float g) { gain_ = g; }

    void noteOn() {
        ampEnv_.noteOn();
        indexEnv_.noteOn();
    }
    void noteOff() {
        ampEnv_.noteOff();
        indexEnv_.noteOff();
    }
    bool isActive() const { return ampEnv_.isActive(); }

    float tick() {
        double modulatorHz = carrierHz_ * modulatorRatio_;
        double carrierInc = carrierHz_ / sampleRate_;
        double modulatorInc = modulatorHz / sampleRate_;

        carrierPhase_ += carrierInc;
        if (carrierPhase_ >= 1.0) carrierPhase_ -= std::floor(carrierPhase_);
        modulatorPhase_ += modulatorInc;
        if (modulatorPhase_ >= 1.0) modulatorPhase_ -= std::floor(modulatorPhase_);

        float indexLevel = indexEnv_.tick();
        double index = peakIndex_ * indexLevel;
        double modulatorSample = std::sin(2.0 * M_PI * modulatorPhase_);
        double carrierSample = std::sin(2.0 * M_PI * carrierPhase_ + index * modulatorSample);

        float amp = ampEnv_.tick();
        return static_cast<float>(carrierSample) * amp * gain_;
    }

private:
    double sampleRate_ = 44100.0;
    double carrierHz_ = 440.0;
    double modulatorRatio_ = 1.0;
    double peakIndex_ = 2.0;
    double sustainIndex_ = 0.3;
    double carrierPhase_ = 0.0;
    double modulatorPhase_ = 0.0;
    Envelope ampEnv_;
    Envelope indexEnv_;
    float gain_ = 0.0f;
};

} // namespace rasgo
