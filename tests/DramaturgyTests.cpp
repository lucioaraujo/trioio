// A dramaturgia (src/core/Dramaturgy.h): as curvas por modo, o clímax, a
// densidade que rareia só os enfeites, o silêncio composto antes do clímax.
#include "Dramaturgy.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace trioio::composition;
using namespace trioio::perception;
using trioio::strategy::Mode;

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
    // uma referência de 120 s que cresce até 80 s e cai
    const double dur = 120.0;
    std::vector<Frame> frames;
    for (double t = 0.0; t < dur; t += 0.05)
    {
        Frame f;
        f.time = t;
        f.rmsDb = static_cast<float>(t < 80.0 ? -45.0 + 35.0 * t / 80.0 : -10.0 - 35.0 * (t - 80.0) / 40.0);
        f.silent = false;
        frames.push_back(f);
    }
    const auto seguir = tensionCurve(frames, dur, Mode::follow, 1);
    const auto contra = tensionCurve(frames, dur, Mode::counter, 1);
    const auto arco = tensionCurve(frames, dur, Mode::transfigure, 1);
    const auto cortes = tensionCurve(frames, dur, Mode::interrupt, 1);
    std::printf("clímax: seguir %.1f s · contrapor %.1f · arco %.1f · interromper %.1f\n", seguir.climax, contra.climax, arco.climax,
                cortes.climax);
    CHECK(std::abs(seguir.climax - 80.0) < 6.0);              // o pico da referência
    CHECK(seguir.at(10.0) < 0.3f && seguir.at(80.0) > 0.9f);
    CHECK(contra.at(10.0) > 0.7f && contra.at(80.0) < 0.1f);  // invertida
    CHECK(std::abs(arco.climax - 0.618 * dur) < 1.0);         // a seção áurea
    int baixos = 0;
    for (float v : cortes.value)
        baixos += std::abs(v - 0.1f) < 1e-6f;
    CHECK(baixos * cortes.step >= 8.0);                       // ≥ 2 cortes de ≥ 4 s

    // a partitura: motivo, baixo, eco, memória, percussão espalhados
    Score sc;
    sc.duration = dur;
    for (int k = 0; k < 480; ++k)
    {
        Note n;
        n.time = k * 0.25;
        n.duration = 0.2;
        n.velocity = 0.5f;
        n.voice = k % 6 == 0 ? 0 : k % 6 == 1 ? 1 : k % 6 == 2 ? 2 : k % 6 == 3 ? 3 : k % 6 == 4 ? 4 : 5;
        sc.notes.push_back(n);
    }
    Pulse p;
    p.period = 0.5;
    p.phase = 0.0;
    auto a = sc;
    const auto linha = applyDramaturgy(a, seguir, p, 7);
    std::printf("%s\n", linha.c_str());
    CHECK(linha.find("respiração") != std::string::npos);
    int motivo = 0, perc = 0, baixo = 0, enfeitesNoVale = 0, enfeitesAntes = 0;
    for (const auto& n : a.notes)
    {
        motivo += n.voice == 0;
        baixo += n.voice == 1;
        perc += n.voice == 4;
        if ((n.voice == 2 || n.voice == 3 || n.voice == 5) && n.time < 15.0)
            ++enfeitesNoVale;
    }
    for (const auto& n : sc.notes)
        if ((n.voice == 2 || n.voice == 3 || n.voice == 5) && n.time < 15.0)
            ++enfeitesAntes;
    // a respiração tira um tempo de tudo; fora dela, motivo/baixo/percussão ficam
    CHECK(motivo >= 78 && baixo >= 78 && perc >= 78);
    CHECK(enfeitesNoVale < enfeitesAntes * 0.8);  // os enfeites rareiam no vale do começo
    // a respiração: nenhuma nota começa no tempo antes do clímax
    const double tc = std::round(seguir.climax / 0.5) * 0.5;
    for (const auto& n : a.notes)
        CHECK(!(n.time >= tc - 0.5 && n.time < tc));
    // a dinâmica: mais forte no clímax que no vale
    float vVale = 0.0f, vPico = 0.0f;
    for (const auto& n : a.notes)
    {
        if (n.voice == 0 && n.time < 5.0) vVale = n.velocity;
        if (n.voice == 0 && n.time > 70.0 && n.time < 86.0) vPico = std::max(vPico, n.velocity);  // a respiração tirou a nota do próprio clímax
    }
    std::printf("força no vale %.3f, no pico %.3f\n", vVale, vPico);
    CHECK(vPico > vVale + 0.1f);
    // o registro: o motivo uma oitava acima perto do clímax, abaixo no vale do começo
    {
        Score r;
        r.duration = dur;
        for (int k = 0; k < 240; ++k)
        {
            Note n;
            n.time = k * 0.5;
            n.pitch = 64.0f;
            n.voice = 0;
            r.notes.push_back(n);
        }
        (void) applyDramaturgy(r, seguir, p, 3);
        float noClimax = 0.0f, noVale = 0.0f;
        for (const auto& n : r.notes)
        {
            if (std::abs(n.time - seguir.climax) < 2.0) noClimax = n.pitch;
            if (n.time < 3.0) noVale = n.pitch;
        }
        std::printf("registro: clímax %.0f, vale %.0f\n", noClimax, noVale);
        CHECK(noClimax == 76.0f && noVale == 52.0f);
    }
    // determinístico
    auto b = sc;
    CHECK(applyDramaturgy(b, seguir, p, 7) == linha && b.notes.size() == a.notes.size());
    // peça curta: sem respiração
    Score curta = sc;
    curta.duration = 30.0;
    CHECK(applyDramaturgy(curta, seguir, p, 7).find("respiração") == std::string::npos);

    // o gesto do autor: a curva interpolada, o clímax no ponto mais alto, a
    // leitura do texto; e na partitura, o gesto que sobe deixa o fim mais forte
    {
        std::vector<std::pair<float, float>> g;
        CHECK(parseGesture("0:0.1, 0.7:1, 1:0.4", g) && g.size() == 3);
        CHECK(!parseGesture("0:0.1", g) && !parseGesture("0:2,1:0", g) && !parseGesture("abc", g));
        CHECK(parseGesture("0:0.1,0.7:1,1:0.4", g));
        const auto c = gestureCurve(g, 100.0);
        std::printf("gesto: %.2f no começo, %.2f aos 70 s, %.2f no fim, clímax %.1f s\n", static_cast<double>(c.at(0.0)),
                    static_cast<double>(c.at(70.0)), static_cast<double>(c.at(99.9)), c.climax);
        CHECK(c.shape == "o gesto do autor" && std::abs(c.at(70.0) - 1.0f) < 0.02f && c.at(0.0) < 0.15f && std::abs(c.at(99.9) - 0.4f) < 0.03f);
        CHECK(std::abs(c.climax - 70.0) < 1.0);
        Score sc;
        sc.duration = 60.0;
        for (int k = 0; k < 120; ++k)
            sc.notes.push_back({ k * 0.5, 0.4, 60.0f, 0.0f, 0.5f, 0, Timbre::additive });
        std::vector<std::pair<float, float>> up;
        CHECK(parseGesture("0:0,1:1", up));
        trioio::perception::Pulse pulse;
        pulse.bpm = 120.0;
        pulse.period = 0.5;
        (void) applyDramaturgy(sc, gestureCurve(up, 60.0), pulse, 1);
        double first = 0.0, last = 0.0;
        int nf = 0, nl = 0;
        for (const auto& n : sc.notes)
            (n.time < 20.0 ? (first += n.velocity, ++nf) : n.time >= 40.0 ? (last += n.velocity, ++nl) : 0);
        std::printf("gesto que sobe: força média %.2f no começo, %.2f no fim\n", first / std::max(1, nf), last / std::max(1, nl));
        CHECK(nf > 0 && nl > 0 && last / nl > first / nf + 0.05);
    }
    // as gramáticas narrativas: as formas, a mistura de duas e a virada
    {
        CHECK(isNarrativeGrammar("freytag") && isNarrativeGrammar("ondas+plato") && !isNarrativeGrammar("sonata") && !isNarrativeGrammar(""));
        const auto f = narrativeCurve("freytag", 100.0);
        CHECK(f.at(10.0) < 0.35f && f.at(55.0) > 0.95f && f.at(98.0) < 0.3f && std::abs(f.climax - 50.0) < 1.0);
        const auto k = narrativeCurve("kishotenketsu", 100.0);
        CHECK(k.at(60.0) > 0.85f && k.at(10.0) < 0.45f);
        // a mistura: perto do começo, a primeira manda; perto do fim, a segunda
        const auto m = narrativeCurve("freytag+plato", 100.0);
        CHECK(std::abs(m.at(1.0) - f.at(1.0)) < 0.05f && std::abs(m.at(99.0) - narrativeCurve("plato", 100.0).at(99.0)) < 0.05f);
        // a virada do ki-shō-ten-ketsu: o motivo uma oitava acima e no timbre do contracanto
        Score sc;
        sc.duration = 40.0;
        for (int i = 0; i < 40; ++i)
        {
            sc.notes.push_back({ i * 1.0, 0.5, 60.0f, 0.0f, 0.6f, 0, Timbre::sax });
            sc.notes.push_back({ i * 1.0, 0.5, 55.0f, 0.0f, 0.6f, 2, Timbre::trumpet });
            sc.notes.push_back({ i * 1.0, 0.1, 36.0f, 0.0f, 0.6f, 4, Timbre::kick });
        }
        const auto line = applyNarrativeEvents(sc, "kishotenketsu", 1);
        std::printf("%s\n", line.c_str());
        int virada = 0, perc = 0;
        for (const auto& n : sc.notes)
        {
            if (n.voice == 0 && n.time >= 20.0 && n.time < 30.0)
                virada += (n.pitch == 72.0f && n.timbre == Timbre::trumpet) ? 1 : 0;
            if (n.voice == 4 && n.time >= 20.0 && n.time < 30.0)
                ++perc;
        }
        CHECK(virada == 10 && perc < 10);
        Score outra = sc;
        CHECK(applyNarrativeEvents(outra, "freytag", 1).empty());
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("dramaturgia: todas as checagens passaram");
    return EXIT_SUCCESS;
}
