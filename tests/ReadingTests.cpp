// A leitura e o gabarito (core/Reading.h, docs/LEITURA_E_GABARITO.md), e o
// efeito da separação harmônico/percussivo (Descriptors.h, HPSS) numa faixa
// sintética de resposta conhecida: pads em progressão, bateria por cima e
// re-ataques do pad num andamento que não é o da bateria.

#include "Reading.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace trioio;
using namespace trioio::composition;
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

struct Noise
{
    std::uint64_t s = 0x2545F4914F6CDD1Dull;
    double next()
    {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        return static_cast<double>(s >> 11) / 4503599627370496.0 - 1.0;  // −1..1
    }
};

double hz(double midi) { return 440.0 * std::pow(2.0, (midi - 69.0) / 12.0); }

// 40 s a 96 BPM em Lá menor harmônica: Lám | Rém | Mi | Lám, um acorde por
// compasso de 4 tempos. Bumbo nos tempos 1 e 3, caixa no 2 e no 4, chimbal
// nas colcheias. O pad re-ataca a cada 1,5 tempo (pontuado): um segundo
// "pulso" harmônico que não é o da música.
std::vector<float> faixa(double& beat, std::vector<std::pair<double, int>>& chords)
{
    const double bpm = 96.0;
    beat = 60.0 / bpm;
    const double bar = 4.0 * beat;
    const int bars = 16;
    const double dur = bars * bar;
    std::vector<float> x(static_cast<std::size_t>(dur * SR), 0.0f);
    const int prog[4][3] = { { 57, 60, 64 }, { 62, 65, 69 }, { 64, 68, 71 }, { 57, 60, 64 } };  // Lám Rém Mi Lám
    const int idx[4] = { 9 * 3 + 1, 2 * 3 + 1, 4 * 3 + 0, 9 * 3 + 1 };
    Noise n;
    for (int b = 0; b < bars; ++b)
    {
        const int* c = prog[b % 4];
        chords.push_back({ b * bar, idx[b % 4] });
        const double t0 = b * bar;
        for (std::size_t i = static_cast<std::size_t>(t0 * SR); i < static_cast<std::size_t>((t0 + bar) * SR) && i < x.size(); ++i)
        {
            const double t = static_cast<double>(i) / SR;
            const double local = t - t0;
            // re-ataque do pad a cada 1,5 tempo: subida rápida, queda lenta
            const double sub = std::fmod(t, 1.5 * beat);
            const double env = 0.35 + 0.65 * std::exp(-sub / 0.25) * std::min(1.0, sub / 0.004);
            const double fade = std::min({ 1.0, local / 0.03, (bar - local) / 0.03 });
            double v = 0.0;
            for (int k = 0; k < 3; ++k)
                for (int h = 1; h <= 6; ++h)
                    v += std::sin(2.0 * PI * hz(c[k]) * h * t) / h;
            v *= 0.05 * env * fade;
            v += 0.12 * fade * (std::sin(2.0 * PI * hz(c[0] - 12) * t) + 0.4 * std::sin(4.0 * PI * hz(c[0] - 12) * t));
            x[i] += static_cast<float>(v);
        }
    }
    // bateria
    for (double t = 0.0; t < dur; t += beat / 2.0)
    {
        const int eighth = static_cast<int>(std::lround(t / (beat / 2.0)));
        const bool onBeat = eighth % 2 == 0;
        const int beatInBar = (eighth / 2) % 4;
        const auto i0 = static_cast<std::size_t>(t * SR);
        double prev = 0.0;
        for (std::size_t i = i0; i < std::min(x.size(), i0 + static_cast<std::size_t>(0.2 * SR)); ++i)
        {
            const double u = static_cast<double>(i - i0) / SR;
            double v = 0.0;
            const double w = n.next();
            v += 0.35 * (w - prev) * std::exp(-u / 0.03);  // chimbal: ruído derivado
            prev = w;
            if (onBeat && (beatInBar == 0 || beatInBar == 2))
                v += 1.4 * std::sin(2.0 * PI * (50.0 * u + 60.0 * 0.04 * (1.0 - std::exp(-u / 0.04)))) * std::exp(-u / 0.12);
            // tom afinado em Sib (fora da tonalidade) no contratempo: um
            // golpe curto com altura, que o croma não deveria contar
            if (!onBeat)
                v += 0.5 * (std::sin(2.0 * PI * 233.08 * u) + 0.5 * std::sin(2.0 * PI * 466.16 * u)) * std::exp(-u / 0.05);
            if (onBeat && (beatInBar == 1 || beatInBar == 3))
                v += 0.9 * w * std::exp(-u / 0.10) + 0.3 * std::sin(2.0 * PI * 190.0 * u) * std::exp(-u / 0.06);
            x[i] += static_cast<float>(v);
        }
    }
    return x;
}

int main()
{
    // ---- nomes e relógio ---------------------------------------------------
    CHECK(parseChordName("Solm") == 7 * 3 + 1);
    CHECK(parseChordName("Sol menor") == 7 * 3 + 1);
    CHECK(parseChordName("Gm") == 7 * 3 + 1);
    CHECK(parseChordName("F#") == 6 * 3);
    CHECK(parseChordName("Si°") == 11 * 3 + 2 && parseChordName("Bdim") == 11 * 3 + 2);
    CHECK(parseChordName("Sib") == 10 * 3 && parseChordName("Bb") == 10 * 3 && parseChordName("B") == 11 * 3);
    CHECK(parseChordName("Lá♭m") == 8 * 3 + 1 && parseChordName("Mib") == 3 * 3 && parseChordName("Dóm") == 1);
    CHECK(parseChordName("N") == -1 && parseChordName("xyz") == -2 && parseChordName("Sol7") == -2);
    for (int i = 0; i < 36; ++i)  // o nome que a leitura imprime volta ao mesmo acorde
        CHECK(parseChordName(chordFromIndex(i).name()) == i);
    int t = 0;
    bool m = false;
    CHECK(parseKeyName("Sol menor", t, m) && t == 7 && m);
    CHECK(parseKeyName("D", t, m) && t == 2 && !m);
    double s = 0.0;
    CHECK(parseClock("1:23.5", s) && std::abs(s - 83.5) < 1e-9);
    CHECK(parseClock("12", s) && s == 12.0 && !parseClock("1:75", s) && !parseClock("abc", s));

    // ---- reamostragem (teste por transformação): 440 Hz × 2^(2/12) ----------
    {
        std::vector<float> x(static_cast<std::size_t>(SR * 2.0)), y;
        for (std::size_t i = 0; i < x.size(); ++i)
            x[i] = static_cast<float>(0.5 * std::sin(2.0 * PI * 440.0 * static_cast<double>(i) / SR));
        const double rate = std::exp2(2.0 / 12.0);
        resampleSpeed(x, rate, y);
        // frequência pelos cruzamentos de zero no meio do sinal; amplitude pelo pico
        int cruz = 0;
        float pico = 0.0f;
        const std::size_t a0 = y.size() / 4, a1 = 3 * y.size() / 4;
        for (std::size_t i = a0 + 1; i < a1; ++i)
        {
            cruz += (y[i - 1] < 0.0f) != (y[i] < 0.0f);
            pico = std::max(pico, std::abs(y[i]));
        }
        const double f = cruz / 2.0 / (static_cast<double>(a1 - a0) / SR);
        std::printf("reamostragem: 440 Hz → %.1f Hz (esperado %.1f) · pico %.3f · %zu → %zu amostras\n", f, 440.0 * rate, pico,
                    x.size(), y.size());
        CHECK(std::abs(f - 440.0 * rate) < 1.0 && std::abs(pico - 0.5f) < 0.01f);
        CHECK(y.size() == static_cast<std::size_t>(static_cast<double>(x.size()) / rate));
    }

    // ---- o formato do gabarito ---------------------------------------------
    {
        std::vector<std::string> erros;
        const auto g = parseGabarito("# comentário\n"
                                     "referência: ref-x\n"
                                     "fonte: escuta do autor\n"
                                     "batidas: 1.0 1:01.5 2.0 3.0\n"
                                     "tonalidade: Sol menor\n"
                                     "acordes: 0:00 Solm, 0:02.5 Ré, 0:05 N\n"
                                     "secoes: 0:00 A, 0:34 B, 1:02 A\n"
                                     "fim: 2:18\n"
                                     "acordes: 0:09 Zzz\n"
                                     "outra: coisa\n",
                                     erros);
        CHECK(g.reference == "ref-x" && g.source == "escuta do autor");
        CHECK(g.beats.size() == 4 && std::abs(g.beats.back() - 61.5) < 1e-9);
        CHECK(g.hasKey && g.keyTonic == 7 && g.keyMinor);
        CHECK(g.chords.size() == 3 && g.chords[2].second == -1);
        CHECK(g.sections.size() == 3 && g.sections[1].second == "B" && std::abs(g.end - 138.0) < 1e-9);
        CHECK(erros.size() == 2);  // o acorde ilegível e a chave desconhecida, com a linha
        for (const auto& e : erros)
            std::printf("  (esperado) %s\n", e.c_str());
    }

    // ---- casamento de instantes e tonalidade ---------------------------------
    {
        const auto mt = matchTimes({ 1.00, 2.05, 3.20 }, { 1.02, 2.00, 3.00, 4.00 }, 0.07);
        CHECK(std::abs(mt.precision - 2.0 / 3.0) < 1e-9 && std::abs(mt.recall - 0.5) < 1e-9);
        std::array<float, 12> c {};
        const double gm[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
        for (int i = 0; i < 12; ++i)
            c[static_cast<std::size_t>((i + 7) % 12)] = static_cast<float>(gm[i]);
        const auto k = keyFromChroma(c);
        CHECK(k.tonic == 7 && k.minor && k.correlation > 0.99f);
        CHECK(k.name() == "Sol menor");
    }

    // ---- a faixa sintética, com e sem HPSS -----------------------------------
    {
        double beat = 0.0;
        Gabarito g;
        const auto x = faixa(beat, g.chords);
        g.bpm = 60.0 / beat;
        for (double b = 0.0; b < static_cast<double>(x.size()) / SR; b += beat)
            g.beats.push_back(b);
        g.hasKey = true;
        g.keyTonic = 9;
        g.keyMinor = true;
        Evaluation ev[2];
        Reading rd[2];
        double sib[2] = { 0.0, 0.0 }, pfShare = 0.0;
        for (int h = 0; h < 2; ++h)
        {
            AnalyzerConfig cfg;
            cfg.hpss = h == 1;
            Analyzer a(SR, cfg);
            a.push(x.data(), x.size());
            a.finish();
            rd[h] = readReference(a.frames(), a.onsets(), static_cast<double>(x.size()) / SR);
            // quanto do croma é Sib (o tom fora da tonalidade), nos quadros
            // do contratempo, onde ele soa
            int counted = 0;
            for (const auto& f : a.frames())
            {
                const double ph = std::fmod(f.time, beat);
                if (!f.silent && std::abs(ph - beat / 2.0) < 0.03)
                {
                    sib[h] += f.chroma[10];
                    ++counted;
                }
                if (h == 1)
                    pfShare += f.percFlux > 0.0f;
            }
            sib[h] /= std::max(1, counted);
            if (h == 1)
                std::printf("fração percussiva %.2f · croma harmônico: %s\n", a.percussiveShare(),
                            a.harmonicChroma() ? "sim" : "não");
            ev[h] = evaluate(rd[h], g);
            std::printf("%s: pulso %.1f BPM (%s, conf %.2f) · tempos F %.2f · tonalidade %s (%.1f) · acordes %.0f %%\n",
                        h ? "com HPSS" : "sem HPSS", rd[h].pulse.bpm, ev[h].tempoVerdict.c_str(), rd[h].pulse.confidence,
                        ev[h].beatF, rd[h].key.name().c_str(), ev[h].keyScore, 100.0 * ev[h].chordOverlap);
        }
        std::printf("croma de Sib no contratempo (o tom): sem HPSS %.3f · com HPSS %.3f\n", sib[0], sib[1]);
        CHECK(sib[1] < 0.5 * sib[0]);   // a separação tira o tom do croma
        CHECK(pfShare > 0.0);           // e produz o fluxo percussivo
        CHECK(ev[1].tempoVerdict == "certo");
        CHECK(ev[1].keyScore >= ev[0].keyScore);
        CHECK(ev[1].chordOverlap >= ev[0].chordOverlap);
        CHECK(ev[1].chordOverlap >= 0.8);
    }

    // ---- o grave decide: Dó6 × Lám7 (as mesmas 4 notas em cima) --------------
    for (int comBateria = 0; comBateria < 2; ++comBateria)
    {
        const double beat = 0.625, bar = 4 * beat;
        const int bars = 12;
        std::vector<float> x(static_cast<std::size_t>(bars * bar * SR), 0.0f);
        Gabarito g;
        Noise n;
        for (int b = 0; b < bars; ++b)
        {
            const bool doMaior = b % 2 == 0;
            g.chords.push_back({ b * bar, doMaior ? 0 : 9 * 3 + 1 });
            const double baixo = doMaior ? 36.0 : 33.0;  // Dó2 (65 Hz) ou Lá1 (55 Hz)
            for (auto i = static_cast<std::size_t>(b * bar * SR); i < static_cast<std::size_t>((b + 1) * bar * SR) && i < x.size(); ++i)
            {
                const double t = static_cast<double>(i) / SR, local = t - b * bar;
                const double fade = std::min({ 1.0, local / 0.02, (bar - local) / 0.02 });
                double v = 0.0;
                for (double m : { 60.0, 64.0, 67.0, 69.0 })  // Dó Mi Sol Lá
                    for (int h = 1; h <= 4; ++h)
                        v += 0.05 * std::sin(2.0 * PI * hz(m) * h * t) / h;
                // baixo cheio embaixo, harmônicos fracos (um sub-baixo): o
                // croma principal (110 Hz–5 kHz) quase não o vê
                v += 0.20 * std::sin(2.0 * PI * hz(baixo) * t) + 0.02 * std::sin(4.0 * PI * hz(baixo) * t);
                x[i] += static_cast<float>(v * fade);
            }
        }
        if (comBateria)  // bumbo em todo tempo (50–110 Hz, a região do baixo) e chimbal
            for (double t = 0.0; t < bars * bar; t += beat)
            {
                const auto i0 = static_cast<std::size_t>(t * SR);
                for (std::size_t i = i0; i < std::min(x.size(), i0 + static_cast<std::size_t>(0.25 * SR)); ++i)
                {
                    const double u = static_cast<double>(i - i0) / SR;
                    x[i] += static_cast<float>(1.2 * std::sin(2.0 * PI * (50.0 * u + 60.0 * 0.04 * (1.0 - std::exp(-u / 0.04)))) * std::exp(-u / 0.15)
                                               + 0.2 * n.next() * std::exp(-u / 0.02));
                }
            }
        Analyzer a(SR);
        a.push(x.data(), x.size());
        a.finish();
        const double dur = static_cast<double>(x.size()) / SR;
        auto frames = a.frames();
        const auto com = readChords(frames, dur, 0.0, beat);
        for (auto& f : frames)
            f.bassChroma.fill(0.0f);
        const auto sem = readChords(frames, dur, 0.0, beat);
        Reading rc, rs;
        rc.duration = rs.duration = dur;
        rc.chords = com;
        rs.chords = sem;
        const double acc = evaluate(rc, g).chordOverlap, semAcc = evaluate(rs, g).chordOverlap;
        std::printf("Dó6 × Lám7%s: acordes com o grave %.0f %% · sem o grave %.0f %% (percussão %.2f, altura definida %.2f, grave = nota mais forte %.2f)\n",
                    comBateria ? " com bateria" : "", 100.0 * acc, 100.0 * semAcc, a.percussiveShare(),
                    readReference(a.frames(), a.onsets(), dur).pitchedShare, bassAgreement(a.frames()));
        CHECK(acc >= 0.85);
        CHECK(acc > semAcc + 0.2);
    }

    // ---- a melodia dentro da mistura: voz com vibrato + pad + bateria ----------
    //      (na região aguda e uma oitava abaixo, a de uma voz masculina)
    for (const double oitava : { 0.0, -12.0 })
    {
        double beat = 0.0;
        std::vector<std::pair<double, int>> acordes;
        auto x = faixa(beat, acordes);  // pad + bateria de 40 s (o primeiro cenário)
        struct Nota { double t0, t1, midi; };
        std::vector<Nota> notas;
        const double linha[] = { 69, 72, 74, 76, 74, 72, 71, 69, 67, 69, 71, 72, 76, 79, 76, 74 };
        double t = 0.5;
        for (int k = 0; t < 39.0; ++k)
        {
            const double d = (k % 3 == 0) ? 0.6 : 0.35;
            notas.push_back({ t, t + d, linha[k % 16] + oitava });
            t += d + 0.05;
        }
        for (const auto& nt : notas)
        {
            double ph = 0.0;
            for (auto i = static_cast<std::size_t>(nt.t0 * SR); i < static_cast<std::size_t>(nt.t1 * SR) && i < x.size(); ++i)
            {
                const double u = static_cast<double>(i) / SR - nt.t0;
                const double vib = u > 0.15 ? 0.3 * std::sin(2.0 * PI * 5.5 * u) : 0.0;  // ±30 cents
                ph += 2.0 * PI * hz(nt.midi + vib) / SR;
                const double env = std::min({ 1.0, u / 0.02, (nt.t1 - nt.t0 - u) / 0.03 });
                double v = 0.0;
                for (int hN = 1; hN <= 8; ++hN)
                    v += std::sin(hN * ph) / hN;
                x[i] += static_cast<float>(0.12 * env * v);
            }
        }
        Analyzer a(SR);
        a.push(x.data(), x.size());
        a.finish();
        int voiced = 0, okMel = 0, okYin = 0;
        for (const auto& f : a.frames())
            for (const auto& nt : notas)
                if (f.time >= nt.t0 + 0.06 && f.time <= nt.t1 - 0.06)
                {
                    ++voiced;
                    const auto cents = [&](float est) {
                        return est > 0.0f ? std::abs(1200.0 * std::log2(est / hz(nt.midi))) : 1e9;
                    };
                    okMel += cents(f.melodyHz) <= 50.0 + 30.0;  // a tolerância do MIREX + o vibrato
                    okYin += cents(f.f0Hz) <= 50.0 + 30.0;
                    break;
                }
        const double rMel = static_cast<double>(okMel) / std::max(1, voiced);
        const double rYin = static_cast<double>(okYin) / std::max(1, voiced);
        std::printf("melodia na mistura (%s): altura certa (±50 cents + vibrato) em %.0f %% dos quadros com saliência · %.0f %% com o YIN\n",
                    oitava < 0.0 ? "Lá3–Sol4" : "Sol4–Sol5", 100.0 * rMel, 100.0 * rYin);
        CHECK(rMel >= 0.7);
        CHECK(rMel > rYin + 0.2);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("leitura: todas as checagens passaram");
    return EXIT_SUCCESS;
}
