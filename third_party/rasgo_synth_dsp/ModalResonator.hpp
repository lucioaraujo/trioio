#pragma once
#include <array>
#include <cmath>
#include <algorithm>

namespace rasgo {

// A modal resonator: a small bank of independently-tuned two-pole resonant
// filters, each standing in for one vibrational mode of a struck object --
// a bar, a bell, a stretched membrane. This is the same core idea behind
// Mutable Instruments' Rings (Emilie Gillet, MIT-licensed,
// github.com/pichenettes/eurorack: physical-modeling resonator over an
// exciter signal) -- reimplemented independently here as a compact
// standalone version rather than a line-by-line port of Rings' actual
// exciter/resonator/parametric-EQ chain. Feed it short excitation bursts
// and each mode rings out at its own frequency and decay, summing into a
// "struck object" timbre that's a genuinely different sound-generation
// principle from this engine's subtractive (oscillator + filter) voice --
// additive/resonant rather than subtractive.
class ModalResonator {
public:
    static constexpr int kNumModes = 6;

    void setSampleRate(double sr) { sampleRate_ = sr; updateCoeffs(); }
    void setFundamental(double hz) { fundamental_ = hz; updateCoeffs(); }

    // ratios describe each mode relative to the fundamental (1.0 = the
    // fundamental itself); slightly inharmonic ratios (not exact integers)
    // are what makes it read as a bar/bell rather than a plucked string.
    void setModes(const std::array<double, kNumModes> &ratios,
                  const std::array<double, kNumModes> &decaySeconds,
                  const std::array<double, kNumModes> &gains) {
        ratios_ = ratios; decaySeconds_ = decaySeconds; gains_ = gains;
        updateCoeffs();
    }

    // Rolls off the higher modes' contribution without changing their
    // decay -- reads as a tone/material control (dark wood vs. bright
    // metal) rather than a length change.
    void setBrightness(float b) { brightness_ = std::clamp(b, 0.0f, 1.0f); }

    // Drives every mode with one forcing sample -- call once per "strike"
    // for a hard, mallet-like attack.
    void excite(float amount) {
        for (int m = 0; m < kNumModes; ++m)
            pendingExcite_[static_cast<size_t>(m)] += amount * static_cast<float>(gains_[static_cast<size_t>(m)]);
    }

    float process() {
        float out = 0.0f;
        for (int m = 0; m < kNumModes; ++m) {
            size_t i = static_cast<size_t>(m);
            float y0 = static_cast<float>(a1_[i]) * y1_[i] - static_cast<float>(a2_[i]) * y2_[i] + pendingExcite_[i];
            pendingExcite_[i] = 0.0f;
            // Same defensive move as SallenKeyFilter/HenonMap/QuadMap
            // elsewhere in this codebase: a mode driven close enough to
            // Nyquist (possible now that KSRandomizerCore's octave bias
            // can push a struck note's fundamental higher than the old
            // flat-uniform jitter typically did) can lose the numerical
            // margin this simple two-pole recursion relies on. Reset that
            // one mode rather than let it ring away to infinity.
            if (!std::isfinite(y0) || std::fabs(y0) > 50.0f) {
                y0 = 0.0f;
                y1_[i] = 0.0f;
                y2_[i] = 0.0f;
            } else {
                y2_[i] = y1_[i];
                y1_[i] = y0;
            }
            float modeWeight = (m == 0) ? 1.0f : brightness_;
            out += y0 * modeWeight;
        }
        return out * 0.18f; // headroom: modes can constructively sum above unity
    }

private:
    void updateCoeffs() {
        for (int m = 0; m < kNumModes; ++m) {
            size_t i = static_cast<size_t>(m);
            double freq = std::clamp(fundamental_ * ratios_[i], 20.0, sampleRate_ * 0.45);
            double w = 2.0 * 3.141592653589793 * freq / sampleRate_;
            double decaySamples = std::max(1.0, decaySeconds_[i] * sampleRate_);
            double r = std::exp(-1.0 / decaySamples);
            a1_[i] = 2.0 * r * std::cos(w);
            a2_[i] = r * r;
        }
    }

    double sampleRate_ = 44100.0;
    double fundamental_ = 220.0;
    std::array<double, kNumModes> ratios_{ {1.0, 2.01, 3.06, 4.15, 5.35, 6.7} };
    std::array<double, kNumModes> decaySeconds_{ {1.2, 0.9, 0.7, 0.5, 0.35, 0.25} };
    std::array<double, kNumModes> gains_{ {1.0, 0.6, 0.4, 0.28, 0.18, 0.12} };
    std::array<double, kNumModes> a1_{}, a2_{};
    std::array<float, kNumModes> y1_{}, y2_{};
    std::array<float, kNumModes> pendingExcite_{};
    float brightness_ = 0.6f;
};

} // namespace rasgo
