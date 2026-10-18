// A comparação entre versões (src/core/Versions.h): duas versões com o mesmo
// andamento, tom, progressão e forma dão invariantes e o mashup; uma terceira
// noutra tonalidade e com outra forma dá divergências.
#include "Versions.h"

#include <cstdio>
#include <cstdlib>

using namespace trioio::composition;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static VersionReading versao(const std::string& id, double bpm, int tonic, bool minor, int shift, bool rising)
{
    VersionReading v;
    v.id = id;
    v.duration = 120.0;
    v.bpm = bpm;
    v.pulseConfidence = 0.8f;
    v.tonic = tonic;
    v.minor = minor;
    v.sections = 4;
    for (int k = 0; k < 64; ++k)
        v.rootsByPosition.push_back((tonic + (k / 8 % 2 ? 5 : 0) + shift) % 12);
    for (int k = 0; k < 32; ++k)
        v.energyCurve.push_back(rising ? k / 31.0f : 1.0f - k / 31.0f);
    v.intent.energy = 0.6f;
    v.intent.brightness = rising ? 0.5f : 0.9f;
    return v;
}

int main()
{
    // iguais (a segunda no dobro do andamento: o mesmo pulso)
    const auto c = compareVersions({ versao("A", 100.0, 9, true, 0, true), versao("B", 200.0, 9, true, 0, true) });
    for (const auto& s : c.invariants)
        std::printf("  = %s\n", s.c_str());
    CHECK(c.invariants.size() == 5 && c.divergences.empty());
    bool mashup = false;
    for (const auto& s : c.operations)
        mashup = mashup || s.find("--com B") != std::string::npos;
    CHECK(mashup);
    // outra tonalidade, outra forma, outro brilho
    const auto d = compareVersions({ versao("A", 100.0, 9, true, 0, true), versao("C", 100.0, 2, false, 5, false) });
    for (const auto& s : d.divergences)
        std::printf("  ≠ %s\n", s.c_str());
    for (const auto& s : d.operations)
        std::printf("  → %s\n", s.c_str());
    bool tom = false, forma = false, brilho = false, progressao = false;
    for (const auto& s : d.divergences)
    {
        tom = tom || s.find("tonalidade") != std::string::npos;
        forma = forma || s.find("curva de energia") != std::string::npos;
        brilho = brilho || s.find("brilho") != std::string::npos;
        progressao = progressao || s.find("progressão") != std::string::npos;
    }
    CHECK(tom && forma && brilho && progressao);
    CHECK(d.invariants.size() == 2);  // o andamento e a duração
    // uma versão só
    CHECK(compareVersions({ versao("A", 100.0, 9, true, 0, true) }).invariants.empty());
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("versões: todas as checagens passaram");
    return EXIT_SUCCESS;
}
