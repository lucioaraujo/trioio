#pragma once

// A política de janela do Trioio (docs/design.md §5; contrato responsivo de
// RASGO_DOCUMENTATION/design/INTERFACES_E_LAYOUTS.md §5.1). Pura e sem JUCE,
// para ser testada na suíte. Escrita para o Trioio (o utilitário comum
// candidato é o do AQUORBIUM, que só será copiado depois de G4).

#include <vector>

namespace trioio::app
{

struct Rect
{
    int x = 0, y = 0, w = 0, h = 0;
    [[nodiscard]] bool empty() const noexcept { return w <= 0 || h <= 0; }
};

// Os regimes reais (design.md §5): completo a partir de 1600 de largura;
// compacto de 1024 a 1599; emergência abaixo de 1024 de largura ou de 650
// de altura.
enum class Regime { full, compact, emergency };
[[nodiscard]] Regime classify(int width, int height) noexcept;
[[nodiscard]] const char* regimeName(Regime r) noexcept;

inline constexpr int kCanvasW = 1920, kCanvasH = 1080;  // canvas de referência
inline constexpr int kMinW = 960, kMinH = 600;          // emergência
inline constexpr int kCompactW = 1024, kCompactH = 650;
inline constexpr int kFullW = 1600;
inline constexpr double kFirstOpenFraction = 0.88;

// Primeira abertura: ~88 % da área útil do monitor, sem passar do canvas de
// referência, nunca menor que o mínimo (salvo se a área for menor: aí a
// área inteira), centralizada nessa área.
[[nodiscard]] Rect initialBounds(const Rect& userArea, double fraction = kFirstOpenFraction) noexcept;

// Restauração: os limites salvos valem se ao menos metade da janela (e a
// faixa do título, 40 px do topo) cair numa área útil existente; então são
// encaixados nela. Senão, a primeira abertura na primeira área (a
// principal). Monitor removido, resolução nova: a janela nunca fica perdida.
[[nodiscard]] Rect revalidate(const Rect& saved, const std::vector<Rect>& userAreas) noexcept;

} // namespace trioio::app
