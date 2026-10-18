#include "Analysis.h"

#include "AudioSource.h"

#include "LibraryImporter.h"
#include "ManifestJson.h"
#include "Provenance.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

namespace trioio::library
{
using namespace trioio::provenance;
using namespace trioio::perception;

namespace
{
juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.data(), static_cast<int>(s.size())); }

constexpr const char* kCacheSchema = "trioio.analise/1";

// O cache guarda índices de quadro, não tempos: o tempo se recalcula exato a
// partir de quadro, hop e taxa.
juce::String cacheText(const AnalysisResult& r, const std::string& sha, const AnalyzerConfig& c)
{
    auto root = juce::var(new juce::DynamicObject());
    auto* o = root.getDynamicObject();
    o->setProperty("schemaVersion", kCacheSchema);
    o->setProperty("analyzerVersion", kAnalyzerVersion);
    o->setProperty("contentSha256", u8(sha));
    o->setProperty("sampleRate", r.sampleRate);
    o->setProperty("duration", r.duration);
    o->setProperty("frameSize", c.frameSize);
    o->setProperty("hopSize", c.hopSize);
    o->setProperty("percussiveShare", r.percussiveShare);
    o->setProperty("harmonicChroma", r.harmonicChroma);
    juce::Array<juce::var> rms, cen, spr, fla, flu, pfl, flo, fhi, sil, chr, bch, pk, f0, per, mhz, msa, oi, os;
    for (const auto& f : r.frames)
    {
        rms.add(f.rmsDb);
        cen.add(f.centroidHz);
        spr.add(f.spreadHz);
        fla.add(f.flatness);
        flu.add(f.flux);
        pfl.add(f.percFlux);
        flo.add(f.fluxLow);
        fhi.add(f.fluxHigh);
        sil.add(f.silent ? 1 : 0);
        for (const float c : f.chroma)
            chr.add(c);
        for (const float c : f.bassChroma)
            bch.add(c);
        for (const float p : f.peaksHz)
            pk.add(p);
        f0.add(f.f0Hz);
        per.add(f.periodicity);
        mhz.add(f.melodyHz);
        msa.add(f.melodySalience);
    }
    const double hopSeconds = c.hopSize / r.sampleRate;
    const double first = (c.frameSize / 2.0) / r.sampleRate;
    for (const auto& on : r.onsets)
    {
        oi.add(static_cast<int>(std::llround((on.time - first) / hopSeconds)));
        os.add(on.strength);
    }
    auto frames = juce::var(new juce::DynamicObject());
    frames.getDynamicObject()->setProperty("rmsDb", rms);
    frames.getDynamicObject()->setProperty("centroidHz", cen);
    frames.getDynamicObject()->setProperty("spreadHz", spr);
    frames.getDynamicObject()->setProperty("flatness", fla);
    frames.getDynamicObject()->setProperty("flux", flu);
    frames.getDynamicObject()->setProperty("percFlux", pfl);
    frames.getDynamicObject()->setProperty("fluxLow", flo);
    frames.getDynamicObject()->setProperty("fluxHigh", fhi);
    frames.getDynamicObject()->setProperty("silent", sil);
    frames.getDynamicObject()->setProperty("chroma", chr);   // 12 por quadro, em sequência
    frames.getDynamicObject()->setProperty("bassChroma", bch);  // 12 por quadro, em sequência
    frames.getDynamicObject()->setProperty("peaksHz", pk);   // kPeaks por quadro, em sequência
    frames.getDynamicObject()->setProperty("f0Hz", f0);
    frames.getDynamicObject()->setProperty("periodicity", per);
    frames.getDynamicObject()->setProperty("melodyHz", mhz);
    frames.getDynamicObject()->setProperty("melodySalience", msa);
    o->setProperty("frames", frames);
    auto onsets = juce::var(new juce::DynamicObject());
    onsets.getDynamicObject()->setProperty("frame", oi);
    onsets.getDynamicObject()->setProperty("strength", os);
    o->setProperty("onsets", onsets);
    // 0 = o JUCE escolhe as casas para a releitura ser exata. (O parâmetro
    // conta casas DECIMAIS, não algarismos: um número fixo cortava a
    // precisão dos valores pequenos e o cache não voltava idêntico.)
    return juce::JSON::toString(root, true, 0);
}

bool readCache(const juce::File& f, const std::string& sha, AnalysisResult& r)
{
    if (!f.existsAsFile())
        return false;
    const auto v = juce::JSON::parse(f);
    if (v["schemaVersion"].toString() != kCacheSchema || static_cast<int>(v["analyzerVersion"]) != kAnalyzerVersion
        || v["contentSha256"].toString() != u8(sha))
        return false;
    const AnalyzerConfig c;
    if (static_cast<int>(v["frameSize"]) != c.frameSize || static_cast<int>(v["hopSize"]) != c.hopSize)
        return false;
    r.sampleRate = v["sampleRate"];
    r.duration = v["duration"];
    r.percussiveShare = v.hasProperty("percussiveShare") ? static_cast<float>(static_cast<double>(v["percussiveShare"])) : -1.0f;
    r.harmonicChroma = static_cast<bool>(v["harmonicChroma"]);
    const auto& fr = v["frames"];
    const auto* rms = fr["rmsDb"].getArray();
    const auto* cen = fr["centroidHz"].getArray();
    const auto* spr = fr["spreadHz"].getArray();
    const auto* fla = fr["flatness"].getArray();
    const auto* flu = fr["flux"].getArray();
    const auto* pfl = fr["percFlux"].getArray();
    const auto* flo = fr["fluxLow"].getArray();
    const auto* fhi = fr["fluxHigh"].getArray();
    const auto* sil = fr["silent"].getArray();
    const auto* chr = fr["chroma"].getArray();
    const auto* bch = fr["bassChroma"].getArray();
    const auto* pk = fr["peaksHz"].getArray();
    const auto* f0 = fr["f0Hz"].getArray();
    const auto* per = fr["periodicity"].getArray();
    const auto* mhz = fr["melodyHz"].getArray();
    const auto* msa = fr["melodySalience"].getArray();
    if (!rms || !cen || !spr || !fla || !flu || !pfl || !flo || !fhi || !sil || !chr || !bch || !pk || !f0 || !per || !mhz || !msa || r.sampleRate <= 0.0)
        return false;
    const int n = rms->size();
    if (cen->size() != n || spr->size() != n || fla->size() != n || flu->size() != n || pfl->size() != n || flo->size() != n || fhi->size() != n || sil->size() != n
        || chr->size() != n * 12 || bch->size() != n * 12 || pk->size() != n * kPeaks || f0->size() != n || per->size() != n || mhz->size() != n || msa->size() != n)
        return false;
    const double hopSeconds = c.hopSize / r.sampleRate;
    const double first = (c.frameSize / 2.0) / r.sampleRate;
    r.frames.clear();
    r.frames.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i)
    {
        Frame f;
        f.time = first + i * hopSeconds;
        f.rmsDb = static_cast<float>(static_cast<double>((*rms)[i]));
        f.centroidHz = static_cast<float>(static_cast<double>((*cen)[i]));
        f.spreadHz = static_cast<float>(static_cast<double>((*spr)[i]));
        f.flatness = static_cast<float>(static_cast<double>((*fla)[i]));
        f.flux = static_cast<float>(static_cast<double>((*flu)[i]));
        f.percFlux = static_cast<float>(static_cast<double>((*pfl)[i]));
        f.fluxLow = static_cast<float>(static_cast<double>((*flo)[i]));
        f.fluxHigh = static_cast<float>(static_cast<double>((*fhi)[i]));
        f.silent = static_cast<int>((*sil)[i]) != 0;
        for (int c = 0; c < 12; ++c)
        {
            f.chroma[static_cast<std::size_t>(c)] = static_cast<float>(static_cast<double>((*chr)[i * 12 + c]));
            f.bassChroma[static_cast<std::size_t>(c)] = static_cast<float>(static_cast<double>((*bch)[i * 12 + c]));
        }
        for (int p = 0; p < kPeaks; ++p)
            f.peaksHz[static_cast<std::size_t>(p)] = static_cast<float>(static_cast<double>((*pk)[i * kPeaks + p]));
        f.f0Hz = static_cast<float>(static_cast<double>((*f0)[i]));
        f.periodicity = static_cast<float>(static_cast<double>((*per)[i]));
        f.melodyHz = static_cast<float>(static_cast<double>((*mhz)[i]));
        f.melodySalience = static_cast<float>(static_cast<double>((*msa)[i]));
        r.frames.push_back(f);
    }
    const auto* oi = v["onsets"]["frame"].getArray();
    const auto* os = v["onsets"]["strength"].getArray();
    if (!oi || !os || oi->size() != os->size())
        return false;
    r.onsets.clear();
    for (int i = 0; i < oi->size(); ++i)
        r.onsets.push_back({ first + static_cast<int>((*oi)[i]) * hopSeconds,
                             static_cast<float>(static_cast<double>((*os)[i])) });
    return true;
}

bool writeAtomically(const juce::File& target, const juce::String& text)
{
    target.getParentDirectory().createDirectory();
    juce::TemporaryFile tmp(target);
    return tmp.getFile().replaceWithText(text, false, false, "\n") && tmp.overwriteTargetFileWithTemporary();
}
} // namespace

namespace
{
std::string bar(float v)
{
    const int n = static_cast<int>(std::lround(std::clamp(v, 0.0f, 1.0f) * 10.0f));
    std::string s;
    for (int i = 0; i < 10; ++i)
        s += i < n ? "█" : "·";
    return s;
}

std::string clock(double t)
{
    const int total = static_cast<int>(t);
    char b[16];
    std::snprintf(b, sizeof b, "%d:%02d", total / 60, total % 60);
    return b;
}

// a maior diferença entre dois trechos, dita em palavras
std::string whatChanged(const Summary& a, const Summary& b)
{
    // Silêncio não tem brilho nem ruído: entrar ou sair dele se diz assim,
    // não como "escurece" (achado na primeira escuta real, 2026-10-05).
    if (a.activeShare >= 0.5f && b.activeShare < 0.2f)
        return "o som se cala";
    if (a.activeShare < 0.2f && b.activeShare >= 0.5f)
        return "o som entra";
    const std::pair<float, const char*> d[] = {
        { b.energy - a.energy, b.energy > a.energy ? "a energia sobe" : "a energia cai" },
        { b.density - a.density, b.density > a.density ? "fica mais denso" : "rareia" },
        { b.brightness - a.brightness, b.brightness > a.brightness ? "fica mais brilhante" : "escurece" },
        { b.noisiness - a.noisiness, b.noisiness > a.noisiness ? "fica mais ruidoso" : "fica mais tonal" },
    };
    const auto* best = &d[0];
    const std::size_t considered = hasTimbre(a) && hasTimbre(b) ? 4 : 2;
    for (std::size_t i = 0; i < considered; ++i)
        if (std::abs(d[i].first) > std::abs(best->first))
            best = &d[i];
    return best->second;
}
} // namespace

std::string formatReport(const std::string& title, const AnalysisResult& r)
{
    std::string out;
    char line[1024];  // as barras usam 3 bytes por caractere
    const auto g = summarize(r.frames, r.onsets, 0.0, r.duration);
    out += title + "\n";
    std::snprintf(line, sizeof line, "duração %s · %.0f Hz · descritores v%d%s\n\n", clock(r.duration).c_str(),
                  r.sampleRate, kAnalyzerVersion, r.fromCache ? " (do cache)" : "");
    out += line;
    std::snprintf(line, sizeof line,
                  "energia     %s  %.2f  (%.1f dBFS médio; faixa dinâmica %.1f dB)\n"
                  "densidade   %s  %.2f  (%.2f ataques/s; %d ataques)\n"
                  "brilho      %s  %.2f  (centroide %.0f Hz)\n"
                  "ruído       %s  %.2f  (planura espectral)\n"
                  "com som     %3.0f %% do tempo\n\n",
                  bar(g.energy).c_str(), g.energy, g.meanRmsDb, g.dynamicRangeDb,
                  bar(g.density).c_str(), g.density, g.onsetsPerSecond, g.onsetCount,
                  bar(g.brightness).c_str(), g.brightness, g.centroidHz,
                  bar(g.noisiness).c_str(), g.noisiness, 100.0 * g.activeShare);
    out += line;

    // linha do tempo: ~20 trechos, nenhum menor que 1 s
    const double win = std::max(1.0, std::ceil(r.duration / 20.0));
    const auto seg = segment(r.frames, r.onsets, r.duration, win);
    out += "tempo    energia     densidade   brilho      ruído\n";
    for (const auto& s : seg)
    {
        std::snprintf(line, sizeof line, "%-7s  %s  %s  %s  %s\n", clock(s.start).c_str(), bar(s.energy).c_str(),
                      bar(s.density).c_str(), bar(s.brightness).c_str(), bar(s.noisiness).c_str());
        out += line;
    }

    // onde muda: os três maiores contrastes entre trechos vizinhos
    std::vector<std::pair<float, std::size_t>> changes;
    for (std::size_t i = 1; i < seg.size(); ++i)
        changes.push_back({ contrast(seg[i - 1], seg[i]), i });
    std::sort(changes.begin(), changes.end(), [](const auto& a, const auto& b) {
        return a.first > b.first || (a.first == b.first && a.second < b.second);
    });
    out += "\nonde muda mais:\n";
    int shown = 0;
    for (const auto& [c, i] : changes)
    {
        if (shown == 3 || c < 0.05f)
            break;
        std::snprintf(line, sizeof line, "  %s  %s  (contraste %.2f)\n", clock(seg[i].start).c_str(),
                      whatChanged(seg[i - 1], seg[i]).c_str(), c);
        out += line;
        ++shown;
    }
    if (shown == 0)
        out += "  nenhuma mudança acima de 0,05: o material é estável nessas escalas\n";
    out += "\nOs descritores medem; não interpretam. Escalas em docs/DESCRITORES.md.\n";
    return out;
}

std::string analysisCachePath(const std::string& dataRoot, const std::string& sha)
{
    return juce::File(u8(dataRoot)).getChildFile("cache").getChildFile("analise")
        .getChildFile(u8(sha) + ".json").getFullPathName().toStdString();
}

bool readReferenceMono(const std::string& dataRoot, const std::string& id, std::vector<float>& mono, double& sampleRate,
                       std::vector<std::string>& problems, double maxSeconds)
{
    mono.clear();
    const auto mf = juce::File(u8(referenceManifestDir(dataRoot))).getChildFile(u8(id) + ".json");
    auto read = fromJson(mf.loadFileAsString().toStdString());
    if (!read.manifest)
    {
        problems.push_back("manifesto não encontrado ou ilegível: " + id);
        return false;
    }
    const auto& m = *read.manifest;
    if (!permits(m, Use::analyze))
    {
        problems.push_back("análise não autorizada para " + id);
        return false;
    }
    const juce::File audio(u8(m.identity.canonicalPath));
    {
        juce::FileInputStream in(audio);
        if (!in.openedOk() || juce::SHA256(in, -1).toHexString().toStdString() != m.identity.contentSha256)
        {
            problems.push_back("o arquivo mudou ou sumiu desde a varredura; rodar trioio_biblioteca varrer antes");
            return false;
        }
    }
    const auto opened = openAudio(audio, maxSeconds + 1.0);
    auto* reader = opened.reader.get();
    if (!reader || reader->sampleRate <= 0.0 || reader->numChannels == 0)
    {
        problems.push_back("formato de áudio não lido: " + m.identity.canonicalPath);
        return false;
    }
    if (static_cast<double>(reader->lengthInSamples) / reader->sampleRate > maxSeconds)
    {
        problems.push_back("duração acima do limite");
        return false;
    }
    sampleRate = reader->sampleRate;
    const int channels = static_cast<int>(reader->numChannels);
    constexpr int block = 65536;
    juce::AudioBuffer<float> buf(channels, block);
    mono.reserve(static_cast<std::size_t>(reader->lengthInSamples));
    for (juce::int64 pos = 0; pos < reader->lengthInSamples; pos += block)
    {
        const int n = static_cast<int>(std::min<juce::int64>(block, reader->lengthInSamples - pos));
        if (!reader->read(&buf, 0, n, pos, true, true))
        {
            problems.push_back("falha de leitura em " + std::to_string(pos));
            mono.clear();
            return false;
        }
        for (int i = 0; i < n; ++i)
        {
            float acc = 0.0f;
            for (int c = 0; c < channels; ++c)
                acc += buf.getSample(c, i);
            mono.push_back(acc / static_cast<float>(channels));
        }
    }
    return true;
}

AnalysisResult analyzeReference(const std::string& dataRoot, const std::string& id, const AnalysisLimits& limits,
                                const std::atomic<bool>* cancel, const std::string& nowArg)
{
    AnalysisResult r;
    const auto mf = juce::File(u8(referenceManifestDir(dataRoot))).getChildFile(u8(id) + ".json");
    if (!mf.existsAsFile())
    {
        r.problems.push_back("manifesto não encontrado: " + id);
        return r;
    }
    auto read = fromJson(mf.loadFileAsString().toStdString());
    if (!read.manifest)
    {
        r.problems = read.errors;
        return r;
    }
    auto m = *read.manifest;

    // O portão: sem `analyze` liberado, nada é lido.
    if (!permits(m, Use::analyze))
    {
        r.problems.push_back("análise não autorizada para " + id
                             + " (declarar com: trioio_biblioteca autorizar ... --usos analyze)");
        return r;
    }
    if (m.material == MaterialType::midi)
    {
        r.problems.push_back("MIDI: os descritores de áudio não se aplicam (análise de MIDI ainda não existe)");
        return r;
    }

    const auto cacheFile = juce::File(u8(analysisCachePath(dataRoot, m.identity.contentSha256)));
    if (readCache(cacheFile, m.identity.contentSha256, r))
    {
        r.ok = true;
        r.fromCache = true;
        return r;
    }

    // O arquivo ainda é o conteúdo autorizado? Confere o SHA-256 inteiro:
    // tamanho e data não bastam (no Linux o JUCE lê a data em segundos, e um
    // arquivo reescrito no mesmo segundo com o mesmo tamanho passava).
    const juce::File audio(u8(m.identity.canonicalPath));
    const auto sameContent = [&] {
        if (!audio.existsAsFile() || static_cast<std::uint64_t>(audio.getSize()) != m.identity.sizeBytes)
            return false;
        juce::FileInputStream in(audio);
        return in.openedOk() && juce::SHA256(in, -1).toHexString().toStdString() == m.identity.contentSha256;
    };
    if (!sameContent())
    {
        r.problems.push_back("o arquivo mudou ou sumiu desde a varredura; rodar trioio_biblioteca varrer antes");
        return r;
    }

    // WAV, AIFF, FLAC, Ogg e MP3 pelo JUCE; os demais decodificados pelo
    // ffmpeg num WAV temporário, apagado ao fim (AudioSource.h)
    const auto opened = openAudio(audio, limits.maxSeconds + 1.0);  // +1: longo demais ainda é recusado abaixo
    auto* reader = opened.reader.get();
    if (!reader || reader->sampleRate <= 0.0 || reader->numChannels == 0)
    {
        r.problems.push_back("formato de áudio não lido: " + m.identity.canonicalPath
                             + (opened.error.empty() ? "" : " (" + opened.error + ")"));
        return r;
    }
    r.sampleRate = reader->sampleRate;
    r.duration = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
    if (r.duration > limits.maxSeconds)
    {
        r.problems.push_back("duração acima do limite de análise");
        return r;
    }

    const int channels = static_cast<int>(reader->numChannels);
    const int block = std::max(1024, limits.blockSamples);
    juce::AudioBuffer<float> buf(channels, block);
    std::vector<float> mono(static_cast<std::size_t>(block));
    Analyzer analyzer(reader->sampleRate);
    for (juce::int64 pos = 0; pos < reader->lengthInSamples; pos += block)
    {
        if (cancel && cancel->load(std::memory_order_relaxed))
        {
            r.problems.push_back("cancelado");
            return r;  // nada gravado
        }
        const int n = static_cast<int>(std::min<juce::int64>(block, reader->lengthInSamples - pos));
        if (!reader->read(&buf, 0, n, pos, true, true))
        {
            r.problems.push_back("falha de leitura em " + std::to_string(pos));
            return r;
        }
        // mono = média dos canais (sem ganho extra: um mono centrado não muda de nível)
        for (int i = 0; i < n; ++i)
        {
            float acc = 0.0f;
            for (int c = 0; c < channels; ++c)
                acc += buf.getSample(c, i);
            mono[static_cast<std::size_t>(i)] = acc / static_cast<float>(channels);
        }
        analyzer.push(mono.data(), static_cast<std::size_t>(n));
    }
    analyzer.finish();
    r.frames = analyzer.frames();
    r.onsets = analyzer.onsets();
    r.percussiveShare = analyzer.percussiveShare();
    r.harmonicChroma = analyzer.harmonicChroma();

    if (!writeAtomically(cacheFile, cacheText(r, m.identity.contentSha256, analyzer.config())))
    {
        r.problems.push_back("falha ao gravar o cache " + cacheFile.getFullPathName().toStdString());
        return r;
    }
    if (m.analysis != AnalysisState::cached)
    {
        const std::string now = nowArg.empty() ? juce::Time::getCurrentTime().toISO8601(true).toStdString() : nowArg;
        m.analysis = AnalysisState::cached;
        m.updatedAt = now;
        m.history.push_back({ now, "analisado", "TRIOIO",
                              "descritores v" + std::to_string(kAnalyzerVersion)
                                  + (opened.via == "ffmpeg" ? " · decodificado pelo ffmpeg (original intacto)" : "") });
        if (!writeAtomically(mf, u8(toJson(m))))
            r.problems.push_back("falha ao atualizar o manifesto " + mf.getFullPathName().toStdString());
    }
    r.ok = r.problems.empty();
    return r;
}

} // namespace trioio::library
