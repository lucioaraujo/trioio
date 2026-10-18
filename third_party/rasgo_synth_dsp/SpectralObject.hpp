#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "Oscillator.hpp"
#include "Envelope.hpp"

namespace rasgo {

// Fase 113 -- objeto espectral (task #3, terceiro item do territorio
// conceitual musical nao explorado): pedido explicito do usuario de
// aprofundar espectralismo de forma original, nao como "pegar espectro e
// virar acorde" mas como uma unica estrutura de proporcoes numericas que
// se manifesta simultaneamente como altura, ritmo e timbre -- a propria
// pergunta que o usuario levantou, "que transformacao do som produz o
// proximo estado da musica?", em vez de "que acorde vem depois?".
//
// Reune tres das ideias experimentais do usuario, cada uma real e
// implementavel neste motor:
//
//   Eixo A -- harmonicidade <-> inarmonicidade (ideia #5 do usuario): os
//   parciais partem de multiplos inteiros puros do fundamental e se
//   afastam continuamente rumo a razoes esticadas, usando o MESMO modelo
//   real de inarmonicidade documentado pra cordas de piano e sinos --
//   f_p = f0 * p * sqrt(1 + B*p^2), Fletcher & Rossing, "The Physics of
//   Musical Instruments" -- controlado pela propria densidade do arco
//   narrativo da peca (secao calma = quase harmonico/estavel, climax =
//   esticado/instavel), entao a forma da peca literalmente pilota a
//   estabilidade acustica do objeto.
//
//   Eixo B -- buracos de sieve no espectro (ideia #7): a MESMA matematica
//   de sieve de Xenakis ja usada pra escalas de 12 tons (fase 61) e pra
//   afinacao microtonal (fase 112), reaproveitada aqui pra decidir QUAIS
//   PARCIAIS SOAM em vez de quais alturas -- um parcial "admitido" se seu
//   indice satisfizer ao menos uma condicao modulo/residuo sorteada. Os
//   buracos migram (condicoes reamostradas) em fronteiras reais de secao,
//   entao o espectro "regenera" ao vivo, nunca fica estatico a peca
//   inteira.
//
//   Eixo C -- ritmo espectral (ideia #8, a mais radical do pedido do
//   usuario, e a que ele mesmo apontou como ponte direta com a pesquisa
//   deste motor em metricas experimentais): o MESMO numero inteiro que
//   define a razao de altura do parcial p tambem define sua propria taxa
//   de pulso -- o parcial p soa p vezes mais rapido que o parcial 1, no
//   mesmo compasso-base que todo o resto do motor ja usa. Altura e ritmo
//   deixam de ser dois sistemas separados: sao a MESMA proporcao numerica
//   lida em duas escalas de tempo diferentes.
class SpectralObject {
public:
    static constexpr int kNumPartials = 6;

    // Tres conjuntos reais de numeros de parciais, cada um com uma
    // justificativa real (nao arbitraria):
    //   FullHarmonic -- a serie harmonica de livro-texto, 1..6.
    //   OddHarmonic -- so parciais impares, o espectro real de um tubo
    //     cilindrico fechado numa ponta (clarinete) -- harmonicos pares
    //     genuinamente suprimidos pela fisica do tubo, fato acustico
    //     documentado (Fletcher & Rossing; Benade, "Fundamentals of
    //     Musical Acoustics"), nao uma escolha estetica arbitraria.
    //   PrimeWeighted -- 1,2,3,5,7,11: uma propriedade real de teoria dos
    //     numeros da propria serie harmonica (numeros compostos como
    //     4=2x2, 6=2x3, 8=2x2x2 majoritariamente repetem classes de altura
    //     ja implicadas por seus fatores menores; primos sao onde conteudo
    //     de altura genuinamente novo entra), a mesma logica que compositores
    //     espectrais usam ao decidir quais parciais destacar.
    enum class PartialSet { FullHarmonic, OddHarmonic, PrimeWeighted, Count };

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        for (auto &e : envelopes_) e.setSampleRate(sr);
    }

    void configure(PartialSet set, double fundamentalHz, unsigned seed) {
        set_ = set;
        fundamentalHz_ = fundamentalHz;
        rng_.seed(seed);
        partials_ = partialNumbersFor(set);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        for (int i = 0; i < kNumPartials; ++i) {
            oscillators_[static_cast<size_t>(i)].setSampleRate(sampleRate_);
            oscillators_[static_cast<size_t>(i)].setWaveform(Oscillator::Waveform::Sine);
            envelopes_[static_cast<size_t>(i)].setADSR(
                0.008 + unit(rng_) * 0.015, 0.18 + unit(rng_) * 0.25, 0.0, 0.05 + unit(rng_) * 0.05);
            admitted_[static_cast<size_t>(i)] = true; // ate a primeira regeneracao de sieve
            samplesUntilNextPulse_[static_cast<size_t>(i)] = 0;
            pulseCounts_[static_cast<size_t>(i)] = 0;
        }
        retuneOscillators();
        regenerateSieveHoles(seed + 1u);
    }

    // Eixo A -- 0 = harmonico puro (multiplos inteiros exatos); cresce
    // rumo a um real desvio audivel de estabilidade. Chamado toda vez que
    // a densidade do arco muda de verdade (mesma cadencia de outros
    // parametros ja pilotados pela densidade neste motor).
    void setInharmonicity(double b) {
        inharmonicityCoeff_ = std::clamp(b, 0.0, 0.004);
        retuneOscillators();
    }
    double inharmonicity() const { return inharmonicityCoeff_; }

    // Fase 116 -- leitura somente-consulta da frequencia atual (ja
    // considerando o Eixo A) de um parcial especifico -- usada pelo
    // RasgoFracture.hpp pra sintonizar a textura de uma fibra rasgada na
    // mesma regiao espectral que esse parcial realmente ocupa. Nao
    // adiciona nenhum estado novo nem muda tick()/retuneOscillators().
    double partialFrequency(int i) const {
        double p = static_cast<double>(partials_[static_cast<size_t>(i)]);
        return fundamentalHz_ * p * std::sqrt(1.0 + inharmonicityCoeff_ * p * p);
    }

    // Eixo B -- reamostra as condicoes de sieve (2-3 pares modulo/residuo
    // sobre os indices 0..kNumPartials-1, indice 0/parcial 1 sempre
    // admitido -- o fundamental nunca desaparece) e recalcula quais dos
    // kNumPartials estao "abertos" agora. Chamado em fronteiras reais de
    // secao.
    void regenerateSieveHoles(unsigned seed) {
        std::mt19937 holeRng(seed);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        std::vector<std::pair<int, int>> conditions;
        int numConditions = 1 + (unit(holeRng) < 0.5 ? 1 : 0);
        for (int c = 0; c < numConditions; ++c) {
            int modulus = 2 + static_cast<int>(unit(holeRng) * 3.0); // 2..4
            int residue = static_cast<int>(unit(holeRng) * modulus);
            conditions.push_back({modulus, residue});
        }
        admittedCount_ = 0;
        for (int i = 0; i < kNumPartials; ++i) {
            bool ok = (i == 0);
            for (auto &cond : conditions) {
                if (i % cond.first == cond.second) { ok = true; break; }
            }
            admitted_[static_cast<size_t>(i)] = ok;
            if (ok) ++admittedCount_;
        }
        holeConditions_ = conditions;
    }
    int admittedCount() const { return admittedCount_; }
    bool admitted(int i) const { return admitted_[static_cast<size_t>(i)]; }

    // Eixo C -- recalcula os periodos de pulso de cada parcial a partir
    // de um periodo-base em amostras (tipicamente samplesPerBar * N
    // compassos) -- o parcial p pulsa exatamente basePeriod/p amostras
    // depois do anterior, entao p soa p vezes mais rapido que o parcial 1
    // no mesmo tempo real. Chamado toda vez que o periodo-base muda
    // (fronteira de compasso, mesma disciplina de phaseSamplesPerStep na
    // fase 111).
    void setBasePeriodSamples(size_t basePeriodSamples) {
        for (int i = 0; i < kNumPartials; ++i) {
            size_t period = std::max<size_t>(1, basePeriodSamples / static_cast<size_t>(partials_[static_cast<size_t>(i)]));
            pulsePeriodSamples_[static_cast<size_t>(i)] = period;
        }
    }

    void setGain(float g) { masterGain_ = g; }

    float tick() {
        float out = 0.0f;
        for (int i = 0; i < kNumPartials; ++i) {
            size_t idx = static_cast<size_t>(i);
            if (admitted_[idx]) {
                if (samplesUntilNextPulse_[idx] == 0) {
                    envelopes_[idx].noteOn();
                    samplesUntilNextPulse_[idx] = pulsePeriodSamples_[idx];
                    ++pulseCounts_[idx];
                }
                --samplesUntilNextPulse_[idx];
            }
            float amp = 1.0f / static_cast<float>(partials_[idx]); // decaimento harmonico natural
            out += oscillators_[idx].tick() * envelopes_[idx].tick() * amp;
        }
        return out * masterGain_;
    }

    int pulseCount(int i) const { return pulseCounts_[static_cast<size_t>(i)]; }
    int totalPulseCount() const {
        int total = 0;
        for (int c : pulseCounts_) total += c;
        return total;
    }

    static std::array<int, kNumPartials> partialNumbersFor(PartialSet set) {
        switch (set) {
            case PartialSet::OddHarmonic: return {1, 3, 5, 7, 9, 11};
            case PartialSet::PrimeWeighted: return {1, 2, 3, 5, 7, 11};
            default: return {1, 2, 3, 4, 5, 6};
        }
    }

    static std::string setName(PartialSet set) {
        switch (set) {
            case PartialSet::OddHarmonic: return "Serie impar (tubo fechado, clarinete)";
            case PartialSet::PrimeWeighted: return "Parciais primos (1,2,3,5,7,11)";
            default: return "Serie harmonica completa (1-6)";
        }
    }

    std::string description() const {
        std::ostringstream desc;
        desc << setName(set_) << ", fundamental " << static_cast<int>(fundamentalHz_ + 0.5) << "Hz";
        return desc.str();
    }

private:
    void retuneOscillators() {
        for (int i = 0; i < kNumPartials; ++i) {
            double p = static_cast<double>(partials_[static_cast<size_t>(i)]);
            // Fletcher & Rossing's real piano-string/bell inharmonicity
            // model: f_p = f0 * p * sqrt(1 + B*p^2). B=0 collapses exactly
            // to the pure harmonic series.
            double freq = fundamentalHz_ * p * std::sqrt(1.0 + inharmonicityCoeff_ * p * p);
            oscillators_[static_cast<size_t>(i)].setFrequency(freq);
        }
    }

    PartialSet set_ = PartialSet::FullHarmonic;
    double fundamentalHz_ = 65.0;
    double sampleRate_ = 44100.0;
    double inharmonicityCoeff_ = 0.0;
    std::array<int, kNumPartials> partials_{ {1, 2, 3, 4, 5, 6} };
    std::array<Oscillator, kNumPartials> oscillators_;
    std::array<Envelope, kNumPartials> envelopes_;
    std::array<bool, kNumPartials> admitted_{};
    std::array<size_t, kNumPartials> pulsePeriodSamples_{};
    std::array<size_t, kNumPartials> samplesUntilNextPulse_{};
    std::array<int, kNumPartials> pulseCounts_{};
    int admittedCount_ = 0;
    std::vector<std::pair<int, int>> holeConditions_;
    float masterGain_ = 0.2f;
    std::mt19937 rng_;
};

} // namespace rasgo
