#include "Game.h"
#include "ScoreSynth.h"

#include <algorithm>
#include <array>
#include <complex>
#include <cstring>
#include <cmath>
#include <cstdio>

namespace trioio::composition
{
using perception::Frame;
using perception::Onset;
using perception::Summary;

namespace
{
constexpr int kMaxFirings = 3;  // o custo de uma regra: até 3 disparos por peça
constexpr double kAudibleSpectralDb = 2.0;  // a distância espectral audível: 4× o ruído do render idêntico (0,5 dB; GameTests)

std::uint64_t mixBits(std::uint64_t x)
{
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

struct Rng
{
    std::uint64_t s;
    double uniform()
    {
        s = mixBits(s);
        return static_cast<double>(s >> 11) / 9007199254740992.0;
    }
    int below(int n) { return std::min(n - 1, static_cast<int>(uniform() * n)); }
};

bool inMaterial(const Note& n, int material)
{
    switch (material)
    {
        case 1: return true;
        case 2: return n.voice == 4;
        case 3: return n.voice == 1;
        default: return n.voice == 0 || n.voice == 2 || n.voice == 3 || n.voice == 5;
    }
}

const char* materialName(int m)
{
    switch (m)
    {
        case 1: return "todas as vozes";
        case 2: return "a percussão";
        case 3: return "o baixo";
        default: return "as vozes que conduzem";
    }
}

float quantile(std::vector<float> v, float q)
{
    if (v.empty())
        return 0.0f;
    const auto k = static_cast<std::size_t>(std::clamp(q, 0.0f, 1.0f) * static_cast<float>(v.size() - 1));
    std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(k), v.end());
    return v[k];
}

// a classe de altura mais perto entre as que a resposta usa ali
float snapToContext(const Score& score, double t, float p)
{
    std::array<int, 12> pcs {};
    bool any = false;
    for (const auto& n : score.notes)
        if (n.voice != 4 && std::abs(n.time - t) <= 8.0)
        {
            ++pcs[static_cast<std::size_t>(((static_cast<int>(std::lround(n.pitch)) % 12) + 12) % 12)];
            any = true;
        }
    if (!any)
        return p;
    for (int d = 0; d <= 2; ++d)
        for (int sgn : { 1, -1 })
        {
            const int c = static_cast<int>(std::lround(p)) + sgn * d;
            if (pcs[static_cast<std::size_t>(((c % 12) + 12) % 12)] > 0)
                return static_cast<float>(c);
        }
    return p;
}
} // namespace

const char* gameActionName(GameAction a)
{
    switch (a)
    {
        case GameAction::imitate: return "imitar o que acabou de soar";
        case GameAction::transpose: return "repetir com diferença";
        case GameAction::delay: return "atrasar";
        case GameAction::hide: return "ocultar";
        case GameAction::interrupt: return "interromper";
        case GameAction::invert: return "inverter";
        case GameAction::densify: return "densificar";
        case GameAction::rarefy: return "rarefazer";
        case GameAction::sustain: return "sustentar";
        case GameAction::selfQuote: return "citar a si mesma, de trás para frente";
    }
    return "?";
}

GameAction oppositeAction(GameAction a)
{
    switch (a)
    {
        case GameAction::imitate: return GameAction::invert;
        case GameAction::invert: return GameAction::imitate;
        case GameAction::transpose: return GameAction::imitate;
        case GameAction::delay: return GameAction::interrupt;
        case GameAction::interrupt: return GameAction::sustain;
        case GameAction::sustain: return GameAction::interrupt;
        case GameAction::hide: return GameAction::densify;
        case GameAction::densify: return GameAction::rarefy;
        case GameAction::rarefy: return GameAction::densify;
        case GameAction::selfQuote: return GameAction::imitate;  // o passado distante contra o imediato
    }
    return a;
}

std::string gameRuleText(const GameRule& r)
{
    char b[420];
    const std::string when = r.when == 0 ? "sempre" : std::string("quando ") + conditionName(static_cast<Condition>(r.when));
    std::string range;
    if (r.from > 0.0 || r.to < 1e8)
    {
        char rr[64];
        if (r.to >= 1e8)
            std::snprintf(rr, sizeof rr, ", de %.0f s até o fim", r.from);
        else
            std::snprintf(rr, sizeof rr, ", de %.0f a %.0f s", r.from, r.to);
        range = rr;
    }
    std::snprintf(b, sizeof b, "jogo · regra #%d: %s, %s — %s, por %.1f s%s · %s%s%s", r.id, when.c_str(), gameActionName(r.action),
                  materialName(r.material), r.span, range.c_str(), r.origin.c_str(), r.why.empty() ? "" : " — ", r.why.c_str());
    return b;
}

std::vector<double> conditionMoments(const std::vector<Frame>& frames, const std::vector<Onset>& onsets, double duration, int condition)
{
    std::vector<double> out;
    const auto win = segment(frames, onsets, duration, 1.0);
    if (win.size() < 4)
        return out;
    std::vector<float> e, d, nz, con;
    for (const auto& w : win)
    {
        e.push_back(w.energy);
        d.push_back(w.density);
        nz.push_back(w.noisiness);
    }
    for (std::size_t i = 0; i + 1 < win.size(); ++i)
        con.push_back(contrast(win[i], win[i + 1]));
    const float eHi = quantile(e, 0.75f), eLo = quantile(e, 0.25f), dHi = quantile(d, 0.75f), dLo = quantile(d, 0.25f),
                nHi = quantile(nz, 0.75f), cHi = quantile(con, 0.8f), cLo = quantile(con, 0.2f);
    for (std::size_t i = 0; i < win.size(); ++i)
    {
        bool ok = false;
        const auto& w = win[i];
        switch (static_cast<Condition>(condition))
        {
            case Condition::none: ok = true; break;
            case Condition::energyHigh: ok = w.energy >= eHi && w.activeShare > 0.3f; break;
            case Condition::energyLow: ok = w.energy <= eLo; break;
            case Condition::energyRising: ok = i + 2 < win.size() && win[i + 2].energy - w.energy > 0.06f; break;
            case Condition::energyFalling: ok = i + 2 < win.size() && w.energy - win[i + 2].energy > 0.06f; break;
            case Condition::noisy: ok = w.noisiness >= nHi && w.activeShare > 0.3f; break;
            case Condition::dense: ok = w.density >= dHi; break;
            case Condition::sparse: ok = w.density <= dLo; break;
            case Condition::changeAhead: ok = i + 1 < win.size() && con[i] >= cHi; break;
            case Condition::stasis: ok = i + 2 < win.size() && con[i] <= cLo && con[i + 1] <= cLo; break;
            case Condition::strongChord: ok = false; break;  // pede os acordes; fora desta versão
        }
        if (ok)
            out.push_back(w.start);
    }
    return out;
}

namespace
{
// a FFT radix-2 (in place), para as bandas da distância espectral
void fft(std::vector<std::complex<double>>& a)
{
    const std::size_t n = a.size();
    for (std::size_t i = 1, j = 0; i < n; ++i)
    {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }
    for (std::size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2.0 * 3.14159265358979323846 / static_cast<double>(len);
        const std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (std::size_t i = 0; i < n; i += len)
        {
            std::complex<double> w(1.0, 0.0);
            for (std::size_t k = 0; k < len / 2; ++k)
            {
                const auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

// as energias em bandas de 1/6 de oitava, de 60 Hz a 8 kHz (dB), de um
// quadro de 2048 amostras com janela de Hann
std::vector<double> bandsDb(const std::vector<float>& L, const std::vector<float>& R, std::size_t at, double sr)
{
    constexpr std::size_t N = 2048;
    std::vector<std::complex<double>> x(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        const std::size_t j = at + i;
        const double s = j < L.size() ? 0.5 * (L[j] + R[j]) : 0.0;
        x[i] = s * (0.5 - 0.5 * std::cos(2.0 * 3.14159265358979323846 * static_cast<double>(i) / (N - 1)));
    }
    fft(x);
    std::vector<double> out;
    for (double lo = 60.0; lo < 8000.0; lo *= std::pow(2.0, 1.0 / 6.0))
    {
        const double hi = lo * std::pow(2.0, 1.0 / 6.0);
        const auto k0 = static_cast<std::size_t>(lo * N / sr), k1 = std::max(k0 + 1, static_cast<std::size_t>(hi * N / sr));
        double e = 0.0;
        for (std::size_t k = k0; k < k1 && k < N / 2; ++k)
            e += std::norm(x[k]);
        out.push_back(10.0 * std::log10(e + 1e-12));
    }
    return out;
}
} // namespace

SoundVerdict soundCheck(const Score& without, const Score& with, GameRule& rule, double sr)
{
    SoundVerdict v;
    // as janelas: 1 s antes e depois de cada disparo, unidas quando se tocam
    std::vector<std::pair<double, double>> win;
    for (double t : rule.firedAt)
    {
        const double a = std::max(0.0, t - 1.0), b = std::min(without.duration, t + rule.span + 1.0);
        if (b <= a)
            continue;
        if (!win.empty() && a <= win.back().second)
            win.back().second = std::max(win.back().second, b);
        else
            win.push_back({ a, b });
    }
    if (win.empty())
        return v;
    // a partitura de escuta: as janelas sem a regra, depois as mesmas com ela,
    // separadas por 1 s (e 2 s entre as metades)
    Score probe;
    probe.bpm = without.bpm;
    probe.room = without.room;
    probe.ensemble = without.ensemble;
    std::vector<double> offA, offB;
    double t0 = 0.0;
    for (const auto& w : win)
    {
        offA.push_back(t0);
        t0 += (w.second - w.first) + 1.0;
    }
    t0 += 1.0;
    for (const auto& w : win)
    {
        offB.push_back(t0);
        t0 += (w.second - w.first) + 1.0;
    }
    probe.duration = t0;
    const auto place = [&](const Score& s, const std::vector<double>& off) {
        for (std::size_t k = 0; k < win.size(); ++k)
        {
            const auto [a, b] = win[k];
            for (auto n : s.notes)
                if (n.time < b && n.time + n.duration > a)
                {
                    const double st = std::max(a, n.time);
                    n.duration = std::min(b, n.time + n.duration) - st;
                    n.time = off[k] + (st - a);
                    if (n.duration > 0.02)
                        probe.notes.push_back(n);
                }
            for (auto g : s.gestures)
                if (g.time < b && g.time + g.duration > a)
                {
                    const double st = std::max(a, g.time);
                    g.duration = std::min(b, g.time + g.duration) - st;
                    g.time = off[k] + (st - a);
                    if (g.duration > 0.05)
                        probe.gestures.push_back(g);
                }
            for (auto sp : s.spectra)
                if (sp.start < b && sp.end > a)
                {
                    sp.start = off[k] + std::max(0.0, sp.start - a);
                    sp.end = off[k] + std::min(b, sp.end) - a;
                    probe.spectra.push_back(sp);
                }
        }
    };
    place(without, offA);
    place(with, offB);
    std::vector<float> L, R;
    renderScore(probe, sr, L, R);
    // as medidas por quadro de 50 ms
    const auto hop = static_cast<std::size_t>(0.05 * sr);
    struct Fr { double db, bright; };
    const auto frames = [&](double from, double len) {
        std::vector<Fr> out;
        const auto s0 = static_cast<std::size_t>(from * sr);
        const auto n = static_cast<std::size_t>(len * sr);
        for (std::size_t i = s0; i + hop <= std::min(L.size(), s0 + n); i += hop)
        {
            double e = 0.0, d = 0.0;
            float prev = 0.5f * (L[i] + R[i]);
            for (std::size_t j = i; j < i + hop; ++j)
            {
                const float m = 0.5f * (L[j] + R[j]);
                e += static_cast<double>(m) * m;
                d += static_cast<double>(m - prev) * (m - prev);
                prev = m;
            }
            e /= static_cast<double>(hop);
            d /= static_cast<double>(hop);
            out.push_back({ 10.0 * std::log10(e + 1e-10), d / (e + 1e-10) });
        }
        return out;
    };
    // a distância espectral: a média, nos quadros com som, da diferença
    // absoluta em dB por banda (o chão de cada banda a 60 dB abaixo do máximo
    // do quadro): percebe a altura que muda sem mudar o nível
    double specSum = 0.0;
    int specN = 0;
    // só dentro dos disparos (o contexto em volta não muda e diluiria a medida)
    for (double fired : rule.firedAt)
    {
        std::size_t k = 0;
        while (k + 1 < win.size() && fired >= win[k + 1].first)
            ++k;
        const double len = win[k].second - win[k].first;
        const double from = fired - win[k].first, to = std::min(len, from + rule.span);
        for (double t = std::max(0.0, from); t + 0.1 < to; t += 0.1)
        {
            auto a = bandsDb(L, R, static_cast<std::size_t>((offA[k] + t) * sr), sr);
            auto b = bandsDb(L, R, static_cast<std::size_t>((offB[k] + t) * sr), sr);
            const double top = std::max(*std::max_element(a.begin(), a.end()), *std::max_element(b.begin(), b.end()));
            if (top < -40.0)
                continue;
            double d = 0.0;
            for (std::size_t i = 0; i < a.size(); ++i)
                d += std::abs(std::max(a[i], top - 60.0) - std::max(b[i], top - 60.0));
            specSum += d / static_cast<double>(a.size());
            ++specN;
        }
    }
    const double spectral = specN > 0 ? specSum / specN : 0.0;
    double sumDb = 0.0, sumAbsDb = 0.0, sumBright = 0.0, sumAbsBright = 0.0;
    int nFrames = 0, onA = 0, onB = 0;
    double peakA = -200.0, peakB = -200.0;
    double longestA = 0.0, longestB = 0.0;
    for (std::size_t k = 0; k < win.size(); ++k)
    {
        const double len = win[k].second - win[k].first;
        const auto fa = frames(offA[k], len), fb = frames(offB[k], len);
        const std::size_t n = std::min(fa.size(), fb.size());
        double runA = 0.0, runB = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            const double a = std::max(-60.0, fa[i].db), b = std::max(-60.0, fb[i].db);
            sumDb += b - a;
            sumAbsDb += std::abs(b - a);
            if (a > -50.0 && b > -50.0)
            {
                const double r = std::log((fb[i].bright + 1e-6) / (fa[i].bright + 1e-6));
                sumBright += r;
                sumAbsBright += std::abs(r);
            }
            ++nFrames;
            peakA = std::max(peakA, a);
            peakB = std::max(peakB, b);
            if (i > 0)
            {
                onA += fa[i].db - fa[i - 1].db >= 6.0 && fa[i].db > -45.0 ? 1 : 0;
                onB += fb[i].db - fb[i - 1].db >= 6.0 && fb[i].db > -45.0 ? 1 : 0;
            }
            runA = a < -50.0 ? runA + 0.05 : 0.0;
            runB = b < -50.0 ? runB + 0.05 : 0.0;
            longestA = std::max(longestA, runA);
            longestB = std::max(longestB, runB);
        }
    }
    if (nFrames == 0)
        return v;
    rule.soundMeasured = true;
    rule.soundDb = static_cast<float>(sumDb / nFrames);
    rule.soundBright = static_cast<float>(sumBright / nFrames);
    rule.soundOnsets = static_cast<float>(onB - onA);
    const double absDb = sumAbsDb / nFrames, absBright = sumAbsBright / nFrames;
    rule.soundSpectral = static_cast<float>(spectral);
    v.audible = absDb >= 1.0 || absBright >= 0.15 || std::abs(onB - onA) >= 2 || spectral >= kAudibleSpectralDb;
    const bool jump = peakB > peakA + 6.0;
    const bool hole = longestB > longestA + 1.5;
    v.coherent = !jump && !hole;
    char b[200];
    std::snprintf(b, sizeof b, "no som: %+.1f dB (|%.1f|), brilho %+.0f %%, %+d ataques, espectro %.1f dB%s%s", rule.soundDb, absDb,
                  100.0 * (std::exp(rule.soundBright) - 1.0), onB - onA, spectral, jump ? ", pico salta" : "", hole ? ", abre um buraco" : "");
    v.why = b;
    return v;
}

bool soundsLike(const GameRule& a, const GameRule& b)
{
    if (!a.soundMeasured || !b.soundMeasured)
        return false;
    const double x[3] = { a.soundDb, 5.0 * a.soundBright, a.soundOnsets / 3.0 };
    const double y[3] = { b.soundDb, 5.0 * b.soundBright, b.soundOnsets / 3.0 };
    double xy = 0.0, xx = 0.0, yy = 0.0;
    for (int i = 0; i < 3; ++i)
    {
        xy += x[i] * y[i];
        xx += x[i] * x[i];
        yy += y[i] * y[i];
    }
    if (xx < 1e-6 || yy < 1e-6)
        return false;
    const double cosine = xy / std::sqrt(xx * yy), ratio = std::sqrt(xx / yy);
    return cosine > 0.97 && ratio > 0.7 && ratio < 1.0 / 0.7;
}

bool ruleFromNote(const std::string& noteIn, GameRule& out)
{
    std::string note = " " + noteIn + " ";
    for (auto& ch : note)
        if (ch >= 'A' && ch <= 'Z')
            ch = static_cast<char>(ch - 'A' + 'a');
    // a palavra começa onde a anterior acabou (para "cala" não achar "escala")
    const auto wordAt = [](const std::string& text, const std::string& w) {
        for (std::size_t p = text.find(w); p != std::string::npos; p = text.find(w, p + 1))
        {
            const unsigned char before = p > 0 ? static_cast<unsigned char>(text[p - 1]) : ' ';
            if (!((before >= 'a' && before <= 'z') || before >= 0x80))
                return true;
        }
        return false;
    };
    const auto any = [&](const std::string& text, std::initializer_list<const char*> ws) {
        for (const char* w : ws)
            if (wordAt(text, w))
                return true;
        return false;
    };
    // a condição: o que vem depois de "quando", "sempre que" ou "se"
    std::string cond, act = note;
    for (const char* k : { " quando ", " sempre que ", " se " })
        if (const auto p = note.find(k); p != std::string::npos)
        {
            cond = note.substr(p + std::strlen(k));
            const auto comma = cond.find(',');
            if (comma != std::string::npos)
            {
                act = note.substr(0, p) + " " + cond.substr(comma + 1);
                cond = cond.substr(0, comma);
            }
            else
                act = note.substr(0, p);
            break;
        }
    struct A { GameAction a; std::initializer_list<const char*> ws; };
    const A actions[] = {
        { GameAction::interrupt, { "interromp", "corta", "corte", "cortar" } },
        { GameAction::hide, { "cala", "calar", "cale", "silenci", "silênci", "ocult", "some", "sumir" } },
        { GameAction::invert, { "invert", "ao contrário", "ao contrario", "avesso" } },
        { GameAction::densify, { "densific", "mais notas", "mais denso", "encher", "enche", "preench" } },
        { GameAction::rarefy, { "rarefa", "menos notas", "respir", "espaço", "espaco" } },
        { GameAction::sustain, { "sustent", "segura", "notas longas", "nota longa" } },
        { GameAction::delay, { "atras", "demor", "espera" } },
        { GameAction::selfQuote, { "cita", "citar", "lembra", "lembrar" } },
        { GameAction::transpose, { "repet", "varia", "transp" } },
        { GameAction::imitate, { "imit", "eco", "ecoa" } },
    };
    bool found = false;
    for (const auto& a : actions)
        if (any(act, a.ws))
        {
            out.action = a.a;
            found = true;
            break;
        }
    if (!found)
        return false;
    out.when = 0;
    if (!cond.empty())
    {
        struct C { Condition c; std::initializer_list<const char*> ws; };
        const C conds[] = {
            { Condition::energyRising, { "cresce", "sobe", "subindo", "crescendo" } },
            { Condition::energyFalling, { "cai", "caindo", "desce", "diminu" } },
            { Condition::energyHigh, { "forte", "intens", "clímax", "climax", "alto" } },
            { Condition::energyLow, { "fraco", "suave", "calmo", "piano" } },
            { Condition::noisy, { "ruído", "ruido", "barulh" } },
            { Condition::dense, { "denso", "cheio", "muitas notas" } },
            { Condition::sparse, { "rarefeit", "vazio", "silêncio", "silencio", "pouca" } },
            { Condition::changeAhead, { "muda", "mudança", "mudanca" } },
            { Condition::stasis, { "parad", "estátic", "estatic", "repetitiv" } },
        };
        for (const auto& c : conds)
            if (any(cond, c.ws))
            {
                out.when = static_cast<int>(c.c);
                break;
            }
    }
    out.material = any(note, { "baixo" }) ? 3 : any(note, { "percuss", "bateria", "tambor" }) ? 2 : any(note, { "tudo", "todas", "todos" }) ? 1 : 0;
    out.span = out.action == GameAction::interrupt ? 1.0 : 2.0;
    out.hidden = false;  // o autor sabe: a regra é dele
    return true;
}

GameState initialGameState(const Options& opt)
{
    GameState s;
    const double u = std::clamp(static_cast<double>(opt.unpredictability), 0.0, 1.0);
    s.surprise = 0.4 + 0.5 * u;
    s.coherence = opt.mode == strategy::Mode::follow ? 0.8 : opt.mode == strategy::Mode::interrupt ? 0.5 : 0.65;
    s.memory = std::min(1.0, static_cast<double>(opt.gameKept.size()) / 8.0);
    s.freedom = std::clamp(0.5 + 0.05 * (static_cast<double>(opt.gameKept.size()) - static_cast<double>(opt.gameCompost.size())), 0.0, 1.0);
    if (opt.hasGamePrevious)
    {
        // a sessão longa: metade vem de onde a anterior parou (a surpresa
        // gasta, o colapso que se aproximou, a energia); o oculto já foi
        // revelado na escuta, e a memória e a liberdade vêm dos julgamentos
        const auto& p = opt.gamePrevious;
        s.energy = 0.5 * (s.energy + p.energy);
        s.density = 0.5 * (s.density + p.density);
        s.coherence = 0.5 * (s.coherence + p.coherence);
        s.surprise = 0.5 * (s.surprise + p.surprise);
        s.collapse = 0.5 * (s.collapse + p.collapse);
    }
    return s;
}

std::string gameStateText(const GameState& s)
{
    char b[220];
    std::snprintf(b, sizeof b, "energia %.2f, densidade %.2f, memória %.2f, coerência %.2f, surpresa %.2f, oculto %.2f, colapso %.2f, liberdade %.2f",
                  s.energy, s.density, s.memory, s.coherence, s.surprise, s.hidden, s.collapse, s.freedom);
    return b;
}

std::vector<GameStateMoment> runGameState(const std::vector<GameRule>& rules, double duration, const GameState& start)
{
    // o que cada ação gasta (−) e ganha (+): energia, densidade, memória,
    // coerência, surpresa, colapso
    static const double kDelta[kGameActions][6] = {
        { 0.00, 0.00, 0.05, 0.10, -0.05, -0.05 },   // imitar
        { -0.05, 0.00, 0.00, -0.02, -0.10, 0.02 },  // repetir com diferença
        { 0.00, 0.00, 0.00, -0.05, -0.15, 0.03 },   // atrasar
        { 0.05, -0.10, 0.00, -0.05, -0.20, 0.05 },  // ocultar
        { -0.15, -0.15, 0.00, -0.20, -0.30, 0.20 }, // interromper
        { -0.05, 0.00, 0.00, -0.15, -0.25, 0.15 },  // inverter
        { -0.15, 0.20, 0.00, -0.05, -0.15, 0.10 },  // densificar
        { 0.10, -0.20, 0.00, 0.00, -0.10, -0.05 },  // rarefazer
        { 0.05, 0.00, 0.00, 0.10, -0.05, -0.10 },   // sustentar
        { -0.05, 0.00, -0.20, 0.05, -0.10, 0.00 },  // citar a si mesma
    };
    struct Ev { double t; const GameRule* r; };
    std::vector<Ev> evs;
    for (const auto& r : rules)
        for (double t : r.firedAt)
            evs.push_back({ t, &r });
    std::stable_sort(evs.begin(), evs.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });
    std::vector<GameStateMoment> out;
    GameState s = start;
    out.push_back({ 0.0, s, "início" });
    double now = 0.0;
    bool noSurprise = false, nearCollapse = false;
    const auto clamp01 = [](double& x) { x = std::clamp(x, 0.0, 1.0); };
    for (const auto& e : evs)
    {
        // o tempo devolve: a surpresa em ~200 s, a energia em ~100 s; o colapso se afasta
        const double dt = std::max(0.0, e.t - now);
        s.surprise += 0.005 * dt;
        s.energy += 0.01 * dt;
        s.collapse -= 0.004 * dt;
        s.coherence += 0.002 * dt;
        now = e.t;
        const auto& d = kDelta[static_cast<int>(e.r->action)];
        s.energy += d[0];
        s.density += d[1];
        s.memory += d[2];
        s.coherence += d[3];
        s.surprise += d[4];
        s.collapse += d[5];
        if (e.r->hidden)
        {
            s.hidden += 0.1;
            s.surprise -= 0.05;  // a regra oculta gasta surpresa
        }
        for (double* x : { &s.energy, &s.density, &s.memory, &s.coherence, &s.surprise, &s.hidden, &s.collapse, &s.freedom })
            clamp01(*x);
        out.push_back({ e.t, s, "regra #" + std::to_string(e.r->id) + ": " + gameActionName(e.r->action) });
        if (!noSurprise && s.surprise <= 0.05)
        {
            noSurprise = true;
            out.push_back({ e.t, s, "sem surpresa disponível" });
        }
        if (!nearCollapse && s.collapse >= 0.85)
        {
            nearCollapse = true;
            out.push_back({ e.t, s, "perto do colapso" });
        }
        if (noSurprise && s.surprise > 0.3)
            noSurprise = false;  // a surpresa voltou: pode esgotar de novo
        if (nearCollapse && s.collapse < 0.5)
            nearCollapse = false;
    }
    // no fim, a escuta revela o que estava oculto
    const double dt = std::max(0.0, duration - now);
    s.surprise = std::min(1.0, s.surprise + 0.005 * dt);
    s.energy = std::min(1.0, s.energy + 0.01 * dt);
    s.collapse = std::max(0.0, s.collapse - 0.004 * dt);
    out.push_back({ duration, s, s.hidden > 0.0 ? "fim (o oculto se revela na escuta)" : "fim" });
    return out;
}

int judgeGameRule(const GameRule& rule, const std::vector<Judgement>& judgements)
{
    int whole = -1;
    for (const auto& j : judgements)
        if (!j.hasSpan())
            whole = static_cast<int>(j.verdict);
    std::array<int, 3> votes {};
    int lastVote = -1;
    for (double t : rule.firedAt)
    {
        const double a = t, b = t + std::max(0.1, rule.span);
        int v = -1;
        for (const auto& j : judgements)
            if (j.hasSpan() && j.spanStart < b && j.spanEnd > a)
                v = static_cast<int>(j.verdict);  // o último que cobre o disparo
        if (v < 0)
            v = whole;
        if (v >= 0 && v < 3)
        {
            ++votes[static_cast<std::size_t>(v)];
            lastVote = v;
        }
    }
    if (lastVote < 0)
        return whole;  // sem disparos guardados (respostas antigas): a resposta inteira
    int best = lastVote;
    for (int v = 0; v < 3; ++v)
        if (votes[static_cast<std::size_t>(v)] > votes[static_cast<std::size_t>(best)])
            best = v;
    return best;
}

void applyGameRule(Score& score, GameRule& rule, const std::vector<double>& moments)
{
    rule.fired = rule.changed = 0;
    rule.firedAt.clear();
    // os momentos que podem disparar (no trecho da regra, sem se sobrepor)…
    std::vector<double> ok;
    double lastEnd = -1e9;
    for (double t : moments)
        if (t >= rule.from && t < rule.to && t >= lastEnd + 1.0 && t + 0.2 <= score.duration)
        {
            ok.push_back(t);
            lastEnd = t + rule.span;
        }
    // …e o custo: no máximo kMaxFirings disparos, espalhados pela peça (uma
    // regra que vale o tempo todo não toma a peça inteira)
    std::vector<double> fire;
    if (ok.size() <= static_cast<std::size_t>(kMaxFirings))
        fire = ok;
    else
        for (int k = 0; k < kMaxFirings; ++k)
            fire.push_back(ok[static_cast<std::size_t>(std::lround(static_cast<double>(k) * static_cast<double>(ok.size() - 1) / (kMaxFirings - 1)))]);
    for (double t : fire)
    {
        const double a = t, b = std::min(score.duration, t + rule.span);
        ++rule.fired;
        rule.firedAt.push_back(a);
        const auto inWin = [&](const Note& n) { return n.time >= a && n.time < b && inMaterial(n, rule.material); };
        int changed = 0;
        switch (rule.action)
        {
            case GameAction::hide:
            {
                const auto before = score.notes.size();
                score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(), inWin), score.notes.end());
                changed = static_cast<int>(before - score.notes.size());
                break;
            }
            case GameAction::interrupt:
            {
                // corta tudo (todas as vozes) por até 1 s: um corte, não um apagão
                const double cut = std::min(b, a + 1.0);
                const auto before = score.notes.size();
                score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                                 [&](const Note& n) { return n.time >= a && n.time < cut; }),
                                  score.notes.end());
                changed = static_cast<int>(before - score.notes.size());
                for (auto& n : score.notes)
                    if (n.time < a && n.time + n.duration > a)
                    {
                        n.duration = std::max(0.05, a - n.time);
                        ++changed;
                    }
                break;
            }
            case GameAction::imitate:
            case GameAction::transpose:
            {
                // o que soou antes no mesmo material volta agora (com diferença: um grau acima, na escala dali)
                std::vector<Note> echo;
                for (const auto& n : score.notes)
                    if (n.time >= a - rule.span && n.time < a && inMaterial(n, rule.material))
                        echo.push_back(n);
                if (echo.empty())
                    break;
                score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(), inWin), score.notes.end());
                for (auto n : echo)
                {
                    n.time += rule.span;
                    if (n.time >= b)
                        continue;
                    if (rule.action == GameAction::transpose && n.voice != 4)
                        n.pitch = snapToContext(score, n.time, n.pitch + 2.0f);
                    score.notes.push_back(n);
                    ++changed;
                }
                break;
            }
            case GameAction::delay:
            {
                const double shift = std::min(0.5, rule.span / 3.0);
                for (auto& n : score.notes)
                    if (inWin(n))
                    {
                        n.time += shift;
                        ++changed;
                    }
                break;
            }
            case GameAction::invert:
            {
                std::vector<float> ps;
                for (const auto& n : score.notes)
                    if (inWin(n) && n.voice != 4)
                        ps.push_back(n.pitch);
                if (ps.empty())
                    break;
                const float axis = quantile(ps, 0.5f);
                for (auto& n : score.notes)
                    if (inWin(n) && n.voice != 4)
                    {
                        n.pitch = snapToContext(score, n.time, 2.0f * axis - n.pitch);
                        ++changed;
                    }
                break;
            }
            case GameAction::densify:
            {
                std::vector<Note> add;
                for (const auto& n : score.notes)
                    if (inWin(n))
                    {
                        Note e = n;
                        e.time += 0.125;
                        e.velocity *= 0.7f;
                        if (e.voice != 4 && e.pitch + 12.0f <= 88.0f)
                            e.pitch += 12.0f;
                        add.push_back(e);
                    }
                for (auto& n : add)
                    score.notes.push_back(n);
                changed = static_cast<int>(add.size());
                break;
            }
            case GameAction::rarefy:
            {
                int k = 0;
                const auto before = score.notes.size();
                score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                                 [&](const Note& n) { return inWin(n) && (k++ % 2) == 1; }),
                                  score.notes.end());
                changed = static_cast<int>(before - score.notes.size());
                break;
            }
            case GameAction::sustain:
                for (auto& n : score.notes)
                    if (inWin(n) && n.time + n.duration < b)
                    {
                        n.duration = b - n.time;
                        ++changed;
                    }
                break;
            case GameAction::selfQuote:
            {
                // a resposta cita o PRÓPRIO passado (≥ 20 s antes), de trás para
                // frente: as notas dela, não o áudio de ninguém (a ideia do
                // "arquivo apócrifo" do Rasgo Synth Studio, reescrita aqui)
                const double src = a - 20.0 - rule.span;
                if (src < 0.0)
                    break;
                std::vector<Note> past;
                for (const auto& n : score.notes)
                    if (n.time >= src && n.time < src + rule.span && inMaterial(n, rule.material))
                        past.push_back(n);
                if (past.empty())
                    break;
                score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(), inWin), score.notes.end());
                for (auto n : past)
                {
                    const double u = n.time - src;
                    n.time = a + (rule.span - u - std::min(n.duration, rule.span - u));
                    n.time = std::clamp(n.time, a, b - 0.02);
                    n.velocity *= 0.85f;
                    score.notes.push_back(n);
                    ++changed;
                }
                break;
            }
        }
        rule.changed += changed;
    }
    std::sort(score.notes.begin(), score.notes.end(), [](const Note& x, const Note& y) { return x.time < y.time; });
}

std::vector<std::string> playGame(Score& score, const std::vector<Frame>& frames, const std::vector<Onset>& onsets, double duration,
                                  const Options& opt)
{
    std::vector<std::string> lines;
    if (!opt.game || duration < 10.0 || score.notes.empty())
        return lines;
    Rng rng { opt.seed ^ 0x6A6F676F5649544Full };

    // os hábitos da referência: quanto do tempo cada condição vale
    std::array<std::vector<double>, kConditions> moments;
    std::array<double, kConditions> share {};
    const double windows = std::max(1.0, std::floor(duration));
    for (int c = 0; c < kConditions; ++c)
    {
        moments[static_cast<std::size_t>(c)] = conditionMoments(frames, onsets, duration, c);
        share[static_cast<std::size_t>(c)] = static_cast<double>(moments[static_cast<std::size_t>(c)].size()) / windows;
    }
    const auto modeAction = [&](bool alt) {
        switch (opt.mode)
        {
            case strategy::Mode::follow: return alt ? GameAction::transpose : GameAction::imitate;
            case strategy::Mode::transfigure: return alt ? GameAction::invert : GameAction::transpose;
            case strategy::Mode::counter: return alt ? GameAction::rarefy : GameAction::hide;
            case strategy::Mode::interrupt: return alt ? GameAction::hide : GameAction::interrupt;
        }
        return GameAction::imitate;
    };
    const double beat = score.bpm > 0.0 ? 60.0 / score.bpm : 0.6;

    std::vector<GameRule> candidates;
    int nextId = 1;
    // 1. a regra LIDA: o hábito mais frequente (entre 12 % e 60 % do tempo)
    int habit = 0;
    for (int c = 1; c < kConditions; ++c)
        if (share[static_cast<std::size_t>(c)] >= 0.12 && share[static_cast<std::size_t>(c)] <= 0.6
            && (habit == 0 || share[static_cast<std::size_t>(c)] > share[static_cast<std::size_t>(habit)]))
            habit = c;
    if (habit != 0)
    {
        GameRule r;
        r.id = nextId++;
        r.when = habit;
        r.action = modeAction(false);
        r.span = std::clamp(4.0 * beat, 1.0, 4.0);
        r.origin = "lida na referência";
        char why[160];
        std::snprintf(why, sizeof why, "a referência fica \"%s\" em %.0f %% do tempo", conditionName(static_cast<Condition>(habit)),
                      100.0 * share[static_cast<std::size_t>(habit)]);
        r.why = why;
        candidates.push_back(r);
    }
    // 4. a regra NASCE DO AUTOR: a nota livre mais recente que diz uma ação
    //    (as regras do autor valem mais: entra antes das variações)
    for (auto it = opt.gameNotes.rbegin(); it != opt.gameNotes.rend(); ++it)
    {
        GameRule r;
        if (!ruleFromNote(*it, r))
            continue;
        r.id = nextId++;
        r.origin = "da nota do autor";
        std::string quote = *it;
        if (quote.size() > 70)
            quote = quote.substr(0, 67) + "...";
        r.why = "a nota: \"" + quote + "\"";
        candidates.push_back(r);
        break;
    }
    // a regra que ficou numa resposta aceita pode VOLTAR (a mais recente)
    if (!opt.gameKept.empty())
    {
        GameRule r = opt.gameKept.back();
        r.id = nextId++;
        r.from = 0.0;
        r.to = 1e9;
        r.origin = "volta";
        r.why = "ficou numa resposta aceita";
        candidates.push_back(r);
    }
    // 2. a regra VARIADA (da primeira): muda um campo
    if (!candidates.empty())
    {
        GameRule r = candidates.front();
        r.id = nextId++;
        const int field = rng.below(4);
        if (field == 0)
        {
            int c = 1 + rng.below(kConditions - 2);  // sem strongChord
            if (c == r.when)
                c = 1 + (c % (kConditions - 2));
            r.when = c;
        }
        else if (field == 1)
            r.action = rng.uniform() < 0.3 ? GameAction::selfQuote : modeAction(true);
        else if (field == 2)
            r.span = std::clamp(r.span * (rng.uniform() < 0.5 ? 0.5 : 2.0), 0.8, 6.0);
        else
            r.material = r.material == 0 ? 1 : 0;
        r.origin = "variada da #" + std::to_string(candidates.front().id);
        r.why.clear();
        candidates.push_back(r);
    }
    // 6. o ADUBO: a condição de uma regra apodrecida com a ação de outra
    if (opt.gameCompost.size() >= 2)
    {
        const auto& x = opt.gameCompost[static_cast<std::size_t>(rng.below(static_cast<int>(opt.gameCompost.size())))];
        const auto& y = opt.gameCompost[static_cast<std::size_t>(rng.below(static_cast<int>(opt.gameCompost.size())))];
        GameRule r;
        r.id = nextId++;
        r.when = x.when;
        r.action = y.action;
        r.material = y.material;
        r.span = 0.5 * (x.span + y.span);
        r.origin = "adubo";
        r.why = "restos de regras que apodreceram (rejeitadas)";
        candidates.push_back(r);
    }
    // 7. CONTRA SI MESMO — "confunde-te a ti mesmo": a ação contrária à da
    //    regra que ficou; senão, a primeira regra vira do avesso na 2.ª metade
    {
        GameRule base = !opt.gameKept.empty() ? opt.gameKept.back() : (candidates.empty() ? GameRule {} : candidates.front());
        if (!candidates.empty() || !opt.gameKept.empty())
        {
            GameRule r = base;
            r.id = nextId++;
            r.action = oppositeAction(base.action);
            r.origin = "contra si mesmo";
            if (opt.gameKept.empty())
            {
                r.from = 0.5 * duration;
                r.to = 1e9;
                r.why = "confunde-te a ti mesmo: a regra #" + std::to_string(base.id) + " vira do avesso na segunda metade";
                // a original passa a valer só na primeira metade
                for (auto& c : candidates)
                    if (c.id == base.id)
                        c.to = 0.5 * duration;
            }
            else
                r.why = "confunde-te a ti mesmo: contradiz a regra que ficou";
            candidates.push_back(r);
        }
    }

    // os critérios, medidos na partitura: cada candidata é jogada sobre o que
    // já está em jogo; só entra a que muda o que se ouve sem desfazer a peça
    const auto total = static_cast<int>(score.notes.size());
    const int budget = 2 + static_cast<int>(std::lround(2.0 * std::clamp(opt.unpredictability, 0.0f, 1.0f)));
    int accepted = 0, rejected = 0;
    std::vector<std::array<int, 3>> inPlay;  // (condição, ação, material) já em jogo
    // o estado decide (§5): perto do colapso, as regras que quebram ficam de
    // fora; sem surpresa para gastar, a regra é dita, não oculta
    const GameState opening = initialGameState(opt);
    for (auto& r : candidates)
    {
        if (accepted >= budget)
            break;
        const bool breaks = r.action == GameAction::interrupt || r.action == GameAction::invert || r.action == GameAction::densify;
        if (opening.collapse >= 0.5 && breaks && r.origin != "da nota do autor")
        {
            GameRule t = r;
            t.why = "quase entrou: o jogo está perto do colapso e esta regra quebra";
            score.nearGame.push_back(t);
            ++rejected;
            continue;
        }
        if (opening.surprise <= 0.15)
            r.hidden = false;
        const std::array<int, 3> key { r.when, static_cast<int>(r.action), r.material };
        if (r.origin != "volta" && std::find(inPlay.begin(), inPlay.end(), key) != inPlay.end())
        {
            GameRule t = r;
            t.why = "quase entrou: repetiria uma regra já em jogo";
            score.nearGame.push_back(t);
            ++rejected;  // não é nova
            continue;
        }
        Score trial = score;
        GameRule t = r;
        applyGameRule(trial, t, moments[static_cast<std::size_t>(std::max(0, r.when))]);
        bool audible = t.fired >= 1 && t.changed >= 4;
        bool coherent = t.changed <= static_cast<int>(0.35 * total) && t.fired * t.span <= 0.4 * duration;
        // os critérios no som: só para a que passou na partitura (o render custa)
        std::string heard;
        if (audible && coherent && opt.gameSoundCriteria)
        {
            const auto sv = soundCheck(score, trial, t);
            heard = sv.why;
            if (!sv.audible || !sv.coherent)
            {
                t.why = std::string(!sv.audible ? "quase entrou: não se ouviria no som" : "quase entrou: desfaria a peça no som") + " (" + sv.why
                      + ")";
                score.nearGame.push_back(t);
                ++rejected;
                continue;
            }
            bool repeats = false;
            for (const auto& g : score.game)
                if (soundsLike(t, g))
                {
                    t.why = "quase entrou: soaria como a regra #" + std::to_string(g.id) + " (" + sv.why + ")";
                    repeats = true;
                    break;
                }
            if (repeats)
            {
                score.nearGame.push_back(t);
                ++rejected;
                continue;
            }
        }
        if (!audible || !coherent)
        {
            // a leitura apócrifa: o caminho que a peça quase tomou fica anotado
            t.why = std::string(!audible ? "quase entrou: não se ouviria" : "quase entrou: tomaria a peça demais")
                  + (t.why.empty() ? "" : " (" + t.why + ")");
            score.nearGame.push_back(t);
            ++rejected;
            continue;
        }
        score = std::move(trial);
        r = t;
        score.game.push_back(r);
        inPlay.push_back(key);
        ++accepted;
        lines.push_back(gameRuleText(r) + " · disparou " + std::to_string(r.fired) + " vezes, mudou " + std::to_string(r.changed)
                        + " notas" + (heard.empty() ? "" : " · " + heard) + (r.hidden ? " · oculta até a escuta" : ""));
    }
    // 3. a regra NASCE DO JOGO (§4, caminho 3): o estado ao longo da peça; o
    //    primeiro recurso esgotado cria uma regra a partir dali ("sem surpresa
    //    disponível, só imitar"; "perto do colapso, sustentar"), que passa
    //    pelos mesmos critérios
    if (!score.game.empty())
    {
        const GameState start = initialGameState(opt);
        auto states = runGameState(score.game, duration, start);
        for (const auto& m : states)
        {
            const bool tired = m.event == "sem surpresa disponível", falling = m.event == "perto do colapso";
            if (!tired && !falling)
                continue;
            GameRule r;
            r.id = nextId++;
            r.when = 0;
            r.action = tired ? GameAction::imitate : GameAction::sustain;
            r.material = 0;
            r.span = std::clamp(2.0 * beat, 1.0, 3.0);
            r.from = m.time + 1.0;
            r.to = std::min(duration, m.time + 1.0 + 0.25 * duration);
            r.hidden = false;
            r.origin = "nasce do jogo";
            r.why = tired ? "sem surpresa disponível, só imitar" : "perto do colapso: proteger o equilíbrio";
            Score trial = score;
            GameRule t = r;
            applyGameRule(trial, t, moments[0]);
            bool audible = t.fired >= 1 && t.changed >= 4;
            bool coherent = t.changed <= static_cast<int>(0.35 * total) && t.fired * t.span <= 0.4 * duration;
            if (audible && coherent && opt.gameSoundCriteria)
            {
                const auto sv = soundCheck(score, trial, t);
                audible = sv.audible;
                coherent = sv.coherent;
            }
            if (audible && coherent)
            {
                score = std::move(trial);
                score.game.push_back(t);
                ++accepted;
                lines.push_back(gameRuleText(t) + " · disparou " + std::to_string(t.fired) + " vezes, mudou " + std::to_string(t.changed)
                                + " notas");
            }
            else
            {
                t.why = std::string("quase entrou: ") + (!audible ? "não se ouviria" : "tomaria a peça demais") + " (" + r.why + ")";
                score.nearGame.push_back(t);
                ++rejected;
            }
            break;  // uma regra nascida do jogo por resposta
        }
        // o estado final, com todas as regras (também a nascida do jogo)
        score.gameState = runGameState(score.game, duration, start);
        for (const auto& m : score.gameState)
            if (m.event == "sem surpresa disponível" || m.event == "perto do colapso" || m.event.rfind("fim", 0) == 0)
                score.marks.push_back({ m.time, "estado do jogo · " + m.event + " — " + gameStateText(m.state) });
        std::stable_sort(score.marks.begin(), score.marks.end(), [](const Mark& a, const Mark& b) { return a.time < b.time; });
    }
    // o jogo apócrifo: às vezes (mais com mais imprevisibilidade) uma regra
    // em jogo é revelada com uma origem inventada, claramente fictícia; a
    // origem verdadeira fica em trueOrigin/trueWhy (no decisoes.json)
    if (!score.game.empty() && rng.uniform() < 0.35 * std::clamp(static_cast<double>(opt.unpredictability), 0.0, 1.0) + 0.1)
    {
        static const char* attributions[] = {
            "encontrada no verso de uma caixa de fósforos",
            "ditada por um ácaro a uma lembrança de infância",
            "anotada à margem de um tratado que nunca existiu",
            "regra de um jogo que ainda não foi inventado",
            "copiada de um manuscrito perdido, com erros de cópia",
        };
        auto& r = score.game[static_cast<std::size_t>(rng.below(static_cast<int>(score.game.size())))];
        r.apocryphal = true;
        r.trueOrigin = r.origin;
        r.trueWhy = r.why;
        r.origin = "apócrifa";
        r.why = attributions[rng.below(5)];
        for (auto& l : lines)
            if (l.rfind("jogo · regra #" + std::to_string(r.id) + ":", 0) == 0)
                l = gameRuleText(r) + " · disparou " + std::to_string(r.fired) + " vezes, mudou " + std::to_string(r.changed) + " notas"
                  + (r.hidden ? " · oculta até a escuta" : "");
    }
    if (accepted > 0 || rejected > 0)
    {
        char head[200];
        std::snprintf(head, sizeof head, "jogo: %d regra(s) em jogo, %d descartada(s) pelos critérios (audível, coerente, nova)", accepted,
                      rejected);
        lines.insert(lines.begin(), head);
        for (const auto& n : score.nearGame)
            lines.push_back("jogo · leitura apócrifa (a regra que quase entrou) — " + gameRuleText(n).substr(std::string("jogo · ").size()));
    }
    return lines;
}

} // namespace trioio::composition
