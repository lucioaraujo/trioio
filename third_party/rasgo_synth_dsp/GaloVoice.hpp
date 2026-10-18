#pragma once
#include "ModalResonator.hpp"
#include "BreathExciter.hpp"
#include <array>
#include <cmath>
#include <algorithm>

namespace rasgo {

// "Galo" (rooster call): a stylized synthesized gesture inspired by the
// real crow's documented shape -- several short rising syllables followed
// by one longer sustained note with a slight falling tail -- built from
// two pieces this engine already has and already validated (ModalResonator,
// fase 43/78; BreathExciter, the tuba voice's own continuous "blow"
// exciter). Honestly labeled as synthesis: this is a frequency-contour
// stylization of a real, documented bioacoustic pattern, not a recording,
// a sample, or a claim of biological accuracy.
//
// galoCallContour is a pure function (same testability idiom as
// AtropeloEnsemble::contourFrequency) so the shape itself -- rising
// syllables, falling tail -- is checkable in isolation before any audio
// ever runs through it.
inline double galoCallContour(double elapsedSeconds, double totalDuration) {
    static const double starts[4]  = {0.00, 0.16, 0.32, 0.52};
    static const double ratios[4]  = {1.00, 1.18, 1.38, 1.65};
    double frac = std::clamp(elapsedSeconds / totalDuration, 0.0, 1.0);
    int idx = 0;
    for (int i = 0; i < 4; ++i) {
        if (frac >= starts[i]) idx = i;
    }
    double ratio = ratios[static_cast<size_t>(idx)];
    if (idx == 3) {
        // Real crow's characteristic falling tail on the final, longest
        // syllable -- a genuine downward glide, not a flat sustain.
        double localFrac = (frac - starts[3]) / (1.0 - starts[3]);
        ratio *= 1.0 - 0.12 * std::clamp(localFrac, 0.0, 1.0);
    }
    return ratio;
}

class GaloVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        resonator_.setSampleRate(sr);
        breath_.setSampleRate(sr);
        breath_.setFilterCutoff(700.0);
        breath_.setAttackRelease(0.015, 0.12);
    }

    void configure(unsigned seed, const std::array<double, ModalResonator::kNumModes> &ratios,
                   const std::array<double, ModalResonator::kNumModes> &decays,
                   const std::array<double, ModalResonator::kNumModes> &gains) {
        resonator_.setModes(ratios, decays, gains);
        breath_.seed(seed);
    }

    void trigger(double baseFreqHz, double totalDurationSeconds) {
        baseFreq_ = baseFreqHz;
        totalDuration_ = totalDurationSeconds;
        elapsed_ = 0.0;
        tailSamplesRemaining_ = static_cast<long>(0.3 * sampleRate_);
        active_ = true;
        breath_.setGate(true);
    }

    bool isActive() const { return active_; }

    float tick() {
        if (!active_) return 0.0f;
        double ratio = galoCallContour(elapsed_, totalDuration_);
        resonator_.setFundamental(baseFreq_ * ratio);
        elapsed_ += 1.0 / sampleRate_;
        if (elapsed_ >= totalDuration_) {
            breath_.setGate(false);
            if (--tailSamplesRemaining_ <= 0) active_ = false;
        }
        float exciteAmt = breath_.next(0.4f);
        resonator_.excite(exciteAmt);
        resonator_.setBrightness(0.3f + 0.35f * breath_.envelope());
        // Same output-stage safety scaling the tuba voice already needed
        // (fase 81's real lesson: continuous excitation runs a resonator
        // far hotter than a single-strike use ever does).
        return std::tanh(resonator_.process() * 0.12f) * 0.9f;
    }

private:
    double sampleRate_ = 44100.0;
    double baseFreq_ = 700.0;
    double totalDuration_ = 1.4;
    double elapsed_ = 0.0;
    long tailSamplesRemaining_ = 0;
    bool active_ = false;
    ModalResonator resonator_;
    BreathExciter breath_;
};

} // namespace rasgo
