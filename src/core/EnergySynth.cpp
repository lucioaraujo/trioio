#include "EnergySynth.h"

#include <algorithm>
#include <cmath>

namespace trioio
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr int kTable = 4096;
constexpr float kHarm4 = 1.0f + 1.0f / 2.0f + 1.0f / 3.0f + 1.0f / 4.0f;  // H_4

float dbToGain(float db) { return std::pow(10.0f, db / 20.0f); }
} // namespace

EnergySynth::EnergySynth(double sampleRate, std::uint64_t seed)
    : sampleRate_(sampleRate), rng_(seed ? seed : 0x9E3779B97F4A7C15ull)
{
    table_.resize(kTable + 1);
    for (int i = 0; i <= kTable; ++i)
        table_[static_cast<std::size_t>(i)] = static_cast<float>(std::sin(2.0 * kPi * i / kTable));
    smoothCoef_ = static_cast<float>(1.0 - std::exp(-1.0 / (0.04 * sampleRate_)));  // ~40 ms
    levelCoef_ = static_cast<float>(1.0 - std::exp(-1.0 / (1.0 * sampleRate_)));    // ~1 s
}

void EnergySynth::setDecision(const BrainDecision& in) noexcept
{
    const auto d = sanitizeDecision(in);
    energy_.target = d.energy;
    density_.target = d.density;
    brightness_.target = d.brightness;
    harmonicity_.target = d.harmonicity;
    transient_.target = d.transientRetention;
    spatial_.target = d.spatialActivity;
}

float EnergySynth::sine(double phase) const noexcept
{
    const double p = (phase - std::floor(phase)) * kTable;
    const auto i = static_cast<int>(p);
    const auto f = static_cast<float>(p - i);
    return table_[static_cast<std::size_t>(i)] + f * (table_[static_cast<std::size_t>(i) + 1] - table_[static_cast<std::size_t>(i)]);
}

float EnergySynth::noise() noexcept
{
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 7;
    rng_ ^= rng_ << 17;
    return static_cast<float>(static_cast<double>(rng_ >> 11) / 9007199254740992.0 * 2.0 - 1.0);
}

// brilho 0..1 → centroide 100 Hz..8 kHz, a mesma escala da escuta
float EnergySynth::centroidTarget() const noexcept
{
    return 100.0f * std::pow(80.0f, brightness_.value);
}

void EnergySynth::startVoice() noexcept
{
    auto* v = &voices_[0];
    for (auto& c : voices_)  // livre, ou a mais fraca
        if (!c.active || c.env < v->env)
        {
            v = &c;
            if (!c.active)
                break;
        }
    // centroide de 4 parciais 1/k: f0·4/H4 → f0 = c·H4/4, deslocado ±meio tom
    // por uma escala pentatônica pequena para não soar como uma nota só
    static constexpr float steps[] = { 0.0f, 3.0f, 5.0f, 7.0f, 10.0f, -2.0f, -5.0f };
    const float semis = steps[static_cast<std::size_t>((noise() * 0.5f + 0.5f) * 6.999f)];
    const float f0 = std::clamp(centroidTarget() * kHarm4 / 4.0f * std::pow(2.0f, semis / 12.0f), 30.0f,
                                static_cast<float>(sampleRate_ * 0.1));
    for (int k = 0; k < 4; ++k)
    {
        v->phase[k] = 0.0;
        v->inc[k] = f0 * (k + 1) / sampleRate_;
    }
    v->tonal = harmonicity_.value;
    v->level = 0.8f + 0.2f * noise();
    v->env = 0.0f;
    v->attacking = true;
    // retenção de ataque: 1 → 1 ms (duro); 0 → 40 ms (macio)
    const float attackSeconds = 0.040f - 0.039f * transient_.value;
    v->attackStep = static_cast<float>(1.0 / (attackSeconds * sampleRate_));
    // denso → notas curtas; rarefeito → longas
    const float decaySeconds = 1.2f - 1.05f * density_.value;
    v->decayMul = static_cast<float>(std::exp(-1.0 / (decaySeconds * 0.25 * sampleRate_)));
    const float pan = 0.5f + 0.5f * spatial_.value * noise();
    v->panL = std::sqrt(1.0f - pan);
    v->panR = std::sqrt(pan);
    const float fc = std::min(centroidTarget(), static_cast<float>(sampleRate_ * 0.45));
    v->g = static_cast<float>(std::tan(kPi * fc / sampleRate_));
    v->k = 1.0f / 1.5f;  // Q 1,5
    v->ic1 = v->ic2 = 0.0f;
    v->active = true;
}

void EnergySynth::render(float* left, float* right, int numSamples) noexcept
{
    for (int n = 0; n < numSamples; ++n)
    {
        for (auto* s : { &energy_, &density_, &brightness_, &harmonicity_, &transient_, &spatial_ })
            s->value += smoothCoef_ * (s->target - s->value);

        // coeficientes da camada contínua: a cada 32 amostras
        if (--coefCountdown_ <= 0)
        {
            coefCountdown_ = 32;
            const float c = centroidTarget();
            const float f0 = std::clamp(c * kHarm4 / 4.0f, 30.0f, static_cast<float>(sampleRate_ * 0.1));
            for (int k = 0; k < 4; ++k)
                bedInc_[k] = f0 * (k + 1) * (1.0 + 0.002 * k) / sampleRate_;
            bedG_ = static_cast<float>(std::tan(kPi * std::min(c, static_cast<float>(sampleRate_ * 0.45)) / sampleRate_));
            bedK_ = 1.0f / 1.2f;
        }

        // relógio de eventos: densidade d → r = 4d/(1−d) eventos/s (até 16)
        const float d = std::min(density_.value, 0.8f);
        // Acumulador de fase: a taxa vale a cada amostra. (Calcular o intervalo
        // no disparo prendia a taxa do instante: o primeiro evento, com a
        // densidade ainda subindo, marcava o próximo para 15 s depois.)
        const double rate = 4.0 * d / (1.0 - d);
        eventPhase_ += rate * eventJitter_ / sampleRate_;
        if (eventPhase_ >= 1.0)
        {
            eventPhase_ -= std::floor(eventPhase_);
            if (energy_.value > 0.01f)
                startVoice();
            eventJitter_ = 0.6 + 0.8 * (noise() * 0.5 + 0.5);  // ±40 % no intervalo seguinte
        }

        float l = 0.0f, r = 0.0f;
        for (auto& v : voices_)
        {
            if (!v.active)
                continue;
            if (v.attacking)
            {
                v.env += v.attackStep;
                if (v.env >= 1.0f)
                {
                    v.env = 1.0f;
                    v.attacking = false;
                }
            }
            else
            {
                v.env *= v.decayMul;
                if (v.env < 1e-4f)
                {
                    v.active = false;
                    continue;
                }
            }
            float tone = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                tone += sine(v.phase[k]) / static_cast<float>(k + 1);
                v.phase[k] += v.inc[k];
            }
            // SVF TPT passa-banda sobre ruído
            const float x = noise();
            const float v1 = (v.ic1 + v.g * (x - v.ic2)) / (1.0f + v.g * (v.g + v.k));
            const float v2 = v.ic2 + v.g * v1;
            v.ic1 = 2.0f * v1 - v.ic1;
            v.ic2 = 2.0f * v2 - v.ic2;
            const float band = v1 * 2.0f;
            const float s = v.env * v.level * (v.tonal * tone + (1.0f - v.tonal) * band * 2.0f);
            l += s * v.panL;
            r += s * v.panR;
        }

        // camada contínua, mais presente quando há poucos eventos
        float bedTone = 0.0f;
        for (int k = 0; k < 4; ++k)
        {
            bedTone += sine(bedPhase_[k]) / static_cast<float>(k + 1);
            bedPhase_[k] += bedInc_[k];
            bedPhase_[k] -= std::floor(bedPhase_[k]);
        }
        const float x = noise();
        const float b1 = (bedIc1_ + bedG_ * (x - bedIc2_)) / (1.0f + bedG_ * (bedG_ + bedK_));
        const float b2 = bedIc2_ + bedG_ * b1;
        bedIc1_ = 2.0f * b1 - bedIc1_;
        bedIc2_ = 2.0f * b2 - bedIc2_;
        const float h = harmonicity_.value;
        // baixa quando há eventos, para que os ataques se destaquem dela
        const float bed = (0.08f + 0.4f * (1.0f - density_.value)) * (h * bedTone * 0.5f + (1.0f - h) * b1 * 2.0f);
        l += bed;
        r += bed;

        // energia → nível RMS na escala da escuta (−60 dB em 0, 0 dB em 1).
        // Um nivelador lento (~1 s) mede o conteúdo e o leva ao nível pedido:
        // lento o bastante para não achatar os ataques.
        meanSquare_ += levelCoef_ * (0.5f * (l * l + r * r) - meanSquare_);
        const float target = energy_.value <= 1e-3f ? 0.0f : dbToGain(-60.0f + 60.0f * energy_.value);
        const float gain = target / std::sqrt(meanSquare_ + 1e-9f);
        left[n] = std::tanh(l * gain);
        right[n] = std::tanh(r * gain);
    }
}

} // namespace trioio
