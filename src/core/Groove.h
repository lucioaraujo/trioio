#pragma once

// O groove da referência (docs/AUDITORIA_2026-10-14.md §6, item 2;
// docs/RITMO.md "Groove e microtempo"): a resposta deixa de ser uma grade
// quantizada e toca com o balanço medido nos ataques da referência.
//
// Três medidas, nos ataques dentro do pulso lido:
//   - o SWING das colcheias: onde cai o contratempo dentro do tempo (reto =
//     0,5; ternário = 0,667), e o das semicolcheias dentro de cada metade;
//   - o MICROTEMPO por posição do compasso: o resíduo mediano de cada
//     semicolcheia depois do swing (a caixa atrás do tempo nos 2 e 4…),
//     descontada a deriva lenta do andamento;
//   - o ACENTO por posição: a força média dos ataques em cada semicolcheia.
//
// Fontes: o swing como razão de duração das colcheias (F. Benadon, "Slicing
// the beat: jazz eighth-notes as expressive microrhythm", Ethnomusicology
// 50(1), 2006); o microtempo sistemático por posição (A. Danielsen,
// Musical Rhythm in the Age of Digital Reproduction, Ashgate, 2010; M. Wright
// e E. Berdahl, "Towards machine learning of expressive microtiming in
// Brazilian drumming", ICMC 2006).

#include "Composer.h"
#include "Structure.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace trioio::composition
{

struct GrooveProfile
{
    int barSteps = 16;
    float swing8 = 0.0f;   // deslocamento do contratempo da colcheia, em fração do tempo (0 = reto; 0,167 = ternário)
    float swing16 = 0.0f;  // o mesmo para a semicolcheia dentro de cada metade, em fração da metade
    std::array<float, perception::kMaxStepsPerBar> offset {};  // segundos, por semicolcheia do compasso
    std::array<float, perception::kMaxStepsPerBar> accent {};  // multiplicador da força, ~0,8..1,2
    std::array<int, perception::kMaxStepsPerBar> count {};
    int onsets = 0;
    bool valid = false;  // ataques suficientes no pulso
};

// Mede o groove nos ataques entre t0 e t1, no pulso dado.
[[nodiscard]] GrooveProfile measureGroove(const std::vector<perception::Onset>& onsets, const perception::Pulse& pulse,
                                          double t0, double t1);

// Toca as notas da partitura com o groove: só as que estão na grade de
// semicolcheias (±5 % do passo) e dentro dos trechos `spans` com pulso.
// `amount` 1 = o groove inteiro, 0 = nada. Abaixo dos limiares (swing de 3 %
// do tempo, microtempo de 8 ms) nada muda: uma referência reta continua reta.
// Devolve a linha da forma (vazia se nada mudou).
std::string applyGroove(Score& score, const GrooveProfile& groove, const perception::Pulse& pulse,
                        const std::vector<std::pair<double, double>>& spans, double amount);

// O rubato nas seções de tempo livre: o arco de frase (N. P. Todd, "The
// dynamics of dynamics: a model of musical expression", JASA 91(6), 1992) —
// dentro de cada frase o tempo se apressa um pouco no meio e se alarga nas
// pontas. A frase são as notas do motivo separadas por pausas de > 0,6 s
// (no máximo, a seção livre inteira). Os extremos da frase ficam onde estão;
// todas as vozes da frase seguem a mesma deformação. `depth` 0 = nada; 1 =
// as pontas ~30 % mais lentas que o meio. Devolve a linha da forma.
std::string applyRubato(Score& score, const std::vector<std::pair<double, double>>& freeSpans, double depth);

} // namespace trioio::composition
