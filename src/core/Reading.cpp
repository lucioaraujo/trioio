#include "Reading.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace trioio::composition
{
using namespace trioio::perception;

namespace
{
const char* const kPcName[12] = { "Dó", "Dó#", "Ré", "Mib", "Mi", "Fá", "Fá#", "Sol", "Lá♭", "Lá", "Sib", "Si" };

// Krumhansl e Kessler (1982), perfis de tonalidade de Dó maior e Dó menor
constexpr double kMajor[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
constexpr double kMinor[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };

int mod12(int x) { return ((x % 12) + 12) % 12; }

double pearson(const double* a, const double* b, int n)
{
    double ma = 0.0, mb = 0.0;
    for (int i = 0; i < n; ++i) { ma += a[i]; mb += b[i]; }
    ma /= n; mb /= n;
    double sab = 0.0, saa = 0.0, sbb = 0.0;
    for (int i = 0; i < n; ++i)
    {
        sab += (a[i] - ma) * (b[i] - mb);
        saa += (a[i] - ma) * (a[i] - ma);
        sbb += (b[i] - mb) * (b[i] - mb);
    }
    return saa > 0.0 && sbb > 0.0 ? sab / std::sqrt(saa * sbb) : 0.0;
}

std::string trim(const std::string& s)
{
    const auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos)
        return {};
    return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

// minúsculas ASCII e sem os acentos das notas (dó ré fá lá; Dó Ré Fá Lá)
std::string fold(std::string s)
{
    const std::pair<const char*, const char*> acc[] = { { "ó", "o" }, { "é", "e" }, { "á", "a" }, { "Ó", "o" },
                                                        { "É", "e" }, { "Á", "a" }, { "ê", "e" }, { "ç", "c" },
                                                        { "õ", "o" }, { "ã", "a" }, { "í", "i" }, { "ú", "u" },
                                                        { "♯", "#" }, { "♭", "b" } };
    for (const auto& [from, to] : acc)
        for (std::size_t p = s.find(from); p != std::string::npos; p = s.find(from, p))
            s.replace(p, std::string(from).size(), to);
    for (auto& c : s)
        if (static_cast<unsigned char>(c) < 128)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// a nota no começo de `s` (já dobrado): devolve a classe e consome os bytes
bool parseNote(const std::string& s, std::size_t& pos, int& pc)
{
    static const std::pair<const char*, int> names[] = { { "sol", 7 }, { "do", 0 }, { "re", 2 }, { "mi", 4 }, { "fa", 5 },
                                                         { "la", 9 }, { "si", 11 }, { "c", 0 }, { "d", 2 }, { "e", 4 },
                                                         { "f", 5 }, { "g", 7 }, { "a", 9 }, { "b", 11 } };
    for (const auto& [name, value] : names)
    {
        const std::string n = name;
        if (s.compare(pos, n.size(), n) == 0)
        {
            pc = value;
            pos += n.size();
            // acidentes: "#" sobe; "b" desce (mas "b" sozinho depois de nota
            // inglesa já foi consumido como a nota Si)
            while (pos < s.size() && (s[pos] == '#' || s[pos] == 'b'))
            {
                pc += s[pos] == '#' ? 1 : -1;
                ++pos;
            }
            pc = mod12(pc);
            return true;
        }
    }
    return false;
}
} // namespace

std::string Key::name() const
{
    if (tonic < 0)
        return "?";
    return std::string(kPcName[mod12(tonic)]) + (minor ? " menor" : " maior");
}

Key keyFromChroma(const std::array<float, 12>& chroma)
{
    Key k;
    double c[12];
    double total = 0.0;
    for (int i = 0; i < 12; ++i)
        total += c[i] = std::max(0.0f, chroma[static_cast<std::size_t>(i)]);
    if (total <= 0.0)
        return k;
    struct Cand { double r; int tonic; bool minor; };
    std::vector<Cand> all;
    for (int t = 0; t < 12; ++t)
        for (bool minor : { false, true })
        {
            double prof[12];
            for (int i = 0; i < 12; ++i)
                prof[i] = (minor ? kMinor : kMajor)[mod12(i - t)];
            all.push_back({ pearson(c, prof, 12), t, minor });
        }
    std::sort(all.begin(), all.end(), [](const Cand& a, const Cand& b) { return a.r > b.r; });
    k.tonic = all[0].tonic;
    k.minor = all[0].minor;
    k.correlation = static_cast<float>(all[0].r);
    k.secondTonic = all[1].tonic;
    k.secondMinor = all[1].minor;
    k.secondCorrelation = static_cast<float>(all[1].r);
    return k;
}

Key keyFromChords(const std::vector<ChordSpan>& chords)
{
    std::array<float, 12> c {};
    for (const auto& s : chords)
    {
        const double d = std::max(0.0, s.end - s.start);
        const auto pcs = s.chord.pitchClasses();
        for (std::size_t i = 0; i < pcs.size(); ++i)
            c[static_cast<std::size_t>(mod12(pcs[i]))] += static_cast<float>(d * (i == 0 ? 1.5 : 1.0));
    }
    return keyFromChroma(c);
}

Key estimateKey(const std::vector<Frame>& frames, double t0, double t1)
{
    return keyFromChroma(harmonicField(frames, t0, t1));
}

float bassAgreement(const std::vector<Frame>& frames)
{
    std::size_t withBass = 0, same = 0;
    for (const auto& f : frames)
    {
        if (f.silent)
            continue;
        const auto bmax = std::max_element(f.bassChroma.begin(), f.bassChroma.end());
        if (*bmax <= 0.0f)
            continue;
        ++withBass;
        same += (bmax - f.bassChroma.begin()) == (std::max_element(f.chroma.begin(), f.chroma.end()) - f.chroma.begin());
    }
    return withBass > 0 ? static_cast<float>(same) / static_cast<float>(withBass) : -1.0f;
}

Reading readReference(const std::vector<Frame>& frames, const std::vector<Onset>& onsets, double duration,
                      bool byRepetition, double tempoScale, PulseEnvelope envelope)
{
    Reading r;
    r.duration = duration;
    Pulse p = estimatePulse(frames, onsets, 0.0, duration, envelope);
    if (p.period > 0.0 && tempoScale > 0.0 && std::abs(tempoScale - 1.0) > 1e-9)
    {
        p.period /= tempoScale;
        p.bpm *= tempoScale;
        p.phase = std::fmod(p.phase, p.period);
    }
    r.pulse = p;
    r.downbeat = estimateDownbeat(frames, p, 0.0, duration);
    if (p.period > 0.0)
        for (double t = p.phase; t < duration; t += p.period)
            r.beats.push_back(t);
    r.chromaKey = estimateKey(frames, 0.0, duration);
    if (p.period > 0.0)
        r.chords = readChords(frames, duration, p.phase, p.period);
    r.key = r.chords.empty() ? r.chromaKey : keyFromChords(r.chords);
    std::vector<Section> secs;
    if (byRepetition)
        secs = findSectionsByRepetition(frames, onsets, duration, r.labels);
    else
        secs = findSections(frames, onsets, duration);
    for (std::size_t i = 1; i < secs.size(); ++i)
        r.boundaries.push_back(secs[i].start);
    std::size_t sounding = 0, pitched = 0;
    for (const auto& f : frames)
        if (!f.silent)
        {
            ++sounding;
            pitched += f.f0Hz > 0.0f && f.periodicity >= 0.6f;
        }
    r.pitchedShare = sounding > 0 ? static_cast<float>(pitched) / static_cast<float>(sounding) : 0.0f;
    r.bassAgreement = bassAgreement(frames);
    if (!onsets.empty() && frames.size() > 1)
    {
        const double hop = frames[1].time - frames[0].time;
        std::size_t withPitch = 0, withYin = 0;
        for (const auto& o : onsets)
        {
            const double t = o.time + 0.04;
            withPitch += std::isfinite(pitchAt(frames, t));
            const auto c = static_cast<long>(std::lround((t - frames.front().time) / hop));
            bool yin = false;
            for (long i = c - 2; i <= c + 2 && !yin; ++i)
                if (i >= 0 && i < static_cast<long>(frames.size()))
                {
                    const auto& f = frames[static_cast<std::size_t>(i)];
                    yin = !f.silent && f.f0Hz > 0.0f && f.periodicity >= 0.6f;
                }
            withYin += yin;
        }
        r.onsetsPitched = static_cast<float>(withPitch) / static_cast<float>(onsets.size());
        r.onsetsPitchedYin = static_cast<float>(withYin) / static_cast<float>(onsets.size());
    }
    std::vector<float> mel;
    std::array<double, 12> melPc {}, chordPc {};
    for (const auto& f : frames)
        if (!f.silent && f.melodyHz > 0.0f)
        {
            mel.push_back(static_cast<float>(69.0 + 12.0 * std::log2(f.melodyHz / 440.0)));
            melPc[static_cast<std::size_t>(mod12(static_cast<int>(std::lround(mel.back()))))] += 1.0;
        }
    for (const auto& s : r.chords)
        for (int pc : s.chord.pitchClasses())
            chordPc[static_cast<std::size_t>(mod12(pc))] += s.end - s.start;
    double sm = 0.0, sc = 0.0;
    for (std::size_t i = 0; i < 12; ++i) { sm += melPc[i]; sc += chordPc[i]; }
    for (std::size_t i = 0; i < 12; ++i)
        r.structuralField[i] = static_cast<float>((sm > 0.0 ? 0.5 * melPc[i] / sm : 0.0) + (sc > 0.0 ? 0.5 * chordPc[i] / sc : 0.0));
    if (!mel.empty())
    {
        std::sort(mel.begin(), mel.end());
        const auto at = [&](double q) { return mel[static_cast<std::size_t>(q * static_cast<double>(mel.size() - 1))]; };
        r.melodyP10 = at(0.1);
        r.melodyP50 = at(0.5);
        r.melodyP90 = at(0.9);
    }
    return r;
}

// ---- gabarito ------------------------------------------------------------------

Chord chordFromIndex(int index)
{
    const ChordQuality q[3] = { ChordQuality::major, ChordQuality::minor, ChordQuality::diminished };
    return { mod12(index / 3), q[((index % 3) + 3) % 3] };
}

int chordIndex(const Chord& c)
{
    return mod12(c.root) * 3 + (c.quality == ChordQuality::major ? 0 : c.quality == ChordQuality::minor ? 1 : 2);
}

int parseChordName(const std::string& text)
{
    const auto s = fold(trim(text));
    if (s == "n" || s == "-" || s == "nc" || s == "sem acorde")
        return -1;
    std::size_t pos = 0;
    int pc = 0;
    if (!parseNote(s, pos, pc))
        return -2;
    const auto rest = trim(s.substr(pos));
    if (rest.empty() || rest == "maior" || rest == "maj")
        return pc * 3;
    if (rest == "m" || rest == "menor" || rest == "min")
        return pc * 3 + 1;
    if (rest == "°" || rest == "dim" || rest == "o" || rest == "diminuto")
        return pc * 3 + 2;
    return -2;
}

bool parseKeyName(const std::string& text, int& tonic, bool& minor)
{
    const int c = parseChordName(text);
    if (c < 0 || c % 3 == 2)
        return false;
    tonic = c / 3;
    minor = c % 3 == 1;
    return true;
}

bool parseClock(const std::string& text, double& seconds)
{
    const auto s = trim(text);
    if (s.empty())
        return false;
    try
    {
        std::size_t used = 0;
        const auto colon = s.find(':');
        if (colon == std::string::npos)
        {
            seconds = std::stod(s, &used);
            return used == s.size() && seconds >= 0.0;
        }
        const double m = std::stod(s.substr(0, colon), &used);
        if (used != colon)
            return false;
        const auto rest = s.substr(colon + 1);
        const double sec = std::stod(rest, &used);
        if (used != rest.size() || sec < 0.0 || sec >= 60.0 || m < 0.0)
            return false;
        seconds = m * 60.0 + sec;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

Gabarito parseGabarito(const std::string& text, std::vector<std::string>& errors)
{
    Gabarito g;
    std::istringstream in(text);
    std::string line;
    int number = 0;
    const auto err = [&](const std::string& what) { errors.push_back("linha " + std::to_string(number) + ": " + what); };
    while (std::getline(in, line))
    {
        ++number;
        if (const auto hash = line.find('#'); hash != std::string::npos)
            line = line.substr(0, hash);
        line = trim(line);
        if (line.empty())
            continue;
        const auto colon = line.find(':');
        if (colon == std::string::npos)
        {
            err("sem \"chave:\"");
            continue;
        }
        const auto key = fold(trim(line.substr(0, colon)));
        const auto value = trim(line.substr(colon + 1));
        if (key == "referencia")
            g.reference = value;
        else if (key == "fonte")
            g.source = value;
        else if (key == "andamento")
        {
            if (!parseClock(value, g.bpm) || g.bpm < 20.0 || g.bpm > 400.0)
                err("andamento ilegível: " + value), g.bpm = 0.0;
        }
        else if (key == "compasso")
        {
            g.beatsPerBar = std::atoi(value.c_str());
            if (g.beatsPerBar < 1 || g.beatsPerBar > 32)
                err("compasso ilegível: " + value), g.beatsPerBar = 0;
        }
        else if (key == "tonalidade")
        {
            g.hasKey = parseKeyName(value, g.keyTonic, g.keyMinor);
            if (!g.hasKey)
                err("tonalidade ilegível: " + value);
        }
        else if (key == "fim")
        {
            if (!parseClock(value, g.end))
                err("fim ilegível: " + value);
        }
        else if (key == "batidas")
        {
            std::istringstream ts(value);
            std::string t;
            while (ts >> t)
            {
                double x = 0.0;
                if (parseClock(t, x))
                    g.beats.push_back(x);
                else
                    err("batida ilegível: " + t);
            }
            std::sort(g.beats.begin(), g.beats.end());
        }
        else if (key == "acordes" || key == "secoes")
        {
            // "0:00 Solm, 0:02.8 Ré, …": cada item é "instante rótulo"
            std::istringstream ps(value);
            std::string item;
            while (std::getline(ps, item, ','))
            {
                item = trim(item);
                if (item.empty())
                    continue;
                const auto sp = item.find(' ');
                double t = 0.0;
                if (sp == std::string::npos || !parseClock(item.substr(0, sp), t))
                {
                    err("item sem instante: \"" + item + "\"");
                    continue;
                }
                const auto label = trim(item.substr(sp + 1));
                if (key == "acordes")
                {
                    const int c = parseChordName(label);
                    if (c == -2)
                        err("acorde ilegível: \"" + label + "\"");
                    else
                        g.chords.push_back({ t, c });
                }
                else
                    g.sections.push_back({ t, label });
            }
            std::sort(g.chords.begin(), g.chords.end());
            std::sort(g.sections.begin(), g.sections.end());
        }
        else
            err("chave desconhecida: " + key);
    }
    if (g.bpm <= 0.0 && g.beats.size() >= 3)
    {
        std::vector<double> ibi;
        for (std::size_t i = 1; i < g.beats.size(); ++i)
            ibi.push_back(g.beats[i] - g.beats[i - 1]);
        std::nth_element(ibi.begin(), ibi.begin() + static_cast<std::ptrdiff_t>(ibi.size() / 2), ibi.end());
        const double m = ibi[ibi.size() / 2];
        if (m > 0.0)
            g.bpm = 60.0 / m;
    }
    return g;
}

Match matchTimes(const std::vector<double>& estimated, const std::vector<double>& reference, double tolerance)
{
    Match m;
    if (estimated.empty() || reference.empty())
        return m;
    std::vector<bool> used(reference.size(), false);
    std::size_t hits = 0;
    for (double e : estimated)
    {
        std::size_t best = reference.size();
        double bestD = tolerance;
        for (std::size_t j = 0; j < reference.size(); ++j)
            if (!used[j] && std::abs(reference[j] - e) <= bestD)
            {
                bestD = std::abs(reference[j] - e);
                best = j;
            }
        if (best < reference.size())
        {
            used[best] = true;
            ++hits;
        }
    }
    m.precision = static_cast<double>(hits) / static_cast<double>(estimated.size());
    m.recall = static_cast<double>(hits) / static_cast<double>(reference.size());
    m.f = m.precision + m.recall > 0.0 ? 2.0 * m.precision * m.recall / (m.precision + m.recall) : 0.0;
    return m;
}

Evaluation evaluate(const Reading& r, const Gabarito& g)
{
    Evaluation e;
    const double end = g.end > 0.0 ? std::min(g.end, r.duration) : r.duration;
    // andamento
    if (g.bpm > 0.0 && r.pulse.bpm > 0.0)
    {
        e.tempoRatio = r.pulse.bpm / g.bpm;
        const std::pair<double, const char*> rel[] = { { 1.0, "certo" }, { 0.5, "metade" }, { 2.0, "dobro" },
                                                       { 1.0 / 3.0, "terço" }, { 3.0, "triplo" },
                                                       { 2.0 / 3.0, "dois terços" }, { 1.5, "uma vez e meia" },
                                                       { 4.0 / 3.0, "quatro terços" }, { 0.75, "três quartos" } };
        e.tempoVerdict = "outro";
        for (const auto& [ratio, name] : rel)
            if (std::abs(e.tempoRatio / ratio - 1.0) <= 0.04)
            {
                e.tempoVerdict = name;
                break;
            }
    }
    // tempos (±70 ms; os primeiros 5 s ficam de fora, como no MIREX)
    if (!g.beats.empty())
    {
        e.hasBeats = true;
        std::vector<double> est, ref;
        for (double t : r.beats)
            if (t >= 5.0 && t < end) est.push_back(t);
        for (double t : g.beats)
            if (t >= 5.0 && t < end) ref.push_back(t);
        const auto m = matchTimes(est, ref, 0.07);
        e.beatF = m.f;
        e.beatPrecision = m.precision;
        e.beatRecall = m.recall;
    }
    // tonalidade (pontuação do MIREX)
    if (g.hasKey && r.key.tonic >= 0)
    {
        e.hasKey = true;
        const int d = mod12(r.key.tonic - g.keyTonic);
        if (r.key.minor == g.keyMinor && d == 0)
            e.keyScore = 1.0, e.keyRelation = "certa";
        else if (r.key.minor == g.keyMinor && (d == 7 || d == 5))
            e.keyScore = 0.5, e.keyRelation = d == 7 ? "a dominante" : "a subdominante";
        else if (!g.keyMinor && r.key.minor && d == 9)
            e.keyScore = 0.3, e.keyRelation = "a relativa menor";
        else if (g.keyMinor && !r.key.minor && d == 3)
            e.keyScore = 0.3, e.keyRelation = "a relativa maior";
        else if (r.key.minor != g.keyMinor && d == 0)
            e.keyScore = 0.2, e.keyRelation = "a homônima";
        else
            e.keyScore = 0.0, e.keyRelation = "outra";
    }
    // acordes: fração do tempo (de 0 ao fim, onde o gabarito tem acorde) em
    // que a leitura dá o mesmo acorde
    if (!g.chords.empty())
    {
        e.hasChords = true;
        double total = 0.0, same = 0.0;
        const double step = 0.05;
        std::size_t gi = 0, ri = 0;
        for (double t = g.chords.front().first; t < end; t += step)
        {
            while (gi + 1 < g.chords.size() && g.chords[gi + 1].first <= t)
                ++gi;
            const int truth = g.chords[gi].second;
            if (truth < 0)
                continue;
            total += step;
            while (ri < r.chords.size() && r.chords[ri].end <= t)
                ++ri;
            if (ri < r.chords.size() && r.chords[ri].start <= t && chordIndex(r.chords[ri].chord) == truth)
                same += step;
        }
        e.chordOverlap = total > 0.0 ? same / total : 0.0;
    }
    // fronteiras de seção (±3 s), sem o começo
    if (g.sections.size() >= 2)
    {
        e.hasSections = true;
        std::vector<double> ref;
        for (std::size_t i = 1; i < g.sections.size(); ++i)
            if (g.sections[i].first < end) ref.push_back(g.sections[i].first);
        const auto m = matchTimes(r.boundaries, ref, 3.0);
        e.boundaryF = m.f;
        e.boundaryPrecision = m.precision;
        e.boundaryRecall = m.recall;
    }
    return e;
}

void resampleSpeed(const std::vector<float>& x, double rate, std::vector<float>& y)
{
    constexpr int a = 8;
    constexpr double pi = 3.14159265358979323846;
    const double cut = std::min(1.0, 1.0 / rate);  // acelerar baixa a banda: filtra antes
    const double half = a / cut;
    const auto n = static_cast<std::size_t>(static_cast<double>(x.size()) / rate);
    y.assign(n, 0.0f);
    const auto sinc = [&](double v) { return v == 0.0 ? 1.0 : std::sin(pi * v) / (pi * v); };
    for (std::size_t i = 0; i < n; ++i)
    {
        const double t = static_cast<double>(i) * rate;
        const auto k0 = static_cast<long long>(std::ceil(t - half)), k1 = static_cast<long long>(std::floor(t + half));
        double acc = 0.0, wsum = 0.0;
        for (long long k = std::max(0LL, k0); k <= std::min(static_cast<long long>(x.size()) - 1, k1); ++k)
        {
            const double d = (t - static_cast<double>(k)) * cut;
            const double w = cut * sinc(d) * sinc(d / a);
            acc += w * x[static_cast<std::size_t>(k)];
            wsum += w;
        }
        y[i] = static_cast<float>(wsum != 0.0 ? acc / wsum : 0.0);  // ganho 1 em DC
    }
}

Equivariance compareTransformed(const Reading& o, const std::vector<Frame>& of, const Reading& t,
                                const std::vector<Frame>& tf, double rate, int k)
{
    Equivariance e;
    if (o.pulse.bpm > 0.0 && t.pulse.bpm > 0.0)
        e.tempoRatio = t.pulse.bpm / o.pulse.bpm / rate;
    e.keyOk = o.key.tonic >= 0 && t.key.tonic >= 0 && mod12(t.key.tonic - o.key.tonic) == mod12(k)
           && t.key.minor == o.key.minor;
    // acordes: no instante s do original, o acorde transposto deve estar no
    // instante s ÷ rate do transformado
    {
        double total = 0.0, same = 0.0;
        std::size_t oi = 0, ti = 0;
        for (double s = 0.0; s < o.duration; s += 0.05)
        {
            while (oi < o.chords.size() && o.chords[oi].end <= s) ++oi;
            if (oi >= o.chords.size() || o.chords[oi].start > s)
                continue;
            total += 1.0;
            Chord want = o.chords[oi].chord;
            want.root = mod12(want.root + k);
            const double u = s / rate;
            while (ti < t.chords.size() && t.chords[ti].end <= u) ++ti;
            same += ti < t.chords.size() && t.chords[ti].start <= u && t.chords[ti].chord == want;
        }
        e.chordAgree = total > 0.0 ? same / total : 0.0;
    }
    // melodia
    {
        std::size_t total = 0, same = 0, j = 0;
        for (const auto& f : of)
        {
            if (f.silent || f.melodyHz <= 0.0f)
                continue;
            const double u = f.time / rate;
            while (j + 1 < tf.size() && std::abs(tf[j + 1].time - u) <= std::abs(tf[j].time - u)) ++j;
            if (j >= tf.size() || tf[j].melodyHz <= 0.0f)
                continue;
            ++total;
            same += std::abs(12.0 * std::log2(tf[j].melodyHz / f.melodyHz) - k) <= 0.5;
        }
        e.melodyAgree = total > 0 ? static_cast<double>(same) / static_cast<double>(total) : 0.0;
    }
    std::vector<double> scaled;
    for (double b : o.boundaries)
        scaled.push_back(b / rate);
    e.boundaryF = matchTimes(t.boundaries, scaled, 3.0).f;
    return e;
}

Gabarito gabaritoFromScore(const Score& score)
{
    Gabarito g;
    g.source = "autogabarito (partitura do Trioio)";
    g.bpm = score.bpm;
    g.end = score.duration;
    const double unit = score.bpm > 0.0 ? 60.0 / score.bpm : 0.5;
    std::array<float, 12> whole {};
    for (double t = 0.0; t < score.duration; t += unit)
    {
        std::array<double, 12> c {};
        for (const auto& n : score.notes)
        {
            if (n.voice == 4)
                continue;
            const double inside = std::min(t + unit, n.time + n.duration) - std::max(t, n.time);
            if (inside <= 0.0)
                continue;
            const double w = inside * n.velocity;
            const int pc = mod12(static_cast<int>(std::lround(n.pitch)));
            c[static_cast<std::size_t>(pc)] += w;
            whole[static_cast<std::size_t>(pc)] += static_cast<float>(w);
        }
        double norm = 0.0;
        for (double x : c)
            norm += x * x;
        norm = std::sqrt(norm);
        int best = -1;
        double bestScore = 0.55;  // o mesmo piso de confiança da leitura (Harmony.cpp)
        if (norm > 0.0)
            for (int k = 0; k < 36; ++k)
            {
                const auto pcs = chordFromIndex(k).pitchClasses();
                const double wt[3] = { 1.0, 0.8, 0.8 };
                double dot = 0.0;
                for (std::size_t j = 0; j < 3; ++j)
                    dot += wt[j] * c[static_cast<std::size_t>(mod12(pcs[j]))];
                const double sc = dot / (norm * std::sqrt(2.28)) - (k % 3 == 2 ? 0.04 : 0.0);
                if (sc > bestScore)
                {
                    bestScore = sc;
                    best = k;
                }
            }
        if (g.chords.empty() || g.chords.back().second != best)
            g.chords.push_back({ t, best });
    }
    const auto key = keyFromChroma(whole);
    if (key.tonic >= 0)
    {
        g.hasKey = true;
        g.keyTonic = key.tonic;
        g.keyMinor = key.minor;
    }
    const std::string prefix = "seção ";
    for (const auto& m : score.marks)
        if (m.text.rfind(prefix, 0) == 0)
            g.sections.push_back({ m.time, std::to_string(std::atoi(m.text.c_str() + prefix.size())) });
    std::sort(g.sections.begin(), g.sections.end());
    return g;
}

MelodyScore melodyAgainstScore(const std::vector<Frame>& frames, const Score& score)
{
    MelodyScore r;
    std::vector<const Note*> mel;
    for (const auto& n : score.notes)
        if (n.voice == 0 && n.duration > 0.15)
            mel.push_back(&n);
    int okPitch = 0, okChroma = 0;
    for (const auto& f : frames)
    {
        if (f.silent)
            continue;
        const Note* hit = nullptr;
        int sounding = 0;
        for (const auto* n : mel)
            if (f.time >= n->time + 0.06 && f.time <= n->time + n->duration - 0.06)
            {
                hit = n;
                ++sounding;
            }
        if (sounding != 1)
            continue;  // pausa ou duas notas da voz 0 ao mesmo tempo: sem resposta única
        ++r.frames;
        const double truth = hit->pitchAt(f.time - hit->time);
        if (f.melodyHz > 0.0f)
        {
            const double est = 69.0 + 12.0 * std::log2(f.melodyHz / 440.0);
            okPitch += std::abs(est - truth) <= 0.5;
            const double d = std::fmod(std::abs(est - truth), 12.0);
            const bool sameClass = std::min(d, 12.0 - d) <= 0.5;
            okChroma += sameClass;
            if (sameClass && std::abs(est - truth) > 0.5)
                (est < truth ? r.octaveBelow : r.octaveAbove) += 1.0;
        }
    }
    if (r.frames > 0)
    {
        r.pitch = static_cast<double>(okPitch) / r.frames;
        r.chroma = static_cast<double>(okChroma) / r.frames;
        r.octaveBelow /= r.frames;
        r.octaveAbove /= r.frames;
    }
    return r;
}

} // namespace trioio::composition
