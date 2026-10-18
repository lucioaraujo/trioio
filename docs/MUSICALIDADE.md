# TRIOIO — roteiro de musicalidade

**Aberto em 2026-10-08.** Pedido do autor: "precisamos de mais
musicalidade, técnicas e astúcias de composição, investigar a fundo a parte
da teoria musical, linguagens musicais, comportamentos, estilos, culturas".

Este documento é o ponto de partida. Ele reúne:

1. o diagnóstico (onde o Trioio está diante de uma música real);
2. o inventário do que o RASGO já tem e pode ser copiado e adaptado;
3. o dossiê de teoria e linguagens, a preencher com fontes;
4. as camadas de implementação, em ordem.

## 1. Diagnóstico: uma faixa do Suno

Análise de 2026-10-08 de uma faixa do autor feita no Suno
(`Track-68 … (Cover)`, 2:18, MP3), com os descritores do Trioio e uma
leitura de croma por segundo:

| O que a faixa tem | O que o Trioio lê hoje |
| --- | --- |
| Tonalidade de **Sol menor harmônica** (Krumhansl: Ré maior 0,63 e Sol menor 0,60 quase empatados; os acordes resolvem a dúvida) | um "campo" de 7 classes por seção, sem centro tonal funcional |
| **Progressão** i–V–iv–III–VI: Solm (34 s), Ré (25 s, a dominante com Fá#), Sol, Dóm, Sib, Mib; troca a cada 1–2 tempos | nenhuma leitura de acordes; o baixo toca a tônica da seção e a quinta na cadência |
| **Pulso ambíguo**: o diagnóstico de 2026-10-08 disse ~85 BPM; a medida de 2026-10-09 (abaixo) mostra que os dados sustentam 172 em todas as bandas | 171,8 BPM, confiança 0,27 |
| Mixagem cheia e constante: energia de −16 a −25 dB, brilho 0,76, ruído 0,06 | lida corretamente desde o analisador v4 |
| Voz principal dentro da mistura: só 18 % dos quadros com altura definida, âmbito MIDI 43–79 | sem separação; a melodia se perde na mistura |
| **Forma** pela harmonia e pela repetição (contraste de descritores máximo de 0,09) | seções por contraste de descritores: em música assim, a forma real some |

**Conclusão.** A composição do Trioio (motivo, sentença de 4 compassos,
desenvolvimento, cadência, Terceiro Olho, memórias, campo rítmico) não
lida com **harmonia em movimento**, que é o que faz uma canção andar. A
escuta não lê acordes, não corrige o pulso dobrado e não acha repetição
de forma.

## 2. Inventário: o que o RASGO já tem

Os motores de composição do **Rasgo Synth Studio**
(`rasgo-synth-core/src/sequencer`, `dsp`, `engine`, cerca de 20 mil
linhas) cobrem boa parte do que falta. Pela regra RASGO (§7.0), são
copiados de um commit conhecido e adaptados aos conceitos do Trioio.

| Módulo do Studio | O que faz | Para o Trioio |
| --- | --- | --- |
| `Scales`, `SieveScale`, `MicrotonalTuning` | 45 escalas, crivo inventado, afinações | **copiado** em 2026-10-08 (`docs/ESCALAS.md`) |
| `HarmonicWanderer` | centros tonais que caminham: ciclo de Coltrane, ii–V com substituição de trítono, mediantes cromáticas (Jobim), troca de modo… | encadeamento entre seções |
| `NeoRiemannian` | transformações P/L/R (Riemann; Cohn, Lewin): acordes distantes a um semitom de condução | "transfigurar" a harmonia da referência |
| `MelodyVoicingBank` | voicings: quartal (McCoy Tyner), planing (Debussy) e outros | a voz de harmonia |
| `DissonanceTreatment` | dissonância como cor, nota errada (Stravinsky) e outros | tratamento das notas fora do acorde |
| `MelodicContourMemory` | intenção de contorno por frase, virada perto do ápice | arco das frases |
| `PolymeterEnsemble` | polimetria (ciclos de 11, 7, 5, 8) | campo rítmico em várias vozes |
| `RhythmSieve` | crivos rítmicos de Xenakis | ritmo inventado |
| `DrumArchetypes` | arquétipos de bateria por seção | ⚠ alterado sem commit na pasta de trabalho: copiar só do commit |
| `CoherenceCollapse` | ruptura por excesso de coerência (entropia de Shannon) | um gatilho do Terceiro Olho |
| `GrooveDialogue`, `BassFunctionPlan`, `NoteMorphology`, `MelodicCharacterIntent` | diálogo de groove, função do baixo, morfologia de nota | a avaliar |

O **Atlas de Referência** traz também Frames (§40), Streams (§46) e
Antescofo (§60.1).

## 3. Dossiê: teoria, linguagens, culturas (a preencher)

Cada entrada deve trazer: fonte primária, técnica, o que muda no som,
como vira código no Trioio e o teste que mostra que funciona. Áreas:

- **Harmonia:**
  - funcional (tônica, dominante, subdominante; cadências);
  - modal e troca de modo;
  - cromática e neo-riemanniana;
  - quartal, clusters, planing;
  - rearmonização (substituição de trítono, mediantes);
  - condução de vozes parcimoniosa.
- **Contraponto:** espécies, imitação, cânone, contracanto livre,
  heterofonia.
- **Melodia:**
  - contorno e ápice;
  - notas de passagem, bordaduras, apojaturas;
  - sequência melódica;
  - motivo e desenvolvimento por variação;
  - ornamentação de cada cultura.
- **Forma:**
  - repetição e retorno (estrofe e refrão, rondó, ABA);
  - variação;
  - desenvolvimento;
  - forma aberta e momentos (Stockhausen).
- **Ritmo:**
  - clave e timeline (África ocidental, Cuba, Brasil);
  - tala (Índia), aksak (Bálcãs, Turquia);
  - polirritmia e polimetria;
  - rubato, swing, microtiming.
- **Sistemas e culturas:**
  - maqam (inflexões e sayr);
  - raga (aroha/avaroha, pakad, rasa);
  - gamelão (colotomia, pelog e slendro);
  - música espectral (Grisey, Murail);
  - minimalismo (processos de fase, aditivos);
  - jazz modal;
  - canção popular brasileira.

**Comportamentos:** pergunta e resposta, imitação, provocação, silêncio,
acompanhamento, solo, ostinato, e quando cada um vale.

**Regra de honestidade (como no Studio):** um nome de cultura ou estilo só
entra com a fonte, e o que o Trioio faz é uma abstração declarada, não uma
reprodução daquela tradição.

## 4. Camadas, em ordem

1. **Harmonia em movimento** (primeira versão em 2026-10-08, `docs/HARMONIA.md`):
   - ler os acordes da referência por compasso (croma);
   - quando ela não tem acordes (voz só), derivar acordes da própria linha;
   - o baixo segue a fundamental do acorde, e a melodia cai em notas do
     acorde nos tempos fortes;
   - cada modo transforma a progressão: seguir, transfigurar
     (neo-riemanniano), contrapor (rearmonização), interromper.
2. **Escuta melhor:** pulso sem o erro de dobro; forma por repetição
   (matriz de autossimilaridade); separação simples de grave e voz.
3. **Máquina de escalas:** arquivos `.scl`; variantes conforme o acorde;
   encadeamentos e desvios (`docs/ESCALAS.md`).
4. **Voicing e dissonância:** banco de voicings e tratamento da
   dissonância do Studio.
5. **Dossiê → técnicas:** uma por vez, com fonte, teste e render.

## Camada 2, primeiro passo: o nível do pulso (2026-10-09)

**A pergunta.** O Trioio lê a faixa do Suno a 171,8 BPM. O diagnóstico
de 08/10 dizia que o tempo sentido era ~85 e que o Trioio dobrava o pulso.

**A medida** (sobre os quadros da faixa, analisador v4):

- a autocorrelação do fluxo suavizado prefere 172 (0,0061) a 86 (0,0051);
- os golpes pares e ímpares no nível de 172 têm a mesma força, tanto no
  fluxo inteiro (razão 1,06) quanto no grave abaixo de 150 Hz (1,025);
- a autocorrelação do grave é igual em 172 e em 86.

O bumbo e o baixo dessa faixa batem em todas as colcheias. **Os dados não
confirmam os ~85 BPM**: aquela leitura veio de uma estimativa mais
grosseira (fluxo cru, sem suavizar), e foi corrigida aqui.

**O que foi tentado e desfeito.** Uma regra de nível métrico: se o pulso
passa de 140 BPM e, no grave, os golpes alternam forte e fraco, o tempo é
o nível de cima (a estrutura métrica em níveis; Lerdahl e Jackendoff,
1983). Ela exigia um fluxo de grave novo no analisador (v5). Não agiu em
nenhum caso que eu conseguisse construir: o estimador já prefere o nível
lento quando o bumbo alterna, e até um pulso uniforme de 172 construído é
lido como 86. Sem um caso que a demonstre, a regra não entrou. O
analisador continua na v4.

**O que entrou: a escuta do autor decide** (`--andamento metade|dobro`).
Quando a leitura é ambígua entre o tempo e a subdivisão, o autor fixa o
nível que sente; a forma registra "metade do lido, pela escuta do autor".

- Teste em `tests/RhythmFieldTests.cpp`: 120 lido → 60.
- v29 em `/media/luc/4tb_21042021/TRIOIO_respostas/v29-andamento/` (apagada em 2026-10-16: arquivo de teste): o Suno
  em Yaman a 171,8 (lido) e a 85,9 (metade), para comparar.

**A escuta do autor decidiu** (2026-10-09): "entendo que a contagem é
feita mais lenta". O tempo da faixa é ~86; o 172 é a subdivisão. Isso
coincide com a literatura de percepção do pulso: a saliência é máxima
entre 600 e 700 ms (R. Parncutt, "A perceptual model of pulse salience and
metrical accent in musical rhythms", *Music Perception* 11(4), 1994; J.
London, *Hearing in Time*, 2004).

- **A preferência do estimador passou de 120 para 100 BPM** (600 ms;
  `src/core/Structure.cpp`). Centrada em 120, ela empatava 86 e 172.
- Testados os centros 100, 105 e 110: só até 105 o Suno sai em 85.
- **Consequência registrada:** o padrão de teste 3+3+2+2 colcheias passou
  de "5 tempos a 120" para "4 tempos a ~96", a leitura de **aksak** (quatro
  tempos desiguais, longo-longo-curto-curto; Brăiloiu, 1951). As duas
  leituras são válidas, e o teste aceita ambas; o ciclo do compasso (2,5
  s) é o mesmo.
- **Atenção:** referências antes lidas entre ~110 e 140 BPM podem agora
  sair na metade; `--andamento dobro` devolve o nível rápido.
- Agora o Suno sai a 85 BPM sem opção nenhuma (`suno-yaman-automatico-100`
  em `v29-andamento/`).

**Próximo na camada 2:** a forma por repetição (matriz de
autossimilaridade do croma e dos timbres, para achar estrofe e refrão).
