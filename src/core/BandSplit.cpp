#include "BandSplit.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trioio::perception
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
};

// passa-baixas Butterworth de 2.ª ordem (RBJ, Q = 1/√2)
Biquad lowpass(double fc, double sr)
{
    const double w = 2.0 * kPi * fc / sr, c = std::cos(w), s = std::sin(w);
    const double alpha = s / (2.0 * std::sqrt(0.5));
    const double a0 = 1.0 + alpha;
    Biquad q;
    q.b0 = (1.0 - c) * 0.5 / a0;
    q.b1 = (1.0 - c) / a0;
    q.b2 = q.b0;
    q.a1 = -2.0 * c / a0;
    q.a2 = (1.0 - alpha) / a0;
    return q;
}

void run(const Biquad& q, std::vector<double>& y)
{
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    for (auto& v : y)
    {
        const double x = v;
        const double out = q.b0 * x + q.b1 * x1 + q.b2 * x2 - q.a1 * y1 - q.a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = out;
        v = out;
    }
}

// ida e volta (fase zero) com reflexão nas pontas, para o transitório do
// filtro não morder o começo nem o fim
std::vector<double> filtfilt(const Biquad& q, const std::vector<double>& x, std::size_t pad)
{
    const std::size_t n = x.size();
    pad = std::min(pad, n > 1 ? n - 1 : 0);
    std::vector<double> y;
    y.reserve(n + 2 * pad);
    for (std::size_t i = pad; i >= 1; --i)
        y.push_back(2.0 * x.front() - x[i]);
    y.insert(y.end(), x.begin(), x.end());
    for (std::size_t i = 1; i <= pad; ++i)
        y.push_back(2.0 * x.back() - x[n - 1 - i]);
    run(q, y);
    std::reverse(y.begin(), y.end());
    run(q, y);
    std::reverse(y.begin(), y.end());
    return std::vector<double>(y.begin() + static_cast<std::ptrdiff_t>(pad), y.begin() + static_cast<std::ptrdiff_t>(pad + n));
}

double energy(const std::vector<float>& x)
{
    double e = 0.0;
    for (float v : x)
        e += static_cast<double>(v) * v;
    return e;
}

std::string hz(double f)
{
    char b[32];
    std::snprintf(b, sizeof b, "%.0f Hz", f);
    return b;
}
} // namespace

std::string BandLine::name() const
{
    if (lowHz <= 0.0 && highHz <= 0.0)
        return "a referência inteira";
    if (lowHz <= 0.0)
        return "grave (< " + hz(highHz) + ")";
    if (highHz <= 0.0)
        return "agudo (> " + hz(lowHz) + ")";
    return hz(lowHz).substr(0, hz(lowHz).size() - 3) + "–" + hz(highHz);
}

std::vector<BandLine> splitBands(const std::vector<float>& x, double sampleRate, std::vector<double> cutsHz)
{
    std::sort(cutsHz.begin(), cutsHz.end());
    std::vector<double> cuts;
    for (double c : cutsHz)
        if (c > 20.0 && c < sampleRate * 0.45 && (cuts.empty() || c > cuts.back() * 1.2))
            cuts.push_back(c);
    if (cuts.size() > 4)
        cuts.resize(4);

    std::vector<BandLine> out;
    const double total = energy(x);
    if (cuts.empty() || x.empty())
    {
        BandLine all;
        all.samples = x;
        all.energyShare = 1.0;
        out.push_back(std::move(all));
        return out;
    }
    std::vector<double> rest(x.begin(), x.end());
    double low = 0.0;
    for (double c : cuts)
    {
        // a reflexão cobre alguns períodos do corte (o transitório do filtro)
        const auto pad = static_cast<std::size_t>(std::ceil(8.0 * sampleRate / c));
        const auto band = filtfilt(lowpass(c, sampleRate), rest, pad);
        BandLine line;
        line.lowHz = low;
        line.highHz = c;
        line.samples.resize(x.size());
        for (std::size_t i = 0; i < x.size(); ++i)
        {
            line.samples[i] = static_cast<float>(band[i]);
            rest[i] -= band[i];
        }
        out.push_back(std::move(line));
        low = c;
    }
    BandLine top;
    top.lowHz = low;
    top.samples.resize(x.size());
    for (std::size_t i = 0; i < x.size(); ++i)
        top.samples[i] = static_cast<float>(rest[i]);
    out.push_back(std::move(top));
    // a parte de cada linha na energia. Perto dos cortes as linhas vizinhas
    // levam o mesmo sinal EM FASE (a amplitude soma 1): a soma das partes fica
    // um pouco abaixo de 1 (0,96 com tons a 1,3 oitava do corte)
    for (auto& l : out)
        l.energyShare = total > 0.0 ? energy(l.samples) / total : 0.0;
    return out;
}

std::vector<float> extractBand(const std::vector<float>& x, double sampleRate, std::vector<double> cutsHz, std::size_t index)
{
    // a mesma limpeza dos cortes de splitBands
    std::sort(cutsHz.begin(), cutsHz.end());
    std::vector<double> cuts;
    for (double c : cutsHz)
        if (c > 20.0 && c < sampleRate * 0.45 && (cuts.empty() || c > cuts.back() * 1.2))
            cuts.push_back(c);
    if (cuts.size() > 4)
        cuts.resize(4);
    if (cuts.empty() || index > cuts.size())
        return index == 0 && cuts.empty() ? x : std::vector<float>(x.size(), 0.0f);
    std::vector<double> rest(x.begin(), x.end());
    for (std::size_t k = 0; k < cuts.size(); ++k)
    {
        const auto pad = static_cast<std::size_t>(std::ceil(8.0 * sampleRate / cuts[k]));
        auto band = filtfilt(lowpass(cuts[k], sampleRate), rest, pad);
        if (k == index)
            return std::vector<float>(band.begin(), band.end());
        for (std::size_t i = 0; i < rest.size(); ++i)
            rest[i] -= band[i];
    }
    return std::vector<float>(rest.begin(), rest.end());  // a última
}

float levelMatchGain(const BandLine& line, const std::vector<float>& reference)
{
    const double el = energy(line.samples), er = energy(reference);
    if (el <= 0.0 || er <= 0.0)
        return 1.0f;
    return static_cast<float>(std::min(std::sqrt(er / el), std::pow(10.0, 30.0 / 20.0)));
}

} // namespace trioio::perception
