#include "Composer.h"
#include "ScoreSynth.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

using namespace trioio;
using namespace trioio::composition;
using namespace trioio::perception;
using Mode = trioio::strategy::Mode;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

constexpr double SR = 48000.0;
constexpr double PI = 3.14159265358979323846;
static double hz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

// referência: 120 BPM, célula 0 3 6 8 12 (cliques em Dó agudo), acorde de Dó
// maior por 16 s e de Lá menor por 16 s
struct Ref
{
    std::vector<float> v;
    double dur;
};

static Ref referencia(bool ruidoNoFim = false, double desafinado = 0.0)
{
    Ref r { std::vector<float>(static_cast<std::size_t>(SR * 32.0), 0.0f), 32.0 };
    const double per = 0.5, passo = per / 4.0, comp = per * 4.0, fase = 0.25;
    for (std::size_t i = 0; i < r.v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR;
        if (ruidoNoFim && t >= 16.0)
            continue;
        const auto acorde = t < 16.0 ? std::vector<double> { 60, 64, 67 } : std::vector<double> { 57, 60, 64 };
        for (const double m : acorde)
            r.v[i] += static_cast<float>(0.08 * std::sin(2.0 * PI * hz(m) * t));
        if (desafinado != 0.0)
            r.v[i] += static_cast<float>(0.08 * std::sin(2.0 * PI * hz(69.0 + desafinado) * t));
    }
    for (double b = fase; b + comp <= (ruidoNoFim ? 16.0 : 32.0); b += comp)
        for (const int k : { 0, 3, 6, 8, 12 })
        {
            const auto i0 = static_cast<std::size_t>((b + k * passo) * SR);
            for (std::size_t j = 0; j < 480 && i0 + j < r.v.size(); ++j)
                r.v[i0 + j] += static_cast<float>(0.7 * std::exp(-static_cast<double>(j) / 80.0) * std::sin(2.0 * PI * hz(96) * j / SR));
        }
    if (ruidoNoFim)  // ruído contínuo, sem pulso
    {
        std::uint32_t x = 4;
        for (auto i = static_cast<std::size_t>(16.0 * SR); i < r.v.size(); ++i)
        {
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            r.v[i] = static_cast<float>(0.15 * (x / 4294967295.0 * 2.0 - 1.0));
        }
    }
    return r;
}

struct Escuta
{
    std::vector<Frame> frames;
    std::vector<Onset> onsets;
    double dur;
};

static Escuta escutar(const Ref& r)
{
    Analyzer a(SR);
    a.push(r.v.data(), r.v.size());
    a.finish();
    return { a.frames(), a.onsets(), r.dur };
}

static Score compor(const Escuta& e, Mode modo, std::uint64_t seed, PitchSource p = PitchSource::reference,
                    TimeMode t = TimeMode::automatic)
{
    // imprevisibilidade 0: aqui se testa o compositor; o Terceiro Olho tem teste próprio
    return compose(e.frames, e.onsets, e.dur, Options { modo, seed, p, t, 0.0f });
}

static int pc(float pitch) { return ((static_cast<int>(std::lround(pitch)) % 12) + 12) % 12; }

int main()
{
    const auto ref = referencia();
    const auto e = escutar(ref);
    const auto pulso = estimatePulse(e.frames, e.onsets, 0.0, e.dur);
    std::printf("pulso: %.2f BPM, fase %.3f\n", pulso.bpm, pulso.phase);

    // 1. determinismo e seed
    {
        const auto a = compor(e, Mode::follow, 3), b = compor(e, Mode::follow, 3), c = compor(e, Mode::follow, 4);
        bool iguais = a.notes.size() == b.notes.size();
        for (std::size_t i = 0; iguais && i < a.notes.size(); ++i)
            iguais = a.notes[i].time == b.notes[i].time && a.notes[i].pitch == b.notes[i].pitch;
        CHECK(iguais);
        bool difere = a.notes.size() != c.notes.size();
        for (std::size_t i = 0; !difere && i < a.notes.size(); ++i)
            difere = a.notes[i].pitch != c.notes[i].pitch;
        CHECK(difere);
    }

    // 2. seguir: forma, campo e ritmo da referência
    const auto s = compor(e, Mode::follow, 1);
    for (const auto& m : s.marks)
        std::printf("  %5.1f s  %s\n", m.time, m.text.c_str());
    {
        bool apresenta = false, recapitula = false;
        for (const auto& m : s.marks)
        {
            apresenta = apresenta || m.text.find("apresentação") != std::string::npos;
            recapitula = recapitula || m.text.find("recapitulação") != std::string::npos;
        }
        CHECK(apresenta && recapitula);
        int dentro = 0, total = 0, naGrade = 0;
        const double passo = pulso.period / 4.0;
        for (const auto& n : s.notes)
        {
            if (n.voice != 0)
                continue;
            ++total;
            const std::set<int> campo = n.time < 15.5 ? std::set<int> { 0, 4, 7 } : std::set<int> { 9, 0, 4 };
            dentro += campo.count(pc(n.pitch)) > 0;
            const double k = (n.time - pulso.phase) / passo;
            naGrade += std::abs(k - std::round(k)) * passo < 0.015;
        }
        std::printf("seguir: %d notas do motivo, %d no campo, %d na grade\n", total, dentro, naGrade);
        CHECK(total > 40);
        CHECK(dentro >= total * 9 / 10);
        CHECK(naGrade >= total * 9 / 10);
        // o motivo da apresentação volta na recapitulação (mesmos intervalos)
        std::vector<float> ini, fim;
        double recap = s.marks.back().time;  // a marca da recapitulação, pelo nome
        for (const auto& m : s.marks)
            if (m.text.find("recapitulação") != std::string::npos)
                recap = m.time;
        for (const auto& n : s.notes)
            if (n.voice == 0)
            {
                if (n.time < recap && ini.size() < 5)
                    ini.push_back(n.pitch);
                else if (n.time >= recap && fim.size() < 5)
                    fim.push_back(n.pitch);
            }
        std::set<int> cIni, cFim;  // classes de intervalo (o campo muda, a direção fica)
        bool mesmaDirecao = ini.size() == 5 && fim.size() == 5;
        for (std::size_t i = 1; mesmaDirecao && i < 5; ++i)
            mesmaDirecao = (ini[i] > ini[i - 1]) == (fim[i] > fim[i - 1]) && (ini[i] == ini[i - 1]) == (fim[i] == fim[i - 1]);
        CHECK(mesmaDirecao);
        int baixos = 0;
        for (const auto& n : s.notes)
            baixos += n.voice == 1;
        CHECK(baixos >= 10);
    }

    // 3. contrapor: hoqueto — o motivo só ataca onde a referência silencia
    {
        const auto c = compor(e, Mode::counter, 1);
        const auto barras = rhythmGrid(e.onsets, pulso, 0.0, e.dur);
        int total = 0, livres = 0;
        const double passo = pulso.period / 4.0;
        for (const auto& n : c.notes)
        {
            if (n.voice != 0)
                continue;
            ++total;
            bool coincide = false;
            for (const auto& b : barras)
            {
                const auto k = static_cast<long>(std::lround((n.time - b.start) / passo));
                if (k >= 0 && k < b.steps && std::abs(n.time - (b.start + k * passo)) < 0.015)
                    coincide = b.hits[static_cast<std::size_t>(k)] > 0.0f;
            }
            livres += !coincide;
        }
        std::printf("contrapor: %d de %d notas fora dos ataques da referência\n", livres, total);
        CHECK(total > 20);
        CHECK(livres >= total * 95 / 100);
        // campo a um trítono: Fá#, Lá#, Dó# na primeira seção
        int tritono = 0, primeira = 0;
        for (const auto& n : c.notes)
            if (n.voice == 0 && n.time < 15.5)
            {
                ++primeira;
                tritono += std::set<int> { 6, 10, 1 }.count(pc(n.pitch)) > 0;
            }
        CHECK(tritono >= primeira * 9 / 10);
    }

    // 4. tempo livre: fora da grade, com respirações
    {
        const auto l = compor(e, Mode::follow, 1, PitchSource::reference, TimeMode::free);
        int fora = 0, total = 0;
        double maiorPausa = 0.0, fimAnterior = -1.0;
        const double passo = pulso.period / 4.0;
        for (const auto& n : l.notes)
        {
            if (n.voice != 0)
                continue;
            ++total;
            const double k = (n.time - pulso.phase) / passo;
            fora += std::abs(k - std::round(k)) * passo > 0.02;
            if (fimAnterior >= 0.0)
                maiorPausa = std::max(maiorPausa, n.time - fimAnterior);
            fimAnterior = std::max(fimAnterior, n.time + n.duration);
        }
        std::printf("livre: %d de %d notas fora da grade, maior pausa %.2f s\n", fora, total, maiorPausa);
        CHECK(fora >= total * 3 / 10);
        CHECK(maiorPausa >= 0.4);
    }

    // 5. auto: a seção sem pulso vira tempo livre
    {
        const auto e2 = escutar(referencia(true));
        const auto a = compor(e2, Mode::follow, 1);
        bool pulsoNaPrimeira = false, livreNaUltima = false;
        for (const auto& m : a.marks)
            std::printf("  auto  %5.1f s  %s\n", m.time, m.text.c_str());
        for (const auto& m : a.marks)
        {
            if (m.time < 1.0 && m.text.find("pulso ·") != std::string::npos)
                pulsoNaPrimeira = true;
            if (m.time >= 14.0 && m.text.find("tempo livre") != std::string::npos)
                livreNaUltima = true;
        }
        CHECK(pulsoNaPrimeira && livreNaUltima);
    }

    // 6. espectral e microtonal: alturas fora do temperamento, glissandos
    {
        const auto e3 = escutar(referencia(false, 0.4));  // um Lá 40 cents acima
        const auto sp = compor(e3, Mode::follow, 1, PitchSource::spectral);
        int fracionarias = 0;
        for (const auto& n : sp.notes)
            fracionarias += n.voice == 0 && std::abs(n.pitch - std::round(n.pitch)) > 0.1f;
        CHECK(fracionarias > 0);
        const auto mt = compor(e, Mode::follow, 1, PitchSource::microtonal);
        int glis = 0;
        for (const auto& n : mt.notes)
            glis += n.glide != 0.0f;
        CHECK(glis > 0);
    }

    // 7. a partitura soa: limites e precisão dos ataques tocados
    {
        std::vector<float> L, R;
        renderScore(s, SR, L, R);
        CHECK(L.size() == static_cast<std::size_t>(e.dur * SR + 0.5));
        bool finito = true;
        float pico = 0.0f;
        for (std::size_t i = 0; i < L.size(); ++i)
        {
            finito = finito && std::isfinite(L[i]) && std::isfinite(R[i]);
            pico = std::max({ pico, std::abs(L[i]), std::abs(R[i]) });
        }
        CHECK(finito && pico <= 0.9f && pico > 0.5f);
        std::vector<float> mono(L.size());
        for (std::size_t i = 0; i < L.size(); ++i)
            mono[i] = 0.5f * (L[i] + R[i]);
        Analyzer a(SR);
        a.push(mono.data(), mono.size());
        a.finish();
        int achadas = 0, total = 0;
        for (const auto& n : s.notes)
        {
            if (n.voice != 0)
                continue;
            ++total;
            for (const auto& o : a.onsets())
                if (std::abs(o.time - n.time) < 0.03)
                {
                    ++achadas;
                    break;
                }
        }
        std::printf("render: %d de %d notas ouvidas como ataque (±30 ms)\n", achadas, total);
        CHECK(achadas >= total * 6 / 10);
    }

    // 8. contorno da melodia real: Dó Mi Sol Mi Ré na célula 0 3 6 8 12
    {
        Ref r { std::vector<float>(static_cast<std::size_t>(SR * 24.0), 0.0f), 24.0 };
        const double per = 0.5, passo = per / 4.0, comp = per * 4.0;
        const int passos[] = { 0, 3, 6, 8, 12, 16 };
        const double notas[] = { 60, 64, 67, 64, 62 };
        for (double b = 0.25; b + comp <= 24.0; b += comp)
            for (int k = 0; k < 5; ++k)
            {
                const double t0 = b + passos[k] * passo, t1 = b + passos[k + 1] * passo;
                for (auto i = static_cast<std::size_t>(t0 * SR); i < static_cast<std::size_t>(t1 * SR) && i < r.v.size(); ++i)
                {
                    const double t = static_cast<double>(i) / SR - t0;
                    r.v[i] += static_cast<float>(0.3 * std::exp(-t * 3.0) * std::sin(2.0 * PI * hz(notas[k]) * t));
                }
            }
        const auto em = escutar(r);
        const auto sc = compor(em, Mode::follow, 1, PitchSource::reference, TimeMode::pulse);
        std::printf("%s\n", sc.marks.front().text.c_str());
        CHECK(sc.marks.front().text.find("da melodia da referência") != std::string::npos);
        std::vector<float> p5;
        for (const auto& n : sc.notes)
            if (n.voice == 0 && p5.size() < 5)
                p5.push_back(n.pitch);
        CHECK(p5.size() == 5);
        if (p5.size() == 5)
        {
            std::printf("motivo: %.0f %.0f %.0f %.0f %.0f\n", p5[0], p5[1], p5[2], p5[3], p5[4]);
            CHECK(p5[1] > p5[0] && p5[2] > p5[1] && p5[3] < p5[2] && p5[4] < p5[3]);
            // e as mesmas classes de altura da melodia: Dó Mi Sol Mi Ré
            CHECK(pc(p5[0]) == 0 && pc(p5[1]) == 4 && pc(p5[2]) == 7 && pc(p5[3]) == 4 && pc(p5[4]) == 2);
        }
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("composição: todas as checagens passaram");
    return EXIT_SUCCESS;
}
