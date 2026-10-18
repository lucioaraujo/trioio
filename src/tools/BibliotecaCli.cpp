// trioio_biblioteca — o importador da Biblioteca na linha de comando.
//
//   trioio_biblioteca varrer <biblioteca> <dados>   registra/atualiza manifestos
//   trioio_biblioteca listar <dados>                mostra o que está registrado
//   trioio_biblioteca autorizar <dados> <id> --origem O --fundamento F --usos a,b --por NOME
//                     [--licenca L] [--evidencia E] [--ferramenta T] [--versao V] [--restricoes R]
//   trioio_biblioteca recusar <dados> <id> --por NOME [--motivo M]
//   trioio_biblioteca revogar <dados> <id> --por NOME [--motivo M]
//   trioio_biblioteca analisar <dados> <id>         descritores + relatório (exige `analyze`)
//   trioio_biblioteca responder <dados> <id> --modo M [--seed N] [--alturas A] [--tempo T] [--motor partitura|energia]
//                     gera a resposta (exige `derive`)
//
// Só lê a Biblioteca; escreve apenas em <dados>/indice/referencias/.
// Ver docs/IMPORTADOR_DA_BIBLIOTECA.md.

#include "Analysis.h"
#include "BandSplit.h"
#include "Lines.h"
#include "Declaration.h"
#include "LibraryImporter.h"
#include "Mashup.h"
#include "Reading.h"
#include "ScaleArchive.h"
#include "RhythmCycles.h"
#include "ScoreSynth.h"
#include "Voicing.h"
#include "Ensemble.h"
#include "HarmonyWriting.h"
#include "Recombination.h"
#include "Response.h"
#include "ListeningStore.h"
#include "ManifestJson.h"
#include "Retention.h"
#include "DataFiles.h"
#include "Prompts.h"
#include "Versions.h"
#include "Dramaturgy.h"
#include "SemanticBridge.h"
#include "Intent.h"

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cmath>
#include <map>
#include <string>

using namespace trioio::provenance;
using namespace trioio::library;
namespace strategy = trioio::strategy;

namespace
{
const char* estado(const Manifest& m)
{
    if (m.analysis == AnalysisState::unavailable)
        return "indisponível";
    switch (m.authorization.status)
    {
        case AuthorizationStatus::pending: return "pendente";
        case AuthorizationStatus::authorized: return "autorizado";
        case AuthorizationStatus::refused: return "recusado";
        case AuthorizationStatus::revoked: return "revogado";
    }
    return "?";
}

const char* material(MaterialType t)
{
    switch (t)
    {
        case MaterialType::audio: return "áudio";
        case MaterialType::stem: return "stem";
        case MaterialType::midi: return "MIDI";
    }
    return "?";
}

int uso()
{
    std::puts("uso:\n"
              "  trioio_biblioteca varrer <biblioteca> <dados>\n"
              "  trioio_biblioteca listar <dados>\n"
              "  trioio_biblioteca versoes <dados> <ref> <ref> [<ref>…]   (o que não muda, o que diverge, as operações)\n"
              "  trioio_biblioteca prompt <dados> <ref> \"texto\" [--fonte autor|gerador|metadados]   (o prompt e a intenção)\n"
              "  trioio_biblioteca prompt <dados> <ref> [--mostrar | --revisar \"energia=0.3,humor=serena\" | --apagar]\n"
              "  trioio_biblioteca limpar <dados> [--manter 3] [--executar sim]   (as sessões de teste não julgadas vão para a lixeira; reversível)\n"
              "  trioio_biblioteca esvaziar-lixeira <dados> [--dias 7] [--executar sim]\n"
              "  trioio_biblioteca limpar-cache <dados> [--tudo sim] [--executar sim]   (o cache de análise é reconstruível)\n"
              "  trioio_biblioteca semantica <pasta-de-dados> \"chuva, saudade\" [ms]   (a ponte semântica: consulta e o cálculo)\n"
              "  trioio_biblioteca misturar-cadencias \"cadência A\" \"cadência B\"   (a mistura conceitual: cadências novas)\n"
              "  trioio_biblioteca cadencias [família|linguagem]   (o arquivo de cadências da máquina de harmonia)\n"
              "  trioio_biblioteca timbres <saida.wav> [família…] [--notas 44,52,…]   (audição: a mesma frase em cada família)\n"
              "  trioio_biblioteca efeitos <saida.wav>   (audição dos efeitos de cinema)\n"
              "  trioio_biblioteca autorizar <dados> <id> --origem O --fundamento F --usos a,b --por NOME\n"
              "        [--licenca L] [--evidencia E] [--ferramenta T] [--versao V] [--restricoes R]\n"
              "  trioio_biblioteca recusar <dados> <id> --por NOME [--motivo M]\n"
              "  trioio_biblioteca revogar <dados> <id> --por NOME [--motivo M]\n"
              "  trioio_biblioteca analisar <dados> <id>\n"
              "  trioio_biblioteca responder <dados> <id> --modo M [--seed N]\n"
              "        [--alturas referencia|catalogo|espectral|microtonal|inventada|edo|justa|pitagorica|quartos|arquivo]\n"
              "        [--escala TERMO|arquivo.scl]  (arquivo de escalas: \"pelog\", \"maqam\", \"Werckmeister\"…)\n"
              "        [--tempo auto|pulso|livre]\n"
              "        [--motor partitura|energia] [--imprevisibilidade 0..1] [--escuta sim|nao]\n"
              "        [--regras ARQUIVO|nao]  (padrão: <dados>/regras.txt, se existir) [--ritmo sim|nao] [--mosaico sim|nao] [--harmonia sim|nao] [--maquina sim|nao]\n"
              "        [--intencao luminosa|sombria|tensa|serena|suspensa|ambigua|arco]  (liga a máquina de escalas)\n"
              "        [--mistura \"rāga Yaman + maqam Hijaz + blues\"] [--tecnica colagem|hibrido|camadas|transicao]\n"
              "        [--ciclo maqsum|tīntāl|clave son|lancaran|…]  (ciclo rítmico de uma cultura, docs/RITMO.md)\n"
              "        [--com OUTRO_ID] [--da-outra harmonia,ritmo,memoria,corpo|tudo]  (mashup de materiais)\n"
              "        [--voicing tercas|quartal|sowhat|planing|bitonal|aberto|cluster]\n"
              "        [--conjunto auto|choro|samba|jazz|manouche|bolero|charanga|camara|orquestra|salsa|acappella|rock|escola|forro|tango|lido]\n"
              "        [--efeitos sim]   (efeitos de cinema na dramaturgia: riser, impacto, whoosh, ambiências)\n"
              "        [--rasgo 0..1]    (o rasgo modular nos corpos: 0 = o instrumento, 1 = o inaudito; padrão 0,5)\n"
              "        [--humano nao]    (desliga a humanização fina: tempo, força e afinação por nota)\n"
              "        [--harmonia-escrita nao]  (desliga as cadências, as dominantes secundárias e as modulações escritas)\n"
              "        [--linguagem-harmonica auto|funcional|jazz|brasileira|impressionista|nordestina|mineira|hermeto|modal|andaluza|cromatica|quartal|politonal|octatonica|clusters|estatica|espectral|bartok|scriabin|villa-lobos|moacir|guinga|frevo|maracatu|gismonti|partch]  (a máquina de harmonia)\n"
              "        [--som-da-aura sim]  (Hermeto Pascoal: a fala da referência vira a melodia, cada nota harmonizada)\n"
              "        [--costura corte|pivo|ponte|sobreposicao]  (fronteiras entre seções, na colagem)\n"
              "        [--andamento metade|dobro]  (o nível do pulso pela sua escuta, quando a leitura é ambígua)\n"
              "        [--forma contraste|repeticao]  (seções pelo contraste, ou pelo que volta: A B A…)\n"
            "        [--jogo sim]  (o Trioio inventa regras e as joga; docs/JOGO.md)\n"
            "        [--jogo-no-som nao]  (os critérios do jogo só na partitura, sem os renders de escuta)\n"
            "        [--stems sim]  (um WAV por voz, os efeitos e a sala em saidas/stems/, para a DAW)\n"
            "        [--prompt nao]  (ignora o prompt e a intenção declarada da referência)\n"
            "        [--rock classico|punk|pesado|blues|progressivo|post-rock|psicodelico|noise]  (o subestilo, com --conjunto rock)\n"
            "        [--influencias \"rock/pesado, brasil/frevo\" | auto]  (a árvore de influências: tendências, polinização, mutação)\n"
            "        [--forca-influencias 0..1] [--sem \"percussão, baixo\"]  (o quanto a árvore pesa; o que o autor tira)\n"
            "        [--narrativa freytag|kishotenketsu|ondas|plato|arco|\"ondas+plato\"]  (a gramática da forma; duas se misturam)\n"
            "        [--palavras \"dançante, maracatu, virada\"]  (o prompt da resposta, opcional: intenção, forma, dança e nós da árvore)\n"
            "        [--semantica sim] [--semantica-tempo 4000]  (as palavras não lidas consultadas na rede: data/semantica/fontes.txt)\n"
            "        [--gesto \"0:0.2,0.6:1,1:0.3\"]  (a curva de intensidade do autor: posição:intensidade, de 0 a 1)\n"
            "        [--dialogo sim]  (a resposta escuta nas frases da referência e responde nas pausas)\n"
            "        [--master nao]  (sem o barramento de master: cola, −16 LUFS, −1 dBTP)\n"
            "        [--linhas 200,2000] [--linhas-modos seguir,transfigurar,contrapor] [--linhas-livres sim] [--linhas-filtro nao]\n"
            "                                 (uma resposta autônoma por região de frequência; docs/LINHAS.md)\n"
              "  trioio_biblioteca escutar <dados> <der-…> aceitar|rejeitar|transformar [--nota \"…\"]\n"
              "        [--trecho 0:00-1:00|1:41-fim | --secao 3|A]  (julga só um trecho; docs/ESCUTA.md)\n"
              "  trioio_biblioteca escuta <dados>\n"
              "  trioio_biblioteca ler <dados> <id> [--forma contraste|repeticao] [--andamento metade|dobro] [--linhas 200,2000]\n"
              "        (a leitura: pulso, tonalidade, acordes, seções; docs/LEITURA_E_GABARITO.md)\n"
              "  trioio_biblioteca gabarito <dados> <id>   (cria <dados>/gabaritos/<id>.txt para o autor preencher)\n"
              "  trioio_biblioteca avaliar <dados> <id> [--gabarito ARQUIVO] [--forma …] [--andamento …]\n"
              "  trioio_biblioteca autoavaliar <dados> <id> [--seeds N] [--modos seguir,contrapor,…]\n"
              "        (o Trioio compõe sobre <id>, sintetiza, escuta o próprio áudio e compara com a partitura)\n"
              "  trioio_biblioteca estabilidade <dados> <id> [--semitons -2,-1,1,2]\n"
              "        (reamostra a gravação por 2^(k/12): a leitura deve se transpor junto)\n"
              "\n"
              "origem:     authorial performerCapture otherInstrument aiGenerated licensed publicDomain\n"
              "fundamento: ownWork license publicDomain writtenPermission serviceTerms\n"
              "usos:       listen analyze learn derive publishDerivative (cada um traz os anteriores)\n"
              "modo:       seguir transfigurar contrapor interromper");
    return 2;
}

// --chave valor, a partir de argv[from]
bool opcoes(int argc, char** argv, int from, std::map<std::string, std::string>& out)
{
    for (int i = from; i < argc; i += 2)
    {
        const std::string k = argv[i];
        if (k.rfind("--", 0) != 0 || i + 1 >= argc)
        {
            std::printf("opção inválida ou sem valor: %s\n", k.c_str());
            return false;
        }
        out[k.substr(2)] = argv[i + 1];
    }
    return true;
}

std::string relogio(double t)
{
    char b[32];
    std::snprintf(b, sizeof b, "%d:%04.1f", static_cast<int>(t) / 60, std::fmod(t, 60.0));
    return b;
}

std::string gabaritoPath(const std::string& dados, const std::string& id)
{
    return juce::File(juce::String::fromUTF8(dados.c_str())).getChildFile("gabaritos")
        .getChildFile(juce::String::fromUTF8(id.c_str()) + ".txt").getFullPathName().toStdString();
}

int lerLinhas(const std::string& dados, const std::string& id, const trioio::composition::Reading& inteira,
              const std::string& cortesTexto);

void imprimirLeitura(const trioio::composition::Reading& r)
{
    using namespace trioio::composition;
    std::printf("pulso       %.1f BPM · confiança %.2f · %d tempos por compasso\n", r.pulse.bpm, r.pulse.confidence,
                r.pulse.beatsPerBar);
    if (r.pulse.period > 0.0)
        std::printf("tempo forte o 1 em %.2f s (%d tempo(s) depois da fase) · confiança %.2f\n", r.downbeat.time,
                    r.downbeat.offsetBeats, r.downbeat.confidence);
    std::printf("tonalidade  %s (%.2f) · depois %s (%.2f)\n", r.key.name().c_str(), r.key.correlation,
                Key { r.key.secondTonic, r.key.secondMinor, 0.0f, -1, false, 0.0f }.name().c_str(),
                r.key.secondCorrelation);
    std::printf("            (pelo croma bruto: %s, %.2f)\n", r.chromaKey.name().c_str(), r.chromaKey.correlation);
    std::printf("altura      %.0f %% dos quadros com som têm altura definida\n", 100.0 * r.pitchedShare);
    if (r.melodyP50 > 0.0f)
        std::printf("predominante altura mais saliente (melodia na mistura): MIDI %.0f · %.0f · %.0f (p10 · mediana · p90)\n",
                    r.melodyP10, r.melodyP50, r.melodyP90);
    std::printf("ataques     %.0f %% com altura para o compositor (só pelo YIN: %.0f %%)\n", 100.0 * r.onsetsPitched,
                100.0 * r.onsetsPitchedYin);
    std::printf("grave       coincide com a nota mais forte em %.0f %% dos quadros com grave\n", 100.0 * r.bassAgreement);
    std::printf("acordes     %zu trechos", r.chords.size());
    std::string last;
    int shown = 0;
    for (const auto& c : r.chords)
    {
        const auto n = c.chord.name();
        if (n == last)
            continue;
        last = n;
        if (shown++ % 8 == 0)
            std::printf("\n            ");
        std::printf("%s %s  ", relogio(c.start).c_str(), n.c_str());
    }
    std::printf("\nseções      %zu:", r.boundaries.size() + 1);
    std::printf(" 0:00.0%s", r.labels.empty() ? "" : " A");
    for (std::size_t i = 0; i < r.boundaries.size(); ++i)
        std::printf(" · %s%s%c", relogio(r.boundaries[i]).c_str(), r.labels.empty() ? "" : " ",
                    r.labels.empty() ? ' ' : static_cast<char>('A' + r.labels[i + 1] % 26));
    std::printf("\n");
}

// As escalas de LINGUAGENS do arquivo do Trioio: fora os sistemas de
// afinação e as abstrações (MOS, EDO, temperamentos, conjuntos de Forte,
// séries, entonação justa, Euler-Fokker, Wilson, não oitavantes), que vestem
// qualquer campo sem dizer de que música ele é. Lido uma vez.
const std::vector<trioio::composition::ArchiveScale>& arquivoDeLinguagens()
{
    static const std::vector<trioio::composition::ArchiveScale> arquivo = [] {
        std::vector<trioio::composition::ArchiveScale> out;
        const char* env = std::getenv("TRIOIO_ESCALAS");
        const juce::File arq = env && *env ? juce::File(juce::CharPointer_UTF8(env))
                                           : trioio::library::trioioDataFile("escalas/arquivo_trioio.txt");
        if (!arq.existsAsFile())
            return out;
        std::vector<std::string> erros;
        for (auto& sc : trioio::composition::parseScaleArchive(arq.loadFileAsString().toStdString(), erros))
            if (trioio::composition::isLanguageScale(sc))
                out.push_back(std::move(sc));
        return out;
    }();
    return arquivo;
}

// O teste por transformação (docs/LEITURA_E_GABARITO.md): reamostra a
// gravação real por 2^(k/12) e confere se a leitura se transforma junto.
int estabilidade(const std::string& dados, const std::string& id, const trioio::library::AnalysisResult& heard,
                 std::map<std::string, std::string>& o)
{
    using namespace trioio::composition;
    std::vector<int> ks;
    const auto lista = juce::StringArray::fromTokens(juce::String::fromUTF8(
        (o["semitons"].empty() ? std::string("-2,-1,1,2") : o["semitons"]).c_str()), ",", "");
    for (const auto& x : lista)
        ks.push_back(x.trim().getIntValue());
    std::vector<float> mono;
    double sr = 0.0;
    std::vector<std::string> problemas;
    if (!readReferenceMono(dados, id, mono, sr, problemas))
    {
        for (const auto& p : problemas)
            std::printf("%s\n", p.c_str());
        return 1;
    }
    const auto base = readReference(heard.frames, heard.onsets, heard.duration);
    const auto& arquivo = arquivoDeLinguagens();
    const auto lingua = [&](const Reading& r) {
        const auto top = arquivo.empty() ? std::vector<RankedScale> {} : rankArchive(arquivo, r.structuralField, 1, 5, 9);
        return top.empty() ? RankedScale {} : top.front();
    };
    const auto top0 = arquivo.empty() ? std::vector<RankedScale> {} : rankArchive(arquivo, base.structuralField, 3, 5, 9);
    std::printf("\n%5s │ %-17s │ %-12s │ %7s │ %7s │ %7s │ %s\n", "k", "andamento", "tonalidade", "acordes", "melodia",
                "seções", "linguagem");
    double sc = 0.0, sm = 0.0, sb = 0.0;
    int nk = 0, keyOk = 0, tempoOk = 0, linguaOk = 0;
    for (int k : ks)
    {
        const double rate = std::exp2(k / 12.0);
        std::vector<float> y;
        resampleSpeed(mono, rate, y);
        trioio::perception::Analyzer a(sr);
        a.push(y.data(), y.size());
        a.finish();
        const auto rt = readReference(a.frames(), a.onsets(), static_cast<double>(y.size()) / sr);
        const auto e = compareTransformed(base, heard.frames, rt, a.frames(), rate, k);
        const auto lt = lingua(rt);
        // a mesma linguagem = o mesmo CONJUNTO de notas transposto em k: o
        // Hijaz, o melakarta 14 e o modo andaluz empatam e trocam de lugar
        const auto conjunto = [](const RankedScale& rs, int shift) {
            std::array<bool, 12> on {};
            for (double c : rs.scale->stepsCents)
                on[static_cast<std::size_t>(((static_cast<int>(std::lround(c / 100.0)) + rs.tonic + shift) % 12 + 12) % 12)] = true;
            return on;
        };
        // estável se a primeira do transformado está entre as 3 primeiras do
        // original (empates próximos trocam de lugar)
        const bool lok = lt.scale && std::any_of(top0.begin(), top0.end(),
                                                 [&](const RankedScale& r0) { return conjunto(lt, 0) == conjunto(r0, k); });
        char tempo[40];
        std::snprintf(tempo, sizeof tempo, "×%.2f (%s)", e.tempoRatio, std::abs(e.tempoRatio - 1.0) <= 0.04 ? "ok" : "mudou");
        std::printf("%+5d │ %-17s │ %-12s │ %6.0f%% │ %6.0f%% │ %7.2f │ %s%s\n", k, tempo, e.keyOk ? "ok" : rt.key.name().c_str(),
                    100.0 * e.chordAgree, 100.0 * e.melodyAgree, e.boundaryF, lok ? "ok" : "mudou: ",
                    lok || !lt.scale ? "" : (lt.scale->name + " sobre " + chordFromIndex(lt.tonic * 3).name()).c_str());
        ++nk;
        keyOk += e.keyOk;
        tempoOk += std::abs(e.tempoRatio - 1.0) <= 0.04;
        linguaOk += lok;
        sc += e.chordAgree;
        sm += e.melodyAgree;
        sb += e.boundaryF;
    }
    std::printf("\nestabilidade em %d transformações: andamento %d/%d · tonalidade %d/%d · linguagem %d/%d · acordes %.0f %% · melodia %.0f %% · seções F %.2f\n",
                nk, tempoOk, nk, keyOk, nk, linguaOk, nk, 100.0 * sc / nk, 100.0 * sm / nk, sb / nk);
    return 0;
}

// O autogabarito (docs/LEITURA_E_GABARITO.md): compõe sobre a referência,
// sintetiza, escuta o próprio áudio e compara a leitura com a partitura.
// Nada é gravado.
int autoavaliar(const trioio::library::AnalysisResult& heard, std::map<std::string, std::string>& o)
{
    using namespace trioio::composition;
    const int seeds = o["seeds"].empty() ? 3 : std::max(1, std::atoi(o["seeds"].c_str()));
    std::vector<trioio::strategy::Mode> modes;
    const auto lista = juce::StringArray::fromTokens(juce::String::fromUTF8(
        (o["modos"].empty() ? std::string("seguir,transfigurar,contrapor,interromper") : o["modos"]).c_str()), ",", "");
    for (const auto& nome : lista)
    {
        trioio::strategy::Mode m {};
        if (!trioio::strategy::parseMode(nome.trim().toStdString(), m))
            return std::printf("modo desconhecido: %s\n", nome.toRawUTF8()), 2;
        modes.push_back(m);
    }
    ResponseRequest q;  // os padrões do responder
    // o 1 verdadeiro das peças: o compositor monta os compassos a partir da
    // fase do pulso lido na referência (Composer.cpp, rhythmGrid)
    const auto refPulse = trioio::perception::estimatePulse(heard.frames, heard.onsets, 0.0, heard.duration);
    int fortes = 0, fortesN = 0;
    std::printf("\n%-12s %4s │ %-14s │ %-10s │ %6s │ %-14s │ %8s │ %s\n", "modo", "seed", "andamento", "tonalidade",
                "acordes", "melodia (classe)", "seções F", "percussão");
    double sumChord = 0.0, sumKey = 0.0, sumMel = 0.0, sumMelC = 0.0, sumSec = 0.0, sumKeyFlux = 0.0, sumChordFlux = 0.0;
    int tempoOkFlux = 0;
    std::array<double, 7> expPitch {}, expClass {}, expBelow {};
    std::array<int, 7> expN {};
    int n = 0, tempoOk = 0, nMel = 0, nSec = 0;
    for (const auto mode : modes)
        for (int s = 1; s <= seeds; ++s)
        {
            Options opt { mode, static_cast<std::uint64_t>(s), q.pitch, q.time, q.unpredictability };
            const auto score = compose(heard.frames, heard.onsets, heard.duration, opt);
            std::vector<float> L, R;
            trioio::renderScore(score, 48000.0, L, R);
            for (std::size_t i = 0; i < L.size() && i < R.size(); ++i)
                L[i] = 0.5f * (L[i] + R[i]);
            trioio::perception::Analyzer a(48000.0);
            a.push(L.data(), L.size());
            a.finish();
            const auto r = readReference(a.frames(), a.onsets(), score.duration);
            const auto g = gabaritoFromScore(score);
            const auto e = evaluate(r, g);
            // a mesma peça lida com o envelope de pulso anterior (o fluxo)
            const auto rFlux = readReference(a.frames(), a.onsets(), score.duration, false, 1.0,
                                             trioio::perception::PulseEnvelope::flux);
            const auto eFlux = evaluate(rFlux, g);
            tempoOkFlux += eFlux.tempoVerdict == "certo";
            sumKeyFlux += eFlux.keyScore;
            sumChordFlux += eFlux.chordOverlap;
            const auto m = melodyAgainstScore(a.frames(), score);
            if (e.tempoVerdict == "certo" && refPulse.period > 0.0)
            {
                const auto db = trioio::perception::estimateDownbeat(a.frames(), r.pulse, 0.0, score.duration);
                const double bar = refPulse.period * refPulse.beatsPerBar;
                const double rr = std::fmod(std::fmod(db.time - refPulse.phase, bar) + bar, bar);
                fortes += r.pulse.beatsPerBar == refPulse.beatsPerBar && std::min(rr, bar - rr) < 0.07;
                ++fortesN;
            }
            if (!o["melodia-exp"].empty())
            {
                // as mesmas peças: o pico cru e a linha rastreada com vários parâmetros
                static const std::pair<double, double> par[] = { { -1.0, 0.0 }, { 0.08, 0.25 }, { 0.08, 0.0 }, { 0.15, 0.25 },
                                                                 { 0.08, 0.5 }, { 0.04, 0.25 }, { 0.15, 0.5 } };
                for (std::size_t v = 0; v < std::size(par); ++v)
                {
                    auto fr = a.frames();
                    if (par[v].first < 0.0)
                        for (auto& f : fr) { f.melodyHz = f.melodyCandHz[0]; f.melodySalience = f.melodyCandSalience[0]; }
                    else
                    {
                        std::vector<float> mh, ms;
                        trioio::perception::trackMelody(fr, mh, ms, { par[v].first, par[v].second });
                        for (std::size_t j = 0; j < fr.size(); ++j) { fr[j].melodyHz = mh[j]; fr[j].melodySalience = ms[j]; }
                    }
                    const auto mv = melodyAgainstScore(fr, score);
                    if (mv.frames > 0)
                    {
                        expPitch[v] += mv.pitch;
                        expClass[v] += mv.chroma;
                        expBelow[v] += mv.octaveBelow;
                        ++expN[v];
                    }
                }
            }
            char tempo[40];
            std::snprintf(tempo, sizeof tempo, "%.0f/%.0f %s", r.pulse.bpm, g.bpm, e.tempoVerdict.c_str());
            char mel[64];
            if (m.frames > 0)
                std::snprintf(mel, sizeof mel, "%3.0f %% (%3.0f %%) ↓%2.0f ↑%2.0f", 100.0 * m.pitch, 100.0 * m.chroma,
                              100.0 * m.octaveBelow, 100.0 * m.octaveAbove);
            else
                std::snprintf(mel, sizeof mel, "—");
            std::printf("%-12s %4d │ %-14s │ %-10s │ %5.0f%% │ %-16s │ %8.2f │ %.2f%s\n", trioio::strategy::modeName(mode), s,
                        tempo, e.keyRelation.c_str(), 100.0 * e.chordOverlap, mel, e.boundaryF, a.percussiveShare(),
                        a.harmonicChroma() ? " (HPSS)" : "");
            ++n;
            tempoOk += e.tempoVerdict == "certo";
            sumChord += e.chordOverlap;
            sumKey += e.keyScore;
            sumSec += e.boundaryF;
            nSec += e.hasSections ? 1 : 0;
            if (m.frames > 0)
            {
                sumMel += m.pitch;
                sumMelC += m.chroma;
                ++nMel;
            }
        }
    std::printf("\nmédia de %d peças: andamento certo %d/%d · tonalidade %.2f · acordes %.0f %% · melodia %.0f %% (classe %.0f %%) · seções F %.2f\n",
                n, tempoOk, n, sumKey / n, 100.0 * sumChord / n, nMel ? 100.0 * sumMel / nMel : 0.0,
                nMel ? 100.0 * sumMelC / nMel : 0.0, nSec ? sumSec / nSec : 0.0);
    if (!o["melodia-exp"].empty())
    {
        const char* nomes[] = { "pico cru", "linha 0,08/0,25 (padrão)", "linha 0,08/0", "linha 0,15/0,25", "linha 0,08/0,5",
                                "linha 0,04/0,25", "linha 0,15/0,5" };
        std::printf("melodia nas mesmas peças (salto por semitom / preferência por oitava acima):\n");
        for (std::size_t v = 0; v < 7; ++v)
            if (expN[v] > 0)
                std::printf("  %-26s altura %3.0f %% · classe %3.0f %% · oitava abaixo %3.0f %%\n", nomes[v],
                            100.0 * expPitch[v] / expN[v], 100.0 * expClass[v] / expN[v], 100.0 * expBelow[v] / expN[v]);
    }
    std::printf("tempo forte (o 1 do compasso), nas peças com andamento certo: %d/%d (o acaso: 1 em %d)\n", fortes, fortesN,
                std::max(1, refPulse.beatsPerBar));
    std::printf("as mesmas peças com o pulso pelo fluxo (antes da v7): andamento certo %d/%d · tonalidade %.2f · acordes %.0f %%\n",
                tempoOkFlux, n, sumKeyFlux / n, 100.0 * sumChordFlux / n);
    return 0;
}

int leitura(const std::string& cmd, int argc, char** argv)
{
    using namespace trioio::composition;
    const std::string dados = argv[2], id = argv[3];
    std::map<std::string, std::string> o;
    if (!opcoes(argc, argv, 4, o))
        return 2;
    if (cmd == "gabarito")
    {
        const juce::File f(juce::String::fromUTF8(gabaritoPath(dados, id).c_str()));
        if (f.existsAsFile())
            return std::printf("já existe: %s (não sobrescrevo)\n", f.getFullPathName().toRawUTF8()), 1;
        f.getParentDirectory().createDirectory();
        const std::string modelo =
            "# Gabarito do Trioio: o que VOCÊ ouve nesta referência (docs/LEITURA_E_GABARITO.md).\n"
            "# Preencha só o que souber; linhas vazias ou comentadas são ignoradas.\n"
            "# Instantes em segundos (83.4) ou minutos:segundos (1:23.4).\n"
            "referencia: " + id + "\n"
            "fonte: escuta de <nome>, <data>\n"
            "# andamento: 120             # o pulso em que você conta\n"
            "# batidas: 0.50 1.00 1.50    # ou os instantes dos tempos, batidos\n"
            "# compasso: 4\n"
            "# tonalidade: Mi maior        # ou Fá# menor, Gm, D…\n"
            "# acordes: 0:00 Mi, 0:04 Dó#m, 0:08 Lá   # cada um vale até o seguinte; N = sem acorde\n"
            "# secoes: 0:00 A, 0:30 B, 1:00 A   # as letras repetem o que volta\n"
            "# fim: 3:00\n";
        if (!f.replaceWithText(juce::String::fromUTF8(modelo.c_str()), false, false, "\n"))
            return std::printf("falha ao gravar %s\n", f.getFullPathName().toRawUTF8()), 1;
        std::printf("modelo criado: %s\n", f.getFullPathName().toRawUTF8());
        return 0;
    }
    const auto a = analyzeReference(dados, id);
    for (const auto& p : a.problems)
        std::printf("%s\n", p.c_str());
    if (!a.ok)
        return 1;
    if (cmd == "autoavaliar")
        return autoavaliar(a, o);
    if (cmd == "estabilidade")
        return estabilidade(dados, id, a, o);
    double scale = 1.0;
    if (o["andamento"] == "metade") scale = 0.5;
    else if (o["andamento"] == "dobro") scale = 2.0;
    else if (!o["andamento"].empty())
        return std::printf("--andamento: metade ou dobro\n"), 2;
    const bool rep = o["forma"] == "repeticao" || o["forma"] == "repetição";
    const auto r = readReference(a.frames, a.onsets, a.duration, rep, scale);
    std::printf("%s · %s · descritores v%d\n", id.c_str(), relogio(a.duration).c_str(), trioio::perception::kAnalyzerVersion);
    if (a.percussiveShare >= 0.0f)
        std::printf("percussão   %.0f %% da magnitude · croma %s\n", 100.0 * a.percussiveShare,
                    a.harmonicChroma ? "só do que se sustenta (HPSS)" : "do espectro inteiro (pouca percussão)");
    imprimirLeitura(r);
    {
        const auto& arquivo = arquivoDeLinguagens();
        if (!arquivo.empty())
        {
            std::printf("linguagem   (acordes + melodia da peça inteira; as %zu escalas de linguagens do arquivo, 5 a 9 graus)\n            campo:",
                        arquivo.size());
            for (int pc = 0; pc < 12; ++pc)
                if (r.structuralField[static_cast<std::size_t>(pc)] >= 0.02f)
                    std::printf(" %s %.0f%%", chordFromIndex(pc * 3).name().c_str(), 100.0 * r.structuralField[static_cast<std::size_t>(pc)]);
            std::printf("\n");
            for (const auto& rs : rankArchive(arquivo, r.structuralField, 5, 5, 9))
                std::printf("            %.2f  %s (%s) sobre %s\n", rs.score, rs.scale->name.c_str(), rs.scale->family.c_str(),
                            chordFromIndex(rs.tonic * 3).name().c_str());
        }
    }
    {
        // os ciclos rítmicos do arquivo (EXPERIMENTAL: 6 de 8 ciclos sintéticos em 1.º)
        const char* env = std::getenv("TRIOIO_RITMOS");
        const juce::File arq = env && *env ? juce::File(juce::CharPointer_UTF8(env))
                                           : trioio::library::trioioDataFile("ritmos/arquivo_trioio_ritmos.txt");
        if (arq.existsAsFile())
        {
            std::vector<std::string> erros;
            const auto ciclos = parseRhythmArchive(arq.loadFileAsString().toStdString(), erros);
            std::printf("ciclo       (experimental: os ataques dobrados sobre os %zu ciclos do arquivo)\n", ciclos.size());
            const auto top = rankCycles(ciclos, a.frames, a.onsets, r.pulse, 0.0, a.duration, 5);
            // os acertos sintéticos ficam em 0,65–0,84; abaixo de 0,6 nenhum
            // ciclo do arquivo veste a peça (uma canção não é um iqa' nem um tala)
            if (top.empty() || !top.front().clear())
            {
                std::printf("            nenhum ciclo claro do arquivo (o melhor: %s, %.2f; claro: ≥ 0,6, ou ≥ 0,4 nas claves)\n",
                            top.empty() ? "—" : top.front().cycle->name.c_str(), top.empty() ? 0.0 : top.front().score);
            }
            else
            for (const auto& rc : top)
                std::printf("            %.2f  %s (%s)%s\n", rc.score, rc.cycle->name.c_str(), rc.cycle->family.c_str(),
                            rc.levelScale < 0.0 ? (" · o ciclo em " + std::to_string(static_cast<int>(-rc.levelScale)) + " pulsos").c_str() : "");
        }
    }
    if (cmd == "ler")
        return o["linhas"].empty() ? 0 : lerLinhas(dados, id, r, o["linhas"]);
    const auto path = o["gabarito"].empty() ? gabaritoPath(dados, id) : o["gabarito"];
    const juce::File gf(juce::String::fromUTF8(path.c_str()));
    if (!gf.existsAsFile())
        return std::printf("sem gabarito em %s (crie com: trioio_biblioteca gabarito %s %s)\n", path.c_str(),
                           dados.c_str(), id.c_str()), 1;
    std::vector<std::string> erros;
    const auto g = parseGabarito(gf.loadFileAsString().toStdString(), erros);
    for (const auto& e : erros)
        std::printf("gabarito, %s\n", e.c_str());
    if (!g.reference.empty() && g.reference != id)
        std::printf("atenção: o gabarito é de %s, não de %s\n", g.reference.c_str(), id.c_str());
    const auto e = evaluate(r, g);
    std::printf("\navaliação contra %s%s\n", path.c_str(), g.source.empty() ? "" : (" (" + g.source + ")").c_str());
    if (e.tempoRatio > 0.0)
        std::printf("  andamento   lido %.1f · gabarito %.1f · %s (×%.2f)\n", r.pulse.bpm, g.bpm, e.tempoVerdict.c_str(),
                    e.tempoRatio);
    if (e.hasBeats)
        std::printf("  tempos      F %.2f (precisão %.2f, revocação %.2f; ±70 ms, depois de 5 s)\n", e.beatF,
                    e.beatPrecision, e.beatRecall);
    if (e.hasKey)
        std::printf("  tonalidade  %.1f — %s\n", e.keyScore, e.keyRelation.c_str());
    if (e.hasChords)
        std::printf("  acordes     %.0f %% do tempo com o mesmo acorde\n", 100.0 * e.chordOverlap);
    if (e.hasSections)
        std::printf("  seções      F %.2f nas fronteiras (precisão %.2f, revocação %.2f; ±3 s)\n", e.boundaryF,
                    e.boundaryPrecision, e.boundaryRecall);
    if (e.tempoRatio <= 0.0 && !e.hasBeats && !e.hasKey && !e.hasChords && !e.hasSections)
        std::printf("  o gabarito ainda não tem nada preenchido\n");
    return 0;
}

// As linhas por banda (docs/LINHAS.md, etapa 1): a referência dividida em
// regiões de frequência, e a escuta inteira em cada uma. Cada leitura traz o
// que VALE nela: acordes lidos só no agudo não se sustentam.
int lerLinhas(const std::string& dados, const std::string& id, const trioio::composition::Reading& inteira,
              const std::string& cortesTexto)
{
    using namespace trioio::composition;
    using namespace trioio::perception;
    std::vector<double> cortes;
    for (const auto& c : juce::StringArray::fromTokens(juce::String::fromUTF8(cortesTexto.c_str()), ",", ""))
        if (c.trim().getDoubleValue() > 0.0)
            cortes.push_back(c.trim().getDoubleValue());
    std::vector<float> mono;
    double sr = 0.0;
    std::vector<std::string> problemas;
    if (!readReferenceMono(dados, id, mono, sr, problemas))
    {
        for (const auto& pr : problemas)
            std::printf("%s\n", pr.c_str());
        return 1;
    }
    const double dur = static_cast<double>(mono.size()) / sr;
    auto linhas = splitBands(mono, sr, cortes);
    std::printf("\nlinhas      %zu, cortes em", linhas.size());
    for (std::size_t k = 0; k + 1 < linhas.size(); ++k)
        std::printf(" %.0f Hz", linhas[k].highHz);
    std::printf(" (Linkwitz-Riley de 4.ª ordem, fase zero; a soma das linhas é a referência)\n");
    const auto& arquivo = arquivoDeLinguagens();
    const auto nota = [](float midi) {
        if (midi <= 0.0f)
            return std::string("—");
        const int m = static_cast<int>(std::lround(midi));
        return chordFromIndex(((m % 12) + 12) % 12 * 3).name() + std::to_string(m / 12 - 1);
    };
    for (auto& linha : linhas)
    {
        // ao nível da referência, para os limiares de silêncio valerem igual
        const float g = levelMatchGain(linha, mono);
        for (auto& v : linha.samples)
            v *= g;
        Analyzer a(sr);
        a.push(linha.samples.data(), linha.samples.size());
        a.finish();
        std::vector<float>().swap(linha.samples);  // a memória volta antes da próxima
        const auto r = readReference(a.frames(), a.onsets(), dur);
        const double dens = static_cast<double>(a.onsets().size()) / std::max(1.0, dur);
        const double hi = linha.highHz > 0.0 ? linha.highHz : sr * 0.5;
        // o que vale nesta linha: as regras de core/Lines.h (as mesmas da composição)
        const auto v = assessLine(linha.lowHz, linha.highHz, sr, r, inteira, dens);
        const bool pulso = v.pulse, concorda = v.agrees, melodia = v.melody, grave = v.bass, acordes = v.chords;
        std::printf("\n── %s · %.0f %% da energia (%.1f dB)\n", linha.name().c_str(), 100.0 * linha.energyShare,
                    10.0 * std::log10(std::max(1e-9, linha.energyShare)));
        std::printf("   ataques   %.1f por segundo\n", dens);
        std::printf("   pulso     %.1f BPM · confiança %.2f · %d tempos%s%s\n", r.pulse.bpm, r.pulse.confidence, r.pulse.beatsPerBar,
                    inteira.pulse.bpm > 0.0 ? (" · ×" + juce::String(r.pulse.bpm / inteira.pulse.bpm, 2).toStdString() + " da peça").c_str() : "",
                    pulso ? "" : concorda ? "  [fraco, mas concorda com a peça]" : "  [não vale aqui]");
        if (!grave)
            std::printf("   melodia   %.0f %% dos quadros com altura · predominante %s (mediana)%s\n", 100.0 * r.pitchedShare,
                        nota(r.melodyP50).c_str(), melodia ? "" : "  [não vale aqui]");
        std::string seq, last;
        int shown = 0;
        for (const auto& c : r.chords)
            if (c.chord.name() != last && shown < 10)
            {
                last = c.chord.name();
                seq += (grave ? last.substr(0, last.find_first_of("m°+7")) : last) + " ";
                ++shown;
            }
        if (grave)
            std::printf("   fundamentais %zu trechos: %s… (o croma abaixo de %.0f Hz é o baixo, não o acorde)\n", r.chords.size(),
                        seq.c_str(), hi);
        else
            std::printf("   acordes   %zu trechos: %s…%s\n", r.chords.size(), seq.c_str(), acordes ? "" : "  [não vale aqui]");
        if (acordes || melodia)
        {
            std::printf("   tonalidade %s (%.2f)%s\n", r.key.name().c_str(), r.key.correlation,
                        r.key.name() == inteira.key.name() ? " · a mesma da peça" : "");
            if (!arquivo.empty())
            {
                const auto top = rankArchive(arquivo, r.structuralField, 1, 5, 9);
                if (!top.empty())
                    std::printf("   linguagem %.2f  %s sobre %s\n", top.front().score, top.front().scale->name.c_str(),
                                chordFromIndex(top.front().tonic * 3).name().c_str());
            }
        }
    }
    return 0;
}

int relatar(const DeclarationResult& r)
{
    for (const auto& p : r.problems)
        std::printf("falta/erro: %s\n", p.c_str());
    std::puts(r.written ? "gravado" : "NADA GRAVADO");
    return r.written ? 0 : 1;
}

int autorizar(const std::string& dados, const std::string& id, std::map<std::string, std::string> o)
{
    AuthorizationRequest q;
    q.id = id;
    if (!parseOrigin(o["origem"], q.origin))
        return std::printf("--origem desconhecida: \"%s\"\n", o["origem"].c_str()), 2;
    if (!parseBasis(o["fundamento"], q.basis))
        return std::printf("--fundamento desconhecido: \"%s\"\n", o["fundamento"].c_str()), 2;
    const auto usos = juce::StringArray::fromTokens(juce::String::fromUTF8(o["usos"].c_str()), ",", "");
    for (const auto& nome : usos)
    {
        Use u {};
        if (!parseUse(nome.trim().toStdString(), u))
            return std::printf("uso desconhecido: \"%s\"\n", nome.toRawUTF8()), 2;
        q.scope = q.scope | u;
    }
    q.declaredBy = o["por"];
    q.license = o["licenca"];
    q.evidence = o["evidencia"];
    q.toolName = o["ferramenta"];
    q.toolVersion = o["versao"];
    q.restrictions = o["restricoes"];
    return relatar(authorize(dados, q));
}

int varrer(const std::string& lib, const std::string& dados)
{
    const auto r = scanLibrary(lib, dados);
    std::printf("novos (pendentes): %d\n"
                "sem mudança:       %d\n"
                "mudaram de lugar:  %d\n"
                "cópias idênticas:  %d\n"
                "reencontrados:     %d\n"
                "ausentes:          %d\n"
                "substituídos:      %d\n",
                r.registered, r.unchanged, r.relocated, r.duplicates, r.reappeared, r.missing, r.replaced);
    for (const auto& s : r.skipped)
        std::printf("pulado: %s\n", s.c_str());
    for (const auto& e : r.errors)
        std::printf("ERRO: %s\n", e.c_str());
    std::printf("manifestos em %s\n", referenceManifestDir(dados).c_str());
    return r.errors.empty() ? 0 : 1;
}

int listar(const std::string& dados)
{
    const juce::File dir(juce::String::fromUTF8(referenceManifestDir(dados).c_str()));
    auto arquivos = dir.findChildFiles(juce::File::findFiles, false, "*.json");
    arquivos.sort();
    int n = 0;
    for (const auto& f : arquivos)
    {
        const auto lido = fromJson(f.loadFileAsString().toStdString());
        if (!lido.manifest)
        {
            std::printf("%-22s  ILEGÍVEL  %s\n", f.getFileName().toRawUTF8(),
                        lido.errors.empty() ? "" : lido.errors.front().c_str());
            continue;
        }
        const auto& m = *lido.manifest;
        std::printf("%-22s  %-12s  %-5s  %7.1f s  %s\n", m.id.c_str(), estado(m), material(m.material),
                    m.identity.durationSeconds, m.identity.canonicalPath.c_str());
        ++n;
    }
    std::printf("%d manifesto(s)\n", n);
    return 0;
}
// O padrão de diretórios (2026-10-16; AUDIOS_RASGO/Trioio/LEIA-ME.md): um
// nome de arquivo SEM pasta vai para $TRIOIO_ESCUTAS/AAAA-MM-DD/, quando a
// variável existe — as audições não se espalham pelo HD. Com pasta, vale o
// caminho dado.
juce::File escutaFile(const char* arg)
{
    const auto name = juce::String::fromUTF8(arg);
    const auto root = juce::SystemStats::getEnvironmentVariable("TRIOIO_ESCUTAS", {});
    if (!name.containsChar('/') && root.isNotEmpty())
    {
        const auto day = juce::Time::getCurrentTime().formatted("%Y-%m-%d");
        const auto dir = juce::File(root).getChildFile(day);
        dir.createDirectory();
        return dir.getChildFile(name);
    }
    return juce::File::getCurrentWorkingDirectory().getChildFile(name);
}

// A audição dos timbres (docs/MAQUINA_DE_TIMBRES.md): a mesma frase em cada
// família, uma depois da outra — grave, médio e agudo, notas curtas e uma
// longa —, com o índice dos tempos na saída. Sem argumentos de família, todas.
int timbres(int argc, char** argv)
{
    using namespace trioio::composition;
    std::vector<Timbre> fam;
    // --notas 44,52,60: em vez da frase, cada nota por 1 s (para medir a extensão)
    std::vector<float> notas;
    for (int a = 3; a + 1 < argc; ++a)
        if (std::string(argv[a]) == "--notas")
            for (const auto& tok : juce::StringArray::fromTokens(argv[a + 1], ",", ""))
                notas.push_back(tok.getFloatValue());
    for (int a = 3; a < argc; ++a)
        for (int k = 0; k < kTimbreCount; ++k)
            if (std::string(timbreName(static_cast<Timbre>(k))) == argv[a])
                fam.push_back(static_cast<Timbre>(k));
    if (fam.empty())
        for (int k = 0; k < kTimbreCount; ++k)
            fam.push_back(static_cast<Timbre>(k));
    // a frase: subida em ré dórico no médio, um salto ao agudo, a descida ao
    // grave e uma nota longa
    std::vector<std::pair<float, double>> phrase { { 62.f, 0.3 }, { 64.f, 0.3 }, { 65.f, 0.3 }, { 69.f, 0.6 }, { 74.f, 0.3 },
                                                          { 72.f, 0.3 }, { 69.f, 0.6 }, { 50.f, 0.45 }, { 45.f, 0.45 }, { 57.f, 2.2 } };
    if (!notas.empty())
    {
        phrase.clear();
        for (float m : notas)
            phrase.push_back({ m, 1.2 });
    }
    Score s;
    double t = 0.5;
    for (auto f : fam)
    {
        std::printf("%6.1f s  %s\n", t, timbreName(f));
        for (const auto& [pitch, dur] : phrase)
        {
            Note n;
            n.time = t;
            n.duration = dur * 0.92;
            n.pitch = pitch;
            n.velocity = dur > 1.0 ? 0.55f : 0.75f;
            n.timbre = f;
            s.notes.push_back(n);
            t += dur;
        }
        t += 1.6;
    }
    s.duration = t;
    std::vector<float> L, R;
    trioio::renderScore(s, 48000.0, L, R);
    juce::AudioBuffer<float> audio(2, static_cast<int>(L.size()));
    audio.copyFrom(0, 0, L.data(), static_cast<int>(L.size()));
    audio.copyFrom(1, 0, R.data(), static_cast<int>(R.size()));
    const juce::File wav = escutaFile(argv[2]);
    wav.getParentDirectory().createDirectory();
    wav.deleteFile();
    juce::WavAudioFormat format;
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream>(wav);
    auto writer = format.createWriterFor(stream, juce::AudioFormatWriterOptions {}.withSampleRate(48000.0).withNumChannels(2).withBitsPerSample(24));
    if (!writer || !writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()))
    {
        std::printf("falha ao gravar %s\n", argv[2]);
        return 1;
    }
    std::printf("%s (%.0f s)\n", wav.getFullPathName().toRawUTF8(), t);
    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    if (argc >= 5 && std::string(argv[1]) == "versoes")
    {
        // a comparação entre versões (core/Versions.h, docs/VERSOES.md)
        const std::string dados = argv[2];
        std::vector<trioio::composition::VersionReading> vs;
        for (int i = 3; i < argc; ++i)
        {
            const auto a = trioio::library::analyzeReference(dados, argv[i]);
            if (!a.ok)
            {
                std::printf("%s: análise indisponível (%s)\n", argv[i], a.problems.empty() ? "?" : a.problems.front().c_str());
                return 1;
            }
            const auto r = trioio::composition::readReference(a.frames, a.onsets, a.duration);
            vs.push_back(trioio::composition::readVersion(argv[i], a.frames, a.onsets, a.duration, r));
            const auto& v = vs.back();
            std::printf("%s: %.0f s · %.0f BPM (confiança %.2f) · %d seções · %s\n", v.id.c_str(), v.duration, v.bpm,
                        static_cast<double>(v.pulseConfidence), v.sections, trioio::composition::intentFieldsText(v.intent).c_str());
        }
        const auto c = trioio::composition::compareVersions(vs);
        std::printf("\no que não muda:\n");
        for (const auto& s : c.invariants)
            std::printf("  · %s\n", s.c_str());
        std::printf("o que diverge:\n");
        for (const auto& s : c.divergences)
            std::printf("  · %s\n", s.c_str());
        std::printf("o que o Trioio pode fazer:\n");
        for (const auto& s : c.operations)
            std::printf("  · %s\n", s.c_str());
        return 0;
    }
    if (argc >= 4 && std::string(argv[1]) == "semantica")
    {
        // a ponte semântica sozinha (SemanticBridge.h): sem render
        std::vector<std::string> errors;
        const auto srcFile = trioio::library::trioioDataFile("semantica/fontes.txt");
        const auto sources = trioio::library::parseSemanticSources(srcFile.loadFileAsString().toStdString(), errors);
        for (const auto& e : errors)
            std::printf("fontes.txt: %s\n", e.c_str());
        std::printf("fontes: %d (%s)\n", static_cast<int>(sources.size()), srcFile.getFullPathName().toRawUTF8());
        std::vector<std::string> words;
        for (const auto& w : juce::StringArray::fromTokens(juce::String::fromUTF8(argv[3]), ",", ""))
            if (w.trim().isNotEmpty())
                words.push_back(w.trim().toStdString());
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        const auto res = trioio::library::lookupSemantic(argv[2], words, sources, argc >= 5 ? std::atoi(argv[4]) : 6000);
        std::printf("tempo total: %.0f ms\n", juce::Time::getMillisecondCounterHiRes() - t0);
        for (const auto& r : res)
        {
            std::printf("\n%s → %s  [%s%s, %d ms]\n", r.word.c_str(), r.english.c_str(), r.cached ? "memória; " : "", r.via.c_str(), r.millis);
            if (!r.ok)
            {
                std::printf("  sem resposta: %s\n", r.error.c_str());
                continue;
            }
            std::string top;
            for (size_t i = 0; i < std::min<size_t>(12, r.associations.size()); ++i)
                top += (i ? ", " : "") + r.associations[i].first;
            std::printf("  associações: %s…\n", top.c_str());
            std::string why;
            const auto f = trioio::composition::intentFromAssociations(r.associations, &why);
            std::printf("  deram campo: %s\n  o cálculo: %s\n", why.empty() ? "(nenhuma)" : why.c_str(),
                        trioio::composition::intentFieldsText(f).c_str());
        }
        return 0;
    }
    if (argc >= 4 && std::string(argv[1]) == "misturar-cadencias")
    {
        // a mistura conceitual de duas cadências do arquivo (HarmonyWriting.h)
        std::vector<std::string> erros;
        const auto cs = trioio::composition::parseCadenceArchive(
            trioio::library::trioioDataFile("harmonia/arquivo_trioio_cadencias.txt").loadFileAsString().toStdString(), erros);
        const auto find = [&](const std::string& n) -> const trioio::composition::CadenceEntry* {
            for (const auto& c : cs)
                if (c.name == n)
                    return &c;
            for (const auto& c : cs)
                if (c.name.find(n) != std::string::npos)
                    return &c;
            return nullptr;
        };
        const auto* a = find(argv[2]);
        const auto* b = find(argv[3]);
        if (!a || !b)
            return std::printf("cadência não encontrada (veja trioio_biblioteca cadencias)\n"), 1;
        std::printf("%s (%s) × %s (%s)\n", a->name.c_str(), a->degrees.c_str(), b->name.c_str(), b->degrees.c_str());
        if (a->chords.back().root != 0 || b->chords.back().root != 0)
            return std::printf("  a mistura pede cadências que chegam à tônica (o final fixo)\n"), 0;
        for (const auto& x : trioio::composition::blendCadences(*a, *b, 8))
            std::printf("  %-10s → %-6s  %5.1f   %s\n", x.penultimate.name().c_str(), x.final.name().c_str(), x.score, x.kept.c_str());
        return 0;
    }
    if (argc >= 4 && std::string(argv[1]) == "prompt")
    {
        // o prompt e a intenção de uma referência (Prompts.h, docs/PROMPTS_E_INTENCOES.md)
        const std::string dados = argv[2], ref = argv[3];
        const auto now = juce::Time::getCurrentTime().toISO8601(true).toStdString();
        std::vector<std::string> problemas;
        const std::string a4 = argc >= 5 ? argv[4] : "--mostrar";
        bool ok = true;
        if (a4 == "--apagar")
            ok = trioio::library::removePrompt(dados, ref, problemas);
        else if (a4 == "--revisar" && argc >= 6)
            ok = trioio::library::reviseIntent(dados, ref, argv[5], now, problemas);
        else if (a4 != "--mostrar")
        {
            std::string fonte = "autor";
            for (int i = 5; i + 1 < argc; i += 2)
                if (std::string(argv[i]) == "--fonte")
                    fonte = argv[i + 1];
            ok = trioio::library::providePrompt(dados, ref, a4, fonte, now, problemas);
        }
        for (const auto& p : problemas)
            std::printf("problema: %s\n", p.c_str());
        if (!ok)
            return 1;
        trioio::library::PromptRecord r;
        if (!trioio::library::loadPrompt(dados, ref, r))
        {
            std::printf("%s: sem prompt nem intenção declarada\n", ref.c_str());
            return 0;
        }
        std::printf("prompt original: %s\n", r.hasOriginal ? ("\"" + r.original + "\" (" + r.source + ", " + r.providedAt + ")").c_str()
                                                             : "(não fornecido)");
        std::printf("intenção declarada%s: %s\n", r.declared.revised ? " (revisada)" : " (tradução)",
                    trioio::composition::intentFieldsText(r.declared.fields).c_str());
        for (const auto& m : r.declared.matches)
            std::printf("    \"%s\" → %s = %s\n", m.word.c_str(), m.field.c_str(), m.value.c_str());
        if (!r.declared.unread.empty())
        {
            std::printf("    não lidas:");
            for (const auto& w : r.declared.unread)
                std::printf(" %s", w.c_str());
            std::printf("\n");
        }
        if (r.hasInferred)
        {
            std::printf("intenção ouvida: %s\n", trioio::composition::intentFieldsText(r.inferred).c_str());
            for (const auto& g : trioio::composition::compareIntent(r.declared.fields, r.inferred))
                std::printf("    %s\n", g.text.c_str());
        }
        else
            std::printf("intenção ouvida: (ainda não; vem com a próxima resposta)\n");
        for (const auto& h : r.history)
            std::printf("  %s\n", h.c_str());
        return 0;
    }
    if (argc >= 3 && std::string(argv[1]) == "limpar-cache")
    {
        // o cache de análise é reconstruível: os órfãos (ou tudo, com --tudo sim)
        const std::string dados = argv[2];
        bool executar = false, tudo = false;
        for (int i = 3; i + 1 < argc; i += 2)
        {
            if (std::string(argv[i]) == "--executar") executar = std::string(argv[i + 1]) == "sim";
            if (std::string(argv[i]) == "--tudo") tudo = std::string(argv[i + 1]) == "sim";
        }
        double mb = 0.0;
        int n = 0;
        for (const auto& c : trioio::library::cacheStatus(dados))
        {
            const bool vai = tudo || c.orphan;
            std::printf("%-72s %6.1f MB  %s\n", c.file.c_str(), c.megabytes, c.orphan ? "órfão" : (tudo ? "em uso (sai com --tudo)" : "em uso (fica)"));
            if (vai)
            {
                mb += c.megabytes;
                ++n;
            }
        }
        if (!executar)
        {
            std::printf("%d cache(s), %.1f MB, sairiam (reconstruíveis: a próxima análise refaz). Para fazer: --executar sim\n", n, mb);
            return 0;
        }
        std::printf("%zu cache(s) apagados\n", trioio::library::removeCaches(dados, tudo).size());
        return 0;
    }
    if (argc >= 3 && (std::string(argv[1]) == "limpar" || std::string(argv[1]) == "esvaziar-lixeira"))
    {
        // a retenção (Retention.h): sem --executar sim, só mostra o que faria
        const std::string cmd = argv[1], dados = argv[2];
        int manter = 3, dias = 7;
        bool executar = false;
        for (int i = 3; i + 1 < argc; i += 2)
        {
            const std::string k = argv[i], v = argv[i + 1];
            if (k == "--manter") manter = std::max(0, std::atoi(v.c_str()));
            else if (k == "--dias") dias = std::max(0, std::atoi(v.c_str()));
            else if (k == "--executar") executar = v == "sim";
        }
        const auto today = juce::Time::getCurrentTime().formatted("%Y-%m-%d").toStdString();
        if (cmd == "limpar")
        {
            double livre = 0.0;
            int n = 0;
            for (const auto& s : trioio::library::sessionStatus(dados, manter))
            {
                const char* tag = s.judged ? "julgada (fica)" : s.recent ? "recente (fica)" : "teste (vai para a lixeira)";
                std::printf("%-44s %8.0f MB  %s\n", s.id.c_str(), s.megabytes, tag);
                if (!s.judged && !s.recent)
                {
                    livre += s.megabytes;
                    ++n;
                }
            }
            if (!executar)
            {
                std::printf("%d sessão(ões), %.0f MB, iriam para %s/lixeira/%s (reversível). Para fazer: --executar sim\n", n, livre,
                            dados.c_str(), today.c_str());
                return 0;
            }
            const auto moved = trioio::library::moveDisposableToTrash(dados, today, manter);
            std::printf("%zu sessão(ões) movidas para %s/lixeira/%s. Para desfazer, mova de volta para sessoes/.\n", moved.size(),
                        dados.c_str(), today.c_str());
            return 0;
        }
        const juce::File trash = juce::File(juce::String::fromUTF8(dados.c_str())).getChildFile("lixeira");
        if (!executar)
        {
            for (const auto& d : trash.findChildFiles(juce::File::findDirectories, false, "????" "-" "??" "-" "??"))
                std::printf("%s\n", d.getFileName().toRawUTF8());
            std::printf("esvaziar apagaria as pastas acima com mais de %d dias (antes de %s). Para fazer: --executar sim\n", dias, today.c_str());
            return 0;
        }
        const auto gone = trioio::library::emptyTrash(dados, today, dias);
        std::printf("%zu pasta(s) da lixeira apagadas\n", gone.size());
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "cadencias")
    {
        // o arquivo de cadências da máquina de harmonia; filtra pela família
        // ou pela linguagem, se dada
        const juce::File arq = trioio::library::trioioDataFile("harmonia/arquivo_trioio_cadencias.txt");
        std::vector<std::string> erros;
        const auto cs = trioio::composition::parseCadenceArchive(arq.loadFileAsString().toStdString(), erros);
        for (const auto& e : erros)
            std::printf("aviso: %s\n", e.c_str());
        const std::string filtro = argc >= 3 ? argv[2] : "";
        int n = 0;
        for (const auto& c : cs)
        {
            if (!filtro.empty() && c.family != filtro && c.language != filtro)
                continue;
            ++n;
            std::printf("%-38s %-20s %s\n    %s\n    fonte: %s\n", c.name.c_str(), c.family.c_str(), c.degrees.c_str(), c.effect.c_str(),
                        c.source.c_str());
        }
        std::printf("%d cadência(s) de %zu no arquivo\n", n, cs.size());
        return erros.empty() ? 0 : 1;
    }
    if (argc >= 3 && std::string(argv[1]) == "timbres")
        return timbres(argc, argv);
    if (argc == 3 && std::string(argv[1]) == "efeitos")
    {
        // a audição dos efeitos de cinema (docs/MAQUINA_DE_TIMBRES.md §3.7): cada um por 4 s
        using namespace trioio::composition;
        Score s;
        double t = 0.5;
        const std::pair<GestureKind, const char*> fx[] = { { GestureKind::wind, "vento" }, { GestureKind::rain, "chuva" },
                                                           { GestureKind::thunder, "trovão" }, { GestureKind::water, "água" },
                                                           { GestureKind::whoosh, "whoosh" }, { GestureKind::riser, "riser" },
                                                           { GestureKind::impact, "impacto" }, { GestureKind::braam, "braam" },
                                                           { GestureKind::fire, "fogo" }, { GestureKind::crowd, "multidão" },
                                                           { GestureKind::city, "cidade" } };
        for (const auto& [k, name] : fx)
        {
            std::printf("%6.1f s  %s\n", t, name);
            Gesture g;
            g.kind = k;
            g.time = t;
            g.duration = k == GestureKind::whoosh ? 1.5 : k == GestureKind::city || k == GestureKind::crowd ? 9.0 : 4.0;
            g.level = 0.5f;
            g.lo = k == GestureKind::braam ? 31.0f : 48.0f;
            g.density = k == GestureKind::rain ? 600.0f : 90.0f;
            g.pan = -0.9f;
            g.panEnd = 0.9f;
            g.seed = static_cast<std::uint64_t>(t * 1000.0) + 1;
            s.gestures.push_back(g);
            t += g.duration + 1.0;
        }
        s.duration = t;
        std::vector<float> L, R;
        trioio::renderScore(s, 48000.0, L, R);
        juce::AudioBuffer<float> audio(2, static_cast<int>(L.size()));
        audio.copyFrom(0, 0, L.data(), static_cast<int>(L.size()));
        audio.copyFrom(1, 0, R.data(), static_cast<int>(R.size()));
        const juce::File wav = escutaFile(argv[2]);
        wav.deleteFile();
        juce::WavAudioFormat format;
        std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream>(wav);
        auto writer = format.createWriterFor(stream, juce::AudioFormatWriterOptions {}.withSampleRate(48000.0).withNumChannels(2).withBitsPerSample(24));
        if (!writer || !writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()))
            return std::printf("falha ao gravar %s\n", argv[2]), 1;
        std::printf("%s (%.0f s)\n", wav.getFullPathName().toRawUTF8(), t);
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "varrer")
        return varrer(argv[2], argv[3]);
    if (argc == 3 && std::string(argv[1]) == "listar")
        return listar(argv[2]);
    if (argc == 4 && std::string(argv[1]) == "analisar")
    {
        const auto r = analyzeReference(argv[2], argv[3]);
        for (const auto& p : r.problems)
            std::printf("%s\n", p.c_str());
        if (!r.ok)
            return 1;
        std::fputs(formatReport(argv[3], r).c_str(), stdout);
        return 0;
    }
    if (argc >= 4)
    {
        const std::string cmd = argv[1];
        std::map<std::string, std::string> o;
        if (cmd == "ler" || cmd == "avaliar" || cmd == "gabarito" || cmd == "autoavaliar" || cmd == "estabilidade")
            return leitura(cmd, argc, argv);
        if (cmd == "responder")
        {
            if (!opcoes(argc, argv, 4, o))
                return 2;
            ResponseRequest q;
            q.id = argv[3];
            if (!strategy::parseMode(o["modo"], q.mode))
                return std::printf("--modo desconhecido: \"%s\" (seguir, transfigurar, contrapor, interromper)\n",
                                   o["modo"].c_str()), 2;
            if (!o["seed"].empty())
                q.seed = std::stoull(o["seed"]);
            if (!o["alturas"].empty() && !trioio::composition::parsePitchSource(o["alturas"], q.pitch))
                return std::printf("--alturas desconhecido: \"%s\"\n", o["alturas"].c_str()), 2;
            if (!o["tempo"].empty() && !trioio::composition::parseTimeMode(o["tempo"], q.time))
                return std::printf("--tempo desconhecido: \"%s\"\n", o["tempo"].c_str()), 2;
            if (!o["imprevisibilidade"].empty())
                q.unpredictability = std::clamp(std::stof(o["imprevisibilidade"]), 0.0f, 1.0f);
            if (o["escuta"] == "nao")
                q.useListening = false;
            if (o["ritmo"] == "nao")
                q.rhythm = false;
            if (!o["escala"].empty())
            {
                q.scaleQuery = o["escala"];
                if (o["alturas"].empty())
                    q.pitch = trioio::composition::PitchSource::archive;  // --escala basta
            }
            if (o["mosaico"] == "nao")
                q.mosaic = false;
            if (!o["intencao"].empty() && !trioio::composition::parseIntention(o["intencao"], q.intention))
                return std::printf("--intencao desconhecida: \"%s\" (luminosa, sombria, tensa, serena, suspensa, ambigua, arco)\n",
                                   o["intencao"].c_str()), 2;
            if (o["forma"] == "repeticao" || o["forma"] == "repetição")
                q.formByRepetition = true;
            else if (!o["forma"].empty() && o["forma"] != "contraste")
                return std::printf("--forma desconhecida: \"%s\" (contraste, repeticao)\n", o["forma"].c_str()), 2;
            if (o["andamento"] == "metade")
                q.tempoScale = 0.5;
            else if (o["andamento"] == "dobro")
                q.tempoScale = 2.0;
            else if (!o["andamento"].empty())
                return std::printf("--andamento desconhecido: \"%s\" (metade, dobro)\n", o["andamento"].c_str()), 2;
            if (!o["costura"].empty())
            {
                trioio::composition::Seam sm;
                if (!trioio::composition::parseSeam(o["costura"], sm))
                    return std::printf("--costura desconhecida: \"%s\" (corte, pivo, ponte, sobreposicao)\n", o["costura"].c_str()), 2;
                q.seam = static_cast<int>(sm);
            }
            if (!o["voicing"].empty())
            {
                trioio::composition::Voicing vv;
                if (!trioio::composition::parseVoicing(o["voicing"], vv))
                    return std::printf("--voicing desconhecido: \"%s\" (tercas, quartal, sowhat, planing, bitonal, aberto, cluster)\n",
                                       o["voicing"].c_str()), 2;
                q.voicing = static_cast<int>(vv);
            }
            if (!o["conjunto"].empty())
            {
                trioio::composition::Ensemble en;
                if (o["conjunto"] == "lido")
                    q.ensemble = -1;  // o conjunto lido na referência (readEnsemble)
                else if (!trioio::composition::parseEnsemble(o["conjunto"], en))
                    return std::printf("--conjunto desconhecido: \"%s\" (auto, choro, samba, jazz, manouche, bolero, charanga, "
                                       "camara, orquestra, salsa, acappella, rock, escola, forro, tango, lido)\n", o["conjunto"].c_str()), 2;
                else
                    q.ensemble = static_cast<int>(en);
            }
            if (!o["efeitos"].empty())
                q.filmFx = o["efeitos"] == "sim";
            // o rasgo modular: 0,5 por padrão (a escuta do autor de 15/10: "ainda muito midi")
            q.humanize = o["humano"] != "nao";
            q.writeHarmony = o["harmonia-escrita"] != "nao";
            q.somDaAura = o["som-da-aura"] == "sim";
            if (!o["linguagem-harmonica"].empty())
            {
                trioio::composition::HarmonyLanguage hl;
                if (!trioio::composition::parseHarmonyLanguage(o["linguagem-harmonica"], hl))
                    return std::printf("--linguagem-harmonica desconhecida: \"%s\" (auto, funcional, jazz, brasileira, impressionista, nordestina, mineira, hermeto, modal, andaluza, cromatica, quartal, politonal, octatonica, clusters, estatica, espectral, bartok, scriabin, villa-lobos, moacir, guinga, frevo, maracatu, gismonti, partch)\n",
                                       o["linguagem-harmonica"].c_str()), 2;
                q.harmonyLanguage = static_cast<int>(hl);
            }
            q.rasgo = o["rasgo"].empty() ? 0.5f : std::clamp(static_cast<float>(std::atof(o["rasgo"].c_str())), 0.0f, 1.0f);
            if (!o["com"].empty())
                q.other = o["com"];
            if (!o["da-outra"].empty() && !trioio::composition::parseMashupLayers(o["da-outra"], q.otherLayers))
                return std::printf("--da-outra desconhecido: \"%s\" (harmonia, ritmo, memoria, corpo, tudo)\n",
                                   o["da-outra"].c_str()), 2;
            if (!o["ciclo"].empty())
                q.cycle = o["ciclo"];
            if (!o["mistura"].empty())
            {
                q.mix = o["mistura"];
                q.pitch = trioio::composition::PitchSource::archive;
            }
            if (!o["tecnica"].empty() && !trioio::composition::parseMixTechnique(o["tecnica"], q.mixTechnique))
                return std::printf("--tecnica desconhecida: \"%s\" (colagem, hibrido, camadas, transicao)\n", o["tecnica"].c_str()), 2;
            if (o["maquina"] == "sim")
                q.scaleMachine = true;
            if (o["harmonia"] == "nao")
                q.harmony = false;
            if (o["regras"] == "nao")
                q.useRules = false;
            else
                q.rulesPath = o["regras"];
            // as linhas por região de frequência (docs/LINHAS.md)
            for (const auto& c : juce::StringArray::fromTokens(juce::String::fromUTF8(o["linhas"].c_str()), ",", ""))
                if (c.trim().getDoubleValue() > 0.0)
                    q.lineCuts.push_back(c.trim().getDoubleValue());
            for (const auto& m : juce::StringArray::fromTokens(juce::String::fromUTF8(o["linhas-modos"].c_str()), ",", ""))
            {
                strategy::Mode mm {};
                if (!strategy::parseMode(m.trim().toStdString(), mm))
                    return std::printf("--linhas-modos: modo desconhecido \"%s\"\n", m.toRawUTF8()), 2;
                q.lineModes.push_back(mm);
            }
            q.freeLines = o["linhas-livres"] == "sim";
            q.lineFilter = o["linhas-filtro"] != "nao";
            q.master = o["master"] != "nao";
            q.dialogue = o["dialogo"] == "sim" || o["diálogo"] == "sim";
            q.game = o["jogo"] == "sim";
            q.gameSound = o["jogo-no-som"] != "nao";
            q.stems = o["stems"] == "sim";
            q.usePrompt = o["prompt"] != "nao";
            if (!o["influencias"].empty())
            {
                std::string acc;
                for (char c : o["influencias"] + ",")
                    if (c == ',')
                    {
                        const auto a = acc.find_first_not_of(' '), b = acc.find_last_not_of(' ');
                        if (a != std::string::npos)
                            q.influences.push_back(acc.substr(a, b - a + 1));
                        acc.clear();
                    }
                    else
                        acc += c;
            }
            q.promptText = o["palavras"];
            q.semantic = o["semantica"] == "sim" || o["semântica"] == "sim";
            if (!o["semantica-tempo"].empty())
                q.semanticBudgetMs = std::clamp(std::atoi(o["semantica-tempo"].c_str()), 300, 10000);
            if (!o["narrativa"].empty())
            {
                if (!trioio::composition::isNarrativeGrammar(o["narrativa"]))
                    return std::printf("--narrativa: freytag, kishotenketsu, ondas, plato, arco, ou duas com + (ondas+plato)\n"), 2;
                q.narrative = o["narrativa"];
            }
            if (!o["forca-influencias"].empty())
                q.influenceStrength = std::clamp(static_cast<float>(std::atof(o["forca-influencias"].c_str())), 0.0f, 1.0f);
            if (!o["sem"].empty())
            {
                std::string acc;
                for (char c : o["sem"] + ",")
                    if (c == ',')
                    {
                        const auto a = acc.find_first_not_of(' '), b = acc.find_last_not_of(' ');
                        if (a != std::string::npos)
                            q.exclude.push_back(acc.substr(a, b - a + 1));
                        acc.clear();
                    }
                    else
                        acc += c;
            }
            if (!o["rock"].empty())
            {
                trioio::composition::RockStyle rs;
                if (!trioio::composition::parseRockStyle(o["rock"], rs))
                    return std::printf("--rock: classico, punk, pesado, blues, progressivo, post-rock, psicodelico, noise\n"), 2;
                q.rockStyle = static_cast<int>(rs);
            }
            if (!o["gesto"].empty() && !trioio::composition::parseGesture(o["gesto"], q.gesture))
                return std::printf("--gesto: \"posição:intensidade, …\" entre 0 e 1, pelo menos dois pontos (ex.: \"0:0.2,0.6:1,1:0.3\")\n"), 2;
            if (!o["linhas-modos"].empty() && q.lineCuts.empty())
                return std::printf("--linhas-modos pede --linhas (os cortes, ex.: 200,2000)\n"), 2;
            if (o["motor"] == "energia")
                q.score = false;
            else if (!o["motor"].empty() && o["motor"] != "partitura")
                return std::printf("--motor desconhecido: \"%s\"\n", o["motor"].c_str()), 2;
            const auto r = respond(argv[2], q);
            for (const auto& p : r.problems)
                std::printf("%s\n", p.c_str());
            if (!r.ok)
                return 1;
            for (const auto& m : r.formMarks)
                std::printf("  %s\n", m.c_str());
            if (!r.midiPath.empty())
                std::printf("partitura:  %s\n", r.midiPath.c_str());
            for (const auto& st : r.stemPaths)
                std::printf("%s %s\n", st.find("/stems/") != std::string::npos ? "stem:      " : "linha:     ", st.c_str());
            if (q.master)
                std::printf("master:     %.1f → %.1f LUFS · pico %.1f → %.1f dBTP · cola até %.1f dB · limitador até %.1f dB\n",
                            r.masterReport.lufsBefore, r.masterReport.lufsAfter, r.masterReport.truePeakBefore,
                            r.masterReport.truePeakAfter, r.masterReport.compressionMaxDb, r.masterReport.limiterMaxDb);
            std::printf("resposta:   %s\n"
                        "decisões:   %s\n"
                        "derivação:  %s (pendente)\n\n"
                        "%s\n"
                        "o quanto a resposta segue (+1) ou contraria (−1) a referência, medido no som:\n"
                        "  energia    %+.2f\n  densidade  %+.2f\n  brilho     %+.2f\n",
                        r.wavPath.c_str(), r.decisionsPath.c_str(), r.derivationId.c_str(),
                        strategy::describe(q.mode).c_str(),
                        r.energyCorrelation, r.densityCorrelation, r.brightnessCorrelation);
            if (q.score)
            {
                std::printf("escuta do autor: %d julgamento(s) pesaram nesta resposta\n", r.judgementsUsed);
                if (r.rulesUsed.empty())
                    std::puts("regras do autor: nenhuma");
                else
                {
                    std::printf("regras do autor (%s):\n", r.rulesFile.c_str());
                    for (const auto& l : r.rulesUsed)
                        std::printf("  %s\n", l.c_str());
                }
            }
            return 0;
        }
        if (cmd == "escutar" && argc >= 5)
        {
            trioio::composition::Verdict v {};
            if (!trioio::composition::parseVerdict(argv[4], v))
                return std::printf("julgamento desconhecido: \"%s\" (aceitar, rejeitar, transformar)\n", argv[4]), 2;
            if (!opcoes(argc, argv, 5, o))
                return 2;
            const auto r = recordJudgement(argv[2], argv[3], v, o["nota"], {}, o["trecho"], o["secao"]);
            for (const auto& p : r.problems)
                std::printf("%s\n", p.c_str());
            if (!r.written)
                return std::puts("NADA GRAVADO"), 1;
            for (const auto& j : r.judgements)
            {
                if (j.hasSpan())
                    std::printf("gravado: %s → %s · trecho %d:%04.1f–%d:%04.1f%s (sala ×%.2f)\n", j.derivationId.c_str(),
                                verdictName(j.verdict), static_cast<int>(j.spanStart) / 60, std::fmod(j.spanStart, 60.0),
                                static_cast<int>(j.spanEnd) / 60, std::fmod(j.spanEnd, 60.0),
                                j.section.empty() ? "" : (" · seção " + j.section).c_str(), j.roomWeight);
                else
                    std::printf("gravado: %s → %s\n", j.derivationId.c_str(), verdictName(j.verdict));
                for (const auto& [fam, c] : j.timbres)
                    std::printf(j.hasSpan() ? "  timbre %-20s %.1f nota(s)\n" : "  timbre %-20s %.0f nota(s)\n",
                                timbreName(static_cast<trioio::composition::Timbre>(fam)), c);
                std::printf("  intervenções do Terceiro Olho: %zu · sala: %s\n", j.eyeChoices.size(),
                            j.room >= 0 ? roomName(static_cast<trioio::composition::Room>(j.room)) : "?");
            }
            std::printf("escuta em %s\n", listeningPath(argv[2]).c_str());
            return 0;
        }
        if (cmd == "autorizar" || cmd == "recusar" || cmd == "revogar")
        {
            if (!opcoes(argc, argv, 4, o))
                return 2;
            if (cmd == "autorizar")
                return autorizar(argv[2], argv[3], o);
            if (cmd == "recusar")
                return relatar(refuse(argv[2], argv[3], o["por"], o["motivo"]));
            return relatar(revoke(argv[2], argv[3], o["por"], o["motivo"]));
        }
    }
    if (argc == 3 && std::string(argv[1]) == "escuta")
    {
        using namespace trioio::composition;
        const auto h = loadJudgements(argv[2]);
        const auto t = tendenciesFrom(h);
        std::printf("%zu julgamento(s) em %s\n", h.size(), listeningPath(argv[2]).c_str());
        int a = 0, rj = 0, tr = 0;
        for (const auto& j : h)
            (j.verdict == Verdict::accept ? a : j.verdict == Verdict::reject ? rj : tr)++;
        std::printf("  aceitas %d · rejeitadas %d · a transformar %d\n\n", a, rj, tr);
        std::puts("tendências (1 = neutro; 0,5..2):");
        for (int k = 0; k < kTimbreCount; ++k)
            if (std::abs(t.timbre[static_cast<std::size_t>(k)] - 1.0f) > 0.02f)
                std::printf("  timbre %-20s ×%.2f\n", timbreName(static_cast<Timbre>(k)), t.timbre[static_cast<std::size_t>(k)]);
        for (int k = 0; k < kRooms; ++k)
            if (std::abs(t.room[static_cast<std::size_t>(k)] - 1.0f) > 0.02f)
                std::printf("  sala   %-20s ×%.2f\n", roomName(static_cast<Room>(k)), t.room[static_cast<std::size_t>(k)]);
        int eyeMoved = 0;
        for (float w : t.eye)
            eyeMoved += std::abs(w - 1.0f) > 0.02f;
        std::printf("  Terceiro Olho: %d escolha(s) com peso alterado\n", eyeMoved);
        for (const auto& j : h)
            if (!j.note.empty())
                std::printf("  nota (%s, %s): %s\n", j.derivationId.c_str(), verdictName(j.verdict), j.note.c_str());
        return 0;
    }
    return uso();
}
