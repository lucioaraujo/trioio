#pragma once

// O rasgo modular (docs/MAQUINA_DE_TIMBRES.md §5.0). A escuta do autor
// (2026-10-15), depois de ouvir os timbres e os estilos: "ainda muito midi —
// se possível trazer mais da síntese modular criativa para dentro dos
// timbres, sons inauditos, não protocolares". O rumo deixa de ser o
// realismo: o corpo acústico é o PONTO DE PARTIDA, e o Trioio o atravessa
// com processos de síntese modular — cada voz, em cada seção, recebe um
// "patch", com a profundidade movida pela tensão da peça:
//   dobra       — wavefolder movido pelo envelope do próprio som (o West
//                 Coast: Buchla 259; o SHAPE do Rasgo Modular)
//   anel        — modulação em anel com uma razão INARMÔNICA, tirada do
//                 corpo espectral da referência quando há
//   deslocamento — o deslocador de frequência (Bode): todo o espectro anda
//                 alguns Hz, os harmônicos deixam de ser harmônicos
//   portão caótico — o lowpass gate (vactrol) re-disparado por um registrador
//                 caótico (o Rungler de Rob Hordijk): o sustentado vira
//                 pulsado e irregular
//   gagueira    — a nota recortada em grãos e embaralhada no tempo
//   simpática   — um pente com realimentação afinado fora da série: o
//                 instrumento ressoa uma corda que não existe
// E as QUIMERAS (AcousticVoice.h): o excitador de um instrumento no corpo de
// outro — o prato friccionado, a corda cantada, a pele soprada.
// Código próprio; os processos têm a genealogia dos módulos do Rasgo Modular
// e do Rasgo Synth (SHAPE, LPG, Rungler), sem cópia.

#include "Composer.h"
#include "Dramaturgy.h"

#include <cstdint>
#include <string>
#include <vector>

namespace trioio::composition
{

enum class Patch : std::uint8_t { none, fold, ring, shift, chaosGate, stutter, sympathetic };
inline constexpr int kPatches = 7;
[[nodiscard]] const char* patchName(Patch p);

// Aplica o patch da nota (Note::patch, patchDepth, patchRatio) ao som mono.
void applyPatch(std::vector<float>& mono, const Note& n, double sampleRate, std::uint64_t seed);

// Distribui os patches e as quimeras pela peça: `amount` 0 = nada (o
// instrumento), 1 = o inaudito. Por voz e por seção; a profundidade cresce
// com a tensão. Devolve a linha da forma.
std::string applyModularRasgo(Score& score, float amount, const TensionCurve& curve, const std::vector<double>& sectionStarts,
                              std::uint64_t seed);

} // namespace trioio::composition
