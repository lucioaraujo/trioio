#pragma once

// O arquivo de escalas do Trioio (docs/ESCALAS.md, "Arquivo de escalas").
//
// Escrito pelo Trioio a partir do conhecimento teórico público:
//   - os gêneros gregos (Arquitas, Dídimo, Ptolomeu, Aristóxeno);
//   - a entonação justa e a série harmônica (Partch, Helmholtz);
//   - os temperamentos históricos, calculados pelo ciclo de quintas (Aron,
//     Salinas, Werckmeister, Kirnberger, Vallotti, Young);
//   - as divisões iguais e as escalas não oitavantes (Bohlen-Pierce,
//     Carlos);
//   - os modelos de maqam (Cairo, 1932) e de makam (Arel–Ezgi–Uzdilek);
//   - os shrutis, thāts e os 72 melakartas (Venkatamakhin);
//   - modelos de gamelão, tailandês, amadinda, timbila, chinês, japonês e
//     os modos nordestinos (Siqueira).
// Cada escala cita a fonte, como numa pesquisa, e diz quando é modelo ou
// aproximação. Gerado por tools/gerar_arquivo_de_escalas.py em
// data/escalas/arquivo_trioio.txt. Nenhum arquivo de terceiros é copiado.
//
// O autor pode juntar escalas próprias no mesmo formato, ou abrir um .scl
// qualquer (o formato Scala é aberto).

#include "ScaleBank.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace trioio::composition
{

// Como a linguagem USA a escala, quando a fonte o descreve (estudo
// transversal, §6.12; aplicado por ScaleIdiom.h). Cents a partir da tônica; podem
// passar da oitava ou ficar abaixo da tônica.
struct ScaleBehavior
{
    std::vector<double> up, down;               // subida e descida
    std::vector<double> support;                // notas de apoio (a 1.ª é a principal)
    std::vector<double> path;                   // percurso: regiões ao longo da peça
    std::vector<std::vector<double>> signatures;  // frases-assinatura
    std::vector<double> cadence;                // fórmula de fim
    std::string ornament;                       // meend, andolan, blue
    std::string texture;                        // bordão, heterofonia ou acordes
    std::string rhythm;                         // o ciclo rítmico usual (RhythmCycles.h), pelo nome
    std::vector<int> vamp;                      // textura vamp: os graus (0 = I) dos dois acordes
    std::string voicing;                        // o voicing usual (Voicing.h): quartal, sowhat…
    std::string source, note;
    [[nodiscard]] bool any() const noexcept
    {
        return !up.empty() || !down.empty() || !support.empty() || !path.empty() || !signatures.empty()
            || !cadence.empty() || !ornament.empty() || !texture.empty() || !rhythm.empty() || !vamp.empty();
    }
};

struct ArchiveScale
{
    std::string name;    // "Werckmeister III"
    std::string family;  // "temperamento histórico (sobre Dó)"
    std::string source;  // "A. Werckmeister, Musicalische Temperatur, 1691"
    std::string note;    // "modelo…", "aproximação…" (pode ser vazio)
    double periodCents = 1200.0;
    std::vector<double> stepsCents;  // dentro do período, crescentes, começando em 0
    ScaleBehavior behavior;
};

// O formato do arquivo: um bloco por escala ("= nome", depois "família:",
// "período:", "graus:", "fonte:", "nota:"); graus e período em cents ou em
// razões (9/8, 2/1). "#" começa comentário. Blocos ilegíveis vão para
// `errors` com o nome.
[[nodiscard]] std::vector<ArchiveScale> parseScaleArchive(const std::string& text, std::vector<std::string>& errors);

// Um arquivo .scl (formato Scala, aberto). nullopt se não for legível.
[[nodiscard]] std::optional<ArchiveScale> parseScl(const std::string& text, const std::string& name);

struct ArchivePick
{
    ScaleChoice choice;  // graus em semitons
    float period = 12.0f;  // em semitons
    std::string source;
    ScaleBehavior behavior;
    int candidates = 0;
    std::string error;
};

// Uma escala de LINGUAGEM (maqam, rāga, thāt, melakarta, modo, gamelão,
// flamenco, Nordeste, Grécia antiga, Messiaen…), e não um sistema de
// afinação ou uma abstração (MOS, EDO, temperamentos, conjuntos de Forte,
// séries, entonação justa, Euler-Fokker, Wilson, não oitavantes), que vestem
// qualquer campo sem dizer de que música ele é.
[[nodiscard]] bool isLanguageScale(const ArchiveScale& scale);

// A escala que melhor veste o campo (12 classes de altura) e a tônica.
// `query` vazio: as escalas de LINGUAGEM de oitava (1190–1210 cents) com 5 a
// 9 graus (antes, todas de 5 a 24 graus, e um temperamento de 12 notas
// vencia qualquer mixagem; 2026-10-11). Com `query`, todo o arquivo.
// `query`: o nome exato, ou todas cujo nome, família ou fonte contém o termo
// (sem caixa, sem acento obrigatório nas letras ASCII), sem restrição de
// período. Sorteia entre as `choices` melhores escalas distintas que estejam
// a até 0,05 da melhor (as quase empatadas), preferindo as que têm
// comportamento (como a linguagem toca a escala).
[[nodiscard]] ArchivePick fitArchive(const std::vector<ArchiveScale>& archive, const std::array<float, 12>& field,
                                     const std::string& query, std::uint64_t seed, int choices = 5);

// As `count` escalas distintas que melhor vestem o campo, da melhor para a
// pior, com a pontuação de fitArchive (sem sorteio): para reconhecer a
// linguagem de uma referência (docs/LEITURA_E_GABARITO.md).
struct RankedScale
{
    const ArchiveScale* scale = nullptr;
    int tonic = 0;
    double score = 0.0;
};
[[nodiscard]] std::vector<RankedScale> rankArchive(const std::vector<ArchiveScale>& archive,
                                                   const std::array<float, 12>& field, int count,
                                                   int minDegrees = 5, int maxDegrees = 24);

} // namespace trioio::composition
