# TRIOIO — composição: motivo, desenvolvimento e forma

**Estado:** em construção desde 2026-10-05.

## Por que esta camada existe

A escuta do autor sobre as quatro primeiras respostas (2026-10-05): elas
"variam bastante uma das outras, ainda meio robótico; o app faz soar, mas
não vejo ainda muita qualidade musical (composições sofisticadas,
elaboradas)". O código explica esse resultado:

1. As decisões mudam numa grade fixa de 0,5 s, sem frase nem gesto.
2. Não há material musical: as alturas são sorteadas numa pentatônica em
   volta do brilho, sem motivo, sem linha e sem harmonia.
3. O ritmo é aleatório. Da referência, só a **quantidade** de ataques era
   usada, não o ritmo.
4. Não há memória nem forma: cada trecho é decidido isoladamente.
5. O timbre é fixo.

## Decisões do autor (2026-10-05)

- **Alturas derivadas da referência:** o campo harmônico vem do croma da
  gravação, e cada modo o segue, desloca ou contradiz.
- **Tempo:** o pulso da referência. A resposta pode estar em fase,
  defasada, em polimetria ou nos buracos dela (hoqueto).
- **Primeira frente:** motivo e desenvolvimento.

Complemento do autor: as escolhas valem "levando em consideração as demais
alternativas também, sem excluí-las completamente". Por isso, cada escolha
é o **padrão**, e as outras ficam como opções reais:

- **Fonte das alturas:**
  - `referencia`: o padrão;
  - `modal`: modo e escala escolhidos pela estratégia;
  - `espectral`: os picos medidos do espectro, sem temperamento;
  - `microtonal`: alturas fracionárias e glissandos.

  A partitura guarda a altura com fração (MIDI fracionário), e a escuta
  guarda os picos espectrais por quadro.
- **Tempo:** `pulso`, o padrão, ou `livre`, com frases, respirações e
  rubato. No modo `auto`, a escolha é por seção: onde o pulso medido é fraco
  ou ambíguo, a resposta passa a tempo livre.
- **Frentes:** motivo e desenvolvimento primeiro, com a forma por seções na
  mesma etapa. Timbre e orquestração vêm depois, com a partitura já
  preparada para várias vozes com papéis diferentes.

## Arquitetura

```
escuta (descritores + croma + pulso + células rítmicas + seções)
  → compositor: motivo da referência → operações → frases → forma → PARTITURA
  → síntese das notas (vozes) + exportação MIDI
```

A estratégia deixa de mover controles contínuos e passa a escrever uma
partitura: notas com tempo, duração, altura, dinâmica e voz. A síntese só
toca a partitura. A partitura também sai em MIDI.

## Etapas

1. **Escuta mais funda:** croma (12 classes de altura), pulso (andamento e
   fase) e células rítmicas (os ataques quantizados na grade do pulso).
2. **Compositor:**
   - **Motivo:** o ritmo de um compasso real da referência e um contorno
     no campo harmônico.
   - **Operações:** repetir, variar, inverter, retrogradar, aumentar,
     diminuir, fragmentar, transpor no campo e deslocar.
   - **Frase em forma de "sentença":** apresentação, repetição variada,
     fragmentação e cadência.
   - **Forma** a partir das seções da referência, com o motivo voltando
     transformado no fim.
   - **Duas vozes:** motivo e baixo.
3. **Voz com timbre que evolui** (ataque brilhante, corpo escurecendo) e
   exportação MIDI.

## Os modos como jogo composicional

| Modo | Ritmo | Alturas | Dinâmica e forma |
| --- | --- | --- | --- |
| seguir | as células da referência, em fase | o campo da referência | acompanha a energia; as seções coincidem |
| transfigurar | mesma grade, motivo invertido ou retrogradado | o campo transposto ou em outro modo | mesma forma, outra matéria |
| contrapor | hoqueto: só nos passos em que a referência não ataca | o campo complementar ou vizinho | dinâmica inversa |
| interromper | segue e corta nas mudanças de seção | o campo da referência | silêncio e depois o fragmento do motivo, forte |

## Estado (2026-10-05): etapas 1 e 2 implementadas

### Escuta mais funda (`src/core/Structure`, `Descriptors` v2)

- **Croma e picos espectrais numa janela longa** (8192 amostras, 5,9 Hz
  por bin). Com 2048, Lá 220 e Dó 261 se fundiam num pico falso; agora o
  croma é calculado só pelos picos, com a frequência interpolada.
- **Pulso:**
  - autocorrelação do fluxo suavizado, com preferência em torno de 120 BPM;
  - **ciclo do compasso:** um 3+3+2 a 120 BPM é lido como 80 BPM, mas se
    repete a cada 2 s; se o tempo não divide o compasso, o tempo passa a ser
    o compasso ÷ 4;
  - a fase vem dos ataques (média circular), e período e fase são
    refinados juntos por regressão.

  Resultado: 120 e 90 BPM exatos, com a fase errando 5 a 6 ms.
- **Grade rítmica:** os ataques são quantizados em compassos de 16 passos.
- **Seções:** cortes por novidade (energia, densidade, brilho, ruído e
  croma), comparando 4 s antes e 4 s depois, com no mínimo 8 s por seção.
- **Campo harmônico** de cada seção: o croma médio, ponderado pela potência.

### Compositor (`src/core/Composer`) e voz (`src/core/ScoreSynth`)

- **Motivo:** o ritmo é o compasso mais típico da referência (o medoide);
  as alturas são um contorno em arco sobre a escala do campo.
- **Frases em forma de sentença** (4 compassos): ideia básica, repetição
  variada, fragmentação em sequência e cadência, que é meia cadência na
  quinta e conclusiva no fim.
- **Variação em desenvolvimento:** cada frase deriva da anterior por uma
  operação da paleta do modo, e a cada terceira frase o motivo da seção
  volta como âncora. O caminho fica escrito na marca da seção.
- **Ritmo vivo:** em seguir e transfigurar, um compasso pode tomar o ritmo
  real do compasso correspondente da referência, com o contorno da frase.
- **Dinâmica e registro** por compasso e por frase, a partir da energia e
  do brilho locais. Onde a referência quase silencia, a melodia respira.
- **Forma:** a primeira seção apresenta, as do meio desenvolvem e a última
  recapitula.
- **Tempo `auto`:** uma seção com pulso de confiança abaixo de 0,3 passa a
  tempo livre, com rubato e respirações de 0,4 a 1,2 s.
- **Voz do motivo:** 8 parciais, com os agudos decaindo mais rápido (o
  ataque brilha e o corpo escurece); a dinâmica abre o timbre; há vibrato
  nas notas longas e glissando no modo microtonal.
- **Baixo:** 3 parciais.
- **Sala:** uma rede de 4 linhas de atraso.
- **Saída:** pico normalizado em −1 dBFS.
- **Partitura em MIDI:** motivo no canal 1 e baixo no canal 2. A fração de
  semitom vai como pitch bend (faixa ±2); a forma vai como marcadores.

**Comando:**

```sh
trioio_biblioteca responder <dados> <id> --modo M [--seed N] \
    [--alturas referencia|modal|espectral|microtonal] [--tempo auto|pulso|livre] \
    [--motor partitura|energia]
```

`partitura` é o motor padrão. `energia` é a primeira cadeia
(`docs/CADEIA_MINIMA.md`).

### Validação

- **Referência sintética:** 120 BPM, célula 3+3+2, Dó maior e depois Lá
  menor (`tests/ComposerTests.cpp`).
  - **seguir:** 69 de 69 notas no campo e na grade.
  - **contrapor:** 68 de 68 notas nos buracos do ritmo da referência
    (hoqueto), com o campo a um trítono.
  - **tempo livre:** as notas saem da grade, com respirações.
  - **auto:** separa a seção pulsada da seção de ruído.
  - **espectral:** gera alturas fora do temperamento.
  - **microtonal:** gera glissandos.
  - **forma:** a recapitulação mantém o contorno do motivo.
  - **som:** 69 de 69 notas tocadas são ouvidas como ataque.
- **Música real** (Dilettante, faixa 7, 3:42; dados descartáveis):
  - 69,9 BPM, célula de 10 ataques, 8 seções, cada uma com seu campo;
  - tempo livre nas seções 4 e 8;
  - correlação de energia: +0,33 em seguir e −0,38 em contrapor;
  - densidade em contrapor: −0,44.

### Limites conhecidos

- **O contorno do motivo é inventado** sobre a escala da referência. A
  melodia real da referência ainda não é rastreada (falta f0).
- **O compasso é sempre de 4 tempos.** Um ciclo de 3 ou 6 tempos é
  detectado, mas ainda escrito em 4.
- **O brilho em contrapor sai positivo (+0,40)** na gravação real: o timbre
  depende da dinâmica, e isso compete com a inversão de registro. Isso fica
  para a etapa de timbre e orquestração.
- **A voz ainda é uma só família de timbre.** Os corpos sonoros do
  inventário RASGO são a etapa seguinte.

## Etapa 3 (2026-10-05): menos contido, mais aberto

Escuta do autor sobre a v2: "interessante, porém contido, como arquivos de
midi... penso em algo mais fora da caixa... com margem para possibilidades
inusitadas e não contidas, algo como música sem previsibilidades". E:
trabalhar nas limitações (contorno inventado, compasso fixo, um timbre só).

### Limitações resolvidas

- **Melodia real:** o analisador v3 mede o f0 por YIN (de Cheveigné e
  Kawahara, 2002) sobre 85 ms reduzidos a cerca de 12 kHz, com
  periodicidade. Acerta inclusive o fundamental ausente: 150 Hz feito só
  dos harmônicos 2 a 5. O motivo toma o contorno da melodia medida em cada
  ataque do compasso típico (lida dentro da nota, porque a janela olha
  85 ms para trás), em graus absolutos da escala. Teste: Dó Mi Sol Mi Ré
  voltam como Dó Mi Sol Mi Ré.
- **Compasso real:** de 2 a 7 tempos, pela razão entre o ciclo de repetição
  do padrão e o tempo. Um 5/4 é lido como 5/4. Um 0-6-8 em 12 passos é
  ambíguo entre 3/4 e 6/8, e o teste exige o ciclo certo (1,5 s), não uma
  das leituras. Num metrônomo sem acento, o compasso fica em 4.
- **Orquestração:** seis famílias de timbre próprias, todas síntese e
  nenhuma amostra:
  - aditivo;
  - sopro (parciais com ar filtrado);
  - metal (parciais de sino 1; 2,76; 5,40; 8,93; 13,34);
  - corda pinçada (Karplus-Strong);
  - membrana (modos inarmônicos, com a altura caindo no golpe);
  - FM (razão 1,41, com o índice caindo no tempo).

  A família de cada seção nasce do caráter dela: ruidosa, densa ou
  brilhante. Transfigurar e contrapor escolhem a família oposta. O baixo
  alterna entre aditivo, FM e membrana.

### O Terceiro Olho (`src/core/ThirdEye`)

Um agente que age **depois** do compositor, sobre a partitura pronta. Em
momentos sorteados, mais prováveis onde a referência muda mais, ele:

- **faz irromper objetos sonoros**, gestos contínuos fora da grade de notas:
  - nuvem granular (grãos em tempos de Poisson);
  - massa de glissandos (fechada que abre, ou aberta que fecha);
  - faixa de ruído móvel;
  - cluster que bate;
  - iteração que acelera ou desacelera;
  - drone que abre o brilho;
  - congelamento do campo, com trêmulo.

  O fim de cada gesto deriva para outro lugar do espaço de alturas,
  proporcionalmente à imprevisibilidade.
- **impõe regras ocultas** por alguns segundos:
  - registro invertido;
  - tempo deformado 3:2 ou 2:3;
  - escala mutante (±¼, ±½, ±1 ou trítono);
  - silêncio súbito, que corta tudo, inclusive o que outras regras
    trouxerem para dentro dele;
  - eco em cânone, em outra voz e timbre;
  - troca de timbre;
  - densificar ou rarefazer;
  - congelar, que troca as notas por um acorde sustentado.

`--imprevisibilidade 0..1` (padrão 0,5): em 0, nada intervém; em 1, há
candidatos a cada cerca de 5 s, aceitos com probabilidade de
0,4 + 0,6 × novidade. **Imprevisível, mas reprodutível:** a mesma seed refaz
as mesmas surpresas. As regras ficam escritas na forma, para o autor
descobrir *depois* de ouvir.

O MIDI leva motivo, baixo e eco nos canais 1 a 3. Os objetos sonoros só
existem no áudio, porque não cabem em notas.

### Validação

- **12/12 testes, sem avisos.** O teste do Terceiro Olho confere:
  - em 0, nenhuma intervenção;
  - em 1, objetos e regras, reprodutíveis pela seed;
  - 37 intervenções em 1,0 contra 6 em 0,2 (seis seeds);
  - silêncio súbito sem nenhuma nota ou objeto dentro dele;
  - as seis famílias e os sete objetos soando entre −22 e −11 dB, finitos e
    sem estourar.
- **Gravação real** (`rasgo_integrated` de 4 out., 19:15, 1:33; dados
  descartáveis): seis versões, de imprevisibilidade 0 a 1, em
  `~/Music/TRIOIO_respostas/v3-fora-da-caixa/`, com WAV, MIDI e a forma.
- **Defeito achado e corrigido:** no modo espectral, um pico se espalhava
  por quartos de tom vizinhos e virava uma "escala" dentro de um semitom.
  Agora só os máximos locais do histograma contam.

### Leitura do momento (2026-10-07)

Até aqui, a novidade da referência decidia **quando** o olho agia, mas
**o que** ele fazia era sorteado com a mesma chance entre os 7 objetos e
as 9 regras. Um trecho denso podia ganhar "densificar", e um trecho já
silencioso podia ganhar "silêncio súbito".

Agora, antes de cada intervenção, o olho **lê o momento**:

- na referência: energia e se ela sobe ou cai, densidade, brilho, ruído,
  se uma mudança de seção está chegando e se o campo harmônico está parado;
- na partitura até ali: quantas notas há, o âmbito da melodia, quanto ela
  se repete, se o timbre é o mesmo há 8 s e o acorde que se destacou da
  vizinhança nos últimos 3 s.

Cada uma das 16 escolhas tem uma **afinidade** com essa leitura. Exemplos:
faixa de ruído e nuvem com referência ruidosa, rarefazer com música densa,
silêncio súbito com energia alta antes de uma mudança clara, congelar com
um acorde que acabou de se destacar.

**A imprevisibilidade passa a dosar quanto a escolha obedece à leitura.**
O peso de cada escolha é exp(afinidade / temperatura), com temperatura
0,08 + 0,5 × imprevisibilidade. Com valor baixo, o olho faz o que o
momento pede; em 1, o sorteio quase se abre de novo, e o inusitado
continua possível. Repetir a escolha anterior pesa menos.

**Parâmetros pelo contexto também:**

- a iteração acelera e a massa de glissandos abre quando a energia sobe;
- o tempo deformado acelera na subida e desacelera na queda;
- o eco usa quinta, quarta ou oitava num campo tonal;
- a troca de timbre evita o timbre que já soava;
- o fim dos gestos deriva para o registro dos segundos seguintes.

Com imprevisibilidade alta, às vezes o olho contraria a direção.

**O congelamento congela o acorde que acabou de soar mais forte**, as
próprias notas da partitura, e não mais o croma médio da referência.
Cada marca na forma diz o motivo da escolha: "· leitura: a música está
densa".

**Calibração** sobre a referência sintética (24 seeds) e a gravação real:

- "acorde forte" pela velocidade absoluta aparecia em quase todo trecho
  (17 de 30 escolhas). Passou a ser o destaque em relação à média dos 8 s
  em volta;
- a planura 0,56 do bloco de ruído rendia afinidade baixa demais;
- "a seção vai mudar" estava inflado, porque o olho já age mais onde a
  referência muda. Agora só conta mudança clara.

**Validação:**

- no bloco de ruído, os gestos de ruído foram de 20% para 45% das
  escolhas (u = 0,3); no tonal ficaram em 0 a 3%. Com u = 1 caem para 32%:
  a escolha fica menos presa à leitura;
- 6 de 7 congelamentos usam exatamente alturas tocadas nos 3 s anteriores;
- toda marca traz a leitura;
- 12/12 testes passando.
- **Gravação real:** a mesma da v3, com as mesmas cinco configurações que
  usam o olho, em `~/Music/TRIOIO_respostas/v4-leitura-do-momento/`.
  Saíram 14 tipos de intervenção, sem um dominante.

### Consequências (2026-10-07)

Escuta do autor sobre a v4: "há variações, mas ficam meio equivalentes". A
causa provável estava na duração: cada intervenção durava de 3 a 15 s e
depois a música voltava ao mesmo lugar. O olho enfeitava, mas não mudava o
rumo.

Agora uma intervenção pode **ficar até o fim da peça**. Isso vale para no
máximo 1 + 3 × imprevisibilidade intervenções por peça, cada uma com
chance de 0,35 + 0,3 × imprevisibilidade. Assim a forma vira
**trajetória**, e cada seed leva a peça para um rumo diferente.

| Intervenção | O que fica |
| --- | --- |
| congelar (regra ou objeto) | o acorde congelado vira o campo: as notas seguintes são levadas às classes de altura dele |
| escala mutante | o desvio fica |
| registro invertido | a inversão fica |
| troca de timbre | o timbre novo fica |
| eco em cânone | a voz do cânone continua |
| densificar / rarefazer | a textura fica assim |
| drone | vira pedal do baixo |
| iteração | a textura segue acelerada ou rarefeita, conforme o gesto |
| nuvem, glissandos, faixa de ruído, cluster | a melodia segue para onde o gesto levou (o desvio de registro do gesto, se for de 2 semitons ou mais) |
| silêncio súbito, tempo deformado | sempre momentâneos |

Na forma, a marca traz `· fica: …`, como em "escala mutante (+0,50 semitom)
por 7,9 s · fica: a escala fica mutada".

**Validação:**

- 12/12 testes passando.
- Em 24 seeds com imprevisibilidade 1, 23 peças têm consequências, com no
  máximo 4 por peça.
- Quando o acorde vira o campo, 6 de 6 casos deixam as notas seguintes,
  até a próxima intervenção, dentro das classes de altura do acorde.
- Em 0, nada fica.
- Amostras dos testes estatísticos ampliadas, porque com a leitura do
  momento o silêncio súbito ficou raro e o teste ficava sem casos para
  conferir.
- **v5:** `~/Music/TRIOIO_respostas/v5-consequencias/`, com a mesma
  gravação e as mesmas configurações da v3 e da v4. Ficaram 13
  consequências entre as cinco versões, de 9 tipos.

### Limites conhecidos

- **As afinidades são heurísticas escritas à mão**, calibradas numa
  referência sintética e numa gravação. A escuta do autor (`escuta.json`,
  próximo passo do roteiro) é que deve ajustá-las.
- **Os objetos sonoros são renderizados sem tempo real** (offline). A versão
  ao vivo precisa de vozes pré-alocadas.
- **A sala é uma só**, e ainda não há espacialização com movimento além do
  pan.


## Mashup de materiais (2026-10-09)

Duas referências compostas juntas (`src/core/Mashup.*`; estudo
transversal, §6.13). A **melodia, a forma e o Terceiro Olho** nascem da
referência A. Da referência B, por escolha, vêm:

| Camada | O que vem de B |
| --- | --- |
| `harmonia` | os acordes lidos em B; a melodia de A se curva a eles (o mashup clássico: a voz de uma canção sobre o instrumental de outra) |
| `ritmo` | o groove: o campo rítmico dos compassos de B, no pulso de B |
| `memoria` | os fragmentos recombinados pelo oráculo e pelo mosaico |
| `corpo` | o corpo espectral: a família espectral toca com os parciais de B |

Uso: `--com OUTRO_ID [--da-outra harmonia,ritmo,memoria,corpo|tudo]` (o
padrão é tudo).

- **Tempo:** B é reescalada para a duração de A, esticando ou encolhendo
  por inteiro, e as duas andam juntas do começo ao fim.
- **Autorização:** B passa pelo mesmo portão de A (o uso `derive`
  declarado).
- **Proveniência:** o manifesto da derivação lista as duas fontes
  (`derived_from` A e B; `derivation.from` = [A, B]) e a relação declarada
  diz quais camadas vieram de B. `decisoes.json` registra `mashupWith` e
  `mashupLayers`.

**Validação** (`tests/MashupTests.cpp`):

- camadas e reescala;
- com a harmonia de B, os acordes lidos são os de B (Solm, Ré) sob uma
  melodia de Dó maior;
- com a memória de B, as passagens mudam em relação à mesma peça sem B;
- sem B, nada muda.

Contar as classes de altura da memória não serve como teste. A escuta lê
o acorde de B pela fundamental implícita (Dó sob Sol e Ré), e o Terceiro
Olho pode transpor um trecho. Isso fica como limite da leitura de altura
em material polifônico.

**Na gravação real** (v25, `/media/luc/4tb_21042021/TRIOIO_respostas/v25-mashup/` (apagada em 2026-10-16: arquivo de teste)):

1. a voz do vocalise sobre a harmonia e o groove do Suno (Solm, Mibm, Ré…);
2. o vocalise com tudo do Suno;
3. o Suno com a memória e o corpo do vocalise;
4. o vocalise sobre o Suno, em maqam Hijaz.

## Forma por repetição (2026-10-09/10)

`--forma repeticao` (`findSectionsByRepetition`, `src/core/Structure.*`):
as seções saem do que **volta**, não do contraste.

1. **A matriz de autossimilaridade** de 1 s: croma normalizado mais
   timbre (energia, densidade, brilho, ruído), com metade do peso.
2. **A novidade de Foote** (J. Foote, ICME 2000): um núcleo em xadrez de 6
   s, afinado por uma gaussiana, corre pela diagonal. As fronteiras são os
   picos acima de média + 0,5 desvio, com no mínimo 8 s por seção.
3. **As letras** (M. Müller, *Fundamentals of Music Processing*, 2015,
   cap. 4). A repetição aparece como **diagonal** na matriz: o trecho
   volta segundo a segundo, na mesma ordem. A matriz é normalizada pela
   média e pelo desvio da peça (z), e dois segmentos têm a mesma letra se
   a média de z ao longo da diagonal (folga de ±2 s) passa de 0,8.
4. **Na composição**, a seção que volta retoma a escala, o motivo, os
   timbres e o papel da primeira vez: "retorno de A (como na seção 1:
   apresentação)". A forma fica registrada ("A B A B").

**Correção registrada:** a primeira versão comparava o perfil **médio**
dos segmentos. Numa canção no mesmo tom e timbre, as médias são iguais, e
o Suno saiu inteiro "A A A A A A". A comparação pela diagonal resolveu
isso. Os renders da versão errada foram substituídos.

**Validação** (`tests/StructureTests.cpp`): um A B A B construído (Dó
maior claro contra Lá menor escuro e ruidoso, 12 s cada) sai **A B A B**,
com as fronteiras em 12, 24 e 36 s; na composição, a 3.ª seção retoma a
1.ª.

**Nas gravações reais** (v30, `/media/luc/4tb_21042021/TRIOIO_respostas/v30-forma-repeticao/` (apagada em 2026-10-16: arquivo de teste)):

- **Suno:** A B C D E F, nenhuma repetição clara. Cada trecho é mais
  parecido consigo mesmo (1,36) do que com qualquer outro; a maior
  semelhança entre trechos diferentes é 0,63 (20–41 s com 91–139 s),
  abaixo do limiar;
- **vocalise:** A B C D E, nenhuma repetição (máximo 0,54), como se espera
  de um exercício que sobe cromaticamente;
- `suno-2-forma-contraste` é a mesma seed pela forma por contraste, para
  comparar.

**Limite:** o limiar de 0,8 foi calibrado num caso construído. Sem saber
a forma real de uma canção (onde entram estrofe e refrão), não há como
dizer se ele é exigente demais para repetições com variação, como as de
uma cover gerada por IA. A forma da faixa do Suno, dada pelo autor,
serviria de primeira referência real.

## Dramaturgia (2026-10-14)

`src/core/Dramaturgy.*`, `Options::dramaturgy` (padrão ligado); auditoria §6,
item 3.

A forma vinha só das seções da referência, e cada seção recebia uma
operação. Havia variedade, mas pouca direção. Agora uma **curva de tensão**
(0..1) para a peça inteira molda a resposta depois de composta. As fontes são
Lerdahl e Krumhansl (2007) e Farbood (2012) para a tensão como curva, e Howat
(1983) para o clímax na seção áurea.

**A curva, por modo** (`tensionCurve`):

| modo | curva |
|---|---|
| seguir | a energia da referência, suavizada em ±4 s e levada a 0..1 pelos percentis 10 e 90 |
| transfigurar | um arco imposto: sobe de 0,25 a 1 na seção áurea (0,618) e desce a 0,15 |
| contrapor | a curva da referência invertida |
| interromper | a curva da referência com 2 ou 3 cortes de 4 a 8 s (a tensão cai a 0,1) |

**O clímax:**

- só existe quando é claro: a curva chega a ≥ 0,75, e o ponto fica entre
  15 % e 90 % da peça;
- o instante vem da energia suavizada **sem** o corte dos percentis, porque
  o corte achata o topo num platô e o primeiro ponto dele não é o pico. Esse
  defeito foi achado pelo teste: o clímax saía 8 s antes.

**O que a curva molda** (`applyDramaturgy`, depois das memórias e antes do
olho):

1. **A dinâmica:** a força de cada nota vai a 0,85–1,15× pela tensão.
2. **A densidade:** nos vales (tensão < 0,35), as vozes de enfeite (eco,
   memória, mosaico) rareiam, até metade no fundo do vale. O motivo, o baixo
   e a percussão nunca são cortados.
3. **O silêncio composto:** em peças de 60 s ou mais com clímax claro, uma
   respiração de um tempo logo antes do clímax. Nenhuma nota começa nela, e
   as que soam são cortadas no começo dela. O olho, que vem depois, pode
   quebrar até esse silêncio.

**Medido (Recording_187, seed 3):**

| modo | forma | clímax | enfeites rareados |
|---|---|---|---|
| seguir | a energia da referência | 2:53, respiração de 806 ms | 0 |
| transfigurar | arco | 2:18 (a seção áurea) | 0 |
| contrapor | invertida | sem clímax claro | 5 |
| interromper | com cortes | 2:53 | 0 |

**O registro (2026-10-14).**

- No clímax, a voz do motivo sobe uma oitava, numa janela contínua de 10 %
  da peça em torno do pico (teto Dó6).
- Nos vales profundos (tensão < 0,15), ela desce uma oitava (piso Dó3).
- As janelas são contínuas: a mudança de registro é de orquestração, uma
  vez na entrada e na saída.
- Medido (Recording_187, seed 3):
  - seguir: 8 notas sobem (o motivo já está alto ali), 31 descem;
  - o arco de transfigurar: 41 sobem.
- Teste: Mi4 vai a Mi5 no clímax e a Mi3 no vale do começo.

**O olho pela tensão (2026-10-14).**

- O compositor passa a curva ao Terceiro Olho (`Options::tensionAt`) e a
  janela da respiração (`Options::breath`).
- O olho aceita um momento candidato com a probabilidade de antes (0,4 +
  0,6 × novidade) vezes 0,5 + tensão: metade nos vales, até 1,5× nos
  trechos tensos.
- Nenhuma intervenção começa na respiração antes do clímax (nem 1 s antes
  dela).
- Teste (`ThirdEyeTests` §9, 8 seeds): 19 intervenções com tensão 0, 43 com
  tensão 1, nenhuma na respiração.
- **Limite:** as regras "obrigatório" do autor (o olho tem de fazer X) ainda
  podem cair na respiração: a regra do autor vale mais que o silêncio
  composto.

**Limites, para decidir com a escuta:**

- **O efeito é modesto.** Nesta peça (heterofonia do Hijaz, poucas notas de
  enfeite nos vales), a dramaturgia age quase só pela dinâmica de ±15 % e
  pela respiração.
- **O próximo passo natural** é a curva também guiar a densidade das vozes
  principais e a orquestração (quantas famílias soam). O registro e o olho
  já são guiados.

**Testes** (`tests/DramaturgyTests.cpp`):

- as quatro curvas e os seus clímax;
- só os enfeites rareiam;
- a respiração vazia antes do clímax;
- mais força no pico que no vale;
- o determinismo;
- a peça curta sem respiração.

As suítes de composição, ciclos, escalas, olho, regras e memórias passam sem
mudança.

## Diálogo com a referência (2026-10-14)

`src/core/Dialogue.*`, `Options::dialogue` (padrão desligado),
`responder --dialogo sim`, e a caixa "diálogo" no painel "Linguagem da
resposta" do app. Auditoria §6, item 4, a metade que não muda a
proveniência.

A resposta corria em paralelo com a referência, com a mesma duração. Com o
diálogo, ela **conversa**:

- **Os pedidos e as respostas** (`dialogueMap`):
  - as frases da referência são os pedidos;
  - as pausas dela são as respostas. Pausa é energia abaixo do meio entre os
    percentis 20 e 80 **e** pelo menos 12 dB abaixo do nível forte, por
    ≥ 0,5 s. O limiar só relativo transformava em "pausas" as oscilações
    de poucos dB de uma música contínua; o teste achou isso.
- **Música contínua:** se as pausas somam menos de 12 % da peça (uma canção,
  o violão da Recording_187), o diálogo vira **alternância de 2 compassos**,
  e a referência abre.
- **Nos pedidos** (`applyDialogue`): as vozes que conduzem (motivo, eco,
  memória, mosaico) calam para escutar, e o baixo, a percussão e a harmonia
  ficam a 60 %, como acompanhamento.
- **Nas respostas:** as vozes que conduzem ganham 10 %. A nota que começa na
  resposta e entra no pedido seguinte é cortada no começo dele: a resposta
  cede a vez.

A fonte é o responsório, a pergunta e resposta como forma (Floyd 1995; a
alternância antifonal; o repente nordestino).

**A resposta continua 100 % sintética.** Só o tempo dela segue as frases da
referência, e o WAV gravado não contém nada da referência.

### Ouvir junto: "▶ juntos" (app)

- O botão toca a resposta e a referência ao mesmo tempo, só para ouvir.
  Nada é gravado: cada uma vai a −3 dB, sob o master, misturadas no próprio
  transporte do app (duas fontes, começando juntas).
- É o jeito de ouvir o diálogo como diálogo.
- **Não se grava nem se publica uma mistura com o áudio da referência.**

**Decisão do autor (2026-10-14):** "vamos criar um arquivo novo — não usar o
áudio da própria referência na resposta"; "a música é uma variação criativa
da versão de referência". A resposta é sempre um arquivo novo, feito só por
síntese. A referência é escutada e analisada; o som dela nunca entra na
resposta, nem misturado, nem como amostra. O "▶ juntos" continua só para
ouvir, sem gravar.

### Medido (seed 4, seguir)

| referência | diálogo |
|---|---|
| vocalise (voz com pausas) | 20 pedidos, 20 respostas (17 s de 82 s); 187 notas que conduzem calaram para escutar |
| Recording_187 (violão contínuo) | alternância de 2 compassos: 19 pedidos, 17 respostas (110 s) |

**Testes** (`tests/DialogueTests.cpp`):

- as frases de 4 s com pausas de 2 s viram 10 pedidos e 10 respostas;
- as vozes que conduzem calam nos pedidos;
- o baixo fica a 60 %;
- a harmonia que entra no pedido é cortada;
- a música contínua vira alternância de 4 s (2 compassos a 120 BPM).

**Limites:**

- **As pausas da vocalise são curtas** (17 s de resposta): o diálogo fica
  rarefeito.

### A resposta retoma o gesto do pedido (2026-10-14)

`imitateCalls` (`src/core/Dialogue.*`), chamado logo depois do
`applyDialogue`, quando o diálogo está ligado.

Antes, a resposta só esperava a vez dela. Agora, como no repente ou no
*trading* do jazz, ela **retoma o gesto** da frase que a referência acabou
de tocar:

1. **O gesto:** as alturas dos ataques do pedido (`pitchAt`: o YIN, ou a
   altura predominante) e o ritmo original. Só a **cauda** que cabe em 90 %
   da resposta: o fim da frase, que é o que se ouve por último.
2. **A transformação, por modo:**
   - seguir imita;
   - transfigurar inverte o contorno;
   - contrapor toca o retrógrado da inversão (as alturas e o ritmo);
   - interromper cita só a primeira metade do gesto e corta.
3. **As alturas:**
   - o gesto é ajustado à **escala que a resposta usa ali**: as classes de
     altura das notas dela (motivo, baixo, harmonia) em ±8 s, a classe mais
     perto a até 2 semitons;
   - a altura de partida é levada à oitava mais perto do registro do motivo
     da resposta.
4. **A voz:** a do motivo, com o timbre do motivo mais próximo. As notas do
   motivo que estavam na resposta dão lugar ao gesto, que entra 80 ms depois
   de a pausa começar.
5. **Sem gesto, nada muda:** se o pedido não tem pelo menos 3 ataques com
   altura, a resposta fica como estava.

**Medido** (vocalise, seed 4):

- **seguir:** 12 de 20 respostas imitam o fim da frase da voz (34 notas);
- **transfigurar:** as mesmas 12, com o contorno invertido;
- as outras 8 vêm de frases sem altura suficiente.

**Testes** (`tests/DialogueTests.cpp` §4):

- frases de Dó-Ré-Mi-Fá-Sol-Lá-Si: a segunda resposta toca Fá-Sol-Lá-Si (a
  cauda que cabe, subindo, na escala da resposta);
- transfigurar desce;
- interromper cita menos;
- um pedido sem altura não muda nada.
- **Defeito achado ao escrever o teste:** os quadros sintéticos estavam
  marcados como silêncio (o padrão do `Frame`), e o `pitchAt` ignora o
  silêncio. A primeira versão do teste passava por coincidência.

**Limite:** a imitação é da altura e do ritmo; o timbre e a dinâmica do
gesto ainda não são retomados.


## As gramáticas narrativas e o que a medição revelou (2026-10-18)

**As gramáticas** (`narrativeCurve`, `Dramaturgy.h`): a forma da peça com
nome, por `--narrativa`, ou sorteada pela árvore de influências (o traço
`forma:`). O conceito é o das "gramáticas plugáveis" do Rasgo Synth Studio,
reescrito aqui, sem código copiado.

- **freytag:** exposição, ascensão, clímax, queda, resolução (G. Freytag,
  *Die Technik des Dramas*, 1863);
- **kishotenketsu:** ki, shō, **ten**, ketsu, a estrutura de quatro partes
  do Leste Asiático, sem autor único. A virada (de 50 a 75 % da peça) muda o
  material: o motivo sobe uma oitava e toma o timbre do contracanto, e a
  percussão rareia pela metade;
- **ondas:** três ondas que crescem;
- **plato:** o patamar que sobe devagar (para os conceitos de repetição);
- **arco:** o clímax perto da seção áurea.

**A mistura:** duas gramáticas se misturam ao longo da peça
(`"ondas+plato"`), a primeira no começo e a segunda no fim.

**O que a medição revelou:** a primeira escuta das gramáticas deu o mesmo
contorno de nível nas três. Os stems mostraram o porquê:

- **as notas seguiam a curva:** a força do motivo ia de 0,32 a 0,85 na
  Freytag;
- **os gestos do Terceiro Olho não seguiam:** soavam entre −22 e −27 dB,
  constantes, tão altos quanto o motivo, e mascaravam a forma.

Desde então:

- **os gestos seguem a curva** (`shapeGestures`, depois do olho), em todas
  as respostas, não só nas com gramática;
- **as curvas explícitas impõem o nível:** o gesto do autor e as gramáticas
  usam a faixa larga (0,6–1,4×) e levam a força até a metade do caminho do
  nível da curva, enquanto antes só escalavam e a forma da referência
  mandava.

A Freytag passou a ter 11 dB entre o vale e o clímax.

A mesma causa vale para o que o autor ouviu na intenção e no gesto, "mais
diferente que nítido": os gestos não acompanhavam.

**Testado** (`DramaturgyTests`): as formas, a mistura das duas e a virada.
A escuta está em `escutas/2026-10-18-narrativas/`, com LEIA-ME.
