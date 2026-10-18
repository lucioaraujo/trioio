# Os formatos dos dados do Trioio

Os arquivos JSON que o Trioio grava na pasta de dados
(`AUDIOS_RASGO/Trioio/dados/`). Cada um declara o seu esquema.

| arquivo | esquema | onde | escrito por |
|---|---|---|---|
| `indice/referencias/*.json`, `indice/derivacoes/*.json` | `trioio.manifesto/1` | o índice | importador, `responder` |
| `sessoes/<sessão>/decisoes.json` | `trioio.decisoes/1` | cada resposta | `responder` |
| `escuta.json` | `trioio.escuta/2` | um por pasta de dados | `escutar`, a aba Sua escuta |
| `prompts.json` | (ainda não existe) | — | — |

O manifesto tem o seu documento próprio:
[`MANIFESTO_DE_PROVENIENCIA.md`](MANIFESTO_DE_PROVENIENCIA.md).

## A regra de versão

- **Campo novo, opcional, que um leitor antigo pode ignorar:** o esquema
  não muda. Quem lê trata a ausência do campo como o valor anterior. Foi o
  caso de `game`, `gameState`, `firedAt` e `sound*`, todos dentro de
  `trioio.decisoes/1`.
- **Campo que muda de sentido, de unidade ou de obrigatoriedade:** o número
  sobe (`/2`), e o leitor continua aceitando a versão anterior. Exemplo: a
  `trioio.escuta/2` acrescentou o trecho (`span`), e um julgamento sem trecho
  segue valendo para a resposta inteira (`ESCUTA.md`).
- **Nunca se reescreve um arquivo antigo para o esquema novo.** O que
  foi gravado fica como foi gravado (proveniência).

## `decisoes.json` (`trioio.decisoes/1`)

A trajetória de uma resposta: o que foi decidido e por quê. Fica ao lado do
WAV, em `sessoes/<sessão>/`.

**A identidade:**

| campo | o que é |
|---|---|
| `schemaVersion` | `"trioio.decisoes/1"` |
| `sessionId` | `sessao-AAAAMMDDhhmmss-<modo>-s<semente>` |
| `referenceId` | a referência respondida (`ref-…`) |
| `strategy` | o modo: seguir, contrapor, transfigurar, interromper |
| `seed` | a semente (texto, 64 bits) |
| `mashupWith`, `mashupLayers` | só no mashup: a outra referência e as camadas |

**O motor de energia:**

- `windowSeconds`: o tamanho da janela de decisão;
- `steps[]`: as janelas. Cada uma tem `start` e `end` e as decisões de 0 a
  1: `energy`, `density`, `brightness`, `harmonicity`,
  `transientRetention`, `mutation`, `spatialActivity`.

**A partitura (o motor de partitura):**

| campo | o que é |
|---|---|
| `engine` | o motor de síntese |
| `bpm`, `timeMode`, `pitchSource`, `unpredictability` | o pulso usado, o tempo (grade ou livre), a fonte das alturas, a imprevisibilidade |
| `form[]` | as marcas da forma: `time` (s) e `text` (seções, harmonia, jogo, estado do jogo, diálogo…) |
| `notes[]` | as notas: `time`, `duration`, `pitch` (MIDI, com fração em Partch), `velocity`, `voice` (0 motivo, 1 baixo, 2 eco, 3 memória, 4 percussão, 5 mosaico, 6 harmonia), `timbre`, `source` |
| `gestures[]` | os gestos do Terceiro Olho e os efeitos de cinema |
| `rules` | as regras do autor usadas (`regras.txt`) |
| `listeningJudgements` | quantos julgamentos pesaram nesta resposta |
| `master` | com `--master sim`: `lufsBefore` e `lufsAfter`, `truePeakBefore` e `truePeakAfter`, `gainDb`, `compressionMaxDb`, `limiterMaxDb` |
| `lines[]` | no modo linhas: cada linha (`name`, `lowHz`, `highHz`, `energyShare`, `mode`, `seed`, `pulse`, a validade, `stem`) |

**O jogo** (com `--jogo sim`; [`JOGO.md`](JOGO.md) §9):

- `game[]`: as regras em jogo.
  - **A regra:** `id`, `when` e `whenName` (a condição), `action` e
    `actionName`, `material`, `span` (s), `from` e `to` (o trecho em que
    vale).
  - **A origem:** `origin` e `why` (com `trueOrigin` e `trueWhy` quando a
    origem revelada é apócrifa); `hidden`, `apocryphal`; `text` (a frase
    da regra).
  - **O que ela fez:** `fired`, `changed` e `firedAt[]` (o começo de cada
    disparo, em segundos).
  - **A medida no som:** `soundDb`, `soundBright`, `soundOnsets` e
    `soundSpectral`.
- `nearGame[]`: as regras que quase entraram (a leitura apócrifa), no
  mesmo formato, com o motivo em `why`.
- `gameState[]`: o estado do jogo depois de cada disparo.
  - **Quando e o quê:** `time`, `event`.
  - **Os recursos, de 0 a 1:** `energy`, `density`, `memory`,
    `coherence`, `surprise`, `hidden`, `collapse`, `freedom`.
- `gameMemory`: as regras que ficaram e as que foram para o adubo, em texto.

**Quem lê:**

- a aba Sua escuta (as notas, a forma e as regras);
- a memória do jogo (`game` e `firedAt`);
- a sessão longa (o último `gameState`);
- a retenção (só pelo índice).

## `escuta.json` (`trioio.escuta/2`)

Os julgamentos do autor. Um por pasta de dados. **Nunca é apagado.**

- `schema`: `"trioio.escuta/2"`.
- `judgements[]`, na ordem em que foram dados:
  - **o julgamento:** `derivation` (a resposta julgada, `der-…`),
    `verdict` (aceitar, rejeitar, transformar), `note` (a palavra livre do
    autor: o jogo lê nela a regra do autor, [`JOGO.md`](JOGO.md) §9.3) e
    `when` (ISO 8601);
  - **o que a resposta usou:** `timbres[]` (`family`, `notes`),
    `eyeChoices[]`, `room` e `roomName`;
  - **o trecho** (desde `/2`): `span`, a lista `[início, fim]` em
    segundos. Quando o trecho veio de `--secao`, vem também `section` ("3"
    ou "A"). Sem `span`, o julgamento vale para a resposta inteira.

O formato completo e a história estão em [`ESCUTA.md`](ESCUTA.md).

## `prompts.json`

Ainda não existe. Ele entra com os prompts e as intenções do Marco 2. O
manifesto já reserva o vínculo (`promptsRef`).
