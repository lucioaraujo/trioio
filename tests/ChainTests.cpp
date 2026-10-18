// Cadeia mínima no núcleo, sem arquivos: escuta → estratégia → síntese →
// escuta da resposta. A prova de que a estratégia chega ao som é medida:
// "seguir" correlaciona com a referência, "contrapor" anticorrelaciona.

#include "Descriptors.h"
#include "EnergySynth.h"
#include "Strategy.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <vector>

// conta alocações para provar que render() não aloca
static std::atomic<long> alocacoes { 0 };
void* operator new(std::size_t n)
{
    ++alocacoes;
    if (void* p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

using namespace trioio;
using namespace trioio::perception;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

constexpr double SR = 48000.0;
constexpr double JANELA = 0.5;

// referência: quatro caracteres que se alternam a cada 2 s
static std::vector<float> referencia()
{
    const double dur = 24.0;
    std::vector<float> v(static_cast<std::size_t>(SR * dur));
    std::uint32_t x = 9;
    const auto rnd = [&] { x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x / 4294967295.0 * 2.0 - 1.0; };
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR;
        const int bloco = static_cast<int>(t / 2.0) % 4;
        double s = 0.0;
        switch (bloco)
        {
            case 0: s = 0.02 * std::sin(2.0 * 3.14159265358979 * 150.0 * t); break;     // baixo, escuro, parado
            case 1: s = 0.6 * rnd() * (std::fmod(t, 0.125) < 0.02 ? 1.0 : 0.05); break;  // alto, brilhante, denso
            case 2: s = 0.15 * std::sin(2.0 * 3.14159265358979 * 600.0 * t)
                        * (std::fmod(t, 0.5) < 0.1 ? 1.0 : 0.3); break;                  // médio
            case 3: s = 0.4 * rnd() * (std::fmod(t, 0.25) < 0.02 ? 1.0 : 0.1)
                        + 0.1 * std::sin(2.0 * 3.14159265358979 * 2000.0 * t); break;   // alto-médio, claro
        }
        v[i] = static_cast<float>(s);
    }
    return v;
}

static std::vector<Summary> escutar(const std::vector<float>& mono)
{
    Analyzer a(SR);
    a.push(mono.data(), mono.size());
    a.finish();
    return segment(a.frames(), a.onsets(), static_cast<double>(mono.size()) / SR, JANELA);
}

static std::vector<float> sintetizar(const std::vector<strategy::Step>& plano, std::uint64_t seed,
                                     std::vector<float>* direito = nullptr, long* alocNoRender = nullptr)
{
    EnergySynth synth(SR, seed);
    const auto total = static_cast<std::size_t>(plano.back().end * SR + 0.5);
    std::vector<float> l(total), r(total);
    const long antes = alocacoes.load();
    for (const auto& p : plano)
    {
        synth.setDecision(p.decision);
        auto i0 = static_cast<std::size_t>(p.start * SR + 0.5);
        const auto i1 = std::min(total, static_cast<std::size_t>(p.end * SR + 0.5));
        while (i0 < i1)
        {
            const auto n = std::min<std::size_t>(512, i1 - i0);
            synth.render(l.data() + i0, r.data() + i0, static_cast<int>(n));
            i0 += n;
        }
    }
    if (alocNoRender)
        *alocNoRender = alocacoes.load() - antes;
    if (direito)
        *direito = r;
    std::vector<float> mono(total);
    for (std::size_t i = 0; i < total; ++i)
        mono[i] = 0.5f * (l[i] + r[i]);
    return mono;
}

int main()
{
    const auto ref = referencia();
    const auto escutaRef = escutar(ref);
    CHECK(escutaRef.size() == 48);

    struct Esperado { strategy::Mode modo; double sinal; };
    for (const auto [modo, sinal] : { Esperado { strategy::Mode::follow, +1.0 },
                                      Esperado { strategy::Mode::counter, -1.0 } })
    {
        const auto plano = strategy::plan(escutaRef, modo, 7);
        const auto resp = escutar(sintetizar(plano, 7));
        const double ce = correlation(escutaRef, resp, &Summary::energy);
        const double cb = correlation(escutaRef, resp, &Summary::brightness);
        const double cd = correlation(escutaRef, resp, &Summary::density);
        std::printf("%-12s energia %+.2f  brilho %+.2f  densidade %+.2f\n", strategy::modeName(modo), ce, cb, cd);
        if (std::getenv("TRIOIO_DEPURAR"))
            for (std::size_t i = 0; i < 16; ++i)
                std::printf("  %4.1fs ref d=%.2f e=%.2f | plano d=%.2f | resp d=%.2f (%d at) e=%.2f\n", escutaRef[i].start,
                            escutaRef[i].density, escutaRef[i].energy, plano[i].decision.density, resp[i].density,
                            resp[i].onsetCount, resp[i].energy);
        CHECK(ce * sinal > 0.6);
        CHECK(cb * sinal > 0.6);
        CHECK(cd * sinal > 0.3);
    }

    // transfigurar: mesma energia, brilho invertido
    {
        const auto plano = strategy::plan(escutaRef, strategy::Mode::transfigure, 7);
        const auto resp = escutar(sintetizar(plano, 7));
        const double ce = correlation(escutaRef, resp, &Summary::energy);
        const double cb = correlation(escutaRef, resp, &Summary::brightness);
        std::printf("%-12s energia %+.2f  brilho %+.2f\n", "transfigurar", ce, cb);
        CHECK(ce > 0.6);
        CHECK(cb < -0.6);
    }

    // interromper: há trechos em silêncio que a referência não tem
    {
        const auto plano = strategy::plan(escutaRef, strategy::Mode::interrupt, 7);
        int cortes = 0;
        for (const auto& p : plano)
            cortes += p.decision.energy == 0.0f;
        CHECK(cortes >= 2);
    }

    // determinismo, seed, limites, silêncio e tempo real
    {
        const auto plano = strategy::plan(escutaRef, strategy::Mode::follow, 3);
        long aloc = -1;
        std::vector<float> dir;
        const auto a = sintetizar(plano, 3, &dir, &aloc);
        const auto b = sintetizar(plano, 3);
        const auto c = sintetizar(strategy::plan(escutaRef, strategy::Mode::follow, 4), 4);
        CHECK(a == b);
        CHECK(a != c);
        CHECK(aloc == 0);
        bool finito = true;
        float pico = 0.0f;
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            finito = finito && std::isfinite(a[i]) && std::isfinite(dir[i]);
            pico = std::max({ pico, std::abs(a[i]), std::abs(dir[i]) });
        }
        CHECK(finito);
        CHECK(pico <= 1.0f);

        EnergySynth s(SR, 1);
        BrainDecision zero;  // energia 0
        s.setDecision(zero);
        std::vector<float> l(48000), r(48000);
        s.render(l.data(), r.data(), 48000);
        bool mudo = true;
        for (std::size_t i = 24000; i < l.size(); ++i)
            mudo = mudo && l[i] == 0.0f && r[i] == 0.0f;
        CHECK(mudo);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("cadeia: todas as checagens passaram");
    return EXIT_SUCCESS;
}
