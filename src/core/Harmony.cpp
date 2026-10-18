#include "Harmony.h"
#include "HarmonyWriting.h"
#include "Ensemble.h"

// cópia fixada do Rasgo Synth Studio (third_party/rasgo_synth_dsp/PROVENANCE.md)
#include "sequencer/NeoRiemannian.hpp"

#include "Voicing.h"

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

const char* const kPc[12] = { "Dó", "Dó#", "Ré", "Mib", "Mi", "Fá", "Fá#", "Sol", "Lá♭", "Lá", "Sib", "Si" };

int mod12(int x) { return ((x % 12) + 12) % 12; }

// o croma de um trecho, pesado pela amplitude de cada quadro
std::array<double, 12> chromaOf(const std::vector<Frame>& frames, double t0, double t1, double& weight)
{
    std::array<double, 12> c {};
    weight = 0.0;
    for (const auto& f : frames)
    {
        if (f.time < t0 || f.time >= t1 || f.silent)
            continue;
        const double w = std::pow(10.0, f.rmsDb / 20.0);
        for (std::size_t k = 0; k < 12; ++k)
            c[k] += w * f.chroma[k];
        weight += w;
    }
    return c;
}

// o grave de [t0, t1), somando 1 quando há grave
std::array<double, 12> bassOf(const std::vector<Frame>& frames, double t0, double t1)
{
    std::array<double, 12> c {};
    double total = 0.0;
    for (const auto& f : frames)
    {
        if (f.time < t0 || f.time >= t1 || f.silent)
            continue;
        const double w = std::pow(10.0, f.rmsDb / 20.0);
        for (std::size_t k = 0; k < 12; ++k)
            c[k] += w * f.bassChroma[k];
    }
    for (double x : c)
        total += x;
    if (total > 0.0)
        for (auto& x : c)
            x /= total;
    return c;
}

rasgo::Triad toTriad(const Chord& c) { return { c.root, c.quality != ChordQuality::minor }; }
Chord fromTriad(const rasgo::Triad& t) { return { t.rootPc, t.isMajor ? ChordQuality::major : ChordQuality::minor }; }

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

std::vector<int> Chord::pitchClasses() const
{
    const int third = (color & kSus4) ? 5 : (quality == ChordQuality::major || quality == ChordQuality::augmented) ? 4 : 3;
    const int fifth = quality == ChordQuality::diminished ? 6 : quality == ChordQuality::augmented ? 8 : 7;
    std::vector<int> out { root, mod12(root + third), mod12(root + fifth) };
    const auto add = [&](int iv) {
        const int pc = mod12(root + iv);
        if (std::find(out.begin(), out.end(), pc) == out.end())
            out.push_back(pc);
    };
    if (color & kSixth) add(quality == ChordQuality::diminished ? 9 : 9);
    if (color & kSeventh) add(10);
    if (color & kMajorSeventh) add(11);
    if (color & kFlatNinth) add(13);
    if (color & kNinth) add(14);
    if (color & kSharpEleventh) add(18);
    if (color & kThirteenth) add(21);
    return out;
}

bool Chord::contains(int pc) const
{
    const auto p = pitchClasses();
    return std::find(p.begin(), p.end(), mod12(pc)) != p.end();
}

std::string Chord::name() const
{
    std::string n = std::string(kPc[mod12(root)])
                  + (quality == ChordQuality::minor ? "m" : quality == ChordQuality::diminished ? "°" : quality == ChordQuality::augmented ? "+" : "");
    if (color & kSixth) n += quality == ChordQuality::diminished ? "7" : "6";
    if (color & kMajorSeventh) n += "7M";
    if (color & kSeventh) n += quality == ChordQuality::diminished ? "(♭5)" : "7";
    if (color & kFlatNinth) n += "(♭9)";
    if (color & kNinth) n += (color & (kSeventh | kMajorSeventh | kSixth)) ? "(9)" : "(add9)";
    if (color & kSharpEleventh) n += "(♯11)";
    if (color & kThirteenth) n += "(13)";
    if (color & kSus4) n += "sus4";
    return n;
}

std::vector<ChordSpan> readChords(const std::vector<Frame>& frames, double duration, double gridStart, double unit)
{
    std::vector<ChordSpan> out;
    if (unit <= 0.1 || duration <= 0.0)
        return out;
    constexpr float kMinConfidence = 0.55f;
    constexpr double kChange = 0.18;  // o custo de trocar de acorde
    constexpr double kBassRoot = 0.30, kBassTone = 0.08;  // o voto do grave (croma do grave, analisador v6)
    // 1. o croma de cada unidade (um tempo). Ler por compasso misturava duas
    //    harmonias quando a fase do compasso não coincidia com a troca, e a
    //    mistura inventava um terceiro acorde (Dóm + Sib → "Mib").
    std::vector<std::array<double, 12>> chroma, bass;
    std::vector<double> starts;
    double t = gridStart - unit * std::ceil(gridStart / unit);  // a unidade que contém 0
    for (; t < duration - 0.05; t += unit)
    {
        const double a = std::max(0.0, t), b = std::min(duration, t + unit);
        if (b - a < 0.5 * unit)
            continue;
        double w = 0.0;
        chroma.push_back(chromaOf(frames, a, b, w));
        bass.push_back(bassOf(frames, a, b));
        starts.push_back(a);
    }
    const std::size_t n = chroma.size();
    if (n == 0)
        return out;
    // 2. o melhor caminho pelos 36 acordes (Viterbi, como no reconhecimento
    //    de acordes por HMM de Sheh e Ellis, 2003): cada tempo vota nos
    //    acordes que contêm as suas notas, e trocar custa. Numa linha de voz
    //    só, as notas de vários tempos se somam até formar o acorde: a
    //    harmonia nasce da linha.
    std::vector<Chord> chords;
    for (int r = 0; r < 12; ++r)
        for (ChordQuality q : { ChordQuality::major, ChordQuality::minor, ChordQuality::diminished })
            chords.push_back({ r, q });
    const std::size_t K = chords.size();
    std::vector<std::vector<double>> score(n, std::vector<double>(K, 0.0));
    std::vector<bool> silent(n, true);
    for (std::size_t i = 0; i < n; ++i)
    {
        double norm = 0.0;
        for (double x : chroma[i])
            norm += x * x;
        norm = std::sqrt(norm);
        silent[i] = norm <= 0.0;
        if (silent[i])
            continue;
        for (std::size_t k = 0; k < K; ++k)
        {
            const auto pcs = chords[k].pitchClasses();
            const double wt[3] = { 1.0, 0.8, 0.8 };
            double dot = 0.0;
            for (std::size_t j = 0; j < 3; ++j)
                dot += wt[j] * chroma[i][static_cast<std::size_t>(pcs[j])];
            score[i][k] = dot / (norm * std::sqrt(2.28)) - (chords[k].quality == ChordQuality::diminished ? 0.04 : 0.0);
            // o grave vota: a fundamental no baixo é a leitura forte; a terça
            // ou a quinta no baixo (inversão) contam pouco. Sem grave, nada.
            const auto& bs = bass[i];
            score[i][k] += kBassRoot * bs[static_cast<std::size_t>(pcs[0])]
                         + kBassTone * (bs[static_cast<std::size_t>(pcs[1])] + bs[static_cast<std::size_t>(pcs[2])]);
        }
    }
    std::vector<double> v(score[0]);
    std::vector<std::vector<int>> back(n, std::vector<int>(K, 0));
    for (std::size_t i = 1; i < n; ++i)
    {
        std::size_t bestPrev = 0;
        for (std::size_t k = 1; k < K; ++k)
            if (v[k] > v[bestPrev]) bestPrev = k;
        std::vector<double> nv(K);
        for (std::size_t k = 0; k < K; ++k)
        {
            const bool stay = v[k] >= v[bestPrev] - kChange;
            back[i][k] = stay ? static_cast<int>(k) : static_cast<int>(bestPrev);
            nv[k] = score[i][k] + (stay ? v[k] : v[bestPrev] - kChange);
        }
        v.swap(nv);
    }
    std::vector<int> path(n);
    path[n - 1] = static_cast<int>(std::max_element(v.begin(), v.end()) - v.begin());
    for (std::size_t i = n - 1; i > 0; --i)
        path[i - 1] = back[i][static_cast<std::size_t>(path[i])];
    // 3. trechos: tempos seguidos com o mesmo acorde; a confiança é a média
    //    do voto; trechos fracos ou em silêncio ficam de fora
    for (std::size_t i = 0; i < n;)
    {
        std::size_t j = i;
        double sum = 0.0;
        int voiced = 0;
        while (j < n && path[j] == path[i])
        {
            if (!silent[j])
            {
                sum += score[j][static_cast<std::size_t>(path[i])];
                ++voiced;
            }
            ++j;
        }
        const float conf = voiced ? static_cast<float>(sum / voiced) : 0.0f;
        if (conf >= kMinConfidence)
            out.push_back({ starts[i], std::min(duration, starts[j - 1] + unit), chords[static_cast<std::size_t>(path[i])], conf });
        i = j;
    }
    return out;
}

std::array<float, 12> languageField(const std::vector<Frame>& frames, const std::vector<perception::Onset>& onsets,
                                    double duration)
{
    std::array<float, 12> out {};
    std::array<double, 12> mel {}, chordPc {};
    for (const auto& f : frames)
        if (!f.silent && f.melodyHz > 0.0f)
            mel[static_cast<std::size_t>(mod12(static_cast<int>(std::lround(69.0 + 12.0 * std::log2(f.melodyHz / 440.0)))))] += 1.0;
    const auto p = perception::estimatePulse(frames, onsets, 0.0, duration);
    if (p.period > 0.0)
        for (const auto& s : readChords(frames, duration, p.phase, p.period))
            for (int pc : s.chord.pitchClasses())
                chordPc[static_cast<std::size_t>(mod12(pc))] += s.end - s.start;
    double sm = 0.0, sc = 0.0;
    for (std::size_t i = 0; i < 12; ++i) { sm += mel[i]; sc += chordPc[i]; }
    if (sm <= 0.0 && sc <= 0.0)
        return perception::harmonicField(frames, 0.0, duration);  // sem melodia nem acordes: o croma
    const double wm = sc > 0.0 ? 0.5 : 1.0, wc = sm > 0.0 ? 0.5 : 1.0;
    for (std::size_t i = 0; i < 12; ++i)
        out[i] = static_cast<float>((sm > 0.0 ? wm * mel[i] / sm : 0.0) + (sc > 0.0 ? wc * chordPc[i] / sc : 0.0));
    return out;
}

void harmonyInMotion(Score& score, const std::vector<Frame>& frames, double beatStart, double beatLength, float shift,
                     const std::vector<SectionScale>& scales, const Options& opt)
{
    if (!opt.harmony)
        return;
    const bool twelveTone = opt.pitch == PitchSource::reference || opt.pitch == PitchSource::modal
                         || opt.pitch == PitchSource::invented;
    // pelos graus da escala: com a máquina de escalas ou fora dos 12 sons
    const bool machine = opt.scaleMachine || opt.intention != Intention::none;
    const bool byDegrees = machine || !twelveTone;
    if (byDegrees && scales.empty())
    {
        score.marks.push_back({ 0.0, "harmonia em movimento: desligada (sem escalas por seção)" });
        return;
    }
    // a unidade: o tempo, se for de 0,3 a 1,2 s; senão 0,5 s (tempo livre)
    const double beat = beatLength >= 0.3 && beatLength <= 1.2 ? beatLength : 0.5;
    auto spans = readChords(frames, score.duration, beatStart, beat);
    if (spans.size() < 2)
        return;
    const auto heard = spans;

    // 1. a transformação do modo (nos 12 sons; pelos graus, quem transforma
    //    é a máquina de escalas, de seção em seção)
    Rng rng { opt.seed * 0xA0761D6478BD642Full + 0x4A4D };
    const int sh = static_cast<int>(std::lround(shift));
    std::string how = "como a referência";
    for (auto& s : spans)
        s.chord.root = mod12(s.chord.root + sh);
    if (!byDegrees && opt.mode == strategy::Mode::transfigure)
    {
        static const char* const kOps[] = { "L", "R", "P", "LP", "RP", "PL", "LR" };
        const char* op = kOps[static_cast<int>(rng.uniform() * 7.0) % 7];
        for (auto& s : spans)
        {
            auto t = toTriad(s.chord);
            for (const char* c = op; *c; ++c)
                t = *c == 'L' ? rasgo::applyLeadingTone(t) : *c == 'R' ? rasgo::applyRelative(t) : rasgo::applyParallel(t);
            s.chord = fromTriad(t);
        }
        how = std::string("transfigurada pela transformação neo-riemanniana ") + op;
    }
    else if (!byDegrees && opt.mode == strategy::Mode::counter)
    {
        int subs = 0, mediants = 0;
        for (std::size_t i = 0; i < spans.size(); ++i)
        {
            auto& c = spans[i].chord;
            const bool resolves = i + 1 < spans.size() && mod12(spans[i + 1].chord.root - c.root) == 5;
            if (resolves && c.quality == ChordQuality::major)
            {
                c.root = mod12(c.root + 6);  // substituição de trítono: a dominante a um trítono
                ++subs;
            }
            else if (rng.uniform() < 0.35)
            {
                c.root = mod12(c.root + (rng.uniform() < 0.5 ? 3 : -3) + (rng.uniform() < 0.5 ? 1 : 0));
                ++mediants;
            }
        }
        char b[120];
        std::snprintf(b, sizeof b, "rearmonizada: %d substituições de trítono, %d mediantes cromáticas", subs, mediants);
        how = b;
    }
    else if (opt.mode == strategy::Mode::interrupt)
        how = "em fragmentos";

    // 1b. a harmonia escrita (HarmonyWriting.h): as cadências, as dominantes
    //     secundárias e as preparações de modulação, nos 12 sons
    std::string written;
    std::vector<CadenceMark> cadences;
    HarmonyLanguage language = HarmonyLanguage::automatic;
    if (!byDegrees && opt.writeHarmony)
    {
        std::vector<std::pair<double, double>> secs;
        for (const auto& x : scales)
            secs.push_back({ x.start, x.end });
        language = resolveLanguage(opt.harmonyLanguage, opt.ensemble, opt.mode);
        // o subestilo do rock escolhe a sua harmonia quando o autor não escolheu
        if (opt.harmonyLanguage == 0 && opt.ensemble == static_cast<int>(Ensemble::rock))
            switch (static_cast<RockStyle>(std::clamp(opt.rockStyle, 0, kRockStyles - 1)))
            {
                case RockStyle::heavy:
                case RockStyle::psychedelic: language = HarmonyLanguage::modal; break;
                case RockStyle::prog: language = HarmonyLanguage::quartal; break;
                case RockStyle::postrock: language = HarmonyLanguage::stasis; break;
                case RockStyle::noise: language = HarmonyLanguage::clusters; break;
                default: break;
            }
        written = writeHarmony(spans, secs, beat, opt.mode, language, opt.seed, &cadences, &score.spectra);
        if (opt.cadenceArchive)
        {
            std::vector<std::string> invented;
            if (const int drawn = drawArchiveCadences(spans, cadences, *opt.cadenceArchive, language, beat, opt.seed, 0.4, 0.5, &invented);
                drawn > 0)
            {
                written += ", " + std::to_string(drawn) + " chegadas sorteadas no arquivo de cadências";
                if (!invented.empty())
                {
                    written += " (" + std::to_string(invented.size()) + " inventadas pela mistura conceitual: ";
                    for (std::size_t k = 0; k < invented.size(); ++k)
                        written += (k ? "; " : "") + invented[k];
                    written += ")";
                }
            }
        }
    }

    // 2. os alvos: o acorde de cada trecho como alturas (classes em semitons,
    //    com fração; a fundamental primeiro)
    struct Target
    {
        double start, end;
        std::vector<float> pcs;
        std::string name;
        float confidence;
        int bassPc = -1;  // o baixo próprio (o pedal, a inversão)
    };
    std::vector<Target> targets;
    const auto wrap12 = [](float x) { return std::fmod(std::fmod(x, 12.0f) + 12.0f, 12.0f); };
    const auto pcDist = [&](float a, float b) {
        const float d = std::abs(wrap12(a) - wrap12(b));
        return std::min(d, 12.0f - d);
    };
    if (!byDegrees)
    {
        for (const auto& s : spans)
        {
            std::vector<float> pcs;
            if (!s.microPcs.empty())
                pcs = s.microPcs;  // a harmonia microtonal (Partch)
            else
                for (int pc : s.chord.pitchClasses())
                    pcs.push_back(static_cast<float>(pc));
            targets.push_back({ s.start, s.end, pcs, s.microPcs.empty() ? s.chord.name() : s.microName, s.confidence, s.bassPc });
        }
    }
    else
    {
        // a tríade de graus da escala da seção que melhor responde ao acorde
        // lido (graus d, d+2, d+4; em escalas de menos de 5 graus, d, d+1, d+2)
        static const char* const kRoman[] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII" };
        for (const auto& s : spans)
        {
            const SectionScale* sc = &scales.front();
            for (const auto& x : scales)
                if (s.start + 1e-6 >= x.start)
                    sc = &x;
            const auto& m = sc->scale;
            const int n = static_cast<int>(m.cents.size());
            const int stride = n >= 5 ? 2 : 1;
            const auto read = s.chord.pitchClasses();
            double bestScore = -1e9;
            std::vector<float> best;
            int bestDeg = 0;
            for (int d = 0; d < n; ++d)
            {
                std::vector<float> pcs;
                for (int k = 0; k < 3; ++k)
                {
                    const int idx = d + k * stride;
                    const double c = m.cents[static_cast<std::size_t>(idx % n)] + m.period * (idx / n);
                    pcs.push_back(wrap12(static_cast<float>((m.tonic + c) / 100.0)));
                }
                double score = 0.0;
                for (std::size_t j = 0; j < read.size(); ++j)
                {
                    float near = 99.0f;
                    for (float p : pcs)
                        near = std::min(near, pcDist(p, static_cast<float>(read[j])));
                    score += (j == 0 ? 1.0 : 0.8) * std::max(0.0, 1.0 - near / 1.2);
                }
                score += pcDist(pcs[0], static_cast<float>(read[0])) < 0.6f ? 0.3 : 0.0;
                if (score > bestScore)
                {
                    bestScore = score;
                    best = pcs;
                    bestDeg = d;
                }
            }
            targets.push_back({ s.start, s.end, best, std::string(kRoman[bestDeg % 12]), s.confidence });
        }
        how = machine ? "pelos graus das escalas da máquina" : "pelos graus da escala";
    }

    // 2b. a variante da escala sob cada acorde (chord-scale, Russell 1953):
    //     a nota do acorde que falta na escala da seção toma o lugar do grau
    //     vizinho, e as notas do motivo e do eco nesse grau seguem a
    //     alteração enquanto o acorde dura. Em Lá menor natural sob Mi
    //     maior, Sol vira Sol#: a menor harmônica nasce do acorde.
    int varied = 0;
    std::string variantExample;
    if (!scales.empty())
        for (const auto& tg : targets)
        {
            const SectionScale* sc = &scales.front();
            for (const auto& x : scales)
                if (tg.start + 1e-6 >= x.start)
                    sc = &x;
            std::vector<double> chordCents;
            for (float p : tg.pcs)
                chordCents.push_back(100.0 * p);
            std::vector<DegreeChange> changes;
            const auto variant = chordVariant(sc->scale, chordCents, &changes);
            if (changes.empty())
                continue;
            for (auto& n : score.notes)
            {
                if ((n.voice != 0 && n.voice != 2) || n.time < tg.start - 1e-6 || n.time >= tg.end)
                    continue;
                const double here = std::fmod(std::fmod(100.0 * n.pitch, 1200.0) + 1200.0, 1200.0);
                for (const auto& ch : changes)
                {
                    const double orig = std::fmod(sc->scale.tonic + sc->scale.cents[static_cast<std::size_t>(ch.degree)], 1200.0);
                    double d = std::abs(here - orig);
                    d = std::min(d, 1200.0 - d);
                    if (d < 30.0)
                    {
                        n.pitch += static_cast<float>(ch.delta / 100.0);
                        ++varied;
                        break;
                    }
                }
            }
            if (variantExample.empty())
            {
                const auto& ch = changes.front();
                const double from = sc->scale.tonic + sc->scale.cents[static_cast<std::size_t>(ch.degree)];
                const double to = sc->scale.tonic + variant.cents[static_cast<std::size_t>(ch.degree)];
                const auto nm = [](double c) {
                    static const char* const pc[12] = { "Dó", "Dó#", "Ré", "Mib", "Mi", "Fá", "Fá#", "Sol", "Lá♭", "Lá", "Sib", "Si" };
                    const double w = std::fmod(std::fmod(c, 1200.0) + 1200.0, 1200.0);
                    const int k = static_cast<int>(std::lround(w / 100.0)) % 12;
                    const double off = w - 100.0 * std::round(w / 100.0);
                    char b[32];
                    if (std::abs(off) >= 5.0) std::snprintf(b, sizeof b, "%s%+.0fc", pc[k], off);
                    else std::snprintf(b, sizeof b, "%s", pc[k]);
                    return std::string(b);
                };
                variantExample = "sob " + tg.name + ": " + nm(from) + " → " + nm(to);
            }
        }

    const auto targetAt = [&](double t) -> const Target* {
        for (const auto& s : targets)
            if (t >= s.start - 1e-6 && t < s.end)
                return &s;
        return nullptr;
    };
    const auto contains = [&](const Target& tg, float pitch) {
        return std::any_of(tg.pcs.begin(), tg.pcs.end(), [&](float p) { return pcDist(p, pitch) < 0.3f; });
    };

    // 3. o baixo segue a fundamental: as notas são partidas nas trocas de acorde
    std::vector<Note> extra;
    int bassMoved = 0;
    for (auto& n : score.notes)
    {
        if (n.voice != 1)
            continue;
        const double end = n.time + n.duration;
        double t = n.time;
        bool first = true;
        Note original = n;
        while (t < end - 0.05)
        {
            const auto* s = targetAt(t + 1e-4);
            double segEnd = end;
            if (s)
                segEnd = std::min(end, s->end);
            else
                for (const auto& sp : targets)
                    if (sp.start > t + 1e-4)
                    {
                        segEnd = std::min(end, sp.start);
                        break;
                    }
            Note seg = original;
            seg.time = t;
            seg.duration = segEnd - t;
            if (s)
            {
                // a fundamental (ou o baixo próprio: o pedal, a inversão) na
                // oitava mais perto da nota original
                const float root = s->bassPc >= 0 ? static_cast<float>(s->bassPc) : s->pcs.front();
                float p = root + 12.0f * std::round((original.pitch - root) / 12.0f);
                while (p < 31.0f) p += 12.0f;
                while (p > 55.0f) p -= 12.0f;
                bassMoved += std::abs(p - original.pitch) > 0.01f;
                seg.pitch = p;
            }
            if (first)
            {
                n = seg;
                first = false;
            }
            else if (seg.duration > 0.08)
                extra.push_back(seg);
            t = segEnd;
        }
    }
    score.notes.insert(score.notes.end(), extra.begin(), extra.end());

    // 4. o motivo cai numa nota do acorde nos ataques fortes (início do
    //    acorde, nota longa ou acentuada); as outras ficam como passagem
    int snapped = 0;
    for (auto& n : score.notes)
    {
        if (n.voice != 0)
            continue;
        const auto* s = targetAt(n.time + 1e-4);
        if (!s)
            continue;
        const bool strong = n.time - s->start < 0.08 || n.duration >= 0.9 * beat || n.velocity >= 0.7f;
        if (!strong || contains(*s, n.pitch))
            continue;
        float bestMove = 99.0f;
        for (float p : s->pcs)
        {
            float d = wrap12(p - n.pitch);
            if (d > 6.0f) d -= 12.0f;
            if (std::abs(d) < std::abs(bestMove)) bestMove = d;
        }
        if (std::abs(bestMove) <= 2.5f)
        {
            n.pitch += bestMove;
            ++snapped;
        }
    }

    // 4b. a dissonância tratada (2026-10-17; HarmonyWriting.h): no tempo forte,
    //     numa nota longa do motivo que é nota do acorde, às vezes a
    //     APOJATURA — a nota de cima (um tom; meio tom sobre a terça maior, o
    //     retardo 4–3) toma ~40 % do valor, acentuada, e resolve por grau na
    //     nota do acorde, mais suave (Piston 1987, cap. 9: as notas
    //     estranhas ao acorde).
    int appoggiaturas = 0;
    if (!byDegrees && opt.writeHarmony)
    {
        const double pApp = opt.mode == strategy::Mode::transfigure ? 0.3 : opt.mode == strategy::Mode::counter ? 0.15
                          : opt.mode == strategy::Mode::interrupt ? 0.1 : 0.2;
        std::vector<Note> app;
        for (auto& n : score.notes)
        {
            if (n.voice != 0 || n.duration < beat * 0.95)
                continue;
            const auto* s = targetAt(n.time + 1e-4);
            if (!s || n.time - s->start > 0.08 || !contains(*s, n.pitch) || rng.uniform() >= pApp)
                continue;
            const bool majorThird = pcDist(n.pitch, s->pcs.front() + 4.0f) < 0.3f;
            Note a = n;
            a.duration = n.duration * 0.4;
            a.pitch = n.pitch + (majorThird ? 1.0f : 2.0f);
            a.velocity = std::min(1.0f, n.velocity * 1.08f);
            a.bend.clear();
            a.glide = 0.0f;
            n.time += a.duration;
            n.duration -= a.duration;
            n.velocity *= 0.88f;
            app.push_back(a);
            ++appoggiaturas;
        }
        score.notes.insert(score.notes.end(), app.begin(), app.end());
        // a melodia chega com a cadência: a última nota do motivo antes do fim
        // do acorde de chegada vai para a tônica (ou a terça, a mais perto) na
        // autêntica, na plagal e no fim; para uma nota da dominante na
        // suspensiva e na modulação; para o vi na de engano; e se alonga até
        // o fim do acorde (o ponto final da frase)
        int closures = 0;
        for (const auto& c : cadences)
        {
            Note* lastNote = nullptr;
            for (auto& n : score.notes)
                if (n.voice == 0 && n.time >= c.start - 0.5 * beat && n.time < c.end && (!lastNote || n.time > lastNote->time))
                    lastNote = &n;
            if (!lastNote)
                continue;
            std::vector<float> goal;
            const float t = static_cast<float>(c.key.tonic);
            switch (c.kind)
            {
                case CadenceMark::Kind::authentic:
                case CadenceMark::Kind::plagal:
                    goal = { t, t + (c.key.major ? 4.0f : 3.0f) };
                    break;
                case CadenceMark::Kind::half:
                case CadenceMark::Kind::modulation:
                    goal = { t + 7.0f, t + 2.0f, t + 11.0f };  // a dominante: 5ª, 2ª, a sensível
                    break;
                case CadenceMark::Kind::deceptive:
                    goal = { t + 9.0f, t };
                    break;
            }
            float bestMove = 99.0f;
            for (float g : goal)
            {
                float d = wrap12(g - lastNote->pitch);
                if (d > 6.0f) d -= 12.0f;
                if (std::abs(d) < std::abs(bestMove)) bestMove = d;
            }
            lastNote->pitch += bestMove;
            lastNote->bend.clear();
            lastNote->glide = 0.0f;
            lastNote->duration = std::max(lastNote->duration, c.end - lastNote->time - 0.02);
            ++closures;
        }
        if (closures > 0)
            written += (written.empty() ? std::string("harmonia escrita: ") : std::string(", ")) + std::to_string(closures)
                     + " frases fechadas na cadência";
        if (appoggiaturas > 0)
            written += (written.empty() ? std::string("harmonia escrita: ") : std::string(", ")) + std::to_string(appoggiaturas)
                     + " apojaturas no motivo";
    }

    // 5. a voz de harmonia: o acorde sustentado, cada um na inversão que
    //    menos se move desde o anterior (condução parcimoniosa)
    const double pick = rng.uniform();
    const Timbre pad = pick < 0.4 ? allowed(opt.constraints, { Timbre::additive, Timbre::breath, Timbre::flute })
                     : pick < 0.7 ? allowed(opt.constraints, { Timbre::breath, Timbre::additive, Timbre::flute })
                                  : allowed(opt.constraints, { Timbre::spectral, Timbre::additive, Timbre::breath });
    std::vector<float> prev, planingShape;
    std::size_t index = 0;
    int suspensions = 0, parallels = 0, ledChords = 0, prevRootPc = -1;
    bool prevMajor = false;
    for (const auto& s : targets)
    {
        ++index;
        if (opt.mode == strategy::Mode::interrupt && index % 4 == 0)
            continue;  // um em cada quatro cala
        const auto& pcs = s.pcs;
        const std::size_t m = pcs.size();
        const std::vector<float> before = prev;  // o acorde anterior, para o retardo
        std::vector<float> best;
        double bestMove = 1e9;
        if (opt.voicing != 0)
        {
            // outro voicing (Voicing.h): a forma empilhada a partir da
            // fundamental, no registro médio e perto do acorde anterior
            const SectionScale* sc = scales.empty() ? nullptr : &scales.front();
            for (const auto& x : scales)
                if (s.start + 1e-6 >= x.start)
                    sc = &x;
            float root = pcs.front();
            while (root < 48.0f) root += 12.0f;
            if (!prev.empty())
                while (root + 6.0f < prev.front()) root += 12.0f;
            while (root > 57.0f) root -= 12.0f;
            best = voiceChord(static_cast<Voicing>(opt.voicing), root, pcs, sc ? &sc->scale : nullptr, planingShape);
            prev = best;
        }
        else if (opt.writeHarmony && !byDegrees && language == HarmonyLanguage::partch)
        {
            // Partch: o acorde justo em bloco, a partir da unidade, na oitava
            // mais perto do anterior (as frações de semitom ficam)
            float base = 48.0f + pcs.front();
            if (!prev.empty())
                while (base + 6.0f < prev.front()) base += 12.0f;
            while (base > 57.0f) base -= 12.0f;
            best.clear();
            float last = -1.0f;
            for (float pc : pcs)
            {
                float p = base + std::fmod(pc - pcs.front() + 24.0f, 12.0f);
                while (p <= last) p += 12.0f;
                best.push_back(p);
                last = p;
            }
            ++ledChords;
        }
        else if (opt.writeHarmony && !byDegrees)
        {
            // a condução de vozes a quatro partes (HarmonyWriting.h): o baixo
            // e três vozes, sem paralelas, a sensível subindo no V → I
            std::vector<int> ip;
            for (float p : pcs)
                ip.push_back(static_cast<int>(std::lround(p)) % 12);
            const int rootPc = ip.empty() ? 0 : ip.front();
            // as vozes de cima: na tríade, as três notas (a fundamental
            // dobrada no baixo); com cor, sem a fundamental (o baixo a toca),
            // no máximo quatro — a 3ª, a 7ª e as cores primeiro, a 5ª por último
            std::vector<int> upper = ip;
            if (ip.size() > 3)
            {
                upper.clear();
                upper.push_back(ip[1]);
                for (std::size_t q = 3; q < ip.size(); ++q)
                    upper.push_back(ip[q]);
                upper.push_back(ip[2]);
                if (upper.size() > 4)
                    upper.resize(4);
            }
            int leading = -1;
            if (prevRootPc >= 0 && ((prevRootPc - rootPc + 12) % 12) == 7 && prevMajor)
                leading = (prevRootPc + 4) % 12;  // a terça do V: a sensível do I
            const auto before4 = prev;
            const bool block = language == HarmonyLanguage::impressionist || language == HarmonyLanguage::scriabin
                            || language == HarmonyLanguage::polytonal || language == HarmonyLanguage::octatonic
                            || language == HarmonyLanguage::spectral || language == HarmonyLanguage::gismonti;
            const bool fourths = language == HarmonyLanguage::quartal || language == HarmonyLanguage::villaLobos;
            if (fourths || language == HarmonyLanguage::clusters)
            {
                // a quartal (e as cordas soltas de Villa-Lobos): as notas do acorde
                // na ordem do ciclo de quartas a partir da fundamental, empilhadas;
                // os clusters (Cowell): o acorde fechado, com as lacunas maiores que
                // um tom preenchidas por segundas
                float base = 48.0f + static_cast<float>(rootPc);
                if (prevRootPc >= 0 && !prev.empty())
                    while (base + 6.0f < prev.front()) base += 12.0f;
                while (base > 55.0f) base -= 12.0f;
                std::vector<int> rel;
                for (int pc : ip)
                    rel.push_back(((pc - rootPc) % 12 + 12) % 12);
                if (fourths)
                {
                    // a posição no ciclo de quartas: 0, 5, 10, 3, 8, 1, 6, 11, 4, 9, 2, 7
                    const auto chainIndex = [](int r) { return (r * 5) % 12; };  // 5 é o próprio inverso mod 12
                    std::sort(rel.begin(), rel.end(), [&](int x, int y) { return chainIndex(x) < chainIndex(y); });
                }
                else
                    std::sort(rel.begin(), rel.end());
                rel.erase(std::unique(rel.begin(), rel.end()), rel.end());
                best.clear();
                float last = -1.0f;
                for (int r : rel)
                {
                    float q = base + static_cast<float>(r);
                    while (q <= last) q += 12.0f;
                    if (!fourths && last >= 0.0f)
                        while (q - last > 2.0f + 1e-3f && best.size() < 7)
                        {
                            last += 2.0f;  // a segunda que fecha a lacuna
                            best.push_back(last);
                            if (q - last <= 2.0f + 1e-3f)
                                break;
                        }
                    best.push_back(q);
                    last = q;
                }
            }
            else if (block)
            {
                // o planing: o acorde empilhado a partir da fundamental, em bloco,
                // na oitava mais perto da anterior (as vozes andam paralelas)
                float base = 48.0f + static_cast<float>(rootPc);
                if (prevRootPc >= 0 && !prev.empty())
                    while (base + 6.0f < prev.front()) base += 12.0f;
                while (base > 57.0f) base -= 12.0f;
                best.clear();
                float last = -1.0f;
                for (int pc : ip)
                {
                    float p = base + static_cast<float>(((pc - rootPc) % 12 + 12) % 12);
                    while (p <= last) p += 12.0f;
                    best.push_back(p);
                    last = p;
                }
            }
            else
            {
                best = voiceLead(prev, upper, prevRootPc < 0 ? rootPc : prevRootPc, rootPc, leading);
                if (!before4.empty() && before4.size() == best.size())
                    parallels += countParallels(before4, best, prevRootPc < 0 ? rootPc : prevRootPc, rootPc);
            }
            prevRootPc = rootPc;
            prevMajor = ip.size() >= 2 && ((ip[1] - ip[0] + 12) % 12) == 4;
            ++ledChords;
        }
        else
        for (std::size_t inv = 0; inv < m; ++inv)
            for (float base : { 48.0f, 60.0f })
            {
                std::vector<float> v;
                float last = -1.0f;
                for (std::size_t k = 0; k < m; ++k)
                {
                    float p = base + pcs[(inv + k) % m];
                    while (p <= last) p += 12.0f;
                    v.push_back(p);
                    last = p;
                }
                double move = 0.0;
                if (prev.size() != v.size())
                    move = std::abs(v[v.size() / 2] - 59.0f);  // começa em volta do Si central
                else
                    for (std::size_t k = 0; k < v.size(); ++k)
                        move += std::abs(v[k] - prev[k]);
                if (v.front() < 50.0f || v.back() > 74.0f)
                    move += 50.0;  // fora do registro médio
                if (move < bestMove)
                {
                    bestMove = move;
                    best = v;
                }
            }
        if (opt.voicing == 0)
            prev = best;
        const double len = (s.end - s.start) * (opt.mode == strategy::Mode::interrupt ? 0.4 : 0.98);
        for (std::size_t k = 0; k < best.size(); ++k)
        {
            const float p = best[k];
            Note n;
            n.time = s.start;
            n.duration = len;
            n.pitch = p;
            n.velocity = 0.22f + 0.1f * s.confidence;
            n.voice = 6;
            n.timbre = pad;
            // o retardo preparado (2026-10-17): a nota do acorde anterior que
            // está um grau acima desta, e não pertence ao acorde novo, fica
            // presa no começo dele (a dissonância) e desce para resolver — o
            // 4–3, o 9–8 da escrita coral (Piston 1987, cap. 9)
            if (opt.writeHarmony && !byDegrees && before.size() == best.size() && len >= 2.0 * beat)
            {
                const float held = before[k];
                const float step = held - p;
                if ((std::abs(step - 1.0f) < 0.01f || std::abs(step - 2.0f) < 0.01f) && !contains(s, held) && rng.uniform() < 0.35)
                {
                    Note sus = n;
                    sus.pitch = held;
                    sus.duration = len * 0.45;
                    sus.velocity = n.velocity * 1.1f;
                    if (sus.time + sus.duration < score.duration - 0.1)
                        score.notes.push_back(sus);
                    n.time += sus.duration;
                    n.duration -= sus.duration;
                    ++suspensions;
                }
            }
            if (n.time + n.duration < score.duration - 0.1)
                score.notes.push_back(n);
        }
    }

    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) {
        return a.time < b.time || (a.time == b.time && a.voice < b.voice);
    });
    std::string seq;
    for (std::size_t i = 0; i < targets.size() && i < 12; ++i)
        seq += (i ? " " : "") + targets[i].name;
    if (targets.size() > 12)
        seq += " …";
    std::string heardSeq;
    for (std::size_t i = 0; i < heard.size() && i < 6; ++i)
        heardSeq += (i ? " " : "") + heard[i].chord.name();
    char text[800];
    std::snprintf(text, sizeof text,
                  "harmonia em movimento: %zu acordes lidos (%s…) · %s: %s · baixo na fundamental (%d notas), "
                  "%d notas do motivo levadas ao acorde, voz de harmonia em %s (voicing %s) · variantes da escala por acorde: "
                  "%d notas%s%s",
                  heard.size(), heardSeq.c_str(), how.c_str(), seq.c_str(), bassMoved, snapped, timbreName(pad),
                  voicingName(static_cast<Voicing>(opt.voicing)), varied,
                  variantExample.empty() ? "" : " (", variantExample.empty() ? "" : (variantExample + ")").c_str());
    score.marks.insert(score.marks.begin() + (score.marks.empty() ? 0 : 1), { 0.0, text });
    if (suspensions > 0)
        written += (written.empty() ? std::string("harmonia escrita: ") : std::string(", ")) + std::to_string(suspensions)
                 + " retardos preparados na voz de harmonia";
    if (ledChords > 0)
        written += (written.empty() ? std::string("harmonia escrita: ") : std::string(", ")) + std::to_string(ledChords)
                 + " acordes conduzidos a quatro partes (" + std::to_string(parallels) + " paralelas restantes)";
    if (!written.empty())
        score.marks.insert(score.marks.begin() + 1, { 0.0, written });
}

} // namespace trioio::composition
