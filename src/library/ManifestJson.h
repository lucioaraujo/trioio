#pragma once

// Leitura e escrita de `manifesto.json` (esquema trioio.manifesto/1).
// Especificação: docs/MANIFESTO_DE_PROVENIENCIA.md §4.
//
// A interface não expõe o JUCE: entra e sai texto UTF-8 em std::string. A
// implementação usa o juce::JSON (juce_core, AGPLv3), por isso este arquivo
// mora na camada da Biblioteca e não no núcleo. Trabalho de disco e de
// texto: nunca na thread de áudio.

#include "Provenance.h"

#include <optional>
#include <string>
#include <vector>

namespace trioio::library
{

struct ManifestReadResult
{
    std::optional<provenance::Manifest> manifest;  // vazio se o texto não for lido
    std::vector<std::string> errors;               // problemas de leitura (JSON, tipos, valores)
};

// Texto JSON indentado, UTF-8. Não valida: grava o que recebe, para que um
// manifesto pendente ou incompleto possa ser salvo e corrigido depois.
[[nodiscard]] std::string toJson(const provenance::Manifest& manifest);

// Lê o texto. Valor desconhecido num campo fechado (categoria, estado, uso...)
// é erro, não silêncio: um manifesto mal lido não pode virar autorização.
// Com qualquer erro, `manifest` vem vazio. A coerência das regras é
// verificada à parte, por provenance::validate().
[[nodiscard]] ManifestReadResult fromJson(const std::string& utf8Text);

// Nomes dos valores fechados, os mesmos do JSON ("authorial", "ownWork",
// "listen"...). Devolvem false para nome desconhecido.
bool parseOrigin(const std::string& name, provenance::OriginCategory& out);
bool parseBasis(const std::string& name, provenance::AuthorizationBasis& out);
bool parseUse(const std::string& name, provenance::Use& out);

} // namespace trioio::library
