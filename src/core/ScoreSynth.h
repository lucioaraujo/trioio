#pragma once

// Toca uma partitura (Composer) offline: vozes aditivas com timbre que evolui
// e uma sala simples. Especificação: docs/COMPOSICAO.md §"Síntese".
//
// Offline: aloca e percorre a partitura nota a nota. A versão em tempo real
// (vozes pré-alocadas) virá com a escuta ao vivo.

#include "Composer.h"

#include <functional>
#include <string>
#include <vector>

namespace trioio
{

// Renderiza `score` em estéreo na taxa `sampleRate`; o pico final fica em
// −1 dBFS. Determinístico.
// Com `stems`, guarda também cada barramento processado (as vozes que
// soaram, os efeitos e a sala) com o mesmo ganho final: a soma dos stems é
// a mistura antes da cola de barramento (glue), que não entra neles.
struct Stem
{
    std::string name;  // "1-motivo", "2-baixo", …, "8-gestos" (o olho, os efeitos de cinema), "9-sala"
    std::vector<float> left, right;
};
// `onProgress` (opcional): a fração 0..1 da síntese, a cada lote de vozes
// (chamado na thread que chamou; a barra do app)
void renderScore(const composition::Score& score, double sampleRate, std::vector<float>& left,
                 std::vector<float>& right, std::vector<Stem>* stems = nullptr,
                 const std::function<void(float)>& onProgress = {});

} // namespace trioio
