#pragma once

// O que a interface mostra, lido da raiz de dados que a CLI já grava
// (manifestos, cache de análise, decisoes.json, escuta.json). A interface não
// guarda estado musical próprio (docs/design.md §7).

#include "Harmony.h"

#include <cstdint>
#include <string>
#include <vector>

namespace trioio::app
{

struct ReferenceItem
{
    std::string id;
    std::string name;    // o nome do arquivo
    std::string path;    // o caminho do arquivo (para ouvir a referência)
    std::string status;  // autorizado, pendente, recusado, revogado, indisponível
    std::string origin;  // autoral, IA, outro instrumento…
    double duration = 0.0;
    bool canDerive = false;
};

struct ResponseItem
{
    std::string derivationId, sessionId, mode, wavPath, decisionsPath;
    std::uint64_t seed = 0;
};

struct ReferenceView
{
    bool ok = false;
    std::string problem;
    double duration = 0.0;
    std::vector<float> energy;  // 0..1 a cada `energyStep` segundos
    double energyStep = 0.25;
    std::vector<composition::ChordSpan> chords;
    double bpm = 0.0;
    int beatsPerBar = 4;
    std::string key, language;  // "Sol menor", "maqam Hijaz sobre Lá"
};

struct SectionView
{
    double start = 0.0, end = 0.0;
    std::string label;  // "A · apresentação · maqam Hijaz sobre Lá"
};

struct EyeView
{
    double time = 0.0, duration = 0.0;
    std::string what;   // "objeto sonoro: cluster por 7.1 s"
    std::string stays;  // a consequência, se ficou ("" = não)
};

struct NoteView
{
    double time = 0.0, duration = 0.0;
    float pitch = 60.0f;
    int voice = 0;
};

struct ResponseView
{
    bool ok = false;
    std::string problem;
    double duration = 0.0;
    std::vector<SectionView> sections;
    std::vector<EyeView> eye;
    std::vector<NoteView> notes;
    std::vector<std::string> header;  // as marcas do começo (pulso, escala, sayr…)
    std::vector<std::string> gameRules;  // as regras do jogo reveladas (decisoes.json → "game"; docs/JOGO.md)
};

class AppModel
{
public:
    void setDataRoot(const std::string& root);
    [[nodiscard]] const std::string& dataRoot() const noexcept { return root_; }
    [[nodiscard]] bool hasDataRoot() const noexcept { return !root_.empty(); }

    [[nodiscard]] std::vector<ReferenceItem> references() const;
    // as respostas de uma referência, a mais nova primeiro
    [[nodiscard]] std::vector<ResponseItem> responsesFor(const std::string& referenceId) const;

    // pode demorar na primeira vez (análise): chamar fora da thread da interface
    [[nodiscard]] ReferenceView loadReference(const std::string& referenceId) const;
    [[nodiscard]] ResponseView loadResponse(const ResponseItem& response) const;

private:
    std::string root_;
};

// Uma referência e uma resposta de demonstração, sintéticas, para os
// instantâneos de layout (docs/design.md) quando não há raiz de dados.
[[nodiscard]] ReferenceView demoReference();
[[nodiscard]] ResponseView demoResponse();

} // namespace trioio::app
