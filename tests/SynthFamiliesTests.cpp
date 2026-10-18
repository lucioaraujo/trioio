// As famílias de timbre (docs/SINTESE.md): cada uma soa, fica finita e
// toca a altura pedida as que têm altura definida; o perfil espectral
// reencontra os parciais de uma referência, inclusive um inarmônico.

#include "Composer.h"
#include "ModularVoice.h"
#include "RasgoSynthVoice.h"
#include "AntitotemVoice.h"
#include "AquorbiumVoice.h"
#include "AcousticVoice.h"
#include "ScoreSynth.h"
#include "Structure.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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

// referência: 200 Hz com parciais 1, 2 e 2,76 (o último inarmônico, de sino).
// Não usar 1, 2 e 3,5: 200 + 400 + 700 Hz se repetem a cada 1/100 s, e o
// YIN acerta ao achar a fundamental ausente de 100 Hz (razões 2, 4 e 7).
static std::vector<Frame> referencia()
{
    std::vector<float> v(static_cast<std::size_t>(SR * 6.0));
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR;
        v[i] = static_cast<float>(0.3 * std::sin(2 * PI * 200 * t) + 0.18 * std::sin(2 * PI * 400 * t)
                                  + 0.15 * std::sin(2 * PI * 552 * t));
    }
    Analyzer a(SR);
    a.push(v.data(), v.size());
    a.finish();
    return a.frames();
}

// a mediana da altura medida (YIN) entre 0,15 e 0,6 s de um sinal mono
static float alturaMedida(const std::vector<float>& mono)
{
    Analyzer a(SR);
    a.push(mono.data(), mono.size());
    a.finish();
    std::vector<float> ps;
    for (double t = 0.15; t < 0.6; t += 0.02)
    {
        const float p = pitchAt(a.frames(), t);
        if (std::isfinite(p))
            ps.push_back(p);
    }
    if (ps.empty())
        return NAN;
    std::nth_element(ps.begin(), ps.begin() + static_cast<long>(ps.size() / 2), ps.end());
    return ps[ps.size() / 2];
}

int main()
{
    // 1. o perfil espectral reencontra 1, 2 e 2,76
    const auto frames = referencia();
    const auto prof = spectralProfile(frames, 0.0, 6.0);
    std::printf("perfil:");
    for (std::size_t i = 0; i < prof.ratios.size(); ++i)
        std::printf("  %.2f (%.2f)", prof.ratios[i], prof.weights[i]);
    std::printf("  · ruído %.2f\n", prof.noise);
    const auto tem = [&](float r) {
        return std::any_of(prof.ratios.begin(), prof.ratios.end(), [&](float x) { return std::abs(x - r) < 0.06f; });
    };
    CHECK(tem(1.0f) && tem(2.0f) && tem(2.76f));
    CHECK(prof.noise < 0.2f);

    // 2. cada família soa (finita, audível, sem estouro) e as de altura
    //    definida tocam a nota pedida (Lá 220, MIDI 57)
    std::printf("Rasgo Modular no build: %s\n", modularAvailable() ? "sim" : "não");
    for (int k = 0; k < kTimbreCount; ++k)
    {
        const auto t = static_cast<Timbre>(k);
        Score s;
        s.duration = 1.6;
        s.spectra.push_back(prof);
        s.spectra.back().start = 0.0;
        s.spectra.back().end = 10.0;
        s.notes.push_back({ 0.05, 1.0, 57.0f, 0.0f, 0.8f, 0, t });
        std::vector<float> L, R;
        renderScore(s, SR, L, R);
        bool finito = true;
        double pot = 0.0;
        for (std::size_t i = 0; i < L.size(); ++i)
        {
            finito = finito && std::isfinite(L[i]) && std::isfinite(R[i]);
            pot += 0.5 * (L[i] * L[i] + R[i] * R[i]);
        }
        const double db = 10.0 * std::log10(pot / static_cast<double>(L.size()) + 1e-12);
        std::vector<float> mono(L.size());
        for (std::size_t i = 0; i < L.size(); ++i)
            mono[i] = 0.5f * (L[i] + R[i]);
        const float p = alturaMedida(mono);
        std::printf("%-16s %6.1f dB   altura %s", timbreName(t), db, std::isfinite(p) ? "" : "—");
        if (std::isfinite(p))
            std::printf("%.2f", p);
        std::printf("\n");
        CHECK(finito);
        CHECK(db > -40.0);
        // altura conferida onde ela é a matéria da família (tambor, modal e
        // membrana têm altura que cai ou modos inarmônicos: não entram)
        const bool comAltura = t == Timbre::additive || t == Timbre::breath || t == Timbre::plucked
                               || (modularAvailable() && (t == Timbre::wavetable || t == Timbre::waveguide
                                                          || t == Timbre::vocal || t == Timbre::folded
                                                          || t == Timbre::pulsar))
                               // Rasgo Synth: as de altura estável (Gendyn vagueia ±25 % por
                               // técnica; galo segue o contorno do chamado)
                               || t == Timbre::vosim || t == Timbre::walsh || t == Timbre::theremin
                               || t == Timbre::flute || t == Timbre::scat || t == Timbre::chip
                               // segunda leva: CMOS (a nota convertida para a "tensão" da
                               // voz) e o ressonador soprado; coro e Rungler não têm altura
                               // única (acorde; saltos em volta da nota)
                               || t == Timbre::cmos || (modularAvailable() && t == Timbre::resonator)
                               // terceira leva: LPG e a corda Bioma têm altura clara; o
                               // operador FM (razões não inteiras), a nuvem (desvio de
                               // altura), o objeto espectral (parciais pulsando por crivo),
                               // o ressonador Bioma (material) e o striker (partículas
                               // espalhadas) não têm uma altura única por técnica
                               || (modularAvailable() && t == Timbre::lpg) || t == Timbre::biomaString
                               // 2026-10-14, os corpos acústicos: todos com altura definida
                               || t == Timbre::bowed || t == Timbre::reed || t == Timbre::bars
                               || t == Timbre::piano || t == Timbre::guitar
                               // segunda leva: as de altura; as percussões não
                               || t == Timbre::upright || t == Timbre::transverseFlute
                               // as receitas da corda dedilhada
                               || t == Timbre::sevenString || t == Timbre::requinto || t == Timbre::cavaquinho
                               || t == Timbre::bandolim || t == Timbre::manouche
                               // terceira leva: os sopros (a bateria não tem altura)
                               || t == Timbre::sax || t == Timbre::trumpet || t == Timbre::trombone
                               // a voz cantada e a boca chiusa (o sussurro não tem altura)
                               || t == Timbre::singing || t == Timbre::hum
                               // os elétricos
                               || t == Timbre::electricGuitar || t == Timbre::distortedGuitar || t == Timbre::electricBass
                               // o órgão e a palheta livre
                               || t == Timbre::organ || t == Timbre::accordion || t == Timbre::bandoneon
                               // os naipes
                               || t == Timbre::stringSection || t == Timbre::brassSection || t == Timbre::voiceChoir;
        if (comAltura)
        {
            // a nota, uma oitava acima ou abaixo (o YIN pode pegar o 2º harmônico)
            const bool certa = std::isfinite(p)
                               && (std::abs(p - 57.0f) < 0.5f || std::abs(p - 69.0f) < 0.5f || std::abs(p - 45.0f) < 0.5f);
            CHECK(certa);
        }
        // espectral: o YIN se inclina com o parcial inarmônico (lê ~56 em vez
        // de 57), então a prova é o espectro: picos em 1, 2 e 2,75 × 220 Hz
        if (t == Timbre::spectral)
        {
            Analyzer a(SR);
            a.push(mono.data(), mono.size());
            a.finish();
            for (double alvo : { 220.0, 440.0, 2.75 * 220.0 })
            {
                bool achou = false;
                for (const auto& f : a.frames())
                    if (f.time > 0.15 && f.time < 0.6)
                        for (float pk : f.peaksHz)
                            achou = achou || std::abs(pk / alvo - 1.0) < 0.015;
                std::printf("  espectral: pico em %.0f Hz %s\n", alvo, achou ? "presente" : "AUSENTE");
                CHECK(achou);
            }
        }
    }

    // 2c. os corpos acústicos afinados em toda a extensão do instrumento
    //     (docs/MAQUINA_DE_TIMBRES.md): sem saltar de registro (a flauta
    //     sobressoprada pula para um harmônico). O YIN mede de 50 a 1000 Hz:
    //     as notas ficam entre MIDI 32 e 83
    {
        struct Range { Timbre t; std::vector<float> notes; };
        const std::vector<Range> ranges {
            { Timbre::bowed, { 36, 43, 48, 55, 62, 67, 74, 79, 83 } },
            { Timbre::reed, { 50, 55, 58, 62, 65, 69, 74, 77 } },
            { Timbre::bars, { 45, 52, 57, 64, 69, 76, 81, 83 } },
            { Timbre::piano, { 33, 40, 45, 52, 57, 64, 69, 76, 83 } },
            { Timbre::guitar, { 40, 45, 52, 57, 62, 67, 72, 76 } },
            { Timbre::upright, { 32, 33, 36, 40, 45, 50, 55 } },
            { Timbre::sevenString, { 35, 40, 45, 52, 57, 64, 72 } },
            { Timbre::requinto, { 50, 55, 62, 67, 74, 79, 83 } },
            { Timbre::cavaquinho, { 62, 67, 71, 74, 79, 83 } },
            { Timbre::bandolim, { 55, 60, 64, 69, 74, 79, 83 } },
            { Timbre::manouche, { 40, 47, 52, 59, 64, 71, 76, 83 } },
            { Timbre::sax, { 44, 49, 52, 56, 61, 64, 68, 73, 76, 80 } },
            { Timbre::trumpet, { 54, 58, 62, 65, 69, 72, 76, 79, 82 } },
            { Timbre::trombone, { 40, 45, 48, 52, 55, 60, 64, 67, 72 } },
            { Timbre::singing, { 40, 45, 50, 55, 60, 64, 67, 72, 76, 79, 83 } },
            { Timbre::hum, { 45, 52, 57, 62, 67, 72 } },
            { Timbre::electricGuitar, { 40, 45, 52, 57, 64, 69, 76, 83 } },
            { Timbre::distortedGuitar, { 40, 45, 52, 57, 64, 69, 76, 83 } },
            { Timbre::electricBass, { 32, 36, 40, 45, 50, 55 } },
            { Timbre::organ, { 44, 48, 55, 60, 67, 72, 79, 83 } },  // o 16' abaixo de 50 Hz não se mede
            { Timbre::accordion, { 45, 52, 57, 64, 69, 76, 81 } },
            { Timbre::bandoneon, { 36, 43, 48, 55, 60, 67, 72, 79 } },
            { Timbre::stringSection, { 43, 50, 57, 64, 72, 79 } },
            { Timbre::brassSection, { 45, 52, 60, 67, 74, 79 } },
            { Timbre::voiceChoir, { 45, 55, 64, 72, 79 } },
            { Timbre::transverseFlute, { 60, 62, 64, 65, 67, 69, 72, 74, 76, 79, 81, 83 } },
        };
        for (const auto& rg : ranges)
        {
            std::printf("extensão %-18s", timbreName(rg.t));
            int fora = 0;
            for (float m : rg.notes)
            {
                // o som SECO do instrumento (antes da sala): a reverberação
                // sobre uma nota grave e escura (o sax no grave) baixa a
                // periodicidade abaixo do limiar do YIN, e a medida passava
                // à melodia predominante, que pegava um harmônico
                // (2026-10-15). Aqui se confere a afinação do instrumento.
                Note nota { 0.0, 0.8, m, 0.0f, 0.7f, 0, rg.t };
                auto seco = trioio::renderAcousticNote(nota, SR, 12345);
                std::vector<float> mono(static_cast<std::size_t>(0.05 * SR), 0.0f);
                mono.insert(mono.end(), seco.begin(), seco.end());
                const float p = alturaMedida(mono);
                // o órgão na registração 888000000: o tirante de 16' soa uma
                // oitava abaixo da tecla (como num Hammond real)
                const float esperado = rg.t == Timbre::organ ? m - 12.0f : m;
                const bool ok = std::isfinite(p) && std::abs(p - esperado) < 0.5f;
                fora += ok ? 0 : 1;
                std::printf(" %g%s", static_cast<double>(m), ok ? "" : (std::isfinite(p) ? "✗" : "?"));
                if (!ok && std::isfinite(p))
                    std::printf("(%.1f)", static_cast<double>(p));
            }
            std::printf("\n");
            CHECK(fora == 0);
        }
    }

    // 2b. o corpo SE MOVE: uma nota espectral que atravessa dois corpos
    //     (1; 2,76 e depois 1; 3,5) começa com o primeiro e termina no segundo
    {
        Score s;
        s.duration = 6.0;
        s.spectra.push_back({ 0.0, 2.0, { 1.0f, 2.76f }, { 0.5f, 0.5f }, 0.0f });
        s.spectra.push_back({ 4.0, 6.0, { 1.0f, 3.5f }, { 0.5f, 0.5f }, 0.0f });
        s.notes.push_back({ 0.5, 4.5, 57.0f, 0.0f, 0.8f, 0, Timbre::spectral });  // de 0,5 a 5 s
        std::vector<float> L, R;
        renderScore(s, SR, L, R);
        std::vector<float> mono(L.size());
        for (std::size_t i = 0; i < L.size(); ++i)
            mono[i] = 0.5f * (L[i] + R[i]);
        // energia de uma frequência num trecho (Goertzel): a medida que
        // distingue um parcial forte de um fraco
        const auto energia = [&](double t0, double t1, double hz) {
            const auto i0 = static_cast<std::size_t>(t0 * SR), i1 = static_cast<std::size_t>(t1 * SR);
            const double w = 2.0 * PI * hz / SR, c = 2.0 * std::cos(w);
            double s1 = 0.0, s2 = 0.0;
            for (std::size_t i = i0; i < i1; ++i)
            {
                const double s0 = mono[i] + c * s1 - s2;
                s2 = s1;
                s1 = s0;
            }
            return s1 * s1 + s2 * s2 - c * s1 * s2;
        };
        // quanto o 2,76× domina o 3,5×, no início e no fim da nota
        const double ini = energia(0.6, 1.2, 2.76 * 220) / (energia(0.6, 1.2, 3.5 * 220) + 1e-12);
        const double fim = energia(4.3, 4.9, 2.76 * 220) / (energia(4.3, 4.9, 3.5 * 220) + 1e-12);
        std::printf("corpo que se move: 2,76× / 3,5× = %.1f no início, %.3f no fim\n", ini, fim);
        CHECK(ini > 4.0);   // o primeiro corpo manda no início
        CHECK(fim < 0.25);  // o segundo, no fim
    }

    // 3. as famílias hospedadas são do Rasgo Modular de fato (não o aditivo)
    if (modularAvailable())
    {
        const auto ouvir = [](Timbre t) {
            return renderModularNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, t }, SR, 7);
        };
        for (Timbre t : { Timbre::wavetable, Timbre::pulsar, Timbre::modal, Timbre::waveguide, Timbre::drum,
                          Timbre::vocal, Timbre::folded })
            CHECK(!ouvir(t).empty());
        CHECK(ouvir(Timbre::additive).empty());
    }

    // 4. as vozes do Rasgo Synth vêm da cópia fixada (não do aditivo)
    for (Timbre t : { Timbre::gendyn, Timbre::vosim, Timbre::walsh, Timbre::theremin, Timbre::flute, Timbre::scat,
                      Timbre::chip, Timbre::galo })
        CHECK(!renderRasgoSynthNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, t }, SR, 7).empty());
    CHECK(renderRasgoSynthNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, Timbre::additive }, SR, 7).empty());
    for (Timbre t : { Timbre::choir, Timbre::rungler })
        CHECK(!renderRasgoSynthNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, t }, SR, 7).empty());
    CHECK(!renderAntitotemNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, Timbre::cmos }, SR, 7).empty());
    CHECK(renderAntitotemNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, Timbre::additive }, SR, 7).empty());
    if (modularAvailable())
        for (Timbre t : { Timbre::resonator, Timbre::operatorFm, Timbre::lpg })
            CHECK(!renderModularNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, t }, SR, 7).empty());
    for (Timbre t : { Timbre::granular, Timbre::spectralObject })
        CHECK(!renderRasgoSynthNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, t }, SR, 7).empty());
    for (Timbre t : { Timbre::biomaModal, Timbre::biomaString, Timbre::striker })
        CHECK(!renderAquorbiumNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, t }, SR, 7).empty());
    CHECK(renderAquorbiumNote({ 0.0, 0.5, 57.0f, 0.0f, 0.8f, 0, Timbre::additive }, SR, 7).empty());

    // 5. as salas do Rasgo Modular: um golpe curto deixa cauda (HALL e SPACE),
    //    finita, sem estourar; a sala do Trioio fica com o ScoreSynth
    if (modularAvailable())
    {
        for (Room r : { Room::hall, Room::space })
        {
            std::vector<float> L(static_cast<std::size_t>(SR * 3.0), 0.0f), R = L;
            for (std::size_t i = 0; i < 2400; ++i)
                L[i] = R[i] = static_cast<float>(0.5 * std::sin(2 * PI * 440 * i / SR) * (1.0 - i / 2400.0));
            CHECK(applyModularRoom(r, SR, L, R, 3));
            double cauda = 0.0;
            bool finito = true;
            float pico = 0.0f;
            for (std::size_t i = 0; i < L.size(); ++i)
            {
                finito = finito && std::isfinite(L[i]) && std::isfinite(R[i]);
                pico = std::max({ pico, std::abs(L[i]), std::abs(R[i]) });
                if (i > static_cast<std::size_t>(0.08 * SR))  // o seco dura 0,05 s
                    cauda += L[i] * L[i] + R[i] * R[i];
            }
            const double db = 10.0 * std::log10(cauda / (2.0 * (L.size() - 0.08 * SR)) + 1e-12);
            std::printf("sala %-22s cauda depois de 0,08 s: %6.1f dB · pico %.2f\n", roomName(r), db, pico);
            CHECK(finito && pico < 1.5f && db > -70.0);
        }
        std::vector<float> L(100, 0.1f), R = L;
        CHECK(!applyModularRoom(Room::trioio, SR, L, R, 3));
    }

    // 5. a curva de altura soa (ornamentos, ScaleIdiom.h): oscilação de ±1
    //    semitom a 2 Hz e uma curva que chega de 2 semitons abaixo, medidas
    //    pela altura fundamental do áudio nas famílias de altura contínua
    for (Timbre t : { Timbre::additive, Timbre::fm, Timbre::breath })
    {
        const auto medir = [&](const Note& nota) {
            Score s;
            s.duration = 1.6;
            s.notes.push_back(nota);
            std::vector<float> L, R;
            renderScore(s, SR, L, R);
            std::vector<float> mono(L.size());
            for (std::size_t i = 0; i < L.size(); ++i)
                mono[i] = 0.5f * (L[i] + R[i]);
            Analyzer an(SR);
            an.push(mono.data(), mono.size());
            an.finish();
            return an.frames();
        };
        const auto f0 = [](const std::vector<Frame>& fr, double t) {
            std::vector<float> v;
            for (const auto& f : fr)
                if (std::abs(f.time - t) < 0.03 && f.f0Hz > 0.0f)
                    v.push_back(f.f0Hz);
            if (v.empty())
                return 0.0f;
            std::sort(v.begin(), v.end());
            return v[v.size() / 2];
        };
        Note osc { 0.05, 1.2, 57.0f, 0.0f, 0.8f, 0, t };
        osc.wobbleDepth = 1.0f;
        osc.wobbleRate = 2.0f;
        const auto fo = medir(osc);
        const float alto = f0(fo, 0.05 + 0.125), baixo = f0(fo, 0.05 + 0.375);
        Note curva { 0.05, 1.2, 57.0f, 0.0f, 0.8f, 0, t };
        curva.bend = { { 0.0f, -2.0f }, { 0.4f, 0.0f } };
        const auto fc = medir(curva);
        const float inicio = f0(fc, 0.12), fim = f0(fc, 0.9);
        std::printf("curva de altura em %s: oscilação %.1f/%.1f Hz (razão %.3f; esperado 1,122) · curva %.1f → %.1f Hz\n",
                    timbreName(t), alto, baixo, baixo > 0 ? alto / baixo : 0.0, inicio, fim);
        CHECK(alto > 0 && baixo > 0 && alto / baixo > 1.08 && alto / baixo < 1.17);
        CHECK(inicio > 0 && fim > 0 && inicio < 0.96f * fim);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("famílias de síntese: todas as checagens passaram");
    return EXIT_SUCCESS;
}
