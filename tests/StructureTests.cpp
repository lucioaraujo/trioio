#include "Composer.h"
#include "Structure.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>

using namespace trioio::perception;

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

static double midiHz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

static void somarAcorde(std::vector<float>& v, double t0, double t1, std::initializer_list<double> notas, double amp)
{
    for (auto i = static_cast<std::size_t>(t0 * SR); i < static_cast<std::size_t>(t1 * SR) && i < v.size(); ++i)
        for (const double m : notas)
            v[i] += static_cast<float>(amp * std::sin(2.0 * PI * midiHz(m) * static_cast<double>(i) / SR));
}

static void somarCliques(std::vector<float>& v, const std::vector<double>& tempos)
{
    for (const double t : tempos)
    {
        const auto i0 = static_cast<std::size_t>(t * SR);
        for (std::size_t k = 0; k < 480 && i0 + k < v.size(); ++k)
            v[i0 + k] += static_cast<float>(0.8 * std::exp(-static_cast<double>(k) / 80.0)
                                             * std::sin(2.0 * PI * 2500.0 * static_cast<double>(k) / SR));
    }
}

static Analyzer analisar(const std::vector<float>& v)
{
    Analyzer a(SR);
    a.push(v.data(), v.size());
    a.finish();
    return a;
}

static std::set<int> top3(const PitchField& f)
{
    std::vector<int> idx(12);
    for (int i = 0; i < 12; ++i)
        idx[static_cast<std::size_t>(i)] = i;
    std::sort(idx.begin(), idx.end(), [&](int a, int b) { return f[static_cast<std::size_t>(a)] > f[static_cast<std::size_t>(b)]; });
    return { idx[0], idx[1], idx[2] };
}

int main()
{
    // 1. croma: Dó maior e Lá menor
    {
        std::vector<float> v(static_cast<std::size_t>(SR * 2.0), 0.0f);
        somarAcorde(v, 0.0, 2.0, { 60, 64, 67 }, 0.2);
        const auto a = analisar(v);
        CHECK((top3(harmonicField(a.frames(), 0.0, 2.0)) == std::set<int> { 0, 4, 7 }));
        std::vector<float> w(static_cast<std::size_t>(SR * 2.0), 0.0f);
        somarAcorde(w, 0.0, 2.0, { 57, 60, 64 }, 0.2);
        const auto b = analisar(w);
        CHECK((top3(harmonicField(b.frames(), 0.0, 2.0)) == std::set<int> { 9, 0, 4 }));
    }

    // 2. picos espectrais: 440 e 1000 Hz
    {
        std::vector<float> v(static_cast<std::size_t>(SR * 1.0), 0.0f);
        for (std::size_t i = 0; i < v.size(); ++i)
            v[i] = static_cast<float>(0.3 * std::sin(2.0 * PI * 440.0 * i / SR) + 0.2 * std::sin(2.0 * PI * 1000.0 * i / SR));
        const auto a = analisar(v);
        const auto& f = a.frames()[10];
        std::printf("picos: %.1f  %.1f Hz\n", f.peaksHz[0], f.peaksHz[1]);
        CHECK(std::abs(f.peaksHz[0] - 440.0f) < 3.0f);
        CHECK(std::abs(f.peaksHz[1] - 1000.0f) < 3.0f);
    }

    // 3. pulso: 120 e 90 BPM, com fase conhecida
    for (const auto& [bpm, fase] : { std::pair { 120.0, 0.30 }, std::pair { 90.0, 0.10 } })
    {
        std::vector<float> v(static_cast<std::size_t>(SR * 20.0), 0.0f);
        std::vector<double> t;
        for (double x = fase; x < 20.0; x += 60.0 / bpm)
            t.push_back(x);
        somarCliques(v, t);
        const auto a = analisar(v);
        const auto p = estimatePulse(a.frames(), a.onsets(), 0.0, 20.0);
        double erroFase = std::fmod(std::abs(p.phase - fase), p.period);
        erroFase = std::min(erroFase, p.period - erroFase);
        std::printf("pulso: %.1f BPM (esperado %.0f), fase erro %.1f ms, confiança %.2f\n", p.bpm, bpm, 1000 * erroFase, p.confidence);
        CHECK(std::abs(p.bpm - bpm) < 2.0);
        CHECK(erroFase < 0.03);
        CHECK(p.confidence > 0.3f);
    }

    // 4. ruído não tem pulso claro
    {
        std::vector<float> v(static_cast<std::size_t>(SR * 20.0));
        std::uint32_t x = 5;
        for (auto& s : v)
        {
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            s = static_cast<float>(0.2 * (x / 4294967295.0 * 2.0 - 1.0));
        }
        const auto an = analisar(v);
        const auto p = estimatePulse(an.frames(), an.onsets(), 0.0, 20.0);
        std::printf("ruído: confiança %.2f\n", p.confidence);
        CHECK(p.confidence < 0.3f);
    }

    // 5. grade rítmica: padrão 0, 3, 6, 8, 12 a 100 BPM
    {
        const double per = 0.6, passo = per / 4.0, comp = per * 4.0;
        std::vector<float> v(static_cast<std::size_t>(SR * 20.0), 0.0f);
        std::vector<double> t;
        for (double b = 0.2; b + comp < 20.0; b += comp)
            for (const int k : { 0, 3, 6, 8, 12 })
                t.push_back(b + k * passo);
        somarCliques(v, t);
        const auto a = analisar(v);
        Pulse p;  // o pulso certo, para testar a grade isoladamente
        p.period = per;
        p.bpm = 100.0;
        p.phase = 0.2;
        const auto barras = rhythmGrid(a.onsets(), p, 0.0, 20.0);
        int certas = 0;
        for (const auto& b : barras)
        {
            std::set<int> h;
            for (int k = 0; k < b.steps; ++k)
                if (b.hits[static_cast<std::size_t>(k)] > 0.0f)
                    h.insert(k);
            certas += h == std::set<int> { 0, 3, 6, 8, 12 };
        }
        std::printf("grade: %d de %zu compassos exatos\n", certas, barras.size());
        CHECK(barras.size() >= 7);
        CHECK(certas >= static_cast<int>(barras.size()) - 1);
    }

    // 5b. compasso: padrões que se repetem a cada 3 e a cada 5 tempos
    for (const auto& [tempos, padrao] : { std::pair { 3, std::vector<int> { 0, 6, 8 } },
                                          std::pair { 5, std::vector<int> { 0, 6, 12, 16 } } })
    {
        const double per = 0.5, passo = per / 4.0, comp = per * tempos;
        std::vector<float> v(static_cast<std::size_t>(SR * 24.0), 0.0f);
        std::vector<double> t;
        for (double b = 0.3; b + comp < 24.0; b += comp)
            for (const int k : padrao)
                t.push_back(b + k * passo);
        somarCliques(v, t);
        const auto a = analisar(v);
        const auto p = estimatePulse(a.frames(), a.onsets(), 0.0, 24.0);
        std::printf("compasso: %d tempos a %.1f BPM = ciclo %.2f s (esperado %.2f s)\n", p.beatsPerBar, p.bpm,
                    p.beatsPerBar * p.period, comp);
        // o ciclo do compasso é o que importa; 3/4 e 6/8 são leituras igualmente válidas do 0-6-8
        CHECK(std::abs(p.beatsPerBar * p.period - comp) < 0.03);
        // o 0-6-12-16 é 3+3+2+2 colcheias: 5 tempos a 120, ou um aksak de 4
        // tempos desiguais (longo-longo-curto-curto; Brăiloiu, 1951), cuja
        // média é ~96 BPM. As duas leituras são válidas; a preferência pelo
        // pulso de contagem em 100 BPM (Parncutt 1994) escolhe a segunda
        if (tempos == 5)
            CHECK((p.beatsPerBar == 5 && std::abs(p.bpm - 120.0) < 2.0) || (p.beatsPerBar == 4 && std::abs(p.bpm - 96.0) < 2.0));
        const auto barras = rhythmGrid(a.onsets(), p, 0.0, 24.0);
        CHECK(!barras.empty() && barras.front().steps == p.beatsPerBar * 4);
    }
    // metrônomo sem acento: fica em 4
    {
        std::vector<float> v(static_cast<std::size_t>(SR * 20.0), 0.0f);
        std::vector<double> t;
        for (double x = 0.3; x < 20.0; x += 0.5)
            t.push_back(x);
        somarCliques(v, t);
        const auto a = analisar(v);
        CHECK(estimatePulse(a.frames(), a.onsets(), 0.0, 20.0).beatsPerBar == 4);
    }

    // 6. seções: Dó maior esparso | ruído denso | Fá# maior
    {
        std::vector<float> v(static_cast<std::size_t>(SR * 45.0), 0.0f);
        somarAcorde(v, 0.0, 15.0, { 60, 64, 67 }, 0.12);
        std::uint32_t x = 3;
        for (auto i = static_cast<std::size_t>(15.0 * SR); i < static_cast<std::size_t>(30.0 * SR); ++i)
        {
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            v[i] = static_cast<float>(0.3 * (x / 4294967295.0 * 2.0 - 1.0) * (std::fmod(i / SR, 0.125) < 0.03 ? 1.0 : 0.1));
        }
        somarAcorde(v, 30.0, 45.0, { 66, 70, 73 }, 0.12);
        const auto a = analisar(v);
        const auto s = findSections(a.frames(), a.onsets(), 45.0);
        for (const auto& x : s)
            std::printf("seção %.0f–%.0f s\n", x.start, x.end);
        CHECK(s.size() == 3);
        if (s.size() == 3)
        {
            CHECK(std::abs(s[1].start - 15.0) <= 2.0 && std::abs(s[2].start - 30.0) <= 2.0);
            CHECK((top3(s[0].field) == std::set<int> { 0, 4, 7 }));
            CHECK((top3(s[2].field) == std::set<int> { 6, 10, 1 }));
        }
    }

    // 7. forma por repetição: A B A B (12 s cada); A em Dó maior claro, B em
    //    Lá menor escuro e ruidoso
    {
        std::vector<float> v(static_cast<std::size_t>(SR * 48.0), 0.0f);
        std::uint32_t r = 7;
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            const double t = static_cast<double>(i) / SR;
            const bool a = static_cast<int>(t / 12.0) % 2 == 0;
            const double env = 0.6 + 0.4 * std::exp(-std::fmod(t, 0.5) * 8.0);
            double x = 0.0;
            for (int m : a ? std::vector<int> { 60, 64, 67, 72 } : std::vector<int> { 57, 60, 64 })
                x += std::sin(2.0 * PI * 440.0 * std::pow(2.0, (m - 69) / 12.0) * t) * (a ? 0.12 : 0.1);
            if (!a)
            {
                r = r * 1664525u + 1013904223u;
                x += 0.05 * (static_cast<double>(r >> 8) / 8388608.0 - 1.0);
            }
            v[i] = static_cast<float>(x * env);
        }
        const auto an = analisar(v);
        std::vector<int> letras;
        const auto secs = findSectionsByRepetition(an.frames(), an.onsets(), 48.0, letras);
        std::printf("forma por repetição:");
        for (std::size_t k = 0; k < secs.size(); ++k)
            std::printf(" %c(%.0f–%.0f)", 'A' + letras[k], secs[k].start, secs[k].end);
        std::printf("\n");
        CHECK((letras == std::vector<int> { 0, 1, 0, 1 }));
        bool fronteiras = secs.size() == 4;
        for (std::size_t k = 1; fronteiras && k < secs.size(); ++k)
            fronteiras = std::abs(secs[k].start - 12.0 * k) <= 1.5;
        CHECK(fronteiras);
        // na composição: a forma é marcada e a 3.ª seção retoma a 1.ª
        trioio::composition::Options o;
        o.formByRepetition = true;
        o.mosaic = false;
        const auto sc = trioio::composition::compose(an.frames(), an.onsets(), 48.0, o);
        bool forma = false, retorno = false;
        for (const auto& m : sc.marks)
        {
            forma = forma || m.text.find("forma por repetição (Foote 2000; Müller 2015): A B A B") != std::string::npos;
            retorno = retorno || (m.text.rfind("seção 3", 0) == 0 && m.text.find("retorno de A (como na seção 1") != std::string::npos);
        }
        CHECK(forma && retorno);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("estrutura: todas as checagens passaram");
    return EXIT_SUCCESS;
}
