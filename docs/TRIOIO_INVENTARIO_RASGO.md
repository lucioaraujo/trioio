# TRIOIO — inventário de código RASGO

**Data:** 2026-08-12  
**Uso:** planejamento interno; nenhum outro projeto é alterado por este
documento.

## 1. Princípio de reutilização

O TRIOIO poderá reutilizar código próprio do RASGO quando houver
compatibilidade técnica e conceitual. Referências externas, código de terceiros,
modelos e interfaces de outros projetos não entram como código reutilizável.

Antes de integrar qualquer componente, registrar:

- origem exata;
- licença e autoria;
- dependências;
- contrato de thread;
- custo de CPU e memória;
- adaptação conceitual necessária;
- testes que comprovam a integração.

## 2. Mapa de reaproveitamento

| Origem | Já existe | Possível papel no TRIOIO |
|---|---|---|
| `AQUORBIUM` | `BiomaBrain`, `AutonomousEnergy`, `Ecosystem` | acaso determinístico, energia autônoma e memória de contexto como referência de arquitetura |
| `AQUORBIUM` | `OrganismVoiceEngine` | despacho tipado entre famílias de síntese energética |
| `AQUORBIUM` | Morph, Pulsar, Modal, Granular, Physical String | primeiros corpos sonoros do barramento de energia |
| `AQUORBIUM` | `GeometricSequencer` | eventos, densidade, rotação, mutação e gesto composicional |
| `AQUORBIUM` | `FixedMpscQueue` | eventos de GUI, MIDI opcional e performance sem lock |
| `AQUORBIUM` | `Numeric` e testes de segurança | saneamento de entradas e proteção numérica |
| `AQUORBIUM` | telemetria atômica e política de janela | separação entre áudio, estado visual e superfície responsiva |
| `RASGO_SYNTH` | osciladores, FM, waveshaping e ring modulation | pesquisa de timbre, energia e transformação espectral |
| `RASGO_SYNTH` | mapas caóticos, ruído, granularidade e feedback | mutação, imprevisibilidade controlada e Terceiro Olho |
| `RASGO_SYNTH` | arcos narrativos, Markov, probabilidade e polimetria | composição de longo prazo e jogo musical |
| `RASGO_SYNTH` | `BluffSignal`, `CoherenceCollapse`, `ProbabilityMarket` | estratégias de informação, confiança, ruptura e regra oculta |
| `RASGO_SYNTH` | `WavWriter`, `Recorder`, metadados e índice | renders autorais e memória de sessões |
| `ANTITOTEM` | `CmosVoice`, efeitos, feedback, mixer e ressonador | matéria sonora e interação entre objetos; não alterar o projeto paralelo |
| `NAVALHA2_JUCE` | `AudioEngine`, filas, `OutputStage`, gravação e análise WAV | engenharia de realtime, segurança de saída e gravação de excelência |

## 3. O que ainda não existe

Não há ainda no RASGO um sistema completo que faça, em uma mesma arquitetura:

- análise musical de áudio de entrada;
- memória de descritores e relações musicais;
- interpretação de prompt em intenções composicionais;
- estratégia do Terceiro Olho;
- decisão entre seguir, reconstruir, transfigurar e contrariar;
- tradução dessas decisões para um barramento comum de síntese.

Essa é a contribuição específica do TRIOIO.

## 4. Fundação já criada no TRIOIO

O primeiro contrato portátil foi iniciado em `src/core/TrioioBrain.h/.cpp`:

- `BrainDecision` representa uma decisão musical normalizada;
- `EnergyControlBus` publica e lê controles escalares sem dependência de JUCE;
- entradas não finitas são saneadas;
- valores são limitados a `[0, 1]`;
- `revision` identifica a publicação da decisão;
- teste inicial está em `tests/TrioioBrainTests.cpp`.

Este é apenas um contrato inicial. Antes de integrar síntese real, será
necessário substituir a leitura de múltiplos atomics por um snapshot versionado
que garanta coerência de todos os campos de uma decisão.

## 5. Ordem de integração

1. validar e versionar `BrainDecision`/`EnergyControlBus`;
2. escolher descritores mínimos de áudio;
3. criar memória curta de contexto;
4. criar vocabulário de intenções de prompt;
5. implementar estratégias offline;
6. conectar um corpo sonoro próprio;
7. comparar reconstrução, transfiguração e contraponto;
8. só então adicionar áudio ao vivo, GUI e modelos externos.

## 6. Limites

- O TRIOIO não copia código de plataformas, serviços ou qualquer outra fonte
  externa.
- O Magenta pode ser estudado como referência de prompt + áudio + geração
  contínua, mas permanece adaptador opcional.
- Conteúdo obtido de serviços de streaming não será capturado nem usado para
  treinamento.
- Músicas autorais do usuário, de qualquer origem, podem entrar mediante
  fornecimento explícito de áudio, prompt opcional e metadados.
- O projeto não importará nem reaproveitará prompts, letras ou áudios de
  terceiros.
- ANTITOTEM não será alterado durante a construção do TRIOIO.
- A integração deverá preservar o contrato RASGO: áudio sem alocação, bloqueio,
  I/O ou interpretação pesada no callback.
