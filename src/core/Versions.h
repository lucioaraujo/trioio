#pragma once

// A comparação entre versões (TRIOIO_TAREFAS, Marco 2: "invariantes,
// divergências e operações possíveis"; docs/VERSOES.md): várias gerações do
// mesmo prompt, várias gravações da mesma peça, as tomadas de uma sessão. O
// que não muda entre elas é o que a peça "é"; o que muda é o espaço onde o
// Trioio pode jogar. Lê o som (a escuta de cada uma), nunca um parâmetro
// interno de um gerador.

#include "Intent.h"
#include "Reading.h"

#include <string>
#include <vector>

namespace trioio::composition
{

struct VersionReading
{
    std::string id;
    double duration = 0.0;
    double bpm = 0.0;
    float pulseConfidence = 0.0f;
    int tonic = -1;
    bool minor = false;
    std::vector<int> rootsByPosition;  // a fundamental do acorde em 64 pontos da peça (−1 = sem acorde)
    std::vector<float> energyCurve;    // a energia em 32 pontos da peça, 0..1 (a forma)
    int sections = 0;
    IntentFields intent;               // a escuta no vocabulário da intenção
};

// lê uma versão a partir da escuta (os quadros, os ataques e a leitura)
[[nodiscard]] VersionReading readVersion(const std::string& id, const std::vector<perception::Frame>& frames,
                                         const std::vector<perception::Onset>& onsets, double duration, const Reading& reading);

struct VersionComparison
{
    std::vector<std::string> invariants;   // o que todas têm em comum
    std::vector<std::string> divergences;  // o que muda, versão a versão
    std::vector<std::string> operations;   // o que o Trioio pode fazer com isso (os comandos)
};

// compara a primeira com cada uma das outras (a primeira é a referência)
[[nodiscard]] VersionComparison compareVersions(const std::vector<VersionReading>& versions);

} // namespace trioio::composition
