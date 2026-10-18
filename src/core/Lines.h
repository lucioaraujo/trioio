#pragma once

// As linhas por região de frequência na composição (docs/LINHAS.md).
//
// - o que VALE na leitura de cada linha (as regras medidas na etapa 1);
// - o empréstimo: onde as alturas de uma linha não valem (o agudo de uma
//   canção, os acordes do grave), a linha compõe com as alturas da peça
//   inteira e guarda o próprio ritmo, energia e brilho;
// - o registro: as notas de cada linha são dobradas em oitavas para a região
//   dela (o grave responde no grave, o agudo no agudo).

#include "Composer.h"
#include "Reading.h"

#include <vector>

namespace trioio::composition
{

struct LineValidity
{
    bool pulse = false;    // confiança ≥ 0,25 e ≥ 0,5 ataque/s
    bool agrees = false;   // o pulso no nível da peça (ou metade/dobro), mesmo fraco
    bool melody = false;   // cobre parte de 400–2000 Hz e ≥ 25 % dos quadros com altura
    bool chords = false;   // cobre parte de 300–1500 Hz
    bool bass = false;     // abaixo de 300 Hz: o croma lê as fundamentais
};

// `lowHz` 0 = desde o começo; `highHz` 0 = até Nyquist.
[[nodiscard]] LineValidity assessLine(double lowHz, double highHz, double sampleRate, const Reading& line,
                                      const Reading& whole, double onsetsPerSecond);

// Onde as alturas da linha não valem, os quadros dela recebem as da peça
// inteira (croma, altura predominante e candidatos, YIN, picos); no grave, o
// croma do grave (as fundamentais) fica o da linha. Os quadros são alinhados
// pelo índice (a mesma análise, o mesmo passo). Devolve o que foi emprestado,
// em palavras ("nada" quando nada).
std::string borrowPitch(std::vector<perception::Frame>& lineFrames, const std::vector<perception::Frame>& whole,
                        const LineValidity& v);

// O registro (MIDI) das fundamentais de uma linha: a banda em notas, com ao
// menos duas oitavas (o agudo desce para o registro cujos parciais caem na
// banda).
void bandRegister(double lowHz, double highHz, float& lowMidi, float& highMidi);

// Dobra as notas (menos a percussão, voz 4) em oitavas para o registro.
void foldIntoRegister(Score& score, float lowMidi, float highMidi);

} // namespace trioio::composition
