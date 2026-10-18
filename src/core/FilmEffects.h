#pragma once

// Os efeitos de cinema na forma (docs/MAQUINA_DE_TIMBRES.md §3.7). Pedido do
// autor (2026-10-14): "sons de efeitos sonoros como de filmes". O Trioio não
// os espalha ao acaso: eles marcam a dramaturgia da resposta, como numa
// trilha —
//   - o riser sobe nos segundos antes do clímax, e o impacto e o braam caem
//     no clímax;
//   - o whoosh passa nas mudanças de seção (no máximo um a cada 20 s);
//   - nos trechos rarefeitos (tensão baixa por 8 s ou mais), uma ambiência
//     pelo brilho da referência: escura → vento ou fogo; média → água ou
//     cidade; clara → chuva (às vezes com um trovão) ou multidão.
// Os efeitos são gestos (objetos sonoros), sintetizados por processo em
// ScoreSynth.cpp (Farnell, Designing Sound, 2010).

#include "Composer.h"
#include "Dramaturgy.h"

#include <string>
#include <vector>

namespace trioio::composition
{

// `sectionStarts`: o começo de cada seção; `brightness`: o brilho médio da
// referência (0..1). Devolve a linha da forma.
std::string applyFilmEffects(Score& score, const TensionCurve& curve, const std::vector<double>& sectionStarts,
                             float brightness, std::uint64_t seed);

} // namespace trioio::composition
