#pragma once

// O Terceiro Olho: o agente que percebe a estrutura da peça e a transforma.
// Age DEPOIS do compositor, sobre a partitura pronta: em momentos sorteados
// (mais prováveis onde a referência muda mais) faz irromper objetos sonoros
// ou impõe regras ocultas por um tempo. Especificação: docs/COMPOSICAO.md
// §"Terceiro Olho".
//
// Imprevisível, mas reprodutível: o mesmo seed refaz as mesmas surpresas.
// Com unpredictability = 0 não faz nada.

#include "Composer.h"

namespace trioio::composition
{

void thirdEye(Score& score, const std::vector<perception::Frame>& frames,
              const std::vector<perception::Onset>& onsets, const Options& options);

[[nodiscard]] const char* gestureName(GestureKind kind);

// O nome de cada uma das 17 escolhas do olho (0..7 objetos, 8..16 regras).
[[nodiscard]] const char* eyeChoiceName(int choice);

} // namespace trioio::composition
