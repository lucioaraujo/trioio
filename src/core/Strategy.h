#pragma once

// Estratégia, primeira versão: transforma a escuta de uma referência (resumos
// por trecho) numa trajetória de BrainDecision, segundo um dos quatro modos do
// Terceiro Olho. Especificação: docs/CADEIA_MINIMA.md.
//
// Puro e determinístico: mesmos resumos + modo + seed = mesma trajetória.

#include "Descriptors.h"
#include "Provenance.h"
#include "TrioioBrain.h"

#include <cstdint>
#include <string>
#include <vector>

namespace trioio::strategy
{

using Mode = provenance::Strategy;  // follow, transfigure, counter, interrupt

struct Step
{
    double start = 0.0;
    double end = 0.0;
    BrainDecision decision;
};

[[nodiscard]] std::vector<Step> plan(const std::vector<perception::Summary>& reference, Mode mode,
                                     std::uint64_t seed);

// A relação declarada que acompanha a derivação, em palavras.
[[nodiscard]] std::string describe(Mode mode);

// Nomes curtos em português para a linha de comando: seguir, transfigurar,
// contrapor, interromper.
[[nodiscard]] bool parseMode(const std::string& name, Mode& out);
[[nodiscard]] const char* modeName(Mode mode);

} // namespace trioio::strategy
