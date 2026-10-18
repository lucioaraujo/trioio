#include "SomDaAura.h"
#include "Harmony.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>

namespace trioio::composition
{
namespace
{
int mod12(int x) { return ((x % 12) + 12) % 12; }

struct Rng
{
    std::uint64_t s;
    double next()
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return static_cast<double>(s >> 11) / 9007199254740992.0;
    }
};

// o timbre mais usado numa voz (para as notas novas herdarem o corpo dela)
Timbre timbreOf(const Score& s, int voice, Timbre fallback)
{
    std::map<int, int> c;
    for (const auto& n : s.notes)
        if (n.voice == voice)
            ++c[static_cast<int>(n.timbre)];
    if (c.empty())
        return fallback;
    return static_cast<Timbre>(std::max_element(c.begin(), c.end(), [](const auto& a, const auto& b) { return a.second < b.second; })->first);
}
} // namespace

std::string applySomDaAura(Score& score, const std::vector<perception::Frame>& frames, const std::vector<perception::Onset>& onsets,
                           double duration, std::uint64_t seed)
{
    if (frames.empty() || onsets.size() < 4)
        return {};
    Rng rng { seed * 0x9E3779B97F4A7C15ull + 0xA0BAull };
    // 1. as sílabas: de um ataque ao seguinte (no máximo 1,2 s), a altura
    //    mediana dos quadros com voz, o nível médio
    struct Syl { double t, d; float pitch; float level; };
    std::vector<Syl> syl;
    std::vector<double> starts;
    for (const auto& o : onsets)
        starts.push_back(o.time);
    std::sort(starts.begin(), starts.end());
    for (std::size_t i = 0; i < starts.size(); ++i)
    {
        const double a = starts[i], b = std::min({ i + 1 < starts.size() ? starts[i + 1] : duration, a + 1.2, duration });
        if (b - a < 0.06)
            continue;
        std::vector<float> ps;
        double lv = 0.0;
        int nl = 0;
        for (const auto& f : frames)
            if (f.time >= a && f.time < b && !f.silent)
            {
                const float hz = f.f0Hz > 0.0f && f.periodicity >= 0.5f ? f.f0Hz : f.melodyHz;
                if (hz > 50.0f && hz < 1200.0f)
                    ps.push_back(69.0f + 12.0f * std::log2(hz / 440.0f));
                lv += f.rmsDb;
                ++nl;
            }
        if (ps.size() < 2)
            continue;  // sem voz (a consoante, o ruído)
        std::nth_element(ps.begin(), ps.begin() + static_cast<long>(ps.size() / 2), ps.end());
        float p = std::round(ps[ps.size() / 2]);
        while (p < 55.0f) p += 12.0f;  // no registro de canto da melodia
        while (p > 84.0f) p -= 12.0f;
        const float level = std::clamp(static_cast<float>((lv / std::max(1, nl) + 50.0) / 40.0), 0.2f, 1.0f);
        syl.push_back({ a, (b - a) * 0.95, p, level });
    }
    if (syl.size() < 4)
        return {};
    const double t0 = syl.front().t, t1 = syl.back().t + syl.back().d;
    // 2. a melodia da fala no lugar do motivo, dobrada em uníssono
    const Timbre melodyT = timbreOf(score, 0, Timbre::additive), doubleT = timbreOf(score, 2, Timbre::breath);
    const Timbre bassT = timbreOf(score, 1, Timbre::additive), harmT = timbreOf(score, 6, Timbre::additive);
    score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                     [&](const Note& n) {
                                         return (n.voice == 0 || n.voice == 1 || n.voice == 2 || n.voice == 6) && n.time >= t0 - 0.05 && n.time < t1;
                                     }),
                      score.notes.end());
    // 3. cada nota ganha o seu acorde: ela é uma cor do acorde, e a
    //    fundamental é a que deixa o baixo andar mais perto
    struct Role { int below; ChordQuality q; std::uint8_t color; };
    static const std::array<Role, 6> roles { { { 2, ChordQuality::major, kMajorSeventh | kNinth },    // a 9ª
                                               { 6, ChordQuality::major, kMajorSeventh | kSharpEleventh },  // a ♯11
                                               { 9, ChordQuality::major, kSeventh | kThirteenth },   // a 13ª
                                               { 11, ChordQuality::major, kMajorSeventh },           // a 7M
                                               { 7, ChordQuality::major, kNinth },                   // a 5ª
                                               { 2, ChordQuality::minor, kSeventh | kNinth } } };   // a 9ª do menor
    int prevRoot = -1, chords = 0;
    for (const auto& s : syl)
    {
        Note m;
        m.time = s.t;
        m.duration = s.d;
        m.pitch = s.pitch;
        m.velocity = 0.35f + 0.5f * s.level;
        m.voice = 0;
        m.timbre = melodyT;
        score.notes.push_back(m);
        Note u = m;  // a dobra em uníssono, um pouco mais baixa
        u.voice = 2;
        u.timbre = doubleT;
        u.velocity *= 0.7f;
        score.notes.push_back(u);
        if (s.d < 0.15 && prevRoot >= 0)
            continue;  // a sílaba curta passa sobre o acorde anterior
        // a fundamental: a que anda menos (o semitom e a quarta ganham)
        double best = 1e9;
        const Role* role = &roles[0];
        int root = 0;
        for (const auto& r : roles)
        {
            const int cand = mod12(static_cast<int>(s.pitch) - r.below);
            double cost = rng.next() * 1.5;
            if (prevRoot >= 0)
            {
                const int d = std::min(mod12(cand - prevRoot), mod12(prevRoot - cand));
                cost += d == 1 ? 0.0 : d == 5 ? 0.3 : d == 0 ? 2.0 : static_cast<double>(d) * 0.5;
            }
            if (cost < best)
            {
                best = cost;
                role = &r;
                root = cand;
            }
        }
        prevRoot = root;
        const Chord c { root, role->q, role->color };
        // o baixo
        Note b;
        b.time = s.t;
        b.duration = s.d;
        b.pitch = 36.0f + static_cast<float>(root);
        if (b.pitch > 47.0f) b.pitch -= 12.0f;
        b.velocity = 0.55f;
        b.voice = 1;
        b.timbre = bassT;
        score.notes.push_back(b);
        // o acorde, cerrado abaixo da melodia (sem a fundamental, que está no baixo)
        const auto pcs = c.pitchClasses();
        float top = s.pitch - 1.0f;
        for (std::size_t k = 1; k < pcs.size() && k <= 4; ++k)
        {
            float p = top - static_cast<float>(mod12(static_cast<int>(top) - pcs[k]));
            if (p < 48.0f) p += 12.0f;
            Note h;
            h.time = s.t;
            h.duration = s.d;
            h.pitch = p;
            h.velocity = 0.3f;
            h.voice = 6;
            h.timbre = harmT;
            score.notes.push_back(h);
        }
        ++chords;
    }
    std::stable_sort(score.notes.begin(), score.notes.end(), [](const Note& a, const Note& b) { return a.time < b.time; });
    return "som da aura (Hermeto Pascoal): " + std::to_string(syl.size()) + " sílabas da referência viram melodia, dobrada em uníssono; "
           + std::to_string(chords) + " acordes, cada nota como cor (9ª, ♯11, 13ª, 7M, 5ª), o baixo andando por semitom e quarta";
}

} // namespace trioio::composition
