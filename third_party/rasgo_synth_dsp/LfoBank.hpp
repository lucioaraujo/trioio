#pragma once
#include <array>
#include <cmath>
#include <random>

// Kosmische's own patch (supercolliderSource/kosmische.sc) defined four
// complete LFOs -- sine, triangle, pulse, and sample-and-hold, each
// independently routable to one of eight destinations (osc1/osc2 frequency
// and width, filter cutoff and resonance, delay time and decay time) via its
// own SuperCollider control bus. The Android UI built on top of it only ever
// exposed two of the four (see Kosmische_Pericia_Tecnica.md: "o codigo
// define quatro LFOs completos ... enquanto a interface visivel menciona
// apenas dois"), so half of that modulation matrix sat compiled into the
// binary but permanently unreachable. This ports all four shapes and wires
// all four LFOs into the piece renderer -- the part of Kosmische that never
// actually got to run.
//
// The sample-and-hold shape needs its own seeded RNG (not a shared global)
// so a piece rendered twice from the same seed comes out identical, in
// keeping with the rest of this engine.

namespace rasgo {

class Lfo {
public:
    enum class Shape { Sine, Triangle, Pulse, SampleHold };

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setShape(Shape s) { shape_ = s; }
    void setFrequency(double hz) { freqHz_ = hz; }
    void setDepth(float d) { depth_ = d; }
    void seed(unsigned s) { rng_.seed(s); }

    // Returns depth * (shape value in [-1, 1]).
    float tick() {
        double inc = freqHz_ / std::max(1.0, sampleRate_);
        phase_ += inc;
        bool wrapped = phase_ >= 1.0;
        if (wrapped) {
            phase_ -= std::floor(phase_);
            if (shape_ == Shape::SampleHold)
                held_ = static_cast<float>(unit_(rng_) * 2.0 - 1.0);
        }

        float raw;
        switch (shape_) {
            case Shape::Triangle: raw = static_cast<float>(4.0 * std::abs(phase_ - 0.5) - 1.0); break;
            case Shape::Pulse:    raw = phase_ < 0.5 ? 1.0f : -1.0f; break;
            case Shape::SampleHold: raw = held_; break;
            case Shape::Sine:
            default: raw = static_cast<float>(std::sin(2.0 * 3.141592653589793 * phase_)); break;
        }
        return raw * depth_;
    }

private:
    double sampleRate_ = 44100.0;
    double freqHz_ = 0.2;
    double phase_ = 0.0;
    float depth_ = 0.0f;
    float held_ = 0.0f;
    Shape shape_ = Shape::Sine;
    std::mt19937 rng_ { 1u };
    std::uniform_real_distribution<double> unit_ { 0.0, 1.0 };
};

class LfoBank {
public:
    static constexpr int kNumLfos = 4;

    void setSampleRate(double sr) { for (auto &l : lfos_) l.setSampleRate(sr); }
    Lfo &lfo(int i) { return lfos_[i]; }

    std::array<float, kNumLfos> tick() {
        std::array<float, kNumLfos> out;
        for (int i = 0; i < kNumLfos; ++i) out[i] = lfos_[i].tick();
        return out;
    }

private:
    Lfo lfos_[kNumLfos];
};

} // namespace rasgo
