# TRIOIO — formatos de áudio aceitos

**Estado:** implementado em 2026-10-08 (`src/library/AudioSource.*`;
validação na seção 9 de `tests/AnalysisTests.cpp`).

## O que entra

| Formato | Quem lê | Observação |
| --- | --- | --- |
| WAV, AIFF, FLAC | JUCE | sem perda |
| Ogg Vorbis, Opus | JUCE | com perda |
| MP3 | JUCE | com perda |
| M4A/AAC, ALAC, CAF | JUCE (CoreAudio) no macOS; **ffmpeg** no Linux e no Windows | gravadores de celular, iTunes |
| WebM, WMA, AC3, AMR, 3GP, WavPack, APE, MP2, MKA | **ffmpeg** | |

A varredura (`trioio_biblioteca varrer`) considera essas extensões. O que
nenhum leitor abre vai para a lista de pulados, com o motivo.

## Como funciona

1. **O formato pelo conteúdo, não pela extensão.** Os primeiros bytes dizem
   o que o arquivo é: RIFF/WAVE, FORM/AIFF, fLaC, OggS (Vorbis ou Opus),
   `ftyp` (MP4/AAC), ID3 ou sincronia de MP3, ADTS (AAC).
   - **Achado (2026-10-08):** um gravador Android salvou MP4/AAC com
     extensão `.mp3` (o `Recording_215.mp3` do vocalise). O leitor de MP3 do
     JUCE aceitou o arquivo e decodificou **lixo**: 98,9 s em vez de 81,8 s.
     Agora o arquivo vai ao leitor certo.
   - Quando a extensão não corresponde ao conteúdo, o manifesto registra um
     `aviso` nos metadados, por exemplo "o conteúdo é MP4/AAC, não .mp3".
2. **O JUCE primeiro.** Ele lê dentro do programa, sem dependências.
3. **O ffmpeg depois, se instalado.**
   - Na varredura, o `ffprobe` lê só o cabeçalho: taxa, canais e duração.
   - Na análise, o `ffmpeg` decodifica para um **WAV temporário** (24 bits;
     canais e taxa do original) na pasta de `TMPDIR`. O WAV é apagado assim
     que a análise termina, e o original nunca é alterado.
   - O manifesto registra `decodificador: ffmpeg` nos metadados e "decodificado
     pelo ffmpeg (original intacto)" no histórico da análise.
   - **Limites:** a decodificação para na duração máxima de análise + 1 s
     (acima dela a análise é recusada) e o processo tem 10 minutos.
   - **Onde fica o ffmpeg:** é procurado no `PATH`; `TRIOIO_FFMPEG` e
     `TRIOIO_FFPROBE` apontam outro executável. Sem ele, os formatos que só
     o ffmpeg lê ficam pulados com "instale o ffmpeg".

O ffmpeg é um programa externo, chamado como processo; nenhum código dele
entra no Trioio.

## Áudio com perda e o descritor de ruído

Os codecs com perda zeram o agudo (acima de ~16 kHz) e abrem buracos no
espectro. A **planura** (média geométrica ÷ média aritmética) caía a ~0 com
um único zero, e **ruído aparecia como tom**. No teste, o ruído branco
codificado media 0,00 em MP3, Ogg e AAC.

**Correção (analisador v4):** a planura passou a ser medida entre 50 Hz e
10 kHz, com piso de −80 dB do pico do quadro. Ruído branco agora mede:

| Formato | Planura |
| --- | --- |
| FLAC | 0,56 |
| Opus | 0,55 |
| AAC | 0,55 |
| Ogg | 0,54 |
| MP3 | 0,52 |

O teórico é ≈ 0,56. Os caches de análise antigos (v3) são refeitos
automaticamente.

## Validação

A seção 9 de `tests/AnalysisTests.cpp` gera 4 s de seno e ruído, codifica
com o ffmpeg em MP3, FLAC, Ogg, Opus e M4A, e acrescenta dois arquivos de
extensão enganosa (AAC como `.mp3`, FLAC como `.wav`). Para cada um, o
teste confere:

- leitor certo;
- duração dentro de 0,1 s;
- nível do seno dentro de 1 dB;
- ruído com planura > 0,3;
- histórico citando o ffmpeg só quando ele foi usado;
- original intacto (SHA-256).

Confere também que nenhum WAV temporário fica para trás e que, sem o
ffprobe, os dois arquivos MP4/AAC são pulados com o motivo enquanto o MP3
continua lido. Sem o ffmpeg na máquina, a seção é pulada.

Primeiro uso real: v15, com o vocalise (MP4/AAC com extensão `.mp3`) e uma
faixa do Suno (MP3), em `/media/luc/4tb_21042021/TRIOIO_respostas/v15-*` (apagada em 2026-10-16: arquivo de teste).
