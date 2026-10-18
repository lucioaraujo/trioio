#pragma once

// A harmonia escrita (2026-10-17; docs/HARMONIA.md, "Harmonia escrita";
// auditoria de 14/10, item 6). Até aqui a harmonia era LIDA na referência e
// transformada pelo modo; a função harmônica não era escrita. Esta camada,
// nos 12 sons, entre a leitura e a aplicação (o baixo, o motivo e a voz de
// harmonia seguem o que for escrito):
//   1. a TONALIDADE de cada seção, pelos perfis de Krumhansl e Kessler
//      ("Tracing the dynamic changes in perceived tonal organization in a
//      spatial representation of musical keys", Psychological Review 89(4),
//      1982) sobre os acordes da seção, pesados pela duração;
//   2. as CADÊNCIAS no fim de cada seção, pelo modo — seguir: autêntica
//      (V → I) ou suspensiva (termina em V); transfigurar: às vezes a plagal
//      (IV → I); contrapor: a de engano (V → vi); interromper: suspensiva; o
//      fim da peça, sempre a perfeita; no jazz, ii–V–I;
//   3. as DOMINANTES SECUNDÁRIAS: antes de um acorde que não é a tônica,
//      às vezes a dominante dele (V/x) toma a segunda metade do acorde
//      anterior;
//   4. a PREPARAÇÃO DA MODULAÇÃO: quando a seção seguinte muda de tom, o fim
//      da seção chega à dominante do tom novo.
// W. Piston, Harmony, 5ª ed., Norton, 1987 (as cadências, as dominantes
// secundárias, a modulação); A. Schoenberg, Structural Functions of
// Harmony, 1954.

#include "Composer.h"
#include "Harmony.h"
#include "Strategy.h"

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace trioio::composition
{

struct KeyEstimate
{
    int tonic = 0;      // classe de altura
    bool major = true;
    double score = 0.0; // a correlação com o perfil
};

// A tonalidade dos acordes que começam em [t0, t1).
[[nodiscard]] KeyEstimate findKey(const std::vector<ChordSpan>& spans, double t0, double t1);
[[nodiscard]] std::string keyName(const KeyEstimate& k);

// Escreve as cadências, as dominantes secundárias e as preparações de
// modulação na sequência de acordes (no lugar). `sections`: [início, fim)
// de cada seção. `ensemble` (Ensemble.h, como inteiro) escolhe o ii–V–I do
// jazz. Devolve a linha da forma.
// Uma cadência escrita: o acorde de chegada [start, end), o tipo e a tônica.
struct CadenceMark
{
    double start = 0.0, end = 0.0;
    enum class Kind { authentic, half, plagal, deceptive, modulation } kind = Kind::authentic;
    KeyEstimate key;
};

// A máquina de harmonia (docs/MAQUINA_DE_HARMONIA.md, fase 1 e a linguagem
// impressionista): cada linguagem tem o seu vocabulário (as cores), a sua
// sintaxe e a sua pontuação (o repertório de cadências).
enum class HarmonyLanguage
{
    automatic, functional, jazz, brazilian, impressionist, northeastern, mineira, hermeto,
    // as fases 2 e 3 e os compositores (2026-10-17)
    modal, andalusian, chromatic, quartal, polytonal, octatonic, clusters, stasis, spectral,
    bartok, scriabin, villaLobos, moacir, guinga,
    // Pernambuco e Gismonti (2026-10-17)
    frevo, maracatu, gismonti,
    // a microtonal: as otonalidades e utonalidades de Partch (2026-10-17)
    partch,
};
inline constexpr int kHarmonyLanguages = 26;
[[nodiscard]] const char* harmonyLanguageName(HarmonyLanguage l);
[[nodiscard]] bool parseHarmonyLanguage(const std::string& name, HarmonyLanguage& out);
// A linguagem pedida, ou a automática: pelo conjunto (o jazz pede o jazz; o
// choro, o samba e o forró pedem a brasileira…) e, sem conjunto, pelo modo
// (transfigurar → impressionista; contrapor → jazz; os outros → funcional).
[[nodiscard]] HarmonyLanguage resolveLanguage(int requested, int ensemble, strategy::Mode mode);

std::string writeHarmony(std::vector<ChordSpan>& spans, const std::vector<std::pair<double, double>>& sections, double beat,
                         strategy::Mode mode, HarmonyLanguage language, std::uint64_t seed, std::vector<CadenceMark>* cadences = nullptr,
                         const std::vector<SpectralProfile>* spectra = nullptr);

// A condução de vozes a quatro partes (2026-10-17): o baixo (a fundamental)
// e três vozes de cima, numa busca entre todas as disposições (as
// permutações das notas do acorde e as oitavas de cada voz, entre Dó 3 e
// Fá 5), pontuadas pelas regras clássicas (Piston 1987, cap. 5–6; D. Huron,
// "Tone and voice: a derivation of the rules of voice-leading from
// perceptual principles", Music Perception 19(1), 2001):
//   - o movimento mínimo (as notas comuns ficam);
//   - sem quintas nem oitavas paralelas entre quaisquer duas vozes, o baixo
//     incluído;
//   - sem cruzamento, as vozes de cima a no máximo uma oitava entre si;
//   - a sensível sobe à tônica no V → I.
// `prev` (vazio = o primeiro acorde): as três vozes anteriores. `prevBassPc`
// e `bassPc`: as fundamentais. `leadingPc`: a sensível que deve subir
// (−1 = nenhuma). Devolve as três vozes, de baixo para cima.
[[nodiscard]] std::vector<float> voiceLead(const std::vector<float>& prev, const std::vector<int>& chordPcs, int prevBassPc, int bassPc,
                                           int leadingPc);

// Conta as quintas e oitavas paralelas entre dois acordes de quatro vozes
// (o baixo e as três de cima), para os testes e o relatório.
[[nodiscard]] int countParallels(const std::vector<float>& prev, const std::vector<float>& next, int prevBassPc, int bassPc);

// o arquivo de cadências (data/harmonia/arquivo_trioio_cadencias.txt,
// gerado por tools/gerar_arquivo_de_cadencias.py): cada cadência com os
// acordes relativos à tônica (Dó), o baixo de cada um (−1: a fundamental) e
// a fonte
struct CadenceEntry
{
    std::string name, family, language, degrees, soprano, bass, effect, source;
    std::vector<Chord> chords;
    std::vector<int> bassPcs;
    // a saliência das notas do penúltimo acorde, dada pelo autor (linha
    // "saliência: 11:3, 5:2", nota relativa à tônica : peso 1..3); o que não
    // vem aqui segue a regra da mistura conceitual
    std::map<int, int> salience;
};
std::vector<CadenceEntry> parseCadenceArchive(const std::string& text, std::vector<std::string>& errors);

// A MISTURA CONCEITUAL de duas cadências (A. Zacharakis, M.
// Kaliakatsos-Papakostas e E. Cambouropoulos, "Conceptual Blending in Music
// Cadences: A Formal Model and Subjective Evaluation", ISMIR 2015; G.
// Fauconnier e M. Turner, The Way We Think, 2002). O acorde final fica (a
// tônica); os PENÚLTIMOS acordes viram traços com saliência:
//   - as notas relativas à tônica (keyNote): a sensível que sobe à tônica e o
//     ♭2 que desce valem 3; a nota que resolve por semitom na terça do final
//     e a fundamental do penúltimo valem 2; as outras, 1;
//   - o tipo do acorde (chordNote): os intervalos sobre a fundamental; no
//     acorde de dominante, a 3ª e a 7ª menor (o trítono) valem 3, os outros 1.
// O espaço genérico é o que as duas têm em comum. Cada acorde candidato (12
// fundamentais × as tríades e tétrades usuais: sem ♭9 nem acordes grandes,
// que acumulariam traços só por terem muitas notas) guarda os traços que contém; a
// mistura precisa de ao menos uma nota PRÓPRIA de cada entrada (fora do
// espaço genérico: uma variante de uma só não é mistura), não
// pode ser igual a nenhuma delas, e obedece às restrições de consistência
// (o acorde do Trioio já não tem terça maior e menor juntas). A nota é a soma
// das saliências mantidas, menos 0,5 por nota que não vem de nenhuma
// entrada. Da perfeita (V7 → I) com a frígia modal (♭vii → I) emerge a
// substituição de trítono (♭II7 → I), como no artigo.
struct CadenceBlend
{
    Chord penultimate;  // relativo à tônica (Dó)
    Chord final;
    double score = 0.0;
    std::string kept;   // os traços mantidos, em palavras
};
[[nodiscard]] std::vector<CadenceBlend> blendCadences(const CadenceEntry& a, const CadenceEntry& b, int maxResults = 5);

// a chegada sorteada no arquivo: em cada cadência marcada, com
// probabilidade `chance`, troca os últimos acordes por uma cadência do
// arquivo da mesma linguagem, transposta à tonalidade da marca (e o baixo,
// se ela pede). Devolve quantas trocou.
// Uma parte das chegadas sorteadas (`blendChance`) é INVENTADA: duas
// cadências do arquivo da mesma linguagem se misturam (blendCadences) e uma
// das três melhores misturas vira a chegada; `invented` recebe os nomes.
int drawArchiveCadences(std::vector<ChordSpan>& spans, std::vector<CadenceMark>& cadences, const std::vector<CadenceEntry>& archive,
                        HarmonyLanguage language, double beat, std::uint64_t seed, double chance = 0.4, double blendChance = 0.3,
                        std::vector<std::string>* invented = nullptr);

} // namespace trioio::composition
