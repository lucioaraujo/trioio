#pragma once

// Afeto e intenção das escalas (RASGO_DOCUMENTATION/teoria/
// ESCALAS_AFETO_INTENCAO_E_FUNCAO.md, §4–5; TRIOIO/docs/ESCALAS.md).
//
// Propriedades ESTRUTURAIS de qualquer escala (em cents, inclusive
// microtonal), que a pesquisa liga a afetos. São tendências declaradas, não
// verdades: a afetividade vem da peça inteira, e muito dela é aprendido
// (Temperley e Tan 2013: familiaridade). A escuta do autor corrige o mapa.
//
//   brilho          média dos graus menos a de uma escala igual de mesmo
//                   tamanho (semitons): graus mais altos, mais "alegre"
//                   (Temperley e Tan 2013)
//   terça           +1 maior, −1 menor, 0 neutra ou ausente (Hevner 1936)
//   tensão          semitons vizinhos, trítonos, b2, trítono sobre a tônica,
//                   quinta ausente (Huron 2006; Lerdahl 2001), 0..1
//   estabilidade    quinta, quarta e terça sobre a tônica, 0..1
//   ambiguidade     parte dos graus a mais de 30 c do temperamento igual
//   familiaridade   notas comuns com o jônio na mesma tônica, 0..1
//   valência        0,4·brilho + 0,3·terça − 0,3·tensão + 0,4·familiaridade
//                   (heurística; o teste confere que ordena os modos como
//                   Temperley e Tan mediram)

#include "ScaleMachine.h"

#include <string>

namespace trioio::composition
{

struct ScaleAffect
{
    double brightness = 0.0;
    int third = 0;
    double tension = 0.0;
    double stability = 0.0;
    double ambiguity = 0.0;
    double familiarity = 0.0;
    double valence = 0.0;
    [[nodiscard]] std::string describe() const;  // "brilho +0,33 · terça maior · tensão 0,23 · valência +0,77"
};

[[nodiscard]] ScaleAffect scaleAffect(const MachineScale& s);

// A intenção do autor: um alvo no espaço afetivo.
enum class Intention { none, luminous, somber, tense, serene, suspended, ambiguous, arc };
[[nodiscard]] bool parseIntention(const std::string& name, Intention& out);  // luminosa sombria tensa serena suspensa ambigua arco
[[nodiscard]] const char* intentionName(Intention i);

// Quão longe a escala está da intenção (0 = no alvo). `position` (0..1) é o
// ponto da peça: no arco, a tensão pedida sobe até 2/3 da peça e resolve.
[[nodiscard]] double intentionDistance(const ScaleAffect& a, Intention i, double position);

} // namespace trioio::composition
