#pragma once

// Declaração do performer sobre um material registrado: origem, fundamento e
// usos liberados — ou recusa/revogação. É a única forma prevista de um
// manifesto sair de `pending`. Especificação: docs/MANIFESTO_DE_PROVENIENCIA.md.
//
// Nada é gravado se o resultado não passar em provenance::validate(): uma
// declaração incompleta volta com a lista do que falta, e o manifesto em
// disco fica como estava.

#include "Provenance.h"

#include <string>
#include <vector>

namespace trioio::library
{

struct AuthorizationRequest
{
    std::string id;                     // ref-...
    provenance::OriginCategory origin = provenance::OriginCategory::thirdPartyUnverified;
    provenance::AuthorizationBasis basis = provenance::AuthorizationBasis::none;
    provenance::UseSet scope = 0;       // fechado pelas implicações (derive traz analyze e listen)
    std::string toolName;               // IA / outro instrumento
    std::string toolVersion;
    std::string license;
    std::string evidence;
    std::string restrictions;
    std::string declaredBy;
    std::vector<provenance::Person> authors;  // opcional; substitui a lista se vier preenchida
};

struct DeclarationResult
{
    bool written = false;
    std::vector<std::string> problems;  // validate() e erros de leitura/gravação
};

// `now` em ISO 8601 (vazio = relógio atual).
[[nodiscard]] DeclarationResult authorize(const std::string& dataRoot, const AuthorizationRequest& request,
                                          const std::string& now = {});

// Recusar (pendente que não será usado) ou revogar (autorizado que deixa de
// valer daqui em diante). Recusado ou revogado pode voltar por authorize().
// O histórico guarda quem, quando e por quê; nada anterior é apagado.
[[nodiscard]] DeclarationResult refuse(const std::string& dataRoot, const std::string& id,
                                       const std::string& by, const std::string& reason,
                                       const std::string& now = {});
[[nodiscard]] DeclarationResult revoke(const std::string& dataRoot, const std::string& id,
                                       const std::string& by, const std::string& reason,
                                       const std::string& now = {});

} // namespace trioio::library
