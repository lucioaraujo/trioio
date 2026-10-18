#pragma once

// Vozes do Rasgo Synth como famílias de timbre do Trioio. O DSP vem de uma
// cópia fixada (third_party/rasgo_synth_dsp, commit 0dc468a, GPL-3.0, mesma
// autoria), não da pasta viva do Rasgo Synth. Especificação: docs/SINTESE.md.

#include "Composer.h"

#include <cstdint>
#include <vector>

namespace trioio
{

[[nodiscard]] bool isRasgoSynthTimbre(composition::Timbre t) noexcept;

// A nota renderizada (mono, amostra 0 = início da nota), ou vazio se a
// família não é do Rasgo Synth. Nível não normalizado. Determinístico.
[[nodiscard]] std::vector<float> renderRasgoSynthNote(const composition::Note& note, double sampleRate,
                                                      std::uint64_t seed);

} // namespace trioio
