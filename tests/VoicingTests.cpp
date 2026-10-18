// Voicings (core/Voicing.h): quartal, So What, aberto, bitonal, cluster e
// planing, conferidos com a construção; ajustados à escala; na peça, a voz
// de harmonia empilha em quartas.

#include "Composer.h"
#include "Voicing.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
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

static bool iguais(const std::vector<float>& a, std::initializer_list<float> b)
{
    if (a.size() != b.size())
        return false;
    std::size_t i = 0;
    for (float x : b)
        if (std::abs(a[i++] - x) > 0.01f)
            return false;
    return true;
}

int main()
{
    MachineScale dorico;  // Ré dórico: Ré Mi Fá Sol Lá Si Dó
    dorico.tonic = 200.0;
    dorico.cents = { 0, 200, 300, 500, 700, 900, 1000 };
    std::vector<float> forma;
    const float re = 50.0f;  // Ré 3
    // So What sobre Ré dórico: Ré Sol Dó Fá Lá (todas da escala)
    CHECK(iguais(voiceChord(Voicing::soWhat, re, {}, &dorico, forma), { 50, 55, 60, 65, 69 }));
    // quartal: Ré Sol Dó
    CHECK(iguais(voiceChord(Voicing::quartal, re, {}, &dorico, forma), { 50, 55, 60 }));
    // aberto: Ré Lá Mi
    CHECK(iguais(voiceChord(Voicing::open, re, {}, &dorico, forma), { 50, 57, 64 }));
    // bitonal: Ré maior + Lá♭ maior (um trítono), sem ajuste
    CHECK(iguais(voiceChord(Voicing::bitonal, re, {}, &dorico, forma), { 50, 54, 56, 57, 60, 63 }));
    // cluster: três graus vizinhos (Ré Mi Fá)
    CHECK(iguais(voiceChord(Voicing::cluster, re, {}, &dorico, forma), { 50, 52, 53 }));
    // quartal sobre Mi: Mi Lá Ré (todas da escala)
    CHECK(iguais(voiceChord(Voicing::quartal, 52.0f, {}, &dorico, forma), { 52, 57, 62 }));
    // um semitom fora da escala fica (cor): sobre Sol, Sol Dó Fá e a terça Sib do So What
    CHECK(iguais(voiceChord(Voicing::soWhat, 55.0f, {}, &dorico, forma), { 55, 60, 65, 70, 74 }));
    // microtonal: uma quarta de 22-EDO (1200·9/22 ≈ 491 c) puxa a quarta justa para ela
    MachineScale edo;
    for (int k = 0; k < 22; ++k)
        edo.cents.push_back(1200.0 * k / 22.0);
    const auto q = voiceChord(Voicing::quartal, 48.0f, {}, &edo, forma);
    CHECK(std::abs(q[1] - (48.0f + 12.0f * 9.0f / 22.0f)) < 0.01f);
    // planing: a forma do primeiro acorde (Dó maior em terças) movida em paralelo, cromática
    std::vector<float> plan;
    CHECK(iguais(voiceChord(Voicing::planing, 48.0f, { 0, 4, 7 }, nullptr, plan), { 48, 52, 55 }));
    CHECK(iguais(voiceChord(Voicing::planing, 51.0f, { 3, 6, 10 }, nullptr, plan), { 51, 55, 58 }));  // Mib maior, não menor
    Voicing v;
    CHECK(parseVoicing("sowhat", v) && v == Voicing::soWhat && !parseVoicing("jazz", v));

    // na peça: com --voicing quartal, os acordes da harmonia empilham quartas
    std::vector<float> s(static_cast<std::size_t>(SR * 24.0));
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR;
        const int r = (static_cast<int>(t / 2.0) % 2) ? 55 : 48;
        double x = 0.0;
        for (int k : { 0, 4, 7 })
            x += std::sin(2 * PI * 440.0 * std::pow(2.0, (r + 12 + k - 69) / 12.0) * t);
        s[i] = static_cast<float>(0.12 * x);
    }
    Analyzer a(SR);
    a.push(s.data(), s.size());
    a.finish();
    Options o { Mode::follow, 1, PitchSource::reference, TimeMode::automatic, 0.0f };
    o.voicing = static_cast<int>(Voicing::quartal);
    o.mosaic = false;
    const auto sc = compose(a.frames(), a.onsets(), 24.0, o);
    std::map<double, std::vector<float>> acordes;
    for (const auto& n : sc.notes)
        if (n.voice == 6)
            acordes[n.time].push_back(n.pitch);
    int quartas = 0, intervalos = 0;
    for (auto& [t, ac] : acordes)
    {
        std::sort(ac.begin(), ac.end());
        for (std::size_t i = 1; i < ac.size(); ++i)
        {
            ++intervalos;
            const float d = ac[i] - ac[i - 1];
            quartas += d >= 4.5f && d <= 6.5f;  // quarta justa (ou aumentada ao ajustar)
        }
    }
    std::printf("quartal na peça: %d de %d intervalos em quartas, %zu acordes\n", quartas, intervalos, acordes.size());
    CHECK(intervalos > 10 && quartas * 10 >= intervalos * 8);

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("voicings: todas as checagens passaram");
    return EXIT_SUCCESS;
}
