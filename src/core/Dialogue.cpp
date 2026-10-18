#include "Dialogue.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace trioio::composition
{

DialogueMap dialogueMap(const std::vector<perception::Frame>& frames, double duration, const perception::Pulse& pulse)
{
    DialogueMap m;
    if (frames.size() < 8 || duration <= 0.0)
        return m;
    // a energia suavizada em ±0,125 s
    const double hop = frames[1].time - frames[0].time;
    const auto half = std::max<long>(1, static_cast<long>(0.125 / std::max(hop, 1e-3)));
    std::vector<float> e(frames.size());
    for (long i = 0; i < static_cast<long>(frames.size()); ++i)
    {
        double s = 0.0;
        int k = 0;
        for (long j = std::max(0L, i - half); j <= std::min(static_cast<long>(frames.size()) - 1, i + half); ++j, ++k)
            s += frames[static_cast<std::size_t>(j)].rmsDb;
        e[static_cast<std::size_t>(i)] = static_cast<float>(s / k);
    }
    auto sorted = e;
    std::sort(sorted.begin(), sorted.end());
    // pausa é relativa (abaixo do meio entre os percentis 20 e 80) E funda
    // (≥ 12 dB abaixo do nível forte): uma oscilação de poucos dB numa música
    // contínua não é pausa
    const float strong = sorted[sorted.size() * 4 / 5];
    const float thr = std::min(0.5f * (sorted[sorted.size() / 5] + strong), strong - 12.0f);
    // as pausas: abaixo do limiar por ≥ 0,5 s
    std::vector<std::pair<double, double>> rests;
    double start = -1.0, rest = 0.0;
    for (std::size_t i = 0; i <= frames.size(); ++i)
    {
        const bool low = i < frames.size() && e[i] < thr;
        const double t = i < frames.size() ? frames[i].time : duration;
        if (low && start < 0.0)
            start = t;
        else if (!low && start >= 0.0)
        {
            if (t - start >= 0.5)
            {
                rests.push_back({ start, t });
                rest += t - start;
            }
            start = -1.0;
        }
    }
    if (rest >= 0.12 * duration)
    {
        double at = 0.0;
        for (const auto& r : rests)
        {
            if (r.first > at)
                m.calls.push_back({ at, r.first });
            m.answers.push_back(r);
            at = r.second;
        }
        if (at < duration)
            m.calls.push_back({ at, duration });
        return m;
    }
    // música contínua: alternância de 2 compassos (a referência abre)
    m.alternating = true;
    const double len = pulse.period > 0.0 ? 2.0 * pulse.period * std::max(1, pulse.beatsPerBar) : 4.0;
    const double origin = pulse.period > 0.0 ? std::fmod(pulse.phase, len) : 0.0;
    bool call = true;
    if (origin > 0.05)
        m.calls.push_back({ 0.0, origin });
    for (double a = origin; a < duration; a += len, call = !call)
        (call ? m.calls : m.answers).push_back({ a, std::min(duration, a + len) });
    return m;
}

std::string applyDialogue(Score& score, const DialogueMap& map)
{
    if (map.calls.empty() || map.answers.empty())
        return {};
    const auto inside = [](const std::vector<std::pair<double, double>>& v, double t) {
        for (const auto& p : v)
            if (t >= p.first && t < p.second)
                return &p;
        return static_cast<const std::pair<double, double>*>(nullptr);
    };
    const auto leads = [](int voice) { return voice == 0 || voice == 2 || voice == 3 || voice == 5; };
    const auto before = score.notes.size();
    score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                     [&](const Note& n) { return leads(n.voice) && inside(map.calls, n.time); }),
                      score.notes.end());
    const auto silenced = before - score.notes.size();
    for (auto& n : score.notes)
    {
        if (inside(map.calls, n.time))
            n.velocity *= 0.6f;  // o acompanhamento enquanto a referência fala
        else if (const auto* a = inside(map.answers, n.time))
        {
            if (leads(n.voice))
                n.velocity = std::min(1.0f, n.velocity * 1.1f);
            if (n.time + n.duration > a->second)
                n.duration = std::max(0.05, a->second - n.time);  // a resposta cede a vez
        }
    }
    double answerTime = 0.0;
    for (const auto& a : map.answers)
        answerTime += a.second - a.first;
    char line[256];
    std::snprintf(line, sizeof line,
                  "diálogo: %s · %zu pedidos da referência, %zu respostas (%.0f s) · %zu notas das vozes que conduzem calaram para escutar",
                  map.alternating ? "alternância de 2 compassos (a referência quase não pausa)" : "a resposta fala nas pausas da referência",
                  map.calls.size(), map.answers.size(), answerTime, silenced);
    return line;
}

std::string imitateCalls(Score& score, const DialogueMap& map, const std::vector<perception::Frame>& frames,
                         const std::vector<perception::Onset>& onsets, strategy::Mode mode)
{
    using strategy::Mode;
    if (map.calls.empty() || map.answers.empty())
        return {};
    int imitated = 0, notesIn = 0;
    for (const auto& ans : map.answers)
    {
        // o pedido que acabou logo antes desta resposta
        const std::pair<double, double>* call = nullptr;
        for (const auto& c : map.calls)
            if (c.second <= ans.first + 0.3 && (!call || c.second > call->second))
                call = &c;
        if (!call)
            continue;
        // os ataques do pedido com altura: (instante, MIDI)
        std::vector<std::pair<double, float>> gesture;
        for (const auto& o : onsets)
            if (o.time >= call->first && o.time < call->second)
            {
                const float p = perception::pitchAt(frames, o.time);
                if (!std::isnan(p) && p > 20.0f && p < 110.0f)
                    gesture.push_back({ o.time, p });
            }
        if (gesture.size() < 3)
            continue;
        // a cauda que cabe na resposta (90 % dela), no ritmo original
        const double room = 0.9 * (ans.second - ans.first);
        std::size_t first = gesture.size() - 1;
        while (first > 0 && gesture.back().first - gesture[first - 1].first <= room - 0.15)
            --first;
        std::vector<std::pair<double, float>> tail(gesture.begin() + static_cast<std::ptrdiff_t>(first), gesture.end());
        if (tail.size() < 2)
            continue;
        if (mode == Mode::interrupt)
            tail.resize(std::max<std::size_t>(2, tail.size() / 2));  // o fragmento: a primeira metade, e corta
        // o contexto da resposta ali: as classes de altura e o registro (motivo e harmonia em ±8 s)
        std::array<int, 12> pcs {};
        std::vector<float> reg;
        for (const auto& n : score.notes)
            if ((n.voice == 0 || n.voice == 6 || n.voice == 1) && std::abs(n.time - ans.first) <= 8.0)
            {
                ++pcs[static_cast<std::size_t>(((static_cast<int>(std::lround(n.pitch)) % 12) + 12) % 12)];
                if (n.voice == 0)
                    reg.push_back(n.pitch);
            }
        const bool hasScale = std::any_of(pcs.begin(), pcs.end(), [](int k) { return k > 0; });
        float center = 67.0f;
        if (!reg.empty())
        {
            std::nth_element(reg.begin(), reg.begin() + static_cast<std::ptrdiff_t>(reg.size() / 2), reg.end());
            center = reg[reg.size() / 2];
        }
        // o contorno em intervalos a partir da primeira nota, transformado pelo modo
        const float p0 = tail.front().second;
        std::vector<float> rel;
        for (const auto& g : tail)
            rel.push_back(g.second - p0);
        if (mode == Mode::transfigure || mode == Mode::counter)
            for (auto& r : rel)
                r = -r;  // a inversão
        std::vector<double> when;
        for (const auto& g : tail)
            when.push_back(g.first - tail.front().first);
        if (mode == Mode::counter)
        {
            std::reverse(rel.begin(), rel.end());  // o retrógrado (da inversão); o ritmo também
            const double span = when.back();
            std::reverse(when.begin(), when.end());
            for (auto& w : when)
                w = span - w;
        }
        // a altura de partida no registro da resposta, a oitava mais perto do centro
        float start = p0;
        while (start < center - 6.0f) start += 12.0f;
        while (start > center + 6.0f) start -= 12.0f;
        // um timbre da voz do motivo por perto (o da resposta)
        Timbre timbre = Timbre::additive;
        double best = 1e9;
        for (const auto& n : score.notes)
            if (n.voice == 0 && std::abs(n.time - ans.first) < best)
            {
                best = std::abs(n.time - ans.first);
                timbre = n.timbre;
            }
        // as notas do motivo que estavam na resposta dão lugar ao gesto
        score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                         [&](const Note& n) { return n.voice == 0 && n.time >= ans.first && n.time < ans.second; }),
                          score.notes.end());
        const double t0 = ans.first + 0.08;  // a resposta entra logo depois da pausa começar
        for (std::size_t k = 0; k < rel.size(); ++k)
        {
            float p = start + rel[k];
            if (hasScale)
            {
                // a classe de altura mais perto que a resposta usa ali
                int bestD = 99;
                float snapped = p;
                for (int d = -2; d <= 2; ++d)
                {
                    const int pc = ((static_cast<int>(std::lround(p)) + d) % 12 + 12) % 12;
                    if (pcs[static_cast<std::size_t>(pc)] > 0 && std::abs(d) < bestD)
                    {
                        bestD = std::abs(d);
                        snapped = static_cast<float>(std::lround(p) + d);
                    }
                }
                p = snapped;
            }
            Note n;
            n.time = t0 + when[k];
            const double next = k + 1 < rel.size() ? t0 + when[k + 1] : ans.second;
            n.duration = std::max(0.08, std::min(next - n.time, ans.second - n.time) * 0.92);
            n.pitch = p;
            n.velocity = 0.62f;
            n.voice = 0;
            n.timbre = timbre;
            if (n.time < ans.second)
            {
                score.notes.push_back(n);
                ++notesIn;
            }
        }
        ++imitated;
    }
    if (imitated == 0)
        return {};
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) { return a.time < b.time; });
    const char* how = mode == Mode::follow ? "imita o fim" : mode == Mode::transfigure ? "inverte o contorno do fim"
                    : mode == Mode::counter ? "toca o retrógrado da inversão do fim" : "cita um fragmento do fim, e corta,";
    char line[220];
    std::snprintf(line, sizeof line, "diálogo: em %d respostas a voz do motivo %s da frase que a referência acabou de tocar (%d notas)",
                  imitated, how, notesIn);
    return line;
}

DialogueSwap chooseDialogueSwap(const DialogueMap& map, double duration, strategy::Mode mode, std::uint64_t seed)
{
    DialogueSwap sw;
    if (map.calls.size() < 3 || map.answers.size() < 3)
        return sw;
    std::uint64_t s = seed * 0x9E3779B97F4A7C15ull + 0x5A0B;
    const auto next = [&] {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return static_cast<double>(s >> 11) / 9007199254740992.0;
    };
    // o pedido em que o trecho começa: o primeiro que começa depois de 35 % da peça
    std::size_t c0 = map.calls.size();
    for (std::size_t i = 0; i < map.calls.size(); ++i)
        if (map.calls[i].first >= 0.35 * duration)
        {
            c0 = i;
            break;
        }
    if (c0 >= map.calls.size())
        return sw;
    const int pairs = 2 + (next() < 0.5 ? 1 : 0);
    // o fim: o fim da resposta depois do pedido c0 + pairs − 1
    const double lastCallEnd = map.calls[std::min(map.calls.size() - 1, c0 + static_cast<std::size_t>(pairs) - 1)].second;
    double end = -1.0;
    for (const auto& a : map.answers)
        if (a.first >= lastCallEnd - 0.3)
        {
            end = a.second;
            break;
        }
    if (end <= map.calls[c0].first || end > 0.8 * duration)
        return sw;
    sw.from = map.calls[c0].first;
    sw.to = end;
    sw.kind = mode == strategy::Mode::interrupt ? 2 : (mode == strategy::Mode::follow && next() < 0.4) ? 2 : 1;
    return sw;
}

DialogueMap swapDialogue(const DialogueMap& map, const DialogueSwap& swap)
{
    if (swap.kind == 0)
        return map;
    const auto in = [&](const std::pair<double, double>& p) { return p.first >= swap.from - 1e-6 && p.second <= swap.to + 1e-6; };
    DialogueMap out;
    out.alternating = map.alternating;
    for (const auto& c : map.calls)
    {
        if (!in(c))
            out.calls.push_back(c);
        else if (swap.kind == 1)
            out.answers.push_back(c);  // a referência fala: agora é a vez da resposta
        // kind 2: o pedido some (as duas falam juntas)
    }
    for (const auto& a : map.answers)
    {
        if (in(a) && swap.kind == 1)
            out.calls.push_back(a);  // a pausa dela: agora a resposta escuta
        else
            out.answers.push_back(a);
    }
    const auto byStart = [](const std::pair<double, double>& x, const std::pair<double, double>& y) { return x.first < y.first; };
    std::sort(out.calls.begin(), out.calls.end(), byStart);
    std::sort(out.answers.begin(), out.answers.end(), byStart);
    return out;
}

std::string dialogueSwapText(const DialogueSwap& swap)
{
    if (swap.kind == 0)
        return {};
    char b[240];
    const auto mmss = [](double t, char* o) { std::snprintf(o, 16, "%d:%02d", static_cast<int>(t) / 60, static_cast<int>(t) % 60); };
    char f[16], t[16];
    mmss(swap.from, f);
    mmss(swap.to, t);
    std::snprintf(b, sizeof b, swap.kind == 1 ? "diálogo · troca de lugar de %s a %s: a resposta fala enquanto a referência fala e escuta as pausas dela"
                                              : "diálogo · sem vez de %s a %s (o atropelo): as duas falam ao mesmo tempo",
                  f, t);
    return b;
}

} // namespace trioio::composition

