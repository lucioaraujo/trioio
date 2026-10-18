#include "Declaration.h"

#include "LibraryImporter.h"
#include "ManifestJson.h"

#include <juce_core/juce_core.h>

#include <optional>

namespace trioio::library
{
using namespace trioio::provenance;

namespace
{
juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.data(), static_cast<int>(s.size())); }

std::string stamp(const std::string& now)
{
    return now.empty() ? juce::Time::getCurrentTime().toISO8601(true).toStdString() : now;
}

juce::File fileFor(const std::string& dataRoot, const std::string& id)
{
    return juce::File(u8(referenceManifestDir(dataRoot))).getChildFile(u8(id) + ".json");
}

std::optional<Manifest> load(const juce::File& f, DeclarationResult& out)
{
    if (!f.existsAsFile())
    {
        out.problems.push_back("manifesto não encontrado: " + f.getFullPathName().toStdString());
        return std::nullopt;
    }
    auto read = fromJson(f.loadFileAsString().toStdString());
    for (const auto& e : read.errors)
        out.problems.push_back(e);
    return read.manifest;
}

// mesma gravação atômica do importador
void save(const juce::File& f, const Manifest& m, DeclarationResult& out)
{
    for (const auto& v : validate(m))
        out.problems.push_back(v.field + ": " + v.message);
    if (!out.problems.empty())
        return;
    juce::TemporaryFile tmp(f);
    if (tmp.getFile().replaceWithText(u8(toJson(m)), false, false, "\n") && tmp.overwriteTargetFileWithTemporary())
        out.written = true;
    else
        out.problems.push_back("falha ao gravar " + f.getFullPathName().toStdString());
}

DeclarationResult close(const std::string& dataRoot, const std::string& id, AuthorizationStatus status,
                        const char* event, const std::string& by, const std::string& reason,
                        const std::string& now)
{
    DeclarationResult out;
    const auto f = fileFor(dataRoot, id);
    auto m = load(f, out);
    if (!m)
        return out;
    if (by.empty())
    {
        out.problems.push_back("declaredBy: quem declara precisa ser nomeado");
        return out;
    }
    if (status == AuthorizationStatus::revoked && m->authorization.status != AuthorizationStatus::authorized)
    {
        out.problems.push_back("authorization.status: só se revoga o que está autorizado");
        return out;
    }
    if (status == AuthorizationStatus::refused && m->authorization.status != AuthorizationStatus::pending)
    {
        out.problems.push_back("authorization.status: só se recusa o que está pendente (autorizado se revoga)");
        return out;
    }
    const auto t = stamp(now);
    m->authorization.status = status;
    m->authorization.declaredBy = by;
    m->authorization.declaredAt = t;
    m->updatedAt = t;
    m->history.push_back({ t, event, by, reason });
    save(f, *m, out);
    return out;
}
} // namespace

DeclarationResult authorize(const std::string& dataRoot, const AuthorizationRequest& r, const std::string& now)
{
    DeclarationResult out;
    const auto f = fileFor(dataRoot, r.id);
    auto m = load(f, out);
    if (!m)
        return out;
    const auto t = stamp(now);
    // Mudar de ideia é do performer: recusado ou revogado pode voltar a ser
    // autorizado; o histórico guarda as duas decisões.
    const bool reauth = m->authorization.status != AuthorizationStatus::pending;

    m->origin.category = r.origin;
    if (!r.toolName.empty())
        m->origin.toolName = r.toolName;
    if (!r.toolVersion.empty())
        m->origin.toolVersion = r.toolVersion;
    if (!r.authors.empty())
        m->origin.authors = r.authors;
    m->authorization.status = AuthorizationStatus::authorized;
    m->authorization.basis = r.basis;
    m->authorization.scope = scopeImplications(r.scope);
    m->authorization.license = r.license;
    m->authorization.evidence = r.evidence;
    m->authorization.restrictions = r.restrictions;
    m->authorization.declaredBy = r.declaredBy;
    m->authorization.declaredAt = t;
    m->updatedAt = t;
    m->history.push_back({ t, reauth ? "autorização redeclarada" : "autorizado", r.declaredBy, r.restrictions });
    save(f, *m, out);
    return out;
}

DeclarationResult refuse(const std::string& dataRoot, const std::string& id, const std::string& by,
                         const std::string& reason, const std::string& now)
{
    return close(dataRoot, id, AuthorizationStatus::refused, "recusado", by, reason, now);
}

DeclarationResult revoke(const std::string& dataRoot, const std::string& id, const std::string& by,
                         const std::string& reason, const std::string& now)
{
    return close(dataRoot, id, AuthorizationStatus::revoked, "revogado", by, reason, now);
}

} // namespace trioio::library
