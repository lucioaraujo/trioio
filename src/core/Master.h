#pragma once

// O barramento de master da resposta (docs/AUDITORIA_2026-10-14.md §6, item
// 1; docs/SINTESE.md "Master"): o que faltava entre a soma das vozes e o WAV.
//
//   passa-altas 30 Hz (linear) → compressão de cola (2:1, joelho suave)
//   → ganho para o alvo de loudness (ITU-R BS.1770: ponderação K, blocos de
//   400 ms, portas absoluta −70 LUFS e relativa −10 LU) → limitador de pico
//   verdadeiro (4× sobreamostrado, antecipação de 5 ms)
//
// Toda a dinâmica é UMA curva de ganho tirada da mistura. Aplicada também aos
// stems (as linhas por frequência), a soma dos stems continua sendo a resposta.
// Offline, o arquivo inteiro na memória.

#include <utility>
#include <vector>

namespace trioio::perception
{

struct MasterSettings
{
    double highPassHz = 30.0;
    // a cola começa a tantos dB ACIMA da loudness da própria peça: só os
    // trechos mais fortes que a média são apertados (um limiar fixo, −20 dBFS,
    // comprimia 5,5 dB uma resposta que já estava em −16 LUFS)
    double compAboveLoudnessDb = 3.0;
    double compRatio = 2.0;
    double compKneeDb = 6.0;
    double compAttackMs = 20.0, compReleaseMs = 250.0;
    double targetLufs = -16.0;
    double ceilingDbtp = -1.0;
    double limiterLookaheadMs = 5.0, limiterReleaseMs = 80.0;
    double maxGainDb = 18.0;          // o alvo não sobe uma peça quase muda além disto
};

struct MasterReport
{
    double lufsBefore = -120.0, lufsAfter = -120.0;
    double truePeakBefore = -120.0, truePeakAfter = -120.0;  // dBTP
    double compressionMaxDb = 0.0;   // a maior redução da cola
    double compThresholdDb = 0.0;    // o limiar usado (potência média, dBFS)
    double limiterMaxDb = 0.0;       // a maior redução do limitador
    double loudnessGainDb = 0.0;
};

// Loudness integrada (LUFS) de um estéreo, BS.1770-4.
[[nodiscard]] double integratedLoudness(const std::vector<float>& left, const std::vector<float>& right, double sampleRate);

// Pico verdadeiro (dBTP): o máximo em 4× sobreamostragem.
[[nodiscard]] double truePeakDb(const std::vector<float>& left, const std::vector<float>& right, double sampleRate);

// Aplica o master à mistura (left/right) e, com o mesmo filtro e a mesma
// curva de ganho, a cada stem (pares esquerda/direita). Devolve o relatório.
MasterReport applyMaster(std::vector<float>& left, std::vector<float>& right, double sampleRate,
                         std::vector<std::pair<std::vector<float>*, std::vector<float>*>> stems = {},
                         const MasterSettings& settings = {});

} // namespace trioio::perception
