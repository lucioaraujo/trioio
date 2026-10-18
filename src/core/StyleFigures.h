#pragma once

// As figuras de cada estilo (docs/MAQUINA_DE_TIMBRES.md §4.2): o conjunto
// troca os corpos; as figuras trocam o JEITO de tocar. A harmonia (voz 6) e o
// baixo (voz 1) que o Trioio compôs — os acordes sustentados e as
// fundamentais — são rearticulados no compasso e no pulso lidos na
// referência, só nos trechos com pulso:
//
//   manouche — a pompe na guitarra (os tempos 1 e 3 para baixo, 2 e 4 curtos
//              e abafados) e o baixo em dois
//   jazz     — o comping de Charleston (variações por compasso) com swing de
//              tercina, e o walking bass: fundamental, notas do acorde e a
//              aproximação cromática da próxima fundamental
//   samba    — a batida do cavaquinho em semicolcheias e o baixo em dois
//              (o segundo tempo pesado, como o surdo), com a baixaria do 7
//              cordas: a corrida que leva à próxima fundamental
//   choro    — a batida do cavaquinho e a baixaria do 7 cordas em toda
//              mudança de acorde com espaço
//   bolero   — a batida do bolero e o baixo na fundamental e na quinta
//   charanga — o montuno do piano em dois compassos e o tumbao (o "2 e" que
//              antecipa e o 4)
//
// Câmara e orquestra não têm figura: a harmonia continua sustentada.
// São aproximações de partida, escritas a partir da prática descrita na
// literatura (ver o documento); a escuta do autor afina.

#include "Composer.h"
#include "Ensemble.h"
#include "Structure.h"

#include <string>
#include <utility>
#include <vector>

namespace trioio::composition
{

// Rearticula as vozes 6 e 1 pelas figuras do estilo; devolve a linha da forma
// (vazia se o estilo não tem figura ou não há pulso).
// `rockStyle` (RockStyle): o subestilo, quando o conjunto é o rock.
std::string applyStyleFigures(Score& score, Ensemble e, const perception::Pulse& pulse,
                              const std::vector<std::pair<double, double>>& pulsedSpans, int rockStyle = 0);

// O fraseado do estilo no motivo (voz 0) e no contracanto (voz 2): a melodia
// que o Trioio compôs, tocada com a articulação de cada estilo —
//   jazz, manouche — swing de tercina nas colcheias do contratempo, a
//                    aproximação cromática (uma nota de passagem meio tom
//                    abaixo) antes das notas longas, a "caída" no fim da frase
//   choro          — apojaturas (a nota de cima, rápida) e as corridas em
//                    semicolcheias que preenchem os saltos de quarta ou mais
//   samba, escola, forró, salsa, charanga — a síncope: a nota do tempo
//                    forte antecipada em uma semicolcheia
//   bolero, a cappella, câmara, orquestra — legato: o portamento que liga
//                    as notas e o vibrato nas longas
//   tango          — o marcato (as notas dos tempos fortes curtas e
//                    acentuadas) e o arrastre (a entrada por baixo, rápida)
//   rock           — os bends (a nota longa que sobe de um tom abaixo) e o
//                    vibrato largo
// O ritmo (swing, síncope, corridas) só nos trechos com pulso; a expressão
// (vibrato, portamento, bends) em toda a peça. Devolve a linha da forma.
// A cor dos acordes do estilo: a tríade vira o acorde do idioma (jazz,
// manouche, choro, samba, escola, bolero, tango, salsa, charanga, forró),
// na faixa do instrumento que toca a harmonia. `nextRootPc` (−1 = nenhum)
// reconhece a dominante (V → I).
[[nodiscard]] std::vector<float> styleVoicing(Ensemble e, const std::vector<float>& triad, int nextRootPc);

std::string applyStylePhrasing(Score& score, Ensemble e, const perception::Pulse& pulse,
                               const std::vector<std::pair<double, double>>& pulsedSpans, std::uint64_t seed, int rockStyle = 0);

} // namespace trioio::composition
