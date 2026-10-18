#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace rasgo {

// A software model of the General Instrument AY-3-8910 (and its many
// compatible clones -- the YM2149 etc.), the 3-channel square-wave sound
// chip behind the ZX Spectrum 128, MSX, Amstrad CPC, Atari ST and a good
// slice of early-80s arcade hardware. Kosmische's native binary carried
// this as `AY_UGen` in its bundled sc3-plugins collection but
// kosmische.sc's actual patch never used it (see
// Kosmische_Pericia_Tecnica.md) -- so it never made a sound in the
// original app despite being compiled in.
//
// This models the chip's well-documented public architecture (three tone
// generators, one shared noise generator, one hardware envelope
// generator, and a mixer with per-channel tone/noise enable bits) from
// general technical references, not from any specific emulator's source
// code -- simplified where the real chip's behaviour is chip-revision-
// specific trivia (e.g. the exact 16-entry envelope shape table is
// collapsed here to the four shapes that cover its actual musically
// distinct cases: decay-once, decay-repeat, attack-once, attack-repeat)
// rather than reproduced byte-for-byte.
class AY8910Chip {
public:
    static constexpr int kNumChannels = 3;

    enum class EnvelopeShape { DecayOnce, DecayRepeat, AttackOnce, AttackRepeat };

    void setSampleRate(double sr) { sampleRate_ = sr; }

    // The real chip divides a master clock (~1.7MHz typical) by 16 per
    // tone period step; exposing setFrequency directly in Hz is simpler to
    // drive from the rest of this engine and produces the same square-wave
    // behaviour the chip's own period counter gives.
    void setToneFrequency(int channel, double hz) {
        toneFreq_[static_cast<size_t>(channel)] = std::max(1.0, hz);
    }
    void setToneEnabled(int channel, bool on) { toneEnabled_[static_cast<size_t>(channel)] = on; }
    void setNoiseEnabled(int channel, bool on) { noiseEnabled_[static_cast<size_t>(channel)] = on; }

    // 0..1 per channel; a channel can instead be driven by the shared
    // envelope generator via useEnvelope(channel, true), matching the
    // real chip's "volume register bit 4 selects envelope instead of a
    // fixed level" behaviour.
    void setVolume(int channel, float v) { volume_[static_cast<size_t>(channel)] = std::clamp(v, 0.0f, 1.0f); }
    void useEnvelope(int channel, bool on) { envelopeDriven_[static_cast<size_t>(channel)] = on; }

    void setNoiseFrequency(double hz) { noiseFreq_ = std::max(1.0, hz); }

    void setEnvelopeFrequency(double hz) { envelopeFreq_ = std::max(0.01, hz); }
    void setEnvelopeShape(EnvelopeShape shape) {
        envelopeShape_ = shape;
        // Decay shapes start from full level and count down; attack
        // shapes start from silence and count up -- same as the real
        // chip's envelope counter resetting appropriately whenever its
        // shape register is (re)written.
        envelopeLevel_ = (shape == EnvelopeShape::DecayOnce || shape == EnvelopeShape::DecayRepeat) ? 1.0 : 0.0;
    }

    float tick() {
        // --- Tone generators: one square wave per channel, own phase. ---
        for (int c = 0; c < kNumChannels; ++c) {
            size_t i = static_cast<size_t>(c);
            tonePhase_[i] += toneFreq_[i] / sampleRate_;
            if (tonePhase_[i] >= 1.0) {
                tonePhase_[i] -= 1.0;
                toneState_[i] = !toneState_[i];
            }
        }

        // --- Shared noise generator: 17-bit LFSR, the same structure as
        // the real chip's, clocked at its own (typically slower) rate. ---
        noisePhase_ += noiseFreq_ / sampleRate_;
        if (noisePhase_ >= 1.0) {
            noisePhase_ -= 1.0;
            bool feedback = ((noiseLfsr_ ^ (noiseLfsr_ >> 3)) & 1u) != 0;
            noiseLfsr_ = (noiseLfsr_ >> 1) | (static_cast<uint32_t>(feedback) << 16);
        }
        bool noiseBit = (noiseLfsr_ & 1u) != 0;

        // --- Hardware envelope generator: a ramp that resets/reflects
        // per the four shapes exposed above. ---
        envelopePhase_ += envelopeFreq_ / sampleRate_;
        if (envelopePhase_ >= 1.0) {
            envelopePhase_ -= 1.0;
            advanceEnvelope();
        }
        float envLevel = static_cast<float>(envelopeLevel_);

        // --- Mixer: per channel, AND the tone/noise gates (real chip
        // behaviour: an unmuted line reads as constantly-high), scale by
        // that channel's volume or the shared envelope. ---
        float sum = 0.0f;
        for (int c = 0; c < kNumChannels; ++c) {
            size_t i = static_cast<size_t>(c);
            bool toneLine = !toneEnabled_[i] || toneState_[i];
            bool noiseLine = !noiseEnabled_[i] || noiseBit;
            float level = (toneLine && noiseLine) ? 1.0f : 0.0f;
            float vol = envelopeDriven_[i] ? envLevel : volume_[i];
            sum += (level * 2.0f - 1.0f) * vol;
        }
        return sum / static_cast<float>(kNumChannels);
    }

private:
    void advanceEnvelope() {
        switch (envelopeShape_) {
            case EnvelopeShape::DecayOnce:
                envelopeLevel_ = std::max(0.0, envelopeLevel_ - 1.0 / 16.0);
                break;
            case EnvelopeShape::DecayRepeat:
                envelopeLevel_ -= 1.0 / 16.0;
                if (envelopeLevel_ < 0.0) envelopeLevel_ = 1.0;
                break;
            case EnvelopeShape::AttackOnce:
                envelopeLevel_ = std::min(1.0, envelopeLevel_ + 1.0 / 16.0);
                break;
            case EnvelopeShape::AttackRepeat:
                envelopeLevel_ += 1.0 / 16.0;
                if (envelopeLevel_ > 1.0) envelopeLevel_ = 0.0;
                break;
        }
    }

    double sampleRate_ = 44100.0;

    std::array<double, kNumChannels> toneFreq_{ {220.0, 220.0, 220.0} };
    std::array<double, kNumChannels> tonePhase_{};
    std::array<bool, kNumChannels> toneState_{};
    std::array<bool, kNumChannels> toneEnabled_{ {true, true, true} };
    std::array<bool, kNumChannels> noiseEnabled_{};
    std::array<float, kNumChannels> volume_{ {0.5f, 0.5f, 0.5f} };
    std::array<bool, kNumChannels> envelopeDriven_{};

    double noiseFreq_ = 5000.0;
    double noisePhase_ = 0.0;
    uint32_t noiseLfsr_ = 0x1ffff;

    double envelopeFreq_ = 4.0;
    double envelopePhase_ = 0.0;
    double envelopeLevel_ = 1.0;
    EnvelopeShape envelopeShape_ = EnvelopeShape::DecayRepeat;
};

} // namespace rasgo
