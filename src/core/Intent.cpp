#include "Intent.h"
#include "TextFold.h"

#include "Ensemble.h"
#include "HarmonyWriting.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>

namespace trioio::composition
{
namespace
{
// as palavras: sem caixa, sem acento, sem pontuação nem símbolos (TextFold.h)
std::vector<std::string> words(const std::string& text) { return splitWords(text); }

bool starts(const std::string& w, const char* stem) { return w.rfind(foldText(stem), 0) == 0; }

struct Lex
{
    const char* stem;   // a palavra começa assim
    const char* field;  // energy density brightness harmonicity tempo mood ensemble harmony voice
    float value;        // o valor do campo (0..1; o andamento em BPM; o índice do enum)
};

// o léxico: português, inglês, francês, espanhol. Os radicais são curtos de
// propósito ("energ" pega energético, energetic, énergique, enérgico)
const Lex kLex[] = {
    // energia
    { "explosiv", "energy", 0.95f }, { "intens", "energy", 0.85f }, { "energ", "energy", 0.85f }, { "agitad", "energy", 0.8f },
    { "upbeat", "energy", 0.8f }, { "power", "energy", 0.85f }, { "pesad", "energy", 0.8f }, { "heavy", "energy", 0.85f },
    { "aggress", "energy", 0.9f }, { "agress", "energy", 0.9f }, { "loud", "energy", 0.85f }, { "fort", "energy", 0.8f },
    { "calm", "energy", 0.2f }, { "suav", "energy", 0.25f }, { "soft", "energy", 0.2f }, { "gentle", "energy", 0.2f },
    { "doux", "energy", 0.2f }, { "tranquil", "energy", 0.2f }, { "quiet", "energy", 0.15f }, { "relax", "energy", 0.2f },
    { "chill", "energy", 0.25f }, { "intim", "energy", 0.25f }, { "delicad", "energy", 0.25f }, { "delicat", "energy", 0.25f },
    // densidade
    { "dens", "density", 0.85f }, { "cheio", "density", 0.8f }, { "busy", "density", 0.85f }, { "lush", "density", 0.75f },
    { "minimal", "density", 0.15f }, { "espars", "density", 0.2f }, { "sparse", "density", 0.2f }, { "rarefe", "density", 0.15f },
    { "vazi", "density", 0.1f }, { "épur", "density", 0.2f },
    // brilho
    { "brilh", "brightness", 0.8f }, { "bright", "brightness", 0.85f }, { "brillant", "brightness", 0.85f }, { "cristal", "brightness", 0.85f },
    { "escur", "brightness", 0.2f }, { "dark", "brightness", 0.2f }, { "sombre", "brightness", 0.2f }, { "abafad", "brightness", 0.15f },
    { "warm", "brightness", 0.35f }, { "quent", "brightness", 0.35f }, { "chaud", "brightness", 0.35f }, { "lo-fi", "brightness", 0.25f },
    { "lofi", "brightness", 0.25f },
    // harmonicidade
    { "ruid", "harmonicity", 0.15f }, { "nois", "harmonicity", 0.15f }, { "bruit", "harmonicity", 0.15f }, { "distor", "harmonicity", 0.3f },
    { "glitch", "harmonicity", 0.2f }, { "industrial", "harmonicity", 0.2f }, { "tonal", "harmonicity", 0.85f },
    { "melód", "harmonicity", 0.8f }, { "melod", "harmonicity", 0.8f }, { "harmon", "harmonicity", 0.8f },
    // andamento (palavras; os números "120 bpm" à parte)
    { "lent", "tempo", 66.0f }, { "slow", "tempo", 66.0f }, { "lento", "tempo", 66.0f }, { "ballad", "tempo", 70.0f },
    { "balad", "tempo", 70.0f }, { "rápid", "tempo", 150.0f }, { "rapid", "tempo", 150.0f }, { "fast", "tempo", 150.0f },
    { "vite", "tempo", 150.0f }, { "moderat", "tempo", 100.0f }, { "moderad", "tempo", 100.0f },
    // humor (Intention)
    { "alegr", "mood", static_cast<float>(Intention::luminous) }, { "happy", "mood", static_cast<float>(Intention::luminous) },
    { "joy", "mood", static_cast<float>(Intention::luminous) }, { "lumin", "mood", static_cast<float>(Intention::luminous) },
    { "uplift", "mood", static_cast<float>(Intention::luminous) }, { "joyeu", "mood", static_cast<float>(Intention::luminous) },
    { "triste", "mood", static_cast<float>(Intention::somber) }, { "sad", "mood", static_cast<float>(Intention::somber) },
    { "melanc", "mood", static_cast<float>(Intention::somber) }, { "sombri", "mood", static_cast<float>(Intention::somber) },
    { "gloom", "mood", static_cast<float>(Intention::somber) }, { "nostalg", "mood", static_cast<float>(Intention::somber) },
    { "tens", "mood", static_cast<float>(Intention::tense) }, { "suspense", "mood", static_cast<float>(Intention::tense) },
    { "anxi", "mood", static_cast<float>(Intention::tense) }, { "ansi", "mood", static_cast<float>(Intention::tense) },
    { "seren", "mood", static_cast<float>(Intention::serene) }, { "peace", "mood", static_cast<float>(Intention::serene) },
    { "paz", "mood", static_cast<float>(Intention::serene) }, { "paisib", "mood", static_cast<float>(Intention::serene) },
    { "dream", "mood", static_cast<float>(Intention::suspended) }, { "sonh", "mood", static_cast<float>(Intention::suspended) },
    { "ethere", "mood", static_cast<float>(Intention::suspended) }, { "etére", "mood", static_cast<float>(Intention::suspended) },
    { "ambient", "mood", static_cast<float>(Intention::suspended) }, { "mysteri", "mood", static_cast<float>(Intention::ambiguous) },
    { "mistér", "mood", static_cast<float>(Intention::ambiguous) }, { "misteri", "mood", static_cast<float>(Intention::ambiguous) },
    { "strange", "mood", static_cast<float>(Intention::ambiguous) }, { "estranh", "mood", static_cast<float>(Intention::ambiguous) },
    // estilo (Ensemble)
    { "choro", "ensemble", static_cast<float>(Ensemble::choro) }, { "samba", "ensemble", static_cast<float>(Ensemble::samba) },
    { "bossa", "ensemble", static_cast<float>(Ensemble::samba) }, { "jazz", "ensemble", static_cast<float>(Ensemble::jazz) },
    { "swing", "ensemble", static_cast<float>(Ensemble::jazz) }, { "bebop", "ensemble", static_cast<float>(Ensemble::jazz) },
    { "manouche", "ensemble", static_cast<float>(Ensemble::manouche) }, { "gypsy", "ensemble", static_cast<float>(Ensemble::manouche) },
    { "bolero", "ensemble", static_cast<float>(Ensemble::bolero) }, { "salsa", "ensemble", static_cast<float>(Ensemble::salsa) },
    { "charanga", "ensemble", static_cast<float>(Ensemble::charanga) }, { "chamber", "ensemble", static_cast<float>(Ensemble::chamber) },
    { "câmara", "ensemble", static_cast<float>(Ensemble::chamber) }, { "quartet", "ensemble", static_cast<float>(Ensemble::chamber) },
    { "quarteto", "ensemble", static_cast<float>(Ensemble::chamber) }, { "orchestr", "ensemble", static_cast<float>(Ensemble::orchestra) },
    { "orquest", "ensemble", static_cast<float>(Ensemble::orchestra) }, { "cinemat", "ensemble", static_cast<float>(Ensemble::orchestra) },
    { "symphon", "ensemble", static_cast<float>(Ensemble::orchestra) }, { "sinfôn", "ensemble", static_cast<float>(Ensemble::orchestra) },
    { "a-cappella", "ensemble", static_cast<float>(Ensemble::acappella) }, { "a-capela", "ensemble", static_cast<float>(Ensemble::acappella) },
    { "a-capella", "ensemble", static_cast<float>(Ensemble::acappella) }, { "acappella", "ensemble", static_cast<float>(Ensemble::acappella) }, { "acapella", "ensemble", static_cast<float>(Ensemble::acappella) },
    { "rock", "ensemble", static_cast<float>(Ensemble::rock) }, { "forró", "ensemble", static_cast<float>(Ensemble::forro) },
    // os subestilos do rock (também escolhem o conjunto rock)
    { "punk", "rock", static_cast<float>(RockStyle::punk) }, { "hardcore", "rock", static_cast<float>(RockStyle::punk) },
    { "metal", "rock", static_cast<float>(RockStyle::heavy) }, { "heavy", "rock", static_cast<float>(RockStyle::heavy) },
    { "pesad", "rock", static_cast<float>(RockStyle::heavy) }, { "doom", "rock", static_cast<float>(RockStyle::heavy) },
    { "stoner", "rock", static_cast<float>(RockStyle::heavy) }, { "blues", "rock", static_cast<float>(RockStyle::blues) },
    { "progress", "rock", static_cast<float>(RockStyle::prog) }, { "prog", "rock", static_cast<float>(RockStyle::prog) },
    { "post-rock", "rock", static_cast<float>(RockStyle::postrock) }, { "postrock", "rock", static_cast<float>(RockStyle::postrock) },
    { "pós-rock", "rock", static_cast<float>(RockStyle::postrock) }, { "shoegaze", "rock", static_cast<float>(RockStyle::postrock) },
    { "psychedel", "rock", static_cast<float>(RockStyle::psychedelic) }, { "psicodél", "rock", static_cast<float>(RockStyle::psychedelic) },
    { "psicodel", "rock", static_cast<float>(RockStyle::psychedelic) }, { "noise", "rock", static_cast<float>(RockStyle::noise) },
    { "grunge", "rock", static_cast<float>(RockStyle::heavy) }, { "hard-rock", "rock", static_cast<float>(RockStyle::classic) },
    { "forro", "ensemble", static_cast<float>(Ensemble::forro) }, { "baião", "ensemble", static_cast<float>(Ensemble::forro) },
    { "baiao", "ensemble", static_cast<float>(Ensemble::forro) }, { "xote", "ensemble", static_cast<float>(Ensemble::forro) },
    { "tango", "ensemble", static_cast<float>(Ensemble::tango) }, { "enredo", "ensemble", static_cast<float>(Ensemble::escola) },
    // linguagem harmônica
    { "modal", "harmony", static_cast<float>(HarmonyLanguage::modal) },
    { "impression", "harmony", static_cast<float>(HarmonyLanguage::impressionist) },
    { "flamenc", "harmony", static_cast<float>(HarmonyLanguage::andalusian) },
    { "atonal", "harmony", static_cast<float>(HarmonyLanguage::clusters) },
    { "microton", "harmony", static_cast<float>(HarmonyLanguage::partch) },
    { "spectral", "harmony", static_cast<float>(HarmonyLanguage::spectral) },
    { "espectral", "harmony", static_cast<float>(HarmonyLanguage::spectral) },
    { "frevo", "harmony", static_cast<float>(HarmonyLanguage::frevo) },
    { "maracatu", "harmony", static_cast<float>(HarmonyLanguage::maracatu) },
    // os descritores que as associações da rede trazem (em inglês, sobretudo)
    { "cold", "brightness", 0.3f }, { "frio", "brightness", 0.3f }, { "icy", "brightness", 0.35f }, { "torrent", "density", 0.9f },
    { "pelting", "density", 0.85f }, { "downpour", "density", 0.85f }, { "drizzl", "density", 0.3f }, { "garoa", "density", 0.3f },
    { "light", "energy", 0.3f }, { "gentle", "energy", 0.2f }, { "fine", "density", 0.35f }, { "hard", "energy", 0.8f },
    { "violent", "energy", 0.95f }, { "fierce", "energy", 0.9f }, { "wild", "energy", 0.85f }, { "still", "energy", 0.15f },
    { "silent", "energy", 0.1f }, { "deep", "brightness", 0.25f }, { "gray", "brightness", 0.3f }, { "grey", "brightness", 0.3f },
    { "cinza", "brightness", 0.3f }, { "golden", "brightness", 0.75f }, { "sunny", "mood", static_cast<float>(Intention::luminous) },
    { "gloomy", "mood", static_cast<float>(Intention::somber) }, { "lonely", "mood", static_cast<float>(Intention::somber) },
    { "solit", "mood", static_cast<float>(Intention::somber) }, { "stormy", "mood", static_cast<float>(Intention::tense) },
    { "storm", "mood", static_cast<float>(Intention::tense) }, { "tempest", "mood", static_cast<float>(Intention::tense) },
    { "endless", "narrative", 3.0f }, { "steady", "narrative", 3.0f }, { "constant", "narrative", 3.0f },
    { "continuous", "narrative", 3.0f }, { "incessant", "narrative", 3.0f }, { "monoton", "narrative", 3.0f },
    { "sudden", "narrative", 1.0f }, { "unexpected", "narrative", 1.0f },
    // a forma (as gramáticas narrativas, Dramaturgy.h)
    { "dram", "narrative", 0.0f }, { "clímax", "narrative", 0.0f }, { "climax", "narrative", 0.0f }, { "épic", "narrative", 0.0f },
    { "epic", "narrative", 0.0f }, { "virada", "narrative", 1.0f }, { "twist", "narrative", 1.0f }, { "reviravolt", "narrative", 1.0f },
    { "surpres", "narrative", 1.0f }, { "onda", "narrative", 2.0f }, { "wave", "narrative", 2.0f }, { "plat", "narrative", 3.0f },
    { "hipnót", "narrative", 3.0f }, { "hipnot", "narrative", 3.0f }, { "hypnot", "narrative", 3.0f }, { "trance", "narrative", 3.0f },
    { "arco", "narrative", 4.0f }, { "arch", "narrative", 4.0f },
    // a dança e a introspecção
    { "dançan", "dance", 0.85f }, { "dançáv", "dance", 0.85f }, { "dancan", "dance", 0.85f }, { "danceab", "dance", 0.85f },
    { "groov", "dance", 0.85f }, { "suingad", "dance", 0.8f }, { "introspect", "dance", 0.15f }, { "introspec", "dance", 0.15f },
    { "contemplat", "dance", 0.15f }, { "medit", "dance", 0.1f },
    // voz
    { "vocal", "voice", 1.0f }, { "voz", "voice", 1.0f }, { "voice", "voice", 1.0f }, { "cantad", "voice", 1.0f },
    { "sung", "voice", 1.0f }, { "singer", "voice", 1.0f }, { "chant", "voice", 1.0f }, { "coro", "voice", 1.0f },
    { "choir", "voice", 1.0f }, { "coral", "voice", 1.0f }, { "choeur", "voice", 1.0f },
    // a liberdade pedida: abre o Terceiro Olho (2026-10-18: "tango improvisado")
    { "improvis", "freedom", 0.8f }, { "improv", "freedom", 0.8f }, { "aleator", "freedom", 0.9f }, { "random", "freedom", 0.9f },
    { "imprevis", "freedom", 0.85f }, { "unpredict", "freedom", 0.85f }, { "imprevu", "freedom", 0.85f }, { "instrumental", "voice", 0.0f },
};

// palavras que não carregam campo (não contam como "não lidas")
bool stopword(const std::string& w)
{
    static const char* const kStop[] = { "a", "o", "e", "de", "da", "do", "das", "dos", "com", "em", "um", "uma", "para", "por", "que",
                                         "the", "and", "with", "of", "in", "for", "on", "an", "to", "le", "la", "les", "et", "des",
                                         "du", "avec", "el", "los", "las", "y", "con", "del", "music", "música", "musique", "música",
                                         "song", "canção", "track", "faixa", "style", "estilo", "bpm", "very", "muito",
                                         "algo", "no", "na", "nos", "nas", "meio", "pouco", "mais", "menos", "como", "sobre", "entre",
                                         "something", "some", "bit", "little", "more", "less", "like", "un", "une", "peu", "algo", "poco" };
    if (w.size() <= 1)
        return true;
    for (const char* s : kStop)
        if (w == foldText(s))
            return true;
    return std::all_of(w.begin(), w.end(), [](char c) { return c >= '0' && c <= '9'; });
}

std::string fmt(float v)
{
    char b[32];
    std::snprintf(b, sizeof b, "%.2f", static_cast<double>(v));
    return b;
}

std::string fieldValueText(const std::string& field, float v)
{
    if (field == "mood")
        return intentionName(static_cast<Intention>(static_cast<int>(v)));
    if (field == "ensemble")
        return ensembleName(static_cast<Ensemble>(static_cast<int>(v)));
    if (field == "harmony")
        return harmonyLanguageName(static_cast<HarmonyLanguage>(static_cast<int>(v)));
    if (field == "voice")
        return v > 0.5f ? "com voz" : "instrumental";
    if (field == "tempo")
        return std::to_string(static_cast<int>(std::lround(v))) + " BPM";
    if (field == "narrative")
        return narrativeName(static_cast<int>(v));
    return fmt(v);
}

float clamp01(float x) { return std::clamp(x, 0.0f, 1.0f); }
} // namespace

const char* narrativeName(int index)
{
    static const char* const k[] = { "freytag", "kishotenketsu", "ondas", "plato", "arco" };
    return index >= 0 && index < 5 ? k[index] : "";
}

DeclaredIntent translatePrompt(const std::string& prompt)
{
    DeclaredIntent d;
    // os termos de duas palavras do léxico ("post rock", "pós rock") viram um
    // só ("post-rock"): senão "post" vai sozinho para a rede (2026-10-18: a
    // Wikidata achou o alemão "Post", correio)
    std::vector<std::string> ws;
    {
        const auto raw = words(prompt);
        for (std::size_t i = 0; i < raw.size(); ++i)
        {
            if (i + 1 < raw.size())
            {
                const auto joined = raw[i] + "-" + raw[i + 1], glued = raw[i] + raw[i + 1];
                bool merge = false;
                for (const auto& l : kLex)
                {
                    const auto stem = foldText(l.stem);
                    if (stem.size() > raw[i].size() + 1 && (joined.rfind(stem, 0) == 0 || (stem.find('-') == std::string::npos && glued.rfind(stem, 0) == 0)))
                    {
                        merge = true;
                        break;
                    }
                }
                if (merge)
                {
                    ws.push_back(joined);
                    ++i;
                    continue;
                }
            }
            ws.push_back(raw[i]);
        }
    }
    // o andamento em número: "120 bpm", "120bpm", "bpm 120"
    for (std::size_t i = 0; i < ws.size(); ++i)
    {
        const auto& w = ws[i];
        int n = 0;
        if (w.size() > 3 && w.substr(w.size() - 3) == "bpm")
            n = std::atoi(w.c_str());
        else if (std::all_of(w.begin(), w.end(), [](char c) { return c >= '0' && c <= '9'; }) && i + 1 < ws.size() && ws[i + 1] == "bpm")
            n = std::atoi(w.c_str());
        else if (w == "bpm" && i + 1 < ws.size())
            n = std::atoi(ws[i + 1].c_str());
        if (n >= 30 && n <= 300)
        {
            d.fields.bpm = static_cast<float>(n);
            d.matches.push_back({ std::to_string(n) + " bpm", "tempo", std::to_string(n) + " BPM" });
            break;
        }
    }
    for (auto w : ws)
    {
        bool hit = false;
        // o número solto (sem "bpm") não é palavra a consultar
        if (std::all_of(w.begin(), w.end(), [](char c) { return (c >= '0' && c <= '9') || c == '-'; }))
            continue;
        if (w.size() > 3 && w.compare(w.size() - 3, 3, "bpm") == 0 && w[0] >= '0' && w[0] <= '9')
            continue;  // "120bpm": o andamento, lido acima
        // o erro de digitação ou de grafia: nenhum radical começa a palavra,
        // mas um está a uma letra (duas, nos longos) — a correção fica à vista
        if (!stopword(w) && std::none_of(std::begin(kLex), std::end(kLex), [&](const Lex& l) { return starts(w, l.stem); }))
        {
            int best = 99;
            std::string fix;
            for (const auto& l : kLex)
            {
                const auto stem = foldText(l.stem);
                const int dist = fuzzyPrefixDistance(w, stem);
                if (dist < best || (dist == best && stem.size() > fix.size()))
                {
                    best = dist;
                    fix = stem;
                }
            }
            if (best < 99)
            {
                d.matches.push_back({ w, "correção", "≈ " + fix });
                w = fix;
            }
        }
        // o subestilo do rock à parte: a mesma palavra pode dar a energia
        // ("heavy") e o subestilo (o pesado); ele também escolhe o conjunto rock
        for (const auto& l : kLex)
            if (std::string(l.field) == "rock" && starts(w, l.stem))
            {
                hit = true;
                if (d.fields.rockStyle < 0)
                {
                    d.fields.rockStyle = static_cast<int>(l.value);
                    if (d.fields.ensemble < 0)
                        d.fields.ensemble = static_cast<int>(Ensemble::rock);
                    d.matches.push_back({ w, "rock", rockStyleName(static_cast<RockStyle>(static_cast<int>(l.value))) });
                }
                break;
            }
        for (const auto& l : kLex)
        {
            if (!starts(w, l.stem) || std::string(l.field) == "rock")
                continue;
            const std::string f = l.field;
            hit = true;
            float* slot = f == "energy" ? &d.fields.energy : f == "density" ? &d.fields.density : f == "brightness" ? &d.fields.brightness
                        : f == "harmonicity" ? &d.fields.harmonicity : f == "tempo" ? &d.fields.bpm : nullptr;
            if (slot)
            {
                if (*slot >= 0.0f)
                    break;  // o primeiro que diz vale (o resto fica na lista, sem sobrescrever)
                *slot = l.value;
            }
            else if (f == "mood" && d.fields.mood < 0)
                d.fields.mood = static_cast<int>(l.value);
            else if (f == "ensemble" && d.fields.ensemble < 0)
                d.fields.ensemble = static_cast<int>(l.value);
            else if (f == "harmony" && d.fields.harmonyLanguage < 0)
                d.fields.harmonyLanguage = static_cast<int>(l.value);
            else if (f == "voice" && d.fields.voice < 0)
                d.fields.voice = static_cast<int>(l.value);
            else if (f == "narrative" && d.fields.narrative < 0)
                d.fields.narrative = static_cast<int>(l.value);
            else if (f == "dance" && d.fields.dance < 0.0f)
                d.fields.dance = l.value;
            else if (f == "freedom" && d.fields.freedom < 0.0f)
                d.fields.freedom = l.value;
            else
                break;
            d.matches.push_back({ w, f, fieldValueText(f, l.value) });
            break;
        }
        if (!hit && !stopword(w))
            d.unread.push_back(w);
    }
    return d;
}

IntentFields inferIntent(const std::vector<perception::Frame>& frames, const std::vector<perception::Onset>& onsets, double duration,
                         const Reading& reading)
{
    IntentFields f;
    double db = 0.0, centroid = 0.0, periodic = 0.0, flat = 0.0;
    int n = 0;
    for (const auto& fr : frames)
        if (!fr.silent)
        {
            db += fr.rmsDb;
            centroid += fr.centroidHz;
            periodic += fr.periodicity;
            flat += fr.flatness;
            ++n;
        }
    if (n == 0 || duration <= 0.0)
        return f;
    db /= n;
    centroid /= n;
    periodic /= n;
    flat /= n;
    // energia: −40 dBFS (contido) a −10 dBFS (cheio)
    f.energy = clamp01(static_cast<float>((db + 40.0) / 30.0));
    // densidade: 0,5 a 8 ataques por segundo (escala log)
    const double rate = static_cast<double>(onsets.size()) / duration;
    f.density = clamp01(static_cast<float>(std::log2(std::max(0.5, rate) / 0.5) / 4.0));
    // brilho: centroide de 300 Hz a 4 kHz (escala log)
    f.brightness = clamp01(static_cast<float>(std::log2(std::max(300.0, centroid) / 300.0) / std::log2(4000.0 / 300.0)));
    // harmonicidade: a periodicidade, menos a planura
    f.harmonicity = clamp01(static_cast<float>(0.75 * periodic + 0.25 * (1.0 - flat)));
    if (reading.pulse.confidence >= 0.3f && reading.pulse.bpm > 0.0)
        f.bpm = static_cast<float>(reading.pulse.bpm);
    // o humor: o modo e a energia (uma leitura grosseira, dita como tal)
    if (reading.key.tonic >= 0)
    {
        if (!reading.key.minor)
            f.mood = static_cast<int>(f.energy >= 0.45f ? Intention::luminous : Intention::serene);
        else
            f.mood = static_cast<int>(f.energy >= 0.6f ? Intention::tense : Intention::somber);
    }
    return f;
}

std::vector<IntentGap> compareIntent(const IntentFields& d, const IntentFields& o)
{
    std::vector<IntentGap> out;
    const auto num = [&](const char* name, const char* pt, float dv, float ov, const char* more, const char* less) {
        if (dv < 0.0f || ov < 0.0f)
            return;
        IntentGap g;
        g.field = name;
        g.declared = dv;
        g.observed = ov;
        g.gap = std::abs(dv - ov);
        g.text = std::string(pt) + ": pedida " + fmt(dv) + ", ouvida " + fmt(ov)
               + (g.gap < 0.2f ? " — como pedido" : dv > ov ? std::string(" — ") + less : std::string(" — ") + more);
        out.push_back(g);
    };
    num("energy", "energia", d.energy, o.energy, "a gravação é mais intensa que o prompt", "a gravação é mais contida que o prompt");
    num("density", "densidade", d.density, o.density, "a gravação é mais cheia que o prompt", "a gravação é mais rarefeita que o prompt");
    num("brightness", "brilho", d.brightness, o.brightness, "a gravação é mais brilhante que o prompt", "a gravação é mais escura que o prompt");
    num("harmonicity", "harmonicidade", d.harmonicity, o.harmonicity, "a gravação é mais tonal que o prompt", "a gravação é mais ruidosa que o prompt");
    if (d.bpm > 0.0f && o.bpm > 0.0f)
    {
        IntentGap g;
        g.field = "tempo";
        g.declared = d.bpm;
        g.observed = o.bpm;
        // o dobro e a metade contam como o mesmo pulso em outro nível
        double r = std::abs(std::log2(static_cast<double>(o.bpm) / d.bpm));
        r = std::min({ r, std::abs(r - 1.0), std::abs(r + 1.0) });
        g.gap = clamp01(static_cast<float>(r / 0.5));
        char b[160];
        std::snprintf(b, sizeof b, "andamento: pedido %.0f BPM, ouvido %.0f BPM%s", static_cast<double>(d.bpm), static_cast<double>(o.bpm),
                      g.gap < 0.2f ? " — como pedido (ou o dobro/metade)" : " — outro andamento");
        g.text = b;
        out.push_back(g);
    }
    if (d.mood >= 0 && o.mood >= 0)
    {
        IntentGap g;
        g.field = "mood";
        g.declared = static_cast<float>(d.mood);
        g.observed = static_cast<float>(o.mood);
        g.gap = d.mood == o.mood ? 0.0f : 0.6f;
        g.text = std::string("humor: pedido ") + intentionName(static_cast<Intention>(d.mood)) + ", ouvido "
               + intentionName(static_cast<Intention>(o.mood)) + (d.mood == o.mood ? " — como pedido" : " — outro clima");
        out.push_back(g);
    }
    return out;
}

std::string intentFieldsText(const IntentFields& f)
{
    std::string s;
    const auto add = [&](const std::string& part) { s += (s.empty() ? "" : ", ") + part; };
    if (f.energy >= 0.0f) add("energia " + fmt(f.energy));
    if (f.density >= 0.0f) add("densidade " + fmt(f.density));
    if (f.brightness >= 0.0f) add("brilho " + fmt(f.brightness));
    if (f.harmonicity >= 0.0f) add("harmonicidade " + fmt(f.harmonicity));
    if (f.bpm > 0.0f) add(fieldValueText("tempo", f.bpm));
    if (f.mood >= 0) add(std::string("humor ") + intentionName(static_cast<Intention>(f.mood)));
    if (f.ensemble >= 0) add(std::string("estilo ") + ensembleName(static_cast<Ensemble>(f.ensemble)));
    if (f.harmonyLanguage >= 0) add(std::string("harmonia ") + harmonyLanguageName(static_cast<HarmonyLanguage>(f.harmonyLanguage)));
    if (f.voice >= 0) add(f.voice ? "com voz" : "instrumental");
    if (f.rockStyle >= 0) add(std::string("rock ") + rockStyleName(static_cast<RockStyle>(f.rockStyle)));
    if (f.narrative >= 0) add(std::string("forma ") + narrativeName(f.narrative));
    if (f.dance >= 0.0f) add(f.dance >= 0.5f ? "dançante" : "introspectivo");
    if (f.freedom >= 0.0f) add("liberdade " + fmt(f.freedom));
    return s.empty() ? "nada lido" : s;
}

std::vector<std::string> applyIntent(Options& opt, const DeclaredIntent& declared, const IntentFields& observed)
{
    std::vector<std::string> lines;
    const auto& d = declared.fields;
    lines.push_back("intenção declarada" + std::string(declared.revised ? " (revisada pelo autor)" : " (traduzida do prompt)") + ": "
                    + intentFieldsText(d));
    lines.push_back("intenção ouvida (a escuta do áudio): " + intentFieldsText(observed));
    // o que o autor não escolheu na interface vem da declarada
    if (opt.intention == Intention::none && d.mood >= 0)
    {
        opt.intention = static_cast<Intention>(d.mood);
        lines.push_back(std::string("o humor declarado guia a escala: ") + intentionName(opt.intention));
    }
    if (opt.ensemble == 0 && d.ensemble > 0)
    {
        opt.ensemble = d.ensemble;
        lines.push_back(std::string("o estilo declarado escolhe o conjunto: ") + ensembleName(static_cast<Ensemble>(d.ensemble)));
    }
    if (opt.narrative.empty() && d.narrative >= 0)
    {
        opt.narrative = narrativeName(d.narrative);
        lines.push_back("a forma pedida: a gramática " + opt.narrative);
    }
    if (opt.rockStyle == 0 && d.rockStyle > 0)
    {
        opt.rockStyle = d.rockStyle;
        lines.push_back(std::string("o subestilo declarado: rock ") + rockStyleName(static_cast<RockStyle>(d.rockStyle)));
    }
    if (opt.harmonyLanguage == 0 && d.harmonyLanguage > 0)
    {
        opt.harmonyLanguage = d.harmonyLanguage;
        lines.push_back(std::string("a linguagem harmônica declarada: ") + harmonyLanguageName(static_cast<HarmonyLanguage>(d.harmonyLanguage)));
    }
    // a liberdade pedida ("improvisado"): o Terceiro Olho abre até ela
    if (d.freedom >= 0.0f && opt.unpredictability < d.freedom)
    {
        char b[160];
        std::snprintf(b, sizeof b, "a liberdade pedida abre o Terceiro Olho: imprevisibilidade %.2f → %.2f",
                      static_cast<double>(opt.unpredictability), static_cast<double>(d.freedom));
        opt.unpredictability = d.freedom;
        lines.push_back(b);
    }
    // o que foi pedido × o que soa: a distância abre o Terceiro Olho
    const auto gaps = compareIntent(d, observed);
    if (!gaps.empty())
    {
        float mean = 0.0f;
        for (const auto& g : gaps)
        {
            mean += g.gap;
            lines.push_back("pedido × ouvido · " + g.text);
        }
        mean /= static_cast<float>(gaps.size());
        const float before = opt.unpredictability;
        opt.unpredictability = std::clamp(opt.unpredictability + 0.25f * mean, 0.0f, 1.0f);
        char b[200];
        std::snprintf(b, sizeof b, "a distância entre o pedido e o que soa (%.2f) abre o Terceiro Olho: imprevisibilidade %.2f → %.2f",
                      static_cast<double>(mean), static_cast<double>(before), static_cast<double>(opt.unpredictability));
        lines.push_back(b);
    }
    return lines;
}

IntentFields intentFromAssociations(const std::vector<std::pair<std::string, double>>& words, std::string* explain)
{
    IntentFields out;
    struct Num { double sum = 0.0, w = 0.0; };
    Num energy, density, brightness, harmonicity, dance;
    std::map<std::string, std::map<int, double>> votes;  // campo → valor → peso
    double voters = 0.0;
    std::string why;
    for (const auto& [word, weight] : words)
    {
        const auto d = translatePrompt(word);
        if (d.matches.empty() || weight <= 0.0)
            continue;
        voters += weight;
        why += (why.empty() ? "" : ", ") + word;
        const auto& f = d.fields;
        const auto num = [&](Num& n, float v) {
            if (v >= 0.0f)
            {
                n.sum += weight * v;
                n.w += weight;
            }
        };
        num(energy, f.energy);
        num(density, f.density);
        num(brightness, f.brightness);
        num(harmonicity, f.harmonicity);
        num(dance, f.dance);
        if (f.mood >= 0) votes["mood"][f.mood] += weight;
        if (f.narrative >= 0) votes["narrative"][f.narrative] += weight;
        // o estilo NÃO: as associações descrevem o caráter de um conceito
        // ("heavy rain" é chuva forte, não rock pesado)
    }
    const auto avg = [](const Num& n) { return n.w > 0.0 ? static_cast<float>(n.sum / n.w) : -1.0f; };
    out.energy = avg(energy);
    out.density = avg(density);
    out.brightness = avg(brightness);
    out.harmonicity = avg(harmonicity);
    out.dance = avg(dance);
    const auto best = [&](const char* field) {
        const auto it = votes.find(field);
        if (it == votes.end())
            return -1;
        int b = -1;
        double bw = 0.0;
        for (const auto& [v, w] : it->second)
            if (w > bw)
            {
                bw = w;
                b = v;
            }
        return bw >= 0.25 * voters ? b : -1;
    };
    out.mood = best("mood");
    out.narrative = best("narrative");
    if (explain)
        *explain = why;
    return out;
}

} // namespace trioio::composition
