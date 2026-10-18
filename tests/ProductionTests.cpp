// A produção (src/core/Production.h): a reverberação de envio tem o tempo
// pedido (RT60 medido pela integração de Schroeder, ±10 %) e a cauda
// descorrelacionada entre os canais (a largura de uma sala); o barramento
// do baixo não perde o grave, o da harmonia perde o que está abaixo do corte.
#include "Production.h"
#include "Composer.h"
#include "ScoreSynth.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace trioio;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static void setThreads(const char* v)
{
#ifdef _WIN32
    _putenv_s("TRIOIO_THREADS", v);
#else
    if (*v)
        setenv("TRIOIO_THREADS", v, 1);
    else
        unsetenv("TRIOIO_THREADS");
#endif
}

int main()
{
    const double sr = 48000.0;
    const auto N = static_cast<std::size_t>(6.0 * sr);
    for (int e : { 0, 3, 8, 10, 11 })
    {
        const auto room = roomForEnsemble(e);
        std::vector<float> sL(N, 0.0f), sR(N, 0.0f), L(N, 0.0f), R(N, 0.0f);
        sL[100] = sR[100] = 1.0f;
        reverb(sL, sR, sr, room, L, R);
        std::vector<double> E(N);
        double acc = 0.0;
        for (std::size_t i = N; i-- > 0;)
        {
            acc += 0.5 * (L[i] * L[i] + R[i] * R[i]);
            E[i] = acc;
        }
        const auto tAt = [&](double db) {
            for (std::size_t i = 0; i < N; ++i)
                if (10.0 * std::log10(E[i] / E[0] + 1e-30) < db)
                    return static_cast<double>(i) / sr;
            return static_cast<double>(N) / sr;
        };
        const double rt = (tAt(-25.0) - tAt(-5.0)) * 3.0;
        double l2 = 0.0, r2 = 0.0, lr = 0.0;
        for (auto i = static_cast<std::size_t>(0.1 * sr); i < static_cast<std::size_t>(1.0 * sr); ++i)
        {
            l2 += L[i] * L[i];
            r2 += R[i] * R[i];
            lr += L[i] * R[i];
        }
        const double corr = lr / std::sqrt(l2 * r2 + 1e-30);
        std::printf("%-18s RT60 %.2f s (pedido %.1f), correlação da cauda %+.2f\n", room.name, rt, room.rt60, corr);
        CHECK(std::abs(rt - room.rt60) < 0.1 * room.rt60);
        CHECK(std::abs(corr) < 0.15);
    }
    // os barramentos: um seno de 60 Hz passa no do baixo e cai no da harmonia
    {
        const auto room = roomForEnsemble(0);
        const auto energia = [&](int group) {
            std::vector<float> bL(48000), bR(48000), L(48000, 0.0f), R(48000, 0.0f), sL(48000, 0.0f), sR(48000, 0.0f);
            for (std::size_t i = 0; i < bL.size(); ++i)
                bL[i] = bR[i] = static_cast<float>(0.1 * std::sin(2.0 * 3.141592653589793 * 60.0 * i / sr));
            mixBus(group, bL, bR, sr, room, L, R, sL, sR);
            double e = 0.0;
            for (std::size_t i = 24000; i < L.size(); ++i)
                e += L[i] * L[i];
            return e;
        };
        const double baixo = energia(1), harmonia = energia(2);
        std::printf("60 Hz: baixo %.3g, harmonia %.3g\n", baixo, harmonia);
        CHECK(harmonia < 0.2 * baixo);
    }
    // a humanização: o tempo muda pouco (≤ 9 ms; ≤ 3 ms na percussão), a
    // nota repetida alterna a força, a afinação fina só nos sem afinação fixa
    {
        using namespace trioio::composition;
        Score s;
        s.duration = 10.0;
        for (int k = 0; k < 16; ++k)
        {
            s.notes.push_back({ k * 0.5, 0.4, 60.0f, 0.0f, 0.6f, 0, Timbre::piano });
            s.notes.push_back({ k * 0.5, 0.4, 67.0f, 0.0f, 0.6f, 2, Timbre::bowed });
            s.notes.push_back({ k * 0.5, 0.1, 40.0f, 0.0f, 0.6f, 4, Timbre::kick });
        }
        const auto antes = s;
        const auto line = humanize(s, 3);
        std::printf("%s\n", line.c_str());
        bool tempoOk = true, pianoAfinado = true, arcoMexe = false;
        int alterna = 0;
        for (std::size_t i = 0; i < s.notes.size(); ++i)
        {
            const auto& a = antes.notes[i];
            const auto& d = s.notes[i];
            const double lim = d.voice == 4 ? 0.0031 : 0.0091;
            tempoOk = tempoOk && std::abs(d.time - a.time) <= lim;
            if (d.timbre == Timbre::piano)
                pianoAfinado = pianoAfinado && d.pitch == a.pitch;
            if (d.timbre == Timbre::bowed)
                arcoMexe = arcoMexe || d.pitch != a.pitch;
        }
        for (std::size_t i = 3; i < s.notes.size(); i += 3)
            alterna += s.notes[i].velocity != s.notes[i - 3].velocity ? 1 : 0;
        CHECK(tempoOk);
        CHECK(pianoAfinado);
        CHECK(arcoMexe);
        CHECK(alterna > 8);
    }
    // a expressão: numa frase de notas iguais, o meio fica mais forte que as
    // pontas; o final alarga (o ritardando) e a última nota segura (a fermata)
    {
        using namespace trioio::composition;
        Score s;
        s.duration = 20.0;
        for (int k = 0; k < 40; ++k)
            s.notes.push_back({ k * 0.5, 0.45, 60.0f, 0.0f, 0.6f, 0, Timbre::piano });
        const auto antes = s;
        const auto line = applyExpression(s);
        std::printf("%s\n", line.c_str());
        // o arco: a nota a 60 % da frase mais forte que a primeira e a última
        const float v0 = s.notes[0].velocity, vMeio = s.notes[23].velocity, vFim = s.notes[39].velocity;
        CHECK(vMeio > v0 && vMeio > vFim);
        // o ritardando: os intervalos do fim maiores que os do começo
        const double passoIni = s.notes[1].time - s.notes[0].time;
        const double passoFim = s.notes[39].time - s.notes[38].time;
        CHECK(passoFim > passoIni * 1.15);
        // a fermata e a duração da peça
        CHECK(s.notes[39].duration > antes.notes[39].duration * 1.5);
        CHECK(s.duration > antes.duration);
        // sem o final (o modo de linhas), nada alarga
        auto s2 = antes;
        (void) applyExpression(s2, false);
        CHECK(s2.notes[39].time == antes.notes[39].time && s2.duration == antes.duration);
    }
    // a síntese em paralelo dá o mesmo que a sequencial, amostra a amostra
    {
        trioio::composition::Score sc;
        sc.duration = 3.0;
        for (int v = 0; v < 7; ++v)
            for (int k = 0; k < 6; ++k)
                sc.notes.push_back({ k * 0.5, 0.4, 48.0f + static_cast<float>(v * 3 + k), 0.0f, 0.6f, v,
                                     v % 2 ? trioio::composition::Timbre::additive : trioio::composition::Timbre::breath });
        std::vector<float> a1, b1, a3, b3;
        setThreads("1");
        trioio::renderScore(sc, 22050.0, a1, b1);
        setThreads("3");
        trioio::renderScore(sc, 22050.0, a3, b3);
        setThreads("");
        float d = 0.0f;
        for (std::size_t i = 0; i < a1.size(); ++i)
            d = std::max({ d, std::abs(a1[i] - a3[i]), std::abs(b1[i] - b3[i]) });
        std::printf("paralelo × sequencial: diferença máxima %g\n", static_cast<double>(d));
        CHECK(a1.size() == a3.size() && d == 0.0f);
    }
    // os stems: a mistura com stems é a mesma (amostra a amostra); um stem por
    // voz que soou, mais a sala; a soma deles acompanha a mistura (só a cola
    // de barramento fica de fora)
    {
        trioio::composition::Score sc;
        sc.duration = 4.0;
        for (int k = 0; k < 8; ++k)
        {
            sc.notes.push_back({ k * 0.5, 0.4, 60.0f + static_cast<float>(k), 0.0f, 0.6f, 0, trioio::composition::Timbre::additive });
            sc.notes.push_back({ k * 0.5, 0.45, 36.0f, 0.0f, 0.6f, 1, trioio::composition::Timbre::additive });
        }
        std::vector<float> L0, R0, L1, R1;
        trioio::renderScore(sc, 22050.0, L0, R0);
        std::vector<trioio::Stem> st;
        trioio::renderScore(sc, 22050.0, L1, R1, &st);
        float d = 0.0f;
        for (std::size_t i = 0; i < L0.size(); ++i)
            d = std::max(d, std::abs(L0[i] - L1[i]));
        CHECK(d < 1e-6f);
        CHECK(st.size() == 3 && st[0].name == "1-motivo" && st[1].name == "2-baixo" && st[2].name == "9-sala");
        double xy = 0.0, xx = 0.0, yy = 0.0;
        for (std::size_t i = 0; i < L1.size(); ++i)
        {
            double sum = 0.0;
            for (const auto& s : st)
                sum += s.left[i];
            xy += sum * L1[i];
            xx += sum * sum;
            yy += static_cast<double>(L1[i]) * L1[i];
        }
        const double corr = xy / std::sqrt(xx * yy + 1e-30);
        std::printf("stems: %zu, correlação da soma com a mistura %.4f, nível %.2f dB\n", st.size(), corr, 10.0 * std::log10(xx / yy));
        CHECK(corr > 0.98 && std::abs(10.0 * std::log10(xx / yy)) < 3.0);
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("produção: todas as checagens passaram");
    return EXIT_SUCCESS;
}
