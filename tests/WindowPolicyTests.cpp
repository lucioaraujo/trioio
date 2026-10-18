// A política de janela do Trioio (src/app/WindowPolicy.h): regimes, primeira
// abertura e restauração com monitor removido.

#include "WindowPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace trioio::app;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static bool inside(const Rect& r, const Rect& a)
{
    return r.x >= a.x && r.y >= a.y && r.x + r.w <= a.x + a.w && r.y + r.h <= a.y + a.h;
}

int main()
{
    // regimes
    CHECK(classify(1920, 1080) == Regime::full);
    CHECK(classify(1600, 900) == Regime::full);
    CHECK(classify(1599, 900) == Regime::compact);
    CHECK(classify(1024, 650) == Regime::compact);
    CHECK(classify(1023, 700) == Regime::emergency);
    CHECK(classify(1366, 640) == Regime::emergency);
    CHECK(classify(960, 600) == Regime::emergency);

    // primeira abertura: ~88 %, sem passar do canvas, centralizada, dentro da área
    {
        const Rect a { 0, 27, 1920, 1053 };  // 1080p com barra de 27 px
        const auto r = initialBounds(a);
        CHECK(inside(r, a) && r.w == 1690 && r.h == 927);
        CHECK(std::abs((r.x - a.x) - (a.x + a.w - r.x - r.w)) <= 1);
        const Rect big { 0, 0, 3840, 2160 };
        const auto r2 = initialBounds(big);
        CHECK(r2.w == kCanvasW && r2.h == kCanvasH && inside(r2, big));
        const Rect small { 0, 0, 1000, 640 };  // menor que o compacto: o mínimo de emergência, dentro da área
        const auto r3 = initialBounds(small);
        CHECK(inside(r3, small) && r3.w >= kMinW && r3.h >= kMinH);
        const Rect tiny { 0, 0, 900, 560 };  // menor que o mínimo: a área inteira
        const auto r4 = initialBounds(tiny);
        CHECK(r4.w == 900 && r4.h == 560 && inside(r4, tiny));
    }

    // restauração
    {
        const Rect left { 0, 0, 1920, 1050 }, right { 1920, 0, 2560, 1410 };
        const Rect saved { 2100, 100, 1600, 1000 };  // no monitor da direita
        CHECK(revalidate(saved, { left, right }).x == 2100);  // os dois monitores: fica onde estava
        const auto lost = revalidate(saved, { left });         // o da direita foi removido
        CHECK(inside(lost, left));
        const Rect offTitle { 100, -500, 1200, 800 };  // título acima da área
        CHECK(inside(revalidate(offTitle, { left }), left));
        const Rect tooBig { 0, 0, 3000, 1800 };
        const auto fit = revalidate(tooBig, { left });
        CHECK(inside(fit, left));
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("política de janela: todas as checagens passaram");
    return EXIT_SUCCESS;
}
