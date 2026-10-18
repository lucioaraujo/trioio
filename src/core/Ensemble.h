#pragma once

// O conjunto por estilo (docs/MAQUINA_DE_TIMBRES.md §4): o estilo não é um
// timbre, é uma formação — cada voz do Trioio recebe o instrumento do seu
// papel naquele estilo. No choro: a flauta no motivo, o bandolim no
// contracanto, o violão de 7 cordas na baixaria, o cavaquinho na harmonia, o
// pandeiro na percussão. A extensão de cada instrumento (AcousticVoice.h)
// põe as notas na oitava que cabe.
//
// O conjunto entra depois do diálogo e do jogo e antes do Terceiro Olho: o
// olho continua livre para trocar um timbre (o desvio fica audível contra a
// formação). As famílias proibidas pelas regras do autor não entram.

#include "Composer.h"
#include "RhythmCycles.h"
#include "Strategy.h"
#include "Structure.h"

#include <cstdint>
#include <utility>
#include <vector>

#include <string>

namespace trioio::composition
{

enum class Ensemble
{
    automatic,  // o Trioio escolhe os timbres como antes (pela escuta da referência)
    choro,
    samba,
    jazz,
    manouche,
    bolero,
    charanga,   // salsa em formação de charanga: flauta e violinos
    chamber,    // câmara: quarteto de cordas
    orchestra,  // orquestra
    salsa,      // salsa com metais (trompetes e trombones), desde 2026-10-15
    acappella,  // só vozes: cantadas, boca chiusa e sussurro
    rock,       // guitarras elétricas, baixo elétrico e bateria
    escola,     // escola de samba: o samba-enredo e a bateria
    forro,      // forró pé-de-serra: sanfona, zabumba e triângulo
    tango,      // tango: bandoneón, violino, piano e contrabaixo
};
inline constexpr int kEnsembles = 15;

// Os subestilos do rock (2026-10-18; o autor: "no rock há uma gama bastante
// larga de subestilos e intenções", "há estilos mais noises, mais postrock,
// mais clássicos, mais pesados"). Cada um muda a bateria, a guitarra base, o
// acorde, o baixo, o fraseado e a linguagem harmônica (StyleFigures.cpp):
//   classic     — o hard rock clássico: backbeat, power chords em colcheias
//   punk        — bumbo nos tempos e caixa nos contratempos; colcheias iguais
//   heavy       — o pesado (metal): bumbo duplo, chug em semicolcheias, grave
//   blues       — o blues-rock: shuffle, o boogie da 5ª à 6ª, a 7ª, bends
//   prog        — o progressivo: 3+3+2, acordes quartais, legato
//   postrock    — o post-rock: esparso, acordes abertos e longos, guitarra limpa
//   psychedelic — o psicodélico: groove nos tons, o órgão, vibrato largo e lento
//   noise       — o noise rock: clusters, palhetada em trêmulo, a microfonia
enum class RockStyle { classic, punk, heavy, blues, prog, postrock, psychedelic, noise };
inline constexpr int kRockStyles = 8;
[[nodiscard]] const char* rockStyleName(RockStyle s);
[[nodiscard]] bool parseRockStyle(const std::string& name, RockStyle& out);

[[nodiscard]] const char* ensembleName(Ensemble e);
[[nodiscard]] bool parseEnsemble(const std::string& name, Ensemble& out);

// Reatribui os timbres da partitura pelo conjunto. Devolve a linha da forma
// (vazia no automático). Com `onlySynthetic`, só as notas que ainda têm um
// timbre sintético (anterior aos corpos acústicos: as que o Terceiro Olho
// criou depois, como o eco em cânone) — as quimeras do rasgo ficam.
std::string applyEnsemble(Score& score, Ensemble e, const Constraints& rules, bool onlySynthetic = false);

// O conjunto LIDO na referência (2026-10-15): o Trioio escolhe o estilo pelo
// que escutou — o ciclo rítmico reconhecido (clave → salsa, baião → forró,
// bossa → samba, shuffle → jazz), o swing medido (jazz; manouche no
// andamento rápido), o pulso e o andamento, o brilho e o ruído (rock), e,
// sem pulso, se a referência é uma voz (a cappella) ou um conjunto (câmara,
// orquestra). Depois o MODO responde: seguir e interromper tocam no estilo
// lido; transfigurar vai a um estilo vizinho (samba → choro, jazz →
// manouche…); contrapor vai ao estilo contrário em energia (rock ↔ câmara…).
// São regras de escuta escritas, não um classificador treinado: o porquê
// vai inteiro para a linha da forma.
struct EnsembleReading
{
    Ensemble read = Ensemble::automatic;    // o que a escuta reconheceu
    Ensemble chosen = Ensemble::automatic;  // o que o modo escolheu tocar
    std::string why;                        // a linha da forma
};
[[nodiscard]] EnsembleReading readEnsemble(const std::vector<perception::Frame>& frames,
                                           const std::vector<perception::Onset>& onsets, double duration,
                                           const perception::Pulse& pulse,
                                           const std::vector<std::pair<double, double>>& pulsedSpans,
                                           const std::vector<RhythmCycle>* archive, strategy::Mode mode, std::uint64_t seed);

} // namespace trioio::composition
