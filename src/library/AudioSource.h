#pragma once

// Leitura de áudio em vários formatos (docs/FORMATOS_DE_AUDIO.md).
//
// O formato é reconhecido pelo conteúdo (os primeiros bytes), não pela
// extensão: um ".mp3" que é MP4/AAC vai para o ffmpeg, e o aviso fica nos
// metadados do manifesto.
//
// Duas camadas:
//   1. leitores do JUCE (dentro do programa), sempre tentados primeiro: WAV,
//      AIFF, FLAC, Ogg Vorbis, Opus e MP3; no macOS também M4A/AAC, ALAC e
//      CAF (CoreAudio);
//   2. o ffmpeg, se instalado (fora do programa), quando o JUCE não lê:
//      M4A/AAC no Linux e no Windows, WebM, WMA e o que mais ele ler. Decodifica para um WAV temporário,
//      apagado quando o leitor é destruído. O original nunca é alterado; o
//      manifesto registra a decodificação.
//
// O ffmpeg é procurado no PATH; TRIOIO_FFMPEG e TRIOIO_FFPROBE apontam
// outro executável.

#include <juce_audio_formats/juce_audio_formats.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace trioio::library
{

// extensões que a varredura considera áudio (as do JUCE e as do ffmpeg)
[[nodiscard]] const juce::StringArray& audioExtensions();
// as que podem depender do ffmpeg (no macOS, algumas o CoreAudio lê)
[[nodiscard]] bool needsFfmpeg(const juce::String& extension);

struct AudioInfo
{
    bool ok = false;
    double sampleRate = 0.0;
    std::uint16_t channels = 0;
    double durationSeconds = 0.0;
    std::string via;    // "juce" ou "ffmpeg"
    std::string error;  // quando !ok
    std::string warning;  // extensão enganosa: "o conteúdo é MP4/AAC, não .mp3"
    std::vector<std::pair<std::string, std::string>> metadata;  // do cabeçalho (JUCE)
};

// Lê só o cabeçalho (JUCE) ou pergunta ao ffprobe: sem decodificar o arquivo.
[[nodiscard]] AudioInfo probeAudio(const juce::File& file);

// Um leitor pronto para `read`. Com o ffmpeg, guarda o WAV temporário e o
// apaga ao ser destruído (o leitor é destruído antes).
struct OpenedAudio
{
    std::unique_ptr<juce::TemporaryFile> decoded;
    std::unique_ptr<juce::AudioFormatReader> reader;
    std::string via;
    std::string error;
    [[nodiscard]] bool ok() const noexcept { return reader != nullptr; }
};

// Abre para leitura. `maxSeconds` limita a decodificação pelo ffmpeg;
// `timeoutMs`, o tempo do processo.
[[nodiscard]] OpenedAudio openAudio(const juce::File& file, double maxSeconds = 3600.0, int timeoutMs = 600000);

} // namespace trioio::library
