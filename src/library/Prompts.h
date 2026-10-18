#pragma once

// O prompts.json de cada referência (docs/PROMPTS_E_INTENCOES.md;
// esquema trioio.prompts/1, em <dados>/indice/prompts/<ref>.json): as três
// camadas da intenção (core/Intent.h), separadas e nunca substituídas.
//
//   promptOriginal  — só quando foi fornecido, com a fonte ("autor",
//                     "gerador", "metadados") e a data; guardado como veio;
//   declaredIntent  — a tradução (as palavras de cada campo à vista e as
//                     não lidas); `revised` quando o autor corrigiu;
//   analysisInferred — a escuta do áudio, no mesmo vocabulário (regravada a
//                     cada resposta que usa o prompt).
// O manifesto da referência ganha promptOriginalProvided e promptsRef.

#include "Intent.h"

#include <string>
#include <vector>

namespace trioio::library
{

struct PromptRecord
{
    std::string referenceId;
    bool hasOriginal = false;
    std::string original, source, providedAt;
    composition::DeclaredIntent declared;
    bool hasInferred = false;
    composition::IntentFields inferred;
    std::vector<std::string> history;  // "2026-10-18T…: fornecido (autor)", "…: tradução revisada (energy=0.3)"
};

[[nodiscard]] std::string promptsPath(const std::string& dataRoot, const std::string& referenceId);
[[nodiscard]] bool loadPrompt(const std::string& dataRoot, const std::string& referenceId, PromptRecord& out);
bool savePrompt(const std::string& dataRoot, const PromptRecord& r, std::vector<std::string>& problems);

// fornece (ou troca) o prompt original: traduz e grava; marca o manifesto
bool providePrompt(const std::string& dataRoot, const std::string& referenceId, const std::string& text, const std::string& source,
                   const std::string& now, std::vector<std::string>& problems);
// revisa a tradução: "energia=0.3", "humor=serena", "estilo=jazz",
// "harmonia=modal", "andamento=90", "voz=sim" (vários, separados por vírgula)
bool reviseIntent(const std::string& dataRoot, const std::string& referenceId, const std::string& edits, const std::string& now,
                  std::vector<std::string>& problems);
// apaga o prompt da referência (o performer pode remover; o manifesto volta a sem prompt)
bool removePrompt(const std::string& dataRoot, const std::string& referenceId, std::vector<std::string>& problems);

} // namespace trioio::library
