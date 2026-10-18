#include "Descriptors.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

static std::vector<float> seno(double hz, double amp, double seg)
{
    std::vector<float> v(static_cast<std::size_t>(SR * seg));
    for (std::size_t i = 0; i < v.size(); ++i)
        v[i] = static_cast<float>(amp * std::sin(2.0 * PI * hz * static_cast<double>(i) / SR));
    return v;
}

static std::vector<float> ruido(double amp, double seg, std::uint32_t semente = 1)
{
    std::vector<float> v(static_cast<std::size_t>(SR * seg));
    std::uint32_t x = semente;
    for (auto& s : v)
    {
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;  // xorshift: determinístico
        s = static_cast<float>(amp * (static_cast<double>(x) / 4294967295.0 * 2.0 - 1.0));
    }
    return v;
}

static Analyzer analisar(const std::vector<float>& v, std::size_t bloco = 512)
{
    Analyzer a(SR);
    for (std::size_t i = 0; i < v.size(); i += bloco)
        a.push(v.data() + i, std::min(bloco, v.size() - i));
    a.finish();
    return a;
}

static double duracao(const std::vector<float>& v) { return static_cast<double>(v.size()) / SR; }

int main()
{
    // 1. silêncio: nada soa, nenhum ataque, energia zero
    {
        const std::vector<float> v(static_cast<std::size_t>(SR * 2.0), 0.0f);
        const auto a = analisar(v);
        const auto s = summarize(a.frames(), a.onsets(), 0.0, 2.0);
        CHECK(a.onsets().empty());
        CHECK(s.activeShare == 0.0f && s.energy == 0.0f && s.brightness == 0.0f);
    }

    // 2. seno de 1 kHz em amplitude 0,5: RMS −9,03 dBFS, centroide ≈ 1 kHz,
    //    tonal, sem ataques depois do começo
    {
        const auto v = seno(1000.0, 0.5, 3.0);
        const auto a = analisar(v);
        const auto s = summarize(a.frames(), a.onsets(), 0.5, 2.5);
        std::printf("seno: rms %.2f dB  centroide %.1f Hz  planura %.4f  ataques %zu\n",
                    s.meanRmsDb, s.centroidHz, s.noisiness, a.onsets().size());
        CHECK(std::abs(s.meanRmsDb - (-9.03f)) < 0.1f);
        CHECK(std::abs(s.centroidHz - 1000.0f) < 30.0f);
        CHECK(s.noisiness < 0.01f);
        CHECK(s.onsetCount == 0);
        CHECK(a.onsets().size() <= 1);  // no máximo o início
    }

    // 3. ruído branco: planura alta, centroide perto de sr/4, sem ataques no meio
    {
        const auto v = ruido(0.3, 4.0);
        const auto a = analisar(v);
        const auto s = summarize(a.frames(), a.onsets(), 0.5, 3.5);
        std::printf("ruído: centroide %.0f Hz  planura %.3f  ataques/s %.2f\n",
                    s.centroidHz, s.noisiness, s.onsetsPerSecond);
        CHECK(s.noisiness > 0.5f);
        CHECK(std::abs(s.centroidHz - 12000.0f) < 1000.0f);
        CHECK(s.onsetsPerSecond < 0.5f);
    }

    // 4. trem de cliques a 4 por segundo sobre ruído baixo: 4 ataques/s,
    //    cada um perto do clique
    {
        auto v = ruido(0.001, 5.0, 7);
        std::vector<double> cliques;
        for (double t = 0.25; t < 5.0; t += 0.25)
        {
            cliques.push_back(t);
            const auto i0 = static_cast<std::size_t>(t * SR);
            for (std::size_t k = 0; k < 480 && i0 + k < v.size(); ++k)  // 10 ms decaindo
                v[i0 + k] += static_cast<float>(0.8 * std::exp(-static_cast<double>(k) / 80.0)
                                                 * std::sin(2.0 * PI * 2000.0 * static_cast<double>(k) / SR));
        }
        const auto a = analisar(v);
        std::printf("cliques: %zu ataques para %zu cliques\n", a.onsets().size(), cliques.size());
        CHECK(a.onsets().size() == cliques.size());
        std::size_t perto = 0;
        for (const auto t : cliques)
            for (const auto& o : a.onsets())
                if (std::abs(o.time - t) < 0.035)
                {
                    ++perto;
                    break;
                }
        CHECK(perto == cliques.size());
        double soma = 0.0, pior = 0.0;
        for (std::size_t i = 0; i < std::min(cliques.size(), a.onsets().size()); ++i)
        {
            const double e = a.onsets()[i].time - cliques[i];
            soma += e;
            pior = std::max(pior, std::abs(e));
        }
        std::printf("cliques: erro médio %+.1f ms, pior %.1f ms\n",
                    1000.0 * soma / static_cast<double>(cliques.size()), 1000.0 * pior);
        const auto s = summarize(a.frames(), a.onsets(), 0.0, duracao(v));
        CHECK(std::abs(s.onsetsPerSecond - 3.8f) < 0.3f);  // 19 cliques em 5 s
        CHECK(std::abs(s.density - s.onsetsPerSecond / (s.onsetsPerSecond + 4.0f)) < 1e-6f);
    }

    // 5. o brilho sobe quando o seno sobe: 200 Hz < 800 Hz < 3,2 kHz
    {
        std::vector<float> v;
        for (const double hz : { 200.0, 800.0, 3200.0 })
        {
            const auto p = seno(hz, 0.4, 2.0);
            v.insert(v.end(), p.begin(), p.end());
        }
        const auto a = analisar(v);
        const auto seg = segment(a.frames(), a.onsets(), duracao(v), 2.0);
        CHECK(seg.size() == 3);
        if (seg.size() == 3)
        {
            std::printf("brilho: %.3f  %.3f  %.3f\n", seg[0].brightness, seg[1].brightness, seg[2].brightness);
            CHECK(seg[0].brightness < seg[1].brightness && seg[1].brightness < seg[2].brightness);
            // a mudança de 800 Hz para 3,2 kHz é tão grande quanto a de 200 para 800 (oitavas iguais)
            CHECK(std::abs((seg[1].brightness - seg[0].brightness) - (seg[2].brightness - seg[1].brightness)) < 0.03f);
        }
    }

    // 6. energia e faixa dinâmica: −6 dB a cada segundo
    {
        std::vector<float> v;
        for (int i = 0; i < 4; ++i)
        {
            const auto p = seno(500.0, 0.5 * std::pow(10.0, -6.0 * i / 20.0), 1.0);
            v.insert(v.end(), p.begin(), p.end());
        }
        const auto a = analisar(v);
        const auto seg = segment(a.frames(), a.onsets(), duracao(v), 1.0);
        for (std::size_t i = 1; i < seg.size(); ++i)
            CHECK(std::abs((seg[i - 1].meanRmsDb - seg[i].meanRmsDb) - 6.0f) < 0.6f);
        const auto s = summarize(a.frames(), a.onsets(), 0.0, duracao(v));
        CHECK(s.dynamicRangeDb > 14.0f && s.dynamicRangeDb < 20.0f);
    }

    // 7. o contraste aponta a mudança: seno → ruído no segundo 3
    {
        auto v = seno(440.0, 0.3, 3.0);
        const auto n = ruido(0.3, 3.0);
        v.insert(v.end(), n.begin(), n.end());
        const auto a = analisar(v);
        const auto seg = segment(a.frames(), a.onsets(), duracao(v), 1.0);
        std::size_t onde = 0;
        float maior = 0.0f;
        for (std::size_t i = 1; i < seg.size(); ++i)
            if (contrast(seg[i - 1], seg[i]) > maior)
            {
                maior = contrast(seg[i - 1], seg[i]);
                onde = i;
            }
        CHECK(onde == 3);
        CHECK(contrast(seg[0], seg[1]) < 0.05f);
    }

    // 7b. brilho de um trecho misto é a média em oitavas: metade 200 Hz,
    //     metade 3,2 kHz dá ≈ 800 Hz (o centro em oitavas), não 1,7 kHz
    {
        auto v = seno(200.0, 0.4, 2.0);
        const auto p = seno(3200.0, 0.4, 2.0);
        v.insert(v.end(), p.begin(), p.end());
        const auto a = analisar(v);
        const auto s = summarize(a.frames(), a.onsets(), 0.0, duracao(v));
        std::printf("misto: centroide %.0f Hz\n", s.centroidHz);
        CHECK(std::abs(s.centroidHz - 800.0f) < 60.0f);
    }

    // 8. determinismo e independência do tamanho de bloco
    {
        const auto v = ruido(0.2, 2.0, 3);
        const auto a = analisar(v, 512);
        const auto b = analisar(v, 37);
        const auto c = analisar(v, v.size());
        CHECK(a.frames().size() == b.frames().size() && b.frames().size() == c.frames().size());
        bool iguais = a.onsets().size() == b.onsets().size();
        for (std::size_t i = 0; iguais && i < a.frames().size(); ++i)
            iguais = a.frames()[i].rmsDb == b.frames()[i].rmsDb && a.frames()[i].flux == c.frames()[i].flux
                  && a.frames()[i].centroidHz == b.frames()[i].centroidHz;
        CHECK(iguais);
    }

    // 9. cobertura do fim: a última amostra entra em algum quadro
    {
        auto v = std::vector<float>(static_cast<std::size_t>(SR * 1.0), 0.0f);
        v.back() = 0.9f;
        v[v.size() - 2] = -0.9f;
        const auto a = analisar(v);
        bool algum = false;
        for (const auto& f : a.frames())
            algum = algum || !f.silent;
        CHECK(algum);
        Analyzer vazio(SR);
        vazio.finish();
        CHECK(vazio.frames().size() == 1);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("descritores: todas as checagens passaram");
    return EXIT_SUCCESS;
}
