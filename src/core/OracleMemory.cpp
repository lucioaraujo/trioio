#include "OracleMemory.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trioio::composition
{
using namespace trioio::perception;

int FactorOracle::transition(int state, int symbol) const
{
    const auto& t = trans_[static_cast<std::size_t>(state)];
    const auto it = t.find(symbol);
    return it == t.end() ? -1 : it->second;
}

// Construção incremental (Allauzen, Crochemore, Raffinot 1999): o novo
// estado m+1 recebe a transição de m; os estados no caminho de sufixos que
// ainda não leem o símbolo ganham uma transição "de fator" até m+1; o link
// de sufixo de m+1 é onde esse caminho encontra o símbolo (ou 0).
void FactorOracle::add(int symbol)
{
    const int m = size();
    symbols_.push_back(symbol);
    trans_.emplace_back();
    sfx_.push_back(0);
    trans_[static_cast<std::size_t>(m)][symbol] = m + 1;
    int k = sfx_[static_cast<std::size_t>(m)];
    while (k > -1 && transition(k, symbol) < 0)
    {
        trans_[static_cast<std::size_t>(k)][symbol] = m + 1;
        k = sfx_[static_cast<std::size_t>(k)];
    }
    sfx_[static_cast<std::size_t>(m + 1)] = k == -1 ? 0 : transition(k, symbol);
}

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

struct Event
{
    int pitch;       // MIDI inteiro: o símbolo do oráculo
    double time;     // o ataque na referência
    double ioi;      // até o próximo ataque
    float strength;  // 0..1
};
} // namespace

void oracleMemory(Score& score, const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                  const Options& opt)
{
    // regra "mínimo memória": garante passagens mesmo com pouca imprevisibilidade
    const int minimum = opt.constraints.minMemory;
    float u = std::clamp(opt.unpredictability, 0.0f, 1.0f);
    if (minimum > 0)
        u = std::max(u, 0.2f);
    if (u <= 0.0f || score.duration < 8.0)
        return;

    // 1. os eventos da referência: cada ataque com altura definida
    std::vector<Event> ev;
    float maxStrength = 1e-6f;
    for (std::size_t i = 0; i < onsets.size(); ++i)
    {
        const float p = pitchAt(frames, onsets[i].time + 0.04);
        if (!std::isfinite(p))
            continue;
        const double next = i + 1 < onsets.size() ? onsets[i + 1].time : onsets[i].time + 0.5;
        ev.push_back({ static_cast<int>(std::lround(p)), onsets[i].time, std::clamp(next - onsets[i].time, 0.08, 1.2), onsets[i].strength });
        maxStrength = std::max(maxStrength, onsets[i].strength);
    }
    if (ev.size() < 8)
        return;

    // 2. o oráculo aprende a sequência de alturas
    FactorOracle oracle;
    double mean = 0.0;
    for (const auto& e : ev)
    {
        oracle.add(e.pitch);
        mean += e.pitch;
    }
    mean /= static_cast<double>(ev.size());

    // 3. passagens de memória: mais e mais saltadas com a imprevisibilidade
    Rng rng { opt.seed * 0xA24BAED4963EE407ull + 0x0AC1E };
    const int passages = std::max(minimum, static_cast<int>(std::lround(1.0 + 3.0 * u)));
    const double continuity = 0.85 - 0.5 * u;  // u = 1: um salto a cada ~3 eventos
    const bool hasBody = !score.spectra.empty();
    double transpose = 0.0;
    if (opt.mode == strategy::Mode::transfigure)
        transpose = rng.uniform() < 0.5 ? 7.0 : -5.0;
    // o corpo da referência (espectral); proibido pelas regras, o aditivo;
    // proibidos os dois, a primeira família permitida
    Timbre memoryTimbre = hasBody ? Timbre::spectral : Timbre::additive;
    for (Timbre t : { memoryTimbre, Timbre::additive })
        if (!opt.constraints.noTimbre[static_cast<std::size_t>(t)])
        {
            memoryTimbre = t;
            break;
        }
    if (opt.constraints.noTimbre[static_cast<std::size_t>(memoryTimbre)])
        for (int k = 0; k < kTimbreCount; ++k)
            if (!opt.constraints.noTimbre[static_cast<std::size_t>(k)])
            {
                memoryTimbre = static_cast<Timbre>(k);
                break;
            }
    std::vector<std::pair<double, double>> used;
    for (int k = 0; k < passages; ++k)
    {
        const int length = 6 + static_cast<int>(rng.uniform() * 11.0);
        int jumps = 0;
        const int start = static_cast<int>(rng.uniform() * oracle.size());
        const auto path = oracle.navigate(start, length, continuity, [&] { return rng.uniform(); }, jumps);
        double span = 0.0;
        for (int s : path)
            span += ev[static_cast<std::size_t>(s - 1)].ioi;
        // um lugar livre na peça (até 8 tentativas), longe das outras passagens
        // com a regra "mínimo memória", insiste mais e, em último caso, aceita
        // ficar junto de outra passagem
        const bool required = k < minimum;
        double t0 = -1.0;
        for (int tries = 0; tries < (required ? 40 : 8) && t0 < 0.0; ++tries)
        {
            const double c = 0.08 * score.duration + rng.uniform() * (0.84 * score.duration - span);
            if (c < 0.0)
                break;
            if (std::none_of(used.begin(), used.end(), [&](const auto& r) { return c < r.second + 1.0 && c + span > r.first - 1.0; }))
                t0 = c;
        }
        if (t0 < 0.0 && required)
            t0 = std::max(0.0, std::min(0.08 * score.duration + rng.uniform() * 0.84 * score.duration,
                                        score.duration - span - 0.5));
        if (t0 < 0.0)
            continue;
        used.push_back({ t0, t0 + span });
        double t = t0;
        int written = 0;
        for (int s : path)
        {
            const auto& e = ev[static_cast<std::size_t>(s - 1)];
            double p = e.pitch + transpose;
            if (opt.mode == strategy::Mode::counter)
                p = 2.0 * mean - e.pitch;  // contrapor: a memória espelhada
            Note n;
            n.time = t;
            n.duration = e.ioi * 0.9;
            n.pitch = static_cast<float>(std::clamp(p, 36.0, 96.0));
            n.velocity = std::clamp(0.3f + 0.45f * e.strength / maxStrength, 0.2f, 0.85f);
            n.voice = 3;
            n.timbre = memoryTimbre;
            n.source = e.time;  // o corpo de onde veio
            if (t + n.duration < score.duration - 0.2)
            {
                score.notes.push_back(n);
                ++written;
            }
            t += e.ioi;
        }
        char text[200];
        std::snprintf(text, sizeof text, "memória (oráculo): %d notas da referência recombinadas, %d salto(s)%s",
                      written, jumps, opt.mode == strategy::Mode::counter ? ", espelhada" : "");
        score.marks.push_back({ t0, text });
    }
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) {
        return a.time < b.time || (a.time == b.time && a.voice < b.voice);
    });
    std::stable_sort(score.marks.begin(), score.marks.end(), [](const Mark& a, const Mark& b) { return a.time < b.time; });
}

} // namespace trioio::composition
