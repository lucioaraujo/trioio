#include "ScaleBank.h"

// cópia fixada do Rasgo Synth Studio (third_party/rasgo_synth_dsp/PROVENANCE.md)
#include "sequencer/MicrotonalTuning.hpp"
#include "sequencer/Scales.hpp"
#include "sequencer/SieveScale.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <utility>

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
};

// os nomes em português, na ordem de rasgo::Scale
const char* const kNames[] = {
    "pentatônica menor", "pentatônica maior", "maior (jônio)", "dórico", "frígio",
    "lídio", "mixolídio", "eólio (menor natural)", "lócrio", "menor melódica",
    "menor harmônica", "dórico b2", "lídio aumentado", "lídio dominante (acústica)",
    "mixolídio b6", "meio-diminuto (lócrio #2)", "alterada (superlócrio)", "lócrio 6",
    "jônio aumentado", "dórico #4", "frígio dominante (hijaz)", "lídio #2",
    "ultralócrio", "tons inteiros", "diminuta (tom-semitom)", "diminuta (semitom-tom)",
    "aumentada", "blues", "suspensa (egípcia)", "man gong", "ritusen", "bizantina (dupla harmônica)",
    "bebop dominante", "menor húngara", "maior húngara", "hirajoshi",
    "in (miyako-bushi)", "kumoi", "cromática", "maior harmônica", "menor napolitana",
    "bebop maior", "bebop menor", "mixolídio b9", "Prometeu (Scriabin)",
};
static_assert(sizeof kNames / sizeof kNames[0] == static_cast<std::size_t>(rasgo::Scale::Count),
              "um nome por escala do catálogo");

const char* const kPc[12] = { "Dó", "Dó#", "Ré", "Mib", "Mi", "Fá", "Fá#", "Sol", "Lá♭", "Lá", "Sib", "Si" };
} // namespace

int scaleCatalogSize() { return static_cast<int>(rasgo::Scale::Count); }

const char* scaleCatalogName(int index)
{
    return index >= 0 && index < scaleCatalogSize() ? kNames[index] : "?";
}

std::vector<float> scaleCatalogSteps(int index)
{
    const auto& t = rasgo::scaleTable(static_cast<rasgo::Scale>(std::clamp(index, 0, scaleCatalogSize() - 1)));
    std::vector<float> out;
    for (int i = 0; i < t.length; ++i)
        out.push_back(static_cast<float>(t.degrees[static_cast<std::size_t>(i)]));
    return out;
}

ScaleChoice fitCatalog(const std::array<float, 12>& field, std::uint64_t seed, int choices)
{
    Rng rng { seed * 0x2545F4914F6CDD1Dull + 0x5CA1E };
    float total = 0.0f, peak = 0.0f;
    for (float w : field)
    {
        total += std::max(0.0f, w);
        peak = std::max(peak, w);
    }
    struct Cand { double score; int scale; int tonic; };
    std::vector<Cand> cands;
    for (int s = 0; s < scaleCatalogSize(); ++s)
    {
        const auto steps = scaleCatalogSteps(s);
        for (int r = 0; r < 12; ++r)
        {
            double score = 0.0;
            if (total > 0.0f)
            {
                // quanto do campo a escala cobre, menos o custo de notas que a
                // referência não toca, mais o peso da tônica
                double covered = 0.0;
                int unused = 0;
                for (float st : steps)
                {
                    const float w = std::max(0.0f, field[static_cast<std::size_t>((r + static_cast<int>(st)) % 12)]);
                    covered += w;
                    unused += w < 0.05f * peak;
                }
                score = covered / total - 0.06 * unused + 0.2 * field[static_cast<std::size_t>(r)] / peak;
            }
            cands.push_back({ score, s, r });
        }
    }
    std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.score > b.score; });
    const int k = std::clamp(choices, 1, static_cast<int>(cands.size()));
    const auto& c = total > 0.0f ? cands[static_cast<std::size_t>(rng.uniform() * k) % static_cast<std::size_t>(k)]
                                 : cands[static_cast<std::size_t>(rng.uniform() * static_cast<double>(cands.size())) % cands.size()];
    ScaleChoice out;
    out.tonic = static_cast<float>(c.tonic);
    out.steps = scaleCatalogSteps(c.scale);
    out.name = std::string(kNames[c.scale]) + " sobre " + kPc[c.tonic];
    return out;
}

ScaleChoice inventedScale(std::uint64_t seed)
{
    // o algoritmo de SieveScale.hpp (generateSieveScale), com o gerador do
    // Trioio e a mesma rejeição de coleções do catálogo
    Rng rng { seed * 0xD1342543DE82EF95ull + 0x51E7E };
    static const auto catalog = rasgo::cataloguedPitchClassSets();
    std::set<int> pcs;
    std::vector<std::pair<int, int>> cond;
    for (int attempt = 0; attempt < 40; ++attempt)
    {
        cond.clear();
        const int n = 2 + (rng.uniform() < 0.4 ? 1 : 0);
        for (int c = 0; c < n; ++c)
        {
            const int mod = 2 + static_cast<int>(rng.uniform() * 4.0);  // 2..5
            cond.push_back({ mod, static_cast<int>(rng.uniform() * mod) });
        }
        pcs = { 0 };
        for (int p = 0; p < 12; ++p)
            for (const auto& [m, r] : cond)
                if (p % m == r)
                {
                    pcs.insert(p);
                    break;
                }
        if (pcs.size() >= 4 && pcs.size() <= 9 && !catalog.count(pcs))
            break;
    }
    ScaleChoice out;
    for (int p : pcs)
        out.steps.push_back(static_cast<float>(p));
    std::ostringstream d;
    d << "crivo inventado (";
    for (std::size_t i = 0; i < cond.size(); ++i)
        d << (i ? " ∪ " : "") << "p%" << cond[i].first << "==" << cond[i].second;
    d << ")";
    out.name = d.str();
    return out;
}

ScaleChoice edoSieveScale(std::uint64_t seed)
{
    // o crivo de MicrotonalTuning.hpp (configureSieveMicrotonal), mas os
    // graus admitidos viram a escala, em vez de desviar os 12 semitons; de
    // 5 a 9 graus, para a melodia continuar cantável
    Rng rng { seed * 0x9E6C63D0676A9A99ull + 0xED0 };
    static const int kEdo[] = { 19, 22, 31 };
    const int edo = kEdo[static_cast<int>(rng.uniform() * 3.0) % 3];
    std::vector<bool> in(static_cast<std::size_t>(edo));
    std::vector<std::pair<int, int>> cond;
    int count = 0;
    for (int attempt = 0; attempt < 60; ++attempt)
    {
        cond.clear();
        const int n = 2 + (rng.uniform() < 0.4 ? 1 : 0);
        const int maxMod = std::max(5, edo / 3);
        for (int c = 0; c < n; ++c)
        {
            const int mod = 3 + static_cast<int>(rng.uniform() * (maxMod - 2));
            cond.push_back({ mod, static_cast<int>(rng.uniform() * mod) });
        }
        std::fill(in.begin(), in.end(), false);
        in[0] = true;
        count = 1;
        for (int st = 1; st < edo; ++st)
            for (const auto& [m, r] : cond)
                if (st % m == r)
                {
                    in[static_cast<std::size_t>(st)] = true;
                    ++count;
                    break;
                }
        if (count >= 5 && count <= 9)
            break;
    }
    ScaleChoice out;
    for (int st = 0; st < edo; ++st)
        if (in[static_cast<std::size_t>(st)])
            out.steps.push_back(12.0f * static_cast<float>(st) / static_cast<float>(edo));
    std::ostringstream d;
    d << "crivo em " << edo << "-EDO (";
    for (std::size_t i = 0; i < cond.size(); ++i)
        d << (i ? " ∪ " : "") << "p%" << cond[i].first << "==" << cond[i].second;
    d << "; " << count << " de " << edo << " graus)";
    out.name = d.str();
    return out;
}

std::vector<float> retune(const std::vector<float>& steps, Tuning tuning, std::uint64_t seed)
{
    std::array<double, 12> cents {};
    if (tuning == Tuning::quarterTone)
    {
        Rng rng { seed * 0xBF58476D1CE4E5B9ull + 0x24ED0 };
        for (int pc = 1; pc < 12; ++pc)  // a tônica fica firme
            cents[static_cast<std::size_t>(pc)] = rng.uniform() < 0.5 ? 50.0 : 0.0;
    }
    else
    {
        rasgo::MicrotonalTuning t;
        t.configure(tuning == Tuning::justIntonation ? rasgo::TuningSystem::JustIntonation5Limit
                                                     : rasgo::TuningSystem::Pythagorean,
                    0u);
        for (int pc = 0; pc < 12; ++pc)
            cents[static_cast<std::size_t>(pc)] = t.centsOffset(pc);
    }
    std::vector<float> out;
    for (float st : steps)
    {
        const int pc = static_cast<int>(std::lround(st)) % 12;
        out.push_back(st + static_cast<float>(cents[static_cast<std::size_t>(pc)] / 100.0));
    }
    std::sort(out.begin(), out.end());
    return out;
}

const char* tuningName(Tuning t)
{
    switch (t)
    {
        case Tuning::justIntonation: return "afinação justa 5-limit (Ptolomeu, Partch)";
        case Tuning::pythagorean: return "afinação pitagórica (quintas puras)";
        case Tuning::quarterTone: return "quartos de tom, 24-EDO (Hába, Wyschnegradsky)";
    }
    return "?";
}

} // namespace trioio::composition
