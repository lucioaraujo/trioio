#pragma once

// Compositor: escreve uma PARTITURA a partir da escuta da referência.
// Motivo tirado da referência (ritmo real de um compasso + alturas do campo
// harmônico), desenvolvido em frases (sentença) e organizado na forma das
// seções da referência, com recapitulação. Especificação: docs/COMPOSICAO.md.
//
// Puro e determinístico: mesma escuta + opções + seed = mesma partitura.

#include "ScaleAffect.h"
#include "Strategy.h"
#include "Structure.h"

#include <array>
#include <utility>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <optional>
#include <vector>

namespace trioio::composition
{
struct CadenceEntry;   // HarmonyWriting.h
struct InfluenceDraw;  // Influences.h

// Famílias de timbre próprias (ScoreSynth). Nenhuma é amostra: tudo é síntese.
enum class Timbre
{
    additive,  // 8 parciais harmônicos, agudos decaem antes
    breath,    // sopro: parciais suaves + ruído de ar no ataque e no corpo
    metal,     // sino: parciais inarmônicos (1; 2,76; 5,40; 8,93), decaimento longo
    plucked,   // corda pinçada (Karplus-Strong)
    membrane,  // pele: modos inarmônicos de membrana, altura que cai no ataque
    fm,        // FM de dois operadores com razão não inteira (rugoso)
    // 2026-10-07: o corpo sonoro tirado da própria referência
    spectral,  // os parciais MEDIDOS da referência (razões e pesos), ressintetizados
    // 2026-10-07: módulos do Rasgo Modular como corpos de voz (ModularVoice.h,
    // cópia fixada em third_party/rasgo_modular_dsp)
    wavetable,  // WAVETABLE: tabela de ondas com posição e dobra da tabela
    pulsar,     // PULSAR: síntese pulsar (Roads), formante e janela
    modal,      // MATTER: banco de 24 modos golpeado (objeto, sino, madeira)
    waveguide,  // STRING: guia de onda com posição de excitação e saturação no laço
    drum,       // DRUM: tambor sintético com queda de altura e estalo
    vocal,      // OSC dente de serra → FORMANT: vogais
    folded,     // OSC senoide → SHAPE: dobra de onda da costa oeste (Buchla 259)
    // 2026-10-07: vozes do Rasgo Synth (RasgoSynthVoice.h, cópia fixada em
    // third_party/rasgo_synth_dsp)
    gendyn,    // síntese estocástica dinâmica (Xenakis): a forma de onda vagueia
    vosim,     // VOSIM: trens de pulsos sin² com formante própria
    walsh,     // funções de Walsh: ondas em degraus, espectro "digital"
    theremin,  // teremim: senoide que desliza até a nota, com vibrato
    flute,     // flauta FM com o "chiff" do ataque
    scat,      // voz scat: pulso glotal em três formantes de vogal real
    chip,      // AY-3-8910: ondas quadradas de chip de 8 bits
    galo,      // Galo: corpo modal soprado com contorno de chamado
    // 2026-10-07, segunda leva
    resonator, // RESONATOR (Rasgo Modular) soprado pelo ruído rosa do Antitotem
    cmos,      // CmosVoice (Antitotem): 5 osciladores CMOS, PLL que caça a nota
    choir,     // ChoirEnsemble (Rasgo Synth): 4 vozes de vogal num acorde
    rungler,   // Rungler (Rasgo Synth): registrador caótico salta a altura em volta da nota
    // 2026-10-08, terceira leva
    operatorFm,     // OPERATOR (Rasgo Modular): FM de 4 operadores, 8 algoritmos
    lpg,            // OSC → LPG (Rasgo Modular): o golpe de vactrol da costa oeste
    granular,       // GranularCloud (Rasgo Synth): grãos sobre o tom da nota
    spectralObject, // SpectralObject (Rasgo Synth): parciais por crivo que pulsam na própria proporção
    biomaModal,     // BiomaModalResonator (Aquorbium): 4 modos, material e dureza
    biomaString,    // BiomaPhysicalString (Aquorbium): corda física
    striker,        // BiomaGranularStriker (Aquorbium): golpe de partículas ressonantes
    // 2026-10-14: corpos de instrumentos acústicos, modelos físicos próprios
    // (AcousticVoice.h; docs/MAQUINA_DE_TIMBRES.md)
    bowed,   // arco: corda friccionada (violino, viola, violoncelo pelo registro)
    reed,    // palheta: clarinete
    bars,    // marimba; nas notas longas, vibrafone
    piano,   // piano: três cordas rígidas, martelo de feltro
    guitar,  // violão: corda de náilon dedilhada e o tampo
    // segunda leva (2026-10-14)
    upright,          // baixo acústico: contrabaixo em pizzicato
    transverseFlute,  // flauta transversal: jato de ar na aresta
    surdo,            // surdo: pele grande, baqueta de feltro
    conga,            // conga: tom aberto; tapa no forte
    pandeiro,         // pandeiro: pele e platinelas
    shaker,           // ganzá: partículas num tubo (PhISEM)
    // receitas da corda dedilhada (2026-10-15)
    sevenString,  // violão de 7 cordas: a baixaria com dedeira
    requinto,     // requinto: o violão pequeno do bolero
    cavaquinho,   // cavaquinho: aço, caixa pequena, palheta
    bandolim,     // bandolim: ordens duplas de aço
    manouche,     // guitarra manouche: aço, a voz anasalada da caixa Selmer
    // terceira leva (2026-10-15): sopros e bateria
    sax,       // sax: palheta num tubo cônico (tenor a alto)
    trumpet,   // trompete: os lábios como válvula
    trombone,  // trombone: o mesmo, mais escuro e grave
    ride,      // prato de condução, com o sino
    hihat,     // chimbal: fechado (curto) ou aberto (nota longa)
    snare,     // caixa com esteira
    brushes,   // caixa com escovinha (o varrido)
    kick,      // bumbo
    bongo,     // bongô
    guiro,     // guiro: a vareta raspando
    // a voz (2026-10-15): fonte glotal e formantes
    singing,  // voz cantada: vogais, vibrato, do baixo à soprano
    whisper,  // sussurro: só o ar pelos formantes
    hum,      // boca chiusa: o murmúrio nasal
    // o kit completo e os elétricos (2026-10-15)
    tom,              // tom-tom
    crash,            // prato de ataque
    electricGuitar,   // guitarra elétrica: aço, captador, amplificador levemente saturado
    distortedGuitar,  // guitarra distorcida: o amplificador no talo
    electricBass,     // baixo elétrico
    // a bateria de escola de samba (2026-10-15)
    tamborim,  // tamborim: pele pequena, baqueta
    repique,   // repique: o tambor agudo que chama
    agogo,     // agogô: duas campânulas
    cuica,     // cuíca: tambor de fricção, o "uó"
    chocalho,  // chocalho de platinelas
    // órgão, palheta livre e o forró (2026-10-15)
    organ,      // órgão de rodas fônicas com a Leslie
    accordion,  // sanfona: palheta livre com musette
    bandoneon,  // bandoneón: palhetas em oitava, seco
    zabumba,    // zabumba: maceta e bacalhau
    triangle,   // triângulo: aberto e abafado
    // os naipes (2026-10-15): vários solistas na mesma nota
    stringSection,  // naipe de cordas: 8 arcos
    brassSection,   // naipe de metais: trompetes no agudo, trombones no grave
    voiceChoir,     // coro de vozes: 6 vozes
    // as quimeras (2026-10-15; ModularRasgo.h): o excitador de um no corpo de outro
    bowedPlate,  // prato friccionado: o arco num prato
    sungString,  // corda cantada: a glote numa corda
    blownSkin,   // pele soprada: o ar numa pele
};
inline constexpr int kTimbreCount = 83;

// A sala da peça inteira. Desde 2026-10-07 há três: a do Trioio e duas do
// Rasgo Modular (cópia fixada), escolhidas pelo caráter da referência.
enum class Room { trioio, hall, space };
[[nodiscard]] const char* roomName(Room r);

// O corpo espectral de um trecho da referência: as razões dos parciais em
// relação à fundamental medida, o peso de cada um e quanto do som é ruído.
struct SpectralProfile
{
    double start = 0.0, end = 0.0;
    std::vector<float> ratios;  // 1 = fundamental; não inteiras = inarmônico
    std::vector<float> weights; // soma 1
    float noise = 0.0f;         // planura média 0..1
};

struct Note
{
    double time = 0.0;      // segundos
    double duration = 0.0;
    float pitch = 60.0f;    // MIDI, com fração (microtons)
    float glide = 0.0f;     // semitons percorridos até o fim da nota (glissando)
    float velocity = 0.5f;  // 0..1
    int voice = 0;          // 0 = motivo, 1 = baixo, 2 = eco/contracanto, 3 = memória (oráculo), 4 = campo rítmico, 5 = mosaico, 6 = harmonia
    Timbre timbre = Timbre::additive;
    // de onde veio na referência (s), para as vozes de memória: a família
    // espectral toca com o corpo daquele instante. < 0 = do instante em que soa
    double source = -1.0;
    // a curva de altura da nota (ornamentos; ScaleIdiom.h): pontos
    // (fração da duração 0..1, semitons), interpolados linearmente, e uma
    // oscilação (profundidade em semitons, frequência em Hz) que entra
    // depois de `wobbleDelay` segundos. Andolan: lenta e larga; gamaka:
    // rápida; blue note: a curva chega de baixo.
    std::vector<std::pair<float, float>> bend {};
    float wobbleDepth = 0.0f, wobbleRate = 0.0f, wobbleDelay = 0.0f;
    // o rasgo modular (ModularRasgo.h): o patch que atravessa o corpo
    // (Patch; 0 = nenhum), a profundidade 0..1 e a razão (anel, simpática)
    std::uint8_t patch = 0;
    float patchDepth = 0.0f, patchRatio = 1.0f;

    // a altura (semitons MIDI, com fração) em `t` segundos desde o ataque:
    // nota + glissando + curva + oscilação
    [[nodiscard]] double pitchAt(double t) const
    {
        const double prog = std::min(1.0, t / std::max(duration, 1e-3));
        double p = pitch + glide * prog;
        if (!bend.empty())
        {
            if (prog <= bend.front().first)
                p += bend.front().second;
            else if (prog >= bend.back().first)
                p += bend.back().second;
            else
                for (std::size_t i = 1; i < bend.size(); ++i)
                    if (prog <= bend[i].first)
                    {
                        const double a = bend[i - 1].first, b = bend[i].first;
                        const double x = b > a ? (prog - a) / (b - a) : 1.0;
                        p += bend[i - 1].second + x * (bend[i].second - bend[i - 1].second);
                        break;
                    }
        }
        if (wobbleDepth > 0.0f && t > wobbleDelay)
            p += wobbleDepth * std::sin(6.283185307179586 * wobbleRate * (t - wobbleDelay));
        return p;
    }
};

// Objetos sonoros: gestos contínuos, fora da grade de notas. Os campos com
// "End" são o estado no fim do gesto; o meio é interpolado.
enum class GestureKind
{
    cloud,      // nuvem granular: grãos curtos numa faixa de alturas
    glissMass,  // massa de glissandos independentes
    noiseBand,  // faixa de ruído com centro e largura móveis
    cluster,    // alturas densas e próximas que batem entre si
    iteration,  // impulsos ressonantes que aceleram ou desaceleram
    drone,      // fundamental grave com brilho que evolui
    freeze,     // congelamento: o acorde do campo, sustentado e trêmulo
    body,       // corpo da referência: os parciais MEDIDOS da gravação naquele
                // instante, sustentados sobre uma fundamental, respirando (2026-10-07)
    // os efeitos de cinema (2026-10-15; FilmEffects.h): fora do sorteio do
    // Terceiro Olho, que só escolhe entre os oito acima
    wind, rain, thunder, water, whoosh, riser, impact, braam,
    fire, crowd, city,  // as ambiências da segunda leva
};

struct Gesture
{
    GestureKind kind = GestureKind::cloud;
    double time = 0.0;
    double duration = 1.0;
    float lo = 48.0f, hi = 72.0f;        // faixa de alturas (MIDI) no início
    float loEnd = 48.0f, hiEnd = 72.0f;  // e no fim
    float density = 10.0f, densityEnd = 10.0f;  // grãos/s, vozes ou impulsos/s
    float level = 0.5f;                  // 0..1
    float pan = 0.0f, panEnd = 0.0f;     // −1..1
    std::vector<float> pitches;          // freeze: as alturas do acorde; body: a fundamental
    std::vector<float> ratios, weights;  // body: o perfil espectral da referência em `time`
    std::uint64_t seed = 1;
};

struct Mark
{
    double time = 0.0;
    std::string text;  // "seção 2: desenvolvimento (inversão)", "tempo livre"...
};

// O jogo (docs/JOGO.md): uma regra na forma comum — quando <condição>, faça
// <ação> sobre <material>, por <alcance>; de onde veio; o que fez. As
// funções que inventam e aplicam as regras estão em Game.h.
enum class GameAction { imitate, transpose, delay, hide, interrupt, invert, densify, rarefy, sustain, selfQuote };
inline constexpr int kGameActions = 10;
struct GameRule
{
    int id = 0;
    int when = 0;            // Condition (0 = sempre)
    GameAction action = GameAction::imitate;
    int material = 0;        // 0 as vozes que conduzem (0, 2, 3, 5); 1 todas; 2 a percussão (4); 3 o baixo (1)
    double span = 2.0;       // segundos por disparo
    double from = 0.0, to = 1e9;  // o trecho da peça em que a regra vale
    bool hidden = true;      // revelada só depois da escuta
    std::string origin;      // "lida na referência", "variada da #1", "adubo", "contra si mesmo", "volta"
    std::string why;         // em palavras: por que a regra nasceu
    int fired = 0, changed = 0;  // o que ela fez nesta resposta
    std::vector<double> firedAt {};  // o começo de cada disparo (s): o julgamento por trecho pesa em cada regra
    // os critérios medidos no SOM (soundCheck): as janelas dos disparos
    // sintetizadas com e sem a regra; a diferença média de nível (dB, com
    // sinal), de brilho (razão log, com sinal) e de ataques
    bool soundMeasured = false;
    float soundDb = 0.0f, soundBright = 0.0f, soundOnsets = 0.0f, soundSpectral = 0.0f;
    // o jogo apócrifo (docs/JOGO.md §1.1): a regra revelada com uma origem
    // inventada; a verdade fica aqui (o arquivo não mente para si mesmo)
    bool apocryphal = false;
    std::string trueOrigin, trueWhy;
};

// o estado do jogo (docs/JOGO.md §5): os recursos do conceito, 0..1, num
// instante da peça, e o que o mudou
struct GameState
{
    double energy = 0.7, density = 0.5, memory = 0.0, coherence = 0.7, surprise = 0.6, hidden = 0.0, collapse = 0.1, freedom = 0.5;
};
struct GameStateMoment
{
    double time = 0.0;
    GameState state;
    std::string event;  // "regra #2: interromper", "sem surpresa disponível"…
};

struct Score
{
    double duration = 0.0;
    double bpm = 120.0;  // pulso usado (para a exportação MIDI)
    std::vector<Note> notes;
    std::vector<Gesture> gestures;
    std::vector<Mark> marks;
    // o corpo espectral da referência ao longo do tempo: janelas de 3 s a
    // cada 1,5 s (antes, uma média por seção). A família espectral e o
    // objeto "corpo" leem daqui o corpo do seu instante.
    std::vector<SpectralProfile> spectra;
    Room room = Room::trioio;
    int ensemble = 0;  // o conjunto que tocou (Ensemble.h): a sala da produção (Production.h)
    std::vector<GameRule> game;  // as regras do jogo que entraram nesta resposta (docs/JOGO.md)
    std::vector<GameRule> nearGame;  // a leitura apócrifa: as que quase entraram (descartadas pelos critérios)
    std::vector<GameStateMoment> gameState;  // o estado do jogo ao longo da peça (docs/JOGO.md §5)
};

// O corpo espectral da referência entre t0 e t1 (picos ÷ f0 nos quadros com
// altura definida; sem altura, picos ÷ o pico mais grave). Vazio se não
// houver material.
[[nodiscard]] SpectralProfile spectralProfile(const std::vector<perception::Frame>& frames, double t0, double t1);
[[nodiscard]] const char* timbreName(Timbre t);

// O perfil da janela cujo centro está mais perto de `t` (nullptr se não houver).
[[nodiscard]] const SpectralProfile* spectralAt(const Score& score, double t);

// referência: o campo medido; modal: o catálogo de 45 escalas (ScaleBank.h)
// que melhor veste o campo; espectral: os picos medidos; microtonal: graus
// desafinados ao acaso; inventada: crivo de Xenakis fora do catálogo; edo:
// crivo em 19/22/31-EDO; justa, pitagorica, quartos: o campo reafinado
// arquivo: o arquivo de escalas do Trioio (ScaleArchive.h)
enum class PitchSource { reference, modal, spectral, microtonal, invented, edo, just, pythagorean, quarterTone, archive };
enum class TimeMode { pulse, free, automatic };

inline constexpr int kEyeChoices = 17;  // escolhas do Terceiro Olho: 8 objetos + 9 regras
inline constexpr int kRooms = 3;

// Multiplicadores que a escuta do autor aplica aos sorteios (Listening.h):
// 1 = neutro, limitados a 0,5..2.
struct Tendencies
{
    std::array<float, kTimbreCount> timbre;
    std::array<float, kEyeChoices> eye;
    std::array<float, kRooms> room;
    int judgements = 0;  // quantos julgamentos as formaram
    Tendencies() { timbre.fill(1.0f); eye.fill(1.0f); room.fill(1.0f); }
};

// Condições das regras "condicionado" (lidas pela leitura do momento do
// Terceiro Olho). 0 = sem condição.
enum class Condition
{
    none = 0, energyHigh, energyLow, energyRising, energyFalling, noisy, dense, sparse,
    changeAhead, stasis, strongChord
};
inline constexpr int kConditions = 11;
[[nodiscard]] const char* conditionName(Condition c);

// Regras musicais declaradas pelo autor (Atlas §55, "restrições musicais":
// permitido, proibido, condicionado, obrigatório, exceção). Lidas de um
// texto simples (Constraints.cpp, docs/REGRAS.md). Vazio = sem regras: a
// composição é idêntica à de antes.
struct Constraints
{
    std::array<bool, kTimbreCount> noTimbre {};
    std::array<bool, kEyeChoices> noEye {};
    std::array<bool, kEyeChoices> mustEye {};
    std::array<bool, kRooms> noRoom {};
    std::array<Condition, kEyeChoices> when {};          // condicionado: só se a condição valer
    std::vector<std::pair<int, int>> incompatible;       // escolhas do olho que nunca soam juntas
    int maxLeap = 0;              // maior intervalo melódico do motivo (semitons); 0 = livre
    float lowest = 0.0f, highest = 0.0f;  // âmbito (MIDI); 0 e 0 = livre
    int minMemory = 0;            // mínimo de passagens da memória (oráculo)
    float exception = 0.0f;       // chance de o olho quebrar uma proibição (marcada)
    std::vector<std::string> lines;  // as regras ativas, como o autor as escreveu
    [[nodiscard]] bool empty() const noexcept { return lines.empty(); }
    [[nodiscard]] bool anyEyeForbidden() const noexcept
    {
        for (bool b : noEye) if (b) return true;
        return false;
    }
};

// Lê as regras de um texto (uma por linha, '#' começa comentário). Linhas
// que não entende vão para `errors`, com o número da linha.
[[nodiscard]] Constraints parseConstraints(const std::string& text, std::vector<std::string>& errors);

struct ArchiveScale;  // ScaleArchive.h
struct RhythmCycle;   // RhythmCycles.h
// as técnicas de recombinação (Recombination.h)
enum class MixTechnique { collage, hybrid, layers, transition };

struct Options
{
    strategy::Mode mode = strategy::Mode::follow;
    std::uint64_t seed = 1;
    PitchSource pitch = PitchSource::reference;
    TimeMode time = TimeMode::automatic;
    // 0 = a composição sem intervenções; 1 = o Terceiro Olho intervém muito,
    // com regras ocultas e objetos sonoros (ThirdEye.h)
    float unpredictability = 0.5f;
    // a escuta do autor (Listening.h); padrão = neutra, tudo 1
    Tendencies tendencies {};
    // as regras do autor (docs/REGRAS.md); padrão = nenhuma
    Constraints constraints {};
    // ritmo como campo (RhythmField.h, Atlas §45) nos trechos com pulso
    bool rhythm = true;
    // memória pesquisável por timbre (Mosaic.h, Atlas §62)
    bool mosaic = true;
    // harmonia em movimento (Harmony.h): acordes lidos, transformados pelo
    // modo; baixo na fundamental, motivo no acorde, voz de harmonia (6)
    bool harmony = true;
    // o arquivo de escalas (ScaleArchive.h), lido pela Biblioteca, e o termo
    // de busca ("pelog", "maqam", "Werckmeister"...; vazio = todas)
    std::shared_ptr<const std::vector<ArchiveScale>> scaleArchive {};
    std::string scaleQuery {};
    // a máquina de escalas (ScaleMachine.h): a escala se desvia de seção em
    // seção, e a harmonia passa a ser feita dos graus de cada escala
    bool scaleMachine = false;
    // a intenção do autor (ScaleAffect.h): guia a máquina de escalas por um
    // alvo afetivo; liga a máquina. 0 = nenhuma
    Intention intention = Intention::none;
    // a mistura de culturas e linguagens (Recombination.h): os termos do
    // arquivo de escalas ("rāga Yaman", "maqam Hijaz", "blues"…) e a técnica
    std::vector<std::string> mix {};
    MixTechnique mixTechnique = MixTechnique::collage;
    // os ciclos rítmicos das culturas (RhythmCycles.h): o arquivo, lido pela
    // Biblioteca, e um ciclo pedido para a peça inteira (vazio = o de cada
    // escala, pelo comportamento)
    std::shared_ptr<const std::vector<RhythmCycle>> rhythmArchive {};
    // o arquivo de cadências (HarmonyWriting.h), lido pela Biblioteca: a
    // máquina de harmonia sorteia nele parte das chegadas
    std::shared_ptr<const std::vector<CadenceEntry>> cadenceArchive {};
    std::string cycleQuery {};
    // mashup de materiais (Mashup.h): a outra referência, já no tempo desta
    // (remapTimeline), e as camadas que vêm dela (bits de MashupLayer)
    const std::vector<perception::Frame>* otherFrames = nullptr;
    const std::vector<perception::Onset>* otherOnsets = nullptr;
    unsigned otherLayers = 0;
    std::string otherName {};
    // o voicing da voz de harmonia e do vamp (Voicing.h): 0 = terças; os
    // outros valores seguem a ordem de Voicing (quartal, sowhat, planing,
    // bitonal, aberto, cluster). O comportamento da escala pode pedir o seu.
    int voicing = 0;
    // a costura entre seções (Recombination.h, Seam): 0 = corte
    int seam = 0;
    // o nível do pulso pela escuta do autor: 0,5 = metade do andamento lido
    // (o tempo é o dobro do período), 2 = o dobro; 1 = como lido
    double tempoScale = 1.0;
    // a forma por repetição (Structure.h, findSectionsByRepetition): as
    // seções saem do que volta (A B A…), e a seção que volta retoma o
    // material da primeira vez; falso = seções por contraste (novidade)
    bool formByRepetition = false;
    // o pulso imposto (as linhas por frequência, docs/LINHAS.md: o regente
    // dá o pulso da peça a todas as linhas); vazio = o pulso lido dos quadros
    std::optional<perception::Pulse> pulse {};
    // o groove da referência (Groove.h): swing, microtempo e acento medidos
    // nos ataques; seguir e interromper tocam com ele inteiro, transfigurar
    // com 60 %, contrapor fica na grade reta (contra o balanço)
    bool groove = true;  // e o rubato nas seções de tempo livre (Groove.h, applyRubato)
    // a dramaturgia (Dramaturgy.h): a curva de tensão da peça molda a
    // dinâmica, a densidade das vozes de enfeite e o silêncio antes do clímax
    bool dramaturgy = true;
    // o gesto do autor (o modo híbrido: áudio + prompt + gesto): uma curva de
    // intensidade para a peça, pontos (posição 0..1, intensidade 0..1); vazio
    // = a curva do modo (Dramaturgy.h). Com gesto, a dramaturgia, o rasgo
    // modular e os efeitos de cinema seguem a curva do autor.
    std::vector<std::pair<float, float>> gesture {};
    // a gramática narrativa (Dramaturgy.h): "freytag", "kishotenketsu", ou
    // duas misturadas ("ondas+plato"); vazio = a curva do modo. O gesto vence.
    std::string narrative {};
    // o subestilo do rock (Ensemble.h, RockStyle), quando o conjunto é o rock
    int rockStyle = 0;
    // as influências sorteadas para esta resposta (Influences.h): aplicadas à
    // partitura depois das figuras do estilo (as presenças, os eixos)
    std::shared_ptr<const InfluenceDraw> influenceDraw {};
    // o diálogo (Dialogue.h): a resposta escuta nas frases da referência e
    // responde nas pausas dela (ou alterna compassos); desligado por padrão
    bool dialogue = false;
    // a tensão da peça para o Terceiro Olho (Dramaturgy.h; preenchida pelo
    // compositor quando a dramaturgia está ligada): o olho intervém mais nos
    // trechos tensos, menos nos vales, e nunca na respiração antes do clímax
    std::function<float(double)> tensionAt {};
    std::pair<double, double> breath { -1.0, -1.0 };
    // o jogo (Game.h, docs/JOGO.md): o Trioio inventa regras e as joga.
    // `gameKept` = as regras que ficaram (respostas aceitas); `gameCompost` =
    // o adubo (as que apodreceram: rejeitadas, ou antigas)
    bool game = false;
    std::vector<GameRule> gameKept {}, gameCompost {};
    // as notas livres do autor nos julgamentos, a mais recente por último:
    // o caminho 4, a regra que nasce do autor (ruleFromNote)
    std::vector<std::string> gameNotes {};
    // o estado em que o jogo da resposta anterior terminou (a sessão longa):
    // a resposta nova parte dele, misturado ao começo próprio
    bool hasGamePrevious = false;
    // os critérios do jogo medidos no som (sintetiza as janelas dos disparos
    // com e sem cada regra candidata); desligado, só a partitura
    bool gameSoundCriteria = true;
    GameState gamePrevious {};
    // o conjunto por estilo (Ensemble.h): 0 = automático; −1 = lido na referência
    int ensemble = 0;
    // a harmonia escrita (HarmonyWriting.h): cadências, dominantes
    // secundárias, modulações preparadas; o app e a CLI ligam, o núcleo não
    bool writeHarmony = false;
    // a linguagem da máquina de harmonia (HarmonyLanguage): 0 = automática
    int harmonyLanguage = 0;
    // o som da aura (SomDaAura.h): a fala da referência vira a melodia harmonizada
    bool somDaAura = false;
    // os efeitos de cinema (FilmEffects.h)
    bool filmFx = false;
    // o rasgo modular (ModularRasgo.h): 0 = o instrumento, 1 = o inaudito
    float rasgo = 0.0f;
    // a humanização fina (Production.h): o app e a CLI ligam; o núcleo não
    bool humanize = false;
    // o final alongado da expressão (ritardando e fermata); o modo de linhas
    // desliga (as linhas somam em posição fixa, no comprimento da referência)
    bool expressiveEnding = true;
};

[[nodiscard]] Score compose(const std::vector<perception::Frame>& frames,
                            const std::vector<perception::Onset>& onsets, double duration,
                            const Options& options);

[[nodiscard]] bool parsePitchSource(const std::string& name, PitchSource& out);  // referencia modal|catalogo espectral microtonal inventada edo justa pitagorica quartos arquivo
[[nodiscard]] bool parseTimeMode(const std::string& name, TimeMode& out);        // pulso livre auto
[[nodiscard]] const char* pitchSourceName(PitchSource p);
[[nodiscard]] const char* timeModeName(TimeMode t);

} // namespace trioio::composition
