#include "ScoreSynth.h"

#include "Denormals.h"

#include "AntitotemVoice.h"
#include "AcousticVoice.h"
#include "ModularRasgo.h"
#include "Production.h"
#include "AquorbiumVoice.h"
#include "ModularVoice.h"
#include "RasgoSynthVoice.h"

#include <algorithm>

#include <cstdio>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <array>
#include <cmath>
#include <cstdint>

namespace trioio
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

double midiHz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

struct Pan
{
    double l, r;
    explicit Pan(double p) : l(std::cos((std::clamp(p, -1.0, 1.0) + 1.0) * kPi / 4.0)), r(std::sin((std::clamp(p, -1.0, 1.0) + 1.0) * kPi / 4.0)) {}
};

struct Noise
{
    std::uint64_t s;
    double next()
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return static_cast<double>(s >> 11) / 9007199254740992.0 * 2.0 - 1.0;
    }
};

// filtro de estado variável (TPT), saída passa-banda
struct Band
{
    double ic1 = 0.0, ic2 = 0.0;
    double run(double x, double hz, double q, double sr)
    {
        const double g = std::tan(kPi * std::clamp(hz, 20.0, sr * 0.45) / sr), k = 1.0 / std::max(q, 0.3);
        const double v1 = (ic1 + g * (x - ic2)) / (1.0 + g * (g + k));
        const double v2 = ic2 + g * v1;
        ic1 = 2.0 * v1 - ic1;
        ic2 = 2.0 * v2 - ic2;
        return v1;
    }
};

void put(std::vector<float>& L, std::vector<float>& R, std::size_t i, double v, const Pan& p)
{
    if (i < L.size())
    {
        L[i] += static_cast<float>(v * p.l);
        R[i] += static_cast<float>(v * p.r);
    }
}

double releaseEnv(double t, double dur, double release)
{
    return t > dur ? std::max(0.0, 1.0 - (t - dur) / release) : 1.0;
}

// aditivo: 8 parciais; os agudos decaem antes (ataque brilhante, corpo que escurece)
void additive(const composition::Note& n, double sr, std::vector<float>& L, std::vector<float>& R, const Pan& pan)
{
    const double vel = n.velocity, rolloff = 1.7 - 0.9 * vel;
    const double decay = std::clamp(n.duration * 1.6, 0.35, 3.5), attack = 0.006 - 0.004 * vel, release = 0.07;
    const auto i0 = static_cast<std::size_t>(n.time * sr);
    const auto len = static_cast<std::size_t>((n.duration + release) * sr);
    const double amp = 0.22 * std::pow(vel, 1.5);
    std::array<double, 8> phase {}, a {}, tau {};
    for (std::size_t k = 0; k < 8; ++k)
    {
        a[k] = 1.0 / std::pow(static_cast<double>(k) + 1.0, rolloff);
        tau[k] = decay / (1.0 + 0.7 * static_cast<double>(k));
    }
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        double semis = n.pitchAt(t);
        if (n.duration > 0.4 && t > 0.25)
            semis += 0.12 * std::min(1.0, (t - 0.25) / 0.3) * std::sin(2.0 * kPi * 5.2 * t);
        const double f0 = midiHz(semis);
        const double env = std::min(1.0, t / attack) * releaseEnv(t, n.duration, release);
        double s = 0.0;
        for (std::size_t k = 0; k < 8; ++k)
        {
            const double f = f0 * static_cast<double>(k + 1);
            if (f > 0.45 * sr)
                break;
            phase[k] += f / sr;
            s += a[k] * std::exp(-t / tau[k]) * std::sin(2.0 * kPi * phase[k]);
        }
        put(L, R, i0 + i, amp * env * s, pan);
    }
}

// sopro: poucos parciais suaves + ar (ruído filtrado), mais forte no ataque
void breath(const composition::Note& n, double sr, std::vector<float>& L, std::vector<float>& R, const Pan& pan)
{
    const double attack = 0.03, release = 0.12;
    const auto i0 = static_cast<std::size_t>(n.time * sr);
    const auto len = static_cast<std::size_t>((n.duration + release) * sr);
    const double amp = 0.2 * std::pow(n.velocity, 1.3);
    Noise nz { 0x9E37 + static_cast<std::uint64_t>(n.time * 1000.0) };
    Band band;
    std::array<double, 4> ph {};
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        double semis = n.pitchAt(t);
        if (n.duration > 0.3 && t > 0.2)
            semis += 0.08 * std::sin(2.0 * kPi * 4.6 * t);
        const double f0 = midiHz(semis);
        double tone = 0.0;
        for (std::size_t k = 0; k < 4; ++k)
        {
            ph[k] += f0 * static_cast<double>(k + 1) / sr;
            tone += std::sin(2.0 * kPi * ph[k]) / static_cast<double>((k + 1) * (k + 1));
        }
        const double air = band.run(nz.next(), f0 * 2.0, 2.5, sr) * (0.35 + 1.2 * std::exp(-t / 0.04));
        const double env = std::min(1.0, t / attack) * releaseEnv(t, n.duration, release);
        put(L, R, i0 + i, amp * env * (tone + air), pan);
    }
}

// metal: parciais inarmônicos de sino, que soam além da nota
void metal(const composition::Note& n, double sr, std::vector<float>& L, std::vector<float>& R, const Pan& pan)
{
    static const double ratio[] = { 1.0, 2.76, 5.40, 8.93, 13.34 };
    static const double amps[] = { 1.0, 0.6, 0.4, 0.25, 0.15 };
    static const double taus[] = { 2.5, 1.6, 1.0, 0.6, 0.4 };
    const double f0 = midiHz(n.pitch), scale = 0.6 + n.velocity;
    const double ring = std::min(4.0, taus[0] * scale * 2.5);
    const auto i0 = static_cast<std::size_t>(n.time * sr);
    const auto len = static_cast<std::size_t>(ring * sr);
    const double amp = 0.16 * n.velocity;
    std::array<double, 5> ph {};
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        double s = 0.0;
        for (std::size_t k = 0; k < 5; ++k)
        {
            const double f = f0 * ratio[k];
            if (f > 0.45 * sr)
                break;
            ph[k] += f / sr;
            s += amps[k] * std::exp(-t / (taus[k] * scale)) * std::sin(2.0 * kPi * ph[k]);
        }
        put(L, R, i0 + i, amp * std::min(1.0, t / 0.002) * s, pan);
    }
}

// corda pinçada: Karplus-Strong (linha de atraso com média e perda)
void plucked(const composition::Note& n, double sr, std::vector<float>& L, std::vector<float>& R, const Pan& pan)
{
    const double f0 = midiHz(n.pitch);
    const double period = sr / f0;
    const auto N = static_cast<std::size_t>(std::max(2.0, std::floor(period)));
    const double frac = period - static_cast<double>(N);
    std::vector<double> line(N + 2);
    Noise nz { 0xC0FFEE + static_cast<std::uint64_t>(n.pitch * 997.0 + n.time * 1000.0) };
    double prev = 0.0;
    const double bright = 0.3 + 0.6 * n.velocity;  // tocar forte: mais agudos no ataque
    for (auto& x : line)
    {
        const double r = nz.next();
        prev = bright * r + (1.0 - bright) * prev;
        x = prev;
    }
    const auto i0 = static_cast<std::size_t>(n.time * sr);
    const auto len = static_cast<std::size_t>((n.duration + 1.0) * sr);
    const double amp = 0.3 * n.velocity;
    std::size_t pos = 0;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const std::size_t a = pos, b = (pos + 1) % line.size();
        const double out = line[a] * (1.0 - frac) + line[b] * frac;
        line[a] = 0.996 * 0.5 * (line[a] + line[b]);
        pos = b;
        put(L, R, i0 + i, amp * out * releaseEnv(t, n.duration + 0.6, 0.4), pan);
    }
}

// membrana: modos inarmônicos curtos; a altura cai logo depois do golpe
void membrane(const composition::Note& n, double sr, std::vector<float>& L, std::vector<float>& R, const Pan& pan)
{
    static const double ratio[] = { 1.0, 1.59, 2.14, 2.30, 2.65, 2.92 };
    static const double amps[] = { 1.0, 0.7, 0.5, 0.4, 0.3, 0.25 };
    const double f0 = midiHz(n.pitch), base = 0.25 + 0.5 * std::min(1.0, n.duration);
    const auto i0 = static_cast<std::size_t>(n.time * sr);
    const auto len = static_cast<std::size_t>(std::min(2.0, base * 4.0) * sr);
    const double amp = 0.28 * n.velocity;
    std::array<double, 6> ph {};
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double drop = 1.0 + 0.3 * std::exp(-t / 0.03);
        double s = 0.0;
        for (std::size_t k = 0; k < 6; ++k)
        {
            ph[k] += f0 * ratio[k] * drop / sr;
            s += amps[k] * std::exp(-t / (base / (1.0 + 0.8 * static_cast<double>(k)))) * std::sin(2.0 * kPi * ph[k]);
        }
        put(L, R, i0 + i, amp * std::min(1.0, t / 0.001) * s, pan);
    }
}

// FM de dois operadores, razão 1,41: rugoso; o índice cai com o tempo
void fm(const composition::Note& n, double sr, std::vector<float>& L, std::vector<float>& R, const Pan& pan)
{
    const double release = 0.08;
    const auto i0 = static_cast<std::size_t>(n.time * sr);
    const auto len = static_cast<std::size_t>((n.duration + release) * sr);
    const double amp = 0.2 * n.velocity;
    double pc = 0.0, pm = 0.0;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double f0 = midiHz(n.pitchAt(t));
        const double index = 3.0 * n.velocity * std::exp(-t / 0.4) + 0.5;
        pm += f0 * 1.41 / sr;
        pc += f0 / sr;
        const double s = std::sin(2.0 * kPi * pc + index * std::sin(2.0 * kPi * pm));
        const double env = std::min(1.0, t / 0.005) * std::exp(-t / std::max(0.3, n.duration * 1.5)) * releaseEnv(t, n.duration, release);
        put(L, R, i0 + i, amp * env * s, pan);
    }
}

// baixo aditivo: 3 parciais, ataque macio
void bassAdditive(const composition::Note& n, double sr, std::vector<float>& L, std::vector<float>& R)
{
    const double release = 0.12, attack = 0.015;
    const auto i0 = static_cast<std::size_t>(n.time * sr);
    const auto len = static_cast<std::size_t>((n.duration + release) * sr);
    const double f0 = midiHz(n.pitch), amp = 0.30 * n.velocity;
    double ph = 0.0;
    const Pan center(0.0);
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double env = std::min(1.0, t / attack) * std::exp(-t / 2.5) * releaseEnv(t, n.duration, release);
        ph += f0 / sr;
        const double s = std::sin(2.0 * kPi * ph) + 0.35 * std::sin(4.0 * kPi * ph) + 0.12 * std::sin(6.0 * kPi * ph);
        put(L, R, i0 + i, amp * env * s * 1.41, center);
    }
}

// espectral: o corpo da referência ressintetizado. Os parciais medidos
// (razões e pesos do SpectralProfile) sobre a fundamental da nota; cada
// parcial decai mais cedo quanto mais agudo, com um batimento lento próprio;
// a parte de ruído vira ar filtrado na região do parcial mais forte.
// Desde 2026-10-07 o corpo SE MOVE: a nota parte do perfil do seu início
// (`a`) e se transforma no do seu fim (`b`), cruzando os dois conjuntos de
// parciais. Sem perfil (referência sem altura), cai no aditivo.
void spectral(const composition::Note& n, const composition::SpectralProfile* a,
              const composition::SpectralProfile* b, double sr, std::vector<float>& L, std::vector<float>& R,
              const Pan& pan)
{
    if (!a || a->ratios.empty())
        return additive(n, sr, L, R, pan);
    if (!b || b->ratios.empty())
        b = a;
    struct Partial { double ratio, weight, tau, beat, ph; bool late; };
    std::vector<Partial> parts;
    const double decay = std::clamp(n.duration * 2.0, 0.4, 4.0);
    const auto add = [&](const composition::SpectralProfile* p, bool late) {
        for (std::size_t j = 0; j < p->ratios.size(); ++j)
            parts.push_back({ p->ratios[j], p->weights[j], decay / (0.6 + 0.4 * p->ratios[j]),
                              0.15 + 0.35 * std::fmod(p->ratios[j] * 7.31, 1.0), 0.0, late });
    };
    add(a, false);
    if (b != a)
        add(b, true);
    double strongRatio = a->ratios.front(), strongW = -1.0;
    for (std::size_t j = 0; j < a->ratios.size(); ++j)
        if (a->weights[j] > strongW) { strongW = a->weights[j]; strongRatio = a->ratios[j]; }
    const double release = 0.18, attack = 0.012;
    const auto i0 = static_cast<std::size_t>(n.time * sr);
    const auto len = static_cast<std::size_t>((n.duration + release) * sr);
    const double amp = 0.24 * std::pow(n.velocity, 1.4);
    Noise nz { 0x51E7 + static_cast<std::uint64_t>(n.time * 1000.0) };
    Band band;
    const double air = 0.6 * std::clamp(0.5 * static_cast<double>(a->noise + b->noise), 0.0, 1.0);
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double x = b == a ? 0.0 : std::min(1.0, t / std::max(n.duration, 1e-3));  // a → b
        const double f0 = midiHz(n.pitchAt(t));
        double s = 0.0;
        for (auto& q : parts)
        {
            const double f = f0 * q.ratio;
            if (f > 0.45 * sr)
                continue;
            q.ph += f / sr;
            const double g = q.late ? x : (b == a ? 1.0 : 1.0 - x);
            s += g * q.weight * std::exp(-t / q.tau) * (1.0 + 0.15 * std::sin(2.0 * kPi * q.beat * t))
                 * std::sin(2.0 * kPi * q.ph);
        }
        s *= 2.2;  // os pesos somam 1: traz ao nível das outras famílias
        if (air > 0.0)
            s += air * band.run(nz.next(), f0 * strongRatio, 3.0, sr) * std::exp(-t / 0.3);
        const double env = std::min(1.0, t / attack) * releaseEnv(t, n.duration, release);
        put(L, R, i0 + i, amp * env * s, pan);
    }
}

// famílias do Rasgo Modular e do Rasgo Synth: a nota vem do host
// (ModularVoice / RasgoSynthVoice) e é levada ao
// nível das famílias próprias (pico ≈ 0,3 × velocidade^1,5), com o pan da voz
void hosted(const composition::Note& n, double sr, std::vector<float>& L, std::vector<float>& R, const Pan& pan)
{
    const auto seed = static_cast<std::uint64_t>(n.time * 1000.0) * 0x9E3779B97F4A7C15ull
                      + static_cast<std::uint64_t>(n.pitch * 100.0f);
    auto mono = renderModularNote(n, sr, seed);
    if (mono.empty())
        mono = renderRasgoSynthNote(n, sr, seed);
    if (mono.empty())
        mono = renderAntitotemNote(n, sr, seed);
    if (mono.empty())
        mono = renderAquorbiumNote(n, sr, seed);
    if (mono.empty())
        mono = renderAcousticNote(n, sr, seed);
    if (mono.empty())
        return additive(n, sr, L, R, pan);
    // o rasgo modular: o patch atravessa o corpo (ModularRasgo.h)
    if (n.patch != 0)
        composition::applyPatch(mono, n, sr, seed ^ 0x5EEDull);
    // nível: pelo pico, mas sem deixar as percussivas (golpe alto, corpo que
    // some depressa) muito abaixo das sustentadas. A referência é o maior
    // entre 35 % do pico e 2 × o RMS da nota: nas sustentadas (RMS ≈ metade
    // do pico) dá o pico, como antes; nas percussivas sobe até ~9 dB.
    float peak = 0.0f;
    double pow2 = 0.0;
    const auto inNote = std::min(mono.size(), static_cast<std::size_t>(std::max(n.duration, 0.05) * sr));
    for (std::size_t i = 0; i < mono.size(); ++i)
    {
        peak = std::max(peak, std::abs(mono[i]));
        if (i < inNote)
            pow2 += static_cast<double>(mono[i]) * mono[i];
    }
    if (peak < 1e-6f)
        return;
    const double rms = std::sqrt(pow2 / static_cast<double>(std::max<std::size_t>(1, inNote)));
    const double g = 0.3 * std::pow(n.velocity, 1.5) / std::max(0.35 * peak, 2.0 * rms);
    const auto i0 = static_cast<std::size_t>(n.time * sr);
    for (std::size_t i = 0; i < mono.size(); ++i)
        put(L, R, i0 + i, g * mono[i], pan);
}

void renderNote(const composition::Note& n, const composition::Score& score, double sr,
                std::vector<float>& L, std::vector<float>& R)
{
    using composition::Timbre;
    // percussão (voz 4): o grave no centro, o agudo levemente à direita
    const Pan pan(n.voice == 1 ? 0.0
                  : n.voice == 4 ? (n.pitch < 55.0f ? 0.0 : 0.35)
                                 : std::clamp((n.pitch - 60.0) / 48.0, -0.6, 0.6) * (n.voice == 2 || n.voice == 5 ? -1.0 : 1.0));
    if (n.voice == 1 && n.timbre == Timbre::additive)
        return bassAdditive(n, sr, L, R);
    switch (n.timbre)
    {
        case Timbre::additive: return additive(n, sr, L, R, pan);
        case Timbre::breath: return breath(n, sr, L, R, pan);
        case Timbre::metal: return metal(n, sr, L, R, pan);
        case Timbre::plucked: return plucked(n, sr, L, R, pan);
        case Timbre::membrane: return membrane(n, sr, L, R, pan);
        case Timbre::fm: return fm(n, sr, L, R, pan);
        case Timbre::spectral:
        {
            // a memória toca com o corpo do instante de onde veio
            const double from = n.source >= 0.0 ? n.source : n.time;
            return spectral(n, composition::spectralAt(score, from),
                            composition::spectralAt(score, from + n.duration), sr, L, R, pan);
        }
        case Timbre::wavetable:
        case Timbre::pulsar:
        case Timbre::modal:
        case Timbre::waveguide:
        case Timbre::drum:
        case Timbre::vocal:
        case Timbre::folded:
        case Timbre::gendyn:
        case Timbre::vosim:
        case Timbre::walsh:
        case Timbre::theremin:
        case Timbre::flute:
        case Timbre::scat:
        case Timbre::chip:
        case Timbre::galo:
        case Timbre::resonator:
        case Timbre::cmos:
        case Timbre::choir:
        case Timbre::rungler:
        case Timbre::operatorFm:
        case Timbre::lpg:
        case Timbre::granular:
        case Timbre::spectralObject:
        case Timbre::biomaModal:
        case Timbre::biomaString:
        case Timbre::striker:
        case Timbre::bowed:
        case Timbre::reed:
        case Timbre::bars:
        case Timbre::piano:
        case Timbre::guitar:
        case Timbre::upright:
        case Timbre::transverseFlute:
        case Timbre::surdo:
        case Timbre::conga:
        case Timbre::pandeiro:
        case Timbre::shaker:
        case Timbre::sevenString:
        case Timbre::requinto:
        case Timbre::cavaquinho:
        case Timbre::bandolim:
        case Timbre::manouche:
        case Timbre::sax:
        case Timbre::trumpet:
        case Timbre::trombone:
        case Timbre::ride:
        case Timbre::hihat:
        case Timbre::snare:
        case Timbre::brushes:
        case Timbre::kick:
        case Timbre::bongo:
        case Timbre::guiro:
        case Timbre::singing:
        case Timbre::whisper:
        case Timbre::hum:
        case Timbre::tom:
        case Timbre::crash:
        case Timbre::electricGuitar:
        case Timbre::distortedGuitar:
        case Timbre::electricBass:
        case Timbre::tamborim:
        case Timbre::repique:
        case Timbre::agogo:
        case Timbre::cuica:
        case Timbre::chocalho:
        case Timbre::organ:
        case Timbre::accordion:
        case Timbre::bandoneon:
        case Timbre::zabumba:
        case Timbre::triangle:
        case Timbre::stringSection:
        case Timbre::brassSection:
        case Timbre::voiceChoir:
        case Timbre::bowedPlate:
        case Timbre::sungString:
        case Timbre::blownSkin:
            return hosted(n, sr, L, R, pan);
    }
}

// ---- objetos sonoros -------------------------------------------------------
double lerp(double a, double b, double x) { return a + (b - a) * x; }

// envelope de gesto: entra e sai suavemente (até 0,5 s ou 1/4 da duração)
double gestureEnv(double t, double dur)
{
    const double edge = std::min(0.5, dur / 4.0);
    return std::min({ 1.0, t / edge, (dur - t) / edge }) * 1.0;
}

// ---- efeitos de cinema (docs/MAQUINA_DE_TIMBRES.md §3.7) -----------------------------------
// Síntese procedural: o som de cada efeito sai do processo físico que o
// produz, não de uma gravação (A. Farnell, Designing Sound, MIT Press, 2010;
// as bolhas: K. van den Doel, "Physically based models for liquid sounds",
// ACM TAP 2(4), 2005; M. Minnaert, "On musical air-bubbles and the sounds
// of running water", Phil. Mag. 16, 1933).
void renderFilmFx(const composition::Gesture& g, double sr, std::vector<float>& L, std::vector<float>& R)
{
    using composition::GestureKind;
    Noise nz { g.seed | 1 };
    const auto i0 = static_cast<std::size_t>(g.time * sr);
    const auto len = static_cast<std::size_t>(g.duration * sr);
    const auto uni = [&] { return 0.5 * (nz.next() + 1.0); };
    const double lv = g.level;
    switch (g.kind)
    {
        case GestureKind::wind:
        {
            // ruído numa banda que as rajadas movem (um passeio aleatório lento)
            Band bL, bR, whistle;
            double gust = 0.5, gv = 0.0, centre = 600.0;
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr;
                if (i % 64 == 0)
                {
                    gv = 0.995 * gv + 0.004 * nz.next();
                    gust = std::clamp(gust + gv * 0.05, 0.15, 1.0);
                    centre = 300.0 + 900.0 * gust;
                }
                const double env = gestureEnv(t, g.duration);
                const double a = nz.next(), b = nz.next();
                double l = bL.run(a, centre, 1.5, sr), r = bR.run(b, centre * 1.07, 1.5, sr);
                const double w = 0.25 * whistle.run(0.5 * (a + b), 2.2 * centre, 25.0, sr) * gust * gust;
                const double amp = 0.6 * lv * env * (0.3 + 0.7 * gust);
                if (i0 + i < L.size())
                {
                    L[i0 + i] += static_cast<float>(amp * (l + w));
                    R[i0 + i] += static_cast<float>(amp * (r + w));
                }
            }
            break;
        }
        case GestureKind::rain:
        {
            // o chiado da chuva e as gotas: cada gota é um tom curto e agudo que
            // cai num lugar do estéreo
            Band hissL, hissR;
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr;
                const double env = gestureEnv(t, g.duration);
                if (i0 + i < L.size())
                {
                    L[i0 + i] += static_cast<float>(0.12 * lv * env * hissL.run(nz.next(), 5000.0, 0.7, sr));
                    R[i0 + i] += static_cast<float>(0.12 * lv * env * hissR.run(nz.next(), 5200.0, 0.7, sr));
                }
            }
            const double rate = std::max(50.0, static_cast<double>(g.density));
            for (double t = 0.0; t < g.duration; t += -std::log(std::max(1e-6, uni())) / rate)
            {
                const double f = 2000.0 + 4000.0 * uni(), dur = 0.002 + 0.006 * uni();
                const Pan p(nz.next());
                const double amp = 0.25 * lv * gestureEnv(t, g.duration) * (0.3 + 0.7 * uni());
                const auto s0 = i0 + static_cast<std::size_t>(t * sr);
                for (std::size_t k = 0; k < static_cast<std::size_t>(dur * 4.0 * sr); ++k)
                {
                    const double tk = static_cast<double>(k) / sr;
                    put(L, R, s0 + k, amp * std::exp(-tk / dur) * std::sin(2.0 * kPi * f * tk), p);
                }
            }
            break;
        }
        case GestureKind::thunder:
        {
            // o estalo (ruído largo e curto) e o ronco: ruído grave com
            // solavancos lentos que decaem por segundos
            Band crackL, crackR, rumbleL, rumbleR;
            double bump = 1.0;
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr;
                if (i % 512 == 0)
                    bump = std::clamp(bump + 0.15 * nz.next(), 0.3, 1.6);
                const double crack = std::exp(-t / 0.08) * (t < 0.003 ? t / 0.003 : 1.0);
                const double rumble = std::min(1.0, t / 0.3) * std::exp(-t / (0.35 * g.duration)) * bump;
                const double cl = crackL.run(nz.next(), 2500.0, 0.5, sr), cr = crackR.run(nz.next(), 2700.0, 0.5, sr);
                const double rl = rumbleL.run(nz.next(), 70.0, 0.8, sr), rr = rumbleR.run(nz.next(), 80.0, 0.8, sr);
                if (i0 + i < L.size())
                {
                    L[i0 + i] += static_cast<float>(lv * (0.5 * crack * cl + 1.6 * rumble * rl));
                    R[i0 + i] += static_cast<float>(lv * (0.5 * crack * cr + 1.6 * rumble * rr));
                }
            }
            break;
        }
        case GestureKind::water:
        {
            // o riacho: bolhas que nascem e sobem de tom (a frequência de
            // Minnaert cresce enquanto a bolha encolhe na superfície)
            Band hiss;
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr;
                put(L, R, i0 + i, 0.05 * lv * gestureEnv(t, g.duration) * hiss.run(nz.next(), 1500.0, 0.6, sr), Pan(0.0));
            }
            const double rate = std::max(20.0, static_cast<double>(g.density));
            for (double t = 0.0; t < g.duration; t += -std::log(std::max(1e-6, uni())) / rate)
            {
                const double f = 500.0 + 2500.0 * uni() * uni(), dur = 0.01 + 0.03 * uni(), rise = 1.0 + 1.5 * uni();
                const Pan p(0.8 * nz.next());
                const double amp = 0.2 * lv * gestureEnv(t, g.duration) * (0.3 + 0.7 * uni());
                const auto s0 = i0 + static_cast<std::size_t>(t * sr);
                double ph = 0.0;
                for (std::size_t k = 0; k < static_cast<std::size_t>(dur * 5.0 * sr); ++k)
                {
                    const double tk = static_cast<double>(k) / sr;
                    ph += 2.0 * kPi * f * (1.0 + rise * tk / dur * 0.2) / sr;
                    put(L, R, s0 + k, amp * std::exp(-tk / dur) * std::sin(ph), p);
                }
            }
            break;
        }
        case GestureKind::whoosh:
        {
            // algo passa: a banda de ruído sobe e desce (o efeito Doppler), a
            // força em sino, da esquerda para a direita
            Band b;
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr, x = t / g.duration;
                const double bell = std::exp(-std::pow((x - 0.6) / 0.22, 2.0));
                const double centre = 400.0 + 3000.0 * std::exp(-std::pow((x - 0.55) / 0.25, 2.0));
                put(L, R, i0 + i, 0.9 * lv * bell * b.run(nz.next(), centre, 1.2, sr), Pan(lerp(g.pan, g.panEnd, x)));
            }
            break;
        }
        case GestureKind::riser:
        {
            // a subida: um feixe de dentes-de-serra que sobe uma oitava e meia
            // e o ruído que abre, em crescendo, até cortar seco no golpe
            std::array<double, 5> ph {};
            Band nb;
            const double f0 = midiHz(g.lo);
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr, x = t / g.duration;
                const double f = f0 * std::pow(2.0, 1.5 * x * x);
                const double amp = 0.35 * lv * x * x * std::min(1.0, (g.duration - t) / 0.01);
                double s = 0.0;
                for (std::size_t k = 0; k < ph.size(); ++k)
                {
                    ph[k] += f * (1.0 + 0.006 * (static_cast<double>(k) - 2.0)) / sr;
                    ph[k] -= std::floor(ph[k]);
                    s += (2.0 * ph[k] - 1.0) * 0.2;
                }
                const double ns = nb.run(nz.next(), 500.0 + 6000.0 * x, 0.8, sr);
                put(L, R, i0 + i, amp * (0.6 * s + 0.8 * ns), Pan(0.0));
            }
            break;
        }
        case GestureKind::impact:
        {
            // o golpe: o boom grave que cai de altura (55 → 32 Hz), o estalo e
            // a cauda
            Band crack;
            double ph = 0.0;
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr;
                const double f = 32.0 + 23.0 * std::exp(-t / 0.15);
                ph += 2.0 * kPi * f / sr;
                const double boom = std::sin(ph) * std::exp(-t / 0.7) * std::min(1.0, t / 0.002);
                const double hit = crack.run(nz.next(), 1800.0, 0.6, sr) * std::exp(-t / 0.05);
                put(L, R, i0 + i, lv * (1.2 * boom + 0.6 * hit), Pan(0.0));
            }
            break;
        }
        case GestureKind::braam:
        {
            // o metal grave das trilhas: muitas vozes de "metal" (o brilho
            // cresce com a força, como no trompete) numa quinta grave,
            // desafinadas entre si, que incham e se abrem
            const std::array<double, 2> roots { midiHz(g.lo), midiHz(g.lo + 7.0f) };
            std::vector<double> ph(2 * 4 * 24, 0.0);
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr, x = t / g.duration;
                const double E = std::min(1.0, t / 0.25) * (x < 0.7 ? 1.0 : std::max(0.0, 1.0 - (x - 0.7) / 0.3));
                double s = 0.0;
                std::size_t idx = 0;
                for (double r : roots)
                    for (int v = 0; v < 4; ++v)
                    {
                        const double det = 1.0 + 0.004 * (v - 1.5);
                        for (int k = 1; k <= 24; ++k, ++idx)
                        {
                            if (r * k > sr * 0.45)
                                continue;
                            ph[idx] += 2.0 * kPi * r * k * det / sr;
                            s += std::pow(E, 1.0 + 0.35 * (k - 1)) * std::sin(ph[idx]) / std::pow(static_cast<double>(k), 0.5);
                        }
                    }
                put(L, R, i0 + i, 0.06 * lv * s, Pan(0.0));
            }
            break;
        }
        case GestureKind::fire:
        {
            // o fogo: estalos em rajadas (a madeira que racha), o chiado que
            // tremula e o ronco grave da chama
            Band hissB, roarB;
            double flicker = 0.5;
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr;
                if (i % 128 == 0)
                    flicker = std::clamp(flicker + 0.08 * nz.next(), 0.2, 1.0);
                const double env = gestureEnv(t, g.duration);
                const double hiss = 0.05 * flicker * hissB.run(nz.next(), 2200.0, 0.8, sr);
                const double roar = 0.5 * (0.5 + 0.5 * flicker) * roarB.run(nz.next(), 90.0, 0.7, sr);
                put(L, R, i0 + i, lv * env * (hiss + roar), Pan(0.0));
            }
            double t = 0.0;
            while (t < g.duration)
            {
                // rajadas: às vezes vários estalos juntos
                const int burst = uni() < 0.25 ? 3 + static_cast<int>(4.0 * uni()) : 1;
                for (int b = 0; b < burst; ++b)
                {
                    const double tb = t + 0.004 * b + 0.01 * uni();
                    const double f = 1500.0 + 4500.0 * uni(), dur = 0.001 + 0.004 * uni();
                    const Pan p(0.6 * nz.next());
                    Band cb;
                    const double amp = 0.6 * lv * gestureEnv(tb, g.duration) * (0.3 + 0.7 * uni());
                    const auto s0 = i0 + static_cast<std::size_t>(tb * sr);
                    for (std::size_t k = 0; k < static_cast<std::size_t>(dur * 5.0 * sr); ++k)
                        put(L, R, s0 + k, amp * std::exp(-static_cast<double>(k) / (dur * sr)) * cb.run(nz.next(), f, 2.0, sr), p);
                }
                t += -std::log(std::max(1e-6, uni())) / 25.0;
            }
            break;
        }
        case GestureKind::crowd:
        {
            // a multidão: muitas vozes ao longe, cada uma um zumbido glotal
            // com a sua altura, que muda de vogal a cada sílaba (~150 ms) —
            // o murmúrio sem palavras (o "babble")
            constexpr int voices = 14;
            struct V { double f0, ph, syl, next; Band f1, f2; double a1, a2, amp, pan; };
            std::vector<V> vs(voices);
            for (auto& v : vs)
            {
                v.f0 = 95.0 + 160.0 * uni();
                v.ph = uni();
                v.syl = 0.0;
                v.next = 0.08 + 0.15 * uni();
                v.a1 = 400.0 + 500.0 * uni();
                v.a2 = 1000.0 + 1300.0 * uni();
                v.amp = 0.0;
                v.pan = nz.next();
            }
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr;
                const double env = gestureEnv(t, g.duration);
                double l = 0.0, r = 0.0;
                for (auto& v : vs)
                {
                    v.syl += 1.0 / sr;
                    if (v.syl >= v.next)  // a sílaba seguinte: outra vogal, às vezes uma pausa
                    {
                        v.syl = 0.0;
                        v.next = 0.08 + 0.15 * uni();
                        v.a1 = 300.0 + 600.0 * uni();
                        v.a2 = 900.0 + 1500.0 * uni();
                        v.amp = uni() < 0.2 ? 0.0 : 0.4 + 0.6 * uni();
                        v.f0 *= 1.0 + 0.04 * nz.next();
                    }
                    v.ph += v.f0 / sr;
                    double x = 0.0;
                    if (v.ph >= 1.0)
                    {
                        v.ph -= 1.0;
                        x = 1.0;
                    }
                    x += 0.02 * nz.next();
                    const double sylEnv = std::sin(kPi * std::min(1.0, v.syl / v.next));
                    const double y = v.amp * sylEnv * (v.f1.run(x, v.a1, 6.0, sr) + 0.6 * v.f2.run(x, v.a2, 8.0, sr));
                    l += y * (0.5 - 0.5 * v.pan);
                    r += y * (0.5 + 0.5 * v.pan);
                }
                if (i0 + i < L.size())
                {
                    L[i0 + i] += static_cast<float>(0.12 * lv * env * l);
                    R[i0 + i] += static_cast<float>(0.12 * lv * env * r);
                }
            }
            break;
        }
        case GestureKind::city:
        {
            // a cidade: o rumor grave do tráfego, carros que passam (a banda de
            // ruído que sobe e desce com o Doppler, de um lado ao outro) e,
            // raramente, uma buzina ao longe
            Band rumbleL, rumbleR;
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr;
                const double env = gestureEnv(t, g.duration);
                if (i0 + i < L.size())
                {
                    L[i0 + i] += static_cast<float>(0.35 * lv * env * rumbleL.run(nz.next(), 75.0, 0.6, sr));
                    R[i0 + i] += static_cast<float>(0.35 * lv * env * rumbleR.run(nz.next(), 82.0, 0.6, sr));
                }
            }
            for (double t = uni() * 2.0; t < g.duration; t += 2.0 + 4.0 * uni())
            {
                const double d = 3.0 + 3.0 * uni();
                const bool lr = uni() < 0.5;
                Band car;
                const auto s0 = i0 + static_cast<std::size_t>(t * sr);
                for (std::size_t k = 0; k < static_cast<std::size_t>(d * sr); ++k)
                {
                    const double x = static_cast<double>(k) / (d * sr);
                    const double bell = std::exp(-std::pow((x - 0.5) / 0.18, 2.0));
                    const double f = 180.0 * (x < 0.5 ? 1.12 : 0.9);  // o Doppler: mais agudo chegando
                    put(L, R, s0 + k, 0.5 * lv * gestureEnv(t, g.duration) * bell * car.run(nz.next(), f + 300.0 * bell, 1.0, sr),
                        Pan(lr ? -0.9 + 1.8 * x : 0.9 - 1.8 * x));
                }
            }
            if (g.duration > 8.0 && uni() < 0.7)
            {
                const double t = g.duration * (0.3 + 0.4 * uni());
                const auto s0 = i0 + static_cast<std::size_t>(t * sr);
                double p1 = 0.0, p2 = 0.0;
                for (std::size_t k = 0; k < static_cast<std::size_t>(0.35 * sr); ++k)
                {
                    p1 += 410.0 / sr;
                    p2 += 520.0 / sr;
                    const double saw = (2.0 * (p1 - std::floor(p1)) - 1.0) + (2.0 * (p2 - std::floor(p2)) - 1.0);
                    const double e = std::min(1.0, static_cast<double>(k) / (0.01 * sr)) * std::min(1.0, (0.35 * sr - k) / (0.02 * sr));
                    put(L, R, s0 + k, 0.04 * lv * e * saw, Pan(0.5));
                }
            }
            break;
        }
        default: break;
    }
}

void renderGesture(const composition::Gesture& g, double sr, std::vector<float>& L, std::vector<float>& R)
{
    using composition::GestureKind;
    Noise nz { g.seed | 1 };
    const auto i0 = static_cast<std::size_t>(g.time * sr);
    const auto len = static_cast<std::size_t>(g.duration * sr);
    const auto uni = [&] { return 0.5 * (nz.next() + 1.0); };
    switch (g.kind)
    {
        case GestureKind::wind:
        case GestureKind::rain:
        case GestureKind::thunder:
        case GestureKind::water:
        case GestureKind::whoosh:
        case GestureKind::riser:
        case GestureKind::impact:
        case GestureKind::braam:
        case GestureKind::fire:
        case GestureKind::crowd:
        case GestureKind::city:
            return renderFilmFx(g, sr, L, R);
        case GestureKind::cloud:
        {
            double t = 0.0;
            while (t < g.duration)
            {
                const double x = t / g.duration;
                const double dens = std::max(0.5, lerp(g.density, g.densityEnd, x));
                const double pitch = lerp(lerp(g.lo, g.loEnd, x), lerp(g.hi, g.hiEnd, x), uni());
                const double gd = 0.02 + 0.07 * uni();
                const bool noisy = uni() < 0.3;
                const Pan pan(lerp(g.pan, g.panEnd, x) + 0.3 * nz.next());
                const double f = midiHz(pitch), amp = 0.4 * g.level * gestureEnv(t, g.duration);
                Band band;
                const auto gs = i0 + static_cast<std::size_t>(t * sr), gl = static_cast<std::size_t>(gd * sr);
                for (std::size_t i = 0; i < gl; ++i)
                {
                    const double w = 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(i) / static_cast<double>(gl));
                    const double s = noisy ? 3.0 * band.run(nz.next(), f, 8.0, sr) : std::sin(2.0 * kPi * f * static_cast<double>(i) / sr);
                    put(L, R, gs + i, amp * w * s, pan);
                }
                t += -std::log(1.0 - 0.999 * uni()) / dens;  // tempos de Poisson
            }
            break;
        }
        case GestureKind::glissMass:
        {
            const int voices = std::max(1, static_cast<int>(g.density));
            for (int v = 0; v < voices; ++v)
            {
                const double p0 = lerp(g.lo, g.hi, uni()), p1 = lerp(g.loEnd, g.hiEnd, uni());
                const double curve = 0.5 + 1.5 * uni();
                const Pan pan(lerp(g.pan, g.panEnd, uni()));
                const double amp = 0.5 * g.level / std::sqrt(static_cast<double>(voices));
                double ph = uni();
                for (std::size_t i = 0; i < len; ++i)
                {
                    const double t = static_cast<double>(i) / sr, x = t / g.duration;
                    ph += midiHz(lerp(p0, p1, std::pow(x, curve))) / sr;
                    put(L, R, i0 + i, amp * gestureEnv(t, g.duration) * (std::sin(2.0 * kPi * ph) + 0.2 * std::sin(4.0 * kPi * ph)), pan);
                }
            }
            break;
        }
        case GestureKind::noiseBand:
        {
            Band band;
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr, x = t / g.duration;
                const double lo = lerp(g.lo, g.loEnd, x), hi = std::max(lo + 0.5, lerp(g.hi, g.hiEnd, x));
                const double fc = midiHz(0.5 * (lo + hi));
                const double q = std::clamp(1.0 / (std::pow(2.0, (hi - lo) / 24.0) - std::pow(2.0, -(hi - lo) / 24.0)), 0.5, 30.0);
                const double swell = std::sin(kPi * x);
                put(L, R, i0 + i, 2.0 * g.level * swell * band.run(nz.next(), fc, q, sr), Pan(lerp(g.pan, g.panEnd, x)));
            }
            break;
        }
        case GestureKind::cluster:
        {
            const int voices = std::max(2, static_cast<int>(g.density));
            for (int v = 0; v < voices; ++v)
            {
                const double pos = uni();
                const Pan pan(lerp(-0.7, 0.7, uni()));
                const double amp = 0.5 * g.level / std::sqrt(static_cast<double>(voices));
                double ph = uni();
                for (std::size_t i = 0; i < len; ++i)
                {
                    const double t = static_cast<double>(i) / sr, x = t / g.duration;
                    ph += midiHz(lerp(lerp(g.lo, g.loEnd, x), lerp(g.hi, g.hiEnd, x), pos)) / sr;
                    const double env = std::min({ 1.0, t / 1.0, (g.duration - t) / 1.0 });
                    put(L, R, i0 + i, amp * std::max(0.0, env) * std::sin(2.0 * kPi * ph), pan);
                }
            }
            break;
        }
        case GestureKind::iteration:
        {
            double t = 0.0;
            while (t < g.duration)
            {
                const double x = t / g.duration;
                const double rate = std::max(0.5, lerp(g.density, g.densityEnd, x));
                const double f = midiHz(lerp(g.lo, g.hi, uni()));
                const Pan pan(lerp(g.pan, g.panEnd, x));
                const auto s0 = i0 + static_cast<std::size_t>(t * sr);
                const auto sl = static_cast<std::size_t>(0.2 * sr);
                for (std::size_t i = 0; i < sl; ++i)
                {
                    const double tt = static_cast<double>(i) / sr;
                    put(L, R, s0 + i, 0.4 * g.level * std::exp(-tt / 0.04) * std::sin(2.0 * kPi * f * tt), pan);
                }
                t += 1.0 / rate;
            }
            break;
        }
        case GestureKind::drone:
        {
            const double f0 = midiHz(g.lo);
            std::array<double, 6> ph {};
            const double fade = std::min(2.0, g.duration / 3.0);
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr, x = t / g.duration;
                const double roll = lerp(2.0, 0.8, x);  // abre o brilho ao longo do drone
                double s = 0.0;
                for (std::size_t k = 0; k < 6; ++k)
                {
                    ph[k] += f0 * static_cast<double>(k + 1) * (1.0 + 0.0015 * std::sin(0.3 * t + static_cast<double>(k))) / sr;
                    s += std::sin(2.0 * kPi * ph[k]) / std::pow(static_cast<double>(k + 1), roll);
                }
                const double env = std::min({ 1.0, t / fade, (g.duration - t) / fade });
                put(L, R, i0 + i, 0.4 * g.level * std::max(0.0, env) * s, Pan(lerp(g.pan, g.panEnd, x) * 0.3));
            }
            break;
        }
        case GestureKind::freeze:
        {
            const double trem = 4.0 + 3.0 * uni();
            const double amp = 0.35 * g.level / std::sqrt(static_cast<double>(std::max<std::size_t>(1, g.pitches.size())));
            for (std::size_t v = 0; v < g.pitches.size(); ++v)
            {
                const double f0 = midiHz(g.pitches[v]);
                const Pan pan(lerp(-0.6, 0.6, g.pitches.size() > 1 ? static_cast<double>(v) / static_cast<double>(g.pitches.size() - 1) : 0.5));
                std::array<double, 3> ph {};
                for (std::size_t i = 0; i < len; ++i)
                {
                    const double t = static_cast<double>(i) / sr;
                    double s = 0.0;
                    for (std::size_t k = 0; k < 3; ++k)
                    {
                        ph[k] += f0 * static_cast<double>(k + 1) / sr;
                        s += std::sin(2.0 * kPi * ph[k]) / static_cast<double>(k + 1);
                    }
                    const double env = std::min({ 1.0, t / 0.5, (g.duration - t) / 0.5 }) * (1.0 - 0.3 * (0.5 + 0.5 * std::sin(2.0 * kPi * trem * t)));
                    put(L, R, i0 + i, amp * std::max(0.0, env) * s, pan);
                }
            }
            break;
        }
        case GestureKind::body:
        {
            // o corpo da referência sustentado: cada parcial medido sobre a
            // fundamental, com uma respiração lenta e uma fase próprias, e
            // espalhado no estéreo do grave ao agudo
            if (g.ratios.empty() || g.pitches.empty())
                break;
            const double f0 = midiHz(g.pitches.front());
            const std::size_t k = g.ratios.size();
            const double amp = 0.55 * g.level;
            std::vector<double> ph(k, 0.0), rate(k), off(k);
            for (std::size_t j = 0; j < k; ++j)
            {
                rate[j] = 0.05 + 0.25 * uni();
                off[j] = 2.0 * kPi * uni();
            }
            for (std::size_t i = 0; i < len; ++i)
            {
                const double t = static_cast<double>(i) / sr;
                const double env = std::max(0.0, std::min({ 1.0, t / 0.8, (g.duration - t) / 0.8 }));
                double l = 0.0, r = 0.0;
                for (std::size_t j = 0; j < k; ++j)
                {
                    const double f = f0 * g.ratios[j];
                    if (f > 0.45 * sr)
                        continue;
                    ph[j] += f / sr;
                    const double breath = 0.55 + 0.45 * std::sin(2.0 * kPi * rate[j] * t + off[j]);
                    const double v = g.weights[j] * breath * std::sin(2.0 * kPi * ph[j]);
                    const double pos = k > 1 ? static_cast<double>(j) / static_cast<double>(k - 1) : 0.5;
                    l += v * (1.0 - 0.6 * pos);
                    r += v * (0.4 + 0.6 * pos);
                }
                if (i0 + i < L.size())
                {
                    L[i0 + i] += static_cast<float>(amp * env * l * 1.8);
                    R[i0 + i] += static_cast<float>(amp * env * r * 1.8);
                }
            }
            break;
        }
    }
}

// sala: rede de 4 linhas de atraso (matriz de Householder), com amortecimento
} // namespace


void renderScore(const composition::Score& score, double sr, std::vector<float>& L, std::vector<float>& R, std::vector<Stem>* stems,
                 const std::function<void(float)>& onProgress)
{
    const ScopedNoDenormals denormals;  // Denormals.h
    static const char* const kStemNames[] = { "1-motivo", "2-baixo", "3-eco", "4-memoria", "5-percussao", "6-mosaico", "7-harmonia",
                                              "8-gestos", "9-sala" };
    const auto total = static_cast<std::size_t>(score.duration * sr + 0.5);
    L.assign(total, 0.0f);
    R.assign(total, 0.0f);
    // a produção (Production.h): um barramento por papel, processado e somado,
    // com o envio para a sala; um de cada vez (a memória fica em ~6 pistas)
    // a sala: a do conjunto quando há um; senão a que a peça escolheu pela
    // referência (HALL → sala de concerto, SPACE → espaço longo, Trioio)
    RoomProfile profile = roomForEnsemble(score.ensemble);
    if (score.ensemble == 0 && score.room == composition::Room::hall)
        profile = roomForEnsemble(8);
    else if (score.ensemble == 0 && score.room == composition::Room::space)
        profile = { "espaço", 4.2, 1.9, 0.04, 0.2, 1.05 };
    std::vector<float> sendL(total, 0.0f), sendR(total, 0.0f);
    // um barramento por voz (0..6) e o dos efeitos (7): o processamento é o do
    // papel (mixGroupOf), o lugar é o do palco do conjunto (stageFor).
    // A SÍNTESE de cada barramento é independente das outras: roda em lotes
    // paralelos (até 3 threads, núcleos − 1; TRIOIO_THREADS=1 desliga), com no
    // máximo um lote na memória. A MISTURA (mixBus, o envio para a sala) é
    // sempre feita depois, na ordem das vozes: o resultado é o mesmo do
    // sequencial, amostra a amostra.
    int workers = static_cast<int>(std::max(1u, std::thread::hardware_concurrency())) - 1;
    if (const char* env = std::getenv("TRIOIO_THREADS"); env && *env)
        workers = std::atoi(env);
    workers = std::clamp(workers, 1, 3);
    struct Bus
    {
        std::vector<float> l, r;
        bool any = false;
    };
    std::array<double, 8> busSeconds {};
    std::array<int, 8> busNotes {};
    const auto synthBus = [&](int voice, Bus& b) {
        const auto start = std::chrono::steady_clock::now();
        b.l.assign(total, 0.0f);
        b.r.assign(total, 0.0f);
        b.any = false;
        if (voice < 7)
        {
            for (const auto& n : score.notes)
                if (std::clamp(n.voice, 0, 6) == voice)
                {
                    renderNote(n, score, sr, b.l, b.r);
                    b.any = true;
                }
        }
        else
            for (const auto& g : score.gestures)
            {
                renderGesture(g, sr, b.l, b.r);
                b.any = true;
            }
        busSeconds[static_cast<std::size_t>(voice)] = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (voice < 7)
            busNotes[static_cast<std::size_t>(voice)] = static_cast<int>(std::count_if(score.notes.begin(), score.notes.end(),
                [voice](const composition::Note& n) { return std::clamp(n.voice, 0, 6) == voice; }));
    };
    // TRIOIO_TEMPOS=1: o tempo de cada etapa no stderr (a medição do Marco 4)
    const bool timing = std::getenv("TRIOIO_TEMPOS") != nullptr;
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();
    double tSynth = 0.0, tMix = 0.0;
    const auto lap = [&](double& acc) {
        const auto t = Clock::now();
        acc += std::chrono::duration<double>(t - t0).count();
        t0 = t;
    };
    std::vector<Bus> batch(static_cast<std::size_t>(workers));
    for (int first = 0; first <= 7; first += workers)
    {
        const int count = std::min(workers, 8 - first);
        if (count == 1)
            synthBus(first, batch[0]);
        else
        {
            std::vector<std::thread> pool;
            for (int k = 1; k < count; ++k)
                pool.emplace_back([&, k] {
                    const ScopedNoDenormals threadDenormals;  // o MXCSR é por thread
                    synthBus(first + k, batch[static_cast<std::size_t>(k)]);
                });
            synthBus(first, batch[0]);
            for (auto& t : pool)
                t.join();
        }
        lap(tSynth);
        for (int k = 0; k < count; ++k)
        {
            const int voice = first + k;
            auto& busL = batch[static_cast<std::size_t>(k)].l;
            auto& busR = batch[static_cast<std::size_t>(k)].r;
            const bool any = batch[static_cast<std::size_t>(k)].any;
            if (any && !stems)
                mixBus(voice < 7 ? mixGroupOf(voice) : kMixGroups - 1, busL, busR, sr, profile, L, R, sendL, sendR,
                       stageFor(score.ensemble, voice));
            else if (any)
            {
                // o stem: o barramento processado sozinho, depois somado à mistura
                Stem st;
                st.name = kStemNames[voice];
                st.left.assign(total, 0.0f);
                st.right.assign(total, 0.0f);
                mixBus(voice < 7 ? mixGroupOf(voice) : kMixGroups - 1, busL, busR, sr, profile, st.left, st.right, sendL, sendR,
                       stageFor(score.ensemble, voice));
                for (std::size_t i = 0; i < total; ++i)
                {
                    L[i] += st.left[i];
                    R[i] += st.right[i];
                }
                stems->push_back(std::move(st));
            }
        }
        lap(tMix);
        if (onProgress)
            onProgress(static_cast<float>(first + count) / 9.0f);
    }
    batch.clear();
    batch.shrink_to_fit();
    double tRoom = 0.0;
    // a reverberação de envio (as salas HALL/SPACE do Rasgo Modular eram
    // aplicadas sobre a mistura inteira, sem envio por papel; desde
    // 2026-10-16 elas viram perfis desta sala)
    if (stems)
    {
        Stem st;
        st.name = kStemNames[8];
        st.left.assign(total, 0.0f);
        st.right.assign(total, 0.0f);
        reverb(sendL, sendR, sr, profile, st.left, st.right);
        for (std::size_t i = 0; i < total; ++i)
        {
            L[i] += st.left[i];
            R[i] += st.right[i];
        }
        stems->push_back(std::move(st));
    }
    else
        reverb(sendL, sendR, sr, profile, L, R);
    glue(L, R);
    lap(tRoom);
    if (timing)
    {
        std::fprintf(stderr, "tempos do render: síntese %.1f s (%d threads), mistura %.1f s, sala e cola %.1f s\n", tSynth, workers, tMix, tRoom);
        for (int v = 0; v < 8; ++v)
            std::fprintf(stderr, "  barramento %d: %.1f s (%d notas)\n", v, busSeconds[static_cast<std::size_t>(v)], busNotes[static_cast<std::size_t>(v)]);
    }
    float peak = 0.0f;
    for (std::size_t i = 0; i < total; ++i)
        peak = std::max({ peak, std::abs(L[i]), std::abs(R[i]) });
    if (peak > 0.0f)
    {
        const float g = 0.891f / peak;  // −1 dBFS
        for (std::size_t i = 0; i < total; ++i)
        {
            L[i] *= g;
            R[i] *= g;
        }
        if (stems)
            for (auto& st : *stems)
                for (std::size_t i = 0; i < total; ++i)
                {
                    st.left[i] *= g;
                    st.right[i] *= g;
                }
    }
}

} // namespace trioio
