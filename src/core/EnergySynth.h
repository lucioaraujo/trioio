#pragma once

// Síntese energética, primeira versão: um motor simples e próprio, dirigido
// pela BrainDecision. Especificação: docs/CADEIA_MINIMA.md §3.
//
// Cada escala da decisão tem efeito mensurável no som, na mesma escala da
// escuta (Descriptors): energia → nível (−60..0 dB), brilho → centroide
// (100 Hz..8 kHz, em oitavas), densidade → eventos por segundo (inverso de
// r/(r+4)), harmonicidade → tom × ruído filtrado, retenção de ataque →
// ataque duro ou macio, atividade espacial → largura estéreo.
//
// Tempo real: aloca só no construtor; setDecision() e render() não alocam,
// não travam e não fazem I/O. Determinístico pelo seed.

#include "TrioioBrain.h"

#include <array>
#include <cstdint>
#include <vector>

namespace trioio
{

class EnergySynth
{
public:
    EnergySynth(double sampleRate, std::uint64_t seed);

    void setDecision(const BrainDecision& decision) noexcept;
    void render(float* left, float* right, int numSamples) noexcept;

private:
    struct Smoothed
    {
        float value = 0.0f, target = 0.0f;
    };
    struct Voice
    {
        bool active = false;
        double phase[4] {};
        double inc[4] {};
        float tonal = 1.0f;     // mistura tom × ruído
        float level = 0.0f;
        float env = 0.0f;
        float attackStep = 0.0f;
        float decayMul = 0.0f;
        bool attacking = true;
        float panL = 0.7f, panR = 0.7f;
        // ruído filtrado (SVF TPT, passa-banda)
        float g = 0.0f, k = 0.0f, ic1 = 0.0f, ic2 = 0.0f;
    };

    float sine(double phase) const noexcept;
    float noise() noexcept;
    void startVoice() noexcept;
    float centroidTarget() const noexcept;

    double sampleRate_;
    std::uint64_t rng_;
    std::vector<float> table_;
    std::array<Voice, 24> voices_ {};
    Smoothed energy_, density_, brightness_, harmonicity_, transient_, spatial_;
    float smoothCoef_ = 0.0f;
    float levelCoef_ = 0.0f;
    float meanSquare_ = 1e-4f;
    double eventPhase_ = 0.0;
    double eventJitter_ = 1.0;
    // camada contínua: sustenta o som quando há poucos eventos
    double bedPhase_[4] {};
    float bedIc1_ = 0.0f, bedIc2_ = 0.0f;
    int coefCountdown_ = 0;
    float bedG_ = 0.0f, bedK_ = 0.0f;
    double bedInc_[4] {};
};

} // namespace trioio
