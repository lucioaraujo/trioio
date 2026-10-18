// Os efeitos de cinema (src/core/FilmEffects.h): onde caem na forma (o riser
// antes do clímax, o impacto e o braam nele, o whoosh nas seções, a
// ambiência nos trechos rarefeitos pelo brilho) e se cada um soa (finito,
// audível, sem estouro).
#include "FilmEffects.h"
#include "ScoreSynth.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace trioio::composition;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static const Gesture* acha(const Score& s, GestureKind k)
{
    for (const auto& g : s.gestures)
        if (g.kind == k)
            return &g;
    return nullptr;
}

int main()
{
    // uma peça de 60 s: rarefeita de 0 a 14 s, clímax aos 40 s; seções a 0,
    // 15, 25 (perto demais da anterior) e 50 s
    TensionCurve c;
    c.step = 0.5;
    for (int i = 0; i < 120; ++i)
    {
        const double t = i * 0.5;
        c.value.push_back(t < 14.0 ? 0.1f : static_cast<float>(0.4 + 0.5 * std::exp(-std::pow((t - 40.0) / 8.0, 2.0))));
    }
    c.climax = 40.0;
    const std::vector<double> secoes { 0.0, 15.0, 25.0, 50.0 };
    for (float brilho : { 0.2f, 0.5f, 0.8f })
    {
        Score s;
        s.duration = 60.0;
        const auto line = applyFilmEffects(s, c, secoes, brilho, 7);
        std::printf("brilho %.1f: %s\n", static_cast<double>(brilho), line.c_str());
        const auto* riser = acha(s, GestureKind::riser);
        const auto* impact = acha(s, GestureKind::impact);
        CHECK(riser && std::abs(riser->time + riser->duration - 40.0) < 1e-6);
        CHECK(impact && std::abs(impact->time - 40.0) < 1e-6);
        CHECK(acha(s, GestureKind::braam));
        int whooshes = 0;
        for (const auto& g : s.gestures)
            whooshes += g.kind == GestureKind::whoosh ? 1 : 0;
        CHECK(whooshes == 2);  // 15 e 50 (25 está a menos de 20 s do anterior)
        const GestureKind amb = brilho > 0.6f ? GestureKind::rain : brilho < 0.35f ? GestureKind::wind : GestureKind::water;
        const GestureKind alt = brilho > 0.6f ? GestureKind::crowd : brilho < 0.35f ? GestureKind::fire : GestureKind::city;
        const auto* a = acha(s, amb);
        if (!a)
            a = acha(s, alt);
        CHECK(a && a->time < 0.01 && a->duration >= 13.0);
    }
    // cada efeito soa: finito, audível, abaixo de 0 dBFS
    for (GestureKind k : { GestureKind::wind, GestureKind::rain, GestureKind::thunder, GestureKind::water, GestureKind::whoosh,
                           GestureKind::riser, GestureKind::impact, GestureKind::braam, GestureKind::fire, GestureKind::crowd,
                           GestureKind::city })
    {
        Score s;
        s.duration = 5.0;
        Gesture g;
        g.kind = k;
        g.time = 0.2;
        g.duration = 4.0;
        g.level = 0.5f;
        g.lo = k == GestureKind::braam ? 31.0f : 48.0f;
        g.density = k == GestureKind::rain ? 600.0f : 90.0f;
        g.pan = -0.8f;
        g.panEnd = 0.8f;
        s.gestures.push_back(g);
        std::vector<float> L, R;
        trioio::renderScore(s, 48000.0, L, R);
        bool fin = true;
        double pot = 0.0;
        for (std::size_t i = 0; i < L.size(); ++i)
        {
            fin = fin && std::isfinite(L[i]) && std::isfinite(R[i]);
            pot += 0.5 * (L[i] * L[i] + R[i] * R[i]);
        }
        const double db = 10.0 * std::log10(pot / static_cast<double>(L.size()) + 1e-12);
        std::printf("efeito %d: %.1f dB\n", static_cast<int>(k), db);
        CHECK(fin);
        CHECK(db > -45.0);
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("efeitos de cinema: todas as checagens passaram");
    return EXIT_SUCCESS;
}
