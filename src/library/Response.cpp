#include "Response.h"

#include "ListeningStore.h"

#include "Analysis.h"
#include "HarmonyWriting.h"
#include "BandSplit.h"
#include "Game.h"
#include "Lines.h"
#include "Reading.h"
#include "EnergySynth.h"
#include "ScoreSynth.h"
#include "Denormals.h"
#include "Prompts.h"
#include "Influences.h"
#include "DataFiles.h"
#include "SemanticBridge.h"
#include "TextFold.h"
#include "Reading.h"
#include "LibraryImporter.h"
#include "ManifestJson.h"
#include "Mashup.h"
#include "Recombination.h"
#include "RhythmCycles.h"
#include "ScaleArchive.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

#include <cstdlib>
#include <map>
#include <memory>

#ifndef TRIOIO_VERSION
#define TRIOIO_VERSION "0.0.0"
#endif

namespace trioio::library
{
using namespace trioio::provenance;
using namespace trioio::perception;

namespace
{
juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.data(), static_cast<int>(s.size())); }
constexpr double kOutputRate = 48000.0;

// O arquivo de escalas (docs/ESCALAS.md). Um .scl no termo: só ele. Senão,
// o arquivo do Trioio (TRIOIO_ESCALAS, ou data/escalas/arquivo_trioio.txt
// do código-fonte) mais as escalas do autor em <dados>/escalas/ (*.txt no
// formato do Trioio, *.scl no formato Scala).
std::shared_ptr<const std::vector<composition::ArchiveScale>> loadScaleArchive(const std::string& dataRoot,
                                                                               const std::string& query,
                                                                               std::string& outQuery,
                                                                               std::vector<std::string>& errors)
{
    auto all = std::make_shared<std::vector<composition::ArchiveScale>>();
    outQuery = query;
    const auto readScl = [&](const juce::File& f) {
        if (auto s = composition::parseScl(f.loadFileAsString().toStdString(), f.getFileNameWithoutExtension().toStdString()))
            all->push_back(*s);
        else
            errors.push_back("arquivo .scl ilegível: " + f.getFullPathName().toStdString());
    };
    const auto readArchive = [&](const juce::File& f) {
        std::vector<std::string> e;
        auto v = composition::parseScaleArchive(f.loadFileAsString().toStdString(), e);
        for (const auto& x : e)
            errors.push_back(f.getFileName().toStdString() + ": " + x);
        all->insert(all->end(), v.begin(), v.end());
    };
    if (juce::String(u8(query)).endsWithIgnoreCase(".scl"))
    {
        const juce::File f(u8(query));
        if (!f.existsAsFile())
            errors.push_back("arquivo .scl não encontrado: " + query);
        else
            readScl(f);
        outQuery.clear();
        return all;
    }
    const char* env = std::getenv("TRIOIO_ESCALAS");
    const juce::File main = env && *env ? juce::File(juce::CharPointer_UTF8(env)) : trioioDataFile("escalas/arquivo_trioio.txt");
    if (!main.existsAsFile())
        errors.push_back("arquivo de escalas não encontrado: " + main.getFullPathName().toStdString()
                         + " (defina TRIOIO_ESCALAS)");
    else
        readArchive(main);
    const auto own = juce::File(u8(dataRoot)).getChildFile("escalas");
    if (own.isDirectory())
    {
        for (const auto& f : own.findChildFiles(juce::File::findFiles, false, "*.txt"))
            readArchive(f);
        for (const auto& f : own.findChildFiles(juce::File::findFiles, false, "*.scl"))
            readScl(f);
    }
    return all;
}

bool writeAtomically(const juce::File& target, const juce::String& text)
{
    target.getParentDirectory().createDirectory();
    juce::TemporaryFile tmp(target);
    return tmp.getFile().replaceWithText(text, false, false, "\n") && tmp.overwriteTargetFileWithTemporary();
}

juce::var decisionsJson(const std::string& sessionId, const ResponseRequest& q,
                        const std::vector<strategy::Step>& steps)
{
    auto root = juce::var(new juce::DynamicObject());
    auto* o = root.getDynamicObject();
    o->setProperty("schemaVersion", "trioio.decisoes/1");
    o->setProperty("sessionId", u8(sessionId));
    o->setProperty("referenceId", u8(q.id));
    if (!q.other.empty())
    {
        o->setProperty("mashupWith", u8(q.other));
        o->setProperty("mashupLayers", u8(composition::mashupLayersName(q.otherLayers)));
    }
    o->setProperty("strategy", strategy::modeName(q.mode));
    o->setProperty("seed", juce::String(static_cast<juce::uint64>(q.seed)));
    o->setProperty("windowSeconds", q.windowSeconds);
    juce::Array<juce::var> arr;
    for (const auto& s : steps)
    {
        auto st = juce::var(new juce::DynamicObject());
        auto* p = st.getDynamicObject();
        p->setProperty("start", s.start);
        p->setProperty("end", s.end);
        p->setProperty("energy", s.decision.energy);
        p->setProperty("density", s.decision.density);
        p->setProperty("brightness", s.decision.brightness);
        p->setProperty("harmonicity", s.decision.harmonicity);
        p->setProperty("transientRetention", s.decision.transientRetention);
        p->setProperty("mutation", s.decision.mutation);
        p->setProperty("spatialActivity", s.decision.spatialActivity);
        arr.add(st);
    }
    o->setProperty("steps", arr);
    return root;
}
// ---- as linhas por região de frequência (docs/LINHAS.md, etapa 2) ----------

struct LineOutput
{
    std::string name;
    double lowHz = 0.0, highHz = 0.0, energyShare = 0.0;
    strategy::Mode mode = strategy::Mode::follow;
    std::uint64_t seed = 1;
    double bpm = 0.0;
    bool ownPulse = false;
    composition::LineValidity valid;
    std::string borrowed;
    float lowMidi = 0.0f, highMidi = 0.0f;
    std::vector<float> left, right;  // o stem
    std::size_t firstNote = 0, notes = 0;
    double gainDb = 0.0;  // o equilíbrio das linhas e o teto da soma
};

std::string slug(const std::string& name)
{
    std::string s;
    for (char c : name)
        if (std::isalnum(static_cast<unsigned char>(c)))
            s += c;
        else if (!s.empty() && s.back() != '-')
            s += '-';
    while (!s.empty() && s.back() == '-')
        s.pop_back();
    return s;
}

// Cada linha da referência é escutada e respondida por um compositor próprio
// (modo e semente próprios); onde as alturas da linha não valem, ela compõe
// com as da peça (borrowPitch). O regente dá o pulso da peça a todas (ou, com
// `freeLines`, deixa cada linha no seu quando ele vale ou concorda), dobra as
// notas de cada linha para a sua região, filtra o som dela na sua banda e soma.
bool composeLines(const std::string& dataRoot, const ResponseRequest& q, const AnalysisResult& heard,
                  const composition::Options& base, std::size_t total, juce::AudioBuffer<float>& audio,
                  composition::Score& score, std::vector<LineOutput>& lines, std::vector<std::string>& problems,
                  std::vector<Stem>* voiceStems = nullptr)
{
    using namespace composition;
    // os stems por voz no modo linhas: cada linha grava os seus (já
    // filtrados pela banda) num temporário da pasta de dados; no fim, cada
    // voz é somada linha a linha com o ganho final da linha (uma voz por vez
    // na memória)
    const juce::File stemTmp = juce::File(u8(dataRoot)).getChildFile("tmp-stems-" + juce::Uuid().toString());
    std::vector<std::string> stemNames;
    const auto writeRaw = [](const juce::File& f, const std::vector<float>& v) {
        juce::FileOutputStream os(f);
        return os.openedOk() && os.write(v.data(), v.size() * sizeof(float));
    };
    const auto readRaw = [](const juce::File& f, std::size_t n) {
        std::vector<float> v(n, 0.0f);
        juce::FileInputStream is(f);
        if (is.openedOk())
            is.read(v.data(), static_cast<int>(std::min<juce::int64>(is.getTotalLength(), static_cast<juce::int64>(n * sizeof(float)))));
        return v;
    };
    std::vector<float> mono;
    double sr = 0.0;
    if (!readReferenceMono(dataRoot, q.id, mono, sr, problems, 1800.0))
        return false;
    const double dur = heard.duration;
    const Reading whole = readReference(heard.frames, heard.onsets, dur, q.formByRepetition, 1.0);
    const Pulse wholePulse = estimatePulse(heard.frames, heard.onsets, 0.0, dur);
    auto bands = splitBands(mono, sr, q.lineCuts);
    if (bands.size() < 2)
    {
        problems.push_back("linhas: nenhum corte válido em --linhas (ex.: 200,2000)");
        return false;
    }
    std::vector<double> cuts;
    for (std::size_t k = 0; k + 1 < bands.size(); ++k)
        cuts.push_back(bands[k].highHz);

    std::vector<Score> scores;
    for (std::size_t k = 0; k < bands.size(); ++k)
    {
        auto& b = bands[k];
        LineOutput lo;
        lo.name = b.name();
        lo.lowHz = b.lowHz;
        lo.highHz = b.highHz;
        lo.energyShare = b.energyShare;
        const float g = levelMatchGain(b, mono);
        for (auto& v : b.samples)
            v *= g;
        Analyzer a(sr);
        a.push(b.samples.data(), b.samples.size());
        a.finish();
        std::vector<float>().swap(b.samples);
        auto frames = a.frames();
        const auto& onsets = a.onsets();
        const Reading r = readReference(frames, onsets, dur);
        lo.valid = assessLine(b.lowHz, b.highHz, sr, r, whole, static_cast<double>(onsets.size()) / std::max(1.0, dur));
        lo.borrowed = borrowPitch(frames, heard.frames, lo.valid);
        Options o = base;
        o.mode = k < q.lineModes.size() ? q.lineModes[k] : q.mode;
        o.seed = q.seed + 7919u * static_cast<std::uint64_t>(k);
        lo.ownPulse = q.freeLines && (lo.valid.pulse || lo.valid.agrees);
        if (!lo.ownPulse)
            o.pulse = wholePulse;
        lo.mode = o.mode;
        lo.seed = o.seed;
        o.expressiveEnding = false;  // as linhas somam no comprimento da referência
        Score s = compose(frames, onsets, dur, o);
        lo.bpm = s.bpm;
        bandRegister(b.lowHz, b.highHz, lo.lowMidi, lo.highMidi);
        foldIntoRegister(s, lo.lowMidi, lo.highMidi);
        std::vector<Stem> lineStems;
        renderScore(s, kOutputRate, lo.left, lo.right, voiceStems ? &lineStems : nullptr);
        lo.left.resize(total, 0.0f);
        lo.right.resize(total, 0.0f);
        if (q.lineFilter)
        {
            lo.left = extractBand(lo.left, kOutputRate, cuts, k);
            lo.right = extractBand(lo.right, kOutputRate, cuts, k);
        }
        for (auto& st : lineStems)
        {
            st.left.resize(total, 0.0f);
            st.right.resize(total, 0.0f);
            if (q.lineFilter)
            {
                st.left = extractBand(st.left, kOutputRate, cuts, k);
                st.right = extractBand(st.right, kOutputRate, cuts, k);
            }
            stemTmp.createDirectory();
            const auto base = juce::String(static_cast<int>(k)) + "-" + u8(st.name);
            if (!writeRaw(stemTmp.getChildFile(base + ".L"), st.left) || !writeRaw(stemTmp.getChildFile(base + ".R"), st.right))
                problems.push_back("stems: falha no temporário " + stemTmp.getFullPathName().toStdString());
            if (std::find(stemNames.begin(), stemNames.end(), st.name) == stemNames.end())
                stemNames.push_back(st.name);
        }
        scores.push_back(std::move(s));
        lines.push_back(std::move(lo));
    }

    // o equilíbrio: cada linha na proporção de energia que a sua banda tem na
    // referência (ganho limitado a ±12 dB; o filtro tira muito do agudo de
    // uma síntese que nasce no médio); a soma mantém a energia total
    const auto energy = [](const LineOutput& lo) {
        double e = 0.0;
        for (std::size_t i = 0; i < lo.left.size(); ++i)
            e += static_cast<double>(lo.left[i]) * lo.left[i] + static_cast<double>(lo.right[i]) * lo.right[i];
        return e;
    };
    double shares = 0.0, rendered = 0.0;
    std::vector<double> e(lines.size());
    for (std::size_t k = 0; k < lines.size(); ++k)
    {
        e[k] = energy(lines[k]);
        shares += lines[k].energyShare;
        rendered += e[k];
    }
    for (std::size_t k = 0; k < lines.size(); ++k)
    {
        double g = 1.0;
        if (e[k] > 0.0 && shares > 0.0 && rendered > 0.0)
            g = std::clamp(std::sqrt((lines[k].energyShare / shares) * rendered / e[k]), 0.25, 4.0);
        lines[k].gainDb = 20.0 * std::log10(g);
        for (std::size_t i = 0; i < total; ++i)
        {
            lines[k].left[i] *= static_cast<float>(g);
            lines[k].right[i] *= static_cast<float>(g);
            audio.addSample(0, static_cast<int>(i), lines[k].left[i]);
            audio.addSample(1, static_cast<int>(i), lines[k].right[i]);
        }
    }
    // a soma até −1 dBFS de pico; os stems recebem o mesmo ganho (continuam
    // somando a peça)
    const float peak = std::max(audio.getMagnitude(0, 0, static_cast<int>(total)), audio.getMagnitude(1, 0, static_cast<int>(total)));
    constexpr float kCeiling = 0.891f;  // −1 dBFS
    if (peak > kCeiling)
    {
        const float g = kCeiling / peak;
        audio.applyGain(g);
        for (auto& lo : lines)
        {
            for (auto& v : lo.left) v *= g;
            for (auto& v : lo.right) v *= g;
            lo.gainDb += 20.0 * std::log10(static_cast<double>(g));
        }
    }

    // os stems por voz: a soma das linhas, cada uma com o seu ganho final
    if (voiceStems && !stemNames.empty())
    {
        std::sort(stemNames.begin(), stemNames.end());
        for (const auto& name : stemNames)
        {
            Stem acc;
            acc.name = name;
            acc.left.assign(total, 0.0f);
            acc.right.assign(total, 0.0f);
            for (std::size_t k = 0; k < lines.size(); ++k)
            {
                const auto base = juce::String(static_cast<int>(k)) + "-" + u8(name);
                const auto fl = stemTmp.getChildFile(base + ".L");
                if (!fl.existsAsFile())
                    continue;
                const auto g = static_cast<float>(std::pow(10.0, lines[k].gainDb / 20.0));
                const auto l = readRaw(fl, total), r = readRaw(stemTmp.getChildFile(base + ".R"), total);
                for (std::size_t i = 0; i < total; ++i)
                {
                    acc.left[i] += g * l[i];
                    acc.right[i] += g * r[i];
                }
            }
            voiceStems->push_back(std::move(acc));
        }
    }
    if (stemTmp.exists())
        stemTmp.deleteRecursively();
    // a linha que guia (a forma e as seções que a interface mostra): a de mais
    // energia entre as que têm melodia e acordes; senão a de mais energia
    std::size_t lead = 0;
    for (std::size_t k = 0; k < lines.size(); ++k)
    {
        const auto ok = [&](std::size_t i) { return lines[i].valid.melody && lines[i].valid.chords; };
        if ((ok(k) && !ok(lead)) || (ok(k) == ok(lead) && lines[k].energyShare > lines[lead].energyShare))
            lead = k;
    }
    score = Score {};
    score.duration = dur;
    score.bpm = scores[lead].bpm;
    score.room = scores[lead].room;
    score.spectra = scores[lead].spectra;
    char head[512];
    std::snprintf(head, sizeof head, "linhas: %zu por região de frequência (cortes", lines.size());
    std::string h = head;
    for (double c : cuts)
        h += " " + std::to_string(static_cast<int>(std::lround(c))) + " Hz";
    h += "; Linkwitz-Riley 4.ª ordem, fase zero) · ";
    h += q.freeLines ? "linhas livres (cada uma no seu pulso, quando ele vale)" : "pulso comum da peça";
    h += " · a forma mostrada é a da linha " + lines[lead].name;
    score.marks.push_back({ 0.0, h });
    for (std::size_t k = 0; k < lines.size(); ++k)
    {
        const auto& lo = lines[k];
        std::snprintf(head, sizeof head, "linha %s: %s · seed %llu · %.1f BPM (%s) · registro MIDI %.0f–%.0f · %+.1f dB (%.0f %% da energia da referência) · alturas: %s",
                      lo.name.c_str(), strategy::modeName(lo.mode), static_cast<unsigned long long>(lo.seed), lo.bpm,
                      lo.ownPulse ? "o pulso da linha" : "o pulso da peça", lo.lowMidi, lo.highMidi, lo.gainDb,
                      100.0 * lo.energyShare, lo.borrowed == "nada" ? "as da linha" : lo.borrowed.c_str());
        score.marks.push_back({ 0.0, head });
    }
    for (std::size_t k = 0; k < scores.size(); ++k)
    {
        for (const auto& m : scores[k].marks)
            score.marks.push_back({ m.time, k == lead ? m.text : "[" + lines[k].name + "] " + m.text });
        lines[k].firstNote = score.notes.size();
        lines[k].notes = scores[k].notes.size();
        score.notes.insert(score.notes.end(), scores[k].notes.begin(), scores[k].notes.end());
        score.gestures.insert(score.gestures.end(), scores[k].gestures.begin(), scores[k].gestures.end());
    }
    std::stable_sort(score.marks.begin(), score.marks.end(),
                     [](const Mark& a, const Mark& b) { return a.time < b.time; });
    return true;
}

// ---- a memória do jogo (docs/JOGO.md §3): o que ficou e o que apodreceu ----
// As regras de cada resposta anterior (decisoes.json → "game") com o veredito
// do autor (escuta.json): aceita → fica; rejeitada ou "a transformar" → adubo.
// Das que ficaram, só as 8 mais recentes seguem vivas; as mais antigas
// apodrecem também (o esquecimento gradual do cérebro).
void loadGameMemory(const std::string& dataRoot, std::vector<composition::GameRule>& kept,
                    std::vector<composition::GameRule>& compost)
{
    // os julgamentos de cada derivação, na ordem dada (a resposta inteira e
    // os trechos): cada regra é julgada pelos trechos que cobrem os seus
    // disparos (judgeGameRule), ou pela resposta inteira
    std::map<std::string, std::vector<composition::Judgement>> verdict;
    for (const auto& j : loadJudgements(dataRoot))
        verdict[j.derivationId].push_back(j);
    std::vector<std::pair<std::string, std::vector<std::pair<int, composition::GameRule>>>> bySession;
    for (const auto& f : juce::File(u8(derivationManifestDir(dataRoot))).findChildFiles(juce::File::findFiles, false, "*.json"))
    {
        const auto read = fromJson(f.loadFileAsString().toStdString());
        if (!read.manifest || !read.manifest->derivation)
            continue;
        const auto it = verdict.find(read.manifest->id);
        if (it == verdict.end())
            continue;
        const auto& sid = read.manifest->derivation->sessionId;
        const auto dj = juce::JSON::parse(juce::File(u8(dataRoot)).getChildFile("sessoes").getChildFile(u8(sid)).getChildFile("decisoes.json"));
        const auto* arr = dj["game"].getArray();
        if (!arr)
            continue;
        std::vector<std::pair<int, composition::GameRule>> rules;
        for (const auto& v : *arr)
        {
            composition::GameRule r;
            r.id = static_cast<int>(v["id"]);
            r.when = static_cast<int>(v["when"]);
            r.action = static_cast<composition::GameAction>(std::clamp(static_cast<int>(v["action"]), 0, composition::kGameActions - 1));
            r.material = static_cast<int>(v["material"]);
            r.span = static_cast<double>(v["span"]);
            // a verdade, não a atribuição apócrifa
            r.origin = v.hasProperty("trueOrigin") && v["trueOrigin"].toString().isNotEmpty() ? v["trueOrigin"].toString().toStdString()
                                                                                              : v["origin"].toString().toStdString();
            if (const auto* at = v["firedAt"].getArray())
                for (const auto& t : *at)
                    r.firedAt.push_back(static_cast<double>(t));
            const int rv = composition::judgeGameRule(r, it->second);
            if (rv >= 0)
                rules.push_back({ rv, r });
        }
        bySession.push_back({ sid, rules });
    }
    std::sort(bySession.begin(), bySession.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (const auto& [sid, rs] : bySession)
        for (const auto& [rv, r] : rs)
            (rv == 0 ? kept : compost).push_back(r);
    while (kept.size() > 8)
    {
        compost.push_back(kept.front());  // a mais antiga apodrece
        kept.erase(kept.begin());
    }
}

} // namespace

std::string derivationManifestDir(const std::string& dataRoot)
{
    return juce::File(u8(dataRoot)).getChildFile("indice").getChildFile("derivacoes").getFullPathName().toStdString();
}

// A árvore de influências: a do Trioio (data/influencias/, gerada e citada)
// mais a do AUTOR, na pasta de dados (influencias_do_autor.txt): ali ele
// escreve as poéticas de artistas e bandas com a sua leitura e as suas fontes
// (o Trioio não as inventa; 2026-10-18, o autor: "se não é possível escrever
// nome de artistas e bandas talvez seja necessário explicar isso no
// tutorial"). Um nó do autor com o mesmo caminho substitui o do Trioio. O
// arquivo nasce com o modelo comentado, e nada o sobrescreve depois.
static std::vector<composition::InfluenceNode> loadInfluences(const std::string& dataRoot, std::vector<std::string>& errors)
{
    auto base = composition::parseInfluenceArchive(
        trioioDataFile("influencias/arquivo_trioio_influencias.txt").loadFileAsString().toStdString(), errors);
    if (dataRoot.empty())
        return base;
    const auto mine = juce::File(u8(dataRoot)).getChildFile("influencias_do_autor.txt");
    if (!mine.existsAsFile() && juce::File(u8(dataRoot)).isDirectory())
        (void) mine.replaceWithText(juce::String::fromUTF8(
            "# As suas influências (o Trioio lê este arquivo a cada resposta; nada o sobrescreve).\n"
            "# Your own influences (read at every response; never overwritten).\n"
            "#\n"
            "# Aqui entram as poéticas de artistas e bandas, com a SUA leitura e as suas fontes:\n"
            "# o Trioio não as inventa. Um bloco por nó, no formato de\n"
            "# data/influencias/arquivo_trioio_influencias.txt (ver docs/INFLUENCIAS.md):\n"
            "#\n"
            "#   = artistas/jupiter-maca\n"
            "#   também: júpiter maçã, jupiter apple\n"
            "#   conceito: (a ideia, nas suas palavras)\n"
            "#   contexto: social: …; histórico: …; tecnológico: …; geração: …\n"
            "#   traço: conjunto:rock 0.7\n"
            "#   traço: eixo:rasgo 0.6\n"
            "#   traço: timbre:contracanto:órgão 0.5\n"
            "#   fonte: (o disco, o livro, a entrevista)\n"
            "#\n"
            "# A linha \"também:\" dá os nomes como se escrevem nas Palavras e nas\n"
            "# Influências. Tire o # do começo das linhas para usar o exemplo.\n"));
    std::vector<std::string> mineErrors;
    const auto own = composition::parseInfluenceArchive(mine.loadFileAsString().toStdString(), mineErrors);
    for (const auto& e : mineErrors)
        errors.push_back("influencias_do_autor.txt: " + e);
    for (const auto& n : own)
    {
        const auto it = std::find_if(base.begin(), base.end(), [&](const auto& b) { return b.path == n.path; });
        if (it != base.end())
            *it = n;
        else
            base.push_back(n);
    }
    return base;
}

ResponseResult respond(const std::string& dataRoot, const ResponseRequest& q, const std::string& nowArg)
{
    const ScopedNoDenormals denormals;  // a escuta e a composição também (core/Denormals.h)
    ResponseResult out;
    const std::string now = nowArg.empty() ? juce::Time::getCurrentTime().toISO8601(true).toStdString() : nowArg;

    // 1. portão
    const auto mf = juce::File(u8(referenceManifestDir(dataRoot))).getChildFile(u8(q.id) + ".json");
    const auto read = fromJson(mf.loadFileAsString().toStdString());
    if (!read.manifest)
    {
        out.problems.push_back("manifesto não encontrado ou ilegível: " + q.id);
        return out;
    }
    if (!permits(*read.manifest, Use::derive))
    {
        out.problems.push_back("derivação não autorizada para " + q.id
                               + " (declarar com: trioio_biblioteca autorizar ... --usos derive)");
        return out;
    }

    const auto step = [&q](float f, const char* what) {
        if (q.progress)
            q.progress(f, what);
    };
    // 2. escuta da referência (usa o cache)
    step(0.02f, "escutando a referência");
    const auto heard = analyzeReference(dataRoot, q.id);
    step(0.10f, "lendo as escolhas");
    if (!heard.ok)
    {
        out.problems = heard.problems;
        return out;
    }
    // 2b. mashup: a outra referência passa pelo mesmo portão e é escutada; os
    //     quadros dela vão para o tempo desta (core/Mashup.h)
    std::vector<perception::Frame> otherFrames;
    std::vector<perception::Onset> otherOnsets;
    if (!q.other.empty())
    {
        const auto omf = juce::File(u8(referenceManifestDir(dataRoot))).getChildFile(u8(q.other) + ".json");
        const auto oread = fromJson(omf.loadFileAsString().toStdString());
        if (!oread.manifest)
        {
            out.problems.push_back("manifesto não encontrado ou ilegível: " + q.other);
            return out;
        }
        if (!permits(*oread.manifest, Use::derive))
        {
            out.problems.push_back("derivação não autorizada para " + q.other
                                   + " (declarar com: trioio_biblioteca autorizar ... --usos derive)");
            return out;
        }
        const auto oh = analyzeReference(dataRoot, q.other);
        if (!oh.ok)
        {
            out.problems = oh.problems;
            return out;
        }
        otherFrames = oh.frames;
        otherOnsets = oh.onsets;
        composition::remapTimeline(otherFrames, otherOnsets, oh.duration, heard.duration);
    }
    const auto refTimeline = segment(heard.frames, heard.onsets, heard.duration, q.windowSeconds);

    // 3. estratégia
    const auto steps = strategy::plan(refTimeline, q.mode, q.seed);
    if (steps.empty())
    {
        out.problems.push_back("referência curta demais para responder");
        return out;
    }

    // 4. síntese (motor partitura ou energia), com escuta da própria saída
    // o comprimento: o da referência; a resposta pode passar dele quando a
    // expressão alonga o final (o ritardando e a fermata, Production.h)
    auto total = static_cast<std::size_t>(heard.duration * kOutputRate + 0.5);
    juce::AudioBuffer<float> audio(2, static_cast<int>(total));
    audio.clear();
    composition::Score score;
    std::vector<LineOutput> lineInfo;  // só com linhas
    std::vector<Stem> voiceStems;      // só com --stems sim
    std::size_t gameKeptN = 0, gameCompostN = 0;  // a memória do jogo usada
    if (q.score)
    {
        // a escuta do autor (escuta.json) vira tendências nos sorteios
        composition::Options opt { q.mode, q.seed, q.pitch, q.time, q.unpredictability };
        opt.rhythm = q.rhythm;
        opt.mosaic = q.mosaic;
        opt.harmony = q.harmony;
        opt.scaleMachine = q.scaleMachine;
        opt.intention = q.intention;
        opt.mixTechnique = q.mixTechnique;
        opt.cycleQuery = q.cycle;
        opt.voicing = q.voicing;
        opt.ensemble = q.ensemble;
        opt.filmFx = q.filmFx;
        opt.rasgo = q.rasgo;
        opt.humanize = q.humanize;
        opt.writeHarmony = q.writeHarmony;
        opt.harmonyLanguage = q.harmonyLanguage;
        opt.somDaAura = q.somDaAura;
        opt.seam = q.seam;
        opt.tempoScale = q.tempoScale;
        opt.formByRepetition = q.formByRepetition;
        opt.dialogue = q.dialogue;
        opt.game = q.game;
        opt.gesture = q.gesture;
        opt.rockStyle = q.rockStyle;
        opt.narrative = q.narrative;
        opt.gameSoundCriteria = q.gameSound;
        if (q.game)
        {
            loadGameMemory(dataRoot, opt.gameKept, opt.gameCompost);
            // as notas livres do autor (o caminho 4): as 8 mais recentes
            for (const auto& j : loadJudgements(dataRoot))
                if (!j.note.empty())
                    opt.gameNotes.push_back(j.note);
            if (opt.gameNotes.size() > 8)
                opt.gameNotes.erase(opt.gameNotes.begin(), opt.gameNotes.end() - 8);
            // a sessão longa: o estado em que terminou o jogo da resposta mais
            // recente que jogou (as sessões têm o instante no nome; olha as 20 últimas)
            {
                auto dirs = juce::File(u8(dataRoot)).getChildFile("sessoes").findChildFiles(juce::File::findDirectories, false, "sessao-*");
                std::sort(dirs.begin(), dirs.end(), [](const juce::File& a, const juce::File& b) { return a.getFileName() > b.getFileName(); });
                for (int i = 0; i < std::min(20, dirs.size()); ++i)
                {
                    const auto dj = juce::JSON::parse(dirs[i].getChildFile("decisoes.json"));
                    const auto* st = dj["gameState"].getArray();
                    if (!st || st->isEmpty())
                        continue;
                    const auto& last = st->getReference(st->size() - 1);
                    auto& g = opt.gamePrevious;
                    g.energy = static_cast<double>(last["energy"]);
                    g.density = static_cast<double>(last["density"]);
                    g.memory = static_cast<double>(last["memory"]);
                    g.coherence = static_cast<double>(last["coherence"]);
                    g.surprise = static_cast<double>(last["surprise"]);
                    g.hidden = static_cast<double>(last["hidden"]);
                    g.collapse = static_cast<double>(last["collapse"]);
                    g.freedom = static_cast<double>(last["freedom"]);
                    opt.hasGamePrevious = true;
                    break;
                }
            }
            gameKeptN = opt.gameKept.size();
            gameCompostN = opt.gameCompost.size();
        }
        if (!q.other.empty())
        {
            opt.otherFrames = &otherFrames;
            opt.otherOnsets = &otherOnsets;
            opt.otherLayers = q.otherLayers;
            opt.otherName = q.other;
        }
        {
            // o arquivo de ciclos rítmicos (TRIOIO_RITMOS, ou o do código-fonte):
            // sempre que existir, para as escalas que pedem o seu ciclo
            const char* env = std::getenv("TRIOIO_RITMOS");
            const juce::File rf = env && *env ? juce::File(juce::CharPointer_UTF8(env)) : trioioDataFile("ritmos/arquivo_trioio_ritmos.txt");
            {
                // o arquivo de cadências da máquina de harmonia (opcional)
                const juce::File cf = trioioDataFile("harmonia/arquivo_trioio_cadencias.txt");
                if (cf.existsAsFile())
                {
                    std::vector<std::string> errors;
                    opt.cadenceArchive = std::make_shared<const std::vector<composition::CadenceEntry>>(
                        composition::parseCadenceArchive(cf.loadFileAsString().toStdString(), errors));
                    for (const auto& e : errors)
                        out.problems.push_back("cadências: " + e);
                }
            }
            if (rf.existsAsFile())
            {
                std::vector<std::string> errors;
                opt.rhythmArchive = std::make_shared<const std::vector<composition::RhythmCycle>>(
                    composition::parseRhythmArchive(rf.loadFileAsString().toStdString(), errors));
                for (const auto& e : errors)
                    out.problems.push_back("ritmos: " + e);
                if (!errors.empty())
                    return out;
            }
            else if (!q.cycle.empty())
            {
                out.problems.push_back("arquivo de ciclos rítmicos não encontrado: " + rf.getFullPathName().toStdString()
                                       + " (defina TRIOIO_RITMOS)");
                return out;
            }
        }
        std::vector<std::string> mixNotes;
        {
            // "rāga Yaman + maqam Hijaz + blues" (ou com vírgulas: "modos gregos, escalas africanas")
            std::string rest = q.mix;
            while (!rest.empty())
            {
                const auto plus = rest.find_first_of("+,");
                auto term = rest.substr(0, plus);
                term.erase(0, term.find_first_not_of(" \t"));
                term.erase(term.find_last_not_of(" \t") + 1);
                // "harmonia funcional", "jazz": uma linguagem harmônica, não uma
                // escala — vai para a harmonia (se o autor não escolheu outra)
                composition::HarmonyLanguage hl {};
                std::string hterm = composition::foldText(term);
                if (hterm.rfind("harmonia ", 0) == 0)
                    hterm = hterm.substr(9);
                // sem "harmonia" na frente, os nomes que também são de escala
                // ("cromática", "modal", "octatônica", "andaluza") ficam escala
                const bool prefixed = composition::foldText(term).rfind("harmonia ", 0) == 0 || composition::foldText(term).rfind("harmony ", 0) == 0;
                const bool scaleName = hterm.rfind("cromat", 0) == 0 || hterm.rfind("modal", 0) == 0 || hterm.rfind("octaton", 0) == 0
                                       || hterm.rfind("andaluz", 0) == 0 || hterm.rfind("quartal", 0) == 0 || hterm.rfind("espectral", 0) == 0;
                if (hterm.rfind("harmony ", 0) == 0)
                    hterm = hterm.substr(8);
                if (!term.empty() && (prefixed || !scaleName) && hterm.find(' ') == std::string::npos
                    && composition::parseHarmonyLanguage(hterm, hl))
                {
                    if (q.harmonyLanguage == 0)
                        opt.harmonyLanguage = static_cast<int>(hl);
                    mixNotes.push_back("mistura: \"" + term + "\" é uma linguagem harmônica"
                                       + (q.harmonyLanguage == 0 ? std::string(" → a harmonia ") + composition::harmonyLanguageName(hl)
                                                                 : std::string(" (a harmonia escolhida na interface fica)")));
                }
                else if (!term.empty())
                    opt.mix.push_back(term);
                rest = plus == std::string::npos ? "" : rest.substr(plus + 1);
            }
        }
        if (q.pitch == composition::PitchSource::archive || q.scaleMachine || q.intention != composition::Intention::none
            || !q.mix.empty())
        {
            std::vector<std::string> errors;
            opt.scaleArchive = loadScaleArchive(dataRoot, q.scaleQuery, opt.scaleQuery, errors);
            for (const auto& e : errors)
                out.problems.push_back("escalas: " + e);
            if (!errors.empty())
                return out;
        }
        // o que a Mistura não reconhece como escala ("coco de roda", "ondas do
        // mar"): não se perde, vai para as Palavras (a árvore, o léxico, a rede)
        std::string promptText = q.promptText;
        if (opt.scaleArchive && !opt.mix.empty())
        {
            std::vector<std::string> kept;
            for (const auto& term : opt.mix)
                if (composition::fitArchive(*opt.scaleArchive, {}, term, 1).error.empty())
                    kept.push_back(term);
                else
                {
                    promptText += (promptText.empty() ? "" : ", ") + term;
                    mixNotes.push_back("mistura: \"" + term + "\" não é uma escala do arquivo → lida como palavras");
                }
            opt.mix = kept;
        }
        if (q.useListening)
            opt.tendencies = composition::tendenciesFrom(loadJudgements(dataRoot));
        out.judgementsUsed = opt.tendencies.judgements;
        if (q.useRules)
        {
            const juce::File rf = q.rulesPath.empty() ? juce::File(u8(dataRoot)).getChildFile("regras.txt")
                                                      : juce::File(u8(q.rulesPath));
            if (!q.rulesPath.empty() && !rf.existsAsFile())
            {
                out.problems.push_back("arquivo de regras não encontrado: " + rf.getFullPathName().toStdString());
                return out;
            }
            if (rf.existsAsFile())
            {
                std::vector<std::string> errors;
                opt.constraints = composition::parseConstraints(rf.loadFileAsString().toStdString(), errors);
                if (!errors.empty())
                {
                    for (const auto& e : errors)
                        out.problems.push_back("regras: " + e);
                    out.problems.push_back("nenhuma resposta gerada: corrija " + rf.getFullPathName().toStdString());
                    return out;
                }
                out.rulesUsed = opt.constraints.lines;
                out.rulesFile = rf.getFullPathName().toStdString();
            }
        }
        // as influências (core/Influences.h): o arquivo, a memória de uso
        // (dados/memoria/influencias.json: o muito usado fica raro) e o sorteio
        std::vector<std::string> influenceLines;
        std::vector<std::string> influenceUsed;
        // o prompt da resposta (as palavras do autor, opcional): os nós da árvore
        // que ele nomeia entram nas influências; a intenção, a forma e a dança,
        // abaixo, junto com o prompt da referência
        std::vector<std::string> wordInfluences;
        composition::DeclaredIntent wordIntent;
        std::vector<std::string> semanticLines;
        if (!promptText.empty())
        {
            std::vector<std::string> errs;
            const auto arch = loadInfluences(dataRoot, errs);
            wordInfluences = composition::influencesFromWords(arch, promptText);
            wordIntent = composition::translatePrompt(promptText);
            // a ponte semântica: as palavras não lidas, na rede (no máximo 5,
            // um só tempo limite); o que vier só preenche os campos que as
            // palavras lidas deixaram em aberto
            // as palavras que já são nós da árvore ("coco") não vão para a rede
            {
                std::vector<std::string> still;
                const auto consumed = composition::influenceAliasWords(arch, promptText);  // "coco de roda": o "roda" junto
                for (const auto& w : wordIntent.unread)
                    if (composition::influencesFromWords(arch, w).empty()
                        && std::find(consumed.begin(), consumed.end(), w) == consumed.end())
                        still.push_back(w);
                wordIntent.unread = still;
            }
            if (q.semantic && !wordIntent.unread.empty())
            {
                step(0.12f, "consultando a rede");
                std::vector<std::string> ask(wordIntent.unread.begin(),
                                             wordIntent.unread.begin() + std::min<size_t>(5, wordIntent.unread.size()));
                std::vector<std::pair<std::string, double>> pooled;
                std::vector<std::string> stillUnread;
                // o tempo cresce com o número de palavras (a Wikidata vai duas a duas)
                const int budget = std::clamp(1200 * static_cast<int>(ask.size()), q.semanticBudgetMs, 9000);
                for (const auto& r : lookupSemantic(dataRoot, ask, defaultSemanticSources(), budget))
                {
                    if (!r.ok)
                    {
                        semanticLines.push_back("rede: \"" + r.word + "\" sem resposta (" + r.error + ")");
                        stillUnread.push_back(r.word);
                        continue;
                    }
                    std::string why;
                    (void) composition::intentFromAssociations(r.associations, &why);
                    semanticLines.push_back("rede: \"" + r.word + "\" → " + r.english + " [" + (r.cached ? "memória; " : "") + r.via
                                            + "]" + (why.empty() ? " — nenhuma associação deu campo" : ": " + why));
                    // cada palavra pesa igual (a de muitas associações não abafa as outras)
                    double total = 0.0;
                    for (const auto& a2 : r.associations)
                        total += a2.second;
                    // as associações que são nós da árvore (macumba → maracatu,
                    // samba…): até dois por palavra, como influências
                    {
                        std::string top;
                        for (std::size_t k = 0; k < std::min<std::size_t>(12, r.associations.size()); ++k)
                            top += r.associations[k].first + ", ";
                        std::string called;
                        int n = 0;
                        for (const auto& node : composition::influencesFromWords(arch, top))
                            if (n < 2 && std::find(wordInfluences.begin(), wordInfluences.end(), node) == wordInfluences.end())
                            {
                                wordInfluences.push_back(node);
                                called += (called.empty() ? "" : ", ") + node;
                                ++n;
                            }
                        if (!called.empty())
                            semanticLines.push_back("rede: \"" + r.word + "\" chamou da árvore: " + called);
                    }
                    for (const auto& a2 : r.associations)
                        pooled.push_back({ a2.first, a2.second / std::max(total, 1e-9) });
                }
                for (size_t i = ask.size(); i < wordIntent.unread.size(); ++i)
                    stillUnread.push_back(wordIntent.unread[i]);
                const auto net = composition::intentFromAssociations(pooled);
                auto& f = wordIntent.fields;
                const auto fillF = [](float& dst, float src) { if (dst < 0.0f && src >= 0.0f) dst = src; };
                const auto fillI = [](int& dst, int src) { if (dst < 0 && src >= 0) dst = src; };
                fillF(f.energy, net.energy);
                fillF(f.density, net.density);
                fillF(f.brightness, net.brightness);
                fillF(f.harmonicity, net.harmonicity);
                fillF(f.dance, net.dance);
                fillI(f.mood, net.mood);
                fillI(f.narrative, net.narrative);
                fillI(f.ensemble, net.ensemble);
                fillI(f.rockStyle, net.rockStyle);
                if (!pooled.empty())
                    semanticLines.push_back("rede, o cálculo: " + composition::intentFieldsText(net));
                wordIntent.unread = stillUnread;
            }
        }
        std::vector<std::string> askedInfluences = q.influences;
        for (const auto& w : wordInfluences)
            if (std::find(askedInfluences.begin(), askedInfluences.end(), w) == askedInfluences.end())
                askedInfluences.push_back(w);
        if (!askedInfluences.empty() || !q.exclude.empty())
        {
            std::vector<std::string> errors;
            const auto archive = loadInfluences(dataRoot, errors);
            for (const auto& e : errors)
                out.problems.push_back("influências: " + e);
            std::map<std::string, int> usage;
            const auto memFile = juce::File(u8(dataRoot)).getChildFile("memoria").getChildFile("influencias.json");
            if (const auto mv = juce::JSON::parse(memFile); mv.getDynamicObject())
                for (const auto& p : mv.getDynamicObject()->getProperties())
                    usage[p.name.toString().toStdString()] = static_cast<int>(p.value);
            std::vector<std::string> asked;
            for (const auto& r : askedInfluences)
                if (r != "auto")
                    asked.push_back(r);
            // só --sem, sem --influencias: só a exclusão (a árvore não sorteia nada)
            auto draw = std::make_shared<composition::InfluenceDraw>();
            // o mapa (dados/memoria/mapa_influencias.json): os pontos já feitos
            const auto mapFile = juce::File(u8(dataRoot)).getChildFile("memoria").getChildFile("mapa_influencias.json");
            std::vector<composition::InfluencePoint> past;
            if (const auto mv = juce::JSON::parse(mapFile); const auto* arr = mv.getArray())
                for (const auto& v : *arr)
                {
                    composition::InfluencePoint p;
                    p.dance = static_cast<float>(static_cast<double>(v["dança"]));
                    p.weight = static_cast<float>(static_cast<double>(v["peso"]));
                    p.noise = static_cast<float>(static_cast<double>(v["ruído"]));
                    p.rasgo = static_cast<float>(static_cast<double>(v["rasgo"]));
                    p.ensemble = v["conjunto"].toString().toStdString();
                    p.harmony = v["harmonia"].toString().toStdString();
                    past.push_back(p);
                }
            if (!askedInfluences.empty())
            {
                // cinco candidatos; fica o mais longe do já feito (a busca pelo não ocupado)
                std::vector<composition::InfluenceDraw> cands;
                for (std::uint64_t k = 0; k < 5; ++k)
                    cands.push_back(composition::drawInfluences(archive, asked, q.seed * 7919u + k, opt.unpredictability, usage,
                                                                q.influenceStrength, q.exclude));
                double dist = 0.0;
                const auto pick = composition::pickNovelDraw(cands, past, &dist);
                *draw = cands[pick];
                if (!past.empty())
                {
                    char b[200];
                    std::snprintf(b, sizeof b, "busca pelo não ocupado: 5 sorteios, o escolhido (o %zu.º) fica a %.2f do mais perto entre %zu já feitos",
                                  pick + 1, dist, past.size());
                    draw->lines.push_back(b);
                }
                // o ponto novo entra no mapa (os 300 mais recentes)
                const auto pt = composition::influencePoint(*draw);
                juce::Array<juce::var> arr;
                const std::size_t from = past.size() >= 300 ? past.size() - 299 : 0;
                for (std::size_t i = from; i <= past.size(); ++i)
                {
                    const auto& p = i < past.size() ? past[i] : pt;
                    auto o = juce::var(new juce::DynamicObject());
                    o.getDynamicObject()->setProperty("dança", p.dance);
                    o.getDynamicObject()->setProperty("peso", p.weight);
                    o.getDynamicObject()->setProperty("ruído", p.noise);
                    o.getDynamicObject()->setProperty("rasgo", p.rasgo);
                    o.getDynamicObject()->setProperty("conjunto", u8(p.ensemble));
                    o.getDynamicObject()->setProperty("harmonia", u8(p.harmony));
                    arr.add(o);
                }
                mapFile.getParentDirectory().createDirectory();
                (void) mapFile.replaceWithText(juce::JSON::toString(arr));
            }
            else
                for (const auto& r : q.exclude)
                {
                    draw->presence[r] = false;
                    draw->lines.push_back("o autor tirou: " + r);
                }
            influenceLines = draw->lines;
            for (const auto& l : composition::applyInfluenceOptions(opt, *draw))
                influenceLines.push_back(l);
            opt.influenceDraw = draw;
            influenceUsed = draw->used;
            // a memória: cada chave usada mais uma vez
            auto obj = juce::var(new juce::DynamicObject());
            for (auto& [k, c] : usage)
                obj.getDynamicObject()->setProperty(u8(k), c);
            for (const auto& k : draw->used)
                obj.getDynamicObject()->setProperty(u8(k), usage[k] + 1);
            memFile.getParentDirectory().createDirectory();
            (void) memFile.replaceWithText(juce::JSON::toString(obj));
        }
        // a intenção (core/Intent.h, Prompts.h): o prompt da referência, se
        // houver, traduzido; a escuta no mesmo vocabulário; o pedido × o ouvido
        std::vector<std::string> intentLines;
        if (!promptText.empty())
        {
            // as palavras da resposta vencem o prompt guardado da referência
            const auto reading = composition::readReference(heard.frames, heard.onsets, heard.duration);
            const auto inferred = composition::inferIntent(heard.frames, heard.onsets, heard.duration, reading);
            intentLines.push_back("palavras da resposta: \"" + promptText + "\"");
            for (const auto& l : semanticLines)
                intentLines.push_back(l);
            for (const auto& l : composition::applyIntent(opt, wordIntent, inferred))
                intentLines.push_back(l);
            if (!wordIntent.unread.empty())
            {
                std::string un;
                for (const auto& w : wordIntent.unread)
                    un += (un.empty() ? "" : ", ") + w;
                intentLines.push_back("palavras não lidas (sem campo nem nó da árvore): " + un);
            }
            if (!wordInfluences.empty())
            {
                std::string nodes;
                for (const auto& w : wordInfluences)
                    nodes += (nodes.empty() ? "" : ", ") + w;
                intentLines.push_back("as palavras chamaram da árvore: " + nodes);
            }
            // a dança pedida: o eixo de caráter (sobre as influências, ou sozinho)
            if (wordIntent.fields.dance >= 0.0f)
            {
                auto d2 = opt.influenceDraw ? std::make_shared<composition::InfluenceDraw>(*opt.influenceDraw)
                                            : std::make_shared<composition::InfluenceDraw>();
                d2->axes["dança"] = wordIntent.fields.dance;
                opt.influenceDraw = d2;
            }
        }
        else if (q.usePrompt)
        {
            PromptRecord pr;
            if (loadPrompt(dataRoot, q.id, pr))
            {
                const auto reading = composition::readReference(heard.frames, heard.onsets, heard.duration);
                pr.inferred = composition::inferIntent(heard.frames, heard.onsets, heard.duration, reading);
                pr.hasInferred = true;
                intentLines = composition::applyIntent(opt, pr.declared, pr.inferred);
                if (pr.hasOriginal)
                    intentLines.insert(intentLines.begin(), "prompt original (" + pr.source + "): \"" + pr.original + "\"");
                std::vector<std::string> ignore;
                (void) savePrompt(dataRoot, pr, ignore);  // a camada ouvida, regravada
            }
        }
        if (q.lineCuts.empty())
        {
            step(0.18f, "compondo a partitura");
            score = composition::compose(heard.frames, heard.onsets, heard.duration, opt);
            step(0.30f, "sintetizando as vozes");
            std::vector<float> L, R;
            renderScore(score, kOutputRate, L, R, q.stems ? &voiceStems : nullptr, [&step](float f) {
                step(0.30f + 0.52f * f, "sintetizando as vozes");
            });
            if (L.size() > total)
            {
                total = L.size();
                audio.setSize(2, static_cast<int>(total), false, true, false);
                audio.clear();
            }
            for (auto& st : voiceStems)
            {
                st.left.resize(total, 0.0f);
                st.right.resize(total, 0.0f);
            }
            std::copy(L.begin(), L.begin() + static_cast<std::ptrdiff_t>(std::min(L.size(), total)), audio.getWritePointer(0));
            std::copy(R.begin(), R.begin() + static_cast<std::ptrdiff_t>(std::min(R.size(), total)), audio.getWritePointer(1));
        }
        else if (step(0.25f, "compondo e sintetizando as linhas"), !composeLines(dataRoot, q, heard, opt, total, audio, score, lineInfo, out.problems, q.stems ? &voiceStems : nullptr))
            return out;
        for (auto it = mixNotes.rbegin(); it != mixNotes.rend(); ++it)
            score.marks.insert(score.marks.begin(), { 0.0, *it });
        for (auto it = intentLines.rbegin(); it != intentLines.rend(); ++it)
            score.marks.insert(score.marks.begin(), { 0.0, "intenção · " + *it });
        for (auto it = influenceLines.rbegin(); it != influenceLines.rend(); ++it)
            score.marks.insert(score.marks.begin(), { 0.0, "influências · " + *it });
        for (const auto& m : score.marks)
            out.formMarks.push_back(m.text);
    }
    else
    {
        EnergySynth synth(kOutputRate, q.seed);
        for (const auto& s : steps)
        {
            synth.setDecision(s.decision);
            auto i0 = static_cast<std::size_t>(s.start * kOutputRate + 0.5);
            const auto i1 = std::min(total, static_cast<std::size_t>(s.end * kOutputRate + 0.5));
            while (i0 < i1)
            {
                const auto n = std::min<std::size_t>(512, i1 - i0);
                synth.render(audio.getWritePointer(0, static_cast<int>(i0)), audio.getWritePointer(1, static_cast<int>(i0)),
                             static_cast<int>(n));
                i0 += n;
            }
        }
    }
    // o barramento de master: a mesma curva de ganho na mistura e nos stems
    if (q.master)
    {
        std::vector<float> L(total), R(total);
        std::copy(audio.getReadPointer(0), audio.getReadPointer(0) + total, L.begin());
        std::copy(audio.getReadPointer(1), audio.getReadPointer(1) + total, R.begin());
        std::vector<std::pair<std::vector<float>*, std::vector<float>*>> stems;
        for (auto& lo : lineInfo)
            stems.push_back({ &lo.left, &lo.right });
        for (auto& st : voiceStems)
            stems.push_back({ &st.left, &st.right });
        step(0.84f, "master");
        out.masterReport = applyMaster(L, R, kOutputRate, stems);
        std::copy(L.begin(), L.end(), audio.getWritePointer(0));
        std::copy(R.begin(), R.end(), audio.getWritePointer(1));
    }
    Analyzer self(kOutputRate);
    {
        std::vector<float> mono(total);
        for (std::size_t k = 0; k < total; ++k)
            mono[k] = 0.5f * (audio.getSample(0, static_cast<int>(k)) + audio.getSample(1, static_cast<int>(k)));
        self.push(mono.data(), mono.size());
        self.finish();
    }

    step(0.88f, "gravando");
    // 5. gravar: sessão, WAV, decisões
    const auto stamp = juce::String(now).retainCharacters("0123456789").substring(0, 14);
    const std::string sessionId = "sessao-" + stamp.toStdString() + "-" + strategy::modeName(q.mode)
                                + (q.score ? "" : "-energia") + "-s" + std::to_string(q.seed);
    const auto sessionDir = juce::File(u8(dataRoot)).getChildFile("sessoes").getChildFile(u8(sessionId));
    const auto wav = sessionDir.getChildFile("saidas").getChildFile(
        u8(q.id) + "-" + strategy::modeName(q.mode) + "-s" + juce::String(static_cast<juce::uint64>(q.seed)) + ".wav");
    wav.getParentDirectory().createDirectory();
    {
        juce::TemporaryFile tmp(wav);
        {
            juce::WavAudioFormat format;
            std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream>(tmp.getFile());
            auto writer = format.createWriterFor(stream, juce::AudioFormatWriterOptions {}
                                                             .withSampleRate(kOutputRate).withNumChannels(2)
                                                             .withBitsPerSample(24));
            if (!writer || !writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()))
            {
                out.problems.push_back("falha ao gravar o WAV");
                return out;
            }
        }
        if (!tmp.overwriteTargetFileWithTemporary())
        {
            out.problems.push_back("falha ao gravar " + wav.getFullPathName().toStdString());
            return out;
        }
    }
    // os stems das linhas, ao lado da resposta (saidas/linhas/)
    for (std::size_t k = 0; k < lineInfo.size(); ++k)
    {
        auto& lo = lineInfo[k];
        const auto stem = sessionDir.getChildFile("saidas").getChildFile("linhas").getChildFile(
            juce::String(static_cast<int>(k + 1)) + "-" + u8(slug(lo.name)) + ".wav");
        stem.getParentDirectory().createDirectory();
        juce::AudioBuffer<float> b(2, static_cast<int>(total));
        b.copyFrom(0, 0, lo.left.data(), static_cast<int>(total));
        b.copyFrom(1, 0, lo.right.data(), static_cast<int>(total));
        std::vector<float>().swap(lo.left);
        std::vector<float>().swap(lo.right);
        juce::TemporaryFile tmp(stem);
        {
            juce::WavAudioFormat format;
            std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream>(tmp.getFile());
            auto writer = format.createWriterFor(stream, juce::AudioFormatWriterOptions {}
                                                             .withSampleRate(kOutputRate).withNumChannels(2)
                                                             .withBitsPerSample(24));
            if (!writer || !writer->writeFromAudioSampleBuffer(b, 0, b.getNumSamples()))
            {
                out.problems.push_back("falha ao gravar o stem " + stem.getFullPathName().toStdString());
                return out;
            }
        }
        if (!tmp.overwriteTargetFileWithTemporary())
        {
            out.problems.push_back("falha ao gravar " + stem.getFullPathName().toStdString());
            return out;
        }
        out.stemPaths.push_back(stem.getFullPathName().toStdString());
    }
    // os stems das vozes (--stems sim), em saidas/stems/, um de cada vez, e
    // o LEIA-ME: quanto cada voz soa e em que nível (um stem tem silêncio
    // onde a voz não toca, e fica no nível da mistura, para somar)
    juce::String readme;
    if (!voiceStems.empty())
        readme << "Stems desta resposta (trioio_biblioteca responder --stems sim)\n\n"
               << "Para OUVIR, use a mistura (a pasta de cima). Os stems são para a DAW:\n"
               << "- cada um só tem som onde a sua voz toca (o resto é silêncio);\n"
               << "- todos estão no nível em que entram na mistura (não normalizados):\n"
               << "  somados em 0 dB, refazem a mistura (só a cola de barramento fica de fora).\n\n"
               << "stem            soa em   RMS onde soa   pico\n";
    for (auto& st : voiceStems)
    {
        {
            // a atividade: quadros de 100 ms acima de −50 dBFS
            const std::size_t hop = static_cast<std::size_t>(0.1 * kOutputRate);
            std::size_t frames = 0, active = 0;
            double activeEnergy = 0.0;
            float peak = 0.0f;
            for (std::size_t i = 0; i + hop <= total; i += hop)
            {
                double e = 0.0;
                for (std::size_t j = i; j < i + hop; ++j)
                {
                    const double m = 0.5 * (st.left[j] + st.right[j]);
                    e += m * m;
                    peak = std::max({ peak, std::abs(st.left[j]), std::abs(st.right[j]) });
                }
                e /= static_cast<double>(hop);
                ++frames;
                if (e > 1e-5)
                {
                    ++active;
                    activeEnergy += e;
                }
            }
            char line[160];
            std::snprintf(line, sizeof line, "%-14s %5.0f %%   %7.1f dB   %5.1f dB\n", st.name.c_str(),
                          frames ? 100.0 * static_cast<double>(active) / static_cast<double>(frames) : 0.0,
                          active ? 10.0 * std::log10(activeEnergy / static_cast<double>(active)) : -120.0,
                          20.0 * std::log10(peak + 1e-9f));
            readme << line;
        }
        const auto f = sessionDir.getChildFile("saidas").getChildFile("stems").getChildFile(u8(st.name) + ".wav");
        f.getParentDirectory().createDirectory();
        juce::AudioBuffer<float> b(2, static_cast<int>(total));
        b.copyFrom(0, 0, st.left.data(), static_cast<int>(total));
        b.copyFrom(1, 0, st.right.data(), static_cast<int>(total));
        std::vector<float>().swap(st.left);
        std::vector<float>().swap(st.right);
        juce::TemporaryFile tmp(f);
        {
            juce::WavAudioFormat format;
            std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream>(tmp.getFile());
            auto writer = format.createWriterFor(stream, juce::AudioFormatWriterOptions {}
                                                             .withSampleRate(kOutputRate).withNumChannels(2)
                                                             .withBitsPerSample(24));
            if (!writer || !writer->writeFromAudioSampleBuffer(b, 0, b.getNumSamples()))
            {
                out.problems.push_back("falha ao gravar o stem " + f.getFullPathName().toStdString());
                return out;
            }
        }
        if (!tmp.overwriteTargetFileWithTemporary())
        {
            out.problems.push_back("falha ao gravar " + f.getFullPathName().toStdString());
            return out;
        }
        out.stemPaths.push_back(f.getFullPathName().toStdString());
    }
    if (readme.isNotEmpty())
        sessionDir.getChildFile("saidas").getChildFile("stems").getChildFile("LEIA-ME.txt").replaceWithText(readme);
    const auto decisions = sessionDir.getChildFile("decisoes.json");
    auto dj = decisionsJson(sessionId, q, steps);
    if (q.score)
    {
        auto* o = dj.getDynamicObject();
        o->setProperty("engine", "partitura");
        o->setProperty("pitchSource", composition::pitchSourceName(q.pitch));
        o->setProperty("timeMode", composition::timeModeName(q.time));
        o->setProperty("unpredictability", q.unpredictability);
        o->setProperty("gestures", static_cast<int>(score.gestures.size()));
        o->setProperty("bpm", score.bpm);
        o->setProperty("listeningJudgements", out.judgementsUsed);  // quantos julgamentos pesaram
        juce::Array<juce::var> rules;
        for (const auto& l : out.rulesUsed)
            rules.add(u8(l));
        o->setProperty("rules", rules);  // as regras do autor que valeram
        juce::Array<juce::var> marks, notes;
        for (const auto& m : score.marks)
        {
            auto mk = juce::var(new juce::DynamicObject());
            mk.getDynamicObject()->setProperty("time", m.time);
            mk.getDynamicObject()->setProperty("text", u8(m.text));
            marks.add(mk);
        }
        for (const auto& n : score.notes)
        {
            auto nt = juce::var(new juce::DynamicObject());
            auto* p = nt.getDynamicObject();
            p->setProperty("time", n.time);
            p->setProperty("duration", n.duration);
            p->setProperty("pitch", n.pitch);
            p->setProperty("glide", n.glide);
            p->setProperty("velocity", n.velocity);
            p->setProperty("voice", n.voice);
            p->setProperty("timbre", static_cast<int>(n.timbre));
            if (n.source >= 0.0)
                p->setProperty("source", n.source);
            notes.add(nt);
        }
        o->setProperty("form", marks);
        o->setProperty("notes", notes);
        if (q.game)
        {
            const auto rulesJson = [](const std::vector<composition::GameRule>& rules) {
                juce::Array<juce::var> arr;
                for (const auto& r : rules)
                {
                    auto v = juce::var(new juce::DynamicObject());
                    auto* p = v.getDynamicObject();
                    p->setProperty("id", r.id);
                    p->setProperty("when", r.when);
                    p->setProperty("whenName", u8(r.when == 0 ? std::string("sempre") : composition::conditionName(static_cast<composition::Condition>(r.when))));
                    p->setProperty("action", static_cast<int>(r.action));
                    p->setProperty("actionName", u8(composition::gameActionName(r.action)));
                    p->setProperty("material", r.material);
                    p->setProperty("span", r.span);
                    p->setProperty("from", r.from);
                    p->setProperty("to", std::min(r.to, 1e7));
                    p->setProperty("origin", u8(r.origin));
                    p->setProperty("why", u8(r.why));
                    p->setProperty("fired", r.fired);
                    p->setProperty("changed", r.changed);
                    {
                        juce::Array<juce::var> at;
                        for (double t : r.firedAt)
                            at.add(t);
                        p->setProperty("firedAt", at);
                    }
                    p->setProperty("hidden", r.hidden);
                    if (r.soundMeasured)
                    {
                        p->setProperty("soundDb", r.soundDb);
                        p->setProperty("soundBright", r.soundBright);
                        p->setProperty("soundOnsets", r.soundOnsets);
                        p->setProperty("soundSpectral", r.soundSpectral);
                    }
                    p->setProperty("apocryphal", r.apocryphal);
                    if (r.apocryphal)
                    {
                        p->setProperty("trueOrigin", u8(r.trueOrigin));  // a verdade fica no arquivo
                        p->setProperty("trueWhy", u8(r.trueWhy));
                    }
                    p->setProperty("text", u8(composition::gameRuleText(r)));
                    arr.add(v);
                }
                return arr;
            };
            o->setProperty("game", rulesJson(score.game));
            o->setProperty("nearGame", rulesJson(score.nearGame));
            {
                // o estado do jogo ao longo da peça (docs/JOGO.md §5)
                juce::Array<juce::var> st;
                for (const auto& m : score.gameState)
                {
                    auto v = juce::var(new juce::DynamicObject());
                    auto* p = v.getDynamicObject();
                    p->setProperty("time", m.time);
                    p->setProperty("event", u8(m.event));
                    p->setProperty("energy", m.state.energy);
                    p->setProperty("density", m.state.density);
                    p->setProperty("memory", m.state.memory);
                    p->setProperty("coherence", m.state.coherence);
                    p->setProperty("surprise", m.state.surprise);
                    p->setProperty("hidden", m.state.hidden);
                    p->setProperty("collapse", m.state.collapse);
                    p->setProperty("freedom", m.state.freedom);
                    st.add(v);
                }
                o->setProperty("gameState", st);
            }
            o->setProperty("gameMemory", juce::String(static_cast<int>(gameKeptN)) + " ficaram · " + juce::String(static_cast<int>(gameCompostN))
                                             + " no adubo");
        }
        if (q.master)
        {
            const auto& m = out.masterReport;
            auto mv = juce::var(new juce::DynamicObject());
            auto* mp = mv.getDynamicObject();
            mp->setProperty("lufsBefore", m.lufsBefore);
            mp->setProperty("lufsAfter", m.lufsAfter);
            mp->setProperty("truePeakBefore", m.truePeakBefore);
            mp->setProperty("truePeakAfter", m.truePeakAfter);
            mp->setProperty("compressionMaxDb", m.compressionMaxDb);
            mp->setProperty("limiterMaxDb", m.limiterMaxDb);
            mp->setProperty("loudnessGainDb", m.loudnessGainDb);
            o->setProperty("master", mv);
        }
        if (!lineInfo.empty())
        {
            // de que linha é cada nota, e o que cada linha decidiu
            juce::Array<juce::var> ls;
            for (std::size_t k = 0; k < lineInfo.size(); ++k)
            {
                const auto& lo = lineInfo[k];
                for (std::size_t i = lo.firstNote; i < lo.firstNote + lo.notes && i < static_cast<std::size_t>(notes.size()); ++i)
                    notes.getReference(static_cast<int>(i)).getDynamicObject()->setProperty("line", static_cast<int>(k));
                auto lv = juce::var(new juce::DynamicObject());
                auto* p = lv.getDynamicObject();
                p->setProperty("name", u8(lo.name));
                p->setProperty("lowHz", lo.lowHz);
                p->setProperty("highHz", lo.highHz);
                p->setProperty("energyShare", lo.energyShare);
                p->setProperty("mode", strategy::modeName(lo.mode));
                p->setProperty("seed", static_cast<juce::int64>(lo.seed));
                p->setProperty("bpm", lo.bpm);
                p->setProperty("pulse", lo.ownPulse ? u8("linha") : u8("peça"));
                p->setProperty("register", juce::String(lo.lowMidi, 0) + "-" + juce::String(lo.highMidi, 0));
                p->setProperty("gainDb", lo.gainDb);
                p->setProperty("pitchFrom", u8(lo.borrowed == "nada" ? std::string("linha") : lo.borrowed));
                p->setProperty("validPulse", lo.valid.pulse);
                p->setProperty("validMelody", lo.valid.melody);
                p->setProperty("validChords", lo.valid.chords);
                if (k < out.stemPaths.size())
                    p->setProperty("stem", u8(juce::File(u8(out.stemPaths[k])).getRelativePathFrom(sessionDir).toStdString()));
                ls.add(lv);
            }
            o->setProperty("lines", ls);
            o->setProperty("lineFilter", q.lineFilter);
        }
    }
    else
        dj.getDynamicObject()->setProperty("engine", "energia");
    if (!writeAtomically(decisions, juce::JSON::toString(dj)))
    {
        out.problems.push_back("falha ao gravar " + decisions.getFullPathName().toStdString());
        return out;
    }

    // partitura em MIDI: voz 0 no canal 1, baixo no canal 2; a fração de
    // semitom vai como pitch bend (faixa ±2); o glissando não é exportado
    if (q.score)
    {
        constexpr int tpq = 960;
        const double ticksPerSecond = score.bpm / 60.0 * tpq;
        juce::MidiMessageSequence seq;
        seq.addEvent(juce::MidiMessage::tempoMetaEvent(static_cast<int>(60000000.0 / score.bpm)), 0.0);
        for (const auto& m : score.marks)
            seq.addEvent(juce::MidiMessage::textMetaEvent(6, u8(m.text)), m.time * ticksPerSecond);  // marcador
        for (const auto& n : score.notes)
        {
            const int ch = n.voice + 1;  // motivo 1, baixo 2, eco 3, memória 4, percussão 5, mosaico 6, harmonia 7 (objetos sonoros não vão para o MIDI)
            const int key = std::clamp(static_cast<int>(std::lround(n.pitch)), 0, 127);
            const double frac = n.pitch - static_cast<double>(key);
            const double on = n.time * ticksPerSecond, off = (n.time + n.duration) * ticksPerSecond;
            seq.addEvent(juce::MidiMessage::pitchWheel(ch, std::clamp(8192 + static_cast<int>(std::lround(frac / 2.0 * 8192.0)), 0, 16383)), on);
            seq.addEvent(juce::MidiMessage::noteOn(ch, key, static_cast<juce::uint8>(std::clamp(static_cast<int>(n.velocity * 127.0f), 1, 127))), on);
            seq.addEvent(juce::MidiMessage::noteOff(ch, key), off);
        }
        seq.sort();
        seq.updateMatchedPairs();
        juce::MidiFile mf;
        mf.setTicksPerQuarterNote(tpq);
        mf.addTrack(seq);
        const auto midi = wav.withFileExtension(".mid");
        juce::TemporaryFile tmp(midi);
        bool ok = false;
        {
            juce::FileOutputStream os(tmp.getFile());
            ok = os.openedOk() && mf.writeTo(os);
        }
        if (!ok || !tmp.overwriteTargetFileWithTemporary())
        {
            out.problems.push_back("falha ao gravar " + midi.getFullPathName().toStdString());
            return out;
        }
        out.midiPath = midi.getFullPathName().toStdString();
    }

    // 6. manifesto da derivação: pendente até o performer declarar o que fazer com ela
    juce::FileInputStream in(wav);
    const auto sha = juce::SHA256(in, -1).toHexString().toStdString();
    Manifest d;
    d.id = "der-" + sha.substr(0, 16);
    d.createdAt = now;
    d.updatedAt = now;
    d.kind = ManifestKind::derivation;
    d.material = MaterialType::audio;
    d.identity = { wav.getFullPathName().toStdString(), static_cast<std::uint64_t>(wav.getSize()),
                   wav.getLastModificationTime().toISO8601(true).toStdString(), "wav", heard.duration,
                   static_cast<std::uint32_t>(kOutputRate), 2, sha };
    d.origin.category = OriginCategory::otherInstrument;
    d.origin.toolName = "TRIOIO";
    d.origin.toolVersion = TRIOIO_VERSION;
    d.analysis = AnalysisState::none;
    d.relations = { { "derived_from", q.id } };
    if (!q.other.empty())
        d.relations.push_back({ "derived_from", q.other });
    const std::string relation = q.score
        ? strategy::describe(q.mode) + "; composta com motivo da referência (alturas: "
              + composition::pitchSourceName(q.pitch) + ", tempo: " + composition::timeModeName(q.time)
              + ", imprevisibilidade: " + juce::String(q.unpredictability, 2).toStdString() + ")"
        : strategy::describe(q.mode);
    std::vector<std::string> from { q.id };
    std::string declared = relation;
    if (!q.other.empty())
    {
        from.push_back(q.other);
        declared += "; mashup de materiais com " + q.other + " (" + composition::mashupLayersName(q.otherLayers)
                  + " da outra referência, reescalada no tempo desta)";
    }
    if (!lineInfo.empty())
    {
        declared += "; em " + std::to_string(lineInfo.size()) + " linhas por região de frequência (cortes";
        for (std::size_t k = 0; k + 1 < lineInfo.size(); ++k)
            declared += " " + std::to_string(static_cast<int>(std::lround(lineInfo[k].highHz))) + " Hz";
        declared += ", Linkwitz-Riley 4.ª ordem em fase zero), cada linha com compositor próprio";
        declared += q.freeLines ? ", linhas livres" : ", pulso comum";
    }
    d.derivation = Derivation { from, sessionId, "decisoes.json", q.seed, q.mode, declared, TRIOIO_VERSION };
    d.history = { { now, "gerado", "TRIOIO", sessionId } };
    // o que mais esta derivação deixou (o acervo próprio: as versões, os
    // stems, o MIDI e as decisões ficam registrados, com o caminho na sessão)
    d.history.push_back({ now, "decisões gravadas", "TRIOIO", "sessoes/" + sessionId + "/decisoes.json" });
    if (!out.midiPath.empty())
        d.history.push_back({ now, "partitura MIDI gravada", "TRIOIO",
                              "sessoes/" + sessionId + "/saidas/" + juce::File(u8(out.midiPath)).getFileName().toStdString() });
    if (!out.stemPaths.empty())
    {
        std::string names;
        for (const auto& p : out.stemPaths)
        {
            const auto f = juce::File(u8(p));
            names += (names.empty() ? "" : ", ") + f.getParentDirectory().getFileName().toStdString() + "/" + f.getFileName().toStdString();
        }
        d.history.push_back({ now, "stems gravados", "TRIOIO", "sessoes/" + sessionId + "/saidas/: " + names });
    }
    if (q.usePrompt && juce::File(u8(promptsPath(dataRoot, q.id))).existsAsFile())
        d.history.push_back({ now, "intenção usada", "TRIOIO", "indice/prompts/" + q.id + ".json" });
    const auto dm = juce::File(u8(derivationManifestDir(dataRoot))).getChildFile(u8(d.id) + ".json");
    for (const auto& v : validate(d))
        out.problems.push_back(v.field + ": " + v.message);
    if (!out.problems.empty() || !writeAtomically(dm, u8(toJson(d))))
    {
        out.problems.push_back("falha ao gravar o manifesto da derivação");
        return out;
    }

    step(0.95f, "escutando a resposta");
    // 7. o quanto a resposta segue ou contraria a referência, medido no som
    const auto respTimeline = segment(self.frames(), self.onsets(), heard.duration, q.windowSeconds);
    out.energyCorrelation = correlation(refTimeline, respTimeline, &Summary::energy);
    out.densityCorrelation = correlation(refTimeline, respTimeline, &Summary::density);
    out.brightnessCorrelation = correlation(refTimeline, respTimeline, &Summary::brightness);
    out.wavPath = wav.getFullPathName().toStdString();
    out.decisionsPath = decisions.getFullPathName().toStdString();
    out.derivationId = d.id;
    out.ok = true;
    return out;
}

} // namespace trioio::library
