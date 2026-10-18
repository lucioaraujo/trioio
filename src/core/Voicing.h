#pragma once

// Voicings: como empilhar o acorde (docs/HARMONIA.md, "Voicings").
// Reescrito no Trioio a partir dos princípios do MelodyVoicingBank do Rasgo
// Synth Studio (o "Eixo B" da flauta; RASGO_SYNTH/rasgo-synth-core/src/dsp/
// MelodyVoicingBank.hpp, commit babe6e8), estendidos para respeitar a escala
// da seção (inclusive microtonal):
//
//   terças     a tríade do acorde, com condução parcimoniosa (o padrão)
//   quartal    quartas empilhadas (0 5 10), sem terça: ambíguo entre maior
//              e menor (McCoy Tyner)
//   sowhat     três quartas e uma terça maior em cima (0 5 10 15 19): o
//              acorde de "So What" (Bill Evans e Miles Davis, Kind of Blue,
//              1959)
//   planing    a forma do primeiro acorde movida em paralelo, sem condução
//              e sem função (Debussy, "Voiles", 1909)
//   bitonal    duas tríades a um trítono (0 4 7 + 6 10 13): duas tônicas
//              ao mesmo tempo (Stravinsky, Petrushka, 1911; Milhaud)
//   aberto     quintas abertas (0 7 14), sem terça: o bordão com a quinta,
//              como o dam do duduk armênio sob a melodia
//   cluster    três graus vizinhos da escala (H. Cowell, New Musical
//              Resources, 1930; Ligeti)
//
// Exceto em planing e bitonal, cada nota é ajustada ao grau da escala mais
// próximo quando está a até 60 cents dele (escalas microtonais); uma nota
// fora da escala por um semitom fica: é cor do voicing.

#include "ScaleMachine.h"

#include <string>
#include <vector>

namespace trioio::composition
{

enum class Voicing { thirds, quartal, soWhat, planing, bitonal, open, cluster };

[[nodiscard]] bool parseVoicing(const std::string& name, Voicing& out);  // tercas quartal sowhat planing bitonal aberto cluster
[[nodiscard]] const char* voicingName(Voicing v);

// As alturas do acorde (semitons, com fração; registro já escolhido), a
// partir da fundamental `rootPitch` (MIDI, com fração). `chordPcs`: as
// classes do acorde (a fundamental primeiro), para terças e planing;
// `scale`: a escala da seção (nullptr = sem ajuste); `shape`: a forma do
// primeiro acorde (planing; vazio = esta é a primeira, devolvida em `shape`).
[[nodiscard]] std::vector<float> voiceChord(Voicing v, float rootPitch, const std::vector<float>& chordPcs,
                                            const MachineScale* scale, std::vector<float>& shape);

} // namespace trioio::composition
