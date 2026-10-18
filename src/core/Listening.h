#pragma once

// A escuta do autor como memória do instrumento (docs/TRIOIO_CEREBRO.md §10:
// "adaptação local, não treinamento"; "alterar tendências, não aprisionar o
// instrumento em um perfil de gosto"). Cada julgamento — aceitar, rejeitar
// ou transformar uma resposta — guarda o que aquela resposta usou: as
// famílias de timbre, as escolhas do Terceiro Olho e a sala. O histórico
// vira TENDÊNCIAS: multiplicadores nos sorteios do compositor, do olho e da
// sala, limitados a 0,5..2 e com os julgamentos antigos pesando cada vez
// menos (esquecimento gradual). Sem julgamentos, tudo é 1 e a composição é
// idêntica à de antes. Especificação: docs/ESCUTA.md.

#include "Composer.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace trioio::composition
{

enum class Verdict { accept, reject, transform };

[[nodiscard]] const char* verdictName(Verdict v);              // aceitar rejeitar transformar
[[nodiscard]] bool parseVerdict(const std::string& s, Verdict& out);

struct Judgement
{
    std::string derivationId;   // der-…
    Verdict verdict = Verdict::accept;
    std::string note;           // a palavra do autor, livre
    std::string when;           // ISO 8601
    std::vector<std::pair<int, double>> timbres;  // (família, quantas notas; num trecho, a de fronteira conta a fração de dentro)
    std::vector<int> eyeChoices;                  // escolhas do Terceiro Olho na resposta (ou no trecho)
    int room = -1;                                // sala, ou −1 se desconhecida
    // Julgamento por trecho (docs/ESCUTA.md): [spanStart, spanEnd) em
    // segundos; sem trecho, vale a resposta inteira.
    double spanStart = -1.0, spanEnd = -1.0;
    std::string section;      // "3" ou "A", quando o trecho veio de --secao
    double roomWeight = 1.0;  // a sala é da peça: num trecho, pesa pela fração da peça
    [[nodiscard]] bool hasSpan() const noexcept { return spanStart >= 0.0 && spanEnd > spanStart; }
};

// O que uma resposta usou, lido das suas marcas de forma e das notas.
void describeResponse(const std::vector<std::string>& formMarks, const std::vector<int>& noteTimbres,
                      Judgement& out);

struct MarkAt
{
    double time = 0.0;
    std::string text;
};
struct NoteAt
{
    double time = 0.0, duration = 0.0;
    int timbre = -1;
};

// O fim da resposta: o fim da última nota.
[[nodiscard]] double responseEnd(const std::vector<NoteAt>& notes);

// O que o trecho [a, b) usou: o timbre pelas notas, a de fronteira pela
// fração que soa dentro; o olho pelas marcas dentro dele; a sala com peso (b − a) / fim.
void describeSpan(const std::vector<MarkAt>& marks, const std::vector<NoteAt>& notes, double a, double b,
                  Judgement& out);

// "0:00-1:00", "12.5-40", "1:41-fim" → [a, b), limitado ao fim. false se
// ilegível, vazio ou começando depois do fim.
[[nodiscard]] bool parseSpan(const std::string& text, double end, double& a, double& b);

// Os intervalos das seções pedidas: por número ("3") ou pela letra da forma
// por repetição ("A" — todas as seções A, inclusive os retornos). Cada seção
// vai da sua marca até a marca da seguinte (ou o fim). Vazio se nenhuma.
[[nodiscard]] std::vector<std::pair<double, double>> sectionSpans(const std::vector<MarkAt>& marks, double end,
                                                                  const std::string& query);

// Lê "Terceiro Olho · objeto sonoro: cluster por …" → índice 0..16 (−1 se
// não for marca do olho). Os nomes vêm de ThirdEye.cpp.
[[nodiscard]] int eyeChoiceFromMark(const std::string& mark);
// Lê "sala: HALL (Rasgo Modular)" → Room (−1 se não for marca de sala).
[[nodiscard]] int roomFromMark(const std::string& mark);

// Histórico do mais antigo ao mais novo → tendências.
[[nodiscard]] Tendencies tendenciesFrom(const std::vector<Judgement>& history);

} // namespace trioio::composition
