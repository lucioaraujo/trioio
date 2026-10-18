// A intenção (src/core/Intent.h): a tradução do prompt para o vocabulário
// do Trioio, com a palavra de cada campo à vista; a inferência pelo som; a
// comparação pedido × ouvido; e a intenção que entra na resposta.
#include "Intent.h"
#include "Ensemble.h"
#include "HarmonyWriting.h"

#include <cmath>
#include <algorithm>
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

int main()
{
    // 1. um prompt de gerador, em inglês
    {
        const auto d = translatePrompt("Upbeat jazz with saxophone, 128 BPM, dark and dreamy, female vocals");
        std::printf("tradução: %s\n", intentFieldsText(d.fields).c_str());
        for (const auto& m : d.matches)
            std::printf("  %s → %s = %s\n", m.word.c_str(), m.field.c_str(), m.value.c_str());
        std::printf("  não lidas:");
        for (const auto& w : d.unread)
            std::printf(" %s", w.c_str());
        std::printf("\n");
        CHECK(d.fields.energy > 0.7f && d.fields.ensemble == static_cast<int>(Ensemble::jazz) && std::abs(d.fields.bpm - 128.0f) < 0.5f);
        CHECK(d.fields.brightness < 0.3f && d.fields.mood == static_cast<int>(Intention::suspended) && d.fields.voice == 1);
        CHECK(!d.unread.empty());  // "saxophone", "female": ditas como não lidas, não inventadas
    }
    // 2. em português e francês; o estilo brasileiro e a linguagem harmônica
    {
        const auto d = translatePrompt("um baião lento e melancólico, modal, instrumental");
        CHECK(d.fields.ensemble == static_cast<int>(Ensemble::forro) && d.fields.bpm < 80.0f && d.fields.mood == static_cast<int>(Intention::somber));
        CHECK(d.fields.harmonyLanguage == static_cast<int>(HarmonyLanguage::modal) && d.fields.voice == 0);
        const auto f = translatePrompt("musique douce et chaude, orchestre");
        CHECK(f.fields.energy < 0.3f && f.fields.brightness < 0.5f && f.fields.ensemble == static_cast<int>(Ensemble::orchestra));
        // um prompt vazio de campos não inventa nada
        const auto z = translatePrompt("uma faixa");
        CHECK(z.matches.empty() && intentFieldsText(z.fields) == "nada lido");
    }
    // 3. a inferência pelo som: quadros altos, brilhantes e periódicos, com
    //    ataques densos, num tom maior
    {
        std::vector<trioio::perception::Frame> fr(400);
        for (std::size_t i = 0; i < fr.size(); ++i)
        {
            fr[i].time = i * 0.01;
            fr[i].silent = false;
            fr[i].rmsDb = -12.0f;
            fr[i].centroidHz = 3000.0f;
            fr[i].periodicity = 0.9f;
            fr[i].flatness = 0.1f;
        }
        std::vector<trioio::perception::Onset> on;
        for (int k = 0; k < 24; ++k)
            on.push_back({ k * 0.16, 1.0f });
        Reading r;
        r.key.tonic = 0;
        r.key.minor = false;
        r.pulse.bpm = 120.0;
        r.pulse.confidence = 0.8f;
        const auto o = inferIntent(fr, on, 4.0, r);
        std::printf("ouvida: %s\n", intentFieldsText(o).c_str());
        CHECK(o.energy > 0.85f && o.brightness > 0.8f && o.harmonicity > 0.8f && o.density > 0.5f);
        CHECK(std::abs(o.bpm - 120.0f) < 0.5f && o.mood == static_cast<int>(Intention::luminous));
        // 4. a comparação: o pedido contido × o som cheio
        IntentFields pedido;
        pedido.energy = 0.2f;
        pedido.bpm = 60.0f;  // a metade de 120: o mesmo pulso
        pedido.mood = static_cast<int>(Intention::somber);
        const auto gaps = compareIntent(pedido, o);
        for (const auto& g : gaps)
            std::printf("  %s\n", g.text.c_str());
        CHECK(gaps.size() == 3);
        CHECK(gaps[0].field == "energy" && gaps[0].gap > 0.6f && gaps[0].text.find("mais intensa") != std::string::npos);
        CHECK(gaps[1].field == "tempo" && gaps[1].gap < 0.05f);
        CHECK(gaps[2].field == "mood" && gaps[2].gap > 0.5f);
        // 5. na resposta: o humor e o estilo declarados entram onde o autor
        //    não escolheu; a distância abre o olho; o que o autor escolheu fica
        DeclaredIntent d;
        d.fields = pedido;
        d.fields.ensemble = static_cast<int>(Ensemble::tango);
        Options opt;
        opt.unpredictability = 0.5f;
        const auto lines = applyIntent(opt, d, o);
        for (const auto& l : lines)
            std::printf("  · %s\n", l.c_str());
        CHECK(opt.intention == Intention::somber && opt.ensemble == static_cast<int>(Ensemble::tango));
        CHECK(opt.unpredictability > 0.5f && opt.unpredictability <= 0.75f);
        Options escolhido;
        escolhido.ensemble = static_cast<int>(Ensemble::jazz);
        escolhido.intention = Intention::serene;
        (void) applyIntent(escolhido, d, o);
        CHECK(escolhido.ensemble == static_cast<int>(Ensemble::jazz) && escolhido.intention == Intention::serene);
    }
    // a forma e a dança pelas palavras
    {
        const auto d = translatePrompt("algo dançante, com uma virada no meio");
        CHECK(d.fields.dance > 0.7f && d.fields.narrative == 1);
        const auto e = translatePrompt("introspective, hypnotic");
        CHECK(e.fields.dance < 0.3f && e.fields.narrative == 3);
        Options o;
        DeclaredIntent di;
        di.fields.narrative = 0;
        (void) applyIntent(o, di, IntentFields {});
        CHECK(o.narrative == "freytag");
    }
    // os subestilos do rock: a palavra dá o subestilo e o conjunto
    {
        const auto d = translatePrompt("heavy doom riffs, slow");
        CHECK(d.fields.rockStyle == static_cast<int>(RockStyle::heavy) && d.fields.ensemble == static_cast<int>(Ensemble::rock));
        CHECK(d.fields.energy > 0.8f);  // "heavy" também dá a energia
        const auto n = translatePrompt("noise rock com microfonia");
        CHECK(n.fields.rockStyle == static_cast<int>(RockStyle::noise));
        RockStyle rs;
        CHECK(parseRockStyle("post-rock", rs) && rs == RockStyle::postrock && !parseRockStyle("bossa", rs));
        Options o;
        DeclaredIntent di;
        di.fields.rockStyle = static_cast<int>(RockStyle::punk);
        (void) applyIntent(o, di, IntentFields {});
        CHECK(o.rockStyle == static_cast<int>(RockStyle::punk));
    }
    // duas palavras e sem acento (2026-10-18: "post rock", "sinfonica")
    {
        const auto d = translatePrompt("macumba, sinfonica, post rock");
        CHECK(d.fields.rockStyle == static_cast<int>(RockStyle::postrock));
        CHECK(d.fields.ensemble == static_cast<int>(Ensemble::orchestra));  // a primeira que diz vale
        CHECK(d.unread.size() == 1 && d.unread[0] == "macumba");
    }
    // a escrita do autor como vier (2026-10-18): caixa, acento faltando ou
    // indevido, pontuação, emoji, números, separação e erro de digitação
    {
        const auto d = translatePrompt("MELANCÓLICO!!! 🎉 “Pós Rock”, Dancánte; 120BPM #ruidoso, 1998");
        CHECK(d.fields.mood == static_cast<int>(Intention::somber));
        CHECK(d.fields.rockStyle == static_cast<int>(RockStyle::postrock));
        CHECK(d.fields.dance > 0.8f && d.fields.bpm == 120.0f && d.fields.harmonicity < 0.2f);
        CHECK(d.unread.empty());  // nem o "1998", nem os símbolos vão para a rede
        const auto t = translatePrompt("melacolico, sifonica");  // uma letra faltando
        CHECK(t.fields.mood == static_cast<int>(Intention::somber) && t.fields.ensemble == static_cast<int>(Ensemble::orchestra));
        CHECK(t.unread.empty());
        CHECK(std::any_of(t.matches.begin(), t.matches.end(), [](const IntentMatch& m) { return m.field == "correção"; }));
        CHECK(translatePrompt("rokc").fields.ensemble == static_cast<int>(Ensemble::rock));  // vizinhas invertidas
        const auto e = translatePrompt("escuta");  // o radical curto "escur" não corrige "escuta"
        CHECK(e.fields.brightness < 0.0f);
    }
    // "tango improvisado, coral a capela" (o terceiro uso no app, 2026-10-18)
    {
        const auto d = translatePrompt("tango improvisado, coral a capela");
        CHECK(d.fields.ensemble == static_cast<int>(Ensemble::tango));  // a primeira que diz vale
        CHECK(d.fields.freedom > 0.7f && d.fields.voice == 1);
        CHECK(d.unread.empty());  // nem "coral" (o recife) nem "capela" (a igreja) vão para a rede
        Options o;
        o.unpredictability = 0.5f;
        (void) applyIntent(o, d, IntentFields {});
        CHECK(o.unpredictability >= 0.8f);
    }
    // as associações (o que a ponte semântica traz da rede): o caráter pela
    // média ponderada, o humor pelo voto; nunca o estilo
    {
        std::string why;
        const auto f = intentFromAssociations({ { "gentle", 1.0 }, { "soft", 0.9 }, { "steady", 0.8 }, { "constant", 0.7 },
                                                { "heavy", 0.2 }, { "xyzzy", 1.0 } },
                                              &why);
        CHECK(f.energy >= 0.0f && f.energy < 0.5f);  // gentle/soft vencem heavy pelo peso
        CHECK(f.narrative == 3);                      // steady/constant: o platô
        CHECK(f.ensemble < 0 && f.rockStyle < 0);     // "heavy" não vira rock
        CHECK(why.find("xyzzy") == std::string::npos && why.find("gentle") != std::string::npos);
        const auto none = intentFromAssociations({ { "xyzzy", 1.0 } });
        CHECK(none.energy < 0.0f && none.mood < 0 && none.narrative < 0);
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("intenção: todas as checagens passaram");
    return EXIT_SUCCESS;
}
