#include "ScaleArchive.h"
#include "TextFold.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace trioio::composition
{
namespace
{
struct Rng
{
    std::uint64_t s;
    double uniform()
    {
        s += 0x9E3779B97F4A7C15ull;
        std::uint64_t x = s;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return static_cast<double>((x ^ (x >> 31)) >> 11) / 9007199254740992.0;
    }
};

std::string lower(std::string s)
{
    for (auto& c : s)
        if (static_cast<unsigned char>(c) < 128)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string trim(const std::string& s)
{
    const auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos)
        return {};
    return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

const char* const kPc[12] = { "Dó", "Dó#", "Ré", "Mib", "Mi", "Fá", "Fá#", "Sol", "Lá♭", "Lá", "Sib", "Si" };

// cents (com ponto ou inteiro grande) ou razão a/b. No arquivo do Trioio um
// número sem barra é cents; no .scl, sem ponto é razão (inteiro).
std::optional<double> valueCents(const std::string& token, bool bareIsRatio)
{
    try
    {
        const auto slash = token.find('/');
        if (slash != std::string::npos)
        {
            const double a = std::stod(token.substr(0, slash)), b = std::stod(token.substr(slash + 1));
            if (a <= 0.0 || b <= 0.0)
                return std::nullopt;
            return 1200.0 * std::log2(a / b);
        }
        if (bareIsRatio && token.find('.') == std::string::npos)
        {
            const double a = std::stod(token);
            return a > 0.0 ? std::optional<double>(1200.0 * std::log2(a)) : std::nullopt;
        }
        return std::stod(token);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

bool normalize(ArchiveScale& s)
{
    if (s.periodCents <= 0.0 || s.stepsCents.empty())
        return false;
    std::vector<double> st { 0.0 };
    for (double c : s.stepsCents)
    {
        const double m = std::fmod(std::fmod(c, s.periodCents) + s.periodCents, s.periodCents);
        if (m > 1e-6 && m < s.periodCents - 1e-6)
            st.push_back(m);
    }
    std::sort(st.begin(), st.end());
    st.erase(std::unique(st.begin(), st.end(), [](double a, double b) { return std::abs(a - b) < 1e-6; }), st.end());
    s.stepsCents = st;
    return true;
}
} // namespace

std::vector<ArchiveScale> parseScaleArchive(const std::string& text, std::vector<std::string>& errors)
{
    std::vector<ArchiveScale> out;
    std::istringstream in(text);
    std::string line;
    std::optional<ArchiveScale> cur;
    bool bad = false;
    const auto flush = [&] {
        if (cur)
        {
            if (!bad && normalize(*cur))
                out.push_back(*cur);
            else
                errors.push_back("escala ilegível: " + cur->name);
        }
        cur.reset();
        bad = false;
    };
    while (std::getline(in, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line[0] == '=')
        {
            flush();
            cur = ArchiveScale {};
            cur->name = trim(line.substr(1));
            cur->stepsCents.clear();
            continue;
        }
        if (!cur)
            continue;
        const auto colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        const auto key = lower(trim(line.substr(0, colon)));
        const auto value = trim(line.substr(colon + 1));
        if (key == "família" || key == "familia") cur->family = value;
        else if (key == "fonte") cur->source = value;
        else if (key == "nota") cur->note = value;
        else if (key == "período" || key == "periodo")
        {
            const auto v = valueCents(value, false);
            if (v) cur->periodCents = *v; else bad = true;
        }
        else if (key == "subida" || key == "descida" || key == "apoio" || key == "percurso" || key == "cadência"
                 || key == "cadencia")
        {
            auto& dst = key == "subida" ? cur->behavior.up : key == "descida" ? cur->behavior.down
                      : key == "apoio" ? cur->behavior.support : key == "percurso" ? cur->behavior.path : cur->behavior.cadence;
            std::istringstream ts(value);
            std::string t;
            while (ts >> t)
            {
                const auto v = valueCents(t, false);
                if (v) dst.push_back(*v); else bad = true;
            }
        }
        else if (key == "assinatura")
        {
            std::istringstream ps(value);
            std::string phrase;
            while (std::getline(ps, phrase, ';'))
            {
                std::istringstream ts(phrase);
                std::string t;
                std::vector<double> ph;
                while (ts >> t)
                {
                    const auto v = valueCents(t, false);
                    if (v) ph.push_back(*v); else bad = true;
                }
                if (!ph.empty())
                    cur->behavior.signatures.push_back(ph);
            }
        }
        else if (key == "ornamento") cur->behavior.ornament = value;
        else if (key == "textura") cur->behavior.texture = value;
        else if (key == "ritmo") cur->behavior.rhythm = value;
        else if (key == "voicing") cur->behavior.voicing = value;
        else if (key == "vamp")
        {
            std::istringstream ts(value);
            int d = 0;
            while (ts >> d)
                cur->behavior.vamp.push_back(d);
        }
        else if (key == "comportamento-fonte") cur->behavior.source = value;
        else if (key == "comportamento-nota") cur->behavior.note = value;
        else if (key == "graus")
        {
            std::istringstream ts(value);
            std::string t;
            while (ts >> t)
            {
                const auto v = valueCents(t, false);
                if (v) cur->stepsCents.push_back(*v); else bad = true;
            }
        }
    }
    flush();
    return out;
}

std::optional<ArchiveScale> parseScl(const std::string& text, const std::string& name)
{
    std::istringstream in(text);
    std::string line;
    std::vector<std::string> useful;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        const auto first = line.find_first_not_of(" \t");
        if (first != std::string::npos && line[first] == '!')
            continue;
        if (useful.empty())
            useful.push_back(trim(line));  // a descrição pode ser vazia
        else if (first != std::string::npos)
            useful.push_back(line.substr(first));
    }
    if (useful.size() < 2)
        return std::nullopt;
    ArchiveScale s;
    s.name = name;
    s.family = "arquivo .scl";
    s.source = useful[0];
    int count = 0;
    try { count = std::stoi(useful[1]); } catch (...) { return std::nullopt; }
    std::vector<double> values;
    for (std::size_t i = 2; i < useful.size() && static_cast<int>(values.size()) < count; ++i)
    {
        std::istringstream tok(useful[i]);
        std::string t;
        tok >> t;
        const auto v = valueCents(t, true);
        if (!v)
            return std::nullopt;
        values.push_back(*v);
    }
    if (count < 1 || static_cast<int>(values.size()) != count || values.back() <= 0.0)
        return std::nullopt;
    s.periodCents = values.back();
    s.stepsCents.assign(values.begin(), values.end() - 1);
    s.stepsCents.push_back(0.0);
    if (!normalize(s))
        return std::nullopt;
    return s;
}

namespace
{
struct Cand { double score; const ArchiveScale* s; int tonic; };

// a pontuação de cada escala candidata em cada tônica (ver fitArchive)
std::vector<Cand> scoreCandidates(const std::vector<const ArchiveScale*>& cands, const std::array<float, 12>& field)
{
    float total = 0.0f, peak = 0.0f;
    for (float w : field)
    {
        total += std::max(0.0f, w);
        peak = std::max(peak, w);
    }
    std::vector<Cand> scored;
    for (const auto* s : cands)
        for (int r = 0; r < 12; ++r)
        {
            double score = 0.0;
            if (total > 0.0f && peak > 0.0f)
            {
                // cada classe tocada é coberta se algum grau está a até 40
                // cents dela (meia cobertura no limite); graus que a
                // referência não toca custam; a tônica tocada conta
                double covered = 0.0;
                for (int p = 0; p < 12; ++p)
                {
                    const float w = std::max(0.0f, field[static_cast<std::size_t>(p)]);
                    if (w <= 0.0f)
                        continue;
                    const double target = std::fmod(((p - r) % 12 + 12) * 100.0, s->periodCents);
                    double best = 1e9;
                    for (double c : s->stepsCents)
                    {
                        const double d = std::abs(c - target);
                        best = std::min(best, std::min(d, s->periodCents - d));
                    }
                    if (best <= 40.0)
                        covered += w * (1.0 - 0.5 * best / 40.0);
                }
                int unused = 0;
                for (double c : s->stepsCents)
                    unused += field[static_cast<std::size_t>(static_cast<int>(std::lround(c / 100.0 + r)) % 12)] < 0.05f * peak;
                score = covered / total - 0.04 * unused + 0.2 * field[static_cast<std::size_t>(r)] / peak;
            }
            scored.push_back({ score, s, r });
        }
    return scored;
}
} // namespace

ArchivePick fitArchive(const std::vector<ArchiveScale>& archive, const std::array<float, 12>& field, const std::string& query,
                       std::uint64_t seed, int choices)
{
    ArchivePick pick;
    std::vector<const ArchiveScale*> cands;
    const auto q = lower(trim(query));
    if (!q.empty())
    {
        const auto fq = foldText(q);
        for (const auto& s : archive)
            if (foldText(s.name) == fq)
                cands = { &s };
        if (cands.empty())
            for (const auto& s : archive)
                if (foldText(s.name).find(fq) != std::string::npos || foldText(s.family).find(fq) != std::string::npos
                    || foldText(s.source).find(fq) != std::string::npos)
                    cands.push_back(&s);
        // uma região ou família em palavras do autor ("modos gregos", "escalas
        // africanas"): cada palavra vira o termo do arquivo (Grécia antiga,
        // Uganda, Moçambique…); as palavras genéricas ("escalas", "modos") saem
        if (cands.empty())
        {
            static const std::pair<const char*, std::vector<const char*>> kAlias[] = {
                { "greg", { "grecia" } }, { "grec", { "grecia" } }, { "greek", { "grecia" } },
                { "afric", { "uganda", "mocambique" } },
                { "arab", { "maqam" } }, { "turc", { "makam" } }, { "turk", { "makam" } },
                { "carnat", { "carnatica" } }, { "hindust", { "hindustani" } }, { "indian", { "india" } }, { "indu", { "india" } },
                { "japon", { "japao" } }, { "japan", { "japao" } }, { "chin", { "china" } }, { "indones", { "indonesia" } },
                { "javan", { "indonesia" } }, { "brasil", { "brasil" } }, { "brazil", { "brasil" } }, { "tailand", { "tailandia" } },
                { "thai", { "tailandia" } }, { "flamenc", { "flamenco" } }, { "just", { "entonacao justa" } },
                { "harmonic", { "serie harmonica" } }, { "harmonik", { "serie harmonica" } }, { "subarmon", { "serie subarmonica" } },
                { "messiaen", { "transposicao limitada" } }, { "microton", { "divisao igual" } }, { "temperad", { "divisao igual", "temperamento" } },
                { "histor", { "temperamento historico" } }, { "mediev", { "pratica modal" } }, { "igreja", { "pratica modal" } },
                { "eclesi", { "pratica modal" } },
                { "dim", { "messiaen modo 2" } }, { "diminut", { "messiaen modo 2" } }, { "octaton", { "messiaen modo 2" } },
                { "miles", { "dorico" } }, { "davis", { "dorico" } }, { "dorian", { "dorico" } }, { "gregorian", { "pratica modal" } }, { "church", { "pratica modal" } }, { "euler", { "euler-fokker" } }, { "partch", { "partch" } },
            };
            static const char* const kGeneric[] = { "escala", "escalas", "modo", "modos", "scale", "scales", "mode", "modes", "gamme",
                                                    "gammes", "de", "do", "da", "dos", "das", "e", "and", "the" };
            std::vector<std::string> terms;
            std::string w;
            for (char c : fq + " ")
            {
                if (c == ' ' || c == ',' || c == '/')
                {
                    if (!w.empty() && std::none_of(std::begin(kGeneric), std::end(kGeneric), [&](const char* g) { return w == g; }))
                    {
                        bool aliased = false;
                        for (const auto& [stem, fams] : kAlias)
                            if (w.rfind(stem, 0) == 0)
                            {
                                aliased = true;
                                for (const char* f : fams)
                                    terms.push_back(f);
                            }
                        if (!aliased && w.size() >= 4)
                            terms.push_back(w);
                    }
                    w.clear();
                }
                else
                    w += c;
            }
            for (const auto& s : archive)
            {
                const auto fam = foldText(s.family), name = foldText(s.name);
                if (std::any_of(terms.begin(), terms.end(),
                                [&](const std::string& t) { return fam.find(t) != std::string::npos || name.find(t) != std::string::npos; }))
                    cands.push_back(&s);
            }
        }
    }
    else
        for (const auto& s : archive)
            if (std::abs(s.periodCents - 1200.0) <= 10.0 && s.stepsCents.size() >= 5 && s.stepsCents.size() <= 9
                && isLanguageScale(s))
                cands.push_back(&s);
    pick.candidates = static_cast<int>(cands.size());
    if (cands.empty())
    {
        pick.error = q.empty() ? "o arquivo de escalas está vazio" : "nenhuma escala com \"" + query + "\"";
        return pick;
    }

    auto scored = scoreCandidates(cands, field);
    std::stable_sort(scored.begin(), scored.end(), [](const Cand& a, const Cand& b) { return a.score > b.score; });
    // o sorteio fica entre as QUASE EMPATADAS com a melhor (até 0,05 abaixo):
    // a variedade entre seeds continua entre equivalentes (Hijaz, melakarta,
    // modo andaluz), sem trazer uma escala que veste pior (2026-10-11: o
    // Suno recebia um modo de Messiaen de 8 notas, 5.º lugar)
    std::vector<Cand> top;
    for (const auto& c : scored)
    {
        if (!top.empty() && c.score < top.front().score - 0.05)
            break;
        if (std::none_of(top.begin(), top.end(), [&](const Cand& t) { return t.s == c.s; }))
            top.push_back(c);
        if (static_cast<int>(top.size()) >= std::max(1, choices))
            break;
    }
    // entre as quase empatadas, as que trazem o COMPORTAMENTO da linguagem
    // (subida, apoio, assinatura, cadência, ornamento, textura, ritmo): o
    // melakarta 14 e o maqam Hijaz têm as mesmas notas, mas só o Hijaz diz
    // como tocá-las (2026-10-11)
    if (std::any_of(top.begin(), top.end(), [](const Cand& c) { return c.s->behavior.any(); }))
        top.erase(std::remove_if(top.begin(), top.end(), [](const Cand& c) { return !c.s->behavior.any(); }), top.end());
    Rng rng { seed * 0x94D049BB133111EBull + 0xA4C1 };
    const auto& c = top[static_cast<std::size_t>(rng.uniform() * static_cast<double>(top.size())) % top.size()];
    pick.choice.tonic = static_cast<float>(c.tonic);
    for (double v : c.s->stepsCents)
        pick.choice.steps.push_back(static_cast<float>(v / 100.0));
    pick.period = static_cast<float>(c.s->periodCents / 100.0);
    pick.source = c.s->source;
    pick.behavior = c.s->behavior;
    pick.choice.name = c.s->name + " (" + c.s->family + ") sobre " + kPc[c.tonic];
    return pick;
}

bool isLanguageScale(const ArchiveScale& s)
{
    static const char* const systems[] = { "MOS", "divisão igual", "temperamento", "conjunto", "série",
                                           "entonação justa", "Euler-Fokker", "produto combinatório", "não oitavante" };
    return std::none_of(std::begin(systems), std::end(systems),
                        [&](const char* f) { return s.family.find(f) != std::string::npos; });
}

std::vector<RankedScale> rankArchive(const std::vector<ArchiveScale>& archive, const std::array<float, 12>& field, int count,
                                     int minDegrees, int maxDegrees)
{
    std::vector<const ArchiveScale*> cands;
    for (const auto& s : archive)
        if (std::abs(s.periodCents - 1200.0) <= 10.0 && static_cast<int>(s.stepsCents.size()) >= minDegrees
            && static_cast<int>(s.stepsCents.size()) <= maxDegrees)
            cands.push_back(&s);
    auto scored = scoreCandidates(cands, field);
    std::stable_sort(scored.begin(), scored.end(), [](const Cand& a, const Cand& b) { return a.score > b.score; });
    std::vector<RankedScale> out;
    for (const auto& c : scored)
    {
        if (std::none_of(out.begin(), out.end(), [&](const RankedScale& t) { return t.scale == c.s; }))
            out.push_back({ c.s, c.tonic, c.score });
        if (static_cast<int>(out.size()) >= count)
            break;
    }
    return out;
}

} // namespace trioio::composition
