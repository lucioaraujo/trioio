#pragma once

// A voz CMOS do Antitotem como família de timbre do Trioio. DSP de uma cópia
// fixada (third_party/antitotem_dsp, AGPL-3.0, mesma autoria). Ver
// docs/SINTESE.md.

#include "Composer.h"

#include <cstdint>
#include <vector>

namespace trioio
{

// A nota (mono, amostra 0 = início), ou vazio se não for a família CMOS.
[[nodiscard]] std::vector<float> renderAntitotemNote(const composition::Note& note, double sampleRate,
                                                     std::uint64_t seed);

} // namespace trioio
