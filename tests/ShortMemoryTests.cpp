// A memória curta (src/core/ShortMemory.h): só descritores, limitada, sem
// crescer com a escuta; o resumo da janela.
#include "ShortMemory.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace trioio::perception;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

int main()
{
    ShortMemory m(10.0, 0.01);
    const auto cap = m.capacity();
    // 60 s de escuta: a energia sobe de −40 a −10 dB; altura 220 Hz; 4 ataques/s
    for (int i = 0; i < 6000; ++i)
    {
        Frame f;
        f.time = i * 0.01;
        f.silent = false;
        f.rmsDb = -40.0f + 30.0f * static_cast<float>(i) / 6000.0f;
        f.centroidHz = 1000.0f;
        f.f0Hz = 220.0f;
        f.periodicity = 0.9f;
        m.push(f);
        if (i % 25 == 0)
            m.pushOnset({ f.time, 1.0f });
    }
    CHECK(m.capacity() == cap && m.frameCount() == cap);  // não cresceu
    const auto r = m.recent(10.0);
    CHECK(!r.empty() && r.front().time >= 49.99 && std::abs(r.back().time - 59.99) < 1e-6);
    const auto s = m.summary(5.0);
    std::printf("resumo de 5 s: %.1f dB, %.1f ataques/s, f0 %.0f Hz, tendência %+.2f dB/s\n", static_cast<double>(s.meanDb),
                static_cast<double>(s.onsetsPerSecond), static_cast<double>(s.medianF0Hz), static_cast<double>(s.trendDbPerSecond));
    CHECK(std::abs(s.onsetsPerSecond - 4.0f) < 0.3f && std::abs(s.medianF0Hz - 220.0f) < 0.5f);
    CHECK(std::abs(s.trendDbPerSecond - 0.5f) < 0.02f && s.meanDb > -13.0f && s.silentShare == 0.0f);
    // a janela pedida maior que a memória fica na memória
    CHECK(m.recent(60.0).size() == cap);
    m.clear();
    CHECK(m.frameCount() == 0 && m.recent(5.0).empty() && m.summary(5.0).silentShare == 1.0f);
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("memória curta: todas as checagens passaram");
    return EXIT_SUCCESS;
}
