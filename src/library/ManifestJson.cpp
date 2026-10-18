#include "ManifestJson.h"

#include <juce_core/juce_core.h>

#include <array>
#include <string_view>
#include <utility>

namespace trioio::library
{
using namespace trioio::provenance;

namespace
{
// ---- texto ↔ enum ---------------------------------------------------------
// Uma tabela por enum; a mesma serve para escrever e para ler.
template <typename E, std::size_t N>
using Table = std::array<std::pair<E, std::string_view>, N>;

constexpr Table<ManifestKind, 2> kKinds { { { ManifestKind::reference, "reference" },
                                            { ManifestKind::derivation, "derivation" } } };
constexpr Table<MaterialType, 3> kMaterials { { { MaterialType::audio, "audio" },
                                                { MaterialType::stem, "stem" },
                                                { MaterialType::midi, "midi" } } };
constexpr Table<OriginCategory, 7> kOrigins { {
    { OriginCategory::authorial, "authorial" },
    { OriginCategory::performerCapture, "performerCapture" },
    { OriginCategory::otherInstrument, "otherInstrument" },
    { OriginCategory::aiGenerated, "aiGenerated" },
    { OriginCategory::licensed, "licensed" },
    { OriginCategory::publicDomain, "publicDomain" },
    { OriginCategory::thirdPartyUnverified, "thirdPartyUnverified" },
} };
constexpr Table<AuthorizationStatus, 4> kStatus { { { AuthorizationStatus::pending, "pending" },
                                                    { AuthorizationStatus::authorized, "authorized" },
                                                    { AuthorizationStatus::refused, "refused" },
                                                    { AuthorizationStatus::revoked, "revoked" } } };
constexpr Table<AuthorizationBasis, 6> kBases { {
    { AuthorizationBasis::none, "none" },
    { AuthorizationBasis::ownWork, "ownWork" },
    { AuthorizationBasis::license, "license" },
    { AuthorizationBasis::publicDomain, "publicDomain" },
    { AuthorizationBasis::writtenPermission, "writtenPermission" },
    { AuthorizationBasis::serviceTerms, "serviceTerms" },
} };
constexpr Table<Use, 5> kUses { { { Use::listen, "listen" },
                                  { Use::analyze, "analyze" },
                                  { Use::learn, "learn" },
                                  { Use::derive, "derive" },
                                  { Use::publishDerivative, "publishDerivative" } } };
constexpr Table<AnalysisState, 6> kAnalysis { { { AnalysisState::none, "none" },
                                                { AnalysisState::indexing, "indexing" },
                                                { AnalysisState::analyzing, "analyzing" },
                                                { AnalysisState::cached, "cached" },
                                                { AnalysisState::changed, "changed" },
                                                { AnalysisState::unavailable, "unavailable" } } };
constexpr Table<Strategy, 4> kStrategies { { { Strategy::follow, "follow" },
                                             { Strategy::transfigure, "transfigure" },
                                             { Strategy::counter, "counter" },
                                             { Strategy::interrupt, "interrupt" } } };

template <typename E, std::size_t N>
juce::String name(const Table<E, N>& t, E value)
{
    for (const auto& [e, n] : t)
        if (e == value)
            return juce::String::fromUTF8(n.data(), static_cast<int>(n.size()));
    jassertfalse;
    return {};
}

// UTF-8 nos dois sentidos, sempre explícito (juce::String(const char*) é ASCII).
juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.data(), static_cast<int>(s.size())); }
std::string std8(const juce::String& s) { return s.toStdString(); }

// ---- leitor com registro de erros ----------------------------------------
struct Reader
{
    std::vector<std::string>& errors;

    void error(const juce::String& field, const juce::String& msg)
    {
        errors.push_back(std8(field + ": " + msg));
    }

    std::string text(const juce::var& obj, const char* key, const juce::String& path, bool required = false)
    {
        const auto& v = obj[key];
        if (v.isVoid())
        {
            if (required)
                error(path + key, juce::String::fromUTF8("campo obrigatório ausente"));
            return {};
        }
        if (!v.isString())
        {
            error(path + key, juce::String::fromUTF8("esperava texto"));
            return {};
        }
        return std8(v.toString());
    }

    double number(const juce::var& obj, const char* key, const juce::String& path)
    {
        const auto& v = obj[key];
        if (v.isVoid())
            return 0.0;
        if (!(v.isInt() || v.isInt64() || v.isDouble()))
        {
            error(path + key, juce::String::fromUTF8("esperava número"));
            return 0.0;
        }
        const double d = static_cast<double>(v);
        if (d < 0.0)
            error(path + key, juce::String::fromUTF8("não pode ser negativo"));
        return d;
    }

    bool boolean(const juce::var& obj, const char* key, const juce::String& path)
    {
        const auto& v = obj[key];
        if (v.isVoid())
            return false;
        if (!v.isBool())
        {
            error(path + key, juce::String::fromUTF8("esperava true ou false"));
            return false;
        }
        return static_cast<bool>(v);
    }

    template <typename E, std::size_t N>
    E closed(const juce::var& obj, const char* key, const juce::String& path, const Table<E, N>& t, E fallback)
    {
        const auto& v = obj[key];
        if (v.isVoid())
        {
            error(path + key, juce::String::fromUTF8("campo obrigatório ausente"));
            return fallback;
        }
        const auto s = v.toString();
        for (const auto& [e, n] : t)
            if (s == juce::String::fromUTF8(n.data(), static_cast<int>(n.size())))
                return e;
        error(path + key, juce::String::fromUTF8("valor desconhecido \"") + s + "\"");
        return fallback;
    }

    template <typename F>
    void each(const juce::var& obj, const char* key, const juce::String& path, F&& f)
    {
        const auto& v = obj[key];
        if (v.isVoid())
            return;
        if (!v.isArray())
        {
            error(path + key, juce::String::fromUTF8("esperava lista"));
            return;
        }
        int i = 0;
        for (const auto& item : *v.getArray())
            f(item, path + key + "[" + juce::String(i++) + "].");
    }
};

juce::var object() { return juce::var(new juce::DynamicObject()); }
void set(juce::var& o, const char* key, const juce::var& value) { o.getDynamicObject()->setProperty(key, value); }
} // namespace

namespace
{
template <typename E, std::size_t N>
bool byName(const Table<E, N>& t, const std::string& s, E& out)
{
    for (const auto& [e, n] : t)
        if (n == s)
        {
            out = e;
            return true;
        }
    return false;
}
} // namespace

bool parseOrigin(const std::string& s, OriginCategory& out) { return byName(kOrigins, s, out); }
bool parseBasis(const std::string& s, AuthorizationBasis& out) { return byName(kBases, s, out); }
bool parseUse(const std::string& s, Use& out) { return byName(kUses, s, out); }

std::string toJson(const Manifest& m)
{
    auto root = object();
    set(root, "schemaVersion", u8(m.schemaVersion));
    set(root, "id", u8(m.id));
    set(root, "createdAt", u8(m.createdAt));
    set(root, "updatedAt", u8(m.updatedAt));
    set(root, "kind", name(kKinds, m.kind));
    set(root, "material", name(kMaterials, m.material));

    auto id = object();
    set(id, "canonicalPath", u8(m.identity.canonicalPath));
    set(id, "sizeBytes", static_cast<juce::int64>(m.identity.sizeBytes));
    set(id, "modifiedAt", u8(m.identity.modifiedAt));
    set(id, "format", u8(m.identity.format));
    set(id, "durationSeconds", m.identity.durationSeconds);
    set(id, "sampleRate", static_cast<int>(m.identity.sampleRate));
    set(id, "channels", static_cast<int>(m.identity.channels));
    set(id, "contentSha256", u8(m.identity.contentSha256));
    set(root, "identity", id);

    auto origin = object();
    set(origin, "category", name(kOrigins, m.origin.category));
    juce::Array<juce::var> authors;
    for (const auto& p : m.origin.authors)
    {
        auto a = object();
        set(a, "name", u8(p.name));
        set(a, "role", u8(p.role));
        authors.add(a);
    }
    set(origin, "authors", authors);
    set(origin, "toolName", u8(m.origin.toolName));
    set(origin, "toolVersion", u8(m.origin.toolVersion));
    set(origin, "createdAt", u8(m.origin.createdAt));
    set(origin, "notes", u8(m.origin.notes));
    set(root, "origin", origin);

    auto auth = object();
    set(auth, "status", name(kStatus, m.authorization.status));
    set(auth, "basis", name(kBases, m.authorization.basis));
    juce::Array<juce::var> scope;
    for (const auto& [u, n] : kUses)
        if (has(m.authorization.scope, u))
            scope.add(name(kUses, u));
    set(auth, "scope", scope);
    set(auth, "license", u8(m.authorization.license));
    set(auth, "evidence", u8(m.authorization.evidence));
    set(auth, "declaredBy", u8(m.authorization.declaredBy));
    set(auth, "declaredAt", u8(m.authorization.declaredAt));
    set(auth, "restrictions", u8(m.authorization.restrictions));
    set(root, "authorization", auth);

    auto meta = object();
    for (const auto& [k, v] : m.metadataImported)
        meta.getDynamicObject()->setProperty(juce::Identifier(u8(k)), u8(v));
    set(root, "metadataImported", meta);

    set(root, "promptOriginalProvided", m.promptOriginalProvided);
    set(root, "promptsRef", u8(m.promptsRef));
    set(root, "analysis", name(kAnalysis, m.analysis));

    juce::Array<juce::var> relations;
    for (const auto& r : m.relations)
    {
        auto o = object();
        set(o, "type", u8(r.type));
        set(o, "target", u8(r.target));
        relations.add(o);
    }
    set(root, "relations", relations);

    if (m.derivation)
    {
        const auto& d = *m.derivation;
        auto o = object();
        juce::Array<juce::var> from;
        for (const auto& f : d.from)
            from.add(u8(f));
        set(o, "from", from);
        set(o, "sessionId", u8(d.sessionId));
        set(o, "decisionsRef", u8(d.decisionsRef));
        // seed em TEXTO: inteiros de 64 bits não cabem com exatidão no número
        // do JSON (double) para outros leitores; o seed precisa voltar idêntico.
        set(o, "seed", juce::String(static_cast<juce::uint64>(d.seed)));
        set(o, "strategy", name(kStrategies, d.strategy));
        set(o, "declaredRelation", u8(d.declaredRelation));
        set(o, "trioioVersion", u8(d.trioioVersion));
        set(root, "derivation", o);
    }

    juce::Array<juce::var> history;
    for (const auto& h : m.history)
    {
        auto o = object();
        set(o, "at", u8(h.at));
        set(o, "event", u8(h.event));
        set(o, "by", u8(h.by));
        set(o, "note", u8(h.note));
        history.add(o);
    }
    set(root, "history", history);

    return std8(juce::JSON::toString(root));
}

ManifestReadResult fromJson(const std::string& utf8Text)
{
    ManifestReadResult out;
    juce::var root;
    const auto parsed = juce::JSON::parse(u8(utf8Text), root);
    if (parsed.failed())
    {
        out.errors.push_back(std8(juce::String::fromUTF8("JSON inválido: ") + parsed.getErrorMessage()));
        return out;
    }
    if (!root.isObject())
    {
        out.errors.push_back("JSON: a raiz precisa ser um objeto");
        return out;
    }

    Reader r { out.errors };
    Manifest m;
    const juce::String top;

    // Versão primeiro: outra versão não é lida campo a campo como se fosse esta.
    m.schemaVersion = r.text(root, "schemaVersion", top, true);
    if (m.schemaVersion != kSchemaVersion)
    {
        if (out.errors.empty())
            r.error("schemaVersion", juce::String::fromUTF8("versão não suportada \"")
                                         + u8(m.schemaVersion) + "\"");
        return out;
    }

    m.id = r.text(root, "id", top, true);
    m.createdAt = r.text(root, "createdAt", top, true);
    m.updatedAt = r.text(root, "updatedAt", top);
    m.kind = r.closed(root, "kind", top, kKinds, ManifestKind::reference);
    m.material = r.closed(root, "material", top, kMaterials, MaterialType::audio);

    const auto& id = root["identity"];
    const juce::String ip = "identity.";
    m.identity.canonicalPath = r.text(id, "canonicalPath", ip);
    m.identity.sizeBytes = static_cast<std::uint64_t>(r.number(id, "sizeBytes", ip));
    m.identity.modifiedAt = r.text(id, "modifiedAt", ip);
    m.identity.format = r.text(id, "format", ip);
    m.identity.durationSeconds = r.number(id, "durationSeconds", ip);
    m.identity.sampleRate = static_cast<std::uint32_t>(r.number(id, "sampleRate", ip));
    m.identity.channels = static_cast<std::uint16_t>(r.number(id, "channels", ip));
    m.identity.contentSha256 = r.text(id, "contentSha256", ip);

    const auto& origin = root["origin"];
    const juce::String op = "origin.";
    m.origin.category = r.closed(origin, "category", op, kOrigins, OriginCategory::thirdPartyUnverified);
    r.each(origin, "authors", op, [&](const juce::var& a, const juce::String& p) {
        m.origin.authors.push_back({ r.text(a, "name", p), r.text(a, "role", p) });
    });
    m.origin.toolName = r.text(origin, "toolName", op);
    m.origin.toolVersion = r.text(origin, "toolVersion", op);
    m.origin.createdAt = r.text(origin, "createdAt", op);
    m.origin.notes = r.text(origin, "notes", op);

    const auto& auth = root["authorization"];
    const juce::String ap = "authorization.";
    m.authorization.status = r.closed(auth, "status", ap, kStatus, AuthorizationStatus::pending);
    m.authorization.basis = r.closed(auth, "basis", ap, kBases, AuthorizationBasis::none);
    r.each(auth, "scope", ap, [&](const juce::var& u, const juce::String& p) {
        auto holder = object();
        set(holder, "use", u);
        m.authorization.scope = m.authorization.scope
                              | r.closed(holder, "use", p, kUses, Use::listen);
    });
    m.authorization.license = r.text(auth, "license", ap);
    m.authorization.evidence = r.text(auth, "evidence", ap);
    m.authorization.declaredBy = r.text(auth, "declaredBy", ap);
    m.authorization.declaredAt = r.text(auth, "declaredAt", ap);
    m.authorization.restrictions = r.text(auth, "restrictions", ap);

    if (auto* meta = root["metadataImported"].getDynamicObject())
        for (const auto& prop : meta->getProperties())
            m.metadataImported.emplace_back(std8(prop.name.toString()), std8(prop.value.toString()));

    m.promptOriginalProvided = r.boolean(root, "promptOriginalProvided", top);
    m.promptsRef = r.text(root, "promptsRef", top);
    m.analysis = r.closed(root, "analysis", top, kAnalysis, AnalysisState::none);

    r.each(root, "relations", top, [&](const juce::var& o, const juce::String& p) {
        m.relations.push_back({ r.text(o, "type", p, true), r.text(o, "target", p, true) });
    });

    const auto& der = root["derivation"];
    if (!der.isVoid())
    {
        const juce::String dp = "derivation.";
        Derivation d;
        r.each(der, "from", dp, [&](const juce::var& f, const juce::String& p) {
            if (f.isString())
                d.from.push_back(std8(f.toString()));
            else
                r.error(p.dropLastCharacters(1), juce::String::fromUTF8("esperava texto"));
        });
        d.sessionId = r.text(der, "sessionId", dp);
        d.decisionsRef = r.text(der, "decisionsRef", dp);
        const auto seed = r.text(der, "seed", dp);
        if (!seed.empty())
        {
            if (seed.find_first_not_of("0123456789") != std::string::npos || seed.size() > 20)
                r.error("derivation.seed", juce::String::fromUTF8("esperava inteiro sem sinal em texto"));
            else
                d.seed = std::stoull(seed);
        }
        d.strategy = r.closed(der, "strategy", dp, kStrategies, Strategy::follow);
        d.declaredRelation = r.text(der, "declaredRelation", dp);
        d.trioioVersion = r.text(der, "trioioVersion", dp);
        m.derivation = std::move(d);
    }

    r.each(root, "history", top, [&](const juce::var& o, const juce::String& p) {
        m.history.push_back({ r.text(o, "at", p, true), r.text(o, "event", p, true),
                              r.text(o, "by", p), r.text(o, "note", p) });
    });

    if (out.errors.empty())
        out.manifest = std::move(m);
    return out;
}

} // namespace trioio::library
