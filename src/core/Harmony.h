#pragma once

// Harmonia em movimento (docs/HARMONIA.md; camada 1 de docs/MUSICALIDADE.md).
//
// Até aqui o Trioio tocava um "campo" de notas por seção: o baixo ficava na
// tônica, e nada acompanhava a progressão de acordes que faz uma canção
// andar. Esta camada:
//
//   1. LÊ os acordes da referência, tempo a tempo, pelo croma pesado pela
//      energia; tempos isolados são alisados e tempos iguais se juntam (o
//      ritmo harmônico sai da música).
//      Numa referência de voz só, o croma é a própria linha: as notas longas
//      e fortes formam o acorde (a harmonia nasce da linha);
//   2. TRANSFORMA a progressão conforme o modo:
//      - seguir: como está;
//      - transfigurar: uma transformação neo-riemanniana (P, L, R ou
//        composta) aplicada a cada acorde (Riemann; Cohn, Lewin), que
//        mantém duas notas comuns e muda a cor; cópia fixada de
//        NeoRiemannian.hpp do Rasgo Synth Studio;
//      - contrapor: rearmonização. A dominante que resolve vira a
//        substituta de trítono, e outros acordes saltam por mediante
//        cromática (as técnicas do HarmonicWanderer do Studio: ii–V com
//        substituição de trítono; Jobim);
//      - interromper: a mesma progressão, em fragmentos;
//   3. APLICA:
//      - o baixo (voz 1) segue a fundamental de cada acorde;
//      - o motivo (voz 0) cai numa nota do acorde nos ataques fortes;
//      - uma voz de harmonia (voz 6) sustenta o acorde, com condução de
//        vozes parcimoniosa (a inversão que menos se move).
//
// Nos 12 sons, os acordes são as tríades lidas (e transformadas). Com a
// máquina de escalas, ou numa escala microtonal ou não oitavante, os acordes
// são tríades dos GRAUS da escala da seção (grau, +2, +4): a que melhor
// responde ao acorde lido naquele tempo. Assim qualquer escala tem harmonia.

#include "Composer.h"
#include "ScaleMachine.h"
#include "Structure.h"

#include <cstdint>
#include <string>
#include <vector>

namespace trioio::composition
{

enum class ChordQuality { major, minor, diminished, augmented };

// A cor do acorde (2026-10-17; docs/MAQUINA_DE_HARMONIA.md): as notas
// acrescentadas à tríade, em bits. A leitura da referência dá só tríades
// (cor 0); a máquina de harmonia escreve as cores das linguagens.
enum ChordColor : std::uint8_t
{
    kSeventh = 1,        // 7ª menor (no diminuto: o meio-diminuto)
    kMajorSeventh = 2,   // 7ª maior
    kNinth = 4,          // 9ª
    kSixth = 8,          // 6ª (no diminuto: a 7ª diminuta)
    kSharpEleventh = 16, // ♯11 (o lídio)
    kThirteenth = 32,    // 13ª
    kFlatNinth = 64,     // ♭9
    kSus4 = 128,         // a 4ª no lugar da 3ª
};

struct Chord
{
    int root = 0;  // classe de altura 0..11
    ChordQuality quality = ChordQuality::major;
    std::uint8_t color = 0;  // ChordColor
    [[nodiscard]] std::vector<int> pitchClasses() const;
    [[nodiscard]] bool contains(int pc) const;
    [[nodiscard]] std::string name() const;  // "Solm", "Ré", "Si°"
    bool operator==(const Chord& o) const { return root == o.root && quality == o.quality; }
};

struct ChordSpan
{
    double start = 0.0, end = 0.0;
    Chord chord;
    float confidence = 0.0f;  // semelhança com o molde do acorde, 0..1
    int bassPc = -1;  // o baixo, quando não é a fundamental (o pedal, a inversão); −1 = a fundamental
    // a harmonia microtonal (a linguagem de Partch): as classes de altura em
    // semitons com fração, a fundamental primeiro, e o nome (O 7/4, U 3/2…);
    // vazia = o acorde de 12 semitons acima
    std::vector<float> microPcs {};
    std::string microName {};
};

// Os acordes da referência, lidos a cada `unit` segundos (um tempo) a partir
// de `gridStart`; trechos sem acorde claro ficam de fora.
[[nodiscard]] std::vector<ChordSpan> readChords(const std::vector<perception::Frame>& frames, double duration,
                                                double gridStart, double unit);

// O campo da LINGUAGEM de [0, duration): as notas dos acordes lidos (pelo
// pulso; cada tríade por duração) e as da melodia predominante, cada metade
// somando ½. É o campo com que o arquivo de escalas reconhece a linguagem
// (Reading, `ler`) e com que o compositor escolhe a escala em
// `--alturas arquivo` e nas misturas: o croma bruto de uma mixagem tem um
// pouco das 12 classes, e uma escala cromática ou um modo de 8 notas o
// vestia (2026-10-11).
[[nodiscard]] std::array<float, 12> languageField(const std::vector<perception::Frame>& frames,
                                                  const std::vector<perception::Onset>& onsets, double duration);

// Aplica a camada à partitura (ver acima). `shift` é o deslocamento de
// tônica que o modo já deu à melodia (transfigurar +7, contrapor +6).
// `scales`: a escala de cada seção (com a máquina de escalas ou fora dos 12
// sons, os acordes são tríades dos graus dela).
void harmonyInMotion(Score& score, const std::vector<perception::Frame>& frames, double beatStart,
                     double beatLength, float shift, const std::vector<SectionScale>& scales, const Options& options);

} // namespace trioio::composition
