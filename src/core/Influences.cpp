#include "Influences.h"
#include "TextFold.h"

#include "Ensemble.h"
#include "HarmonyWriting.h"
#include "ScaleAffect.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace trioio::composition
{
namespace
{
struct Rng
{
    std::uint64_t s;
    double next()
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return static_cast<double>(s >> 11) / 9007199254740992.0;
    }
};

std::string trim(const std::string& v)
{
    const auto a = v.find_first_not_of(" \t\r");
    const auto b = v.find_last_not_of(" \t\r");
    return a == std::string::npos ? std::string() : v.substr(a, b - a + 1);
}

std::string category(const std::string& key) { return key.substr(0, key.find(':')); }
std::string valueOf(const std::string& key) { return key.find(':') == std::string::npos ? std::string() : key.substr(key.find(':') + 1); }
std::string topBranch(const std::string& path) { return path.substr(0, path.find('/')); }

std::string fmt2(float v)
{
    char b[32];
    std::snprintf(b, sizeof b, "%.2f", static_cast<double>(v));
    return b;
}
} // namespace

std::vector<InfluenceNode> parseInfluenceArchive(const std::string& text, std::vector<std::string>& errors)
{
    std::vector<InfluenceNode> out;
    std::size_t pos = 0;
    int lineNo = 0;
    while (pos <= text.size())
    {
        const auto nl = text.find('\n', pos);
        const std::string line = trim(text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos));
        pos = nl == std::string::npos ? text.size() + 1 : nl + 1;
        ++lineNo;
        if (line.empty() || line[0] == '#')
            continue;
        if (line[0] == '=')
        {
            out.push_back({});
            out.back().path = trim(line.substr(1));
            continue;
        }
        const auto colon = line.find(':');
        if (out.empty() || colon == std::string::npos)
        {
            errors.push_back("linha " + std::to_string(lineNo) + ": fora de um nó");
            continue;
        }
        auto& n = out.back();
        const std::string k = line.substr(0, colon), v = trim(line.substr(colon + 1));
        if (k == "conceito") n.conceito = v;
        else if (k == "contexto") n.context = v;
        else if (k == "fonte") n.source = v;
        else if (k == "também" || k == "tambem")
        {
            std::string a;
            for (char c : v + ",")
                if (c == ',')
                {
                    const auto t = trim(a);
                    if (!t.empty())
                        n.aliases.push_back(t);
                    a.clear();
                }
                else
                    a += c;
        }
        else if (k == "traço")
        {
            const auto sp = v.rfind(' ');
            char* end = nullptr;
            const float x = sp == std::string::npos ? -1.0f : std::strtof(v.c_str() + sp + 1, &end);
            if (sp == std::string::npos || end == v.c_str() + sp + 1 || x < 0.0f || x > 1.0f || v.find(':') == std::string::npos)
                errors.push_back("linha " + std::to_string(lineNo) + ": traço ilegível em \"" + n.path + "\"");
            else
                n.traits.push_back({ trim(v.substr(0, sp)), x });
        }
        else
            errors.push_back("linha " + std::to_string(lineNo) + ": campo desconhecido \"" + k + "\"");
    }
    return out;
}

const InfluenceNode* findInfluence(const std::vector<InfluenceNode>& archive, const std::string& nameIn)
{
    const std::string name = trim(nameIn);
    for (const auto& n : archive)
        if (n.path == name)
            return &n;
    for (const auto& n : archive)
        if (n.path.size() > name.size() && n.path.compare(n.path.size() - name.size(), name.size(), name) == 0
            && n.path[n.path.size() - name.size() - 1] == '/')
            return &n;
    return nullptr;
}

std::vector<InfluenceTrait> inheritedTraits(const std::vector<InfluenceNode>& archive, const InfluenceNode& node)
{
    // da raiz ao nó: cada nível muda o que herdou
    std::vector<InfluenceTrait> out;
    std::string prefix;
    std::size_t start = 0;
    while (true)
    {
        const auto slash = node.path.find('/', start);
        prefix = node.path.substr(0, slash);
        for (const auto& n : archive)
            if (n.path == prefix)
                for (const auto& t : n.traits)
                {
                    auto it = std::find_if(out.begin(), out.end(), [&](const InfluenceTrait& x) { return x.key == t.key; });
                    if (it != out.end())
                        it->value = t.value;
                    else
                        out.push_back(t);
                }
        if (slash == std::string::npos)
            break;
        start = slash + 1;
    }
    return out;
}

InfluenceDraw drawInfluences(const std::vector<InfluenceNode>& archive, const std::vector<std::string>& requested, std::uint64_t seed,
                             float unpredictability, const std::map<std::string, int>& usage, float strengthIn,
                             const std::vector<std::string>& exclude)
{
    const double strength = std::clamp(static_cast<double>(strengthIn), 0.0, 1.0);
    InfluenceDraw d;
    if (archive.empty())
        return d;
    Rng rng { seed * 0x9E3779B97F4A7C15ull + 0x1F1u };
    const double u = std::clamp(static_cast<double>(unpredictability), 0.0, 1.0);
    const auto used = [&](const std::string& k) {
        const auto it = usage.find(k);
        return it == usage.end() ? 0 : it->second;
    };
    // os nós pedidos
    std::vector<std::pair<const InfluenceNode*, double>> chosen;
    for (const auto& r : requested)
        if (const auto* n = findInfluence(archive, r))
            chosen.push_back({ n, 1.0 });
        else if (!trim(r).empty())
        {
            // a escrita livre (2026-10-18: o autor pediu "rock 60"): as palavras
            // pela árvore, com a mesma tolerância das Palavras; e a década
            // ("60", "60s", "anos 60", "1970") escolhe, dentro do ramo, os nós
            // cujo contexto histórico a cita
            std::vector<const InfluenceNode*> found;
            for (const auto& path : influencesFromWords(archive, r))
                if (const auto* n = findInfluence(archive, path))
                    found.push_back(n);
            std::string decade;
            for (const auto& w : splitWords(r))
            {
                std::string digits;
                for (char c : w)
                    if (c >= '0' && c <= '9')
                        digits += c;
                if (digits.size() == 2 && digits.back() == '0' && w.size() <= 3)
                    decade = (digits[0] >= '2' ? "19" : "20") + digits;
                else if (digits.size() == 4 && digits[0] <= '2' && w.size() <= 5)
                    decade = digits.substr(0, 3) + "0";
            }
            std::string how;
            if (!decade.empty())
            {
                std::vector<const InfluenceNode*> inDecade;
                for (const auto& n : archive)
                {
                    if (foldText(n.context).find("anos " + decade) == std::string::npos && foldText(n.context).find(decade.substr(0, 3)) == std::string::npos)
                        continue;
                    // dentro do ramo pedido (se houver palavras), ou em toda a árvore
                    const bool inBranch = found.empty() || std::any_of(found.begin(), found.end(), [&](const InfluenceNode* f) {
                        return n.path == f->path || n.path.rfind(f->path + "/", 0) == 0;
                    });
                    if (inBranch)
                        inDecade.push_back(&n);
                }
                if (!inDecade.empty())
                {
                    const auto* pick = inDecade[static_cast<std::size_t>(rng.next() * inDecade.size()) % inDecade.size()];
                    found = { pick };
                    how = " (os anos " + decade + " no contexto: " + std::to_string(inDecade.size()) + " nó(s))";
                }
            }
            if (found.empty())
                d.lines.push_back("influências: \"" + trim(r) + "\" não está no arquivo (ignorada)");
            for (const auto* n : found)
            {
                chosen.push_back({ n, 1.0 });
                d.lines.push_back("influências: \"" + trim(r) + "\" → " + n->path + how);
            }
        }
    // nenhum pedido: um nó escolhido pela raridade (o pouco usado vem mais)
    const auto pickByRarity = [&](auto accept) -> const InfluenceNode* {
        double total = 0.0;
        for (const auto& n : archive)
            if (accept(n))
                total += 1.0 / (1.0 + 0.6 * used("nó:" + n.path));
        double x = rng.next() * total;
        for (const auto& n : archive)
            if (accept(n))
            {
                x -= 1.0 / (1.0 + 0.6 * used("nó:" + n.path));
                if (x <= 0.0)
                    return &n;
            }
        return nullptr;
    };
    if (chosen.empty())
        if (const auto* n = pickByRarity([](const InfluenceNode&) { return true; }))
        {
            chosen.push_back({ n, 1.0 });
            d.lines.push_back("influências: nenhuma pedida; a árvore escolheu \"" + n->path + "\" (o menos usado pesa mais)");
        }
    if (chosen.empty())
        return d;
    // a combinação dos traços (com os herdados)
    std::map<std::string, std::pair<double, double>> acc;  // chave → (soma valor·peso, soma peso)
    std::map<std::string, std::vector<std::pair<float, double>>> sources;  // chave → (valor, peso) de cada nó
    const auto mix = [&](const InfluenceNode& n, double w) {
        for (const auto& t : inheritedTraits(archive, n))
        {
            acc[t.key].first += w * t.value;
            acc[t.key].second += w;
            sources[t.key].push_back({ t.value, w });
        }
    };
    for (const auto& [n, w] : chosen)
        mix(*n, w);
    const auto axis = [&](const std::string& name, float dflt) {
        const auto it = acc.find("eixo:" + name);
        return it == acc.end() || it->second.second <= 0.0 ? dflt : static_cast<float>(it->second.first / it->second.second);
    };
    // a repetição que o conceito pede: a novidade cede
    const float repetition = axis("repetição", 0.3f);
    const double novelty = 1.0 - repetition;
    // a polinização: um galho distante, com peso menor
    if (rng.next() < novelty * (0.3 + 0.5 * u))
    {
        std::set<std::string> tops;
        for (const auto& [n, w] : chosen)
            tops.insert(topBranch(n->path));
        if (const auto* far = pickByRarity([&](const InfluenceNode& n) { return !tops.count(topBranch(n.path)); }))
        {
            chosen.push_back({ far, 0.35 });
            mix(*far, 0.35);
            d.lines.push_back("polinização: \"" + far->path + "\" entra de longe, com peso 0,35"
                              + (far->conceito.empty() ? std::string() : " (" + far->conceito + ")"));
        }
    }
    // o galho da virada: o polinizado (peso < 1) ou o segundo pedido
    {
        const InfluenceNode* tw = nullptr;
        for (const auto& [n, w] : chosen)
            if (w < 1.0)
                tw = n;
        if (!tw && chosen.size() >= 2)
            tw = chosen[1].first;
        if (tw)
        {
            d.twistNode = tw->path;
            std::map<std::string, float> bestT;
            for (const auto& t : inheritedTraits(archive, *tw))
            {
                if (category(t.key) == "eixo")
                    d.twistAxes[valueOf(t.key)] = t.value;
                else if (category(t.key) == "timbre")
                {
                    const auto rest = valueOf(t.key);
                    const auto c2 = rest.find(':');
                    if (c2 != std::string::npos && t.value > bestT[rest.substr(0, c2)])
                    {
                        bestT[rest.substr(0, c2)] = t.value;
                        d.twistTimbres[rest.substr(0, c2)] = rest.substr(c2 + 1);
                    }
                }
            }
        }
    }
    for (const auto& [n, w] : chosen)
    {
        d.nodes.push_back(n->path);
        d.used.push_back("nó:" + n->path);
        if (w >= 1.0)
            d.lines.push_back("influência: \"" + n->path + "\"" + (n->conceito.empty() ? std::string() : " — " + n->conceito));
    }
    // as categorias (uma escolha cada): a chance é a tendência; o muito usado
    // fica raro, a não ser que o conceito peça a repetição
    const auto choose = [&](const std::string& cat) -> std::string {
        std::vector<std::pair<std::string, double>> cands;
        double best = 0.0;
        for (const auto& [k, sw] : acc)
            if (category(k) == cat && sw.second > 0.0)
            {
                const double tendency = sw.first / sw.second;
                const double rarity = 1.0 / (1.0 + novelty * 0.4 * used(k));
                cands.push_back({ k, tendency * rarity * sw.second });
                best = std::max(best, tendency);
            }
        if (cands.empty() || rng.next() >= best * strength)
            return {};  // a tendência não se cumpriu nesta peça (ou a força é baixa)
        double total = 0.0;
        for (const auto& c : cands)
            total += c.second;
        double x = rng.next() * total;
        for (const auto& c : cands)
        {
            x -= c.second;
            if (x <= 0.0)
            {
                d.used.push_back(c.first);
                return valueOf(c.first);
            }
        }
        return {};
    };
    d.ensemble = choose("conjunto");
    d.rockStyle = choose("rock");
    d.harmony = choose("harmonia");
    d.mood = choose("humor");
    d.scale = choose("escala");
    d.cycle = choose("ciclo");
    d.narrative = choose("forma");
    // os timbres: em cada papel, uma família pela tendência (a mesma regra
    // das escolhas: a chance é a tendência, o muito usado fica raro)
    {
        std::map<std::string, std::vector<std::pair<std::string, double>>> byRole;  // papel → (chave, peso)
        std::map<std::string, double> bestByRole;
        for (const auto& [k, sw] : acc)
            if (category(k) == "timbre" && sw.second > 0.0)
            {
                const auto rest = valueOf(k);
                const auto c2 = rest.find(':');
                if (c2 == std::string::npos)
                    continue;
                const double tendency = sw.first / sw.second;
                byRole[rest.substr(0, c2)].push_back({ k, tendency * sw.second / (1.0 + novelty * 0.4 * used(k)) });
                bestByRole[rest.substr(0, c2)] = std::max(bestByRole[rest.substr(0, c2)], tendency);
            }
        for (auto& [role, cands] : byRole)
        {
            if (rng.next() >= bestByRole[role] * strength)
                continue;
            double total = 0.0;
            for (const auto& c : cands)
                total += c.second;
            double x = rng.next() * total;
            for (const auto& c : cands)
            {
                x -= c.second;
                if (x <= 0.0)
                {
                    d.timbres[role] = valueOf(c.first).substr(role.size() + 1);
                    d.used.push_back(c.first);
                    break;
                }
            }
        }
    }
    // as presenças: cada voz pela sua tendência
    for (const auto& [k, sw] : acc)
        if (category(k) == "presença" && sw.second > 0.0)
            d.presence[valueOf(k)] = rng.next() < sw.first / sw.second;
    // os eixos pelo CRUZAMENTO, não pela média: cada eixo vem inteiro de um
    // dos nós (sorteado pelo peso). A média puxava tudo para o neutro (o noise
    // com o maracatu dava dança +0,08, peso +0,09) e apagava os extremos,
    // que é onde mora o inusitado. Com uma folga que cresce com a
    // imprevisibilidade e a novidade.
    for (const char* a : { "dança", "peso", "ruído", "rasgo", "repetição", "densidade" })
    {
        const auto it = sources.find(std::string("eixo:") + a);
        if (it == sources.end() || it->second.empty())
            continue;
        double total = 0.0;
        for (const auto& s : it->second)
            total += s.second;
        double x = rng.next() * total;
        float v = it->second.front().first;
        for (const auto& s : it->second)
        {
            x -= s.second;
            if (x <= 0.0)
            {
                v = s.first;
                break;
            }
        }
        const float jitter = static_cast<float>((rng.next() - 0.5) * 0.3 * u * novelty);
        // a força: o eixo volta ao meio na proporção do que falta
        d.axes[a] = std::clamp(0.5f + static_cast<float>(strength) * (v - 0.5f) + jitter, 0.0f, 1.0f);
    }
    // a exclusão do autor: vale sempre
    for (const auto& role : exclude)
        if (!trim(role).empty())
            d.presence[trim(role)] = false;
    if (strength < 0.999)
        d.lines.push_back("força das influências " + fmt2(static_cast<float>(strength)) + ": as escolhas e os eixos pesam menos que a escuta");
    if (!exclude.empty())
    {
        std::string ex;
        for (const auto& r : exclude)
            ex += (ex.empty() ? "" : ", ") + trim(r);
        d.lines.push_back("o autor tirou: " + ex);
    }
    // a mutação: uma categoria troca por um valor de qualquer galho
    if (rng.next() < 0.35 * u * novelty)
    {
        static const char* const cats[] = { "conjunto", "harmonia", "humor", "escala", "ciclo" };
        const std::string cat = cats[static_cast<std::size_t>(rng.next() * 5.0) % 5];
        std::vector<std::string> pool;
        for (const auto& n : archive)
            for (const auto& t : n.traits)
                if (category(t.key) == cat)
                    pool.push_back(valueOf(t.key));
        if (!pool.empty())
        {
            const auto v = pool[static_cast<std::size_t>(rng.next() * static_cast<double>(pool.size())) % pool.size()];
            (cat == "conjunto" ? d.ensemble : cat == "harmonia" ? d.harmony : cat == "humor" ? d.mood : cat == "escala" ? d.scale : d.cycle) = v;
            d.used.push_back(cat + ":" + v);
            d.lines.push_back("mutação: " + cat + " → " + v + " (de outro galho)");
        }
    }
    // a forma: o que entrou
    std::string picks;
    const auto add = [&](const char* label, const std::string& v) {
        if (!v.empty())
            picks += (picks.empty() ? "" : ", ") + std::string(label) + " " + v;
    };
    add("conjunto", d.ensemble);
    add("rock", d.rockStyle);
    add("harmonia", d.harmony);
    add("humor", d.mood);
    add("escala", d.scale);
    add("ciclo", d.cycle);
    add("forma", d.narrative);
    for (const auto& [role, t] : d.timbres)
        picks += (picks.empty() ? "" : ", ") + role + " em " + t;
    if (!picks.empty())
        d.lines.push_back("sorteado: " + picks);
    std::string absent;
    for (const auto& [role, present] : d.presence)
        if (!present)
            absent += (absent.empty() ? "" : ", ") + role;
    if (!absent.empty())
        d.lines.push_back("ausentes nesta peça (a tendência não se cumpriu): " + absent);
    std::string ax;
    for (const auto& [a, v] : d.axes)
        ax += (ax.empty() ? "" : ", ") + a + " " + fmt2(v);
    if (!ax.empty())
        d.lines.push_back("eixos: " + ax + (repetition >= 0.6f ? " — o conceito pede a repetição: a novidade cede" : ""));
    return d;
}

std::vector<std::string> influenceAliasWords(const std::vector<InfluenceNode>& archive, const std::string& text)
{
    std::vector<std::string> out;
    const auto tw = splitWords(text);
    for (const auto& n : archive)
        for (const auto& a : n.aliases)
        {
            const auto aw = splitWords(a);
            for (std::size_t i = 0; !aw.empty() && i + aw.size() <= tw.size(); ++i)
                if (std::equal(aw.begin(), aw.end(), tw.begin() + static_cast<std::ptrdiff_t>(i)))
                    for (const auto& w : aw)
                        if (std::find(out.begin(), out.end(), w) == out.end())
                            out.push_back(w);
        }
    return out;
}

std::vector<std::string> influencesFromWords(const std::vector<InfluenceNode>& archive, const std::string& text)
{
    // os outros nomes dos nós, com mais de uma palavra ("coco de roda"): o
    // nome inteiro chama o nó, e as suas palavras não seguem avulsas
    std::vector<std::string> out;
    const auto consumed = influenceAliasWords(archive, text);
    for (const auto& n : archive)
        for (const auto& a : n.aliases)
        {
            const auto aw = splitWords(a);
            const auto tw = splitWords(text);
            for (std::size_t i = 0; !aw.empty() && i + aw.size() <= tw.size(); ++i)
                if (std::equal(aw.begin(), aw.end(), tw.begin() + static_cast<std::ptrdiff_t>(i))
                    && std::find(out.begin(), out.end(), n.path) == out.end())
                    out.push_back(n.path);
        }
    // as palavras sem caixa, acento, pontuação nem símbolos (TextFold.h); e
    // os nós de duas palavras: "post rock" = rock/post-rock
    auto words = splitWords(text);
    words.erase(std::remove_if(words.begin(), words.end(),
                               [&](const std::string& w) { return std::find(consumed.begin(), consumed.end(), w) != consumed.end(); }),
                words.end());
    {
        std::vector<std::string> merged;
        for (std::size_t i = 0; i < words.size(); ++i)
        {
            if (i + 1 < words.size())
            {
                const auto joined = words[i] + "-" + words[i + 1];
                const bool node = std::any_of(archive.begin(), archive.end(), [&](const InfluenceNode& n) {
                    const auto f = foldText(n.path);
                    return f.size() >= joined.size() && f.compare(f.size() - joined.size(), joined.size(), joined) == 0
                           && (f.size() == joined.size() || f[f.size() - joined.size() - 1] == '/');
                });
                if (node)
                {
                    merged.push_back(joined);
                    ++i;
                    continue;
                }
            }
            merged.push_back(words[i]);
        }
        words = merged;
    }
    for (const auto& word : words)
    {
        const InfluenceNode* best = nullptr;
        std::size_t bestDepth = 99;
        for (const auto& n : archive)
        {
            const auto path = foldText(n.path);
            const bool whole = path == word;
            bool segment = false;
            std::size_t start = 0, depth = 0;
            while (true)
            {
                const auto slash = path.find('/', start);
                if (path.substr(start, slash == std::string::npos ? std::string::npos : slash - start) == word)
                    segment = true;
                ++depth;
                if (slash == std::string::npos)
                    break;
                start = slash + 1;
            }
            // o nó cujo ÚLTIMO segmento é a palavra (o mais raso entre eles);
            // os nós qualificativos (rock/dançante, jazz/livre…) só com a
            // palavra do pai também no texto: "dançante" sozinha é o eixo, não o rock
            const auto last = path.substr(path.rfind('/') == std::string::npos ? 0 : path.rfind('/') + 1);
            static const char* const qualifiers[] = { "dançante", "introspectivo", "clássico", "livre", "lento", "modal" };
            bool qualifier = false;
            for (const char* q : qualifiers)
                qualifier = qualifier || last == foldText(q);
            if (qualifier && !whole)
            {
                const auto parent = path.substr(0, path.rfind('/'));
                const auto parentLast = parent.substr(parent.rfind('/') == std::string::npos ? 0 : parent.rfind('/') + 1);
                if (std::find(words.begin(), words.end(), parentLast) == words.end())
                    continue;
            }
            if ((whole || (segment && last == word)) && depth < bestDepth)
            {
                best = &n;
                bestDepth = depth;
            }
        }
        // nenhum nó com o nome exato: o mais perto com um erro pequeno
        // ("marcatu", "maracatús", "postrock") — nunca os qualificativos
        if (!best && word.size() >= 5)
        {
            int bestDist = 99;
            for (const auto& n : archive)
            {
                const auto path = foldText(n.path);
                const auto last = path.substr(path.rfind('/') == std::string::npos ? 0 : path.rfind('/') + 1);
                static const char* const qualifiers[] = { "dancante", "introspectivo", "classico", "livre", "lento", "modal" };
                if (std::any_of(std::begin(qualifiers), std::end(qualifiers), [&](const char* q) { return last == q; }))
                    continue;
                if (!fuzzyEqual(word, last))
                    continue;
                const int dist = editDistance(word, last);
                if (dist < bestDist)
                {
                    best = &n;
                    bestDist = dist;
                }
            }
        }
        if (best && std::find(out.begin(), out.end(), best->path) == out.end())
            out.push_back(best->path);
    }
    return out;
}

InfluencePoint influencePoint(const InfluenceDraw& d)
{
    InfluencePoint p;
    const auto get = [&](const char* a) {
        const auto it = d.axes.find(a);
        return it == d.axes.end() ? 0.5f : it->second;
    };
    p.dance = get("dança");
    p.weight = get("peso");
    p.noise = get("ruído");
    p.rasgo = get("rasgo");
    p.ensemble = d.ensemble;
    p.harmony = d.harmony;
    return p;
}

double influenceDistance(const InfluencePoint& a, const InfluencePoint& b)
{
    const double e = std::sqrt(std::pow(a.dance - b.dance, 2.0) + std::pow(a.weight - b.weight, 2.0) + std::pow(a.noise - b.noise, 2.0)
                               + std::pow(a.rasgo - b.rasgo, 2.0));
    return e + (a.ensemble != b.ensemble ? 0.5 : 0.0) + (a.harmony != b.harmony ? 0.5 : 0.0);
}

std::size_t pickNovelDraw(const std::vector<InfluenceDraw>& candidates, const std::vector<InfluencePoint>& past, double* distance)
{
    if (candidates.empty())
        return 0;
    const auto rep = [](const InfluenceDraw& d) {
        const auto it = d.axes.find("repetição");
        return it == d.axes.end() ? 0.3f : it->second;
    };
    const auto nearest = [&](const InfluenceDraw& d) {
        double m = 1e9;
        for (const auto& q : past)
            m = std::min(m, influenceDistance(influencePoint(d), q));
        return past.empty() ? 0.0 : m;
    };
    std::size_t best = 0;
    if (rep(candidates.front()) < 0.6f && !past.empty())
        for (std::size_t k = 1; k < candidates.size(); ++k)
            if (nearest(candidates[k]) > nearest(candidates[best]))
                best = k;
    if (distance)
        *distance = nearest(candidates[best]);
    return best;
}

std::vector<std::string> applyInfluenceOptions(Options& opt, const InfluenceDraw& d)
{
    std::vector<std::string> lines;
    if (opt.ensemble == 0 && !d.ensemble.empty())
    {
        Ensemble e;
        if (parseEnsemble(d.ensemble, e))
            opt.ensemble = static_cast<int>(e);
    }
    if (opt.rockStyle == 0 && !d.rockStyle.empty())
    {
        RockStyle r;
        if (parseRockStyle(d.rockStyle, r))
            opt.rockStyle = static_cast<int>(r);
    }
    if (opt.harmonyLanguage == 0 && !d.harmony.empty())
    {
        HarmonyLanguage h;
        if (parseHarmonyLanguage(d.harmony, h))
            opt.harmonyLanguage = static_cast<int>(h);
    }
    if (opt.intention == Intention::none && !d.mood.empty())
    {
        Intention i;
        if (parseIntention(d.mood, i))
            opt.intention = i;
    }
    if (opt.mix.empty() && !d.scale.empty())
        opt.mix = { d.scale };
    if (opt.cycleQuery.empty() && !d.cycle.empty())
        opt.cycleQuery = d.cycle;
    if (opt.narrative.empty() && !d.narrative.empty())
        opt.narrative = d.narrative;
    // o rasgo e o ruído puxam o rasgo modular (a metade do caminho)
    const auto ax = [&](const char* a) {
        const auto it = d.axes.find(a);
        return it == d.axes.end() ? -1.0f : it->second;
    };
    float target = std::max(ax("rasgo"), ax("ruído"));
    if (target >= 0.0f)
    {
        const float before = opt.rasgo;
        opt.rasgo = std::clamp(0.5f * (opt.rasgo + target), 0.0f, 1.0f);
        lines.push_back("o rasgo modular pelas influências: " + fmt2(before) + " → " + fmt2(opt.rasgo));
    }
    return lines;
}

std::string applyCharacter(Score& score, const std::map<std::string, float>& axes, std::uint64_t seed, const TensionCurve* curve)
{
    const auto get = [&](const char* a) {
        const auto it = axes.find(a);
        return it == axes.end() ? 0.5f : it->second;
    };
    const float dance0 = get("dança") - 0.5f, weight0 = get("peso") - 0.5f, density0 = get("densidade") - 0.5f;
    if (!curve && std::abs(dance0) < 0.05f && std::abs(weight0) < 0.05f && std::abs(density0) < 0.05f)
        return {};
    Rng rng { seed ^ 0xCA4A7E5ull };
    int thinned = 0, shaped = 0, moved = 0, sparser = 0, repeated = 0;
    std::vector<Note> out;
    out.reserve(score.notes.size());
    for (auto n : score.notes)
    {
        // a forma: os eixos sobem no clímax e descem nos vales (±0,2)
        const float f = curve ? 0.4f * (curve->at(n.time) - 0.5f) : 0.0f;
        const float dance = std::clamp(dance0 + f, -0.5f, 0.5f), weight = std::clamp(weight0 + f, -0.5f, 0.5f),
                    density = std::clamp(density0 + f, -0.5f, 0.5f);
        // a densidade: abaixo do meio, as vozes de enfeite e de harmonia
        // rareiam (o motivo e o baixo ficam); acima, as notas longas do motivo,
        // do contracanto e da harmonia se partem em ataques repetidos
        if (density < 0.0f && (n.voice == 2 || n.voice == 3 || n.voice == 5 || n.voice == 6) && rng.next() < -density * 1.3)
        {
            ++sparser;
            continue;
        }
        if (density > 0.0f && (n.voice == 0 || n.voice == 2 || n.voice == 6) && n.duration >= 0.3 && rng.next() < density * 1.6)
        {
            const double step = n.voice == 6 ? 0.25 : 0.125 + 0.125 * rng.next();
            const int parts = std::min(8, static_cast<int>(n.duration / step));
            if (parts >= 2)
            {
                for (int k = 0; k < parts; ++k)
                {
                    Note r = n;
                    r.time = n.time + k * step;
                    r.duration = step * 0.85;
                    r.velocity = std::clamp(n.velocity * (k == 0 ? 1.0f : 0.8f), 0.05f, 1.0f);
                    out.push_back(r);
                }
                ++repeated;
                continue;
            }
        }
        // a dança: o groove (percussão acentuada, baixo e harmonia curtos) ou
        // o introspectivo (percussão rara, notas longas)
        if (n.voice == 4)
        {
            if (dance < 0.0f && rng.next() < -dance * 1.2)
            {
                ++thinned;
                continue;
            }
            n.velocity = std::clamp(n.velocity * (1.0f + 0.5f * dance), 0.05f, 1.0f);
        }
        else if (n.voice == 1 || n.voice == 6)
        {
            n.duration = std::max(0.05, n.duration * (1.0 - 0.9 * dance));
            ++shaped;
        }
        else if (dance < 0.0f)
            n.duration *= 1.0 - 0.5 * dance;
        // o peso: a força e o registro do baixo e da harmonia
        n.velocity = std::clamp(n.velocity * (1.0f + 0.35f * weight), 0.05f, 1.0f);
        if (weight > 0.3f && ((n.voice == 1 && n.pitch - 12.0f >= 28.0f) || (n.voice == 6 && n.pitch - 12.0f >= 40.0f)))
        {
            n.pitch -= 12.0f;
            ++moved;
        }
        else if (weight < -0.3f && n.voice == 6 && n.pitch + 12.0f <= 84.0f)
        {
            n.pitch += 12.0f;
            ++moved;
        }
        out.push_back(n);
    }
    score.notes = std::move(out);
    char buf[320];
    std::snprintf(buf, sizeof buf, "caráter: dança %+.2f, peso %+.2f, densidade %+.2f · %d golpes de percussão rareados, %d notas do baixo e da harmonia rearticuladas, %d mudaram de oitava, %d notas rareadas, %d partidas em ataques repetidos",
                  static_cast<double>(dance0), static_cast<double>(weight0), static_cast<double>(density0), thinned, shaped, moved, sparser, repeated);
    return buf;
}

namespace
{
// a família de timbre pelo nome (timbreName), ou −1
int timbreByName(const std::string& name)
{
    for (int k = 0; k < 200; ++k)
    {
        const char* n = timbreName(static_cast<Timbre>(k));
        if (!n || std::string(n) == "?")
            break;
        if (name == n)
            return k;
    }
    return -1;
}

int voiceOfRole(const std::string& role)
{
    static const std::map<std::string, int> roles { { "motivo", 0 }, { "baixo", 1 }, { "contracanto", 2 }, { "memória", 3 },
                                                    { "percussão", 4 }, { "mosaico", 5 }, { "harmonia", 6 } };
    const auto it = roles.find(role);
    return it == roles.end() ? -1 : it->second;
}

// troca o timbre das vozes dos papéis, entre `a` e `b` segundos
int retimbre(Score& score, const std::map<std::string, std::string>& timbres, double a, double b)
{
    int count = 0;
    for (const auto& [role, name] : timbres)
    {
        const int voice = voiceOfRole(role), t = timbreByName(name);
        if (voice < 0 || voice == 4 || t < 0)
            continue;
        for (auto& n : score.notes)
            if (n.voice == voice && n.time >= a && n.time < b)
            {
                n.timbre = static_cast<Timbre>(t);
                ++count;
            }
    }
    return count;
}
} // namespace

std::string applyInfluenceScore(Score& score, const InfluenceDraw& d, std::uint64_t seed, const TensionCurve* curve,
                                const std::string& narrative)
{
    int removed = 0;
    for (const auto& [role, present] : d.presence)
    {
        const int voice = voiceOfRole(role);
        if (present || voice <= 0)
            continue;
        const auto before = score.notes.size();
        score.notes.erase(std::remove_if(score.notes.begin(), score.notes.end(), [&](const Note& n) { return n.voice == voice; }),
                          score.notes.end());
        removed += static_cast<int>(before - score.notes.size());
    }
    // os timbres sorteados: a família de cada papel
    const int retimbred = retimbre(score, d.timbres, -1.0, 1e12);
    // o caráter, que segue a forma da peça
    std::string line = applyCharacter(score, d.axes, seed, curve);
    // a virada do ki-shō-ten-ketsu traz o galho distante
    if (narrative.find("kishotenketsu") != std::string::npos && !d.twistNode.empty() && score.duration > 0.0)
    {
        const double a = 0.5 * score.duration, b = 0.75 * score.duration;
        const int tw = retimbre(score, d.twistTimbres, a, b);
        Score w;
        w.duration = score.duration;
        std::vector<Note> keep;
        for (const auto& n : score.notes)
            (n.time >= a && n.time < b ? w.notes : keep).push_back(n);
        (void) applyCharacter(w, d.twistAxes, seed ^ 0x7E157ull, nullptr);
        keep.insert(keep.end(), w.notes.begin(), w.notes.end());
        std::stable_sort(keep.begin(), keep.end(), [](const Note& x, const Note& y) { return x.time < y.time; });
        score.notes = std::move(keep);
        score.marks.push_back({ a, "a virada traz o galho distante: \"" + d.twistNode + "\" (os seus timbres e eixos, de 50 a 75 %)" });
        line += (line.empty() ? "" : " · ") + std::string("a virada traz \"") + d.twistNode + "\" (" + std::to_string(tw) + " notas no timbre dele)";
    }
    if (retimbred > 0)
        line = "influências: " + std::to_string(retimbred) + " notas com o timbre sorteado" + (line.empty() ? "" : " · " + line);
    if (removed > 0)
        line = "influências: " + std::to_string(removed) + " notas das vozes ausentes saíram" + (line.empty() ? "" : " · " + line);
    return line;
}

} // namespace trioio::composition
