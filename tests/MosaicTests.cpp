// Memória pesquisável por timbre (core/Mosaic.h, docs/MOSAICO.md): as
// unidades vêm de ataques da referência; "seguir" escolhe as que soam como o
// momento, "contrapor" e "transfigurar" as opostas; sem imprevisibilidade ou
// com o mosaico desligado, nada muda; as regras e o oráculo são respeitados.

#include "Composer.h"
#include "Mosaic.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

using namespace trioio;
using namespace trioio::composition;
using namespace trioio::perception;
using Mode = trioio::strategy::Mode;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

constexpr double SR = 48000.0;
constexpr double PI = 3.14159265358979323846;
constexpr double SECAO = 10.0;

// trechos alternados de 10 s: pares claros, fortes e ruidosos; ímpares
// escuros, fracos e puros
static bool claro(double t) { return static_cast<int>(t / SECAO) % 2 == 0; }

int main()
{
    const int mel[] = { 60, 64, 67, 62, 65, 69, 64, 71 };
    std::vector<float> v(static_cast<std::size_t>(SR * 60.0));
    std::uint32_t ruido = 12345;
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.4);
        const int k = static_cast<int>(t / 0.4) % 8;
        const double f = 440.0 * std::pow(2.0, (mel[k] - 69) / 12.0);
        ruido = ruido * 1664525u + 1013904223u;
        const double nz = static_cast<double>(ruido >> 8) / 8388608.0 - 1.0;
        double s;
        if (claro(t))
        {
            s = 0.0;  // dente de serra de 24 harmônicos + chiado
            for (int h = 1; h <= 24; ++h)
                s += std::sin(2 * PI * f * h * t) / h;
            s = 0.45 * s + 0.12 * nz;
        }
        else
            s = 0.06 * std::sin(PI * f * t);  // uma oitava abaixo, pura e fraca
        v[i] = static_cast<float>(std::exp(-tt * 3.0) * s);
    }
    Analyzer a(SR);
    a.push(v.data(), v.size());
    a.finish();
    const auto opcoes = [](Mode m, std::uint64_t seed, float u, bool mosaico = true) {
        Options o { m, seed, PitchSource::reference, TimeMode::automatic, u };
        o.mosaic = mosaico;
        return o;
    };
    const auto mosaico = [](const Score& sc) {
        std::vector<Note> r;
        for (const auto& n : sc.notes)
            if (n.voice == 5)
                r.push_back(n);
        return r;
    };

    // 1. sem imprevisibilidade ou desligado: nenhuma nota do mosaico
    CHECK(mosaico(compose(a.frames(), a.onsets(), 60.0, opcoes(Mode::follow, 1, 0.0f))).empty());
    CHECK(mosaico(compose(a.frames(), a.onsets(), 60.0, opcoes(Mode::follow, 1, 0.8f, false))).empty());
    CHECK(!mosaico(compose(a.frames(), a.onsets(), 60.0, opcoes(Mode::follow, 1, 0.8f))).empty());

    // 2. a escolha pelo som: de que trecho vêm as unidades. A base é composta
    // sem mosaico; o mosaico é aplicado direto (sem o Terceiro Olho depois)
    struct Conta { int todas = 0, notas = 0, mesmoTrecho = 0, naoSaoAtaques = 0, foraDoOraculo = 0, espectrais = 0; };
    const auto contar = [&](Mode m, const Constraints& regras = {}) {
        Conta c;
        for (std::uint64_t s = 1; s <= 8; ++s)
        {
            auto o = opcoes(m, s, 0.8f, false);
            o.constraints = regras;
            Score sc = compose(a.frames(), a.onsets(), 60.0, o);
            o.mosaic = true;
            descriptorMosaic(sc, a.frames(), a.onsets(), o);
            std::vector<std::pair<double, double>> oraculo;
            for (const auto& n : sc.notes)
                if (n.voice == 3)
                {
                    if (!oraculo.empty() && n.time < oraculo.back().second + 1.5)
                        oraculo.back().second = std::max(oraculo.back().second, n.time + n.duration);
                    else
                        oraculo.push_back({ n.time, n.time + n.duration });
                }
            for (const auto& n : mosaico(sc))
            {
                ++c.todas;
                c.foraDoOraculo += std::none_of(oraculo.begin(), oraculo.end(),
                                                [&](const auto& r) { return n.time >= r.first && n.time <= r.second; });
                c.espectrais += n.timbre == Timbre::spectral;
                c.naoSaoAtaques += std::none_of(a.onsets().begin(), a.onsets().end(),
                                                [&](const Onset& on) { return std::abs(on.time - n.source) < 1e-9; });
                // longe das fronteiras: o alvo olha ±0,75 s
                const double r = std::fmod(n.time, SECAO);
                if (r < 1.0 || r > SECAO - 1.0)
                    continue;
                ++c.notas;
                c.mesmoTrecho += claro(n.time) == claro(n.source);
            }
        }
        return c;
    };
    const auto seguir = contar(Mode::follow), contrapor = contar(Mode::counter), transfigurar = contar(Mode::transfigure);
    std::printf("unidades do mesmo trecho que o momento: seguir %d/%d · contrapor %d/%d · transfigurar %d/%d\n",
                seguir.mesmoTrecho, seguir.notas, contrapor.mesmoTrecho, contrapor.notas,
                transfigurar.mesmoTrecho, transfigurar.notas);
    CHECK(seguir.notas > 40 && contrapor.notas > 40 && transfigurar.notas > 40);
    CHECK(seguir.mesmoTrecho * 10 >= seguir.notas * 9);
    // contrapor inverte energia e ruído; desde a planura em 50 Hz–10 kHz
    // (analisador v4) o ruído separa menos os dois trechos deste teste
    CHECK(contrapor.mesmoTrecho * 100 <= contrapor.notas * 15);
    // transfigurar inverte só o brilho: energia e ruído ainda puxam para o
    // mesmo trecho, e o sorteio entre as k mais próximas deixa passar algumas
    CHECK(transfigurar.mesmoTrecho * 5 <= transfigurar.notas);
    // cada nota é um ataque da referência, fora das passagens do oráculo
    CHECK(seguir.naoSaoAtaques == 0 && contrapor.naoSaoAtaques == 0);
    CHECK(seguir.todas > 0 && seguir.foraDoOraculo == seguir.todas && seguir.espectrais == seguir.todas);
    {
        int total = 0;
        for (std::uint64_t s = 1; s <= 8; ++s)
        {
            Score sc = compose(a.frames(), a.onsets(), 60.0, opcoes(Mode::follow, s, 0.8f, false));
            const auto antes = sc.notes.size();
            descriptorMosaic(sc, a.frames(), a.onsets(), opcoes(Mode::follow, s, 0.8f));
            total += static_cast<int>(sc.notes.size() - antes);
        }
        CHECK(total > 0);
    }

    // 3. regras: a família espectral proibida → o mosaico usa outra
    {
        std::vector<std::string> e;
        const auto r = parseConstraints("proibido timbre espectral\n", e);
        const auto c = contar(Mode::follow, r);
        CHECK(e.empty() && c.notas > 0 && c.espectrais == 0);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("mosaico: todas as checagens passaram");
    return EXIT_SUCCESS;
}
