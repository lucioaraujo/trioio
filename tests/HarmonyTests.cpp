// Harmonia em movimento (core/Harmony.h, docs/HARMONIA.md): os acordes de
// uma progressão conhecida são lidos; o baixo segue a fundamental; o motivo
// cai no acorde nos ataques fortes; a voz de harmonia conduz com pouco
// movimento; cada modo transforma a progressão; uma linha sozinha também
// gera acordes; nas afinações microtonais a camada se desliga.

#include "Composer.h"
#include "Harmony.h"

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

struct Acorde { int raiz; bool maior; };
// Sol menor: i V i iv VI III V i — 2 s cada (um compasso a 120 BPM), 2 voltas
const Acorde kProg[] = { { 7, false }, { 2, true }, { 7, false }, { 0, false }, { 10, true }, { 3, true }, { 2, true }, { 7, false } };

static double hz(double midi) { return 440.0 * std::pow(2.0, (midi - 69.0) / 12.0); }

static Analyzer escutar(const std::vector<float>& v)
{
    Analyzer a(SR);
    a.push(v.data(), v.size());
    a.finish();
    return a;
}

int main()
{
    // referência: tríades sustentadas + baixo + um ataque por tempo (para o pulso)
    std::vector<float> v(static_cast<std::size_t>(SR * 32.0));
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR;
        const auto& c = kProg[static_cast<int>(t / 2.0) % 8];
        const double tb = std::fmod(t, 0.5);
        double s = 0.0;
        for (int k : { 0, c.maior ? 4 : 3, 7 })
            s += std::sin(2 * PI * hz(60 + c.raiz + k) * t);
        s = 0.12 * s + 0.15 * std::sin(2 * PI * hz(36 + c.raiz) * t);
        v[i] = static_cast<float>(s * (0.6 + 0.4 * std::exp(-tb * 12.0)));
    }
    const auto a = escutar(v);

    // 1. leitura: os 16 acordes, nas posições certas
    {
        const auto lidos = readChords(a.frames(), 32.0, 0.0, 0.5);
        int certos = 0, segundos = 0;
        for (const auto& s : lidos)
            for (double t = std::ceil(s.start) + 0.5; t < s.end; t += 1.0)
            {
                const auto& c = kProg[static_cast<int>(t / 2.0) % 8];
                ++segundos;
                certos += s.chord.root == c.raiz && (s.chord.quality == ChordQuality::major) == c.maior;
            }
        std::printf("leitura: %zu trechos, %d de %d segundos com o acorde certo\n", lidos.size(), certos, segundos);
        CHECK(segundos >= 28 && certos * 10 >= segundos * 9);
    }

    const auto compor = [&](Mode m, std::uint64_t seed, PitchSource p = PitchSource::reference, bool harmonia = true) {
        Options o { m, seed, p, TimeMode::pulse, 0.0f };
        o.harmony = harmonia;
        o.mosaic = false;
        o.rhythm = false;
        return compose(a.frames(), a.onsets(), 32.0, o);
    };
    const auto marca = [](const Score& sc, const std::string& termo) {
        for (const auto& m : sc.marks)
            if (m.text.find(termo) != std::string::npos)
                return m.text;
        return std::string();
    };

    // 2. seguir: baixo na fundamental, motivo no acorde, voz de harmonia conduzida
    {
        int baixos = 0, baixosNaRaiz = 0, fortes = 0, fortesNoAcorde = 0, harmonia = 0;
        double movimento = 0.0;
        int trocas = 0;
        for (std::uint64_t s = 1; s <= 4; ++s)
        {
            const auto sc = compor(Mode::follow, s);
            std::vector<std::vector<float>> acordes;
            double tAnterior = -1.0;
            for (const auto& n : sc.notes)
            {
                const auto& c = kProg[static_cast<int>((n.time + 0.01) / 2.0) % 8];  // a grade medida fica a ms da troca
                const int pc = ((static_cast<int>(std::lround(n.pitch)) % 12) + 12) % 12;
                const bool noAcorde = pc == c.raiz || pc == (c.raiz + (c.maior ? 4 : 3)) % 12 || pc == (c.raiz + 7) % 12;
                if (n.voice == 1 && std::fmod(n.time, 2.0) < 1.9)
                {
                    ++baixos;
                    baixosNaRaiz += pc == c.raiz;
                }
                const double r = std::fmod(n.time + 0.025, 2.0);  // perto de uma troca de acorde
                if (n.voice == 0 && r < 0.05)
                {
                    ++fortes;
                    fortesNoAcorde += noAcorde;
                }
                if (n.voice == 6)
                {
                    ++harmonia;
                    if (std::abs(n.time - tAnterior) > 1e-6)
                        acordes.push_back({});
                    acordes.back().push_back(n.pitch);
                    tAnterior = n.time;
                }
            }
            for (auto& x : acordes)
                std::sort(x.begin(), x.end());  // as 3 notas, de baixo para cima
            for (std::size_t k = 1; k < acordes.size(); ++k)
                if (acordes[k].size() == 3 && acordes[k - 1].size() == 3)
                {
                    for (std::size_t j = 0; j < 3; ++j)
                        movimento += std::abs(acordes[k][j] - acordes[k - 1][j]);
                    ++trocas;
                }
            CHECK(!marca(sc, "harmonia em movimento: 16 acordes").empty() || !marca(sc, "harmonia em movimento:").empty());
        }
        std::printf("seguir: baixo na fundamental %d/%d · ataques fortes do motivo no acorde %d/%d · "
                    "voz de harmonia: %d notas, movimento médio %.1f semitons por troca\n",
                    baixosNaRaiz, baixos, fortesNoAcorde, fortes, harmonia, trocas ? movimento / trocas : 0.0);
        CHECK(baixos > 20 && baixosNaRaiz * 10 >= baixos * 9);
        CHECK(fortes > 10 && fortesNoAcorde * 10 >= fortes * 9);
        CHECK(harmonia > 30 && trocas > 10 && movimento / trocas <= 5.0);
    }

    // 3. transfigurar e contrapor transformam a progressão, e marcam como
    {
        const auto t = marca(compor(Mode::transfigure, 2), "harmonia em movimento");
        const auto c = marca(compor(Mode::counter, 3), "harmonia em movimento");
        std::printf("transfigurar: %s\ncontrapor: %s\n", t.c_str(), c.c_str());
        CHECK(t.find("neo-riemanniana") != std::string::npos);
        CHECK(c.find("rearmonizada") != std::string::npos && c.find(": 0 substituições") == std::string::npos);
        // a operação L sobre Sol menor dá Mib maior: duas notas comuns (Sol, Sib)
        const Chord mib { 3, ChordQuality::major };
        CHECK(mib.contains(7) && mib.contains(10));
    }

    // 4. uma linha sozinha (arpejos da progressão, uma nota por vez) gera acordes
    {
        std::vector<float> l(static_cast<std::size_t>(SR * 32.0));
        for (std::size_t i = 0; i < l.size(); ++i)
        {
            const double t = static_cast<double>(i) / SR;
            const auto& c = kProg[static_cast<int>(t / 2.0) % 8];
            const int k = static_cast<int>(t / 0.5) % 4;
            const int iv[4] = { 0, c.maior ? 4 : 3, 7, 12 };
            const double tt = std::fmod(t, 0.5);
            l[i] = static_cast<float>(0.3 * std::exp(-tt * 3.0) * std::sin(2 * PI * hz(60 + c.raiz + iv[k]) * t));
        }
        const auto al = escutar(l);
        const auto lidos = readChords(al.frames(), 32.0, 0.0, 0.5);
        int certos = 0, segundos = 0;
        for (const auto& s : lidos)
            for (double t = std::ceil(s.start) + 0.5; t < s.end; t += 1.0)
            {
                const auto& c = kProg[static_cast<int>(t / 2.0) % 8];
                ++segundos;
                certos += s.chord.root == c.raiz && (s.chord.quality == ChordQuality::major) == c.maior;
            }
        std::printf("linha sozinha: %d de %d segundos com o acorde certo\n", certos, segundos);
        CHECK(segundos >= 24 && certos * 10 >= segundos * 8);
    }

    // 5. microtonal: a harmonia é feita dos graus da escala (aqui a justa:
    //    terças de 5/4 = 3,86 semitons); desligada: sem voz de harmonia
    {
        const auto j = compor(Mode::follow, 1, PitchSource::just);
        const auto off = compor(Mode::follow, 1, PitchSource::reference, false);
        const auto tem6 = [](const Score& sc) {
            return std::any_of(sc.notes.begin(), sc.notes.end(), [](const Note& n) { return n.voice == 6; });
        };
        int fracionarias = 0;
        for (const auto& n : j.notes)
            fracionarias += n.voice == 6 && std::abs(n.pitch - std::round(n.pitch)) > 0.05f;
        std::printf("justa: %s\n", marca(j, "harmonia em movimento").substr(0, 160).c_str());
        CHECK(tem6(j) && !marca(j, "pelos graus da escala").empty() && fracionarias > 0);
        CHECK(!tem6(off));
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("harmonia em movimento: todas as checagens passaram");
    return EXIT_SUCCESS;
}
