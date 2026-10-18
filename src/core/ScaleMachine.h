#pragma once

// A máquina de escalas (docs/ESCALAS.md, "A máquina de escalas").
//
// Uma escala não precisa ficar parada: entre uma seção e outra ela se
// desvia, e o desvio abre um encadeamento harmônico novo. A máquina opera
// sobre QUALQUER escala, em cents e com período (de 12 sons, microtonal ou
// não oitavante):
//
//   rotação          um modo da mesma escala (começa noutro grau);
//   alteração        um grau sobe ou desce (um semitom, um quarto de tom
//                    ou uma coma): a "nota estrangeira" que muda a cor;
//   espelho          os intervalos invertidos (de cima para baixo): o
//                    dual da escala (o frígio do jônio, a utonalidade da
//                    otonalidade);
//   interpolação     cada grau caminha uma fração em direção à escala de
//                    destino: estados intermediários entre duas escalas
//                    (o Frames do Atlas §40 aplicado à altura);
//   pivô             a mesma escala noutra tônica, a que guarda mais notas
//                    comuns: a modulação por notas comuns;
//   acréscimo        um grau novo no meio do maior intervalo;
//   supressão        some o grau que menos pesa no campo;
//   alargamento      o período estica ou encolhe (oitava alargada).
//
// O encadeamento escolhe a operação pelo modo e pelo campo da seção
// seguinte, e registra o que mudou e quantas notas ficaram em comum.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace trioio::composition
{

struct ArchiveScale;  // ScaleArchive.h

struct MachineScale
{
    double tonic = 0.0;              // em cents acima de Dó (0..1200)
    std::vector<double> cents;       // graus acima da tônica, crescentes, cents[0] = 0
    double period = 1200.0;
    std::string name;
};

// A escala de uma seção da peça.
struct SectionScale
{
    double start = 0.0, end = 0.0;
    MachineScale scale;
};

// As operações (puras: devolvem uma escala nova).
[[nodiscard]] MachineScale rotateScale(const MachineScale& s, int degree);
[[nodiscard]] MachineScale alterDegree(const MachineScale& s, int degree, double deltaCents);
[[nodiscard]] MachineScale mirrorScale(const MachineScale& s);
[[nodiscard]] MachineScale interpolateScales(const MachineScale& from, const MachineScale& to, double amount);
[[nodiscard]] MachineScale addDegree(const MachineScale& s);
[[nodiscard]] MachineScale removeDegree(const MachineScale& s, int degree);
[[nodiscard]] MachineScale stretchScale(const MachineScale& s, double factor);

// A variante da escala sob um acorde (chord-scale: G. Russell, The Lydian
// Chromatic Concept of Tonal Organization, 1953): cada nota do acorde que
// não está na escala toma o lugar do grau mais próximo (até `reach` cents).
// Lá menor natural sob Mi maior: Sol → Sol# (a menor harmônica); Dó maior
// sob Ré maior: Fá → Fá#. `chord`: alturas absolutas em cents (0..1200).
// `changes` recebe (grau, desvio em cents) de cada grau alterado.
struct DegreeChange
{
    int degree = 0;
    double delta = 0.0;
};
[[nodiscard]] MachineScale chordVariant(const MachineScale& s, const std::vector<double>& chord,
                                        std::vector<DegreeChange>* changes = nullptr, double reach = 150.0);

// Notas em comum entre duas escalas (alturas absolutas, tolerância em cents).
[[nodiscard]] int commonTones(const MachineScale& a, const MachineScale& b, double tolerance = 15.0);
// A mesma escala na tônica (≠ da atual) que guarda mais notas comuns; com
// `field`, desempata pela que melhor veste o campo.
[[nodiscard]] MachineScale pivotScale(const MachineScale& s, const std::array<float, 12>& field);
// Quanto a escala veste o campo (0..1).
[[nodiscard]] double fieldFit(const MachineScale& s, const std::array<float, 12>& field);

enum class MachineMode { follow, transfigure, counter, interrupt };

struct MachineStep
{
    MachineScale scale;
    std::string how;   // "alteração: o 4.º grau sobe uma coma (21,5 c)", "pivô: …"
    int common = 0;    // notas comuns com a escala anterior
};

// O próximo elo: a partir da escala anterior e do campo da seção seguinte.
// `archive` (pode ser vazio) serve de destino para interpolar e para o
// salto de "interromper".
// `intention` (ScaleAffect.h; 0 = nenhuma) e `position` (0..1 na peça):
// com uma intenção, os desvios e os destinos são escolhidos entre os
// candidatos pela distância afetiva ao alvo.
[[nodiscard]] MachineStep nextScale(const MachineScale& previous, const std::array<float, 12>& field, MachineMode mode,
                                    std::uint64_t seed, const std::vector<ArchiveScale>* archive, int intention = 0,
                                    double position = 0.5);

} // namespace trioio::composition
