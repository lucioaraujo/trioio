#include "Composer.h"
#include "Influences.h"
#include "Dialogue.h"
#include "Dramaturgy.h"
#include "Game.h"
#include "Groove.h"
#include "Harmony.h"
#include "Ensemble.h"
#include "FilmEffects.h"
#include "ModularRasgo.h"
#include "SomDaAura.h"
#include "Production.h"
#include "StyleFigures.h"
#include "Mosaic.h"
#include "OracleMemory.h"
#include "RhythmField.h"
#include "ScaleArchive.h"
#include "ScaleBank.h"
#include "Mashup.h"
#include "Recombination.h"
#include "RhythmCycles.h"
#include "ScaleIdiom.h"
#include "ScaleAffect.h"
#include "ScaleMachine.h"
#include "ThirdEye.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <map>
#include <numeric>
#include <optional>

namespace trioio::composition
{
using namespace trioio::perception;
using strategy::Mode;

namespace
{
// Passos do compasso corrente (tempos × 4). Ajustado no início de compose()
// pelo compasso medido; thread_local para que composições em threads
// diferentes não se misturem.
thread_local int barSteps = 16;

// ---- acaso determinístico ------------------------------------------------
struct Rng
{
    std::uint64_t s;
    std::uint64_t next()
    {
        s += 0x9E3779B97F4A7C15ull;
        std::uint64_t x = s;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) / 9007199254740992.0; }
    int range(int lo, int hi) { return lo + static_cast<int>(uniform() * (hi - lo + 1) - 1e-9); }  // [lo, hi]
    bool chance(double p) { return uniform() < p; }
};

const char* kPcNames[12] = { "Dó", "Dó#", "Ré", "Mib", "Mi", "Fá", "Fá#", "Sol", "Lá♭", "Lá", "Sib", "Si" };

// ---- campo → escala ------------------------------------------------------
// A escala é uma lista de classes de altura (0..12, com fração) a partir da
// tônica: degree 0 = tônica.
struct Scale
{
    float tonic = 0.0f;
    std::vector<float> steps;  // deslocamentos 0..12 acima da tônica, crescentes, steps[0] = 0
    std::string name;          // do banco de escalas (ScaleBank.h); vazio = o campo da referência
    float period = 12.0f;      // em semitons: 12 = oitava; outro valor = escala não oitavante (ScaleArchive.h)

    float pitch(int degree, float tonicMidi) const
    {
        const int n = static_cast<int>(steps.size());
        const int oct = degree >= 0 ? degree / n : -((-degree + n - 1) / n);
        const int idx = degree - oct * n;
        return tonicMidi + steps[static_cast<std::size_t>(idx)] + period * static_cast<float>(oct);
    }
    // grau cuja classe de altura é a mais próxima de `semitones` acima da tônica
    int nearestDegree(float semitones) const
    {
        int best = 0;
        float bd = 99.0f;
        for (std::size_t i = 0; i < steps.size(); ++i)
        {
            const float d = std::abs(steps[i] - semitones);
            if (d < bd)
            {
                bd = d;
                best = static_cast<int>(i);
            }
        }
        return best;
    }
    std::string describe() const
    {
        std::string s = name.empty() ? std::string() : name + ": ";
        const auto head = s.size();
        for (const float st : steps)
        {
            const float pc = std::fmod(tonic + st, 12.0f);
            const int r = static_cast<int>(std::lround(pc)) % 12;
            if (s.size() > head)
                s += " ";
            s += kPcNames[r];
            const float cents = (pc - std::round(pc)) * 100.0f;
            if (std::abs(cents) >= 5.0f)
                s += (cents > 0 ? "+" : "") + std::to_string(static_cast<int>(std::lround(cents))) + "c";
        }
        return s;
    }
};

Scale fromPitchClasses(std::vector<std::pair<float, float>> weighted)  // (classe 0..12, peso)
{
    std::sort(weighted.begin(), weighted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    Scale sc;
    if (weighted.empty() || weighted.front().second <= 0.0f)
    {
        sc.tonic = 0.0f;
        sc.steps = { 0, 2, 4, 7, 9 };
        return sc;
    }
    sc.tonic = weighted.front().first;
    const float maxW = weighted.front().second;
    for (const auto& [pc, w] : weighted)
    {
        if (sc.steps.size() >= 7 || (sc.steps.size() >= 3 && w < 0.4f * maxW) || w <= 0.0f)
            break;
        float st = std::fmod(pc - sc.tonic + 12.0f, 12.0f);
        if (std::all_of(sc.steps.begin(), sc.steps.end(), [&](float x) { return std::abs(x - st) > 0.2f; }))
            sc.steps.push_back(st);
    }
    std::sort(sc.steps.begin(), sc.steps.end());
    return sc;
}

Scale scaleFor(PitchSource src, const Section& sec, const std::vector<Frame>& frames, Rng& rng, std::uint64_t pieceSeed,
               const ArchivePick* archived)
{
    // do banco de escalas, com a tônica do campo da referência
    const auto fromBank = [&](const ScaleChoice& c, bool keepTonic) {
        Scale sc;
        std::vector<std::pair<float, float>> w;
        for (int i = 0; i < 12; ++i)
            w.push_back({ static_cast<float>(i), sec.field[static_cast<std::size_t>(i)] });
        sc.tonic = keepTonic ? c.tonic : fromPitchClasses(w).tonic;
        sc.steps = c.steps;
        sc.name = c.name;
        return sc;
    };
    const auto tuned = [&](Tuning t) {
        std::vector<std::pair<float, float>> w;
        for (int i = 0; i < 12; ++i)
            w.push_back({ static_cast<float>(i), sec.field[static_cast<std::size_t>(i)] });
        auto sc = fromPitchClasses(w);
        sc.steps = retune(sc.steps, t, pieceSeed);
        sc.name = tuningName(t);
        return sc;
    };
    std::vector<std::pair<float, float>> w;
    for (int i = 0; i < 12; ++i)
        w.push_back({ static_cast<float>(i), sec.field[static_cast<std::size_t>(i)] });
    switch (src)
    {
        case PitchSource::reference:
            return fromPitchClasses(w);
        case PitchSource::microtonal:
        {
            auto sc = fromPitchClasses(w);
            for (std::size_t i = 1; i < sc.steps.size(); ++i)  // graus desafinados, tônica firme
                sc.steps[i] += static_cast<float>(rng.uniform() * 0.8 - 0.4);
            std::sort(sc.steps.begin(), sc.steps.end());
            return sc;
        }
        case PitchSource::modal:
            // o catálogo de 45 escalas do Studio: a que melhor veste o campo
            // desta seção (antes, um de 6 modos diatônicos sorteado)
            return fromBank(fitCatalog(sec.field, rng.next()), true);
        case PitchSource::invented:
            return fromBank(inventedScale(pieceSeed), false);
        case PitchSource::edo:
            return fromBank(edoSieveScale(pieceSeed), false);
        case PitchSource::just:
            return tuned(Tuning::justIntonation);
        case PitchSource::pythagorean:
            return tuned(Tuning::pythagorean);
        case PitchSource::quarterTone:
            return tuned(Tuning::quarterTone);
        case PitchSource::archive:
            if (archived && archived->error.empty())
            {
                // a escala do arquivo, escolhida uma vez para a peça inteira,
                // com a tônica que melhor veste a referência
                auto sc = fromBank(archived->choice, true);
                sc.period = archived->period;
                return sc;
            }
            break;
        case PitchSource::spectral:
        {
            // picos medidos, em quartos de tom, sem temperamento
            std::map<int, float> hist;
            for (const auto& f : frames)
            {
                if (f.time < sec.start || f.time >= sec.end || f.silent)
                    continue;
                const float pw = std::pow(10.0f, f.rmsDb / 10.0f);
                for (const float hz : f.peaksHz)
                    if (hz >= 80.0f && hz <= 4000.0f)
                    {
                        const float midi = 69.0f + 12.0f * std::log2(hz / 440.0f);
                        const int q = static_cast<int>(std::lround(std::fmod(midi, 12.0f) * 4.0f)) % 48;
                        hist[q] += pw;
                    }
            }
            // só os máximos locais do histograma (circular): um pico espectral
            // se espalha por quartos de tom vizinhos, e sem isto a "escala"
            // virava 6 alturas dentro de um semitom
            std::vector<std::pair<float, float>> qw;
            const auto at = [&](int q) { const auto it = hist.find(((q % 48) + 48) % 48); return it == hist.end() ? 0.0f : it->second; };
            for (const auto& [q, v] : hist)
                if (v > at(q - 1) && v >= at(q + 1) && v > at(q - 2) && v >= at(q + 2))
                    qw.push_back({ static_cast<float>(q) / 4.0f, v });
            auto sc = fromPitchClasses(qw);
            if (sc.steps.size() > 6)
                sc.steps.resize(6);
            return sc;
        }
    }
    return fromPitchClasses(w);
}

// ---- motivo ----------------------------------------------------------------
struct MNote
{
    int step = 0;      // semicolcheias desde o início do motivo
    int len = 1;
    int degree = 0;
    float accent = 0.5f;
};
using Motif = std::vector<MNote>;

float jaccard(const Bar& a, const Bar& b)
{
    int inter = 0, uni = 0;
    for (std::size_t i = 0; i < a.hits.size(); ++i)
    {
        const bool x = a.hits[i] > 0, y = b.hits[i] > 0;
        inter += x && y;
        uni += x || y;
    }
    return uni ? static_cast<float>(inter) / static_cast<float>(uni) : 0.0f;
}

// o compasso mais típico da referência (medoide), com 3 a 10 ataques
Bar typicalBar(const std::vector<Bar>& bars, Rng& rng)
{
    std::vector<const Bar*> cand;
    for (const auto& b : bars)
        if (b.count() >= 3 && b.count() <= 10)
            cand.push_back(&b);
    if (cand.empty())
    {
        Bar b;  // sem ritmo aproveitável: uma célula própria
        static const std::vector<std::vector<int>> cells = { { 0, 3, 6, 8, 12 }, { 0, 4, 6, 10 }, { 0, 2, 4, 8, 11 } };
        b.steps = barSteps;
        for (const int s : cells[static_cast<std::size_t>(rng.range(0, 2))])
            b.hits[static_cast<std::size_t>(s * barSteps / 16)] = 1.0f;
        return b;
    }
    const Bar* best = cand.front();
    float bestSum = -1.0f;
    for (const auto* a : cand)
    {
        float s = 0.0f;
        for (const auto* b : cand)
            s += jaccard(*a, *b);
        if (s > bestSum)
        {
            bestSum = s;
            best = a;
        }
    }
    return *best;
}

// grau da escala (com oitava) mais próximo de uma altura MIDI
int degreeOf(const Scale& sc, float midi)
{
    const float rel = midi - sc.tonic;
    float oct = std::floor(rel / 12.0f);
    float off = rel - 12.0f * oct;
    int idx = sc.nearestDegree(off);
    if (12.0f - off < std::abs(off - sc.steps[static_cast<std::size_t>(idx)]))  // mais perto da tônica de cima
    {
        idx = 0;
        oct += 1.0f;
    }
    return static_cast<int>(oct) * static_cast<int>(sc.steps.size()) + idx;
}

Motif makeMotif(const Bar& bar, const Scale& scale, const std::vector<Frame>& frames, double stepDur, Rng& rng,
                bool& fromReference)
{
    const std::size_t scaleSize = scale.steps.size();
    std::vector<std::pair<int, float>> hits;
    for (int i = 0; i < barSteps; ++i)
        if (bar.hits[static_cast<std::size_t>(i)] > 0)
            hits.push_back({ i, bar.hits[static_cast<std::size_t>(i)] });
    if (hits.size() > 7)  // os 7 mais fortes, na ordem do tempo
    {
        std::sort(hits.begin(), hits.end(), [](auto a, auto b) { return a.second > b.second; });
        hits.resize(7);
        std::sort(hits.begin(), hits.end());
    }
    Motif m;
    int degree = 0, leaps = 0;
    const int n = static_cast<int>(hits.size());

    // contorno da referência: a altura medida em cada ataque do compasso
    std::vector<float> heard;
    int pitched = 0;
    // lê a altura DENTRO da nota: a janela do YIN olha 85 ms para trás, e
    // logo depois do ataque ainda ouviria a nota anterior
    for (std::size_t i = 0; i < hits.size(); ++i)
    {
        const int len = (i + 1 < hits.size() ? hits[i + 1].first : barSteps) - hits[i].first;
        const double into = std::min(0.1, 0.7 * len * stepDur);
        heard.push_back(pitchAt(frames, bar.start + hits[i].first * stepDur + into));
        pitched += !std::isnan(heard.back());
    }
    fromReference = n > 0 && pitched * 10 >= n * 6;
    if (fromReference)
    {
        // graus absolutos na escala da seção: o motivo começa onde a melodia
        // começa (só a oitava é normalizada para perto da tônica)
        int prev = 0;
        bool first = true;
        int origin = 0;
        for (int i = 0; i < n; ++i)
        {
            int d = std::isnan(heard[static_cast<std::size_t>(i)]) ? prev : degreeOf(scale, heard[static_cast<std::size_t>(i)]);
            if (first && !std::isnan(heard[static_cast<std::size_t>(i)]))
            {
                const int sz = static_cast<int>(scaleSize);
                origin = (d >= 0 ? d / sz : -((-d + sz - 1) / sz)) * sz;  // só a oitava
                first = false;
            }
            prev = d;
            m.push_back({ hits[static_cast<std::size_t>(i)].first,
                          (i + 1 < n ? hits[static_cast<std::size_t>(i) + 1].first : barSteps) - hits[static_cast<std::size_t>(i)].first,
                          d, hits[static_cast<std::size_t>(i)].second });
        }
        for (auto& x : m)
            x.degree -= origin;  // contorno relativo: o motivo começa no grau 0
        return m;
    }
    const int stable = scaleSize > 3 ? 2 : 0;
    for (int i = 0; i < n; ++i)
    {
        MNote x;
        x.step = hits[static_cast<std::size_t>(i)].first;
        x.len = (i + 1 < n ? hits[static_cast<std::size_t>(i) + 1].first : barSteps) - x.step;
        x.accent = hits[static_cast<std::size_t>(i)].second;
        if (i == n - 1 && n > 2)
            degree = rng.chance(0.5) ? 0 : stable;  // termina num grau estável
        x.degree = degree;
        m.push_back(x);
        // arco: sobe na primeira metade, desce na segunda; graus conjuntos e um salto
        const int dir = i < n / 2 ? 1 : -1;
        if (leaps == 0 && rng.chance(0.3))
        {
            degree += dir * rng.range(2, 3);
            ++leaps;
        }
        else
            degree += rng.chance(0.8) ? dir : -dir;
    }
    return m;
}

Motif transpose(Motif m, int k) { for (auto& x : m) x.degree += k; return m; }
Motif invert(Motif m) { for (auto& x : m) x.degree = -x.degree; return m; }
Motif retrograde(Motif m)
{
    std::vector<int> d;
    for (const auto& x : m) d.push_back(x.degree);
    std::reverse(d.begin(), d.end());
    for (std::size_t i = 0; i < m.size(); ++i) m[i].degree = d[i];
    return m;
}
Motif augment(Motif m) { for (auto& x : m) { x.step *= 2; x.len *= 2; } return m; }
Motif diminish(Motif m)
{
    Motif out;
    for (int rep = 0; rep < 2; ++rep)
        for (auto x : m)
        {
            x.step = x.step / 2 + rep * barSteps / 2;
            x.len = std::max(1, x.len / 2);
            out.push_back(x);
        }
    return out;
}
Motif fragment(const Motif& m)
{
    Motif out;
    for (const auto& x : m)
        if (x.step < barSteps / 2)
            out.push_back(x);
    if (out.size() < 2 && m.size() >= 2)
        out.assign(m.begin(), m.begin() + 2);
    if (!out.empty())
        out.back().len = std::min(out.back().len, barSteps / 2 - out.back().step);
    for (auto& x : out)
        x.len = std::max(1, x.len);
    return out;
}
Motif vary(Motif m, Rng& rng)
{
    if (m.size() > 1)
    {
        auto& x = m[static_cast<std::size_t>(rng.range(1, static_cast<int>(m.size()) - 1))];
        x.degree += rng.chance(0.5) ? 1 : -1;
    }
    return m;
}
Motif displace(Motif m)
{
    Motif out;
    for (auto x : m)
    {
        x.step += 2;  // uma colcheia adiante
        if (x.step < barSteps)
        {
            x.len = std::min(x.len, barSteps - x.step);
            out.push_back(x);
        }
    }
    return out;
}

// Variação em desenvolvimento: cada frase deriva da anterior por uma operação
// da paleta do modo (não sempre do original), e o caminho fica registrado.
Motif develop(const Motif& m, Mode mode, Rng& rng, std::string& name)
{
    const int k = rng.range(0, 3);
    switch (mode)
    {
        case Mode::follow:
            switch (k)
            {
                case 0: name = "transposição ↑"; return transpose(m, rng.range(1, 2));
                case 1: name = "transposição ↓"; return transpose(m, -rng.range(1, 2));
                case 2: name = "variação"; return vary(m, rng);
                default: name = "variação + transposição"; return vary(transpose(m, rng.chance(0.5) ? 1 : -1), rng);
            }
        case Mode::transfigure:
            switch (k)
            {
                case 0: name = "inversão"; return invert(m);
                case 1: name = "retrógrado"; return retrograde(m);
                case 2: name = "retrógrado da inversão"; return retrograde(invert(m));
                default: name = "inversão transposta"; return transpose(invert(m), rng.range(2, 4));
            }
        case Mode::counter:
            switch (k)
            {
                case 0: name = "deslocamento"; return displace(m);
                case 1: name = "inversão"; return invert(m);
                case 2: name = "deslocamento invertido"; return displace(invert(m));
                default: name = "variação"; return vary(m, rng);
            }
        case Mode::interrupt:
            switch (k)
            {
                case 0: name = "fragmentação"; return fragment(m).size() >= 2 ? fragment(m) : m;
                case 1: name = "diminuição"; return diminish(fragment(m));
                case 2: name = "deslocamento"; return displace(m);
                default: name = "variação"; return vary(m, rng);
            }
    }
    name = "repetição";
    return m;
}

// ---- escrita ---------------------------------------------------------------
struct Writer
{
    Score& score;
    double end;
    Timbre motifTimbre = Timbre::additive;
    Timbre bassTimbre = Timbre::additive;
    void note(double t, double dur, float pitch, float vel, int voice, float glide = 0.0f)
    {
        if (t >= end || dur <= 0.01)
            return;
        score.notes.push_back({ t, std::min(dur, end - t), pitch, glide, std::clamp(vel, 0.05f, 1.0f), voice,
                                voice == 1 ? bassTimbre : motifTimbre });
    }
};

// orquestração: a família do motivo nasce do caráter da seção; transfigurar
// e contrapor escolhem o oposto. Desde 2026-10-07 cada caráter tem cinco
// famílias à escolha (as seis próprias, a espectral e as do Rasgo Modular).
// A espectral entra onde a referência tem corpo tonal: é o som dela mesma.
Timbre motifTimbreFor(const Summary& s, Mode mode, Rng& rng, const Tendencies& tendIn, const Constraints& rules)
{
    // família proibida pelas regras pesa 0
    Tendencies tend = tendIn;
    for (std::size_t k = 0; k < tend.timbre.size(); ++k)
        if (rules.noTimbre[k])
            tend.timbre[k] = 0.0f;
    using T = Timbre;
    const bool noisy = s.noisiness > 0.25f, dense = s.density > 0.5f, bright = s.brightness > 0.6f;
    // sorteio ponderado pela escuta do autor (Listening.h); com todos os
    // pesos em 1 escolhe exatamente o mesmo que o sorteio uniforme de antes
    const auto pick = [&](std::initializer_list<T> set) {
        double total = 0.0;
        for (T t : set)
            total += tend.timbre[static_cast<std::size_t>(t)];
        if (total <= 0.0)  // todas as do conjunto proibidas: a primeira permitida
        {
            (void) rng.uniform();
            for (int k = 0; k < kTimbreCount; ++k)
                if (!rules.noTimbre[static_cast<std::size_t>(k)])
                    return static_cast<T>(k);
            return T::additive;
        }
        double u = rng.uniform() * total;
        for (T t : set)
        {
            u -= tend.timbre[static_cast<std::size_t>(t)];
            if (u < 0.0)
                return t;
        }
        return *(set.end() - 1);
    };
    if (mode == Mode::transfigure || mode == Mode::counter)
        return noisy ? pick({ T::metal, T::additive, T::modal, T::spectral, T::vocal, T::theremin, T::flute, T::choir, T::biomaModal, T::reed })
             : dense ? pick({ T::breath, T::additive, T::vocal, T::wavetable, T::spectral, T::theremin, T::galo, T::resonator, T::spectralObject, T::bowed, T::transverseFlute })
             : bright ? pick({ T::membrane, T::breath, T::drum, T::folded, T::pulsar, T::gendyn, T::chip, T::cmos, T::lpg, T::bars })
                      : pick({ T::metal, T::plucked, T::pulsar, T::modal, T::folded, T::walsh, T::vosim, T::rungler, T::operatorFm, T::piano });
    return noisy ? pick({ T::breath, T::membrane, T::drum, T::pulsar, T::folded, T::gendyn, T::scat, T::cmos, T::striker, T::granular, T::guitar })
         : dense ? pick({ T::plucked, T::fm, T::waveguide, T::wavetable, T::spectral, T::chip, T::walsh, T::rungler, T::lpg, T::biomaString, T::piano, T::guitar })
         : bright ? pick({ T::metal, T::additive, T::modal, T::vocal, T::spectral, T::vosim, T::flute, T::resonator, T::operatorFm, T::biomaModal, T::bars, T::reed, T::transverseFlute })
                  : pick({ T::additive, T::breath, T::spectral, T::wavetable, T::waveguide, T::theremin, T::galo, T::choir, T::spectralObject, T::granular, T::bowed, T::piano });
}
} // namespace

const char* timbreName(Timbre t)
{
    switch (t)
    {
        case Timbre::additive: return "aditivo";
        case Timbre::breath: return "sopro";
        case Timbre::metal: return "metal";
        case Timbre::plucked: return "corda pinçada";
        case Timbre::membrane: return "membrana";
        case Timbre::fm: return "FM";
        case Timbre::spectral: return "espectral";
        case Timbre::wavetable: return "tabela de ondas";
        case Timbre::pulsar: return "pulsar";
        case Timbre::modal: return "modal";
        case Timbre::waveguide: return "guia de onda";
        case Timbre::drum: return "tambor";
        case Timbre::vocal: return "voz formante";
        case Timbre::folded: return "dobra";
        case Timbre::gendyn: return "Gendyn";
        case Timbre::vosim: return "VOSIM";
        case Timbre::walsh: return "Walsh";
        case Timbre::theremin: return "teremim";
        case Timbre::flute: return "flauta FM";
        case Timbre::scat: return "scat";
        case Timbre::chip: return "chip AY";
        case Timbre::galo: return "galo";
        case Timbre::resonator: return "ressonador soprado";
        case Timbre::cmos: return "CMOS";
        case Timbre::choir: return "coro";
        case Timbre::rungler: return "Rungler";
        case Timbre::operatorFm: return "operador FM";
        case Timbre::lpg: return "LPG";
        case Timbre::granular: return "nuvem granular";
        case Timbre::spectralObject: return "objeto espectral";
        case Timbre::biomaModal: return "ressonador Bioma";
        case Timbre::biomaString: return "corda Bioma";
        case Timbre::striker: return "striker granular";
        case Timbre::bowed: return "arco";
        case Timbre::reed: return "palheta";
        case Timbre::bars: return "marimba";
        case Timbre::piano: return "piano";
        case Timbre::guitar: return "violão";
        case Timbre::upright: return "baixo acústico";
        case Timbre::transverseFlute: return "flauta transversal";
        case Timbre::surdo: return "surdo";
        case Timbre::conga: return "conga";
        case Timbre::pandeiro: return "pandeiro";
        case Timbre::shaker: return "ganzá";
        case Timbre::sevenString: return "violão de 7 cordas";
        case Timbre::requinto: return "requinto";
        case Timbre::cavaquinho: return "cavaquinho";
        case Timbre::bandolim: return "bandolim";
        case Timbre::manouche: return "guitarra manouche";
        case Timbre::sax: return "sax";
        case Timbre::trumpet: return "trompete";
        case Timbre::trombone: return "trombone";
        case Timbre::ride: return "prato de condução";
        case Timbre::hihat: return "chimbal";
        case Timbre::snare: return "caixa";
        case Timbre::brushes: return "caixa com escovinha";
        case Timbre::kick: return "bumbo";
        case Timbre::bongo: return "bongô";
        case Timbre::guiro: return "guiro";
        case Timbre::singing: return "voz";
        case Timbre::whisper: return "sussurro";
        case Timbre::hum: return "boca chiusa";
        case Timbre::tom: return "tom-tom";
        case Timbre::crash: return "prato de ataque";
        case Timbre::electricGuitar: return "guitarra elétrica";
        case Timbre::distortedGuitar: return "guitarra distorcida";
        case Timbre::electricBass: return "baixo elétrico";
        case Timbre::tamborim: return "tamborim";
        case Timbre::repique: return "repique";
        case Timbre::agogo: return "agogô";
        case Timbre::cuica: return "cuíca";
        case Timbre::chocalho: return "chocalho";
        case Timbre::organ: return "órgão";
        case Timbre::accordion: return "sanfona";
        case Timbre::bandoneon: return "bandoneón";
        case Timbre::zabumba: return "zabumba";
        case Timbre::triangle: return "triângulo";
        case Timbre::stringSection: return "naipe de cordas";
        case Timbre::brassSection: return "naipe de metais";
        case Timbre::voiceChoir: return "coro de vozes";
        case Timbre::bowedPlate: return "prato friccionado";
        case Timbre::sungString: return "corda cantada";
        case Timbre::blownSkin: return "pele soprada";
    }
    return "?";
}

// Âmbito e salto máximo, por último (depois do Terceiro Olho, que também
// move alturas). Primeiro dobra por oitavas; se ainda passar do salto
// máximo, encurta o salto até o limite. Vale para motivo, eco e memória; o
// baixo fica livre.
void applyPitchRules(Score& score, const Constraints& rules)
{
    if (rules.maxLeap <= 0 && rules.highest <= rules.lowest)
        return;
    int fixed = 0;
    float prev = -1.0f;
    for (auto& n : score.notes)
    {
        if (n.voice == 1 || n.voice == 4)  // baixo e percussão ficam livres
            continue;
        const float before = n.pitch;
        if (rules.highest > rules.lowest)
        {
            while (n.pitch > rules.highest && n.pitch - 12.0f >= rules.lowest) n.pitch -= 12.0f;
            while (n.pitch < rules.lowest && n.pitch + 12.0f <= rules.highest) n.pitch += 12.0f;
            n.pitch = std::clamp(n.pitch, rules.lowest, rules.highest);
        }
        if (rules.maxLeap > 0 && n.voice == 0)
        {
            if (prev >= 0.0f && std::abs(n.pitch - prev) > static_cast<float>(rules.maxLeap))
            {
                float p = n.pitch + 12.0f * std::round((prev - n.pitch) / 12.0f);
                if (std::abs(p - prev) > static_cast<float>(rules.maxLeap))
                    p = prev + (p > prev ? 1.0f : -1.0f) * static_cast<float>(rules.maxLeap);
                if (rules.highest > rules.lowest)
                    p = std::clamp(p, rules.lowest, rules.highest);
                n.pitch = p;
            }
            prev = n.pitch;
        }
        fixed += n.pitch != before;
    }
    if (fixed > 0)
        score.marks.push_back({ 0.0, "regras: " + std::to_string(fixed) + " altura(s) ajustada(s) (âmbito e salto máximo)" });
}

const char* roomName(Room r)
{
    switch (r)
    {
        case Room::trioio: return "sala do Trioio";
        case Room::hall: return "HALL (Rasgo Modular)";
        case Room::space: return "SPACE (Rasgo Modular)";
    }
    return "?";
}

const SpectralProfile* spectralAt(const Score& score, double t)
{
    const SpectralProfile* best = nullptr;
    double d = 1e18;
    for (const auto& p : score.spectra)
    {
        const double c = 0.5 * (p.start + p.end);
        if (std::abs(c - t) < d) { d = std::abs(c - t); best = &p; }
    }
    return best;
}

// O corpo espectral: cada pico de cada quadro vira uma razão em relação à
// fundamental medida (YIN) e entra num histograma com o peso da potência
// do quadro. Os máximos locais mais fortes são os parciais. Os quadros
// guardam 4 picos, sem amplitude: um parcial pesa pelo quanto aparece entre
// os mais fortes, e em quadros de quanta potência. É uma estimativa do
// CORPO, não uma análise-ressíntese quadro a quadro.
SpectralProfile spectralProfile(const std::vector<Frame>& frames, double t0, double t1)
{
    SpectralProfile p;
    p.start = t0;
    p.end = t1;
    constexpr int kBins = 8;  // resolução: 1/8 de razão
    std::vector<double> hist(24 * kBins + 2, 0.0);
    double flat = 0.0, flatW = 0.0;
    for (const auto& f : frames)
    {
        if (f.time < t0 || f.time >= t1 || f.silent)
            continue;
        const double power = std::pow(10.0, f.rmsDb / 10.0);
        flat += f.flatness * power;
        flatW += power;
        double base = (f.f0Hz > 0.0f && f.periodicity >= 0.5f) ? f.f0Hz : 0.0;
        // o YIN dá a fundamental com alguns % de erro quando há parciais
        // inarmônicos (200 Hz lido como 189); o pico espectral é mais
        // preciso: se um pico está a menos de 8 % do f0, ele é a base
        if (base > 0.0)
        {
            double best = 0.08;
            for (float pk : f.peaksHz)
                if (pk > 0.0f && std::abs(pk / base - 1.0) < best)
                {
                    best = std::abs(pk / base - 1.0);
                    base = pk;
                }
        }
        if (base <= 0.0)
            for (float pk : f.peaksHz)
                if (pk > 0.0f && (base <= 0.0 || pk < base))
                    base = pk;
        if (base <= 0.0)
            continue;
        if (f.f0Hz > 0.0f)
            hist[kBins] += 0.5 * power;  // a fundamental, mesmo ausente dos picos
        for (float pk : f.peaksHz)
        {
            const double r = pk / base;
            if (pk <= 0.0f || r < 0.45 || r > 24.0)
                continue;
            hist[static_cast<std::size_t>(std::lround(r * kBins))] += power;
        }
    }
    if (flatW <= 0.0)
        return p;
    p.noise = static_cast<float>(flat / flatW);
    std::vector<std::pair<double, double>> peaks;  // peso, razão
    for (std::size_t i = 1; i + 1 < hist.size(); ++i)
        if (hist[i] > 0.0 && hist[i] >= hist[i - 1] && hist[i] > hist[i + 1])
        {
            const double w = hist[i - 1] + hist[i] + hist[i + 1];
            const double r = ((i - 1.0) * hist[i - 1] + i * hist[i] + (i + 1.0) * hist[i + 1]) / (w * kBins);
            peaks.push_back({ w, r });
        }
    std::sort(peaks.begin(), peaks.end(), std::greater<>());
    if (peaks.size() > 10)
        peaks.resize(10);
    if (peaks.empty())
        return p;
    if (std::none_of(peaks.begin(), peaks.end(), [](const auto& q) { return std::abs(q.second - 1.0) < 0.1; }))
        peaks.push_back({ peaks.front().first * 0.6, 1.0 });
    double sum = 0.0;
    for (const auto& q : peaks) sum += q.first;
    std::sort(peaks.begin(), peaks.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
    for (const auto& q : peaks)
    {
        p.ratios.push_back(static_cast<float>(q.second));
        p.weights.push_back(static_cast<float>(q.first / sum));
    }
    return p;
}

Score compose(const std::vector<Frame>& frames, const std::vector<Onset>& onsets, double duration, const Options& opt)
{
    // num conjunto de estilo tonal (2026-10-17; o jazz que "não soa como
    // jazz" vinha com um modo de Messiaen do arquivo de escalas), a escala é
    // o campo tonal da referência: o arquivo procura a escala mais rara que a
    // veste — ótimo no automático, fora do idioma num choro ou num jazz.
    // Câmara, orquestra e a cappella ficam com o arquivo; as outras fontes
    // escolhidas à mão também.
    if (opt.pitch == PitchSource::archive && opt.mix.empty() && opt.ensemble != 0 && opt.ensemble != 7 && opt.ensemble != 8
        && opt.ensemble != 10)
    {
        Options tonal = opt;
        tonal.pitch = PitchSource::reference;
        auto s = compose(frames, onsets, duration, tonal);
        s.marks.insert(s.marks.begin() + std::min<std::size_t>(1, s.marks.size()),
                       { 0.0, "escala: o campo tonal da referência (num conjunto de estilo, o arquivo de escalas fica de fora)" });
        return s;
    }
    Score score;
    score.duration = duration;
    Rng rng { opt.seed * 0x2545F4914F6CDD1Dull + 17 };
    Writer w { score, duration };

    // a forma: por contraste (novidade) ou por repetição (o que volta: A B A…)
    std::vector<int> formLabels;
    const auto sections = opt.formByRepetition ? findSectionsByRepetition(frames, onsets, duration, formLabels)
                                               : findSections(frames, onsets, duration);
    struct FormMemory { Scale scale; Motif motif; Timbre motifTimbre, bassTimbre; std::string role; std::size_t first; };
    std::map<int, FormMemory> formMemory;
    if (!formLabels.empty())
    {
        std::string f;
        for (std::size_t k = 0; k < formLabels.size(); ++k)
            f += (k ? " " : "") + std::string(1, static_cast<char>('A' + formLabels[k] % 26));
        score.marks.push_back({ 0.0, "forma por repetição (Foote 2000; Müller 2015): " + f });
    }
    // o arquivo de escalas (ScaleArchive.h): uma escala para a peça, pelo
    // campo da referência inteira, entre todas ou as que casam com o termo
    std::optional<ArchivePick> archived;
    if (opt.pitch == PitchSource::archive)
    {
        if (!opt.scaleArchive || opt.scaleArchive->empty())
            score.marks.push_back({ 0.0, "arquivo de escalas: não carregado; usando o campo da referência" });
        else
        {
            archived = fitArchive(*opt.scaleArchive, languageField(frames, onsets, duration), opt.scaleQuery, opt.seed);
            if (!archived->error.empty())
                score.marks.push_back({ 0.0, "arquivo de escalas: " + archived->error + "; usando o campo da referência" });
            else
                score.marks.push_back({ 0.0, "escala do arquivo: " + archived->choice.name + " · fonte: " + archived->source
                                                 + " · " + std::to_string(archived->candidates) + " candidata(s)" });
        }
    }
    // a mistura de culturas e linguagens (Recombination.h): uma escala do
    // arquivo para cada termo, e a técnica decide como elas se combinam
    std::vector<ArchivePick> mixPicks;
    std::optional<ArchivePick> layerOther;
    if (!opt.mix.empty() && opt.scaleArchive && !opt.scaleArchive->empty())
    {
        const auto field = languageField(frames, onsets, duration);
        std::string names;
        for (std::size_t i = 0; i < opt.mix.size(); ++i)
        {
            auto p = fitArchive(*opt.scaleArchive, field, opt.mix[i], opt.seed + 7919ull * (i + 1));
            if (!p.error.empty())
            {
                score.marks.push_back({ 0.0, "mistura: " + p.error });
                continue;
            }
            names += (names.empty() ? "" : " · ") + p.choice.name;
            mixPicks.push_back(std::move(p));
        }
        if (!mixPicks.empty())
        {
            score.marks.push_back({ 0.0, std::string("mistura (") + mixTechniqueName(opt.mixTechnique) + "): " + names });
            switch (opt.mixTechnique)
            {
                case MixTechnique::collage: archived.reset(); break;
                case MixTechnique::hybrid:
                    archived = mixPicks.size() >= 2 ? hybridPick(mixPicks[0], mixPicks[1]) : mixPicks[0];
                    score.marks.push_back({ 0.0, "escala da mistura: " + archived->choice.name });
                    break;
                case MixTechnique::layers:
                    archived = mixPicks[0];
                    if (mixPicks.size() >= 2)
                        layerOther = mixPicks[1];
                    break;
                case MixTechnique::transition: archived = mixPicks[0]; break;
            }
        }
    }
    // o SAYR (o percurso do maqam): numa linguagem LINEAR (textura
    // heterofonia), sem acordes, o movimento vem de outro lugar. As seções
    // do meio alternam com uma escala vizinha do arquivo: com comportamento,
    // tônica no ghammaz (a 2.ª nota de apoio) e no máximo uma nota diferente
    // da de casa (o Hijaz sobre Lá vai ao Nahawand sobre Ré: as mesmas sete
    // notas, centradas no ghammaz). A primeira e a última seção ficam em casa.
    // Pedido do autor ao ouvir o Hijaz sobre Recording_187: mais variação
    // (2026-10-11). Ver J. Farraj e S. Abu Shumays, Inside Arabic Music, 2019
    // (sayr e modulação por ajnas).
    std::optional<ArchivePick> sayrPick;
    // Só na escolha automática: uma escala pedida pelo nome (--escala) fica
    // fixa, como foi pedida.
    if (archived && archived->error.empty() && opt.mix.empty() && opt.scaleArchive && sections.size() >= 3
        && opt.scaleQuery.empty() && archived->behavior.texture == "heterofonia")
    {
        const auto pcsOf = [](double tonicSemis, const std::vector<double>& centsSteps) {
            std::array<bool, 12> on {};
            for (double c : centsSteps)
                on[static_cast<std::size_t>(((static_cast<int>(std::lround(tonicSemis + c / 100.0)) % 12) + 12) % 12)] = true;
            return on;
        };
        std::vector<double> homeCents;
        for (float st : archived->choice.steps)
            homeCents.push_back(st * 100.0);
        const auto home = pcsOf(archived->choice.tonic, homeCents);
        int homeSize = 0;
        for (bool b : home) homeSize += b;
        const auto& sup = archived->behavior.support;
        const double ghammazCents = sup.size() >= 2 ? sup[1] : 700.0;
        const int target = ((static_cast<int>(std::lround(archived->choice.tonic + ghammazCents / 100.0)) % 12) + 12) % 12;
        const ArchiveScale* best = nullptr;
        double bestScore = -1e9;
        int bestCommon = 0;
        for (const auto& s : *opt.scaleArchive)
        {
            // a mesma textura (linear com linear): uma vizinha de acordes
            // traria baixo e acordes para dentro do maqam
            if (!isLanguageScale(s) || !s.behavior.any() || s.behavior.texture != archived->behavior.texture
                || std::abs(s.periodCents - 1200.0) > 10.0 || s.stepsCents.size() < 5
                || s.stepsCents.size() > 9 || archived->choice.name.rfind(s.name + " (", 0) == 0)
                continue;
            const auto on = pcsOf(target, s.stepsCents);
            int common = 0, foreign = 0;
            for (std::size_t q = 0; q < 12; ++q)
            {
                common += on[q] && home[q];
                foreign += on[q] && !home[q];
            }
            if (foreign > 1 || common < homeSize - 1)
                continue;
            const double score = common - 2.0 * foreign;
            if (score > bestScore)
            {
                bestScore = score;
                best = &s;
                bestCommon = common;
            }
        }
        if (best)
        {
            ArchivePick p;
            p.choice.tonic = static_cast<float>(target);
            for (double v : best->stepsCents)
                p.choice.steps.push_back(static_cast<float>(v / 100.0));
            static const char* const pcName[12] = { "Dó", "Dó#", "Ré", "Mib", "Mi", "Fá", "Fá#", "Sol", "Lá♭", "Lá", "Sib", "Si" };
            p.choice.name = best->name + " (" + best->family + ") sobre " + pcName[target];
            p.period = static_cast<float>(best->periodCents / 100.0);
            p.source = best->source;
            p.behavior = best->behavior;
            sayrPick = p;
            score.marks.push_back({ 0.0, "sayr: as seções do meio alternam com " + p.choice.name + " (no ghammaz; "
                                             + std::to_string(bestCommon) + " notas em comum com a escala de casa)" });
        }
    }
    const auto pickFor = [&](std::size_t si) -> const ArchivePick* {
        if (!mixPicks.empty() && opt.mixTechnique == MixTechnique::collage)
            return &mixPicks[si % mixPicks.size()];
        if (sayrPick && si > 0 && si + 1 < sections.size() && si % 2 == 1)
            return &*sayrPick;
        return archived && archived->error.empty() ? &*archived : nullptr;
    };
    // o corpo espectral ao longo da referência, em janelas de 3 s a cada
    // 1,5 s, para o timbre se mover como ela (família espectral e objeto
    // "corpo"). Antes era uma média por seção: o corpo ficava parado.
    // mashup de materiais (Mashup.h): cada camada pedida vem da outra referência
    const bool other = opt.otherFrames && opt.otherOnsets && opt.otherLayers != 0;
    const auto& bodyFrames = other && (opt.otherLayers & kMashBody) ? *opt.otherFrames : frames;
    const auto& harmonyFrames = other && (opt.otherLayers & kMashHarmony) ? *opt.otherFrames : frames;
    const auto& memoryFrames = other && (opt.otherLayers & kMashMemory) ? *opt.otherFrames : frames;
    const auto& memoryOnsets = other && (opt.otherLayers & kMashMemory) ? *opt.otherOnsets : onsets;
    if (other)
        score.marks.push_back({ 0.0, "mashup de materiais: " + mashupLayersName(opt.otherLayers) + " da referência "
                                         + opt.otherName + " (no tempo desta); melodia, forma e olho desta" });
    for (double w = 0.0; w < duration; w += 1.5)
    {
        auto p = spectralProfile(bodyFrames, w, std::min(duration, w + 3.0));
        if (!p.ratios.empty())
            score.spectra.push_back(std::move(p));
    }
    // a sala da peça: densa ou ruidosa pede ecos e difusão (SPACE), rarefeita
    // e tonal pede cauda longa (HALL); no meio, qualquer uma. Sorteio próprio,
    // para não deslocar os outros sorteios da composição.
    {
        Rng roomRng { opt.seed * 0x9E3779B97F4A7C15ull + 0x5A1A };
        const auto whole = summarize(frames, onsets, 0.0, duration);
        const bool busy = whole.noisiness > 0.25f || whole.density > 0.5f;
        const bool open = whole.density < 0.3f && whole.noisiness < 0.15f;
        // probabilidades pelo caráter, multiplicadas pela escuta do autor.
        // A ordem de cada lista é a do sorteio de antes: com a escuta neutra,
        // a mesma seed escolhe a mesma sala.
        struct Option { Room room; double p; };
        const std::array<Option, kRooms> order = busy
            ? std::array<Option, kRooms> { { { Room::space, 0.6 }, { Room::hall, 0.2 }, { Room::trioio, 0.2 } } }
            : open ? std::array<Option, kRooms> { { { Room::hall, 0.6 }, { Room::space, 0.2 }, { Room::trioio, 0.2 } } }
                   : std::array<Option, kRooms> { { { Room::trioio, 1.0 / 3 }, { Room::hall, 1.0 / 3 }, { Room::space, 1.0 / 3 } } };
        const auto weightOf = [&](Room r) {
            return opt.constraints.noRoom[static_cast<std::size_t>(r)] ? 0.0
                                                                       : static_cast<double>(opt.tendencies.room[static_cast<std::size_t>(r)]);
        };
        double total = 0.0;
        for (const auto& o : order)
            total += o.p * weightOf(o.room);
        double u = roomRng.uniform() * total;
        score.room = total > 0.0 ? order.back().room : Room::trioio;
        for (const auto& o : order)
            if (total > 0.0 && (u -= o.p * weightOf(o.room)) < 0.0)
            {
                score.room = o.room;
                break;
            }
        score.marks.push_back({ 0.0, std::string("sala: ") + roomName(score.room) });
    }
    const Pulse global = opt.pulse ? *opt.pulse : estimatePulse(frames, onsets, 0.0, duration);
    Pulse pulseRead = global.period > 0 ? global : Pulse { 100.0, 0.6, 0.0, 0.0f, 0.0, 4 };
    // o nível do pulso decidido pela escuta do autor (--andamento): quando a
    // gravação é ambígua entre o tempo e a subdivisão (a faixa do Suno: 172
    // ou 86?), a medida não decide sozinha
    if (opt.tempoScale > 0.0 && std::abs(opt.tempoScale - 1.0) > 1e-9)
    {
        pulseRead.period /= opt.tempoScale;
        pulseRead.bpm *= opt.tempoScale;
        pulseRead.phase = std::fmod(pulseRead.phase, pulseRead.period);
    }
    const Pulse pulse = pulseRead;
    barSteps = pulse.beatsPerBar * kStepsPerBeat;
    const double stepDur = pulse.period / kStepsPerBeat;
    const double barLen = pulse.period * pulse.beatsPerBar;
    const auto refBars = rhythmGrid(onsets, pulse, 0.0, duration);
    score.bpm = pulse.bpm;

    std::vector<std::pair<double, double>> pulsedSpans;  // trechos com pulso (campo rítmico)
    std::vector<SectionScale> sectionScales;             // a escala de cada seção (harmonia, máquina)
    // o motivo nasce da referência inteira e é a memória da peça
    const Bar cell = typicalBar(refBars, rng);
    bool contourFromReference = false;

    // faixa de energia da peça (para dinâmica relativa)
    float eLo = 1.0f, eHi = 0.0f;
    for (const auto& s : sections)
    {
        eLo = std::min(eLo, s.summary.energy);
        eHi = std::max(eHi, s.summary.energy);
    }
    const auto eNorm = [&](float e) { return eHi - eLo > 1e-3f ? (e - eLo) / (eHi - eLo) : 0.5f; };

    const float fieldShift = opt.mode == Mode::transfigure ? 7.0f : opt.mode == Mode::counter ? 6.0f : 0.0f;
    Motif original;

    for (std::size_t si = 0; si < sections.size(); ++si)
    {
        const auto& sec = sections[si];
        Scale scale = scaleFor(opt.pitch, sec, frames, rng, opt.seed, pickFor(si));
        scale.tonic = std::fmod(scale.tonic + fieldShift, 12.0f);
        // a máquina de escalas (ScaleMachine.h): da 2.ª seção em diante, a
        // escala nasce da anterior por um desvio, guiado pelo modo e pelo
        // campo desta seção (o sorteio de scaleFor acima continua, para os
        // outros sorteios da peça não mudarem)
        std::string machineHow;
        if ((opt.scaleMachine || opt.intention != Intention::none) && si > 0 && !sectionScales.empty())
        {
            const MachineMode mm = opt.mode == Mode::transfigure ? MachineMode::transfigure
                                 : opt.mode == Mode::counter     ? MachineMode::counter
                                 : opt.mode == Mode::interrupt   ? MachineMode::interrupt
                                                                 : MachineMode::follow;
            const double position = duration > 0.0 ? 0.5 * (sec.start + sec.end) / duration : 0.5;
            const auto st = nextScale(sectionScales.back().scale, sec.field, mm, opt.seed * 1000003ull + si,
                                      opt.scaleArchive ? opt.scaleArchive.get() : nullptr, static_cast<int>(opt.intention),
                                      position);
            scale.tonic = static_cast<float>(st.scale.tonic / 100.0);
            scale.steps.clear();
            for (double c : st.scale.cents)
                scale.steps.push_back(static_cast<float>(c / 100.0));
            scale.period = static_cast<float>(st.scale.period / 100.0);
            char b[320];
            std::snprintf(b, sizeof b, " · máquina de escalas: %s (%d de %zu notas em comum)", st.how.c_str(), st.common,
                          st.scale.cents.size());
            machineHow = b;
        }
        // a transição entre culturas: a escala da seção é um estado
        // intermediário entre as escalas da mistura, na ordem dada
        if (opt.mixTechnique == MixTechnique::transition && mixPicks.size() >= 2 && sections.size() > 1)
        {
            const double x = static_cast<double>(si) / static_cast<double>(sections.size() - 1) * (mixPicks.size() - 1);
            const std::size_t i0 = std::min(static_cast<std::size_t>(x), mixPicks.size() - 2);
            const double f = x - static_cast<double>(i0);
            const auto a = machineFromPick(mixPicks[i0]), b = machineFromPick(mixPicks[i0 + 1]);
            const auto st = interpolateScales(a, b, f);
            scale.tonic = static_cast<float>(st.tonic / 100.0);
            scale.steps.clear();
            for (double c : st.cents)
                scale.steps.push_back(static_cast<float>(c / 100.0));
            scale.period = static_cast<float>(st.period / 100.0);
            char b2[400];
            std::snprintf(b2, sizeof b2, " · mistura (transição): %.0f %% de %s a %s", 100.0 * f,
                          mixPicks[i0].choice.name.c_str(), mixPicks[i0 + 1].choice.name.c_str());
            machineHow += b2;
        }
        // forma por repetição: a seção que volta retoma a escala da primeira vez
        if (!formLabels.empty())
            if (const auto it = formMemory.find(formLabels[si]); it != formMemory.end())
                scale = it->second.scale;
        {
            MachineScale ms;
            ms.tonic = 100.0 * scale.tonic;
            for (float x : scale.steps)
                ms.cents.push_back(100.0 * x);
            ms.period = 100.0 * scale.period;
            ms.name = scale.name;
            sectionScales.push_back({ sec.start, sec.end, ms });
            // o afeto estimado da escala da seção (ScaleAffect.h), com a
            // máquina ou com uma intenção
            if (opt.scaleMachine || opt.intention != Intention::none)
                machineHow += " · afeto: " + scaleAffect(ms).describe();
        }
        if (si == 0)
            original = makeMotif(cell, scale, frames, stepDur, rng, contourFromReference);

        // registro pelo brilho (invertido em transfigurar e contrapor)
        const float b = sec.summary.brightness;
        const float center = 48.0f + 30.0f * (opt.mode == Mode::follow || opt.mode == Mode::interrupt ? b : 1.0f - b);
        float tonicMidi = scale.tonic;
        while (tonicMidi + 6.0f < center)
            tonicMidi += 12.0f;

        const float en = eNorm(sec.summary.energy);
        const float base = 0.35f + 0.55f * (opt.mode == Mode::counter ? 1.0f - en : en);

        const bool free = opt.time == TimeMode::free
                       || (opt.time == TimeMode::automatic && sec.pulse.confidence < 0.3f);
        if (!free)
            pulsedSpans.push_back({ sec.start, sec.end });

        // papel da seção na forma
        const bool last = si + 1 == sections.size() && sections.size() > 1;
        Motif m = original;
        std::string role;
        if (si == 0)
            role = "apresentação";
        else if (last)
        {
            role = "recapitulação";
            m = (opt.mode == Mode::transfigure) ? invert(original) : original;
        }
        else
        {
            struct Op { const char* name; Motif (*f)(Motif); };
            static const Op follow[] = { { "transposição", [](Motif x) { return transpose(x, 2); } },
                                         { "aumentação", augment } };
            static const Op transf[] = { { "inversão", invert }, { "retrógrado", retrograde } };
            static const Op counterOps[] = { { "deslocamento", displace }, { "inversão", invert } };
            static const Op inter[] = { { "fragmentação", [](Motif x) { return fragment(x); } }, { "diminuição", diminish } };
            const Op* ops = opt.mode == Mode::follow ? follow : opt.mode == Mode::transfigure ? transf
                          : opt.mode == Mode::counter ? counterOps : inter;
            const auto& op = ops[(si + static_cast<std::size_t>(rng.range(0, 1))) % 2];
            m = op.f(original);
            if (sec.summary.density > 0.6f && opt.mode == Mode::follow)
            {
                m = diminish(m);
                role = std::string("desenvolvimento (") + op.name + " + diminuição)";
            }
            else
                role = std::string("desenvolvimento (") + op.name + ")";
        }
        w.motifTimbre = motifTimbreFor(sec.summary, opt.mode, rng, opt.tendencies, opt.constraints);
        static const Timbre bassChoices[] = { Timbre::additive, Timbre::fm, Timbre::membrane, Timbre::upright };
        w.bassTimbre = bassChoices[static_cast<std::size_t>(rng.range(0, 3))];
        for (int k = 0; k < 4 && opt.constraints.noTimbre[static_cast<std::size_t>(w.bassTimbre)]; ++k)
            w.bassTimbre = bassChoices[static_cast<std::size_t>(k)];
        // forma por repetição: a seção que volta retoma o motivo, os timbres e o
        // papel da primeira vez (o refrão volta como refrão)
        if (!formLabels.empty())
        {
            const int lab = formLabels[si];
            if (const auto it = formMemory.find(lab); it != formMemory.end())
            {
                m = it->second.motif;
                w.motifTimbre = it->second.motifTimbre;
                w.bassTimbre = it->second.bassTimbre;
                role = std::string("retorno de ") + static_cast<char>('A' + lab % 26) + " (como na seção "
                     + std::to_string(it->second.first + 1) + ": " + it->second.role + ")";
            }
            else
            {
                formMemory[lab] = { scale, m, w.motifTimbre, w.bassTimbre, role, si };
                role = std::string(1, static_cast<char>('A' + lab % 26)) + " · " + role;
            }
        }
        char head[320];
        std::snprintf(head, sizeof head, "seção %zu (%.0f–%.0f s): %s · %s · %s/%s · campo: ", si + 1, sec.start, sec.end,
                      role.c_str(), free ? "tempo livre" : "pulso", timbreName(w.motifTimbre), timbreName(w.bassTimbre));
        score.marks.push_back({ sec.start, head + scale.describe() + machineHow });
        const std::size_t markIndex = score.marks.size() - 1;

        // ---- as frases (sentença de 4 compassos) ------------------------
        // `cur` é o motivo da frase corrente: deriva da frase anterior
        // (variação em desenvolvimento) e volta ao motivo da seção a cada
        // terceira frase, como âncora.
        Motif cur = m;
        std::vector<std::string> path;
        const auto nextPhrase = [&](int phrase) {
            if (phrase == 0)
                return;
            if (phrase % 3 == 2)
            {
                cur = m;
                path.push_back("retorno");
                return;
            }
            std::string name;
            cur = develop(cur, opt.mode, rng, name);
            if (cur.empty())
                cur = m;
            path.push_back(name);
        };
        const auto sentence = [&](int bar) -> Motif {
            const Motif& m = cur;
            switch (bar % 4)
            {
                case 0: return m;
                case 1: return vary(transpose(m, rng.chance(0.5) ? 1 : 2), rng);
                case 2:
                {
                    Motif a = fragment(m), out = a;
                    for (auto x : transpose(a, 1))
                    {
                        x.step += barSteps / 2;
                        out.push_back(x);
                    }
                    return out;
                }
                default:  // cadência: começo do motivo e nota longa num grau estável
                {
                    Motif c = fragment(m);
                    if (c.size() > 2)
                        c.resize(2);
                    const int lastStep = c.empty() ? 0 : c.back().step + c.back().len;
                    const bool half = (bar / 4) % 2 == 0 && !last;  // meia cadência na quinta
                    c.push_back({ std::min(lastStep, barSteps - 4), barSteps - std::min(lastStep, barSteps - 4),
                                  half ? scale.nearestDegree(7.0f) : 0, 0.6f });
                    return c;
                }
            }
        };
        const float arch[4] = { 0.85f, 0.95f, 1.05f, 0.9f };
        const int fifthDeg = scale.nearestDegree(7.0f);
        const float bassTonic = 36.0f + std::fmod(scale.tonic, 12.0f);

        if (!free)
        {
            // compassos da grade global que cabem na seção
            int bar = 0;
            const double first = pulse.phase + std::ceil((sec.start - pulse.phase) / barLen - 1e-9) * barLen;
            for (double bs = first; bs + barLen <= sec.end + 1e-6; bs += barLen, ++bar)
            {
                if (bar % 4 == 0)
                {
                    nextPhrase(bar / 4);
                    // registro da frase pelo brilho local
                    const auto ph = summarize(frames, onsets, bs, std::min(bs + 4 * barLen, sec.end));
                    if (hasTimbre(ph))
                    {
                        const float lb = ph.brightness;
                        const float c = 48.0f + 30.0f * (opt.mode == Mode::follow || opt.mode == Mode::interrupt ? lb : 1.0f - lb);
                        tonicMidi = std::fmod(scale.tonic, 12.0f);
                        while (tonicMidi + 6.0f < c)
                            tonicMidi += 12.0f;
                    }
                }
                const bool cut = opt.mode == Mode::interrupt && si > 0 && bar == 0;
                if (cut)
                    continue;  // silêncio: a mudança é marcada pela ausência
                Motif phrase = (opt.mode == Mode::interrupt && si > 0 && bar == 1) ? diminish(fragment(cur)) : sentence(bar);
                // dinâmica do compasso pela energia local, dentro do arco da frase
                const auto loc = summarize(frames, onsets, bs, bs + barLen);
                const float le = eNorm(loc.energy);
                const float barVel = (0.3f + 0.65f * (opt.mode == Mode::counter ? 1.0f - le : le)) * arch[bar % 4]
                                   * ((opt.mode == Mode::interrupt && si > 0 && bar == 1) ? 1.4f : 1.0f);
                // a barra da referência neste instante (ritmo vivo e hoqueto)
                const auto refIdx = static_cast<std::size_t>(std::lround((bs - (refBars.empty() ? 0.0 : refBars.front().start)) / barLen));
                const Bar* ref = refIdx < refBars.size() ? &refBars[refIdx] : nullptr;
                // respiração: onde a referência quase silencia, quem segue descansa
                if (!hasTimbre(loc) && opt.mode != Mode::counter)
                {
                    w.note(bs, barLen * 0.95, bassTonic, base * 0.4f, 1);
                    continue;
                }
                // ritmo vivo: o compasso toma o ritmo real da referência com o
                // contorno da frase (memória + reação), exceto na cadência
                if ((opt.mode == Mode::follow || opt.mode == Mode::transfigure) && ref && bar % 4 != 3
                    && ref->count() >= 2 && ref->count() <= 10 && !phrase.empty() && rng.chance(0.55))
                {
                    Motif live;
                    std::vector<int> hits;
                    for (int k = 0; k < barSteps; ++k)
                        if (ref->hits[static_cast<std::size_t>(k)] > 0.0f)
                            hits.push_back(k);
                    for (std::size_t i = 0; i < hits.size(); ++i)
                        live.push_back({ hits[i], (i + 1 < hits.size() ? hits[i + 1] : barSteps) - hits[i],
                                         phrase[i % phrase.size()].degree, ref->hits[static_cast<std::size_t>(hits[i])] });
                    phrase = live;
                }
                std::array<bool, kMaxStepsPerBar * 2> used {};
                for (auto x : phrase)
                {
                    if (opt.mode == Mode::counter && ref && x.step < barSteps)
                    {
                        // hoqueto: só onde a referência não ataca
                        int s = -1;
                        for (const int d : { 0, 1, -1, 2, -2 })
                        {
                            const int c = x.step + d;
                            if (c >= 0 && c < barSteps && ref->hits[static_cast<std::size_t>(c)] <= 0.0f && !used[static_cast<std::size_t>(c)])
                            {
                                s = c;
                                break;
                            }
                        }
                        if (s < 0)
                            continue;
                        x.len = std::max(1, x.len - (s - x.step));
                        x.step = s;
                    }
                    if (x.step >= 0 && x.step < barSteps * 2)
                        used[static_cast<std::size_t>(x.step)] = true;
                    const float pitch = scale.pitch(x.degree, tonicMidi);
                    const float glide = opt.pitch == PitchSource::microtonal && rng.chance(0.35) ? static_cast<float>(rng.uniform() * 2.0 - 1.0) : 0.0f;
                    w.note(bs + x.step * stepDur, x.len * stepDur * 0.92, pitch, barVel * (0.85f + 0.3f * x.accent), 0, glide);
                }
                // baixo: fundamental; quinta na cadência
                const float bassPitch = bassTonic + (bar % 4 == 3 ? scale.steps[static_cast<std::size_t>(fifthDeg)] : 0.0f);
                w.note(bs, barLen * 0.95, bassPitch, base * 0.6f, 1);
            }
        }
        else
        {
            // tempo livre: frases com rubato e respirações
            double t = sec.start + 0.2;
            int bar = 0;
            const double freeBar = std::clamp(barLen, 1.6, 3.2);
            while (t + freeBar * 0.8 < sec.end)
            {
                const double stretch = (bar % 4 == 2 ? 0.85 : bar % 4 == 3 ? 1.3 : 1.0) * (0.92 + 0.16 * rng.uniform());
                const double step = freeBar * stretch / barSteps;
                for (const auto& x : sentence(bar))
                {
                    const double jitter = (rng.uniform() - 0.5) * 0.25 * step;
                    const float pitch = scale.pitch(x.degree, tonicMidi);
                    const float glide = opt.pitch == PitchSource::microtonal && rng.chance(0.35) ? static_cast<float>(rng.uniform() * 2.0 - 1.0) : 0.0f;
                    w.note(t + x.step * step + jitter, x.len * step * 0.95, pitch, base * arch[bar % 4] * (0.85f + 0.3f * x.accent), 0, glide);
                }
                if (bar % 4 == 0)
                    w.note(t, freeBar * 4.0 * 0.9, bassTonic, base * 0.5f, 1);
                t += freeBar * stretch;
                if (bar % 4 == 3)
                    t += 0.4 + 0.8 * rng.uniform();  // respiração entre frases
                ++bar;
                if (bar % 4 == 0)
                    nextPhrase(bar / 4);
            }
        }
        if (!path.empty())
        {
            std::string p = " · frases: ";
            for (std::size_t i = 0; i < path.size() && i < 8; ++i)
                p += (i ? " → " : "") + path[i];
            if (path.size() > 8)
                p += " → …";
            score.marks[markIndex].text += p;
        }
    }
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) {
        return a.time < b.time || (a.time == b.time && a.voice < b.voice);
    });
    char head[260];
    std::snprintf(head, sizeof head, "pulso da referência: %.1f BPM%s, compasso de %d tempos (confiança %.2f) · célula: %d ataques · contorno: %s",
                  pulse.bpm,
                  opt.tempoScale < 0.99 ? " (metade do lido, pela escuta do autor)" : opt.tempoScale > 1.01 ? " (o dobro do lido, pela escuta do autor)" : "",
                  pulse.beatsPerBar, pulse.confidence, cell.count(),
                  contourFromReference ? "da melodia da referência" : "inventado");
    score.marks.insert(score.marks.begin(), { 0.0, head });
    // harmonia em movimento (camada 1 de docs/MUSICALIDADE.md): antes das
    // outras camadas, para elas (e o olho) já ouvirem os acordes
    // o comportamento da escala do arquivo (ScaleIdiom.h): a melodia segue a
    // linguagem (subida/descida, percurso, apoio, assinatura, cadência,
    // ornamento); com textura "bordão", um bordão no lugar dos acordes
    // o comportamento de cada seção: da escala do arquivo, ou, na mistura,
    // da cultura daquela seção (colagem) ou da mais próxima (transição)
    std::vector<const ScaleBehavior*> perSection(sectionScales.size(), nullptr);
    for (std::size_t k = 0; k < sectionScales.size(); ++k)
    {
        if (opt.mixTechnique == MixTechnique::transition && mixPicks.size() >= 2 && sectionScales.size() > 1)
        {
            const double x = static_cast<double>(k) / static_cast<double>(sectionScales.size() - 1) * (mixPicks.size() - 1);
            perSection[k] = &mixPicks[std::min(static_cast<std::size_t>(std::lround(x)), mixPicks.size() - 1)].behavior;
        }
        else if (const auto* p = pickFor(k))
            perSection[k] = &p->behavior;
    }
    {
        const auto line = applyScaleIdiom(score, sectionScales, perSection, opt.seed);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
    }
    // a textura de cada seção: acordes (harmonia em movimento), bordão ou
    // heterofonia; numa colagem, cada trecho com a sua
    std::vector<SectionScale> droneSecs, hetSecs;
    bool anyChords = false, anyVamp = false;
    for (std::size_t k = 0; k < sectionScales.size(); ++k)
    {
        const std::string tex = perSection[k] ? perSection[k]->texture : "";
        if (tex == "bordão")
            droneSecs.push_back(sectionScales[k]);
        else if (tex == "heterofonia")
            hetSecs.push_back(sectionScales[k]);
        else if (tex == "vamp")
            anyVamp = true;
        else
            anyChords = true;
    }
    if (anyChords || sectionScales.empty())
        harmonyInMotion(score, harmonyFrames, pulse.phase, pulse.period, fieldShift, sectionScales, opt);
    if (!hetSecs.empty())
        score.marks.insert(score.marks.begin() + 1, { 0.0, heterophonyLayer(score, hetSecs, opt.seed) });
    if (!droneSecs.empty() && opt.harmony)
        score.marks.insert(score.marks.begin() + 1, { 0.0, droneLayer(score, droneSecs, opt) });
    if (anyVamp && opt.harmony)
    {
        const auto line = vampLayer(score, sectionScales, perSection, barLen, opt);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
    }
    if (layerOther)
    {
        // a segunda escala precisa de uma voz que a cante: sem segunda voz,
        // uma heterofônica é acrescentada (mantendo bordão e acordes)
        if (std::none_of(score.notes.begin(), score.notes.end(), [](const Note& n) { return n.voice == 2; }))
            (void) heterophonyLayer(score, sectionScales, opt.seed + 1, true);
        score.marks.insert(score.marks.begin() + 1, { 0.0, layerSecondScale(score, sectionScales, *layerOther) });
    }
    // ritmo como campo (Atlas §45, Grids): a camada de percussão nos trechos com pulso
    if (opt.rhythm)
    {
        if (other && (opt.otherLayers & kMashRhythm))
        {
            // o groove da outra: os compassos dela, no pulso dela (já no tempo desta)
            const Pulse pb = estimatePulse(*opt.otherFrames, *opt.otherOnsets, 0.0, duration);
            if (pb.period > 0.0)
                rhythmField(score, rhythmGrid(*opt.otherOnsets, pb, 0.0, duration), pb, pulsedSpans, opt);
            else
                rhythmField(score, refBars, pulse, pulsedSpans, opt);
        }
        else
            rhythmField(score, refBars, pulse, pulsedSpans, opt);
    }
    // os ciclos das culturas: o pedido para a peça (--ciclo), ou o ciclo
    // usual da escala de cada seção (comportamento "ritmo")
    if (opt.rhythm && opt.rhythmArchive && !opt.rhythmArchive->empty())
    {
        std::vector<const RhythmCycle*> cycles(sectionScales.size(), nullptr);
        std::vector<std::pair<double, double>> spans;
        const RhythmCycle* forced = opt.cycleQuery.empty() ? nullptr : findCycle(*opt.rhythmArchive, opt.cycleQuery);
        if (!opt.cycleQuery.empty() && !forced)
            score.marks.push_back({ 0.0, "ciclo rítmico: nenhum (ou mais de um) com \"" + opt.cycleQuery + "\"" });
        for (std::size_t k = 0; k < sectionScales.size(); ++k)
        {
            spans.push_back({ sectionScales[k].start, sectionScales[k].end });
            if (forced)
                cycles[k] = forced;
            else if (k < perSection.size() && perSection[k] && !perSection[k]->rhythm.empty())
                cycles[k] = findCycle(*opt.rhythmArchive, perSection[k]->rhythm);
        }
        const auto line = cultureRhythm(score, spans, cycles, pulse, opt);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
        // a melodia chega no ciclo (sam, gong, dum) e os tala fecham com tihai
        const auto arrival = alignMelodyToCycles(score, sectionScales, cycles, perSection, pulse);
        if (!arrival.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, arrival });
    }
    // as costuras entre as seções (Recombination.h): pivô, ponte, sobreposição
    if (opt.seam != 0)
    {
        const auto line = seamSections(score, sectionScales, static_cast<Seam>(opt.seam), barLen);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
    }
    // o groove da referência (Groove.h): a grade ganha o balanço medido —
    // antes das memórias (que vêm da referência e já têm o tempo dela) e do olho
    if (opt.groove)
    {
        const double amount = opt.mode == Mode::counter ? 0.0 : opt.mode == Mode::transfigure ? 0.6 : 1.0;
        const auto groove = measureGroove(onsets, pulse, 0.0, duration);
        const auto line = applyGroove(score, groove, pulse, pulsedSpans, amount);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
        else if (opt.mode == Mode::counter && groove.valid && groove.swing8 != 0.0f)
            score.marks.insert(score.marks.begin() + 1, { 0.0, "groove: a referência balança; contrapor fica na grade reta" });
        // o rubato nas seções de tempo livre (as que não estão em pulsedSpans):
        // seguir 1, transfigurar 1,5 (mais fundo), interromper 0,5, contrapor reto
        std::vector<std::pair<double, double>> freeSpans;
        for (const auto& ss : sectionScales)
        {
            const bool pulsed = std::any_of(pulsedSpans.begin(), pulsedSpans.end(),
                                            [&](const auto& p) { return std::abs(p.first - ss.start) < 1e-6; });
            if (!pulsed)
                freeSpans.push_back({ ss.start, ss.end });
        }
        const double depth = opt.mode == Mode::counter ? 0.0 : opt.mode == Mode::transfigure ? 1.5 : opt.mode == Mode::interrupt ? 0.5 : 1.0;
        const auto rub = applyRubato(score, freeSpans, depth);
        if (!rub.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, rub });
    }
    // a memória performativa (Atlas §63, OMax): a referência recombinada pelo
    // oráculo de fatores, antes do olho, para ele também a ler
    oracleMemory(score, memoryFrames, memoryOnsets, opt);
    // e pelo som (Atlas §62, CataRT): o mosaico por descritores
    descriptorMosaic(score, memoryFrames, memoryOnsets, opt);
    // a dramaturgia (Dramaturgy.h): depois das memórias (que também rareiam
    // nos vales) e antes do olho (que pode quebrar até o silêncio composto)
    Options eyeOpt = opt;  // o olho recebe a tensão da peça (Options::tensionAt)
    std::shared_ptr<TensionCurve> pieceCurve;  // a mesma curva molda os gestos depois do olho
    if (opt.dramaturgy)
    {
        auto curve = std::make_shared<TensionCurve>(!opt.gesture.empty()   ? gestureCurve(opt.gesture, duration)
                                                    : !opt.narrative.empty() ? narrativeCurve(opt.narrative, duration)
                                                                             : tensionCurve(frames, duration, opt.mode, opt.seed));
        const auto line = applyDramaturgy(score, *curve, pulse, opt.seed, &eyeOpt.breath);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
        eyeOpt.tensionAt = [curve](double t) { return curve->at(t); };
        pieceCurve = curve;
    }
    // o diálogo (Dialogue.h): depois da dramaturgia, antes do olho
    if (opt.dialogue)
    {
        auto map = dialogueMap(frames, duration, pulse);
        // a troca de lugar (princípio 4) é um lance do jogo
        DialogueSwap swap;
        if (opt.game)
        {
            swap = chooseDialogueSwap(map, duration, opt.mode, opt.seed);
            map = swapDialogue(map, swap);
        }
        const auto line = applyDialogue(score, map);
        if (swap.kind != 0)
            score.marks.insert(std::upper_bound(score.marks.begin(), score.marks.end(), swap.from,
                                                [](double t, const Mark& m) { return t < m.time; }),
                               { swap.from, dialogueSwapText(swap) });
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
        const auto echo = imitateCalls(score, map, frames, onsets, opt.mode);
        if (!echo.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, echo });
    }
    // o jogo (Game.h, docs/JOGO.md): as regras inventadas entram depois do
    // diálogo e antes do olho (que ainda pode quebrá-las)
    if (opt.game)
    {
        const auto lines = playGame(score, frames, onsets, duration, opt);
        for (auto it = lines.rbegin(); it != lines.rend(); ++it)
            score.marks.insert(score.marks.begin() + 1, { 0.0, *it });
    }
    // o conjunto por estilo (Ensemble.h, docs/MAQUINA_DE_TIMBRES.md §4): os
    // timbres pelos papéis do estilo, antes do olho (que ainda pode desviar)
    // o conjunto lido na referência (opt.ensemble = −1): a escuta escolhe o
    // estilo e o modo responde (Ensemble.h, readEnsemble)
    int ensemble = opt.ensemble;
    if (ensemble < 0)
    {
        const auto reading = readEnsemble(frames, onsets, duration, pulse, pulsedSpans, opt.rhythmArchive.get(), opt.mode, opt.seed);
        ensemble = static_cast<int>(reading.chosen);
        score.marks.insert(score.marks.begin() + 1, { 0.0, reading.why });
    }
    if (ensemble > 0 && ensemble < kEnsembles)
    {
        score.ensemble = ensemble;
        const auto line = applyEnsemble(score, static_cast<Ensemble>(ensemble), opt.constraints);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
        // as figuras do estilo (StyleFigures.h): a pompe, walking, baixaria, tumbao…
        const auto fig = applyStyleFigures(score, static_cast<Ensemble>(ensemble), pulse, pulsedSpans, opt.rockStyle);
        if (!fig.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, fig });
        const auto phr = applyStylePhrasing(score, static_cast<Ensemble>(ensemble), pulse, pulsedSpans, opt.seed, opt.rockStyle);
        if (static_cast<Ensemble>(ensemble) == Ensemble::rock)
            score.marks.insert(score.marks.begin() + 1,
                               { 0.0, std::string("rock: o subestilo ") + rockStyleName(static_cast<RockStyle>(std::clamp(opt.rockStyle, 0, kRockStyles - 1))) });
        if (!phr.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, phr });
    }
    // os eventos da gramática narrativa (a virada do ki-shō-ten-ketsu)
    if (!opt.narrative.empty())
    {
        const auto line = applyNarrativeEvents(score, opt.narrative, opt.seed);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
    }
    // as influências (Influences.h): as vozes que a tendência deixou de fora e
    // os eixos de caráter (dança, peso), sobre qualquer conjunto
    if (opt.influenceDraw)
    {
        const auto line = applyInfluenceScore(score, *opt.influenceDraw, opt.seed, pieceCurve.get(), opt.narrative);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
    }
    // o som da aura (SomDaAura.h): a fala da referência, sílaba a sílaba,
    // vira a melodia; cada nota ganha o seu acorde
    if (opt.somDaAura)
    {
        const auto line = applySomDaAura(score, frames, onsets, duration, opt.seed);
        score.marks.insert(score.marks.begin() + 1, { 0.0, line.empty() ? std::string("som da aura: a referência não tem voz bastante") : line });
    }
    // o rasgo modular (ModularRasgo.h): os corpos atravessados por patches,
    // por voz e por seção, mais fundo na tensão
    if (opt.rasgo > 0.0f)
    {
        std::vector<double> starts;
        for (const auto& ss : sectionScales)
            starts.push_back(ss.start);
        const auto rCurve = !opt.gesture.empty()     ? gestureCurve(opt.gesture, duration)
                            : !opt.narrative.empty() ? narrativeCurve(opt.narrative, duration)
                                                     : tensionCurve(frames, duration, opt.mode, opt.seed);
        const auto line = applyModularRasgo(score, opt.rasgo, rCurve, starts, opt.seed);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
    }
    // os efeitos de cinema (FilmEffects.h): na dramaturgia da resposta
    if (opt.filmFx)
    {
        std::vector<double> starts;
        for (const auto& ss : sectionScales)
            starts.push_back(ss.start);
        const auto fxCurve = !opt.gesture.empty()     ? gestureCurve(opt.gesture, duration)
                            : !opt.narrative.empty() ? narrativeCurve(opt.narrative, duration)
                                                     : tensionCurve(frames, duration, opt.mode, opt.seed);
        const auto line = applyFilmEffects(score, fxCurve, starts, summarize(frames, onsets, 0.0, duration).brightness, opt.seed);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
    }
    // num conjunto de estilo, o olho fala a língua do estilo (a escuta do
    // autor, 16/10: "não soa como jazz"): sem os objetos sonoros (cluster,
    // drone, nuvem, faixa de ruído, corpo…) nem as regras que quebram o
    // idioma (registro invertido, tempo deformado, escala mutante, troca de
    // timbre); ficam o eco em cânone, o silêncio súbito, densificar,
    // rarear e congelar; e ele age a metade. Com o rasgo modular em 0,5 ou
    // mais, o olho volta inteiro: aí o inaudito é o pedido.
    if (ensemble > 0 && opt.rasgo < 0.5f)
    {
        for (int c = 0; c < 8; ++c)
            eyeOpt.constraints.noEye[static_cast<std::size_t>(c)] = true;
        for (int r : { 0, 1, 2, 5 })  // mirror, warp, mutate, retimbre
            eyeOpt.constraints.noEye[static_cast<std::size_t>(8 + r)] = true;
        eyeOpt.unpredictability *= 0.5f;
        score.marks.insert(score.marks.begin() + 1,
                           { 0.0, std::string("Terceiro Olho no idioma de ") + ensembleName(static_cast<Ensemble>(ensemble))
                                      + ": só eco, silêncio, densificar, rarear e congelar, com metade da imprevisibilidade" });
    }
    thirdEye(score, frames, onsets, eyeOpt);
    if (pieceCurve)
        (void) shapeGestures(score, *pieceCurve);
    // as notas que o olho criou depois (o eco em cânone) voltam ao instrumento
    // do seu papel no conjunto
    if (ensemble > 0 && ensemble < kEnsembles && opt.rasgo < 0.5f)
        (void) applyEnsemble(score, static_cast<Ensemble>(ensemble), opt.constraints, true);
    applyPitchRules(score, opt.constraints);
    if (opt.humanize)
    {
        const auto ex = applyExpression(score, opt.expressiveEnding);
        if (!ex.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, ex });
        const auto line = humanize(score, opt.seed);
        if (!line.empty())
            score.marks.insert(score.marks.begin() + 1, { 0.0, line });
    }
    if (!opt.constraints.empty())
    {
        std::string text = "regras do autor:";
        for (const auto& l : opt.constraints.lines)
            text += " " + l + ";";
        text.pop_back();
        score.marks.insert(score.marks.begin() + 1, { 0.0, text });
    }
    return score;
}

bool parsePitchSource(const std::string& n, PitchSource& out)
{
    if (n == "referencia") out = PitchSource::reference;
    else if (n == "modal") out = PitchSource::modal;
    else if (n == "espectral") out = PitchSource::spectral;
    else if (n == "microtonal") out = PitchSource::microtonal;
    else if (n == "catalogo") out = PitchSource::modal;
    else if (n == "inventada") out = PitchSource::invented;
    else if (n == "edo") out = PitchSource::edo;
    else if (n == "justa") out = PitchSource::just;
    else if (n == "pitagorica") out = PitchSource::pythagorean;
    else if (n == "quartos") out = PitchSource::quarterTone;
    else if (n == "arquivo") out = PitchSource::archive;
    else return false;
    return true;
}

bool parseTimeMode(const std::string& n, TimeMode& out)
{
    if (n == "pulso") out = TimeMode::pulse;
    else if (n == "livre") out = TimeMode::free;
    else if (n == "auto") out = TimeMode::automatic;
    else return false;
    return true;
}

const char* pitchSourceName(PitchSource p)
{
    switch (p)
    {
        case PitchSource::reference: return "referencia";
        case PitchSource::modal: return "modal";
        case PitchSource::spectral: return "espectral";
        case PitchSource::microtonal: return "microtonal";
        case PitchSource::invented: return "inventada";
        case PitchSource::edo: return "edo";
        case PitchSource::just: return "justa";
        case PitchSource::pythagorean: return "pitagorica";
        case PitchSource::quarterTone: return "quartos";
        case PitchSource::archive: return "arquivo";
    }
    return "?";
}

const char* timeModeName(TimeMode t)
{
    switch (t)
    {
        case TimeMode::pulse: return "pulso";
        case TimeMode::free: return "livre";
        case TimeMode::automatic: return "auto";
    }
    return "?";
}

} // namespace trioio::composition
