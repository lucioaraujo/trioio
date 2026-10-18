// O groove da referência (src/core/Groove.h): medir o swing, o microtempo e o
// acento em ataques conhecidos, e tocar uma grade reta com eles.
#include "Groove.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

using namespace trioio::composition;
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
    Pulse p;
    p.period = 0.6;  // 100 BPM
    p.bpm = 100.0;
    p.phase = 0.0;
    p.beatsPerBar = 4;
    p.confidence = 0.6f;
    const double dur = 30.0;

    // 1. ternário (o contratempo em 2/3 do tempo) e a caixa 20 ms atrás nos 2 e 4
    std::vector<Onset> swung;
    for (int b = 0; b * p.period < dur; ++b)
    {
        const bool backbeat = b % 4 == 1 || b % 4 == 3;
        swung.push_back({ b * p.period + (backbeat ? 0.020 : 0.0), backbeat ? 1.0f : 0.8f });
        swung.push_back({ (b + 2.0 / 3.0) * p.period, 0.4f });
    }
    const auto g = measureGroove(swung, p, 0.0, dur);
    std::printf("swing medido %.3f (ternário: 0.167) · microtempo: tempo 1 %+.0f ms, tempo 2 %+.0f ms, tempo 4 %+.0f ms · acento tempo 2 %.2f, contratempo %.2f\n",
                g.swing8, g.offset[0] * 1000.0, g.offset[4] * 1000.0, g.offset[12] * 1000.0, g.accent[4], g.accent[2]);
    CHECK(g.valid);
    CHECK(std::abs(g.swing8 - 0.1667f) < 0.01f);
    CHECK(g.swing16 == 0.0f);
    CHECK(std::abs(g.offset[4] - 0.020f) < 0.004f && std::abs(g.offset[12] - 0.020f) < 0.004f);
    CHECK(g.offset[0] == 0.0f && g.offset[8] == 0.0f);
    CHECK(g.accent[4] > g.accent[0] && g.accent[0] > g.accent[2]);

    // 2. tocar colcheias retas com esse groove
    Score sc;
    for (int k = 0; k < 16; ++k)
    {
        Note n;
        n.time = k * p.period / 2.0;
        n.duration = 0.2;
        n.velocity = 0.5f;
        sc.notes.push_back(n);
    }
    Note livre;  // fora da grade: fica
    livre.time = 1.234;
    sc.notes.push_back(livre);
    auto inteira = sc;
    const auto linha = applyGroove(inteira, g, p, { { 0.0, dur } }, 1.0);
    std::printf("%s\n", linha.c_str());
    CHECK(!linha.empty());
    const auto em = [](const Score& s, double t) {
        for (const auto& n : s.notes)
            if (std::abs(n.time - t) < 1e-6)
                return true;
        return false;
    };
    CHECK(em(inteira, 0.0));                     // o tempo 1 fica
    CHECK(em(inteira, 0.6 * 2.0 / 3.0));         // o contratempo vai para 2/3
    CHECK(em(inteira, 0.6 + 0.020));             // o tempo 2 atrasa 20 ms
    CHECK(em(inteira, 1.234));                   // a nota fora da grade fica
    auto meia = sc;
    (void) applyGroove(meia, g, p, { { 0.0, dur } }, 0.5);
    CHECK(em(meia, 0.6 * (0.5 + 0.1667 * 0.5)) || em(meia, 0.6 * (0.5 + g.swing8 * 0.5)));
    auto nada = sc;
    CHECK(applyGroove(nada, g, p, { { 0.0, dur } }, 0.0).empty());
    CHECK(em(nada, 0.3));
    auto foraDoTrecho = sc;
    (void) applyGroove(foraDoTrecho, g, p, { { 20.0, 30.0 } }, 1.0);
    CHECK(em(foraDoTrecho, 0.3));  // o trecho com pulso não inclui o começo

    // 3. uma referência reta (colcheias e semicolcheias exatas): nada muda
    std::vector<Onset> reto;
    for (int k = 0; k * p.period / 4.0 < dur; ++k)
        if (k % 4 != 3)
            reto.push_back({ k * p.period / 4.0, k % 4 == 0 ? 1.0f : 0.5f });
    const auto gr = measureGroove(reto, p, 0.0, dur);
    std::printf("reto: swing %.3f · semicolcheia %.3f\n", gr.swing8, gr.swing16);
    CHECK(gr.swing8 == 0.0f && gr.swing16 == 0.0f);
    auto retoSc = sc;
    for (auto& a : retoSc.notes)
        a.velocity = 0.5f;
    const auto lr = applyGroove(retoSc, gr, p, { { 0.0, dur } }, 1.0);
    CHECK(em(retoSc, 0.3) && em(retoSc, 0.6));  // a grade não se move (no máximo o acento muda)
    (void) lr;

    // 4. ataques soltos (tempo expressivo, sem swing): o desvio aleatório não
    //    vira swing; e pulso fraco: nada
    {
        std::vector<Onset> solto;
        std::uint32_t x = 3;
        for (int k = 0; k * p.period / 2.0 < dur; ++k)
        {
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            const double jitter = (x / 4294967295.0 * 2.0 - 1.0) * 0.12 * p.period;  // ±12 % do tempo
            solto.push_back({ k * p.period / 2.0 + jitter, 0.6f });
        }
        const auto gs = measureGroove(solto, p, 0.0, dur);
        std::printf("solto: swing %.3f\n", gs.swing8);
        CHECK(gs.swing8 == 0.0f);
        Pulse fraco = p;
        fraco.confidence = 0.1f;
        CHECK(!measureGroove(swung, fraco, 0.0, dur).valid);
    }

    // 5. o rubato nas seções livres: o arco de frase (pontas mais lentas que o
    //    meio, extremos no lugar, notas de fora intactas)
    {
        Score r;
        for (int k = 0; k < 16; ++k)
        {
            Note n;
            n.time = 10.0 + k * 0.25;
            n.duration = 0.2;
            n.voice = 0;
            r.notes.push_back(n);
        }
        Note fora;
        fora.time = 2.0;
        fora.voice = 0;
        r.notes.push_back(fora);
        const auto linha = applyRubato(r, { { 9.0, 15.0 } }, 1.0);
        std::printf("%s\n", linha.c_str());
        CHECK(!linha.empty());
        std::vector<double> t;
        for (const auto& n : r.notes)
            if (n.time >= 9.0)
                t.push_back(n.time);
        CHECK(t.size() == 16);
        CHECK(std::abs(t.front() - 10.0) < 1e-9 && std::abs(t.back() - 13.75) < 1e-9);  // os extremos
        const double ponta = t[1] - t[0], meio = t[8] - t[7], fim = t[15] - t[14];
        std::printf("rubato: ponta %.3f s, meio %.3f s, fim %.3f s (antes, 0,25)\n", ponta, meio, fim);
        CHECK(ponta > meio * 1.15 && fim > meio * 1.15);
        CHECK(r.notes.front().time == 2.0);  // fora da seção livre: intacta
        Score reto = r;
        CHECK(applyRubato(reto, { { 9.0, 15.0 } }, 0.0).empty());  // contrapor: reto
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("groove: todas as checagens passaram");
    return EXIT_SUCCESS;
}
