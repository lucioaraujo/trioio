#include "SemanticBridge.h"

#include "DataFiles.h"
#include "TextFold.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <chrono>
#include <future>
#include <map>
#include <mutex>
#include <sstream>
#include <cstdlib>
#include <tuple>

#if JUCE_LINUX
 #include <cstdio>
#endif

namespace trioio::library
{
namespace
{
using Clock = std::chrono::steady_clock;

juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.c_str()); }

// a identificação que a política da Wikimedia pede: o nome, a versão e o contato
#ifndef TRIOIO_VERSION
 #define TRIOIO_VERSION "dev"
#endif
const char* const kUserAgent = "Trioio/" TRIOIO_VERSION " (https://github.com/lucioaraujo/trioio; RASGO, instrumento musical)";

int remainingMs(Clock::time_point deadline)
{
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now()).count());
}

// uma requisição HTTPS até o prazo: a rede nativa do JUCE (Windows, macOS);
// no Linux, o curl do sistema (o JUCE aqui é compilado sem a libcurl)
juce::String fetch(const juce::String& url, Clock::time_point deadline)
{
    const int ms = remainingMs(deadline);
    if (ms < 150)
        return {};
#if JUCE_LINUX
    // popen: o endereço vai entre aspas simples (já vem codificado; uma aspa
    // vira %27). O ChildProcess do JUCE custava ~2 s de espera por consulta.
    const auto cmd = "curl -s -L --max-time " + juce::String(ms / 1000.0, 2) + " -A '" + juce::String(kUserAgent) + "' '"
                     + url.replace("'", "%27") + "' 2>/dev/null";
    FILE* f = popen(cmd.toRawUTF8(), "r");
    if (!f)
        return {};
    juce::MemoryBlock mb;
    char buf[8192];
    for (size_t n; (n = fread(buf, 1, sizeof buf, f)) > 0;)
        mb.append(buf, n);
    const int status = pclose(f);
    if (std::getenv("TRIOIO_SEMANTICA_DEBUG"))
        std::fprintf(stderr, "[%d ms, status %d, %d bytes] %s\n", ms, status, static_cast<int>(mb.getSize()), url.toRawUTF8());
    // o status do pclose não decide: um processo que ignore o SIGCHLD recebe
    // -1 mesmo com o curl bem-sucedido. A resposta que chegou vale (o JSON é
    // conferido por quem lê); a que não chegou fica vazia.
    juce::ignoreUnused(status);
    return mb.toString();
#else
    auto stream = juce::URL(url).createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                       .withConnectionTimeoutMs(ms)
                                                       .withExtraHeaders(juce::String("User-Agent: ") + kUserAgent));
    return stream ? stream->readEntireStreamAsString() : juce::String();
#endif
}

juce::var fetchJson(const juce::String& url, Clock::time_point deadline)
{
    const auto text = fetch(url, deadline);
    return text.isEmpty() ? juce::var() : juce::JSON::parse(text);
}

juce::String fill(const std::string& tpl, const std::string& word, const std::string& en, const std::string& lang = {})
{
    const auto enc = [](const std::string& s) { return juce::URL::addEscapeChars(u8(s), true); };
    return u8(tpl).replace("{palavra}", enc(word)).replace("{en}", enc(en)).replace("{lingua}", u8(lang));
}

// wikidata-lexema, em lote para todas as palavras: UMA busca por palavra
// (a busca de lexemas acha todas as línguas; a língua vem em "match"), duas
// por vez; depois UMA requisição com os sentidos de todos os lexemas
// achados e UMA com os lemas ingleses (P5972) e os rótulos dos conceitos
// (P5137, quando o sentido não tem a tradução), estas duas juntas. A Wikimedia
// limita rajadas de clientes sem conta (em 2026-10-18 uma rajada de 30
// buscas paralelas recebeu 429;
// https://www.mediawiki.org/wiki/Wikimedia_APIs/Rate_limits).
using Translation = std::pair<std::string, std::string>;  // (inglês, de onde veio)

std::map<std::string, Translation> translateWikidata(const SemanticSource& s, const std::vector<std::string>& words,
                                                      Clock::time_point deadline)
{
    std::map<std::string, Translation> out;
    const auto api = u8(s.url);
    // 1. as buscas (duas por vez)
    std::map<std::string, std::vector<std::pair<std::string, juce::String>>> found;  // palavra → (língua, lexema), na preferência
    for (size_t i = 0; i < words.size(); i += 2)
    {
        std::vector<std::future<juce::var>> pair;
        for (size_t k = i; k < std::min(words.size(), i + 2); ++k)
            pair.push_back(std::async(std::launch::async, [&, k] {
                return fetchJson(api + "?action=wbsearchentities&type=lexeme&format=json&limit=12&language="
                                     + u8(s.languages.front()) + "&search=" + juce::URL::addEscapeChars(u8(words[k]), true),
                                 deadline);
            }));
        for (size_t k = i; k < std::min(words.size(), i + 2); ++k)
        {
            const auto res = pair[k - i].get();
            if (const auto* hits = res["search"].getArray())
                for (const auto& lang : s.languages)
                    for (const auto& h : *hits)
                        if (h["match"]["language"].toString() == u8(lang)
                            && h["match"]["text"].toString().toLowerCase() == u8(words[k]))
                            found[words[k]].push_back({ lang, h["id"].toString() });
        }
    }
    // 2. os sentidos de todos os lexemas não ingleses, numa requisição
    juce::StringArray lexemes;
    for (auto& [w, list] : found)
    {
        if (!list.empty() && list.front().first == "en")
            out[w] = { w, "Wikidata " + list.front().second.toStdString() + " (en)" };
        else
            for (const auto& [lang, id] : list)
                if (lang != "en" && lexemes.size() < 50)
                    lexemes.addIfNotAlreadyThere(id);
    }
    if (lexemes.isEmpty())
        return out;
    const auto ent = fetchJson(api + "?action=wbgetentities&format=json&ids=" + lexemes.joinIntoString("|"), deadline);
    // a tradução do sentido (P5972 → um lexema inglês) ou, quando o sentido
    // não a tem, o conceito do sentido (P5137 → um item, o rótulo inglês)
    std::map<juce::String, juce::StringArray> targetsOf, itemsOf;  // lexema → lexemas ingleses / itens
    juce::StringArray allTargets, allItems;
    const auto collect = [](const juce::var& sense, const char* prop, juce::StringArray& mine, juce::StringArray& all) {
        if (const auto* cl = sense["claims"][prop].getArray())
            for (const auto& c : *cl)
                if (const auto t = c["mainsnak"]["datavalue"]["value"]["id"].toString(); t.isNotEmpty())
                {
                    const auto id = t.upToFirstOccurrenceOf("-", false, false);
                    mine.addIfNotAlreadyThere(id);
                    if (all.size() < 50)
                        all.addIfNotAlreadyThere(id);
                }
    };
    for (const auto& id : lexemes)
        if (const auto* senses = ent["entities"][id.toRawUTF8()]["senses"].getArray())
            for (const auto& sense : *senses)
            {
                collect(sense, "P5972", targetsOf[id], allTargets);
                collect(sense, "P5137", itemsOf[id], allItems);
            }
    if (allTargets.isEmpty() && allItems.isEmpty())
        return out;
    // 3. os lemas ingleses e os rótulos ingleses dos itens, as duas em paralelo
    auto lemF = std::async(std::launch::async, [&] {
        return allTargets.isEmpty() ? juce::var()
                                    : fetchJson(api + "?action=wbgetentities&format=json&ids=" + allTargets.joinIntoString("|"), deadline);
    });
    auto itemF = std::async(std::launch::async, [&] {
        return allItems.isEmpty() ? juce::var()
                                  : fetchJson(api + "?action=wbgetentities&format=json&props=labels&languages=en&ids="
                                                  + allItems.joinIntoString("|"),
                                              deadline);
    });
    const auto lem = lemF.get();
    const auto items = itemF.get();
    for (auto& [w, list] : found)
        for (const auto& [lang, id] : list)
        {
            if (out.count(w))
                break;
            for (const auto& t : targetsOf[id])
                if (const auto lemma = lem["entities"][t.toRawUTF8()]["lemmas"]["en"]["value"].toString(); lemma.isNotEmpty())
                {
                    out[w] = { lemma.toLowerCase().toStdString(), "Wikidata " + id.toStdString() + " (" + lang + ") → " + t.toStdString() };
                    break;
                }
            if (out.count(w))
                break;
            for (const auto& q : itemsOf[id])
                if (const auto label = items["entities"][q.toRawUTF8()]["labels"]["en"]["value"].toString(); label.isNotEmpty())
                {
                    out[w] = { label.toLowerCase().toStdString(), "Wikidata " + id.toStdString() + " (" + lang + ") → " + q.toStdString() };
                    break;
                }
        }
    // uma palavra que também existe em inglês e não tem tradução: fica como veio
    for (auto& [w, list] : found)
        if (!out.count(w) && std::any_of(list.begin(), list.end(), [](const auto& f) { return f.first == "en"; }))
            out[w] = { w, "Wikidata (en)" };
    return out;
}

juce::File cacheFile(const std::string& dataRoot)
{
    return juce::File(u8(dataRoot)).getChildFile("memoria").getChildFile("semantica.json");
}

std::mutex& cacheMutex()
{
    static std::mutex m;
    return m;
}

SemanticLookup fromCache(const juce::var& cache, const std::string& word)
{
    SemanticLookup r;
    r.word = word;
    const auto hit = cache[u8(word).toRawUTF8()];
    if (!hit.isObject())
        return r;
    // a palavra que as fontes responderam não conhecer: não se pergunta de
    // novo por 7 dias (a cada versão gerada a mesma consulta voltava)
    if (static_cast<bool>(hit["unknown"]))
    {
        const auto when = juce::Time::fromISO8601(hit["when"].toString());
        if (juce::Time::getCurrentTime() - when < juce::RelativeTime::days(7))
        {
            r.cached = true;
            r.error = "as fontes não conhecem a palavra (memória)";
        }
        return r;
    }
    r.english = hit["english"].toString().toStdString();
    r.via = hit["via"].toString().toStdString();
    if (const auto* a = hit["associations"].getArray())
        for (const auto& x : *a)
            r.associations.push_back({ x[0].toString().toStdString(), static_cast<double>(x[1]) });
    r.ok = !r.associations.empty();
    r.cached = true;
    return r;
}

std::string normalized(const std::string& w)
{
    return u8(w).trim().toLowerCase().toStdString();
}

// as associações de UM conceito: todas as fontes em paralelo, somadas
void associate(SemanticLookup& r, const std::vector<SemanticSource>& sources, Clock::time_point deadline)
{
    std::vector<std::future<std::tuple<const SemanticSource*, std::vector<std::pair<std::string, double>>, std::string>>> as;
    for (const auto& s : sources)
        if (s.role == "associacao")
            as.push_back(std::async(std::launch::async, [&s, &r, deadline] {
                const auto body = fetch(fill(s.url, r.word, r.english), deadline).toStdString();
                return std::make_tuple(&s, parseAssociationResponse(s.format, body), body);
            }));
    std::map<std::string, double> sum;
    std::vector<std::string> order;
    juce::StringArray used;
    bool answered = false;
    for (auto& f : as)
    {
        auto [s, list, body] = f.get();
        answered = answered || juce::String(u8(body)).trim().startsWithChar('[') || juce::String(u8(body)).trim().startsWithChar('{');
        if (list.empty())
            continue;
        used.addIfNotAlreadyThere(juce::URL(u8(s->url)).getDomain());
        for (const auto& [w, v] : list)
        {
            if (!sum.count(w))
                order.push_back(w);
            sum[w] += v * s->weight;
        }
    }
    double mx = 0.0;
    for (const auto& [w, v] : sum)
        mx = std::max(mx, v);
    for (const auto& w : order)
        r.associations.push_back({ w, std::clamp(sum[w] / std::max(mx, 1e-9), 0.05, 1.0) });
    r.ok = !r.associations.empty();
    if (r.ok)
        r.via += " · " + used.joinIntoString(", ").toStdString() + " (" + std::to_string(r.associations.size()) + " associações)";
    else
        r.error = answered ? "as fontes responderam, mas não conhecem a palavra"
                  : remainingMs(deadline) <= 150 ? "o tempo limite acabou"
                                                 : "as fontes não responderam (sem internet, ou fora do ar)";
}
} // namespace

std::vector<SemanticSource> parseSemanticSources(const std::string& text, std::vector<std::string>& errors)
{
    std::vector<SemanticSource> out;
    std::istringstream in(text);
    std::string line;
    int n = 0;
    while (std::getline(in, line))
    {
        ++n;
        const auto t = juce::String(u8(line)).trim();
        if (t.isEmpty() || t.startsWithChar('#'))
            continue;
        juce::StringArray tok;
        tok.addTokens(t, " \t", "");
        tok.removeEmptyStrings();
        SemanticSource s;
        if (tok.size() < 3)
        {
            errors.push_back("linha " + std::to_string(n) + ": esperado <papel> <formato> <endereço> [peso]");
            continue;
        }
        s.role = tok[0].toStdString();
        s.format = tok[1].toStdString();
        s.url = tok[2].toStdString();
        for (int i = 3; i < tok.size(); ++i)
        {
            if (tok[i].startsWith("linguas="))
            {
                juce::StringArray ls;
                ls.addTokens(tok[i].fromFirstOccurrenceOf("=", false, false), ",", "");
                for (const auto& l : ls)
                    if (l.isNotEmpty())
                        s.languages.push_back(l.trim().toStdString());
            }
            else
                s.weight = tok[i].getDoubleValue();
        }
        const bool roleOk = s.role == "traducao" || s.role == "associacao" || s.role == "ortografia";
        const bool fmtOk = (s.role == "traducao" && s.format == "wikidata-lexema") || (s.role == "ortografia" && s.format == "datamuse-sug")
                           || (s.role == "associacao" && (s.format == "datamuse" || s.format == "conceptnet"));
        if (!roleOk || !fmtOk || !juce::String(u8(s.url)).startsWith("https://"))
        {
            errors.push_back("linha " + std::to_string(n) + ": papel/formato desconhecido ou endereço sem https://");
            continue;
        }
        if (s.format == "wikidata-lexema" && s.languages.empty())
            s.languages = { "pt", "es", "fr", "it", "de", "en" };
        out.push_back(std::move(s));
    }
    return out;
}

std::vector<SemanticSource> defaultSemanticSources()
{
    std::vector<std::string> errors;
    return parseSemanticSources(trioioDataFile("semantica/fontes.txt").loadFileAsString().toStdString(), errors);
}

std::vector<std::pair<std::string, double>> parseAssociationResponse(const std::string& format, const std::string& body)
{
    std::vector<std::pair<std::string, double>> out;
    const auto v = juce::JSON::parse(u8(body));
    if (format == "datamuse")
    {
        if (const auto* arr = v.getArray())
        {
            double mx = 1.0;
            for (const auto& x : *arr)
                mx = std::max(mx, static_cast<double>(x["score"]));
            for (const auto& x : *arr)
                if (const auto w = x["word"].toString(); w.isNotEmpty())
                    out.push_back({ w.toLowerCase().toStdString(), std::max(0.05, static_cast<double>(x["score"]) / mx) });
        }
    }
    else if (format == "conceptnet")
    {
        // /related: {"related": [{"@id": "/c/en/drizzle", "weight": 0.71}, …]}
        if (const auto* arr = v["related"].getArray())
            for (const auto& x : *arr)
            {
                const auto id = x["@id"].toString();
                const auto w = id.fromLastOccurrenceOf("/", false, false).replace("_", " ");
                if (w.isNotEmpty())
                    out.push_back({ w.toLowerCase().toStdString(), std::clamp(static_cast<double>(x["weight"]), 0.05, 1.0) });
            }
    }
    return out;
}

SemanticLookup cachedSemantic(const std::string& dataRoot, const std::string& word)
{
    std::lock_guard<std::mutex> lock(cacheMutex());
    return fromCache(juce::JSON::parse(cacheFile(dataRoot)), normalized(word));
}

std::vector<SemanticLookup> lookupSemantic(const std::string& dataRoot, const std::vector<std::string>& wordsIn,
                                           const std::vector<SemanticSource>& sources, int budgetMs)
{
    const auto deadline = Clock::now() + std::chrono::milliseconds(std::max(200, budgetMs));
    std::vector<SemanticLookup> out;
    juce::var cache;
    {
        std::lock_guard<std::mutex> lock(cacheMutex());
        cache = juce::JSON::parse(cacheFile(dataRoot));
    }
    std::vector<std::string> ask;
    for (const auto& w0 : wordsIn)
    {
        const auto w = normalized(w0);
        if (w.empty() || std::find(ask.begin(), ask.end(), w) != ask.end())
            continue;
        if (auto c = fromCache(cache, w); c.ok || c.cached)
            out.push_back(std::move(c));
        else if (!sources.empty())
            ask.push_back(w);
    }
    std::vector<SemanticLookup> fresh;
    if (!ask.empty())
    {
        const auto t0 = Clock::now();
        // 1. a tradução, com até 70 % do tempo (as associações precisam do resto);
        //    várias fontes de tradução em paralelo, a primeira da lista vence
        const auto trDeadline = Clock::now() + std::chrono::milliseconds(remainingMs(deadline) * 7 / 10);
        std::vector<std::future<std::map<std::string, Translation>>> tr;
        for (const auto& s : sources)
            if (s.role == "traducao" && s.format == "wikidata-lexema")
                tr.push_back(std::async(std::launch::async, [&s, &ask, trDeadline] { return translateWikidata(s, ask, trDeadline); }));
        // a ortografia, junto: a sugestão para a palavra que nenhuma fonte
        // traduzir ("janaese" → "japanese"), só se estiver a 1–2 letras
        std::vector<std::future<std::pair<std::string, std::string>>> sp;
        for (const auto& w : ask)
            for (const auto& s : sources)
                if (s.role == "ortografia" && w.size() >= 5)  // as curtas não ("mar" virava "mark")
                {
                    sp.push_back(std::async(std::launch::async, [&s, w, trDeadline] {
                        const auto list = parseAssociationResponse("datamuse", fetch(fill(s.url, w, w), trDeadline).toStdString());
                        if (!list.empty() && list.front().first != w
                            && composition::editDistance(composition::foldText(list.front().first), w) <= 2)
                            return std::make_pair(w, list.front().first);
                        return std::make_pair(w, std::string());
                    }));
                    break;
                }
        std::vector<std::map<std::string, Translation>> translations;
        for (auto& f : tr)
            translations.push_back(f.get());
        std::map<std::string, std::string> spelling;
        for (auto& f : sp)
            if (auto [w, fix] = f.get(); !fix.empty())
                spelling[w] = fix;
        fresh.resize(ask.size());
        for (size_t i = 0; i < ask.size(); ++i)
        {
            fresh[i].word = ask[i];
            for (const auto& m : translations)
                if (const auto it = m.find(ask[i]); it != m.end())
                {
                    std::tie(fresh[i].english, fresh[i].via) = it->second;
                    break;
                }
            if (fresh[i].english.empty() && spelling.count(ask[i]))
            {
                fresh[i].english = spelling[ask[i]];
                fresh[i].via = "ortografia ≈ " + spelling[ask[i]] + " (Datamuse)";
            }
            if (fresh[i].english.empty())
            {
                fresh[i].english = ask[i];  // talvez já seja inglês: as associações tentam assim mesmo
                fresh[i].via = "sem tradução";
            }
        }
        // 2. as associações de todas as palavras, em paralelo
        std::vector<std::future<void>> as;
        for (auto& r : fresh)
            as.push_back(std::async(std::launch::async, [&r, &sources, deadline] { associate(r, sources, deadline); }));
        for (auto& f : as)
            f.get();
        const int ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count());
        for (auto& r : fresh)
            r.millis = ms;
    }
    // guarda o que respondeu (a memória da pessoa, nunca o repositório)
    const auto unknown = [](const SemanticLookup& r) { return !r.ok && r.error.rfind("as fontes responderam", 0) == 0; };
    if (std::any_of(fresh.begin(), fresh.end(), [&](const auto& r) { return r.ok || unknown(r); }))
    {
        std::lock_guard<std::mutex> lock(cacheMutex());
        const auto cf = cacheFile(dataRoot);
        auto disk = juce::JSON::parse(cf);
        if (!disk.isObject())
            disk = juce::var(new juce::DynamicObject());
        for (const auto& r : fresh)
            if (unknown(r))
            {
                auto o = juce::var(new juce::DynamicObject());
                o.getDynamicObject()->setProperty("unknown", true);
                o.getDynamicObject()->setProperty("when", juce::Time::getCurrentTime().toISO8601(true));
                disk.getDynamicObject()->setProperty(u8(r.word), o);
            }
            else if (r.ok)
            {
                auto o = juce::var(new juce::DynamicObject());
                o.getDynamicObject()->setProperty("english", u8(r.english));
                o.getDynamicObject()->setProperty("via", u8(r.via));
                o.getDynamicObject()->setProperty("when", juce::Time::getCurrentTime().toISO8601(true));
                juce::Array<juce::var> arr;
                for (const auto& [w, s] : r.associations)
                    arr.add(juce::Array<juce::var> { u8(w), s });
                o.getDynamicObject()->setProperty("associations", arr);
                disk.getDynamicObject()->setProperty(u8(r.word), o);
            }
        cf.getParentDirectory().createDirectory();
        (void) cf.replaceWithText(juce::JSON::toString(disk));
    }
    for (auto& r : fresh)
        out.push_back(std::move(r));
    return out;
}

} // namespace trioio::library
