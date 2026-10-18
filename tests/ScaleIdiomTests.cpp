// O comportamento das escalas (core/ScaleIdiom.h; RASGO_DOCUMENTATION/
// teoria/…, §6): a Yaman sobe sem Sa e Pa, desce com eles, abre com o pakaḍ,
// fecha na cadência e toca sobre bordão; o Rast desce com Sib; o blues
// arrasta as blue notes; uma escala sem comportamento não muda.

#include "Composer.h"
#include "ScaleArchive.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <map>
#include <set>
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
    CHECK(erros.empty());

    // 1. o comportamento é lido do arquivo
    const ArchiveScale* yaman = nullptr;
    for (const auto& s : *arq)
        if (s.name == "rāga Yaman")
            yaman = &s;
    CHECK(yaman && yaman->behavior.any() && yaman->behavior.up.size() == 7 && yaman->behavior.signatures.size() == 3
          && yaman->behavior.texture == "bordão" && yaman->behavior.ornament == "meend" && !yaman->behavior.source.empty());

    // referência: uma melodia que sobe e desce em Mi (60 s)
    const int mel[] = { 64, 66, 68, 69, 71, 73, 75, 76, 75, 73, 71, 69, 68, 66, 64, 66 };
    std::vector<float> v(static_cast<std::size_t>(SR * 60.0));
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.4);
        const double f = 440.0 * std::pow(2.0, (mel[static_cast<int>(t / 0.4) % 16] - 69) / 12.0);
        v[i] = static_cast<float>(0.3 * std::exp(-tt * 4.0) * std::sin(2 * PI * f * t));
    }
    Analyzer a(SR);
    a.push(v.data(), v.size());
    a.finish();
    const auto compor = [&](const std::string& termo, std::uint64_t seed) {
        Options o { Mode::follow, seed, PitchSource::archive, TimeMode::automatic, 0.0f };
        o.scaleArchive = arq;
        o.scaleQuery = termo;
        o.mosaic = false;
        return compose(a.frames(), a.onsets(), 60.0, o);
    };
    const auto marca = [](const Score& sc, const std::string& termo) {
        for (const auto& m : sc.marks)
            if (m.text.find(termo) != std::string::npos)
                return m.text;
        return std::string();
    };
    const auto tonica = [&](const Score& sc) {
        // a tônica escolhida, em cents: da marca "… sobre X"
        const auto m = marca(sc, "escala do arquivo:");
        static const char* pcs[] = { "Dó#", "Dó", "Ré", "Mib", "Mi", "Fá#", "Fá", "Sol", "Lá♭", "Lá", "Sib", "Si" };
        static const int val[] = { 1, 0, 2, 3, 4, 6, 5, 7, 8, 9, 10, 11 };
        const auto p = m.find(" sobre ");
        for (int i = 0; i < 12; ++i)
            if (m.compare(p + 7, std::string(pcs[i]).size(), pcs[i]) == 0)
                return 100.0 * val[i];
        return -1.0;
    };

    // 2. Yaman: sobe sem Sa e Pa; bordão; ornamento; assinatura e cadência
    {
        int subidas = 0, subidasEmSaPa = 0, glides = 0;
        for (std::uint64_t s = 1; s <= 4; ++s)
        {
            const auto sc = compor("rāga Yaman", s);
            const double T = tonica(sc);
            CHECK(T >= 0.0);
            CHECK(!marca(sc, "comportamento da escala").empty() && !marca(sc, "textura: bordão").empty());
            std::set<int> bordao;
            float prev = -1.0f;
            for (const auto& n : sc.notes)
            {
                if (n.voice == 6)
                    bordao.insert(static_cast<int>(std::lround(std::fmod(100.0 * n.pitch - T + 12000.0, 1200.0))) / 10);
                if (n.voice != 0)
                    continue;
                glides += std::abs(n.glide) > 0.01f;
                if (prev >= 0.0f && n.pitch > prev + 0.05f)
                {
                    // o āroha (N R G M D N S') evita o Pa e só chega ao Sa vindo do Ni
                    ++subidas;
                    const double r = std::fmod(100.0 * n.pitch - T + 12000.0, 1200.0);
                    const double rp = std::fmod(100.0 * prev - T + 12000.0, 1200.0);
                    const bool sa = std::min(r, 1200.0 - r) < 15.0, pa = std::abs(r - 701.955) < 15.0;
                    const bool deNi = std::abs(rp - 1088.269) < 15.0;
                    subidasEmSaPa += pa || (sa && !deNi);
                }
                prev = n.pitch;
            }
            CHECK(bordao.size() == 2);  // só tônica e quinta
        }
        std::printf("Yaman: %d subidas, %d em Pa ou em Sa sem vir do Ni (assinatura e cadência incluídas) · %d notas com meend\n", subidas,
                    subidasEmSaPa, glides);
        CHECK(subidas > 30 && subidasEmSaPa * 10 <= subidas);
        CHECK(glides > 10);
    }

    // 3. Rast: desce com Sib (1000 c), sobe com o Si meio-bemol (1050 c)
    {
        int desceSib = 0, desceSiNeutro = 0, sobeSiNeutro = 0;
        for (std::uint64_t s = 1; s <= 4; ++s)
        {
            const auto sc = compor("maqam Rast", s);
            const double T = tonica(sc);
            float prev = -1.0f;
            for (const auto& n : sc.notes)
            {
                if (n.voice != 0)
                    continue;
                const double r = std::fmod(100.0 * n.pitch - T + 12000.0, 1200.0);
                if (prev >= 0.0f && n.pitch < prev - 0.05f)
                {
                    desceSib += std::abs(r - 1000.0) < 15.0;
                    desceSiNeutro += std::abs(r - 1050.0) < 15.0;
                }
                if (prev >= 0.0f && n.pitch > prev + 0.05f)
                    sobeSiNeutro += std::abs(r - 1050.0) < 15.0;
                prev = n.pitch;
            }
        }
        std::printf("Rast: descendo, %d em Sib e %d no Si meio-bemol; subindo, %d no Si meio-bemol\n", desceSib, desceSiNeutro,
                    sobeSiNeutro);
        CHECK(desceSib > 0 && desceSiNeutro <= desceSib / 4);
    }

    // 4. blues: as blue notes chegam de baixo (a curva começa meio tom abaixo)
    {
        const auto sc = compor("blues (com blue notes)", 1);
        int blue = 0;
        for (const auto& n : sc.notes)
            if (n.voice == 0 && !n.bend.empty())
            {
                ++blue;
                CHECK(std::abs(n.pitchAt(0.0) - (n.pitch - 0.5)) < 1e-4 && std::abs(n.pitchAt(n.duration) - n.pitch) < 1e-4);
            }
        CHECK(blue > 0 && !marca(sc, "ornamentos (blue").empty());
    }

    // 4b. a curva de altura: interpolação e oscilação
    {
        Note n;
        n.pitch = 60.0f;
        n.duration = 1.0;
        n.bend = { { 0.0f, -1.0f }, { 0.5f, 0.0f } };
        CHECK(std::abs(n.pitchAt(0.25) - 59.5) < 1e-6 && std::abs(n.pitchAt(0.9) - 60.0) < 1e-6);
        n.bend.clear();
        n.wobbleDepth = 0.5f;
        n.wobbleRate = 2.0f;
        CHECK(std::abs(n.pitchAt(0.125) - 60.5) < 1e-4 && std::abs(n.pitchAt(0.375) - 59.5) < 1e-4);
    }

    // 4c. Bhairav: andolan (oscilação lenta); melakarta 29: gamaka (rápida), Sa e Pa firmes
    {
        const auto b = compor("rāga Bhairav", 1);
        int andolan = 0;
        for (const auto& n : b.notes)
            andolan += n.voice == 0 && n.wobbleDepth > 0.0f && n.wobbleRate < 2.0f;
        const auto g = compor("melakarta 29", 1);
        const double T = tonica(g);
        int gamaka = 0, firmesComGamaka = 0;
        for (const auto& n : g.notes)
            if (n.voice == 0 && n.wobbleDepth > 0.0f)
            {
                ++gamaka;
                const double r = std::fmod(100.0 * n.pitch - T + 12000.0, 1200.0);
                firmesComGamaka += std::min(r, 1200.0 - r) < 15.0 || std::abs(r - 702.0) < 15.0;
                CHECK(n.wobbleRate > 4.0f);
            }
        std::printf("andolan na Bhairav: %d notas · gamaka no melakarta 29: %d notas (%d em Sa/Pa)\n", andolan, gamaka,
                    firmesComGamaka);
        CHECK(andolan > 0 && gamaka > 0 && firmesComGamaka == 0);
    }

    // 4d. heterofonia (maqam Rast): a segunda voz toca a melodia um pouco
    //     depois, sem acordes nem baixo
    {
        const auto sc = compor("maqam Rast", 2);
        int mel = 0, het = 0, baixo = 0, acordes = 0;
        for (const auto& n : sc.notes)
        {
            mel += n.voice == 0;
            het += n.voice == 2;
            baixo += n.voice == 1;
            acordes += n.voice == 6;
        }
        std::printf("heterofonia no Rast: %d notas na melodia, %d na segunda voz\n", mel, het);
        CHECK(!marca(sc, "textura: heterofonia").empty());
        CHECK(baixo == 0 && acordes == 0 && het >= mel * 7 / 10);
    }

    // 4e. prática modal: o vamp dórico alterna dois acordes do modo (i e IV)
    //     sobre o pedal; a cadência frígia fecha com o b2 descendo à tônica
    {
        const auto sc = compor("modo dórico (prática modal)", 1);
        const double T = tonica(sc);
        CHECK(!marca(sc, "textura: vamp modal").empty() && !marca(sc, "I–IV").empty());
        std::set<std::set<int>> acordes;
        std::map<double, std::set<int>> porTempo;
        int baixoForaDaTonica = 0;
        for (const auto& n : sc.notes)
        {
            const int pc = static_cast<int>(std::lround(std::fmod(100.0 * n.pitch - T + 12000.0, 1200.0) / 100.0)) % 12;
            if (n.voice == 6)
                porTempo[n.time].insert(pc);
            if (n.voice == 1)
                baixoForaDaTonica += pc != 0;
        }
        for (const auto& [t, c] : porTempo)
            acordes.insert(c);
        std::printf("vamp dórico: %zu acordes diferentes em %zu tempos; baixo fora da tônica: %d\n", acordes.size(),
                    porTempo.size(), baixoForaDaTonica);
        // o dórico modal usa o voicing So What (três quartas e uma terça
        // maior): sobre o i, Ré Sol Dó Fá Lá = {0, 5, 10, 3, 7}; dois acordes
        CHECK(acordes.size() == 2 && acordes.count({ 0, 3, 5, 7, 10 }) == 1);
        for (const auto& c : acordes)
            CHECK(c.size() == 5);
        CHECK(!marca(sc, "So What").empty());
        CHECK(baixoForaDaTonica == 0);

        const auto fr = compor("modo frígio (prática modal)", 1);
        const double Tf = tonica(fr);
        std::vector<int> mel;
        for (const auto& n : fr.notes)
            if (n.voice == 0)
                mel.push_back(static_cast<int>(std::lround(std::fmod(100.0 * n.pitch - Tf + 12000.0, 1200.0) / 100.0)) % 12);
        int cadencias = 0;
        for (std::size_t i = 1; i < mel.size(); ++i)
            cadencias += mel[i - 1] == 1 && mel[i] == 0;
        std::printf("frígio: %d chegadas b2 → 1\n", cadencias);
        CHECK(cadencias >= 2);
    }

    // 5. sem comportamento: nada muda e não há marca
    {
        const auto sc = compor("Werckmeister III", 1);
        CHECK(marca(sc, "comportamento da escala").empty() && marca(sc, "textura: bordão").empty());
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("comportamento das escalas: todas as checagens passaram");
    return EXIT_SUCCESS;
}
