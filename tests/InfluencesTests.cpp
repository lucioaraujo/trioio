// A árvore de influências (src/core/Influences.h): a herança, as tendências
// (não regras), a polinização, a raridade do muito usado, a repetição que o
// conceito pede, e o caráter (dança, peso) sobre a partitura.
#include "Influences.h"
#include "HarmonyWriting.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

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
    std::ifstream f(TRIOIO_SOURCE_DIR "/data/influencias/arquivo_trioio_influencias.txt");
    std::stringstream ss;
    ss << f.rdbuf();
    std::vector<std::string> erros;
    const auto arq = parseInfluenceArchive(ss.str(), erros);
    for (const auto& e : erros)
        std::fprintf(stderr, "arquivo: %s\n", e.c_str());
    CHECK(erros.empty() && arq.size() >= 30);
    // a busca e a herança: "pesado/lento" herda do rock e do pesado e muda o peso
    const auto* lento = findInfluence(arq, "pesado/lento");
    CHECK(lento && lento->path == "rock/pesado/lento" && !findInfluence(arq, "inexistente"));
    const auto tr = inheritedTraits(arq, *lento);
    float peso = -1.0f, percussao = -1.0f;
    for (const auto& t : tr)
    {
        if (t.key == "eixo:peso") peso = t.value;
        if (t.key == "presença:percussão") percussao = t.value;
    }
    CHECK(peso == 1.0f && percussao == 0.7f);  // o filho muda o que herdou
    // as tendências: em 200 sorteios do rock, a percussão às vezes falta
    int semPercussao = 0, polinizadas = 0;
    for (std::uint64_t s = 1; s <= 200; ++s)
    {
        const auto d = drawInfluences(arq, { "rock" }, s, 0.6f, {});
        if (d.presence.count("percussão") && !d.presence.at("percussão"))
            ++semPercussao;
        polinizadas += d.nodes.size() > 1 ? 1 : 0;
    }
    std::printf("rock em 200 sorteios: sem percussão %d, com polinização %d\n", semPercussao, polinizadas);
    CHECK(semPercussao > 10 && semPercussao < 70);  // ~15 %: a tendência, não a regra
    CHECK(polinizadas > 20);
    // a raridade: com o rock muito usado, a árvore livre escolhe outros nós
    std::map<std::string, int> uso;
    for (const auto& n : arq)
        if (n.path.rfind("rock", 0) == 0)
            uso["nó:" + n.path] = 50;
    int rock = 0;
    for (std::uint64_t s = 1; s <= 100; ++s)
        rock += drawInfluences(arq, {}, s, 0.5f, uso).nodes.front().rfind("rock", 0) == 0 ? 1 : 0;
    std::printf("com o rock muito usado, a árvore livre escolheu rock em %d de 100\n", rock);
    CHECK(rock < 10);
    // a repetição que o conceito pede: o drone quase não poliniza nem muta
    int novidades = 0;
    for (std::uint64_t s = 1; s <= 100; ++s)
    {
        const auto d = drawInfluences(arq, { "drone" }, s, 1.0f, {});
        novidades += d.nodes.size() > 1 ? 1 : 0;
        for (const auto& l : d.lines)
            novidades += l.rfind("mutação", 0) == 0 ? 1 : 0;
    }
    std::printf("drone (repetição 1,0) com imprevisibilidade 1: %d novidades em 100\n", novidades);
    CHECK(novidades == 0);
    // a força das influências e a exclusão do autor
    {
        int escolhasFortes = 0, escolhasFracas = 0;
        for (std::uint64_t s = 1; s <= 100; ++s)
        {
            escolhasFortes += !drawInfluences(arq, { "rock/punk" }, s, 0.0f, {}, 1.0f).rockStyle.empty() ? 1 : 0;
            escolhasFracas += !drawInfluences(arq, { "rock/punk" }, s, 0.0f, {}, 0.2f).rockStyle.empty() ? 1 : 0;
        }
        std::printf("o subestilo do punk: %d em 100 com força 1, %d com força 0,2\n", escolhasFortes, escolhasFracas);
        CHECK(escolhasFortes > 70 && escolhasFracas < 30);
        const auto fraco = drawInfluences(arq, { "erudita/minimalismo" }, 3, 0.0f, {}, 0.0f);
        for (const auto& [a, v] : fraco.axes)
            CHECK(std::abs(v - 0.5f) < 1e-6f);  // força 0: os eixos no meio
        const auto sem = drawInfluences(arq, { "eletrônica/pista" }, 3, 0.5f, {}, 1.0f, { "percussão" });
        CHECK(sem.presence.count("percussão") && !sem.presence.at("percussão"));  // a exclusão vence a tendência de 0,95
    }
    // o mapa: entre candidatos, fica o mais longe do já feito; com a
    // repetição pedida, a busca cede
    {
        std::vector<InfluencePoint> passado(10);
        for (auto& p : passado)
        {
            p.dance = 0.9f;
            p.ensemble = "rock";
        }
        InfluenceDraw perto, longe;
        perto.axes["dança"] = 0.85f;
        perto.ensemble = "rock";
        longe.axes["dança"] = 0.1f;
        longe.ensemble = "câmara";
        double dist = 0.0;
        CHECK(pickNovelDraw({ perto, longe }, passado, &dist) == 1 && dist > 1.0);
        InfluenceDraw rep = perto;
        rep.axes["repetição"] = 0.9f;
        CHECK(pickNovelDraw({ rep, longe }, passado) == 0);  // o conceito pede a repetição
        CHECK(pickNovelDraw({ perto, longe }, {}) == 0);     // sem passado, o primeiro
    }
    // os timbres e a densidade
    {
        int sax = 0;
        for (std::uint64_t s = 1; s <= 100; ++s)
        {
            const auto d = drawInfluences(arq, { "jazz" }, s, 0.3f, {});
            sax += d.timbres.count("motivo") && d.timbres.at("motivo") == "sax" ? 1 : 0;
        }
        std::printf("jazz: o motivo no sax em %d de 100\n", sax);
        CHECK(sax > 15 && sax < 75);  // a tendência de 0,5, às vezes com outros galhos
        Score sc;
        sc.duration = 10.0;
        for (int k = 0; k < 10; ++k)
        {
            sc.notes.push_back({ k * 1.0, 0.9, 60.0f, 0.0f, 0.6f, 0, Timbre::additive });
            sc.notes.push_back({ k * 1.0, 0.9, 64.0f, 0.0f, 0.6f, 6, Timbre::additive });
        }
        InfluenceDraw d;
        d.timbres["motivo"] = "sax";
        Score t = sc;
        (void) applyInfluenceScore(t, d, 1);
        CHECK(t.notes[0].timbre == Timbre::sax && t.notes[1].timbre == Timbre::additive);
        Score denso = sc, ralo = sc;
        (void) applyCharacter(denso, { { "densidade", 1.0f } }, 2);
        (void) applyCharacter(ralo, { { "densidade", 0.0f } }, 2);
        int harmonia = 0;
        for (const auto& n : ralo.notes)
            harmonia += n.voice == 6 ? 1 : 0;
        CHECK(denso.notes.size() > sc.notes.size() + 10 && harmonia < 5);
    }
    // o cruzamento da forma com as influências: os eixos seguem a curva; a
    // virada traz o galho distante
    {
        const auto d = drawInfluences(arq, { "rock/noise", "erudita/minimalismo" }, 4, 0.0f, {});
        CHECK(d.twistNode == "erudita/minimalismo" || d.twistNode.find("/") != std::string::npos);
        Score sc;
        sc.duration = 40.0;
        for (int k = 0; k < 80; ++k)
        {
            sc.notes.push_back({ k * 0.5, 0.45, 45.0f, 0.0f, 0.6f, 1, Timbre::additive });
            sc.notes.push_back({ k * 0.5, 0.45, 62.0f, 0.0f, 0.6f, 0, Timbre::additive });
        }
        // os eixos neutros, a curva de Freytag: o baixo mais curto no clímax que na exposição
        Score f = sc;
        const auto curva = narrativeCurve("freytag", 40.0);
        (void) applyCharacter(f, {}, 1, &curva);
        double inicio = 0.0, climax = 0.0;
        for (const auto& n : f.notes)
            if (n.voice == 1)
            {
                if (n.time < 6.0) inicio = n.duration;
                if (n.time > 21.0 && n.time < 23.0) climax = n.duration;
            }
        CHECK(climax < inicio);
        // a virada: só o trecho de 50 a 75 % ganha o timbre do galho distante
        InfluenceDraw v;
        v.twistNode = "jazz";
        v.twistTimbres["motivo"] = "sax";
        Score t = sc;
        (void) applyInfluenceScore(t, v, 1, nullptr, "kishotenketsu");
        int dentro = 0, fora = 0;
        for (const auto& n : t.notes)
            if (n.voice == 0 && n.timbre == Timbre::sax)
                (n.time >= 20.0 && n.time < 30.0 ? dentro : fora)++;
        CHECK(dentro == 20 && fora == 0);
    }
    // as palavras do autor chamam nós da árvore pelo nome
    {
        const auto w = influencesFromWords(arq, "Algo dançante com Maracatu, um pouco de post-rock e jazz");
        for (const auto& x : w)
            std::printf("  palavra → nó: %s\n", x.c_str());
        CHECK(std::find(w.begin(), w.end(), "brasil/nordeste/maracatu") != w.end());
        CHECK(std::find(w.begin(), w.end(), "rock/post-rock") != w.end());
        CHECK(std::find(w.begin(), w.end(), "jazz") != w.end());
        CHECK(influencesFromWords(arq, "nada aqui").empty());
        // duas palavras e sem acento (2026-10-18: "post rock" e "maracatú")
        const auto pr = influencesFromWords(arq, "macumba, post rock");
        CHECK(std::find(pr.begin(), pr.end(), "rock/post-rock") != pr.end());
        CHECK(std::find(pr.begin(), pr.end(), "rock") == pr.end());
        // a tecla morta solta (o JUCE no Linux não compõe): "bai~ao", "maracat´u"
        const auto dk = influencesFromWords(arq, "maracat´u, bai~ao");
        CHECK(std::find(dk.begin(), dk.end(), "brasil/nordeste/maracatu") != dk.end());
        // a escrita livre no campo Influências: "rock 60" → um nó do rock dos anos 1960
        {
            const auto dd = drawInfluences(arq, { "rock 60" }, 7, 0.5f, {});
            bool sixties = false;
            for (const auto& n : dd.nodes)
                sixties = sixties || (n.rfind("rock", 0) == 0 && findInfluence(arq, n)->context.find("1960") != std::string::npos);
            CHECK(sixties);
        }
        const auto vc = influencesFromWords(arq, "tango improvisado, coral a capela, coco de roda");
        CHECK(std::find(vc.begin(), vc.end(), "vocal/coral") != vc.end());
        CHECK(std::find(vc.begin(), vc.end(), "mundo/tango") != vc.end());
        CHECK(std::find(vc.begin(), vc.end(), "brasil/nordeste/coco") != vc.end());
        // o nome inteiro ("coco de roda") chama o nó e consome as suas palavras
        CHECK(std::find(vc.begin(), vc.end(), "brasil/nordeste/coco") != vc.end());
        // um outro nome de uma palavra só (com hífen) também chama o nó
        const auto fc = influencesFromWords(arq, "um frevo-canção lento");
        CHECK(std::find(fc.begin(), fc.end(), "brasil/nordeste/frevo") != fc.end());
        const auto cw = influenceAliasWords(arq, "tango, coco de roda");
        CHECK(std::find(cw.begin(), cw.end(), "roda") != cw.end());
        // um nó do autor com "também:" (o formato de influencias_do_autor.txt)
        {
            std::vector<std::string> e;
            auto mine = parseInfluenceArchive("= artistas/exemplo\ntambém: banda exemplar\ncontexto: x\ntraço: eixo:rasgo 0.9\nfonte: y\n", e);
            CHECK(e.empty() && mine.size() == 1 && mine[0].aliases.size() == 1);
            const auto hit = influencesFromWords(mine, "algo como Banda Exemplar, lento");
            CHECK(hit.size() == 1 && hit[0] == "artistas/exemplo");
        }
        const auto ty = influencesFromWords(arq, "MARCATU!!, Postrock 🎸, sambas");
        CHECK(std::find(ty.begin(), ty.end(), "brasil/nordeste/maracatu") != ty.end());
        CHECK(std::find(ty.begin(), ty.end(), "rock/post-rock") != ty.end());
        CHECK(std::find(ty.begin(), ty.end(), "brasil/samba") != ty.end());
        CHECK(std::find(w.begin(), w.end(), "rock/dançante") == w.end());  // sem "rock", "dançante" é só o eixo
        const auto r = influencesFromWords(arq, "rock dançante");
        CHECK(std::find(r.begin(), r.end(), "rock/dançante") != r.end());
    }
    // nas opções: o que o autor escolheu fica
    {
        InfluenceDraw d;
        d.ensemble = "jazz";
        d.harmony = "modal";
        d.mood = "tensa";
        Options o;
        o.harmonyLanguage = static_cast<int>(HarmonyLanguage::frevo);
        (void) applyInfluenceOptions(o, d);
        CHECK(o.ensemble == 3 && o.harmonyLanguage == static_cast<int>(HarmonyLanguage::frevo) && o.intention == Intention::tense);
    }
    // o caráter: a dança encurta o baixo; o introspectivo rareia a percussão;
    // o peso desce o baixo uma oitava
    {
        Score sc;
        sc.duration = 20.0;
        for (int k = 0; k < 40; ++k)
        {
            sc.notes.push_back({ k * 0.5, 0.45, 45.0f, 0.0f, 0.6f, 1, Timbre::additive });
            sc.notes.push_back({ k * 0.5, 0.1, 60.0f, 0.0f, 0.6f, 4, Timbre::additive });
        }
        Score dan = sc, intro = sc;
        (void) applyCharacter(dan, { { "dança", 1.0f }, { "peso", 1.0f } }, 1);
        (void) applyCharacter(intro, { { "dança", 0.0f } }, 1);
        int perc = 0;
        for (const auto& n : intro.notes)
            perc += n.voice == 4 ? 1 : 0;
        CHECK(dan.notes[0].duration < 0.45 * 0.6 && dan.notes[0].pitch == 33.0f);
        CHECK(perc < 30);
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("influências: todas as checagens passaram");
    return EXIT_SUCCESS;
}
