#pragma once
#include "Oscillator.hpp"
#include "LfoBank.hpp"
#include <cmath>

namespace rasgo {

// A real theremin never re-triggers a note -- there is no key, no gate, no
// attack transient, only a hand's continuous distance from an antenna
// mapped to continuous pitch, with the tone itself close to a pure sine
// (the audible output of heterodyning two RF oscillators a few hundred Hz
// apart is, by construction, very close to a sine at their difference
// frequency). The one thing that makes an instrument read as "theremin"
// rather than "portamento synth" is that the glide is ALWAYS on, toward
// whatever pitch its current musical relation proposes, never snapping --
// every other voice in this engine (osc1/osc2, tuba, Harmonaig...) re-tunes
// instantly to a new note; this one never does. A player's natural hand
// tremor is modeled as a small, real vibrato (5-7Hz, a fraction of a
// semitone) via a dedicated Lfo instance (LfoBank.hpp) -- not a new
// oscillator class, the same shape this engine already uses for its four
// existing modulation channels.
class ThereminVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        osc_.setSampleRate(sr);
        osc_.setWaveform(Oscillator::Waveform::Sine);
        vibrato_.setSampleRate(sr);
        vibrato_.setShape(Lfo::Shape::Sine);
    }

    // glideSeconds: real portamento time constant (how long it takes to
    // close ~63% of the distance to a new target pitch) -- a real theremin
    // player's hand can move at very different speeds, so this varies
    // piece to piece rather than being one fixed glide time.
    void configure(double glideSeconds, double vibratoHz, float vibratoDepthSemitones, unsigned vibratoSeed) {
        glideCoeff_ = 1.0 - std::exp(-1.0 / (glideSeconds * sampleRate_));
        vibrato_.setFrequency(vibratoHz);
        vibrato_.setDepth(vibratoDepthSemitones);
        vibrato_.seed(vibratoSeed);
    }

    // targetHz: wherever the current interpreter/relationship says the pitch
    // should be -- this voice never jumps there, it only ever glides. Before
    // the relational orchestra this target came directly from the melody;
    // the DSP body itself never needed to know who made the proposal.
    void setTargetFrequency(double targetHz) { targetHz_ = targetHz; }

    void setGain(float g) { gain_ = g; }

    float tick() {
        currentHz_ += glideCoeff_ * (targetHz_ - currentHz_);
        double vibratoSemitones = vibrato_.tick();
        double soundingHz = currentHz_ * std::pow(2.0, vibratoSemitones / 12.0);
        osc_.setFrequency(soundingHz);
        return osc_.tick() * gain_;
    }

private:
    double sampleRate_ = 44100.0;
    Oscillator osc_;
    Lfo vibrato_;
    double glideCoeff_ = 0.001;
    double targetHz_ = 220.0;
    double currentHz_ = 220.0;
    float gain_ = 0.0f;
};

} // namespace rasgo
