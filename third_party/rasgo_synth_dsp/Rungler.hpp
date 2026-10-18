#pragma once
#include <cstdint>
#include <cmath>

namespace rasgo {

// Rob Hordijk's Rungler, the feedback core of his Benjolin circuit -- two
// simple oscillators cross-feed an 8-bit shift register: one oscillator's
// edge clocks the register, the other's polarity is the bit shifted in,
// and the register's own contents read back out as a stepped voltage that
// modulates both oscillators' pitch in turn. At low feedback the loop
// settles into a repeating pattern; pushed past a certain depth it stops
// behaving like a sequencer and starts lurching unpredictably between
// rhythmic lock-in and pure noise -- the exact "fratura matematica"
// character the Nonlinearcircuits/Benjolin-clone DIY scene built a whole
// tradition chasing. Implemented here as an original behavioural
// reinterpretation (two oscillators, a shift register, feedback), not as an
// analogue-circuit model -- there is no attempt to reproduce the original
// transistor-level instability.
class Rungler {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRates(double hzA, double hzB) { rateA_ = hzA; rateB_ = hzB; }
    // 0 = the two oscillators never hear each other, output settles into a
    // fixed repeating byte pattern. 1 = full lurch, pitch of both
    // oscillators swings +-50% with the register's own last output.
    void setFeedbackDepth(double depth) { feedbackDepth_ = depth; }
    void reset(uint8_t seedByte) { register_ = seedByte; phaseA_ = 0.0; phaseB_ = 0.0; }

    // Advance one sample; returns the current stepped Rungler voltage,
    // -1..1, updated only on oscillator A's clock edge and held between
    // edges, same sample-and-hold character the real circuit has.
    double next() {
        double freqA = rateA_ * (1.0 + feedbackDepth_ * runglerOut_ * 0.5);
        double freqB = rateB_ * (1.0 - feedbackDepth_ * runglerOut_ * 0.5);
        phaseA_ += freqA / sampleRate_;
        phaseB_ += freqB / sampleRate_;
        if (phaseA_ >= 1.0) {
            phaseA_ -= std::floor(phaseA_);
            bool bitB = phaseB_ < 0.5;
            register_ = static_cast<uint8_t>((register_ << 1) | (bitB ? 1 : 0));
            runglerOut_ = (static_cast<double>(register_) / 255.0) * 2.0 - 1.0;
        }
        if (phaseB_ >= 1.0) phaseB_ -= std::floor(phaseB_);
        return runglerOut_;
    }

private:
    double sampleRate_ = 44100.0;
    double rateA_ = 5.0, rateB_ = 7.0;
    double feedbackDepth_ = 0.0;
    double phaseA_ = 0.0, phaseB_ = 0.0;
    uint8_t register_ = 0b10110010;
    double runglerOut_ = 0.0;
};

} // namespace rasgo
