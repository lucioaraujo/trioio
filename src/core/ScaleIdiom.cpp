#include "ScaleIdiom.h"

#include "Voicing.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

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

// a altura (MIDI, com fração) do conjunto mais perto de `pitch`, em
// qualquer período: os valores do conjunto são cents a partir da tônica
float snapTo(float pitch, const std::vector<double>& set, double tonic, double period)
{
    if (set.empty())
        return pitch;
    const double rel = 100.0 * pitch - tonic;
    double best = rel, bestD = 1e18;
    for (double c : set)
    {
        const double m0 = std::round((rel - c) / period);
        for (double m = m0 - 1; m <= m0 + 1; m += 1.0)
        {
            const double cand = c + m * period;
            if (std::abs(cand - rel) < bestD) { bestD = std::abs(cand - rel); best = cand; }
        }
    }
    return static_cast<float>((tonic + best) / 100.0);
}

bool near(float pitch, double cents, double tonic, double period, double tol)
{
    double r = std::fmod(100.0 * pitch - tonic - cents, period);
    if (r < 0) r += period;
    return std::min(r, period - r) <= tol;
}
} // namespace

std::string applyScaleIdiom(Score& score, const std::vector<SectionScale>& sections,
                            const std::vector<const ScaleBehavior*>& perSection, std::uint64_t seed)
{
    if (sections.empty() || std::none_of(perSection.begin(), perSection.end(), [](const ScaleBehavior* x) { return x && x->any(); }))
        return {};
    Rng rng { seed * 0x2127599BF4325C37ull + 0x1D10 };
    std::vector<Note*> mel;
    for (auto& n : score.notes)
        if (n.voice == 0)
            mel.push_back(&n);
    std::sort(mel.begin(), mel.end(), [](const Note* a, const Note* c) { return a->time < c->time; });
    if (mel.empty())
        return {};

    // o registro de base: a tônica na oitava do motivo
    double meanAll = 0.0;
    for (auto* n : mel)
        meanAll += n->pitch;
    meanAll /= static_cast<double>(mel.size());

    int directional = 0, supported = 0, signatures = 0, cadences = 0, ornaments = 0, rebodied = 0;
    std::vector<float> durations;
    for (auto* n : mel)
        durations.push_back(static_cast<float>(n->duration));
    std::nth_element(durations.begin(), durations.begin() + static_cast<std::ptrdiff_t>(durations.size() / 2), durations.end());
    const float median = durations[durations.size() / 2];

    const std::size_t N = sections.size();
    std::string sources;
    for (std::size_t k = 0; k < N; ++k)
    {
        if (k >= perSection.size() || !perSection[k] || !perSection[k]->any())
            continue;
        const ScaleBehavior& b = *perSection[k];
        if (sources.find(b.source) == std::string::npos)
            sources += (sources.empty() ? "" : " | ") + b.source;
        const auto& sec = sections[k];
        const double T = sec.scale.tonic, P = sec.scale.period;
        std::vector<Note*> notes;
        for (auto* n : mel)
            if (n->time >= sec.start - 1e-6 && n->time < sec.end)
                notes.push_back(n);
        if (notes.empty())
            continue;
        // a tônica de base na oitava média do motivo
        const double baseCents = T + P * std::round((100.0 * meanAll - T) / P);

        // 1. percurso: o registro da seção vai à região k do caminho
        if (!b.path.empty())
        {
            const std::size_t idx = N > 1 ? static_cast<std::size_t>(std::lround(static_cast<double>(k) * (b.path.size() - 1) / (N - 1)))
                                          : 0;
            const double region = (baseCents + b.path[std::min(idx, b.path.size() - 1)]) / 100.0;
            double mean = 0.0;
            for (auto* n : notes)
                mean += n->pitch;
            mean /= static_cast<double>(notes.size());
            const float shift = static_cast<float>(std::clamp(region - mean, -9.0, 9.0));
            for (auto* n : notes)
                n->pitch = snapTo(n->pitch + shift, sec.scale.cents, T, P);
        }
        // 2. apoio: os ataques fortes às notas de apoio
        std::vector<bool> fixed(notes.size(), false);  // assinatura e cadência: não mexer depois
        if (!b.support.empty())
            for (std::size_t i = 0; i < notes.size(); ++i)
            {
                auto* n = notes[i];
                const bool afterRest = i == 0 || n->time - (notes[i - 1]->time + notes[i - 1]->duration) > 0.25;
                const bool strong = afterRest || n->duration >= 1.5f * median;
                if (!strong || rng.uniform() > 0.7)
                    continue;
                const float p = snapTo(n->pitch, b.support, T, P);
                if (std::abs(p - n->pitch) <= 3.0f)
                {
                    supported += 1;
                    n->pitch = p;
                }
            }
        // 3. assinatura: abre a seção e o meio dela, com o ritmo do motivo
        if (!b.signatures.empty())
            for (std::size_t at : { std::size_t { 0 }, notes.size() / 2 })
            {
                const auto& sig = b.signatures[(k + (at ? 1 : 0)) % b.signatures.size()];
                if (at + sig.size() > notes.size() || (at && notes.size() < 2 * sig.size() + 2))
                    continue;
                for (std::size_t j = 0; j < sig.size(); ++j)
                {
                    notes[at + j]->pitch = static_cast<float>((baseCents + sig[j]) / 100.0);
                    fixed[at + j] = true;
                }
                ++signatures;
            }
        // 4. cadência: as últimas notas da seção
        if (!b.cadence.empty() && notes.size() >= b.cadence.size() + 2)
        {
            const std::size_t first = notes.size() - b.cadence.size();
            for (std::size_t j = 0; j < b.cadence.size(); ++j)
            {
                notes[first + j]->pitch = static_cast<float>((baseCents + b.cadence[j]) / 100.0);
                fixed[first + j] = true;
            }
            ++cadences;
        }
        // 5. subida e descida, por último: a forma da escala na direção do
        //    movimento (a Yaman sobe sem Sa e Pa; o Rast desce com Sib),
        //    sem mexer nas frases da tradição
        float prev = notes.front()->pitch;
        for (std::size_t i = 0; i < notes.size(); ++i)
        {
            auto* n = notes[i];
            const float dir = i ? n->pitch - prev : 0.0f;
            const auto& set = dir > 0.05f ? b.up : dir < -0.05f ? b.down : std::vector<double> {};
            if (!fixed[i] && !set.empty())
            {
                float p = snapTo(n->pitch, set, T, P);
                // sem inverter a direção: se o vizinho da forma cair do outro
                // lado da nota anterior, tenta o próximo na direção certa
                if ((dir > 0.0f && p <= prev) || (dir < 0.0f && p >= prev))
                {
                    float best = p, bestD = 1e9f;
                    for (double c : set)
                        for (int m = -2; m <= 2; ++m)
                        {
                            const float q = static_cast<float>((T + c + m * P) / 100.0);
                            if ((dir > 0.0f ? q > prev : q < prev) && std::abs(q - n->pitch) < bestD)
                            {
                                bestD = std::abs(q - n->pitch);
                                best = q;
                            }
                        }
                    p = best;
                }
                directional += std::abs(p - n->pitch) > 0.01f;
                n->pitch = p;
            }
            prev = n->pitch;
        }
        // 6. ornamento, com a curva de altura da nota (Note::pitchAt)
        for (std::size_t i = 0; i < notes.size(); ++i)
        {
            auto* n = notes[i];
            bool done = false;
            if (b.ornament == "meend" && i + 1 < notes.size() && n->duration >= 0.35
                && notes[i + 1]->time - (n->time + n->duration) < 0.1)
            {
                n->glide = 0.7f * (notes[i + 1]->pitch - n->pitch);  // desliza até a seguinte
                done = true;
            }
            else if (b.ornament == "andolan" && n->duration >= 0.3 && b.support.size() >= 2
                     && (near(n->pitch, b.support[0], T, P, 15.0) || near(n->pitch, b.support[1], T, P, 15.0)))
            {
                // oscilação lenta e larga em volta da nota (re e dha komal da Bhairav)
                n->wobbleDepth = 0.25f;
                n->wobbleRate = 1.1f;
                n->wobbleDelay = 0.08f;
                done = true;
            }
            else if (b.ornament == "gamaka" && n->duration >= 0.3 && !near(n->pitch, 0.0, T, P, 15.0)
                     && !near(n->pitch, 702.0, T, P, 15.0))
            {
                // kampita: oscilação rápida; Sa e Pa ficam firmes
                n->wobbleDepth = 0.55f;
                n->wobbleRate = 6.0f;
                n->wobbleDelay = 0.05f;
                done = true;
            }
            else if (b.ornament == "blue"
                     && (near(n->pitch, 300.0, T, P, 30.0) || near(n->pitch, 600.0, T, P, 30.0) || near(n->pitch, 1000.0, T, P, 30.0)))
            {
                n->bend = { { 0.0f, -0.5f }, { 0.3f, 0.0f } };  // chega de baixo, arrastada
                done = true;
            }
            if (done)
            {
                ++ornaments;
                // famílias de corpo fixo não deslizam: o ornamento pede um corpo contínuo
                if (n->timbre == Timbre::plucked || n->timbre == Timbre::membrane || n->timbre == Timbre::metal)
                {
                    n->timbre = Timbre::breath;
                    ++rebodied;
                }
            }
        }
    }
    std::string orn, hasPath;
    for (const auto* x : perSection)
        if (x && !x->ornament.empty() && orn.find(x->ornament) == std::string::npos)
            orn += (orn.empty() ? "" : ", ") + x->ornament;
    for (const auto* x : perSection)
        if (x && !x->path.empty())
            hasPath = " · percurso pelas regiões";
    char text[900];
    std::snprintf(text, sizeof text,
                  "comportamento da escala: %d notas na forma de subida/descida, %d ataques nas notas de apoio, "
                  "%d frases-assinatura, %d cadências, %d ornamentos (%s%s)%s · fonte: %s",
                  directional, supported, signatures, cadences, ornaments, orn.empty() ? "nenhum" : orn.c_str(),
                  rebodied ? (", " + std::to_string(rebodied) + " em sopro para poder deslizar").c_str() : "",
                  hasPath.c_str(), sources.c_str());
    return text;
}

std::string heterophonyLayer(Score& score, const std::vector<SectionScale>& sections, std::uint64_t seed, bool keepOthers)
{
    if (sections.empty())
        return {};
    Rng rng { seed * 0x8CB92BA72F3D8DD7ull + 0x4E7E };
    // a segunda voz da heterofonia substitui o eco e o baixo (no takht e no
    // gamelão não há baixo funcional nem acordes)
    const auto inside = [&](double t) {
        return std::any_of(sections.begin(), sections.end(), [&](const SectionScale& x) { return t >= x.start - 1e-6 && t < x.end; });
    };
    std::vector<Note> mel;
    std::vector<Note> keep;
    int removed = 0;
    for (const auto& n : score.notes)
    {
        const bool in = inside(n.time);
        if (n.voice == 0 && in)
            mel.push_back(n);
        if (in && !keepOthers && (n.voice == 1 || n.voice == 2 || n.voice == 6))
            ++removed;
        else
            keep.push_back(n);
    }
    score.notes.swap(keep);
    const auto scaleAt = [&](double t) -> const MachineScale& {
        const SectionScale* sc = &sections.front();
        for (const auto& x : sections)
            if (t + 1e-6 >= x.start)
                sc = &x;
        return sc->scale;
    };
    // o corpo da segunda voz contrasta com o da primeira: corda pinçada (ud,
    // qanun) se a melodia é soprada; sopro (ney) se a melodia é outra coisa
    int doubled = 0, graces = 0, octaves = 0, omitted = 0;
    for (const auto& m : mel)
    {
        if (rng.uniform() < 0.15)
        {
            ++omitted;  // simplifica: deixa de tocar algumas notas
            continue;
        }
        Note h = m;
        h.voice = 2;
        h.time = m.time + 0.02 + 0.06 * rng.uniform();  // um pouco depois, nunca junto
        h.velocity = 0.6f * m.velocity;
        h.timbre = m.timbre == Timbre::breath || m.timbre == Timbre::flute ? Timbre::plucked : Timbre::breath;
        h.bend.clear();
        h.wobbleDepth = 0.0f;
        h.glide = 0.0f;
        if (rng.uniform() < 0.12)
        {
            h.pitch += 12.0f;  // às vezes uma oitava acima
            ++octaves;
        }
        // elaboração: uma apojatura curta pelo grau vizinho de cima
        if (m.duration > 0.3 && rng.uniform() < 0.3)
        {
            const auto& sc = scaleAt(m.time);
            const double rel = 100.0 * m.pitch - sc.tonic;
            double up = 1e9;
            for (int k = -1; k <= 1; ++k)
                for (double c : sc.cents)
                {
                    const double cand = c + k * sc.period + sc.period * std::floor(rel / sc.period);
                    if (cand > rel + 30.0 && cand - rel < 400.0)
                        up = std::min(up, cand - rel);
                }
            if (up < 400.0)
            {
                Note g = h;
                g.duration = 0.08;
                g.pitch = h.pitch + static_cast<float>(up / 100.0);
                h.time += 0.08;
                h.duration = std::max(0.05, h.duration - 0.08);
                score.notes.push_back(g);
                ++graces;
            }
        }
        if (h.time + h.duration < score.duration - 0.05)
        {
            score.notes.push_back(h);
            ++doubled;
        }
    }
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) {
        return a.time < b.time || (a.time == b.time && a.voice < b.voice);
    });
    char text[260];
    std::snprintf(text, sizeof text,
                  "textura: heterofonia — a segunda voz toca a mesma melodia um pouco depois (%d notas), com %d apojaturas, "
                  "%d oitavas e %d notas omitidas; sem acordes nem baixo (%d notas de eco, baixo e acordes retiradas)",
                  doubled, graces, octaves, omitted, removed);
    return text;
}

std::string vampLayer(Score& score, const std::vector<SectionScale>& sections,
                      const std::vector<const ScaleBehavior*>& behaviors, double barLength, const Options& opt)
{
    const double bar = barLength >= 1.0 && barLength <= 6.0 ? barLength : 2.0;
    const Timbre pad = opt.constraints.noTimbre[static_cast<std::size_t>(Timbre::additive)] ? Timbre::breath : Timbre::additive;
    int chords = 0, pedal = 0;
    std::string names;
    for (std::size_t k = 0; k < sections.size() && k < behaviors.size(); ++k)
    {
        const auto* b = behaviors[k];
        if (!b || b->vamp.size() < 2)
            continue;
        const auto& sec = sections[k];
        const auto& sc = sec.scale;
        const int n = static_cast<int>(sc.cents.size());
        if (n < 5)
            continue;
        // tira os acordes da harmonia em movimento neste trecho
        score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                         [&](const Note& x) { return x.voice == 6 && x.time >= sec.start - 1e-6 && x.time < sec.end; }),
                          score.notes.end());
        // a tríade de um grau, em semitons absolutos, no registro médio
        const auto triad = [&](int degree) {
            std::vector<float> v;
            for (int j = 0; j < 3; ++j)
            {
                const int idx = degree + 2 * j;
                const double c = sc.tonic + sc.cents[static_cast<std::size_t>(idx % n)] + sc.period * (idx / n);
                float p = static_cast<float>(c / 100.0);
                while (p < 52.0f) p += 12.0f;
                while (p > 70.0f) p -= 12.0f;
                v.push_back(p);
            }
            std::sort(v.begin(), v.end());
            return v;
        };
        // o voicing: o pedido para a peça, ou o usual da escala, ou terças
        Voicing style = static_cast<Voicing>(opt.voicing);
        Voicing own;
        if (opt.voicing == 0 && !b->voicing.empty() && parseVoicing(b->voicing, own))
            style = own;
        std::vector<float> shape;
        int i = 0;
        for (double t = sec.start; t < sec.end - 0.2; t += bar, ++i)
        {
            auto ch = triad(b->vamp[static_cast<std::size_t>(i) % b->vamp.size()]);
            if (style != Voicing::thirds)
            {
                // a fundamental é a nota do grau (a 1.ª da tríade antes de ordenar)
                const int d = b->vamp[static_cast<std::size_t>(i) % b->vamp.size()];
                float root = static_cast<float>((sc.tonic + sc.cents[static_cast<std::size_t>(d % n)] + sc.period * (d / n)) / 100.0);
                while (root < 48.0f) root += 12.0f;
                while (root > 57.0f) root -= 12.0f;
                std::vector<float> pcs;
                for (float p : ch)
                    pcs.push_back(p);
                std::sort(pcs.begin(), pcs.end(), [&](float a, float c) {
                    return std::fmod(a - root + 1200.0f, 12.0f) < std::fmod(c - root + 1200.0f, 12.0f);
                });
                ch = voiceChord(style, root, pcs, &sc, shape);
            }
            for (float p : ch)
            {
                Note x;
                x.time = t;
                x.duration = std::min(bar * 0.95, sec.end - t);
                x.pitch = p;
                x.velocity = 0.3f;
                x.voice = 6;
                x.timbre = pad;
                if (x.time + x.duration < score.duration - 0.05)
                    score.notes.push_back(x);
            }
            ++chords;
        }
        // o baixo vira pedal na tônica
        for (auto& x : score.notes)
            if (x.voice == 1 && x.time >= sec.start - 1e-6 && x.time < sec.end)
            {
                x.pitch = snapTo(x.pitch, { 0.0 }, sc.tonic, sc.period);
                ++pedal;
            }
        static const char* const kRoman[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
        std::string v = style != Voicing::thirds ? std::string(voicingName(style)) + " " : std::string();
        for (int d : b->vamp)
            v += (v.empty() ? "" : "–") + std::string(kRoman[d % 7]);
        if (names.find(v) == std::string::npos)
            names += (names.empty() ? "" : ", ") + v;
    }
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& c) {
        return a.time < c.time || (a.time == c.time && a.voice < c.voice);
    });
    if (chords == 0)
        return {};
    char text[240];
    std::snprintf(text, sizeof text, "textura: vamp modal %s (%d acordes alternando por compasso), baixo em pedal na tônica (%d notas)",
                  names.c_str(), chords, pedal);
    return text;
}

std::string droneLayer(Score& score, const std::vector<SectionScale>& sections, const Options& opt)
{
    if (sections.empty())
        return {};
    const Timbre pad = opt.constraints.noTimbre[static_cast<std::size_t>(Timbre::breath)] ? Timbre::additive : Timbre::breath;
    int pedal = 0;
    // os acordes que houver nestes trechos dão lugar ao bordão
    score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                     [&](const Note& n) {
                                         return n.voice == 6 && std::any_of(sections.begin(), sections.end(), [&](const SectionScale& x) {
                                                    return n.time >= x.start - 1e-6 && n.time < x.end;
                                                });
                                     }),
                      score.notes.end());
    for (const auto& sec : sections)
    {
        const double T = sec.scale.tonic, P = sec.scale.period;
        const bool fifth = std::any_of(sec.scale.cents.begin(), sec.scale.cents.end(), [](double c) { return std::abs(c - 702.0) < 25.0; });
        const float tonicLow = static_cast<float>((T + P * std::round((4800.0 - T) / P)) / 100.0);  // perto do Dó 3
        std::vector<float> tones { tonicLow };
        if (fifth)
            tones.push_back(tonicLow + 7.02f);
        for (float p : tones)
        {
            Note n;
            n.time = sec.start;
            n.duration = std::max(0.2, sec.end - sec.start - 0.05);
            n.pitch = p;
            n.velocity = 0.2f;
            n.voice = 6;
            n.timbre = pad;
            if (n.time + n.duration < score.duration)
                score.notes.push_back(n);
        }
        // o baixo vira pedal na tônica
        for (auto& n : score.notes)
            if (n.voice == 1 && n.time >= sec.start - 1e-6 && n.time < sec.end)
            {
                n.pitch = snapTo(n.pitch, { 0.0 }, T, P);
                ++pedal;
            }
    }
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) {
        return a.time < b.time || (a.time == b.time && a.voice < b.voice);
    });
    char text[200];
    std::snprintf(text, sizeof text, "textura: bordão de tônica e quinta (sem acordes), baixo em pedal (%d notas)", pedal);
    return text;
}

} // namespace trioio::composition
