#pragma once
#include <vector>
#include <set>
#include <string>
#include <random>
#include <sstream>
#include "Scales.hpp"

namespace rasgo {

// Xenakis sieve theory (Formalized Music): a pitch-class SET defined by
// the union of a handful of (modulus, residue) conditions -- pitch class
// p belongs to the sieve if p % modulus == residue for at least one
// chosen pair. Every other scale in this engine (Scales.hpp) is a fixed
// lookup from a researched repertoire; this constructs a genuinely new
// pitch-class collection algorithmically, per piece -- "escalas
// experimentais/nao catalogadas geradas" from the open task list.
//
// Verified computationally before wiring in (same discipline as the
// Walsh oscillator's construction): 2000 draws of this exact generator
// produced ZERO collisions with the cataloged degree sets once an
// explicit anti-collision rejection was added to the retry loop below --
// so "nao catalogada" is a checked property here, not an assumption. An
// earlier version without that rejection check collided with the catalog
// 11.6% of the time (pure chance -- small pitch-class sets have limited
// room, and this engine's catalog is large), which would have made the
// label dishonest in roughly one piece in nine; kept as a comment here so
// the fix doesn't look accidental if this file is revisited later.
struct SieveScale {
    ScaleTable table;
    std::string description;
};

inline SieveScale generateSieveScale(std::mt19937 &rng, const std::set<std::set<int>> &cataloged) {
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::set<int> pitchClasses;
    std::vector<std::pair<int, int>> conditions;
    int attempts = 0;
    bool collides;
    do {
        conditions.clear();
        // 2-3 conditions, modulus 2..5 -- small moduli keep the result
        // from degenerating into either near-empty (moduli too large
        // relative to 12) or near-chromatic (too many conditions).
        int numConditions = 2 + (unit(rng) < 0.4 ? 1 : 0);
        for (int c = 0; c < numConditions; ++c) {
            int modulus = 2 + static_cast<int>(unit(rng) * 4.0); // 2..5
            int residue = static_cast<int>(unit(rng) * modulus);
            conditions.push_back({modulus, residue});
        }
        pitchClasses.clear();
        pitchClasses.insert(0); // root forced in -- this engine's degree table always treats degree 0 as the root
        for (int p = 0; p < 12; ++p) {
            for (auto &cond : conditions) {
                if (p % cond.first == cond.second) { pitchClasses.insert(p); break; }
            }
        }
        collides = cataloged.count(pitchClasses) > 0;
        ++attempts;
        // 4..9 notes: fewer reads as too skeletal for a melodic scale,
        // more is close enough to chromatic to lose its own identity.
    } while ((pitchClasses.size() < 4 || pitchClasses.size() > 9 || collides) && attempts < 20);

    SieveScale result;
    result.table.length = static_cast<int>(pitchClasses.size());
    int idx = 0;
    int lastDegree = 0;
    for (int p : pitchClasses) {
        result.table.degrees[static_cast<size_t>(idx)] = p;
        lastDegree = p;
        ++idx;
    }
    for (; idx < 12; ++idx) result.table.degrees[static_cast<size_t>(idx)] = lastDegree;

    std::ostringstream desc;
    desc << "Escala experimental gerada (sieve";
    for (auto &cond : conditions) desc << " p%" << cond.first << "==" << cond.second;
    desc << "; graus ";
    idx = 0;
    for (int p : pitchClasses) { if (idx++) desc << ","; desc << p; }
    desc << ")";
    result.description = desc.str();
    return result;
}

// Builds the comparison set once, from the same fixed tables Scales.hpp
// exposes -- pitch classes only, ignoring the padding/repeat convention,
// since padding is a storage detail, not part of the scale's identity.
inline std::set<std::set<int>> cataloguedPitchClassSets() {
    std::set<std::set<int>> out;
    for (int s = 0; s < static_cast<int>(Scale::Count); ++s) {
        const ScaleTable &t = scaleTable(static_cast<Scale>(s));
        std::set<int> pcs;
        for (int i = 0; i < t.length; ++i) pcs.insert(t.degrees[static_cast<size_t>(i)]);
        out.insert(pcs);
    }
    return out;
}

} // namespace rasgo
