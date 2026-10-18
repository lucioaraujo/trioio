#include "AudioSource.h"

#include <cstdlib>
#include <cstring>

namespace trioio::library
{
namespace
{
const juce::StringArray kJuceExt { "wav", "wave", "aif", "aiff", "aifc", "flac", "ogg", "oga", "opus", "mp3" };
const juce::StringArray kFfmpegExt { "m4a", "aac", "mp4", "alac", "caf", "webm", "weba", "mka",
                                     "wma", "ac3", "amr", "3gp", "wv", "ape", "mp2" };

// texto com acento: sempre por UTF-8 explícito (regra UTF-8 do RASGO)
juce::String u8(const char* s) { return juce::String(juce::CharPointer_UTF8(s)); }

juce::String tool(const char* env, const char* fallback)
{
    const char* v = std::getenv(env);
    return v && *v ? juce::String(juce::CharPointer_UTF8(v)) : juce::String(fallback);
}

// roda um processo com tempo limite; devolve a saída (stdout + stderr)
bool run(const juce::StringArray& args, int timeoutMs, juce::String& output)
{
    juce::ChildProcess p;
    if (!p.start(args, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr))
    {
        output = u8("não foi possível executar ") + args[0] + " (instale o ffmpeg ou defina TRIOIO_FFMPEG/TRIOIO_FFPROBE)";
        return false;
    }
    output = p.readAllProcessOutput();  // pequena: -v error, sem estatísticas
    if (!p.waitForProcessToFinish(timeoutMs))
    {
        p.kill();
        output = args[0] + ": tempo limite excedido";
        return false;
    }
    if (p.getExitCode() == 0)
        return true;
    // no Linux e no macOS o processo "inicia" mesmo sem o executável (o exec
    // falha no filho, que sai sem dizer nada)
    if (output.trim().isEmpty())
        output = u8("não foi possível executar ") + args[0] + " (instale o ffmpeg ou defina TRIOIO_FFMPEG/TRIOIO_FFPROBE)";
    return false;
}

juce::File tempRoot()
{
    const char* t = std::getenv("TMPDIR");
    const juce::File dir = t && *t ? juce::File(juce::CharPointer_UTF8(t))
                                   : juce::File::getSpecialLocation(juce::File::tempDirectory);
    return dir.isDirectory() ? dir : juce::File::getSpecialLocation(juce::File::tempDirectory);
}

void registerFormats(juce::AudioFormatManager& m)
{
    // WAV, AIFF, FLAC, Ogg Vorbis, Opus e MP3 em todos os sistemas; no macOS
    // também o CoreAudio (M4A/AAC, CAF, ALAC)
    m.registerBasicFormats();
}

// O formato pelo CONTEÚDO, não pela extensão: um gravador de celular salva
// MP4/AAC como ".mp3", e o leitor de MP3 do JUCE aceitava e decodificava
// lixo (achado em 2026-10-08). Devolve a extensão que o JUCE usa para o
// formato reconhecido, ou "" (o JUCE não deve tentar; vai para o ffmpeg).
struct Kind
{
    juce::String juceExt;  // "" = não ler pelo JUCE
    juce::String name;     // para os avisos
};

Kind sniff(const juce::File& file)
{
    juce::FileInputStream in(file);
    unsigned char b[64] {};
    const auto n = in.openedOk() ? in.read(b, sizeof b) : 0;
    const auto is = [&](int at, const char* s) {
        const auto len = static_cast<int>(std::strlen(s));
        return n >= at + len && std::memcmp(b + at, s, static_cast<std::size_t>(len)) == 0;
    };
    if ((is(0, "RIFF") || is(0, "RF64")) && is(8, "WAVE")) return { "wav", "WAV" };
    if (is(0, "FORM") && (is(8, "AIFF") || is(8, "AIFC"))) return { "aiff", "AIFF" };
    if (is(0, "fLaC")) return { "flac", "FLAC" };
    if (is(0, "OggS"))
    {
        for (int i = 26; i + 8 <= n; ++i)
            if (std::memcmp(b + i, "OpusHead", 8) == 0) return { "opus", "Opus" };
        return { "ogg", "Ogg Vorbis" };
    }
    if (is(4, "ftyp"))
    {
#if JUCE_MAC
        return { "m4a", "MP4/AAC" };  // o CoreAudio lê
#else
        return { "", "MP4/AAC" };
#endif
    }
    if (is(0, "ID3")) return { "mp3", "MP3" };
    if (n >= 2 && b[0] == 0xFF && (b[1] & 0xE0) == 0xE0)
        return (b[1] & 0x06) != 0 ? Kind { "mp3", "MP3" } : Kind { "", "AAC (ADTS)" };
    return { "", "" };
}

// o leitor do JUCE escolhido pelo conteúdo (nullptr se o JUCE não lê)
std::unique_ptr<juce::AudioFormatReader> juceReader(const juce::File& file, const Kind& kind)
{
    if (kind.juceExt.isEmpty())
        return nullptr;
    juce::AudioFormatManager formats;
    registerFormats(formats);
    auto* format = formats.findFormatForFileExtension(kind.juceExt);
    if (!format)
        return nullptr;
    std::unique_ptr<juce::AudioFormatReader> r(format->createReaderFor(new juce::FileInputStream(file), true));
    return r && r->sampleRate > 0.0 && r->numChannels > 0 ? std::move(r) : nullptr;
}

// aviso quando a extensão não corresponde ao conteúdo
std::string mismatch(const juce::File& file, const Kind& kind)
{
    if (kind.name.isEmpty())
        return {};
    const auto ext = file.getFileExtension().trimCharactersAtStart(".").toLowerCase();
    const auto same = ext == kind.juceExt || (kind.juceExt == "aiff" && (ext == "aif" || ext == "aifc"))
                   || (kind.juceExt == "wav" && ext == "wave") || (kind.juceExt == "ogg" && ext == "oga")
                   || (kind.name == "MP4/AAC" && (ext == "m4a" || ext == "mp4" || ext == "aac" || ext == "3gp" || ext == "alac"))
                   || (kind.name == "AAC (ADTS)" && ext == "aac");
    return same ? std::string() : (u8("o conteúdo é ") + kind.name + u8(", não .") + ext).toStdString();
}
} // namespace

const juce::StringArray& audioExtensions()
{
    static const juce::StringArray all = [] {
        juce::StringArray a(kJuceExt);
        a.addArray(kFfmpegExt);
        return a;
    }();
    return all;
}

bool needsFfmpeg(const juce::String& extension)
{
    return kFfmpegExt.contains(extension.trimCharactersAtStart(".").toLowerCase());
}

AudioInfo probeAudio(const juce::File& file)
{
    AudioInfo info;
    const auto kind = sniff(file);
    info.warning = mismatch(file, kind);
    {
        const auto reader = juceReader(file, kind);
        if (reader)
        {
            info.ok = true;
            info.via = "juce";
            info.sampleRate = reader->sampleRate;
            info.channels = static_cast<std::uint16_t>(reader->numChannels);
            info.durationSeconds = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
            for (const auto& key : reader->metadataValues.getAllKeys())
                info.metadata.emplace_back(key.toStdString(), reader->metadataValues[key].toStdString());
            return info;
        }
    }
    // o ffprobe lê o cabeçalho do primeiro fluxo de áudio
    juce::String out;
    const bool ran = run({ tool("TRIOIO_FFPROBE", "ffprobe"), "-v", "error", "-select_streams", "a:0",
                           "-show_entries", "stream=sample_rate,channels:format=duration", "-of", "default=nw=1",
                           file.getFullPathName() },
                         30000, out);
    if (!ran)
    {
        info.error = out.trim().toStdString();
        return info;
    }
    juce::StringArray lines;
    lines.addLines(out);
    for (const auto& l : lines)
    {
        const auto key = l.upToFirstOccurrenceOf("=", false, false).trim();
        const auto value = l.fromFirstOccurrenceOf("=", false, false).trim();
        if (key == "sample_rate") info.sampleRate = value.getDoubleValue();
        else if (key == "channels") info.channels = static_cast<std::uint16_t>(value.getIntValue());
        else if (key == "duration") info.durationSeconds = value.getDoubleValue();
    }
    info.ok = info.sampleRate > 0.0 && info.channels > 0 && info.durationSeconds > 0.0;
    info.via = "ffmpeg";
    if (!info.ok)
        info.error = "sem fluxo de áudio legível";
    return info;
}

OpenedAudio openAudio(const juce::File& file, double maxSeconds, int timeoutMs)
{
    OpenedAudio o;
    o.reader = juceReader(file, sniff(file));
    if (o.reader)
    {
        o.via = "juce";
        return o;
    }
    // o ffmpeg decodifica para um WAV temporário (24 bits, canais e taxa do original)
    o.decoded = std::make_unique<juce::TemporaryFile>(tempRoot().getChildFile("trioio-decodificado.wav"));
    const auto target = o.decoded->getFile();
    juce::String out;
    const bool ran = run({ tool("TRIOIO_FFMPEG", "ffmpeg"), "-v", "error", "-nostdin", "-y", "-i", file.getFullPathName(),
                           "-map", "0:a:0", "-t", juce::String(maxSeconds, 3), "-c:a", "pcm_s24le", "-f", "wav",
                           target.getFullPathName() },
                         timeoutMs, out);
    if (!ran)
    {
        o.error = out.trim().isEmpty() ? std::string("o ffmpeg não decodificou o arquivo") : out.trim().toStdString();
        target.deleteFile();
        o.decoded.reset();
        return o;
    }
    o.reader = juceReader(target, { "wav", "WAV" });
    if (!o.reader || o.reader->sampleRate <= 0.0)
    {
        o.reader.reset();
        o.error = "o WAV decodificado pelo ffmpeg não abriu";
        target.deleteFile();
        o.decoded.reset();
        return o;
    }
    o.via = "ffmpeg";
    return o;
}

} // namespace trioio::library
