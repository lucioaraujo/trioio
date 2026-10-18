#pragma once

// Corpos de voz do Rasgo Modular (AGPL-3.0, mesma autoria e licença do
// Trioio), a partir de uma CÓPIA fixada em third_party/rasgo_modular_dsp
// (commit e checksums em PROVENANCE.md): o Trioio é autônomo. Cada família de timbre hospedada é uma
// pequena cadeia de módulos reais do Rasgo Modular — fonte e, às vezes, um
// processador —, tocada nota a nota, offline. O Trioio é o SEGUNDO
// consumidor desse DSP (regra do inventário global: segundo consumidor antes
// de promover a código comum). Especificação: docs/SINTESE.md.
//
// TRIOIO_HAS_MODULAR vem sempre definido pelo CMake; sem ele (outro build),
// nenhuma família é hospedada e o ScoreSynth toca o aditivo no lugar.

#include "Composer.h"

#include <cstdint>
#include <vector>

namespace trioio
{

// true quando o build inclui os módulos do Rasgo Modular
[[nodiscard]] bool modularAvailable() noexcept;

// true se a família é tocada por módulos do Rasgo Modular
[[nodiscard]] bool isModularTimbre(composition::Timbre t) noexcept;

// A nota renderizada (mono, amostra 0 = início da nota), ou vazio se a
// família não é hospedada ou o build não tem o Rasgo Modular. O nível não é
// normalizado: quem chama ajusta. Determinístico pela `seed`.
[[nodiscard]] std::vector<float> renderModularNote(const composition::Note& note, double sampleRate,
                                                   std::uint64_t seed);

// A sala da peça com os módulos HALL ou SPACE do Rasgo Modular (envio do
// meio do estéreo, retorno somado). `false` para Room::trioio (a sala
// própria fica com o ScoreSynth) ou sem o Rasgo Modular no build.
bool applyModularRoom(composition::Room room, double sampleRate, std::vector<float>& left,
                      std::vector<float>& right, std::uint64_t seed);

} // namespace trioio
