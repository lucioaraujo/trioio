// Conformidade da saída técnica do Trioio (16/11/2026, nivelamento pela
// saida-ref 1.0, RASGO/REFERENCIAS/SAIDA_TECNICA; auditoria de excelência
// sonora, itens 1-3). Sinais da EBU Tech 3341 V4 (2023), tabela 1,
// sintetizados a partir da descrição (nenhum arquivo da EBU). Prova:
// integratedLoudness (casos 1-5, +-0,1 LU), truePeakDb (15-23,
// +0,2/-0,4 dBTP), o master dourado (a mesma entrada dá a mesma saída, bit a
// bit) e o dither de 24 bits até o arquivo do JUCE.
#include "Dither.h"
#include "Master.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kFs = 48000.0;
int failures = 0;

void check(const std::string& name, bool ok)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", name.c_str());
    if (!ok) ++failures;
}

std::vector<float> segments(const std::vector<std::pair<double, double>>& list)
{
    std::vector<float> x;
    std::size_t n = 0;
    for (const auto& [seconds, db] : list)
    {
        const double a = std::pow(10.0, db / 20.0);
        const auto count = static_cast<std::size_t>(std::llround(seconds * kFs));
        for (std::size_t k = 0; k < count; ++k, ++n)
            x.push_back(static_cast<float>(a * std::sin(2.0 * kPi * 1000.0 * static_cast<double>(n) / kFs)));
    }
    return x;
}

double tpDb(const std::vector<double>& x)
{
    std::vector<float> f(x.begin(), x.end());
    return trioio::perception::truePeakDb(f, f, kFs);
}

std::vector<double> tone(double f, double a, double degrees)
{
    const std::size_t n = 48000, fade = 480;
    std::vector<double> x(n);
    for (std::size_t i = 0; i < n; ++i)
        x[i] = std::min({ 1.0, double(i) / fade, double(n - 1 - i) / fade })
            * a * std::sin(2.0 * kPi * f * double(i) / kFs + degrees * kPi / 180.0);
    return x;
}

std::vector<double> singlePeriod(std::size_t offset)
{
    const double high = 4.0 * kFs;
    const auto n = static_cast<std::size_t>(high * 0.5);
    std::vector<double> x(n);
    const double slow = high / (kFs / 6.0), fast = high / (kFs / 4.0);
    const auto start = static_cast<std::size_t>(slow * 2000.0);
    const auto fade = static_cast<std::size_t>(high * 0.005);
    for (std::size_t i = 0; i < n; ++i)
    {
        double v;
        if (i < start) v = 0.5 * std::sin(2.0 * kPi * double(i) / slow);
        else if (double(i - start) < fast) v = std::sin(2.0 * kPi * double(i - start) / fast);
        else v = 0.5 * std::sin(2.0 * kPi * (double(i - start) - fast) / slow);
        x[i] = std::min({ 1.0, double(i) / fade, double(n - 1 - i) / fade }) * v;
    }
    constexpr int taps = 255;
    const double cutoff = 0.45 * kFs / high;
    std::array<double, taps> h {};
    double sum = 0.0;
    for (int k = 0; k < taps; ++k)
    {
        const int m = k - (taps - 1) / 2;
        const double sinc = m == 0 ? 2.0 * cutoff : std::sin(2.0 * kPi * cutoff * m) / (kPi * m);
        h[std::size_t(k)] = sinc * (0.42 - 0.5 * std::cos(2.0 * kPi * k / (taps - 1)) + 0.08 * std::cos(4.0 * kPi * k / (taps - 1)));
        sum += h[std::size_t(k)];
    }
    std::vector<double> out;
    for (std::size_t i = offset + taps; i + taps < n; i += 4)
    {
        double y = 0.0;
        for (int k = 0; k < taps; ++k) y += h[std::size_t(k)] / sum * x[i - std::size_t(k)];
        out.push_back(y);
    }
    return out;
}
} // namespace

int main()
{
    const std::vector<std::pair<int, std::vector<std::pair<double, double>>>> cases {
        { 1, { { 20.0, -23.0 } } },
        { 2, { { 20.0, -33.0 } } },
        { 3, { { 10.0, -36.0 }, { 60.0, -23.0 }, { 10.0, -36.0 } } },
        { 4, { { 10.0, -72.0 }, { 10.0, -36.0 }, { 60.0, -23.0 }, { 10.0, -36.0 }, { 10.0, -72.0 } } },
        { 5, { { 20.0, -26.0 }, { 20.1, -20.0 }, { 20.0, -26.0 } } } };
    for (const auto& [test, list] : cases)
    {
        const auto x = segments(list);
        const double expected = test == 2 ? -33.0 : -23.0;
        const double i = trioio::perception::integratedLoudness(x, x, kFs);
        std::printf("      #%d: I %.2f LUFS (esperado %.0f)\n", test, i, expected);
        check("EBU 3341 #" + std::to_string(test) + " loudness integrada", std::abs(i - expected) <= 0.1);
    }

    const struct { const char* name; double f, a, deg, expected; } tones[] = {
        { "#15", 12000.0, 0.5, 0.0, -6.0 }, { "#16", 12000.0, 0.5, 45.0, -6.0 }, { "#17", 8000.0, 0.5, 60.0, -6.0 },
        { "#18", 6000.0, 0.5, 67.5, -6.0 }, { "#19", 12000.0, 1.41, 45.0, 3.0 } };
    for (const auto& t : tones)
    {
        const double db = tpDb(tone(t.f, t.a, t.deg));
        check(std::string("EBU 3341 ") + t.name + " " + std::to_string(db) + " dBTP",
              db <= t.expected + 0.2 && db >= t.expected - 0.4);
    }
    for (std::size_t offset = 0; offset < 4; ++offset)
    {
        const double db = tpDb(singlePeriod(offset));
        check("EBU 3341 #" + std::to_string(20 + offset) + " " + std::to_string(db) + " dBTP", db <= 0.2 && db >= -0.4);
    }

    // master dourado: a mesma entrada, duas vezes, sai igual bit a bit
    {
        auto make = [] {
            std::vector<float> l(48000 * 6), r(48000 * 6);
            std::uint32_t s = 12345u;
            for (std::size_t i = 0; i < l.size(); ++i)
            {
                s ^= s << 13; s ^= s >> 17; s ^= s << 5;
                const double t = double(i) / kFs;
                l[i] = static_cast<float>(0.3 * std::sin(2.0 * kPi * 220.0 * t) + 0.05 * (double(s) / 4294967296.0 - 0.5));
                r[i] = static_cast<float>(0.3 * std::sin(2.0 * kPi * 330.0 * t));
            }
            return std::pair { l, r };
        };
        auto [l1, r1] = make();
        auto [l2, r2] = make();
        const auto a = trioio::perception::applyMaster(l1, r1, kFs);
        const auto b = trioio::perception::applyMaster(l2, r2, kFs);
        std::printf("      master: true peak depois %.2f dBTP\n", a.truePeakAfter);
        check("master dourado: a mesma entrada dá a mesma saída", l1 == l2 && r1 == r2 && a.truePeakAfter == b.truePeakAfter);
        check("master: true peak final <= -1 dBTP", a.truePeakAfter <= -1.0 + 1e-6 && trioio::perception::truePeakDb(l1, r1, kFs) <= -0.95);
    }

    // dither: os inteiros de 24 bits chegam intactos ao arquivo do JUCE 9.0.3
    bool exact = true;
    for (std::int32_t k = -8388608; k <= 8388607 && exact; ++k)
    {
        const double x = trioio::floatForJuceInt24(k);
        const int i32 = x <= -1.0 ? std::numeric_limits<int>::min()
                      : x >= 1.0 ? std::numeric_limits<int>::max()
                      : static_cast<int>(std::lround(std::numeric_limits<int>::max() * x));
        exact = (i32 >> 8) == k;
    }
    check("dither: os inteiros de 24 bits chegam intactos ao arquivo", exact);

    std::printf("%s\n", failures == 0 ? "conformidade da saída: OK" : "conformidade da saída: FALHOU");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
