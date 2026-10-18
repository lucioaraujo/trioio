// Os ciclos rítmicos das culturas (core/RhythmCycles.h; docs/RITMO.md):
// o arquivo é legível e citado; o maqsum cai nas posições certas; o khālī
// do tīntāl fica sem grave; o gong do lancaran fecha o ciclo; a escala com
// comportamento chama o seu ciclo; --ciclo força um ciclo na peça.

#include "Composer.h"
#include "Descriptors.h"
#include "Structure.h"
#include "RhythmCycles.h"
#include "ScaleArchive.h"
#include "ScaleMachine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
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

static std::string ler(const char* caminho)
{
    std::ifstream in(caminho);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main()
{
    std::vector<std::string> erros;
    const auto ritmos = std::make_shared<const std::vector<RhythmCycle>>(
        parseRhythmArchive(ler(TRIOIO_SOURCE_DIR "/data/ritmos/arquivo_trioio_ritmos.txt"), erros));

    // 1. o arquivo
    CHECK(erros.empty() && ritmos->size() >= 25);
    CHECK(std::all_of(ritmos->begin(), ritmos->end(), [](const RhythmCycle& c) { return !c.source.empty(); }));
    const auto* tin = findCycle(*ritmos, "tīntāl");
    CHECK(tin && tin->strokes.size() == 16 && tin->empties == std::vector<int> { 9 } && tin->unitBeats == 1.0);
    CHECK(findCycle(*ritmos, "clave son") && !findCycle(*ritmos, "clave"));  // "clave" é ambíguo
    CHECK(findCycle(*ritmos, "sino padrão")->unitBeats < 0.34);              // 12/8: colcheia ternária

    // 2. o ciclo tocado, posição a posição (pulso de 0,5 s, fase 0)
    Pulse p;
    p.period = 0.5;
    p.phase = 0.0;
    p.bpm = 120.0;
    Options o;
    const auto tocar = [&](const char* nome) {
        Score sc;
        sc.duration = 9.0;
        (void) cultureRhythm(sc, { { 0.0, 8.0 } }, { findCycle(*ritmos, nome) }, p, o);
        return sc;
    };
    {
        // maqsum: D T - T D - T - em colcheias (0,25 s): D em 0 e 1,0; T em 0,25, 0,75 e 1,5
        const auto sc = tocar("maqsum");
        int certos = 0, total = 0;
        for (const auto& n : sc.notes)
        {
            ++total;
            const double r = std::fmod(n.time, 2.0);
            const bool grave = n.pitch < 50.0f;
            const auto perto = [&](double x) { return std::abs(r - x) < 1e-6; };
            certos += grave ? (perto(0.0) || perto(1.0)) : (perto(0.25) || perto(0.75) || perto(1.5));
        }
        std::printf("maqsum: %d de %d golpes nas posições do ciclo\n", certos, total);
        CHECK(total == 20 && certos == total);
    }
    {
        // tīntāl: o khālī se ouve pela falta do grave em tin tin tā tā
        // (tempos 10 a 13: 4,5 a 6,0 s); o sam acentuado
        const auto sc = tocar("tīntāl");
        bool graveNoKhali = false, samForte = false;
        for (const auto& n : sc.notes)
        {
            if (n.time > 4.4 && n.time < 6.1 && n.pitch < 50.0f)
                graveNoKhali = true;
            if (std::abs(n.time) < 1e-6 && n.pitch < 50.0f && n.velocity > 0.8f)
                samForte = true;
        }
        CHECK(!graveNoKhali && samForte);
    }
    {
        // lancaran: o gong no 16.º tempo (7,5 s)
        const auto sc = tocar("lancaran");
        CHECK(std::any_of(sc.notes.begin(), sc.notes.end(), [](const Note& n) { return std::abs(n.time - 7.5) < 1e-6 && n.pitch < 35.0f; }));
    }

    // 2b. a melodia chega no ciclo
    {
        // uma melodia de notas a cada 0,37 s, numa seção de 0 a 20 s, tônica Dó
        const auto melodia = [] {
            Score sc;
            sc.duration = 22.0;
            for (double t = 0.1; t < 20.0; t += 0.37)
            {
                Note n;
                n.time = t;
                n.duration = 0.3;
                n.pitch = 64.0f;
                n.voice = 0;
                sc.notes.push_back(n);
            }
            return sc;
        };
        SectionScale sec;
        sec.start = 0.0;
        sec.end = 20.0;
        sec.scale.tonic = 0.0;
        sec.scale.cents = { 0, 240, 480, 720, 960 };
        // lancaran (gong no 16.º tempo): chegadas em 7,5 e 15,5 s, no tom de apoio (960 c = Lá+60c)
        ScaleBehavior pathet;
        pathet.support = { 960.0 };
        auto g = melodia();
        const auto lg = alignMelodyToCycles(g, { sec }, { findCycle(*ritmos, "lancaran") }, { &pathet }, p);
        int noGong = 0;
        for (const auto& n : g.notes)
            if (n.voice == 0 && (std::abs(n.time - 7.5) < 1e-9 || std::abs(n.time - 15.5) < 1e-9))
                noGong += std::abs(std::fmod(100.0 * n.pitch, 1200.0) - 960.0) < 1.0;
        std::printf("lancaran: %s · %d chegadas no gong, no tom de apoio\n", lg.c_str(), noGong);
        CHECK(noGong == 2);
        // tīntāl (ciclo de 8 s): sam em 0, 8 e 16; o tihai começa em 11 e chega no 16
        auto t = melodia();
        const auto lt = alignMelodyToCycles(t, { sec }, { findCycle(*ritmos, "tīntāl") }, { nullptr }, p);
        std::vector<double> tempos;
        for (const auto& n : t.notes)
            if (n.voice == 0 && n.time >= 10.99 && n.time <= 16.01)
                tempos.push_back(n.time);
        std::printf("tīntāl: %s · notas do tihai em", lt.c_str());
        for (double x : tempos)
            std::printf(" %.1f", x);
        std::printf("\n");
        CHECK(tempos == (std::vector<double> { 11.0, 11.5, 12.0, 13.0, 13.5, 14.0, 15.0, 15.5, 16.0 }));
        CHECK(lt.find("1 tihai") != std::string::npos);
    }

    // 3. na peça: a Yaman chama o tīntāl; --ciclo força a clave
    {
        std::vector<std::string> e2;
        const auto escalas = std::make_shared<const std::vector<ArchiveScale>>(
            parseScaleArchive(ler(TRIOIO_SOURCE_DIR "/data/escalas/arquivo_trioio.txt"), e2));
        std::vector<float> v(static_cast<std::size_t>(SR * 30.0));
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.5);
            v[i] = static_cast<float>(0.3 * std::exp(-tt * 8.0) * std::sin(2 * PI * 330.0 * t));
        }
        Analyzer a(SR);
        a.push(v.data(), v.size());
        a.finish();
        const auto compor = [&](const std::string& escala, const std::string& ciclo) {
            Options op { Mode::follow, 1, PitchSource::archive, TimeMode::pulse, 0.0f };
            op.scaleArchive = escalas;
            op.scaleQuery = escala;
            op.rhythmArchive = ritmos;
            op.cycleQuery = ciclo;
            op.mosaic = false;
            return compose(a.frames(), a.onsets(), 30.0, op);
        };
        const auto marca = [](const Score& sc, const std::string& termo) {
            for (const auto& m : sc.marks)
                if (m.text.find(termo) != std::string::npos)
                    return m.text;
            return std::string();
        };
        const auto yaman = compor("rāga Yaman", "");
        CHECK(!marca(yaman, "ciclos das culturas: tīntāl").empty());
        CHECK(!marca(yaman, "a melodia chega no ciclo").empty());
        CHECK(!marca(compor("blues", "clave son"), "ciclos das culturas: clave son 3-2").empty());
        CHECK(!marca(compor("Werckmeister III", "nada-assim"), "ciclo rítmico: nenhum").empty());
    }

    // ---- reconhecer o ciclo (rankCycles): ciclos do arquivo sintetizados a
    //      100 BPM com tambor grave (D/B) e agudo (T/k), 30 s cada
    {
        const auto& arq = *ritmos;
        const double SR = 48000.0, PI = 3.14159265358979323846, beat = 0.6, dur = 30.0;
        std::uint64_t ruido = 0x2545F4914F6CDD1Dull;
        const auto noise = [&] {
            ruido ^= ruido << 13; ruido ^= ruido >> 7; ruido ^= ruido << 17;
            return static_cast<double>(ruido >> 11) / 4503599627370496.0 - 1.0;
        };
        // os 8 da primeira bancada (2026-10-10) e, desde 2026-10-14, todos os
        // ciclos do arquivo: a bancada completa
        const std::vector<std::string> oito = { "maqsum", "baladi", "sa'idi", "wahda", "clave son 3-2", "clave bossa nova",
                                                "ruchenitsa (7/8)", "shuffle" };
        for (const auto& nome : oito)
            CHECK(findCycle(arq, nome) != nullptr);
        int primeiro = 0, entreTres = 0, todosPrimeiro = 0, todosTres = 0, todos = 0;
        for (const auto& ciclo : arq)
        {
            const auto* c = &ciclo;
            const std::string nome = c->name;
            ruido = 0x2545F4914F6CDD1Dull;  // o mesmo ruído para cada ciclo: o resultado não depende da ordem do arquivo
            const bool dosOito = std::find(oito.begin(), oito.end(), nome) != oito.end();
            std::vector<float> x(static_cast<std::size_t>(dur * SR), 0.0f);
            const double unit = beat * c->unitBeats;
            for (std::size_t k = 0; k * unit < dur; ++k)
            {
                const char g = c->strokes[k % c->strokes.size()];
                if (g == '-')
                    continue;
                const auto i0 = static_cast<std::size_t>(k * unit * SR);
                for (std::size_t i = i0; i < std::min(x.size(), i0 + static_cast<std::size_t>(0.2 * SR)); ++i)
                {
                    const double u = static_cast<double>(i - i0) / SR;
                    double v = 0.0;
                    if (g == 'D' || g == 'B' || g == 'G' || g == 'N')  // grave: pele a 70 Hz
                        v = 0.9 * std::sin(2.0 * PI * (70.0 * u + 40.0 * 0.03 * (1.0 - std::exp(-u / 0.03)))) * std::exp(-u / 0.12);
                    else if (g == 'x')  // clave: madeira
                        v = 0.6 * std::sin(2.0 * PI * 2500.0 * u) * std::exp(-u / 0.01);
                    else  // agudo: tak (T forte; k, t, P fracos)
                        v = (g == 'T' ? 0.6 : 0.3) * (0.6 * noise() * std::exp(-u / 0.03) + 0.4 * std::sin(2.0 * PI * 600.0 * u) * std::exp(-u / 0.04));
                    x[i] += static_cast<float>(v);
                }
            }
            trioio::perception::Analyzer a(SR);
            a.push(x.data(), x.size());
            a.finish();
            const auto p = trioio::perception::estimatePulse(a.frames(), a.onsets(), 0.0, dur);
            const auto todosR = rankCycles(arq, a.frames(), a.onsets(), p, 0.0, dur, 100);
            const std::vector<RankedCycle> top(todosR.begin(), todosR.begin() + std::min<std::size_t>(5, todosR.size()));
            for (std::size_t i = 0; i < todosR.size(); ++i)
                if (todosR[i].cycle == c && i >= 5)
                    std::printf("    (o certo em %zu.º, %.2f, nível %.2f)\n", i + 1, todosR[i].score, todosR[i].levelScale);
            int pos = 0;
            for (std::size_t i = 0; i < top.size(); ++i)
                if (top[i].cycle == c)
                    pos = static_cast<int>(i) + 1;
            primeiro += dosOito && pos == 1;
            entreTres += dosOito && pos >= 1 && pos <= 3;
            todosPrimeiro += pos == 1;
            todosTres += pos >= 1 && pos <= 3;
            ++todos;
            std::printf("  ciclo %-32s pulso %5.1f · reconhecido em %d.º · 1.º: %s (%.2f)%s\n", nome.c_str(), p.bpm, pos,
                        top.empty() ? "—" : top[0].cycle->name.c_str(), top.empty() ? 0.0 : top[0].score,
                        top.size() > 1 ? (", 2.º: " + top[1].cycle->name).c_str() : "");
        }
        std::printf("reconhecer o ciclo: %d de 8 em 1.º, %d entre os 3 primeiros · bancada completa: %d de %d em 1.º, %d entre os 3\n",
                    primeiro, entreTres, todosPrimeiro, todos, todosTres);
        // EXPERIMENTAL (docs/LEITURA_E_GABARITO.md). Desde 2026-10-14 a
        // bancada cobre os 26 ciclos do arquivo, com o mesmo ruído para cada
        // um (antes o ruído corria entre os ciclos e o resultado dependia da
        // ordem: a ruchenitsa e a wahda passavam ou falhavam por sorte).
        // Medido: os 8 de antes, 6 em 1.º e 6 entre os 3; os 26, 19 em 1.º e
        // 21 entre os 3. O teste guarda que não piore.
        CHECK(primeiro >= 6 && entreTres >= 6);
        CHECK(todosPrimeiro >= 19 && todosTres >= 21);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("ciclos rítmicos: todas as checagens passaram");
    return EXIT_SUCCESS;
}
