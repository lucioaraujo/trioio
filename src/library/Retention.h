#pragma once

// A retenção das sessões (TRIOIO_TAREFAS, Marco 2: "política de retenção e
// limpeza ..., sempre reversível pelo performer"; docs/ESTRUTURA_DO_PROJETO.md):
//
// - uma sessão JULGADA (alguma derivação dela tem julgamento no escuta.json)
//   nunca é tocada: é fonte da escuta, da memória do jogo e do gabarito;
// - as N sessões mais recentes ficam (o autor ainda pode escutá-las);
// - as outras, não julgadas, são testes descartáveis: `limpar` as MOVE para
//   dados/lixeira/AAAA-MM-DD/ (reversível: mover de volta para sessoes/);
// - `esvaziar` apaga da lixeira só o que está lá há mais de D dias.
// Os manifestos de derivação ficam no índice (o registro do que foi feito);
// a interface esconde as versões cuja sessão não existe mais.

#include <string>
#include <vector>

namespace trioio::library
{

struct SessionStatus
{
    std::string id;         // sessao-AAAAMMDDhhmmss-…
    bool judged = false;
    bool recent = false;    // entre as `keepRecent` mais recentes
    double megabytes = 0.0;
};

// Todas as sessões, da mais nova para a mais velha.
[[nodiscard]] std::vector<SessionStatus> sessionStatus(const std::string& dataRoot, int keepRecent = 3);

// Move as sessões descartáveis (não julgadas e não recentes) para a lixeira
// do dia (`today` = AAAA-MM-DD). Devolve as movidas.
std::vector<std::string> moveDisposableToTrash(const std::string& dataRoot, const std::string& today, int keepRecent = 3);

// Apaga da lixeira as pastas de dia com mais de `days` dias antes de `today`.
// Devolve as pastas apagadas.
std::vector<std::string> emptyTrash(const std::string& dataRoot, const std::string& today, int days);

// O cache de análise (cache/analise/<sha256>.json) é reconstruível: um
// cache cujo conteúdo nenhuma referência do índice tem mais é órfão.
struct CacheStatus
{
    std::string file;
    bool orphan = false;
    double megabytes = 0.0;
};
[[nodiscard]] std::vector<CacheStatus> cacheStatus(const std::string& dataRoot);
// apaga os órfãos (ou, com `all`, todo o cache: a próxima análise o refaz)
std::vector<std::string> removeCaches(const std::string& dataRoot, bool all);

// As ações de uma resposta pelo app (2026-10-19, o autor: "faça as
// implementações necessárias" — ver, exportar e descartar uma resposta sem a
// linha de comando). `sessionId` é o nome da pasta em sessoes/
// (sessao-…); nada fora dela é tocado.

// Uma sessão escolhida para a lixeira: dados/lixeira/<today>/<id>,
// reversível (mover de volta para sessoes/). O julgamento, se houver,
// continua em escuta.json, e o manifesto da derivação, no índice.
bool moveSessionToTrash(const std::string& dataRoot, const std::string& sessionId, const std::string& today, std::string& error);

// As saídas de uma sessão copiadas para fora da pasta de dados:
// <destination>/<id>/ com a mistura, o MIDI, stems/ e linhas/ (se houver) e
// decisoes.json. Se a pasta já existe, <id>-2, <id>-3… Nada é sobrescrito.
bool exportSession(const std::string& dataRoot, const std::string& sessionId, const std::string& destination,
                   std::string& createdDir, std::string& error);

} // namespace trioio::library
