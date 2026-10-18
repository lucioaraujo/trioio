// Ritmo como campo (core/RhythmField.h, docs/RITMO.md): os golpes caem na
// grade e nos passos que a referência usa; o modo move a posição no campo;
// tempo livre e --ritmo nao não têm percussão; regras valem.

#include "Composer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

int main()
{
    // referência: 120 BPM, compasso de 4, golpes nos passos 0, 4, 8, 12 (no
    // tempo) e um fraco no 10 — quase sem síncope, densidade baixa
    const double step = 0.125;  // semicolcheia a 120 BPM
    std::vector<float> v(static_cast<std::size_t>(SR * 32.0));
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR;
        const int s = static_cast<int>(std::fmod(t, 2.0) / step);
        const double tt = std::fmod(t, step);
        const bool hit = s == 0 || s == 4 || s == 8 || s == 12 || s == 10;
        const double amp = (s == 10 ? 0.25 : 0.7) * (hit ? std::exp(-tt * 60.0) : 0.0);
        v[i] = static_cast<float>(amp * std::sin(2 * PI * 180.0 * t) + 0.05 * std::sin(2 * PI * 262 * t));
    }
    Analyzer a(SR);
    a.push(v.data(), v.size());
    a.finish();
    const auto compor = [&](Mode m, std::uint64_t seed, TimeMode tm = TimeMode::pulse, bool ritmo = true,
                            Constraints regras = {}) {
        Options o { m, seed, PitchSource::reference, tm, 0.3f };
        o.rhythm = ritmo;
        // o teste mede o campo rítmico: sem a dramaturgia, que desde
        // 2026-10-14 guia o olho pela tensão e muda os momentos dele — uma
        // regra oculta de tempo deformado (2:3) tira golpes da grade de
        // propósito, e o "tudo na grade" passava antes por sorte das sementes
        o.dramaturgy = false;
        o.constraints = std::move(regras);
        return compose(a.frames(), a.onsets(), 32.0, o);
    };
    struct Medida { int golpes = 0, naGrade = 0, nosPassosDaRef = 0, foraDoTempo = 0, compassos = 0; };
    const auto medir = [&](const Score& sc) {
        Medida m;
        for (const auto& n : sc.notes)
        {
            if (n.voice != 4)
                continue;
            ++m.golpes;
            const double k = n.time / step;
            const bool grade = std::abs(k - std::round(k)) < 0.15;
            m.naGrade += grade;
            const int s = static_cast<int>(std::lround(k)) % 16;
            m.nosPassosDaRef += s == 0 || s == 4 || s == 8 || s == 12 || s == 10;
            m.foraDoTempo += s % 4 != 0;
        }
        for (const auto& mk : sc.marks)
            if (mk.text.rfind("campo rítmico:", 0) == 0)
                m.compassos = std::stoi(mk.text.substr(std::string("campo rítmico: ").size()));
        return m;
    };

    // 1. seguir: golpes na grade e nos passos da referência
    Medida seguir, contrapor, transfigurar;
    for (std::uint64_t s = 1; s <= 6; ++s)
    {
        const auto f = medir(compor(Mode::follow, s)), c = medir(compor(Mode::counter, s)), t = medir(compor(Mode::transfigure, s));
        seguir.golpes += f.golpes; seguir.naGrade += f.naGrade; seguir.nosPassosDaRef += f.nosPassosDaRef; seguir.foraDoTempo += f.foraDoTempo; seguir.compassos += f.compassos;
        contrapor.golpes += c.golpes; contrapor.compassos += c.compassos;
        transfigurar.golpes += t.golpes; transfigurar.foraDoTempo += t.foraDoTempo;
    }
    std::printf("seguir: %d golpes em %d compassos, %d na grade, %d nos passos da referência, %d fora do tempo\n",
                seguir.golpes, seguir.compassos, seguir.naGrade, seguir.nosPassosDaRef, seguir.foraDoTempo);
    std::printf("contrapor: %d golpes · transfigurar: %d golpes, %d fora do tempo\n", contrapor.golpes,
                transfigurar.golpes, transfigurar.foraDoTempo);
    CHECK(seguir.golpes > 40 && seguir.compassos > 20);
    CHECK(seguir.naGrade == seguir.golpes);
    CHECK(seguir.nosPassosDaRef * 10 >= seguir.golpes * 9);
    // 2. contrapor: densidade complementar (a referência é rala → mais golpes)
    CHECK(contrapor.golpes > seguir.golpes);
    // 3. transfigurar: síncope espelhada → proporção maior fora do tempo
    CHECK(transfigurar.golpes > 0);
    CHECK(static_cast<double>(transfigurar.foraDoTempo) / transfigurar.golpes
          > 1.5 * static_cast<double>(seguir.foraDoTempo) / std::max(1, seguir.golpes));

    // 4. sem ritmo e em tempo livre: nenhuma percussão
    CHECK(medir(compor(Mode::follow, 1, TimeMode::pulse, false)).golpes == 0);
    CHECK(medir(compor(Mode::follow, 1, TimeMode::free)).golpes == 0);

    // 5. regras: tambor proibido → o grave usa outra família
    {
        std::vector<std::string> e;
        const auto r = parseConstraints("proibido timbre tambor\n", e);
        const auto sc = compor(Mode::follow, 2, TimeMode::pulse, true, r);
        int tambores = 0, golpes = 0;
        for (const auto& n : sc.notes)
            if (n.voice == 4)
            {
                ++golpes;
                tambores += n.timbre == Timbre::drum;
            }
        CHECK(e.empty() && golpes > 0 && tambores == 0);
    }

    // 6. --andamento metade: o pulso lido (120) vira 60, e a marca diz por quê
    {
        Options o { Mode::follow, 1, PitchSource::reference, TimeMode::pulse, 0.3f };
        o.tempoScale = 0.5;
        const auto sc = compose(a.frames(), a.onsets(), 32.0, o);
        bool metade = false;
        for (const auto& m : sc.marks)
            metade = metade || (m.text.find("pulso da referência: 60.0 BPM (metade do lido") != std::string::npos);
        CHECK(metade && std::abs(sc.bpm - 60.0) < 0.5);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("campo rítmico: todas as checagens passaram");
    return EXIT_SUCCESS;
}
