#pragma once

// Corpos do Aquorbium (ressonador modal, corda física, striker granular)
// como famílias de timbre do Trioio. DSP de uma cópia fixada
// (third_party/aquorbium_dsp, AGPL-3.0, mesma autoria). Ver docs/SINTESE.md.

#include "Composer.h"

#include <cstdint>
#include <vector>

namespace trioio
{

// A nota (mono, amostra 0 = início), ou vazio se não for família do Aquorbium.
[[nodiscard]] std::vector<float> renderAquorbiumNote(const composition::Note& note, double sampleRate,
                                                     std::uint64_t seed);

} // namespace trioio
