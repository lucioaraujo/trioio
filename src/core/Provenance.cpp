#include "Provenance.h"

#include <algorithm>

namespace trioio::provenance
{
namespace
{
void need(std::vector<Violation>& out, bool ok, const char* field, const char* message)
{
    if (!ok)
        out.push_back({ field, message });
}

bool authorizedAndUsable(const Manifest& m)
{
    return m.authorization.status == AuthorizationStatus::authorized
        && m.analysis != AnalysisState::unavailable && validate(m).empty();
}
}

UseSet scopeImplications(UseSet scope) noexcept
{
    if (has(scope, Use::publishDerivative))
        scope = scope | Use::derive;
    if (has(scope, Use::learn) || has(scope, Use::derive))
        scope = scope | Use::analyze;
    if (has(scope, Use::analyze))
        scope = scope | Use::listen;
    return scope;
}

std::vector<Violation> validate(const Manifest& m)
{
    std::vector<Violation> v;
    const auto& a = m.authorization;
    const auto& o = m.origin;

    need(v, m.schemaVersion == kSchemaVersion, "schemaVersion", "versão de esquema desconhecida");
    need(v, !m.id.empty(), "id", "manifesto sem id");
    need(v, !m.createdAt.empty(), "createdAt", "sem data de criação");

    if (m.kind == ManifestKind::reference)
    {
        need(v, !m.identity.canonicalPath.empty(), "identity.canonicalPath",
             "referência sem caminho de origem");
        need(v, m.identity.sizeBytes > 0, "identity.sizeBytes", "referência sem tamanho");
    }

    // Origem
    if (o.category == OriginCategory::aiGenerated || o.category == OriginCategory::otherInstrument)
        need(v, !o.toolName.empty(), "origin.toolName",
             "material gerado por ferramenta ou instrumento precisa nomear a ferramenta");

    // Autorização
    need(v, scopeImplications(a.scope) == a.scope, "authorization.scope",
         "escopo incoerente: learn/derive exigem analyze, analyze exige listen, "
         "publishDerivative exige derive");

    if (a.status == AuthorizationStatus::authorized)
    {
        need(v, a.basis != AuthorizationBasis::none, "authorization.basis",
             "autorizado sem fundamento declarado");
        need(v, a.scope != 0, "authorization.scope", "autorizado sem nenhum uso");
        need(v, !a.declaredBy.empty(), "authorization.declaredBy", "autorização sem quem declarou");
        need(v, !a.declaredAt.empty(), "authorization.declaredAt", "autorização sem data");
        need(v, o.category != OriginCategory::thirdPartyUnverified, "origin.category",
             "material de origem não verificada não pode ser autorizado");
        if (a.basis == AuthorizationBasis::license)
            need(v, !a.license.empty(), "authorization.license", "fundamento 'licença' sem a licença");
        if (a.basis == AuthorizationBasis::license || a.basis == AuthorizationBasis::writtenPermission
            || a.basis == AuthorizationBasis::serviceTerms)
            need(v, !a.evidence.empty(), "authorization.evidence",
                 "licença, permissão ou termos de serviço precisam de evidência registrada");
        if (a.basis == AuthorizationBasis::ownWork)
            need(v, o.category == OriginCategory::authorial
                        || o.category == OriginCategory::performerCapture
                        || o.category == OriginCategory::otherInstrument,
                 "authorization.basis", "'obra própria' não combina com esta origem");
        if (o.category == OriginCategory::aiGenerated)
            need(v, a.basis == AuthorizationBasis::serviceTerms
                        || a.basis == AuthorizationBasis::license
                        || a.basis == AuthorizationBasis::writtenPermission,
                 "authorization.basis",
                 "material gerado por IA se apoia nos termos do serviço (ou licença/permissão), "
                 "não em 'obra própria'");
    }

    // Prompt
    if (m.promptOriginalProvided)
        need(v, !m.promptsRef.empty(), "promptsRef", "prompt declarado sem vínculo com prompts.json");

    // Derivação
    if (m.kind == ManifestKind::derivation)
    {
        need(v, m.derivation.has_value(), "derivation", "manifesto de derivação sem o bloco derivation");
        if (m.derivation)
        {
            need(v, !m.derivation->from.empty(), "derivation.from", "derivação sem referência de origem");
            need(v, !m.derivation->sessionId.empty(), "derivation.sessionId", "derivação sem sessão");
            need(v, !m.derivation->declaredRelation.empty(), "derivation.declaredRelation",
                 "derivação sem a relação declarada com a referência");
        }
    }
    else
    {
        need(v, !m.derivation.has_value(), "derivation", "referência não carrega bloco derivation");
    }

    need(v, !m.history.empty(), "history", "histórico vazio: o registro é o primeiro evento");
    return v;
}

bool permits(const Manifest& m, Use use)
{
    // Triagem: o pendente pode ser ouvido — é assim que o performer decide se
    // autoriza. Nada além disso (docs/BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md).
    if (m.authorization.status == AuthorizationStatus::pending)
        return use == Use::listen && m.analysis != AnalysisState::unavailable
            && validate(m).empty();
    if (!authorizedAndUsable(m))
        return false;
    // Arquivo alterado desde a análise: pode ouvir, mas nada que dependa do
    // mapa antigo até reanalisar.
    if (m.analysis == AnalysisState::changed && use != Use::listen && use != Use::analyze)
        return false;
    return has(m.authorization.scope, use);
}

bool canPublishDerivation(const Manifest& d, const std::vector<Manifest>& sources)
{
    if (d.kind != ManifestKind::derivation || !d.derivation || !validate(d).empty())
        return false;
    for (const auto& id : d.derivation->from)
    {
        const auto it = std::find_if(sources.begin(), sources.end(),
                                     [&](const Manifest& s) { return s.id == id; });
        if (it == sources.end() || !permits(*it, Use::publishDerivative))
            return false;
    }
    return true;
}

} // namespace trioio::provenance
