#include "Dramaturgy.h"
#include <cstdlib>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trioio::composition
{
using strategy::Mode;

namespace
{
constexpr double kPi = 3.14159265358979323846;

std::uint64_t mix(std::uint64_t x)
{
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

double uniform(std::uint64_t& s)
{
    s = mix(s);
    return static_cast<double>(s >> 11) / 9007199254740992.0;
}

std::string clock(double s)
{
    char b[16];
    std::snprintf(b, sizeof b, "%d:%02d", static_cast<int>(s) / 60, static_cast<int>(s) % 60);
    return b;
}
} // namespace

float TensionCurve::at(double t) const
{
    if (value.empty())
        return 0.5f;
    const double x = std::clamp(t / step, 0.0, static_cast<double>(value.size() - 1));
    const auto i = static_cast<std::size_t>(x);
    const auto j = std::min(i + 1, value.size() - 1);
    const double f = x - static_cast<double>(i);
    return static_cast<float>(value[i] * (1.0 - f) + value[j] * f);
}

TensionCurve tensionCurve(const std::vector<perception::Frame>& frames, double duration, Mode mode, std::uint64_t seed)
{
    TensionCurve c;
    if (duration <= 0.0)
        return c;
    const auto n = static_cast<std::size_t>(std::ceil(duration / c.step)) + 1;
    // a energia da referência por ponto, suavizada em ±4 s e levada a 0..1
    // pelos percentis 10 e 90 (os extremos não decidem a escala)
    std::vector<double> e(n, 0.0);
    std::vector<int> cnt(n, 0);
    for (const auto& f : frames)
    {
        const auto i = static_cast<std::size_t>(std::clamp(f.time / c.step, 0.0, static_cast<double>(n - 1)));
        e[i] += std::clamp((f.rmsDb + 60.0) / 60.0, 0.0, 1.0);
        ++cnt[i];
    }
    for (std::size_t i = 0; i < n; ++i)
        e[i] = cnt[i] > 0 ? e[i] / cnt[i] : 0.0;
    const auto half = static_cast<long>(4.0 / c.step);
    std::vector<double> sm(n, 0.0);
    for (long i = 0; i < static_cast<long>(n); ++i)
    {
        double s = 0.0;
        int k = 0;
        for (long j = std::max(0L, i - half); j <= std::min(static_cast<long>(n) - 1, i + half); ++j, ++k)
            s += e[static_cast<std::size_t>(j)];
        sm[static_cast<std::size_t>(i)] = k > 0 ? s / k : 0.0;
    }
    auto sorted = sm;
    std::sort(sorted.begin(), sorted.end());
    const double lo = sorted[sorted.size() / 10], hi = sorted[sorted.size() * 9 / 10];
    std::vector<double> ref(n);
    for (std::size_t i = 0; i < n; ++i)
        ref[i] = hi > lo + 1e-6 ? std::clamp((sm[i] - lo) / (hi - lo), 0.0, 1.0) : 0.5;

    c.value.resize(n);
    switch (mode)
    {
        case Mode::follow:
            c.shape = "a energia da referência";
            for (std::size_t i = 0; i < n; ++i)
                c.value[i] = static_cast<float>(ref[i]);
            break;
        case Mode::counter:
            c.shape = "a energia da referência invertida";
            for (std::size_t i = 0; i < n; ++i)
                c.value[i] = static_cast<float>(1.0 - ref[i]);
            break;
        case Mode::transfigure:
        {
            // o arco: sobe de 0,25 até 1 na seção áurea e desce até 0,15
            c.shape = "arco (clímax na seção áurea)";
            const double peak = 0.618 * duration;
            for (std::size_t i = 0; i < n; ++i)
            {
                const double t = static_cast<double>(i) * c.step;
                const double v = t < peak ? 0.25 + 0.75 * 0.5 * (1.0 - std::cos(kPi * t / peak))
                                          : 0.15 + 0.85 * 0.5 * (1.0 + std::cos(kPi * (t - peak) / std::max(1.0, duration - peak)));
                c.value[i] = static_cast<float>(v);
            }
            break;
        }
        case Mode::interrupt:
        {
            // a curva da referência com 2 ou 3 cortes de 4 a 8 s (a tensão cai a 0,1)
            c.shape = "a energia da referência, com cortes";
            for (std::size_t i = 0; i < n; ++i)
                c.value[i] = static_cast<float>(ref[i]);
            std::uint64_t s = seed ^ 0xD1A7u;
            const int cuts = duration >= 60.0 ? 2 + static_cast<int>(uniform(s) * 2.0) : 1;
            for (int k = 0; k < cuts; ++k)
            {
                const double a = (0.15 + 0.7 * uniform(s)) * duration, len = 4.0 + 4.0 * uniform(s);
                for (std::size_t i = 0; i < n; ++i)
                {
                    const double t = static_cast<double>(i) * c.step;
                    if (t >= a && t < a + len)
                        c.value[i] = 0.1f;
                }
            }
            break;
        }
    }
    // o clímax: claro (a curva chega a ≥ 0,75) e não colado nas pontas. O
    // instante vem da energia suavizada SEM o corte dos percentis (o corte
    // achata o topo num platô e o primeiro ponto dele não é o pico): o
    // máximo dela em seguir e interromper (fora dos cortes), o mínimo em
    // contrapor; no arco, o próprio pico
    std::size_t best = 0;
    if (mode == Mode::transfigure)
        best = static_cast<std::size_t>(std::max_element(c.value.begin(), c.value.end()) - c.value.begin());
    else
    {
        double bestV = -1e9;
        for (std::size_t i = 0; i < n; ++i)
        {
            if (c.value[i] <= 0.1f + 1e-6f && mode == Mode::interrupt)
                continue;
            const double v = mode == Mode::counter ? -sm[i] : sm[i];
            if (v > bestV)
            {
                bestV = v;
                best = i;
            }
        }
    }
    const double tc = static_cast<double>(best) * c.step;
    if (c.value[best] >= 0.75f && tc >= 0.15 * duration && tc <= 0.9 * duration)
        c.climax = tc;
    return c;
}

std::string applyDramaturgy(Score& score, const TensionCurve& curve, const perception::Pulse& pulse, std::uint64_t seed,
                            std::pair<double, double>* breath)
{
    if (breath)
        *breath = { -1.0, -1.0 };
    if (curve.value.empty() || score.notes.empty())
        return {};
    // 1. a dinâmica: 0,85–1,15× pela tensão; com o gesto do autor (uma
    //    intenção explícita), 0,6–1,4×, e os vales rareiam mais
    // o gesto e as gramáticas narrativas são escolhas explícitas: a faixa larga
    const bool authored = curve.shape == "o gesto do autor" || curve.shape.rfind("a gramática", 0) == 0 || curve.shape.rfind("as gramáticas", 0) == 0;
    const float lo = authored ? 0.6f : 0.85f, span = authored ? 0.8f : 0.3f;
    for (auto& n : score.notes)
    {
        const float c = curve.at(n.time);
        n.velocity = std::clamp(n.velocity * (lo + span * c), 0.0f, 1.0f);
        // a curva explícita IMPÕE o nível (2026-10-18: só escalar deixava a
        // forma da referência mandar, e as gramáticas soavam iguais): metade
        // do caminho até o nível da curva, os acentos relativos ficam
        if (authored)
            n.velocity = std::clamp(0.5f * n.velocity + 0.5f * (0.1f + 0.85f * c), 0.0f, 1.0f);
    }
    // 2. a densidade: nos vales (tensão < 0,35) as vozes de enfeite rareiam,
    //    até metade das notas no fundo do vale
    std::uint64_t s = seed ^ 0xD2A3u;
    const auto before = score.notes.size();
    score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                     [&](const Note& n) {
                                         if (n.voice != 2 && n.voice != 3 && n.voice != 5)
                                             return false;
                                         const float t = curve.at(n.time);
                                         const double deep = authored ? 0.8 : 0.5;
                                         const double p = t < 0.35f ? (0.35 - t) / 0.35 * deep : 0.0;
                                         return uniform(s) < p;
                                     }),
                      score.notes.end());
    const auto thinned = before - score.notes.size();
    // 2b. o registro: no clímax a voz do motivo sobe uma oitava (uma janela
    //     contínua em torno do pico, 10 % da peça; teto Dó6), nos vales
    //     profundos (tensão < 0,15) desce uma (piso Dó3). Janelas contínuas: a
    //     mudança é de orquestração, uma vez na entrada e na saída
    int up = 0, down = 0;
    const double win = 0.05 * score.duration;
    for (auto& n : score.notes)
    {
        if (n.voice != 0)
            continue;
        if (curve.climax > 0.0 && std::abs(n.time - curve.climax) <= win && n.pitch + 12.0f <= 84.0f)
        {
            n.pitch += 12.0f;
            ++up;
        }
        else if (curve.at(n.time) < 0.15f && n.pitch - 12.0f >= 48.0f)
        {
            n.pitch -= 12.0f;
            ++down;
        }
    }
    // 3. o silêncio composto: uma respiração de um tempo logo antes do clímax
    //    (a peça para; o clímax chega depois do vazio)
    std::string rest;
    if (curve.climax > 0.0 && score.duration >= 60.0)
    {
        double tc = curve.climax;
        double len = 0.8;
        if (pulse.period > 0.0)
        {
            // no tempo mais próximo, e a respiração dura um tempo
            tc = pulse.phase + std::round((tc - pulse.phase) / pulse.period) * pulse.period;
            len = std::clamp(pulse.period, 0.4, 1.2);
        }
        const double a = tc - len;
        if (a > 0.0)
        {
            score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                             [&](const Note& n) { return n.time >= a && n.time < tc; }),
                              score.notes.end());
            for (auto& n : score.notes)
                if (n.time < a && n.time + n.duration > a)
                    n.duration = std::max(0.05, a - n.time);
            if (breath)
                *breath = { a, tc };
            rest = " · respiração de " + std::to_string(static_cast<int>(std::lround(len * 1000.0))) + " ms antes do clímax em "
                 + clock(tc);
        }
    }
    else if (curve.climax > 0.0)
        rest = " · clímax em " + clock(curve.climax);
    char line[320];
    std::snprintf(line, sizeof line,
                  "dramaturgia: %s · dinâmica %s pela tensão · %zu notas de enfeite rareadas nos vales · o motivo sobe uma oitava no clímax (%d notas) e desce nos vales (%d)",
                  curve.shape.c_str(), authored ? "0,6–1,4×" : "0,85–1,15×", thinned, up, down);
    return line + rest;
}

TensionCurve gestureCurve(const std::vector<std::pair<float, float>>& pointsIn, double duration)
{
    TensionCurve c;
    c.shape = "o gesto do autor";
    auto points = pointsIn;
    std::sort(points.begin(), points.end());
    const auto n = static_cast<std::size_t>(std::max(1.0, std::ceil(duration / c.step)));
    c.value.assign(n, 0.5f);
    if (points.empty())
        return c;
    for (std::size_t i = 0; i < n; ++i)
    {
        const float x = static_cast<float>((static_cast<double>(i) + 0.5) * c.step / std::max(duration, 1e-6));
        float v = points.front().second;
        if (x >= points.back().first)
            v = points.back().second;
        else
            for (std::size_t k = 1; k < points.size(); ++k)
                if (x <= points[k].first)
                {
                    const float a = points[k - 1].first, b = points[k].first;
                    const float u = b > a ? (x - a) / (b - a) : 1.0f;
                    v = points[k - 1].second + u * (points[k].second - points[k - 1].second);
                    break;
                }
        c.value[i] = std::clamp(v, 0.0f, 1.0f);
    }
    auto sorted = c.value;
    std::nth_element(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(sorted.size() / 2), sorted.end());
    const float median = sorted[sorted.size() / 2];
    const auto peak = std::max_element(c.value.begin(), c.value.end());
    if (*peak >= median + 0.2f)
        c.climax = (static_cast<double>(peak - c.value.begin()) + 0.5) * c.step;
    return c;
}

bool parseGesture(const std::string& text, std::vector<std::pair<float, float>>& out)
{
    out.clear();
    std::size_t pos = 0;
    while (pos < text.size())
    {
        auto end = text.find(',', pos);
        if (end == std::string::npos)
            end = text.size();
        const auto item = text.substr(pos, end - pos);
        const auto colon = item.find(':');
        if (colon == std::string::npos)
            return false;
        char* e1 = nullptr;
        char* e2 = nullptr;
        const std::string a = item.substr(0, colon), b = item.substr(colon + 1);
        const float x = std::strtof(a.c_str(), &e1), y = std::strtof(b.c_str(), &e2);
        if (e1 == a.c_str() || e2 == b.c_str() || x < 0.0f || x > 1.0f || y < 0.0f || y > 1.0f)
            return false;
        out.push_back({ x, y });
        pos = end + 1;
    }
    return out.size() >= 2;
}

namespace
{
// a tensão de uma gramática na posição x (0..1 da peça)
double grammarAt(const std::string& g, double x)
{
    const auto lerp = [](double a, double b, double t) { return a + (b - a) * std::clamp(t, 0.0, 1.0); };
    if (g == "freytag")
    {
        if (x < 0.2) return 0.3;                                  // exposição
        if (x < 0.5) return lerp(0.3, 0.85, (x - 0.2) / 0.3);     // ação ascendente
        if (x < 0.6) return 1.0;                                  // clímax
        if (x < 0.85) return lerp(1.0, 0.4, (x - 0.6) / 0.25);    // queda
        return lerp(0.4, 0.2, (x - 0.85) / 0.15);                 // resolução
    }
    if (g == "kishotenketsu")
    {
        if (x < 0.25) return 0.4;                                 // ki: apresenta
        if (x < 0.5) return lerp(0.45, 0.6, (x - 0.25) / 0.25);   // shō: desenvolve
        if (x < 0.75) return 0.9;                                 // ten: a virada
        return lerp(0.6, 0.4, (x - 0.75) / 0.25);                 // ketsu: reconcilia
    }
    if (g == "ondas")
    {
        const double w = std::fmod(x * 3.0, 1.0), k = std::floor(x * 3.0);
        return 0.2 + (0.35 + 0.15 * k) * std::sin(3.14159265358979 * w);
    }
    if (g == "plato")
        return lerp(0.45, 0.65, x);
    // arco: o clímax perto da seção áurea
    return x < 0.62 ? lerp(0.25, 1.0, x / 0.62) : lerp(1.0, 0.3, (x - 0.62) / 0.38);
}

std::vector<std::string> splitGrammars(const std::string& names)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : names + "+")
        if (c == '+')
        {
            const auto a = cur.find_first_not_of(' '), b = cur.find_last_not_of(' ');
            if (a != std::string::npos)
                out.push_back(cur.substr(a, b - a + 1));
            cur.clear();
        }
        else
            cur += c;
    return out;
}
} // namespace

bool isNarrativeGrammar(const std::string& name)
{
    for (const auto& g : splitGrammars(name))
        if (g != "freytag" && g != "kishotenketsu" && g != "ondas" && g != "plato" && g != "arco")
            return false;
    return !splitGrammars(name).empty();
}

TensionCurve narrativeCurve(const std::string& names, double duration)
{
    TensionCurve c;
    const auto gs = splitGrammars(names);
    if (gs.empty() || duration <= 0.0)
        return c;
    c.shape = gs.size() == 1 ? "a gramática " + gs[0] : "as gramáticas " + gs[0] + " → " + gs[1];
    const auto n = static_cast<std::size_t>(std::ceil(duration / c.step)) + 1;
    c.value.resize(n);
    double best = -1.0;
    for (std::size_t i = 0; i < n; ++i)
    {
        const double x = std::min(1.0, static_cast<double>(i) * c.step / duration);
        // a mistura: a primeira pesa no começo, a segunda no fim
        const double v = gs.size() == 1 ? grammarAt(gs[0], x) : (1.0 - x) * grammarAt(gs[0], x) + x * grammarAt(gs[1], x);
        c.value[i] = static_cast<float>(std::clamp(v, 0.0, 1.0));
        if (v > best)
        {
            best = v;
            c.climax = static_cast<double>(i) * c.step;
        }
    }
    return c;
}

std::string applyNarrativeEvents(Score& score, const std::string& names, std::uint64_t seed)
{
    const auto gs = splitGrammars(names);
    if (std::find(gs.begin(), gs.end(), "kishotenketsu") == gs.end() || score.duration <= 0.0)
        return {};
    // a virada (ten): de 50 a 75 % da peça
    const double a = 0.5 * score.duration, b = 0.75 * score.duration;
    Timbre motif = Timbre::additive, counter = Timbre::additive;
    bool hasMotif = false, hasCounter = false;
    for (const auto& n : score.notes)
    {
        if (!hasMotif && n.voice == 0) { motif = n.timbre; hasMotif = true; }
        if (!hasCounter && n.voice == 2) { counter = n.timbre; hasCounter = true; }
    }
    std::uint64_t st = seed ^ 0x7E4u;
    const auto rnd = [&] {
        st ^= st << 13;
        st ^= st >> 7;
        st ^= st << 17;
        return static_cast<double>(st >> 11) / 9007199254740992.0;
    };
    int moved = 0, dropped = 0;
    std::vector<Note> out;
    for (auto n : score.notes)
    {
        if (n.time >= a && n.time < b)
        {
            if (n.voice == 4 && rnd() < 0.5)
            {
                ++dropped;
                continue;
            }
            if (n.voice == 0)
            {
                if (n.pitch + 12.0f <= 96.0f)
                    n.pitch += 12.0f;
                if (hasCounter)
                    n.timbre = counter;
                ++moved;
            }
            else if (n.voice == 2 && hasMotif)
                n.timbre = motif;
        }
        out.push_back(n);
    }
    score.notes = std::move(out);
    score.marks.push_back({ a, "a virada (ten, do ki-shō-ten-ketsu): o motivo uma oitava acima e no timbre do contracanto, a percussão pela metade" });
    char buf[200];
    std::snprintf(buf, sizeof buf, "gramática narrativa: a virada do ki-shō-ten-ketsu (%d notas do motivo deslocadas, %d golpes a menos)", moved, dropped);
    return buf;
}

int shapeGestures(Score& score, const TensionCurve& curve)
{
    if (curve.value.empty())
        return 0;
    const bool authored = curve.shape == "o gesto do autor" || curve.shape.rfind("a gramática", 0) == 0 || curve.shape.rfind("as gramáticas", 0) == 0;
    const float lo = authored ? 0.6f : 0.85f, span = authored ? 0.8f : 0.3f;
    int n = 0;
    for (auto& g : score.gestures)
    {
        const float c = curve.at(g.time + 0.5 * g.duration);
        g.level = std::clamp(g.level * (lo + span * c), 0.0f, 1.0f);
        if (authored)  // metade do caminho até o fator da curva (o meio da curva = 1×)
            g.level = std::clamp(g.level * (0.5f + 0.5f * (0.1f + 0.85f * c) / 0.525f), 0.0f, 1.0f);
        ++n;
    }
    return n;
}

} // namespace trioio::composition
