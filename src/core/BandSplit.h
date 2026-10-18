#pragma once

// Linhas de referência por região de frequência (docs/LINHAS.md, etapa 1).
//
// A referência é dividida em bandas complementares: cada corte é um
// Butterworth de 2.ª ordem aplicado em ida e volta (fase zero), o que dá a
// magnitude de um Linkwitz-Riley de 4.ª ordem (−6 dB no corte, 24 dB/oitava;
// S. Linkwitz, JAES 24(1), 1976). Cada banda sai do RESTO por subtração —
// banda_k = PB_k(resto_k), resto_{k+1} = resto_k − banda_k —, então a soma das
// linhas é a referência, amostra a amostra: nada se perde nem se duplica.
//
// Não é separação de fontes: uma banda corta a voz e o violão onde eles se
// cruzam. A linha é a escuta de uma região do espectro, não de um instrumento.
// Offline (a gravação inteira na memória), fora da thread de áudio.

#include <string>
#include <vector>

namespace trioio::perception
{

struct BandLine
{
    double lowHz = 0.0, highHz = 0.0;  // 0 = sem limite (a primeira começa em 0, a última vai até Nyquist)
    std::vector<float> samples;
    double energyShare = 0.0;          // fração da energia da referência que está nesta linha
    [[nodiscard]] std::string name() const;  // "grave (< 200 Hz)", "200–2000 Hz", "agudo (> 2000 Hz)"
};

// `cutsHz` crescentes, dentro de (20 Hz, Nyquist × 0,9); de 1 a 4 cortes
// (2 a 5 linhas). Cortes inválidos são descartados; sem nenhum válido,
// devolve uma linha só (a referência inteira).
[[nodiscard]] std::vector<BandLine> splitBands(const std::vector<float>& x, double sampleRate, std::vector<double> cutsHz);

// Só a banda `index` de `x` (a mesma divisão, sem guardar as outras): o
// filtro da resposta de cada linha, para ela soar na sua região.
[[nodiscard]] std::vector<float> extractBand(const std::vector<float>& x, double sampleRate, std::vector<double> cutsHz,
                                             std::size_t index);

// O ganho que leva a linha ao RMS da referência (para os limiares de
// silêncio da análise valerem igual em todas), limitado a +30 dB.
[[nodiscard]] float levelMatchGain(const BandLine& line, const std::vector<float>& reference);

} // namespace trioio::perception
