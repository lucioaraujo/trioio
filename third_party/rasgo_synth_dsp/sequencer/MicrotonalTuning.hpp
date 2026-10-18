#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace rasgo {

// Fase 112 -- microtonalidade/afinacao justa (task #3, territorio conceitual
// musical nao explorado): ate aqui, toda altura deste motor vem de
// midiToFreq(), 12-TET fixo (semitom igual, a norma ocidental pos-Bach), e
// o unico desvio ja existente (freeIntonationEnabled) e uma deriva CAOTICA
// nao-estruturada -- um "errar a nota", nao um sistema de afinacao
// coerente. Esta classe reafina a voz principal pra um de quatro sistemas:
//
//   - Just Intonation 5-limit: razoes inteiras simples (Ptolomeu's "intense
//     diatonic", central em Harry Partch, Genesis of a Music, 1949).
//   - Pythagorean: pilha de quintas puras 3/2 (3-limit), o sistema
//     ocidental antes do temperamento mesotonico/igual.
//   - Quarter-tone (24-EDO): a tradicao de Alois Haba/Ivan Wyschnegradsky
//     no microtonalismo ocidental do seculo XX -- deliberadamente NAO
//     reivindicada como reproducao de nenhuma entonacao de maqam arabe
//     real (que usa inflexoes nao-equidistantes e dependentes de
//     contexto).
//   - Sieve microtonal (a contribuicao original desta fase): a MESMA
//     matematica de sieve de Xenakis que SieveScale.hpp ja usa pra gerar
//     escalas de 12 tons nao catalogadas (uniao de condicoes
//     modulo/residuo), estendida aqui pra uma divisao igual da oitava mais
//     fina que 12 -- 19-EDO (Easley Blackwood), 22-EDO (explorada em
//     teoria xenarmonica pelas boas aproximacoes de razoes 7-limit) ou
//     31-EDO (o orgao de 31 tons de Huygens/Fokker). O sieve roda sobre
//     essa grade mais fina; cada um dos 12 graus cromaticos que o resto
//     do motor ja usa recebe o desvio em cents ate o degrau de sieve mais
//     proximo -- uma inflexao microtonal genuinamente gerada por peca, nao
//     copiada de nenhuma tabela historica.
//
// Todos os desvios estaticos sao expressos em cents RELATIVOS ao 12-TET,
// medidos a partir da tonica da propria peca (nao um C fixo de teclado) --
// exatamente como esses sistemas historicos eram de fato usados. Alem
// disso, o Just Intonation e o Pythagorean carregam uma segunda camada
// original: deriva real de comma. Modulacoes tonais genuinas (ja existentes
// no motor) encadeiam o intervalo de razao PURA entre a tonica antiga e a
// nova em vez de resetar pra 12-TET a cada modulacao -- exatamente o
// fenomeno real que corais e conjuntos de afinacao justa vivem (uma pilha
// de quintas puras nunca fecha exatamente numa oitava; o erro acumulado E
// o comma pitagorico/sintonico, ~23.46/~21.51 cents), aqui reproduzido
// como consequencia matematica honesta da propria cadeia de modulacoes da
// peca, nao como um efeito programado a parte.
enum class TuningSystem { JustIntonation5Limit, Pythagorean, QuarterTone, SieveMicrotonal, Count };

class MicrotonalTuning {
public:
    void configure(TuningSystem system, unsigned seed) {
        system_ = system;
        commaDriftCents_ = 0.0;
        if (system_ == TuningSystem::QuarterTone) {
            std::mt19937 rng(seed);
            std::uniform_real_distribution<double> unit(0.0, 1.0);
            for (int pc = 1; pc < 12; ++pc) {
                quarterToneSharp_[static_cast<size_t>(pc)] = unit(rng) < 0.5;
            }
            quarterToneSharp_[0] = false; // a tonica sempre fica afinada consigo mesma
        } else if (system_ == TuningSystem::SieveMicrotonal) {
            configureSieveMicrotonal(seed);
        }
    }

    TuningSystem system() const { return system_; }

    // So tem efeito real em JustIntonation5Limit/Pythagorean -- as duas
    // afinacoes de razao pura onde encadear intervalos genuinamente
    // acumula erro (ver comentario da classe). Chamada uma vez por
    // modulacao tonal real, com a tonica ANTES e DEPOIS da modulacao.
    void accumulateModulationDrift(int oldRootMidiNote, int newRootMidiNote) {
        if (system_ != TuningSystem::JustIntonation5Limit && system_ != TuningSystem::Pythagorean) return;
        int interval = ((newRootMidiNote - oldRootMidiNote) % 12 + 12) % 12;
        const std::array<double, 12> &table = (system_ == TuningSystem::JustIntonation5Limit) ? kJustCents : kPythagoreanCents;
        double pureCents = table[static_cast<size_t>(interval)];
        double equalCents = interval * 100.0;
        commaDriftCents_ += (pureCents - equalCents);
    }
    double commaDrift() const { return commaDriftCents_; }

    // Real desvio em cents pra essa classe de altura (0-11, semitons a
    // partir da tonica da peca), relativo ao 12-TET.
    double centsOffset(int pc) const {
        switch (system_) {
            case TuningSystem::JustIntonation5Limit:
                return (kJustCents[static_cast<size_t>(pc)] - kEqualCents[static_cast<size_t>(pc)]) + commaDriftCents_;
            case TuningSystem::Pythagorean:
                return (kPythagoreanCents[static_cast<size_t>(pc)] - kEqualCents[static_cast<size_t>(pc)]) + commaDriftCents_;
            case TuningSystem::QuarterTone:
                return quarterToneSharp_[static_cast<size_t>(pc)] ? 50.0 : 0.0;
            case TuningSystem::SieveMicrotonal:
                return sieveCentsOffset_[static_cast<size_t>(pc)];
            default:
                return 0.0;
        }
    }

    // equalTemperedFreq ja reflete midiNote via a formula 12-TET padrao
    // (midiToFreq); isto so multiplica por um real desvio em cents da
    // classe de altura correspondente.
    double retune(double equalTemperedFreq, int midiNote, int rootMidiNote) const {
        int pc = ((midiNote - rootMidiNote) % 12 + 12) % 12;
        return equalTemperedFreq * std::pow(2.0, centsOffset(pc) / 1200.0);
    }

    static std::string systemName(TuningSystem s) {
        switch (s) {
            case TuningSystem::JustIntonation5Limit: return "Afinacao justa 5-limit (Ptolomeu/Partch)";
            case TuningSystem::Pythagorean: return "Afinacao pitagorica (pilha de quintas puras)";
            case TuningSystem::QuarterTone: return "Grade de quarto-de-tom 24-EDO (Haba/Wyschnegradsky)";
            case TuningSystem::SieveMicrotonal: return "Sieve de Xenakis sobre EDO fina (gerada por peca)";
            default: return "12-TET";
        }
    }

    // Descricao completa, com os parametros reais gerados (transparencia
    // no mesmo espirito de generatedScaleDescription) -- usada pro score/
    // metadados quando o sistema e SieveMicrotonal, cujos parametros
    // variam por peca em vez de serem um nome fixo.
    std::string description() const {
        if (system_ != TuningSystem::SieveMicrotonal) return systemName(system_);
        return sieveDescription_;
    }

private:
    void configureSieveMicrotonal(unsigned seed) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> unit(0.0, 1.0);

        static const int kEdoChoices[] = {19, 22, 31};
        int edo = kEdoChoices[static_cast<int>(unit(rng) * 3.0) % 3];

        // Mesma proporcao alvo de graus admitidos que SieveScale.hpp usa
        // pra 12 (4..9, ou seja 33%-75%), so escalada pra este N mais
        // fino.
        int minAdmitted = std::max(4, edo * 4 / 12);
        int maxAdmitted = std::max(minAdmitted + 1, edo * 9 / 12);

        std::vector<bool> admitted(static_cast<size_t>(edo), false);
        std::vector<std::pair<int, int>> conditions;
        int attempts = 0;
        int admittedCount = 0;
        do {
            conditions.clear();
            int numConditions = 2 + (unit(rng) < 0.4 ? 1 : 0);
            int maxModulus = std::max(5, edo / 4);
            for (int c = 0; c < numConditions; ++c) {
                int modulus = 2 + static_cast<int>(unit(rng) * (maxModulus - 1));
                int residue = static_cast<int>(unit(rng) * modulus);
                conditions.push_back({modulus, residue});
            }
            std::fill(admitted.begin(), admitted.end(), false);
            admitted[0] = true; // a tonica sempre pertence ao sieve
            admittedCount = 1;
            for (int step = 0; step < edo; ++step) {
                if (admitted[static_cast<size_t>(step)]) continue;
                for (auto &cond : conditions) {
                    if (step % cond.first == cond.second) {
                        admitted[static_cast<size_t>(step)] = true;
                        ++admittedCount;
                        break;
                    }
                }
            }
            ++attempts;
        } while ((admittedCount < minAdmitted || admittedCount > maxAdmitted) && attempts < 30);

        // Cada um dos 12 graus cromaticos que o resto do motor ja usa
        // recebe o desvio em cents ate o degrau de sieve admitido mais
        // proximo nesta grade EDO mais fina -- distancia circular (a
        // oitava e um ciclo, nao uma reta).
        for (int pc = 0; pc < 12; ++pc) {
            double target = pc * 100.0;
            double bestAbs = 1e9;
            double bestOffset = 0.0;
            for (int step = 0; step < edo; ++step) {
                if (!admitted[static_cast<size_t>(step)]) continue;
                double stepCents = step * 1200.0 / edo;
                double diff = stepCents - target;
                while (diff > 600.0) diff -= 1200.0;
                while (diff <= -600.0) diff += 1200.0;
                if (std::fabs(diff) < bestAbs) { bestAbs = std::fabs(diff); bestOffset = diff; }
            }
            sieveCentsOffset_[static_cast<size_t>(pc)] = bestOffset;
        }

        std::ostringstream desc;
        desc << "Sieve de Xenakis sobre " << edo << "-EDO (";
        for (size_t i = 0; i < conditions.size(); ++i) {
            if (i) desc << ",";
            desc << "p%" << conditions[i].first << "==" << conditions[i].second;
        }
        desc << "; " << admittedCount << "/" << edo << " graus admitidos, mapeados pros 12 graus"
             << " cromaticos existentes por vizinho mais proximo)";
        sieveDescription_ = desc.str();
    }

    TuningSystem system_ = TuningSystem::JustIntonation5Limit;
    std::array<bool, 12> quarterToneSharp_{};
    std::array<double, 12> sieveCentsOffset_{};
    std::string sieveDescription_;
    double commaDriftCents_ = 0.0;

    // Cents a partir da tonica, valores publicados padrao
    // (log2(razao)*1200). kEqualCents e o proprio 12-TET, so pra facilitar
    // o calculo do desvio acima.
    static constexpr std::array<double, 12> kEqualCents = {
        0.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0
    };
    // Escala cromatica justa 5-limit: 1/1, 16/15, 9/8, 6/5, 5/4, 4/3,
    // 45/32, 3/2, 8/5, 5/3, 16/9, 15/8.
    static constexpr std::array<double, 12> kJustCents = {
        0.0, 111.73, 203.91, 315.64, 386.31, 498.04, 590.22, 701.96, 813.69, 884.36, 996.09, 1088.27
    };
    // Pilha pitagorica de quintas puras 3/2: 1/1, 256/243, 9/8, 32/27,
    // 81/64, 4/3, 729/512, 3/2, 128/81, 27/16, 16/9, 243/128.
    static constexpr std::array<double, 12> kPythagoreanCents = {
        0.0, 90.22, 203.91, 294.13, 407.82, 498.04, 611.73, 701.96, 792.18, 905.87, 996.09, 1109.78
    };
};

} // namespace rasgo
