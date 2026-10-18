#include "Composer.h"
#include "ScoreSynth.h"
#include "ThirdEye.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

// referência de 60 s que muda de caráter a cada 10 s (dá ao olho onde olhar)
static void escuta(std::vector<Frame>& frames, std::vector<Onset>& onsets)
{
    std::vector<float> v(static_cast<std::size_t>(SR * 60.0), 0.0f);
    std::uint32_t x = 3;
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR;
        const int bloco = static_cast<int>(t / 10.0) % 3;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        const double r = x / 4294967295.0 * 2.0 - 1.0;
        const double clique = std::fmod(t, 0.5) < 0.015 ? 0.6 : 0.0;
        v[i] = static_cast<float>(bloco == 0 ? 0.1 * std::sin(2 * PI * 262 * t) + clique
                                : bloco == 1 ? 0.2 * r * (std::fmod(t, 0.25) < 0.03 ? 1.0 : 0.1)
                                             : 0.1 * std::sin(2 * PI * 392 * t) + 0.05 * std::sin(2 * PI * 494 * t) + clique * 0.5);
    }
    Analyzer a(SR);
    a.push(v.data(), v.size());
    a.finish();
    frames = a.frames();
    onsets = a.onsets();
}

static int contar(const Score& s, const char* termo)
{
    return static_cast<int>(std::count_if(s.marks.begin(), s.marks.end(),
                                          [&](const Mark& m) { return m.text.find(termo) != std::string::npos; }));
}

int main()
{
    std::vector<Frame> fr;
    std::vector<Onset> on;
    escuta(fr, on);
    const auto compor = [&](float u, std::uint64_t seed, Mode m = Mode::follow) {
        return compose(fr, on, 60.0, Options { m, seed, PitchSource::reference, TimeMode::automatic, u });
    };

    // 1. imprevisibilidade 0: o olho não intervém
    {
        const auto s = compor(0.0f, 1);
        CHECK(s.gestures.empty());
        CHECK(contar(s, "Terceiro Olho") == 0);
    }

    // 2. imprevisibilidade 1: intervém, com objetos e regras; reprodutível
    {
        const auto a = compor(1.0f, 1), b = compor(1.0f, 1), c = compor(1.0f, 2);
        for (const auto& m : a.marks)
            if (m.text.find("Terceiro Olho") != std::string::npos)
                std::printf("  %5.1f s  %s\n", m.time, m.text.c_str());
        CHECK(contar(a, "objeto sonoro") >= 1);
        CHECK(contar(a, "regra oculta") >= 2);
        CHECK(a.notes.size() == b.notes.size() && a.gestures.size() == b.gestures.size() && a.marks.size() == b.marks.size());
        bool diferente = a.marks.size() != c.marks.size();
        for (std::size_t i = 0; !diferente && i < a.marks.size(); ++i)
            diferente = a.marks[i].text != c.marks[i].text || a.marks[i].time != c.marks[i].time;
        CHECK(diferente);
    }

    // 3. mais imprevisibilidade, mais intervenções (somado em 6 seeds)
    {
        int baixa = 0, alta = 0;
        for (std::uint64_t s = 1; s <= 6; ++s)
        {
            baixa += contar(compor(0.2f, s), "Terceiro Olho");
            alta += contar(compor(1.0f, s), "Terceiro Olho");
        }
        std::printf("intervenções em 6 seeds: %d com 0,2 · %d com 1,0\n", baixa, alta);
        CHECK(alta > baixa * 2);
    }

    // 4. silêncio súbito é silêncio: nenhuma nota nem objeto começa dentro dele
    {
        // 48 seeds: com a leitura do momento o silêncio ficou reservado a
        // "energia alta antes de uma mudança clara" e é mais raro
        int vistos = 0;
        for (std::uint64_t s = 1; s <= 48; ++s)
        {
            const auto sc = compor(1.0f, s);
            for (const auto& m : sc.marks)
            {
                const auto p = m.text.find("silêncio súbito por ");
                if (p == std::string::npos)
                    continue;
                ++vistos;
                const double d = std::stod(m.text.substr(p + std::string("silêncio súbito por ").size()));
                for (const auto& n : sc.notes)
                    CHECK(!(n.time >= m.time && n.time < m.time + d - 0.05));
                for (const auto& g : sc.gestures)
                    CHECK(!(g.time >= m.time && g.time < m.time + d - 0.05));
            }
        }
        std::printf("silêncios conferidos: %d\n", vistos);
        CHECK(vistos >= 1);
    }

    // 5. cada família de timbre e cada objeto sonoro soa (finito, audível, sem estouro)
    {
        Score s;
        s.duration = 30.0;
        for (int t = 0; t < 6; ++t)
            s.notes.push_back({ 0.5 + t * 1.5, 0.8, 60.0f + t, 0.0f, 0.8f, 0, static_cast<Timbre>(t) });
        s.notes.push_back({ 9.5, 1.0, 36.0f, 0.0f, 0.7f, 1, Timbre::additive });
        for (int k = 0; k < 8; ++k)
        {
            Gesture g;
            g.kind = static_cast<GestureKind>(k);
            g.ratios = { 1.0f, 2.0f, 2.76f };
            g.weights = { 0.5f, 0.3f, 0.2f };
            g.time = 11.0 + k * 2.5;
            g.duration = 2.0;
            g.lo = 55; g.hi = 70; g.loEnd = 40; g.hiEnd = 80;
            g.density = 8; g.densityEnd = 20;
            g.level = 0.6f;
            g.pitches = { 60, 64, 67 };
            g.seed = static_cast<std::uint64_t>(k + 1);
            s.gestures.push_back(g);
        }
        std::vector<float> L, R;
        renderScore(s, SR, L, R);
        bool finito = true;
        float pico = 0.0f;
        for (std::size_t i = 0; i < L.size(); ++i)
        {
            finito = finito && std::isfinite(L[i]) && std::isfinite(R[i]);
            pico = std::max({ pico, std::abs(L[i]), std::abs(R[i]) });
        }
        CHECK(finito && pico <= 0.9f);
        const auto rmsDb = [&](double a, double b) {
            double p = 0.0;
            const auto i0 = static_cast<std::size_t>(a * SR), i1 = static_cast<std::size_t>(b * SR);
            for (std::size_t i = i0; i < i1; ++i)
                p += 0.5 * (L[i] * L[i] + R[i] * R[i]);
            return 10.0 * std::log10(p / static_cast<double>(i1 - i0) + 1e-12);
        };
        const char* nomes[] = { "aditivo", "sopro", "metal", "corda", "membrana", "FM" };
        for (int t = 0; t < 6; ++t)
        {
            const double db = rmsDb(0.5 + t * 1.5, 0.5 + t * 1.5 + 0.3);
            std::printf("timbre %-9s %6.1f dB\n", nomes[t], db);
            CHECK(db > -45.0);
        }
        for (int k = 0; k < 8; ++k)
        {
            const double db = rmsDb(11.5 + k * 2.5, 12.5 + k * 2.5);
            std::printf("objeto %-20s %6.1f dB\n", gestureName(static_cast<GestureKind>(k)), db);
            CHECK(db > -50.0);
        }
    }

    // 6. a leitura do momento: nos blocos de ruído (10–20 s e 40–50 s) o olho
    //    escolhe gestos de ruído (nuvem, faixa de ruído) bem mais do que nos
    //    blocos tonais; com imprevisibilidade alta essa preferência afrouxa
    {
        const auto ruidoso = [](double t) { return static_cast<int>(t / 10.0) % 3 == 1; };
        const auto deRuido = [](const std::string& s) {
            return s.find("nuvem granular") != std::string::npos || s.find("faixa de ruído") != std::string::npos;
        };
        const auto medir = [&](float u, int& noRuido, int& totalRuido, int& noTonal, int& totalTonal) {
            noRuido = totalRuido = noTonal = totalTonal = 0;
            for (std::uint64_t s = 1; s <= 64; ++s)
                for (const auto& m : compor(u, s).marks)
                {
                    if (m.text.find("Terceiro Olho") == std::string::npos)
                        continue;
                    if (ruidoso(m.time)) { ++totalRuido; noRuido += deRuido(m.text); }
                    else { ++totalTonal; noTonal += deRuido(m.text); }
                }
        };
        int nr, tr, nt, tt;
        medir(0.3f, nr, tr, nt, tt);
        const double fRuido = tr ? static_cast<double>(nr) / tr : 0.0, fTonal = tt ? static_cast<double>(nt) / tt : 0.0;
        std::printf("u=0,3 · gestos de ruído: %d/%d no ruído (%.0f %%), %d/%d no tonal (%.0f %%)\n",
                    nr, tr, 100 * fRuido, nt, tt, 100 * fTonal);
        CHECK(tr >= 8 && tt >= 8);
        CHECK(fRuido > 2.0 * fTonal + 0.1);
        int nr1, tr1, nt1, tt1;
        medir(1.0f, nr1, tr1, nt1, tt1);
        const double fRuido1 = tr1 ? static_cast<double>(nr1) / tr1 : 0.0;
        std::printf("u=1,0 · gestos de ruído: %d/%d no ruído (%.0f %%)\n", nr1, tr1, 100 * fRuido1);
        CHECK(fRuido1 < fRuido);  // mais imprevisível = menos preso à leitura
    }

    // 7. cada intervenção diz por que foi escolhida
    {
        const auto s = compor(0.6f, 3);
        int olho = 0, comLeitura = 0;
        for (const auto& m : s.marks)
            if (m.text.find("Terceiro Olho") != std::string::npos)
            {
                ++olho;
                comLeitura += m.text.find(" · leitura: ") != std::string::npos;
            }
        CHECK(olho >= 1 && comLeitura == olho);
    }

    // 8. congelar é congelar o acorde que acabou de soar: as alturas do
    //    congelamento estão entre as notas que SOAVAM nos 3 s anteriores
    //    (desde a harmonia em movimento, um acorde sustentado pode ter
    //    começado antes disso e ainda estar soando)
    {
        int vistos = 0, doAcorde = 0;
        for (std::uint64_t s = 1; s <= 24; ++s)
        {
            const auto sc = compor(0.5f, s);
            for (const auto& m : sc.marks)
            {
                if (m.text.find("acorde forte") == std::string::npos)
                    continue;
                for (const auto& g : sc.gestures)
                {
                    if (g.kind != GestureKind::freeze || std::abs(g.time - m.time) > 1e-6)
                        continue;
                    ++vistos;
                    bool todas = !g.pitches.empty();
                    for (float p : g.pitches)
                        todas = todas && std::any_of(sc.notes.begin(), sc.notes.end(), [&](const Note& n) {
                                    return n.time < m.time && n.time + n.duration > m.time - 3.05 && std::abs(n.pitch - p) < 1e-3f;
                                });
                    doAcorde += todas;
                }
            }
        }
        std::printf("congelamentos do acorde recente: %d de %d\n", doAcorde, vistos);
        CHECK(vistos >= 1);
        CHECK(doAcorde * 10 >= vistos * 8);
    }

    // 9. consequências: com imprevisibilidade alta, algumas intervenções FICAM
    //    até o fim (poucas por peça); quando o acorde congelado vira o campo,
    //    as notas depois dele, até a próxima intervenção, estão nesse acorde
    {
        int pecas = 0, comConsequencia = 0, maxPorPeca = 0, campos = 0, camposOk = 0;
        for (std::uint64_t s = 1; s <= 24; ++s)
        {
            const auto sc = compor(1.0f, s);
            ++pecas;
            const int n = contar(sc, " · fica: ");
            comConsequencia += n > 0;
            maxPorPeca = std::max(maxPorPeca, n);
            std::vector<double> olho;
            for (const auto& m : sc.marks)
                if (m.text.find("Terceiro Olho") != std::string::npos)
                    olho.push_back(m.time);
            for (const auto& m : sc.marks)
            {
                if (m.text.find("fica: o acorde vira o campo") == std::string::npos)
                    continue;
                const Gesture* g = nullptr;
                for (const auto& x : sc.gestures)
                    if (x.kind == GestureKind::freeze && std::abs(x.time - m.time) < 1e-6)
                        g = &x;
                if (!g)
                    continue;
                ++campos;
                const double de = g->time + g->duration;
                double ate = 1e9;
                for (double t : olho)
                    if (t > m.time + 1e-6) { ate = t; break; }
                bool dentro = true;
                for (const auto& nota : sc.notes)
                {
                    if (nota.time < de || nota.time >= ate || nota.voice == 4)  // a percussão não vira altura do campo
                        continue;
                    const int pc = ((static_cast<int>(std::lround(nota.pitch)) % 12) + 12) % 12;
                    dentro = dentro && std::any_of(g->pitches.begin(), g->pitches.end(), [&](float p) {
                                 return ((static_cast<int>(std::lround(p)) % 12) + 12) % 12 == pc;
                             });
                }
                camposOk += dentro;
            }
        }
        std::printf("consequências: %d de %d peças, até %d numa peça · campo do acorde: %d de %d\n",
                    comConsequencia, pecas, maxPorPeca, camposOk, campos);
        CHECK(comConsequencia * 2 >= pecas);
        CHECK(maxPorPeca <= 4);
        CHECK(campos == 0 || camposOk == campos);
        CHECK(contar(compor(0.0f, 1), " · fica: ") == 0);
    }

    // 9. a dramaturgia guia o olho (Options::tensionAt, breath): tensão alta
    //    traz mais intervenções que tensão baixa; nenhuma na respiração
    {
        int baixa = 0, alta = 0, naRespiracao = 0;
        for (std::uint64_t s = 1; s <= 8; ++s)
        {
            Options lo { Mode::follow, s, PitchSource::reference, TimeMode::automatic, 0.8f };
            lo.dramaturgy = false;  // a curva vem só daqui
            lo.tensionAt = [](double) { return 0.0f; };
            Options hi = lo;
            hi.tensionAt = [](double) { return 1.0f; };
            hi.breath = { 20.0, 30.0 };
            baixa += contar(compose(fr, on, 60.0, lo), "Terceiro Olho");
            const auto sc = compose(fr, on, 60.0, hi);
            alta += contar(sc, "Terceiro Olho");
            for (const auto& m : sc.marks)
                if (m.text.rfind("Terceiro Olho", 0) == 0 && m.time >= 19.0 && m.time < 30.0)
                    ++naRespiracao;
        }
        std::printf("olho pela tensão (8 seeds): %d intervenções com tensão 0 · %d com tensão 1 · %d na respiração\n", baixa, alta,
                    naRespiracao);
        CHECK(alta > baixa);
        CHECK(naRespiracao == 0);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("terceiro olho: todas as checagens passaram");
    return EXIT_SUCCESS;
}
