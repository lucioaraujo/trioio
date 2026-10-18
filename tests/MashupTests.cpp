// Mashup de materiais (core/Mashup.h): a melodia de A sobre a harmonia, o
// groove, as memórias e o corpo de B, com B reescalada no tempo de A.

#include "Composer.h"
#include "Mashup.h"

#include <algorithm>
#include <cmath>
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

static double hz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

int main()
{
    // 1. as camadas e o tempo
    unsigned l = 0;
    CHECK(parseMashupLayers("harmonia, memoria", l) && l == (kMashHarmony | kMashMemory));
    CHECK(parseMashupLayers("tudo", l) && l == 15u && !parseMashupLayers("cor", l));
    {
        std::vector<Frame> f(1);
        f[0].time = 10.0;
        std::vector<Onset> o(1);
        o[0].time = 20.0;
        remapTimeline(f, o, 40.0, 20.0);  // B de 40 s no tempo de A (20 s)
        CHECK(std::abs(f[0].time - 5.0) < 1e-9 && std::abs(o[0].time - 10.0) < 1e-9);
    }

    // A: uma melodia de Dó maior, sem acordes (30 s)
    std::vector<float> va(static_cast<std::size_t>(SR * 30.0));
    const int mel[] = { 60, 62, 64, 65, 67, 65, 64, 62 };
    for (std::size_t i = 0; i < va.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.5);
        va[i] = static_cast<float>(0.3 * std::exp(-tt * 4.0) * std::sin(2 * PI * hz(mel[static_cast<int>(t / 0.5) % 8]) * t));
    }
    // B: acordes de Sol menor e Ré maior (60 s, o dobro de A)
    std::vector<float> vb(static_cast<std::size_t>(SR * 60.0));
    for (std::size_t i = 0; i < vb.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR;
        const bool sol = static_cast<int>(t / 4.0) % 2 == 0;
        const int r = sol ? 55 : 50, third = sol ? 3 : 4;
        double s = 0.0;
        for (int k : { 0, third, 7 })
            s += std::sin(2 * PI * hz(r + 12 + k) * t);
        vb[i] = static_cast<float>(0.12 * s * (0.6 + 0.4 * std::exp(-std::fmod(t, 0.5) * 10.0)));
    }
    Analyzer a(SR), b(SR);
    a.push(va.data(), va.size());
    a.finish();
    b.push(vb.data(), vb.size());
    b.finish();
    auto fb = b.frames();
    auto ob = b.onsets();
    remapTimeline(fb, ob, 60.0, 30.0);

    const auto compor = [&](unsigned camadas) {
        Options o { Mode::follow, 1, PitchSource::reference, TimeMode::automatic, 0.6f };
        o.otherFrames = &fb;
        o.otherOnsets = &ob;
        o.otherLayers = camadas;
        o.otherName = "B";
        return compose(a.frames(), a.onsets(), 30.0, o);
    };
    const auto marca = [](const Score& sc, const std::string& termo) {
        for (const auto& m : sc.marks)
            if (m.text.find(termo) != std::string::npos)
                return m.text;
        return std::string();
    };

    // 2. harmonia de B: os acordes lidos são os de B (Solm, Ré), não os de A
    {
        const auto sc = compor(kMashHarmony);
        const auto h = marca(sc, "harmonia em movimento");
        std::printf("harmonia de B: %s\n", h.substr(0, 120).c_str());
        CHECK(!marca(sc, "mashup de materiais: harmonia da referência B").empty());
        CHECK(h.find("Solm") != std::string::npos && h.find("Ré") != std::string::npos);
    }
    // 3. memória de B: as passagens de memória vêm de B (a sequência de
    //    alturas muda em relação à mesma peça sem a outra). Contar classes de
    //    altura não serve: a escuta lê o acorde de B pela fundamental
    //    implícita, e o Terceiro Olho pode transpor um trecho
    {
        const auto com = compor(kMashMemory);
        Options o { Mode::follow, 1, PitchSource::reference, TimeMode::automatic, 0.6f };
        const auto sem = compose(a.frames(), a.onsets(), 30.0, o);
        const auto memoria = [](const Score& sc) {
            std::vector<int> v;
            for (const auto& n : sc.notes)
                if (n.voice == 3)
                    v.push_back(static_cast<int>(std::lround(n.pitch)));
            return v;
        };
        const auto mc = memoria(com), ms = memoria(sem);
        std::printf("memória: %zu notas com B, %zu sem B\n", mc.size(), ms.size());
        CHECK(!mc.empty() && mc != ms);
        CHECK(!marca(com, "mashup de materiais: memória da referência B").empty());
    }
    // 4. sem a outra: nada muda
    {
        Options o { Mode::follow, 1, PitchSource::reference, TimeMode::automatic, 0.6f };
        const auto sc = compose(a.frames(), a.onsets(), 30.0, o);
        CHECK(marca(sc, "mashup de materiais").empty());
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("mashup de materiais: todas as checagens passaram");
    return EXIT_SUCCESS;
}
