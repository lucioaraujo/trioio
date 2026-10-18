#pragma once
#include <cmath>
#include <random>

namespace rasgo {

// PolyBLEP (polynomial band-limited step, Valimaki/Huovilainen): a cheap
// polynomial correction applied only to the sample(s) right around a
// waveform's hard discontinuity, approximating the band-limited version of
// that step without oversampling. t is phase normalized 0..1, dt is the
// phase increment for this sample (frequency/sampleRate). This is what
// turns a naive saw/pulse (a raw digital step -- infinite harmonics,
// straight into aliasing above Nyquist that folds back as harsh, "crunchy"
// digital noise, worse the higher the pitch) into one whose harmonics
// actually roll off before Nyquist, the audible difference between a
// synth that sounds designed and one that sounds like a bug.
inline float polyBlep(double t, double dt) {
    if (t < dt) {
        double x = t / dt;
        return static_cast<float>(x + x - x * x - 1.0);
    } else if (t > 1.0 - dt) {
        double x = (t - 1.0) / dt;
        return static_cast<float>(x * x + x + x + 1.0);
    }
    return 0.0f;
}

// Classic multi-waveform oscillator: saw, pulse (variable width), sine, noise.
// Saw and pulse are corrected with PolyBLEP at their discontinuities --
// sine has none to begin with, and noise is already full-spectrum by
// definition, so neither needs (or benefits from) the correction.
class Oscillator {
public:
    enum class Waveform { Saw, Pulse, Sine, Noise };

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double hz) { frequency_ = hz; }
    void setWaveform(Waveform w) { waveform_ = w; }
    void setPulseWidth(double w) { pulseWidth_ = w; } // 0..1

    // Noise waveform used to draw from the global C std::rand() -- state
    // that persists for the lifetime of the process, not per-render, so a
    // second piece rendered in the same process (the JUCE app's normal
    // usage pattern) picked up wherever the first render's noise voices
    // left the stream, silently breaking "same seed = same piece" for any
    // voice using this waveform. Seeding this instead from the render's own
    // seed-derived rng keeps noise fully reproducible per-piece like every
    // other stochastic choice in the engine.
    void seedNoise(unsigned seed) { noiseRng_.seed(seed); }

    // Hard sync (VCO3's real "VCO Sync input", among countless other
    // analog VCOs): forcing this oscillator's phase back to a fixed point
    // regardless of where it currently sits, every time some OTHER
    // ("master") oscillator completes a cycle. didWrap() reports whether
    // THIS oscillator's own phase crossed 0 on the most recent tick() --
    // read it on the intended master, then call resetPhase() on the slave.
    void resetPhase(double p = 0.0) { phase_ = p; }
    bool didWrap() const { return wrapped_; }

    // Advance one sample and return the next output in roughly [-1, 1].
    float tick() {
        double dt = frequency_ / sampleRate_;
        float out = 0.0f;
        switch (waveform_) {
            case Waveform::Saw:
                out = static_cast<float>(2.0 * phase_ - 1.0);
                out -= polyBlep(phase_, dt);
                break;
            case Waveform::Pulse: {
                out = (phase_ < pulseWidth_) ? 1.0f : -1.0f;
                out += polyBlep(phase_, dt);
                double shifted = phase_ + (1.0 - pulseWidth_);
                shifted -= std::floor(shifted);
                out -= polyBlep(shifted, dt);
                break;
            }
            case Waveform::Sine:
                out = static_cast<float>(std::sin(2.0 * M_PI * phase_));
                break;
            case Waveform::Noise:
                out = noiseDist_(noiseRng_);
                break;
        }

        phase_ += dt;
        wrapped_ = false;
        if (phase_ >= 1.0) { phase_ -= 1.0; wrapped_ = true; }
        if (phase_ < 0.0) phase_ += 1.0;

        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double frequency_ = 220.0;
    double phase_ = 0.0;
    double pulseWidth_ = 0.5;
    Waveform waveform_ = Waveform::Saw;
    bool wrapped_ = false;
    std::mt19937 noiseRng_{1};
    std::uniform_real_distribution<float> noiseDist_{-1.0f, 1.0f};
};

} // namespace rasgo
