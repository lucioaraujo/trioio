#include "StyleFigures.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace trioio::composition
{
namespace
{
// o subestilo do rock desta composição (RockStyle): posto na entrada de
// applyStyleFigures e applyStylePhrasing, lido pelos padrões; por thread,
// como o compasso (barSteps), para composições em paralelo
thread_local RockStyle tRock = RockStyle::classic;

struct Hit
{
    int step;      // semicolcheia do compasso
    double dur;    // em semicolcheias
    float vel;     // multiplicador
};

// os acordes da voz de harmonia: as notas que começam juntas (±30 ms)
struct Block
{
    double start, end;
    std::vector<float> pitches;
    float velocity;
    Timbre timbre;
};

std::vector<Block> harmonyBlocks(const Score& s)
{
    std::vector<const Note*> h;
    for (const auto& n : s.notes)
        if (n.voice == 6)
            h.push_back(&n);
    std::sort(h.begin(), h.end(), [](const Note* a, const Note* b) { return a->time < b->time; });
    std::vector<Block> out;
    for (const Note* n : h)
    {
        if (!out.empty() && std::abs(n->time - out.back().start) < 0.03)
        {
            out.back().pitches.push_back(n->pitch);
            out.back().end = std::max(out.back().end, n->time + n->duration);
            continue;
        }
        out.push_back({ n->time, n->time + n->duration, { n->pitch }, n->velocity, n->timbre });
    }
    return out;
}

bool inPulse(const std::vector<std::pair<double, double>>& spans, double a, double b)
{
    return std::any_of(spans.begin(), spans.end(), [&](const auto& r) { return a >= r.first - 1e-6 && b <= r.second + 1e-6; });
}

// o padrão de harmonia de um compasso (`bar` = índice do compasso, para os
// padrões de dois compassos e as variações)
std::vector<Hit> harmonyPattern(Ensemble e, int bar)
{
    switch (e)
    {
        case Ensemble::manouche:  // a pompe
            return { { 0, 1.4, 0.55f }, { 4, 0.6, 0.85f }, { 8, 1.4, 0.55f }, { 12, 0.6, 0.85f } };
        case Ensemble::jazz:
        {
            // Charleston e as suas variações (o comping não repete igual)
            static const std::vector<std::vector<Hit>> v {
                { { 0, 1.5, 0.7f }, { 6, 1.2, 0.6f } },
                { { 6, 1.2, 0.65f }, { 14, 1.2, 0.6f } },
                { { 2, 1.2, 0.6f }, { 10, 1.5, 0.65f } },
                { { 0, 1.5, 0.7f }, { 10, 1.2, 0.6f } },
            };
            return v[static_cast<std::size_t>((bar * 7 + 3) % 4)];
        }
        case Ensemble::samba:  // a batida do cavaquinho (partido-alto)
        case Ensemble::escola:
            return { { 0, 0.8, 0.75f }, { 3, 0.8, 0.6f }, { 6, 0.8, 0.7f }, { 8, 0.8, 0.6f }, { 11, 0.8, 0.7f }, { 14, 0.8, 0.6f } };
        case Ensemble::choro:  // o centro do cavaquinho
            return { { 0, 0.7, 0.7f }, { 2, 0.7, 0.5f }, { 3, 0.7, 0.6f }, { 5, 0.7, 0.55f }, { 7, 0.7, 0.6f },
                     { 8, 0.7, 0.7f }, { 10, 0.7, 0.5f }, { 11, 0.7, 0.6f }, { 13, 0.7, 0.55f }, { 15, 0.7, 0.6f } };
        case Ensemble::bolero:  // colcheias 1, 2, 2e, 3, 4
            return { { 0, 1.6, 0.7f }, { 4, 1.4, 0.5f }, { 6, 1.4, 0.6f }, { 8, 1.6, 0.65f }, { 12, 1.4, 0.55f } };
        case Ensemble::forro:  // a mão direita da sanfona nos contratempos (o baião)
            return { { 2, 1.2, 0.65f }, { 6, 1.2, 0.6f }, { 10, 1.2, 0.65f }, { 14, 1.2, 0.6f } };
        case Ensemble::tango:
            // o marcato em quatro, e no compasso seguinte a síncopa 3-3-2
            if (bar % 2 == 0)
                return { { 0, 1.5, 0.85f }, { 4, 1.5, 0.6f }, { 8, 1.5, 0.8f }, { 12, 1.5, 0.6f } };
            return { { 0, 2.0, 0.85f }, { 6, 2.0, 0.8f }, { 12, 2.0, 0.75f } };
        case Ensemble::rock:
            switch (tRock)
            {
                case RockStyle::punk:  // colcheias iguais, todas para baixo, fortes
                    return { { 0, 1.7, 0.85f }, { 2, 1.7, 0.8f }, { 4, 1.7, 0.85f }, { 6, 1.7, 0.8f },
                             { 8, 1.7, 0.85f }, { 10, 1.7, 0.8f }, { 12, 1.7, 0.85f }, { 14, 1.7, 0.8f } };
                case RockStyle::heavy:  // o chug: semicolcheias abafadas, o acento no tempo
                {
                    std::vector<Hit> v;
                    for (int k = 0; k < 16; ++k)
                        v.push_back({ k, 0.7, k % 4 == 0 ? 0.9f : (k % 2 ? 0.55f : 0.68f) });
                    return v;
                }
                case RockStyle::blues:  // o boogie em shuffle: longo-curto
                    return { { 0, 2.4, 0.8f }, { 3, 0.9, 0.55f }, { 4, 2.4, 0.75f }, { 7, 0.9, 0.55f },
                             { 8, 2.4, 0.8f }, { 11, 0.9, 0.55f }, { 12, 2.4, 0.75f }, { 15, 0.9, 0.55f } };
                case RockStyle::prog:  // 3 + 3 + 2, o acento deslocado; no outro compasso, 2 + 3 + 3
                    if (bar % 2 == 0)
                        return { { 0, 2.6, 0.85f }, { 3, 2.6, 0.75f }, { 6, 1.6, 0.7f }, { 8, 2.6, 0.85f }, { 11, 2.6, 0.75f }, { 14, 1.6, 0.7f } };
                    return { { 0, 1.6, 0.85f }, { 2, 2.6, 0.75f }, { 5, 2.6, 0.7f }, { 8, 1.6, 0.85f }, { 10, 2.6, 0.75f }, { 13, 2.6, 0.7f } };
                case RockStyle::postrock:  // o acorde aberto, longo, deixado soar; a cada dois compassos
                    if (bar % 2 == 0)
                        return { { 0, 30.0, 0.6f } };
                    return {};
                case RockStyle::psychedelic:  // o acorde que se arrasta, e a resposta no "e" do 3
                    return { { 0, 9.0, 0.7f }, { 10, 6.0, 0.6f } };
                case RockStyle::noise:  // a palhetada em trêmulo: semicolcheias que não param, a força irregular
                {
                    std::vector<Hit> v;
                    for (int k = 0; k < 16; ++k)
                        v.push_back({ k, 1.0, 0.55f + 0.35f * static_cast<float>(((bar * 7 + k * 13) % 11)) / 10.0f });
                    return v;
                }
                case RockStyle::classic:
                default:  // colcheias com a mão abafando; o tempo acentuado
                    return { { 0, 1.6, 0.8f }, { 2, 1.6, 0.55f }, { 4, 1.6, 0.75f }, { 6, 1.6, 0.55f },
                             { 8, 1.6, 0.8f }, { 10, 1.6, 0.55f }, { 12, 1.6, 0.75f }, { 14, 1.6, 0.55f } };
            }
        case Ensemble::charanga:  // o montuno, em dois compassos
        case Ensemble::salsa:
            if (bar % 2 == 0)
                return { { 2, 1.6, 0.7f }, { 6, 1.6, 0.65f }, { 8, 1.6, 0.7f }, { 12, 1.6, 0.65f } };
            return { { 0, 1.6, 0.7f }, { 4, 1.6, 0.65f }, { 6, 1.6, 0.7f }, { 10, 1.6, 0.65f }, { 14, 1.6, 0.65f } };
        default: return {};
    }
}

// a percussão de cada estilo: (semicolcheia, timbre, duração em
// semicolcheias, força, altura). A altura escolhe o tambor do par (grave ou
// agudo) e, no pandeiro, o polegar (forte, grave) ou a ponta dos dedos.
struct PercHit
{
    int step;
    Timbre timbre;
    double dur;
    float vel;
    float pitch;
};

std::vector<PercHit> percussionPattern(Ensemble e, int bar)
{
    using T = Timbre;
    switch (e)
    {
        case Ensemble::jazz:
        {
            // o prato de condução ("ding, ding-a, ding, ding-a", com swing), o
            // chimbal no 2 e no 4, a escovinha varrendo no 2 e no 4 e o bumbo
            // "feathered" (quase inaudível) nos quatro tempos
            std::vector<PercHit> v { { 0, T::ride, 4, 0.6f, 70 }, { 4, T::ride, 2, 0.7f, 70 }, { 6, T::ride, 2, 0.5f, 70 },
                                     { 8, T::ride, 4, 0.6f, 70 }, { 12, T::ride, 2, 0.7f, 70 }, { 14, T::ride, 2, 0.5f, 70 },
                                     { 4, T::hihat, 0.3, 0.5f, 70 }, { 12, T::hihat, 0.3, 0.5f, 70 },
                                     { 4, T::brushes, 1, 0.35f, 60 }, { 12, T::brushes, 1, 0.35f, 60 } };
            for (int b = 0; b < 16; b += 4)
                v.push_back({ b, T::kick, 1, 0.18f, 36 });
            // de vez em quando a caixa comenta (o "comping" da bateria)
            if (bar % 4 == 3)
                v.push_back({ 14, T::snare, 0.5, 0.3f, 60 });
            return v;
        }
        case Ensemble::salsa:
        case Ensemble::charanga:
        {
            // a conga em tumbao (o tapa no 2, os tons abertos no 4 e no 4 e),
            // o bongô em martillo (colcheias, o tempo acentuado), o guiro
            std::vector<PercHit> v { { 4, T::conga, 1, 0.9f, 52 }, { 12, T::conga, 1, 0.6f, 50 }, { 14, T::conga, 1, 0.6f, 47 } };
            if (e == Ensemble::salsa)
                for (int k = 0; k < 16; k += 2)
                    v.push_back({ k, T::bongo, 1, k % 4 == 0 ? 0.65f : 0.35f, k % 8 == 6 ? 64.0f : 68.0f });
            v.push_back({ 0, T::guiro, 3, 0.5f, 70 });
            v.push_back({ 4, T::guiro, 1, 0.4f, 70 });
            v.push_back({ 6, T::guiro, 1, 0.4f, 70 });
            v.push_back({ 8, T::guiro, 3, 0.5f, 70 });
            v.push_back({ 12, T::guiro, 1, 0.4f, 70 });
            v.push_back({ 14, T::guiro, 1, 0.4f, 70 });
            return v;
        }
        case Ensemble::forro:
        {
            // o baião, em cada metade do compasso: a maceta da zabumba no 1 e
            // no "1 a" (a pontuada), o bacalhau nos contratempos; o triângulo
            // em semicolcheias, aberto no contratempo ("tin") e abafado no resto
            std::vector<PercHit> v;
            for (int h = 0; h < 16; h += 8)
            {
                v.push_back({ h + 0, T::zabumba, 2, 0.9f, 40 });
                v.push_back({ h + 3, T::zabumba, 2, 0.75f, 40 });
                v.push_back({ h + 2, T::zabumba, 0.5, 0.55f, 70 });
                v.push_back({ h + 6, T::zabumba, 0.5, 0.6f, 70 });
                for (int k = 0; k < 8; ++k)
                {
                    const bool open = k == 2 || k == 6;
                    v.push_back({ h + k, T::triangle, open ? 2.0 : 0.3, open ? 0.6f : 0.35f, 80 });
                }
            }
            return v;
        }
        case Ensemble::rock:
        {
            const bool fill = bar % 8 == 7, top = bar % 8 == 0;
            const float tomPitch[] = { 55.0f, 52.0f, 47.0f, 43.0f };
            switch (tRock)
            {
                case RockStyle::punk:
                {
                    // o bumbo em todos os tempos, a caixa nos contratempos (a
                    // "skank" do punk), o chimbal aberto e o ataque a cada 4
                    std::vector<PercHit> v;
                    for (int k = 0; k < 16; k += 4)
                        v.push_back({ k, T::kick, 1, 0.9f, 36 });
                    for (int k = 2; k < 16; k += 4)
                        v.push_back({ k, T::snare, 1, 0.85f, 60 });
                    for (int k = 0; k < 16; k += 2)
                        v.push_back({ k, T::hihat, 1.2, 0.55f, 70 });
                    if (bar % 4 == 0)
                        v.push_back({ 0, T::crash, 8, 0.85f, 70 });
                    return v;
                }
                case RockStyle::heavy:
                {
                    // o bumbo duplo em semicolcheias, a caixa no 2 e no 4, o
                    // prato de condução nos tempos (o pesado)
                    std::vector<PercHit> v;
                    for (int k = 0; k < 16; ++k)
                        v.push_back({ k, T::kick, 1, k % 4 == 0 ? 0.9f : 0.62f, 34 });
                    v.push_back({ 4, T::snare, 1, 0.95f, 60 });
                    v.push_back({ 12, T::snare, 1, 0.95f, 60 });
                    for (int k = 0; k < 16; k += 4)
                        v.push_back({ k, T::crash, 2, 0.45f, 70 });
                    if (fill)
                        for (int k = 8; k < 16; ++k)
                            v.push_back({ k, T::tom, 1, 0.8f, tomPitch[(k - 8) / 2] });
                    return v;
                }
                case RockStyle::blues:
                {
                    // o shuffle: o chimbal longo-curto, o bumbo no 1 e no 3, a
                    // caixa no 2 e no 4
                    std::vector<PercHit> v { { 0, T::kick, 1, 0.85f, 36 }, { 8, T::kick, 1, 0.8f, 36 },
                                             { 4, T::snare, 1, 0.8f, 60 }, { 12, T::snare, 1, 0.8f, 60 } };
                    for (int k : { 0, 3, 4, 7, 8, 11, 12, 15 })
                        v.push_back({ k, T::hihat, 0.4, k % 4 == 0 ? 0.6f : 0.4f, 70 });
                    if (top)
                        v.push_back({ 0, T::crash, 8, 0.7f, 70 });
                    return v;
                }
                case RockStyle::prog:
                {
                    // 3 + 3 + 2: o bumbo marca os grupos, a caixa no fim do
                    // segundo grupo, o prato de condução em colcheias
                    std::vector<PercHit> v { { 0, T::kick, 1, 0.9f, 36 }, { 3, T::kick, 1, 0.75f, 36 },
                                             { 6, T::snare, 1, 0.85f, 60 }, { 8, T::kick, 1, 0.9f, 36 },
                                             { 11, T::kick, 1, 0.75f, 36 }, { 14, T::snare, 1, 0.85f, 60 } };
                    for (int k = 0; k < 16; k += 2)
                        v.push_back({ k, T::ride, 2, k % 3 == 0 ? 0.6f : 0.45f, 70 });
                    if (fill)
                        for (int k = 10; k < 16; ++k)
                            v.push_back({ k, T::tom, 1, 0.7f, tomPitch[(k - 10) / 2 % 4] });
                    return v;
                }
                case RockStyle::postrock:
                {
                    // esparso: o bumbo no 1, o tom grave no 3, o prato de
                    // condução em semínimas; a cada quatro compassos, o rufo
                    // que cresce na caixa até o ataque
                    std::vector<PercHit> v { { 0, T::kick, 1, 0.7f, 36 }, { 8, T::tom, 2, 0.5f, 43 } };
                    for (int k = 0; k < 16; k += 4)
                        v.push_back({ k, T::ride, 4, 0.4f, 70 });
                    if (bar % 4 == 3)
                        for (int k = 8; k < 16; ++k)
                            v.push_back({ k, T::snare, 1, 0.35f + 0.07f * (k - 8), 60 });
                    if (bar % 4 == 0)
                        v.push_back({ 0, T::crash, 12, 0.7f, 70 });
                    return v;
                }
                case RockStyle::psychedelic:
                {
                    // o groove nos tons: o bumbo no 1 e no "2 e", a caixa leve no
                    // 2 e no 4, o surdo do tom no fim, o chimbal aberto
                    std::vector<PercHit> v { { 0, T::kick, 1, 0.85f, 36 }, { 6, T::kick, 1, 0.7f, 36 },
                                             { 10, T::kick, 1, 0.7f, 36 }, { 4, T::snare, 1, 0.6f, 60 },
                                             { 12, T::snare, 1, 0.6f, 60 }, { 14, T::tom, 1, 0.65f, 47 }, { 15, T::tom, 1, 0.6f, 43 } };
                    for (int k = 0; k < 16; k += 2)
                        v.push_back({ k, T::hihat, 1.5, 0.45f, 70 });
                    if (top)
                        v.push_back({ 0, T::crash, 8, 0.75f, 70 });
                    return v;
                }
                case RockStyle::noise:
                {
                    // a bateria que se desmancha: o ataque em quase todo tempo,
                    // os tons em lugares que mudam a cada compasso, o bumbo e a
                    // caixa juntos
                    std::vector<PercHit> v;
                    for (int k = 0; k < 16; k += 4)
                    {
                        v.push_back({ k, T::crash, 4, 0.7f, 70 });
                        v.push_back({ k, T::kick, 1, 0.85f, 34 });
                    }
                    for (int k = 0; k < 16; ++k)
                        if ((bar * 5 + k * 7) % 6 == 0)
                            v.push_back({ k, k % 2 ? T::snare : T::tom, 1, 0.75f, tomPitch[(bar + k) % 4] });
                    return v;
                }
                case RockStyle::classic:
                default: break;
            }
            // o clássico: o bumbo no 1, no 3 e no "3 e"; a caixa no 2 e no 4; o
            // chimbal em colcheias; o prato de ataque na cabeça de cada frase de
            // oito compassos; no oitavo, a virada nos tons (do agudo ao grave)
            std::vector<PercHit> v { { 0, T::kick, 1, 0.9f, 36 }, { 8, T::kick, 1, 0.85f, 36 }, { 10, T::kick, 1, 0.7f, 36 },
                                     { 4, T::snare, 1, 0.85f, 60 } };
            if (!fill)
                v.push_back({ 12, T::snare, 1, 0.85f, 60 });
            for (int k = 0; k < (fill ? 8 : 16); k += 2)
                v.push_back({ k, T::hihat, 0.3, k % 4 == 0 ? 0.6f : 0.42f, 70 });
            if (top)
                v.push_back({ 0, T::crash, 8, 0.8f, 70 });
            if (fill)
            {
                for (int k = 0; k < 8; ++k)
                    v.push_back({ 8 + k, T::tom, 1, 0.7f + 0.03f * k, tomPitch[k / 2] });
            }
            return v;
        }
        case Ensemble::escola:
        {
            // a bateria, em cada metade do compasso (o samba é em dois):
            // o surdo de segunda no 1 e o de primeira (o mais grave) no 2 — a
            // marcação; o de terceira (o cortador) recorta entre eles; a
            // caixa em semicolcheias acentuadas; o repique; o tamborim no
            // telecoteco; o agogô nas duas campânulas; a cuíca no "uó"; o
            // chocalho contínuo. A cada oito compassos o repique chama.
            std::vector<PercHit> v;
            for (int h = 0; h < 16; h += 8)
            {
                v.push_back({ h + 0, T::surdo, 2, 0.75f, 38 });   // segunda (~73 Hz)
                v.push_back({ h + 4, T::surdo, 3, 0.95f, 33 });   // primeira (~55 Hz)
                v.push_back({ h + 2, T::surdo, 0.5, 0.55f, 43 }); // terceira (~98 Hz)
                v.push_back({ h + 3, T::surdo, 0.5, 0.5f, 43 });
                v.push_back({ h + 6, T::surdo, 1, 0.6f, 43 });
                v.push_back({ h + 3, T::repique, 0.5, 0.45f, 62 });
                v.push_back({ h + 6, T::repique, 0.5, 0.5f, 62 });
            }
            for (int k = 0; k < 16; ++k)
            {
                v.push_back({ k, T::snare, 0.5, k % 4 == 3 ? 0.55f : k % 4 == 0 ? 0.45f : 0.28f, 60 });
                v.push_back({ k, T::chocalho, 0.5, k % 4 == 3 ? 0.55f : 0.3f, 80 });
            }
            // o tamborim no telecoteco (dois compassos de 2/4)
            for (int k : { 0, 2, 3, 5, 7, 9, 10, 12, 14 })
                v.push_back({ k, T::tamborim, 0.5, 0.7f, 76 });
            // o agogô: aguda, grave
            for (const auto& [k, hi] : std::vector<std::pair<int, bool>> { { 0, true }, { 3, false }, { 6, true }, { 8, false },
                                                                            { 10, true }, { 13, false } })
                v.push_back({ k, T::agogo, 1, 0.55f, hi ? 74.0f : 66.0f });
            // a cuíca: sobe no 2 e desce no "4"
            v.push_back({ 4, T::cuica, 2, 0.6f, 64 });
            v.push_back({ 12, T::cuica, 2, 0.55f, 63 });
            // a forma da bateria em frases de 16 compassos:
            //   - no 15º, o repique chama (a segunda metade em semicolcheias);
            //   - no 16º, a PARADINHA: a bateria para na segunda metade e só
            //     acentua a bossa em uníssono (1, "2 a" e 4 da metade): surdos,
            //     caixa, repique e tamborim juntos; o canto continua;
            //   - no 1º da frase seguinte, a volta: os três surdos juntos no 1.
            const int phrase = ((bar % 16) + 16) % 16;
            if (phrase == 14)
                for (int k = 8; k < 16; ++k)
                    v.push_back({ k, T::repique, 0.5, 0.8f, 64 });
            if (phrase == 15)
            {
                v.erase(std::remove_if(v.begin(), v.end(), [](const PercHit& h) { return h.step >= 8; }), v.end());
                for (int k : { 8, 11, 14 })
                    for (const auto& [tb, p] : std::vector<std::pair<T, float>> { { T::surdo, 33.0f }, { T::surdo, 38.0f }, { T::surdo, 43.0f },
                                                                                  { T::snare, 60.0f }, { T::repique, 64.0f }, { T::tamborim, 76.0f } })
                        v.push_back({ k, tb, 0.6, 0.95f, p });
            }
            if (phrase == 0 && bar >= 16)  // a volta (só depois de uma paradinha)
                for (float p : { 33.0f, 38.0f, 43.0f })
                    v.push_back({ 0, T::surdo, 2, 1.0f, p });
            return v;
        }
        case Ensemble::samba:
        {
            // o surdo (abafado no 1, aberto no 2, em cada metade), o pandeiro
            // em semicolcheias (o polegar no tempo, o acento na quarta) e o
            // ganzá contínuo
            std::vector<PercHit> v { { 0, T::surdo, 0.5, 0.6f, 40 }, { 4, T::surdo, 3, 0.9f, 40 },
                                     { 8, T::surdo, 0.5, 0.6f, 40 }, { 12, T::surdo, 3, 0.9f, 40 } };
            for (int k = 0; k < 16; ++k)
            {
                const bool thumb = k % 4 == 0, accent = k % 4 == 3;
                v.push_back({ k, T::pandeiro, 0.8, thumb ? 0.75f : accent ? 0.55f : 0.3f, thumb ? 55.0f : 62.0f });
                v.push_back({ k, T::shaker, 0.8, accent ? 0.5f : 0.25f, 80 });
            }
            // o breque a cada 8 compassos: a segunda metade para, e só o
            // surdo e o pandeiro marcam juntos o 1 e o "2 a"
            if (((bar % 8) + 8) % 8 == 7)
            {
                v.erase(std::remove_if(v.begin(), v.end(), [](const PercHit& h) { return h.step >= 8; }), v.end());
                for (int k : { 8, 11 })
                {
                    v.push_back({ k, T::surdo, 0.6, 0.9f, 40 });
                    v.push_back({ k, T::pandeiro, 0.6, 0.85f, 55 });
                }
            }
            return v;
        }
        default: return {};
    }
}

} // namespace

// A cor dos acordes do estilo (2026-10-16, "não soa como jazz"): a tríade lida
// vira o acorde do idioma. No jazz, as disposições sem fundamental do piano
// (o baixo já a toca): o maior vira maj7(9), ou dominante 7(9,13) quando o
// próximo acorde está uma quarta acima (V → I); o menor, m7(9); o
// diminuto, m7(♭5). No manouche, os acordes de Django: o 6 e o m6. A
// disposição fica em volta do Dó central (a nota de baixo entre Ré 3 e Lá 3).
// M. Levine, The Jazz Piano Book, Sher Music, 1989 (as disposições A/B sem
// fundamental); M. Dregni, Django, 2004.
std::vector<float> styleVoicing(Ensemble e, const std::vector<float>& triad, int nextRootPc)
{
    const bool colored = e == Ensemble::jazz || e == Ensemble::manouche || e == Ensemble::choro || e == Ensemble::samba
                         || e == Ensemble::escola || e == Ensemble::bolero || e == Ensemble::tango || e == Ensemble::salsa
                         || e == Ensemble::charanga || e == Ensemble::forro;
    if (triad.empty() || !colored)
        return triad;
    std::array<bool, 12> has {};
    for (float p : triad)
        has[static_cast<std::size_t>(((static_cast<int>(std::lround(p)) % 12) + 12) % 12)] = true;
    int root = -1, quality = -1;  // 0 maior, 1 menor, 2 diminuto
    for (int r = 0; r < 12 && root < 0; ++r)
        if (has[static_cast<std::size_t>(r)])
        {
            const auto h = [&](int i) { return has[static_cast<std::size_t>((r + i) % 12)]; };
            if (h(4) && h(7)) { root = r; quality = 0; }
            else if (h(3) && h(7)) { root = r; quality = 1; }
            else if (h(3) && h(6)) { root = r; quality = 2; }
        }
    if (root < 0)
        return triad;
    const bool dominant = quality == 0 && nextRootPc >= 0 && (nextRootPc - root + 12) % 12 == 5;
    std::vector<int> iv;
    // a nota de baixo da disposição, pelo instrumento que toca a harmonia
    int lowMin = 50, lowMax = 57;  // o piano do jazz e do bolero
    switch (e)
    {
        case Ensemble::choro:
        case Ensemble::samba:
        case Ensemble::escola:
            // a cor do choro e do samba (o cavaquinho): a dominante com 7ª e 9ª
            // antes da resolução, o maior com 6ª (a tônica do samba), o menor com 7ª
            iv = dominant ? std::vector<int> { 0, 4, 10, 14 } : quality == 0 ? std::vector<int> { 0, 4, 7, 9 }
               : quality == 1 ? std::vector<int> { 0, 3, 7, 10 } : std::vector<int> { 0, 3, 6, 9 };  // o diminuto de passagem
            lowMin = 59;
            lowMax = 66;
            break;
        case Ensemble::bolero:
            // o romantismo do bolero: maj7, a dominante com 9ª, o menor com 7ª e 9ª
            iv = dominant ? std::vector<int> { 4, 10, 14, 19 } : quality == 0 ? std::vector<int> { 4, 11, 14, 19 }
               : quality == 1 ? std::vector<int> { 3, 10, 14, 19 } : std::vector<int> { 3, 6, 10 };
            break;
        case Ensemble::tango:
            // o tango: a dominante com 7ª, o menor com 6ª
            iv = dominant ? std::vector<int> { 0, 4, 7, 10 } : quality == 1 ? std::vector<int> { 0, 3, 7, 9 }
               : quality == 0 ? std::vector<int> { 0, 4, 7, 12 } : std::vector<int> { 0, 3, 6, 9 };
            lowMin = 52;
            lowMax = 59;
            break;
        case Ensemble::salsa:
        case Ensemble::charanga:
        case Ensemble::forro:
            // a dominante com 7ª; o resto em tríade com a oitava (o montuno, a sanfona)
            iv = dominant ? std::vector<int> { 0, 4, 7, 10 } : quality == 0 ? std::vector<int> { 0, 4, 7, 12 }
               : quality == 1 ? std::vector<int> { 0, 3, 7, 12 } : std::vector<int> { 0, 3, 6, 9 };
            lowMin = 55;
            lowMax = 62;
            break;
        default: break;
    }
    // a faixa tem uma oitava inteira (com menos, alguma fundamental não cabe
    // e a disposição caía fora: o Dó do tango ia ao Dó 3)
    lowMax = lowMin + 11;
    if (!iv.empty())
    {
        int base = 48 + root;
        while (base + iv.front() < lowMin)
            base += 12;
        while (base + iv.front() > lowMax)
            base -= 12;
        std::vector<float> out;
        for (int i : iv)
            out.push_back(static_cast<float>(base + i));
        return out;
    }
    if (e == Ensemble::manouche)
        iv = quality == 0 ? std::vector<int> { 0, 4, 7, 9 } : quality == 1 ? std::vector<int> { 0, 3, 7, 9 } : std::vector<int> { 0, 3, 6, 9 };
    else if (quality == 0)
        iv = nextRootPc >= 0 && (nextRootPc - root + 12) % 12 == 5 ? std::vector<int> { 4, 10, 14, 21 }   // 3 7 9 13 (dominante)
                                                                  : std::vector<int> { 4, 11, 14, 19 };  // 3 7 9 5 (maj7)
    else if (quality == 1)
        iv = { 3, 10, 14, 19 };  // ♭3 ♭7 9 5 (m7)
    else
        iv = { 3, 6, 10, 15 };   // ♭3 ♭5 ♭7 ♭9… (m7♭5, com a 9ª menor no topo)
    // a nota de baixo da disposição entre Ré 3 (50) e Dó♯ 4 (61): uma oitava
    // inteira, para toda fundamental caber
    int base = 48 + root;
    while (base + iv.front() < 50)
        base += 12;
    while (base + iv.front() > 61)
        base -= 12;
    std::vector<float> out;
    for (int i : iv)
        out.push_back(static_cast<float>(base + i));
    return out;
}

namespace
{

bool hasFigures(Ensemble e)
{
    return e == Ensemble::manouche || e == Ensemble::jazz || e == Ensemble::samba || e == Ensemble::choro
           || e == Ensemble::bolero || e == Ensemble::charanga || e == Ensemble::salsa || e == Ensemble::rock
           || e == Ensemble::escola || e == Ensemble::forro || e == Ensemble::tango;
}

bool swung(Ensemble e) { return e == Ensemble::jazz || e == Ensemble::manouche; }

// a classe de altura mais próxima de `near` (em semitons MIDI)
float nearestPc(float pc, float near)
{
    float best = pc, dist = 1e9f;
    for (int o = -2; o <= 10; ++o)
    {
        const float p = pc + 12.0f * static_cast<float>(o);
        if (std::abs(p - near) < dist)
        {
            dist = std::abs(p - near);
            best = p;
        }
    }
    return best;
}
} // namespace

std::string applyStyleFigures(Score& score, Ensemble e, const perception::Pulse& pulse,
                              const std::vector<std::pair<double, double>>& pulsedSpans, int rockStyle)
{
    tRock = static_cast<RockStyle>(std::clamp(rockStyle, 0, kRockStyles - 1));
    if (!hasFigures(e) || pulse.period <= 0.0 || pulsedSpans.empty())
        return {};
    const double beat = pulse.period, step = beat / 4.0;
    const int beats = std::clamp(pulse.beatsPerBar, 2, 7);
    const double barLen = beat * beats;
    const int barSteps = beats * 4;
    const auto barOf = [&](double t) { return static_cast<int>(std::floor((t - pulse.phase) / barLen)); };
    const auto at = [&](int bar, double s) {
        // o swing de tercina nas colcheias do contratempo (jazz, manouche)
        double x = s;
        if (swung(e) && std::fmod(s, 4.0) == 2.0)
            x += 0.667;
        return pulse.phase + bar * barLen + x * step;
    };

    // ---- a harmonia ---------------------------------------------------------
    const auto blocks = harmonyBlocks(score);
    std::vector<Note> add;
    int harmonyHits = 0;
    std::vector<std::pair<double, double>> rewrittenH;
    for (const auto& b : blocks)
    {
        // fora do pulso (ou acorde curto), a harmonia fica sustentada; no jazz
        // e no manouche ela ainda ganha a cor do estilo (os voicings)
        const bool pulsedBlock = inPulse(pulsedSpans, b.start, b.end) && b.end - b.start >= beat;
        // (com o rock e os sem figura, sem pulso o acorde fica como veio)
        if (!pulsedBlock && (e == Ensemble::rock || !hasFigures(e)))
            continue;
        rewrittenH.push_back({ b.start, b.end });
        // no rock, o acorde vira power chord: a fundamental (a nota mais
        // grave do acorde, entre Mi 2 e Mi 3), a quinta e a oitava
        std::vector<float> chordPitches = b.pitches;
        if (!chordPitches.empty())
        {
            // a fundamental do próximo acorde (para reconhecer o V → I)
            int nextRoot = -1;
            for (const auto& nb : blocks)
                if (nb.start >= b.end - 0.05 && !nb.pitches.empty())
                {
                    nextRoot = ((static_cast<int>(std::lround(*std::min_element(nb.pitches.begin(), nb.pitches.end()))) % 12) + 12) % 12;
                    break;
                }
            chordPitches = styleVoicing(e, chordPitches, nextRoot);
        }
        if (e == Ensemble::rock && !chordPitches.empty())
        {
            float p0 = *std::min_element(chordPitches.begin(), chordPitches.end());
            // o registro: o pesado mais grave (afinação baixa), o post-rock no médio-agudo
            const float lo = tRock == RockStyle::heavy ? 35.0f : tRock == RockStyle::postrock ? 50.0f : 40.0f;
            while (p0 > lo + 12.0f)
                p0 -= 12.0f;
            while (p0 < lo)
                p0 += 12.0f;
            const bool minorChord = chordPitches.size() >= 2 && std::fmod(chordPitches[1] - chordPitches[0] + 24.0f, 12.0f) < 3.5f;
            switch (tRock)
            {
                case RockStyle::blues:  // a 7ª do blues no power chord
                    chordPitches = { p0, p0 + 7.0f, p0 + 10.0f };
                    break;
                case RockStyle::prog:  // quartas empilhadas
                    chordPitches = { p0, p0 + 5.0f, p0 + 10.0f, p0 + 15.0f };
                    break;
                case RockStyle::postrock:  // aberto, com a 9ª e a terça em cima
                    chordPitches = { p0, p0 + 7.0f, p0 + 14.0f, p0 + (minorChord ? 15.0f : 16.0f) };
                    break;
                case RockStyle::psychedelic:  // o acorde inteiro com a 7ª
                    chordPitches = { p0, p0 + 7.0f, p0 + (minorChord ? 15.0f : 16.0f), p0 + 22.0f };
                    break;
                case RockStyle::noise:  // o cluster: a fundamental, a segunda menor, o trítono, a quinta
                    chordPitches = { p0, p0 + 1.0f, p0 + 6.0f, p0 + 7.0f, p0 + 12.0f };
                    break;
                default:  // o power chord: a fundamental, a quinta e a oitava
                    chordPitches = { p0, p0 + 7.0f, p0 + 12.0f };
            }
        }
        if (!pulsedBlock)
        {
            for (float p : chordPitches)
            {
                Note n;
                n.time = b.start;
                n.duration = b.end - b.start;
                n.pitch = p;
                n.velocity = b.velocity;
                n.voice = 6;
                n.timbre = b.timbre;
                add.push_back(n);
            }
            continue;
        }
        for (int bar = barOf(b.start); bar <= barOf(b.end); ++bar)
            for (const auto& h : harmonyPattern(e, bar))
            {
                if (h.step >= barSteps)
                    continue;
                const double t = at(bar, h.step);
                if (t < b.start - 1e-6 || t >= b.end)
                    continue;
                for (float p : chordPitches)
                {
                    Note n;
                    n.time = t;
                    n.duration = std::min(h.dur * step, b.end - t);
                    n.pitch = p;
                    n.velocity = std::clamp(b.velocity * h.vel * 1.25f, 0.05f, 1.0f);
                    n.voice = 6;
                    n.timbre = b.timbre;
                    add.push_back(n);
                }
                ++harmonyHits;
            }
    }
    const auto rewritten = [](const std::vector<std::pair<double, double>>& spans, const Note& n) {
        return std::any_of(spans.begin(), spans.end(), [&](const auto& r) { return n.time >= r.first - 0.031 && n.time < r.second; });
    };
    score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                     [&](const Note& n) { return n.voice == 6 && rewritten(rewrittenH, n); }),
                      score.notes.end());

    // ---- a percussão ----------------------------------------------------------
    // nos trechos com pulso, a voz 4 vira a percussão do estilo
    int percHits = 0;
    if (!percussionPattern(e, 0).empty())
    {
        float vsum = 0.0f;
        int vn = 0;
        for (const auto& n : score.notes)
            if (n.voice == 4)
            {
                vsum += n.velocity;
                ++vn;
            }
        const float base = vn > 0 ? std::clamp(vsum / static_cast<float>(vn) * 1.3f, 0.3f, 1.0f) : 0.7f;
        score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                         [&](const Note& n) { return n.voice == 4 && inPulse(pulsedSpans, n.time, n.time + 1e-3); }),
                          score.notes.end());
        for (const auto& span : pulsedSpans)
            for (int bar = barOf(span.first); bar <= barOf(span.second); ++bar)
                for (const auto& h : percussionPattern(e, bar))
                {
                    if (h.step >= barSteps)
                        continue;
                    const double t = at(bar, h.step);
                    if (t < span.first - 1e-6 || t >= span.second)
                        continue;
                    Note n;
                    n.time = t;
                    n.duration = h.dur * step;
                    n.pitch = h.pitch;
                    n.velocity = std::clamp(h.vel * base, 0.05f, 1.0f);
                    n.voice = 4;
                    n.timbre = h.timbre;
                    add.push_back(n);
                    ++percHits;
                }
    }

    // ---- o baixo ------------------------------------------------------------
    std::vector<Note> bass;
    for (const auto& n : score.notes)
        if (n.voice == 1)
            bass.push_back(n);
    std::sort(bass.begin(), bass.end(), [](const Note& a, const Note& b) { return a.time < b.time; });
    // as notas do acorde (da harmonia original) que soam em t
    const auto chordAt = [&](double t) {
        for (const auto& b : blocks)
            if (t >= b.start - 1e-6 && t < b.end)
                return b.pitches;
        return std::vector<float> {};
    };
    std::vector<std::pair<double, double>> rewrittenB;
    int bassNotes = 0, runs = 0;
    for (std::size_t i = 0; i < bass.size(); ++i)
    {
        const Note& bn = bass[i];
        const double a = bn.time, b = bn.time + bn.duration;
        if (!inPulse(pulsedSpans, a, b) || bn.duration < beat * 0.9)
            continue;
        rewrittenB.push_back({ a, b });
        const float root = bn.pitch;
        const float fifth = root + 7.0f <= 55.0f ? root + 7.0f : root - 5.0f;
        const bool hasNext = i + 1 < bass.size() && bass[i + 1].time - b < beat * 0.5;
        const float next = hasNext ? bass[i + 1].pitch : root;
        const auto chord = chordAt(a + 0.01);
        const auto emit = [&](double t, double d, float p, float v) {
            if (t < a - 1e-6 || t >= b)
                return;
            Note n = bn;
            n.time = t;
            n.duration = std::min(d, b - t);
            n.pitch = p;
            n.velocity = std::clamp(bn.velocity * v, 0.05f, 1.0f);
            n.bend.clear();
            n.glide = 0.0f;
            add.push_back(n);
            ++bassNotes;
        };
        // as semínimas do trecho, no pulso
        const double first = pulse.phase + std::ceil((a - pulse.phase) / beat - 1e-6) * beat;
        std::vector<double> beatTimes;
        for (double t = first; t < b - 1e-6; t += beat)
            beatTimes.push_back(t);
        const auto beatInBar = [&](double t) {
            const double x = std::fmod((t - pulse.phase) / beat, static_cast<double>(beats));
            return static_cast<int>(std::lround(x < 0 ? x + beats : x)) % beats;
        };
        // a baixaria: nos dois últimos tempos antes da próxima fundamental, a
        // corrida em colcheias que a alcança por graus (tons e semitons)
        const auto baixaria = [&](double from) {
            const int n = 4;
            float dir = next >= root ? 1.0f : -1.0f;
            // a corrida não desce abaixo da corda mais grave do 7 cordas (Si 1):
            // perto do fundo, chega por cima
            if (next - dir * 7.0f < 35.0f)
                dir = -1.0f;
            // a quinta antes do alvo, subindo (ou descendo) por tons até a
            // sensível: para chegar a Fá, Si♭ Dó Ré Mi → Fá
            float p = next - dir * 7.0f;
            for (int k = 0; k < n; ++k)
            {
                emit(from + k * beat / 2.0, beat / 2.0 * 0.9, p, k == 0 ? 0.8f : 0.7f);
                p += dir * 2.0f;
            }
            ++runs;
        };
        switch (e)
        {
            case Ensemble::jazz:
            {
                // walking: fundamental, notas do acorde, aproximação cromática
                float cur = root;
                for (std::size_t k = 0; k < beatTimes.size(); ++k)
                {
                    float p;
                    if (k == 0)
                        p = root;
                    else if (k + 1 == beatTimes.size() && hasNext)
                        p = next + (next >= cur ? -1.0f : 1.0f);  // a aproximação
                    else if (!chord.empty())
                    {
                        const float pc = std::fmod(chord[(k * 2 + 1) % chord.size()], 12.0f);
                        p = nearestPc(pc, cur + (k % 2 == 0 ? -3.0f : 3.0f));
                        if (p < 28.0f) p += 12.0f;
                        if (p > 55.0f) p -= 12.0f;
                    }
                    else
                        p = k % 2 ? fifth : root;
                    emit(beatTimes[k], beat * 0.92, p, 0.85f);
                    cur = p;
                }
                break;
            }
            case Ensemble::manouche:
            case Ensemble::bolero:
                // em dois: a fundamental no 1, a quinta no 3 (no bolero, mais curta)
                for (double t : beatTimes)
                {
                    const int bi = beatInBar(t);
                    if (bi == 0)
                        emit(t, beat * (e == Ensemble::bolero ? 1.5 : 1.9), root, 0.9f);
                    else if (bi == beats / 2)
                        emit(t, beat * (e == Ensemble::bolero ? 1.5 : 1.9), fifth, 0.8f);
                }
                break;
            case Ensemble::samba:
            case Ensemble::escola:
            case Ensemble::choro:
            {
                // em dois: curto no 1, longo no 2 (o tempo do surdo); a
                // baixaria antes da mudança de acorde
                const bool run = hasNext && std::abs(next - root) > 0.5f && beatTimes.size() >= (e == Ensemble::choro ? 3u : 4u);
                const double runFrom = run ? beatTimes[beatTimes.size() - 2] : 1e9;
                for (double t : beatTimes)
                {
                    if (t >= runFrom - 1e-6)
                        break;
                    const int bi = beatInBar(t) % 2;
                    // o segundo tempo responde na quinta abaixo (a "resposta" do surdo)
                    const float low = root - 5.0f >= 28.0f ? root - 5.0f : root + 7.0f;
                    emit(t, beat * (bi == 0 ? 0.45 : 0.95), bi == 0 ? root : low, bi == 0 ? 0.75f : 0.95f);
                }
                if (run)
                    baixaria(runFrom);
                break;
            }
            case Ensemble::forro:
                // o baião: a fundamental pontuada e a quinta, em cada metade do compasso
                for (int bar = barOf(a); bar <= barOf(b); ++bar)
                    for (int h = 0; h < barSteps; h += 8)
                    {
                        emit(at(bar, h), 3.0 * step, root, 0.9f);
                        emit(at(bar, h + 3), 3.0 * step, fifth, 0.75f);
                    }
                break;
            case Ensemble::tango:
                // no compasso do marcato, o 1 e o 3; no da síncopa, o 3-3-2
                for (int bar = barOf(a); bar <= barOf(b); ++bar)
                {
                    if (bar % 2 == 0)
                    {
                        emit(at(bar, 0), 3.0 * step, root, 0.9f);
                        emit(at(bar, 8), 3.0 * step, fifth, 0.8f);
                    }
                    else
                    {
                        emit(at(bar, 0), 4.0 * step, root, 0.9f);
                        emit(at(bar, 6), 4.0 * step, fifth, 0.85f);
                        emit(at(bar, 12), 3.0 * step, root, 0.8f);
                    }
                }
                break;
            case Ensemble::rock:
                switch (tRock)
                {
                    case RockStyle::heavy:  // semicolcheias na fundamental, uma oitava abaixo se couber
                    {
                        const float low = root - 12.0f >= 28.0f ? root - 12.0f : root;
                        for (double t = first; t < b - 1e-6; t += beat / 4.0)
                            emit(t, beat / 4.0 * 0.8, low, std::fmod((t - pulse.phase) / beat, 1.0) < 0.1 ? 0.95f : 0.7f);
                        break;
                    }
                    case RockStyle::blues:  // o boogie: 1, 3, 5, 6, ♭7, 6, 5, 3, em shuffle
                    {
                        static const float walk[] = { 0.0f, 4.0f, 7.0f, 9.0f, 10.0f, 9.0f, 7.0f, 4.0f };
                        int k = 0;
                        for (double t = first; t < b - 1e-6; t += beat / 2.0, ++k)
                        {
                            const bool onBeat = k % 2 == 0;
                            const double tt = onBeat ? t : t + beat / 6.0;  // o shuffle: o contratempo atrasado
                            emit(tt, beat * (onBeat ? 0.6 : 0.3), root + walk[k % 8], onBeat ? 0.9f : 0.7f);
                        }
                        break;
                    }
                    case RockStyle::prog:  // a fundamental e a quinta nos grupos de 3 + 3 + 2
                        for (int bar = barOf(a); bar <= barOf(b); ++bar)
                        {
                            emit(at(bar, 0), 2.6 * step, root, 0.9f);
                            emit(at(bar, 3), 2.6 * step, fifth, 0.75f);
                            emit(at(bar, 6), 1.6 * step, root + 12.0f <= 55.0f ? root + 12.0f : root, 0.7f);
                            emit(at(bar, 8), 2.6 * step, root, 0.9f);
                            emit(at(bar, 11), 2.6 * step, fifth, 0.75f);
                            emit(at(bar, 14), 1.6 * step, root, 0.7f);
                        }
                        break;
                    case RockStyle::postrock:  // a fundamental longa, uma por compasso
                    case RockStyle::noise:     // o bordão que não se mexe
                        for (int bar = barOf(a); bar <= barOf(b); ++bar)
                            emit(at(bar, 0), barSteps * step * 0.98, root, tRock == RockStyle::noise ? 0.95f : 0.75f);
                        break;
                    case RockStyle::psychedelic:  // o riff: fundamental, oitava, quinta
                        for (int bar = barOf(a); bar <= barOf(b); ++bar)
                        {
                            emit(at(bar, 0), 3.0 * step, root, 0.9f);
                            emit(at(bar, 6), 2.0 * step, root + 12.0f <= 55.0f ? root + 12.0f : root, 0.75f);
                            emit(at(bar, 10), 4.0 * step, fifth, 0.8f);
                        }
                        break;
                    default:  // clássico e punk: colcheias na fundamental (o punk, todas fortes)
                        for (double t = first; t < b - 1e-6; t += beat / 2.0)
                            emit(t, beat / 2.0 * 0.85, hasNext && t + beat / 2.0 >= b - 1e-6 ? next : root,
                                 tRock == RockStyle::punk ? 0.9f : (std::fmod((t - pulse.phase) / beat, 1.0) < 0.25 ? 0.9f : 0.75f));
                }
                break;
            case Ensemble::charanga:
            case Ensemble::salsa:
                // tumbao: o "2 e" (antecipa a fundamental) e o 4 (a quinta), ligados
                for (int bar = barOf(a); bar <= barOf(b); ++bar)
                {
                    const double t1 = at(bar, 6.0), t2 = at(bar, 12.0);
                    emit(t1, 6.0 * step, root, 0.9f);
                    if (beats >= 4)
                        emit(t2, 10.0 * step, fifth, 0.8f);
                }
                break;
            default: break;
        }
    }
    score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(),
                                     [&](const Note& n) { return n.voice == 1 && rewritten(rewrittenB, n); }),
                      score.notes.end());
    score.notes.insert(score.notes.end(), add.begin(), add.end());
    std::stable_sort(score.notes.begin(), score.notes.end(), [](const Note& x, const Note& y) { return x.time < y.time; });
    if (harmonyHits == 0 && bassNotes == 0 && percHits == 0)
        return {};
    return std::string("figuras de ") + ensembleName(e) + ": " + std::to_string(harmonyHits) + " ataques de harmonia, "
           + std::to_string(bassNotes) + " notas de baixo" + (runs > 0 ? ", " + std::to_string(runs) + " baixarias" : "")
           + (percHits > 0 ? ", " + std::to_string(percHits) + " toques de percussão" : "");
}

std::string applyStylePhrasing(Score& score, Ensemble e, const perception::Pulse& pulse,
                               const std::vector<std::pair<double, double>>& pulsedSpans, std::uint64_t seed, int rockStyle)
{
    tRock = static_cast<RockStyle>(std::clamp(rockStyle, 0, kRockStyles - 1));
    if (e == Ensemble::automatic)
        return {};
    const double beat = pulse.period > 0.0 ? pulse.period : 0.6, step = beat / 4.0;
    std::uint64_t st = seed * 0x2545F4914F6CDD1Dull + 0x51ull;
    const auto rnd = [&] {
        st ^= st << 13;
        st ^= st >> 7;
        st ^= st << 17;
        return static_cast<double>(st >> 11) / 9007199254740992.0;
    };
    // as notas do motivo e do contracanto, por voz, em ordem
    std::vector<std::size_t> idx;
    for (std::size_t i = 0; i < score.notes.size(); ++i)
        if (score.notes[i].voice == 0 || score.notes[i].voice == 2)
            idx.push_back(i);
    std::stable_sort(idx.begin(), idx.end(), [&](std::size_t a, std::size_t b) {
        const auto& x = score.notes[a];
        const auto& y = score.notes[b];
        return x.voice != y.voice ? x.voice < y.voice : x.time < y.time;
    });
    const auto pulsed = [&](double t) { return pulse.period > 0.0 && inPulse(pulsedSpans, t, t + 1e-3); };
    // a posição na grade de semicolcheias (fração de passo; 0 = no passo)
    const auto gridPos = [&](double t) {
        const double x = (t - pulse.phase) / step;
        return x - std::floor(x + 0.5);
    };
    const auto stepIndex = [&](double t) {
        return static_cast<long>(std::lround((t - pulse.phase) / step));
    };
    std::vector<Note> add;
    int swung = 0, approaches = 0, falls = 0, graces = 0, runs = 0, antic = 0, legato = 0, vib = 0, marc = 0, bends = 0;
    for (std::size_t k = 0; k < idx.size(); ++k)
    {
        Note& n = score.notes[idx[k]];
        const Note* prev = k > 0 && score.notes[idx[k - 1]].voice == n.voice ? &score.notes[idx[k - 1]] : nullptr;
        const Note* next = k + 1 < idx.size() && score.notes[idx[k + 1]].voice == n.voice ? &score.notes[idx[k + 1]] : nullptr;
        const bool onGrid = pulse.period > 0.0 && std::abs(gridPos(n.time)) < 0.12;
        const long si = stepIndex(n.time);
        const bool beatOn = onGrid && si % 4 == 0, offEighth = onGrid && ((si % 4) + 4) % 4 == 2;
        const bool restAfter = !next || next->time - (n.time + n.duration) > beat * 0.9;
        const auto graceBefore = [&](float interval, double len, float vel) {
            if (prev && prev->time + prev->duration > n.time - len - 1e-3 && prev->time > n.time - len - 0.05)
                return false;  // sem espaço: a nota anterior está colada
            Note g = n;
            g.time = n.time - len;
            g.duration = len * 0.95;
            g.pitch = n.pitch + interval;
            g.velocity = n.velocity * vel;
            g.bend.clear();
            g.glide = 0.0f;
            g.wobbleDepth = 0.0f;
            if (g.time < 0.0)
                return false;
            add.push_back(g);
            return true;
        };
        switch (e)
        {
            case Ensemble::jazz:
            case Ensemble::manouche:
                if (offEighth && pulsed(n.time))
                {
                    n.time += step * 0.667;
                    n.duration = std::max(0.05, n.duration - step * 0.667);
                    ++swung;
                }
                if (n.duration >= beat && rnd() < 0.5 && graceBefore(-1.0f, std::min(0.07, step * 0.8), 0.7f))
                    ++approaches;
                if (restAfter && n.duration >= beat * 0.75 && e == Ensemble::jazz && rnd() < 0.5)
                {
                    n.bend = { { 0.7f, 0.0f }, { 1.0f, -3.0f } };  // a caída
                    ++falls;
                }
                break;
            case Ensemble::choro:
                if (n.duration >= beat * 0.9 && rnd() < 0.45 && graceBefore(rnd() < 0.5 ? 2.0f : 1.0f, std::min(0.05, step * 0.6), 0.65f))
                    ++graces;
                // a corrida: um salto de quarta ou mais para a próxima nota,
                // com a nota atual longa, vira semicolcheias por grau
                if (next && pulsed(n.time) && n.duration >= 3.0 * step && std::abs(next->pitch - n.pitch) >= 5.0f
                    && next->time - (n.time + n.duration) < step && rnd() < 0.6)
                {
                    const float dir = next->pitch > n.pitch ? 1.0f : -1.0f;
                    const int steps = std::min(4, static_cast<int>(std::abs(next->pitch - n.pitch) / 2.0f));
                    const double runStart = next->time - steps * step;
                    if (runStart > n.time + step)
                    {
                        n.duration = runStart - n.time;
                        float p = next->pitch - dir * static_cast<float>(steps) * 2.0f + (dir > 0 ? 1.0f : -1.0f) * 0.0f;
                        for (int r = 0; r < steps; ++r)
                        {
                            Note rn = n;
                            rn.time = runStart + r * step;
                            rn.duration = step * 0.9;
                            rn.pitch = r + 1 == steps ? next->pitch - dir : p;
                            rn.velocity = n.velocity * 0.75f;
                            rn.bend.clear();
                            rn.wobbleDepth = 0.0f;
                            add.push_back(rn);
                            p += dir * 2.0f;
                        }
                        ++runs;
                    }
                }
                break;
            case Ensemble::samba:
            case Ensemble::escola:
            case Ensemble::forro:
            case Ensemble::salsa:
            case Ensemble::charanga:
                // a síncope: a nota do tempo antecipada em uma semicolcheia
                if (beatOn && pulsed(n.time) && rnd() < 0.4 && (!prev || prev->time + prev->duration <= n.time - step + 1e-3))
                {
                    n.time -= step;
                    n.duration += step;
                    ++antic;
                }
                if (e == Ensemble::forro && n.duration >= beat && rnd() < 0.3 && graceBefore(-2.0f, 0.045, 0.6f))
                    ++graces;
                break;
            case Ensemble::bolero:
            case Ensemble::acappella:
            case Ensemble::chamber:
            case Ensemble::orchestra:
                // legato: o portamento até a próxima nota, quando ela vem colada
                if (next && std::abs(next->time - (n.time + n.duration)) < 0.06 && std::abs(next->pitch - n.pitch) >= 1.0f
                    && std::abs(next->pitch - n.pitch) <= 7.0f && n.bend.empty())
                {
                    n.bend = { { 0.0f, 0.0f }, { 0.82f, 0.0f }, { 1.0f, next->pitch - n.pitch } };
                    ++legato;
                }
                if (n.duration >= 0.6 && n.wobbleDepth <= 0.0f)
                {
                    n.wobbleDepth = e == Ensemble::bolero || e == Ensemble::acappella ? 0.25f : 0.15f;
                    n.wobbleRate = 5.5f;
                    n.wobbleDelay = 0.25f;
                    ++vib;
                }
                break;
            case Ensemble::tango:
                if (beatOn && si % 8 == 0 && pulsed(n.time))
                {
                    n.duration = std::min(n.duration, step * 1.5);  // o marcato
                    n.velocity = std::min(1.0f, n.velocity * 1.25f);
                    ++marc;
                }
                if (n.duration >= beat && rnd() < 0.35 && graceBefore(-3.0f, 0.06, 0.55f))  // o arrastre
                    ++graces;
                break;
            case Ensemble::rock:
            {
                // o fraseado de cada subestilo: a chance de bend, o bend, o vibrato
                float bendChance = 0.4f, bendFrom = -2.0f, vibDepth = 0.35f, vibRate = 6.0f;
                switch (tRock)
                {
                    case RockStyle::punk: bendChance = 0.0f; vibDepth = 0.0f; n.duration = std::min(n.duration, beat * 0.9); break;
                    case RockStyle::heavy: bendChance = 0.3f; vibDepth = 0.5f; vibRate = 7.5f; break;
                    case RockStyle::blues: bendChance = 0.7f; bendFrom = rnd() < 0.5 ? -1.0f : -0.5f; vibDepth = 0.4f; vibRate = 5.5f; break;
                    case RockStyle::prog: bendChance = 0.1f; vibDepth = 0.15f; vibRate = 5.0f; break;
                    case RockStyle::postrock: bendChance = 0.0f; vibDepth = 0.1f; vibRate = 3.0f; break;
                    case RockStyle::psychedelic: bendChance = 0.4f; vibDepth = 0.6f; vibRate = 3.5f; break;
                    case RockStyle::noise:
                        // a microfonia: a nota longa escorrega para longe (até uma quinta)
                        if (n.duration >= beat && rnd() < 0.5)
                        {
                            n.glide = (rnd() < 0.5 ? -1.0f : 1.0f) * static_cast<float>(1 + static_cast<int>(rnd() * 7.0));
                            ++bends;
                        }
                        bendChance = 0.0f;
                        vibDepth = 0.8f;
                        vibRate = 9.0f;
                        break;
                    default: break;
                }
                if (n.duration >= beat && n.bend.empty() && rnd() < bendChance)
                {
                    n.bend = { { 0.0f, bendFrom }, { 0.12f, 0.0f } };  // o bend que sobe até a nota
                    ++bends;
                }
                if (vibDepth > 0.0f && n.duration >= 0.5 && n.wobbleDepth <= 0.0f)
                {
                    n.wobbleDepth = vibDepth;
                    n.wobbleRate = vibRate;
                    n.wobbleDelay = 0.2f;
                    ++vib;
                }
                break;
            }
            default: break;
        }
    }
    score.notes.insert(score.notes.end(), add.begin(), add.end());
    std::stable_sort(score.notes.begin(), score.notes.end(), [](const Note& x, const Note& y) { return x.time < y.time; });
    std::string line = std::string("fraseado de ") + ensembleName(e) + ":";
    const auto part = [&](int count, const char* what) {
        if (count > 0)
            line += " " + std::to_string(count) + " " + what + ",";
    };
    part(swung, "colcheias com swing");
    part(approaches, "aproximações cromáticas");
    part(falls, "caídas");
    part(graces, "apojaturas");
    part(runs, "corridas");
    part(antic, "síncopes");
    part(legato, "portamentos");
    part(vib, "vibratos");
    part(marc, "marcatos");
    part(bends, "bends");
    if (line.back() != ',')
        return {};
    line.pop_back();
    return line;
}

} // namespace trioio::composition
