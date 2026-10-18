#include "ManifestJson.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

using namespace trioio::provenance;
using namespace trioio::library;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static bool erroEm(const ManifestReadResult& r, const std::string& campo)
{
    for (const auto& e : r.errors)
        if (e.rfind(campo, 0) == 0)
            return true;
    return false;
}

static Manifest completo()
{
    Manifest m;
    m.id = "der-ção-01";
    m.createdAt = "2026-10-05T11:00:00Z";
    m.updatedAt = m.createdAt;
    m.kind = ManifestKind::derivation;
    m.material = MaterialType::stem;
    m.identity = { "/home/performer/TRIOIO_LIBRARY/sessões/s1/saídas/órbita ☉.wav",
                   123456789, "2026-10-05T11:00:00Z", "wav", 61.25, 48000, 2, "ab12" };
    m.origin.category = OriginCategory::otherInstrument;
    m.origin.authors = { { "Lúcio de Araújo", "performer" }, { "TRIOIO", "síntese" } };
    m.origin.toolName = "TRIOIO";
    m.origin.toolVersion = "0.0.0";
    m.authorization.status = AuthorizationStatus::authorized;
    m.authorization.basis = AuthorizationBasis::ownWork;
    m.authorization.scope = Use::listen | Use::analyze | Use::derive;
    m.authorization.declaredBy = "Lúcio";
    m.authorization.declaredAt = m.createdAt;
    m.authorization.restrictions = "só ao vivo — \"ensaio\"";
    m.metadataImported = { { "title", "Órbita" }, { "comment", "linha 1\nlinha 2" } };
    m.promptOriginalProvided = true;
    m.promptsRef = "prompts.json#p1";
    m.analysis = AnalysisState::changed;
    m.relations = { { "derived_from", "ref-1" } };
    // seed no limite de 64 bits: não pode passar por double
    m.derivation = Derivation { { "ref-1", "ref-2" }, "s1", "decisoes.json#4",
                                18446744073709551615ull, Strategy::counter,
                                "contrapõe a energia; mantém os ataques", "0.0.0" };
    m.history = { { m.createdAt, "gerado", "TRIOIO", "" },
                  { "2026-10-05T12:00:00Z", "arquivo alterado", "", "reamostrado" } };
    return m;
}

static std::string exemploDaEspecificacao()
{
    std::ifstream f(TRIOIO_SOURCE_DIR "/docs/MANIFESTO_DE_PROVENIENCIA.md");
    std::stringstream ss;
    ss << f.rdbuf();
    const auto doc = ss.str();
    const auto ini = doc.find("```json\n");
    if (ini == std::string::npos)
        return {};
    const auto corpo = ini + 8;
    return doc.substr(corpo, doc.find("```", corpo) - corpo);
}

int main()
{
    // 1. Ida e volta preserva tudo, inclusive UTF-8 e seed de 64 bits.
    {
        const auto m = completo();
        const auto texto = toJson(m);
        const auto lido = fromJson(texto);
        CHECK(lido.errors.empty());
        CHECK(lido.manifest.has_value());
        if (lido.manifest)
        {
            const auto& r = *lido.manifest;
            CHECK(toJson(r) == texto);
            CHECK(r.identity.canonicalPath == m.identity.canonicalPath);
            CHECK(r.origin.authors[0].name == "Lúcio de Araújo");
            CHECK(r.authorization.scope == m.authorization.scope);
            CHECK(r.authorization.restrictions == m.authorization.restrictions);
            CHECK(r.metadataImported == m.metadataImported);
            CHECK(r.derivation && r.derivation->seed == 18446744073709551615ull);
            CHECK(r.derivation && r.derivation->strategy == Strategy::counter);
            CHECK(r.analysis == AnalysisState::changed);
            CHECK(r.history.size() == 2);
            CHECK(validate(r).empty());
        }
        CHECK(texto.find("\"seed\": \"18446744073709551615\"") != std::string::npos);
    }

    // 2. O exemplo publicado na especificação é lido e é válido — a
    //    documentação não pode divergir do formato.
    {
        const auto texto = exemploDaEspecificacao();
        CHECK(!texto.empty());
        const auto lido = fromJson(texto);
        for (const auto& e : lido.errors)
            std::fprintf(stderr, "  exemplo: %s\n", e.c_str());
        CHECK(lido.errors.empty());
        if (lido.manifest)
        {
            CHECK(validate(*lido.manifest).empty());
            CHECK(lido.manifest->origin.category == OriginCategory::aiGenerated);
            CHECK(permits(*lido.manifest, Use::derive));
            CHECK(!permits(*lido.manifest, Use::publishDerivative));
        }
    }

    // 3. Valor desconhecido em campo fechado é erro, nunca autorização.
    {
        auto texto = toJson(completo());
        const auto pos = texto.find("\"authorized\"");
        texto.replace(pos, 12, "\"autorizado\"");
        const auto lido = fromJson(texto);
        CHECK(!lido.manifest);
        CHECK(erroEm(lido, "authorization.status"));
    }
    {
        auto texto = toJson(completo());
        const auto pos = texto.find("\"derive\"");
        texto.replace(pos, 8, "\"tudo\"");
        const auto lido = fromJson(texto);
        CHECK(!lido.manifest);
        CHECK(erroEm(lido, "authorization.scope[2]"));
    }

    // 4. Outra versão de esquema não é lida como se fosse esta.
    {
        auto texto = toJson(completo());
        texto.replace(texto.find("trioio.manifesto/1"), 18, "trioio.manifesto/2");
        const auto lido = fromJson(texto);
        CHECK(!lido.manifest);
        CHECK(erroEm(lido, "schemaVersion"));
    }

    // 5. JSON quebrado, raiz errada, tipos errados, seed numérico.
    {
        CHECK(!fromJson("{ \"schemaVersion\": ").manifest);
        CHECK(!fromJson("[1,2,3]").manifest);
        auto texto = toJson(completo());
        texto.replace(texto.find("\"18446744073709551615\""), 22, "42");
        CHECK(erroEm(fromJson(texto), "derivation.seed"));
        auto t2 = toJson(completo());
        t2.replace(t2.find("\"promptOriginalProvided\": true"), 30, "\"promptOriginalProvided\": \"sim\"");
        CHECK(erroEm(fromJson(t2), "promptOriginalProvided"));
    }

    // 6. Um pendente incompleto pode ser gravado e lido (triagem): ler não é
    //    validar.
    {
        Manifest p;
        p.id = "ref-pendente";
        p.createdAt = "2026-10-05T09:00:00Z";
        const auto lido = fromJson(toJson(p));
        CHECK(lido.errors.empty());
        CHECK(lido.manifest && !validate(*lido.manifest).empty());
        CHECK(lido.manifest && lido.manifest->authorization.status == AuthorizationStatus::pending);
    }

    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("manifest json: todas as checagens passaram");
    return EXIT_SUCCESS;
}
