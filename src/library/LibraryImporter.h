#pragma once

// Importador da Biblioteca: varre uma raiz escolhida pelo performer e mantém
// um manifesto (trioio.manifesto/1) por material encontrado.
// Regras: docs/IMPORTADOR_DA_BIBLIOTECA.md.
//
// - Só LÊ a Biblioteca. Manifestos vão para a raiz de dados
//   (<dados>/indice/referencias/<id>.json), que não pode ficar dentro dela.
// - Todo material novo entra PENDENTE, com origem não declarada: o
//   importador nunca presume autoria nem autorização.
// - A identidade é o conteúdo (SHA-256): mover ou renomear um arquivo não
//   perde a autorização; trocar o conteúdo num mesmo caminho não a herda.
// - Trabalho de disco, fora da thread de áudio, com limites e cancelamento.

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace trioio::library
{

struct ImportLimits
{
    std::uint64_t maxFileBytes = 4ull * 1024 * 1024 * 1024;  // acima disto, pula
    int maxFiles = 20000;                                     // por varredura
};

struct ScanReport
{
    int registered = 0;  // material novo, manifesto pendente criado
    int unchanged = 0;   // já conhecido, no mesmo lugar
    int relocated = 0;   // mesmo conteúdo em outro caminho (movido/renomeado)
    int duplicates = 0;  // cópias idênticas de um material já conhecido
    int reappeared = 0;  // estava ausente e voltou
    int missing = 0;     // conhecido, não encontrado nesta varredura
    int replaced = 0;    // o caminho agora guarda outro conteúdo
    std::vector<std::string> skipped;  // "caminho: motivo"
    std::vector<std::string> errors;   // problemas que impedem parte do trabalho
    bool cancelled = false;
};

struct ManagedCopyResult { bool ok = false; std::string path, error; };
[[nodiscard]] ManagedCopyResult copyReferenceToManagedFolder(
    const std::string& dataRoot, const std::string& referenceId,
    const std::string& sourcePath);

// `libraryRoot` pode ser um arquivo só: registra só ele (sem tocar a pasta
// vizinha nem dar ninguém por ausente).
// `now` em ISO 8601 (vazio = relógio atual). `cancel` é consultado entre
// arquivos; ao cancelar, nada do que ficou pela metade é gravado.
[[nodiscard]] ScanReport scanLibrary(const std::string& libraryRoot,
                                     const std::string& dataRoot,
                                     const ImportLimits& limits = {},
                                     const std::atomic<bool>* cancel = nullptr,
                                     const std::string& now = {});

// Pasta onde os manifestos de referência ficam dentro da raiz de dados.
[[nodiscard]] std::string referenceManifestDir(const std::string& dataRoot);

} // namespace trioio::library
