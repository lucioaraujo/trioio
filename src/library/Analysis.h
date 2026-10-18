#pragma once

// Análise de um material registrado: lê o áudio em blocos, mede os
// descritores (perception::Analyzer) e guarda o resultado em cache ligado ao
// conteúdo. Especificação: docs/DESCRITORES.md.
//
// O portão vem antes de tudo: só analisa o que o manifesto libera para
// `analyze` (provenance::permits). Trabalho de disco, fora da thread de áudio.

#include "Descriptors.h"

#include <atomic>
#include <string>
#include <vector>

namespace trioio::library
{

struct AnalysisLimits
{
    double maxSeconds = 3600.0;  // acima disto, recusa
    int blockSamples = 65536;    // leitura em blocos: memória limitada
};

struct AnalysisResult
{
    bool ok = false;
    bool fromCache = false;
    std::vector<std::string> problems;
    double sampleRate = 0.0;
    double duration = 0.0;
    std::vector<perception::Frame> frames;
    std::vector<perception::Onset> onsets;
    float percussiveShare = -1.0f;  // HPSS: fração percussiva da magnitude (−1 = sem separação)
    bool harmonicChroma = false;    // o croma é só o que se sustenta (peça com percussão)
};

// Analisa (ou relê do cache) o material `id`. Atualiza o manifesto para
// `cached` na primeira análise; o histórico ganha o evento "analisado".
[[nodiscard]] AnalysisResult analyzeReference(const std::string& dataRoot, const std::string& id,
                                              const AnalysisLimits& limits = {},
                                              const std::atomic<bool>* cancel = nullptr,
                                              const std::string& now = {});

// O áudio da referência em mono (média dos canais), com o mesmo portão
// (`analyze`) e a mesma conferência de SHA-256 da análise. Para testes de
// transformação (docs/LEITURA_E_GABARITO.md); recusa acima de `maxSeconds`.
[[nodiscard]] bool readReferenceMono(const std::string& dataRoot, const std::string& id, std::vector<float>& mono,
                                     double& sampleRate, std::vector<std::string>& problems, double maxSeconds = 900.0);

// Relatório offline em texto: resumo, linha do tempo e onde a música muda.
// `title` costuma ser o id e o caminho do material.
[[nodiscard]] std::string formatReport(const std::string& title, const AnalysisResult& analysis);

// Onde fica o cache de análise de um conteúdo.
[[nodiscard]] std::string analysisCachePath(const std::string& dataRoot, const std::string& contentSha256);

} // namespace trioio::library
