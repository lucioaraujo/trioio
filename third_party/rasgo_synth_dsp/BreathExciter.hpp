#pragma once
#include <random>
#include <cmath>

namespace rasgo {

// A slow-moving noise source shaped like breath pressure, not a raw hiss --
// white noise through a one-pole lowpass (turbulence colour) times a soft
// attack/release envelope (a lung filling and easing, not a switch). Meant
// to drive ModalResonator::excite() every sample while a note is held,
// instead of ModalResonator's usual single mallet-strike impulse -- the
// same "Blow" vs. "Strike" exciter distinction Mutable Instruments'
// Elements offers over its own resonator. This is an independent behavioral
// reinterpretation of that general exciter distinction, not a circuit model.
class BreathExciter {
public:
    void seed(unsigned s) { rng_.seed(s); }
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFilterCutoff(double hz) { cutoffHz_ = hz; }
    void setAttackRelease(double attackSeconds, double releaseSeconds) {
        attackCoeff_ = std::exp(-1.0 / (attackSeconds * sampleRate_));
        releaseCoeff_ = std::exp(-1.0 / (releaseSeconds * sampleRate_));
    }
    void setGate(bool held) { gateHeld_ = held; }
    float envelope() const { return static_cast<float>(envelope_); }

    // Call once per sample; returns the shaped excitation amount, ready to
    // feed straight into ModalResonator::excite().
    float next(float amount) {
        double target = gateHeld_ ? 1.0 : 0.0;
        double coeff = (target > envelope_) ? attackCoeff_ : releaseCoeff_;
        envelope_ = target + (envelope_ - target) * coeff;

        double white = unit_(rng_) * 2.0 - 1.0;
        double rc = 1.0 / (2.0 * 3.141592653589793 * cutoffHz_);
        double alpha = (1.0 / sampleRate_) / (rc + 1.0 / sampleRate_);
        lpState_ += alpha * (white - lpState_);

        return static_cast<float>(lpState_ * envelope_ * amount);
    }

private:
    double sampleRate_ = 44100.0;
    double cutoffHz_ = 900.0;
    double attackCoeff_ = 0.99, releaseCoeff_ = 0.999;
    double envelope_ = 0.0;
    double lpState_ = 0.0;
    bool gateHeld_ = false;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
};

} // namespace rasgo
