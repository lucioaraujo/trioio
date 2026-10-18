#pragma once

// O jogo (docs/JOGO.md, §2–§4 e §7): o Trioio inventa as próprias regras e as
// joga sobre a resposta composta. Uma regra tem a forma comum (GameRule em
// Composer.h): quando <condição lida na referência>, faça <ação> sobre
// <material>, por <alcance>.
//
// Os caminhos de invenção desta primeira versão (§4):
//   1. a regra LIDA na referência: o hábito mais frequente dela vira condição;
//   2. a regra VARIADA: outra condição, ação, alcance ou material;
//   6. o ADUBO: os restos de regras que apodreceram (rejeitadas) se recombinam;
//   7. CONTRA SI MESMO ("confunde-te a ti mesmo"): a ação contrária à de uma
//      regra que ficou, ou à da própria regra, na outra metade da peça.
// E a regra que ficou numa resposta aceita pode VOLTAR.
//
// Os critérios do "musicalmente interessante" (§4), nesta versão medidos na
// PARTITURA (não ainda no som): audível (muda ≥ 4 notas), coerente (muda ≤ 35 %
// das notas e cobre ≤ 40 % da peça), nova (não repete uma regra já em jogo).
// A medida no som (compor com e sem a regra e escutar) é o passo seguinte.

#include "Composer.h"
#include "Listening.h"

#include <string>
#include <vector>

namespace trioio::composition
{

[[nodiscard]] const char* gameActionName(GameAction a);
[[nodiscard]] GameAction oppositeAction(GameAction a);
[[nodiscard]] std::string gameRuleText(const GameRule& r);

// Quando a condição vale na referência: os começos das janelas de 1 s.
[[nodiscard]] std::vector<double> conditionMoments(const std::vector<perception::Frame>& frames,
                                                   const std::vector<perception::Onset>& onsets, double duration, int condition);

// Aplica uma regra à partitura (no lugar); preenche fired e changed. O custo:
// no máximo 3 disparos por peça, espalhados entre os momentos em que a
// condição vale.
void applyGameRule(Score& score, GameRule& rule, const std::vector<double>& moments);

// Inventa, mede e joga as regras desta resposta. Devolve as linhas da forma.
std::vector<std::string> playGame(Score& score, const std::vector<perception::Frame>& frames,
                                  const std::vector<perception::Onset>& onsets, double duration, const Options& opt);

// O estado do jogo (§5) ao longo da peça: começa em `start`; cada disparo
// gasta e ganha recursos conforme a ação (interromper gasta coerência e
// surpresa e aproxima o colapso; sustentar devolve coerência e protege o
// equilíbrio; a regra oculta acumula informação oculta); o tempo devolve
// surpresa e energia e afasta o colapso. Anota cada disparo e os
// esgotamentos ("sem surpresa disponível", "perto do colapso").
[[nodiscard]] std::vector<GameStateMoment> runGameState(const std::vector<GameRule>& rules, double duration, const GameState& start);

// O estado inicial: a surpresa pela imprevisibilidade, a memória pelas regras
// que ficaram, a liberdade de desvio pelo saldo de aceitas e rejeitadas.
[[nodiscard]] GameState initialGameState(const Options& opt);

[[nodiscard]] std::string gameStateText(const GameState& s);

// Os critérios no SOM (§4: "ele compõe com a regra, escuta o resultado e
// compara com a peça sem ela"): as janelas dos disparos (1 s antes, 1 s
// depois) são sintetizadas sem a regra e com ela NUM SÓ render (o pico é
// normalizado uma vez, então os níveis se comparam) e medidas em quadros de
// 50 ms: nível RMS, brilho (a energia da derivada sobre a energia) e ataques
// (subidas de ≥ 6 dB).
//   audível: |ΔdB| médio ≥ 1, ou brilho ≥ 0,15, ou ≥ 2 ataques de diferença;
//   coerente: o pico com a regra não sobe mais de 6 dB, e o silêncio mais
//     longo não passa de 1,5 s além do que já havia;
// e preenche soundDb/soundBright/soundOnsets da regra.
struct SoundVerdict
{
    bool audible = false, coherent = true;
    std::string why;
};
SoundVerdict soundCheck(const Score& without, const Score& with, GameRule& rule, double sampleRate = 22050.0);

// nova no som: a mudança desta regra (nível, brilho, ataques) não repete a
// de uma regra já em jogo (cosseno > 0,97 e tamanhos a ±30 %)
[[nodiscard]] bool soundsLike(const GameRule& a, const GameRule& b);

// O caminho 4 (§4): a regra que nasce da nota livre do autor. Lê, em
// português, uma ação ("imitar", "calar", "cortar", "inverter", "mais
// notas", "respirar", "sustentar", "citar"…), uma condição depois de
// "quando"/"sempre que"/"se" ("cresce", "forte", "rarefeito", "parado"…) e o
// material ("o baixo", "a percussão", "tudo"). Sem ação reconhecida, não há
// regra (a nota "achei estranho o jazz" não vira lei).
[[nodiscard]] bool ruleFromNote(const std::string& note, GameRule& out);

// O veredito de uma regra (docs/JOGO.md §3: fica, volta, apodrece), pelos
// julgamentos da resposta em que ela jogou, na ordem em que foram dados:
// cada disparo [t, t + span) é julgado pelo último julgamento por trecho que
// o cobre; a maioria dos disparos decide (no empate, o último voto). Sem
// trecho que cubra um disparo, vale o último julgamento da resposta inteira.
// Devolve o Verdict como int, ou −1 se a regra não foi julgada.
[[nodiscard]] int judgeGameRule(const GameRule& rule, const std::vector<Judgement>& judgements);

} // namespace trioio::composition
