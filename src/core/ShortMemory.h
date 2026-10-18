#pragma once

// A janela temporal e a memória curta (TRIOIO_TAREFAS, Marco 2; docs/
// BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md, "RESPONDER AGORA"): os últimos N
// segundos da escuta como DESCRITORES (os quadros e os ataques), nunca o
// áudio. O buffer é um anel pré-alocado: push() não aloca nem trava, e a
// memória não cresce com a duração da escuta. É a base da escuta ao vivo: o
// "o que acabou de soar" que o diálogo e o Terceiro Olho leem.

#include "Descriptors.h"

#include <cstddef>
#include <vector>

namespace trioio::perception
{

class ShortMemory
{
public:
    // `seconds` de memória com quadros a cada `hopSeconds` (o Analyzer usa ~10 ms)
    ShortMemory(double seconds, double hopSeconds);

    void push(const Frame& f) noexcept;   // sem alocação: sobrescreve o mais antigo
    void pushOnset(const Onset& o) noexcept;
    void clear() noexcept;

    [[nodiscard]] std::size_t frameCount() const noexcept { return frames_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return ring_.size(); }
    [[nodiscard]] double seconds() const noexcept { return seconds_; }
    [[nodiscard]] double newestTime() const noexcept;

    // os quadros dos últimos `window` segundos, do mais antigo ao mais novo
    // (cópia: para quem decide fora do caminho do áudio)
    [[nodiscard]] std::vector<Frame> recent(double window) const;
    [[nodiscard]] std::vector<Onset> recentOnsets(double window) const;

    // o resumo dos últimos `window` segundos
    struct Summary
    {
        float meanDb = -120.0f, peakDb = -120.0f;
        float centroidHz = 0.0f;
        float onsetsPerSecond = 0.0f;
        float pitchedShare = 0.0f;   // quadros com altura definida
        float medianF0Hz = 0.0f;     // 0 = sem altura
        float silentShare = 1.0f;
        float trendDbPerSecond = 0.0f;  // a energia subindo (+) ou caindo (−)
    };
    [[nodiscard]] Summary summary(double window) const;

private:
    double seconds_ = 0.0, hop_ = 0.01;
    std::vector<Frame> ring_;
    std::size_t head_ = 0, frames_ = 0;
    std::vector<Onset> onsetRing_;
    std::size_t onsetHead_ = 0, onsets_ = 0;
};

} // namespace trioio::perception
