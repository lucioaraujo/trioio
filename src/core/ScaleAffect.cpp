#include "ScaleAffect.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trioio::composition
{
namespace
{
bool hasDegree(const MachineScale& s, double lo, double hi)
{
    return std::any_of(s.cents.begin(), s.cents.end(), [&](double c) { return c >= lo && c <= hi; });
}
} // namespace

ScaleAffect scaleAffect(const MachineScale& s)
{
    ScaleAffect a;
    const auto n = s.cents.size();
    if (n < 2)
        return a;
    // brilho: os graus (sem a tônica) contra os de uma escala igual
    double mean = 0.0, equal = 0.0;
    for (std::size_t i = 1; i < n; ++i)
    {
        mean += s.cents[i];
        equal += s.period * static_cast<double>(i) / static_cast<double>(n);
    }
    a.brightness = (mean - equal) / static_cast<double>(n - 1) / 100.0;
    // terça sobre a tônica
    if (hasDegree(s, 370.0, 420.0)) a.third = 1;
    else if (hasDegree(s, 280.0, 330.0)) a.third = -1;
    // tensão
    int semis = 0, tritones = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        const double next = i + 1 < n ? s.cents[i + 1] : s.period;
        semis += next - s.cents[i] <= 120.0;
        for (std::size_t j = i + 1; j < n; ++j)
        {
            const double d = s.cents[j] - s.cents[i];
            tritones += d >= 580.0 && d <= 620.0;
        }
    }
    const bool b2 = hasDegree(s, 70.0, 130.0);
    const bool tritoneOnTonic = hasDegree(s, 580.0, 620.0);
    const bool fifth = hasDegree(s, 682.0, 722.0);
    a.tension = std::clamp(0.35 * semis / static_cast<double>(n) + 0.125 * std::min(2, tritones) + 0.2 * b2
                               + 0.2 * tritoneOnTonic + 0.2 * !fifth,
                           0.0, 1.0);
    // estabilidade
    a.stability = 0.5 * fifth + 0.2 * hasDegree(s, 478.0, 518.0) + 0.3 * (a.third != 0);
    // ambiguidade: graus longe do temperamento igual
    int far = 0;
    for (double c : s.cents)
        far += std::abs(c - 100.0 * std::round(c / 100.0)) > 30.0;
    a.ambiguity = static_cast<double>(far) / static_cast<double>(n);
    // familiaridade: notas comuns com o jônio na mesma tônica
    static const double ionian[] = { 0, 200, 400, 500, 700, 900, 1100 };
    int common = 0;
    for (double c : s.cents)
        common += std::any_of(std::begin(ionian), std::end(ionian), [&](double x) { return std::abs(c - x) <= 15.0; });
    a.familiarity = static_cast<double>(common) / static_cast<double>(std::max<std::size_t>(n, 7));
    a.valence = std::clamp(0.4 * a.brightness + 0.3 * a.third - 0.3 * a.tension + 0.4 * a.familiarity, -1.0, 1.0);
    return a;
}

std::string ScaleAffect::describe() const
{
    char b[200];
    std::snprintf(b, sizeof b, "brilho %+.2f · terça %s · tensão %.2f · estabilidade %.2f · ambiguidade %.2f · valência %+.2f",
                  brightness, third > 0 ? "maior" : third < 0 ? "menor" : "neutra/ausente", tension, stability, ambiguity,
                  valence);
    return b;
}

bool parseIntention(const std::string& n, Intention& out)
{
    if (n == "luminosa") out = Intention::luminous;
    else if (n == "sombria") out = Intention::somber;
    else if (n == "tensa") out = Intention::tense;
    else if (n == "serena") out = Intention::serene;
    else if (n == "suspensa") out = Intention::suspended;
    else if (n == "ambigua" || n == "ambígua") out = Intention::ambiguous;
    else if (n == "arco") out = Intention::arc;
    else if (n == "nenhuma") out = Intention::none;
    else return false;
    return true;
}

const char* intentionName(Intention i)
{
    switch (i)
    {
        case Intention::none: return "nenhuma";
        case Intention::luminous: return "luminosa";
        case Intention::somber: return "sombria";
        case Intention::tense: return "tensa";
        case Intention::serene: return "serena";
        case Intention::suspended: return "suspensa";
        case Intention::ambiguous: return "ambígua";
        case Intention::arc: return "arco";
    }
    return "?";
}

double intentionDistance(const ScaleAffect& a, Intention i, double position)
{
    const auto sq = [](double x) { return x * x; };
    switch (i)
    {
        case Intention::none: return 0.0;
        case Intention::luminous: return sq(a.valence - 0.8) + sq(a.tension - 0.15);
        case Intention::somber: return sq(a.valence + 0.4) + sq(a.brightness + 0.4);
        case Intention::tense: return sq(a.tension - 0.75) + 0.5 * sq(a.stability - 0.4);
        case Intention::serene: return sq(a.tension - 0.1) + sq(a.stability - 1.0) + 0.5 * sq(a.valence - 0.3);
        case Intention::suspended: return sq(a.stability - 0.4) + sq(a.tension - 0.4) + 0.5 * sq(static_cast<double>(a.third));
        case Intention::ambiguous: return sq(a.ambiguity - 0.6) + 0.5 * sq(a.stability - 0.5);
        case Intention::arc:
        {
            // a tensão pedida sobe até 2/3 da peça e resolve no fim
            const double p = std::clamp(position, 0.0, 1.0);
            const double want = p < 2.0 / 3.0 ? 0.15 + 0.7 * p / (2.0 / 3.0) : 0.85 - 0.75 * (p - 2.0 / 3.0) / (1.0 / 3.0);
            return sq(a.tension - want) + 0.5 * sq(a.stability - (1.0 - want));
        }
    }
    return 0.0;
}

} // namespace trioio::composition
