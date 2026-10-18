// O diálogo (src/core/Dialogue.h): as frases e as pausas da referência, a
// resposta que cala nos pedidos e fala nas pausas, e a alternância na música
// contínua.
#include "Dialogue.h"

#include <cmath>
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
    // frases de 4 s e pausas de 2 s, 60 s
    const double dur = 60.0;
    std::vector<Frame> frases;
    for (double t = 0.0; t < dur; t += 0.0213)
    {
        Frame f;
        f.time = t;
        f.rmsDb = std::fmod(t, 6.0) < 4.0 ? -15.0f : -55.0f;
        frases.push_back(f);
    }
    Pulse p;
    p.period = 0.5;
    p.beatsPerBar = 4;
    const auto m = dialogueMap(frases, dur, p);
    std::printf("frases: %zu pedidos, %zu respostas, alternância %d\n", m.calls.size(), m.answers.size(), m.alternating);
    CHECK(!m.alternating);
    CHECK(m.answers.size() == 10);
    CHECK(std::abs(m.answers[0].first - 4.0) < 0.2 && std::abs(m.answers[0].second - 6.0) < 0.2);
    CHECK(std::abs(m.calls[1].first - 6.0) < 0.2);

    // a partitura: uma nota de cada voz a cada 0,5 s
    Score sc;
    for (int k = 0; k < 120; ++k)
        for (int v = 0; v <= 6; ++v)
        {
            Note n;
            n.time = k * 0.5;
            n.duration = v == 6 ? 3.0 : 0.4;
            n.velocity = 0.5f;
            n.voice = v;
            sc.notes.push_back(n);
        }
    auto a = sc;
    const auto linha = applyDialogue(a, m);
    std::printf("%s\n", linha.c_str());
    CHECK(!linha.empty());
    bool liderNoPedido = false, acompanhaNoPedido = false, liderNaResposta = false, cortada = false;
    for (const auto& n : a.notes)
    {
        const double ph = std::fmod(n.time, 6.0);
        const bool pedido = ph > 0.3 && ph < 3.7;  // longe das bordas (a suavização move a borda ~0,1 s)
        const bool pausa = ph > 4.3 && ph < 5.7;
        const bool lider = n.voice == 0 || n.voice == 2 || n.voice == 3 || n.voice == 5;
        if (pedido && lider) liderNoPedido = true;
        if (pedido && n.voice == 1 && std::abs(n.velocity - 0.3f) < 1e-4f) acompanhaNoPedido = true;
        if (pausa && lider && std::abs(n.velocity - 0.55f) < 1e-4f) liderNaResposta = true;
        // a harmonia de 3 s que começa na pausa (5 s) cede a vez em 6 s
        if (n.voice == 6 && std::abs(n.time - 5.0) < 1e-6 && n.time + n.duration <= 6.0 + 0.2) cortada = true;
    }
    CHECK(!liderNoPedido);       // as vozes que conduzem escutam
    CHECK(acompanhaNoPedido);    // o baixo acompanha a 60 %
    CHECK(liderNaResposta);      // e falam nas pausas, 10 % mais
    CHECK(cortada);

    // música contínua: alternância de 2 compassos (4 s a 120 BPM em 4)
    std::vector<Frame> continua;
    for (double t = 0.0; t < dur; t += 0.0213)
    {
        Frame f;
        f.time = t;
        f.rmsDb = static_cast<float>(-18.0 + 2.0 * std::sin(t));
        continua.push_back(f);
    }
    const auto c = dialogueMap(continua, dur, p);
    std::printf("contínua: alternância %d, %zu pedidos, %zu respostas\n", c.alternating, c.calls.size(), c.answers.size());
    CHECK(c.alternating);
    CHECK(c.answers.size() >= 7 && std::abs((c.answers[0].second - c.answers[0].first) - 4.0) < 1e-6);

    // 4. a resposta retoma o gesto: frases de Dó Ré Mi Fá Sol Lá Si (ataques
    //    a cada 0,5 s, de 0,5 a 3,5 s de cada frase de 4 s); a resposta (as
    //    pausas de 2 s) toca a cauda que cabe, na escala dela (Dó maior)
    {
        using trioio::strategy::Mode;
        const float escala[] = { 60, 62, 64, 65, 67, 69, 71 };
        std::vector<Frame> fr = frases;
        std::vector<Onset> ataques;
        for (int f = 0; f < 10; ++f)
            for (int k = 0; k < 7; ++k)
                ataques.push_back({ f * 6.0 + 0.5 + k * 0.5, 1.0f });
        for (auto& x : fr)
        {
            const double ph = std::fmod(x.time, 6.0);
            if (ph >= 0.45 && ph < 3.95)
            {
                const int k = std::min(6, static_cast<int>((ph - 0.45) / 0.5));
                x.f0Hz = static_cast<float>(440.0 * std::pow(2.0, (escala[k] - 69.0) / 12.0));
                x.periodicity = 0.9f;
                x.silent = false;  // o padrão do Frame é silêncio, e pitchAt ignora o silêncio
            }
        }
        Score base;
        for (int k = 0; k < 120; ++k)
        {
            Note n;
            n.time = k * 0.5;
            n.duration = 0.4;
            n.pitch = escala[(k * 3) % 7] + 12.0f;  // o motivo da resposta, em Dó maior, sem ordem (não sobe sozinho)
            n.voice = 0;
            base.notes.push_back(n);
        }
        const auto mp = dialogueMap(fr, dur, p);
        const auto alturasNa = [](const Score& s, double a, double b) {
            std::vector<float> v;
            for (const auto& n : s.notes)
                if (n.voice == 0 && n.time >= a && n.time < b)
                    v.push_back(n.pitch);
            return v;
        };
        auto segue = base;
        const auto ls = imitateCalls(segue, mp, fr, ataques, Mode::follow);
        std::printf("%s\n", ls.c_str());
        CHECK(!ls.empty());
        const auto r1 = alturasNa(segue, mp.answers[1].first, mp.answers[1].second);
        std::printf("resposta 2 (seguir):");
        for (float x : r1) std::printf(" %.0f", x);
        std::printf("\n");
        CHECK(r1.size() >= 3);
        bool sobe = r1.size() >= 3;
        for (std::size_t i = 1; i < r1.size(); ++i)
            sobe = sobe && r1[i] > r1[i - 1];
        CHECK(sobe);  // a cauda Mi Fá Sol Lá Si sobe
        for (float x : r1)
        {
            const int pc = static_cast<int>(std::lround(x)) % 12;
            CHECK(pc == 0 || pc == 2 || pc == 4 || pc == 5 || pc == 7 || pc == 9 || pc == 11);  // na escala da resposta
        }
        auto inverte = base;
        (void) imitateCalls(inverte, mp, fr, ataques, Mode::transfigure);
        const auto r2 = alturasNa(inverte, mp.answers[1].first, mp.answers[1].second);
        bool desce = r2.size() >= 3;
        for (std::size_t i = 1; i < r2.size(); ++i)
            desce = desce && r2[i] < r2[i - 1];
        CHECK(desce);  // transfigurar inverte o contorno
        auto corta = base;
        (void) imitateCalls(corta, mp, fr, ataques, Mode::interrupt);
        CHECK(alturasNa(corta, mp.answers[1].first, mp.answers[1].second).size() < r1.size());  // o fragmento
        // sem altura no pedido: nada muda
        auto mudo = base;
        CHECK(imitateCalls(mudo, mp, frases, ataques, Mode::follow).empty());
    }

    // a troca de lugar (princípio 4): pedidos de 4 s e respostas de 2 s numa
    // peça de 60 s
    {
        DialogueMap m;
        for (double t = 0.0; t < 60.0; t += 6.0)
        {
            m.calls.push_back({ t, t + 4.0 });
            m.answers.push_back({ t + 4.0, t + 6.0 });
        }
        bool troca = false, semVez = false;
        for (std::uint64_t sd = 1; sd <= 20; ++sd)
        {
            const auto sw = chooseDialogueSwap(m, 60.0, trioio::strategy::Mode::follow, sd);
            CHECK(sw.kind != 0 && sw.from >= 0.35 * 60.0 - 1e-6 && sw.to <= 0.8 * 60.0 + 1e-6);
            troca = troca || sw.kind == 1;
            semVez = semVez || sw.kind == 2;
            const auto x = swapDialogue(m, sw);
            double fala = 0.0, escuta = 0.0;
            for (const auto& a : x.answers)
                fala += a.second - a.first;
            for (const auto& c : x.calls)
                escuta += c.second - c.first;
            if (sw.kind == 1)
            {
                // no trecho, a resposta fala onde a referência fala (4 s) e escuta nas pausas
                bool inv = false;
                for (const auto& a : x.answers)
                    inv = inv || (a.first >= sw.from && std::abs((a.second - a.first) - 4.0) < 1e-6);
                CHECK(inv && std::abs(fala + escuta - 60.0) < 1e-6);
            }
            else
                CHECK(escuta < 40.0 - 1e-6);  // os pedidos do trecho sumiram
        }
        CHECK(troca && semVez);
        // interromper: sempre o atropelo
        CHECK(chooseDialogueSwap(m, 60.0, trioio::strategy::Mode::interrupt, 3).kind == 2);
        // poucas frases: nada
        DialogueMap pouco;
        pouco.calls = { { 0.0, 4.0 } };
        pouco.answers = { { 4.0, 6.0 } };
        CHECK(chooseDialogueSwap(pouco, 60.0, trioio::strategy::Mode::follow, 1).kind == 0);
        CHECK(!dialogueSwapText(chooseDialogueSwap(m, 60.0, trioio::strategy::Mode::counter, 1)).empty());
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("diálogo: todas as checagens passaram");
    return EXIT_SUCCESS;
}
