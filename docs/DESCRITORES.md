# TRIOIO — descritores mínimos (percepção, versão 1)

**Estado:** implementado em 2026-10-05.

- **Núcleo:** `src/core/Descriptors.h/.cpp`, sem JUCE.
- **Biblioteca:** `src/library/Analysis.h/.cpp`, com cache e relatório.
- **Ferramenta:** `trioio_biblioteca analisar <dados> <id>`.
- **Testes:** `tests/DescriptorsTests.cpp` e `tests/AnalysisTests.cpp`.

Primeira camada da Percepção (`TRIOIO_CEREBRO.md §3.1`). **Mede e não
decide.** O que fazer com as medidas cabe ao Contexto e à Estratégia.

## 1. Por quadro

O quadro tem 2048 amostras, com salto de 1024 (≈ 43 ms e 21 ms a 48 kHz),
janela de Hann e FFT radix-2 própria. O sinal analisado é mono: a média dos
canais.

| Medida | O que é | Para que serve |
| --- | --- | --- |
| `rmsDb` | nível RMS do quadro, em dBFS | energia e dinâmica |
| `centroidHz` | centro de massa do espectro de magnitude | brilho |
| `spreadHz` | espalhamento em torno do centroide | estreito ou largo |
| `flatness` | média geométrica ÷ média aritmética do espectro de potência, entre 50 Hz e 10 kHz, com piso de −80 dB do pico (desde o analisador v4, para áudio com perda; ver `FORMATOS_DE_AUDIO.md`) | 0 é tonal; ≈ 0,56 é ruído branco |
| `flux` | soma das subidas de `log(1 + 100·\|X\|)` entre quadros, por bin | ataques |
| `silent` | `rmsDb ≤ −70` | silêncio não tem brilho nem ruído |

**Ataques.** Um ataque é um pico local do fluxo (±3 quadros) acima de
`1,5 × mediana local (±10 quadros) + 0,02`, com pelo menos 50 ms entre dois
ataques. A busca é offline, olhando para a frente e para trás; a versão ao
vivo virá com a janela curta. O ataque é datado pelo centro do quadro. Num
trem de cliques, o erro foi de −4 ms em média e 14 ms no pior caso. Datar
pela metade do trecho novo pioraria a média, por isso o centro foi mantido.

**Versão 2 do analisador (2026-10-05):** croma (12 classes de altura) e os 4
picos espectrais mais fortes por quadro, calculados numa **janela longa de
8192 amostras** (5,9 Hz por bin), separada da janela de 2048 usada para
ataques e brilho. O croma é calculado só pelos picos, com a frequência
interpolada, porque o vazamento da janela espalharia a energia por semitons
vizinhos. Os caches da versão 1 são invalidados automaticamente. A estrutura
(pulso, compasso, seções e campo) está em `src/core/Structure` e em
`docs/COMPOSICAO.md`.

**Versão 5 do analisador (2026-10-10):** separação harmônico/percussivo
(HPSS, Fitzgerald 2010), só no offline. Cada quadro ganha `percFlux`, o
fluxo só da parte percussiva, que o pulso passa a usar. Quando a peça tem
percussão (fração percussiva ≥ 0,40), o croma passa a ser só o que se
sustenta. Os detalhes, medidas e limites estão em
[`LEITURA_E_GABARITO.md`](LEITURA_E_GABARITO.md). Os ataques, o brilho e o
f0 não mudam.

**Versão 6 (2026-10-10):** o croma do grave (`bassChroma`, de 45 a
260 Hz), que vota nos acordes, e a altura predominante por saliência
harmônica (`melodyHz`, `melodySalience`), que acha a melodia na mistura.
Ver [`LEITURA_E_GABARITO.md`](LEITURA_E_GABARITO.md).

## 2. Por trecho (`Summary`)

| Escala 0..1 | Definição |
| --- | --- |
| energia | `(nível médio + 60) / 60`, com o nível médio calculado em potência e incluindo o silêncio |
| densidade | `r / (r + 4)`, sendo `r` os ataques por segundo: 4 ataques/s dá 0,5 |
| brilho | `log2(c / 100) / log2(80)`: 100 Hz dá 0 e 8 kHz dá 1, com `c` = média do centroide **em oitavas** |
| ruído | planura média dos quadros com som |

Também entram no resumo a faixa dinâmica (p95 − p10 do nível dos quadros com
som) e a fração do tempo com som.

**Decisões tomadas na primeira escuta real (2026-10-05):**

- **Brilho em oitavas.** A média em Hz dava peso demais aos agudos: uma peça
  metade seno grave e metade ruído saía com brilho 0,94. A média em oitavas
  é a escala do ouvido. Exemplo: metade 200 Hz e metade 3,2 kHz dá cerca de
  800 Hz.
- **Silêncio não tem timbre.** Brilho e ruído só entram na comparação entre
  dois trechos que tenham som em pelo menos 20 % do tempo cada. Antes, o fim
  de uma gravação aparecia como "escurece"; agora aparece como "o som se
  cala".

## 3. Relatório e "onde muda"

A gravação é dividida em cerca de 20 trechos de duração igual, nenhum menor
que 1 s. O contraste entre dois trechos vizinhos é a distância média nas
quatro escalas, ou só em energia e densidade quando um dos dois não tem
timbre. O relatório mostra os três maiores contrastes e diz em palavras
qual escala mais mudou.

## 4. Portão, cache e garantias

- **Só analisa material liberado para `analyze`** (`permits`). Pendente,
  recusado ou revogado não é lido, nem mesmo a partir do cache.
- **Antes de uma análise nova**, confere o SHA-256 do arquivo contra o
  manifesto. Tamanho e data não bastam: no Linux, o JUCE lê a data com
  resolução de segundos, e um arquivo reescrito no mesmo segundo e com o
  mesmo tamanho passava. Se o conteúdo mudou, a resposta é "rodar `varrer`".
- **Cache** em `<dados>/cache/analise/<sha256>.json`, no esquema
  `trioio.analise/1` e na versão de analisador `kAnalyzerVersion`. Guarda
  os quadros e os ataques e volta **idêntico** quadro a quadro. O resumo não
  entra no cache: é recalculado a partir dos quadros.
- **Leitura em blocos** de 65 536 amostras, com limite de 1 h por material.
  Uma análise cancelada não grava nada. A primeira análise muda o manifesto
  para `cached` e acrescenta o evento "analisado".
- **MIDI** é recusado com aviso claro: a análise de MIDI ainda não existe.

## 5. Validação (2026-10-05)

- **Sinais com resposta conhecida:**
  - seno de 1 kHz a 0,5: −9,03 dBFS (teórico −9,03) e centroide em 1000,5 Hz;
  - ruído branco: centroide em 12 kHz e planura de 0,562 (teórico ≈ 0,561);
  - trem de cliques: 19 de 19, todos a menos de 35 ms;
  - brilho que sobe em degraus iguais por oitava;
  - degraus de −6 dB;
  - o contraste aponta a passagem de seno para ruído no segundo certo;
  - resultado idêntico com blocos de 37, de 512 e com o arquivo inteiro.
- **Mutação:** sem o limiar adaptativo, 4 checagens falham.
- **Biblioteca:**
  - o portão barra pendente e revogado, mesmo com cache presente;
  - o cache volta idêntico;
  - arquivo reescrito sem nova varredura é recusado;
  - análise cancelada não grava nada;
  - estéreo com canais iguais tem o mesmo nível que o mono.
- **Música real** (gravações do Rasgo Synth, só leitura, com dados
  descartáveis):
  - 3:42 analisados em 3 s, e a releitura do cache leva 25 ms; o cache
    ocupa cerca de 1 MB;
  - o relatório descreve a peça adensando e clareando até cerca de 1:24, e o
    fim, quando a energia cai e o som se cala;
  - a pasta ficou intacta.

## 6. Fontes de método

São conceitos publicados, implementados do zero. Nenhum código externo foi
usado.

- **FFT:** Cooley e Tukey, 1965.
- **Fluxo espectral com compressão log e picos com limiar adaptativo:**
  Bello et al., "A Tutorial on Onset Detection in Music Signals", IEEE TSAP,
  2005; Dixon, "Onset Detection Revisited", DAFx 2006; Böck, Krebs e Schedl,
  ISMIR 2012.
- **Centroide, espalhamento e planura espectral:** descritores de baixo nível
  do MPEG-7 (ISO/IEC 15938-4) e Peeters, "A large set of audio features for
  sound description", relatório do projeto CUIDADO, IRCAM, 2004.

Os metadados bibliográficos ainda serão conferidos no texto primário antes de
qualquer publicação. A entrada correspondente está no Atlas de Referência.

## 7. Próximos passos

- **Janela curta ao vivo:** ataques sem olhar para a frente, para o modo
  RESPONDER AGORA.
- **Descritores que faltam no §3.1 do Cérebro:** f0 provável, harmonicidade,
  pulsação, imagem estéreo, repetição e novidade.
- **Análise de MIDI:** densidade, registro e duração das notas.
- **Ligação com a `BrainDecision`:** pertence à Estratégia (Marco 3), não a
  esta camada.
