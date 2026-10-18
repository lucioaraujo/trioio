// Recombinação de culturas e linguagens (core/Recombination.h,
// docs/ESCALAS.md "Misturas"): colagem (cada seção com a sua cultura,
// comportamento e textura), híbrido (metade de cada), camadas
// (polimodalidade) e transição (estados intermediários).

#include "Composer.h"
#include "Recombination.h"
#include "ScaleArchive.h"

#include <algorithm>
#include <cmath>
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

int main()
{
    std::ifstream in(TRIOIO_SOURCE_DIR "/data/escalas/arquivo_trioio.txt");
    std::stringstream ss;
    ss << in.rdbuf();
    std::vector<std::string> erros;
    const auto arq = std::make_shared<const std::vector<ArchiveScale>>(parseScaleArchive(ss.str(), erros));

    // referência de 4 trechos contrastantes (15 s cada)
    std::vector<float> v(static_cast<std::size_t>(SR * 60.0));
    const int mel[] = { 60, 62, 64, 65, 67, 69, 71, 72 };
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.5);
        const int sec = static_cast<int>(t / 15.0);
        const double f = 440.0 * std::pow(2.0, (mel[static_cast<int>(t / 0.5) % 8] + 3 * sec - 69) / 12.0);
        const double amp = sec % 2 ? 0.15 : 0.4;
        v[i] = static_cast<float>(amp * std::exp(-tt * 4.0) * (std::sin(2 * PI * f * t) + 0.3 * std::sin(6 * PI * f * t)));
    }
    Analyzer a(SR);
    a.push(v.data(), v.size());
    a.finish();
    const auto compor = [&](std::vector<std::string> mix, MixTechnique tec) {
        Options o { Mode::follow, 4, PitchSource::archive, TimeMode::automatic, 0.0f };
        o.scaleArchive = arq;
        o.mix = std::move(mix);
        o.mixTechnique = tec;
        o.mosaic = false;
        return compose(a.frames(), a.onsets(), 60.0, o);
    };
    const auto marca = [](const Score& sc, const std::string& termo) {
        for (const auto& m : sc.marks)
            if (m.text.find(termo) != std::string::npos)
                return m.text;
        return std::string();
    };
    std::string t;
    MixTechnique mt;
    CHECK(parseMixTechnique("camadas", mt) && mt == MixTechnique::layers && !parseMixTechnique("sopa", mt));

    // 1. colagem: as seções alternam Yaman, Hijaz e blues, cada uma com a sua textura
    {
        const auto sc = compor({ "rāga Yaman", "maqam Hijaz", "blues" }, MixTechnique::collage);
        std::vector<std::string> ordem;
        for (const auto& m : sc.marks)
            if (m.text.rfind("seção ", 0) == 0)
                ordem.push_back(m.text.find("Yaman") != std::string::npos   ? "Yaman"
                                : m.text.find("Hijaz") != std::string::npos ? "Hijaz"
                                : m.text.find("blues") != std::string::npos ? "blues"
                                                                            : "?");
        std::printf("colagem: seções");
        for (const auto& o : ordem)
            std::printf(" %s", o.c_str());
        std::printf("\n");
        CHECK(ordem.size() >= 3 && ordem[0] == "Yaman" && ordem[1] == "Hijaz" && ordem[2] == "blues");
        CHECK(!marca(sc, "mistura (colagem)").empty());
        CHECK(!marca(sc, "textura: bordão").empty() && !marca(sc, "textura: heterofonia").empty());
        CHECK(!marca(sc, "comportamento da escala").empty());
    }

    // 2. híbrido: a metade de baixo de uma, a de cima de outra; o ornamento da segunda
    {
        ArchivePick lo, hi;
        lo.choice.steps = { 0.0f, 2.04f, 3.86f, 5.90f, 7.02f, 8.84f, 10.88f };  // Yaman (justa)
        lo.choice.name = "A";
        lo.behavior.texture = "bordão";
        hi.choice.steps = { 0.0f, 1.0f, 4.0f, 5.0f, 7.0f, 8.0f, 10.0f };        // Hijaz
        hi.choice.name = "B";
        hi.behavior.ornament = "blue";
        const auto h = hybridPick(lo, hi);
        CHECK(h.choice.steps == (std::vector<float> { 0.0f, 2.04f, 3.86f, 5.90f, 7.0f, 8.0f, 10.0f }));
        CHECK(h.behavior.ornament == "blue" && h.behavior.texture == "bordão");
        const auto sc = compor({ "rāga Yaman", "blues" }, MixTechnique::hybrid);
        CHECK(!marca(sc, "escala da mistura: híbrido").empty());
    }

    // 3. camadas: a harmonia e a segunda voz na outra escala, sobre o mesmo centro
    {
        const auto sc = compor({ "rāga Yaman", "maqam Hijaz" }, MixTechnique::layers);
        CHECK(!marca(sc, "mistura (camadas)").empty());
        // o bordão da Yaman fica, e a segunda voz canta no Hijaz
        int segunda = 0, bordao = 0;
        for (const auto& n : sc.notes)
        {
            segunda += n.voice == 2;
            bordao += n.voice == 6;
        }
        CHECK(segunda > 20 && bordao > 0);
    }

    // 4. transição: de Yaman a Hijaz, por estados intermediários
    {
        const auto sc = compor({ "rāga Yaman", "maqam Hijaz" }, MixTechnique::transition);
        int passos = 0;
        bool chegou = false;
        for (const auto& m : sc.marks)
            if (m.text.rfind("seção ", 0) == 0 && m.text.find("mistura (transição)") != std::string::npos)
            {
                ++passos;
                chegou = chegou || m.text.find("100 % de") != std::string::npos;
            }
        std::printf("transição: %d seções marcadas, chegou a 100 %%: %s\n", passos, chegou ? "sim" : "não");
        CHECK(passos >= 3 && chegou);
    }

    // 4b. costuras: Dó maior (0–8 s) → Lá♭ maior (8–16 s), compasso de 2 s
    {
        const auto base = [] {
            Score sc;
            sc.duration = 18.0;
            for (double t = 0.0; t < 16.0; t += 0.25)
            {
                Note n;
                n.time = t;
                n.duration = 0.2;
                n.pitch = 60.0f + static_cast<float>(static_cast<int>(t * 4) % 12);  // cromática
                n.voice = 0;
                sc.notes.push_back(n);
                Note g = n;
                g.voice = 4;
                g.pitch = 38.0f;
                sc.notes.push_back(g);
            }
            return sc;
        };
        SectionScale a, b;
        a.start = 0.0; a.end = 8.0;
        a.scale.tonic = 0.0;
        a.scale.cents = { 0, 200, 400, 500, 700, 900, 1100 };
        b.start = 8.0; b.end = 16.0;
        b.scale.tonic = 800.0;  // Lá♭ maior: Lá♭ Sib Dó Réb Mib Fá Sol
        b.scale.cents = { 0, 200, 400, 500, 700, 900, 1100 };
        const auto pc = [](float p) { return ((static_cast<int>(std::lround(p)) % 12) + 12) % 12; };
        // pivô: as notas comuns são Dó, Fá e Sol (0, 5, 7)
        {
            auto sc = base();
            const auto l = seamSections(sc, { a, b }, Seam::pivot, 2.0);
            bool soComuns = true, atravessa = false;
            for (const auto& n : sc.notes)
            {
                if (n.voice == 0 && n.time >= 6.0 && n.time < 8.0)
                    soComuns = soComuns && (pc(n.pitch) == 0 || pc(n.pitch) == 5 || pc(n.pitch) == 7);
                if (n.voice == 6 && n.time < 8.0 && n.time + n.duration > 8.0)
                    atravessa = true;
            }
            std::printf("costura pivô: %s\n", l.c_str());
            CHECK(soComuns && atravessa && l.find("3 notas comuns") != std::string::npos);
        }
        // ponte: de 6 a 10 s, a escala intermediária (metade do caminho)
        {
            auto sc = base();
            (void) seamSections(sc, { a, b }, Seam::bridge, 2.0);
            const auto meio = interpolateScales(a.scale, b.scale, 0.5);
            int fora = 0;
            for (const auto& n : sc.notes)
                if (n.voice == 0 && n.time >= 6.0 && n.time < 10.0)
                {
                    const double r = std::fmod(100.0 * n.pitch - meio.tonic + 12000.0, 1200.0);
                    fora += std::none_of(meio.cents.begin(), meio.cents.end(), [&](double c) { return std::abs(c - r) < 1.0; });
                }
            CHECK(fora == 0);
        }
        // sobreposição: a percussão de 6–8 s continua em 8–10 s, mais baixa
        {
            auto sc = base();
            const auto antes = sc.notes.size();
            (void) seamSections(sc, { a, b }, Seam::overlap, 2.0);
            int baixas = 0;
            for (const auto& n : sc.notes)
                baixas += n.voice == 4 && n.time >= 8.0 && n.time < 10.0 && n.velocity < 0.4f;
            CHECK(sc.notes.size() == antes + 8 && baixas == 8);
        }
        Seam sm;
        CHECK(parseSeam("ponte", sm) && sm == Seam::bridge && !parseSeam("cola", sm));
    }

    // 5. termo sem escala: avisa e segue com as outras
    {
        const auto sc = compor({ "rāga Yaman", "nenhuma-escala" }, MixTechnique::collage);
        CHECK(!marca(sc, "mistura: nenhuma escala").empty());
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("recombinação: todas as checagens passaram");
    return EXIT_SUCCESS;
}
