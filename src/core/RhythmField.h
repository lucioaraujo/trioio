#pragma once

// Ritmo como campo (Atlas de Referência RASGO §45, Grids: "pensar ritmo não
// como uma sequência fixa de passos, mas como navegação em um campo de
// padrões e densidades": campo → posição → densidade → probabilidade →
// evento).
//
// O campo é feito da própria referência: cada compasso dela vira um ponto
// num mapa de duas dimensões, densidade (quantos ataques) e síncope (quantos
// fora do tempo), levando junto o seu padrão de ataques. A resposta caminha
// por esse mapa, compasso a compasso; a cada posição, os padrões vizinhos se
// misturam em probabilidades por passo, que viram golpes em duas faixas:
// grave (tambor) e aguda (striker, LPG ou corda). Inspirado no
// comportamento do Mutable Grids (Émilie Gillet, MIT), sem código dele: no
// Grids o mapa é fixo e treinado em baterias; aqui ele é a referência.
// Especificação: docs/RITMO.md.

#include "Composer.h"

#include <utility>
#include <vector>

namespace trioio::composition
{

// Acrescenta a camada rítmica (voice 4) nos trechos com pulso. `pulsed` são
// os trechos (início, fim) em que a composição usa o pulso; sem compassos da
// referência ou sem trechos com pulso, nada muda.
void rhythmField(Score& score, const std::vector<perception::Bar>& referenceBars, const perception::Pulse& pulse,
                 const std::vector<std::pair<double, double>>& pulsed, const Options& options);

} // namespace trioio::composition
