// O conjunto por estilo (src/core/Ensemble.h): cada voz recebe o instrumento
// do seu papel; a formação sem percussão tira a voz 4; as famílias proibidas
// pelo autor não entram; o automático não muda nada.
#include "Ensemble.h"
#include "StyleFigures.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <cstdio>
#include <cstdlib>

using namespace trioio::composition;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static Score partitura()
{
    Score s;
    s.duration = 10.0;
    // uma nota por voz (0..6) e duas de percussão (grave e aguda)
    for (int v = 0; v < 7; ++v)
        s.notes.push_back({ 1.0 + v, 0.5, 60.0f, 0.0f, 0.6f, v, Timbre::additive });
    s.notes.push_back({ 8.0, 0.2, 40.0f, 0.0f, 0.6f, 4, Timbre::drum });
    s.notes.push_back({ 8.5, 0.2, 84.0f, 0.0f, 0.6f, 4, Timbre::striker });
    return s;
}

static Timbre daVoz(const Score& s, int voice, float pitch = 60.0f)
{
    for (const auto& n : s.notes)
        if (n.voice == voice && n.pitch == pitch)
            return n.timbre;
    return static_cast<Timbre>(kTimbreCount);
}

int main()
{
    // 1. o automático não muda nada
    {
        auto s = partitura();
        CHECK(applyEnsemble(s, Ensemble::automatic, {}).empty());
        CHECK(daVoz(s, 0) == Timbre::additive);
    }
    // 2. o choro: flauta, 7 cordas, bandolim, cavaquinho, pandeiro
    {
        auto s = partitura();
        const auto line = applyEnsemble(s, Ensemble::choro, {});
        std::printf("%s\n", line.c_str());
        CHECK(line.rfind("conjunto: choro", 0) == 0);
        CHECK(daVoz(s, 0) == Timbre::transverseFlute);
        CHECK(daVoz(s, 1) == Timbre::sevenString);
        CHECK(daVoz(s, 2) == Timbre::bandolim);
        CHECK(daVoz(s, 6) == Timbre::cavaquinho);
        CHECK(daVoz(s, 4, 40.0f) == Timbre::pandeiro);
        CHECK(daVoz(s, 4, 84.0f) == Timbre::pandeiro);
    }
    // 3. o samba: surdo no grave, ganzá no agudo (≥ 80)
    {
        auto s = partitura();
        (void) applyEnsemble(s, Ensemble::samba, {});
        CHECK(daVoz(s, 4, 40.0f) == Timbre::surdo);
        CHECK(daVoz(s, 4, 84.0f) == Timbre::shaker);
    }
    // 4. a câmara: tudo arco, e a percussão sai
    {
        auto s = partitura();
        const std::size_t antes = s.notes.size();
        (void) applyEnsemble(s, Ensemble::chamber, {});
        int perc = 0, arco = 0;
        for (const auto& n : s.notes)
        {
            perc += n.voice == 4 ? 1 : 0;
            arco += n.timbre == Timbre::bowed ? 1 : 0;
        }
        CHECK(perc == 0);
        CHECK(s.notes.size() == antes - 3);
        CHECK(arco == static_cast<int>(s.notes.size()));
    }
    // 5. a família proibida pelo autor não entra (a voz fica como estava)
    {
        auto s = partitura();
        Constraints c;
        c.noTimbre[static_cast<std::size_t>(Timbre::transverseFlute)] = true;
        (void) applyEnsemble(s, Ensemble::choro, c);
        CHECK(daVoz(s, 0) == Timbre::additive);
        CHECK(daVoz(s, 1) == Timbre::sevenString);
    }
    // 6. os nomes
    {
        Ensemble e;
        CHECK(parseEnsemble("salsa", e) && e == Ensemble::salsa);
        CHECK(parseEnsemble("charanga", e) && e == Ensemble::charanga);
        CHECK(parseEnsemble("camara", e) && e == Ensemble::chamber);
        CHECK(parseEnsemble("coro", e) && e == Ensemble::acappella);
        CHECK(parseEnsemble("baião", e) && e == Ensemble::forro);
        CHECK(!parseEnsemble("polca", e));
    }
    // 7. as figuras do estilo: pulso de 0,5 s em 4/4, um acorde de Dó maior
    //    de 0 a 4 s (dois compassos) e o baixo Dó (0–2 s) e Fá (2–4 s)
    {
        trioio::perception::Pulse pulse;
        pulse.bpm = 120.0;
        pulse.period = 0.5;
        pulse.phase = 0.0;
        pulse.beatsPerBar = 4;
        const std::vector<std::pair<double, double>> spans { { 0.0, 4.0 } };
        const auto base = [] {
            Score s;
            s.duration = 4.0;
            for (float p : { 60.0f, 64.0f, 67.0f })
                s.notes.push_back({ 0.0, 4.0, p, 0.0f, 0.6f, 6, Timbre::guitar });
            s.notes.push_back({ 0.0, 2.0, 36.0f, 0.0f, 0.7f, 1, Timbre::upright });
            s.notes.push_back({ 2.0, 2.0, 41.0f, 0.0f, 0.7f, 1, Timbre::upright });
            return s;
        };
        const auto voz = [](const Score& s, int v) {
            std::vector<Note> out;
            for (const auto& n : s.notes)
                if (n.voice == v)
                    out.push_back(n);
            return out;
        };
        // a pompe: um acorde (3 notas) em cada tempo; 2 e 4 curtos
        {
            auto s = base();
            const auto line = applyStyleFigures(s, Ensemble::manouche, pulse, spans);
            std::printf("%s\n", line.c_str());
            const auto h = voz(s, 6);
            CHECK(h.size() == 8 * 4);  // o acorde de Django: Dó 6 (Dó Mi Sol Lá)
            bool curtos = true;
            for (const auto& n : h)
            {
                const int beat = static_cast<int>(std::lround(n.time / 0.5)) % 4;
                if (beat % 2 == 1)
                    curtos = curtos && n.duration < 0.1;
            }
            CHECK(curtos);
            // o baixo em dois: 1 e 3 de cada compasso
            CHECK(voz(s, 1).size() == 4);
        }
        // o jazz: a tríade de Dó vira Dó maj7(9) sem fundamental (Mi Si Ré Sol),
        // com a nota de baixo entre Ré 3 e Lá 3
        {
            auto s = base();
            (void) applyStyleFigures(s, Ensemble::jazz, pulse, spans);
            bool setima = false, semFundamental = true, registro = true;
            for (const auto& n : s.notes)
                if (n.voice == 6)
                {
                    const int pc = static_cast<int>(std::lround(n.pitch)) % 12;
                    setima = setima || pc == 11;
                    semFundamental = semFundamental && pc != 0;
                    registro = registro && n.pitch >= 50.0f && n.pitch <= 76.0f;
                }
            CHECK(setima && semFundamental && registro);
        }
        // a disposição cabe na faixa do estilo para TODA fundamental (as
        // doze), nos estilos com cor
        for (Ensemble e : { Ensemble::jazz, Ensemble::choro, Ensemble::bolero, Ensemble::tango, Ensemble::salsa })
            for (int r = 0; r < 12; ++r)
            {
                const auto v = styleVoicing(e, { 48.0f + r, 52.0f + r, 55.0f + r }, -1);
                const float low = *std::min_element(v.begin(), v.end());
                CHECK(low >= 48.0f && low <= 72.0f);
            }
        // walking: uma nota por tempo; o último tempo antes do Fá é a
        // aproximação cromática (Mi ou Fá♯)
        {
            auto s = base();
            (void) applyStyleFigures(s, Ensemble::jazz, pulse, spans);
            auto b = voz(s, 1);
            std::sort(b.begin(), b.end(), [](const Note& x, const Note& y) { return x.time < y.time; });
            CHECK(b.size() == 8);
            CHECK(b.size() >= 4 && b[0].pitch == 36.0f);
            CHECK(b.size() >= 4 && (b[3].pitch == 40.0f || b[3].pitch == 42.0f));
            CHECK(b.size() >= 5 && b[4].pitch == 41.0f);
        }
        // tumbao: o "2 e" (1,5 s… ) e o 4, nunca o 1
        {
            auto s = base();
            (void) applyStyleFigures(s, Ensemble::charanga, pulse, spans);
            bool semUm = true;
            for (const auto& n : voz(s, 1))
                semUm = semUm && std::abs(std::fmod(n.time, 2.0)) > 0.01;
            CHECK(semUm);
            CHECK(!voz(s, 1).empty());
        }
        // samba: a baixaria chega à próxima fundamental por grau (a última
        // nota da corrida a 1 ou 2 semitons do Fá)
        {
            Score s = base();
            s.notes.back().time = 4.0;  // o Fá mais tarde, para caber a corrida
            s.notes[3].duration = 4.0;
            s.duration = 6.0;
            const std::vector<std::pair<double, double>> sp { { 0.0, 6.0 } };
            const auto line = applyStyleFigures(s, Ensemble::samba, pulse, sp);
            std::printf("%s\n", line.c_str());
            CHECK(line.find("baixaria") != std::string::npos);
            auto b = voz(s, 1);
            std::sort(b.begin(), b.end(), [](const Note& x, const Note& y) { return x.time < y.time; });
            const Note* last = nullptr;
            for (const auto& n : b)
                if (n.time < 4.0)
                    last = &n;
            CHECK(last && std::abs(last->pitch - 41.0f) >= 1.0f && std::abs(last->pitch - 41.0f) <= 2.0f);
        }
        // a bateria de jazz: o prato no "2 e" cai na tercina (swing), o
        // chimbal no 2 e no 4
        {
            auto s = base();
            (void) applyStyleFigures(s, Ensemble::jazz, pulse, spans);
            bool swingOk = false, chimbal24 = true;
            int chimbais = 0;
            for (const auto& n : s.notes)
            {
                if (n.timbre == Timbre::ride && std::abs(n.time - (0.5 + 0.5 * 2.667 / 4.0)) < 0.002)
                    swingOk = true;  // o contratempo do 2: 0,5 + 2,667 semicolcheias
                if (n.timbre == Timbre::hihat)
                {
                    ++chimbais;
                    const int beat = static_cast<int>(std::lround(n.time / 0.5)) % 4;
                    chimbal24 = chimbal24 && (beat == 1 || beat == 3);
                }
            }
            CHECK(swingOk);
            CHECK(chimbais == 4 && chimbal24);
        }
        // o rock: power chords (fundamental, quinta, oitava), a caixa no 2 e
        // no 4, o prato de ataque na cabeça da frase, o baixo em colcheias
        {
            auto s = base();
            (void) applyStyleFigures(s, Ensemble::rock, pulse, spans);
            bool power = true, crash0 = false, caixa24 = true;
            int baixo = 0;
            for (const auto& n : s.notes)
            {
                if (n.voice == 6)
                    power = power && (n.pitch == 48.0f || n.pitch == 55.0f || n.pitch == 60.0f);
                if (n.timbre == Timbre::crash && n.time < 0.01)
                    crash0 = true;
                if (n.timbre == Timbre::snare)
                {
                    const int beat = static_cast<int>(std::lround(n.time / 0.5)) % 4;
                    caixa24 = caixa24 && (beat == 1 || beat == 3);
                }
                baixo += n.voice == 1 ? 1 : 0;
            }
            CHECK(power);
            CHECK(crash0);
            CHECK(caixa24);
            CHECK(baixo == 16);
        }
        // a escola de samba: o surdo de primeira (o mais grave, MIDI 33) no 2
        // de cada metade do compasso; a cuíca, o tamborim e o agogô presentes
        {
            auto s = base();
            (void) applyStyleFigures(s, Ensemble::escola, pulse, spans);
            bool primeira = true, cuica = false, tamborim = false, agogo = false;
            int primeiras = 0;
            for (const auto& n : s.notes)
            {
                if (n.timbre == Timbre::surdo && n.pitch == 33.0f)
                {
                    ++primeiras;
                    const int beat = static_cast<int>(std::lround(n.time / 0.5)) % 2;
                    primeira = primeira && beat == 1;
                }
                cuica = cuica || n.timbre == Timbre::cuica;
                tamborim = tamborim || n.timbre == Timbre::tamborim;
                agogo = agogo || n.timbre == Timbre::agogo;
            }
            CHECK(primeiras == 4 && primeira);
            CHECK(cuica && tamborim && agogo);
        }
        // a paradinha da escola: no 16º compasso da frase, a segunda metade
        // só tem a bossa em uníssono (três ataques: 8, 11 e 14); a volta tem os
        // três surdos juntos no 1
        {
            Score s;
            s.duration = 17 * 2.0;
            const std::vector<std::pair<double, double>> sp { { 0.0, 17 * 2.0 } };
            const auto line = applyStyleFigures(s, Ensemble::escola, pulse, sp);
            std::vector<double> tempos;
            for (const auto& n : s.notes)
                if (n.voice == 4 && n.time >= 15 * 2.0 + 1.0 - 1e-6 && n.time < 16 * 2.0 - 1e-6)
                    tempos.push_back(n.time);
            std::sort(tempos.begin(), tempos.end());
            tempos.erase(std::unique(tempos.begin(), tempos.end(), [](double x, double y) { return std::abs(x - y) < 1e-6; }), tempos.end());
            CHECK(tempos.size() == 3);
            int volta = 0;
            for (const auto& n : s.notes)
                if (n.timbre == Timbre::surdo && std::abs(n.time - 16 * 2.0) < 1e-6)
                    ++volta;
            CHECK(volta == 4);  // a segunda de sempre e os três da volta
        }
        // o forró: a maceta da zabumba no 1 e no "1 a" de cada metade; o
        // triângulo aberto no contratempo
        {
            auto s = base();
            (void) applyStyleFigures(s, Ensemble::forro, pulse, spans);
            int macetas = 0, abertos = 0;
            for (const auto& n : s.notes)
            {
                if (n.timbre == Timbre::zabumba && n.pitch < 55.0f)
                    ++macetas;
                if (n.timbre == Timbre::triangle && n.duration > 0.12)
                    ++abertos;
            }
            CHECK(macetas == 8);   // 2 por metade × 2 metades × 2 compassos
            CHECK(abertos == 8);
        }
        // o tango: o marcato (4 por compasso) e a síncopa 3-3-2 (3), sem percussão
        {
            auto s = base();
            (void) applyStyleFigures(s, Ensemble::tango, pulse, spans);
            int ataques = 0;
            for (const auto& n : s.notes)
                ataques += n.voice == 6 && n.pitch == 60.0f ? 1 : 0;
            CHECK(ataques == 7);
        }
        // o fraseado do motivo
        {
            const auto motivo = [] {
                Score s;
                s.duration = 4.0;
                // colcheias de 0 a 2 s (a 2ª, 4ª… no contratempo), depois uma
                // nota longa (Dó 4) e um salto de sexta colado (Lá 4)
                for (int k = 0; k < 8; ++k)
                    s.notes.push_back({ k * 0.25, 0.22, 60.0f + static_cast<float>(k % 3), 0.0f, 0.6f, 0, Timbre::sax });
                s.notes.push_back({ 2.0, 1.0, 60.0f, 0.0f, 0.6f, 0, Timbre::sax });
                s.notes.push_back({ 3.0, 1.0, 69.0f, 0.0f, 0.6f, 0, Timbre::sax });
                return s;
            };
            const std::vector<std::pair<double, double>> sp { { 0.0, 4.0 } };
            // jazz: as colcheias do contratempo vão para a tercina (+0,667 de semicolcheia)
            {
                auto s = motivo();
                const auto line = applyStylePhrasing(s, Ensemble::jazz, pulse, sp, 1);
                std::printf("%s\n", line.c_str());
                bool tercina = false;
                for (const auto& n : s.notes)
                    tercina = tercina || std::abs(n.time - (0.25 + 0.125 * 0.667)) < 1e-6;
                CHECK(tercina);
            }
            // choro: a nota longa antes do salto vira corrida que termina meio tom
            // ou um tom antes do Lá
            {
                auto s = motivo();
                const auto line = applyStylePhrasing(s, Ensemble::choro, pulse, sp, 3);
                std::printf("%s\n", line.c_str());
                CHECK(line.find("corridas") != std::string::npos);
            }
            // bolero: o portamento liga a nota longa à seguinte (a curva chega à
            // diferença de altura)
            {
                auto s = motivo();
                s.notes.back().pitch = 65.0f;  // uma quarta (o portamento liga até a quinta)
                (void) applyStylePhrasing(s, Ensemble::bolero, pulse, sp, 1);
                bool porta = false;
                for (const auto& n : s.notes)
                    if (n.time == 2.0 && !n.bend.empty())
                        porta = std::abs(n.bend.back().second - 5.0f) < 1e-6;
                CHECK(porta);
            }
            // samba: alguma nota de tempo forte antecipada em uma semicolcheia
            {
                bool antecipou = false;
                for (std::uint64_t sd = 1; sd < 6 && !antecipou; ++sd)
                {
                    auto s = motivo();
                    const auto line = applyStylePhrasing(s, Ensemble::samba, pulse, sp, sd);
                    antecipou = line.find("síncopes") != std::string::npos;
                }
                CHECK(antecipou);
            }
        }
        // câmara: sem figura
        {
            auto s = base();
            CHECK(applyStyleFigures(s, Ensemble::chamber, pulse, spans).empty());
            CHECK(voz(s, 6).size() == 3);
        }
        // sem pulso (fora dos trechos), nada muda
        {
            auto s = base();
            CHECK(applyStyleFigures(s, Ensemble::jazz, pulse, {}).empty());
        }
    }
    // 8. o conjunto lido: uma referência sem pulso, periódica (uma voz) →
    //    a cappella; transfigurar → câmara (o vizinho); contrapor → um dos
    //    enérgicos
    {
        std::vector<trioio::perception::Frame> fr;
        for (int i = 0; i < 400; ++i)
        {
            trioio::perception::Frame f;
            f.time = i * 0.025;
            f.silent = false;
            f.periodicity = 0.9f;
            f.rmsDb = -20.0f;
            fr.push_back(f);
        }
        trioio::perception::Pulse sem;
        const auto seguir = readEnsemble(fr, {}, 10.0, sem, {}, nullptr, trioio::strategy::Mode::follow, 1);
        std::printf("%s\n", seguir.why.c_str());
        CHECK(seguir.read == Ensemble::acappella && seguir.chosen == Ensemble::acappella);
        const auto transf = readEnsemble(fr, {}, 10.0, sem, {}, nullptr, trioio::strategy::Mode::transfigure, 1);
        CHECK(transf.chosen == Ensemble::chamber);
        const auto contra = readEnsemble(fr, {}, 10.0, sem, {}, nullptr, trioio::strategy::Mode::counter, 1);
        CHECK(contra.chosen == Ensemble::rock || contra.chosen == Ensemble::escola || contra.chosen == Ensemble::salsa
              || contra.chosen == Ensemble::samba || contra.chosen == Ensemble::forro);
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("conjunto: todas as checagens passaram");
    return EXIT_SUCCESS;
}
