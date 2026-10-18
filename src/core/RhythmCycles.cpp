#include "RhythmCycles.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace trioio::composition
{
namespace
{
std::string lower(std::string s)
{
    for (auto& c : s)
        if (static_cast<unsigned char>(c) < 128)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string trim(const std::string& s)
{
    const auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos)
        return {};
    return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

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

std::vector<RhythmCycle> parseRhythmArchive(const std::string& text, std::vector<std::string>& errors)
{
    std::vector<RhythmCycle> out;
    std::istringstream in(text);
    std::string line;
    RhythmCycle cur;
    bool open = false, bad = false;
    const auto flush = [&] {
        if (open)
        {
            if (!bad && !cur.strokes.empty())
                out.push_back(cur);
            else
                errors.push_back("ciclo ilegível: " + cur.name);
        }
        cur = RhythmCycle {};
        open = bad = false;
    };
    while (std::getline(in, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line[0] == '=')
        {
            flush();
            open = true;
            cur.name = trim(line.substr(1));
            continue;
        }
        const auto colon = line.find(':');
        if (!open || colon == std::string::npos)
            continue;
        const auto key = lower(trim(line.substr(0, colon)));
        const auto value = trim(line.substr(colon + 1));
        if (key == "família" || key == "familia") cur.family = value;
        else if (key == "fonte") cur.source = value;
        else if (key == "nota") cur.note = value;
        else if (key == "unidade")
        {
            if (value == "tempo") cur.unitBeats = 1.0;
            else if (value == "colcheia") cur.unitBeats = 0.5;
            else if (value == "colcheia-ternária" || value == "colcheia-ternaria") cur.unitBeats = 1.0 / 3.0;
            else if (value == "semicolcheia") cur.unitBeats = 0.25;
            else bad = true;
        }
        else if (key == "golpes")
        {
            std::istringstream ts(value);
            std::string t;
            while (ts >> t)
            {
                if (t.size() != 1 || std::string("DTkBxGNPt-").find(t[0]) == std::string::npos)
                    bad = true;
                else
                    cur.strokes += t[0];
            }
        }
        else if (key == "acentos" || key == "vazios")
        {
            std::istringstream ts(value);
            int v = 0;
            while (ts >> v)
                (key == "acentos" ? cur.accents : cur.empties).push_back(v);
        }
    }
    flush();
    return out;
}

namespace
{
double pearson(const std::vector<double>& a, const std::vector<double>& b)
{
    const auto n = static_cast<double>(a.size());
    if (a.size() < 3)
        return 0.0;
    double ma = 0.0, mb = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) { ma += a[i]; mb += b[i]; }
    ma /= n; mb /= n;
    double sab = 0.0, saa = 0.0, sbb = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        sab += (a[i] - ma) * (b[i] - mb);
        saa += (a[i] - ma) * (a[i] - ma);
        sbb += (b[i] - mb) * (b[i] - mb);
    }
    return saa > 1e-12 && sbb > 1e-12 ? sab / std::sqrt(saa * sbb) : 0.0;
}
} // namespace

std::vector<RankedCycle> rankCycles(const std::vector<RhythmCycle>& archive, const std::vector<perception::Frame>& frames,
                                    const std::vector<perception::Onset>& onsets, const perception::Pulse& pulse, double t0,
                                    double t1, int count)
{
    std::vector<RankedCycle> out;
    if (pulse.period <= 0.0 || frames.size() < 2)
        return out;
    // os ataques POR BANDA (Descriptors.h, bandOnsets): golpes próximos de
    // bandas diferentes não se escondem um atrás do outro, e o grave (o dum)
    // vem da banda grave, não do centroide. Os ataques do fluxo inteiro
    // (`onsets`) ficam só como reserva, se a análise não tiver as bandas.
    struct Hit { double t, s; bool grave; };
    std::vector<Hit> hits;
    for (const auto& o : perception::bandOnsets(frames))
        if (o.time >= t0 && o.time < t1)
            hits.push_back({ o.time, o.strength, o.grave() });
    if (hits.size() < 8)
    {
        hits.clear();
        const double hop = frames[1].time - frames[0].time;
        std::vector<float> cents;
        const auto centroidAt = [&](double t) {
            const auto i = static_cast<long>(std::lround((t + 0.02 - frames.front().time) / hop));
            return frames[static_cast<std::size_t>(std::clamp(i, 0L, static_cast<long>(frames.size()) - 1))].centroidHz;
        };
        for (const auto& o : onsets)
            if (o.time >= t0 && o.time < t1)
                cents.push_back(centroidAt(o.time));
        if (cents.size() < 8)
            return out;
        auto sorted = cents;
        std::nth_element(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(sorted.size() / 2), sorted.end());
        const float median = sorted[sorted.size() / 2];
        for (const auto& o : onsets)
            if (o.time >= t0 && o.time < t1)
                hits.push_back({ o.time, o.strength, centroidAt(o.time) < median });
    }
    // a linha do tempo dos ataques numa grade fina comum (1/12 do tempo):
    // assim um ciclo curto não leva vantagem por dobrar tudo em poucas casas.
    // (O envelope do fluxo no lugar dos ataques foi medido e piorou: 1 de 8
    // em 1.º, contra 4 de 8; o fundo de fluxo dilui a correlação.)
    const double step = pulse.period / 12.0;
    const auto cells = static_cast<std::size_t>(std::ceil((t1 - t0) / step));
    std::vector<double> real(cells, 0.0), realGrave(cells, 0.0);
    for (const auto& h : hits)
    {
        const auto j = static_cast<std::size_t>(std::lround((h.t - t0) / step));
        if (j < cells)
        {
            real[j] += h.s;
            realGrave[j] += h.grave ? h.s : 0.0;
        }
    }
    // tolerância de ±1 célula: espalha os ataques reais nos vizinhos
    std::vector<double> realWide(cells, 0.0);
    for (std::size_t j = 0; j < cells; ++j)
        for (long d = -1; d <= 1; ++d)
        {
            const long q = static_cast<long>(j) + d;
            if (q >= 0 && q < static_cast<long>(cells))
                realWide[j] = std::max(realWide[j], real[static_cast<std::size_t>(q)] * (d == 0 ? 1.0 : 0.7));
        }

    for (const auto& c : archive)
    {
        const auto n = c.strokes.size();
        if (n < 3)
            continue;
        RankedCycle best { &c, -1e9, 0, 1.0 };
        best.graveUsed = c.strokes.find_first_of("DBGNTkt") != std::string::npos;
        // os níveis do pulso: os erros que a bancada de andamentos mostrou
        // a unidade pelo nível do pulso (os erros que a bancada mostrou) OU o
        // ciclo inteiro em m pulsos (aksak: 7/8 em 2+2+3 não é múltiplo da
        // colcheia, mas o ciclo é m pulsos lidos)
        std::vector<std::pair<double, double>> units;  // (unidade, nível para registro)
        for (double level : { 0.5, 2.0 / 3.0, 0.75, 1.0, 4.0 / 3.0, 1.5, 2.0 })
            units.push_back({ pulse.period / level * c.unitBeats, level });
        for (int m : { 2, 3, 4, 6, 8 })
            units.push_back({ m * pulse.period / static_cast<double>(n), -static_cast<double>(m) });
        for (const auto& [baseUnit, level] : units)
        for (int fine = -6; fine <= 6; ++fine)  // o período afinado em ±3 %: 1 % de erro deriva 0,3 s em 30 s
        {
            const double unit = baseUnit * (1.0 + 0.005 * fine);
            const double len = unit * static_cast<double>(n);
            if (len > 0.5 * (t1 - t0) || unit < 0.06)
                continue;  // menos de duas voltas no trecho, ou unidade curta demais
            for (std::size_t q = 0; q < n; ++q)
            {
                // a previsão do ciclo na grade fina, volta após volta
                std::vector<double> want(cells, 0.0);
                std::vector<double> gs, gw;
                const double origin = pulse.phase - static_cast<double>(q) * unit;
                const double first = std::ceil((t0 - origin) / unit);
                for (double k = first; origin + k * unit < t1; k += 1.0)
                {
                    const double t = origin + k * unit;
                    const auto j = static_cast<std::size_t>(std::lround((t - t0) / step));
                    if (j >= cells)
                        continue;
                    const auto p = static_cast<std::size_t>(((static_cast<long long>(k) % static_cast<long long>(n))
                                                             + static_cast<long long>(n)) % static_cast<long long>(n));
                    const char g = c.strokes[p];
                    want[j] = g == '-' ? 0.0 : (g == 'T' ? 0.8 : (g == 'k' || g == 't' || g == 'P') ? 0.5 : 1.0);
                    const bool known = g == 'D' || g == 'B' || g == 'G' || g == 'N' || g == 'T' || g == 'k' || g == 't';
                    // grave/agudo: no ataque real mais próximo (±1 célula)
                    double sr = 0.0, sg = 0.0;
                    for (long d = -1; d <= 1; ++d)
                    {
                        const long qj = static_cast<long>(j) + d;
                        if (qj >= 0 && qj < static_cast<long>(cells) && real[static_cast<std::size_t>(qj)] > sr)
                        {
                            sr = real[static_cast<std::size_t>(qj)];
                            sg = realGrave[static_cast<std::size_t>(qj)];
                        }
                    }
                    if (known && sr > 0.0)
                    {
                        gs.push_back(sg / sr);
                        gw.push_back((g == 'D' || g == 'B' || g == 'G' || g == 'N') ? 1.0 : 0.0);
                    }
                }
                const double score = pearson(realWide, want) + 0.5 * pearson(gs, gw);
                if (score > best.score)
                {
                    best.score = score;
                    best.offset = static_cast<int>(q);
                    best.levelScale = level;
                }
            }
        }
        if (best.score > -1e8)
            out.push_back(best);
    }
    std::sort(out.begin(), out.end(), [](const RankedCycle& a, const RankedCycle& b) { return a.score > b.score; });
    if (static_cast<int>(out.size()) > count)
        out.resize(static_cast<std::size_t>(count));
    return out;
}

const RhythmCycle* findCycle(const std::vector<RhythmCycle>& archive, const std::string& query)
{
    const auto q = lower(trim(query));
    if (q.empty())
        return nullptr;
    for (const auto& c : archive)
        if (lower(c.name) == q)
            return &c;
    const RhythmCycle* hit = nullptr;
    int n = 0;
    for (const auto& c : archive)
        if (lower(c.name).find(q) != std::string::npos)
        {
            hit = &c;
            ++n;
        }
    return n == 1 ? hit : nullptr;
}

std::string cultureRhythm(Score& score, const std::vector<std::pair<double, double>>& spans,
                          const std::vector<const RhythmCycle*>& perSection, const perception::Pulse& pulse,
                          const Options& opt)
{
    if (pulse.period <= 0.0 || std::none_of(perSection.begin(), perSection.end(), [](const RhythmCycle* c) { return c; }))
        return {};
    const Timbre low = allowed(opt.constraints, { Timbre::drum, Timbre::membrane, Timbre::lpg });
    const Timbre high = allowed(opt.constraints, { Timbre::membrane, Timbre::drum, Timbre::lpg });
    const Timbre dry = allowed(opt.constraints, { Timbre::lpg, Timbre::plucked, Timbre::membrane });
    const Timbre wood = allowed(opt.constraints, { Timbre::modal, Timbre::plucked, Timbre::lpg });
    const Timbre gong = allowed(opt.constraints, { Timbre::metal, Timbre::biomaModal, Timbre::modal });
    std::vector<std::string> used;
    int strokes = 0, removed = 0;
    for (std::size_t k = 0; k < spans.size() && k < perSection.size(); ++k)
    {
        const auto* c = perSection[k];
        if (!c)
            continue;
        const auto [a, b] = spans[k];
        // a percussão que houver no trecho dá lugar ao ciclo
        const auto before = score.notes.size();
        score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                         [&](const Note& n) { return n.voice == 4 && n.time >= a - 1e-6 && n.time < b; }),
                          score.notes.end());
        removed += static_cast<int>(before - score.notes.size());
        if (std::find(used.begin(), used.end(), c->name) == used.end())
            used.push_back(c->name);
        // ancorado no primeiro tempo do pulso dentro do trecho
        const double unit = c->unitBeats * pulse.period;
        double t = pulse.phase + std::ceil((a - pulse.phase) / pulse.period - 1e-9) * pulse.period;
        const auto L = static_cast<int>(c->strokes.size());
        for (int i = 0; t < b - 0.05; ++i, t += unit)
        {
            const int pos = i % L;  // 0-based
            const char s = c->strokes[static_cast<std::size_t>(pos)];
            if (s == '-')
                continue;
            const bool accent = std::find(c->accents.begin(), c->accents.end(), pos + 1) != c->accents.end();
            const float vel = accent ? 0.85f : 0.55f;
            const auto hit = [&](Timbre tb, float pitch, double dur, float v) {
                Note n;
                n.time = t;
                n.duration = std::min(dur, b - t);
                n.pitch = pitch;
                n.velocity = v;
                n.voice = 4;
                n.timbre = tb;
                if (n.time + n.duration < score.duration - 0.05)
                {
                    score.notes.push_back(n);
                    ++strokes;
                }
            };
            switch (s)
            {
                case 'D': hit(low, 38.0f, 0.4, vel); break;
                case 'T': hit(high, 64.0f, 0.2, vel * 0.85f); break;
                case 'k': hit(dry, 72.0f, 0.08, vel * 0.5f); break;
                case 'B': hit(low, 38.0f, 0.4, vel); hit(high, 64.0f, 0.25, vel * 0.8f); break;
                case 'x': hit(wood, 79.0f, 0.12, vel); break;
                case 'G': hit(gong, 31.0f, 4.0 * pulse.period, 0.9f); break;
                case 'N': hit(gong, 55.0f, 1.5 * pulse.period, vel); break;
                case 'P': hit(gong, 50.0f, 1.2 * pulse.period, vel * 0.9f); break;
                case 't': hit(gong, 67.0f, 0.15, vel * 0.6f); break;
                default: break;
            }
        }
    }
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& x, const Note& y) {
        return x.time < y.time || (x.time == y.time && x.voice < y.voice);
    });
    std::string names;
    for (const auto& u : used)
        names += (names.empty() ? "" : " · ") + u;
    char text[400];
    std::snprintf(text, sizeof text, "ciclos das culturas: %s (%d golpes; %d golpes do campo rítmico substituídos)",
                  names.c_str(), strokes, removed);
    return text;
}

std::string alignMelodyToCycles(Score& score, const std::vector<SectionScale>& sections,
                                const std::vector<const RhythmCycle*>& cycles,
                                const std::vector<const ScaleBehavior*>& behaviors, const perception::Pulse& pulse)
{
    if (pulse.period <= 0.0)
        return {};
    int arrivals = 0, tihais = 0;
    // a altura de chegada: a nota de apoio (ou a tônica) mais perto de `near`
    const auto arrivalPitch = [](const SectionScale& sc, const ScaleBehavior* b, float near) {
        const double T = sc.scale.tonic, P = sc.scale.period;
        const double target = b && !b->support.empty() ? b->support.front() : 0.0;
        const double rel = 100.0 * near - T;
        const double m = std::round((rel - target) / P);
        return static_cast<float>((T + target + m * P) / 100.0);
    };
    for (std::size_t k = 0; k < sections.size() && k < cycles.size(); ++k)
    {
        const auto* c = cycles[k];
        if (!c)
            continue;
        const auto& sec = sections[k];
        const ScaleBehavior* b = k < behaviors.size() ? behaviors[k] : nullptr;
        const double unit = c->unitBeats * pulse.period;
        const double cycleLen = unit * static_cast<double>(c->strokes.size());
        const double start = pulse.phase + std::ceil((sec.start - pulse.phase) / pulse.period - 1e-9) * pulse.period;
        const int mainPos = c->accents.empty() ? 1 : c->accents.front();
        std::vector<double> points;  // os tempos principais de cada volta
        for (double t = start + (mainPos - 1) * unit; t < sec.end - 0.05; t += cycleLen)
            points.push_back(t);
        const bool tala = c->family.find("tala") != std::string::npos;
        // tihai: antes do último ponto do trecho, se couber
        double tihaiAt = -1.0;
        if (tala && points.size() >= 2)
        {
            const double sam = points.back();
            const double begin = sam - 10.0 * unit;  // 3 frases de 3 notas + 2 respiros de 1 unidade
            if (begin > sec.start + 0.5)
            {
                // a frase: as três notas da melodia logo antes do trecho (o motivo da seção)
                std::vector<float> phrase;
                for (auto it = score.notes.rbegin(); it != score.notes.rend() && phrase.size() < 3; ++it)
                    if (it->voice == 0 && it->time < begin && it->time >= sec.start)
                        phrase.insert(phrase.begin(), it->pitch);
                if (phrase.size() == 3)
                {
                    phrase.back() = arrivalPitch(sec, b, phrase.back());
                    score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                                     [&](const Note& n) { return n.voice == 0 && n.time >= begin - 1e-6 && n.time < sam + 1.5 * unit; }),
                                      score.notes.end());
                    Note proto;
                    for (const auto& n : score.notes)
                        if (n.voice == 0 && n.time >= sec.start && n.time < sec.end)
                        {
                            proto = n;
                            break;
                        }
                    proto.voice = 0;
                    proto.glide = 0.0f;
                    proto.bend.clear();
                    proto.wobbleDepth = 0.0f;
                    for (int r = 0; r < 3; ++r)
                        for (int j = 0; j < 3; ++j)
                        {
                            Note n = proto;
                            n.time = begin + (4.0 * r + j) * unit;
                            n.duration = 0.9 * unit;
                            n.pitch = phrase[static_cast<std::size_t>(j)];
                            n.velocity = 0.6f + 0.1f * static_cast<float>(r);
                            if (r == 2 && j == 2)
                            {
                                n.time = sam;  // a última no sam
                                n.duration = 1.5 * unit;
                                n.velocity = 0.9f;
                            }
                            score.notes.push_back(n);
                        }
                    tihaiAt = begin;
                    ++tihais;
                }
            }
        }
        // chegadas: a nota mais perto de cada ponto principal começa nele
        for (double p : points)
        {
            if (tihaiAt >= 0.0 && p >= tihaiAt - 1e-6)
                continue;  // o tihai já chega
            Note* best = nullptr;
            double bestD = 0.6 * pulse.period;
            for (auto& n : score.notes)
                if (n.voice == 0 && std::abs(n.time - p) < bestD)
                {
                    bestD = std::abs(n.time - p);
                    best = &n;
                }
            if (!best)
                continue;
            best->time = p;
            best->pitch = arrivalPitch(sec, b, best->pitch);
            best->velocity = std::min(1.0f, best->velocity * 1.15f + 0.05f);
            ++arrivals;
        }
    }
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& x, const Note& y) {
        return x.time < y.time || (x.time == y.time && x.voice < y.voice);
    });
    if (arrivals == 0 && tihais == 0)
        return {};
    char text[240];
    std::snprintf(text, sizeof text,
                  "a melodia chega no ciclo: %d chegadas no tempo principal (sam, gong, dum), na nota de apoio; %d tihai "
                  "(frase três vezes, a última no sam)",
                  arrivals, tihais);
    return text;
}

} // namespace trioio::composition
