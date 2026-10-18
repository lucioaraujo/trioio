#pragma once

// Recombinação de culturas, linguagens e estilos (docs/ESCALAS.md,
// "Misturas"; RASGO_DOCUMENTATION/teoria/ESCALAS_AFETO_INTENCAO_E_FUNCAO.md,
// §6.13). Quatro técnicas, cada uma com raiz numa prática documentada:
//
//   colagem      cada seção toma uma escala de uma cultura diferente, com o
//                comportamento e a textura dela; o corte entre seções é
//                parte do som (L. Berio, Sinfonia, 1968, 3.º movimento; B. A.
//                Zimmermann, a "composição pluralista")
//   híbrido      uma escala nova: a metade de baixo de uma tradição e a de
//                cima de outra, como os gêneros gregos e os ajnas montam
//                escalas por tetracordes, agora atravessando culturas; o
//                ornamento vem da segunda (enxerto)
//   camadas      duas escalas juntas sobre o mesmo centro: a melodia numa,
//                a segunda voz e a harmonia noutra (a polimodalidade de
//                Bartók; o acorde de Petrushka de Stravinsky)
//   transição    a peça caminha de uma cultura à outra por estados
//                intermediários (interpolação da máquina de escalas), que
//                não pertencem a nenhuma das duas

#include "Composer.h"
#include "ScaleArchive.h"
#include "ScaleMachine.h"

#include <string>
#include <vector>

namespace trioio::composition
{

[[nodiscard]] bool parseMixTechnique(const std::string& name, MixTechnique& out);  // colagem hibrido camadas transicao
[[nodiscard]] const char* mixTechniqueName(MixTechnique t);

// O híbrido: os graus de `low` abaixo da quinta (até 690 c) e os de `high`
// da quinta para cima, sobre a tônica de `low`; apoio, percurso, assinatura
// e cadência de `low`, ornamento de `high`, textura de `low`.
[[nodiscard]] ArchivePick hybridPick(const ArchivePick& low, const ArchivePick& high);

// Uma escala do arquivo como MachineScale (tônica em cents).
[[nodiscard]] MachineScale machineFromPick(const ArchivePick& p);

// As camadas: as vozes 2 (segunda voz, eco) e 6 (harmonia) vão à escala
// `other`, sobre a tônica de cada seção. Devolve a linha da forma.
[[nodiscard]] std::string layerSecondScale(Score& score, const std::vector<SectionScale>& sections, const ArchivePick& other);

// As costuras entre seções (a colagem e as outras misturas):
//   corte          nada (o corte seco da colagem)
//   pivo           no último compasso antes da fronteira, a melodia fica nas
//                  notas comuns às duas escalas, e uma nota comum sustentada
//                  (voz 6) atravessa a fronteira: a modulação por nota comum
//   ponte          um compasso de cada lado da fronteira toca a escala
//                  intermediária (metade do caminho entre as duas)
//   sobreposicao   a percussão e a harmonia da seção que sai continuam um
//                  compasso dentro da que entra, mais baixas (o crossfade)
enum class Seam { cut, pivot, bridge, overlap };
[[nodiscard]] bool parseSeam(const std::string& name, Seam& out);  // corte pivo ponte sobreposicao
[[nodiscard]] const char* seamName(Seam s);

// Costura as fronteiras entre seções vizinhas. Devolve a linha da forma.
[[nodiscard]] std::string seamSections(Score& score, const std::vector<SectionScale>& sections, Seam seam, double barLength);

} // namespace trioio::composition
