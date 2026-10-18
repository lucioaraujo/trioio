#pragma once

// A cadeia mínima audível, offline: referência autorizada → escuta →
// estratégia → síntese → WAV + decisões + manifesto da derivação → escuta da
// própria resposta. Especificação: docs/CADEIA_MINIMA.md.
//
// Exige `derive` no manifesto da referência. Nada é escrito na Biblioteca:
// tudo vai para a raiz de dados.

#include "Composer.h"
#include "Master.h"
#include "Strategy.h"

#include <functional>
#include <string>
#include <vector>

namespace trioio::library
{

struct ResponseRequest
{
    std::string id;  // referência
    strategy::Mode mode = strategy::Mode::follow;
    std::uint64_t seed = 1;
    double windowSeconds = 0.5;
    // motor: partitura (compositor: motivo, frases, forma; padrão) ou
    // energia (a primeira cadeia: controles contínuos, docs/CADEIA_MINIMA.md)
    bool score = true;
    composition::PitchSource pitch = composition::PitchSource::reference;
    composition::TimeMode time = composition::TimeMode::automatic;
    float unpredictability = 0.5f;  // o Terceiro Olho: 0 = sem intervenções
    bool useListening = true;       // aplicar a escuta do autor (escuta.json)
    // regras do autor (docs/REGRAS.md): vazio = <dados>/regras.txt, se existir
    std::string rulesPath;
    bool useRules = true;
    bool rhythm = true;             // camada rítmica do campo (docs/RITMO.md)
    bool mosaic = true;             // memória pesquisável por timbre (docs/MOSAICO.md)
    bool harmony = true;            // harmonia em movimento (docs/HARMONIA.md)
    // arquivo de escalas (docs/ESCALAS.md): o termo de busca ou um arquivo .scl;
    // o arquivo do Trioio mais as escalas do autor em <dados>/escalas/
    std::string scaleQuery;
    bool scaleMachine = false;      // a máquina de escalas (docs/ESCALAS.md)
    composition::Intention intention = composition::Intention::none;  // --intencao
    // a mistura (Recombination.h): termos do arquivo separados por "+"
    std::string mix;
    std::string cycle;              // --ciclo: um ciclo rítmico para a peça inteira (docs/RITMO.md)
    // mashup de materiais (core/Mashup.h): a outra referência (com a mesma
    // autorização de derivação) e as camadas que vêm dela
    std::string other;
    unsigned otherLayers = 1u | 2u | 4u | 8u;
    int voicing = 0;                // --voicing (core/Voicing.h)
    int ensemble = 0;               // --conjunto (core/Ensemble.h): 0 automático, −1 lido na referência
    bool filmFx = false;            // --efeitos sim (core/FilmEffects.h)
    float rasgo = 0.0f;             // --rasgo 0..1 (core/ModularRasgo.h); a CLI e o app usam 0,5 por padrão
    bool humanize = false;          // --humano nao desliga (core/Production.h); a CLI e o app ligam
    bool writeHarmony = false;      // --harmonia-escrita nao desliga (core/HarmonyWriting.h); a CLI e o app ligam
    int harmonyLanguage = 0;        // --linguagem-harmonica (HarmonyLanguage): 0 automática
    bool somDaAura = false;         // --som-da-aura sim (core/SomDaAura.h)
    int seam = 0;                   // --costura (core/Recombination.h)
    double tempoScale = 1.0;        // --andamento metade|dobro
    bool formByRepetition = false;  // --forma repeticao
    composition::MixTechnique mixTechnique = composition::MixTechnique::collage;
    // as linhas por região de frequência (docs/LINHAS.md, etapa 2): vazio =
    // uma resposta só, como sempre. Com cortes, cada linha da referência é
    // escutada e respondida por um compositor próprio; o regente as junta
    std::vector<double> lineCuts;
    std::vector<strategy::Mode> lineModes;  // um por linha; vazio ou curto = `mode`
    bool freeLines = false;                 // cada linha no seu pulso (quando ele vale ou concorda)
    bool lineFilter = true;                 // a resposta de cada linha soa só na sua banda
    // o barramento de master (core/Master.h): cola, alvo de loudness, pico
    // verdadeiro; falso = só a normalização de pico de antes
    bool master = true;
    bool dialogue = false;  // --dialogo sim (core/Dialogue.h)
    bool game = false;      // --jogo sim (core/Game.h, docs/JOGO.md): o Trioio inventa regras e as joga
    bool gameSound = true;
    bool stems = false;
    bool usePrompt = true;
    int rockStyle = 0;      // --rock: o subestilo do rock (core/Ensemble.h, RockStyle)
    // --influencias "rock/pesado, brasil/frevo" (ou "auto": a árvore escolhe):
    // a árvore de influências (core/Influences.h); vazio = desligada
    std::vector<std::string> influences;
    float influenceStrength = 1.0f;          // --forca-influencias 0..1
    std::vector<std::string> exclude;        // --sem "percussão, baixo": os papéis que o autor tira
    // --palavras "dançante, maracatu, virada": o prompt da resposta (opcional);
    // vazio = o Trioio segue os próprios critérios (Intent.h, Influences.h)
    std::string promptText;
    // --semantica sim: as palavras que o léxico não leu são consultadas nos
    // serviços de data/semantica/fontes.txt (SemanticBridge.h), dentro de
    // semanticBudgetMs; desligada por padrão (as palavras saem da máquina)
    bool semantic = false;
    // o andamento (opcional): a fração 0..1 e a etapa, para a barra do app
    // (chamado da thread da resposta; quem recebe passa para a sua thread)
    std::function<void(float, const std::string&)> progress;
    int semanticBudgetMs = 4000;
    std::string narrative;                   // --narrativa freytag|kishotenketsu|ondas|plato|arco (ou duas: "ondas+plato")
    std::vector<std::pair<float, float>> gesture;  // --gesto "0:0.2,0.6:1,1:0.3": a curva de intensidade do autor  // --prompt nao: ignora o prompts.json da referência (Prompts.h)     // --stems sim: um WAV por voz (e os efeitos e a sala) em saidas/stems/, além da mistura  // --jogo-no-som nao: os critérios do jogo só na partitura (sem os renders de escuta)
};

struct ResponseResult
{
    bool ok = false;
    std::vector<std::string> problems;
    std::string wavPath;
    std::string decisionsPath;
    std::string midiPath;                  // só no motor partitura
    std::vector<std::string> formMarks;    // a forma, em palavras
    std::string derivationId;
    int judgementsUsed = 0;                // julgamentos do escuta.json que pesaram
    std::vector<std::string> rulesUsed;    // regras ativas nesta resposta
    std::string rulesFile;                 // de onde vieram
    // quanto a resposta acompanha (+1) ou contraria (−1) a referência
    float energyCorrelation = 0.0f;
    float densityCorrelation = 0.0f;
    float brightnessCorrelation = 0.0f;
    std::vector<std::string> stemPaths;    // um WAV por linha (com linhas) ou por voz (com --stems sim)
    perception::MasterReport masterReport; // o que o master fez (zeros se desligado)
};

[[nodiscard]] ResponseResult respond(const std::string& dataRoot, const ResponseRequest& request,
                                     const std::string& now = {});

// Onde ficam os manifestos das saídas do TRIOIO dentro da raiz de dados.
[[nodiscard]] std::string derivationManifestDir(const std::string& dataRoot);

} // namespace trioio::library
