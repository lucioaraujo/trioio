#pragma once
#include "ScatVoice.hpp"
#include <array>
#include <cmath>
#include <random>

namespace rasgo {

// "Coral": real polyphony built from ScatVoice (fase 98) -- four
// independent voices in SATB-like registers (soprano/alto/tenor/bass),
// each choosing its OWN chord tone voice-led into its own register (real
// vertical harmony, not four voices doubling one pitch), triggered by a
// shared syllable pulse -- same vowel across all four, so the result
// reads as ONE choir singing together, not four unrelated voices -- but
// each voice carries its own onset-timing jitter and dynamics variation,
// the real "imperfect ensemble" character of a human choir: singers never
// land in perfect unison, and never sing at identical loudness.
class ChoirEnsemble {
public:
    static constexpr int kNumVoices = 4;

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        for (auto &v : voices_) v.setSampleRate(sr);
    }

    void seed(unsigned s) {
        rng_.seed(s);
        for (int i = 0; i < kNumVoices; ++i) {
            voices_[static_cast<size_t>(i)].seed(s + static_cast<unsigned>(i) * 977u);
        }
    }

    // Semitone offset from the piece's root each voice aims for --
    // soprano highest, bass lowest, real SATB spread.
    void setRegisterOffsets(const std::array<int, kNumVoices> &offsets) { registerOffsets_ = offsets; }

    // Arms a shared syllable across the whole ensemble.
    void triggerPhrase(int rootMidiNote, const std::array<int, 4> &chordOffsets,
                       int vowelIndex, double baseDurationSeconds) {
        for (int i = 0; i < kNumVoices; ++i) {
            size_t vi = static_cast<size_t>(i);
            // Voice-led chord-tone choice: whichever chord tone (across a
            // couple of octaves) lands closest to this voice's own
            // register target -- real vertical harmony, not doubling.
            int target = rootMidiNote + registerOffsets_[vi];
            int bestNote = target;
            int bestDist = 999;
            for (int octave = -1; octave <= 1; ++octave) {
                for (int c = 0; c < 4; ++c) {
                    int candidate = rootMidiNote + chordOffsets[static_cast<size_t>(c)] + octave * 12;
                    int dist = std::abs(candidate - target);
                    if (dist < bestDist) { bestDist = dist; bestNote = candidate; }
                }
            }
            // 0-50ms onset spread and per-voice loudness/duration
            // jitter -- real ensembles never land perfectly together.
            onsetDelaySamples_[vi] = static_cast<int>(unit_(rng_) * 0.05 * sampleRate_);
            pendingNote_[vi] = bestNote;
            pendingVowel_[vi] = vowelIndex;
            pendingDuration_[vi] = baseDurationSeconds * (0.85 + unit_(rng_) * 0.3);
            dynamicsJitter_[vi] = 0.7f + unit_(rng_) * 0.5f;
            pendingArmed_[vi] = true;
        }
    }

    float tick() {
        float sum = 0.0f;
        for (int i = 0; i < kNumVoices; ++i) {
            size_t vi = static_cast<size_t>(i);
            if (pendingArmed_[vi]) {
                if (onsetDelaySamples_[vi] > 0) {
                    --onsetDelaySamples_[vi];
                } else {
                    voices_[vi].beginSyllable(midiToFreqLocal(pendingNote_[vi]), pendingDuration_[vi], pendingVowel_[vi]);
                    pendingArmed_[vi] = false;
                }
            }
            sum += voices_[vi].tick() * dynamicsJitter_[vi];
        }
        return sum;
    }

    bool anyActive() const {
        for (int i = 0; i < kNumVoices; ++i) {
            size_t vi = static_cast<size_t>(i);
            if (pendingArmed_[vi] || voices_[vi].isActive()) return true;
        }
        return false;
    }

private:
    static double midiToFreqLocal(int midiNote) {
        return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    }

    double sampleRate_ = 44100.0;
    std::array<ScatVoice, kNumVoices> voices_;
    std::array<int, kNumVoices> registerOffsets_{12, 5, -5, -12};
    std::array<int, kNumVoices> onsetDelaySamples_{};
    std::array<int, kNumVoices> pendingNote_{};
    std::array<int, kNumVoices> pendingVowel_{};
    std::array<double, kNumVoices> pendingDuration_{};
    std::array<float, kNumVoices> dynamicsJitter_{};
    std::array<bool, kNumVoices> pendingArmed_{};
    std::mt19937 rng_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
};

} // namespace rasgo
