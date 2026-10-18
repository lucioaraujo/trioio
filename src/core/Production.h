#pragma once

// A produção (docs/MAQUINA_DE_TIMBRES.md §6): a mistura como um engenheiro a
// faria. A escuta do autor (2026-10-16): "está muito amador, precisamos de
// sonoridade mais sofisticada, amadurecida, profissional […] para que gere
// emoção". Antes, todas as vozes somavam num par estéreo e passavam pela
// mesma reverberação de 4 linhas com entrada mono, sem equalização nem
// profundidade. Agora:
//   1. BARRAMENTOS por papel (melodia, baixo, harmonia e camadas, percussão,
//      efeitos), cada um processado à parte — o passa-altas que limpa, o
//      corte da "lama" (300 Hz) na harmonia, a presência (3 kHz) na melodia,
//      a compressão por papel, a largura estéreo (as camadas abertas, o
//      baixo no centro) e a profundidade (o que vai ao fundo perde brilho);
//   2. a REVERBERAÇÃO como envio, cada papel com a sua quantidade: uma rede
//      de atrasos com realimentação de 8 linhas (J.-M. Jot e A. Chaigne,
//      "Digital delay networks for designing artificial reverberators", AES
//      90, 1991), difusão de entrada por passa-tudos e linhas levemente
//      moduladas (J. Dattorro, "Effect design, part 1: reverberator and
//      other filters", JAES 45(9), 1997), reflexões iniciais estéreo e o
//      decaimento que escurece com o tempo;
//   3. a SALA pelo estilo: o clube do jazz, a sala do choro, o estúdio do
//      rock, a sala de concerto da orquestra, a igreja do a cappella;
//   4. a COLA da soma (uma saturação de fita sutil) antes do master.
// Fontes de mistura: R. Izhaki, Mixing Audio, 3ª ed., Routledge, 2018; B.
// Owsinski, The Mixing Engineer's Handbook, 4ª ed., 2017.

#include "Composer.h"

#include <cstdint>
#include <string>

#include <vector>

namespace trioio
{

struct RoomProfile
{
    const char* name = "sala";
    double rt60 = 1.6;      // s (nos médios)
    double size = 1.0;      // escala das linhas
    double predelay = 0.018;  // s
    double damping = 0.35;  // perda de agudos por volta (0..1)
    double wet = 1.0;       // multiplica os envios
};

// A sala pelo conjunto (Ensemble, como inteiro; 0 = automático).
[[nodiscard]] RoomProfile roomForEnsemble(int ensemble);

// O grupo de mistura de uma voz: 0 melodia (0, 2), 1 baixo (1), 2 harmonia e
// camadas (3, 5, 6), 3 percussão (4); 4 é o dos gestos (efeitos).
[[nodiscard]] int mixGroupOf(int voice);
inline constexpr int kMixGroups = 5;

// O lugar de uma voz no palco do conjunto (2026-10-16): o lado (−1..1), a
// largura (1 = como veio; 0 = um ponto) e a profundidade (0 = na frente;
// 1 = no fundo: mais sala, menos brilho, um pouco mais baixo). O palco
// automático deixa o estéreo como as notas vieram.
struct StageSpot
{
    float pan = 0.0f, width = 1.0f, depth = 0.3f;
};
[[nodiscard]] StageSpot stageFor(int ensemble, int voice);

// Processa um barramento (no lugar) e o soma em L/R e no envio da sala.
void mixBus(int group, std::vector<float>& busL, std::vector<float>& busR, double sr, const RoomProfile& room,
            std::vector<float>& L, std::vector<float>& R, std::vector<float>& sendL, std::vector<float>& sendR,
            const StageSpot& spot = {});

// A reverberação: lê o envio e soma o molhado em L/R.
void reverb(const std::vector<float>& sendL, const std::vector<float>& sendR, double sr, const RoomProfile& room,
            std::vector<float>& L, std::vector<float>& R);

// A cola da soma: saturação de fita sutil.
void glue(std::vector<float>& L, std::vector<float>& R);

} // namespace trioio

namespace trioio::composition
{

// A humanização fina (2026-10-16): cada nota com as pequenas diferenças de
// quem toca — o tempo (alguns ms, menos na percussão), a força (alguns %, e
// o arco/palheta que alterna nas notas repetidas) e, nos instrumentos sem
// afinação fixa (sopros, cordas friccionadas, voz), um ou dois cents. As
// variações são de baixa frequência ao longo da frase (um passeio, não ruído
// branco): o músico adianta e atrasa por trechos, não nota a nota (H. Hennig
// et al., "The nature and perception of fluctuations in human musical
// rhythms", PLoS ONE 6(10), 2011: as flutuações têm correlação de longo
// alcance, 1/f). Devolve a linha da forma.
std::string humanize(Score& score, std::uint64_t seed);

// A expressão (2026-10-16; o autor: "para que gere emoção pra quem escuta"):
//   - o ARCO DA FRASE nas melodias (vozes 0 e 2): a força cresce para o
//     ponto alto (~60 % da frase) e se recolhe no fim (N. P. Todd, "The
//     dynamics of dynamics", JASA 91(6), 1992);
//   - a CARGA MELÓDICA: as notas mais agudas que a média da frase um pouco
//     mais fortes (J. Sundberg, A. Friberg e L. Frydén, as regras de
//     interpretação do KTH: "high loud", "phrase arch"; A. Friberg et al.,
//     "Overview of the KTH rule system for musical performance", Advances
//     in Cognitive Psychology 2(2–3), 2006);
//   - o FINAL que respira: um ritardando nos últimos ~6 s (o tempo alarga
//     até ~25 %, a curva do "final ritard" de Friberg e Sundberg, "Does
//     music performance allude to locomotion?", JASA 105(3), 1999) e a
//     fermata no último acorde.
// Devolve a linha da forma.
std::string applyExpression(Score& score, bool ending = true);

} // namespace trioio
