// A máquina de escalas (core/ScaleMachine.h, docs/ESCALAS.md): as operações
// conferidas com a teoria (o espelho do jônio é o frígio; a rotação do jônio
// é o dórico; o pivô do Dó maior guarda 6 notas), e o encadeamento na peça.

#include "Composer.h"
#include "ScaleMachine.h"

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

static bool iguais(const std::vector<double>& a, std::initializer_list<double> b)
{
    if (a.size() != b.size())
        return false;
    std::size_t i = 0;
    for (double x : b)
        if (std::abs(a[i++] - x) > 0.01)
            return false;
    return true;
}

int main()
{
    MachineScale maior;
    maior.tonic = 0.0;
    maior.cents = { 0, 200, 400, 500, 700, 900, 1100 };
    maior.name = "maior";

    // 1. as operações
    CHECK(iguais(rotateScale(maior, 1).cents, { 0, 200, 300, 500, 700, 900, 1000 }));  // dórico, sobre Ré
    CHECK(std::abs(rotateScale(maior, 1).tonic - 200.0) < 0.01);
    CHECK(iguais(mirrorScale(maior).cents, { 0, 100, 300, 500, 700, 800, 1000 }));    // frígio: o dual do jônio
    CHECK(iguais(alterDegree(maior, 3, 100.0).cents, { 0, 200, 400, 600, 700, 900, 1100 }));  // lídio
    CHECK(alterDegree(maior, 0, 50.0).cents.front() == 0.0);                            // a tônica fica
    MachineScale pent;
    pent.cents = { 0, 200, 400, 700, 900 };
    CHECK(iguais(addDegree(pent).cents, { 0, 200, 400, 550, 700, 900 }));  // no maior intervalo (Mi–Sol, 300 c)
    CHECK(removeDegree(pent, 2).cents.size() == 4);
    CHECK(std::abs(stretchScale(maior, 1.01).period - 1212.0) < 0.01);
    // interpolação: metade do caminho do maior ao menor harmônico (Mi→Mib, Lá→Lá♭)
    MachineScale menorH = maior;
    menorH.cents = { 0, 200, 300, 500, 700, 800, 1100 };
    CHECK(iguais(interpolateScales(maior, menorH, 0.5).cents, { 0, 200, 350, 500, 700, 850, 1100 }));
    CHECK(iguais(interpolateScales(maior, menorH, 1.0).cents, { 0, 200, 300, 500, 700, 800, 1100 }));
    // notas comuns e pivô: Dó maior → Sol ou Fá maior (6 em comum)
    std::array<float, 12> campoSol {};
    for (int pc : { 7, 9, 11, 0, 2, 4, 6 })
        campoSol[static_cast<std::size_t>(pc)] = pc == 7 ? 0.25f : 0.125f;
    const auto piv = pivotScale(maior, campoSol);
    std::printf("pivô do Dó maior, campo de Sol: tônica %.0f cents, %d notas comuns\n", piv.tonic, commonTones(maior, piv));
    CHECK(std::abs(piv.tonic - 700.0) < 0.01 && commonTones(maior, piv) == 6);
    CHECK(fieldFit(piv, campoSol) > 0.99 && fieldFit(maior, campoSol) < 0.95);

    // 1b. variantes por acorde (chord-scale)
    {
        MachineScale laMenor;
        laMenor.tonic = 900.0;  // Lá
        laMenor.cents = { 0, 200, 300, 500, 700, 800, 1000 };
        std::vector<DegreeChange> ch;
        const auto v = chordVariant(laMenor, { 400.0, 800.0, 1100.0 }, &ch);  // Mi maior: Mi Sol# Si
        CHECK(ch.size() == 1 && ch[0].degree == 6 && std::abs(ch[0].delta - 100.0) < 0.01);  // Sol → Sol#
        CHECK(iguais(v.cents, { 0, 200, 300, 500, 700, 800, 1100 }));                       // a menor harmônica
        ch.clear();
        const auto l = chordVariant(maior, { 200.0, 600.0, 900.0 }, &ch);  // Dó maior sob Ré maior: Fá → Fá#
        CHECK(ch.size() == 1 && ch[0].degree == 3 && iguais(l.cents, { 0, 200, 400, 600, 700, 900, 1100 }));
        ch.clear();
        (void) chordVariant(maior, { 0.0, 400.0, 700.0 }, &ch);  // o acorde da própria escala: nada muda
        CHECK(ch.empty());
        MachineScale rast;  // maqam Rast sobre Dó: Mi meio-bemol (350)
        rast.cents = { 0, 200, 350, 500, 700, 900, 1050 };
        ch.clear();
        const auto r = chordVariant(rast, { 0.0, 400.0, 700.0 }, &ch);  // Dó maior: o Mi natural puxa o 3.º grau
        CHECK(ch.size() == 1 && ch[0].degree == 2 && std::abs(r.cents[2] - 400.0) < 0.01);
        ch.clear();
        (void) chordVariant(maior, { 100.0, 500.0, 800.0 }, &ch);  // Réb maior: a tônica Dó não se move
        CHECK(std::none_of(ch.begin(), ch.end(), [](const DegreeChange& c) { return c.degree == 0; }));
    }

    // 2. o encadeamento: cada modo faz o seu desvio, e é reprodutível
    {
        std::array<float, 12> campo {};
        for (int pc : { 0, 2, 4, 5, 7, 9, 11 })
            campo[static_cast<std::size_t>(pc)] = 0.14f;
        int seguirComuns = 0, seguirN = 0;
        for (std::uint64_t s = 1; s <= 20; ++s)
        {
            const auto f = nextScale(maior, campo, MachineMode::follow, s, nullptr);
            seguirComuns += f.common;
            seguirN += static_cast<int>(maior.cents.size());
            CHECK(f.how == nextScale(maior, campo, MachineMode::follow, s, nullptr).how);
        }
        const auto c = nextScale(maior, campo, MachineMode::counter, 1, nullptr);
        const auto t = nextScale(maior, campo, MachineMode::transfigure, 1, nullptr);
        std::printf("seguir: %d de %d notas comuns em 20 passos · contrapor: %s · transfigurar: %s\n", seguirComuns, seguirN,
                    c.how.c_str(), t.how.c_str());
        CHECK(seguirComuns * 10 >= seguirN * 6);  // desvios pequenos
        CHECK(c.how.rfind("espelho", 0) == 0 && t.how.rfind("interpolação", 0) == 0);
    }

    // 3. na peça: com --maquina, cada seção depois da 1.ª marca o desvio, e
    //    a harmonia sai dos graus de cada escala
    {
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
        for (Mode m : { Mode::follow, Mode::transfigure, Mode::counter })
        {
            Options o { m, 4, PitchSource::reference, TimeMode::automatic, 0.0f };
            o.scaleMachine = true;
            o.mosaic = false;
            const auto sc = compose(a.frames(), a.onsets(), 60.0, o);
            int secoes = 0, comMaquina = 0;
            std::string exemplo, harmonia;
            for (const auto& mk : sc.marks)
            {
                if (mk.text.rfind("seção ", 0) == 0)
                {
                    ++secoes;
                    if (mk.text.find("máquina de escalas") != std::string::npos)
                    {
                        ++comMaquina;
                        if (exemplo.empty())
                            exemplo = mk.text.substr(mk.text.find("máquina de escalas"));
                    }
                }
                if (mk.text.rfind("harmonia em movimento", 0) == 0)
                    harmonia = mk.text;
            }
            const bool tem6 = std::any_of(sc.notes.begin(), sc.notes.end(), [](const Note& n) { return n.voice == 6; });
            std::printf("  %s: %d seções, %d com desvio · %s\n", strategy::modeName(m), secoes, comMaquina,
                        exemplo.substr(0, 150).c_str());
            CHECK(secoes >= 2 && comMaquina == secoes - 1);
            CHECK(tem6 && harmonia.find("pelos graus das escalas da máquina") != std::string::npos);
            CHECK(harmonia.find("variantes da escala por acorde:") != std::string::npos);
        }
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("máquina de escalas: todas as checagens passaram");
    return EXIT_SUCCESS;
}
