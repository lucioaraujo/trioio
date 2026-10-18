#pragma once

// Memória performativa (Atlas de Referência RASGO §63, OMax/IRCAM: "o
// sistema escuta, aprende, reorganiza, responde"; aplicações: "repetir
// deformado", "reconstruir", "gerar memória falsa").
//
// O Trioio aprende a sequência de eventos da referência (altura, duração,
// força de cada ataque) num ORÁCULO DE FATORES e o percorre de novo: segue
// adiante na ordem original ou salta, pelos links de sufixo, para outro
// ponto da gravação que compartilha o mesmo contexto recente. Sai material
// da própria referência, recombinado: reconhecível e imprevisível ao mesmo
// tempo.
//
// Algoritmo (público; implementação própria, do zero):
//   C. Allauzen, M. Crochemore, M. Raffinot, "Factor oracle: a new
//   structure for pattern matching", SOFSEM 1999;
//   G. Assayag, S. Dubnov, "Using factor oracles for machine improvisation",
//   Soft Computing 8, 2004 (a base do OMax).
// Especificação: docs/MEMORIA.md.

#include "Composer.h"

#include <cstdint>
#include <map>
#include <vector>

namespace trioio::composition
{

// O oráculo de fatores de uma sequência de símbolos inteiros, construído
// incrementalmente. Estados 0..n; o estado i (i ≥ 1) é "depois do i-ésimo
// símbolo".
class FactorOracle
{
public:
    void add(int symbol);
    [[nodiscard]] int size() const noexcept { return static_cast<int>(symbols_.size()); }
    [[nodiscard]] int symbol(int state) const { return symbols_[static_cast<std::size_t>(state - 1)]; }  // state ≥ 1
    [[nodiscard]] int suffix(int state) const { return sfx_[static_cast<std::size_t>(state)]; }         // −1 no estado 0
    [[nodiscard]] int transition(int state, int symbol) const;  // −1 se não houver
    // Um caminho de `length` estados (≥ 1) a partir de `start`: a cada passo
    // segue adiante com probabilidade `continuity` ou salta pelo link de
    // sufixo. `uniform` dá números em [0,1). Devolve os estados visitados e
    // conta os saltos em `jumps`.
    template <typename Uniform>
    std::vector<int> navigate(int start, int length, double continuity, Uniform uniform, int& jumps) const
    {
        std::vector<int> path;
        jumps = 0;
        int s = start;
        const int n = size();
        while (static_cast<int>(path.size()) < length && n > 0)
        {
            const bool canGo = s < n;
            if (!canGo || uniform() >= continuity)
            {
                // salto: volta pelo sufixo (um ponto anterior com o mesmo
                // contexto) e segue dali. Se o contexto nunca se repetiu antes
                // (sufixo 0), vai a outra ocorrência do MESMO símbolo, em
                // qualquer ponto: o par seguinte continua sendo um par da
                // referência. Só sem nenhuma outra ocorrência o salto é livre.
                int k = s > 0 ? suffix(s) : 0;
                if (k <= 0 && s > 0)
                {
                    std::vector<int> same;
                    for (int q = 1; q < n; ++q)
                        if (q != s && symbol(q) == symbol(s))
                            same.push_back(q);
                    if (!same.empty())
                        k = same[static_cast<std::size_t>(uniform() * static_cast<double>(same.size())) % same.size()];
                }
                if (k <= 0 || k >= n)
                    k = static_cast<int>(uniform() * n);
                if (k != s)
                    ++jumps;
                s = k;
            }
            s = std::min(s + 1, n);
            path.push_back(s);
        }
        return path;
    }

private:
    std::vector<int> symbols_;
    std::vector<std::map<int, int>> trans_ { {} };  // trans_[estado][símbolo] = estado
    std::vector<int> sfx_ { -1 };
};

// Insere na partitura passagens de memória: a referência recombinada pelo
// oráculo, numa voz própria (voice 3), com o corpo espectral da referência.
// Quantas e quão saltadas dependem da imprevisibilidade (0 = nenhuma: a
// composição fica idêntica). Marca cada passagem na forma.
void oracleMemory(Score& score, const std::vector<perception::Frame>& frames,
                  const std::vector<perception::Onset>& onsets, const Options& options);

} // namespace trioio::composition
