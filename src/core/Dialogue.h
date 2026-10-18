#pragma once

// O diálogo com a referência (docs/AUDITORIA_2026-10-14.md §6, item 4, a
// metade que não muda a proveniência; docs/COMPOSICAO.md "Diálogo"): a
// resposta deixa de correr em paralelo e passa a conversar — escuta enquanto
// a referência toca uma frase e responde nas pausas dela.
//
// A resposta continua 100 % sintética: só o TEMPO dela segue as frases da
// referência. Ouvir as duas juntas ("▶ juntos" no app) não grava nada; gravar
// ou publicar uma mistura com o áudio da referência é decisão do autor.
//
// Fonte: o responsório e a pergunta e resposta como forma (call and
// response; S. A. Floyd Jr., The Power of Black Music, Oxford UP, 1995; a
// alternância antifonal na liturgia e no repente nordestino).

#include "Composer.h"
#include "Structure.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace trioio::composition
{

struct DialogueMap
{
    std::vector<std::pair<double, double>> calls;    // a referência fala (a resposta escuta)
    std::vector<std::pair<double, double>> answers;  // as pausas dela (a resposta fala)
    bool alternating = false;  // música contínua: alternância de compassos em vez de pausas
};

// As frases e as pausas da referência: pausa = energia abaixo do meio entre os
// percentis 20 e 80, e ≥ 12 dB abaixo do nível forte (percentil 80), por
// ≥ 0,5 s. Se as pausas somam menos de 12 % da peça
// (uma canção, um drone), alterna a cada 2 compassos do pulso (ou 4 s).
[[nodiscard]] DialogueMap dialogueMap(const std::vector<perception::Frame>& frames, double duration,
                                      const perception::Pulse& pulse);

// Nos pedidos, as vozes que conduzem (motivo 0, eco 2, memória 3, mosaico 5)
// calam e o acompanhamento (baixo 1, percussão 4, harmonia 6) fica a 60 %;
// nas respostas, as que conduzem ganham 10 %. Nota que começa na resposta e
// entra no pedido é cortada no começo dele. Devolve a linha da forma.
std::string applyDialogue(Score& score, const DialogueMap& map);

// A resposta retoma o gesto do pedido (o repente, o *trading* do jazz): em
// cada resposta, a voz do motivo toca o FIM da frase que a referência acabou
// de tocar — as alturas dos ataques (pitchAt) e o ritmo original, só a cauda
// que cabe na resposta —, ajustado à escala que a resposta usa ali (as
// classes de altura das notas dela em ±8 s) e ao registro dela. Por modo:
// seguir imita; transfigurar inverte o contorno; contrapor toca o retrógrado
// da inversão; interromper cita só a primeira metade do gesto e corta.
// As notas do motivo que estavam na resposta dão lugar ao gesto. Respostas
// sem ≥ 3 ataques com altura no pedido ficam como estavam.
std::string imitateCalls(Score& score, const DialogueMap& map, const std::vector<perception::Frame>& frames,
                         const std::vector<perception::Onset>& onsets, strategy::Mode mode);

// A troca de lugar (docs/JOGO.md §1.1, princípio 4: "emissor e receptor se
// tornam vitoriamario"): num trecho do meio da peça (do começo de um pedido
// ao fim de uma resposta, de 2 a 3 pares), o jogo mexe no próprio diálogo.
//   kind 1, a TROCA: os pedidos viram respostas e as respostas viram pedidos
//     — a resposta fala enquanto a referência fala e escuta as pausas dela;
//   kind 2, SEM VEZ (o "atropelo"): os pedidos do trecho somem — as duas
//     falam ao mesmo tempo.
// Por modo: interromper atropela; os outros trocam (seguir atropela em 40 %).
struct DialogueSwap
{
    double from = 0.0, to = 0.0;
    int kind = 0;  // 0 nenhuma
};
[[nodiscard]] DialogueSwap chooseDialogueSwap(const DialogueMap& map, double duration, strategy::Mode mode, std::uint64_t seed);
// O mapa com o trecho trocado (ou sem vez).
[[nodiscard]] DialogueMap swapDialogue(const DialogueMap& map, const DialogueSwap& swap);
[[nodiscard]] std::string dialogueSwapText(const DialogueSwap& swap);

} // namespace trioio::composition
