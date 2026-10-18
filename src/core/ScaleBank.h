#pragma once

// O banco de escalas do Trioio (docs/ESCALAS.md), adaptado do Rasgo Synth
// Studio (cópia fixada em third_party/rasgo_synth_dsp/sequencer/, com
// proveniência):
//
//   - o CATÁLOGO de 45 escalas de 12 sons do Studio (Scales.hpp): modos
//     diatônicos e da menor melódica e harmônica, simétricas, pentatônicas,
//     bebop, húngaras, japonesas, Prometeu...; no Trioio, a escolha é a que
//     melhor veste o campo de alturas da referência;
//   - ESCALAS INVENTADAS por crivo de Xenakis (SieveScale.hpp), sempre fora
//     do catálogo;
//   - AFINAÇÕES (MicrotonalTuning.hpp): justa 5-limit, pitagórica e quartos
//     de tom;
//   - o crivo sobre uma divisão mais fina da oitava (19, 22 ou 31 partes).
//     Adaptação do Trioio: os graus admitidos SÃO a escala, fora dos 12
//     semitons. No Studio, servem só para desviar os 12 semitons.
//
// Os sorteios usam o gerador do Trioio (a mesma seed dá a mesma escala em
// qualquer sistema), não as distribuições da biblioteca padrão.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace trioio::composition
{

struct ScaleChoice
{
    float tonic = 0.0f;        // classe de altura 0..12 (com fração)
    std::vector<float> steps;  // semitons acima da tônica, crescentes, steps[0] = 0, < 12
    std::string name;          // "dórico b2", "crivo inventado (p%3==0 ∪ p%4==1)", "crivo em 22-EDO (…)"
};

[[nodiscard]] int scaleCatalogSize();                // 45
[[nodiscard]] const char* scaleCatalogName(int index);
[[nodiscard]] std::vector<float> scaleCatalogSteps(int index);

// A escala do catálogo, e a tônica, que melhor vestem `field` (o peso de
// cada classe de altura na referência): sorteada entre as `choices`
// melhores. Sem campo, uma qualquer.
[[nodiscard]] ScaleChoice fitCatalog(const std::array<float, 12>& field, std::uint64_t seed, int choices = 3);

// Uma escala de 12 sons por crivo de Xenakis (2 ou 3 condições
// módulo/resíduo, módulos 2..5, de 4 a 9 notas), que não está no catálogo.
[[nodiscard]] ScaleChoice inventedScale(std::uint64_t seed);

// O crivo sobre 19, 22 ou 31 divisões iguais da oitava: os graus admitidos,
// em semitons fracionários. Entre 5 e 9 graus.
[[nodiscard]] ScaleChoice edoSieveScale(std::uint64_t seed);

enum class Tuning { justIntonation, pythagorean, quarterTone };

// Reafina os graus (relativos à tônica), cada um pelo desvio da sua classe
// de altura no sistema: justa 5-limit e pitagórica pelas razões puras; os
// quartos de tom sobem meio semitom em metade das classes, sorteadas.
[[nodiscard]] std::vector<float> retune(const std::vector<float>& steps, Tuning tuning, std::uint64_t seed);
[[nodiscard]] const char* tuningName(Tuning tuning);

} // namespace trioio::composition
