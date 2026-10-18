#pragma once

// Corpos de instrumentos acústicos (docs/MAQUINA_DE_TIMBRES.md): modelos
// físicos escritos para o Trioio a partir da teoria. Não há amostras: a
// resposta continua sendo só síntese. Pedido do autor (2026-10-14):
// "precisamos incluir mais sons e timbres, timbres de instrumentos, talvez
// menos sintéticos".
//
//   arco     — corda friccionada: guia de onda com a curva de atrito do arco
//              (Smith 1986; McIntyre, Schumacher e Woodhouse 1983) e o corpo
//              de violino, viola ou violoncelo pelo registro (Woodhouse 2014)
//   palheta  — clarinete: tubo cilíndrico (fechado-aberto) e a palheta como
//              válvula de pressão (Smith 1986)
//   marimba  — barras afinadas 1 : 4 : 10 golpeadas por baqueta macia, com o
//              tubo ressonador; nas notas longas, vibrafone com o motor
//              (Fletcher e Rossing 1998, cap. 19)
//   piano    — três cordas rígidas (inarmonicidade), o feltro do martelo e o
//              decaimento em dois estágios (Weinreich 1977; Chaigne e
//              Askenfelt 1994)
//   violão   — Karplus-Strong estendido (Jaffe e Smith 1983) com o lugar do
//              toque e os modos do tampo (Karjalainen, Välimäki e Tolonen 1998)
//
// Todos seguem a curva de altura da nota (Note::pitchAt): glissando,
// ornamentos e vibrato.

#include "Composer.h"

#include <cstdint>
#include <vector>

namespace trioio
{

// A nota (mono, amostra 0 = início), ou vazio se não for um corpo acústico.
[[nodiscard]] std::vector<float> renderAcousticNote(const composition::Note& note, double sampleRate, std::uint64_t seed);

// Verdadeiro para as famílias de instrumento acústico (as cinco acima e, entre
// as antigas, a corda pinçada e a corda Bioma).
[[nodiscard]] bool isAcousticTimbre(composition::Timbre t);

} // namespace trioio
