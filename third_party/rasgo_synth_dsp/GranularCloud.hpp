#pragma once
#include <array>
#include <cmath>
#include <random>
#include <vector>

namespace rasgo {

// A granular texture processor: it continuously records its own input into
// a rolling buffer, then scatters short overlapping windowed "grains" back
// out, each read from a random recent point in that buffer at a randomized
// pitch and duration. This is the same core idea as Mutable Instruments'
// Clouds (Emilie Gillet, MIT-licensed, github.com/pichenettes/eurorack) in
// its simplest granular-delay mode -- reimplemented here as a compact
// time-domain version rather than Clouds' actual phase-vocoder/spectral
// engine, which is considerably more involved. Feeding it the engine's own
// recent output turns whatever just played into a smeared, drifting wash
// of itself -- the "waves breaking and generating backwash" texture from
// the original Kosmische pericia, now literally built from the piece's own
// immediate past.
class GranularCloud {
public:
    static constexpr int kMaxGrains = 16;

    void setSampleRate(double sr, double bufferSeconds = 2.0) {
        sampleRate_ = sr;
        bufferSize_ = std::max<size_t>(1, static_cast<size_t>(bufferSeconds * sr));
        buffer_.assign(bufferSize_, 0.0f);
        writePos_ = 0;
        for (auto &g : grains_) g.active = false;
    }
    void seed(unsigned s) { rng_.seed(s); }

    void setDensity(double grainsPerSecond) { density_ = grainsPerSecond; }
    void setGrainDuration(double minSeconds, double maxSeconds) { grainMin_ = minSeconds; grainMax_ = maxSeconds; }
    // How far back into its own recent past a grain can start (seconds).
    void setSpread(double lookbackSeconds) { spreadSeconds_ = lookbackSeconds; }
    void setPitchJitter(double semitones) { pitchJitterSemis_ = semitones; }
    void setMix(float m) { mix_ = m; }

    float process(float input) {
        buffer_[writePos_] = input;
        writePos_ = (writePos_ + 1) % bufferSize_;

        // Per-sample trigger probability approximating a Poisson process at
        // the configured grain density.
        if (unit_(rng_) < density_ / sampleRate_) spawnGrain();

        float wet = 0.0f;
        for (auto &g : grains_) {
            if (!g.active) continue;
            wet += readGrain(g);
            g.age += 1.0;
            if (g.age >= g.lengthSamples) g.active = false;
        }
        return input * (1.0f - mix_) + wet * mix_;
    }

private:
    struct Grain {
        bool active = false;
        double startReadPos = 0.0;
        double playbackRate = 1.0;
        double age = 0.0;
        double lengthSamples = 1.0;
        float gain = 1.0f;
    };

    void spawnGrain() {
        for (auto &g : grains_) {
            if (g.active) continue;
            double lookback = unit_(rng_) * spreadSeconds_ * sampleRate_;
            double startPos = static_cast<double>(writePos_) - lookback;
            while (startPos < 0.0) startPos += static_cast<double>(bufferSize_);
            g.startReadPos = startPos;
            double durSec = grainMin_ + unit_(rng_) * (grainMax_ - grainMin_);
            g.lengthSamples = std::max(1.0, durSec * sampleRate_);
            double semis = (unit_(rng_) * 2.0 - 1.0) * pitchJitterSemis_;
            g.playbackRate = std::pow(2.0, semis / 12.0);
            g.age = 0.0;
            g.gain = 0.25f + 0.2f * static_cast<float>(unit_(rng_));
            g.active = true;
            return; // one grain per spawn attempt; if every slot is busy, skip
        }
    }

    float readGrain(const Grain &g) const {
        double readPos = g.startReadPos + g.age * g.playbackRate;
        double bs = static_cast<double>(bufferSize_);
        while (readPos >= bs) readPos -= bs;
        while (readPos < 0.0) readPos += bs;
        size_t i0 = static_cast<size_t>(readPos);
        size_t i1 = (i0 + 1) % bufferSize_;
        float frac = static_cast<float>(readPos - static_cast<double>(i0));
        float sample = buffer_[i0] * (1.0f - frac) + buffer_[i1] * frac;

        double t = g.age / g.lengthSamples; // 0..1 through the grain
        float window = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * 3.141592653589793 * t));
        return sample * window * g.gain;
    }

    double sampleRate_ = 44100.0;
    std::vector<float> buffer_;
    size_t bufferSize_ = 0;
    size_t writePos_ = 0;
    std::array<Grain, kMaxGrains> grains_{};

    double density_ = 10.0;      // grains per second
    double grainMin_ = 0.05, grainMax_ = 0.16; // seconds
    double spreadSeconds_ = 0.7;
    double pitchJitterSemis_ = 5.0;
    float mix_ = 0.15f;

    std::mt19937 rng_ { 1u };
    std::uniform_real_distribution<double> unit_ { 0.0, 1.0 };
};

} // namespace rasgo
