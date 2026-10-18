// O banco de escalas (core/ScaleBank.h, docs/ESCALAS.md): o catálogo do
// Studio, o ajuste ao campo da referência, as escalas inventadas (sempre
// fora do catálogo), o crivo em EDO fina, as afinações e o uso pelo
// compositor.

#include "Composer.h"
#include "ScaleBank.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
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

int main()
{
    // 1. o catálogo: 45 escalas, graus crescentes começando na tônica
    CHECK(scaleCatalogSize() == 45);
    std::set<std::vector<float>> distintas;
    for (int i = 0; i < scaleCatalogSize(); ++i)
    {
        const auto s = scaleCatalogSteps(i);
        CHECK(!s.empty() && s.front() == 0.0f && std::is_sorted(s.begin(), s.end()) && s.back() < 12.0f);
        distintas.insert(s);
    }
    CHECK(static_cast<int>(distintas.size()) == scaleCatalogSize());  // nenhuma duplicada
    std::set<std::vector<float>> catalogo(distintas.begin(), distintas.end());

    // 2. o ajuste ao campo: um campo de Sol menor harmônica (Sol Lá Sib Dó
    //    Ré Mib Fá#) escolhe uma escala que cobre essas notas, sobre Sol
    {
        std::array<float, 12> campo {};
        for (int pc : { 7, 9, 10, 0, 2, 3, 6 })
            campo[static_cast<std::size_t>(pc)] = pc == 7 ? 0.25f : 0.125f;
        int sobreSol = 0, menorHarmonica = 0;
        for (std::uint64_t s = 1; s <= 30; ++s)
        {
            const auto c = fitCatalog(campo, s, 1);  // a melhor
            sobreSol += c.tonic == 7.0f;
            menorHarmonica += c.name.rfind("menor harmônica", 0) == 0;
        }
        std::printf("campo de Sol menor harmônica: %d/30 sobre Sol, %d/30 menor harmônica\n", sobreSol, menorHarmonica);
        CHECK(sobreSol == 30 && menorHarmonica == 30);
        // entre as 3 melhores: variedade, sempre cobrindo o campo quase inteiro
        std::set<std::string> nomes;
        for (std::uint64_t s = 1; s <= 30; ++s)
            nomes.insert(fitCatalog(campo, s, 3).name);
        CHECK(nomes.size() >= 2 && nomes.size() <= 3);
    }

    // 3. inventadas: 300 seeds, nenhuma no catálogo, 4 a 9 notas, reprodutíveis
    {
        std::set<std::vector<float>> vistas;
        int noCatalogo = 0, foraDoTamanho = 0;
        for (std::uint64_t s = 1; s <= 300; ++s)
        {
            const auto c = inventedScale(s);
            noCatalogo += catalogo.count(c.steps) > 0;
            foraDoTamanho += c.steps.size() < 4 || c.steps.size() > 9;
            vistas.insert(c.steps);
            CHECK(c.steps == inventedScale(s).steps);
        }
        std::printf("inventadas: %zu coleções distintas em 300 seeds, %d no catálogo, %d fora de 4..9 notas\n",
                    vistas.size(), noCatalogo, foraDoTamanho);
        CHECK(noCatalogo == 0 && foraDoTamanho == 0 && vistas.size() >= 40);
    }

    // 4. crivo em EDO fina: graus fora dos 12 semitons, 5 a 9 por oitava
    {
        int foraDos12 = 0, tamanhoErrado = 0;
        std::set<std::string> edos;
        for (std::uint64_t s = 1; s <= 60; ++s)
        {
            const auto c = edoSieveScale(s);
            tamanhoErrado += c.steps.size() < 5 || c.steps.size() > 9;
            foraDos12 += std::any_of(c.steps.begin(), c.steps.end(), [](float x) { return std::abs(x - std::round(x)) > 0.05f; });
            edos.insert(c.name.substr(0, c.name.find('(')));
        }
        std::printf("crivo em EDO: %d/60 com graus fora dos 12 semitons, divisões usadas: %zu\n", foraDos12, edos.size());
        CHECK(tamanhoErrado == 0 && foraDos12 >= 50 && edos.size() == 3);
    }

    // 5. afinações: a terça maior justa (5/4) tem 386 cents; a pitagórica (81/64), 408
    {
        const auto j = retune({ 0, 4, 7 }, Tuning::justIntonation, 1);
        const auto p = retune({ 0, 4, 7 }, Tuning::pythagorean, 1);
        CHECK(std::abs(j[1] - 3.8631f) < 0.001f && std::abs(j[2] - 7.0196f) < 0.001f);
        CHECK(std::abs(p[1] - 4.0782f) < 0.001f);
        const auto q = retune({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, Tuning::quarterTone, 3);
        int quartos = 0;
        for (float x : q)
            quartos += std::abs(x - std::round(x) ) > 0.4f;
        CHECK(q.front() == 0.0f && quartos >= 2 && quartos <= 10);
    }

    // 6. no compositor: cada fonte de alturas compõe, marca o nome e toca
    //    alturas da escala
    {
        const int mel[] = { 67, 69, 70, 72, 74, 75, 78, 79 };
        std::vector<float> v(static_cast<std::size_t>(SR * 24.0));
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.5);
            const double f = 440.0 * std::pow(2.0, (mel[static_cast<int>(t / 0.5) % 8] - 69) / 12.0);
            v[i] = static_cast<float>(0.3 * std::exp(-tt * 4.0) * std::sin(2 * PI * f * t));
        }
        Analyzer a(SR);
        a.push(v.data(), v.size());
        a.finish();
        struct Caso { const char* nome; const char* marca; };
        for (const Caso c : { Caso { "catalogo", "sobre" }, Caso { "inventada", "crivo inventado" },
                              Caso { "edo", "-EDO" }, Caso { "justa", "afinação justa" },
                              Caso { "pitagorica", "pitagórica" }, Caso { "quartos", "quartos de tom" } })
        {
            PitchSource ps;
            CHECK(parsePitchSource(c.nome, ps));
            Options o { Mode::follow, 5, ps, TimeMode::automatic, 0.0f };
            o.mosaic = false;
            const auto sc = compose(a.frames(), a.onsets(), 24.0, o);
            const bool marcada = std::any_of(sc.marks.begin(), sc.marks.end(),
                                             [&](const Mark& m) { return m.text.find(c.marca) != std::string::npos; });
            const auto motivo = std::count_if(sc.notes.begin(), sc.notes.end(), [](const Note& n) { return n.voice == 0; });
            int fracionarias = 0;
            for (const auto& n : sc.notes)
                fracionarias += n.voice == 0 && std::abs(n.pitch - std::round(n.pitch)) > 0.05f;
            std::printf("  %-10s · %ld notas do motivo, %d fora dos 12 semitons · marca: %s\n", c.nome,
                        static_cast<long>(motivo), fracionarias, marcada ? "sim" : "NÃO");
            CHECK(marcada && motivo > 10);
            if (std::string(c.nome) == "catalogo" || std::string(c.nome) == "inventada")
                CHECK(fracionarias == 0);
            if (std::string(c.nome) == "edo" || std::string(c.nome) == "justa")
                CHECK(fracionarias > 0);
        }
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("banco de escalas: todas as checagens passaram");
    return EXIT_SUCCESS;
}
