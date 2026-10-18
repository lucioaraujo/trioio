// O arquivo de escalas do Trioio (core/ScaleArchive.h, data/escalas/,
// docs/ESCALAS.md): o arquivo inteiro é legível e citado; valores conferidos
// com a literatura; a busca por termo e o ajuste ao campo; .scl avulso; o
// compositor toca a escala escolhida, inclusive não oitavante.

#include "Composer.h"
#include "ScaleArchive.h"

#include <array>
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

static const ArchiveScale* achar(const std::vector<ArchiveScale>& a, const std::string& nome)
{
    for (const auto& s : a)
        if (s.name == nome)
            return &s;
    return nullptr;
}

static bool grau(const ArchiveScale* s, double cents)
{
    return s && std::any_of(s->stepsCents.begin(), s->stepsCents.end(), [&](double c) { return std::abs(c - cents) < 0.01; });
}

int main()
{
    std::ifstream in(TRIOIO_SOURCE_DIR "/data/escalas/arquivo_trioio.txt");
    std::stringstream ss;
    ss << in.rdbuf();
    std::vector<std::string> erros;
    const auto arq = parseScaleArchive(ss.str(), erros);

    // 1. o arquivo inteiro: legível, citado, graus em ordem dentro do período
    {
        int semFonte = 0, foraDeOrdem = 0;
        for (const auto& s : arq)
        {
            semFonte += s.source.empty();
            foraDeOrdem += s.stepsCents.empty() || s.stepsCents.front() != 0.0
                        || !std::is_sorted(s.stepsCents.begin(), s.stepsCents.end()) || s.stepsCents.back() >= s.periodCents;
        }
        std::printf("arquivo: %zu escalas, %zu erros, %d sem fonte, %d fora de ordem\n", arq.size(), erros.size(), semFonte, foraDeOrdem);
        CHECK(arq.size() >= 180 && erros.empty() && semFonte == 0 && foraDeOrdem == 0);
    }

    // 2. valores conferidos com a literatura (cents)
    CHECK(grau(achar(arq, "Werckmeister III"), 390.225) && grau(achar(arq, "Werckmeister III"), 1092.180));
    CHECK(grau(achar(arq, "Kirnberger III"), 386.314) && grau(achar(arq, "Kirnberger III"), 590.224));
    CHECK(grau(achar(arq, "mesotônico de 1/4 de coma"), 193.157) && grau(achar(arq, "mesotônico de 1/4 de coma"), 696.578));
    CHECK(grau(achar(arq, "Ptolomeu diatônico intenso"), 315.641));      // 6/5: na oitava dórica começa pelo semitom 16/15
    CHECK(grau(achar(arq, "justa diatônica de Zarlino"), 386.314));      // 5/4
    CHECK(grau(achar(arq, "makam Rast"), 1200.0 * 9 / 53));              // 9 comas de Holder
    CHECK(grau(achar(arq, "maqam Rast"), 350.0));                         // a terça neutra do modelo
    {
        const auto* p = achar(arq, "Partch 43 tons");
        CHECK(p && p->stepsCents.size() == 43);
        const auto* bp = achar(arq, "Bohlen-Pierce justa");
        CHECK(bp && std::abs(bp->periodCents - 1901.955) < 0.01 && bp->stepsCents.size() == 13);
        const auto* m15 = achar(arq, "melakarta 15 Māyāmāḷavagauḷa");
        CHECK(m15 && m15->stepsCents == (std::vector<double> { 0, 100, 400, 500, 700, 800, 1100 }));
        int melakartas = 0;
        for (const auto& s : arq)
            melakartas += s.family == "Índia carnática (melakarta)";
        CHECK(melakartas == 72);
        // as famílias geradas por regra
        int forte[13] = {}, especies = 0, edos = 0, mos = 0;
        for (const auto& s : arq)
        {
            if (s.family.rfind("conjunto de classes de altura", 0) == 0)
                ++forte[s.stepsCents.size()];
            especies += s.family.find("espécie de oitava") != std::string::npos;
            edos += s.family == "divisão igual da oitava";
            mos += s.family == "MOS (escala bem formada)";
        }
        // as formas primas de Forte: 38, 50, 38, 29 e 12 conjuntos de 5 a 9 notas
        CHECK(forte[5] == 38 && forte[6] == 50 && forte[7] == 38 && forte[8] == 29 && forte[9] == 12);
        CHECK(especies == 20 * 6 && edos == 95 && mos > 1000);
        // a hexany {1 3 5 7} de Wilson (a partir de 35/32): 1 8/7 6/5 48/35 8/5 12/7
        const auto* hx = achar(arq, "hexany {1 3 5 7}");
        CHECK(hx && hx->stepsCents.size() == 6 && grau(hx, 1200.0 * std::log2(48.0 / 35.0)));
        // Messiaen modo 3: 9 notas
        CHECK(achar(arq, "Messiaen modo 3") && achar(arq, "Messiaen modo 3")->stepsCents.size() == 9);
    }

    // 3. a busca e o ajuste ao campo
    {
        // um campo de Hijaz sobre Ré: Ré Mib Fá# Sol Lá Sib Dó
        std::array<float, 12> campo {};
        for (int pc : { 2, 3, 6, 7, 9, 10, 0 })
            campo[static_cast<std::size_t>(pc)] = pc == 2 ? 0.25f : 0.125f;
        const auto h = fitArchive(arq, campo, "maqam", 1, 1);
        std::printf("campo de Hijaz, entre os maqamat: %s (%d candidatas)\n", h.choice.name.c_str(), h.candidates);
        CHECK(h.error.empty() && h.candidates >= 11 && h.choice.name.rfind("maqam Hijaz ", 0) == 0 && h.choice.tonic == 2.0f);
        const auto p = fitArchive(arq, campo, "pelog", 1);
        CHECK(p.error.empty() && p.candidates == 2 && !p.source.empty());
        CHECK(!fitArchive(arq, campo, "nenhuma-escala-assim", 1).error.empty());
        // as famílias em palavras do autor, sem depender do acento (2026-10-18)
        const auto gregos = fitArchive(arq, campo, "modos gregos", 1);
        CHECK(gregos.error.empty() && gregos.candidates >= 100);
        const auto africa = fitArchive(arq, campo, "escalas africanas", 1);
        CHECK(africa.error.empty() && africa.candidates >= 2);
        CHECK(fitArchive(arq, campo, "grecia antiga", 1).candidates == gregos.candidates);
        const auto med = fitArchive(arq, campo, "escalas medievais", 1);  // os modos da prática modal
        CHECK(med.error.empty() && med.candidates >= 3);
        const auto dimdom = fitArchive(arq, campo, "dim dom", 1);
        CHECK(dimdom.candidates == 1 && dimdom.choice.name.rfind("Messiaen modo 2", 0) == 0);  // a diminuta (octatônica)
        CHECK(fitArchive(arq, campo, "miles davis", 1).error.empty());  // os modos dóricos
        const auto todas = fitArchive(arq, campo, "", 1);
        CHECK(todas.error.empty() && todas.candidates > 100 && todas.period == 12.0f);
        const auto bp = fitArchive(arq, campo, "Bohlen-Pierce igual", 1);
        CHECK(bp.error.empty() && std::abs(bp.period - 19.01955f) < 0.001f);
    }

    // 4. um .scl avulso (formato aberto): pentatônica justa escrita aqui
    {
        const auto s = parseScl("! teste.scl\n!\nPentatônica justa de teste\n 5\n!\n 9/8\n 5/4\n 3/2\n 5/3\n 2/1\n", "teste");
        CHECK(s && s->stepsCents.size() == 5 && std::abs(s->stepsCents[2] - 386.3137) < 0.001 && s->periodCents == 1200.0);
        CHECK(!parseScl("lixo\n", "x"));
    }

    // 5. no compositor: escala do arquivo marcada, pelog fora dos 12 sons,
    //    Bohlen-Pierce sem oitava
    {
        const int mel[] = { 62, 63, 66, 67, 69, 70, 72, 74 };
        std::vector<float> v(static_cast<std::size_t>(SR * 24.0));
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            const double t = static_cast<double>(i) / SR, tt = std::fmod(t, 0.5);
            const double f = 440.0 * std::pow(2.0, (mel[static_cast<int>(t / 0.5) % 8] - 69) / 12.0);
            v[i] = static_cast<float>(0.3 * std::exp(-tt * 4.0) * std::sin(2 * PI * f * t));
        }
        Analyzer a(SR);
        a.push(v.data(), v.size());
        a.finish();
        const auto lib = std::make_shared<const std::vector<ArchiveScale>>(arq);
        for (const std::string termo : { "pelog (modelo", "Bohlen-Pierce igual", "maqam" })
        {
            Options o { Mode::follow, 3, PitchSource::archive, TimeMode::automatic, 0.0f };
            o.scaleArchive = lib;
            o.scaleQuery = termo;
            o.mosaic = false;
            const auto sc = compose(a.frames(), a.onsets(), 24.0, o);
            std::string marca;
            for (const auto& m : sc.marks)
                if (m.text.rfind("escala do arquivo:", 0) == 0)
                    marca = m.text;
            int fracionarias = 0, motivo = 0;
            for (const auto& n : sc.notes)
                if (n.voice == 0)
                {
                    ++motivo;
                    fracionarias += std::abs(n.pitch - std::round(n.pitch)) > 0.05f;
                }
            std::printf("  %-20s · %d notas, %d fora dos 12 sons · %s\n", termo.c_str(), motivo, fracionarias,
                        marca.substr(0, 110).c_str());
            CHECK(!marca.empty() && marca.find("fonte:") != std::string::npos && motivo > 10);
            if (termo != "maqam")
                CHECK(fracionarias > motivo / 2);
        }
        // sem arquivo carregado: avisa e usa o campo da referência
        Options o { Mode::follow, 3, PitchSource::archive, TimeMode::automatic, 0.0f };
        const auto sc = compose(a.frames(), a.onsets(), 24.0, o);
        CHECK(std::any_of(sc.marks.begin(), sc.marks.end(), [](const Mark& m) { return m.text.find("não carregado") != std::string::npos; }));
    }

    // reconhecer a linguagem (rankArchive): um campo de Lá frígio dominante
    // (Lá Sib Dó# Ré Mi Fá Sol, Lá e Mi mais fortes) acha o Hijaz, o
    // melakarta 14 ou o modo andaluz SOBRE LÁ entre as primeiras
    {
        std::array<float, 12> campo {};
        const std::pair<int, float> notas[] = { { 9, 0.25f }, { 10, 0.08f }, { 1, 0.14f }, { 2, 0.11f },
                                                { 4, 0.18f }, { 5, 0.10f }, { 7, 0.08f } };
        for (const auto& [pc, w] : notas)
            campo[static_cast<std::size_t>(pc)] = w;
        const auto top = rankArchive(arq, campo, 8, 5, 9);
        bool achou = false;
        for (const auto& r : top)
        {
            const bool lingua = r.scale->name.find("Hijaz") != std::string::npos || r.scale->name == "melakarta 14"
                             || r.scale->family.find("flamenco") != std::string::npos;
            achou = achou || (lingua && r.tonic == 9);
            std::printf("  reconhecer: %.2f %s sobre %d\n", r.score, r.scale->name.c_str(), r.tonic);
        }
        CHECK(achou);
        CHECK(!top.empty() && top.front().tonic == 9);
        // a escolha automática (fitArchive sem termo): só linguagens, sobre
        // Lá, e entre as equivalentes as que trazem o comportamento (Hijaz,
        // modo andaluz), não as que só têm as notas (melakarta 14)
        int comComportamento = 0;
        for (std::uint64_t seed = 1; seed <= 8; ++seed)
        {
            const auto pick = fitArchive(arq, campo, "", seed);
            comComportamento += pick.behavior.any() && pick.choice.tonic == 9.0f;
            std::printf("  escolha automática (seed %d): %s\n", static_cast<int>(seed), pick.choice.name.c_str());
        }
        CHECK(comComportamento == 8);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("arquivo de escalas: todas as checagens passaram");
    return EXIT_SUCCESS;
}
