#include "Versions.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

namespace trioio::composition
{
namespace
{
const char* const kPc[] = { "Dó", "Dó#", "Ré", "Mib", "Mi", "Fá", "Fá#", "Sol", "Láb", "Lá", "Sib", "Si" };

double correlation(const std::vector<float>& a, const std::vector<float>& b)
{
    const std::size_t n = std::min(a.size(), b.size());
    if (n < 3)
        return 0.0;
    double ma = 0.0, mb = 0.0;
    for (std::size_t i = 0; i < n; ++i)
    {
        ma += a[i];
        mb += b[i];
    }
    ma /= n;
    mb /= n;
    double xy = 0.0, xx = 0.0, yy = 0.0;
    for (std::size_t i = 0; i < n; ++i)
    {
        xy += (a[i] - ma) * (b[i] - mb);
        xx += (a[i] - ma) * (a[i] - ma);
        yy += (b[i] - mb) * (b[i] - mb);
    }
    return xx > 1e-12 && yy > 1e-12 ? xy / std::sqrt(xx * yy) : 0.0;
}

// o mesmo pulso: a razão perto de 1, 2 ou 1/2 (±4 %)
bool sameTempo(double a, double b)
{
    if (a <= 0.0 || b <= 0.0)
        return false;
    double r = std::abs(std::log2(b / a));
    r = std::min({ r, std::abs(r - 1.0), std::abs(r + 1.0) });
    return r < std::log2(1.04);
}

std::string keyText(const VersionReading& v)
{
    return v.tonic < 0 ? std::string("sem tonalidade clara") : std::string(kPc[v.tonic]) + (v.minor ? " menor" : " maior");
}

std::string num(double x, const char* f = "%.2f")
{
    char b[32];
    std::snprintf(b, sizeof b, f, x);
    return b;
}
} // namespace

VersionReading readVersion(const std::string& id, const std::vector<perception::Frame>& frames,
                           const std::vector<perception::Onset>& onsets, double duration, const Reading& reading)
{
    VersionReading v;
    v.id = id;
    v.duration = duration;
    v.bpm = reading.pulse.bpm;
    v.pulseConfidence = reading.pulse.confidence;
    v.tonic = reading.key.tonic;
    v.minor = reading.key.minor;
    v.sections = static_cast<int>(reading.boundaries.size()) + 1;
    v.intent = inferIntent(frames, onsets, duration, reading);
    v.rootsByPosition.assign(64, -1);
    for (int k = 0; k < 64 && duration > 0.0; ++k)
    {
        const double t = (k + 0.5) / 64.0 * duration;
        for (const auto& c : reading.chords)
            if (t >= c.start && t < c.end)
            {
                v.rootsByPosition[static_cast<std::size_t>(k)] = c.chord.root;
                break;
            }
    }
    v.energyCurve.assign(32, 0.0f);
    std::vector<int> count(32, 0);
    for (const auto& f : frames)
    {
        if (duration <= 0.0)
            break;
        const auto k = static_cast<std::size_t>(std::clamp(f.time / duration * 32.0, 0.0, 31.0));
        v.energyCurve[k] += std::clamp((f.rmsDb + 60.0f) / 60.0f, 0.0f, 1.0f);
        ++count[k];
    }
    for (std::size_t k = 0; k < 32; ++k)
        if (count[k] > 0)
            v.energyCurve[k] /= static_cast<float>(count[k]);
    return v;
}

VersionComparison compareVersions(const std::vector<VersionReading>& vs)
{
    VersionComparison out;
    if (vs.size() < 2)
    {
        out.divergences.push_back("uma versão só: nada a comparar");
        return out;
    }
    const auto& a = vs.front();
    bool allTempo = true, allKey = true, allForm = true, allChords = true, allDuration = true;
    std::vector<double> formCorr, chordAgree;
    for (std::size_t i = 1; i < vs.size(); ++i)
    {
        const auto& b = vs[i];
        const bool reliable = a.pulseConfidence >= 0.3f && b.pulseConfidence >= 0.3f;
        if (!(reliable && sameTempo(a.bpm, b.bpm)))
        {
            allTempo = false;
            out.divergences.push_back(b.id + ": andamento " + num(b.bpm, "%.0f") + " BPM × " + num(a.bpm, "%.0f") + " BPM"
                                      + (reliable ? "" : " (pulso pouco confiável)"));
        }
        if (a.tonic != b.tonic || a.minor != b.minor)
        {
            allKey = false;
            out.divergences.push_back(b.id + ": tonalidade " + keyText(b) + " × " + keyText(a)
                                      + (a.tonic == b.tonic && a.tonic >= 0 ? " (o mesmo centro, outro modo)" : ""));
        }
        const double fc = correlation(a.energyCurve, b.energyCurve);
        formCorr.push_back(fc);
        if (fc < 0.7)
        {
            allForm = false;
            out.divergences.push_back(b.id + ": outra curva de energia (correlação " + num(fc) + ") — a forma muda");
        }
        int both = 0, same = 0;
        for (std::size_t k = 0; k < 64; ++k)
            if (a.rootsByPosition[k] >= 0 && b.rootsByPosition[k] >= 0)
            {
                ++both;
                same += a.rootsByPosition[k] == b.rootsByPosition[k] ? 1 : 0;
            }
        const double agree = both >= 8 ? static_cast<double>(same) / both : 0.0;
        chordAgree.push_back(agree);
        if (agree < 0.5)
        {
            allChords = false;
            out.divergences.push_back(b.id + ": outra progressão (" + num(100.0 * agree, "%.0f") + " % das fundamentais iguais)");
        }
        if (std::abs(b.duration - a.duration) > 0.1 * a.duration)
        {
            allDuration = false;
            out.divergences.push_back(b.id + ": " + num(b.duration, "%.0f") + " s × " + num(a.duration, "%.0f") + " s");
        }
        // a intenção ouvida: o que muda mais de 0,2
        const auto gaps = compareIntent(a.intent, b.intent);
        for (const auto& g : gaps)
            if (g.gap >= 0.2f && g.field != "tempo")
            {
                const std::string pt = g.field == "energy" ? "energia" : g.field == "density" ? "densidade" : g.field == "brightness" ? "brilho"
                                     : g.field == "harmonicity" ? "harmonicidade" : g.field == "mood" ? "humor" : g.field;
                out.divergences.push_back(b.id + ": " + pt + " " + (g.field == "mood" ? std::string(intentionName(static_cast<Intention>(static_cast<int>(g.observed))))
                                                                                     : num(g.observed))
                                          + " × " + (g.field == "mood" ? std::string(intentionName(static_cast<Intention>(static_cast<int>(g.declared)))) : num(g.declared)));
            }
    }
    const double meanForm = std::accumulate(formCorr.begin(), formCorr.end(), 0.0) / formCorr.size();
    const double meanChord = std::accumulate(chordAgree.begin(), chordAgree.end(), 0.0) / chordAgree.size();
    if (allTempo)
        out.invariants.push_back("o andamento (~" + num(a.bpm, "%.0f") + " BPM, ou o dobro/metade)");
    if (allKey && a.tonic >= 0)
        out.invariants.push_back("a tonalidade (" + keyText(a) + ")");
    if (allForm)
        out.invariants.push_back("a curva de energia, a forma (correlação média " + num(meanForm) + ")");
    if (allChords)
        out.invariants.push_back("a progressão (" + num(100.0 * meanChord, "%.0f") + " % das fundamentais iguais)");
    if (allDuration)
        out.invariants.push_back("a duração (~" + num(a.duration, "%.0f") + " s)");
    if (out.invariants.empty())
        out.invariants.push_back("nenhum: as versões são peças diferentes no que a escuta lê");
    // as operações
    std::string others;
    for (std::size_t i = 1; i < vs.size(); ++i)
        others += (i > 1 ? ", " : "") + vs[i].id;
    if (allTempo && allKey)
        out.operations.push_back("mashup: responder " + a.id + " --com " + vs[1].id
                                 + " (o mesmo andamento e tom: as camadas de uma casam com as da outra)");
    else if (allTempo)
        out.operations.push_back("mashup só do ritmo: responder " + a.id + " --com " + vs[1].id + " --da-outra ritmo (o tom diverge)");
    if (allForm && !(allTempo && allKey))
        out.operations.push_back("a mesma forma com outra matéria: a resposta pode seguir a forma de uma e o campo de outra (--com)");
    if (!out.divergences.empty())
        out.operations.push_back("contrapor: o que diverge é o espaço do Terceiro Olho; responder a uma com a intenção da outra "
                                 "(prompt --revisar com os valores da outra)");
    if (allChords)
        out.operations.push_back("a mesma progressão: a harmonia escrita pode reler as versões com outra linguagem (--linguagem-harmonica)");
    out.operations.push_back("linhas: as regiões de frequência de cada versão (--linhas) para ver em que banda elas diferem");
    return out;
}

} // namespace trioio::composition
