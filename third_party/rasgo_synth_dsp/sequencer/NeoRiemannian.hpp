#pragma once
#include <string>

namespace rasgo {

// Neo-Riemannian triadic transformations (Hugo Riemann's original theory,
// the modern P/L/R formalization from Cohn/Lewin's transformational
// theory) -- three involutions on the 24 major/minor triads, each
// changing exactly ONE of the triad's three pitch classes and leaving the
// other two untouched. This is what "parsimonious voice leading" means:
// harmonically distant chords in ordinary functional terms (C major to E
// minor, say) turn out to be a single semitone move away in this system.
// Genuinely different from this engine's existing chord-progression
// system (StepSequencer's fixed roman-numeral progressions, built by
// stacking thirds WITHIN whatever scale table happens to be active, so
// the interval sizes vary with the scale) -- these three operations work
// on real, fixed-interval chromatic triads (major = root/root+4/root+7,
// minor = root/root+3/root+7), entirely independent of any scale.
//
// Verified by an isolated test before wiring in (same discipline as the
// Walsh oscillator / sieve scale): the three canonical textbook examples
// (P(C major)=C minor, L(C major)=E minor, R(C major)=A minor), the
// involution property for all 24 triads (apply twice, get back the
// original), and the shared-tone property (each operation preserves
// exactly 2 of the 3 pitch classes, never 0, never 3).
struct Triad {
    int rootPc = 0;   // 0-11, pitch class of the root
    bool isMajor = true;
};

inline int mod12(int x) { int m = x % 12; return m < 0 ? m + 12 : m; }

// Parallel: same root, quality flips (C major <-> C minor).
inline Triad applyParallel(Triad t) { return { t.rootPc, !t.isMajor }; }

// Leading-tone exchange: major root r <-> minor root r+4 (C major <-> E minor).
inline Triad applyLeadingTone(Triad t) {
    if (t.isMajor) return { mod12(t.rootPc + 4), false };
    return { mod12(t.rootPc - 4), true };
}

// Relative: major root r <-> minor root r+9 (C major <-> A minor).
inline Triad applyRelative(Triad t) {
    if (t.isMajor) return { mod12(t.rootPc + 9), false };
    return { mod12(t.rootPc - 9), true };
}

inline int triadThirdPc(Triad t) { return mod12(t.rootPc + (t.isMajor ? 4 : 3)); }
inline int triadFifthPc(Triad t) { return mod12(t.rootPc + 7); }

inline bool triadContainsPc(Triad t, int pc) {
    pc = mod12(pc);
    return pc == mod12(t.rootPc) || pc == triadThirdPc(t) || pc == triadFifthPc(t);
}

inline std::string triadDescription(Triad t) {
    static const char *names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    return std::string(names[mod12(t.rootPc)]) + (t.isMajor ? " maior" : " menor");
}

} // namespace rasgo
