#pragma once
#include "MultibandFilter.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <random>

namespace rasgo {

// Real vowel formant values (F1/F2/F3, Hz) from Peterson & Barney's 1952
// acoustic study of American English vowels ("Control Methods Used in a
// Study of the Vowels", JASA) -- the classic, most-cited formant-frequency
// reference table in speech-synthesis literature. Five cardinal vowels,
// chosen for scat syllables (dah/deh/dee/doh/doo).
struct VowelFormants { double f1, f2, f3; };
inline VowelFormants scatVowelFormants(int vowelIndex) {
    static const VowelFormants table[5] = {
        {730.0, 1090.0, 2440.0},  // "ah"
        {530.0, 1840.0, 2480.0},  // "eh"
        {270.0, 2290.0, 3010.0},  // "ee"
        {570.0, 840.0,  2410.0},  // "oh"
        {300.0, 870.0,  2240.0},  // "oo"
    };
    return table[static_cast<size_t>(vowelIndex % 5)];
}

// "Scat": real source-filter (formant) speech synthesis -- the same
// documented technique AtropeloVoice (fase 90) uses, glottal pulse
// exciting three parallel bandpass formants -- but tuned to REAL,
// documented vowel-formant values instead of Atropelo's deliberately
// uncanny off-vowel placement. Opposite design goal from Atropelo: this
// is meant to read as a genuine (if synthetic-sounding) vowel, not
// almost-speech that refuses to resolve. Shares the underlying technique,
// not the code -- a fresh, independent instance, Atropelo untouched.
//
// No real consonant articulation modeled beyond a short noise-burst
// attack -- full consonant places/manners of articulation are a much
// larger undertaking. This produces wordless, meaning-free syllables
// (scat), not intelligible speech, by design and by honest limitation.
class ScatVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        for (auto &f : formants_) f.setSampleRate(sr);
    }

    void seed(unsigned s) { rng_.seed(s); }

    void beginSyllable(double pitchHz, double durationSeconds, int vowelIndex) {
        pitchHz_ = pitchHz;
        durationSamples_ = static_cast<int>(durationSeconds * sampleRate_);
        elapsedSamples_ = 0;
        glottalPhase_ = 0.0;
        consonantNoiseState_ = 0.0f;
        active_ = true;

        VowelFormants vf = scatVowelFormants(vowelIndex);
        formants_[0].setFrequency(vf.f1); formants_[0].setQ(9.0);
        formants_[1].setFrequency(vf.f2); formants_[1].setQ(11.0);
        formants_[2].setFrequency(vf.f3); formants_[2].setQ(8.0);
    }

    bool isActive() const { return active_; }

    float tick() {
        if (!active_) return 0.0f;
        glottalPhase_ += pitchHz_ / sampleRate_;
        if (glottalPhase_ >= 1.0) glottalPhase_ -= 1.0;
        float excitation = static_cast<float>(std::pow(1.0 - glottalPhase_, 8.0));

        // Short noise burst at onset -- a plain plosive/consonant-like
        // attack, fading out over the syllable's first ~15ms.
        int consonantSamples = static_cast<int>(0.015 * sampleRate_);
        if (elapsedSamples_ < consonantSamples) {
            float white = unit_(rng_) * 2.0f - 1.0f;
            consonantNoiseState_ += 0.3f * (white - consonantNoiseState_);
            float consonantEnv = 1.0f - static_cast<float>(elapsedSamples_) / static_cast<float>(consonantSamples);
            excitation += consonantNoiseState_ * 1.5f * consonantEnv;
        }

        float out = formants_[0].process(excitation) * 1.0f
                  + formants_[1].process(excitation) * 0.7f
                  + formants_[2].process(excitation) * 0.45f;

        int attackSamples = static_cast<int>(0.01 * sampleRate_);
        int releaseStart = static_cast<int>(durationSamples_ * 0.8);
        float env = 1.0f;
        if (elapsedSamples_ < attackSamples) {
            env = static_cast<float>(elapsedSamples_) / static_cast<float>(attackSamples);
        } else if (elapsedSamples_ > releaseStart) {
            int releaseSamples = durationSamples_ - releaseStart;
            env = 1.0f - static_cast<float>(elapsedSamples_ - releaseStart) / static_cast<float>(std::max(1, releaseSamples));
        }

        ++elapsedSamples_;
        if (elapsedSamples_ >= durationSamples_) active_ = false;
        return out * env * 0.3f;
    }

private:
    double sampleRate_ = 44100.0;
    std::array<StateVariableBandpass, 3> formants_;

    double pitchHz_ = 220.0;
    double glottalPhase_ = 0.0;
    int durationSamples_ = 0;
    int elapsedSamples_ = 0;
    bool active_ = false;
    float consonantNoiseState_ = 0.0f;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> unit_{0.0f, 1.0f};
};

} // namespace rasgo
