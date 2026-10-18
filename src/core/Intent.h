#pragma once

// A intenção (docs/BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md, "prompts.json";
// docs/PROMPTS_E_INTENCOES.md): três camadas que nunca se substituem.
//
//   promptOriginal   — o texto, só quando foi fornecido (pelo autor ou junto
//                      com o arquivo de um gerador); guardado como veio;
//   declaredIntent   — a TRADUÇÃO do prompt para o vocabulário do Trioio,
//                      com cada palavra que deu um campo à vista (editável:
//                      o autor revisa a tradução);
//   analysisInferred — o que a escuta do áudio mostra, no mesmo vocabulário.
//
// O Trioio NÃO alega recuperar parâmetros internos de um gerador: a tradução
// lê palavras, e a inferência lê o som. A comparação das duas (o que foi
// pedido × o que soa) entra na resposta como a abertura do Terceiro Olho.

#include "Composer.h"
#include "Reading.h"
#include "ScaleAffect.h"

#include <string>
#include <vector>

namespace trioio::composition
{

// o vocabulário: cada campo é opcional (< 0 = não dito / não lido)
struct IntentFields
{
    float energy = -1.0f, density = -1.0f, brightness = -1.0f, harmonicity = -1.0f;  // 0..1
    float bpm = -1.0f;
    int mood = -1;      // Intention (ScaleAffect.h): luminosa, sombria, tensa, serena, suspensa, ambígua
    int ensemble = -1;  // Ensemble (Ensemble.h), o estilo
    int harmonyLanguage = -1;  // HarmonyLanguage (HarmonyWriting.h)
    int voice = -1;     // 1 = com voz cantada, 0 = instrumental
    int rockStyle = -1; // RockStyle (Ensemble.h): punk, pesado, blues, progressivo, post-rock, psicodélico, noise
    int narrative = -1; // a gramática narrativa: 0 freytag, 1 kishotenketsu, 2 ondas, 3 plato, 4 arco
    float dance = -1.0f;  // o eixo dança (0 introspectivo … 1 dançante)
    float freedom = -1.0f;  // a liberdade pedida ("improvisado", "aleatório"): abre o Terceiro Olho
};

// uma palavra do prompt e o campo que ela deu (a tradução à vista)
struct IntentMatch
{
    std::string word, field, value;
};

struct DeclaredIntent
{
    IntentFields fields;
    std::vector<IntentMatch> matches;
    std::vector<std::string> unread;  // as palavras de conteúdo que não deram campo
    bool revised = false;             // o autor revisou a tradução
};

// Traduz um prompt (português, inglês, francês ou espanhol) para o
// vocabulário: energia ("calmo", "energetic", "explosivo"), densidade,
// brilho, harmonicidade ("ruidoso", "tonal"), andamento ("120 bpm",
// "lento"), humor, estilo (jazz, samba, choro, rock, orquestra…), linguagem
// harmônica ("modal", "impressionista") e voz.
[[nodiscard]] DeclaredIntent translatePrompt(const std::string& prompt);

// O que a escuta mostra: energia (nível médio), densidade (ataques por
// segundo), brilho (centroide), harmonicidade (periodicidade e planura),
// andamento (o pulso, se confiável) e humor (o modo da tonalidade e a energia).
[[nodiscard]] IntentFields inferIntent(const std::vector<perception::Frame>& frames, const std::vector<perception::Onset>& onsets,
                                       double duration, const Reading& reading);

// O que foi pedido × o que soa: um item por campo declarado E lido.
struct IntentGap
{
    std::string field;
    float declared = 0.0f, observed = 0.0f;
    float gap = 0.0f;  // 0..1 (o andamento: a razão, dobro/metade contam como perto)
    std::string text;  // "energia: pedida 0,80, ouvida 0,35 — a gravação é mais contida que o prompt"
};
[[nodiscard]] std::vector<IntentGap> compareIntent(const IntentFields& declared, const IntentFields& observed);

// A intenção entra na resposta: o que o autor não escolheu na interface vem
// da declarada (o humor → Options::intention, o estilo → o conjunto, a
// linguagem harmônica); a distância média entre o pedido e o ouvido abre o
// Terceiro Olho (a imprevisibilidade sobe até +0,25). Devolve as linhas da
// forma.
std::vector<std::string> applyIntent(Options& opt, const DeclaredIntent& declared, const IntentFields& observed);

[[nodiscard]] std::string intentFieldsText(const IntentFields& f);
[[nodiscard]] const char* narrativeName(int index);  // freytag kishotenketsu ondas plato arco

// O cálculo a partir das ASSOCIAÇÕES de uma palavra (as que a ponte semântica
// trouxe da rede: os adjetivos que a descrevem, os sentidos próximos), cada
// uma com o seu peso: cada associação passa pelo léxico; os campos
// numéricos são a média ponderada; o humor e a forma, o mais votado (com ao
// menos 25 % do peso dos que votaram). Só o caráter: o estilo (conjunto,
// rock) nunca vem das associações ("heavy rain" é chuva forte, não rock
// pesado). `explain` recebe as associações que deram campo.
[[nodiscard]] IntentFields intentFromAssociations(const std::vector<std::pair<std::string, double>>& words, std::string* explain = nullptr);

} // namespace trioio::composition
