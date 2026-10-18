#pragma once
#include <array>
#include <random>
#include <cmath>
#include <algorithm>

namespace rasgo {

// Dynamic Stochastic Synthesis -- Iannis Xenakis's own technique (described
// in "Formalized Music", and later implemented as SuperCollider's core
// Gendy1/Gendy2 UGens; Kosmische's native binary carried a copy of this as
// `GendynUGens` in its bundled sc3-plugins collection, but kosmische.sc's
// actual patch never used it -- see Kosmische_Pericia_Tecnica.md). This is
// a from-scratch reimplementation of the publicly documented algorithm,
// not a port of any specific UGen's source (the original GendynUGens.cpp
// from Kosmische's bundled binary wasn't available to inspect in this
// session, unlike ChaosUGens.cpp).
//
// The idea: a waveform is a small number of breakpoints, cycled through in
// order. Every time a breakpoint is reached, its OWN amplitude and the
// duration to the NEXT breakpoint are each nudged by an independent random
// step -- not redrawn from scratch, a genuine random *walk* -- and the
// waveform is the linear interpolation between breakpoints as that walk
// slowly wanders. Amplitude and duration are each kept inside a min/max
// range not by clamping (which would flatten the walk against a wall and
// keep it stuck there) but by reflecting: a step that would overshoot the
// boundary bounces back in, the same way Xenakis's original describes it.
// The result never repeats -- there is no waveform table, no phase that
// wraps back to a fixed start -- and never runs away either, since the
// walk is always bounded. Structurally this is a genuinely different
// synthesis principle from every other module in this engine (oscillator+
// filter subtractive synthesis, modal/additive resonance, granular
// wavetable-of-the-recent-past) -- it doesn't have a "pitch" so much as a
// wandering, self-mutating shape.
class GendynOscillator {
public:
    enum class Distribution { Uniform, Gaussian, Cauchy, Logistic };

    static constexpr int kMaxPoints = 16;

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void seed(unsigned s) { rng_.seed(s); }

    // numPoints breakpoints per cycle; more points reads as a more complex/
    // "grainy" wander, fewer as a simpler, more clearly pitched buzz.
    void configure(int numPoints, double minDurationSeconds, double maxDurationSeconds) {
        numPoints_ = std::clamp(numPoints, 3, kMaxPoints);
        minDurSamples_ = std::max(1.0, minDurationSeconds * sampleRate_);
        maxDurSamples_ = std::max(minDurSamples_ + 1.0, maxDurationSeconds * sampleRate_);
        std::uniform_real_distribution<double> ampInit(-1.0, 1.0);
        std::uniform_real_distribution<double> durInit(minDurSamples_, maxDurSamples_);
        for (int i = 0; i < numPoints_; ++i) {
            amps_[static_cast<size_t>(i)] = ampInit(rng_);
            durs_[static_cast<size_t>(i)] = durInit(rng_);
        }
        currentIndex_ = 0;
        samplesIntoSegment_ = 0.0;
    }

    // How far each step of the random walk can move amplitude/duration, as
    // a fraction of the total range -- the "ADSCALE"/"DURSCALE" controls
    // in the original technique. 0 freezes the walk (a fixed, repeating
    // waveform); higher values wander further per step, reading as more
    // restless/noisy.
    void setStepScale(float ampStep, float durStep) {
        ampStepScale_ = std::clamp(ampStep, 0.0f, 1.0f);
        durStepScale_ = std::clamp(durStep, 0.0f, 1.0f);
    }
    void setDistribution(Distribution d) { distribution_ = d; }

    float tick() {
        double currentDur = durs_[static_cast<size_t>(currentIndex_)];
        double frac = currentDur > 0.0 ? std::clamp(samplesIntoSegment_ / currentDur, 0.0, 1.0) : 0.0;
        int nextIndex = (currentIndex_ + 1) % numPoints_;
        float out = static_cast<float>(amps_[static_cast<size_t>(currentIndex_)] * (1.0 - frac) +
                                        amps_[static_cast<size_t>(nextIndex)] * frac);

        samplesIntoSegment_ += 1.0;
        if (samplesIntoSegment_ >= durs_[static_cast<size_t>(currentIndex_)]) {
            samplesIntoSegment_ = 0.0;
            // Perturb THIS breakpoint's amplitude and duration now that
            // we're moving past it -- the walk step, reflected at the
            // [-1, 1] amplitude range and the [min, max] duration range so
            // the wander keeps moving instead of sticking to a wall.
            double ampStep = draw() * ampStepScale_ * 2.0;
            amps_[static_cast<size_t>(currentIndex_)] =
                reflect(amps_[static_cast<size_t>(currentIndex_)] + ampStep, -1.0, 1.0);

            double durStep = draw() * durStepScale_ * (maxDurSamples_ - minDurSamples_);
            durs_[static_cast<size_t>(currentIndex_)] =
                reflect(durs_[static_cast<size_t>(currentIndex_)] + durStep, minDurSamples_, maxDurSamples_);

            currentIndex_ = nextIndex;
        }
        return out;
    }

private:
    // Draws one random step in roughly [-1, 1] from the chosen distribution
    // -- these are the same family of shapes Xenakis's own technique (and
    // SuperCollider's Gendy1/2) offer for the random walk, picked for
    // variety of character rather than exhaustive coverage of all of them.
    double draw() {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        switch (distribution_) {
            case Distribution::Uniform:
                return unit(rng_) * 2.0 - 1.0;
            case Distribution::Gaussian: {
                std::normal_distribution<double> g(0.0, 0.35);
                return std::clamp(g(rng_), -1.0, 1.0);
            }
            case Distribution::Cauchy: {
                // Heavier tails than Gaussian -- occasional large jumps
                // among mostly small ones, Xenakis's own favourite for its
                // "spiky" character.
                double u = unit(rng_) - 0.5;
                double v = std::tan(3.141592653589793 * u * 0.9) * 0.15;
                return std::clamp(v, -1.0, 1.0);
            }
            case Distribution::Logistic: {
                double u = std::clamp(unit(rng_), 1e-6, 1.0 - 1e-6);
                double v = std::log(u / (1.0 - u)) * 0.2;
                return std::clamp(v, -1.0, 1.0);
            }
        }
        return 0.0;
    }

    static double reflect(double v, double lo, double hi) {
        double range = hi - lo;
        if (range <= 0.0) return lo;
        double x = v - lo;
        double m = std::fmod(x, 2.0 * range);
        if (m < 0.0) m += 2.0 * range;
        double reflected = (m <= range) ? m : (2.0 * range - m);
        return lo + reflected;
    }

    double sampleRate_ = 44100.0;
    int numPoints_ = 8;
    std::array<double, kMaxPoints> amps_{};
    std::array<double, kMaxPoints> durs_{};
    int currentIndex_ = 0;
    double samplesIntoSegment_ = 0.0;
    double minDurSamples_ = 10.0, maxDurSamples_ = 400.0;
    float ampStepScale_ = 0.1f, durStepScale_ = 0.1f;
    Distribution distribution_ = Distribution::Uniform;
    std::mt19937 rng_;
};

} // namespace rasgo
