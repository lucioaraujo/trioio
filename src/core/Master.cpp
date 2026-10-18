#include "Master.h"

#include <algorithm>
#include <cmath>
#include <deque>

namespace trioio::perception
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    double run(double x)
    {
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }
};

// RBJ: passa-altas e prateleira alta
Biquad highpass(double fc, double q, double sr)
{
    const double w = 2.0 * kPi * fc / sr, c = std::cos(w), alpha = std::sin(w) / (2.0 * q), a0 = 1.0 + alpha;
    Biquad b;
    b.b0 = (1.0 + c) * 0.5 / a0;
    b.b1 = -(1.0 + c) / a0;
    b.b2 = b.b0;
    b.a1 = -2.0 * c / a0;
    b.a2 = (1.0 - alpha) / a0;
    return b;
}

Biquad highShelf(double fc, double gainDb, double q, double sr)
{
    const double A = std::pow(10.0, gainDb / 40.0), w = 2.0 * kPi * fc / sr, c = std::cos(w), s = std::sin(w);
    const double alpha = s / (2.0 * q), sq = 2.0 * std::sqrt(A) * alpha;
    const double a0 = (A + 1.0) - (A - 1.0) * c + sq;
    Biquad b;
    b.b0 = A * ((A + 1.0) + (A - 1.0) * c + sq) / a0;
    b.b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * c) / a0;
    b.b2 = A * ((A + 1.0) + (A - 1.0) * c - sq) / a0;
    b.a1 = 2.0 * ((A - 1.0) - (A + 1.0) * c) / a0;
    b.a2 = ((A + 1.0) - (A - 1.0) * c - sq) / a0;
    return b;
}

// a ponderação K (BS.1770): prateleira de +4 dB acima de ~1,5 kHz e o
// passa-altas RLB em ~38 Hz (os parâmetros do pyloudnorm, válidos para
// qualquer taxa; a 48 kHz batem com os coeficientes da norma em ~0,01 dB)
std::vector<double> kWeighted(const std::vector<float>& x, double sr)
{
    auto shelf = highShelf(1681.97, 3.99984, 0.7071752, sr);
    auto hp = highpass(38.13547, 0.5003270, sr);
    if (std::abs(sr - 48000.0) < 0.5)
    {
        // os coeficientes da própria norma a 48 kHz (a taxa de saída do Trioio)
        shelf = Biquad {};
        shelf.b0 = 1.53512485958697; shelf.b1 = -2.69169618940638; shelf.b2 = 1.19839281085285;
        shelf.a1 = -1.69065929318241; shelf.a2 = 0.73248077421585;
        hp = Biquad {};
        hp.b0 = 1.0; hp.b1 = -2.0; hp.b2 = 1.0;
        hp.a1 = -1.99004745483398; hp.a2 = 0.99007225036621;
    }
    std::vector<double> y(x.size());
    for (std::size_t i = 0; i < x.size(); ++i)
        y[i] = hp.run(shelf.run(x[i]));
    return y;
}

double toDb(double g) { return 20.0 * std::log10(std::max(g, 1e-12)); }

// 4× sobreamostragem por sinc janelado (Blackman, 12 lóbulos por fase): o
// pico entre amostras, para o pico verdadeiro (BS.1770 anexo 2)
struct Upsampler4
{
    static constexpr int kTaps = 12;  // por fase
    double h[4][2 * kTaps] {};
    Upsampler4()
    {
        for (int p = 0; p < 4; ++p)
            for (int k = 0; k < 2 * kTaps; ++k)
            {
                const double t = (k - kTaps + 1) - p / 4.0;  // distância em amostras de entrada
                const double sinc = std::abs(t) < 1e-12 ? 1.0 : std::sin(kPi * t) / (kPi * t);
                const double u = (t + kTaps) / (2.0 * kTaps);  // 0..1 na janela
                const double w = 0.42 - 0.5 * std::cos(2.0 * kPi * u) + 0.08 * std::cos(4.0 * kPi * u);
                h[p][k] = sinc * (u > 0.0 && u < 1.0 ? w : 0.0);
            }
    }
    // o maior valor absoluto entre x[i] e x[i+1], nas 4 fases
    double peakAround(const std::vector<float>& x, std::size_t i) const
    {
        double m = std::abs(static_cast<double>(x[i]));
        for (int p = 1; p < 4; ++p)
        {
            double s = 0.0;
            for (int k = 0; k < 2 * kTaps; ++k)
            {
                const long j = static_cast<long>(i) + (k - kTaps + 1);
                if (j >= 0 && j < static_cast<long>(x.size()))
                    s += h[p][k] * x[static_cast<std::size_t>(j)];
            }
            m = std::max(m, std::abs(s));
        }
        return m;
    }
};

const Upsampler4& upsampler()
{
    static const Upsampler4 u;
    return u;
}

// pico verdadeiro amostra a amostra (o maior dos dois canais)
std::vector<double> truePeakEnvelope(const std::vector<float>& l, const std::vector<float>& r)
{
    const auto& up = upsampler();
    std::vector<double> e(l.size());
    for (std::size_t i = 0; i < l.size(); ++i)
    {
        // só vale sobreamostrar perto de picos: abaixo de −6 dB do teto não importa
        const double raw = std::max(std::abs(static_cast<double>(l[i])), std::abs(static_cast<double>(r[i])));
        e[i] = raw < 0.25 ? raw : std::max(up.peakAround(l, i), up.peakAround(r, i));
    }
    return e;
}
} // namespace

double integratedLoudness(const std::vector<float>& left, const std::vector<float>& right, double sr)
{
    const auto l = kWeighted(left, sr), r = kWeighted(right, sr);
    const auto block = static_cast<std::size_t>(0.4 * sr), hop = static_cast<std::size_t>(0.1 * sr);
    if (l.size() < block)
        return -120.0;
    std::vector<double> z;  // energia média de cada bloco (soma dos canais)
    for (std::size_t s = 0; s + block <= l.size(); s += hop)
    {
        double e = 0.0;
        for (std::size_t i = s; i < s + block; ++i)
            e += l[i] * l[i] + r[i] * r[i];
        z.push_back(e / static_cast<double>(block));
    }
    const auto lufs = [](double e) { return -0.691 + 10.0 * std::log10(std::max(e, 1e-12)); };
    double sum = 0.0;
    int n = 0;
    for (double e : z)
        if (lufs(e) > -70.0)
        {
            sum += e;
            ++n;
        }
    if (n == 0)
        return -120.0;
    const double relative = lufs(sum / n) - 10.0;
    sum = 0.0;
    n = 0;
    for (double e : z)
        if (lufs(e) > -70.0 && lufs(e) > relative)
        {
            sum += e;
            ++n;
        }
    return n == 0 ? -120.0 : lufs(sum / n);
}

double truePeakDb(const std::vector<float>& left, const std::vector<float>& right, double /*sampleRate*/)
{
    double m = 0.0;
    for (double v : truePeakEnvelope(left, right))
        m = std::max(m, v);
    return toDb(m);
}

MasterReport applyMaster(std::vector<float>& left, std::vector<float>& right, double sr,
                         std::vector<std::pair<std::vector<float>*, std::vector<float>*>> stems, const MasterSettings& s)
{
    MasterReport rep;
    const std::size_t n = std::min(left.size(), right.size());
    if (n == 0)
        return rep;
    rep.lufsBefore = integratedLoudness(left, right, sr);
    rep.truePeakBefore = truePeakDb(left, right, sr);

    // 1. passa-altas (linear: na mistura e em cada stem, a soma se mantém)
    const auto hp = [&](std::vector<float>& x) {
        auto f1 = highpass(s.highPassHz, 0.7071, sr);
        for (auto& v : x)
            v = static_cast<float>(f1.run(v));
    };
    hp(left);
    hp(right);
    for (auto& [l, r] : stems)
    {
        hp(*l);
        hp(*r);
    }

    // 2. a cola: detector de potência (ligado nos dois canais), joelho suave.
    //    O limiar é relativo à peça: a potência média de L e R fica ~3 dB
    //    abaixo da loudness (que soma os canais), então limiar = LUFS − 3 + acima
    std::vector<double> gain(n, 1.0);
    const double lufsHp = integratedLoudness(left, right, sr);
    const double threshold = (lufsHp > -100.0 ? lufsHp : -20.0) - 3.0 + s.compAboveLoudnessDb;
    rep.compThresholdDb = threshold;
    {
        const double att = std::exp(-1.0 / (s.compAttackMs * 1e-3 * sr)), rel = std::exp(-1.0 / (s.compReleaseMs * 1e-3 * sr));
        double env = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            const double p = 0.5 * (static_cast<double>(left[i]) * left[i] + static_cast<double>(right[i]) * right[i]);
            env = p > env ? att * env + (1.0 - att) * p : rel * env + (1.0 - rel) * p;
            const double level = 10.0 * std::log10(std::max(env, 1e-12));
            const double over = level - threshold, k = s.compKneeDb * 0.5;
            double red = 0.0;  // dB
            if (over > k)
                red = over * (1.0 - 1.0 / s.compRatio);
            else if (over > -k)
                red = (1.0 - 1.0 / s.compRatio) * (over + k) * (over + k) / (4.0 * k);
            gain[i] = std::pow(10.0, -red / 20.0);
            rep.compressionMaxDb = std::max(rep.compressionMaxDb, red);
        }
    }

    // 3 e 4. o alvo de loudness e o limitador de pico verdadeiro. O limitador
    //    corta os picos e a peça cai abaixo do alvo: mede de novo e completa o
    //    ganho (até 3 vezes), sem deixar o limitador cortar mais de 8 dB
    const std::vector<double> comp = gain;
    const double ceiling = std::pow(10.0, s.ceilingDbtp / 20.0);
    const auto look = std::max<std::size_t>(1, static_cast<std::size_t>(s.limiterLookaheadMs * 1e-3 * sr));
    const double relCoef = 1.0 - std::exp(-1.0 / (s.limiterReleaseMs * 1e-3 * sr));
    const auto withGain = [&](const std::vector<double>& g, std::vector<float>& tl, std::vector<float>& tr) {
        tl.resize(n);
        tr.resize(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            tl[i] = static_cast<float>(left[i] * g[i]);
            tr[i] = static_cast<float>(right[i] * g[i]);
        }
    };
    // o limitador sobre a curva `g` (modificada no lugar); devolve a maior redução
    const auto limit = [&](std::vector<double>& g) {
        double most = 0.0;
        for (int pass = 0; pass < 2; ++pass)
        {
            std::vector<float> tl, tr;
            withGain(g, tl, tr);
            const auto tp = truePeakEnvelope(tl, tr);
            const double margin = pass == 0 ? 1.0 : 0.985;
            std::vector<double> want(n, 1.0);
            bool any = false;
            for (std::size_t i = 0; i < n; ++i)
                if (tp[i] > ceiling * margin)
                {
                    want[i] = ceiling * margin / tp[i];
                    any = true;
                }
            if (!any)
                break;
            // o mínimo em [i, i + look] (fila monotônica)
            std::vector<double> mn(n);
            std::deque<std::size_t> dq;
            for (std::size_t j = n; j-- > 0;)
            {
                while (!dq.empty() && want[dq.back()] >= want[j])
                    dq.pop_back();
                dq.push_back(j);
                while (dq.front() > j + look)
                    dq.pop_front();
                mn[j] = want[dq.front()];
            }
            // a média em [i − look, i]: desce em rampa e chega ao mínimo no pico
            double acc = 0.0, lg = 1.0;
            for (std::size_t i = 0; i < n; ++i)
            {
                acc += mn[i];
                if (i >= look + 1)
                    acc -= mn[i - look - 1];
                const double avg = std::min(acc / static_cast<double>(std::min(i + 1, look + 1)), mn[i]);
                // a soltura: volta para 1 devagar, nunca acima do pedido
                lg = std::min(avg, lg + (1.0 - lg) * relCoef);
                g[i] *= lg;
                most = std::max(most, -toDb(lg));
            }
        }
        return most;
    };
    std::vector<float> tl, tr;
    withGain(comp, tl, tr);
    const double afterComp = integratedLoudness(tl, tr, sr);
    double loudDb = afterComp > -100.0 ? std::min(s.maxGainDb, s.targetLufs - afterComp) : 0.0;
    std::vector<double> kept;
    double keptLimiter = 0.0, keptLoud = 0.0;
    for (int iter = 0; iter < 3; ++iter)
    {
        gain = comp;
        const double g = std::pow(10.0, loudDb / 20.0);
        for (auto& v : gain)
            v *= g;
        const double most = limit(gain);
        // passou de 8 dB de corte para chegar ao alvo: fica a tentativa
        // anterior (a dinâmica vale mais que o alvo)
        if (iter > 0 && most > 8.0)
        {
            gain = kept;
            break;
        }
        kept = gain;
        keptLimiter = most;
        keptLoud = loudDb;
        withGain(gain, tl, tr);
        const double deficit = s.targetLufs - integratedLoudness(tl, tr, sr);
        if (deficit < 0.3 || loudDb + deficit > s.maxGainDb || afterComp <= -100.0)
            break;
        loudDb += deficit;
    }
    rep.limiterMaxDb = keptLimiter;
    rep.loudnessGainDb = keptLoud;

    // 5. aplicar a curva: a mistura e os stems, a mesma em todos
    for (std::size_t i = 0; i < n; ++i)
    {
        left[i] = static_cast<float>(left[i] * gain[i]);
        right[i] = static_cast<float>(right[i] * gain[i]);
    }
    for (auto& [l, r] : stems)
        for (std::size_t i = 0; i < n && i < l->size() && i < r->size(); ++i)
        {
            (*l)[i] = static_cast<float>((*l)[i] * gain[i]);
            (*r)[i] = static_cast<float>((*r)[i] * gain[i]);
        }
    rep.lufsAfter = integratedLoudness(left, right, sr);
    rep.truePeakAfter = truePeakDb(left, right, sr);
    return rep;
}

} // namespace trioio::perception
