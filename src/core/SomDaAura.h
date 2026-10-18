#pragma once

// O "som da aura" (2026-10-17; docs/MAQUINA_DE_HARMONIA.md §7). Hermeto
// Pascoal chama assim a fala ouvida como melodia: ele transcreve a entonação
// de quem fala, sílaba por sílaba, dobra essa melodia com os instrumentos e
// harmoniza cada nota ("a vibração sonora da alma de cada pessoa, refletida
// na sua fala"; L. Costa-Lima Neto, "The first 'Sound of the Aura' of
// Hermeto Pascoal (1984): hearing the spoken voice as a sung melody", IASPM
// Journal; o disco Lagoa da Canoa, Município de Arapiraca, 1984).
//
// No Trioio: cada ataque da referência com voz vira uma nota (a altura
// mediana do trecho, arredondada ao semitom; a força pelo nível), que
// substitui o motivo e é dobrada em uníssono por outra voz; cada nota ganha o
// seu acorde, em que ela é uma cor (a 9ª, a ♯11, a 13ª, a 7M ou a 5ª), com a
// fundamental escolhida para o baixo andar perto (o semitom, a quarta). A
// referência não entra no som: a fala é ressintetizada.

#include "Composer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace trioio::composition
{

std::string applySomDaAura(Score& score, const std::vector<perception::Frame>& frames,
                           const std::vector<perception::Onset>& onsets, double duration, std::uint64_t seed);

} // namespace trioio::composition
