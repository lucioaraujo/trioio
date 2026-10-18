// A escuta do autor (core/Listening.h, docs/ESCUTA.md): julgamentos viram
// tendências limitadas, que esquecem aos poucos e que de fato puxam os
// sorteios — sem alterar nada quando não há escuta.

#include "Composer.h"
#include "Listening.h"
#include "ThirdEye.h"

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

static void escuta(std::vector<Frame>& frames, std::vector<Onset>& onsets)
{
    std::vector<float> v(static_cast<std::size_t>(SR * 40.0));
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        const double t = static_cast<double>(i) / SR;
        v[i] = static_cast<float>(0.1 * std::sin(2 * PI * 262 * t) + (std::fmod(t, 0.5) < 0.015 ? 0.5 : 0.0));
    }
    Analyzer a(SR);
    a.push(v.data(), v.size());
    a.finish();
    frames = a.frames();
    onsets = a.onsets();
}

static Judgement julgar(Verdict v, std::vector<std::pair<int, double>> timbres, std::vector<int> eye, int room)
{
    Judgement j;
    j.verdict = v;
    j.timbres = std::move(timbres);
    j.eyeChoices = std::move(eye);
    j.room = room;
    return j;
}

int main()
{
    const auto fam = [](Timbre t) { return static_cast<int>(t); };
    const auto peso = [](const Tendencies& t, Timbre f) { return t.timbre[static_cast<std::size_t>(f)]; };

    // 1. sem escuta: tudo 1
    {
        const auto t = tendenciesFrom({});
        bool neutro = t.judgements == 0;
        for (float w : t.timbre) neutro = neutro && w == 1.0f;
        for (float w : t.eye) neutro = neutro && w == 1.0f;
        for (float w : t.room) neutro = neutro && w == 1.0f;
        CHECK(neutro);
    }

    // 2. aceitar puxa, rejeitar afasta, transformar puxa pouco; tudo em 0,5..2
    {
        const auto t = tendenciesFrom({ julgar(Verdict::accept, { { fam(Timbre::choir), 10 } }, { 3 }, 1),
                                        julgar(Verdict::reject, { { fam(Timbre::chip), 10 } }, { 5 }, 2),
                                        julgar(Verdict::transform, { { fam(Timbre::galo), 10 } }, {}, -1) });
        std::printf("coro ×%.2f · chip ×%.2f · galo ×%.2f · sala HALL ×%.2f · SPACE ×%.2f\n", peso(t, Timbre::choir),
                    peso(t, Timbre::chip), peso(t, Timbre::galo), t.room[1], t.room[2]);
        CHECK(peso(t, Timbre::choir) > 1.5f && peso(t, Timbre::chip) < 0.8f);
        CHECK(peso(t, Timbre::galo) > 1.0f && peso(t, Timbre::galo) < peso(t, Timbre::choir));
        CHECK(t.eye[3] > 1.0f && t.eye[5] < 1.0f && t.room[1] > 1.0f && t.room[2] < 1.0f);
        std::vector<Judgement> muitos(30, julgar(Verdict::accept, { { fam(Timbre::choir), 10 } }, {}, 1));
        const auto teto = tendenciesFrom(muitos);
        CHECK(peso(teto, Timbre::choir) <= 2.0f && teto.room[1] <= 2.0f);
        std::vector<Judgement> rej(30, julgar(Verdict::reject, { { fam(Timbre::choir), 10 } }, {}, 1));
        CHECK(peso(tendenciesFrom(rej), Timbre::choir) >= 0.5f);
    }

    // 3. esquecimento: um julgamento antigo pesa menos que um novo
    {
        std::vector<Judgement> h { julgar(Verdict::reject, { { fam(Timbre::vosim), 1 } }, {}, -1) };
        for (int i = 0; i < 8; ++i)
            h.push_back(julgar(Verdict::accept, { { fam(Timbre::flute), 1 } }, {}, -1));
        h.push_back(julgar(Verdict::reject, { { fam(Timbre::walsh), 1 } }, {}, -1));
        const auto t = tendenciesFrom(h);
        std::printf("rejeição antiga (VOSIM) ×%.2f · rejeição nova (Walsh) ×%.2f\n", peso(t, Timbre::vosim), peso(t, Timbre::walsh));
        CHECK(peso(t, Timbre::vosim) > peso(t, Timbre::walsh));
    }

    // 4. as marcas de forma são lidas: cada intervenção do olho e a sala
    std::vector<Frame> fr;
    std::vector<Onset> on;
    escuta(fr, on);
    {
        int marcas = 0, lidas = 0, salas = 0;
        for (std::uint64_t s = 1; s <= 12; ++s)
        {
            const auto sc = compose(fr, on, 40.0, Options { Mode::follow, s, PitchSource::reference, TimeMode::automatic, 1.0f });
            for (const auto& m : sc.marks)
            {
                if (m.text.find("Terceiro Olho") != std::string::npos)
                {
                    ++marcas;
                    lidas += eyeChoiceFromMark(m.text) >= 0;
                }
                salas += roomFromMark(m.text) >= 0;
            }
        }
        std::printf("marcas do olho lidas: %d de %d · salas lidas: %d de 12\n", lidas, marcas, salas);
        CHECK(marcas > 10 && lidas == marcas && salas == 12);
    }

    // 5. sem escuta a composição não muda; com escuta, a família aceita
    //    aparece mais (somado em 24 seeds)
    {
        Options a { Mode::follow, 7, PitchSource::reference, TimeMode::automatic, 0.5f };
        Options b = a;
        b.tendencies = tendenciesFrom({});
        const auto x = compose(fr, on, 40.0, a), y = compose(fr, on, 40.0, b);
        bool igual = x.notes.size() == y.notes.size() && x.marks.size() == y.marks.size() && x.room == y.room;
        for (std::size_t i = 0; igual && i < x.notes.size(); ++i)
            igual = x.notes[i].time == y.notes[i].time && x.notes[i].pitch == y.notes[i].pitch && x.notes[i].timbre == y.notes[i].timbre;
        CHECK(igual);

        Tendencies gosto;
        gosto.timbre[static_cast<std::size_t>(Timbre::spectral)] = 2.0f;
        gosto.room[static_cast<std::size_t>(Room::hall)] = 2.0f;
        int neutro = 0, comGosto = 0, hallN = 0, hallG = 0;
        for (std::uint64_t s = 1; s <= 24; ++s)
        {
            Options o { Mode::follow, s, PitchSource::reference, TimeMode::automatic, 0.0f };
            const auto n0 = compose(fr, on, 40.0, o);
            o.tendencies = gosto;
            const auto n1 = compose(fr, on, 40.0, o);
            for (const auto& n : n0.notes) neutro += n.voice == 0 && n.timbre == Timbre::spectral;
            for (const auto& n : n1.notes) comGosto += n.voice == 0 && n.timbre == Timbre::spectral;
            hallN += n0.room == Room::hall;
            hallG += n1.room == Room::hall;
        }
        std::printf("notas espectrais: %d sem escuta · %d com o espectral aceito · sala HALL: %d → %d\n",
                    neutro, comGosto, hallN, hallG);
        CHECK(comGosto > neutro);
        CHECK(hallG >= hallN);
    }

    // ---- julgamento por trecho (docs/ESCUTA.md) -------------------------
    const auto perto = [](double a, double b) { return std::abs(a - b) < 1e-3; };
    const std::string olho = std::string("Terceiro Olho · objeto sonoro: ") + gestureName(static_cast<GestureKind>(0)) + " por 2 s";
    const std::string sala = std::string("sala: ") + roomName(static_cast<Room>(1));
    // espectral de 0 a 12 s (três notas de 4 s), chip de 20 a 40 s
    const std::vector<NoteAt> notas { { 0.0, 4.0, fam(Timbre::spectral) }, { 4.0, 4.0, fam(Timbre::spectral) },
                                      { 8.0, 4.0, fam(Timbre::spectral) }, { 20.0, 20.0, fam(Timbre::chip) } };
    const std::vector<MarkAt> marcas { { 0.0, sala }, { 0.0, "seção 1 (0–10 s): A · exposição · pulso" },
                                       { 5.0, olho },
                                       { 10.0, "seção 2 (10–20 s): B · contraste · pulso" },
                                       { 20.0, "seção 3 (20–40 s): retorno de A (como na seção 1: exposição) · pulso" },
                                       { 25.0, olho } };

    // 7. compatibilidade: sem trecho, a idade continua um julgamento por vez,
    //    mesmo com a mesma derivação no mesmo minuto (o placar de hoje)
    {
        auto a = julgar(Verdict::reject, { { fam(Timbre::choir), 1 } }, {}, -1);
        auto b = julgar(Verdict::accept, { { fam(Timbre::choir), 1 } }, {}, -1);
        a.derivationId = b.derivationId = "der-x";
        a.when = b.when = "2026-10-10T02:00:00";
        const auto t = tendenciesFrom({ a, b });
        std::printf("inteiro, mesmo minuto: coro ×%.3f (esperado ×%.3f)\n", peso(t, Timbre::choir), std::exp(1.2 * 0.15));
        CHECK(perto(peso(t, Timbre::choir), std::exp(1.2 * 0.15)));
    }

    // 8. localidade: aceitar só o trecho espectral não puxa o chip
    {
        Judgement j;
        j.verdict = Verdict::accept;
        describeSpan(marcas, notas, 0.0, 10.0, j);
        bool soEspectral = !j.timbres.empty();
        for (const auto& [f, c] : j.timbres)
            soEspectral = soEspectral && f == fam(Timbre::spectral);
        const auto t = tendenciesFrom({ j });
        std::printf("trecho 0–10: espectral ×%.2f · chip ×%.2f · olho %zu\n", peso(t, Timbre::spectral), peso(t, Timbre::chip),
                    j.eyeChoices.size());
        CHECK(soEspectral);
        CHECK(peso(t, Timbre::spectral) > 1.5f && peso(t, Timbre::chip) == 1.0f);
        CHECK(j.eyeChoices.size() == 1);  // a marca dos 5 s entra; a dos 25 s não
    }

    // 9. fronteira: a nota de 8 a 12 s conta só a metade de dentro; e o
    //    trecho [0, fim] é exatamente a resposta inteira
    {
        Judgement j, tudo, inteiro;
        describeSpan(marcas, notas, 0.0, 10.0, j);
        CHECK(j.timbres.size() == 1 && perto(j.timbres[0].second, 2.5));  // 1 + 1 + 0,5
        describeSpan(marcas, notas, 0.0, responseEnd(notas), tudo);
        std::vector<std::string> textos;
        std::vector<int> timbres;
        for (const auto& m : marcas) textos.push_back(m.text);
        for (const auto& n : notas) timbres.push_back(n.timbre);
        describeResponse(textos, timbres, inteiro);
        bool igual = tudo.timbres.size() == inteiro.timbres.size() && tudo.eyeChoices == inteiro.eyeChoices
                  && tudo.room == inteiro.room && perto(tudo.roomWeight, 1.0);
        for (std::size_t k = 0; igual && k < tudo.timbres.size(); ++k)
            igual = tudo.timbres[k].first == inteiro.timbres[k].first && perto(tudo.timbres[k].second, inteiro.timbres[k].second);
        CHECK(igual);
    }

    // 10. sala: rejeitar o último quarto (30–40 de 40 s) pesa um quarto
    {
        Judgement j;
        j.verdict = Verdict::reject;
        describeSpan(marcas, notas, 30.0, 40.0, j);
        const auto t = tendenciesFrom({ j });
        std::printf("último quarto rejeitado: peso da sala %.2f · sala ×%.3f\n", j.roomWeight, t.room[1]);
        CHECK(perto(j.roomWeight, 0.25));
        CHECK(j.room == 1 && perto(t.room[1], std::exp(-1.2 * 0.25)));
    }

    // 11. mesmo ato: três trechos julgados juntos têm a mesma idade
    {
        const auto trecho = [&](Verdict v, double a, double b, const char* when) {
            Judgement j;
            j.verdict = v;
            j.derivationId = "der-y";
            j.when = when;
            describeSpan(marcas, notas, a, b, j);
            return j;
        };
        const auto junto = tendenciesFrom({ trecho(Verdict::transform, 20.0, 40.0, "2026-10-10T02:00:05"),
                                            trecho(Verdict::accept, 0.0, 4.0, "2026-10-10T02:00:30"),
                                            trecho(Verdict::accept, 4.0, 8.0, "2026-10-10T02:00:50") });
        const auto separado = tendenciesFrom({ trecho(Verdict::transform, 20.0, 40.0, "2026-10-10T01:00:00"),
                                               trecho(Verdict::accept, 0.0, 4.0, "2026-10-10T02:00:30"),
                                               trecho(Verdict::accept, 4.0, 8.0, "2026-10-10T02:00:50") });
        std::printf("chip transformado: no mesmo ato ×%.3f · uma hora antes ×%.3f\n", peso(junto, Timbre::chip),
                    peso(separado, Timbre::chip));
        CHECK(perto(peso(junto, Timbre::chip), std::exp(1.2 * 0.3)));
        CHECK(perto(peso(separado, Timbre::chip), std::exp(1.2 * 0.3 * 0.85)));
    }

    // 12. seção pelo número e pela letra; trecho por texto
    {
        const auto a = sectionSpans(marcas, 40.0, "A");
        const auto dois = sectionSpans(marcas, 40.0, "2");
        CHECK(a.size() == 2 && perto(a[0].first, 0.0) && perto(a[0].second, 10.0) && perto(a[1].first, 20.0)
              && perto(a[1].second, 40.0));
        CHECK(dois.size() == 1 && perto(dois[0].first, 10.0) && perto(dois[0].second, 20.0));
        CHECK(sectionSpans(marcas, 40.0, "C").empty() && sectionSpans(marcas, 40.0, "9").empty());
        double x = 0.0, y = 0.0;
        CHECK(parseSpan("0:10-fim", 40.0, x, y) && perto(x, 10.0) && perto(y, 40.0));
        CHECK(parseSpan("12.5-90", 40.0, x, y) && perto(x, 12.5) && perto(y, 40.0));
        CHECK(!parseSpan("1:41-fim", 40.0, x, y));  // começa depois do fim
        CHECK(!parseSpan("abc", 40.0, x, y) && !parseSpan("20-10", 40.0, x, y) && !parseSpan("0:75-1:00", 40.0, x, y));
        CHECK(perto(responseEnd(notas), 40.0));
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("escuta: todas as checagens passaram");
    return EXIT_SUCCESS;
}
