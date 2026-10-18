#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace rasgo {

// Walsh functions (Walsh, 1923): the complete orthogonal basis of +-1
// rectangular waves, the square-wave equivalent of the sine/cosine basis
// ordinary Fourier synthesis uses. Every oscillator elsewhere in this
// engine builds its waveform from sinusoids (Oscillator/MorphOscillator),
// interpolated random breakpoints (GendynOscillator), or an LFO-swept
// crossfade between fixed shapes (MorphOscillator again) -- this is a
// genuinely different fourth principle: additive combination of
// orthogonal RECTANGULAR functions, ordered by "sequency" (number of
// sign changes per period) the same way a Fourier series is ordered by
// harmonic number. It is also real vintage-hardware technique -- Walsh
// synthesis needs no trigonometry, only sign flips and additions, which
// is exactly why it saw real use on cheap digital hardware; the same
// "port real, historically-used cheap-circuit synthesis" spirit already
// behind AtariPunkConsole/AY8910Chip/FixedPointOverflow in this codebase.
//
// Construction, verified by a standalone test before wiring this in
// (exact +-1 values, exact orthogonality via integer dot products, and
// strictly non-decreasing sign-change count with k -- the actual
// "sequency" property, not just a plausible-looking formula): build the
// natural-order Hadamard matrix in closed form (entry (r,c) is -1 to the
// power of the parity of popcount(r & c), the standard closed-form
// definition -- no recursion needed), then reorder its rows via
// bitReverse(grayCode(k)) to walk them in ascending sequency instead of
// natural Hadamard order. An earlier attempt built each row directly by
// sampling sign(sin(2^i * pi * t)) products at the table's discrete
// points -- it passed the "+-1 values" check but FAILED orthogonality
// (discrete sampling of a continuous definition doesn't preserve it);
// the integer Hadamard-matrix route below is exact by construction and
// is what actually shipped.
class WalshOscillator {
public:
    static constexpr int kOrder = 6;               // 2^6 = 64 steps/period
    static constexpr int kTableSize = 1 << kOrder; // 64

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double hz) { frequency_ = hz; }

    // weights[k] is this piece's amount of the k-th sequency (k=0 is the
    // constant/DC term, k=1 the lowest-sequency square-wave-like term,
    // and so on) -- the same role "which harmonics, how loud" plays in
    // additive synthesis. Values beyond weights.size() are silent. The
    // resulting one-period table is renormalized to fill -1..1 exactly,
    // since arbitrary weight combinations don't self-normalize.
    void configure(const std::vector<double> &weights) {
        std::array<double, kTableSize> raw{};
        double maxAbs = 0.0;
        for (int step = 0; step < kTableSize; ++step) {
            double sum = 0.0;
            for (size_t k = 0; k < weights.size() && k < static_cast<size_t>(kTableSize); ++k) {
                sum += weights[k] * static_cast<double>(walsh(static_cast<int>(k), step));
            }
            raw[static_cast<size_t>(step)] = sum;
            maxAbs = std::max(maxAbs, std::abs(sum));
        }
        double norm = maxAbs > 1e-9 ? 1.0 / maxAbs : 1.0;
        for (int step = 0; step < kTableSize; ++step) {
            table_[static_cast<size_t>(step)] = static_cast<float>(raw[static_cast<size_t>(step)] * norm);
        }
    }

    // Advance one sample and return the next output in [-1, 1]. Reads the
    // table step nearest the current phase with no interpolation -- the
    // steps ARE this oscillator's identity ("sintese de onda escalonada
    // digital"); smoothing them would just make it a worse ordinary
    // wavetable oscillator instead of what it actually is.
    float tick() {
        double dt = frequency_ / sampleRate_;
        int step = static_cast<int>(phase_ * kTableSize);
        step = std::clamp(step, 0, kTableSize - 1);
        float out = table_[static_cast<size_t>(step)];
        phase_ += dt;
        if (phase_ >= 1.0) phase_ -= 1.0;
        if (phase_ < 0.0) phase_ += 1.0;
        return out;
    }

private:
    // Natural-order Hadamard matrix entry, closed form: H(r,c) = (-1)^popcount(r & c).
    static int hadamardNatural(int r, int c) {
        int bits = r & c;
        int parity = 0;
        while (bits) { parity ^= (bits & 1); bits >>= 1; }
        return parity ? -1 : 1;
    }
    static int grayCode(int n) { return n ^ (n >> 1); }
    static int bitReverse(int n, int numBits) {
        int result = 0;
        for (int i = 0; i < numBits; ++i) {
            if (n & (1 << i)) result |= (1 << (numBits - 1 - i));
        }
        return result;
    }
    // wal(k, step): sequency-ordered Walsh function, value in {-1, +1}.
    static int walsh(int k, int step) {
        int r = bitReverse(grayCode(k), kOrder);
        return hadamardNatural(r, step);
    }

    double sampleRate_ = 44100.0;
    double frequency_ = 220.0;
    double phase_ = 0.0;
    std::array<float, kTableSize> table_{};
};

} // namespace rasgo
