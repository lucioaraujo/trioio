// O barramento de master (src/core/Master.h): a loudness pela BS.1770, o alvo,
// o teto de pico verdadeiro, e a soma dos stems preservada.
#include "Master.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
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
    const double sr = 48000.0, PI = 3.14159265358979323846;
    const std::size_t n = static_cast<std::size_t>(sr * 10.0);

    // 1. a medida: 997 Hz a −20 dBFS de pico nos dois canais ≈ −20 LUFS
    //    (BS.1770: 0 dBFS num canal = −3,01 LUFS; dois canais somam +3 dB)
    {
        std::vector<float> l(n), r(n);
        for (std::size_t i = 0; i < n; ++i)
            l[i] = r[i] = static_cast<float>(0.1 * std::sin(2.0 * PI * 997.0 * static_cast<double>(i) / sr));
        const double lufs = integratedLoudness(l, r, sr);
        std::printf("997 Hz a −20 dBFS: %.2f LUFS · pico verdadeiro %.2f dBTP\n", lufs, truePeakDb(l, r, sr));
        CHECK(std::abs(lufs + 20.0) < 0.2);
        CHECK(std::abs(truePeakDb(l, r, sr) + 20.0) < 0.1);
        std::vector<float> one(n, 0.0f);
        CHECK(std::abs(integratedLoudness(l, one, sr) + 23.01) < 0.2);  // um canal só: −3,01 dB
    }

    // 2. uma "peça" baixa e com ataques: o master leva a −16 LUFS e segura o
    //    teto de −1 dBTP; a soma de dois stems continua sendo a mistura
    {
        std::vector<float> a(n), b(n), l(n), r(n);
        std::uint32_t x = 7;
        for (std::size_t i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(i) / sr;
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            const double noise = x / 4294967295.0 * 2.0 - 1.0;
            const double hit = std::fmod(t, 0.5) < 0.01 ? 0.9 * noise : 0.0;  // golpes curtos e altos
            a[i] = static_cast<float>(0.05 * std::sin(2.0 * PI * 220.0 * t) + hit);
            b[i] = static_cast<float>(0.03 * std::sin(2.0 * PI * 1760.0 * t) * (0.5 + 0.5 * std::sin(2.0 * PI * 0.2 * t)));
            l[i] = a[i] + b[i];
            r[i] = 0.8f * a[i] + b[i];
        }
        std::vector<float> al = a, ar(n), bl = b, br = b;
        for (std::size_t i = 0; i < n; ++i)
            ar[i] = 0.8f * a[i];
        const auto rep = applyMaster(l, r, sr, { { &al, &ar }, { &bl, &br } });
        std::printf("master: %.1f → %.1f LUFS · pico %.2f → %.2f dBTP · cola até %.1f dB · limitador até %.1f dB · ganho %+.1f dB\n",
                    rep.lufsBefore, rep.lufsAfter, rep.truePeakBefore, rep.truePeakAfter, rep.compressionMaxDb,
                    rep.limiterMaxDb, rep.loudnessGainDb);
        // golpes de ruído a 0,9 que já passam do zero digital (+3,5 dBTP): o
        // teto obriga a baixar; o limitador não corta mais de 8 dB e a peça
        // fica abaixo do alvo (a dinâmica vale mais que o alvo)
        CHECK(rep.truePeakBefore > 0.0 && rep.lufsAfter <= -15.5);
        CHECK(rep.truePeakAfter <= -0.95);
        CHECK(rep.limiterMaxDb > 0.5 && rep.limiterMaxDb <= 8.5);
        double err = 0.0;
        for (std::size_t i = 0; i < n; ++i)
            err = std::max({ err, static_cast<double>(std::abs(al[i] + bl[i] - l[i])), static_cast<double>(std::abs(ar[i] + br[i] - r[i])) });
        std::printf("soma dos stems − mistura: %.2g\n", err);
        CHECK(err < 1e-5);
    }

    // 2b. uma peça de golpes moderados chega ao alvo
    {
        std::vector<float> l(n), r(n);
        std::uint32_t x = 11;
        for (std::size_t i = 0; i < n; ++i)
        {
            const double t = static_cast<double>(i) / sr;
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            const double noise = x / 4294967295.0 * 2.0 - 1.0;
            const double hit = std::fmod(t, 0.5) < 0.03 ? 0.25 * noise * std::exp(-std::fmod(t, 0.5) / 0.01) : 0.0;
            l[i] = static_cast<float>(0.05 * std::sin(2.0 * PI * 220.0 * t) + 0.03 * std::sin(2.0 * PI * 660.0 * t) + hit);
            r[i] = static_cast<float>(0.05 * std::sin(2.0 * PI * 330.0 * t) + hit);
        }
        const auto rep = applyMaster(l, r, sr);
        std::printf("master (golpes moderados): %.1f → %.1f LUFS · pico %.2f dBTP · limitador até %.1f dB\n", rep.lufsBefore,
                    rep.lufsAfter, rep.truePeakAfter, rep.limiterMaxDb);
        CHECK(std::abs(rep.lufsAfter + 16.0) < 0.5);
        CHECK(rep.truePeakAfter <= -0.95);
    }

    // 3. silêncio: nada explode
    {
        std::vector<float> l(n, 0.0f), r(n, 0.0f);
        const auto rep = applyMaster(l, r, sr);
        CHECK(rep.loudnessGainDb == 0.0);
        float m = 0.0f;
        for (float v : l)
            m = std::max(m, std::abs(v));
        CHECK(m == 0.0f);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("master: todas as checagens passaram");
    return EXIT_SUCCESS;
}
