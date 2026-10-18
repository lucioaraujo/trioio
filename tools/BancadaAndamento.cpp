// Bancada de andamentos (docs/LEITURA_E_GABARITO.md, "Nível do pulso"):
// faixas sintéticas de andamento conhecido, em seis estilos, varridas de 60
// a 180 BPM em passos de ~6 % (um semitom de andamento). Mede, por estilo:
//   - quantas leituras saem no andamento contado (±4 %);
//   - em que nível cada leitura cai (½, ⅔, 1, 3⁄2, 2…);
//   - quantas vezes o nível SALTA entre andamentos vizinhos (o defeito que o
//     teste por transformação achou).
// Fora da suíte: leva alguns minutos. Uso:
//     trioio_bancada_andamento [segundos por faixa = 24]

#include "Descriptors.h"
#include "Structure.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace trioio::perception;

namespace
{
constexpr double SR = 48000.0;
constexpr double PI = 3.14159265358979323846;

struct Noise
{
    std::uint64_t s = 0x9E3779B97F4A7C15ull;
    double next()
    {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        return static_cast<double>(s >> 11) / 4503599627370496.0 - 1.0;
    }
};

void kick(std::vector<float>& x, double t, double g)
{
    const auto i0 = static_cast<std::size_t>(t * SR);
    for (std::size_t i = i0; i < std::min(x.size(), i0 + static_cast<std::size_t>(0.25 * SR)); ++i)
    {
        const double u = static_cast<double>(i - i0) / SR;
        x[i] += static_cast<float>(g * std::sin(2.0 * PI * (50.0 * u + 60.0 * 0.04 * (1.0 - std::exp(-u / 0.04)))) * std::exp(-u / 0.12));
    }
}
void snare(std::vector<float>& x, double t, double g, Noise& n)
{
    const auto i0 = static_cast<std::size_t>(t * SR);
    for (std::size_t i = i0; i < std::min(x.size(), i0 + static_cast<std::size_t>(0.2 * SR)); ++i)
    {
        const double u = static_cast<double>(i - i0) / SR;
        x[i] += static_cast<float>(g * (0.7 * n.next() * std::exp(-u / 0.07) + 0.3 * std::sin(2.0 * PI * 190.0 * u) * std::exp(-u / 0.05)));
    }
}
void hat(std::vector<float>& x, double t, double g, Noise& n)
{
    const auto i0 = static_cast<std::size_t>(t * SR);
    double prev = 0.0;
    for (std::size_t i = i0; i < std::min(x.size(), i0 + static_cast<std::size_t>(0.05 * SR)); ++i)
    {
        const double u = static_cast<double>(i - i0) / SR, w = n.next();
        x[i] += static_cast<float>(g * (w - prev) * std::exp(-u / 0.015));
        prev = w;
    }
}
void click(std::vector<float>& x, double t, double g, double hz)
{
    const auto i0 = static_cast<std::size_t>(t * SR);
    for (std::size_t i = i0; i < std::min(x.size(), i0 + static_cast<std::size_t>(0.03 * SR)); ++i)
    {
        const double u = static_cast<double>(i - i0) / SR;
        x[i] += static_cast<float>(g * std::sin(2.0 * PI * hz * u) * std::exp(-u / 0.008));
    }
}
// pad sustentado (Lá menor) que re-ataca a cada `every` segundos
void pad(std::vector<float>& x, double every)
{
    for (std::size_t i = 0; i < x.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR, sub = std::fmod(t, every);
        const double env = 0.35 + 0.65 * std::exp(-sub / 0.25) * std::min(1.0, sub / 0.004);
        double v = 0.0;
        for (double f : { 220.0, 261.63, 329.63 })
            for (int h = 1; h <= 5; ++h)
                v += std::sin(2.0 * PI * f * h * t) / h;
        x[i] += static_cast<float>(0.04 * env * v);
    }
}

std::vector<float> faixa(int estilo, double bpm, double dur)
{
    std::vector<float> x(static_cast<std::size_t>(dur * SR), 0.0f);
    Noise n;
    const double b = 60.0 / bpm;
    for (int k = 0; k * b < dur; ++k)
    {
        const double t = k * b;
        const int inBar4 = k % 4, inBar3 = k % 3;
        switch (estilo)
        {
            case 0:  // rock: bumbo 1 e 3, caixa 2 e 4, chimbal em colcheias
            case 5:  // rock + pad pontuado (distrator a cada 1,5 tempo)
            case 6:  // pop: o rock + acordes e baixo (o 1 marcado pela harmonia)
                if (inBar4 == 0 || inBar4 == 2) kick(x, t, 1.0);
                if (inBar4 == 1 || inBar4 == 3) snare(x, t, 0.6, n);
                hat(x, t, 0.25, n);
                hat(x, t + b / 2.0, 0.18, n);
                break;
            case 1:  // metrônomo com acento no 1
                click(x, t, inBar4 == 0 ? 0.8 : 0.5, inBar4 == 0 ? 1500.0 : 1000.0);
                break;
            case 2:  // shuffle 12/8: chimbal longo-curto (tercina), bumbo 1 e 3, caixa 2 e 4
                if (inBar4 == 0 || inBar4 == 2) kick(x, t, 1.0);
                if (inBar4 == 1 || inBar4 == 3) snare(x, t, 0.6, n);
                hat(x, t, 0.25, n);
                hat(x, t + 2.0 * b / 3.0, 0.15, n);
                break;
            case 3:  // bossa nova: clave de 16 semicolcheias em 2 compassos de 2 tempos
            {
                if (k % 2 == 0) kick(x, t, 0.9);
                kick(x, t + 0.75 * b, 0.5);  // o bumbo antecipado
                for (int s = 0; s < 4; ++s)
                    hat(x, t + s * b / 4.0, s == 0 ? 0.18 : 0.08, n);
                const int pos = (k % 4) * 4;  // 16 semicolcheias por 4 tempos
                for (int s = 0; s < 4; ++s)
                {
                    static const int clave[16] = { 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0 };
                    if (clave[(pos + s) % 16])
                        click(x, t + s * b / 4.0, 0.45, 2500.0);
                }
                break;
            }
            case 4:  // valsa 3/4: bumbo no 1, caixa no 2 e no 3
                if (inBar3 == 0) kick(x, t, 1.0);
                else snare(x, t, 0.45, n);
                hat(x, t, 0.15, n);
                break;
        }
    }
    if (estilo == 5)
        pad(x, 1.5 * b);
    if (estilo == 6)
    {
        // pop: rock + acordes que mudam no 1 de cada compasso (Lám Fá Dó Sol)
        // e baixo na fundamental no 1 e no 3: o 1 é marcado pela harmonia
        const double roots[4] = { 57, 53, 48, 55 };
        const int third[4] = { 3, 4, 4, 4 };
        for (std::size_t i = 0; i < x.size(); ++i)
        {
            const double t = static_cast<double>(i) / SR;
            const int bar = static_cast<int>(t / (4.0 * b));
            const double r = roots[bar % 4];
            const double inBar = t - bar * 4.0 * b, inHalf = std::fmod(inBar, 2.0 * b);
            const auto hz = [](double m) { return 440.0 * std::exp2((m - 69.0) / 12.0); };
            double v = 0.0;
            for (double m : { r + 12.0, r + 12.0 + third[bar % 4], r + 19.0 })
                for (int h = 1; h <= 4; ++h)
                    v += 0.03 * std::sin(2.0 * PI * hz(m) * h * t) / h * std::min(1.0, inBar / 0.01);
            v += 0.18 * std::sin(2.0 * PI * hz(r - 12.0) * t) * std::exp(-inHalf / 0.6) * std::min(1.0, inHalf / 0.005);
            x[i] += static_cast<float>(v);
        }
    }
    return x;
}

const char* nivel(double r)
{
    const std::pair<double, const char*> nv[] = { { 1.0, "1" }, { 0.5, "½" }, { 2.0, "2" }, { 2.0 / 3.0, "⅔" },
                                                  { 1.5, "3⁄2" }, { 1.0 / 3.0, "⅓" }, { 3.0, "3" }, { 0.75, "¾" },
                                                  { 4.0 / 3.0, "4⁄3" }, { 0.25, "¼" }, { 4.0, "4" } };
    for (const auto& [v, nome] : nv)
        if (std::abs(r / v - 1.0) <= 0.04)
            return nome;
    return "?";
}
} // namespace

int main(int argc, char** argv)
{
    const double dur = argc > 1 ? std::atof(argv[1]) : 24.0;
    const double margem = argc > 2 ? std::atof(argv[2]) : 0.05;  // a margem do compasso de 3 (estimateMeter)
    const char* nomes[] = { "rock", "metrônomo", "shuffle 12/8", "bossa nova", "valsa 3/4", "rock + pad pontuado",
                            "pop (acordes no 1)" };
    int certosTotal = 0, saltosTotal = 0, n = 0, fortesSoma = 0, fortesN = 0, metroSoma = 0;
    for (int estilo = 0; estilo < 7; ++estilo)
    {
        std::printf("%-20s", nomes[estilo]);
        std::string anterior;
        int certos = 0, saltos = 0, fortes = 0, fortesTotal = 0, fases = 0;
        int porCompasso[9] = {};
        int metroCerto = 0;
        for (int s = 0; s <= 19; ++s)
        {
            const double bpm = 60.0 * std::exp2(s / 12.0);
            const auto x = faixa(estilo, bpm, dur);
            Analyzer a(SR);
            a.push(x.data(), x.size());
            a.finish();
            const auto p = estimatePulse(a.frames(), a.onsets(), 0.0, dur);
            const std::string nv = nivel(p.bpm / bpm);
            certos += nv == "1";
            if (nv == "1")
            {
                // a fase: os tempos lidos caem nos tempos verdadeiros (t = k · 60/bpm)?
                const double per = 60.0 / bpm, r0 = std::fmod(p.phase, per);
                fases += std::min(r0, per - r0) < 0.05;
                // o 1 de cada faixa está em t = 0: o tempo forte lido cai num múltiplo do compasso?
                const int compasso = estilo == 4 ? 3 : 4;
                ++porCompasso[std::clamp(p.beatsPerBar, 0, 8)];
                const auto me = estimateMeter(a.frames(), p, 0.0, dur, margem);
                metroCerto += me.beatsPerBar == compasso;
                if (p.beatsPerBar == compasso)
                {
                    const auto db = estimateDownbeat(a.frames(), p, 0.0, dur);
                    const double bar = compasso * 60.0 / bpm;
                    const double r = std::fmod(db.time, bar);
                    fortes += std::min(r, bar - r) < 0.07;
                }
                ++fortesTotal;
            }
            if (!anterior.empty() && nv != anterior)
                ++saltos;
            anterior = nv;
            std::printf(" %3s", nv.c_str());
            std::fflush(stdout);
            ++n;
        }
        std::printf("  │ certos %2d/20 · saltos %d · fase %d/%d · tempo forte %d/%d\n", certos, saltos, fases, fortesTotal, fortes,
                    fortesTotal);
        std::printf("%20s  compasso por estimateMeter: %d/%d certos · ", "", metroCerto, fortesTotal);
        metroSoma += metroCerto;
        std::printf("tempos por compasso lidos (pulso):");
        for (int k = 1; k <= 8; ++k)
            if (porCompasso[k]) std::printf(" %d×%d", porCompasso[k], k);
        std::printf("\n");
        fortesSoma += fortes;
        fortesN += fortesTotal;
        certosTotal += certos;
        saltosTotal += saltos;
    }
    std::printf("\ntotal: certos %d/%d · saltos de nível %d (andamentos de 60 a 180 BPM, passos de ~6 %%) · tempo forte %d/%d\n",
                certosTotal, n, saltosTotal, fortesSoma, fortesN);
    std::printf("compasso (3 ou 4) por estimateMeter: %d/%d\n", metroSoma, fortesN);
    return 0;
}
