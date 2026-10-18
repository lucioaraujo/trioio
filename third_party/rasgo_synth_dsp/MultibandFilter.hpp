#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace rasgo {

// Inspired by the CBS Musical Instruments "Bandpass Filter #194" -- a
// hardware 4-band frequency divider, one of the schematics gathered during
// the circuit-translation phase but left as a pendency until now. Rather
// than reproduce its exact discrete topology, this ports the idea behind
// it: split one signal into 4 fixed bandpass bands whose levels can move
// independently, so a voice can be pulled apart into shifting spectral
// emphasis instead of always sounding "whole" -- a formant-like texture
// that fits the same "crossing energy flows" character as the rest of the
// engine, driven here by whichever modulation source (chaos map, Buchla
// stepped source, the narrative arc) makes sense for a given patch.
//
// Each band is a Chamberlin-style state-variable filter run in bandpass
// mode: cheap and stable enough to run several in parallel per sample,
// independently written from the classic (public-domain-adjacent, widely
// published) SVF difference equations -- not a port of any particular
// codebase.
class StateVariableBandpass {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double hz) { frequencyHz_ = hz; }
    void setQ(double q) { q_ = std::max(0.5, q); }

    float process(float input) {
        double f = 2.0 * std::sin(kPi * frequencyHz_ / sampleRate_);
        f = std::min(f, 1.9); // stability guard as frequency approaches Nyquist
        double qInv = 1.0 / q_;

        low_ += f * band_;
        double high = static_cast<double>(input) - low_ - qInv * band_;
        band_ += f * high;

        return static_cast<float>(band_);
    }

private:
    static constexpr double kPi = 3.14159265358979323846;
    double sampleRate_ = 44100.0;
    double frequencyHz_ = 1000.0;
    double q_ = 2.0;
    double low_ = 0.0, band_ = 0.0;
};

class MultibandFilter {
public:
    static constexpr int kNumBands = 4;

    void setSampleRate(double sr) {
        for (auto &band : bands_) band.setSampleRate(sr);
    }

    void setBandFrequencies(const std::array<double, kNumBands> &freqsHz) {
        for (int i = 0; i < kNumBands; ++i) bands_[i].setFrequency(freqsHz[i]);
    }

    void setBandQ(int band, double q) { bands_[band].setQ(q); }
    void setBandGain(int band, float gain) { gains_[band] = gain; }
    float getBandGain(int band) const { return gains_[band]; }

    // Sum of the 4 bandpassed-and-weighted bands. Gains are typically kept
    // roughly summing to ~1 by the caller so the output doesn't balloon in
    // level as bands are added, but nothing here enforces that -- letting
    // several bands peak together is part of the effect.
    float process(float input) {
        float sum = 0.0f;
        for (int i = 0; i < kNumBands; ++i)
            sum += bands_[i].process(input) * gains_[i];
        return sum;
    }

private:
    std::array<StateVariableBandpass, kNumBands> bands_;
    std::array<float, kNumBands> gains_ { 1.0f, 1.0f, 1.0f, 1.0f };
};

} // namespace rasgo
