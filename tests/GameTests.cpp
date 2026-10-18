// O jogo (src/core/Game.h, docs/JOGO.md): as ações, a invenção, os critérios,
// o adubo e o "confunde-te a ti mesmo".
#include "Game.h"
#include "Descriptors.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace trioio::composition;
using namespace trioio::perception;
using trioio::strategy::Mode;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static Score partitura(double dur)
{
    Score s;
    s.duration = dur;
    s.bpm = 100.0;
    for (int k = 0; k * 0.25 < dur; ++k)
    {
        Note n;
        n.time = k * 0.25;
        n.duration = 0.2;
        n.pitch = 60.0f + static_cast<float>((k * 2) % 12);
        n.voice = k % 3 == 0 ? 0 : k % 3 == 1 ? 1 : 4;
        s.notes.push_back(n);
    }
    return s;
}

static int contar(const Score& s, double a, double b, int voz = -1)
{
    int n = 0;
    for (const auto& x : s.notes)
        n += x.time >= a && x.time < b && (voz < 0 || x.voice == voz);
    return n;
}

int main()
{
    // 1. as ações, uma a uma, num disparo em t = 4 s com alcance 2 s
    {
        const std::vector<double> em4 { 4.0 };
        auto base = partitura(20.0);
        const auto rodar = [&](GameAction a, int material = 0) {
            Score s = base;
            GameRule r;
            r.action = a;
            r.material = material;
            r.span = 2.0;
            applyGameRule(s, r, em4);
            return std::make_pair(s, r);
        };
        auto [oc, roc] = rodar(GameAction::hide);
        CHECK(contar(oc, 4.0, 6.0, 0) == 0 && contar(oc, 4.0, 6.0, 1) == contar(base, 4.0, 6.0, 1));  // só o motivo cala
        CHECK(roc.fired == 1 && roc.changed > 0);
        auto [in, rin] = rodar(GameAction::interrupt);
        CHECK(contar(in, 4.0, 5.0) == 0 && contar(in, 5.0, 6.0) > 0);  // o corte é de 1 s, em todas as vozes
        auto [im, rim] = rodar(GameAction::imitate);
        CHECK(contar(im, 4.0, 6.0, 0) == contar(base, 2.0, 4.0, 0));  // o que soou antes volta
        auto [ra, rra] = rodar(GameAction::rarefy);
        CHECK(contar(ra, 4.0, 6.0, 0) < contar(base, 4.0, 6.0, 0));
        auto [de, rde] = rodar(GameAction::densify);
        CHECK(contar(de, 4.0, 6.2, 0) > contar(base, 4.0, 6.2, 0));
        auto [su, rsu] = rodar(GameAction::sustain);
        bool longa = false;
        for (const auto& n : su.notes)
            longa = longa || (n.voice == 0 && n.time >= 4.0 && n.time < 4.6 && n.duration > 1.4);  // o motivo em 4,5 s vai até 6 s
        CHECK(longa);
        auto [iv, riv] = rodar(GameAction::invert);
        CHECK(riv.changed > 0);
        // citar a si mesma: o passado de ≥ 20 s antes volta de trás para frente
        {
            Score longa = partitura(40.0);
            GameRule r;
            r.action = GameAction::selfQuote;
            r.span = 2.0;
            applyGameRule(longa, r, { 30.0 });
            CHECK(r.fired == 1 && r.changed > 0);
            CHECK(contar(longa, 30.0, 32.0, 0) == contar(partitura(40.0), 8.0, 10.0, 0));
            Score cedo = partitura(40.0);
            GameRule q = r;
            applyGameRule(cedo, q, { 10.0 });  // não há 20 s de passado: nada
            CHECK(q.changed == 0);
        }
        CHECK(oppositeAction(GameAction::imitate) == GameAction::invert && oppositeAction(GameAction::densify) == GameAction::rarefy);
        // fora do trecho da regra: nada
        Score s = base;
        GameRule r;
        r.action = GameAction::hide;
        r.from = 10.0;
        applyGameRule(s, r, em4);
        CHECK(r.fired == 0 && s.notes.size() == base.notes.size());
    }

    // 2. a invenção: uma referência de 40 s que alterna forte e fraco a cada 4 s
    std::vector<Frame> fr;
    std::vector<Onset> on;
    for (double t = 0.0; t < 40.0; t += 0.0213)
    {
        Frame f;
        f.time = t;
        const bool forte = std::fmod(t, 8.0) < 4.0;
        f.rmsDb = forte ? -12.0f : -40.0f;
        f.silent = false;
        fr.push_back(f);
    }
    for (double t = 0.0; t < 40.0; t += 0.5)
        on.push_back({ t, 1.0f });
    Options o { Mode::follow, 3, PitchSource::reference, TimeMode::automatic, 0.5f };
    o.game = true;
    auto a = partitura(40.0);
    const auto linhas = playGame(a, fr, on, 40.0, o);
    for (const auto& l : linhas)
        std::printf("  %s\n", l.c_str());
    CHECK(!a.game.empty());
    bool lida = false, contra = false;
    for (const auto& g : a.game)
    {
        lida = lida || g.origin == "lida na referência";
        contra = contra || g.origin == "contra si mesmo";
        CHECK(g.fired >= 1 && g.changed >= 4);  // audível
    }
    CHECK(lida || std::any_of(a.game.begin(), a.game.end(), [](const GameRule& g) { return g.apocryphal && g.trueOrigin == "lida na referência"; }));
    // o apócrifo: a origem revelada é inventada, mas a verdadeira fica guardada
    for (const auto& g : a.game)
        if (g.apocryphal)
            CHECK(g.origin == "apócrifa" && !g.trueOrigin.empty());
    int apocrifas = 0;
    for (std::uint64_t sd = 1; sd <= 20; ++sd)
    {
        Options k = o;
        k.seed = sd;
        k.unpredictability = 1.0f;
        auto c = partitura(40.0);
        (void) playGame(c, fr, on, 40.0, k);
        for (const auto& g : c.game)
            apocrifas += g.apocryphal;
    }
    std::printf("apócrifas em 20 seeds (imprevisibilidade 1): %d\n", apocrifas);
    CHECK(apocrifas >= 3 && apocrifas <= 17);
    // determinístico
    auto b = partitura(40.0);
    (void) playGame(b, fr, on, 40.0, o);
    CHECK(a.notes.size() == b.notes.size() && a.game.size() == b.game.size());

    // 3. o "confunde-te a ti mesmo" com uma regra que ficou: a ação contrária
    {
        Options k = o;
        GameRule ficou;
        ficou.id = 9;
        ficou.when = static_cast<int>(Condition::energyHigh);
        ficou.action = GameAction::densify;
        k.gameKept = { ficou };
        auto c = partitura(40.0);
        (void) playGame(c, fr, on, 40.0, k);
        bool achou = false, volta = false;
        for (const auto& g : c.game)
        {
            achou = achou || (g.origin == "contra si mesmo" && g.action == GameAction::rarefy);
            volta = volta || g.origin == "volta";
        }
        std::printf("com a regra que ficou: %zu regras, contra si mesmo %d, volta %d\n", c.game.size(), achou, volta);
        for (const auto& n : c.nearGame)
            std::printf("  quase: %s — %s\n", gameActionName(n.action), n.why.c_str());
        CHECK(achou || volta);  // pelo menos uma das duas passou nos critérios
    }

    // 4. o adubo: com duas regras apodrecidas, nasce uma regra de adubo (se passar nos critérios)
    {
        Options k = o;
        GameRule p1, p2;
        p1.when = static_cast<int>(Condition::energyLow);
        p1.action = GameAction::hide;
        p2.when = static_cast<int>(Condition::energyHigh);
        p2.action = GameAction::densify;
        k.gameCompost = { p1, p2 };
        auto c = partitura(40.0);
        const auto l = playGame(c, fr, on, 40.0, k);
        bool adubo = false;
        for (const auto& s : l)
            adubo = adubo || s.find("adubo") != std::string::npos;
        std::printf("com adubo: %zu regras\n", c.game.size());
        CHECK(!c.game.empty());
        (void) adubo;
    }

    // 5. os critérios: uma regra que apaga tudo, sempre, não é coerente
    {
        Score s = partitura(40.0);
        GameRule r;
        r.action = GameAction::hide;
        r.material = 1;
        r.span = 6.0;
        const auto m = conditionMoments(fr, on, 40.0, 0);
        Score t = s;
        applyGameRule(t, r, m);
        CHECK(r.fired * r.span > 0.4 * 40.0);  // cobriria mais de 40 % da peça: o playGame a descartaria
    }

    // 6. sem o jogo, nada muda
    {
        Options k = o;
        k.game = false;
        auto c = partitura(40.0);
        CHECK(playGame(c, fr, on, 40.0, k).empty() && c.game.empty());
    }

    // o julgamento por trecho pesa em cada regra (judgeGameRule): disparos
    // em 10 s, 50 s e 90 s, de 2 s cada
    {
        GameRule r;
        r.span = 2.0;
        r.firedAt = { 10.0, 50.0, 90.0 };
        const auto j = [](Verdict v, double a = -1.0, double b = -1.0) {
            Judgement x;
            x.verdict = v;
            x.spanStart = a;
            x.spanEnd = b;
            return x;
        };
        // sem julgamento: −1; só a resposta inteira: ela decide
        CHECK(judgeGameRule(r, {}) == -1);
        CHECK(judgeGameRule(r, { j(Verdict::accept) }) == 0);
        // aceita no todo, mas os trechos de dois disparos rejeitados: apodrece
        CHECK(judgeGameRule(r, { j(Verdict::accept), j(Verdict::reject, 0.0, 30.0), j(Verdict::reject, 85.0, 95.0) }) == 1);
        // um trecho só rejeitado e o resto aceito no todo: fica
        CHECK(judgeGameRule(r, { j(Verdict::accept), j(Verdict::reject, 0.0, 30.0) }) == 0);
        // sem a resposta inteira, só trechos: o trecho que não cobre nenhum
        // disparo não julga a regra
        CHECK(judgeGameRule(r, { j(Verdict::reject, 20.0, 40.0) }) == -1);
        CHECK(judgeGameRule(r, { j(Verdict::transform, 45.0, 60.0) }) == 2);
        // o último julgamento do mesmo trecho vale
        CHECK(judgeGameRule(r, { j(Verdict::reject, 45.0, 60.0), j(Verdict::accept, 45.0, 60.0) }) == 0);
        // resposta antiga, sem disparos guardados: a resposta inteira
        GameRule old;
        CHECK(judgeGameRule(old, { j(Verdict::reject), j(Verdict::accept, 0.0, 10.0) }) == 1);
        // applyGameRule guarda os disparos
        Score sc;
        sc.duration = 40.0;
        for (int i = 0; i < 80; ++i)
            sc.notes.push_back({ i * 0.5, 0.4, 60.0f + static_cast<float>(i % 7), 0.0f, 0.6f, 0, Timbre::sax });
        GameRule h;
        h.action = GameAction::hide;
        h.span = 2.0;
        applyGameRule(sc, h, { 5.0, 20.0, 35.0 });
        CHECK(h.fired == 3 && h.firedAt.size() == 3 && std::abs(h.firedAt[1] - 20.0) < 1e-9);
    }
    // o estado do jogo (§5): interromper gasta coerência e surpresa e aproxima
    // o colapso; três interrupções seguidas esgotam a surpresa; sustentar
    // protege o equilíbrio; o tempo devolve a surpresa
    {
        GameRule cut;
        cut.id = 1;
        cut.action = GameAction::interrupt;
        cut.firedAt = { 5.0, 6.0, 7.0 };
        GameState st;
        st.surprise = 0.6;
        st.collapse = 0.1;
        const auto m = runGameState({ cut }, 200.0, st);
        bool esgotou = false, colapso = false;
        for (const auto& x : m)
        {
            esgotou = esgotou || x.event == "sem surpresa disponível";
            colapso = colapso || x.event == "perto do colapso";
        }
        CHECK(esgotou && !colapso);
        CHECK(m[1].state.coherence < st.coherence && m[1].state.collapse > st.collapse);
        CHECK(m.back().event.rfind("fim", 0) == 0 && m.back().state.surprise > 0.5);  // o tempo devolveu
        GameRule hold;
        hold.id = 2;
        hold.action = GameAction::sustain;
        hold.hidden = false;
        hold.firedAt = { 10.0 };
        const auto h = runGameState({ hold }, 20.0, st);
        CHECK(h[1].state.collapse < st.collapse && h[1].state.coherence > st.coherence);
        // a regra oculta acumula informação oculta
        CHECK(m[1].state.hidden > 0.0);
        // o estado inicial: mais imprevisibilidade, mais surpresa
        Options a = o, b = o;
        a.unpredictability = 0.0f;
        b.unpredictability = 1.0f;
        CHECK(initialGameState(b).surprise > initialGameState(a).surprise);
        // na peça: o jogo anota o estado
        Options k = o;
        k.unpredictability = 1.0f;
        auto c = partitura(40.0);
        (void) playGame(c, fr, on, 40.0, k);
        CHECK(c.game.empty() || (!c.gameState.empty() && c.gameState.front().event == "início"));
    }
    // a sessão longa: a resposta parte, pela metade, de onde a anterior parou
    {
        Options a = o;
        a.unpredictability = 1.0f;
        const GameState base = initialGameState(a);
        a.hasGamePrevious = true;
        a.gamePrevious.surprise = 0.0;
        a.gamePrevious.collapse = 0.9;
        a.gamePrevious.hidden = 0.8;
        const GameState carried = initialGameState(a);
        CHECK(std::abs(carried.surprise - 0.5 * base.surprise) < 1e-9);
        CHECK(carried.collapse > base.collapse && carried.hidden == 0.0);
    }
    // o estado decide: perto do colapso, nenhuma regra que quebra entra; sem
    // surpresa, nenhuma é oculta
    {
        Options a = o;
        a.mode = trioio::strategy::Mode::interrupt;
        a.unpredictability = 1.0f;
        a.hasGamePrevious = true;
        a.unpredictability = 0.0f;           // a surpresa própria: 0,4
        a.gamePrevious.collapse = 1.0;       // o colapso misturado: 0,55
        a.gamePrevious.surprise = -0.2;      // a surpresa misturada: 0,1
        a.gamePrevious.energy = a.gamePrevious.coherence = a.gamePrevious.density = 0.5;
        auto c = partitura(40.0);
        (void) playGame(c, fr, on, 40.0, a);
        for (const auto& g : c.game)
        {
            CHECK(g.action != GameAction::interrupt && g.action != GameAction::invert && g.action != GameAction::densify);
            CHECK(!g.hidden);
        }
    }
    // o caminho 4: a regra que nasce da nota do autor
    {
        GameRule r;
        CHECK(ruleFromNote("quando cresce, o baixo cala", r) && r.action == GameAction::hide && r.when == static_cast<int>(Condition::energyRising)
              && r.material == 3 && !r.hidden);
        CHECK(ruleFromNote("Corta tudo sempre que ficar parado", r) && r.action == GameAction::interrupt
              && r.when == static_cast<int>(Condition::stasis) && r.material == 1);
        CHECK(ruleFromNote("precisa respirar mais", r) && r.action == GameAction::rarefy && r.when == 0);
        CHECK(ruleFromNote("se estiver rarefeito, imitar", r) && r.action == GameAction::imitate && r.when == static_cast<int>(Condition::sparse));
        // sem ação, não há regra; "cala" não se acha dentro de "escala"
        CHECK(!ruleFromNote("achei estranho os sons do jazz", r));
        CHECK(!ruleFromNote("gostei da escala", r));
        // na peça: a nota entra como candidata, com a origem e a nota
        Options k = o;
        k.unpredictability = 1.0f;
        k.gameNotes = { "não gostei", "quando denso, sustentar as notas longas" };
        auto c = partitura(40.0);
        (void) playGame(c, fr, on, 40.0, k);
        bool autor = false;
        for (const auto& g : c.game)
            autor = autor || (g.origin == "da nota do autor" && g.action == GameAction::sustain);
        for (const auto& g : c.nearGame)
            autor = autor || g.origin == "da nota do autor";
        CHECK(autor);
    }
    // os critérios no som: a mesma partitura dos dois lados não muda nada
    // (o render é determinístico); a transposição de uma oitava muda o
    // espectro sem mudar o nível; ocultar o motivo muda o nível
    {
        auto base = partitura(20.0);
        GameRule same;
        same.span = 2.0;
        same.firedAt = { 5.0, 12.0 };
        const auto v0 = soundCheck(base, base, same);
        std::printf("no som, igual: %s\n", v0.why.c_str());
        CHECK(!v0.audible && same.soundSpectral < 0.5f);
        auto up = base;
        for (auto& n : up.notes)
            if (n.voice == 0 && ((n.time >= 5.0 && n.time < 7.0) || (n.time >= 12.0 && n.time < 14.0)))
                n.pitch += 7.0f;
        GameRule tr = same;
        const auto v1 = soundCheck(base, up, tr);
        std::printf("no som, uma quinta acima: %s\n", v1.why.c_str());
        CHECK(v1.audible && v1.coherent);
        auto hid = base;
        hid.notes.erase(std::remove_if(hid.notes.begin(), hid.notes.end(),
                                       [](const Note& n) { return n.voice != 1 && n.time >= 5.0 && n.time < 7.0; }),
                        hid.notes.end());
        GameRule h = same;
        const auto v2 = soundCheck(base, hid, h);
        std::printf("no som, oculto: %s\n", v2.why.c_str());
        CHECK(v2.audible && h.soundDb < 0.0f);
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("jogo: todas as checagens passaram");
    return EXIT_SUCCESS;
}
