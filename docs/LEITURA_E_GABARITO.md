# TRIOIO — leitura das referências e gabarito do autor

**Estado:** implementado em 2026-10-10, no analisador v6. Entraram:

- `src/core/Reading.*`;
- o HPSS em `src/core/Descriptors.*`, analisador v5;
- os comandos `ler`, `gabarito` e `avaliar`;
- `tests/ReadingTests.cpp`.

## Por quê

A escuta do Trioio falhava em mixagens cheias:

- a tonalidade do Suno saía como a dominante (Ré maior em Sol menor);
- os acordes eram instáveis;
- a confiança do pulso era 0,21;
- a forma por repetição tinha um limiar nunca comparado com uma forma
  conhecida.

E não havia como **medir** se uma mudança melhorava a escuta. A ordem
aprovada pelo autor foi o gabarito primeiro, depois a separação
harmônico/percussivo.

## Comandos

```sh
trioio_biblioteca ler      <dados> <id> [--forma repeticao] [--andamento metade|dobro]
trioio_biblioteca gabarito <dados> <id>        # cria <dados>/gabaritos/<id>.txt (modelo vazio)
trioio_biblioteca avaliar  <dados> <id> [--gabarito ARQUIVO] [--forma …] [--andamento …]
```

O `ler` mostra:

- a fração percussiva e qual croma foi usado;
- o pulso;
- a tonalidade, pelos acordes e pelo croma bruto;
- quanto da peça tem altura definida;
- os acordes;
- as seções.

## A leitura

- **Pulso:** a autocorrelação de sempre (`docs/COMPOSICAO.md`), agora
  sobre o **fluxo percussivo** quando o analisador separou.
- **Tonalidade:** os perfis de Krumhansl e Kessler (1982), correlacionados
  com as **notas dos acordes lidos**, cada tríade pesada pela duração e com
  a fundamental pesando 1,5.
  - O croma bruto continua visível como segunda opinião.
  - **Achado:** pelo croma bruto, os harmônicos puxam para o modo maior,
    porque o 5.º harmônico de Lá é Dó#. Na faixa sintética em Lá menor, o
    croma bruto dava Lá maior com os acordes 100 % certos. Pelos acordes
    lidos, dá Lá menor. No Suno, pelos acordes, dá Sol menor (0,75); pelo
    croma bruto, Ré maior.
- **Acordes:** o HMM de sempre (`docs/HARMONIA.md`), tempo a tempo.
- **Seções:** por contraste ou por repetição (`--forma repeticao`).

## Separação harmônico/percussivo (HPSS), analisador v5

Método de D. Fitzgerald, "Harmonic/Percussive Separation using Median
Filtering", DAFx 2010: num espectrograma, o harmônico se estende no tempo
e o percussivo, na frequência. Implementação própria, só no offline,
porque precisa olhar à frente.

- **Fluxo percussivo** (janela curta): mediana no tempo de ±8 quadros
  (≈ ±180 ms), mediana na frequência de ±190 Hz e máscara de Wiener. O
  pulso passa a usar esse fluxo.
- **Croma harmônico** (janela longa): é **o que se sustenta**, ou seja, a
  mediana no tempo de ±24 quadros (≈ ±0,5 s), limitada pelo valor atual.
  - A máscara de Wiener deixava passar um tom afinado, que é estreito na
    frequência e por isso não conta como "percussivo".
  - Com ±8 quadros, a janela de 170 ms espalhava o golpe por ~10 quadros,
    e ele passava por harmônico.
- **Decisão por peça:** o croma harmônico só substitui o croma quando a
  fração percussiva da peça é **≥ 0,40**.
  - Numa melodia sozinha, a mediana longa apaga as notas rápidas, e a
    harmonia tirada da linha se perde. O `ComposerTests §8` pegou isso: o
    motivo Dó Mi Sol Mi Ré virou Dó Mi Mi Mi Ré.

| Material | Fração percussiva | Croma |
|---|---|---|
| pad sustentado (sintético) | 0,01 | inteiro |
| melodia pinçada (`ComposerTests §8`) | 0,08 | inteiro |
| vocalise (voz sozinha) | 0,31 | inteiro |
| Suno (banda) | 0,51 | harmônico |
| bateria + pad (`ReadingTests`) | 0,76 | harmônico |

O limiar é **provisório**: separa a voz sozinha da banda, mas falta o
gabarito para confirmá-lo.

- **Custo:** um anel de 49 espectros, cerca de 1,6 MB, e as medianas por
  bin. A faixa do Suno (2:18) analisa em poucos segundos.
- **Cache:** guarda `percFlux`, `percussiveShare` e `harmonicChroma`. A
  versão 5 invalida os caches da versão 4.

### Efeito medido

| | v4 | v5 |
|---|---|---|
| Suno, tonalidade | Ré maior | **Sol menor** (pelos acordes) |
| Suno, trechos de acorde | 36, com Mibm, Sim, Dó#m e Lám espúrios | 27, progressão legível (Solm, Ré, Mib, Sib, Dóm…), ainda com alguns erros |
| Suno, confiança do pulso | 0,21 | 0,24 |
| faixa sintética, Sib do tom no croma | 0,048 | 0,021 |
| faixa sintética: pulso, tempos, acordes | 96 BPM, F 0,98, 100 % | igual |

A faixa sintética já era fácil para a v4: o HMM soma o croma de um tempo
inteiro e resiste a golpes curtos. O ganho do HPSS aparece na mixagem real.

## Croma do grave (analisador v6)

Em cada quadro, as classes de altura dos picos entre 45 e 260 Hz, com os
mais graves pesando mais (magnitude × 130 Hz / frequência). Nas peças com
percussão vale a versão do que se sustenta, sem o bumbo. No HMM dos
acordes, o grave vota:

- **+0,30** quando a nota do grave é a fundamental do acorde;
- **+0,08** quando é a terça ou a quinta (inversão).

**Por que é preciso:** Dó6 e Lám7 têm as mesmas quatro notas (Dó Mi Sol
Lá), e só o baixo diz qual dos dois soa. Na faixa de teste, com um baixo
cheio e harmônicos fracos, a leitura acerta **100 % com o grave e 50 %
sem ele**, também com bateria. Pesos menores (0,20 / 0,12 / 0,08) perdem
o caso com bateria (75 % / 58 % / 58 %).

**Limite:** numa voz sozinha, a nota grave da própria voz entra como
"baixo".

- No vocalise, os trechos de acorde passam de 78 para 93. Esse vocalise
  já pulava (74 trechos na v4), porque ali a harmonia nasce da própria
  linha.
- Dois critérios para detectar "não há baixo separado" foram medidos e
  descartados, porque não separam os casos:
  - a fração de altura definida: 0,99 no acorde sintético contra 0,87 no
    vocalise;
  - a coincidência entre o grave e a nota mais forte: 0,87 contra 0,78.
- O gabarito do vocalise vai dizer se isso piora a leitura.

## Melodia na mistura: altura predominante (analisador v6)

O YIN (`f0Hz`) funciona no domínio do tempo e se perde quando voz, banda e
bateria soam juntas: só 28 % dos quadros do Suno têm altura definida. A
**saliência harmônica** procura a voz principal no espectro:

- **Método:** o núcleo do MELODIA (J. Salamon e E. Gómez, IEEE TASLP 2012,
  eq. 2–3; A. Klapuri, ISMIR 2006). Cada candidato a f0, entre 100 e
  1000 Hz numa grade de 10 cents, soma os picos que caem nos seus
  harmônicos (h = 1..20), com peso 0,8^(h−1) e espalhamento cos² de ±1
  semitom.
- **Fonte:** o espectro longo cru, para servir também à escuta ao vivo e
  não apagar notas rápidas.
- **Picos usados:** só a partir de **250 Hz**.
  - Com eles a partir de 55 Hz, o baixo do pad (110–150 Hz) vencia a
    melodia, e o acerto era de 33 %.
  - A melodia grave continua sendo achada pelos harmônicos.

| Faixa de teste (voz com vibrato + pad + bateria) | Saliência | YIN |
|---|---|---|
| melodia Sol4–Sol5 | **91 %** | 0 % |
| melodia Lá3–Sol4 (voz masculina) | **86 %** | 1 % |

A medida é a altura certa a ±50 cents, mais os ±30 do vibrato, nos
quadros em que a melodia soa.

- **Suno:** a altura predominante fica entre MIDI 58 e 78 (p10–p90), com
  mediana em Lá4. É plausível para a voz principal.
- **Limite:** a saliência não decide se há melodia ou só acompanhamento.
  Num trecho só de pad, a "altura predominante" é a nota de cima do pad. O
  MELODIA resolve isso pelos contornos, que ainda faltam aqui.
- **Uso no compositor (2026-10-10):** `pitchAt` (Structure.h), que dá a
  altura de cada ataque para o contorno do motivo e para o oráculo, recorre
  à altura predominante quando o YIN não dá ao menos 2 dos 5 quadros em
  volta. O piso de saliência é 0,15.
  - **Suno:** os ataques com altura vão de **46 % para 100 %**.
  - Recording_187 já tinha 100 %; o vocalise vai de 94 % para 100 %.
  - **Na resposta** (`seguir`, seed 1): sem a reserva, o motivo do Suno
    sai com "contorno: inventado", e o oráculo aprende 10, 10 e 7 notas.
    Com ela, sai "contorno: da melodia da referência", e o oráculo aprende
    10, 16 e 15.
  - O piso de 0,15 quase nunca recusa. Na mistura sempre há uma altura
    predominante, mesmo que seja a nota de cima do acompanhamento. A
    melodia sozinha continua pelo YIN (`ComposerTests §8`).

## Autogabarito: medir sem a escuta do autor (2026-10-10)

O autor decidiu não preencher gabaritos. A resposta exata passou então a
vir da **própria partitura do Trioio**:

1. o Trioio compõe sobre uma referência;
2. sintetiza a composição;
3. escuta o próprio áudio como se fosse uma referência;
4. compara a leitura com a partitura (`gabaritoFromScore` e
   `melodyAgainstScore`, em `Reading.h`).

O que vem da partitura:

- **andamento:** `score.bpm`;
- **acordes:** as notas de cada tempo, sem a percussão, pelo molde das 36
  tríades;
- **tonalidade:** Krumhansl sobre as notas;
- **seções:** as marcas `seção N`;
- **melodia:** a voz 0 nota a nota, com glissandos e curvas.

```sh
trioio_biblioteca autoavaliar <dados> <id> [--seeds N] [--modos seguir,contrapor,…]
```

Nada é gravado. Cada peça leva ~30–45 s (compor, sintetizar, escutar) e
usa até ~190 MB.

**Limite:** a música é a do Trioio. Há timbres inarmônicos e os objetos do
Terceiro Olho, como clusters e faixas de ruído, que soam mas não estão nas
notas. É uma medida de acerto, não de toda a música. Uma voz de ruído
desafia a leitura de verdade.

| Referência de base | Peças | Andamento certo | Tonalidade | Acordes | Melodia (classe) | Seções F |
|---|---|---|---|---|---|---|
| Suno (Track-68) | 8 | 5/8 | 1,00 | 58 % | 45 % (66 %) | 0,52 |
| Recording_187 | 4 | 2/4 | 0,65 | 39 % | 37 % (59 %) | 0,39 |

**O que o autogabarito já ensinou:**

- **Melodia: os erros de oitava são todos para baixo** (↓ 9–47 %, ↑ 0).
  - Escolher a oitava de cima quando ela tem saliência comparável deu só
    +5–7 pontos e piorou a melodia grave sintética (de 86 % para
    57–74 %). **Descartado.**
  - A causa provável são as outras vozes do Trioio (o baixo e a voz de
    harmonia) na mesma nota uma ou duas oitavas abaixo, porque a melodia
    cai em notas do acorde. Separar melodia de acompanhamento é trabalho
    dos contornos do MELODIA.
- **Andamento:** os erros são de razão 4:3 ou 2:1.
  - Com o pulso pelo sinal inteiro, e não pela parte percussiva: andamento
    6/8 em vez de 5/8, mas acordes 54 % em vez de 58 %, melodia 42 % em
    vez de 45 % e seções 0,43 em vez de 0,52.
  - Amostra pequena; **mantida a parte percussiva**, com a dúvida
    registrada.
- **Primeira armadilha do método:** uma variável de ambiente vazia
  (`VAR=`) não é nula, e um experimento mediu duas vezes a mesma variante.
  Refeito com `env -u`.

## Teste por transformação: estabilidade nas gravações reais (2026-10-10)

```sh
trioio_biblioteca estabilidade <dados> <id> [--semitons -2,-1,1,2]
```

Toca a gravação 2^(k/12) vezes mais rápido: reamostragem de Lanczos
(a = 8), com corte antialiasing e ganho 1. O teste confere que 440 Hz vira
493,9 Hz com o mesmo pico. Uma escuta correta se transforma junto:

- tonalidade, acordes, melodia e linguagem deslocados k semitons;
- BPM × fator;
- seções no tempo ÷ fator.

Isso **não mede acerto**, porque uma leitura constante seria "coerente";
o autogabarito faz esse papel. Mostra o que é frágil nas gravações de
verdade, sem precisar de gabarito.

- **Linguagem:** conta como estável se a primeira linguagem depois da
  transformação está entre as 3 primeiras do original, transpostas e
  comparadas pelo **conjunto de notas**.
  - Hijaz, melakarta 14 e modo andaluz são o mesmo conjunto e trocam de
    lugar.
  - No Suno, melakarta 13 (1,03) e Hijaz (1,01) diferem numa nota e
    quase empatam.

| Gravação | Andamento | Tonalidade | Linguagem | Acordes | Melodia | Seções F |
|---|---|---|---|---|---|---|
| Recording_187 | 3/4 | 4/4 | 4/4 | 84 % | 91 % | 0,81 |
| Suno | 3/4 | 4/4 | 3/4 | 71 % | 76 % | 0,74 |
| vocalise | 2/4 | 1/4 | 4/4 | 53 % | 74 % | 0,66 |

São 4 transformações por gravação, de ±1 e ±2 semitons. Cada gravação leva
de 40 s a 2 min, e a memória fica abaixo de 110 MB.

**Achado: o nível do pulso salta.** Em 4 dos 12 casos, o BPM transformado
saiu ×2,01, ×1,52 e ×0,65 (duas vezes) em vez de ×1,00.

- O pulso escolhe, entre todos os atrasos, o melhor pesado pela
  preferência em 100 BPM. Quando o andamento transformado passa por ela,
  o empate entre níveis (1, 2, 3:2) se desloca.
- Em Recording_187 com −2 semitons (66 BPM), a leitura pulou para o nível
  pontuado, a ~100 BPM.
- **Duas correções foram tentadas e revertidas:**
  1. escolher só entre os níveis ×¼…×4 da periodicidade mais forte;
  2. a mais forte pelo pente de múltiplos.

  As duas travaram no distrator pontuado da faixa sintética (o pad que
  re-ataca a cada 1,5 tempo; 64 BPM em vez de 96) e quebraram o
  `ComposerTests`.
- **Terceira tentativa, a estimativa de andamento de Ellis 2007, §3.1
  (TPS2/TPS3):** a força de τ soma a do nível de cima (½·2τ, ou ⅓·3τ).
  Testados três ajustes: pesos plenos, pesos de ¼ e o nível de cima pelo
  máximo dos vizinhos.
  - Os três quebraram o `ComposerTests`: o metrônomo de 120 BPM passava a
    60, com o tempo livre no lugar do pulso.
  - Os pesos plenos quebraram também o 5/4 do `StructureTests`.
  - **Revertido.**
- Fica o estimador anterior, que passa em todos os testes e guarda
  decisões validadas pelo autor (a contagem a 85 no Suno). Estabilizar o
  nível pede uma bateria de referências de andamento conhecido, que pode
  vir do autogabarito com o andamento variando, antes de mexer de novo.

## Bancada de andamentos (2026-10-10)

```sh
build/trioio_bancada_andamento [segundos por faixa = 24]   # fora da suíte: ~3–4 min
```

São faixas sintéticas de andamento conhecido, em 6 estilos (rock,
metrônomo, shuffle 12/8, bossa nova com clave, valsa 3/4, rock com pad
pontuado). Cada estilo varre de 60 a 180 BPM em 20 passos de ~6 %. A
bancada mede quantas leituras saem no andamento contado e **quantas vezes
o nível salta** entre andamentos vizinhos. Ler a metade acima de ~150 BPM
não é erro: muita gente conta assim.

| Estimador | Certos | Saltos | Onde erra |
|---|---|---|---|
| **atual** (preferência em 100 BPM) | **68/120** | **21** | rock: dobro em 60–70 e **⅔ em 120–127**; bossa: 3/20, 10 saltos (4/3, ⅔) |
| + Ellis TPS2/TPS3 (pesos ½ ou ¼) | 72–73 | 21 | ganho pequeno; quebra o metrônomo do `ComposerTests` |
| + saliência da grade (5 candidatos, melhor fase) | 53 | 60 | a grade de período inteiro deriva e a fase se perde |
| sem a regra do ciclo do compasso | 68 | 21 | idêntico: não é ela a causa |

### Ataques e pulso por banda (analisador v7) — o que resolveu

Cada quadro ganha o fluxo da banda **grave** (< 200 Hz) e o da **aguda**
(> 2 kHz). Com isso:

1. **Ataques por banda** (`bandOnsets`): os ataques de cada banda são
   escolhidos à parte e juntados. Golpes próximos de bandas diferentes não
   se escondem um atrás do outro, e o grave é decidido pela **intensidade**
   relativa das duas bandas.
   - Um "sim/não" por banda falhou: o dum tem clique de ataque e o tak tem
     ruído largo, e os dois acendem as duas bandas.
2. **O pulso escuta as bandas:** a autocorrelação usa (fluxo grave ÷ p90)
   + (fluxo agudo ÷ p90). Bumbo e chimbal pesam igual, e o meio (o pad, a
   voz) não puxa o pulso. Uma banda sem atividade fica de fora; sem as
   duas, volta o fluxo.

| | Bancada: certos | Saltos | Estabilidade (andamento): R187 / Suno / vocalise |
|---|---|---|---|
| fluxo percussivo (v5–v6) | 68/120 | 21 | 3/4 · 3/4 · 2/4 |
| **bandas (v7)** | **92/120** | **6** | **4/4** · 2/4 · 1/4 |

- **Bancada:** rock, bossa e rock com pad pontuado ficam certos de 60 a
  150 BPM. Os 6 saltos que sobram são a metade acima de ~150, que é uma
  contagem aceitável.
- **Autogabarito, nas mesmas peças** (`autoavaliar` mostra as duas
  leituras): empate. Sobre o Suno, andamento 6/8 contra 7/8, com
  tonalidade e acordes iguais; sobre Recording_187, 3/4 contra 2/4.
  - Uma queda aparente de tonalidade (1,00 → 0,55) vinha de **outras
    peças**: o compositor ouve a referência com o pulso novo e compõe
    diferente.
  - Comparar versões sempre nas mesmas peças.
- **Gravações:** o Suno continua em 84,9 BPM, a contagem do autor.
  Recording_187 fica estável, com os acordes de 84 % para 91 %. O vocalise
  piora, mas uma voz sem bateria não tem pulso definido (confiança 0,07).

Metrônomo, shuffle e valsa já são sólidos: 15–16/20 e um salto só, para a
metade, acima de 150 BPM. Os defeitos estão no rock (o nível pontuado vence
quando o chimbal em colcheias torna todos os atrasos parecidos) e na clave
da bossa. **Fica o estimador atual.** A bancada é a régua para a próxima
tentativa séria: o acompanhamento completo por programação dinâmica (Ellis
2007), com fase e andamento locais, em vez de uma grade fixa.

### Fase do pulso e tempo forte (2026-10-11)

A bancada passou a medir também se os tempos lidos caem nos tempos
verdadeiros (a **fase**, ±50 ms) e se o 1 do compasso cai no 1 verdadeiro
(o **tempo forte**, ±70 ms). Entrou um sétimo estilo, **pop**: o rock com
acordes que mudam no 1 de cada compasso e o baixo na fundamental no 1 e no
3.

**Achado: a fase estava errada** em dois estilos. Ela vinha da média
circular dos ataques, e os ataques fora do tempo (o bumbo antecipado da
bossa, o chimbal em tercina do shuffle) puxavam a média.

| Estilo | Fase, antes (média circular) | **Fase, agora (a grade que soma mais envelope)** |
|---|---|---|
| shuffle 12/8 | 0/15 | **15/15** |
| bossa nova | 0/15 | 5/15 |
| rock, rock + pad, metrônomo, valsa, pop | 14–16 de 15–16 | todos certos |

A média circular foi retirada. A suíte continua 29/29. A fase vale para
toda a leitura: os acordes são lidos por tempo, e a grade rítmica parte
dela.

**O tempo forte** (`estimateDownbeat`, Structure.h) escolhe, entre os
deslocamentos possíveis, o que soma mais nos seus tempos dois sinais
(Davies e Plumbley 2006; Peeters e Papadopoulos 2011):

- o grave **dominante** (grave − agudo, cada um normalizado), porque a
  caixa acende as duas bandas;
- a mudança de acorde.

| Estilo | Tempo forte |
|---|---|
| **pop (acordes no 1)** | **12/13** |
| metrônomo (acento agudo no 1) | 13/16 |
| rock, rock + pad, bossa | 1–3 de 15 |
| shuffle | 6/15 |
| valsa | 0/16 (13/16 com o compasso de 3, abaixo) |

- **Bateria sem harmonia:** o 1 e o 3 do rock soam iguais, uma
  ambiguidade real. A caixa sintética tem um tom de 190 Hz, dentro da
  banda grave.
- **Valsa:** é lida em **4 tempos** em todas as faixas. O compasso de 3
  não é detectado quando o chimbal soa em todo tempo.
- **Autogabarito:** não serve para o tempo forte. O 1 das peças do Trioio
  é a primeira batida lida na referência, arbitrária e não marcada pela
  música. Ali deu 2/5 e 2/8, perto do acaso de 1 em 4, como esperado.
- **Gravações reais:** a confiança fica em 0,01–0,05, e o método não
  decide. No Suno, o 1 sai em 2,73 s, onde os acordes mostram a primeira
  troca para Ré; é um indício, não uma prova.
- **Uso:** por ora só o `ler` mostra o tempo forte. O compositor continua
  a partir da fase.
- **O compasso de 3 (`estimateMeter`, 2026-10-11):**
  - **Método:** por tempo, o grave dominante e o croma. Para m = 2, 3 e 4,
    soma a correlação do grave no atraso de m tempos e ½ da semelhança do
    croma no mesmo atraso (descontada a do tempo vizinho). O 3 só vence se
    superar o 2 e o 4 por 0,05.
  - **Margens:** 0,1 dava a valsa 8/16; **0,05 dá 13/16**; 0 dá 14/16, mas
    erra 7 faixas em 4/4. Com 0,05, nenhum 4/4 vira 3: **102/105** na
    bancada.
  - **No pulso:** só quando o 4 é o padrão (o ciclo do compasso não foi
    medido). Os compassos ímpares que o pulso já mede (5, 7) ficam como
    estão.
  - **Valsa:** lida em 3 em 13 das 16 faixas. O tempo forte vai de 0/16
    para **13/16**, e o total de tempo forte na bancada de 24 para 49/105.
  - **Suíte:** 29/29. As gravações reais continuam em 4 tempos.
- **Próximo:** usar o tempo forte no compositor quando a confiança for
  alta. Nas gravações de hoje ela fica em ≤ 0,05, e por isso ainda não.

## Reconhecer a linguagem (2026-10-10)

O `ler` mostra as 5 escalas do arquivo do Trioio que melhor vestem o
**campo estrutural** da peça (`Reading::structuralField`):

- as notas dos acordes lidos, por duração;
- as alturas da melodia predominante;
- cada metade somando 0,5.

O croma bruto tem um pouco de todas as 12 notas, e com ele uma escala
cromática (24-EDO, mesotônicos) sempre vencia.

**Só as linguagens entram.** Ficam de fora os sistemas de afinação e as
abstrações: MOS, EDO, temperamentos, conjuntos de Forte, séries
harmônicas, entonação justa, Euler-Fokker, Wilson e as escalas não
oitavantes. Elas vestem qualquer campo sem dizer de que música ele é.
Sobram 330 escalas (maqamat, rāgas, thāts, melakartas, modos, gamelão,
flamenco, Nordeste, Grécia antiga, Messiaen…), de 5 a 9 graus.

| Referência | Campo estrutural | Primeiras linguagens |
|---|---|---|
| Recording_187 | Lá 25 %, Mi 18 %, Dó# 14 %, Ré 11 %, Fá 10 %, Sol 7 %, Sib 7 % | maqam Hijaz, melakarta 14, jins hijaz + kurd e modo andaluz, **todos sobre Lá** |
| Suno | Ré 29 %, Sol 16 %, Sib 12 %… | Hijaz / frígio dominante sobre Ré |
| vocalise | quase plano | modos de Messiaen |

- **Recording_187:** são as mesmas notas em quatro tradições, e os
  acordes confirmam com a cadência andaluza Rém–Dó–Sib–Lá. A tonalidade
  de Krumhansl dizia Lá maior: os perfis só conhecem maior e menor.
- **Suno:** Hijaz sobre Ré tem as mesmas notas de Sol menor harmônica (é
  o seu 5.º modo). A tônica cai em Ré porque a dominante é a classe mais
  forte do campo.
- **Vocalise:** o resultado só diz que não há uma linguagem clara.

### A linguagem reconhecida chega ao compositor (2026-10-11)

**O defeito:** com `--alturas arquivo`, o compositor escolhia a escala pelo
**croma bruto** e entre **todas** as famílias do arquivo. Em
Recording_187, as 7 seções recebiam o "mesotônico de 1/11 de coma" (12
notas). No Suno saía um modo de Messiaen de 8 notas. Era o mesmo defeito
que o reconhecimento já tinha resolvido.

**A correção:**

1. **`languageField`** (Harmony.h) é o campo da linguagem, o mesmo do
   `ler`: as notas dos acordes lidos pelo pulso e as da melodia
   predominante. O compositor o usa em `--alturas arquivo` e nas misturas.
   - Uma versão intermediária (croma ≥ 30 % + melodia) ainda dava Messiaen
     no Suno e foi substituída.
2. **`isLanguageScale`** (ScaleArchive.h): a escolha **automática** (sem
   `--escala`) fica entre as linguagens de 5 a 9 graus. Com `--escala
   TERMO`, todo o arquivo continua disponível.
3. **Sorteio só entre as quase empatadas** (até 0,05 da melhor). A
   variedade entre seeds continua entre escalas equivalentes.

| Referência | seed 1 | seed 2 | seed 3 |
|---|---|---|---|
| Recording_187 (antes: mesotônico de 12 notas) | melakarta 14 sobre Lá | jins hijaz + kurd sobre Lá | jins hijaz + kurd sobre Lá |
| Suno (antes: Messiaen 4 e 6) | maqam Hijaz sobre Ré | melakarta 14 sobre Ré | melakarta 14 sobre Ré |

Em Recording_187, a harmonia segue os graus da escala (Lá, Rém, Lá,
Sib…). A suíte continua 29/29.

**O comportamento entra também.** Das escalas equivalentes sobre Lá, só o
maqam Hijaz e o modo de Mi andaluz têm comportamento no arquivo. O
melakarta 14 e o jins hijaz + kurd têm só as notas. Entre as quase
empatadas, o sorteio agora **prefere as que têm comportamento**.
Recording_187, seeds 1 a 4:

- **seeds 1 e 2, maqam Hijaz:**
  - heterofonia: a segunda voz repete a melodia com atraso, com ~75
    apojaturas e ~45 oitavas, sem acordes;
  - 13 frases-assinatura e 7 cadências;
  - o ciclo **maqsum**, com 63 chegadas no dum.
- **seeds 3 e 4, modo de Mi andaluz:**
  - 14 frases-assinatura e 7 cadências;
  - o **compás de 12 (soleá, bulerías)**.

A subida e a descida mostram 0 notas porque, no Hijaz, elas têm as notas
da própria escala: não há o que ajustar. O `ScaleArchiveTests` guarda que
8 de 8 escolhas automáticas num campo de Lá frígio dominante têm
comportamento e tônica Lá.

## Reconhecer o ciclo rítmico (2026-10-10, experimental)

`rankCycles` (RhythmCycles.h) compara a peça com os 26 ciclos do arquivo
(iqa', tala, usul, aksak, claves, gamelão, flamenco, blues):

1. **Previsão:** cada ciclo, repetido volta após volta, prevê onde há e
   onde não há ataque numa grade fina comum a todos (1/12 do tempo).
2. **Comparação:** a previsão é comparada com os ataques reais da peça
   inteira. Dobrar tudo nas poucas casas do ciclo, a primeira versão, dava
   vantagem aos ciclos curtos.
3. **Grave e agudo:** um ataque é grave se o centroide está abaixo da
   mediana dos ataques. Separa o dum do tak e, assim, maqsum de sa'idi,
   que têm as mesmas posições.
4. **Pontuação:** correlação de tempo + ½ da correlação grave/agudo com
   D/B/G/N contra T/k/t.
5. **Unidade:** pelo nível do pulso (×½, ⅔, ¾, 1, 4/3, 3/2, 2, os erros da
   bancada) **ou** o ciclo inteiro em m pulsos (m = 2…8: o aksak 2+2+3 não
   é múltiplo da colcheia).
6. **Afinação do período:** ±3 % em passos de 0,5 %. Um erro de 1,3 % no
   pulso (101,3 em vez de 100) deriva 0,4 s em 30 s, e era essa a causa
   das falhas de baladi, sa'idi e clave son.

| Ciclos sintéticos (100 BPM, tambor grave e agudo, 30 s) | Em 1.º | Entre os 3 primeiros |
|---|---|---|
| primeira versão (dobra nas casas do ciclo) | 3/8 | 4/8 |
| linha do tempo comum | 3/8 | 3/8 |
| + período afinado ±3 % | **4/8** | **5/8** |
| + o ciclo em m pulsos | 4/8 | 5/8 |
| envelope do fluxo no lugar dos ataques | 1/8 | 5/8 |

- **Acertam:** maqsum, baladi, sa'idi e wahda.
- **Falham:**
  - a clave son perde para a wahda, um ciclo esparso que casa com
    qualquer padrão de tempos fortes;
  - a bossa sai em 2.º;
  - a ruchenitsa e o shuffle falham.
- **Achado:** o detector de ataques **perde golpes próximos** que se
  sobrepõem. No shuffle sintético ficou cerca de metade dos golpes. O
  envelope do fluxo, que não perde nada, foi medido e piorou, porque o
  fundo de fluxo dilui a correlação.
- **Gravações reais:** nenhum ciclo do arquivo veste (0,20–0,38). São
  canções, não iqa' nem tala. Abaixo de 0,6 o `ler` diz "nenhum ciclo
  claro", porque os acertos sintéticos ficam em 0,65–0,84.
- **Com os ataques por banda (v7):** **6 de 8 em 1.º e 7 de 8 entre os 3
  primeiros**: maqsum (0,98), baladi, sa'idi, wahda, clave son e bossa. A
  ruchenitsa (4.º) e o shuffle (3.º) ainda falham.
- **"Claro":** ≥ 0,6 com o termo grave/agudo, ≥ 0,4 nas claves (só golpes
  "x", sem esse termo). Nas gravações reais: nenhum ciclo claro
  (0,22–0,26).

### A bancada completa: os 26 ciclos do arquivo (2026-10-14)

**A bancada de 8 media sorte.** O ruído sintético corria entre os ciclos, e o
resultado de cada um dependia da ordem do arquivo. Com o analisador v8, os 8
saíram em 1.º. Com o mesmo ruído para cada ciclo, a wahda e a ruchenitsa
passam a falhar. A bancada agora cobre **todos os 26 ciclos**, com o mesmo
ruído para cada um, e mostra a posição do ciclo certo quando ele fica fora
dos 5 primeiros.

| | em 1.º | entre os 3 |
|---|---|---|
| os 8 de antes | 6 | 6 |
| **os 26 do arquivo** | **19** | **21** |

**As falhas** (ciclo certo em N.º, com nota):

- wahda, 6.º (0,79; o maqsum leva 0,84);
- sofyan, 4.º;
- ruchenitsa, 10.º;
- cinquillo, 2.º;
- sino padrão (12/8), 11.º;
- baião, 2.º;
- compás de 12, 7.º.

Quase todas têm o pulso lido em outro nível: 50, 60 ou 200 BPM, num ciclo a
100. A ruchenitsa é aksak (2+2+3).

**Tentativa descartada: varrer a fase dentro da unidade.**

- **A ideia:** o ciclo só podia começar na fase do pulso lido mais unidades
  inteiras; a tentativa testou também os começos entre dois golpes, de 2 em
  2 células da grade fina.
- **O resultado:**
  - o sofyan passa e a ruchenitsa sobe para 2.º;
  - mas a clave son cai para 3.º (a kopanitsa acha encaixe);
  - em 1.º, continua 19 de 26; entre os 3 primeiros, sobe para 23;
  - custa +40 % de tempo.
- **Variantes:** com penalidade para a fase deslocada (0,03 a 0,1), ou com a
  varredura só fora do nível lido, nada muda em 1.º lugar.
- **O diagnóstico:** o limite é do método. Os ciclos mais densos ou mais
  longos têm mais combinações de nível, fase e período, e o melhor encaixe
  entre muitas tentativas acaba superando o ciclo certo.
- **O que poderia ajudar:** comparar a periodicidade do próprio sinal com o
  comprimento do ciclo, ou ponderar a nota pelo número de tentativas que o
  ciclo teve. Fica para quando o reconhecimento de ciclo for usado pela
  composição. Hoje ele é só informativo no `ler` e segue experimental.

## A linha da melodia: separar do acompanhamento (analisador v8, 2026-10-11)

O autogabarito mostrou que os erros da altura predominante eram quase
todos **uma ou duas oitavas abaixo**: o leitor escolhia a voz do baixo ou
da harmonia. O analisador passou a guardar os **3 candidatos** por quadro
(os máximos locais da saliência, a ≥ 1 semitom um do outro), e
`trackMelody` escolhe entre eles por um caminho de Viterbi:

- custa 0,08 por semitom saltado (a melodia é uma linha);
- ganha 0,25 por oitava acima de Dó4 (a melodia costuma ficar acima do
  acompanhamento);
- o silêncio reinicia a linha.

O analisador offline grava a linha em `melodyHz`. A escuta ao vivo fica com
o maior candidato. Os candidatos não vão para o cache.

**Calibração nas mesmas peças** (`autoavaliar --melodia-exp 1`, 8 peças por
referência):

| Variante (salto / preferência pelo agudo) | Suno: altura · oitava abaixo | Recording_187: altura · oitava abaixo |
|---|---|---|
| pico cru (v6–v7) | 47 % · 15 % | 43 % · 10 % |
| **linha 0,08 / 0,25 (padrão)** | **53 % · 7 %** | **44 % · 7 %** |
| só continuidade (0,08 / 0) | 45 % · 16 % | 43 % · 11 % |
| 0,15 / 0,25 | 52 % · 8 % | 44 % · 7 % |
| 0,08 / 0,5 | 51 % · 5 % | 43 % · 7 % |
| 0,04 / 0,25 | 52 % · 7 % | 44 % · 7 % |

- **O que funciona é a preferência pelo agudo.** Só a continuidade não
  muda nada. Os erros de oitava caem pela metade.
- **A classe de altura não muda** (~64 %). Os erros que restam são outra
  voz mais saliente que a melodia, ou trechos sem melodia. Pedem os
  contornos completos do MELODIA, com a decisão de voz.
- **Faixa sintética:** a melodia aguda vai de 91 % para **100 %**. A
  melodia grave, que ali fica **entre** as notas do pad, vai de 86 % para
  74 %: a preferência pelo agudo pega o pad.
  - É o preço conhecido. Na música do Trioio e nas gravações, a melodia
    costuma ficar acima.
- **Suno:** a região da melodia sobe de MIDI 58–78 (mediana 69) para
  62–79 (mediana 72).
- **Tentado e descartado (2026-10-11): penalizar o candidato sustentado**,
  isto é, a mesma altura soando ~0,4 s e ~0,85 s antes. A hipótese era que
  o pad segura a nota e a melodia se move. Nas mesmas peças:

  | Peças sobre | Linha | + 0,3 | + 0,6 | + 1,0 |
  |---|---|---|---|---|
  | Suno | 44 % | 44 % | 43 % | 41 % |
  | Recording_187 | 29 % | 29 % | 28 % | 25 % |

  A melodia do Trioio também repete e segura notas, e a penalidade a
  atinge. Separar a melodia pede outra pista: o timbre de cada voz, ou os
  contornos com começo e fim de nota.
  - Os números absolutos mudaram desde a v8 porque a fase e o compasso
    novos mudaram as composições. As comparações valem só dentro de cada
    rodada.
- **Tentado e descartado (2026-10-11): o timbre.** Para cada candidato, o
  perfil dos harmônicos 1..8, e um custo por trocar de timbre entre quadros
  no Viterbi.

  | Peças sobre | Linha | timbre 0,5 | 1,0 | 2,0 | 4,0 |
  |---|---|---|---|---|---|
  | Suno | 44 % | 44 % | 44 % | 44 % | 44 % |
  | Recording_187 | 29 % | 29 % | 29 % | 29 % | 29 % |

  Nenhum efeito. No Trioio, a própria melodia troca de família de timbre
  a cada seção (o compositor sorteia por seção), e o perfil não identifica
  a voz ao longo da peça. Em gravações com instrumentos fixos a pista pode
  valer, mas sem uma régua com melodia conhecida não há como medir. O
  código foi revertido.

## Gabarito

É um arquivo de texto do autor, com o que **ele** ouve. Tudo é opcional:

```text
referencia: ref-c2de6fb10363075a
fonte: escuta de Lúcio, 2026-10-11
andamento: 86                  # o pulso em que se conta
batidas: 0.70 1.40 2.10        # ou os instantes dos tempos
compasso: 4
tonalidade: Sol menor          # ou Solm, Gm
acordes: 0:00 Solm, 0:02.8 Ré  # cada um vale até o seguinte; N = sem acorde
secoes: 0:00 A, 0:34 B, 1:02 A
fim: 2:18
```

- **Nomes de acorde e de tonalidade:** em português (Solm, Sol menor, Sib,
  Si°) ou em inglês (Gm, Bb, Bdim).
- **Instantes:** em segundos ou em m:ss.s.
- **Erros:** voltam com o número da linha.

Os gabaritos das duas referências de trabalho estão em
`/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/dados/gabaritos/`, fora do
repositório, como as próprias referências (em
`AUDIOS_RASGO/Trioio/referencias/`).

## Métricas (`avaliar`)

São as convenções públicas de avaliação em MIR (MIREX; Raffel et al.,
"mir_eval", ISMIR 2014), reimplementadas:

- **Andamento:** lido ÷ gabarito. "Certo" vale ±4 %; também são
  reconhecidos metade, dobro, terço, triplo, dois terços e uma vez e meia.
- **Tempos:** F-measure com ±70 ms (Dixon 2007), sem os primeiros 5 s.
- **Tonalidade:** 1 se certa, 0,5 para a quinta, 0,3 para a relativa, 0,2
  para a homônima e 0 nos outros casos.
- **Acordes:** fração do tempo com o mesmo acorde (maior, menor ou
  diminuto); os trechos "N" ficam fora da conta.
- **Seções:** F-measure das fronteiras com ±3 s, sem o começo.

## Limites e próximos passos

- **Nível do pulso estável sob mudança de andamento:** ver "Teste por
  transformação". Pede a programação dinâmica de Ellis ou o pulso local.
- **Linguagem com comportamento:** comparar a melodia com as frases de
  assinatura e as cadências do arquivo, e os ataques com os 26 ciclos.

- **Gabaritos:** sem o do autor, os números das faixas reais são
  comparações entre versões, não acertos. Calibrar com eles:
  - o limiar 0,40 da fração percussiva;
  - o limiar 0,8 da forma por repetição.
- **Croma NNLS** (Mauch e Dixon 2010), para os acordes que ainda erram. O
  croma do grave já está feito.
- **Melodia:**
  - os contornos do MELODIA, para decidir onde há melodia;
  - a correção de oitava do Maravi;
  - depois, o compositor usar a altura predominante quando o YIN não tiver
    leitura (o "contorno da melodia da referência").
- **Tempo forte** (portar o `downbeat` do Maravi) e acompanhamento do pulso
  por programação dinâmica (Ellis 2007) ou por pulso local (Grosche e
  Müller 2011).
- **Ao vivo:** o HPSS causal, só com o passado, fica para a escuta ao vivo.
- **Reconhecer linguagens:** comparar os ataques com os 26 ciclos do
  arquivo, e as alturas com o comportamento das escalas.
