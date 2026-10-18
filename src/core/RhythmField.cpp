#include "RhythmField.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace trioio::composition
{
using namespace trioio::perception;

namespace
{
struct Rng
{
    std::uint64_t s;
    double uniform()
    {
        s += 0x9E3779B97F4A7C15ull;
        std::uint64_t x = s;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return static_cast<double>((x ^ (x >> 31)) >> 11) / 9007199254740992.0;
    }
};

// um ponto do campo: a posição (densidade, síncope) e o padrão do compasso
struct Point
{
    double x = 0.0, y = 0.0;
    std::vector<double> hits;  // força 0..1 por passo
};

// a família permitida mais próxima da preferida (regras do autor)
Timbre allowed(const Constraints& rules, std::initializer_list<Timbre> prefs)
{
    for (Timbre t : prefs)
        if (!rules.noTimbre[static_cast<std::size_t>(t)])
            return t;
    for (int k = 0; k < kTimbreCount; ++k)
        if (!rules.noTimbre[static_cast<std::size_t>(k)])
            return static_cast<Timbre>(k);
    return Timbre::additive;
}
} // namespace

void rhythmField(Score& score, const std::vector<Bar>& bars, const Pulse& pulse,
                 const std::vector<std::pair<double, double>>& pulsed, const Options& opt)
{
    if (bars.size() < 2 || pulsed.empty() || pulse.period <= 0.0)
        return;
    const int steps = bars.front().steps;

    // 1. o campo: cada compasso da referência com ataques vira um ponto
    float maxHit = 1e-6f;
    for (const auto& b : bars)
        for (int s = 0; s < b.steps; ++s)
            maxHit = std::max(maxHit, b.hits[static_cast<std::size_t>(s)]);
    std::vector<Point> field;
    std::vector<double> starts;
    for (const auto& b : bars)
    {
        if (b.steps != steps || b.count() == 0)
            continue;
        Point p;
        p.hits.resize(static_cast<std::size_t>(steps));
        int on = 0, off = 0;
        for (int s = 0; s < steps; ++s)
        {
            const double h = b.hits[static_cast<std::size_t>(s)] / maxHit;
            p.hits[static_cast<std::size_t>(s)] = h;
            if (h > 0.0)
                (s % kStepsPerBeat == 0 ? on : off)++;
        }
        p.x = static_cast<double>(on + off) / steps;
        p.y = on + off > 0 ? static_cast<double>(off) / (on + off) : 0.0;
        field.push_back(std::move(p));
        starts.push_back(b.start);
    }
    if (field.size() < 2)
        return;

    // 2. o caminho da resposta pelo campo, compasso a compasso
    Rng rng { opt.seed * 0xD6E8FEB86659FD93ull + 0x6121D5 };
    const float u = std::clamp(opt.unpredictability, 0.0f, 1.0f);
    const double barLen = pulse.period * pulse.beatsPerBar, stepDur = pulse.period / kStepsPerBeat;
    // as vozes da camada: grave sempre tambor (ou a permitida mais próxima);
    // a aguda muda de peça para peça
    // desde 2026-10-14 também as percussões acústicas (docs/MAQUINA_DE_TIMBRES.md),
    // num sorteio próprio, para não mudar o resto do caminho
    Rng kit { opt.seed * 0x9E3779B97F4A7C15ull + 0x5A3D0 };
    const double pickLow = kit.uniform(), pickKit = kit.uniform();
    const Timbre low = pickLow < 0.4 ? allowed(opt.constraints, { Timbre::drum, Timbre::membrane, Timbre::lpg })
                     : pickLow < 0.7 ? allowed(opt.constraints, { Timbre::surdo, Timbre::drum, Timbre::membrane })
                                     : allowed(opt.constraints, { Timbre::conga, Timbre::drum, Timbre::membrane });
    const double pickHigh = rng.uniform();
    const Timbre high = pickKit < 0.3 ? allowed(opt.constraints, { Timbre::pandeiro, Timbre::striker, Timbre::lpg })
                      : pickKit < 0.5 ? allowed(opt.constraints, { Timbre::shaker, Timbre::striker, Timbre::lpg })
                      : pickHigh < 0.4 ? allowed(opt.constraints, { Timbre::striker, Timbre::lpg, Timbre::plucked })
                      : pickHigh < 0.75 ? allowed(opt.constraints, { Timbre::lpg, Timbre::striker, Timbre::plucked })
                                        : allowed(opt.constraints, { Timbre::plucked, Timbre::waveguide, Timbre::lpg });
    const float lowPitch = 36.0f + static_cast<float>(static_cast<int>(rng.uniform() * 7.0));
    const float highPitch = 74.0f + static_cast<float>(static_cast<int>(rng.uniform() * 10.0));
    int written = 0, barsUsed = 0;
    double sumX = 0.0, sumY = 0.0;
    double driftX = 0.0, driftY = 0.0;
    int barIndex = 0;
    for (double tb = pulse.phase; tb + barLen <= score.duration - 0.2; tb += barLen, ++barIndex)
    {
        const bool inPulse = std::any_of(pulsed.begin(), pulsed.end(), [&](const auto& r) { return tb >= r.first && tb + barLen <= r.second + 1e-6; });
        if (!inPulse)
            continue;
        // interromper: um compasso em cada quatro fica vazio
        if (opt.mode == strategy::Mode::interrupt && barIndex % 4 == 3)
            continue;
        // a posição de agora: o compasso da referência neste instante...
        std::size_t near = 0;
        for (std::size_t k = 1; k < starts.size(); ++k)
            if (std::abs(starts[k] - tb) < std::abs(starts[near] - tb))
                near = k;
        double x = field[near].x, y = field[near].y;
        // ...transformado pelo modo
        if (opt.mode == strategy::Mode::transfigure)
            y = 1.0 - y;   // a síncope espelhada
        if (opt.mode == strategy::Mode::counter)
            x = 1.0 - x;   // densidade complementar
        // ...e desviado: um passeio que a imprevisibilidade solta
        driftX = 0.7 * driftX + (rng.uniform() - 0.5) * 0.5 * u;
        driftY = 0.7 * driftY + (rng.uniform() - 0.5) * 0.5 * u;
        x = std::clamp(x + driftX, 0.0, 1.0);
        y = std::clamp(y + driftY, 0.0, 1.0);
        sumX += x;
        sumY += y;
        ++barsUsed;

        // 3. as probabilidades: os 4 padrões mais próximos, pesados pela distância
        std::vector<std::pair<double, std::size_t>> d;
        for (std::size_t k = 0; k < field.size(); ++k)
            d.push_back({ std::hypot(field[k].x - x, field[k].y - y), k });
        std::sort(d.begin(), d.end());
        if (d.size() > 4)
            d.resize(4);
        std::vector<double> prob(static_cast<std::size_t>(steps), 0.0);
        double wsum = 0.0, mixY = 0.0;
        for (const auto& [dist, k] : d)
        {
            const double w = 1.0 / (dist + 0.05);
            wsum += w;
            mixY += w * field[k].y;
            for (int s = 0; s < steps; ++s)
                prob[static_cast<std::size_t>(s)] += w * field[k].hits[static_cast<std::size_t>(s)];
        }
        mixY /= wsum;
        // A posição pede mais (ou menos) síncope do que os padrões vizinhos
        // têm: desloca golpes — do tempo para o contratempo (+2 passos), ou
        // do contratempo para o tempo mais próximo — na proporção da
        // diferença. Sem isso, numa referência de compassos iguais, mover a
        // posição não mudava o som.
        const double dy = y - mixY;
        if (std::abs(dy) > 0.05)
        {
            std::vector<double> moved(prob.size(), 0.0);
            const double share = std::clamp(std::abs(dy) * 1.5, 0.0, 0.9);
            for (int s = 0; s < steps; ++s)
            {
                const double h = prob[static_cast<std::size_t>(s)];
                const bool onBeat = s % kStepsPerBeat == 0;
                int to = s;
                if (dy > 0.0 && onBeat)
                    to = (s + 2) % steps;
                else if (dy < 0.0 && !onBeat)
                    to = ((s + kStepsPerBeat / 2) / kStepsPerBeat * kStepsPerBeat) % steps;
                moved[static_cast<std::size_t>(s)] += h * (to == s ? 1.0 : 1.0 - share);
                if (to != s)
                    moved[static_cast<std::size_t>(to)] += h * share;
            }
            prob.swap(moved);
        }
        // 4. os golpes: a densidade da posição escala a chance de cada passo
        const double scale = 0.5 + 1.0 * x;
        for (int s = 0; s < steps; ++s)
        {
            const double p = std::clamp(prob[static_cast<std::size_t>(s)] / wsum * scale, 0.0, 0.95);
            if (p <= 0.02 || rng.uniform() >= p)
                continue;
            const bool onBeat = s % kStepsPerBeat == 0;
            Note n;
            n.time = tb + s * stepDur;
            n.duration = stepDur * (onBeat ? 2.0 : 1.2);
            n.voice = 4;
            n.velocity = static_cast<float>(std::clamp(0.3 + 0.55 * p, 0.2, 0.9));
            // no tempo e forte: grave; fora do tempo ou fraco: agudo
            const bool grave = onBeat && p >= 0.35;
            n.timbre = grave ? low : high;
            n.pitch = grave ? lowPitch : highPitch + (s % 3 == 2 ? 2.0f : 0.0f);
            if (n.time + n.duration < score.duration - 0.1)
            {
                score.notes.push_back(n);
                ++written;
            }
        }
    }
    if (written == 0)
        return;
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) {
        return a.time < b.time || (a.time == b.time && a.voice < b.voice);
    });
    char text[220];
    std::snprintf(text, sizeof text,
                  "campo rítmico: %d compassos, %d golpes · posição média: densidade %.2f, síncope %.2f · %s/%s",
                  barsUsed, written, sumX / barsUsed, sumY / barsUsed, timbreName(low), timbreName(high));
    // na posição do tempo, para a forma continuar em ordem
    const Mark mark { pulsed.front().first, text };
    score.marks.insert(std::upper_bound(score.marks.begin(), score.marks.end(), mark,
                                        [](const Mark& a, const Mark& b) { return a.time < b.time; }),
                       mark);
}

} // namespace trioio::composition
