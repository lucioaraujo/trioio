#include "Provenance.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace trioio::provenance;

// Checagem explícita: `assert` some em build Release (NDEBUG) e o teste
// passaria sem verificar nada.
static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static bool temViolacao(const Manifest& m, const std::string& campo)
{
    for (const auto& v : validate(m))
        if (v.field == campo)
            return true;
    return false;
}

// Uma gravação própria, autorizada para tudo.
static Manifest autoral()
{
    Manifest m;
    m.id = "ref-0001";
    m.createdAt = "2026-10-05T10:00:00Z";
    m.updatedAt = m.createdAt;
    m.identity = { "/home/performer/TRIOIO_LIBRARY/referencias/autorais/improviso.wav",
                   10584044, "2026-09-30T21:14:00Z", "wav", 60.0, 44100, 2, "" };
    m.origin.category = OriginCategory::authorial;
    m.origin.authors = { { "Performer", "autor e intérprete" } };
    m.authorization.status = AuthorizationStatus::authorized;
    m.authorization.basis = AuthorizationBasis::ownWork;
    m.authorization.scope = Use::listen | Use::analyze | Use::learn | Use::derive | Use::publishDerivative;
    m.authorization.declaredBy = "Performer";
    m.authorization.declaredAt = m.createdAt;
    m.analysis = AnalysisState::cached;
    m.history = { { m.createdAt, "registrado", "Performer", "" } };
    return m;
}

static Manifest derivacaoDe(const std::string& origem)
{
    Manifest d;
    d.id = "der-0001";
    d.createdAt = "2026-10-05T11:00:00Z";
    d.kind = ManifestKind::derivation;
    d.origin.category = OriginCategory::otherInstrument;
    d.origin.toolName = "TRIOIO";
    d.origin.toolVersion = "0.0.0";
    d.derivation = Derivation { { origem }, "sessao-0001", "decisoes.json#3", 42,
                                Strategy::transfigure,
                                "mantém a curva de energia, troca toda a matéria sonora", "0.0.0" };
    d.history = { { d.createdAt, "gerado", "TRIOIO", "" } };
    return d;
}

int main()
{
    // 1. O caso de referência é válido e libera todos os usos.
    const auto ok = autoral();
    CHECK(validate(ok).empty());
    CHECK(permits(ok, Use::listen));
    CHECK(permits(ok, Use::derive));
    CHECK(permits(ok, Use::publishDerivative));

    // 2. O escopo é uma cadeia: derive sem analyze é incoerente.
    {
        auto m = autoral();
        m.authorization.scope = Use::listen | Use::derive;
        CHECK(temViolacao(m, "authorization.scope"));
        CHECK(!permits(m, Use::listen));  // inválido não entra em uso nenhum
        CHECK(scopeImplications(static_cast<UseSet>(Use::publishDerivative))
              == (Use::listen | Use::analyze | Use::derive | Use::publishDerivative));
    }

    // 3. Pendente: só ouvir, para decidir (triagem). Recusado: nada.
    {
        auto m = autoral();
        m.authorization.status = AuthorizationStatus::pending;
        CHECK(validate(m).empty());
        CHECK(permits(m, Use::listen));
        CHECK(!permits(m, Use::analyze));
        CHECK(!permits(m, Use::derive));
        m.authorization.status = AuthorizationStatus::refused;
        CHECK(!permits(m, Use::listen));
    }

    // 4. Origem não verificada nunca é autorizável; em triagem, só ouvir.
    {
        auto m = autoral();
        m.origin.category = OriginCategory::thirdPartyUnverified;
        CHECK(temViolacao(m, "origin.category"));
        CHECK(!permits(m, Use::listen));
        m.authorization = Authorization {};  // volta a pendente, sem fundamento
        CHECK(validate(m).empty());
        CHECK(permits(m, Use::listen));
        CHECK(!permits(m, Use::analyze));
    }

    // 5. Gerado por IA: nomear a ferramenta, apoiar-se nos termos do
    //    serviço com evidência — nunca em "obra própria".
    {
        auto m = autoral();
        m.origin.category = OriginCategory::aiGenerated;
        CHECK(temViolacao(m, "origin.toolName"));
        CHECK(temViolacao(m, "authorization.basis"));
        m.origin.toolName = "serviço de geração X";
        m.authorization.basis = AuthorizationBasis::serviceTerms;
        CHECK(temViolacao(m, "authorization.evidence"));
        m.authorization.evidence = "termos do serviço em 2026-09-30, cópia em docs-locais/termos-x.pdf";
        CHECK(validate(m).empty());
        CHECK(permits(m, Use::analyze));
    }

    // 6. Licença: precisa dizer qual e onde está a prova.
    {
        auto m = autoral();
        m.origin.category = OriginCategory::licensed;
        m.authorization.basis = AuthorizationBasis::license;
        CHECK(temViolacao(m, "authorization.license"));
        CHECK(temViolacao(m, "authorization.evidence"));
        m.authorization.license = "CC-BY-4.0";
        m.authorization.evidence = "https://exemplo.org/faixa";
        CHECK(validate(m).empty());
    }

    // 7. Escopo parcial: autorizado a ouvir e analisar, não a derivar.
    {
        auto m = autoral();
        m.authorization.scope = Use::listen | Use::analyze;
        CHECK(validate(m).empty());
        CHECK(permits(m, Use::analyze));
        CHECK(!permits(m, Use::learn));
        CHECK(!permits(m, Use::derive));
    }

    // 8. Revogado: o histórico fica, o uso para.
    {
        auto m = autoral();
        m.authorization.status = AuthorizationStatus::revoked;
        m.history.push_back({ "2026-10-06T09:00:00Z", "revogado", "Performer", "" });
        CHECK(!permits(m, Use::listen));
        CHECK(m.history.size() == 2);
    }

    // 9. Arquivo alterado: ouvir e reanalisar sim; derivar do mapa velho não.
    //    Ausente: nada.
    {
        auto m = autoral();
        m.analysis = AnalysisState::changed;
        CHECK(permits(m, Use::listen));
        CHECK(permits(m, Use::analyze));
        CHECK(!permits(m, Use::derive));
        CHECK(!permits(m, Use::learn));
        m.analysis = AnalysisState::unavailable;
        CHECK(!permits(m, Use::listen));
    }

    // 10. Prompt declarado precisa apontar para prompts.json.
    {
        auto m = autoral();
        m.promptOriginalProvided = true;
        CHECK(temViolacao(m, "promptsRef"));
        m.promptsRef = "prompts.json#ref-0001";
        CHECK(validate(m).empty());
    }

    // 11. Derivação: válida, e publicável só se TODAS as fontes permitem.
    {
        const auto d = derivacaoDe("ref-0001");
        CHECK(validate(d).empty());
        CHECK(canPublishDerivation(d, { autoral() }));

        auto semPublicar = autoral();
        semPublicar.authorization.scope = Use::listen | Use::analyze | Use::derive;
        CHECK(!canPublishDerivation(d, { semPublicar }));
        CHECK(!canPublishDerivation(d, {}));  // fonte desconhecida

        auto duas = derivacaoDe("ref-0001");
        duas.derivation->from.push_back("ref-0002");
        auto outra = autoral();
        outra.id = "ref-0002";
        outra.authorization.status = AuthorizationStatus::revoked;
        CHECK(!canPublishDerivation(duas, { autoral(), outra }));

        auto semRelacao = derivacaoDe("ref-0001");
        semRelacao.derivation->declaredRelation.clear();
        CHECK(temViolacao(semRelacao, "derivation.declaredRelation"));
        CHECK(!canPublishDerivation(semRelacao, { autoral() }));
    }

    // 12. Campos de base.
    {
        auto m = autoral();
        m.schemaVersion = "trioio.manifesto/0";
        m.history.clear();
        CHECK(temViolacao(m, "schemaVersion"));
        CHECK(temViolacao(m, "history"));
        auto r = autoral();
        r.derivation = Derivation {};
        CHECK(temViolacao(r, "derivation"));
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("provenance: todas as checagens passaram");
    return EXIT_SUCCESS;
}
