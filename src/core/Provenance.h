#pragma once

// Manifesto de proveniência do TRIOIO, versão 1 — o modelo em memória e as
// regras que decidem o que uma referência pode fazer dentro do instrumento.
// Especificação: docs/MANIFESTO_DE_PROVENIENCIA.md.
//
// Este arquivo não lê nem grava JSON: a serialização chega com a Biblioteca
// (Marco 2). Aqui fica o contrato que qualquer leitor ou importador terá de
// respeitar, testável sem dependências. Nada disto roda na thread de áudio.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace trioio::provenance
{

inline constexpr const char* kSchemaVersion = "trioio.manifesto/1";

enum class ManifestKind { reference, derivation };

enum class MaterialType { audio, stem, midi };

// De onde o material veio. `thirdPartyUnverified` é a triagem: algo que
// chegou sem origem conhecida e por isso não pode ser autorizado ainda.
enum class OriginCategory
{
    authorial,             // obra do próprio performer
    performerCapture,      // capturado pelo performer (gravação ao vivo, microfone)
    otherInstrument,       // saída de outro instrumento, inclusive do RASGO
    aiGenerated,           // gerado por ferramenta de IA a pedido do performer
    licensed,              // de terceiros, sob licença
    publicDomain,
    thirdPartyUnverified,
};

enum class AuthorizationStatus { pending, authorized, refused, revoked };

// Em que se apoia a autorização declarada.
enum class AuthorizationBasis
{
    none,
    ownWork,            // o performer é o autor
    license,            // licença identificada (campo `license`)
    publicDomain,
    writtenPermission,  // permissão por escrito do titular
    serviceTerms,       // termos do serviço que gerou o material (IA)
};

// Usos, do mais leve ao mais comprometedor. Cada um exige os anteriores
// da sua cadeia (ver `scopeImplications`).
enum class Use : std::uint8_t
{
    listen = 1u << 0,             // tocar e ver a forma de onda
    analyze = 1u << 1,            // descritores, mapa de relações, cache
    learn = 1u << 2,              // entrar na memória consolidada / tendências
    derive = 1u << 3,             // servir de referência para síntese derivada
    publishDerivative = 1u << 4,  // publicar uma saída derivada dela
};

using UseSet = std::uint8_t;
[[nodiscard]] constexpr UseSet operator|(Use a, Use b) noexcept
{
    return static_cast<UseSet>(static_cast<UseSet>(a) | static_cast<UseSet>(b));
}
[[nodiscard]] constexpr UseSet operator|(UseSet a, Use b) noexcept
{
    return static_cast<UseSet>(a | static_cast<UseSet>(b));
}
[[nodiscard]] constexpr bool has(UseSet set, Use u) noexcept
{
    return (set & static_cast<UseSet>(u)) != 0;
}

// Estado técnico do arquivo e da análise — separado da autorização: um
// arquivo pode estar autorizado e ausente, ou presente e sem autorização.
enum class AnalysisState { none, indexing, analyzing, cached, changed, unavailable };

enum class Strategy { follow, transfigure, counter, interrupt };

struct Person
{
    std::string name;
    std::string role;  // "autor", "intérprete", "produtor", ...
};

struct TechnicalIdentity
{
    std::string canonicalPath;  // onde estava ao ser registrado; nunca movido pelo TRIOIO
    std::uint64_t sizeBytes = 0;
    std::string modifiedAt;     // ISO 8601
    std::string format;         // "wav", "flac", "mid", ...
    double durationSeconds = 0.0;
    std::uint32_t sampleRate = 0;  // 0 em MIDI
    std::uint16_t channels = 0;    // 0 em MIDI
    std::string contentSha256;     // opcional; calculado fora da thread de áudio
};

struct Origin
{
    OriginCategory category = OriginCategory::thirdPartyUnverified;
    std::vector<Person> authors;
    std::string toolName;     // obrigatório em aiGenerated e otherInstrument
    std::string toolVersion;
    std::string createdAt;    // data da obra, se conhecida
    std::string notes;
};

struct Authorization
{
    AuthorizationStatus status = AuthorizationStatus::pending;
    AuthorizationBasis basis = AuthorizationBasis::none;
    UseSet scope = 0;
    std::string license;       // SPDX ou texto da licença, quando basis == license
    std::string evidence;      // caminho, URL ou descrição do documento que prova
    std::string declaredBy;
    std::string declaredAt;    // ISO 8601
    std::string restrictions;  // texto livre: "não comercial", "só ao vivo"...
};

struct Relation
{
    std::string type;    // version_of, stem_of, midi_of, same_prompt_as, derived_from
    std::string target;  // id de outro manifesto
};

struct Derivation
{
    std::vector<std::string> from;  // ids das referências usadas
    std::string sessionId;
    std::string decisionsRef;       // entrada em decisoes.json
    std::uint64_t seed = 0;
    Strategy strategy = Strategy::follow;
    std::string declaredRelation;   // o que a saída faz com a referência, em palavras
    std::string trioioVersion;
};

struct HistoryEvent
{
    std::string at;     // ISO 8601
    std::string event;  // "registrado", "autorizado", "revogado", "arquivo alterado", ...
    std::string by;
    std::string note;
};

struct Manifest
{
    std::string schemaVersion = kSchemaVersion;
    std::string id;
    std::string createdAt;
    std::string updatedAt;
    ManifestKind kind = ManifestKind::reference;
    MaterialType material = MaterialType::audio;
    TechnicalIdentity identity;
    Origin origin;
    Authorization authorization;
    // Metadados encontrados no arquivo, guardados como vieram (chave, valor).
    std::vector<std::pair<std::string, std::string>> metadataImported;
    // O texto dos prompts mora em prompts.json; aqui só o vínculo.
    bool promptOriginalProvided = false;
    std::string promptsRef;
    AnalysisState analysis = AnalysisState::none;
    std::vector<Relation> relations;
    std::optional<Derivation> derivation;  // só em kind == derivation
    std::vector<HistoryEvent> history;     // só cresce
};

struct Violation
{
    std::string field;
    std::string message;
};

// Regras de coerência do manifesto (docs/MANIFESTO_DE_PROVENIENCIA.md §5).
// Lista vazia = válido. Um manifesto inválido não entra em nenhum uso.
[[nodiscard]] std::vector<Violation> validate(const Manifest& manifest);

// Fecha o escopo pelas implicações: learn e derive exigem analyze, analyze
// exige listen, publishDerivative exige derive.
[[nodiscard]] UseSet scopeImplications(UseSet scope) noexcept;

// O portão: esta referência pode ser usada assim, agora? Pendente só ouve
// (triagem); recusado e revogado, nada.
[[nodiscard]] bool permits(const Manifest& manifest, Use use);

// Uma saída derivada pode ser publicada? Exige a própria derivação válida e
// `publishDerivative` em TODAS as referências de que ela partiu.
[[nodiscard]] bool canPublishDerivation(const Manifest& derivation,
                                        const std::vector<Manifest>& sources);

} // namespace trioio::provenance
