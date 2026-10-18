#pragma once

// Mashup de materiais (docs/COMPOSICAO.md, "Mashup"; estudo transversal
// §6.13): duas referências compostas juntas. A melodia, a forma e o olho
// nascem da referência A; da referência B podem vir:
//   harmonia   os acordes lidos em B (a melodia de A se curva a eles: o
//              mashup clássico, a voz de uma canção sobre o instrumental de
//              outra)
//   ritmo      o groove de B (o campo rítmico dos compassos de B)
//   memória    os fragmentos recombinados pelo oráculo e pelo mosaico
//   corpo      o corpo espectral (a família espectral toca com os parciais
//              de B)
// B é reescalada no tempo para a duração de A (estica ou encolhe por
// inteiro), para as duas andarem juntas do começo ao fim.

#include "Descriptors.h"

#include <string>
#include <vector>

namespace trioio::composition
{

enum MashupLayer : unsigned
{
    kMashHarmony = 1u,
    kMashRhythm = 2u,
    kMashMemory = 4u,
    kMashBody = 8u,
};

// "harmonia,ritmo,memoria,corpo" (ou "tudo"); false se algum nome é desconhecido.
[[nodiscard]] bool parseMashupLayers(const std::string& text, unsigned& out);
[[nodiscard]] std::string mashupLayersName(unsigned layers);

// Os quadros e ataques de B no tempo de A (multiplicados por durA/durB).
void remapTimeline(std::vector<perception::Frame>& frames, std::vector<perception::Onset>& onsets, double fromDuration,
                   double toDuration);

} // namespace trioio::composition
