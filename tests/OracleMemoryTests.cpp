// Memória performativa (core/OracleMemory.h, docs/MEMORIA.md): o oráculo de
// fatores reconhece os fatores da sequência, navega por continuidades reais
// da referência e entra na partitura só com imprevisibilidade.

#include "Composer.h"
#include "OracleMemory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
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

int main()
{
    // 1. o oráculo reconhece todo fator da sequência (a propriedade que o define)
    const std::vector<int> seq { 60, 62, 64, 60, 62, 67, 64, 60, 62, 64, 65, 67, 60, 62 };
    FactorOracle o;
    for (int s : seq)
        o.add(s);
    bool todos = true;
    for (std::size_t i = 0; i < seq.size(); ++i)
        for (std::size_t j = i + 1; j <= seq.size(); ++j)
        {
            int st = 0;
            for (std::size_t k = i; k < j && st >= 0; ++k)
                st = o.transition(st, seq[k]);
            todos = todos && st >= 0;
        }
    CHECK(todos);
    CHECK(o.transition(0, 99) < 0);   // símbolo que não está na sequência
    CHECK(o.suffix(0) == -1 && o.size() == static_cast<int>(seq.size()));
    // o link de sufixo aponta para um estado com o mesmo símbolo (contexto comum)
    bool sufixos = true;
    for (int s = 1; s <= o.size(); ++s)
        if (o.suffix(s) > 0)
            sufixos = sufixos && o.symbol(o.suffix(s)) == o.symbol(s);
    CHECK(sufixos);

    // 2. navegação: continuidade 1 = a ordem original; com saltos, os pares
    //    consecutivos continuam sendo pares que existem na referência
    {
        std::uint64_t r = 7;
        const auto uni = [&] {
            r += 0x9E3779B97F4A7C15ull;
            std::uint64_t x = r;
            x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
            x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
            return static_cast<double>((x ^ (x >> 31)) >> 11) / 9007199254740992.0;
        };
        int jumps = 0;
        const auto linear = o.navigate(0, 10, 1.0, uni, jumps);
        bool ordem = jumps == 0;
        for (std::size_t i = 0; i < linear.size(); ++i)
            ordem = ordem && linear[i] == static_cast<int>(i) + 1;
        CHECK(ordem);
        std::set<std::pair<int, int>> bigramas;
        for (std::size_t i = 0; i + 1 < seq.size(); ++i)
            bigramas.insert({ seq[i], seq[i + 1] });
        int pares = 0, reais = 0, saltos = 0;
        for (int rodada = 0; rodada < 50; ++rodada)
        {
            const auto p = o.navigate(static_cast<int>(uni() * o.size()), 12, 0.4, uni, jumps);
            saltos += jumps;
            for (std::size_t i = 0; i + 1 < p.size(); ++i)
            {
                ++pares;
                reais += bigramas.count({ o.symbol(p[i]), o.symbol(p[i + 1]) }) > 0;
            }
        }
        std::printf("navegação com saltos: %d saltos; %d de %d pares consecutivos existem na referência\n",
                    saltos, reais, pares);
        CHECK(saltos > 50);
        CHECK(reais * 10 >= pares * 9);
    }

    // 3. na partitura: só com imprevisibilidade; alturas da própria referência
    {
        // melodia com repetição de motivo (o oráculo tem o que reconhecer)
        const int mel[] = { 60, 62, 64, 67, 64, 62, 60, 62, 64, 69, 67, 64 };
        std::vector<float> v(static_cast<std::size_t>(SR * 30.0));
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.5);
            const int k = static_cast<int>(t / 0.5) % 12;
            const double f = 440.0 * std::pow(2.0, (mel[k] - 69) / 12.0);
            v[i] = static_cast<float>(0.3 * std::exp(-tt * 4.0) * (std::sin(2 * PI * f * t) + 0.3 * std::sin(4 * PI * f * t)));
        }
        Analyzer a(SR);
        a.push(v.data(), v.size());
        a.finish();
        const auto compor = [&](float u, Mode m, std::uint64_t s) {
            return compose(a.frames(), a.onsets(), 30.0, Options { m, s, PitchSource::reference, TimeMode::automatic, u });
        };
        const auto sem = compor(0.0f, Mode::follow, 1);
        CHECK(std::none_of(sem.notes.begin(), sem.notes.end(), [](const Note& n) { return n.voice == 3; }));
        // a memória antes do Terceiro Olho (que depois pode deformá-la, como
        // deforma o resto): só alturas da referência
        int passagens = 0, notas = 0, daReferencia = 0;
        for (std::uint64_t s = 1; s <= 8; ++s)
        {
            Score sc;
            sc.duration = 30.0;
            oracleMemory(sc, a.frames(), a.onsets(), Options { Mode::follow, s, PitchSource::reference, TimeMode::automatic, 0.7f });
            for (const auto& m : sc.marks)
                passagens += m.text.find("memória (oráculo)") != std::string::npos;
            for (const auto& n : sc.notes)
                if (n.voice == 3)
                {
                    ++notas;
                    const int p = static_cast<int>(std::lround(n.pitch));
                    daReferencia += std::find(std::begin(mel), std::end(mel), p) != std::end(mel)
                                    // a análise pode ler a oitava do 2º harmônico
                                    || std::find(std::begin(mel), std::end(mel), p - 12) != std::end(mel);
                }
        }
        std::printf("memória em 8 seeds: %d passagens, %d notas, %d com altura da referência\n", passagens, notas,
                    daReferencia);
        CHECK(passagens >= 8 && notas >= 40);
        CHECK(daReferencia == notas);
        // e na composição completa ela aparece, com a marca na forma
        const auto completa = compor(0.7f, Mode::follow, 3);
        CHECK(std::any_of(completa.notes.begin(), completa.notes.end(), [](const Note& n) { return n.voice == 3; }));
        // contrapor espelha: a mesma memória, invertida em torno da média
        const auto cp = compor(0.7f, Mode::counter, 1);
        CHECK(std::any_of(cp.marks.begin(), cp.marks.end(),
                          [](const Mark& m) { return m.text.find("espelhada") != std::string::npos; }));
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("memória performativa: todas as checagens passaram");
    return EXIT_SUCCESS;
}
