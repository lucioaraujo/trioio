# A máquina de timbres do Trioio

Iniciada em 2026-10-14. O que o autor pediu, na ordem:

- "precisamos incluir mais sons e timbres, timbres de instrumentos, talvez
  menos sintéticos";
- "sons de efeitos sonoros como de filmes… timbres de instrumentos de jazz,
  samba, rock, orquestra, câmara, manouche, salsa, bolero, choro, e de muitos
  outros estilos";
- "vocais, vozes, noises", "baixo acústico", "flauta transversal",
  "percussões";
- "o musescore por exemplo traz vários sons próprios de midi, com uma lista de
  timbres vasta — precisamos criar nossos sons";
- "uma máquina de timbres…".

## 1. O princípio

- **Os sons são nossos:** modelos físicos e síntese escritos para o Trioio a
  partir da teoria, com as fontes citadas.
  - Nenhuma amostra e nenhum banco de terceiros (*soundfont*, SFZ). A regra
    de "não incluir arquivos de terceiros" e a decisão de que "a resposta é
    só síntese" continuam valendo.
  - O MuseScore toca uma lista vasta porque usa gravações de instrumentos
    num *soundfont*. O Trioio chega à lista vasta por outro caminho: por
    receitas.
- **Uma máquina, não 200 instrumentos escritos à mão.** Um instrumento é uma
  **receita**: uma combinação de peças. As peças se reaproveitam: a corda
  dedilhada que faz o violão faz também, com outro corpo e outro toque, o
  cavaquinho, o bandolim, o violão de 7 cordas, a guitarra manouche e o
  requinto do bolero.
- **O estilo é um conjunto, não um timbre.** O choro não é "um som de
  choro". São os papéis: a flauta no motivo, o bandolim no contracanto, o
  violão de 7 cordas na baixaria, o cavaquinho na harmonia, o pandeiro na
  percussão. O Trioio já compõe por vozes (motivo, baixo, contracanto,
  memória, ritmo, mosaico, harmonia). O conjunto distribui as receitas por
  essas vozes.

## 2. As peças da máquina

| peça | o que é | já existe |
|---|---|---|
| **excitador** | arco (atrito), palheta (válvula), jato de ar (flauta), lábios (metais), dedo ou palheta de guitarra, baqueta ou martelo, mão na pele, glote (voz), partículas (chocalhos), ruído | arco, palheta, dedo, baqueta, martelo (2026-10-14); glote (scat e coro, cópia do Rasgo Synth); ruído |
| **vibrador** | corda (com rigidez), tubo cilíndrico, tubo cônico, barra, pele, placa ou prato, sino, trato vocal (formantes) | corda, tubo cilíndrico, barra (2026-10-14); pele e sino (antigos, simples); formantes |
| **corpo** | caixa de violino, violoncelo ou contrabaixo; tampo de violão; tampo de piano; cabaça; tubo ressonador; amplificador e caixa (guitarra elétrica) | violino, viola, violoncelo, violão, tubo de marimba (2026-10-14) |
| **articulação** | ataque, legato, vibrato natural, *pizzicato*, abafado, *slap*, surdina, *flatterzunge* | vibrato natural, abafador, dedo que abafa (2026-10-14) |
| **sala** | Trioio, HALL, SPACE | as três |

**O código:**

- `src/core/AcousticVoice.cpp` tem os corpos acústicos.
- Os corpos antigos ficam onde estão: `ScoreSynth.cpp`, as cópias do Rasgo
  Modular, do Rasgo Synth, do Antitotem e do Aquorbium.
- A máquina vai crescer separando as peças em funções (excitador, vibrador,
  corpo) e descrevendo cada instrumento como uma receita.

## 3. Primeira leva (2026-10-14, implementada)

| timbre | modelo | fonte |
|---|---|---|
| **arco** | guia de onda da corda com a curva de atrito do arco (prende e solta: o movimento de Helmholtz); corpo de violino, viola ou violoncelo pelo registro; vibrato que entra depois de 0,22 s | J. O. Smith, "Efficient simulation of the reed-bore and bow-string mechanisms", ICMC 1986; M. E. McIntyre, R. T. Schumacher e J. Woodhouse, "On the oscillations of musical instruments", JASA 74(5), 1983; J. Woodhouse, "The acoustics of the violin: a review", Rep. Prog. Phys. 77, 2014 |
| **palheta** | clarinete: o tubo cilíndrico fechado-aberto (os harmônicos ímpares) e a palheta como válvula de pressão; o som irradiado pela campana | Smith 1986; McIntyre, Schumacher e Woodhouse 1983 |
| **marimba** | barras afinadas 1 : 4 : 10; a baqueta macia, cujo tempo de contato filtra os agudos; o tubo ressonador; nas notas longas, **vibrafone** (1 : 4 : 9,2, o motor e o abafador) | N. H. Fletcher e T. D. Rossing, *The Physics of Musical Instruments*, 2ª ed., Springer, 1998, cap. 19 |
| **piano** | 3 cordas por nota (2 no grave) desafinadas em ~2 cents; a rigidez fₙ = n·f₀·√(1 + B·n²); o martelo a 1/8 da corda; o feltro mais escuro no piano; o decaimento em dois estágios; o abafador; o baque no tampo | G. Weinreich, "Coupled piano strings", JASA 62(6), 1977; A. Chaigne e A. Askenfelt, "Numerical simulations of piano strings", JASA 95(2–3), 1994; H. Fletcher, E. D. Blackham e R. Stratton, "Quality of piano tones", JASA 34(6), 1962 |
| **violão** | Karplus-Strong estendido: o laço afinado, a perda que escurece, o lugar do toque e a polpa do dedo; o tampo (~100, 200 e 400 Hz); o dedo que abafa | D. A. Jaffe e J. O. Smith, "Extensions of the Karplus-Strong plucked-string algorithm", CMJ 7(2), 1983; M. Karjalainen, V. Välimäki e T. Tolonen, "Plucked-string models: from the Karplus-Strong algorithm to digital waveguides and beyond", CMJ 22(3), 1998 |

**Medido:**

- **A altura** (`tests/SynthFamiliesTests.cpp`, Lá 220 Hz = MIDI 57): arco
  56,97; palheta 57,01; marimba 57,00; piano 57,06; violão 56,96.
- **A audição** (`trioio_biblioteca timbres`): a mesma frase em cada
  família.

| família | centróide (potência) | energia acima de 5 kHz |
|---|---|---|
| arco | 1140 Hz | −25 dB |
| palheta | 1117 Hz | −22 dB |
| marimba | 259 Hz | −37 dB |
| piano | 548 Hz | −29 dB |
| violão (depois do ajuste) | 755 Hz | −19 dB |

O violão saiu brilhante na primeira versão (−9 dB acima de 5 kHz, como a
corda pinçada antiga) e foi escurecido.

**Nas respostas:** os cinco entram no sorteio de timbres do motivo, nos
conjuntos coerentes com o caráter da referência
(`motifTimbreFor`, `Composer.cpp`), e pesam pela escuta como os outros.

**Ouvir:**

```
trioio_biblioteca timbres <saida.wav> [família…]
```

A audição desta leva está em
`/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/escutas/2026-10-14-a-16-timbres/acusticos-2026-10-14b.wav`:
arco, palheta, marimba, piano, violão e as duas cordas antigas, para
comparar.

## 3.1 Segunda leva (2026-10-15, implementada)

| timbre | modelo | fonte |
|---|---|---|
| **baixo acústico** | contrabaixo em *pizzicato*: a polpa do dedo como pulso largo e macio (não ruído); o laço que escurece e o grave que dura; o corpo grande (~62, 112 e 205 Hz); o "tum" do dedo no espelho | Jaffe e Smith 1983; Fletcher e Rossing 1998, cap. 10 |
| **flauta transversal** | o jato de ar que leva ~ meio período até a aresta; a aresta como não linearidade em S; o tubo aberto nas duas pontas; o ar no ataque (*chiff*) e o vibrato | P. R. Cook, "A meta-wind-instrument physical model", ICMC 1992; M.-P. Verge, A. Hirschberg e R. Caussé, JASA 101(5), 1997 |
| **surdo** | pele grande com os modos de Bessel (1; 1,59; 2,14…); a altura que cai logo após o golpe; a baqueta de feltro; a nota curta é abafada pela mão | Fletcher e Rossing 1998, cap. 18 |
| **conga** | o tom aberto; no forte, o tapa (*slap*): os modos altos e o estalo | idem |
| **pandeiro** | a pele pequena (o polegar no forte, a ponta dos dedos no fraco) e as platinelas: modos inarmônicos agudos em várias colisões nos primeiros 25 ms | idem |
| **ganzá** | partículas num tubo: a energia do gesto decai e cada grão colide com probabilidade proporcional a ela; cada colisão excita ressoadores agudos | P. R. Cook, "Physically informed sonic modeling (PhISM)", CMJ 21(3), 1997 |

**Afinação em toda a extensão** (`tests/SynthFamiliesTests.cpp` §2c,
desde esta leva). O YIN do Trioio lê de 50 a 1000 Hz, então as notas vão
de MIDI 32 a 83.

- **Instrumentos medidos:** arco, palheta, marimba, piano, violão, baixo
  acústico e flauta, cada um na sua extensão.
- **Achado 1, a flauta saltava de registro.** No primeiro ajuste (jato a
  0,32 do período), 7 de 12 notas saíam num harmônico acima. Uma varredura
  do jato (0,32 a 0,6) contra a pressão (0,5 a 1,0) achou a região estável:
  0,50 a 0,54 do período, com pressão de 0,83 a 0,98. Ficou em 0,54, e a
  dinâmica toda cabe nessa região.
- **Achado 2, a marimba apagava o fundamental no agudo.** O filtro da
  baqueta era absoluto. Agora só filtra os parciais acima do fundamental, e
  o contato encurta no agudo.
- **Resultado:** todas as notas medidas ficam a menos de 0,5 semitom do
  pedido.

**Nas respostas:**

- **o baixo acústico** é uma das 4 opções do baixo;
- **a flauta** entra em dois conjuntos do motivo;
- **as percussões** entram na camada rítmica, num sorteio próprio:
  - grave: tambor, surdo ou conga;
  - agudo: pandeiro, ganzá ou as de antes.

  O sorteio próprio garante que o resto do caminho rítmico não muda.

**Três respostas reais** (Recording_187, seguir, sementes 6, 7 e 8; dados
de teste em `AUDIOS_RASGO/Trioio/dados/sessoes/sessao-20261015…`) usam
arco, marimba, palheta, piano, baixo acústico, surdo, pandeiro e ganzá.

**A audição:**
`/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/escutas/2026-10-14-a-16-timbres/segunda-leva-2026-10-15.wav`.

## 3.2 As receitas da corda dedilhada (2026-10-15, implementadas)

A primeira receita de verdade: o violão virou `pluckString` +
`Pluck`, e os outros instrumentos mudam só a receita.

| receita | material, toque | corpo (A0 · placas) | o que muda |
|---|---|---|---|
| **violão** | náilon, polpa do dedo | 102 · 204 · 398 Hz | (a base) |
| **violão de 7 cordas** | náilon; o grave com dedeira (mais claro, mais perto do cavalete) | 95 · 190 · 380 | a baixaria do choro |
| **requinto** | náilon | 135 · 270 · 520 | o violão pequeno do bolero |
| **cavaquinho** | aço, palheta, o estalo do toque | 260 · 520 · 980 | caixa pequena, decaimento curto |
| **bandolim** | aço, palheta; **ordens duplas** desafinadas em 3 cents e tocadas com 4 ms de intervalo | 230 · 450 · 820 | o brilho trêmulo do par |
| **guitarra manouche** | aço, palheta perto do cavalete | 115 · 230 · **1400 (largo)** | a voz anasalada do ressoador interno Selmer-Maccaferri |

**De onde vêm os corpos:** o violão segue medidas publicadas (Fletcher e
Rossing 1998, cap. 9). Os outros foram estimados pelo tamanho da caixa e
são valores de partida, a afinar pela escuta.

**A extensão de cada instrumento** (`instrumentRange`): uma nota escrita
fora da extensão soa na oitava que cabe. O cavaquinho não desce abaixo do
Ré 4, e a flauta não desce abaixo do Dó 4.

**Medido:**

- **A afinação**, na extensão de cada receita: todas as notas a menos de
  0,5 semitom.
- **A audição** (`cordas-dedilhadas-2026-10-15.wav`):

  | instrumento | centróide | energia acima de 5 kHz |
  |---|---|---|
  | náilon (violão, 7 cordas, requinto) | 0,8 a 1,1 kHz | −16 a −18 dB |
  | aço (cavaquinho, bandolim, manouche) | 2,6 a 2,8 kHz | −8 a −10 dB |

  O aço sai brilhante, como deve. Falta conferir na escuta se não ficou
  áspero.

**Ainda não entram no sorteio aleatório do motivo:** as receitas são do
conjunto por estilo (§7, passo 3), que as põe no papel certo (o 7 cordas
no baixo, o cavaquinho na harmonia).

## 3.3 Terceira leva: sopros e bateria (2026-10-15, implementada)

| timbre | modelo | fonte |
|---|---|---|
| **sax** | a palheta-válvula do clarinete com o reflexo **sem inversão** e o laço de um período inteiro: a série harmônica completa do cone (pares a −3/−9 dB; no clarinete, −45 dB); o som irradiado em dois polos acima de ~5 × f₀ e dois formantes discretos (tenor no grave, alto no agudo) | Smith 1986; R. D. Ayers, L. J. Eliason e D. Mahgerefteh, "The conical bore in musical acoustics", AJP 53(6), 1985 |
| **trompete**, **trombone** | **modelo de sinal**: o brilho que cresce com a força (o k-ésimo harmônico vai com E^(1 + 0,45 (k − 1))), o formante da campana (1,25 kHz; 550 Hz), o ataque que chega de baixo (o "buzz"), uma deriva lenta de altura, o ar | J.-C. Risset e M. V. Mathews, "Analysis of musical-instrument tones", Physics Today 22(2), 1969; J. W. Beauchamp, JAES 30(6), 1982 |
| **prato de condução**, **chimbal** | placa: muitos modos inarmônicos densos e o ruído agudo do toque; o sino do prato; o chimbal fechado (curto) ou aberto (nota longa) | Fletcher e Rossing 1998, cap. 20; T. D. Rossing, *Science of Percussion Instruments*, World Scientific, 2000 |
| **caixa**, **caixa com escovinha** | a pele e a esteira (ruído agudo que segue a pele); com escovinha, o ataque é um varrido | idem |
| **bumbo** | a pele grande, a altura que cai e o estalo do batedor | idem |
| **bongô** | o par agudo (macho ~ 400 Hz, hembra ~ 300 Hz), seco | idem |
| **guiro** | a vareta raspando as ranhuras: um trem de estalos (~ 40 por segundo) num ressoador de cabaça | — |

**O que deu errado no caminho (e ficou registrado):**

- **O lábio como válvula** (Cook 1992; Adachi e Sato 1996) não oscilou de
  forma estável em nenhuma combinação da varredura (raio 0,99–0,997, ganho
  0,03–0,3, pressão 0,85–3, perda 0,85–0,95). Os metais ficaram com o
  modelo de sinal, sempre afinado. Voltar ao modelo físico do lábio é um
  passo futuro.
- **O sax, primeira versão.** A palheta num ponto interno de duas linhas (o
  atalho de Cook para o cone) tinha um laço curto que dominava: soava ~4
  semitons acima.
- **O sax, segunda versão:** caótico no grave. Duas varreduras em Python
  (perda × filtro × inclinação × sopro, notas 44–76) acharam a região
  estável: perda 0,9, inclinação 0,6, sopro ≥ 0,7. Nela a aperiodicidade
  fica em 0,01 em toda a extensão, e a nota firma em 70 ms.
- **O medidor e a sala.** O teste de extensão media a nota depois da sala:
  a reverberação sobre uma nota grave e escura baixava a periodicidade
  abaixo do limiar do YIN, e a medida caía na melodia predominante, que
  pegava um harmônico. Um teste mínimo em C++ mostrou o sax periódico antes
  da sala. Desde então o teste confere o som seco do instrumento.
  - **Pendente:** a sala pesa sobre notas graves sustentadas. Vale ouvir se
    o sax e o trombone no grave ficam embolados na mistura.

**A afinação em toda a extensão:** os 15 instrumentos com altura passam
(§2c de `SynthFamiliesTests.cpp`).

**A audição:**
`/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/escutas/2026-10-14-a-16-timbres/sopros-e-bateria-2026-10-15.wav`.
A ordem é sax, trompete, trombone, prato de condução, chimbal, caixa, caixa
com escovinha, bumbo, bongô e guiro.

**O comando de audição** ganhou `--notas 44,52,…`: cada nota por 1,2 s,
para medir a extensão.

## 3.4 A voz (2026-10-15, implementada)

O modelo é a fonte e o filtro: a glote dá pulsos de ar, e o trato vocal os
molda em vogal.

- **A fonte:** o pulso de Rosenberg (abre em ~42 % do período, fecha em 12
  a 20 %; mais rápido no forte, que é mais brilhante). A saída é a sua
  derivada (a radiação dos lábios). Fontes: A. E. Rosenberg, JASA 49(2B),
  1971.
- **O trato:** cinco formantes em cascata (D. H. Klatt, JASA 67(3), 1980).
- **As vogais:** a, é, i, ó, u, com as médias de homens e mulheres de G. E.
  Peterson e H. L. Barney (JASA 24(2), 1952). A vogal de cada nota é
  sorteada pela semente; o a e o ó saem mais, como num vocalise.
- **O registro:** masculino abaixo do Dó 4 (o terceiro formante perto do
  formante do cantor, ~2,8 kHz) e feminino acima. No agudo, o trato é mais
  curto e o primeiro formante sobe com a nota (a afinação de formantes das
  sopranos; J. Sundberg, *The Science of the Singing Voice*, 1987).
- **A expressão:** o vibrato (5,5 Hz, ±0,3 semitom, entrando aos 0,28 s),
  o jitter (0,4 %), o shimmer (3 %) e o ar da glote aberta.

| timbre | o que é |
|---|---|
| **voz** | o canto, do baixo à soprano (MIDI 38–84) |
| **sussurro** | só o ar pelos formantes, sem altura |
| **boca chiusa** | o murmúrio nasal: o F1 baixo e os formantes altos abafados |

**Medido:** a voz afinada de MIDI 40 a 83, e a boca chiusa de 45 a 72.

**Ainda não tem:** consoantes, palavras e a mudança de vogal dentro da
nota. O canto é um vocalise.

**Nos conjuntos:**

- **bolero e samba:** a voz no motivo;
- **o conjunto novo *a cappella*:** vozes no motivo, no contracanto e no
  baixo; boca chiusa na harmonia e na memória; sussurro no mosaico; sem
  instrumentos.

**A audição:** `vozes-2026-10-15.wav`.

**Respostas reais** (Recording_187, semente 6): *a cappella* e bolero.

## 3.5 A bateria completa, os elétricos e o rock (2026-10-15, implementados)

O autor pediu "bateria".

**O kit agora tem** bumbo, caixa, caixa com escovinha, chimbal, prato de
condução e as duas peças novas:

- **tom-tom:** a pele média; a nota escolhe o tom entre 80 e 240 Hz;
- **prato de ataque:** a placa grande e o chiado largo que abre depois do
  golpe.

**Os elétricos:**

- **A corda:** a mesma do violão (`pluckString`), de aço e sem caixa.
- **O captador:** o captador magnético ouve a corda num ponto (o pente a
  ~1/5 da corda).
- **O amplificador valvulado:** o grave cortado na entrada, o ganho, a
  saturação assimétrica (os harmônicos pares da válvula), o tom e a caixa
  (a ressonância de ~110 Hz do falante de 12" e o corte acima de ~4,5 kHz).
  Fontes: D. T. Yeh, J. S. Abel e J. O. Smith, DAFx 2007; J. Pakarinen e D.
  T. Yeh, "A review of digital techniques for modeling vacuum-tube guitar
  amplifiers", CMJ 33(2), 2009.

| timbre | o que é |
|---|---|
| **guitarra elétrica** | levemente saturada (ganho 3,5) |
| **guitarra distorcida** | o amplificador no talo (ganho 16) |
| **baixo elétrico** | corda grave de aço, os dedos, um pouco de válvula |

**Medido:** os três afinados em toda a extensão (guitarras de MIDI 40 a 83,
baixo de 32 a 55).

**O conjunto rock:**

- guitarra distorcida no motivo;
- guitarra elétrica no contracanto, no mosaico e na base;
- voz na memória;
- baixo elétrico;
- bateria.

**As figuras do rock:**

- **A base:** os acordes viram *power chords* (a fundamental entre Mi 2 e
  Mi 3, a quinta, a oitava) em colcheias, com o tempo acentuado.
- **O baixo:** colcheias na fundamental; a última antes da mudança antecipa
  a próxima.
- **A levada:**
  - o bumbo no 1, no 3 e no "3 e";
  - a caixa no 2 e no 4;
  - o chimbal em colcheias;
  - o prato de ataque na cabeça de cada frase de oito compassos;
  - no oitavo compasso, a virada nos tons, do agudo ao grave.

**Testado:** os *power chords*, a caixa no 2 e no 4, o prato de ataque na
cabeça, o baixo em colcheias.

**Resposta real** (Recording_187, semente 6): 356 ataques de harmonia, 380
notas de baixo, 775 toques de percussão.

**A audição:** `rock-2026-10-15.wav`.

**Ainda falta** a bateria de escola de samba: surdos de primeira, segunda e
terceira, caixa, repique, tamborim, agogô, cuíca e chocalho.

## 3.6 A bateria de escola de samba (2026-10-15, implementada)

| timbre | modelo |
|---|---|
| **tamborim** | a pele pequena e tensa (420–720 Hz), a baqueta, seco |
| **repique** | o tambor agudo que chama (220–380 Hz), baqueta e mão |
| **agogô** | duas campânulas de metal, a aguda (~920 Hz) e a grave (~660 Hz), com parciais de sino (1; 2,71; 5,12; 8,21) |
| **cuíca** | o tambor de fricção: a vareta esfregada prende e solta (um trem de pulsos, como o arco), e o polegar na pele faz a altura subir ("uí") ou descer ("uó"); os modos da pele seguem a altura |
| **chocalho** | as fileiras de platinelas que se chocam a cada movimento |

Os surdos de primeira, segunda e terceira são o surdo já existente, em
três afinações:

- **primeira:** ~55 Hz, a mais grave;
- **segunda:** ~73 Hz;
- **terceira, o cortador:** ~98 Hz.

**O conjunto escola de samba** (`--conjunto escola`):

- o puxador (voz) no motivo e o coro na memória;
- cavaquinho e violão de 7 cordas;
- a bateria, em cada metade do compasso (o samba é em dois):
  - **os surdos:** a segunda no 1 e a primeira no 2 (a marcação); a
    terceira recorta entre elas;
  - **a caixa** em semicolcheias acentuadas;
  - **o repique**, que chama a cada oito compassos;
  - **o tamborim** no telecoteco;
  - **o agogô** nas duas campânulas;
  - **a cuíca** sobe no 2 e desce no 4;
  - **o chocalho** é contínuo.

**Testado:** a primeira cai sempre no 2; cuíca, tamborim e agogô estão
presentes.

**Resposta real** (Recording_187, semente 6): 3672 toques de percussão. O
master deixou a peça em −16,0 LUFS, com pico verdadeiro de −1,1 dBTP.

**A audição:** `escola-de-samba-2026-10-15.wav`, na ordem tamborim,
repique, agogô, cuíca, chocalho e surdo.

**São aproximações de partida.** As baterias têm a identidade de cada
escola (as bossas, as paradinhas, o andamento), e a escuta do autor afina.

## 3.7 Os efeitos de cinema (2026-10-15, implementados)

**A síntese:** procedural, em `ScoreSynth.cpp`, `renderFilmFx`. O som de
cada efeito sai do processo físico que o produz. Fontes: A. Farnell,
*Designing Sound*, MIT Press, 2010; para as bolhas, K. van den Doel, ACM
TAP 2(4), 2005, e M. Minnaert, Phil. Mag. 16, 1933.

| efeito | processo |
|---|---|
| **vento** | ruído numa banda que as rajadas movem (um passeio aleatório lento) e o assobio nas rajadas fortes |
| **chuva** | o chiado agudo e as gotas: tons curtos e agudos, cada uma num lugar do estéreo |
| **trovão** | o estalo (ruído largo e curto) e o ronco grave, com solavancos que decaem por segundos |
| **água** | o riacho: bolhas que nascem e sobem de tom |
| ***whoosh*** | algo passa: a banda de ruído sobe e desce (Doppler), a força em sino, de um lado ao outro |
| ***riser*** | um feixe de dentes-de-serra que sobe uma oitava e meia e o ruído que abre, em crescendo, até cortar seco |
| **impacto** | o boom grave que cai de 55 a 32 Hz, o estalo e a cauda |
| ***braam*** | o metal grave das trilhas: muitas vozes numa quinta grave, desafinadas, cujo brilho cresce com a força |

**Na forma** (`FilmEffects.h`): o Trioio não espalha os efeitos ao acaso.
Eles marcam a dramaturgia da resposta, como numa trilha:

- **no clímax:** o *riser* nos 4 s antes, o impacto e o *braam* no
  clímax;
- **nas mudanças de seção:** o *whoosh* (no máximo um a cada 20 s, longe
  do clímax);
- **nos trechos rarefeitos** (tensão baixa por 8 s ou mais, até três por
  peça), uma ambiência escolhida pelo brilho da referência:
  - escura: vento;
  - média: água;
  - clara: chuva, com um trovão quando o trecho dura 12 s ou mais.

Os efeitos são gestos, fora do sorteio do Terceiro Olho.

**Onde ligar:** a caixa **efeitos de cinema na dramaturgia**, em
**Linguagem ▸**, ou `--efeitos sim` na linha de comando.

**Testado** (`tests/FilmEffectsTests.cpp`):

- o *riser* termina no clímax; o impacto e o *braam* caem nele;
- dois *whoosh* (a seção perto demais da anterior fica de fora);
- a ambiência pelo brilho;
- os oito efeitos soam, finitos e sem estouro.

**Respostas reais:** Recording_187, semente 6, na orquestra e no
automático. As duas têm o clímax, 4 *whoosh* e 3 ambiências de chuva com
trovão. O master deixou as duas em −16 LUFS, com limitador de 0,2 a 3,4
dB: os efeitos não dominam a mistura.

**A audição:**

```
trioio_biblioteca efeitos <wav>
```

Gera `efeitos-de-cinema-2026-10-15.wav`, com os oito efeitos em
sequência.

## 3.8 O órgão, a palheta livre, o forró e o tango (2026-10-15, implementados)

| timbre | modelo | fonte |
|---|---|---|
| **órgão** | as rodas fônicas (quase senoides) nos pés do órgão; a registração 888000000 (16', 5⅓', 8'); o clique da tecla; a percussão no 3º harmônico; um pouco de válvula; a Leslie rápida (o rotor faz o som pulsar entre os canais, com Doppler leve) | L. Hammond, patente US 1.956.350, 1934; D. Leslie, patente US 2.489.653, 1949; J. Smith et al., "Doppler simulation and the Leslie", DAFx 2002 |
| **sanfona** | a palheta livre (espectro ~k^-0,8) e o registro musette: três palhetas, duas a ±12 cents, cujos batimentos fazem o trêmulo | J. P. Cottingham, "Acoustics of free-reed instruments", Physics Today 64(3), 2011 |
| **bandoneón** | duas palhetas em oitava, sem musette; o ataque marcado do fole | idem |
| **zabumba** | a maceta na pele de cima (o "tum" a ~68 Hz) ou o bacalhau na de baixo (o estalo seco); nota grave = maceta | Fletcher e Rossing 1998, cap. 18 |
| **triângulo** | a barra livre dobrada (1; 2,76; 5,40; 8,93…); aberto, ou abafado na nota curta | idem, cap. 19 |

**O órgão soa uma oitava abaixo da tecla.** É o tirante de 16', como num
Hammond real com essa registração. O teste espera isso, e a extensão
medida começa no MIDI 44, porque abaixo dele o 16' cai sob os 50 Hz do
medidor.

**Os conjuntos:**

- **forró** (o trio pé-de-serra): a sanfona no motivo, nos baixos e nos
  acordes; voz no contracanto; zabumba e triângulo. As figuras, em cada
  metade do compasso:
  - **a zabumba:** a maceta no 1 e no "1 a" (a pontuada), o bacalhau nos
    contratempos;
  - **o triângulo:** aberto no contratempo e abafado no resto;
  - **a mão direita da sanfona:** nos contratempos;
  - **o baixo:** a fundamental pontuada e a quinta.
- **tango:** o bandoneón no motivo e na memória, violino no contracanto,
  piano na harmonia, contrabaixo, sem percussão. A figura alterna:
  - **um compasso de *marcato*:** quatro acordes, o baixo no 1 e no 3;
  - **um compasso de síncopa 3-3-2.**
- **rock:** o órgão entra no contracanto.

**Testado:**

- a maceta no lugar certo (8 em dois compassos);
- o triângulo aberto no contratempo;
- o tango: 4 + 3 ataques em dois compassos.

**Respostas reais** (Recording_187, semente 6): forró, tango e rock.

**A audição:** `orgao-sanfona-2026-10-15.wav`.

## 3.9 As bossas, as paradinhas e mais ambiências (2026-10-15)

**A forma da bateria** (`StyleFigures.cpp`).

- **Na escola de samba,** em frases de 16 compassos:
  - **o 15º:** o repique chama;
  - **o 16º, a paradinha:** a bateria para na segunda metade e só acentua a
    bossa em uníssono, no 1, no "2 a" e no 4 da metade (surdos, caixa,
    repique e tamborim juntos). O canto continua;
  - **o 1º da frase seguinte, a volta:** os três surdos juntos no 1, só
    depois de uma paradinha.
- **No samba,** um breque a cada 8 compassos: a segunda metade para, e só o
  surdo e o pandeiro marcam juntos o 1 e o "2 a".

**Testado:** na paradinha, a segunda metade tem exatamente três ataques;
na volta, os três surdos.

**As ambiências novas** (`renderFilmFx`):

| ambiência | processo |
|---|---|
| **fogo** | estalos em rajadas (a madeira que racha), o chiado que tremula, o ronco grave da chama |
| **multidão** | 14 vozes ao longe, cada uma um zumbido glotal com a sua altura, que muda de vogal a cada sílaba (~150 ms), com pausas: o murmúrio sem palavras |
| **cidade** | o rumor grave do tráfego, carros que passam (Doppler, de um lado ao outro), uma buzina rara ao longe |

**A escolha** fica entre duas por faixa de brilho, pela semente:

- escura: vento ou fogo;
- média: água ou cidade;
- clara: chuva ou multidão.

**Respostas reais:**

- **escola de samba:** 3615 toques;
- **contrapor com efeitos, semente 8:** chuva, multidão e trovão.

**A audição:** `efeitos-de-cinema-2-2026-10-15.wav`, os onze efeitos.

## 3.10 Os naipes (2026-10-15)

Um naipe são vários solistas na mesma nota. Cada músico tem uma afinação um
pouco diferente (alguns cents), entra alguns milissegundos antes ou depois,
e tem o seu próprio vibrato (velocidade e fase). Fontes: J. Meyer,
*Acoustics and the Performance of Music*, 2009, cap. 9; S. Ternström, "Choir
acoustics", 2003.

| naipe | músicos | dispersão |
|---|---|---|
| **naipe de cordas** | 8 arcos | ±9 cents, 25 ms |
| **naipe de metais** | 4: trompetes no agudo, trombones no grave | ±7 cents, 18 ms |
| **coro de vozes** | 6 vozes | ±12 cents, 30 ms |

**Nos conjuntos:**

- **orquestra:** naipes de cordas no motivo, no baixo e na harmonia, e o
  naipe de metais no mosaico;
- **salsa:** o naipe de metais no motivo;
- **escola de samba:** o coro de vozes na memória.

**Medido:** os três afinados.

**A audição:** `naipes-2026-10-15.wav`, que compara cada solista com o seu
naipe.

## 4. O mapa dos estilos (os conjuntos a montar)

Os instrumentos de cada conjunto estão na ordem: motivo · contracanto ·
baixo · harmonia · percussão.

| estilo | conjunto |
|---|---|
| **choro** | flauta transversal ou bandolim · clarinete · violão de 7 cordas (baixaria) · cavaquinho · pandeiro |
| **samba** | cavaquinho, voz · trombone, flauta · violão de 7 cordas · cavaquinho · surdo, tamborim, pandeiro, agogô, cuíca, reco-reco, ganzá |
| **jazz** | sax tenor, trompete · trombone · **baixo acústico** (*pizzicato*, *walking*) · piano · bateria com escovinha e prato de condução |
| **manouche** | violino, guitarra manouche (solo) · clarinete · contrabaixo · guitarra em *la pompe* · — |
| **salsa** | trompetes, trombones · flauta · baixo (*tumbao*) · piano em *montuno* · congas, timbales, bongô, clave, cencerro, guiro |
| **bolero** | voz, requinto · violino · violão, baixo · violão · maracas, bongô |
| **rock** | guitarra elétrica (amplificador) · órgão · baixo elétrico · guitarra base · bateria |
| **orquestra** | cordas (violinos, violas, violoncelos, contrabaixos), madeiras (flauta, oboé, clarinete, fagote), metais (trompa, trompete, trombone, tuba), tímpanos, harpa, celesta |
| **câmara** | quarteto de cordas; quinteto de sopros; trio com piano |
| **outros** (a crescer) | forró (sanfona, zabumba, triângulo), maracatu (alfaias, gonguê), baião, frevo (metais), tango (bandoneón), música andina (charango, quena), gamelão (metalofones), rāga (sitar, tabla, tanpura), música árabe (oud, ney, darbuka) |

**O que o conjunto faz na composição:**

- dá a cada voz do Trioio a sua receita;
- dá os registros de cada instrumento (a flauta não desce abaixo do Dó 4;
  o contrabaixo vive no grave);
- dá as articulações próprias (a *la pompe*, o *tumbao*, a baixaria).

O que o estilo tem de ritmo, harmonia e forma já está em parte no Trioio
(ciclos rítmicos, idiomas de escala, groove). O conjunto liga tudo isso aos
corpos.

## 4.1 O conjunto implementado (2026-10-15)

`src/core/Ensemble.h/.cpp`. A escolha fica em **Linguagem ▸ Conjunto** e
na CLI (`--conjunto choro|samba|jazz|manouche|bolero|charanga|camara|orquestra`).
Ela reatribui os timbres pelo papel de cada voz, depois do diálogo e do jogo
e antes do Terceiro Olho, que continua livre para desviar.

| conjunto | motivo | contracanto | memória | baixo | harmonia | percussão (grave · agudo) |
|---|---|---|---|---|---|---|
| choro | flauta | bandolim | clarinete | 7 cordas | cavaquinho | pandeiro · pandeiro |
| samba | cavaquinho | flauta | bandolim | 7 cordas | cavaquinho | surdo · pandeiro, ganzá ≥ Sol♯ 5 |
| jazz | palheta | piano | vibrafone | baixo acústico | piano | tambor · chocalho |
| manouche | violino | guitarra manouche | clarinete | contrabaixo | guitarra manouche (*la pompe*) | sem percussão |
| bolero | requinto | violino | violão | contrabaixo | violão | conga · ganzá (maracas) |
| charanga (salsa) | flauta | violinos | violinos | contrabaixo | piano (*montuno*) | conga · ganzá (guiro) |
| câmara | quarteto de cordas em todas as vozes | | | | | sem percussão |
| orquestra | cordas | flauta | clarinete | cordas | cordas | surdo · marimba |

Em todos os conjuntos, o mosaico (voz 5) recebe um instrumento do mesmo
grupo.

**Regras:**

- A extensão de cada instrumento põe as notas na oitava que cabe.
- As famílias proibidas pelo autor não entram.
- A linha da forma diz qual conjunto tocou e quantas notas mudaram.

**Testado** (`tests/EnsembleTests.cpp`):

- os papéis do choro;
- o grave e o agudo do samba;
- a câmara sem percussão;
- a regra do autor que proíbe uma família;
- os nomes.

**Respostas reais:** Recording_187, seguir, semente 6, nos conjuntos choro,
jazz, samba, manouche e câmara (`sessao-20261015…`).

**Limites desta versão:**

- **Instrumentos que ainda faltam:** os metais e o sax (o jazz e a salsa
  usam a palheta e os violinos), a bateria de jazz, o bongô, o guiro e a
  voz.
- **Ritmo e harmonia:** as figuras vieram no passo seguinte (§4.2).

## 4.2 As figuras de cada estilo (2026-10-15)

`src/core/StyleFigures.h/.cpp`. O conjunto troca os corpos; as figuras
trocam o jeito de tocar.

- **O que muda:** a harmonia (voz 6) e o baixo (voz 1) que o Trioio compôs
  (os acordes sustentados e as fundamentais) são rearticulados no compasso
  e no pulso lidos na referência. A rearticulação vale só nos trechos com
  pulso, e só nos acordes e nas fundamentais que duram pelo menos um tempo.
- **Swing:** no jazz e no manouche, as colcheias do contratempo vão para a
  posição de tercina.

| estilo | harmonia | baixo |
|---|---|---|
| manouche | *la pompe*: os tempos 1 e 3 para baixo, os tempos 2 e 4 curtos e abafados (a formação perde a percussão: a *pompe* é o ritmo) | em dois: a fundamental e a quinta |
| jazz | *comping* de Charleston com quatro variações por compasso | *walking*: a fundamental, notas do acorde, a aproximação cromática da próxima fundamental |
| samba | a batida do cavaquinho (partido-alto) em semicolcheias | em dois: curto no 1, longo no 2, na quinta abaixo (a resposta do surdo); a baixaria na mudança de acorde |
| choro | o "centro" do cavaquinho | em dois, com a baixaria do 7 cordas em toda mudança de acorde com espaço |
| bolero | colcheias 1 · 2 · 2e · 3 · 4 | a fundamental no 1 e a quinta no 3 |
| charanga (salsa) | o *montuno* em dois compassos | *tumbao*: o "2 e", que antecipa, e o 4, nunca o 1 |

- **A baixaria:** quatro colcheias que saem uma quinta antes da próxima
  fundamental e chegam a ela por tons, terminando na sensível. Para chegar
  a Fá: Si♭, Dó, Ré, Mi → Fá. A corrida não desce abaixo do Si 1 (a corda
  mais grave do 7 cordas); perto do fundo, chega por cima.
- **Câmara e orquestra** não têm figura: a harmonia continua sustentada.

**Testado** (`tests/EnsembleTests.cpp` §7):

- a *pompe* (um acorde por tempo; 2 e 4 curtos);
- o *walking* (uma nota por tempo, a aproximação cromática no último tempo
  antes da fundamental seguinte);
- o *tumbao* (nunca no 1);
- a baixaria (a última nota a um ou dois semitons do alvo);
- a câmara sem figura;
- sem pulso, nada muda.

**Achado no teste:** a primeira baixaria terminava na própria fundamental,
uma colcheia cedo. Agora termina na sensível.

**Seis respostas reais:** Recording_187, seguir, semente 6, nos seis
estilos (`sessao-20261015010142…010642`).

| estilo | ataques de harmonia | notas de baixo | baixarias |
|---|---|---|---|
| choro | 444 | 211 | 10 |
| samba | 263 | 195 | 2 |
| jazz | 93 | 191 | — |
| manouche | 171 | 92 | — |
| bolero | 215 | 92 | — |
| charanga | 200 | 102 | — |

**São aproximações de partida.** Os padrões vêm da prática descrita nas
fontes abaixo. As variantes regionais e de época são muitas, e a escuta do
autor afina.

- **A batida do cavaquinho e a baixaria:** a prática do choro e do samba
  descrita em M. Sève, *Vocabulário do choro*, Lumiar, 1999; C. Sandroni,
  *Feitiço decente*, Zahar, 2001 (o paradigma do Estácio, o partido-alto).
- **O *tumbao* e o *montuno*:** R. Mauleón, *Salsa Guidebook*, Sher Music,
  1993.
- **O *walking* e o *comping*:** M. Levine, *The Jazz Theory Book*, Sher
  Music, 1995.
- **A *pompe*:** a prática de Django Reinhardt descrita por M. Dregni,
  *Django*, Oxford University Press, 2004.

## 4.3 A percussão de cada estilo e o conjunto salsa (2026-10-15)

**A percussão** (`StyleFigures.cpp`, `percussionPattern`): nos trechos com
pulso, a voz 4 vira a percussão do estilo.

| estilo | percussão |
|---|---|
| **jazz** | o prato de condução ("ding, ding-a", com swing de tercina), o chimbal no 2 e no 4, a escovinha varrendo no 2 e no 4, o bumbo *feathered* nos quatro tempos e um comentário da caixa a cada quatro compassos |
| **salsa** | a conga em *tumbao* (o tapa no 2; os tons abertos no 4 e no 4 e), o bongô em *martillo* (colcheias, o tempo acentuado) e o guiro (longo no 1 e no 3, curtos no meio) |
| **charanga** | a conga e o guiro, sem o bongô |
| **samba** | o surdo (abafado no 1, aberto no 2, em cada metade), o pandeiro em semicolcheias (o polegar no tempo, o acento na quarta) e o ganzá contínuo |

**Os conjuntos:**

- **O jazz agora tem** sax no motivo, trompete no contracanto e a bateria.
- **O conjunto novo, salsa**, tem trompete no motivo, trombone no
  contracanto, sax na memória, flauta no mosaico, piano no *montuno*, baixo
  em *tumbao*, congas, bongô e guiro. A charanga continua como a formação
  de flauta e violinos.
- **A orquestra ganhou** trombones no mosaico.

**Testado:** o prato no "2 e" cai na tercina (swing), e o chimbal soa só no
2 e no 4.

**Três respostas reais** (Recording_187, seguir, semente 6):

| conjunto | toques de percussão |
|---|---|
| jazz | 817 |
| salsa | 979 |
| samba | 2070 |

## 4.4 O fraseado de cada estilo (2026-10-15)

`applyStylePhrasing` (`StyleFigures.cpp`) toca a melodia que o Trioio
compôs, o motivo (voz 0) e o contracanto (voz 2), com a articulação de cada
estilo. O ritmo (swing, síncope, corridas) muda só nos trechos com pulso; a
expressão (vibrato, portamento, bends) vale em toda a peça.

| estilo | fraseado |
|---|---|
| jazz, manouche | as colcheias do contratempo na tercina (swing); a aproximação cromática (meio tom abaixo, rápida) antes das notas longas; a "caída" (−3 semitons) no fim da frase (jazz) |
| choro | apojaturas (a nota de cima, um tom ou meio tom); as corridas em semicolcheias que preenchem os saltos de quarta ou mais, chegando por grau |
| samba, escola, forró, salsa, charanga | a síncope: a nota do tempo antecipada em uma semicolcheia (40 %); no forró, também apojaturas |
| bolero, *a cappella*, câmara, orquestra | legato: o portamento até a próxima nota colada (até a quinta); vibrato nas notas longas |
| tango | o *marcato* nos tempos fortes (curto e acentuado); o arrastre (a entrada uma terça abaixo, rápida) |
| rock | os bends (a nota longa sobe de um tom abaixo) e o vibrato largo |

**Testado:**

- o swing (a colcheia do contratempo na tercina);
- a corrida do choro;
- o portamento do bolero (a curva chega à diferença de altura);
- a síncope do samba.

**Respostas reais** (Recording_187, semente 6):

| estilo | o que o fraseado fez |
|---|---|
| jazz | 61 colcheias com swing, 29 aproximações, 1 caída |
| choro | 40 apojaturas, 7 corridas |
| bolero | 31 portamentos, 107 vibratos |
| rock | 23 bends, 117 vibratos |

**Limites:** o fraseado articula a melodia existente; ele não recompõe a
linha melódica no idioma do estilo. Fazer isso (o vocabulário de frases do
choro, as *licks* do jazz) é um passo maior, que pede um corpo de
referências próprias.

## 4.5 O conjunto lido na referência (2026-10-15)

`readEnsemble` (`Ensemble.cpp`): o Trioio escolhe o estilo pelo que
escutou. A escolha fica em **Linguagem ▸ Conjunto ▸ lido na referência (o
modo responde)**, ou `--conjunto lido` na linha de comando. São regras de
escuta escritas, não um classificador treinado, e o porquê vai inteiro
para a linha da forma.

**A leitura**, nesta ordem:

1. **Sem pulso** (menos de 30 % da peça):
   - uma voz (periodicidade > 0,7 e pouco ruído): *a cappella*;
   - senão, orquestra (denso) ou câmara (rarefeito).
2. **Com pulso, o ciclo reconhecido** (`rankCycles`, só se claro):
   - clave ou tresillo: salsa;
   - baião: forró;
   - bossa nova: samba;
   - shuffle: jazz.
3. **O swing medido** (> 7 % do tempo): jazz; manouche acima de 170 BPM.
4. **O andamento e o caráter:**
   - ≥ 112 BPM, ruidoso e brilhante: rock;
   - < 85 BPM: bolero se cantado, tango se não;
   - denso: samba;
   - o resto: choro.

**O modo responde:**

- **seguir e interromper:** o estilo lido;
- **transfigurar:** o vizinho:
  - choro ↔ samba, jazz ↔ manouche, salsa ↔ charanga, bolero ↔ tango;
  - forró → choro, rock → jazz, câmara ↔ orquestra, *a cappella* →
    câmara;
- **contrapor:** o contrário em energia. Os enérgicos (rock, escola,
  salsa, samba, forró) contra os contidos (câmara, *a cappella*, bolero,
  tango, orquestra), escolhido pela semente.

**Lido nas três referências de teste:**

| referência | leitura | conjunto |
|---|---|---|
| vocalise | sem pulso (14 %), uma voz (0,80) | *a cappella* |
| suno | 85 BPM, lento | tango |
| rec187 | 74 BPM, lento, cantado | bolero |

No rec187, transfigurar deu tango e contrapor deu forró.

**Testado:** uma voz sem pulso é lida como *a cappella*; transfigurada vira
câmara; contraposta vira um dos estilos enérgicos.

**Limites:** as regras são grossas. Muitas músicas cairão no choro ou no
samba por exclusão. Um classificador mais fino pediria descritores de
timbre por instrumento, e a escuta do autor vai mostrar onde as regras
erram.

## 5.0 O rasgo modular (2026-10-15/16)

**O que o autor ouviu** (15/10, depois dos timbres e dos estilos): "ainda
muito midi — se possível trazer mais da síntese modular criativa para
dentro dos timbres, sons inauditos, não protocolares".

**O rumo** deixa de ser o realismo. O corpo acústico é o ponto de partida,
e o Trioio o atravessa com processos modulares. Cada voz, em cada seção,
recebe um patch, e a profundidade cresce com a tensão da peça.

O código é `src/core/ModularRasgo.h/.cpp`, próprio. A genealogia são os
módulos SHAPE e LPG do Rasgo Modular e o Rungler do Rasgo Synth, sem cópia.

| patch | processo |
|---|---|
| **dobra** | wavefolder movido pelo envelope do próprio som (Buchla 259): o ataque se rasga, a cauda volta ao corpo |
| **anel inarmônico** | modulação em anel com uma razão tirada do corpo espectral da referência (as razões não inteiras), ou de uma lista (√2, 2,76, φ…) |
| **deslocamento de frequência** | o deslocador de Bode (par de Hilbert de Niemitalo): o espectro inteiro anda ±25–275 Hz, e os harmônicos deixam de ser harmônicos |
| **portão caótico** | o lowpass gate re-disparado por um registrador caótico de 8 bits (o Rungler de Rob Hordijk): o sustentado vira pulsado e irregular |
| **gagueira granular** | a nota em grãos de 25–90 ms, embaralhados ou repetidos |
| **ressonância simpática** | um pente com realimentação afinado numa razão fora da série: uma corda que não existe ressoa |

**As quimeras** (`AcousticVoice.cpp`) põem o excitador de um instrumento no
corpo de outro:

- **prato friccionado:** o arco irregular num prato de modos densos;
- **corda cantada:** a glote numa corda;
- **pele soprada:** o ar numa pele de Bessel, sustentada.

**A quantidade:**

- no app, **Linguagem ▸ Rasgo modular** (0 = o instrumento, 1 = o
  inaudito), com padrão 0,5;
- na CLI, `--rasgo` (padrão 0,5);
- no núcleo, `Options::rasgo` (padrão 0), para os testes antigos
  continuarem iguais.

**Testado** (`tests/ModularRasgoTests.cpp`):

- cada patch transforma o som (a diferença medida) e devolve o nível do
  corpo;
- as quimeras soam;
- com 0, nada muda; com 1, quase tudo é atravessado.

**Respostas** (rec187, semente 6) na pasta de escuta, `rasgo-*.wav`:

- jazz com 0,5 e com 0,9;
- orquestra com 0,6;
- o conjunto lido com 0,7.

## 6. A produção: a mistura e a sala (2026-10-16)

**O que o autor ouviu** (16/10): "está muito amador, precisamos de
sonoridade mais sofisticada, amadurecida, profissional […] para que gere
emoção". Sobre a síntese realista: "creio que fx né, confirma?".

**A resposta:** sim. Grande parte do som profissional vem da cadeia de
efeitos e da mistura, e a do Trioio era primitiva:

- todas as vozes somavam num par estéreo;
- uma reverberação de 4 linhas com entrada mono, a mesma quantidade para
  tudo;
- nenhuma equalização, profundidade ou palco;
- as salas HALL/SPACE do Rasgo Modular aplicadas sobre a mistura inteira.

**O que mudou** (`src/core/Production.h/.cpp`; fontes: R. Izhaki, *Mixing
Audio*, 2018; B. Owsinski, *The Mixing Engineer's Handbook*, 2017):

1. **Barramentos por papel,** processados um de cada vez (a memória fica
   em ~6 pistas):

   | barramento | passa-altas | lama 300 Hz | presença 3 kHz | ar 9 kHz | compressão | largura | envio |
   |---|---|---|---|---|---|---|---|
   | melodia (vozes 0, 2) | 90 Hz | — | +2 dB | +1 dB | 2,5:1 | 1,0 | 0,16 |
   | baixo (1) | 30 Hz | — | +1 dB | −2 dB | 3:1 | 0 (mono) | 0,04 |
   | harmonia e camadas (3, 5, 6) | 150 Hz | −2,5 dB | — | −1,5 dB | 1,8:1 | 1,5 | 0,32 |
   | percussão (4) | 30 Hz | −1,5 dB | +1 dB | +1 dB | 1,8:1 | 1,2 | 0,12 |
   | efeitos (gestos) | 40 Hz | — | — | — | — | 1,3 | 0,35 |

   A compressão tem o limiar 4 dB acima do nível médio do barramento:
   segura os picos, não esmaga.
2. **A reverberação de envio:**
   - **o envio filtrado:** passa-altas de 180 Hz e passa-baixas de 9 kHz;
   - **o pré-atraso** e **8 reflexões iniciais estéreo** diferentes em
     cada canal;
   - **a difusão:** 4 passa-tudos por canal (Dattorro 1997);
   - **a rede:** 8 linhas com mistura de Hadamard (Jot e Chaigne 1991),
     duas delas moduladas, e a perda de agudos a cada volta;
   - **as saídas:** duas linhas ortogonais da matriz, para a cauda não se
     correlacionar entre os canais.
3. **A sala pelo estilo:**

   | sala | estilos | RT60 |
   |---|---|---|
   | clube | jazz, manouche, bolero, tango | 0,9 s |
   | sala de choro | choro, samba, salsa, forró | 1,1 s |
   | salão | charanga | 1,2 s |
   | estúdio com placa | rock | 1,3 s |
   | rua | escola de samba | 1,4 s |
   | sala do Trioio | automático | 1,6 s |
   | sala de câmara | câmara | 1,9 s |
   | sala de concerto | orquestra; HALL no automático | 2,4 s |
   | igreja | *a cappella* | 3,2 s |
   | espaço | SPACE no automático | 4,2 s |
4. **A cola da soma:** 25 % de saturação de fita, antes do master (que
   continua: −16 LUFS, −1 dBTP).

**Medido** (`tests/ProductionTests.cpp`):

- **o RT60** pela integração de Schroeder fica a menos de 10 % do pedido em
  todas as salas. Na primeira versão saía em ~82 %, porque o filtro de
  agudos tirava energia; foi corrigido no ganho das linhas;
- **a correlação da cauda** entre os canais fica perto de 0 (era ~0,7 na
  sala antiga);
- **um seno de 60 Hz** passa no barramento do baixo e cai ~17 dB no da
  harmonia.

**A escuta:**
`/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/escutas/2026-10-16-producao/`,
com jazz, orquestra (com efeitos), choro e *a cappella* sem rasgo, e o jazz
e o conjunto lido com rasgo.

**A humanização fina** (`humanize`, `Production.cpp`, na mesma data). Cada
nota recebe as pequenas diferenças de quem toca:

- **o tempo:** até ~9 ms, até ~3 ms na percussão;
- **a força:** ~5 %, e a alternância nas notas repetidas (o arco ou a
  palheta que alterna);
- **a afinação:** ~1,5 cent, só nos instrumentos sem afinação fixa (sopros,
  cordas friccionadas, voz, contrabaixo).

As variações são um passeio lento por voz, não ruído branco: o músico
adianta e atrasa por trechos. As flutuações humanas têm correlação de longo
alcance (H. Hennig et al., PLoS ONE 6(10), 2011).

- **Onde liga:** no app e na CLI, ligada por padrão (`--humano nao`
  desliga); no núcleo, desligada (`Options::humanize`), para os testes
  antigos.
- **Testado:** os limites de tempo; o piano não desafina; o arco varia.
- **Para comparar:** na pasta de escuta, cada arquivo `-humano` está ao
  lado do mesmo sem humanização.

**Ainda falta:** o palco por instrumento em cada conjunto (a posição de
cada naipe).

## 6.1 O palco e o idioma (2026-10-16)

**O palco** (`stageFor`, `Production.cpp`): cada voz ganhou o seu
barramento (processado um de cada vez) e o seu lugar no palco do conjunto,
com um lado, uma largura e uma profundidade. A profundidade traz mais
sala, menos brilho (prateleira de 9 kHz até −3 dB) e um nível um pouco
mais baixo.

| palco | conjuntos | disposição |
|---|---|---|
| orquestra | orquestra | a disposição clássica: os primeiros violinos à esquerda, os baixos à direita, as madeiras no meio e atrás, os metais mais atrás, a percussão no fundo |
| câmara | câmara | o quarteto em semicírculo |
| jazz | jazz, manouche | o solista no centro e na frente, o trompete ao lado, o piano à direita, o baixo à esquerda, a bateria aberta no fundo |
| rock | rock | a voz e o baixo no centro, as bases abertas nos dois lados |
| coro | *a cappella* | as vozes em arco |
| roda | os outros estilos | o solista no centro, a harmonia aberta, a percussão um pouco atrás |
| automático | sem conjunto | o estéreo como as notas vieram |

**O idioma** veio da escuta do autor de 16/10 sobre o jazz: "achei
estranho os sons do jazz, pode conferir, não soa como jazz".

O que estava errado no jazz:

1. **O Terceiro Olho atravessava o estilo** com clusters, drones, nuvem
   granular, faixa de ruído, corpo da referência, troca de timbre, tempo
   deformado e uma escala mutante de +0,5 semitom.
2. **A harmonia eram tríades** lidas na referência.
3. **O eco em cânone criava notas depois do conjunto,** e elas saíam com
   um timbre sintético (teremim).

Correções:

1. **O olho num conjunto de estilo** (com o rasgo abaixo de 0,5) fica só
   com eco, silêncio, densificar, rarear e congelar, e com metade da
   imprevisibilidade. Com o rasgo em 0,5 ou mais, o olho volta inteiro: aí
   o inaudito é o pedido.
2. **A cor dos acordes** (`styleVoicing`):
   - **jazz:** as disposições sem fundamental do piano (M. Levine, *The
     Jazz Piano Book*, 1989). O maior vira maj7(9), ou dominante 7(9,13)
     antes de um acorde uma quarta acima; o menor vira m7(9); o diminuto,
     m7(♭5). A nota de baixo fica entre Ré 3 e Lá 3;
   - **manouche:** o 6 e o m6 de Django.

   Vale também nos trechos sem pulso, com os acordes sustentados.
3. **As notas criadas pelo olho** voltam ao instrumento do seu papel no
   conjunto. As quimeras do rasgo ficam.

**Medido:** no jazz do rec187, todas as vozes são agora instrumentos de
jazz: sax, trompete, vibrafone, baixo acústico, piano, guitarra e bateria.

**A escuta:** `escutas/2026-10-16-producao/jazz-v2-idioma.wav`.

**O que pode ainda soar fora:**

- **a peça segue a referência** (rec187: 74 BPM, uma balada, com a escala
  e o contorno dela, que não são de jazz);
- **o próprio timbre do sax e do trompete,** que são modelos.

## 6.2 Os pratos refeitos (2026-10-16)

**O que o autor ouviu:** "o som do prato (ou chimbal) não está parecido com
o habitual do jazz".

**O que estava errado:** o prato de condução tinha 28 parciais a partir de
410 Hz, poucos e graves demais, e soava como sino ou gongo.

**O modelo novo** (`cymbalCore`):

- **os modos:** 70 a 140 por golpe, densos e inarmônicos (log-uniformes
  com desvio), com a energia concentrada numa faixa e o decaimento menor
  nos agudos;
- **o estalo da baqueta;**
- **a lavagem:** o chiado que cresce logo depois do golpe, quando a energia
  passa dos modos graves aos agudos, e decai devagar (A. Chaigne, C. Touzé
  e O. Thomas, *Acoust. Sci. & Tech.* 26(5), 2005; Rossing 2000, cap. 8).

Cada modo é um ressoador recursivo, e cada golpe é sintetizado uma vez por
variação (4) e por faixa de força (4), depois reaproveitado (cache com
trava).

| prato | o que é | centróide | queda de 20 dB |
|---|---|---|---|
| **condução** (jazz) | escuro e lavado, o ping em 3–6 kHz | 4,65 kHz (antes 2,8) | ~1 s |
| **chimbal fechado** (o "tchik" do pé) | curtíssimo, com o estalo médio das duas placas (~1,3 kHz) | 4,9 kHz | 0,16 s |
| **chimbal aberto** | o mesmo, longo | — | — |
| **ataque** | grande e brilhante, a lavagem forte | 6,4 kHz | — |

**Na primeira medida** a energia do prato de condução ficava em 1–3 kHz:
o centro subiu para 5,2 kHz, e o decaimento dos agudos ficou mais perto do
dos graves.

**A audição:** `pratos-2026-10-16b.wav` (condução, chimbal e ataque, quatro
golpes cada) e o jazz com os pratos novos, `jazz-v3-pratos.wav`, na pasta
de escuta de 16/10.

## 6.3 A expressão (2026-10-16)

`applyExpression` (`Production.cpp`) responde ao pedido do autor: "para que
gere emoção pra quem escuta".

- **O arco da frase,** nas melodias (vozes 0 e 2): a força cresce até ~60 %
  da frase e se recolhe no fim, entre 0,82 e 1,12× (N. P. Todd, JASA 91(6),
  1992).
- **A carga melódica:** as notas acima da média da frase ficam um pouco mais
  fortes (+1,2 % por semitom). São as regras "phrase arch" e "high loud" do
  KTH (A. Friberg et al., *Advances in Cognitive Psychology* 2(2–3), 2006).
- **O final que respira:**
  - o *ritardando* nos últimos ~6 s, com o tempo local alargando até +25 %
    (Friberg e Sundberg, JASA 105(3), 1999);
  - a fermata: o último acorde dura 1,6× mais;
  - a peça ganha esse tempo.

**O comprimento:** a resposta passa a poder ser um pouco mais longa que a
referência (no rec187, 226,6 s contra 224,3 s), e o arquivo não corta mais
o final. No modo de linhas, que soma as linhas em posição fixa, fica só o
arco das frases (`Options::expressiveEnding`).

**Onde liga:** junto com a humanização, ligada no app e na CLI.

**Testado** (`tests/ProductionTests.cpp`):

- o meio da frase mais forte que as pontas;
- os intervalos do fim maiores que os do começo;
- a fermata;
- sem o final, nada alarga.

**A escuta:** `AUDIOS_RASGO/Trioio/escutas/2026-10-16-expressao/` (jazz e
orquestra).

## 6.4 O piano e o sax do jazz (2026-10-16)

**O piano:**

- **O tampo harmônico:** as cordas não soam direto. Passam pelos 8 modos do
  tampo de madeira (110 Hz a 1,85 kHz), que dão o corpo e o calor; o som
  direto guarda o ataque. Fontes: N. Giordano, JASA 102(2), 1997; H.
  Suzuki, JASA 80(6), 1986.
- **O abafador:** quando a tecla sobe, o feltro toca a corda (um sopro curto
  de ruído escuro).

**O sax:**

- **O ar:** o sopro que passa pela palheta (uma banda em torno de 2,8 kHz),
  escalado pelo nível do próprio som, mais presente no piano e quase
  ausente no agudo.
- **O subtone:** no piano, o tenor fica mais escuro (o corte vai de 3 a
  5,5 × f₀ conforme a força).
- **O primeiro ensaio deu errado:** o ar tinha nível fixo e encobria a
  nota (o medidor lia só o sopro). Agora é escalado pelo nível do som e
  pela altura.

**Medido:** os dois afinados em toda a extensão; ctest 40/40.

**A escuta:**

- `escutas/2026-10-16/piano-sax-2026-10-16.wav`, a audição;
- `escutas/2026-10-16-expressao/jazz-piano-sax.wav`, o jazz.

## 6.5 A cor dos acordes em todos os estilos (2026-10-16)

O que corrigiu o jazz vale para os outros estilos: a tríade lida vira o
acorde do idioma (`styleVoicing`), na faixa do instrumento que toca a
harmonia. A dominante é reconhecida quando o próximo acorde está uma quarta
acima (V → I).

| estilo | maior | dominante | menor | diminuto | faixa da nota de baixo |
|---|---|---|---|---|---|
| jazz | maj7(9) sem fundamental | 7(9,13) sem fundamental | m7(9) | m7(♭5) | Ré 3 – Dó♯ 4 |
| manouche | 6 | — | m6 | dim | (idem) |
| choro, samba, escola | 6 (a tônica do samba) | 7(9) | m7 | dim (de passagem) | Si 3 – Lá♯ 4 (o cavaquinho) |
| bolero | maj7(9) | 7(9) | m7(9) | m7(♭5) | Ré 3 – Dó♯ 4 |
| tango | tríade com a oitava | 7 | m6 | dim | Mi 3 – Ré♯ 4 |
| salsa, charanga, forró | tríade com a oitava | 7 | tríade com a oitava | dim | Sol 3 – Fá♯ 4 |

**Corrigido no caminho:** as faixas tinham menos de uma oitava, e para
algumas fundamentais a disposição caía fora (o Dó do tango ia ao Dó 3).
Agora toda faixa tem uma oitava inteira.

**Testado:** as doze fundamentais cabem na faixa em todos os estilos com
cor.

Sem pulso, a harmonia fica sustentada, mas com a cor do estilo.

**A escuta:** `escutas/2026-10-16-acordes/` (choro, samba, bolero e tango).

## 6.6 Medidas que descartaram caminhos (2026-10-17)

**O *aliasing*:** foi medido antes de implementar a sobreamostragem (a
energia fora dos harmônicos, entre 0,2 e 0,7 s da nota).

- **guitarra distorcida:** −61 a −63 dB nas notas médias, −47 dB no Lá 5;
- **a dobra do rasgo:** não muda a medida (piano −23,4 contra −23,5 dB);
  o que aparece fora dos harmônicos é a inarmonicidade natural das cordas.

Não vale sobreamostrar agora.

**A escala nos conjuntos de estilo** (`compose`): com a fonte de alturas
"arquivo", que é o padrão do app, um conjunto de estilo tonal (todos, menos
câmara, orquestra e *a cappella*) passa a usar o campo tonal da referência.
O arquivo procura a escala mais rara que veste a referência (no rec187, um
modo de Messiaen), e isso tira a música do idioma.

A CLI já usava o campo da referência por padrão, então as respostas de
escuta geradas pela CLI já eram tonais. A mudança vale para quem compõe
pelo app.

## 5. As levas seguintes (a fazer)

1. **Cordas dedilhadas e friccionadas.**
   - **Por receita, a partir do violão** (a mesma corda dedilhada, com
     outro corpo e outro toque): cavaquinho, bandolim (pares de cordas de
     aço), violão de 7 cordas, guitarra manouche (aço, corpo Selmer,
     brilhante), requinto, harpa.
   - **Baixo acústico em *pizzicato*:** corda grave, corpo grande, o "tum"
     do dedo.
   - **O arco** em contrabaixo e em *pizzicato*.
2. **Sopros.**
   - **Flauta transversal:** o jato de ar contra a aresta, um tubo aberto
     nas duas pontas; M.-P. Verge et al., JASA 1997; Cook 1992.
   - **Tubo cônico:** sax e oboé.
   - **Lábios:** trompete, trombone, trompa; o modelo de lábio de
     Adachi e Sato, JASA 1996; Cook 1992.
   - **Surdina.**
3. **Percussões.** A partir de peles com modos de membrana circular (as
   razões de Bessel), placas e partículas:
   - pandeiro (pele e platinelas), surdo, tamborim, alfaia, zabumba;
   - congas, bongô, timbales, tabla;
   - caixa (com a esteira), bumbo, pratos e chimbal (placa e ruído
     modal);
   - chocalhos, ganzá, caxixi, maracas (o PhISEM: P. R. Cook, "Physically
     informed sonic modeling (PhISM)", CMJ 21(3), 1997);
   - agogô e cencerro (sino curto), clave e reco-reco.
4. **Vozes.**
   - **O trato vocal:** a glote (o modelo LF: Fant, Liljencrants e Lin,
     1985) e o trato como tubo de seções (Kelly-Lochbaum, 1962), ou os
     formantes.
   - **Os timbres:** voz falada, cantada, coro, sussurro, vocalise, scat e
     os registros masculino, feminino e infantil.
   - **O ponto de partida:** o scat e o coro (cópia do Rasgo Synth); a
     máquina os amplia sem os substituir.
5. **Teclados e elétricos.**
   - órgão de rodas fônicas (aditivo por tirantes, a *Leslie*);
   - piano elétrico (diapasão golpeado);
   - guitarra e baixo elétricos (corda de aço, captador, amplificador
     valvulado com saturação suave e a caixa);
   - sanfona e bandoneón (palheta livre).
6. **Efeitos de filme** (síntese procedural; A. Farnell, *Designing Sound*,
   MIT Press, 2010):
   - vento, chuva, trovão, fogo, água, passos, portas, motores;
   - *whoosh*, *riser*, *impact*, *drone*, *braam*;
   - zumbidos e ruídos (**noises**) de muitas cores.

   Entram como **gestos** (o Trioio já tem os objetos sonoros contínuos,
   `GestureKind`), não como notas.

## 6. O portão conceitual (Método RASGO)

- **Por que no Trioio?**
  - O Trioio escuta gravações, muitas delas de instrumentos acústicos, e
    responde. Ter corpos acústicos aproxima a conversa: um choro escutado
    pode receber uma resposta de choro, ou de outro estilo, à escolha do
    jogo e do modo.
  - A máquina serve à estratégia situada do Trioio. Ela não é um banco de
    sons para tocar ao teclado.
- **O que permanece diferente:**
  - o Rasgo Synth Studio compõe por seed com os seus corpos;
  - o Rasgo Modular dá módulos para quem toca;
  - o Trioio escolhe o conjunto pela escuta da referência, pela escuta do
    autor e pelo jogo, que pode trocar o conjunto como regra ("quando
    rarefeito, chamar o quarteto").
- **Reaproveitamento:** nenhum código de irmão foi copiado nesta leva. Os
  modelos foram escritos a partir das fontes acima. A corda da STRING
  (Rasgo Modular) e a corda Bioma (Aquorbium) continuam como cópias fixadas
  e servem de comparação.

## 7. Próximo passo

1. ~~A segunda leva~~ (feita em 2026-10-15, §3.1).
2. ~~As receitas da corda dedilhada~~ (feitas em 2026-10-15, §3.2). A
   separação em funções segue para o arco, a palheta e as peles.
3. ~~O conjunto como escolha da resposta~~ (feito em 2026-10-15, §4.1).
   Seguem as figuras de cada estilo (baixaria, *la pompe*, *tumbao*) e os
   instrumentos que faltam.
4. **Efeitos de filme e vozes**, cada um na sua leva.
