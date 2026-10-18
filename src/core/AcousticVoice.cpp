#include "AcousticVoice.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <mutex>
#include <tuple>
#include <utility>

namespace trioio
{
namespace
{
using composition::Note;
using composition::Timbre;

constexpr double kPi = 3.14159265358979323846;

double midiHz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

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

// linha de atraso com leitura fracionária (linear): o atraso muda a cada
// amostra quando a altura da nota se move
struct Delay
{
    std::vector<double> buf;
    std::size_t w = 0;
    explicit Delay(std::size_t n) : buf(std::max<std::size_t>(n, 4)) {}
    void push(double x)
    {
        buf[w] = x;
        w = (w + 1) % buf.size();
    }
    // o valor escrito `d` amostras atrás (d ≥ 1)
    [[nodiscard]] double read(double d) const
    {
        d = std::clamp(d, 1.0, static_cast<double>(buf.size() - 2));
        const auto i = static_cast<std::size_t>(d);
        const double f = d - static_cast<double>(i);
        const std::size_t n = buf.size();
        const double a = buf[(w + n - i) % n], b = buf[(w + n - i - 1) % n];
        return a + f * (b - a);
    }
};

// ressonância de corpo: passa-banda biquad (RBJ, ganho de pico 1)
struct Reso
{
    double b0 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void set(double hz, double q, double sr)
    {
        const double w0 = 2.0 * kPi * std::clamp(hz, 20.0, sr * 0.45) / sr, al = std::sin(w0) / (2.0 * q), a0 = 1.0 + al;
        b0 = al / a0;
        b2 = -al / a0;
        a1 = -2.0 * std::cos(w0) / a0;
        a2 = (1.0 - al) / a0;
    }
    double run(double x)
    {
        const double y = b0 * x + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }
};

// a altura em Hz no instante t, com o vibrato natural do instrumento quando a
// nota não traz oscilação própria (o idioma já decidiu o ornamento)
double hzAt(const Note& n, double t, double vibDepth, double vibRate, double vibDelay)
{
    double p = n.pitchAt(std::min(t, n.duration));
    if (n.wobbleDepth <= 0.0f && vibDepth > 0.0 && t > vibDelay)
    {
        const double fadeIn = std::min(1.0, (t - vibDelay) / 0.4);
        p += vibDepth * fadeIn * std::sin(2.0 * kPi * vibRate * (t - vibDelay));
    }
    return midiHz(p);
}

// ---- arco: corda friccionada -------------------------------------------------
// Duas linhas (do arco ao cavalete e do arco à pestana); no ponto do arco, a
// diferença de velocidade passa pela curva de atrito (prende e solta: o
// movimento de Helmholtz). O cavalete leva ao corpo: modos de violino
// (A0 ~ 275 Hz, B1− ~ 460, B1+ ~ 550, a "colina do cavalete" ~ 2,5 kHz),
// abaixados para viola e violoncelo pelo registro.
std::vector<float> bowed(const Note& n, double sr, std::uint64_t seed)
{
    const double f0 = midiHz(n.pitch);
    const double scale = f0 < 120.0 ? 0.42 : f0 < 190.0 ? 0.75 : 1.0;  // violoncelo, viola, violino
    const double release = 0.35;
    const auto len = static_cast<std::size_t>((n.duration + release + 0.25) * sr);
    std::vector<float> out(len);
    const auto maxD = static_cast<std::size_t>(sr / 25.0) + 4;
    Delay neck(maxD), bridge(maxD);
    const double beta = 0.127;  // o arco perto do cavalete
    double lp = 0.0;
    std::array<Reso, 5> body;
    const std::array<double, 5> bf { 275.0, 460.0, 550.0, 700.0, 2500.0 }, bq { 8.0, 10.0, 10.0, 6.0, 1.2 },
        bg { 0.9, 0.6, 0.8, 0.4, 0.7 };
    for (std::size_t k = 0; k < body.size(); ++k)
        body[k].set(bf[k] * (k == 4 ? std::sqrt(scale) : scale), bq[k], sr);
    Noise nz { seed | 1 };
    const double vMax = 0.22 + 0.25 * n.velocity;
    const double pressure = 0.25 + 0.35 * n.velocity;
    const double slope = 5.0 - 4.0 * pressure;
    double hp = 0.0, hpIn = 0.0;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = hzAt(n, t, 0.12, 5.6, 0.22);
        const double P = sr / hz;
        // o arco: entra em ~80 ms, sai no fim da nota
        const double env = t < n.duration ? std::min(1.0, t / 0.08) : std::max(0.0, 1.0 - (t - n.duration) / 0.06);
        const double vBow = vMax * env * (1.0 + 0.01 * nz.next());
        const double fromBridge = bridge.read(beta * P - 0.5);
        const double fromNeck = neck.read((1.0 - beta) * P - 0.5);
        lp = 0.6 * lp + 0.4 * fromBridge;  // a perda no cavalete
        const double bridgeRefl = -0.995 * lp;
        const double nutRefl = -fromNeck;
        const double vString = bridgeRefl + nutRefl;
        const double dv = vBow - vString;
        double f = std::pow(std::abs(dv * slope + 0.001) + 0.75, -4.0);
        f = std::clamp(f, 0.01, 0.98);
        const double newV = env > 0.0 ? dv * f : 0.0;
        neck.push(bridgeRefl + newV);
        bridge.push(nutRefl + newV);
        // o corpo
        double y = 0.25 * fromBridge;
        for (std::size_t k = 0; k < body.size(); ++k)
            y += bg[k] * body[k].run(fromBridge);
        // o grave sem fundo (corrente contínua) fora
        hp = 0.995 * (hp + y - hpIn);
        hpIn = y;
        out[i] = static_cast<float>(hp);
    }
    return out;
}

// ---- palheta: clarinete -------------------------------------------------------
// O tubo cilíndrico fechado na palheta e aberto na campana: a ida e volta é
// meio período (o reflexo invertido na ponta aberta dobra o período), e só os
// harmônicos ímpares ficam fortes no grave. A palheta é uma válvula: quanto
// maior a diferença de pressão, mais ela fecha.
std::vector<float> reed(const Note& n, double sr, std::uint64_t seed)
{
    const double release = 0.25;
    const auto len = static_cast<std::size_t>((n.duration + release + 0.2) * sr);
    std::vector<float> out(len);
    Delay bore(static_cast<std::size_t>(sr / 40.0) + 4);
    Noise nz { seed | 1 };
    double lp = 0.0, prev = 0.0, last = 0.0;
    const double pMax = 0.62 + 0.28 * n.velocity;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = hzAt(n, t, 0.05, 5.0, 0.45);
        const double env = t < n.duration ? std::min(1.0, t / 0.03) : std::max(0.0, 1.0 - (t - n.duration) / 0.05);
        const double breath = pMax * env * (1.0 + 0.06 * nz.next());
        const double fromBell = bore.read(0.5 * sr / hz - 1.0);
        lp = 0.5 * lp + 0.5 * fromBell;  // a perda da campana e dos furos
        const double back = -0.95 * lp;
        const double dp = back - breath;
        const double r = std::clamp(0.7 - 0.3 * dp, -1.0, 1.0);  // a palheta
        bore.push(breath + dp * r);
        // o som irradiado: a derivada do que sai pela campana
        const double y = fromBell - prev;
        prev = fromBell;
        last = 0.7 * last + 0.3 * y;
        out[i] = static_cast<float>(last + 0.25 * y);
    }
    return out;
}

// ---- marimba e vibrafone --------------------------------------------------------
// Barras com o arco cavado embaixo: os modos afinados 1 : 4 : 10 (marimba) ou
// 1 : 4 : 9,2 (vibrafone, a terceira menos afinada). A baqueta macia toca o
// fundamental e apaga os agudos (o tempo de contato filtra); o tubo ressonador
// reforça e alonga o fundamental. Notas longas (> 1,1 s): vibrafone, com o
// motor (trêmolo) e o abafador no fim da nota.
std::vector<float> bars(const Note& n, double sr, std::uint64_t seed)
{
    const bool vibes = n.duration > 1.1;
    const double f0 = midiHz(n.pitch);
    const std::array<double, 4> ratio = vibes ? std::array<double, 4> { 1.0, 4.0, 9.2, 16.1 } : std::array<double, 4> { 1.0, 3.99, 10.6, 18.2 };
    // o tempo de contato da baqueta: macia no piano, mais dura no forte, e
    // mais curto no agudo (baquetas mais duras e barras mais rígidas)
    const double contact = (2.6 - 1.6 * n.velocity) * 1e-3 * std::sqrt(std::min(1.0, 300.0 / f0));
    // tempo de decaimento (s para −60 dB): o grave soa mais; os modos agudos menos
    const double t60base = vibes ? std::clamp(9.0 * std::pow(261.6 / f0, 0.6), 2.0, 12.0) : std::clamp(2.2 * std::pow(261.6 / f0, 0.8), 0.35, 4.0);
    const double ring = vibes ? std::min(t60base, n.duration + 0.15) : t60base;
    const auto len = static_cast<std::size_t>((ring + 0.3) * sr);
    std::vector<float> out(len);
    Noise nz { seed | 1 };
    for (std::size_t k = 0; k < ratio.size(); ++k)
    {
        const double f = f0 * ratio[k];
        if (f > sr * 0.45)
            break;
        // o contato filtra os parciais acima do fundamental (o fundamental
        // sempre soa: a baqueta é escolhida para a barra)
        const double x = kPi * (f - f0) * contact;
        double a = std::exp(-x * x * 0.5) / (1.0 + 0.6 * static_cast<double>(k));
        if (k == 0)
            a *= 1.6;  // o tubo ressonador
        const double t60 = t60base / (1.0 + 1.8 * static_cast<double>(k) * (vibes ? 0.6 : 1.0));
        const double decay = std::log(1000.0) / t60;
        const double ph = 0.3 * static_cast<double>(k);
        double phase = ph;
        for (std::size_t i = 0; i < len; ++i)
        {
            const double t = static_cast<double>(i) / sr;
            const double hz = midiHz(n.pitchAt(std::min(t, n.duration))) * ratio[k];
            phase += 2.0 * kPi * hz / sr;
            double e = std::exp(-decay * t);
            if (vibes && t > n.duration)
                e *= std::exp(-(t - n.duration) * 30.0);  // o abafador
            out[i] += static_cast<float>(a * e * std::sin(phase));
        }
    }
    // o motor do vibrafone: trêmolo de amplitude ~5 Hz
    if (vibes)
        for (std::size_t i = 0; i < len; ++i)
            out[i] *= static_cast<float>(1.0 - 0.28 * (0.5 + 0.5 * std::sin(2.0 * kPi * 5.2 * static_cast<double>(i) / sr)));
    // o toque da baqueta: um estalo curto e escuro, mais presente no forte
    double lp = 0.0;
    const auto click = static_cast<std::size_t>(0.006 * sr);
    for (std::size_t i = 0; i < std::min(click, len); ++i)
    {
        lp = 0.8 * lp + 0.2 * nz.next();
        out[i] += static_cast<float>(0.25 * n.velocity * lp * (1.0 - static_cast<double>(i) / static_cast<double>(click)));
    }
    return out;
}

// ---- piano ----------------------------------------------------------------------
// Três cordas por nota (duas no grave), levemente desafinadas: os batimentos e
// o decaimento em dois estágios (o som "pronto", que cai depressa, e o "pós-
// som", que fica). A rigidez da corda abre os parciais: fₙ = n·f₀·√(1 + B·n²),
// com B maior no agudo. O martelo bate a 1/8 da corda (some o 8º parcial) e o
// feltro filtra mais no piano que no forte. O abafador para a nota no fim.
std::vector<float> piano(const Note& n, double sr, std::uint64_t seed)
{
    const double f0 = midiHz(n.pitch);
    const double B = std::clamp(1.2e-4 * std::exp((n.pitch - 48.0) * 0.055), 4e-5, 0.02);
    const double fc = 700.0 + 5200.0 * n.velocity * n.velocity;  // o feltro
    const double tLong = std::clamp(14.0 * std::pow(65.4 / f0, 0.55), 0.8, 18.0);  // pós-som do fundamental
    const bool damper = n.pitch < 89.0f;
    const double ring = damper ? std::min(tLong, n.duration + 0.2) : std::min(tLong, 3.0);
    const auto len = static_cast<std::size_t>((ring + 0.25) * sr);
    std::vector<float> out(len);
    Noise nz { seed | 1 };
    const int strings = n.pitch < 40.0f ? 2 : 3;
    const std::array<double, 3> detune { 0.0, 0.0011, -0.0008 };  // ~1,9 e −1,4 cents
    const int partials = std::clamp(static_cast<int>(sr * 0.45 / f0), 1, 40);
    // a curva de altura da nota, uma vez por amostra (não por parcial e por
    // corda); sem curva, a razão é 1 e cada parcial vira um oscilador por
    // rotação (2026-10-18: o piano custava ~0,25 s por nota no jazz, com um
    // pow, dois exp e um sin por amostra × parcial × corda)
    const bool bent = n.glide != 0.0f || !n.bend.empty() || n.wobbleDepth != 0.0f;
    std::vector<double> ratio;
    if (bent)
    {
        ratio.resize(len);
        for (std::size_t i = 0; i < len; ++i)
        {
            const double t = static_cast<double>(i) / sr;
            ratio[i] = std::pow(2.0, (n.pitchAt(std::min(t, n.duration)) - n.pitch) / 12.0);
        }
    }
    // o abafador: o fator depois do fim da nota, igual para todos os parciais
    const auto damperAt = static_cast<std::size_t>(std::max(0.0, n.duration) * sr) + 1;
    const double damperStep = std::exp(-25.0 / sr);
    for (int k = 1; k <= partials; ++k)
    {
        const double fk = static_cast<double>(k) * f0 * std::sqrt(1.0 + B * k * k);
        if (fk > sr * 0.45)
            break;
        double a = std::abs(std::sin(static_cast<double>(k) * kPi / 8.0)) / std::pow(static_cast<double>(k), 0.9);
        a /= 1.0 + std::pow(fk / fc, 2.0);
        if (a < 1e-4)
            continue;
        const double t60slow = tLong / (1.0 + fk / 1800.0);
        const double t60fast = std::max(0.08, 0.18 * t60slow);
        const double dSlow = std::log(1000.0) / t60slow, dFast = std::log(1000.0) / t60fast;
        const double rFast = std::exp(-dFast / sr), rSlow = std::exp(-dSlow / sr);
        for (int s = 0; s < strings; ++s)
        {
            const double det = 1.0 + detune[static_cast<std::size_t>(s)] * (1.0 + 0.2 * k);
            const double w = 2.0 * kPi * fk * det / sr;
            const double amp = a / strings;
            double phase = 0.7 * s + 0.11 * k;
            double eF = 0.65, eS = 0.35, damp = 1.0;
            // o oscilador por rotação: (c, z) gira de w por amostra
            double c = std::cos(phase + w), z = std::sin(phase + w);
            const double cw = std::cos(w), sw = std::sin(w);
            for (std::size_t i = 0; i < len; ++i)
            {
                double sn;
                if (bent)
                {
                    phase += w * ratio[i];
                    sn = std::sin(phase);
                }
                else
                {
                    sn = z;
                    const double nc = c * cw - z * sw;
                    z = z * cw + c * sw;
                    c = nc;
                    if ((i & 4095) == 4095)
                    {
                        const double m = 1.0 / std::sqrt(c * c + z * z);  // a rotação não deriva
                        c *= m;
                        z *= m;
                    }
                }
                if (damper && i >= damperAt)
                    damp *= damperStep;
                out[i] += static_cast<float>(amp * (eF + eS) * damp * sn);
                eF *= rFast;
                eS *= rSlow;
            }
        }
    }
    // o golpe do martelo no tampo: um baque curto e grave
    double lp = 0.0;
    const auto thump = static_cast<std::size_t>(0.02 * sr);
    for (std::size_t i = 0; i < std::min(thump, len); ++i)
    {
        lp = 0.95 * lp + 0.05 * nz.next();
        out[i] += static_cast<float>(0.6 * n.velocity * lp * std::exp(-static_cast<double>(i) / (0.004 * sr)));
    }
    // o abafador: quando a tecla sobe, o feltro toca a corda (um sopro curto
    // de ruído escuro no fim da nota)
    if (damper)
    {
        const auto at = static_cast<std::size_t>(n.duration * sr);
        double dl = 0.0;
        for (std::size_t i = at; i < std::min(len, at + static_cast<std::size_t>(0.04 * sr)); ++i)
        {
            dl = 0.9 * dl + 0.1 * nz.next();
            out[i] += static_cast<float>(0.05 * dl * std::exp(-static_cast<double>(i - at) / (0.012 * sr)));
        }
    }
    // o tampo harmônico (2026-10-16): as cordas não soam direto, passam pela
    // madeira, que tem os seus modos (~100 Hz a 2 kHz; o tampo do piano de
    // cauda: N. Giordano, "Simple model of a piano soundboard", JASA 102(2),
    // 1997; H. Suzuki, "Vibration and sound radiation of a piano soundboard",
    // JASA 80(6), 1986). Uma parte do som vem do tampo, que dá o corpo e o
    // calor; o direto guarda o ataque.
    {
        static const std::array<double, 8> bf { 110.0, 185.0, 290.0, 420.0, 610.0, 880.0, 1260.0, 1850.0 };
        static const std::array<double, 8> bq { 4.0, 5.0, 5.0, 6.0, 6.0, 7.0, 7.0, 6.0 };
        static const std::array<double, 8> bg { 0.9, 0.8, 0.75, 0.7, 0.6, 0.5, 0.4, 0.3 };
        std::array<Reso, 8> board;
        for (std::size_t k = 0; k < board.size(); ++k)
            board[k].set(bf[k], bq[k], sr);
        for (auto& v : out)
        {
            double y = 0.0;
            for (std::size_t k = 0; k < board.size(); ++k)
                y += bg[k] * board[k].run(v);
            v = static_cast<float>(0.65 * v + 0.55 * y);
        }
    }
    return out;
}

// ---- cordas dedilhadas: o violão e as suas receitas ------------------------------
// Karplus-Strong estendido: o laço de atraso afinado (leitura fracionária),
// um filtro de perda que deixa os agudos morrerem antes, o lugar do toque (o
// pente que tira os harmônicos com nó naquele ponto) e o tampo. No fim da
// nota, o dedo abafa a corda. Jaffe e Smith 1983; Karjalainen, Välimäki e
// Tolonen 1998.
//
// A mesma corda faz vários instrumentos (docs/MAQUINA_DE_TIMBRES.md §2): a
// receita muda o material (náilon perde os agudos depressa; aço guarda), o
// que toca (polpa do dedo, dedeira, palheta), o lugar do toque, as ordens
// (pares de cordas levemente desafinadas, tocadas quase juntas: o bandolim) e
// o corpo (as três ressonâncias do tampo e da caixa).
struct Pluck
{
    double t60At110 = 4.5, t60Exp = 0.5;    // decaimento (s, em 110 Hz) e a queda com a altura
    double softLo = 0.08, softSpan = 0.17;  // a excitação: escura (dedo) a clara (palheta)
    double position = 0.18;                 // o lugar do toque, em fração da corda
    double keep = 0.55;                     // o brilho que o laço guarda (náilon 0,55; aço ~0,8)
    std::array<double, 3> bf { 102.0, 204.0, 398.0 }, bq { 6.0, 9.0, 8.0 }, bg { 0.7, 0.55, 0.4 };
    double direct = 0.45, bodyGain = 1.4;
    int courses = 1;          // cordas por ordem
    double courseCents = 0.0; // a desafinação do par
    double courseMs = 0.0;    // o atraso entre as duas cordas do par
    double pickClick = 0.0;   // o estalo da palheta no aço
};

std::vector<float> pluckString(const Note& n, double sr, std::uint64_t seed, const Pluck& r)
{
    const double f0 = midiHz(n.pitch);
    const double t60 = std::clamp(r.t60At110 * std::pow(110.0 / f0, r.t60Exp), 0.4, 6.0);
    const double ring = std::min(t60, n.duration + 0.25);
    const auto len = static_cast<std::size_t>((ring + 0.3 + r.courseMs * 1e-3) * sr);
    std::vector<float> out(len);
    std::vector<double> strings(len, 0.0);
    Noise nz { seed | 1 };
    const double P0 = sr / f0;
    const auto exLen = static_cast<std::size_t>(P0);
    const auto pluck = static_cast<std::size_t>(std::max(1.0, r.position * P0));
    const double soft = r.softLo + r.softSpan * n.velocity;
    for (int c = 0; c < r.courses; ++c)
    {
        Delay loop(static_cast<std::size_t>(sr / 30.0) + 4);
        // a excitação desta corda: ruído escurecido pelo que toca
        std::vector<double> ex(exLen + 1);
        double lpEx = 0.0;
        for (auto& x : ex)
        {
            lpEx += soft * (nz.next() - lpEx);
            x = lpEx;
        }
        const double detune = std::pow(2.0, (c == 0 ? 0.0 : r.courseCents) / 1200.0);
        const auto offset = static_cast<std::size_t>(c * r.courseMs * 1e-3 * sr);
        double lpLoop = 0.0;
        for (std::size_t i = offset; i < len; ++i)
        {
            const std::size_t j = i - offset;
            const double t = static_cast<double>(j) / sr;
            const double hz = hzAt(n, t, 0.0, 0.0, 0.0) * detune;
            const double g = std::pow(10.0, -3.0 / (t60 * hz)) * (t > n.duration ? 0.985 : 1.0);
            const double fb = loop.read(sr / hz - 0.5);
            lpLoop = r.keep * fb + (1.0 - r.keep) * lpLoop;
            double in = 0.0;
            if (j < exLen)
                in = ex[j] - (j >= pluck ? ex[j - pluck] : 0.0);
            const double sv = g * lpLoop + in;
            loop.push(sv);
            strings[i] += sv / r.courses;
        }
    }
    std::array<Reso, 3> body;
    for (std::size_t k = 0; k < body.size(); ++k)
        body[k].set(r.bf[k], r.bq[k], sr);
    double hp = 0.0, hpIn = 0.0;
    Reso click;
    click.set(3500.0, 0.8, sr);
    const auto clickLen = static_cast<std::size_t>(0.004 * sr);
    for (std::size_t i = 0; i < len; ++i)
    {
        const double sv = strings[i];
        double y = r.direct * sv;
        for (std::size_t k = 0; k < body.size(); ++k)
            y += r.bodyGain * r.bg[k] * body[k].run(sv);
        if (r.pickClick > 0.0 && i < clickLen)
            y += r.pickClick * n.velocity * click.run(nz.next()) * (1.0 - static_cast<double>(i) / static_cast<double>(clickLen));
        hp = 0.995 * (hp + y - hpIn);
        hpIn = y;
        out[i] = static_cast<float>(hp);
    }
    return out;
}

// As receitas. Os corpos: as frequências do modo de ar (A0) e das primeiras
// placas, de medidas publicadas para o violão (Fletcher e Rossing 1998,
// cap. 9) e estimadas para os outros pelo tamanho da caixa. São valores de
// partida, a afinar pela escuta.
const Pluck& recipe(Timbre t)
{
    static const Pluck violao {};
    // violão de 7 cordas: a caixa um pouco maior, o grave (a baixaria) tocado
    // com dedeira, mais claro e mais perto do cavalete
    static const Pluck seteCordas { 4.8, 0.5, 0.15, 0.25, 0.15, 0.56, { 95.0, 190.0, 380.0 }, { 6.0, 9.0, 8.0 }, { 0.85, 0.55, 0.4 },
                                    0.45, 1.4, 1, 0.0, 0.0, 0.0 };
    // requinto: o violão pequeno (do bolero), a caixa mais aguda
    static const Pluck requinto { 3.2, 0.5, 0.1, 0.2, 0.17, 0.58, { 135.0, 270.0, 520.0 }, { 6.0, 9.0, 8.0 }, { 0.65, 0.55, 0.45 },
                                  0.5, 1.3, 1, 0.0, 0.0, 0.0 };
    // cavaquinho: quatro cordas de aço, caixa pequena, palheta
    static const Pluck cavaquinho { 2.0, 0.5, 0.3, 0.4, 0.13, 0.78, { 260.0, 520.0, 980.0 }, { 7.0, 8.0, 6.0 }, { 0.6, 0.5, 0.45 },
                                    0.6, 1.1, 1, 0.0, 0.0, 0.15 };
    // bandolim: quatro ordens duplas de aço, palheta; o par levemente
    // desafinado dá o brilho trêmulo
    static const Pluck bandolim { 2.4, 0.5, 0.35, 0.4, 0.12, 0.8, { 230.0, 450.0, 820.0 }, { 7.0, 8.0, 6.0 }, { 0.6, 0.5, 0.45 },
                                  0.6, 1.1, 2, 3.0, 4.0, 0.15 };
    // guitarra manouche (o modelo Selmer-Maccaferri): aço, palheta perto do
    // cavalete, a caixa com o ressoador interno que dá a voz anasalada (~1,4 kHz)
    static const Pluck manouche { 3.0, 0.5, 0.4, 0.4, 0.1, 0.82, { 115.0, 230.0, 1400.0 }, { 6.0, 8.0, 2.0 }, { 0.6, 0.4, 0.7 },
                                  0.55, 1.2, 1, 0.0, 0.0, 0.2 };
    switch (t)
    {
        case Timbre::sevenString: return seteCordas;
        case Timbre::requinto: return requinto;
        case Timbre::cavaquinho: return cavaquinho;
        case Timbre::bandolim: return bandolim;
        case Timbre::manouche: return manouche;
        default: return violao;
    }
}

// ---- baixo acústico (contrabaixo em pizzicato) ----------------------------------
// A corda grave puxada pela polpa do dedo: a excitação é um pulso largo e
// macio (meio seno de ~5 ms), não ruído; o laço perde os agudos depressa e o
// grave dura. O corpo grande (~60, 110 e 200 Hz) e o "tum" do dedo que bate
// no espelho. Jaffe e Smith 1983; Fletcher e Rossing 1998, cap. 10.
std::vector<float> upright(const Note& n, double sr, std::uint64_t seed)
{
    const double f0 = midiHz(n.pitch);
    const double t60 = std::clamp(2.6 * std::pow(55.0 / f0, 0.4), 0.6, 3.5);
    const double ring = std::min(t60, n.duration + 0.15);
    const auto len = static_cast<std::size_t>((ring + 0.3) * sr);
    std::vector<float> out(len);
    Delay loop(static_cast<std::size_t>(sr / 20.0) + 4);
    Noise nz { seed | 1 };
    const double P0 = sr / f0;
    const auto pulse = static_cast<std::size_t>(std::min(P0 * 0.45, (0.004 + 0.003 * (1.0 - n.velocity)) * sr));
    std::array<Reso, 3> body;
    const std::array<double, 3> bf { 62.0, 112.0, 205.0 }, bq { 5.0, 6.0, 5.0 }, bg { 0.8, 0.6, 0.35 };
    for (std::size_t k = 0; k < body.size(); ++k)
        body[k].set(bf[k], bq[k], sr);
    double lpLoop = 0.0, hp = 0.0, hpIn = 0.0, thump = 0.0;
    const double bright = 0.35 + 0.25 * n.velocity;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = hzAt(n, t, 0.0, 0.0, 0.0);
        const double g = std::pow(10.0, -3.0 / (t60 * hz)) * (t > n.duration ? 0.97 : 1.0);
        const double fb = loop.read(sr / hz - 0.5);
        lpLoop = bright * fb + (1.0 - bright) * lpLoop;
        const double in = i < pulse ? std::sin(kPi * static_cast<double>(i) / static_cast<double>(pulse)) : 0.0;
        const double sv = g * lpLoop + in;
        loop.push(sv);
        // o dedo no espelho: ruído grave e curto
        if (i < static_cast<std::size_t>(0.03 * sr))
            thump = 0.9 * thump + 0.1 * nz.next() * std::exp(-t / 0.008);
        else
            thump *= 0.9;
        double y = 0.35 * sv + 0.4 * thump * n.velocity;
        for (std::size_t k = 0; k < body.size(); ++k)
            y += 1.3 * bg[k] * body[k].run(sv);
        hp = 0.998 * (hp + y - hpIn);
        hpIn = y;
        out[i] = static_cast<float>(hp);
    }
    return out;
}

// ---- flauta transversal ------------------------------------------------------------
// O jato de ar cruza o bocal e bate na aresta: o jato demora a chegar (a
// linha do jato, ~ meio período) e a aresta é uma não linearidade em S que
// o empurra para dentro ou para fora do tubo. O tubo é aberto nas duas
// pontas (todos os harmônicos). O sopro tem ruído de ar, mais no ataque.
// P. R. Cook, "A meta-wind-instrument physical model", ICMC 1992;
// M.-P. Verge, A. Hirschberg e R. Caussé, "Sound production in recorderlike
// instruments II", JASA 101(5), 1997.
std::vector<float> transverseFlute(const Note& n, double sr, std::uint64_t seed)
{
    const double release = 0.12;
    const auto len = static_cast<std::size_t>((n.duration + release + 0.2) * sr);
    std::vector<float> out(len);
    Delay bore(static_cast<std::size_t>(sr / 100.0) + 4), jet(static_cast<std::size_t>(sr / 200.0) + 4);
    Noise nz { seed | 1 };
    double lp = 0.0, dcIn = 0.0, dc = 0.0, last = 0.0;
    // a pressão fica dentro da região estável medida (0,83–0,98) em toda a dinâmica
    const double pMax = 0.82 + 0.14 * n.velocity;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = hzAt(n, t, 0.07, 4.8, 0.3);
        const double env = t < n.duration ? std::min(1.0, t / 0.045) : std::max(0.0, 1.0 - (t - n.duration) / 0.06);
        const double chiff = std::exp(-t / 0.03);  // o ar do ataque
        const double breath = env * (pMax + (0.035 + 0.12 * chiff) * nz.next());
        const double P = sr / hz;
        // o tubo, com a perda e sem corrente contínua
        const double fromBore = bore.read(P - 1.0);
        lp = 0.7 * fromBore + 0.3 * lp;
        dc = lp - dcIn + 0.995 * dc;
        dcIn = lp;
        const double temp = dc;
        jet.push(breath - 0.5 * temp);
        // o jato leva ~ meio período até a aresta: a varredura de 2026-10-14
        // (0,32 a 0,6 do período × pressão 0,5 a 1,0, 12 notas de Dó 4 a Si 5) deu a
        // região estável em 0,50–0,54 com pressão 0,83–0,98; fora dela a flauta
        // salta de registro
        double x = jet.read(std::max(1.0, 0.54 * P));
        x = std::clamp(x * (x * x - 1.0), -1.0, 1.0);  // a aresta
        bore.push(x + 0.5 * temp);
        last = 0.8 * last + 0.2 * temp;
        out[i] = static_cast<float>(0.5 * temp + 0.5 * last);
    }
    return out;
}

// ---- percussões ----------------------------------------------------------------
// A pele circular presa na borda: os modos de Bessel (1; 1,59; 2,14; 2,30;
// 2,65; 2,92; 3,16; 3,50 × o fundamental). Onde se bate decide quais soam
// (no centro, o fundamental; perto da borda, os modos altos). Fletcher e
// Rossing 1998, cap. 18. Cada instrumento traz a sua faixa de afinação: a
// nota escolhe a altura dentro dela, por oitavas.
double foldInto(double hz, double lo, double hi)
{
    while (hz < lo)
        hz *= 2.0;
    while (hz > hi)
        hz *= 0.5;
    return std::max(hz, lo);
}

void membraneModes(std::vector<float>& out, double sr, double f, const std::array<double, 8>& amp, double t60, double drop,
                   double phase0)
{
    static const std::array<double, 8> bessel { 1.0, 1.594, 2.136, 2.296, 2.653, 2.918, 3.156, 3.501 };
    for (std::size_t k = 0; k < bessel.size(); ++k)
    {
        if (amp[k] <= 0.0 || f * bessel[k] > sr * 0.45)
            continue;
        const double d = std::log(1000.0) / (t60 / (1.0 + 0.5 * static_cast<double>(k)));
        double ph = phase0 * static_cast<double>(k);
        for (std::size_t i = 0; i < out.size(); ++i)
        {
            const double t = static_cast<double>(i) / sr;
            // a altura cai logo após o golpe (a pele afrouxa ao voltar)
            ph += 2.0 * kPi * f * bessel[k] * (1.0 + drop * std::exp(-t / 0.04)) / sr;
            out[i] += static_cast<float>(amp[k] * std::exp(-d * t) * std::sin(ph));
        }
    }
}

// a mão ou a baqueta: um estalo de ruído filtrado
void strikeNoise(std::vector<float>& out, double sr, Noise& nz, double hz, double q, double amp, double tau)
{
    Reso r;
    r.set(hz, q, sr);
    const auto n = std::min(out.size(), static_cast<std::size_t>(tau * 6.0 * sr));
    for (std::size_t i = 0; i < n; ++i)
        out[i] += static_cast<float>(amp * r.run(nz.next()) * std::exp(-static_cast<double>(i) / (tau * sr)));
}

// surdo: a pele grande e funda, baqueta de feltro; nota curta = abafado com a mão
std::vector<float> surdo(const Note& n, double sr, std::uint64_t seed)
{
    const double f = foldInto(midiHz(n.pitch), 48.0, 95.0);
    const bool muted = n.duration < 0.18;
    const double t60 = muted ? 0.25 : 1.4;
    std::vector<float> out(static_cast<std::size_t>((t60 + 0.1) * sr));
    Noise nz { seed | 1 };
    membraneModes(out, sr, f, { 1.0, 0.35, 0.2, 0.08, 0.06, 0.03, 0.02, 0.01 }, t60, 0.06, 0.4);
    strikeNoise(out, sr, nz, 180.0, 1.2, 0.6 * n.velocity, 0.006);
    return out;
}

// conga: aberta (o tom), ou o tapa (slap) no forte: modos altos e estalo
std::vector<float> conga(const Note& n, double sr, std::uint64_t seed)
{
    const double f = foldInto(midiHz(n.pitch), 170.0, 340.0);
    const bool slap = n.velocity > 0.8f;
    std::vector<float> out(static_cast<std::size_t>(0.8 * sr));
    Noise nz { seed | 1 };
    if (slap)
        membraneModes(out, sr, f, { 0.4, 0.5, 0.6, 0.5, 0.45, 0.4, 0.3, 0.25 }, 0.18, 0.02, 0.9);
    else
        membraneModes(out, sr, f, { 1.0, 0.45, 0.3, 0.15, 0.1, 0.05, 0.03, 0.02 }, 0.55, 0.03, 0.6);
    strikeNoise(out, sr, nz, slap ? 2800.0 : 900.0, slap ? 0.8 : 1.0, (slap ? 0.9 : 0.35) * n.velocity, slap ? 0.004 : 0.003);
    return out;
}

// pandeiro: pele pequena e as platinelas (pares de discos de metal que se
// chocam: modos inarmônicos agudos, em várias colisões nos primeiros ms)
std::vector<float> pandeiro(const Note& n, double sr, std::uint64_t seed)
{
    const double f = foldInto(midiHz(n.pitch), 190.0, 320.0);
    std::vector<float> out(static_cast<std::size_t>(0.5 * sr));
    Noise nz { seed | 1 };
    // a pele: o polegar (forte, grave) ou a ponta dos dedos (fraco)
    const double skin = n.velocity > 0.6f ? 0.8 : 0.3;
    membraneModes(out, sr, f, { skin, 0.3 * skin, 0.25 * skin, 0.1, 0.05, 0.03, 0.0, 0.0 }, 0.3, 0.03, 0.5);
    // as platinelas
    static const std::array<double, 5> jr { 1.0, 1.47, 1.93, 2.61, 3.27 };
    const int hits = 3 + static_cast<int>(3.0 * (0.5 + 0.5 * nz.next()));
    for (int h = 0; h < hits; ++h)
    {
        const auto at = static_cast<std::size_t>((0.002 + 0.025 * (0.5 + 0.5 * nz.next())) * sr);
        const double base = 4200.0 + 1600.0 * nz.next();
        const double a = 0.12 * (0.6 + 0.4 * n.velocity) / (1.0 + 0.4 * h);
        for (double r : jr)
        {
            const double hz = base * r * 0.5;
            if (hz > sr * 0.45)
                continue;
            const double d = std::log(1000.0) / 0.12;
            double ph = 0.0;
            for (std::size_t i = at; i < out.size(); ++i)
            {
                const double t = static_cast<double>(i - at) / sr;
                ph += 2.0 * kPi * hz / sr;
                out[i] += static_cast<float>(a * std::exp(-d * t) * std::sin(ph));
            }
        }
    }
    return out;
}

// ganzá / chocalho: partículas dentro de um tubo (PhISEM — Cook, "Physically
// informed sonic modeling (PhISM)", CMJ 21(3), 1997): a energia do gesto
// decai; a cada amostra, cada grão pode colidir com probabilidade
// proporcional à energia, e cada colisão excita ressoadores agudos.
std::vector<float> shaker(const Note& n, double sr, std::uint64_t seed)
{
    const double dur = std::clamp(n.duration, 0.06, 0.4);
    std::vector<float> out(static_cast<std::size_t>((dur + 0.15) * sr));
    Noise nz { seed | 1 };
    std::array<Reso, 3> res;
    const std::array<double, 3> rf { 3200.0, 5400.0, 7600.0 };
    for (std::size_t k = 0; k < res.size(); ++k)
        res[k].set(rf[k] * (1.0 + 0.05 * nz.next()), 6.0, sr);
    const int grains = 48;
    double energy = 0.0, sound = 0.0;
    for (std::size_t i = 0; i < out.size(); ++i)
    {
        const double t = static_cast<double>(i) / sr;
        if (t < dur)
            energy = std::min(1.0, energy + 0.002);  // a mão sacode
        energy *= 0.9995;
        if ((0.5 + 0.5 * nz.next()) < energy * grains / sr * 40.0)
            sound += 0.6 * energy * n.velocity * nz.next();
        sound *= 0.96;
        double y = 0.0;
        for (auto& r : res)
            y += r.run(sound);
        out[i] = static_cast<float>(y);
    }
    return out;
}

// ---- sax: palheta num tubo cônico ------------------------------------------------
// O cone se comporta como um tubo aberto nas duas pontas: todos os
// harmônicos, ao contrário do clarinete (só os ímpares). O modelo é a mesma
// palheta-válvula do clarinete (Smith 1986), com o reflexo sem inversão e o
// laço de um período inteiro: o atalho que dá ao tubo a série harmônica
// completa (o cone de Ayers, Eliason e Mahgerefteh, "The conical bore in
// musical acoustics", AJP 53(6), 1985). Medido em 2026-10-15: os pares a −3
// a −9 dB (no clarinete, a −45 dB). O corpo: dois formantes do tenor no
// grave e do alto no médio-agudo.
// (A primeira tentativa, a palheta num ponto interno de duas linhas, tinha
// um laço curto que dominava: soava ~4 semitons acima.)
std::vector<float> sax(const Note& n, double sr, std::uint64_t seed)
{
    const double release = 0.12;
    const auto len = static_cast<std::size_t>((n.duration + release + 0.2) * sr);
    std::vector<float> out(len);
    Delay bore(static_cast<std::size_t>(sr / 40.0) + 4);
    Noise nz { seed | 1 };
    double lp = 0.0, prev = 0.0, tone = 0.0, dcIn = 0.0, dc = 0.0;
    const double pMax = 0.72 + 0.25 * n.velocity;  // dentro da região estável (≥ 0,7)
    const double f0 = midiHz(n.pitch);
    const double lpA = 1.0 - std::exp(-2.0 * kPi * std::clamp(5.0 * f0, 900.0, 3000.0) / sr);
    Reso f1, f2;
    f1.set(f0 < 220.0 ? 500.0 : 650.0, 1.4, sr);
    f2.set(f0 < 220.0 ? 1450.0 : 1750.0, 2.2, sr);
    Reso breathBand;
    breathBand.set(2800.0, 0.8, sr);
    double level = 0.0;
    // no piano, o tenor fica mais escuro (o subtone): o corte cai
    const double lpSoft = 1.0 - std::exp(-2.0 * kPi * std::clamp((3.0 + 2.5 * n.velocity) * f0, 700.0, 3000.0) / sr);
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = hzAt(n, t, 0.09, 5.2, 0.35);
        const double env = t < n.duration ? std::min(1.0, t / 0.03) : std::max(0.0, 1.0 - (t - n.duration) / 0.05);
        const double breath = pMax * env * (1.0 + 0.02 * nz.next());
        const double fb = bore.read(sr / hz - 1.0);
        lp = 0.5 * lp + 0.5 * fb;
        // perda 0,9 e palheta rígida (inclinação 0,6), sopro ≥ 0,7: as
        // varreduras de 2026-10-15 (perda 0,8–0,95 × filtro × inclinação
        // 0,2–0,6 × sopro 0,6–0,95, notas 44–76) mostraram a palheta caótica
        // (aperiodicidade 0,4–0,6, a nota firmando só em ~0,25 s) fora desta
        // região; aqui 0,01 em toda a extensão, firme em 70 ms
        const double dp = 0.9 * lp - breath;
        const double r = std::clamp(0.7 - 0.6 * dp, -1.0, 1.0);
        bore.push(breath + dp * r);
        // a pressão no tubo é quase um trem de pulsos (todos os harmônicos
        // iguais, medido); o som irradiado cai ~12 dB por oitava acima de
        // ~5 × f₀ (dois polos), com os formantes por cima, discretos
        tone += lpSoft * (fb - tone);
        prev += lpA * (tone - prev);
        dc = prev - dcIn + 0.995 * dc;
        dcIn = prev;
        // o ar (2026-10-16): o sopro que passa pela palheta, mais presente no
        // piano (o "subtone" do tenor), modulado pela própria vibração
        // (escalado pelo nível do próprio som: um primeiro ensaio com nível fixo
        // encobria a nota)
        level += 0.002 * (std::abs(dc) - level);
        // (no agudo o ar pesa menos: as notas acima de ~350 Hz quase não sopram)
        const double air = breathBand.run(nz.next()) * level * (0.15 + 0.25 * (1.0 - n.velocity)) * std::min(1.0, 350.0 / f0);
        out[i] = static_cast<float>(dc + 0.25 * f1.run(dc) + 0.12 * f2.run(dc) + air);
    }
    return out;
}

// ---- metais: o modelo de sinal ------------------------------------------------------
// O lábio como válvula (Cook 1992; Adachi e Sato 1996) não ficou estável em
// toda a extensão (2026-10-15). Os metais vêm, por ora, do modelo de sinal
// clássico: J.-C. Risset e M. V. Mathews, "Analysis of musical-instrument
// tones", Physics Today 22(2), 1969 — o que faz o metal é o BRILHO QUE CRESCE
// COM A FORÇA: os harmônicos altos crescem mais depressa que os baixos (J. W.
// Beauchamp, "Synthesis by spectral amplitude and 'brightness' matching",
// JAES 30(6), 1982). O ataque chega de baixo (o "buzz" dos lábios se
// acertando), a campana dá o formante e há o ar.
// Trompete: formante ~1,2 kHz, ataque rápido. Trombone: ~550 Hz, mais lento.
std::vector<float> brass(const Note& n, double sr, std::uint64_t seed, bool trombone)
{
    const double release = 0.09;
    const auto len = static_cast<std::size_t>((n.duration + release + 0.05) * sr);
    std::vector<float> out(len);
    Noise nz { seed | 1 };
    const double f0 = midiHz(n.pitch);
    const double formant = trombone ? 550.0 : 1250.0, bw = trombone ? 700.0 : 1400.0;
    const double attack = trombone ? 0.06 : 0.035;
    const int harmonics = std::clamp(static_cast<int>(sr * 0.45 / f0), 1, 30);
    std::vector<double> w(static_cast<std::size_t>(harmonics) + 1);
    for (int k = 1; k <= harmonics; ++k)
    {
        const double fk = k * f0;
        w[static_cast<std::size_t>(k)] = std::pow(static_cast<double>(k), -0.3) / std::sqrt(1.0 + std::pow((fk - formant) / bw, 2.0));
    }
    // a fase do fundamental; o harmônico k é k vezes ela (a fase inicial 0,4·k
    // também), então sin(kθ) sai da recorrência de Chebyshev (2026-10-18)
    double theta = 0.4;
    Reso air;
    air.set(trombone ? 1200.0 : 2400.0, 1.0, sr);
    double drift = 0.0;
    const double loud = 0.45 + 0.55 * n.velocity;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        // o envelope: ataque, um leve decaimento, o fim
        double env = t < attack ? t / attack : 1.0 - 0.12 * std::min(1.0, (t - attack) / 0.4);
        if (t > n.duration)
            env *= std::max(0.0, 1.0 - (t - n.duration) / release);
        const double E = std::clamp(env * loud, 0.0, 1.0);
        // a altura: chega de baixo nos primeiros 40 ms; deriva lenta de ~3 cents
        drift += 0.0005 * nz.next();
        drift *= 0.9995;
        const double scoop = -0.35 * std::exp(-t / 0.018);
        const double hz = hzAt(n, t, trombone ? 0.0 : 0.06, 5.0, 0.45) * std::pow(2.0, (scoop + drift * 0.3) / 12.0);
        double y = 0.0;
        theta += 2.0 * kPi * hz / sr;
        if (theta > 2.0 * kPi)
            theta -= 2.0 * kPi;
        {
            // o brilho cresce com a força: a amplitude do k-ésimo harmônico
            // vai com E^(1 + 0,45 (k − 1)) = E · (E^0,45)^(k − 1)
            const double q = std::pow(E, 0.45), c2 = 2.0 * std::cos(theta);
            double amp = E, prev = 0.0, cur = std::sin(theta);
            for (int k = 1; k <= harmonics; ++k)
            {
                y += w[static_cast<std::size_t>(k)] * amp * cur;
                amp *= q;
                const double next = c2 * cur - prev;
                prev = cur;
                cur = next;
            }
        }
        y += 0.03 * E * air.run(nz.next());
        out[i] = static_cast<float>(y);
    }
    return out;
}

// ---- a bateria e a percussão latina ---------------------------------------------------
// Pratos e chimbal: muitos modos inarmônicos densos (a placa fina) mais o
// ruído agudo do toque; o prato de condução tem o "sino" (o domo) que soa no
// toque com a ponta da baqueta. Caixa: a pele e a esteira (ruído agudo que
// segue a pele); com escovinha, o ataque é um varrido. Bumbo: a pele grande,
// a altura que cai e o estalo do batedor. Fletcher e Rossing 1998, cap. 18 e
// 20; T. D. Rossing, Science of Percussion Instruments, World Scientific, 2000.

// ---- os pratos (refeitos em 2026-10-16: "o som do prato não está parecido com
// o habitual do jazz") ------------------------------------------------------------------------
// O prato real tem CENTENAS de modos densos e inarmônicos, a maior parte entre
// 3 e 12 kHz; a baqueta excita um "ping" e um estalo curto; logo depois a
// energia passa dos modos graves aos agudos (o acoplamento não linear da
// placa fina) e cresce a "lavagem" — o chiado que decai devagar (A. Chaigne,
// C. Touzé e O. Thomas, "Nonlinear vibrations and chaos in gongs and cymbals",
// Acoustical Science and Technology 26(5), 2005; T. D. Rossing, Science of
// Percussion Instruments, 2000, cap. 8). O de condução do jazz é escuro e
// lavado; o chimbal fechado pelo pé é o "tchik" curto das duas placas.
// Cada parcial é um ressoador recursivo (barato), e o golpe é sintetizado uma
// vez por variação e faixa de força e reaproveitado (o cache abaixo).
struct CymbalSpec
{
    int partials;
    double fLo, fHi;       // a faixa dos modos (Hz)
    double centre, spread; // onde a energia se concentra (Hz) e a largura (oitavas)
    double t60At1k;        // o decaimento em 1 kHz (s); os agudos mais curtos
    double washT60, washLevel, washRise;  // a lavagem: decaimento, nível, subida (s)
    double tick;           // o estalo da baqueta
    double length;         // s
};

std::vector<float> cymbalCore(const CymbalSpec& c, double velocity, double sr, std::uint64_t seed)
{
    const auto len = static_cast<std::size_t>(c.length * sr);
    std::vector<float> out(len, 0.0f);
    Noise nz { seed | 1 };
    const double lo = std::log2(c.fLo), hi = std::log2(c.fHi);
    // os modos: frequências log-uniformes com desvio (inarmônicas e densas)
    std::vector<double> b1(static_cast<std::size_t>(c.partials)), b2(b1.size()), amp(b1.size()), y1(b1.size()), y2(b1.size());
    for (std::size_t k = 0; k < b1.size(); ++k)
    {
        const double u = (static_cast<double>(k) + 0.5 + 0.45 * nz.next()) / static_cast<double>(b1.size());
        const double f = std::pow(2.0, lo + (hi - lo) * u);
        const double t60 = std::clamp(c.t60At1k * std::pow(1000.0 / f, 0.3), 0.08, 8.0);
        const double r = std::exp(-std::log(1000.0) / (t60 * sr));
        const double w = 2.0 * kPi * f / sr;
        b1[k] = 2.0 * r * std::cos(w);
        b2[k] = -r * r;
        const double oct = std::log2(f / c.centre) / c.spread;
        amp[k] = std::exp(-0.5 * oct * oct) * (0.6 + 0.4 * (0.5 + 0.5 * nz.next())) * (0.4 + 0.6 * velocity);
        // a condição inicial: o golpe põe cada modo em movimento
        y1[k] = amp[k] * std::sin(w);
        y2[k] = 0.0;
    }
    Reso washA, washB, tickR;
    washA.set(c.centre * 1.3, 0.7, sr);
    washB.set(c.centre * 2.4, 0.9, sr);
    tickR.set(6500.0, 0.9, sr);
    const double washDecay = std::log(1000.0) / c.washT60;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        double s = 0.0;
        for (std::size_t k = 0; k < b1.size(); ++k)
        {
            const double y = b1[k] * y1[k] + b2[k] * y2[k];
            y2[k] = y1[k];
            y1[k] = y;
            s += y;
        }
        s /= std::sqrt(static_cast<double>(b1.size()));
        // a lavagem: cresce em washRise e decai devagar
        const double washEnv = (1.0 - std::exp(-t / c.washRise)) * std::exp(-washDecay * t);
        const double nzv = nz.next();
        const double wash = c.washLevel * (0.4 + 0.6 * velocity) * washEnv * (washA.run(nzv) + 0.7 * washB.run(nzv));
        const double tick = t < 0.004 ? c.tick * velocity * tickR.run(nz.next()) * (1.0 - t / 0.004) : 0.0;
        out[i] = static_cast<float>(s + wash + tick);
    }
    return out;
}

// o cache dos golpes: (timbre, variação 0..3, faixa de força 0..3)
const std::vector<float>& cachedCymbal(int kind, const CymbalSpec& c, double velocity, double sr, std::uint64_t seed)
{
    static std::mutex m;
    static std::map<std::tuple<int, int, int, int>, std::vector<float>> cache;
    const int variant = static_cast<int>(seed % 4);
    const int band = std::clamp(static_cast<int>(velocity * 4.0), 0, 3);
    const auto key = std::make_tuple(kind, variant, band, static_cast<int>(sr));
    std::lock_guard<std::mutex> lock(m);
    auto it = cache.find(key);
    if (it == cache.end())
        it = cache.emplace(key, cymbalCore(c, (band + 0.5) / 4.0, sr, 0xC1BA1ull * (variant + 1) + static_cast<std::uint64_t>(kind))).first;
    return it->second;
}

std::vector<float> ride(const Note& n, double sr, std::uint64_t seed)
{
    // o de condução do jazz: escuro e lavado, o "ping" em ~3,5 kHz
    // (medido em 2026-10-16: com o centro em 3,6 kHz a energia ficava em
    // 1–3 kHz, escura demais; o ping do jazz mora em 3–8 kHz)
    static const CymbalSpec spec { 140, 700.0, 15000.0, 5200.0, 0.9, 3.6, 3.2, 0.07, 0.03, 0.35, 3.5 };
    return cachedCymbal(0, spec, n.velocity, sr, seed);
}

std::vector<float> hihat(const Note& n, double sr, std::uint64_t seed)
{
    const bool open = n.duration > 0.3;
    if (open)
    {
        static const CymbalSpec spec { 90, 900.0, 16000.0, 7000.0, 0.9, 0.9, 0.55, 0.06, 0.008, 0.4, 1.0 };
        return cachedCymbal(1, spec, n.velocity, sr, seed);
    }
    // fechado pelo pé (o "tchik"): curtíssimo, e o estalo médio das duas placas
    static const CymbalSpec spec { 70, 1500.0, 16000.0, 7500.0, 0.8, 0.09, 0.07, 0.12, 0.003, 0.6, 0.18 };
    auto out = cachedCymbal(2, spec, n.velocity, sr, seed);
    Noise nz { seed | 1 };
    Reso clap;
    clap.set(1300.0, 1.4, sr);
    for (std::size_t i = 0; i < std::min<std::size_t>(out.size(), static_cast<std::size_t>(0.02 * sr)); ++i)
        out[i] += static_cast<float>(0.25 * n.velocity * clap.run(nz.next()) * std::exp(-static_cast<double>(i) / (0.004 * sr)));
    return out;
}

std::vector<float> snare(const Note& n, double sr, std::uint64_t seed, bool brushes)
{
    std::vector<float> out(static_cast<std::size_t>(0.5 * sr));
    Noise nz { seed | 1 };
    if (!brushes)
        membraneModes(out, sr, 185.0, { 0.8, 0.5, 0.3, 0.2, 0.1, 0.05, 0.0, 0.0 }, 0.25, 0.02, 0.7);
    // a esteira (ou a escovinha varrendo): ruído agudo
    Reso r1, r2;
    r1.set(brushes ? 4500.0 : 3500.0, 0.6, sr);
    r2.set(brushes ? 9000.0 : 7000.0, 0.8, sr);
    const double tau = brushes ? 0.09 : 0.06;
    const double rise = brushes ? 0.025 : 0.0005;
    for (std::size_t i = 0; i < out.size(); ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double env = std::min(1.0, t / rise) * std::exp(-t / tau);
        out[i] += static_cast<float>((brushes ? 0.25 : 0.45) * n.velocity * env * (r1.run(nz.next()) + 0.6 * r2.run(nz.next())));
    }
    return out;
}

std::vector<float> kick(const Note& n, double sr, std::uint64_t seed)
{
    std::vector<float> out(static_cast<std::size_t>(0.6 * sr));
    Noise nz { seed | 1 };
    membraneModes(out, sr, 52.0, { 1.0, 0.25, 0.12, 0.05, 0.03, 0.0, 0.0, 0.0 }, 0.45, 0.35, 0.2);
    strikeNoise(out, sr, nz, 2500.0, 0.8, 0.25 * n.velocity, 0.002);
    return out;
}

// bongô: o par agudo (macho ~ 400 Hz, hembra ~ 300 Hz), seco
std::vector<float> bongo(const Note& n, double sr, std::uint64_t seed)
{
    const double f = foldInto(midiHz(n.pitch), 290.0, 560.0);
    std::vector<float> out(static_cast<std::size_t>(0.4 * sr));
    Noise nz { seed | 1 };
    membraneModes(out, sr, f, { 1.0, 0.5, 0.35, 0.2, 0.12, 0.06, 0.03, 0.02 }, 0.22, 0.03, 0.5);
    strikeNoise(out, sr, nz, 3200.0, 0.9, 0.4 * n.velocity, 0.002);
    return out;
}

// guiro: a vareta raspando as ranhuras — um trem de estalos (~ 40 por
// segundo) num ressoador de cabaça
std::vector<float> guiro(const Note& n, double sr, std::uint64_t seed)
{
    const double dur = std::clamp(n.duration, 0.08, 0.6);
    std::vector<float> out(static_cast<std::size_t>((dur + 0.08) * sr));
    Noise nz { seed | 1 };
    Reso r1, r2;
    r1.set(2600.0, 4.0, sr);
    r2.set(4800.0, 5.0, sr);
    const double rate = 38.0 + 10.0 * n.velocity;
    double phase = 0.0;
    for (std::size_t i = 0; i < out.size(); ++i)
    {
        const double t = static_cast<double>(i) / sr;
        double x = 0.0;
        phase += rate / sr;
        if (t < dur && phase >= 1.0)
        {
            phase -= 1.0;
            x = 1.0 + 0.3 * nz.next();
        }
        x += 0.05 * nz.next() * (t < dur ? 1.0 : 0.0);
        out[i] = static_cast<float>(0.5 * n.velocity * (r1.run(x) + 0.7 * r2.run(x)));
    }
    return out;
}

// ---- a voz: fonte e filtro -------------------------------------------------------------
// A glote dá pulsos de ar (o pulso de Rosenberg: abre devagar em ~40 % do
// período, fecha depressa em ~16 %; A. E. Rosenberg, "Effect of glottal
// pulse shape on the quality of natural vowels", JASA 49(2B), 1971); o trato
// vocal os molda em vogal por cinco formantes em cascata (D. H. Klatt,
// "Software for a cascade/parallel formant synthesizer", JASA 67(3), 1980).
// As vogais (a, é, i, ó, u) vêm das médias de G. E. Peterson e H. L.
// Barney, "Control methods used in a study of the vowels", JASA 24(2), 1952,
// para homens e mulheres; no agudo, o primeiro formante sobe com a nota
// (a "afinação de formantes" das sopranos; J. Sundberg, The Science of the
// Singing Voice, Northern Illinois University Press, 1987). Na voz grave,
// o terceiro formante perto do "formante do cantor" (~2,8 kHz, Sundberg).
// O vibrato (~5,5 Hz, ±0,3 semitom), o jitter e o shimmer (as pequenas
// irregularidades de período e amplitude) e o ar da glote aberta.
struct Formant
{
    double b = 0, c = 0, a = 1, y1 = 0, y2 = 0;
    void set(double hz, double bw, double sr)
    {
        const double T = 1.0 / sr;
        c = -std::exp(-2.0 * kPi * bw * T);
        b = 2.0 * std::exp(-kPi * bw * T) * std::cos(2.0 * kPi * hz * T);
        a = 1.0 - b - c;
    }
    double run(double x)
    {
        const double y = a * x + b * y1 + c * y2;
        y2 = y1;
        y1 = y;
        return y;
    }
};

enum class VoiceKind { sung, whisper, hum };

std::vector<float> voice(const Note& n, double sr, std::uint64_t seed, VoiceKind kind)
{
    const double release = 0.12;
    const auto len = static_cast<std::size_t>((n.duration + release + 0.1) * sr);
    std::vector<float> out(len);
    Noise nz { seed | 1 };
    const double f0 = midiHz(n.pitch);
    // o registro pela altura
    const bool male = n.pitch < 60.0f;
    const bool high = n.pitch >= 74.0f;
    // a vogal da nota, pela semente (a e ó mais frequentes, como num vocalise)
    static const double M[5][3] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 }, { 570, 840, 2410 }, { 300, 870, 2240 } };
    static const double W[5][3] = { { 850, 1220, 2810 }, { 610, 2330, 2990 }, { 310, 2790, 3310 }, { 590, 920, 2710 }, { 370, 950, 2670 } };
    static const int pickVowel[8] = { 0, 0, 3, 3, 1, 4, 2, 0 };
    const int v = pickVowel[(seed >> 7) % 8];
    double F[5] = { male ? M[v][0] : W[v][0], male ? M[v][1] : W[v][1], male ? M[v][2] : W[v][2], male ? 3300.0 : 3800.0,
                    male ? 4100.0 : 4600.0 };
    if (high)
        for (int k = 0; k < 3; ++k)
            F[k] *= 1.1;  // o trato mais curto do agudo
    if (F[0] < f0 * 1.05)
        F[0] = f0 * 1.05;  // a afinação de formantes: o F1 acompanha a nota
    if (male)
        F[2] = std::max(F[2], 2700.0);  // perto do formante do cantor
    if (kind == VoiceKind::hum)
    {
        // boca fechada: o murmúrio nasal (F1 baixo, os altos abafados)
        F[0] = std::max(250.0, f0 * 1.05);
        F[1] = 1000.0;
        F[2] = 2200.0;
    }
    const double BW[5] = { 80.0, 90.0, 120.0, 130.0, 140.0 };
    std::array<Formant, 5> tract;
    for (int k = 0; k < 5; ++k)
        tract[static_cast<std::size_t>(k)].set(F[k], kind == VoiceKind::hum && k > 0 ? BW[k] * 3.0 : BW[k], sr);
    Formant nasalLow;
    nasalLow.set(std::max(60.0, 2.0 * f0), 400.0, sr);
    // o esforço: forte = pulso que fecha mais depressa (mais brilho)
    const double closing = 0.2 - 0.08 * n.velocity, opening = 0.42;
    double phase = 0.0, jitter = 0.0, shimmer = 1.0, prevFlow = 0.0, lpOut = 0.0;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = hzAt(n, t, 0.3, 5.5, 0.28) * (1.0 + jitter);
        double env = t < n.duration ? std::min(1.0, t / 0.05) : std::max(0.0, 1.0 - (t - n.duration) / 0.1);
        env *= shimmer;
        phase += hz / sr;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            jitter = 0.004 * nz.next();        // ~0,4 % de período
            shimmer = 1.0 + 0.03 * nz.next();  // ~3 % de amplitude
        }
        // o fluxo de Rosenberg e a sua derivada (a radiação dos lábios)
        double flow = 0.0;
        if (phase < opening)
            flow = 0.5 * (1.0 - std::cos(kPi * phase / opening));
        else if (phase < opening + closing)
            flow = std::cos(0.5 * kPi * (phase - opening) / closing);
        const double dflow = (flow - prevFlow) * sr / std::max(hz, 1.0) * 0.1;
        prevFlow = flow;
        // o ar: ruído que respira mais na glote aberta; no sussurro, só ele
        const double air = nz.next() * (kind == VoiceKind::whisper ? 0.5 : 0.04 * (0.3 + flow));
        double x = (kind == VoiceKind::whisper ? 0.0 : dflow) + air;
        x *= env;
        for (auto& f : tract)
            x = f.run(x);
        if (kind == VoiceKind::hum)
            x = 0.5 * x + 0.8 * nasalLow.run(x);
        lpOut += 0.6 * (x - lpOut);
        out[i] = static_cast<float>(lpOut);
    }
    return out;
}

// ---- o kit completo: tons e prato de ataque -----------------------------------------------
// tom-tom: a pele média e funda, a baqueta; a nota escolhe o tom (grave,
// médio, agudo) dentro de 80–240 Hz
std::vector<float> tom(const Note& n, double sr, std::uint64_t seed)
{
    const double f = foldInto(midiHz(n.pitch), 80.0, 240.0);
    std::vector<float> out(static_cast<std::size_t>(0.9 * sr));
    Noise nz { seed | 1 };
    membraneModes(out, sr, f, { 1.0, 0.45, 0.3, 0.15, 0.1, 0.05, 0.03, 0.02 }, 0.7, 0.08, 0.5);
    strikeNoise(out, sr, nz, 1800.0, 0.9, 0.35 * n.velocity, 0.003);
    return out;
}

// prato de ataque: a placa grande golpeada de lado, o "chiado" largo que
// abre depois do golpe
std::vector<float> crash(const Note& n, double sr, std::uint64_t seed)
{
    // o de ataque: grande e brilhante, a lavagem forte que abre depois do golpe
    static const CymbalSpec spec { 140, 300.0, 16000.0, 5200.0, 1.3, 2.4, 2.2, 0.35, 0.02, 0.5, 3.0 };
    return cachedCymbal(3, spec, n.velocity, sr, seed);
}

// ---- elétricos: a corda de aço, o captador e o amplificador -------------------------------
// A corda é a mesma do violão (pluckString), sem caixa: o som vem do
// captador magnético, que "ouve" a corda num ponto (o pente do captador).
// O amplificador valvulado: o grave cortado na entrada, o ganho, a
// saturação assimétrica (tanh deslocado: os harmônicos pares da válvula), o
// tom e a caixa (o falante de 12": a ressonância grave ~110 Hz e o corte
// acima de ~4,5 kHz). Fontes: Jaffe e Smith 1983; D. T. Yeh, J. S. Abel e
// J. O. Smith, "Simulation of the diode limiter in guitar distortion
// circuits by numerical solution of ordinary differential equations",
// DAFx 2007; J. Pakarinen e D. T. Yeh, "A review of digital techniques for
// modeling vacuum-tube guitar amplifiers", CMJ 33(2), 2009.
std::vector<float> amp(std::vector<float> x, double sr, double drive, double cabCut)
{
    float peak = 1e-6f;
    for (float v : x)
        peak = std::max(peak, std::abs(v));
    double hp = 0.0, hpIn = 0.0, tone = 0.0, c1 = 0.0, c2 = 0.0;
    const double aHp = std::exp(-2.0 * kPi * 90.0 / sr);
    const double aTone = 1.0 - std::exp(-2.0 * kPi * 6000.0 / sr);
    const double aCab = 1.0 - std::exp(-2.0 * kPi * cabCut / sr);
    Reso low;
    low.set(110.0, 1.4, sr);
    const double bias = 0.25;
    for (auto& v : x)
    {
        const double in = 0.5 * v / peak;
        hp = aHp * (hp + in - hpIn);
        hpIn = in;
        const double y = (std::tanh(drive * hp + bias) - std::tanh(bias)) / std::tanh(std::max(1.0, drive));
        tone += aTone * (y - tone);
        c1 += aCab * (tone - c1);
        c2 += aCab * (c1 - c2);
        v = static_cast<float>(c2 + 0.4 * low.run(c2));
    }
    return x;
}

std::vector<float> electricGuitar(const Note& n, double sr, std::uint64_t seed, double drive)
{
    // aço, palheta, a nota dura (sem caixa que roube energia)
    static const Pluck steel { 6.0, 0.45, 0.35, 0.4, 0.12, 0.86, { 100.0, 200.0, 400.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 },
                               1.0, 0.0, 1, 0.0, 0.0, 0.08 };
    auto s = pluckString(n, sr, seed, steel);
    // o captador perto do cavalete: o pente a ~1/5 da corda
    const auto lag = static_cast<std::size_t>(0.2 * sr / midiHz(n.pitch));
    std::vector<float> pick(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
        pick[i] = s[i] - (i >= lag ? 0.8f * s[i - lag] : 0.0f);
    return amp(std::move(pick), sr, drive, drive > 8.0 ? 4200.0 : 5200.0);
}

std::vector<float> electricBass(const Note& n, double sr, std::uint64_t seed)
{
    static const Pluck round { 3.2, 0.3, 0.15, 0.2, 0.15, 0.72, { 100.0, 200.0, 400.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 },
                               1.0, 0.0, 1, 0.0, 0.0, 0.0 };
    auto s = pluckString(n, sr, seed, round);
    return amp(std::move(s), sr, 1.6, 2600.0);
}

// ---- a bateria de escola de samba ----------------------------------------------------------
// tamborim: a pele pequena e tensa, a baqueta, seco
std::vector<float> tamborim(const Note& n, double sr, std::uint64_t seed)
{
    const double f = foldInto(midiHz(n.pitch), 420.0, 720.0);
    std::vector<float> out(static_cast<std::size_t>(0.25 * sr));
    Noise nz { seed | 1 };
    membraneModes(out, sr, f, { 1.0, 0.6, 0.4, 0.3, 0.2, 0.1, 0.05, 0.03 }, 0.12, 0.02, 0.6);
    strikeNoise(out, sr, nz, 4200.0, 0.8, 0.6 * n.velocity, 0.0015);
    return out;
}

// repique (repinique): o tambor agudo que chama a bateria, baqueta e mão
std::vector<float> repique(const Note& n, double sr, std::uint64_t seed)
{
    const double f = foldInto(midiHz(n.pitch), 220.0, 380.0);
    std::vector<float> out(static_cast<std::size_t>(0.5 * sr));
    Noise nz { seed | 1 };
    membraneModes(out, sr, f, { 1.0, 0.55, 0.4, 0.25, 0.15, 0.08, 0.04, 0.02 }, 0.35, 0.03, 0.7);
    strikeNoise(out, sr, nz, 3000.0, 0.8, 0.5 * n.velocity, 0.002);
    return out;
}

// agogô: duas campânulas de metal (a aguda e a grave), parciais de sino
std::vector<float> agogo(const Note& n, double sr, std::uint64_t seed)
{
    const double f = n.pitch >= 70.0f ? 920.0 : 660.0;
    std::vector<float> out(static_cast<std::size_t>(0.8 * sr));
    Noise nz { seed | 1 };
    static const std::array<double, 4> r { 1.0, 2.71, 5.12, 8.21 }, a { 1.0, 0.45, 0.25, 0.12 }, t60 { 0.7, 0.4, 0.25, 0.15 };
    for (std::size_t k = 0; k < r.size(); ++k)
    {
        const double d = std::log(1000.0) / t60[k];
        double ph = 0.0;
        for (std::size_t i = 0; i < out.size(); ++i)
        {
            ph += 2.0 * kPi * f * r[k] / sr;
            out[i] += static_cast<float>(0.4 * n.velocity * a[k] * std::exp(-d * static_cast<double>(i) / sr) * std::sin(ph));
        }
    }
    strikeNoise(out, sr, nz, 5000.0, 1.0, 0.2 * n.velocity, 0.001);
    return out;
}

// cuíca: o tambor de fricção. A vareta presa à pele, esfregada com um pano
// úmido, prende e solta (como o arco): um trem de pulsos cuja frequência o
// polegar na pele faz subir ou descer — o "uó". Os pulsos passam pelos modos
// da pele, que seguem a altura.
std::vector<float> cuica(const Note& n, double sr, std::uint64_t seed)
{
    const double dur = std::clamp(n.duration, 0.08, 0.6);
    std::vector<float> out(static_cast<std::size_t>((dur + 0.1) * sr));
    Noise nz { seed | 1 };
    const double base = foldInto(midiHz(n.pitch), 280.0, 560.0);
    const bool up = (static_cast<int>(std::lround(n.pitch)) % 2) == 0;  // sobe ("uí") ou desce ("uó")
    std::array<Reso, 3> modes;
    double phase = 0.0;
    for (std::size_t i = 0; i < out.size(); ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double u = std::min(1.0, t / dur);
        const double glide = up ? 1.0 + 0.7 * u * u : 1.7 - 0.7 * std::sqrt(u);
        const double f = base * glide;
        if (i % 32 == 0)
            for (std::size_t k = 0; k < modes.size(); ++k)
                modes[k].set(f * (1.0 + 0.6 * static_cast<double>(k)), 8.0, sr);
        phase += f / sr;
        double x = 0.0;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            x = 1.0 + 0.2 * nz.next();
        }
        const double env = t < dur ? std::min(1.0, t / 0.01) * (0.6 + 0.4 * std::sin(kPi * u)) : std::max(0.0, 1.0 - (t - dur) / 0.04);
        double y = 0.0;
        for (std::size_t k = 0; k < modes.size(); ++k)
            y += modes[k].run(x) / (1.0 + static_cast<double>(k));
        out[i] = static_cast<float>(0.8 * n.velocity * env * y);
    }
    return out;
}

// chocalho: as fileiras de platinelas que se chocam a cada movimento
std::vector<float> chocalho(const Note& n, double sr, std::uint64_t seed)
{
    std::vector<float> out(static_cast<std::size_t>(0.25 * sr));
    Noise nz { seed | 1 };
    static const std::array<double, 5> jr { 1.0, 1.47, 1.93, 2.61, 3.27 };
    const int hits = 6 + static_cast<int>(4.0 * (0.5 + 0.5 * nz.next()));
    for (int h = 0; h < hits; ++h)
    {
        const auto at = static_cast<std::size_t>((0.001 + 0.03 * (0.5 + 0.5 * nz.next())) * sr);
        const double base = 4600.0 + 1800.0 * nz.next();
        const double a = 0.08 * (0.5 + 0.5 * n.velocity);
        for (double r : jr)
        {
            const double hz = base * r * 0.5;
            if (hz > sr * 0.45)
                continue;
            const double d = std::log(1000.0) / 0.08;
            double ph = 0.0;
            for (std::size_t i = at; i < out.size(); ++i)
            {
                ph += 2.0 * kPi * hz / sr;
                out[i] += static_cast<float>(a * std::exp(-d * static_cast<double>(i - at) / sr) * std::sin(ph));
            }
        }
    }
    return out;
}

// ---- órgão de rodas fônicas ----------------------------------------------------------------
// Cada tirante é uma roda fônica, quase uma senoide, num pé do órgão: 16'
// (½), 5⅓' (1½), 8' (1), 4' (2), 2⅔' (3), 2' (4), 1⅗' (5), 1⅓' (6) e 1' (8).
// A registração "888000000" é o som clássico do jazz e do gospel. O clique
// da tecla (os contatos que fecham), a percussão (o 2º ou 3º harmônico que
// decai no ataque) e a caixa Leslie: o rotor que gira e faz o som pulsar
// entre os canais, com um leve Doppler. L. Hammond, patente US 1.956.350,
// 1934; D. Leslie, patente US 2.489.653, 1949; J. Smith et al., "Doppler
// simulation and the Leslie", DAFx 2002.
std::vector<float> organ(const Note& n, double sr, std::uint64_t seed, std::vector<float>* right)
{
    const double release = 0.06;
    const auto len = static_cast<std::size_t>((n.duration + release + 0.05) * sr);
    std::vector<float> out(len);
    Noise nz { seed | 1 };
    static const std::array<double, 9> foot { 0.5, 1.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0 };
    static const std::array<double, 9> bars { 8, 8, 8, 0, 0, 0, 0, 0, 0 };  // 888000000
    std::array<double, 9> ph {};
    const double rotor = 6.2;  // a Leslie rápida (Hz)
    if (right)
        right->assign(len, 0.0f);
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = hzAt(n, t, 0.0, 0.0, 0.0);
        double env = t < n.duration ? std::min(1.0, t / 0.005) : std::max(0.0, 1.0 - (t - n.duration) / release);
        const double doppler = 1.0 + 0.003 * std::sin(2.0 * kPi * rotor * t);
        double s = 0.0;
        for (std::size_t k = 0; k < foot.size(); ++k)
        {
            if (bars[k] <= 0.0 || hz * foot[k] > sr * 0.45)
                continue;
            ph[k] += 2.0 * kPi * hz * foot[k] * doppler / sr;
            s += bars[k] / 8.0 * std::sin(ph[k]);
        }
        // a percussão: o 3º harmônico (2⅔') que decai em ~0,3 s
        s += 0.5 * std::exp(-t / 0.3) * std::sin(ph[2] * 3.0);
        // o clique da tecla
        if (t < 0.006)
            s += 0.4 * nz.next() * (1.0 - t / 0.006);
        // um pouco de válvula
        s = std::tanh(0.6 * s) / 0.6;
        const double rot = 2.0 * kPi * rotor * t;
        out[i] = static_cast<float>(0.3 * env * s * (1.0 + 0.35 * std::sin(rot)));
        if (right)
            (*right)[i] = static_cast<float>(0.3 * env * s * (1.0 + 0.35 * std::sin(rot + kPi)));
    }
    return out;
}

// ---- palheta livre: sanfona e bandoneón ------------------------------------------------------
// A palheta de metal vibra livre na fenda, cortando o ar em pulsos: um
// espectro rico, que cai suave (~k^-0,8). O fole dá o envelope. Na sanfona,
// o registro "musette": três palhetas, uma afinada e duas desafinadas ±12
// cents, cujos batimentos fazem o trêmulo característico. No bandoneón, duas
// palhetas em oitava, sem musette: o som seco do tango, com ataque
// marcado. J. P. Cottingham, "Acoustics of free-reed instruments", Physics
// Today 64(3), 2011.
std::vector<float> freeReed(const Note& n, double sr, std::uint64_t seed, bool bandoneon)
{
    const double release = bandoneon ? 0.05 : 0.08;
    const auto len = static_cast<std::size_t>((n.duration + release + 0.05) * sr);
    std::vector<float> out(len);
    Noise nz { seed | 1 };
    const double f0 = midiHz(n.pitch);
    const int harmonics = std::clamp(static_cast<int>(sr * 0.45 / f0), 1, 40);
    struct Reed { double ratio, cents, gain; };
    const std::vector<Reed> reeds = bandoneon ? std::vector<Reed> { { 1.0, 0.0, 1.0 }, { 2.0, 1.5, 0.55 } }
                                              : std::vector<Reed> { { 1.0, 0.0, 1.0 }, { 1.0, -12.0, 0.8 }, { 1.0, 12.0, 0.8 } };
    // a fase de cada palheta; o harmônico k é k vezes ela, então sin(kθ) sai
    // da recorrência de Chebyshev (2026-10-18: um sin e um cos por palheta,
    // não um por harmônico; e os ganhos k^-0,8 e as razões calculados uma vez)
    std::vector<double> ph(reeds.size(), 0.0);
    std::vector<double> gainK(static_cast<std::size_t>(harmonics) + 1, 0.0);
    for (int k = 1; k <= harmonics; ++k)
        gainK[static_cast<std::size_t>(k)] = std::pow(static_cast<double>(k), -0.8);
    std::vector<double> reedRatio;
    for (const auto& r : reeds)
        reedRatio.push_back(r.ratio * std::pow(2.0, r.cents / 1200.0));
    std::array<Reso, 2> chamber;
    chamber[0].set(bandoneon ? 900.0 : 1200.0, 1.2, sr);
    chamber[1].set(bandoneon ? 2200.0 : 2800.0, 1.5, sr);
    const double attack = bandoneon ? 0.012 : 0.03;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = hzAt(n, t, 0.0, 0.0, 0.0);
        double env = t < n.duration ? std::min(1.0, t / attack) : std::max(0.0, 1.0 - (t - n.duration) / release);
        if (bandoneon && t < 0.08)
            env *= 1.0 + 0.4 * (1.0 - t / 0.08);  // o ataque marcado do fole
        double s = 0.0;
        for (std::size_t r = 0; r < reeds.size(); ++r)
        {
            const double f = hz * reedRatio[r];
            ph[r] += 2.0 * kPi * f / sr;
            if (ph[r] > 2.0 * kPi)
                ph[r] -= 2.0 * kPi;
            const int kmax = std::min(harmonics, static_cast<int>(sr * 0.45 / f));
            const double s1 = std::sin(ph[r]), c2 = 2.0 * std::cos(ph[r]);
            double prev = 0.0, cur = s1, acc = 0.0;  // sin(0θ), sin(1θ)
            for (int k = 1; k <= kmax; ++k)
            {
                acc += gainK[static_cast<std::size_t>(k)] * cur;
                const double next = c2 * cur - prev;
                prev = cur;
                cur = next;
            }
            s += reeds[r].gain * acc;
        }
        s *= 0.12;
        s += 0.004 * nz.next() * env;  // o ar do fole
        const double y = 0.6 * s + 0.4 * (chamber[0].run(s) + 0.6 * chamber[1].run(s));
        out[i] = static_cast<float>(env * y);
    }
    return out;
}

// ---- zabumba e triângulo -----------------------------------------------------------------------
// zabumba: o tambor largo e raso do forró. A maceta na pele de cima (o
// "tum" grave, ~70 Hz) ou o bacalhau (a vareta fina) na pele de baixo (o
// estalo agudo e seco). A nota grave (< 55) é a maceta.
std::vector<float> zabumba(const Note& n, double sr, std::uint64_t seed)
{
    Noise nz { seed | 1 };
    if (n.pitch < 55.0f)
    {
        std::vector<float> out(static_cast<std::size_t>(0.8 * sr));
        membraneModes(out, sr, 68.0, { 1.0, 0.4, 0.2, 0.1, 0.05, 0.03, 0.0, 0.0 }, 0.6, 0.12, 0.3);
        strikeNoise(out, sr, nz, 900.0, 0.8, 0.3 * n.velocity, 0.004);
        return out;
    }
    std::vector<float> out(static_cast<std::size_t>(0.2 * sr));
    membraneModes(out, sr, 240.0, { 0.3, 0.4, 0.4, 0.3, 0.3, 0.2, 0.15, 0.1 }, 0.08, 0.0, 0.8);
    strikeNoise(out, sr, nz, 3500.0, 0.7, 0.7 * n.velocity, 0.0015);
    return out;
}

// triângulo: a barra de aço dobrada, livre: os modos de barra livre (1;
// 2,76; 5,40; 8,93) e muitos outros pela dobra. Aberto (a mão solta) ou
// abafado (a mão fecha): a nota curta (< 0,12 s) é abafada.
std::vector<float> triangle(const Note& n, double sr, std::uint64_t seed)
{
    const bool muted = n.duration < 0.12;
    const double t60 = muted ? 0.07 : 1.8;
    std::vector<float> out(static_cast<std::size_t>((t60 + 0.05) * sr));
    Noise nz { seed | 1 };
    static const std::array<double, 7> r { 1.0, 2.76, 5.40, 8.93, 3.6, 6.9, 11.2 };
    const double base = 1150.0;
    for (std::size_t k = 0; k < r.size(); ++k)
    {
        const double hz = base * r[k];
        if (hz > sr * 0.45)
            continue;
        const double d = std::log(1000.0) / (t60 / (1.0 + 0.2 * static_cast<double>(k)));
        const double a = 0.15 * n.velocity / (1.0 + 0.3 * static_cast<double>(k));
        double ph = 0.7 * static_cast<double>(k);
        for (std::size_t i = 0; i < out.size(); ++i)
        {
            ph += 2.0 * kPi * hz / sr;
            out[i] += static_cast<float>(a * std::exp(-d * static_cast<double>(i) / sr) * std::sin(ph));
        }
    }
    strikeNoise(out, sr, nz, 7000.0, 1.0, 0.15 * n.velocity, 0.001);
    return out;
}

// ---- as quimeras: o excitador de um instrumento no corpo de outro ---------------------------
// (docs/MAQUINA_DE_TIMBRES.md §5.0; a máquina de timbres §2, aberta: as peças
// trocadas de lugar). Não imitam nada que exista.

// prato friccionado: o arco (prende e solta, um trem de pulsos irregular na
// nota) num prato — os modos densos e inarmônicos da placa, o primeiro
// afinado na nota
std::vector<float> bowedPlate(const Note& n, double sr, std::uint64_t seed)
{
    const double ring = 1.6;
    const auto len = static_cast<std::size_t>((n.duration + ring) * sr);
    std::vector<float> out(len);
    Noise nz { seed | 1 };
    constexpr int kModes = 14;
    std::array<Reso, kModes> modes;
    std::array<double, kModes> gains {};
    const double f0 = midiHz(n.pitch);
    for (int k = 0; k < kModes; ++k)
    {
        const double r = 1.0 + 0.61 * k + 0.07 * k * k + 0.03 * nz.next();
        modes[static_cast<std::size_t>(k)].set(std::min(sr * 0.45, f0 * r), 60.0 + 10.0 * k, sr);
        gains[static_cast<std::size_t>(k)] = 1.0 / (1.0 + 0.25 * k);
    }
    double phase = 0.0;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const bool bowing = t < n.duration;
        const double hz = hzAt(n, std::min(t, n.duration), 0.0, 0.0, 0.0);
        phase += hz * (1.0 + 0.02 * nz.next()) / sr;  // o arco não é regular
        double x = 0.0;
        if (bowing && phase >= 1.0)
        {
            phase -= 1.0;
            x = (0.6 + 0.4 * n.velocity) * (1.0 + 0.3 * nz.next());
        }
        if (bowing)
            x += 0.01 * nz.next();
        const double env = bowing ? std::min(1.0, t / 0.25) : 1.0;
        double y = 0.0;
        for (int k = 0; k < kModes; ++k)
            y += gains[static_cast<std::size_t>(k)] * modes[static_cast<std::size_t>(k)].run(x * env);
        out[i] = static_cast<float>(y);
    }
    return out;
}

// corda cantada: a glote (pulsos de Rosenberg, com vibrato) excita sem parar
// uma corda (o laço de Karplus-Strong) afinada um fio acima — a voz que
// vira corda, a corda que respira
std::vector<float> sungString(const Note& n, double sr, std::uint64_t seed)
{
    const double release = 0.8;
    const auto len = static_cast<std::size_t>((n.duration + release) * sr);
    std::vector<float> out(len);
    Noise nz { seed | 1 };
    Delay loop(static_cast<std::size_t>(sr / 30.0) + 4);
    double phase = 0.0, prevFlow = 0.0, lp = 0.0;
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double hz = hzAt(n, std::min(t, n.duration), 0.25, 5.5, 0.3);
        const double env = t < n.duration ? std::min(1.0, t / 0.08) : 0.0;
        phase += hz / sr;
        if (phase >= 1.0)
            phase -= 1.0;
        const double flow = phase < 0.42 ? 0.5 * (1.0 - std::cos(kPi * phase / 0.42))
                          : phase < 0.58 ? std::cos(0.5 * kPi * (phase - 0.42) / 0.16) : 0.0;
        const double src = (flow - prevFlow) * 8.0 * env + 0.01 * nz.next() * env;
        prevFlow = flow;
        const double fb = loop.read(sr / (hz * 1.004) - 0.5);
        lp = 0.6 * fb + 0.4 * lp;
        const double s = 0.985 * lp + 0.25 * src;
        loop.push(s);
        out[i] = static_cast<float>(s);
    }
    return out;
}

// pele soprada: o ar (ruído numa banda em volta da nota) soprando uma pele
// — os modos de Bessel, o primeiro na nota, ressoando muito (o sopro
// sustenta o que a baqueta só golpearia)
std::vector<float> blownSkin(const Note& n, double sr, std::uint64_t seed)
{
    const double release = 0.6;
    const auto len = static_cast<std::size_t>((n.duration + release) * sr);
    std::vector<float> out(len);
    Noise nz { seed | 1 };
    static const std::array<double, 6> bessel { 1.0, 1.594, 2.136, 2.296, 2.653, 2.918 };
    std::array<Reso, 6> modes;
    const double f0 = midiHz(n.pitch);
    for (std::size_t k = 0; k < modes.size(); ++k)
        modes[k].set(std::min(sr * 0.45, f0 * bessel[k]), 90.0 - 8.0 * k, sr);
    Reso breath;
    breath.set(f0 * 1.5, 0.8, sr);
    for (std::size_t i = 0; i < len; ++i)
    {
        const double t = static_cast<double>(i) / sr;
        const double env = t < n.duration ? std::min(1.0, t / 0.12) * (0.8 + 0.2 * std::sin(2.0 * kPi * 3.1 * t)) : 0.0;
        const double x = env * breath.run(nz.next());
        double y = 0.0;
        for (std::size_t k = 0; k < modes.size(); ++k)
            y += modes[k].run(x) / (1.0 + 0.4 * static_cast<double>(k));
        out[i] = static_cast<float>(y);
    }
    return out;
}

} // namespace


bool isAcousticTimbre(Timbre t)
{
    switch (t)
    {
        case Timbre::bowed:
        case Timbre::reed:
        case Timbre::bars:
        case Timbre::piano:
        case Timbre::guitar:
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
        case Timbre::upright:
        case Timbre::transverseFlute:
        case Timbre::surdo:
        case Timbre::conga:
        case Timbre::pandeiro:
        case Timbre::shaker:
        case Timbre::plucked:
        case Timbre::biomaString: return true;
        default: return false;
    }
}

// A extensão de cada instrumento (MIDI): uma nota escrita fora dela é tocada
// na oitava que cabe (o cavaquinho não desce abaixo do Ré 4; a flauta não
// desce abaixo do Dó 4). Fora desta tabela, a nota fica como veio.
std::pair<float, float> instrumentRange(Timbre t)
{
    switch (t)
    {
        case Timbre::bowed: return { 36.f, 100.f };  // do violoncelo ao violino
        case Timbre::reed: return { 50.f, 91.f };
        case Timbre::bars: return { 45.f, 96.f };
        case Timbre::piano: return { 21.f, 108.f };
        case Timbre::guitar: return { 40.f, 83.f };
        case Timbre::sevenString: return { 35.f, 79.f };
        case Timbre::requinto: return { 50.f, 88.f };
        case Timbre::cavaquinho: return { 62.f, 88.f };
        case Timbre::bandolim: return { 55.f, 88.f };
        case Timbre::manouche: return { 40.f, 88.f };
        case Timbre::upright: return { 28.f, 67.f };
        case Timbre::transverseFlute: return { 60.f, 96.f };
        case Timbre::sax: return { 44.f, 80.f };       // do tenor ao alto
        case Timbre::trumpet: return { 54.f, 82.f };
        case Timbre::trombone: return { 40.f, 72.f };
        case Timbre::singing:
        case Timbre::whisper:
        case Timbre::hum: return { 38.f, 84.f };  // do baixo à soprano
        case Timbre::electricGuitar:
        case Timbre::distortedGuitar: return { 40.f, 88.f };
        case Timbre::electricBass: return { 28.f, 67.f };
        case Timbre::organ: return { 24.f, 96.f };
        case Timbre::accordion: return { 41.f, 93.f };
        case Timbre::bandoneon: return { 33.f, 88.f };
        default: return { 0.f, 127.f };
    }
}

// ---- os naipes ---------------------------------------------------------------------------------
// Um naipe são vários músicos na mesma nota: cada um um pouco desafinado
// (alguns cents), entrando alguns milissegundos antes ou depois, com o seu
// próprio vibrato (velocidade e fase). É o "efeito de coro" que faz um
// naipe de cordas soar como naipe e não como um violino alto (J. Meyer,
// Acoustics and the Performance of Music, 5ª ed., Springer, 2009, cap. 9;
// a dispersão de afinação e de ataque nos naipes: S. Ternström, "Choir
// acoustics", 2003). O som de cada músico vem do instrumento solo.
static std::vector<float> renderSection(const Note& n, double sr, std::uint64_t seed, Timbre solo, int players, double cents, double ms)
{
    std::vector<float> out;
    std::uint64_t s = seed | 1;
    const auto rnd = [&] {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return static_cast<double>(s >> 11) / 9007199254740992.0 * 2.0 - 1.0;
    };
    for (int p = 0; p < players; ++p)
    {
        Note c = n;
        c.timbre = solo;
        c.pitch = n.pitch + static_cast<float>(cents * rnd() / 100.0);
        if (n.wobbleDepth <= 0.0f)
        {
            c.wobbleDepth = 0.1f + 0.06f * static_cast<float>(rnd());
            c.wobbleRate = 5.2f + 0.8f * static_cast<float>(rnd());
            c.wobbleDelay = 0.18f + 0.1f * static_cast<float>(std::abs(rnd()));
        }
        c.velocity = std::clamp(n.velocity * (1.0f + 0.1f * static_cast<float>(rnd())), 0.05f, 1.0f);
        const auto one = renderAcousticNote(c, sr, seed * 7919ull + static_cast<std::uint64_t>(p) * 104729ull + 1);
        const auto lag = static_cast<std::size_t>((ms * 0.5 * (1.0 + rnd())) * 1e-3 * sr);
        if (out.size() < one.size() + lag)
            out.resize(one.size() + lag, 0.0f);
        for (std::size_t i = 0; i < one.size(); ++i)
            out[i + lag] += one[i];
    }
    const float g = 1.0f / std::sqrt(static_cast<float>(players));
    for (auto& v : out)
        v *= g;
    return out;
}

std::vector<float> renderAcousticNote(const Note& noteIn, double sampleRate, std::uint64_t seed)
{
    // os naipes: vários solistas
    switch (noteIn.timbre)
    {
        case Timbre::stringSection: return renderSection(noteIn, sampleRate, seed, Timbre::bowed, 8, 9.0, 25.0);
        case Timbre::brassSection:
            return renderSection(noteIn, sampleRate, seed, noteIn.pitch >= 55.0f ? Timbre::trumpet : Timbre::trombone, 4, 7.0, 18.0);
        case Timbre::voiceChoir: return renderSection(noteIn, sampleRate, seed, Timbre::singing, 6, 12.0, 30.0);
        default: break;
    }

    Note note = noteIn;
    const auto [lo, hi] = instrumentRange(note.timbre);
    while (note.pitch < lo)
        note.pitch += 12.0f;
    while (note.pitch > hi)
        note.pitch -= 12.0f;
    switch (note.timbre)
    {
        case Timbre::bowed: return bowed(note, sampleRate, seed);
        case Timbre::reed: return reed(note, sampleRate, seed);
        case Timbre::bars: return bars(note, sampleRate, seed);
        case Timbre::piano: return piano(note, sampleRate, seed);
        case Timbre::guitar:
        case Timbre::sevenString:
        case Timbre::requinto:
        case Timbre::cavaquinho:
        case Timbre::bandolim:
        case Timbre::manouche: return pluckString(note, sampleRate, seed, recipe(note.timbre));
        case Timbre::upright: return upright(note, sampleRate, seed);
        case Timbre::transverseFlute: return transverseFlute(note, sampleRate, seed);
        case Timbre::surdo: return surdo(note, sampleRate, seed);
        case Timbre::conga: return conga(note, sampleRate, seed);
        case Timbre::pandeiro: return pandeiro(note, sampleRate, seed);
        case Timbre::shaker: return shaker(note, sampleRate, seed);
        case Timbre::sax: return sax(note, sampleRate, seed);
        case Timbre::trumpet: return brass(note, sampleRate, seed, false);
        case Timbre::trombone: return brass(note, sampleRate, seed, true);
        case Timbre::ride: return ride(note, sampleRate, seed);
        case Timbre::hihat: return hihat(note, sampleRate, seed);
        case Timbre::snare: return snare(note, sampleRate, seed, false);
        case Timbre::brushes: return snare(note, sampleRate, seed, true);
        case Timbre::kick: return kick(note, sampleRate, seed);
        case Timbre::bongo: return bongo(note, sampleRate, seed);
        case Timbre::guiro: return guiro(note, sampleRate, seed);
        case Timbre::singing: return voice(note, sampleRate, seed, VoiceKind::sung);
        case Timbre::whisper: return voice(note, sampleRate, seed, VoiceKind::whisper);
        case Timbre::hum: return voice(note, sampleRate, seed, VoiceKind::hum);
        case Timbre::tom: return tom(note, sampleRate, seed);
        case Timbre::crash: return crash(note, sampleRate, seed);
        case Timbre::electricGuitar: return electricGuitar(note, sampleRate, seed, 3.5);
        case Timbre::distortedGuitar: return electricGuitar(note, sampleRate, seed, 16.0);
        case Timbre::electricBass: return electricBass(note, sampleRate, seed);
        case Timbre::tamborim: return tamborim(note, sampleRate, seed);
        case Timbre::repique: return repique(note, sampleRate, seed);
        case Timbre::agogo: return agogo(note, sampleRate, seed);
        case Timbre::cuica: return cuica(note, sampleRate, seed);
        case Timbre::chocalho: return chocalho(note, sampleRate, seed);
        case Timbre::organ: return organ(note, sampleRate, seed, nullptr);
        case Timbre::accordion: return freeReed(note, sampleRate, seed, false);
        case Timbre::bandoneon: return freeReed(note, sampleRate, seed, true);
        case Timbre::zabumba: return zabumba(note, sampleRate, seed);
        case Timbre::triangle: return triangle(note, sampleRate, seed);
        case Timbre::bowedPlate: return bowedPlate(note, sampleRate, seed);
        case Timbre::sungString: return sungString(note, sampleRate, seed);
        case Timbre::blownSkin: return blownSkin(note, sampleRate, seed);
        default: return {};
    }
}

} // namespace trioio
