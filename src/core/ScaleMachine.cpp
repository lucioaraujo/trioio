#include "ScaleMachine.h"

#include "ScaleAffect.h"
#include "ScaleArchive.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace trioio::composition
{
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
    int below(int n) { return static_cast<int>(uniform() * n) % std::max(1, n); }
};

double wrap(double x, double p) { return std::fmod(std::fmod(x, p) + p, p); }

void tidy(MachineScale& s)
{
    for (auto& c : s.cents)
        c = wrap(c, s.period);
    std::sort(s.cents.begin(), s.cents.end());
    s.cents.erase(std::unique(s.cents.begin(), s.cents.end(), [](double a, double b) { return std::abs(a - b) < 0.5; }),
                  s.cents.end());
    if (s.cents.empty() || s.cents.front() > 0.5)
        s.cents.insert(s.cents.begin(), 0.0);
    s.cents.front() = 0.0;
    s.tonic = wrap(s.tonic, 1200.0);
}

// a altura absoluta (cents acima de Dó, módulo 1200) de cada grau
std::vector<double> absolute(const MachineScale& s)
{
    std::vector<double> out;
    for (double c : s.cents)
        out.push_back(wrap(s.tonic + c, 1200.0));
    return out;
}

const char* ordinal(int k)
{
    static const char* const n[] = { "1.º", "2.º", "3.º", "4.º", "5.º", "6.º", "7.º", "8.º", "9.º", "10.º", "11.º", "12.º" };
    return k >= 0 && k < 12 ? n[k] : "?";
}

const char* const kPc[12] = { "Dó", "Dó#", "Ré", "Mib", "Mi", "Fá", "Fá#", "Sol", "Lá♭", "Lá", "Sib", "Si" };

std::string tonicName(double cents)
{
    const int pc = static_cast<int>(std::lround(cents / 100.0)) % 12;
    const double off = cents - 100.0 * std::round(cents / 100.0);
    char b[48];
    if (std::abs(off) >= 5.0)
        std::snprintf(b, sizeof b, "%s%+.0fc", kPc[pc], off);
    else
        std::snprintf(b, sizeof b, "%s", kPc[pc]);
    return b;
}

MachineScale fromArchive(const ArchiveScale& a, double tonic)
{
    MachineScale s;
    s.tonic = tonic;
    s.cents = a.stepsCents;
    s.period = a.periodCents;
    s.name = a.name;
    tidy(s);
    return s;
}
} // namespace

MachineScale rotateScale(const MachineScale& s, int degree)
{
    MachineScale r = s;
    const int n = static_cast<int>(s.cents.size());
    const int k = ((degree % n) + n) % n;
    const double base = s.cents[static_cast<std::size_t>(k)];
    for (auto& c : r.cents)
        c = c - base;
    r.tonic = s.tonic + base;
    tidy(r);
    return r;
}

MachineScale alterDegree(const MachineScale& s, int degree, double delta)
{
    MachineScale r = s;
    const int n = static_cast<int>(s.cents.size());
    const int k = std::clamp(degree, 1, n - 1);  // a tônica fica
    r.cents[static_cast<std::size_t>(k)] += delta;
    tidy(r);
    return r;
}

MachineScale mirrorScale(const MachineScale& s)
{
    MachineScale r = s;
    for (auto& c : r.cents)
        c = s.period - c;
    tidy(r);
    return r;
}

MachineScale interpolateScales(const MachineScale& from, const MachineScale& to, double amount)
{
    // cada grau de `from` (em altura absoluta) caminha em direção ao grau mais
    // próximo de `to`; a tônica fica
    MachineScale r = from;
    const auto target = absolute(to);
    for (std::size_t i = 1; i < r.cents.size(); ++i)
    {
        const double here = wrap(from.tonic + from.cents[i], 1200.0);
        double best = 0.0, bestAbs = 1e9;
        for (double t : target)
        {
            double d = t - here;
            while (d > 600.0) d -= 1200.0;
            while (d <= -600.0) d += 1200.0;
            if (std::abs(d) < bestAbs) { bestAbs = std::abs(d); best = d; }
        }
        r.cents[i] += amount * best;
    }
    r.period = from.period + amount * (to.period - from.period);
    tidy(r);
    return r;
}

MachineScale addDegree(const MachineScale& s)
{
    MachineScale r = s;
    double gap = -1.0, at = 0.0;
    for (std::size_t i = 0; i < s.cents.size(); ++i)
    {
        const double next = i + 1 < s.cents.size() ? s.cents[i + 1] : s.period;
        if (next - s.cents[i] > gap) { gap = next - s.cents[i]; at = s.cents[i] + gap / 2.0; }
    }
    r.cents.push_back(at);
    tidy(r);
    return r;
}

MachineScale removeDegree(const MachineScale& s, int degree)
{
    MachineScale r = s;
    if (r.cents.size() <= 4)
        return r;
    const int k = std::clamp(degree, 1, static_cast<int>(r.cents.size()) - 1);
    r.cents.erase(r.cents.begin() + k);
    return r;
}

MachineScale stretchScale(const MachineScale& s, double factor)
{
    MachineScale r = s;
    for (auto& c : r.cents)
        c *= factor;
    r.period *= factor;
    tidy(r);
    return r;
}

MachineScale chordVariant(const MachineScale& s, const std::vector<double>& chord, std::vector<DegreeChange>* changes,
                          double reach)
{
    MachineScale v = s;
    for (double c : chord)
    {
        // a nota do acorde já está na escala (inclusive na tônica)?
        const auto distTo = [&](std::size_t i) {
            double d = wrap(c - (v.tonic + v.cents[i]), 1200.0);
            return d > 600.0 ? d - 1200.0 : d;
        };
        if (std::abs(distTo(0)) <= 5.0)
            continue;
        // o grau vizinho mais próximo, fora a tônica (ela não se move: a
        // variante é da cor, não do centro). Em Lá menor, Sol# fica a 100 c
        // do Sol e do Lá; o Sol é que sobe
        int nearest = -1;
        double bestD = 1e9, bestSigned = 0.0;
        for (std::size_t i = 1; i < v.cents.size(); ++i)
        {
            const double d = distTo(i);
            if (std::abs(d) < bestD) { bestD = std::abs(d); bestSigned = d; nearest = static_cast<int>(i); }
        }
        if (nearest < 0 || bestD <= 5.0 || bestD > reach)
            continue;
        v.cents[static_cast<std::size_t>(nearest)] += bestSigned;
        if (changes)
            changes->push_back({ nearest, bestSigned });
    }
    // sem reordenar: o grau alterado continua sendo o mesmo grau
    return v;
}

int commonTones(const MachineScale& a, const MachineScale& b, double tol)
{
    const auto x = absolute(a), y = absolute(b);
    int n = 0;
    for (double p : x)
        n += std::any_of(y.begin(), y.end(), [&](double q) {
            const double d = std::abs(p - q);
            return std::min(d, 1200.0 - d) <= tol;
        });
    return n;
}

double fieldFit(const MachineScale& s, const std::array<float, 12>& field)
{
    double total = 0.0, covered = 0.0;
    const auto abs = absolute(s);
    for (int p = 0; p < 12; ++p)
    {
        const double w = std::max(0.0f, field[static_cast<std::size_t>(p)]);
        total += w;
        double best = 1e9;
        for (double c : abs)
        {
            const double d = std::abs(c - 100.0 * p);
            best = std::min(best, std::min(d, 1200.0 - d));
        }
        if (best <= 40.0)
            covered += w * (1.0 - 0.5 * best / 40.0);
    }
    return total > 0.0 ? covered / total : 0.0;
}

MachineScale pivotScale(const MachineScale& s, const std::array<float, 12>& field)
{
    // candidatas: a escala transposta para cada um dos seus próprios graus
    // e para cada semitom (se for de oitava)
    std::vector<double> shifts;
    for (double c : s.cents)
        if (c > 0.5) shifts.push_back(c);
    for (int k = 1; k < 12; ++k)
        shifts.push_back(100.0 * k);
    MachineScale best = s;
    double bestScore = -1e9;
    for (double t : shifts)
    {
        MachineScale cand = s;
        cand.tonic = s.tonic + t;
        tidy(cand);
        if (std::abs(wrap(cand.tonic - s.tonic + 600.0, 1200.0) - 600.0) < 1.0)
            continue;
        const double score = commonTones(s, cand) + 2.0 * fieldFit(cand, field);
        if (score > bestScore) { bestScore = score; best = cand; }
    }
    return best;
}

MachineStep nextScale(const MachineScale& prev, const std::array<float, 12>& field, MachineMode mode, std::uint64_t seed,
                      const std::vector<ArchiveScale>* archive, int intentionCode, double position)
{
    const auto intent = static_cast<Intention>(intentionCode);
    const auto away = [&](const MachineScale& s) { return intentionDistance(scaleAffect(s), intent, position); };
    Rng rng { seed * 0xC2B2AE3D27D4EB4Full + 0x5CA1E };
    MachineStep step;
    const int n = static_cast<int>(prev.cents.size());
    char b[200];

    // o grau mais fraco no campo da seção seguinte (para alterar ou suprimir)
    const auto weakest = [&] {
        int k = 1;
        double low = 1e9;
        for (int i = 1; i < n; ++i)
        {
            const int pc = static_cast<int>(std::lround(wrap(prev.tonic + prev.cents[static_cast<std::size_t>(i)], 1200.0) / 100.0)) % 12;
            const double w = field[static_cast<std::size_t>(pc)];
            if (w < low) { low = w; k = i; }
        }
        return k;
    };
    // um destino no arquivo: a escala (e tônica) que melhor veste o campo
    // `blend` > 0: a escala tocada será a interpolação até o destino, e é
    // ela que a intenção avalia (não o destino em si)
    const auto destination = [&](bool farthest, double blend = 0.0) -> MachineScale {
        if (!archive || archive->empty())
        {
            auto m = mirrorScale(prev);
            m.name = "o espelho da escala";
            return m;
        }
        // sorteia primeiro a família, depois a escala dentro dela: senão as
        // famílias geradas em massa (1753 MOS) tomariam quase todo sorteio
        std::vector<std::string> families;
        for (const auto& a : *archive)
            if (std::find(families.begin(), families.end(), a.family) == families.end())
                families.push_back(a.family);
        MachineScale best = prev;
        double bestScore = farthest ? 1e9 : -1e9;
        for (int tries = 0; tries < 60; ++tries)
        {
            const auto& fam = families[static_cast<std::size_t>(rng.below(static_cast<int>(families.size())))];
            std::vector<const ArchiveScale*> inFamily;
            for (const auto& x : *archive)
                if (x.family == fam)
                    inFamily.push_back(&x);
            const auto& a = *inFamily[static_cast<std::size_t>(rng.below(static_cast<int>(inFamily.size())))];
            // do tamanho da escala atual (±2 graus; ±4 no salto): transfigurar
            // muda a cor sem mudar a densidade da melodia
            const int sizeDiff = std::abs(static_cast<int>(a.stepsCents.size()) - n);
            if (std::abs(a.periodCents - 1200.0) > 10.0 || a.stepsCents.size() < 5 || a.stepsCents.size() > 12
                || sizeDiff > (farthest ? 4 : 2))
                continue;
            for (int t = 0; t < 12; ++t)
            {
                const auto cand = fromArchive(a, 100.0 * t);
                // o tamanho da escala deve se aproximar do número de classes
                // importantes do campo (≥ 30 % do pico): senão uma escala de
                // 12 sons "veste" qualquer campo e vence sempre (numa mistura
                // cheia, todas as classes aparecem um pouco)
                float peak = 0.0f;
                for (float w : field) peak = std::max(peak, w);
                int important = 0;
                for (float w : field) important += w >= 0.3f * peak;
                const double fit = fieldFit(cand, field)
                                 - 0.05 * std::abs(static_cast<double>(cand.cents.size()) - std::max(5, std::min(important, n + 2)))
                                 - (intent != Intention::none ? 2.0 * away(blend > 0.0 ? interpolateScales(prev, cand, blend) : cand) : 0.0);
                const double score = farthest ? commonTones(prev, cand) - 2.0 * fit : fit;
                if (farthest ? score < bestScore : score > bestScore) { bestScore = score; best = cand; }
            }
        }
        return best;
    };

    switch (mode)
    {
        case MachineMode::follow:
        {
            if (intent != Intention::none)
            {
                // com intenção: entre todos os desvios pequenos (cada grau,
                // cada alteração; o pivô; cada rotação), o que mais se
                // aproxima do alvo afetivo; desempate pelo campo
                struct Cand { MachineScale s; std::string how; double score; };
                std::vector<Cand> cands;
                static const double deltas[] = { 100.0, -100.0, 50.0, -50.0, 21.506, -21.506 };
                for (int k = 1; k < n; ++k)
                    for (double d : deltas)
                    {
                        char t[120];
                        std::snprintf(t, sizeof t, "alteração: o %s grau %s %.1f cents", ordinal(k), d > 0 ? "sobe" : "desce", std::abs(d));
                        cands.push_back({ alterDegree(prev, k, d), t, 0.0 });
                    }
                for (int k = 1; k < n; ++k)
                {
                    char t[120];
                    std::snprintf(t, sizeof t, "rotação: o modo que começa no %s grau", ordinal(k));
                    cands.push_back({ rotateScale(prev, k), t, 0.0 });
                }
                const auto pv = pivotScale(prev, field);
                cands.push_back({ pv, "pivô por notas comuns: a escala vai para " + tonicName(pv.tonic), 0.0 });
                for (auto& c : cands)
                    c.score = away(c.s) - 0.3 * fieldFit(c.s, field) + 0.02 * rng.uniform();
                const auto best = std::min_element(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.score < b.score; });
                step.scale = best->s;
                std::snprintf(b, sizeof b, "%s", best->how.c_str());
                break;
            }
            // desvios pequenos: alterar um grau, ou pivô por notas comuns
            const double r = rng.uniform();
            if (r < 0.45)
            {
                const int k = weakest();
                static const double deltas[] = { 100.0, -100.0, 50.0, -50.0, 21.506, -21.506 };
                const double d = deltas[rng.below(6)];
                step.scale = alterDegree(prev, k, d);
                std::snprintf(b, sizeof b, "alteração: o %s grau %s %.1f cents", ordinal(k), d > 0 ? "sobe" : "desce", std::abs(d));
            }
            else if (r < 0.8)
            {
                step.scale = pivotScale(prev, field);
                std::snprintf(b, sizeof b, "pivô por notas comuns: a escala vai para %s", tonicName(step.scale.tonic).c_str());
            }
            else
            {
                const int k = 1 + rng.below(std::max(1, n - 1));
                step.scale = rotateScale(prev, k);
                std::snprintf(b, sizeof b, "rotação: o modo que começa no %s grau", ordinal(k));
            }
            break;
        }
        case MachineMode::transfigure:
        {
            // estados intermediários: metade do caminho até a escala do
            // arquivo que melhor veste a seção seguinte; às vezes alarga o período
            const double amount = 0.35 + 0.4 * rng.uniform();
            const auto to = destination(false, amount);
            step.scale = interpolateScales(prev, to, amount);
            std::snprintf(b, sizeof b, "interpolação: %.0f %% do caminho até %s sobre %s", 100.0 * amount,
                          to.name.empty() ? "outra escala" : to.name.c_str(), tonicName(to.tonic).c_str());
            if (rng.uniform() < 0.25 && std::abs(step.scale.period - 1200.0) < 15.0)  // não acumula de seção em seção
            {
                step.scale = stretchScale(step.scale, 1.0 + (rng.uniform() < 0.5 ? 1 : -1) * (0.01 + 0.02 * rng.uniform()));
                std::snprintf(b + std::strlen(b), sizeof b - std::strlen(b), " · período %.1f c", step.scale.period);
            }
            break;
        }
        case MachineMode::counter:
        {
            // o dual: os intervalos invertidos; às vezes ganha ou perde um grau
            step.scale = mirrorScale(prev);
            std::snprintf(b, sizeof b, "espelho: os intervalos invertidos");
            if (intent != Intention::none)
            {
                const auto plus = addDegree(step.scale), minus = removeDegree(step.scale, weakest());
                const double d0 = away(step.scale), d1 = away(plus), d2 = away(minus);
                if (d1 < d0 && d1 <= d2)
                {
                    step.scale = plus;
                    std::snprintf(b + std::strlen(b), sizeof b - std::strlen(b), " · ganha um grau no maior intervalo");
                }
                else if (d2 < d0)
                {
                    step.scale = minus;
                    std::snprintf(b + std::strlen(b), sizeof b - std::strlen(b), " · perde o grau mais fraco");
                }
            }
            else if (rng.uniform() < 0.4)
            {
                if (n <= 6 || rng.uniform() < 0.5)
                {
                    step.scale = addDegree(step.scale);
                    std::snprintf(b + std::strlen(b), sizeof b - std::strlen(b), " · ganha um grau no maior intervalo");
                }
                else
                {
                    step.scale = removeDegree(step.scale, weakest());
                    std::snprintf(b + std::strlen(b), sizeof b - std::strlen(b), " · perde o grau mais fraco");
                }
            }
            break;
        }
        case MachineMode::interrupt:
        {
            // o salto: a escala do arquivo com menos notas em comum
            step.scale = destination(true);
            std::snprintf(b, sizeof b, "salto: %s sobre %s", step.scale.name.c_str(), tonicName(step.scale.tonic).c_str());
            break;
        }
    }
    if (step.scale.name.empty() || mode != MachineMode::interrupt)
        step.scale.name = prev.name;
    step.how = b;
    step.common = commonTones(prev, step.scale);
    return step;
}

} // namespace trioio::composition
