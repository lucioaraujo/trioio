#pragma once

// A escala tocada como a linguagem a usa (RASGO_DOCUMENTATION/teoria/
// ESCALAS_AFETO_INTENCAO_E_FUNCAO.md, §6; TRIOIO/docs/ESCALAS.md,
// "Comportamento").
//
// Quando a escala do arquivo traz comportamento (ScaleBehavior), a melodia
// (voz 0) passa a segui-lo, seção a seção:
//   subida/descida   cada nota vai à forma da escala na direção em que a
//                    melodia anda (āroha/avaroha; a descida do Rast com Sib)
//   percurso         o registro de cada seção caminha pelas regiões (sayr,
//                    seyir, ālāp): da tônica à nota de apoio, ao agudo, e de
//                    volta
//   apoio            os ataques fortes vão às notas de apoio (vādī/saṃvādī,
//                    ghammaz, tons do pathet)
//   assinatura       frases que identificam a escala abrem a seção e o meio
//                    dela (pakaḍ, cengkok), com o ritmo do motivo
//   cadência         as últimas notas da seção fazem a fórmula de fim
//   ornamento        meend (deslizar até a nota seguinte), andolan
//                    (oscilação lenta e larga), gamaka (oscilação rápida),
//                    blue (a nota chega de baixo), pela curva de altura da
//                    nota (Note::pitchAt)
// e, com textura "bordão", a harmonia em movimento dá lugar a um bordão de
// tônica e quinta, e o baixo vira pedal (a prática da rāga e do maqam).

#include "Composer.h"
#include "ScaleArchive.h"
#include "ScaleMachine.h"

#include <string>
#include <vector>

namespace trioio::composition
{

// Aplica o comportamento à voz 0 e devolve a linha da forma. Um
// comportamento por seção (nullptr = nenhum): a colagem
// (Recombination.h) dá a cada seção a linguagem de uma cultura.
[[nodiscard]] std::string applyScaleIdiom(Score& score, const std::vector<SectionScale>& sections,
                                          const std::vector<const ScaleBehavior*>& perSection, std::uint64_t seed);

// A heterofonia: a mesma melodia numa segunda voz (2), um pouco depois, com
// variações próprias (apojaturas, oitavas, omissões); sem acordes nem baixo
// (o takht árabe: A. J. Racy, Making Music in the Arab World, 2003; o
// gamelão: a elaboração em camadas).
// Só nos trechos de `sections` (as outras partes da peça ficam como estão).
// `keepOthers`: acrescenta a segunda voz sem tirar baixo, eco e acordes
// (as camadas da mistura usam assim, sobre um bordão ou acordes).
[[nodiscard]] std::string heterophonyLayer(Score& score, const std::vector<SectionScale>& sections, std::uint64_t seed,
                                           bool keepOthers = false);

// O vamp (prática modal): dois acordes do modo — tríades dos graus de
// `vamp` — alternando a cada `barLength` segundos sobre um pedal na tônica
// (voz 6 e baixo), sem a função tonal (Persichetti 1961; Levine 1995).
// Tira os acordes e o baixo que houver nos trechos. Devolve a linha da forma.
[[nodiscard]] std::string vampLayer(Score& score, const std::vector<SectionScale>& sections,
                                    const std::vector<const ScaleBehavior*>& behaviors, double barLength,
                                    const Options& options);

// O bordão: tônica e quinta sustentadas por seção (voz 6), baixo em pedal;
// tira os acordes (voz 6) que houver nos trechos de `sections`.
[[nodiscard]] std::string droneLayer(Score& score, const std::vector<SectionScale>& sections, const Options& options);

} // namespace trioio::composition
