// As linhas por banda (src/core/BandSplit.h): a soma reconstrói a
// referência; cada senoide cai na sua banda; cortes inválidos são descartados.
#include "BandSplit.h"
#include "Lines.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace trioio::perception;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

int main()
{
    const double sr = 48000.0;
    const std::size_t n = static_cast<std::size_t>(sr * 2.0);
    std::vector<float> x(n);
    const double f[] = { 80.0, 700.0, 6000.0 };
    for (std::size_t i = 0; i < n; ++i)
    {
        double v = 0.0;
        for (double fi : f)
            v += 0.25 * std::sin(2.0 * 3.14159265358979 * fi * static_cast<double>(i) / sr);
        x[i] = static_cast<float>(v);
    }

    // 1. três linhas; a soma é a referência (subtração exata, a menos do float)
    const auto lines = splitBands(x, sr, { 2000.0, 200.0 });  // fora de ordem: ordena
    CHECK(lines.size() == 3);
    double maxErr = 0.0;
    for (std::size_t i = 0; i < n; ++i)
    {
        double s = 0.0;
        for (const auto& l : lines)
            s += l.samples[i];
        maxErr = std::max(maxErr, std::abs(s - x[i]));
    }
    CHECK(maxErr < 1e-5);
    CHECK(lines[0].lowHz == 0.0 && lines[0].highHz == 200.0);
    CHECK(lines[2].lowHz == 2000.0 && lines[2].highHz == 0.0);
    CHECK(lines[0].name() == "grave (< 200 Hz)");
    CHECK(lines[1].name() == "200–2000 Hz");
    CHECK(lines[2].name() == "agudo (> 2000 Hz)");

    // 2. cada senoide na sua banda: cada linha com ~1/3 da energia
    // (80 Hz está a 1,3 oitava do corte de 200: ~2,5 % da amplitude fica na
    // linha vizinha, EM FASE — a amplitude soma 1, a energia das partes um
    // pouco menos; a soma das partes fica entre 0,9 e 1)
    for (const auto& l : lines)
        CHECK(l.energyShare > 0.30 && l.energyShare < 0.37);
    double sum = 0.0;
    for (const auto& l : lines)
        sum += l.energyShare;
    CHECK(sum > 0.90 && sum <= 1.01);

    // 3. só a senoide de 700 Hz: quase tudo no meio
    std::vector<float> m(n);
    for (std::size_t i = 0; i < n; ++i)
        m[i] = static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979 * 700.0 * static_cast<double>(i) / sr));
    const auto mid = splitBands(m, sr, { 200.0, 2000.0 });
    CHECK(mid[1].energyShare > 0.95);
    CHECK(mid[0].energyShare < 0.02 && mid[2].energyShare < 0.02);
    CHECK(levelMatchGain(mid[1], m) > 0.99f && levelMatchGain(mid[1], m) < 1.05f);
    CHECK(levelMatchGain(mid[0], m) <= std::pow(10.0f, 1.5f) + 0.01f);  // limitado a +30 dB

    // 4. cortes inválidos: abaixo de 20 Hz, acima de Nyquist, colados demais
    const auto bad = splitBands(x, sr, { 10.0, 30000.0, 500.0, 520.0 });
    CHECK(bad.size() == 2 && bad[0].highHz == 500.0);
    CHECK(splitBands(x, sr, {}).size() == 1);

    // 5. extractBand: a mesma banda que splitBands
    for (std::size_t k = 0; k < lines.size(); ++k)
    {
        const auto b = extractBand(x, sr, { 200.0, 2000.0 }, k);
        double d = 0.0;
        for (std::size_t i = 0; i < n; ++i)
            d = std::max(d, static_cast<double>(std::abs(b[i] - lines[k].samples[i])));
        CHECK(d < 1e-6);
    }
    CHECK(extractBand(x, sr, { 200.0, 2000.0 }, 7).size() == n);

    // 6. core/Lines.h: o que vale, o empréstimo, o registro
    {
        using namespace trioio::composition;
        Reading whole, line;
        whole.pulse.bpm = 120.0;
        line.pulse.bpm = 60.5;  // metade: concorda
        line.pulse.confidence = 0.1f;
        line.pitchedShare = 0.8f;
        line.chords.push_back({});
        auto v = assessLine(0.0, 200.0, 48000.0, line, whole, 2.0);
        CHECK(!v.pulse && v.agrees && v.bass && !v.melody && !v.chords);
        v = assessLine(200.0, 2000.0, 48000.0, line, whole, 2.0);
        CHECK(v.melody && v.chords && !v.bass);
        v = assessLine(4000.0, 0.0, 48000.0, line, whole, 2.0);
        CHECK(!v.melody && !v.chords);
        line.pulse.confidence = 0.5f;
        line.pulse.bpm = 97.0;
        v = assessLine(4000.0, 0.0, 48000.0, line, whole, 0.2);
        CHECK(!v.pulse && !v.agrees);  // poucos ataques; fora do nível da peça

        std::vector<Frame> lf(3), wf(3);
        for (auto& f : wf) { f.melodyHz = 440.0f; f.chroma[9] = 1.0f; f.bassChroma[2] = 1.0f; }
        for (auto& f : lf) { f.bassChroma[7] = 1.0f; f.flux = 0.5f; }
        LineValidity grave;
        grave.bass = true;
        CHECK(borrowPitch(lf, wf, grave) != "nada");
        CHECK(lf[1].melodyHz == 440.0f && lf[1].chroma[9] == 1.0f);
        CHECK(lf[1].bassChroma[7] == 1.0f && lf[1].bassChroma[2] == 0.0f);  // as fundamentais ficam as da linha
        CHECK(lf[1].flux == 0.5f);                                           // o ritmo também
        LineValidity medio;
        medio.melody = medio.chords = true;
        auto lf2 = lf;
        lf2[0].melodyHz = 220.0f;
        CHECK(borrowPitch(lf2, wf, medio) == "nada" && lf2[0].melodyHz == 220.0f);

        float a = 0, b = 0;
        bandRegister(0.0, 200.0, a, b);
        CHECK(a > 23.5f && a < 24.5f && b > 54.5f && b < 55.5f);  // Dó1 … Sol3
        bandRegister(2000.0, 0.0, a, b);
        CHECK(b == 108.0f && b - a >= 24.0f);
        Score sc;
        sc.notes = { Note {}, Note {}, Note {} };
        sc.notes[0].pitch = 72.0f;
        sc.notes[1].pitch = 30.0f;
        sc.notes[2].pitch = 90.0f;
        sc.notes[2].voice = 4;  // percussão: fica
        foldIntoRegister(sc, 36.0f, 60.0f);
        CHECK(sc.notes[0].pitch == 60.0f && sc.notes[1].pitch == 42.0f && sc.notes[2].pitch == 90.0f);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("linhas por banda: todas as checagens passaram");
    return EXIT_SUCCESS;
}
