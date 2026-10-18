#include "Ensemble.h"
#include "Descriptors.h"
#include "Groove.h"

#include <algorithm>
#include <array>
#include <map>

namespace trioio::composition
{
namespace
{
constexpr Timbre kKeep = static_cast<Timbre>(kTimbreCount);  // a voz fica como está

// os papéis: 0 motivo, 1 baixo, 2 eco/contracanto, 3 memória, 4 percussão
// (grave e aguda), 5 mosaico, 6 harmonia
struct Formation
{
    std::array<Timbre, 7> voice;
    Timbre percLow, percHigh;
    bool percussion = true;  // false: a formação não tem percussão (as notas da voz 4 saem)
    const char* text;
};

const Formation& formation(Ensemble e)
{
    using T = Timbre;
    static const Formation choro { { T::transverseFlute, T::sevenString, T::bandolim, T::reed, kKeep, T::bandolim, T::cavaquinho },
                                   T::pandeiro, T::pandeiro, true,
                                   "flauta no motivo, bandolim no contracanto, clarinete na memória, violão de 7 cordas na baixaria, "
                                   "cavaquinho na harmonia, pandeiro" };
    static const Formation samba { { T::singing, T::sevenString, T::cavaquinho, T::transverseFlute, kKeep, T::guitar, T::cavaquinho },
                                   T::surdo, T::pandeiro, true,
                                   "voz no motivo, cavaquinho no contracanto e na harmonia, flauta na memória, violão de 7 cordas "
                                   "no baixo, surdo, pandeiro e ganzá" };
    static const Formation jazz { { T::sax, T::upright, T::trumpet, T::bars, kKeep, T::guitar, T::piano },
                                  T::kick, T::ride, true,
                                  "sax no motivo, trompete no contracanto, piano na harmonia, vibrafone na memória, baixo acústico, "
                                  "bateria (prato de condução, chimbal, escovinha, bumbo)" };
    static const Formation manouche { { T::bowed, T::upright, T::manouche, T::reed, kKeep, T::manouche, T::manouche },
                                      T::manouche, T::manouche, false,
                                      "violino no motivo, guitarra manouche no contracanto e na harmonia (a pompe faz o ritmo: "
                                      "sem percussão), clarinete na memória, contrabaixo" };
    static const Formation bolero { { T::singing, T::upright, T::requinto, T::bowed, kKeep, T::guitar, T::guitar },
                                    T::conga, T::shaker, true,
                                    "voz no motivo, requinto no contracanto, violino na memória, violões na harmonia, baixo, "
                                    "conga e maracas" };
    static const Formation charanga { { T::transverseFlute, T::upright, T::bowed, T::bowed, kKeep, T::bowed, T::piano },
                                      T::conga, T::shaker, true,
                                      "flauta no motivo, violinos no contracanto, piano no montuno, baixo, congas e guiro "
                                      "(o ganzá faz as vezes)" };
    static const Formation chamber { { T::bowed, T::bowed, T::bowed, T::bowed, kKeep, T::bowed, T::bowed },
                                     T::bowed, T::bowed, false, "quarteto de cordas: violinos, viola e violoncelo, sem percussão" };
    static const Formation orchestra { { T::stringSection, T::stringSection, T::transverseFlute, T::reed, kKeep, T::brassSection,
                                         T::stringSection },
                                       T::surdo, T::bars, true,
                                       "naipes de cordas no motivo, no baixo e na harmonia, flauta no contracanto, clarinete na memória, "
                                       "naipe de metais no mosaico, tambor grave e lâminas" };
    static const Formation salsa { { T::brassSection, T::upright, T::trombone, T::sax, kKeep, T::transverseFlute, T::piano },
                                   T::conga, T::bongo, true,
                                   "trompete no motivo, trombone no contracanto, sax na memória, piano no montuno, baixo, "
                                   "congas, bongô e guiro" };
    static const Formation acappella { { T::singing, T::singing, T::singing, T::hum, kKeep, T::whisper, T::hum },
                                       kKeep, kKeep, false,
                                       "vozes no motivo, no contracanto e no baixo, boca chiusa na harmonia e na memória, sussurro "
                                       "no mosaico, sem instrumentos" };
    static const Formation rock { { T::distortedGuitar, T::electricBass, T::organ, T::singing, kKeep, T::electricGuitar,
                                    T::electricGuitar },
                                  T::kick, T::hihat, true,
                                  "guitarra distorcida no motivo, órgão no contracanto, guitarra elétrica na base (power chords), "
                                  "voz na memória, baixo elétrico, bateria (bumbo, caixa, chimbal, prato de ataque, tons)" };
    static const Formation forro { { T::accordion, T::accordion, T::singing, T::accordion, kKeep, T::accordion, T::accordion },
                                   T::zabumba, T::triangle, true,
                                   "o trio pé-de-serra: a sanfona no motivo, nos baixos e nos acordes, voz no contracanto, "
                                   "zabumba e triângulo" };
    static const Formation tango { { T::bandoneon, T::upright, T::bowed, T::bandoneon, kKeep, T::bowed, T::piano },
                                   kKeep, kKeep, false,
                                   "bandoneón no motivo e na memória, violino no contracanto, piano na harmonia, contrabaixo; "
                                   "sem percussão (o ritmo é o marcato)" };
    static const Formation escola { { T::singing, T::sevenString, T::cavaquinho, T::voiceChoir, kKeep, T::guitar, T::cavaquinho },
                                    T::surdo, T::tamborim, true,
                                    "o puxador (voz) no motivo, o coro na memória, cavaquinho e violão de 7 cordas, e a bateria: "
                                    "surdos de primeira, segunda e terceira, caixa, repique, tamborim, agogô, cuíca e chocalho" };
    static const Formation none { { kKeep, kKeep, kKeep, kKeep, kKeep, kKeep, kKeep }, kKeep, kKeep, true, "" };
    switch (e)
    {
        case Ensemble::choro: return choro;
        case Ensemble::samba: return samba;
        case Ensemble::jazz: return jazz;
        case Ensemble::manouche: return manouche;
        case Ensemble::bolero: return bolero;
        case Ensemble::charanga: return charanga;
        case Ensemble::chamber: return chamber;
        case Ensemble::orchestra: return orchestra;
        case Ensemble::salsa: return salsa;
        case Ensemble::acappella: return acappella;
        case Ensemble::rock: return rock;
        case Ensemble::escola: return escola;
        case Ensemble::forro: return forro;
        case Ensemble::tango: return tango;
        case Ensemble::automatic: break;
    }
    return none;
}
} // namespace

const char* ensembleName(Ensemble e)
{
    switch (e)
    {
        case Ensemble::automatic: return "automático";
        case Ensemble::choro: return "choro";
        case Ensemble::samba: return "samba";
        case Ensemble::jazz: return "jazz";
        case Ensemble::manouche: return "manouche";
        case Ensemble::bolero: return "bolero";
        case Ensemble::charanga: return "charanga";
        case Ensemble::chamber: return "câmara";
        case Ensemble::orchestra: return "orquestra";
        case Ensemble::salsa: return "salsa";
        case Ensemble::acappella: return "a cappella";
        case Ensemble::rock: return "rock";
        case Ensemble::escola: return "escola de samba";
        case Ensemble::forro: return "forró";
        case Ensemble::tango: return "tango";
    }
    return "?";
}

bool parseEnsemble(const std::string& name, Ensemble& out)
{
    static const std::map<std::string, Ensemble> names {
        { "auto", Ensemble::automatic }, { "automatico", Ensemble::automatic }, { "automático", Ensemble::automatic },
        { "choro", Ensemble::choro }, { "samba", Ensemble::samba }, { "jazz", Ensemble::jazz },
        { "manouche", Ensemble::manouche }, { "bolero", Ensemble::bolero }, { "charanga", Ensemble::charanga },
        { "salsa", Ensemble::salsa }, { "camara", Ensemble::chamber }, { "câmara", Ensemble::chamber },
        { "quarteto", Ensemble::chamber }, { "orquestra", Ensemble::orchestra },
        { "rock", Ensemble::rock }, { "escola", Ensemble::escola }, { "batucada", Ensemble::escola },
        { "forro", Ensemble::forro }, { "forró", Ensemble::forro }, { "baiao", Ensemble::forro }, { "baião", Ensemble::forro },
        { "tango", Ensemble::tango }, { "acappella", Ensemble::acappella }, { "a cappella", Ensemble::acappella }, { "coro", Ensemble::acappella },
    };
    const auto it = names.find(name);
    if (it == names.end())
        return false;
    out = it->second;
    return true;
}

std::string applyEnsemble(Score& score, Ensemble e, const Constraints& rules, bool onlySynthetic)
{
    if (e == Ensemble::automatic)
        return {};
    const auto& f = formation(e);
    const auto ok = [&](Timbre t) { return t != kKeep && !rules.noTimbre[static_cast<std::size_t>(t)]; };
    if (!f.percussion && !onlySynthetic)
        score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(), [](const Note& n) { return n.voice == 4; }),
                          score.notes.end());
    int changed = 0;
    for (auto& n : score.notes)
    {
        if (onlySynthetic && static_cast<int>(n.timbre) >= static_cast<int>(Timbre::bowed))
            continue;
        Timbre t = kKeep;
        if (n.voice == 4)
            t = n.pitch < 60.0f ? f.percLow : f.percHigh;
        else if (n.voice >= 0 && n.voice < 7)
            t = f.voice[static_cast<std::size_t>(n.voice)];
        // no samba, o agudo da percussão alterna pandeiro e ganzá pela altura
        if (e == Ensemble::samba && n.voice == 4 && n.pitch >= 80.0f)
            t = Timbre::shaker;
        if (ok(t) && n.timbre != t)
        {
            n.timbre = t;
            ++changed;
        }
    }
    return std::string("conjunto: ") + ensembleName(e) + " — " + f.text + " (" + std::to_string(changed) + " notas)";
}

EnsembleReading readEnsemble(const std::vector<perception::Frame>& frames, const std::vector<perception::Onset>& onsets,
                             double duration, const perception::Pulse& pulse,
                             const std::vector<std::pair<double, double>>& pulsedSpans, const std::vector<RhythmCycle>* archive,
                             strategy::Mode mode, std::uint64_t seed)
{
    using E = Ensemble;
    EnsembleReading r;
    const auto sum = perception::summarize(frames, onsets, 0.0, duration);
    double pulsedLen = 0.0;
    for (const auto& s : pulsedSpans)
        pulsedLen += s.second - s.first;
    const double pulsedShare = duration > 0.0 ? pulsedLen / duration : 0.0;
    // a voz: quadros periódicos e pouco ruído (uma linha só que canta)
    double per = 0.0;
    int sounding = 0;
    for (const auto& f : frames)
        if (!f.silent)
        {
            per += f.periodicity;
            ++sounding;
        }
    per = sounding > 0 ? per / sounding : 0.0;
    char buf[160];
    std::string reason;
    if (pulsedShare < 0.3 || pulse.period <= 0.0)
    {
        if (per > 0.7 && sum.noisiness < 0.2f)
        {
            r.read = E::acappella;
            std::snprintf(buf, sizeof buf, "sem pulso (%.0f %% da peça) e uma voz (periodicidade %.2f)", 100.0 * pulsedShare, per);
        }
        else
        {
            r.read = sum.density > 0.45f ? E::orchestra : E::chamber;
            std::snprintf(buf, sizeof buf, "sem pulso (%.0f %% da peça), %s", 100.0 * pulsedShare,
                          sum.density > 0.45f ? "denso" : "rarefeito");
        }
        reason = buf;
    }
    else
    {
        // o ciclo reconhecido nos trechos com pulso
        const RhythmCycle* cycle = nullptr;
        double cycleScore = 0.0;
        if (archive && !archive->empty())
        {
            const auto ranked = rankCycles(*archive, frames, onsets, pulse, pulsedSpans.front().first, pulsedSpans.back().second, 1);
            if (!ranked.empty() && ranked.front().clear())
            {
                cycle = ranked.front().cycle;
                cycleScore = ranked.front().score;
            }
        }
        const auto groove = measureGroove(onsets, pulse, pulsedSpans.front().first, pulsedSpans.back().second);
        const double bpm = 60.0 / pulse.period;
        const auto family = [&](const char* f) { return cycle && cycle->family.find(f) != std::string::npos; };
        if (cycle && (family("afro-cubana") || cycle->name == "tresillo"))
            r.read = E::salsa;
        else if (cycle && cycle->name == "baião")
            r.read = E::forro;
        else if (cycle && family("bossa"))
            r.read = E::samba;
        else if (cycle && family("blues"))
            r.read = E::jazz;
        if (r.read != E::automatic)
        {
            std::snprintf(buf, sizeof buf, "o ciclo %s (%.2f)", cycle->name.c_str(), cycleScore);
            reason = buf;
        }
        else if (groove.valid && groove.swing8 > 0.07f)
        {
            r.read = bpm > 170.0 ? E::manouche : E::jazz;
            std::snprintf(buf, sizeof buf, "swing medido (%.0f %% do tempo) a %.0f BPM", 100.0 * groove.swing8, bpm);
            reason = buf;
        }
        else if (bpm >= 112.0 && sum.noisiness > 0.25f && sum.brightness > 0.5f)
        {
            r.read = E::rock;
            std::snprintf(buf, sizeof buf, "%.0f BPM, ruidoso e brilhante", bpm);
            reason = buf;
        }
        else if (bpm < 85.0)
        {
            r.read = per > 0.6 ? E::bolero : E::tango;
            std::snprintf(buf, sizeof buf, "%.0f BPM, lento%s", bpm, per > 0.6 ? ", cantado" : "");
            reason = buf;
        }
        else if (sum.density > 0.5f)
        {
            r.read = E::samba;
            std::snprintf(buf, sizeof buf, "%.0f BPM, denso (%.1f ataques/s)", bpm, sum.onsetsPerSecond);
            reason = buf;
        }
        else
        {
            r.read = E::choro;
            std::snprintf(buf, sizeof buf, "%.0f BPM, ataques moderados (%.1f/s)", bpm, sum.onsetsPerSecond);
            reason = buf;
        }
    }
    // o modo responde
    const auto neighbour = [](E e) {
        switch (e)
        {
            case E::choro: return E::samba;
            case E::samba: return E::choro;
            case E::escola: return E::samba;
            case E::jazz: return E::manouche;
            case E::manouche: return E::jazz;
            case E::salsa: return E::charanga;
            case E::charanga: return E::salsa;
            case E::bolero: return E::tango;
            case E::tango: return E::bolero;
            case E::forro: return E::choro;
            case E::rock: return E::jazz;
            case E::chamber: return E::orchestra;
            case E::orchestra: return E::chamber;
            case E::acappella: return E::chamber;
            default: return e;
        }
    };
    // o contrário em energia: os enérgicos contra os contidos
    static const E energetic[] = { E::rock, E::escola, E::salsa, E::samba, E::forro };
    static const E restrained[] = { E::chamber, E::acappella, E::bolero, E::tango, E::orchestra };
    const bool isEnergetic = std::find(std::begin(energetic), std::end(energetic), r.read) != std::end(energetic);
    const bool isRestrained = std::find(std::begin(restrained), std::end(restrained), r.read) != std::end(restrained);
    const auto pick = [&](const E* set) { return set[(seed * 2654435761ull >> 7) % 5]; };
    const char* how = "";
    switch (mode)
    {
        case strategy::Mode::transfigure:
            r.chosen = neighbour(r.read);
            how = "transfigurar: o estilo vizinho";
            break;
        case strategy::Mode::counter:
            r.chosen = isEnergetic ? pick(restrained) : isRestrained ? pick(energetic) : (seed % 2 ? E::rock : E::chamber);
            how = "contrapor: o contrário em energia";
            break;
        default:
            r.chosen = r.read;
            how = "seguir: o estilo lido";
            break;
    }
    r.why = std::string("conjunto lido na referência: ") + ensembleName(r.read) + " (" + reason + "); " + how + " → "
            + ensembleName(r.chosen);
    return r;
}

const char* rockStyleName(RockStyle s)
{
    switch (s)
    {
        case RockStyle::classic: return "clássico";
        case RockStyle::punk: return "punk";
        case RockStyle::heavy: return "pesado";
        case RockStyle::blues: return "blues-rock";
        case RockStyle::prog: return "progressivo";
        case RockStyle::postrock: return "post-rock";
        case RockStyle::psychedelic: return "psicodélico";
        case RockStyle::noise: return "noise";
    }
    return "?";
}

bool parseRockStyle(const std::string& n, RockStyle& out)
{
    if (n == "classico" || n == "clássico" || n == "hard" || n == "hard-rock") out = RockStyle::classic;
    else if (n == "punk") out = RockStyle::punk;
    else if (n == "pesado" || n == "metal" || n == "heavy") out = RockStyle::heavy;
    else if (n == "blues" || n == "blues-rock") out = RockStyle::blues;
    else if (n == "progressivo" || n == "prog") out = RockStyle::prog;
    else if (n == "post-rock" || n == "postrock" || n == "pos-rock") out = RockStyle::postrock;
    else if (n == "psicodelico" || n == "psicodélico" || n == "psych") out = RockStyle::psychedelic;
    else if (n == "noise") out = RockStyle::noise;
    else return false;
    return true;
}

} // namespace trioio::composition
