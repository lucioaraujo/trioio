// A harmonia escrita (src/core/HarmonyWriting.h): a tonalidade (Krumhansl–
// Kessler), a cadência perfeita no fim, a de engano ao contrapor, a
// modulação preparada, as dominantes secundárias e o ii–V–I do jazz.
#include "HarmonyWriting.h"
#include "SomDaAura.h"

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

static std::vector<ChordSpan> progressao(const std::vector<Chord>& cs, double each, double t0 = 0.0)
{
    std::vector<ChordSpan> v;
    double t = t0;
    for (const auto& c : cs)
    {
        v.push_back({ t, t + each, c, 0.8f });
        t += each;
    }
    return v;
}

int main()
{
    const Chord C { 0, ChordQuality::major }, F { 5, ChordQuality::major }, G { 7, ChordQuality::major },
        Am { 9, ChordQuality::minor }, Dm { 2, ChordQuality::minor }, Em { 4, ChordQuality::minor }, D { 2, ChordQuality::major };
    // 1. a tonalidade: Dó Fá Sol Lám Dó → Dó maior; Lám Rém Mi Lám → Lá menor
    {
        const auto k = findKey(progressao({ C, F, G, Am, C }, 2.0), 0.0, 10.0);
        std::printf("tonalidade: %s (%.2f)\n", keyName(k).c_str(), k.score);
        CHECK(k.tonic == 0 && k.major);
        const auto m = findKey(progressao({ Am, Dm, Chord { 4, ChordQuality::major }, Am, Dm, Am }, 2.0), 0.0, 12.0);
        std::printf("tonalidade: %s\n", keyName(m).c_str());
        CHECK(m.tonic == 9 && !m.major);
    }
    // 2. seguir: o fim da peça é a cadência perfeita (IV V I): os três
    //    últimos acordes Fá, Sol, Dó
    {
        auto s = progressao({ C, Am, F, Em, Dm, C }, 2.0);
        const auto line = writeHarmony(s, { { 0.0, 12.0 } }, 0.5, trioio::strategy::Mode::follow, HarmonyLanguage::functional, 1);
        std::printf("%s\n", line.c_str());
        const auto n = s.size();
        CHECK(n >= 3 && s[n - 1].chord == C && s[n - 2].chord == G && s[n - 3].chord == F);
        CHECK(s.back().end == 12.0);
    }
    // 3. contrapor: no fim de uma seção que não é a última, a de engano (V → vi)
    {
        auto s = progressao({ C, F, Dm, C, Am, F, G, C }, 2.0);
        (void) writeHarmony(s, { { 0.0, 8.0 }, { 8.0, 16.0 } }, 0.5, trioio::strategy::Mode::counter, HarmonyLanguage::functional, 1);
        // o último acorde da primeira seção (que acaba em 8 s)
        const ChordSpan* lastFirst = nullptr;
        for (const auto& x : s)
            if (x.start < 8.0 - 1e-6)
                lastFirst = &x;
        CHECK(lastFirst && lastFirst->chord == Am);
    }
    // 4. a modulação preparada: a seção 1 em Dó maior, a 2 em Sol maior → a
    //    seção 1 chega à dominante de Sol (Ré maior)
    {
        auto s = progressao({ C, F, G, C, G, D, Em, D, G, C, D, G }, 2.0);
        const auto line = writeHarmony(s, { { 0.0, 8.0 }, { 8.0, 24.0 } }, 0.5, trioio::strategy::Mode::follow, HarmonyLanguage::functional, 1);
        std::printf("%s\n", line.c_str());
        const ChordSpan* lastFirst = nullptr;
        for (const auto& x : s)
            if (x.start < 8.0 - 1e-6)
                lastFirst = &x;
        CHECK(lastFirst && lastFirst->chord == D);
    }
    // 5. as dominantes secundárias aparecem em algumas sementes (transfigurar)
    {
        int achou = 0;
        for (std::uint64_t sd = 1; sd <= 10; ++sd)
        {
            auto s = progressao({ C, Am, F, Dm, G, Em, Am, F, C }, 2.0);
            const auto line = writeHarmony(s, { { 0.0, 18.0 } }, 0.5, trioio::strategy::Mode::transfigure, HarmonyLanguage::functional, sd);
            for (std::size_t i = 0; i + 1 < s.size(); ++i)
                if (s[i].chord.quality == ChordQuality::major && (s[i + 1].chord.root - s[i].chord.root + 12) % 12 == 5
                    && s[i + 1].chord.root != 0)
                {
                    ++achou;
                    break;
                }
        }
        std::printf("dominantes secundárias em %d de 10 sementes\n", achou);
        CHECK(achou >= 3);
    }
    // 6. o jazz: o fim em ii–V–I (Rém, Sol, Dó)
    {
        auto s = progressao({ C, Am, F, Em, Dm, C }, 2.0);
        (void) writeHarmony(s, { { 0.0, 12.0 } }, 0.5, trioio::strategy::Mode::follow, HarmonyLanguage::jazz, 1);
        const auto n = s.size();
        // ii–V–I (Rém, Sol, Dó) ou a backdoor (Fám, Sib, Dó)
        const bool iiVI = n >= 3 && s[n - 1].chord == C && s[n - 2].chord == G && s[n - 3].chord == Dm;
        const bool backdoor = n >= 3 && s[n - 1].chord == C && s[n - 2].chord.root == 10 && s[n - 3].chord.root == 5;
        CHECK(iiVI || backdoor);
        // no jazz, todo acorde tem cor (a 7ª)
        bool cores = true;
        for (const auto& x : s)
            cores = cores && x.chord.color != 0;
        CHECK(cores);
    }
    // 7. as cadências registradas: o tipo e o acorde de chegada (o fim da peça
    //    é autêntico e termina no fim da última seção)
    {
        auto s = progressao({ C, Am, F, G, C, F, Dm, C }, 2.0);
        std::vector<CadenceMark> cads;
        (void) writeHarmony(s, { { 0.0, 8.0 }, { 8.0, 16.0 } }, 0.5, trioio::strategy::Mode::interrupt, HarmonyLanguage::functional, 1, &cads);
        CHECK(cads.size() == 2);
        CHECK(cads.size() == 2 && cads[0].kind == CadenceMark::Kind::half);
        CHECK(cads.size() == 2 && cads[1].kind == CadenceMark::Kind::authentic && std::abs(cads[1].end - 16.0) < 1e-6);
    }
    // 8. a condução de vozes: I–IV–V–I–vi–ii–V–I em Dó, sem quintas nem
    //    oitavas paralelas; no V → I a sensível (Si) sobe a Dó; sem
    //    cruzamento e no registro
    {
        const std::vector<std::vector<int>> prog { { 0, 4, 7 }, { 5, 9, 0 }, { 7, 11, 2 }, { 0, 4, 7 },
                                                   { 9, 0, 4 }, { 2, 5, 9 }, { 7, 11, 2 }, { 0, 4, 7 } };
        std::vector<float> prev;
        int prevRoot = -1, paralelas = 0, sensivel = 0, sensivelSubiu = 0;
        bool prevMaior = false, ok = true;
        for (const auto& c : prog)
        {
            int leading = -1;
            if (prevRoot >= 0 && (prevRoot - c[0] + 12) % 12 == 7 && prevMaior)
                leading = (prevRoot + 4) % 12;
            const auto v = voiceLead(prev, c, prevRoot < 0 ? c[0] : prevRoot, c[0], leading);
            ok = ok && v.size() == 3 && v[0] < v[1] && v[1] < v[2] && v[0] >= 48.0f && v[2] <= 77.0f;
            if (!prev.empty())
            {
                paralelas += countParallels(prev, v, prevRoot, c[0]);
                if (leading >= 0)
                    for (std::size_t k = 0; k < 3; ++k)
                        if (static_cast<int>(std::lround(prev[k])) % 12 == leading)
                        {
                            ++sensivel;
                            sensivelSubiu += std::abs(v[k] - prev[k] - 1.0f) < 0.01f ? 1 : 0;
                        }
            }
            std::printf("[%g %g %g] ", static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]));
            prev = v;
            prevRoot = c[0];
            prevMaior = (c[1] - c[0] + 12) % 12 == 4;
        }
        std::printf("\nparalelas: %d; a sensível subiu %d de %d\n", paralelas, sensivelSubiu, sensivel);
        CHECK(ok);
        CHECK(paralelas == 0);
        // os dois V → I, e o I → IV (Dó é a dominante de Fá: Mi sobe a Fá)
        CHECK(sensivel >= 2 && sensivelSubiu == sensivel);
    }
    // 9. as linguagens da máquina de harmonia
    {
        using L = HarmonyLanguage;
        const Chord E { 4, ChordQuality::major };
        // funcional no menor: a frígia, a napolitana e a picarda aparecem
        // em alguma semente
        bool frigia = false, napolitana = false, picarda = false;
        for (std::uint64_t sd = 1; sd <= 30; ++sd)
        {
            auto s = progressao({ Am, Dm, E, Am, F, Dm, E, Am }, 2.0);
            const auto line = writeHarmony(s, { { 0.0, 8.0 }, { 8.0, 16.0 } }, 0.5, trioio::strategy::Mode::follow, L::functional, sd);
            frigia = frigia || line.find("frígias") != std::string::npos;
            napolitana = napolitana || line.find("napolitana") != std::string::npos;
            picarda = picarda || line.find("picarda") != std::string::npos;
        }
        std::printf("funcional no menor: frígia %d, napolitana %d, picarda %d\n", frigia, napolitana, picarda);
        CHECK(frigia && napolitana && picarda);
        // brasileira: o diminuto de passagem aparece; a tônica final é 6(9)
        bool dim = false, seisNove = false;
        for (std::uint64_t sd = 1; sd <= 20; ++sd)
        {
            auto s = progressao({ C, Dm, G, C, F, Dm, G, C }, 2.0);
            const auto line = writeHarmony(s, { { 0.0, 16.0 } }, 0.5, trioio::strategy::Mode::transfigure, L::brazilian, sd);
            dim = dim || line.find("diminutos de passagem") != std::string::npos;
            seisNove = seisNove || (s.back().chord.root == 0 && (s.back().chord.color & kSixth) && (s.back().chord.color & kNinth));
        }
        CHECK(dim && seisNove);
        // impressionista: nenhuma cadência V → I; os tons inteiros
        // aumentados; o planing com a mesma qualidade; o pedal
        bool tons = false, planing = false, pedal = false, semVI = true;
        for (std::uint64_t sd = 1; sd <= 30; ++sd)
        {
            auto s = progressao({ C, Am, F, G, Em, Am, Dm, G, C, F, G, C }, 2.0);
            std::vector<CadenceMark> cads;
            const auto line = writeHarmony(s, { { 0.0, 8.0 }, { 8.0, 16.0 }, { 16.0, 24.0 } }, 0.5, trioio::strategy::Mode::transfigure,
                                           L::impressionist, sd, &cads);
            tons = tons || line.find("tons inteiros") != std::string::npos;
            pedal = pedal || line.find("pedal") != std::string::npos;
            if (line.find("planing real") != std::string::npos)
            {
                bool igual = true;
                for (const auto& x : s)
                    if (x.start < 8.0 && x.end <= 8.0 - 2.0 + 1e-6)
                        igual = igual && x.chord.quality == ChordQuality::major && (x.chord.color & kNinth);
                planing = planing || igual;
            }
            // o fim de cada seção nunca é V → I
            for (const auto& c : cads)
                for (std::size_t i = 1; i < s.size(); ++i)
                    if (std::abs(s[i].start - c.start) < 1e-6 && s[i].chord.root == c.key.tonic && s[i - 1].chord.root == (c.key.tonic + 7) % 12)
                        semVI = false;
        }
        std::printf("impressionista: tons inteiros %d, planing %d, pedal %d, sem V→I %d\n", tons, planing, pedal, semVI);
        CHECK(tons && pedal && semVI);
        // a condução a quatro vozes: o ii7–V7–Imaj7 sem paralelas
        const std::vector<std::vector<int>> jazzProg { { 5, 0, 9, 4 }, { 11, 5, 9, 2 }, { 4, 11, 2, 7 } };  // 3ª,7ª,9ª,5ª de cada
        std::vector<float> prev;
        int pr = -1, paralelas = 0;
        const int roots[3] = { 2, 7, 0 };
        for (int k = 0; k < 3; ++k)
        {
            const auto v = voiceLead(prev, jazzProg[static_cast<std::size_t>(k)], pr < 0 ? roots[k] : pr, roots[k], -1);
            CHECK(v.size() == 4);
            if (!prev.empty())
                paralelas += countParallels(prev, v, pr, roots[k]);
            prev = v;
            pr = roots[k];
        }
        CHECK(paralelas == 0);
    }
    // 10. as linguagens brasileiras modais e a de Hermeto
    {
        using L = HarmonyLanguage;
        const auto prog = [&] { return progressao({ C, F, G, C, Am, F, G, C, Dm, G, C, C }, 2.0); };
        const std::vector<std::pair<double, double>> secs { { 0.0, 8.0 }, { 8.0, 16.0 }, { 16.0, 24.0 } };
        // nordestina: as técnicas de Siqueira aparecem; nenhuma cadência V → I;
        // a tônica do baião é I7 (o mixolídio)
        bool baiao = false, lidio = false, trimodal = false, repente = false, armorial = false, semVI = true, i7 = false;
        for (std::uint64_t sd = 1; sd <= 40; ++sd)
        {
            auto s = prog();
            std::vector<CadenceMark> cads;
            const auto line = writeHarmony(s, secs, 0.5, trioio::strategy::Mode::follow, L::northeastern, sd, &cads);
            baiao = baiao || line.find("baião") != std::string::npos;
            lidio = lidio || line.find("lídio (I7M") != std::string::npos;
            trimodal = trimodal || line.find("trimodal") != std::string::npos;
            repente = repente || line.find("repente") != std::string::npos;
            armorial = armorial || line.find("Guerra-Peixe") != std::string::npos;
            for (const auto& c : cads)
                for (std::size_t i = 1; i < s.size(); ++i)
                    if (std::abs(s[i].start - c.start) < 1e-6 && s[i - 1].chord.root == (c.key.tonic + 7) % 12 && s[i - 1].chord.quality == ChordQuality::major)
                        semVI = false;
            i7 = i7 || (s.back().chord.root == 0 && (s.back().chord.color & kSeventh) && s.back().chord.quality == ChordQuality::major);
        }
        std::printf("nordestina: baião %d, lídio %d, trimodal %d, repente %d, armorial %d, sem V→I %d, I7 %d\n", baiao, lidio, trimodal,
                    repente, armorial, semVI, i7);
        CHECK(baiao && lidio && trimodal && repente && armorial && semVI && i7);
        // mineira: o ♭II7M(♯11) e o Vm7
        bool bII = false, vm = false;
        for (std::uint64_t sd = 1; sd <= 20; ++sd)
        {
            auto s = prog();
            (void) writeHarmony(s, secs, 0.5, trioio::strategy::Mode::follow, L::mineira, sd);
            for (const auto& x : s)
            {
                bII = bII || ((x.chord.root == 1) && (x.chord.color & kSharpEleventh));
                vm = vm || (x.chord.root == 7 && x.chord.quality == ChordQuality::minor);
            }
        }
        CHECK(bII && vm);
        // Hermeto: todo acorde com cor; as chegadas cromáticas; o baixo próprio
        {
            auto s = prog();
            const auto line = writeHarmony(s, secs, 0.5, trioio::strategy::Mode::follow, L::hermeto, 3);
            std::printf("%s\n", line.c_str());
            bool cores = true, baixo = false;
            for (const auto& x : s)
            {
                cores = cores && x.chord.color != 0;
                baixo = baixo || x.bassPc >= 0;
            }
            CHECK(cores && baixo && line.find("chegadas cromáticas") != std::string::npos);
        }
    }
    // 11. o som da aura: uma "fala" sintética (a entonação que sobe e desce,
    //     ataques a cada 0,25 s) vira a melodia, dobrada, com um acorde por
    //     sílaba em que a nota da fala é uma cor (nunca a fundamental)
    {
        std::vector<trioio::perception::Frame> fr;
        std::vector<trioio::perception::Onset> on;
        for (int i = 0; i < 400; ++i)
        {
            trioio::perception::Frame f;
            f.time = i * 0.01;
            f.silent = false;
            f.rmsDb = -20.0f;
            f.periodicity = 0.9f;
            const int syl = i / 25;  // 0,25 s por sílaba
            static const float contour[] = { 62, 64, 65, 67, 65, 64, 62, 60, 62, 64, 67, 69, 67, 65, 64, 62 };
            f.f0Hz = static_cast<float>(440.0 * std::pow(2.0, (contour[syl % 16] - 69.0) / 12.0));
            fr.push_back(f);
        }
        for (int k = 0; k < 16; ++k)
            on.push_back({ k * 0.25, 1.0f });
        Score sc;
        sc.duration = 4.0;
        sc.notes.push_back({ 0.0, 4.0, 60.0f, 0.0f, 0.5f, 0, Timbre::sax });
        const auto line = applySomDaAura(sc, fr, on, 4.0, 7);
        std::printf("%s\n", line.c_str());
        int melodia = 0, dobra = 0, cor = 0, acordes = 0;
        for (const auto& n : sc.notes)
        {
            melodia += n.voice == 0 ? 1 : 0;
            dobra += n.voice == 2 ? 1 : 0;
        }
        // a nota da fala nunca é a fundamental do baixo que soa com ela
        for (const auto& m : sc.notes)
            if (m.voice == 0)
                for (const auto& b : sc.notes)
                    if (b.voice == 1 && std::abs(b.time - m.time) < 1e-6)
                    {
                        ++acordes;
                        cor += static_cast<int>(std::lround(m.pitch - b.pitch)) % 12 != 0 ? 1 : 0;
                    }
        CHECK(melodia == 16 && dobra == 16);
        CHECK(acordes >= 12 && cor == acordes);
    }
    // 12. as linguagens modernas e as dos compositores (fases 2 e 3): cada
    //     uma deixa a sua marca nos acordes
    {
        using L = HarmonyLanguage;
        // o nome e a leitura de todas as linguagens vão e voltam
        for (int k = 0; k < kHarmonyLanguages; ++k)
        {
            HarmonyLanguage back;
            const auto name = harmonyLanguageName(static_cast<L>(k));
            CHECK(parseHarmonyLanguage(name, back) || k == 0);
        }
        HarmonyLanguage parsed;
        CHECK(parseHarmonyLanguage("villa-lobos", parsed) && parsed == L::villaLobos);
        CHECK(parseHarmonyLanguage("guinga", parsed) && parsed == L::guinga);
        const auto prog = [&] { return progressao({ C, F, G, C, Am, F, G, C, Dm, G, C, C }, 2.0); };
        const std::vector<std::pair<double, double>> secs { { 0.0, 8.0 }, { 8.0, 16.0 }, { 16.0, 24.0 } };
        const auto any = [&](L lang, auto pred, const std::vector<SpectralProfile>* sp = nullptr) {
            for (std::uint64_t sd = 1; sd <= 30; ++sd)
            {
                auto s = prog();
                (void) writeHarmony(s, secs, 0.5, trioio::strategy::Mode::follow, lang, sd, nullptr, sp);
                for (const auto& x : s)
                    if (pred(x))
                        return true;
            }
            return false;
        };
        const auto all = [&](L lang, auto pred) {
            for (std::uint64_t sd = 1; sd <= 30; ++sd)
            {
                auto s = prog();
                (void) writeHarmony(s, secs, 0.5, trioio::strategy::Mode::follow, lang, sd);
                for (const auto& x : s)
                    if (!pred(x))
                        return false;
            }
            return true;
        };
        using Q = ChordQuality;
        const auto rootIs = [](int r) { return [r](const ChordSpan& x) { return x.chord.root == r; }; };
        // modal: o ♭VII maior (mixolídio/eólio) e o ♭II (frígio) aparecem
        CHECK(any(L::modal, rootIs(10)) && any(L::modal, rootIs(1)));
        // andaluza: o ♭VI e o V(♭9)
        CHECK(any(L::andalusian, [](const ChordSpan& x) { return x.chord.root == 7 && (x.chord.color & kFlatNinth); }));
        // cromática: a mediante (Lá♭ maior em Dó, pelo L) e a troca de modo (Dó menor, pelo P)
        CHECK(any(L::chromatic, [](const ChordSpan& x) { return x.chord.root == 8 && x.chord.quality == Q::major; }));
        CHECK(any(L::chromatic, [](const ChordSpan& x) { return x.chord.root == 0 && x.chord.quality == Q::minor; }));
        // quartal, Scriabin, politonal, octatônica: todo acorde com a sua cor
        CHECK(all(L::quartal, [](const ChordSpan& x) { return (x.chord.color & kSus4) != 0; }));
        CHECK(all(L::scriabin, [](const ChordSpan& x) {
            return (x.chord.color & (kSeventh | kNinth | kSharpEleventh | kThirteenth)) == (kSeventh | kNinth | kSharpEleventh | kThirteenth);
        }));
        CHECK(all(L::polytonal, [](const ChordSpan& x) { return (x.chord.color & kSharpEleventh) != 0; }));
        CHECK(all(L::octatonic, [](const ChordSpan& x) { return (x.chord.color & kFlatNinth) && x.chord.root % 3 == 0; }));
        // estática: poucos acordes distintos (a mesma fundamental, a cor mudando)
        CHECK(all(L::stasis, [](const ChordSpan& x) { return x.chord.root == 0; }));
        // Bartók: o 7(♯11) da escala acústica
        CHECK(any(L::bartok, [](const ChordSpan& x) { return (x.chord.color & kSeventh) && (x.chord.color & kSharpEleventh); }));
        // Villa-Lobos: o pedal no baixo ou a sequência de quintas
        CHECK(any(L::villaLobos, [](const ChordSpan& x) { return x.bassPc == 0 && (x.chord.color & kSus4); }));
        // Moacir Santos: o im7(9) ou o I7sus(9)
        CHECK(any(L::moacir, [](const ChordSpan& x) { return (x.chord.color & kNinth) && x.chord.root == 0; }));
        // Guinga: o 7M(♯5) e o m(7M)
        CHECK(any(L::guinga, [](const ChordSpan& x) { return x.chord.quality == Q::augmented && (x.chord.color & kMajorSeventh); }));
        CHECK(any(L::guinga, [](const ChordSpan& x) { return x.chord.quality == Q::minor && (x.chord.color & kMajorSeventh); }));
        // espectral: um corpo com o 7º parcial (≈ 7ª menor) e o 11º (≈ ♯11)
        // vira a cor 7(♯11)
        std::vector<SpectralProfile> corpo(1);
        corpo[0].start = 0.0;
        corpo[0].end = 24.0;
        corpo[0].ratios = { 1.0f, 2.0f, 3.0f, 7.0f, 11.0f };
        corpo[0].weights = { 0.4f, 0.2f, 0.2f, 0.1f, 0.1f };
        CHECK(any(L::spectral, [](const ChordSpan& x) { return (x.chord.color & kSeventh) && (x.chord.color & kSharpEleventh); }, &corpo));
        // frevo: as dominantes encadeadas (mais acordes que os lidos) e o V7(♭9) da chegada
        {
            auto s = prog();
            (void) writeHarmony(s, secs, 0.5, trioio::strategy::Mode::follow, L::frevo, 4);
            CHECK(s.size() > prog().size());
            CHECK(s.back().chord.root == 0 && s.back().chord.quality == Q::major);
        }
        CHECK(any(L::frevo, [](const ChordSpan& x) { return x.chord.root == 7 && (x.chord.color & kFlatNinth); }));
        // maracatu: o pedal da tônica em todo acorde; o ♭VII
        CHECK(all(L::maracatu, [](const ChordSpan& x) { return x.bassPc == 0; }));
        CHECK(any(L::maracatu, rootIs(10)));
        // Gismonti: o pedal e a 9ª
        CHECK(all(L::gismonti, [](const ChordSpan& x) { return x.bassPc == 0; }));
        CHECK(any(L::gismonti, [](const ChordSpan& x) { return x.chord.root == 8 && (x.chord.color & kMajorSeventh); }));
        // Partch: todo acorde é justo (microPcs com seis notas); a otonalidade
        // tem o 7º harmônico 968,8 cents acima da unidade (31 cents abaixo da
        // 7ª menor temperada) e a utonalidade o espelho
        {
            bool o = false, u = false, ok = true;
            for (std::uint64_t sd = 1; sd <= 10; ++sd)
            {
                auto s = prog();
                (void) writeHarmony(s, secs, 0.5, trioio::strategy::Mode::follow, L::partch, sd);
                for (const auto& x : s)
                {
                    ok = ok && x.microPcs.size() == 6;
                    if (x.microPcs.size() != 6)
                        continue;
                    const float seventh = std::fmod(x.microPcs[3] - x.microPcs[0] + 24.0f, 12.0f);
                    o = o || (x.microName[0] == 'O' && std::abs(seventh - 9.688f) < 0.01f);
                    u = u || (x.microName[0] == 'U' && std::abs(seventh - (12.0f - 9.688f)) < 0.01f);
                }
            }
            CHECK(ok && o && u);
        }
        std::printf("as linguagens modernas e dos compositores: ok\n");
    }
    // 13. o arquivo de cadências: todas legíveis, com fonte; a autêntica
    //     perfeita termina no I maior; a andaluza tem quatro acordes
    {
        std::ifstream f(TRIOIO_SOURCE_DIR "/data/harmonia/arquivo_trioio_cadencias.txt");
        std::stringstream ss;
        ss << f.rdbuf();
        std::vector<std::string> erros;
        const auto cs = parseCadenceArchive(ss.str(), erros);
        for (const auto& e : erros)
            std::fprintf(stderr, "arquivo de cadências: %s\n", e.c_str());
        CHECK(erros.empty() && cs.size() >= 40);
        bool perfeita = false, andaluza = false;
        for (const auto& c : cs)
        {
            CHECK(c.chords.size() == c.bassPcs.size() && !c.source.empty());
            if (c.name == "autêntica perfeita")
                perfeita = c.chords.back() == Chord { 0, ChordQuality::major } && c.chords[c.chords.size() - 2].root == 7;
            if (c.name == "andaluza")
                andaluza = c.chords.size() == 4 && (c.chords.back().color & kFlatNinth);
        }
        CHECK(perfeita && andaluza);
        // a saliência dada pelo autor é lida; a mal escrita é recusada
        {
            std::vector<std::string> e3;
            const auto x = parseCadenceArchive("= x\nacordes: 7 M 7 | 0 M\nfonte: y\nsaliência: 11:3, 2:1\n", e3);
            CHECK(e3.empty() && x.size() == 1 && x[0].salience.size() == 2 && x[0].salience.at(11) == 3);
            std::vector<std::string> e4;
            (void) parseCadenceArchive("= x\nacordes: 7 M | 0 M\nfonte: y\nsaliência: 13:2\n", e4);
            CHECK(!e4.empty());
        }
        // o leitor recusa o acorde mal escrito
        std::vector<std::string> e2;
        (void) parseCadenceArchive("= x\nacordes: 13 M\nfonte: y\n", e2);
        CHECK(!e2.empty());
        std::printf("arquivo de cadências: %zu cadências\n", cs.size());
        // 15. a mistura conceitual: da perfeita com a frígia modal emerge a
        //     substituição de trítono (♭II7 → I), como em Zacharakis,
        //     Kaliakatsos-Papakostas e Cambouropoulos (ISMIR 2015); a backdoor
        //     (♭VII7) aparece, mais fraca
        {
            const CadenceEntry* perf = nullptr;
            const CadenceEntry* frig = nullptr;
            for (const auto& c : cs)
            {
                if (c.name == "autêntica perfeita com cadencial 6/4") perf = &c;  // o penúltimo é o V7, como no artigo
                if (c.name == "frígia modal (século XV)") frig = &c;
            }
            CHECK(perf && frig);
            if (perf && frig)
            {
                const auto bl = blendCadences(*perf, *frig, 8);
                for (const auto& x : bl)
                    std::printf("  mistura: %s → %s (%.1f): %s\n", x.penultimate.name().c_str(), x.final.name().c_str(), x.score, x.kept.c_str());
                CHECK(!bl.empty() && bl[0].penultimate.root == 1 && bl[0].penultimate.quality == ChordQuality::major
                      && (bl[0].penultimate.color & kSeventh));
                bool backdoor = false;
                for (std::size_t k = 1; k < bl.size(); ++k)
                    backdoor = backdoor || (bl[k].penultimate.root == 10 && bl[k].penultimate.quality == ChordQuality::major);
                CHECK(backdoor);
            }
        }
        // 14. o sorteio: com chance 1, toda cadência marcada da linguagem
        //     andaluza vira a do arquivo (o V(♭9) no fim), transposta à tonalidade
        {
            auto s = progressao({ C, F, G, C, Am, F, G, C, Dm, G, C, C }, 2.0);
            const std::vector<std::pair<double, double>> secs { { 0.0, 8.0 }, { 8.0, 16.0 }, { 16.0, 24.0 } };
            std::vector<CadenceMark> cads;
            (void) writeHarmony(s, secs, 0.5, trioio::strategy::Mode::follow, HarmonyLanguage::andalusian, 2, &cads);
            const int n = drawArchiveCadences(s, cads, cs, HarmonyLanguage::andalusian, 0.5, 2, 1.0);
            CHECK(n == static_cast<int>(cads.size()) && n > 0);
            for (const auto& c : cads)
                for (const auto& x : s)
                    if (std::abs(x.end - c.end) < 1e-6)
                        CHECK(x.chord.root == (c.key.tonic + 7) % 12 && (x.chord.color & kFlatNinth));
            bool contiguo = true;
            for (std::size_t i = 1; i < s.size(); ++i)
                contiguo = contiguo && std::abs(s[i].start - s[i - 1].end) < 1e-6;
            CHECK(contiguo && std::abs(s.back().end - 24.0) < 1e-6);
            // uma linguagem sem cadência no arquivo não troca nada
            std::vector<CadenceMark> none = cads;
            CHECK(drawArchiveCadences(s, none, cs, HarmonyLanguage::clusters, 0.5, 2, 1.0) == 0);
        }
    }
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("harmonia escrita: todas as checagens passaram");
    return EXIT_SUCCESS;
}
