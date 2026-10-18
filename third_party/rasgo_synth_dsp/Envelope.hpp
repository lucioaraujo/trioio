#pragma once
#include <algorithm>

namespace rasgo {

// Standard ADSR envelope generator, linear segments (fine for v1 -- can move
// to exponential/curved segments later without changing the interface).
class Envelope {
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setADSR(double attackSec, double decaySec, double sustainLevel, double releaseSec) {
        attackSec_ = std::max(attackSec, 0.0001);
        decaySec_ = std::max(decaySec, 0.0001);
        sustainLevel_ = std::clamp(sustainLevel, 0.0, 1.0);
        releaseSec_ = std::max(releaseSec, 0.0001);
    }

    // Polivoks EG "loop" function: instead of settling into Sustain once
    // Decay reaches its target, immediately re-trigger Attack -- turns the
    // whole envelope into a self-cycling AD source, a real modular "EG as
    // LFO" trick, not gated by any external note-on/note-off. Off by
    // default: every existing caller of this class (the main voice's own
    // amplitude envelope among them) keeps its exact current ADSR
    // behaviour unless this is explicitly turned on.
    void setLoopEnabled(bool loop) { loopEnabled_ = loop; }

    void noteOn() { stage_ = Stage::Attack; }
    void noteOff() { stage_ = Stage::Release; }

    // Polivoks EG "end of attack" output: true for exactly the one sample
    // where Attack completes and hands off to Decay, false every other
    // sample -- a real modular EG's own dedicated trigger output, usable
    // to fire something else at that exact instant rather than only
    // reading this envelope's continuous level.
    bool endOfAttack() const { return endOfAttackPulse_; }

    float tick() {
        endOfAttackPulse_ = false;
        double attackInc = 1.0 / (attackSec_ * sampleRate_);
        double decayInc = 1.0 / (decaySec_ * sampleRate_);
        double releaseInc = 1.0 / (releaseSec_ * sampleRate_);

        switch (stage_) {
            case Stage::Idle:
                level_ = 0.0;
                break;
            case Stage::Attack:
                level_ += attackInc;
                if (level_ >= 1.0) {
                    level_ = 1.0;
                    stage_ = Stage::Decay;
                    endOfAttackPulse_ = true;
                }
                break;
            case Stage::Decay:
                level_ -= decayInc * (1.0 - sustainLevel_);
                if (level_ <= sustainLevel_) {
                    level_ = sustainLevel_;
                    stage_ = loopEnabled_ ? Stage::Attack : Stage::Sustain;
                }
                break;
            case Stage::Sustain:
                level_ = sustainLevel_;
                break;
            case Stage::Release:
                level_ -= releaseInc * sustainLevel_ + releaseInc * 0.05;
                if (level_ <= 0.0) { level_ = 0.0; stage_ = Stage::Idle; }
                break;
        }
        return static_cast<float>(level_);
    }

    bool isActive() const { return stage_ != Stage::Idle; }

private:
    double sampleRate_ = 44100.0;
    double attackSec_ = 0.01, decaySec_ = 0.3, sustainLevel_ = 0.6, releaseSec_ = 0.3;
    double level_ = 0.0;
    Stage stage_ = Stage::Idle;
    bool loopEnabled_ = false;
    bool endOfAttackPulse_ = false;
};

} // namespace rasgo
