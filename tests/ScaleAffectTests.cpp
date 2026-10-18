// Afeto e intenção das escalas (core/ScaleAffect.h; RASGO_DOCUMENTATION/
// teoria/ESCALAS_AFETO_INTENCAO_E_FUNCAO.md): a valência estimada ordena os
// modos diatônicos como Temperley e Tan (2013) mediram; terça, tensão e
// ambiguidade respondem à estrutura; a intenção guia a máquina de escalas.

#include "ScaleAffect.h"
#include "ScaleMachine.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace trioio::composition;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static MachineScale escala(std::vector<double> c)
{
    MachineScale s;
    s.cents = std::move(c);
    return s;
}

int main()
{
    const auto jonio = escala({ 0, 200, 400, 500, 700, 900, 1100 });
    const auto lidio = escala({ 0, 200, 400, 600, 700, 900, 1100 });
    const auto mixo = escala({ 0, 200, 400, 500, 700, 900, 1000 });
    const auto dorico = escala({ 0, 200, 300, 500, 700, 900, 1000 });
    const auto eolio = escala({ 0, 200, 300, 500, 700, 800, 1000 });
    const auto frigio = escala({ 0, 100, 300, 500, 700, 800, 1000 });
    const auto locrio = escala({ 0, 100, 300, 500, 600, 800, 1000 });

    // 1. a ordem de Temperley e Tan (2013): jônio > mixolídio, lídio > dórico > eólio > frígio
    const char* nomes[] = { "jônio", "lídio", "mixolídio", "dórico", "eólio", "frígio", "lócrio" };
    const MachineScale* modos[] = { &jonio, &lidio, &mixo, &dorico, &eolio, &frigio, &locrio };
    double v[7];
    for (int i = 0; i < 7; ++i)
    {
        const auto a = scaleAffect(*modos[i]);
        v[i] = a.valence;
        std::printf("  %-10s %s\n", nomes[i], a.describe().c_str());
    }
    CHECK(v[0] > v[1] && v[0] > v[2]);           // o jônio é o mais "alegre"
    CHECK(v[1] > v[3] && v[2] > v[3]);           // lídio e mixolídio acima do dórico
    CHECK(v[3] > v[4] && v[4] > v[5] && v[5] > v[6]);
    CHECK(scaleAffect(lidio).brightness > scaleAffect(jonio).brightness);  // o lídio é o mais "agudo"...
    CHECK(scaleAffect(lidio).tension > scaleAffect(jonio).tension);        // ...mas o trítono sobre a tônica pesa

    // 2. terça, tensão, ambiguidade
    CHECK(scaleAffect(escala({ 0, 200, 350, 500, 700, 900, 1050 })).third == 0);  // maqam Rast: terça neutra
    CHECK(scaleAffect(eolio).third == -1 && scaleAffect(jonio).third == 1);
    const auto tons = scaleAffect(escala({ 0, 200, 400, 600, 800, 1000 }));  // tons inteiros: sem quinta
    CHECK(tons.tension > scaleAffect(jonio).tension && tons.stability < scaleAffect(jonio).stability);
    std::vector<double> pelog;
    for (int k : { 0, 1, 2, 4, 5, 6, 7 })
        pelog.push_back(1200.0 * k / 9.0);
    CHECK(scaleAffect(escala(pelog)).ambiguity > 0.5 && scaleAffect(jonio).ambiguity == 0.0);
    std::string n;
    Intention it;
    CHECK(parseIntention("luminosa", it) && it == Intention::luminous && !parseIntention("feliz", it));

    // 3. a intenção guia a máquina (seguir, desvios pequenos)
    std::array<float, 12> campo {};
    campo.fill(1.0f / 12.0f);
    {
        auto s = eolio;
        const double antes = scaleAffect(s).valence;
        for (int k = 1; k <= 4; ++k)
            s = nextScale(s, campo, MachineMode::follow, static_cast<std::uint64_t>(k), nullptr,
                          static_cast<int>(Intention::luminous), 0.5).scale;
        const double depois = scaleAffect(s).valence;
        std::printf("luminosa a partir do eólio: valência %+.2f → %+.2f\n", antes, depois);
        CHECK(depois > antes + 0.3);
    }
    {
        auto s = jonio;
        const double antes = scaleAffect(s).valence;
        for (int k = 1; k <= 4; ++k)
            s = nextScale(s, campo, MachineMode::follow, static_cast<std::uint64_t>(k), nullptr,
                          static_cast<int>(Intention::somber), 0.5).scale;
        std::printf("sombria a partir do jônio: valência %+.2f → %+.2f\n", antes, scaleAffect(s).valence);
        CHECK(scaleAffect(s).valence < antes - 0.3);
    }
    {
        // o arco: a tensão sobe até 2/3 da peça e resolve
        auto s = jonio;
        std::vector<double> tensao { scaleAffect(s).tension };
        for (double pos : { 0.25, 0.45, 0.66, 0.85, 0.97 })
        {
            s = nextScale(s, campo, MachineMode::follow, static_cast<std::uint64_t>(pos * 100), nullptr,
                          static_cast<int>(Intention::arc), pos).scale;
            tensao.push_back(scaleAffect(s).tension);
        }
        std::printf("arco: tensão");
        for (double t : tensao)
            std::printf(" %.2f", t);
        std::printf("\n");
        CHECK(tensao[3] > tensao[0] + 0.15);  // no clímax (2/3)
        CHECK(tensao[5] < tensao[3]);         // resolve no fim
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("afeto e intenção: todas as checagens passaram");
    return EXIT_SUCCESS;
}
