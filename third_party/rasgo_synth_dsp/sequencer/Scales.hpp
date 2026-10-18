#pragma once
#include <array>

namespace rasgo {

// The initial nine-name repertoire was prompted by studying Kosmische's
// Sequence.java, but the tables here were independently written and then
// revised into a genuinely researched,
// much larger repertoire after auditing that original data turned up
// three real errors, not just style preferences:
//   - HarmonicMinor's degrees were {0,2,3,5,6,8,10} -- neither harmonic
//     nor natural minor (the 5th and 7th were both wrong; real harmonic
//     minor is {0,2,3,5,7,8,11}). This was the *default* scale, so it
//     silently affected most pieces ever rendered here.
//   - "Bebop" and "Byzantine" had identical degree tables -- a
//     copy-paste duplicate, not an actual bebop scale.
//   - "Arabian" and "Chinese" didn't match any maqam or Chinese
//     pentatonic mode traceable to a real source -- invented
//     exotic-sounding sets, not researched ones. Retired rather than
//     replaced with another guess: their sonic territory is now covered
//     honestly (Phrygian Dominant for the "exotic minor" colour, the
//     pentatonic rotations below for modal pentatonic colour -- Chinese
//     gong-diao pentatonic modes are, note-for-note, the same rotations
//     as the Western pentatonic modes already here, a difference of
//     which degree is treated as final, not of interval content, which
//     is all this enum can represent).
//
// Every mode below was derived by rotating a parent scale's own interval
// sequence (not recalled from memory alone) and cross-checked against
// standard jazz/theory naming. The "world" entries (Hungarian, Japanese)
// are kept honestly labelled as the Western pedagogical abstractions
// they are -- common reference-book degree sets, not the source
// traditions' own microtonal/modal theory, which this fixed 12-tone
// system can't represent anyway.
enum class Scale {
    MinorPentatonic,
    MajorPentatonic,
    Major,              // Ionian
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Aeolian,             // natural minor
    Locrian,
    MelodicMinor,        // ascending/jazz form
    HarmonicMinor,       // fixed: 0,2,3,5,7,8,11
    DorianB2,            // melodic minor mode 2
    LydianAugmented,     // melodic minor mode 3
    LydianDominant,      // melodic minor mode 4, aka "acoustic scale"
    MixolydianB6,        // melodic minor mode 5
    HalfDiminished,      // melodic minor mode 6, aka Locrian #2
    Altered,             // melodic minor mode 7, aka Super Locrian
    LocrianNat6,         // harmonic minor mode 2
    IonianAugmented,     // harmonic minor mode 3
    DorianSharp4,        // harmonic minor mode 4
    PhrygianDominant,    // harmonic minor mode 5 -- Freygish/Hijaz family
    LydianSharp2,        // harmonic minor mode 6
    Ultralocrian,        // harmonic minor mode 7
    WholeTone,
    DiminishedWholeHalf,
    DiminishedHalfWhole,
    AugmentedScale,
    Blues,
    Suspended,           // major pentatonic mode 2, aka "Egyptian"
    ManGong,             // major pentatonic mode 3
    Ritusen,             // major pentatonic mode 4
    Byzantine,           // double harmonic major
    Bebop,               // fixed: real bebop dominant scale
    HungarianMinor,
    HungarianMajor,
    Hirajoshi,
    InScale,             // Miyako-bushi
    Kumoi,
    Chromatic,           // all twelve -- a scale too, the limit case of one
    HarmonicMajor,       // major scale with lowered sixth
    NeapolitanMinor,     // 1 b2 b3 4 5 b6 7
    BebopMajor,          // major scale plus chromatic b7
    BebopMinor,          // Dorian colour plus chromatic major seventh
    MixolydianB9,        // dominant scale with lowered ninth
    Prometheus,           // Scriabin's synthetic hexatonic collection
    Count
};

// 12 slots, not 8 -- Chromatic needs all twelve; every shorter scale pads
// its unused trailing slots by repeating its own last real degree (same
// convention the original Kosmische data already used).
struct ScaleTable {
    std::array<int, 12> degrees;
    int length;
};

inline const ScaleTable &scaleTable(Scale scale) {
    static const ScaleTable tables[] = {
        {{0, 3, 5, 7, 10, 10, 10, 10, 10, 10, 10, 10}, 5},          // MinorPentatonic
        {{0, 2, 4, 7, 9, 9, 9, 9, 9, 9, 9, 9}, 5},                   // MajorPentatonic
        {{0, 2, 4, 5, 7, 9, 11, 11, 11, 11, 11, 11}, 7},             // Major/Ionian
        {{0, 2, 3, 5, 7, 9, 10, 10, 10, 10, 10, 10}, 7},             // Dorian
        {{0, 1, 3, 5, 7, 8, 10, 10, 10, 10, 10, 10}, 7},             // Phrygian
        {{0, 2, 4, 6, 7, 9, 11, 11, 11, 11, 11, 11}, 7},             // Lydian
        {{0, 2, 4, 5, 7, 9, 10, 10, 10, 10, 10, 10}, 7},             // Mixolydian
        {{0, 2, 3, 5, 7, 8, 10, 10, 10, 10, 10, 10}, 7},             // Aeolian
        {{0, 1, 3, 5, 6, 8, 10, 10, 10, 10, 10, 10}, 7},             // Locrian
        {{0, 2, 3, 5, 7, 9, 11, 11, 11, 11, 11, 11}, 7},             // MelodicMinor
        {{0, 2, 3, 5, 7, 8, 11, 11, 11, 11, 11, 11}, 7},             // HarmonicMinor (fixed)
        {{0, 1, 3, 5, 7, 9, 10, 10, 10, 10, 10, 10}, 7},             // DorianB2
        {{0, 2, 4, 6, 8, 9, 11, 11, 11, 11, 11, 11}, 7},             // LydianAugmented
        {{0, 2, 4, 6, 7, 9, 10, 10, 10, 10, 10, 10}, 7},             // LydianDominant
        {{0, 2, 4, 5, 7, 8, 10, 10, 10, 10, 10, 10}, 7},             // MixolydianB6
        {{0, 2, 3, 5, 6, 8, 10, 10, 10, 10, 10, 10}, 7},             // HalfDiminished
        {{0, 1, 3, 4, 6, 8, 10, 10, 10, 10, 10, 10}, 7},             // Altered
        {{0, 1, 3, 5, 6, 9, 10, 10, 10, 10, 10, 10}, 7},             // LocrianNat6
        {{0, 2, 4, 5, 8, 9, 11, 11, 11, 11, 11, 11}, 7},             // IonianAugmented
        {{0, 2, 3, 6, 7, 9, 10, 10, 10, 10, 10, 10}, 7},             // DorianSharp4
        {{0, 1, 4, 5, 7, 8, 10, 10, 10, 10, 10, 10}, 7},             // PhrygianDominant
        {{0, 3, 4, 6, 7, 9, 11, 11, 11, 11, 11, 11}, 7},             // LydianSharp2
        {{0, 1, 3, 4, 6, 8, 9, 9, 9, 9, 9, 9}, 7},                   // Ultralocrian
        {{0, 2, 4, 6, 8, 10, 10, 10, 10, 10, 10, 10}, 6},            // WholeTone
        {{0, 2, 3, 5, 6, 8, 9, 11, 11, 11, 11, 11}, 8},              // DiminishedWholeHalf
        {{0, 1, 3, 4, 6, 7, 9, 10, 10, 10, 10, 10}, 8},              // DiminishedHalfWhole
        {{0, 3, 4, 7, 8, 11, 11, 11, 11, 11, 11, 11}, 6},            // AugmentedScale
        {{0, 3, 5, 6, 7, 10, 10, 10, 10, 10, 10, 10}, 6},            // Blues
        {{0, 2, 5, 7, 10, 10, 10, 10, 10, 10, 10, 10}, 5},           // Suspended
        {{0, 3, 5, 8, 10, 10, 10, 10, 10, 10, 10, 10}, 5},           // ManGong
        {{0, 2, 5, 7, 9, 9, 9, 9, 9, 9, 9, 9}, 5},                   // Ritusen
        {{0, 1, 4, 5, 7, 8, 11, 11, 11, 11, 11, 11}, 7},             // Byzantine
        {{0, 2, 4, 5, 7, 9, 10, 11, 11, 11, 11, 11}, 8},             // Bebop (dominant, fixed)
        {{0, 2, 3, 6, 7, 8, 11, 11, 11, 11, 11, 11}, 7},             // HungarianMinor
        {{0, 3, 4, 6, 7, 9, 10, 10, 10, 10, 10, 10}, 7},             // HungarianMajor
        {{0, 2, 3, 7, 8, 8, 8, 8, 8, 8, 8, 8}, 5},                   // Hirajoshi
        {{0, 1, 5, 7, 8, 8, 8, 8, 8, 8, 8, 8}, 5},                   // InScale
        {{0, 2, 3, 7, 9, 9, 9, 9, 9, 9, 9, 9}, 5},                   // Kumoi
        {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, 12},                // Chromatic
        {{0, 2, 4, 5, 7, 8, 11, 11, 11, 11, 11, 11}, 7},             // Harmonic Major
        {{0, 1, 3, 5, 7, 8, 11, 11, 11, 11, 11, 11}, 7},             // Neapolitan Minor
        {{0, 2, 4, 5, 7, 8, 9, 11, 11, 11, 11, 11}, 8},             // Bebop Major
        {{0, 2, 3, 5, 7, 9, 10, 11, 11, 11, 11, 11}, 8},             // Bebop Minor
        {{0, 1, 4, 5, 7, 9, 10, 10, 10, 10, 10, 10}, 7},             // Mixolydian b9
        {{0, 2, 4, 6, 9, 10, 10, 10, 10, 10, 10, 10}, 6},             // Prometheus
    };
    return tables[static_cast<int>(scale)];
}

inline const char *scaleName(Scale scale) {
    static const char *names[] = {
        "Minor Pentatonic", "Major Pentatonic", "Major", "Dorian", "Phrygian",
        "Lydian", "Mixolydian", "Aeolian", "Locrian", "Melodic Minor",
        "Harmonic Minor", "Dorian b2", "Lydian Augmented", "Lydian Dominant",
        "Mixolydian b6", "Half-Diminished", "Altered", "Locrian nat6",
        "Ionian Augmented", "Dorian #4", "Phrygian Dominant", "Lydian #2",
        "Ultralocrian", "Whole Tone", "Diminished (W-H)", "Diminished (H-W)",
        "Augmented", "Blues", "Suspended", "Man Gong", "Ritusen", "Byzantine",
        "Bebop Dominant", "Hungarian Minor", "Hungarian Major", "Hirajoshi",
        "In", "Kumoi", "Chromatic", "Harmonic Major", "Neapolitan Minor",
        "Bebop Major", "Bebop Minor", "Mixolydian b9", "Prometheus"
    };
    return names[static_cast<int>(scale)];
}

} // namespace rasgo
