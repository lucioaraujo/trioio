// O rasgo modular (src/core/ModularRasgo.h): cada patch transforma o som (a
// diferença é medida), o nível volta ao do corpo, nada estoura; as quimeras
// soam na nota; a distribuição respeita a quantidade (0 = nada).
#include "AcousticVoice.h"
#include "ModularRasgo.h"

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

int main()
{
    const double sr = 48000.0;
    Note base { 0.0, 1.0, 57.0f, 0.0f, 0.7f, 0, Timbre::guitar };
    const auto seco = trioio::renderAcousticNote(base, sr, 5);
    for (int p = 1; p < kPatches; ++p)
    {
        Note n = base;
        n.patch = static_cast<std::uint8_t>(p);
        n.patchDepth = 0.8f;
        n.patchRatio = 2.76f;
        auto x = seco;
        applyPatch(x, n, sr, 9);
        double diff = 0.0, pot = 0.0;
        float pk = 0.0f;
        bool fin = true;
        for (std::size_t i = 0; i < x.size(); ++i)
        {
            fin = fin && std::isfinite(x[i]);
            pk = std::max(pk, std::abs(x[i]));
            const float s = i < seco.size() ? seco[i] : 0.0f;
            diff += (x[i] - s) * (x[i] - s);
            pot += s * s;
        }
        float pkSeco = 0.0f;
        for (float v : seco)
            pkSeco = std::max(pkSeco, std::abs(v));
        std::printf("%-28s diferença %.2f, pico %.3f (seco %.3f)\n", patchName(static_cast<Patch>(p)), diff / pot, pk, pkSeco);
        CHECK(fin);
        CHECK(diff / pot > 0.05);                     // transforma
        CHECK(std::abs(pk - pkSeco) < 0.02f * pkSeco + 1e-6f);  // o nível volta ao do corpo
    }
    for (Timbre t : { Timbre::bowedPlate, Timbre::sungString, Timbre::blownSkin })
    {
        Note n = base;
        n.timbre = t;
        const auto x = trioio::renderAcousticNote(n, sr, 3);
        double pot = 0.0;
        bool fin = true;
        for (float v : x)
        {
            fin = fin && std::isfinite(v);
            pot += v * v;
        }
        std::printf("%-20s %.1f dB\n", timbreName(t), 10.0 * std::log10(pot / x.size() + 1e-12));
        CHECK(fin && pot > 1e-6);
    }
    // a quantidade: 0 = nada; 1 = quase tudo atravessado
    {
        Score s;
        s.duration = 20.0;
        for (int v : { 0, 2, 3, 5, 6 })
            for (int k = 0; k < 10; ++k)
                s.notes.push_back({ k * 2.0, 1.0, 60.0f, 0.0f, 0.6f, v, Timbre::guitar });
        TensionCurve c;
        auto s0 = s;
        CHECK(applyModularRasgo(s0, 0.0f, c, { 0.0, 10.0 }, 1).empty());
        int com = 0;
        auto s1 = s;
        const auto line = applyModularRasgo(s1, 1.0f, c, { 0.0, 10.0 }, 1);
        std::printf("%s\n", line.c_str());
        for (const auto& n : s1.notes)
            com += n.patch != 0 ? 1 : 0;
        CHECK(com >= 40);
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("rasgo modular: todas as checagens passaram");
    return EXIT_SUCCESS;
}
