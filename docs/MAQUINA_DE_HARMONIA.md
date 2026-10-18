# A máquina de harmonia — estudo e desenho

Iniciado em 2026-10-17, a pedido do autor: "sugiro um estudo aprofundado de
harmonia, cadências, variações usuais e não usuais, e por que não pensarmos
numa máquina de harmonia — harmonia funcional, harmonia experimental, etc."

Este documento tem quatro partes:

1. o estudo: as linguagens harmônicas, das usuais às não usuais, com as
   fontes;
2. o que já existe no RASGO e no Trioio;
3. o desenho da máquina;
4. o portão conceitual (Método RASGO) e as fases.

Uma regra vale para tudo o que vem daqui: o código é próprio, escrito a
partir da teoria, com as fontes citadas. Nenhum arquivo de terceiros.

---

## 1. O estudo: as linguagens harmônicas

Uma linguagem harmônica tem cinco partes:

| parte | o que é |
|---|---|
| **vocabulário** | que acordes existem |
| **sintaxe** | como eles se seguem |
| **pontuação** | as cadências |
| **condução** | como as vozes se movem |
| **ritmo harmônico** | quanto tempo cada acorde dura |

Organizo as linguagens em oito famílias.

### 1.1 Funcional tonal (a prática comum, ~1650–1900)

- **As funções:** tônica (T), subdominante (S) e dominante (D). O movimento
  típico é T → S → D → T (H. Riemann, *Vereinfachte Harmonielehre*, 1893).
  A fundamental como gerador vem de J.-P. Rameau (*Traité de l'harmonie*,
  1722).
- **As cadências** (W. Piston, *Harmony*, 1941; 5ª ed., 1987;
  A. Schoenberg, *Harmonielehre*, 1911):

  | cadência | os acordes | o efeito |
  |---|---|---|
  | autêntica perfeita | V → I, fundamental no baixo, tônica na melodia | o ponto final |
  | autêntica imperfeita | V → I, a terça ou a quinta em cima, ou invertido | a vírgula forte |
  | suspensiva (meia cadência) | … → V | a pergunta |
  | frígia | iv⁶ → V, no menor | a pergunta arcaica, barroca |
  | plagal | IV → I | o "amém" |
  | de engano | V → vi (no menor, V → VI) | a surpresa: o ouvido espera a tônica |
  | evitada | V → V⁷/IV, ou a tônica invertida | o fim adiado |
  | picarda | uma peça menor que termina no I maior | a luz no fim |

- **O enriquecimento:**
  - as dominantes secundárias (V/x) e os acordes de sensível (vii°/x);
  - a sexta napolitana (♭II⁶);
  - as sextas aumentadas (italiana, francesa, alemã);
  - o empréstimo modal (iv, ♭VI, ♭VII do homônimo menor);
  - os pedais de tônica e de dominante.
- **As sequências:** o círculo de quintas; a de Pachelbel (I–V–vi–iii–IV–I–IV–V);
  a do baixo descendente.
- **A modulação:**
  - por acorde-pivô (comum aos dois tons);
  - por nota comum;
  - cromática;
  - enarmônica (pela sétima diminuta ou pela sexta alemã);
  - direta (frase a frase).
- **A condução de vozes:** o movimento mínimo, nenhuma quinta ou oitava
  paralela, a sensível que sobe, a sétima que desce. D. Huron ("Tone and
  voice", *Music Perception* 19(1), 2001) deriva essas regras da percepção
  de fluxos sonoros.
- **A sintaxe formal:**
  - a harmonia tonal tem uma gramática gerativa, com prolongamento e
    preparação recursivos (M. Rohrmeier, "Towards a generative syntax of
    tonal harmony", *Journal of Mathematics and Music* 5(1), 2011);
  - a tonalidade como inferência probabilística: D. Temperley, *Music and
    Probability*, 2007.

### 1.2 O jazz

- **As cadeias de ii–V:** o ii–V–I; os ii–V que se encadeiam por quintas ou
  por semitons; o *turnaround* (I–vi–ii–V); o *rhythm changes*.
  M. Steedman ("A generative grammar for jazz chord sequences", *Music
  Perception* 2(1), 1984) escreve as regras que geram os blues.
- **As substituições:**
  - de trítono (♭II⁷ no lugar do V⁷);
  - a *backdoor* (iv–♭VII⁷–I);
  - os acordes diminutos de passagem;
  - a dominante alterada (♭9, ♯9, ♯11, ♭13).
- **Coltrane:** três centros a uma terça maior ("Giant Steps", 1959).
- **O jazz modal:** a harmonia quase parada (Miles Davis, *Kind of Blue*,
  1959), o acorde quartal do "So What" e as estruturas superiores.
- **Fonte geral:** M. Levine, *The Jazz Theory Book*, 1995.

### 1.3 A harmonia brasileira

- **O choro:**
  - as dominantes secundárias encadeadas;
  - o diminuto de passagem (I–♯I°–ii);
  - o baixo de sétima cordas, que faz a condução;
  - as modulações à relativa e à subdominante.
  - Fonte: M. Sève, *Vocabulário do choro*, 1999.
- **O samba e a bossa nova:**
  - o IIm7–V7(♭9)–I6/9;
  - os cromatismos das vozes internas (Jobim, "Desafinado", "Corcovado");
  - o maj7(♯11);
  - o empréstimo modal.
  - Fonte: A. Chediak, *Harmonia e improvisação*, Lumiar, 1986.
- **O baião e a música nordestina:** a harmonia modal.
  - O mixolídio, e o "modo nordestino" (mixolídio com a quarta aumentada).
  - O dórico.
  - Pouca função, muito bordão.
- **A MPB:** os encadeamentos cromáticos de mediantes; o "acorde de
  empréstimo" como cor.

### 1.4 Modal e com bordão (não funcional)

- **Os *vamps*:** dórico (i–IV), mixolídio (I–♭VII), frígio (i–♭II).
- **A cadência andaluza:** i–♭VII–♭VI–V; a dupla tonicidade do flamenco
  e das músicas latinas (P. Manuel, "From Scarlatti to 'Guantanamera'",
  *JAMS* 55(2), 2002).
- **O bordão:** a raga e o maqam não têm harmonia funcional; o tanpura e o
  ison sustentam a tônica. A heterofonia como textura (o Trioio já a tem).

### 1.5 O cromatismo romântico e pós-romântico

- **As mediantes cromáticas:** I → ♭VI, I → III, sem nota diatônica em
  comum, só uma comum.
- **A teoria neo-riemanniana:** as transformações P, L e R; os ciclos
  hexatônicos e octatônicos; o *Tonnetz* (R. Cohn, *Audacious Euphony*,
  2012).
- **A geometria da condução de vozes:** os acordes como pontos num espaço,
  e a condução como o caminho mais curto (D. Tymoczko, *A Geometry of
  Music*, 2011).

### 1.6 O impressionismo

- **O *planing*:** o mesmo acorde movido em bloco (Debussy, "Voiles").
- **As cores sem função:** a escala de tons inteiros e a pentatônica; os
  acordes de 9ª, 11ª e 13ª como cor; o pandiatonismo.

### 1.7 O século XX e o experimental

- **O quartal e o quintal:** P. Hindemith, *Unterweisung im Tonsatz*, 1937,
  com o "valor" dos intervalos.
- **O politonal:** duas tríades ao mesmo tempo (Milhaud; o acorde de
  *Petrushka*).
- **O octatônico e os modos de transposição limitada:** O. Messiaen,
  *Technique de mon langage musical*, 1944. Inclui o "acorde da
  ressonância".
- **Os *clusters*:** H. Cowell, *New Musical Resources*, 1930.
- **Os conjuntos de classes de altura:** A. Forte, *The Structure of Atonal
  Music*, 1973. O Trioio já os tem no arquivo de escalas.
- **A harmonia estática:** M. Feldman, os acordes que se repetem e mudam
  por uma nota.
- **A harmonia espectral:** o acorde tirado do espectro de um som, às vezes
  distorcido (comprimido, esticado) e interpolado entre dois espectros
  (G. Grisey, T. Murail; J. Fineberg, "Guide to the basic concepts and
  techniques of spectral music", *Contemporary Music Review* 19(2), 2000).
- **A microtonal:** as tonalidades *otonal* e *utonal* de H. Partch
  (*Genesis of a Music*, 1949) e a entonação justa estendida.

### 1.8 A harmonia própria do Trioio (o que só ele pode fazer)

- **A harmonia lida no corpo da referência:** os acordes vindos dos
  parciais medidos da gravação, a harmonia espectral a partir do que o
  Trioio escuta. O corpo espectral já existe.
- **As cadências do jogo:** uma regra pode ser harmônica ("quando
  rarefeito, cadência de engano"), e as cadências podem ser apócrifas,
  inventadas.
- **A cadência por timbre, por densidade e por registro:** a chegada
  marcada pela fusão espectral, ou pelo rarefazer, e não pela função. A
  dramaturgia e o olho já dão o lugar.

---

## 2. O que já existe

| onde | o que há |
|---|---|
| Trioio, `Harmony.cpp` | a leitura dos acordes (tríades); o neo-Riemann por modo; a rearmonização do contrapor (trítono, mediantes); os graus da escala (máquina de escalas) |
| Trioio, `HarmonyWriting.cpp` | a tonalidade (Krumhansl–Kessler); as cadências por modo; as dominantes secundárias; a modulação preparada; as apojaturas; o retardo; a condução a quatro partes |
| Trioio, `StyleFigures.cpp` | a cor dos acordes por estilo (`styleVoicing`) |
| Trioio, `Voicing.h` | terças, quartal, So What, *planing*, bitonal, aberto, *cluster* |
| Rasgo Synth Studio (lido, não copiado) | progressões funcionais (14); HarmonicWanderer (Coltrane, trítono, giro cromático, intercâmbio modal, jazz modal, *backdoor*); a modulação real e a modal; o banco vertical (quartal, *planing*, politonal, armênio) |
| Rasgo Modular (lido) | o HARMONY (seis percursos de centro tonal) e o CHORD |

**Falta, pelo estudo:**

- o vocabulário além das tríades (a leitura e a escrita de 7ªs, 9ªs,
  alterações);
- as cadências não usuais (frígia, evitada, picarda, andaluza, *backdoor*,
  modal);
- as substituições como operação;
- as sequências;
- a escolha da linguagem por seção;
- as linguagens não funcionais como sintaxe, e não só como cor de acorde.

---

## 3. O desenho da máquina de harmonia

A máquina segue a forma da máquina de escalas (`ScaleMachine.h`) e da de
timbres: **linguagens** e **operações**, com um arquivo próprio em texto.

### 3.1 As linguagens

Cada linguagem é um objeto com cinco partes:

| parte | o que contém |
|---|---|
| **vocabulário** | a qualidade dos acordes (tríade, 7ª, 9ª, quartal, *cluster*, espectral…) e as alterações |
| **sintaxe** | a matriz de transições entre funções ou graus (a de Rohrmeier e a de Steedman simplificadas como cadeias de Markov ponderadas), ou uma regra não funcional (*planing*, ciclo hexatônico, bordão) |
| **pontuação** | o repertório de cadências da linguagem, com os pesos |
| **condução** | a funcional (`voiceLead`), o bloco paralelo, a mais parcimoniosa (Tymoczko), ou nenhuma |
| **ritmo harmônico** | quantos tempos por acorde, e se acompanha a frase |

As linguagens previstas, que correspondem às famílias de §1:

1. funcional
2. jazz
3. brasileira (choro, samba e bossa, nordestina)
4. modal e com bordão
5. romântica cromática (neo-Riemann, mediantes)
6. impressionista
7. século XX (quartal, politonal, octatônica, *cluster*, estática)
8. espectral (o corpo da referência)
9. microtonal (*otonal*/*utonal*)
10. a do jogo (as cadências inventadas)

### 3.2 As operações

Elas transformam uma progressão, como as operações da máquina de escalas
transformam uma escala:

| operação | o que faz |
|---|---|
| **substituir** | trítono, relativa, mediante, empréstimo |
| **interpolar** | a dominante secundária, o ii antes do V, o diminuto de passagem |
| **rearmonizar** | trocar os acordes mantendo a melodia |
| **inverter a função** | T ↔ D (o contrapor) |
| **retrogradar** | a progressão de trás para a frente |
| **comprimir ou expandir** | o ritmo harmônico |
| **modular** | por pivô, nota comum, enarmônica ou direta |
| **misturar linguagens** | uma por seção, como as misturas de escalas |

### 3.3 O arquivo de cadências

Um arquivo em texto (`data/harmonia/arquivo_trioio_cadencias.txt`), gerado
por uma ferramenta própria, como o de escalas e o de ritmos. Cada cadência
tem:

- o nome e a família;
- os graus;
- a condução esperada (a voz de cima, o baixo);
- a fonte.

Conteúdo inicial: as cadências de §1, cerca de 30.

### 3.4 Onde ela entra no Trioio

- **Na escolha:** em **Linguagem ▸ Harmonia**, "automática" ou uma das
  linguagens. No automático, a linguagem vem do conjunto (o jazz pede a
  linguagem de jazz) e do modo:
  - **seguir:** a linguagem lida;
  - **transfigurar:** uma linguagem vizinha;
  - **contrapor:** uma linguagem contrária (o funcional contra o *planing*,
    o espectral contra o tonal).
- **Na forma:** uma linguagem por seção, com transições escritas.
- **No jogo:** ações harmônicas novas ("trocar de linguagem", "cadência de
  engano", "suspender a resolução") e cadências apócrifas.
- **Na escuta:** ler as 7ªs e 9ªs (o croma NNLS de Mauch e Dixon) e a
  linguagem da referência (o funcional ou o modal pelo perfil das
  transições).

---

## 4. O portão conceitual (Método RASGO) e as fases

- **Por que no Trioio?**
  - O Trioio responde a uma gravação. A harmonia é uma das maneiras mais
    fortes de responder: seguir a harmonia lida, transfigurá-la, ou
    contrapô-la com outra linguagem.
  - A máquina serve à estratégia situada (o modo, o conjunto, o jogo). Não
    é um gerador de progressões solto.
- **O que permanece diferente:**
  - o Rasgo Synth Studio compõe a harmonia por seed, com percursos
    escolhidos por peça;
  - o Rasgo Modular dá módulos para quem toca;
  - o Trioio LÊ a harmonia na referência e responde a ela, e só ele tem a
    harmonia espectral do corpo escutado e as cadências do jogo.
- **Reaproveitamento:** os percursos do HarmonicWanderer e do HARMONY são
  ideias conhecidas (Coltrane, trítono, *backdoor*) e serão reescritos a
  partir da teoria, citando de onde vieram, sem copiar o código.

### As fases

1. **O núcleo funcional, jazz e brasileiro:**
   - o arquivo de cadências;
   - as linguagens 1, 2 e 3 como objetos (vocabulário, sintaxe,
     pontuação);
   - as operações de substituir e interpolar;
   - a escolha por conjunto e por modo;
   - os testes de cada cadência e de cada linguagem.
2. **O não funcional:** as linguagens 4, 5 e 6 (modal, bordão, andaluza;
   neo-Riemann em sintaxe; *planing*).
3. **O experimental:** as linguagens 7, 8 e 9 (quartal, politonal,
   octatônica, *cluster*, estática; a espectral do corpo; a microtonal).
4. **A interface e o jogo:** **Linguagem ▸ Harmonia**, as ações
   harmônicas do jogo, as cadências apócrifas.
5. **A escuta:** ler as 7ªs e 9ªs e a linguagem da referência.

---

## 5. Implementado: a fase 1 e a linguagem impressionista (2026-10-17)

O autor: "faça as duas juntas, avance" (o núcleo e a impressionista); "gosto
desses compositores".

### O vocabulário

O acorde (`Chord`, `Harmony.h`) ganhou:

- **a qualidade aumentada;**
- **as cores:** 7ª, 7ª maior, 9ª, ♭9, 6ª, ♯11, 13ª, sus4;
- **o baixo próprio** (`ChordSpan::bassPc`), para o pedal e as inversões.

O nome reflete tudo isso: "Dó7M(9)", "Sol7(♭9)", "Lá+7", "Dó6(9)". A
leitura da referência continua dando tríades (cor 0).

### As linguagens

`writeHarmony` e `resolveLanguage`, em `HarmonyWriting.h/.cpp`.

| linguagem | o que ela escreve |
|---|---|
| **funcional** | dominantes secundárias; cadências autêntica, imperfeita (o I com a terça no baixo), frígia (iv⁶–V, o ♭6 no baixo, no menor), plagal (transfigurar), de engano (contrapor), suspensiva (interromper); o fim IV–V–I, ou a napolitana (♭II⁶–V–i) no menor; a terça picarda; a modulação preparada |
| **jazz** | todo acorde com a cor da função: maj7(9), m7(9), 7(9/13/♭9), m7(♭5), o IV lídio 7M(♯11); o ii–V antes das dominantes secundárias; ii–V–I, ii–♭II7–I (o trítono), o *turnaround* I–vi–ii–V, a *backdoor* iv7–♭VII7–I no fim |
| **brasileira** | a tônica 6(9), o menor m6/m7, a dominante 7(9) ou 7(♭9) antes do menor; o diminuto de passagem I–♯I°–ii; a cadência IIm7–V7(♭9)–I6(9); a plagal com o iv menor (a bossa) |
| **impressionista** | por seção: *planing* real (a nona de dominante deslizando), *planing* diatônico (as qualidades da escala com a 9ª), tons inteiros (acordes aumentados) ou a sonoridade pentatônica (6/9); o pedal de tônica em ~35 % das seções; nunca V → I, e sim a cadência modal ♭VII(add9) → I6/9, a plagal IV7M → I, ou a evitada (para numa nona de dominante sem resolver) |

### A condução

- **Nas linguagens funcionais:** `voiceLead` passou a três ou quatro vozes
  de cima. Nos acordes com cor, sem a fundamental, que fica com o baixo: a
  3ª, a 7ª e as cores primeiro, a 5ª por último.
- **Na impressionista:** o acorde empilhado a partir da fundamental anda em
  bloco (*planing*).
- **O baixo** segue o baixo próprio quando há (o pedal, a inversão).

### A escolha

- **Na interface e na CLI:** **Linguagem ▸ Harmonia** (automática,
  funcional, jazz, brasileira, impressionista) ou `--linguagem-harmonica`.
- **No automático, pelo conjunto:**

  | conjunto | linguagem |
  |---|---|
  | jazz, manouche | jazz |
  | choro, samba, escola, forró | brasileira |
  | bolero, charanga, salsa, tango, rock | funcional |
  | câmara, orquestra, *a cappella* | impressionista ao transfigurar; senão funcional |
- **Sem conjunto, pelo modo:** transfigurar → impressionista; contrapor →
  jazz; os outros → funcional.

### Testado

`tests/HarmonyWritingTests.cpp` §9:

- **funcional no menor:** a frígia, a napolitana e a picarda aparecem;
- **brasileira:** o diminuto de passagem e a tônica 6(9);
- **impressionista:** os tons inteiros, o *planing* com a mesma qualidade,
  o pedal, e nenhuma cadência V → I;
- **jazz:** todo acorde com cor, e o ii–V–I ou a *backdoor* no fim;
- **condução:** o ii7–V7–I7M a quatro vozes sem paralelas;
- **suíte:** 41/41.

### A escuta

`AUDIOS_RASGO/Trioio/escutas/2026-10-17-maquina-de-harmonia/`, no rec187:

- a impressionista na câmara (transfigurar);
- a funcional na orquestra;
- a brasileira no choro;
- o jazz.

### Diferenças em relação ao desenho

- **O repertório de cadências** está no código, não ainda num arquivo de
  texto. O arquivo (§3.3) fica para a fase seguinte.
- **As operações explícitas** (retrogradar, comprimir o ritmo harmônico,
  misturar linguagens por seção) ainda não estão expostas.

### Próximo

- **a fase 2:** modal, bordão, andaluza, cromática (Cohn), *planing* com
  bordões;
- **a fase 3:** quartal (Hindemith), politonal (Milhaud, *Petrushka*),
  octatônica (Stravinsky, Messiaen), *clusters* (Cowell), estática
  (Feldman), espectral, microtonal;
- **depois:** Bartók (eixos, escala acústica) e Scriabin (o acorde
  místico).

---

## 6. O Brasil: o Nordeste, Minas, Guerra-Peixe e Hermeto (2026-10-17)

O autor pediu "a parte de baião, repente, música regional nordestina e
também as harmonias de Hermeto Pascoal e músicos compositores expoentes
brasileiros", e citou Guerra-Peixe.

### O estudo

- **O sistema trimodal nordestino.** José Siqueira, em *O Sistema Modal na
  Música Folclórica do Brasil* (João Pessoa, 1981) e *Sistema Pentatônico
  Brasileiro* (1981), reconhece três modos na música folclórica do
  Nordeste: o lídio, o mixolídio e a mistura dos dois (o lídio-mixolídio,
  com a 4ª aumentada e a 7ª menor). É a "escala nordestina", a mesma escala
  acústica de Bartók.
  - **No baião de Luiz Gonzaga:** o mixolídio e o dórico, e os acordes do IV
    e do V do mixolídio ("Como se toca o baião", *Per Musi*, SciELO).
  - **Sobre os procedimentos modais na música brasileira:** P. J. de S.
    Tiné, tese, USP, 2008.
- **A cantoria de viola (o repente).** Dois cantadores improvisam versos à
  viola de dez cordas, em formas poéticas fixas. A música serve à palavra:
  o ritmo e a métrica valem mais que a melodia e a harmonia, e a viola
  mantém o ritmo e o tom (D. Tavares, "Função da música na cantoria de
  viola", GERFLINT). O aboio é canto modal, sem harmonia.
- **Guerra-Peixe.** Em 1949 foi a Recife, pesquisou o maracatu e o frevo e
  escreveu *Os Maracatus do Recife* (1955; 2ª ed., 1980). Nessa fase,
  mantém os ritmos e as frases tradicionais numa linguagem modal
  dissonante, áspera, que não acomoda as melodias à harmonia tonal europeia
  (Enciclopédia Itaú Cultural; *Revista do IEB*, SciELO).
- **O Movimento Armorial.** Ariano Suassuna, 1970; o Quinteto Armorial. Uma
  música de concerto a partir da cultura popular nordestina, modal, com
  sonoridades renascentistas e a rabeca (M. P. dos Santos, "Música
  armorial: revisão bibliográfica", *Revista Música*, USP).
- **Hermeto Pascoal.**
  - **O *Calendário do Som*:** 366 peças, uma por dia, de 23 jul. 1996 a 23
    jul. 1997; a melodia primeiro, a cifra depois.
  - **As análises:** pela tonalidade flutuante e suspensa e pelas regiões de
    Schoenberg (F. A. Costa e F. Borém, UFMG; ANPPOM 2005); pela teoria do
    jazz e pela neo-riemanniana; pelo music21 (ANPPOM 2024); e a tese de M.
    A. G. Arrais, *A música de Hermeto Pascoal* (USP).
  - **O "som da aura":** a fala como melodia harmonizada.
- **O Clube da Esquina.**
  - **Os procedimentos:** a hibridação tonal-modal, o modalismo misto, os
    pedais, o Vm7, os acordes sus4, o ♭II7M(♯11) como opção de dominante,
    as progressões cromáticas;
  - **"Novena":** Ré dórico com Dm7, A7sus4 e Am7;
  - **fontes:** L. Ribeiro, *A tribo toda reunida*, dissertação, Unespar; T.
    Nunes, "A sonoridade específica do Clube da Esquina", Unicamp.

### O que entrou na máquina

| linguagem | o que ela escreve |
|---|---|
| **nordestina** | por seção, uma de cinco técnicas: **baião** (mixolídio: I7 como tônica, IV7, ♭VII, Vm7, iim7); **lídio** (I7M(♯11), o II maior); **trimodal de Siqueira** (lídio-mixolídio: I7(♯11)); **repente e aboio** (o bordão: acordes sus4 sobre o pedal de tônica); **Guerra-Peixe e Armorial** (o modal com as notas acrescentadas: sus4(9), ♭VII(add9), IV7(♯11), iim7(9), às vezes com pedal). As cadências: ♭VII → I7, IV7 → I7, II → I(♯11); nunca V → I |
| **mineira** (Clube da Esquina) | dórica (im7(9), IV7, ♭VII7M, Vm7, ♭III7M) ou mixolídia (I7sus4, ♭VII7M, Vm7, IVsus(9)); pedais em ~40 % das seções. As cadências: ♭II7M(♯11) → I, Vm7 → I, IVsus(9) → I |
| **Hermeto** | a fundamental lida, com as cores ricas (7M(9)(♯11), 7(9)(13), 7(♭9)(♯11), sus4(9), m(7M)(9)); em ~60 % dos acordes longos, a chegada cromática: o acorde meio tom acima do próximo (7M(♯11) ou 7(♯11)), com o baixo andando por semitom. As cadências: ♭II7M(♯11) → I7M(9), ♭VI7M(♯11) → I |

- **No automático,** o forró passa a usar a linguagem nordestina.
- **Na interface:** **Linguagem ▸ Harmonia** (nordestina, mineira, Hermeto
  Pascoal) e `--linguagem-harmonica nordestina|mineira|hermeto`.

**A melodia acompanha os modos:** a variante da escala sob cada acorde (o
*chord-scale*, que já existia) muda a 7ª da melodia sob o I7 e a 4ª sob o
♯11.

**Testado** (`HarmonyWritingTests` §10):

- **nordestina:** as cinco técnicas aparecem; nenhuma cadência V → I; o I7
  no fim;
- **mineira:** o ♭II7M(♯11) e o Vm7;
- **Hermeto:** todo acorde com cor, as chegadas cromáticas, o baixo próprio.

**A escuta:** em `escutas/2026-10-17-maquina-de-harmonia/`:

- a nordestina no forró e na orquestra (transfigurar);
- a mineira sem conjunto;
- Hermeto com o conjunto de jazz.

**Ainda falta:**

- **Hermeto:** o "som da aura" (harmonizar a fala da referência, que o
  Trioio já escuta), as politonalidades e os *clusters*;
- **Villa-Lobos:** os acordes de cordas soltas, o paralelismo, as
  *Bachianas*;
- **Gismonti:** as afinações abertas, os pedais;
- **Moacir Santos** (*Coisas*: o modal afro-brasileiro);
- **Guinga:** as cores e as vozes internas;
- **Jobim:** a bossa, que já está em parte na brasileira;
- **frevo e maracatu:** como ritmo e harmonia própria (Guerra-Peixe).

---

## 7. O som da aura (Hermeto Pascoal) (2026-10-17)

`src/core/SomDaAura.h/.cpp`.

Hermeto chama de "som da aura" a fala ouvida como melodia: ele transcreve a
entonação de quem fala, sílaba por sílaba, dobra essa melodia com os
instrumentos e harmoniza cada nota. Ele descreve a ideia como "a vibração
sonora da alma de cada pessoa, refletida na sua fala". A primeira gravação
está no disco *Lagoa da Canoa, Município de Arapiraca* (1984). Fonte: L.
Costa-Lima Neto, "The first 'Sound of the Aura' of Hermeto Pascoal (1984):
hearing the spoken voice as a sung melody", *IASPM Journal*.

No Trioio:

1. **cada ataque com voz vira uma nota** (as sílabas). A altura é a mediana
   do trecho, arredondada ao semitom e trazida para o registro de canto; a
   força vem do nível. Os trechos sem voz (as consoantes, o ruído) ficam de
   fora;
2. **a melodia da fala substitui o motivo** (voz 0) e é dobrada em uníssono
   (voz 2, como Hermeto dobra a fala com os instrumentos);
3. **cada sílaba de 0,15 s ou mais ganha o seu acorde**, em que a nota da
   fala é uma cor: a 9ª, a ♯11, a 13ª, a 7M ou a 5ª (ou a 9ª do menor). A
   fundamental é a que deixa o baixo andar mais perto (o semitom, a quarta).
   O acorde vai cerrado abaixo da melodia, sem a fundamental, que fica com
   o baixo. As sílabas curtas passam sobre o acorde anterior.

A referência não entra no som: a fala é ressintetizada (a decisão de 14/10).

**Onde ligar:** a caixa "som da aura (Hermeto): a fala vira melodia
harmonizada", em **Linguagem ▸**, ou `--som-da-aura sim` na linha de
comando. Combina com `--linguagem-harmonica hermeto`.

**Testado** (`HarmonyWritingTests` §11): numa "fala" sintética de 16
sílabas, as 16 notas e a dobra; a nota da fala é sempre uma cor do acorde,
nunca a fundamental.

**A escuta:** `escutas/2026-10-17-maquina-de-harmonia/som-da-aura-vocalise.wav`
(o vocalise, com a linguagem Hermeto).

## 8. As fases 2 e 3 e os compositores (2026-10-17)

Catorze linguagens novas entraram na máquina (`HarmonyLanguage`, de
`modal` a `guinga`; `writeModern` em `HarmonyWriting.cpp`). Cada uma
trabalha seção por seção, como a nordestina: escolhe uma técnica, reescreve
os acordes lidos e, quando tem chegada própria, escreve a cadência e a marca
no arquivo de cadências. Fundamentação nas §1.4–1.7 e §6.

| Linguagem (`--linguagem-harmonica`) | O que faz | Fonte |
|---|---|---|
| `modal` | vamps dórico (im7–IV7), mixolídio (I–♭VII), frígio (i–♭II7M), eólio (i–♭VI–♭VII); bordão na tônica em metade das seções | §1.4 |
| `andaluza` | i–♭VII–♭VI–V(♭9) em ciclo; meia-cadência no V | Manuel 2002 |
| `cromatica` | ciclo hexatônico (P–L) ou octatônico (P–R) a partir da tônica; chegada pela mediante ♭VI → I | Cohn 2012 |
| `quartal` | todo acorde sus4 com 7ª (e 9ª), empilhado em quartas; chegada ♭VII → I | Hindemith 1937 |
| `politonal` | o acorde de *Petrushka* (7(♭9)(♯11)) ou as duas tríades a um tom (9, ♯11, 13), em bloco | Milhaud; Stravinsky |
| `octatonica` | as dominantes 7(♭9) andando por 3ªs menores (a coleção octatônica), em bloco | Messiaen 1944 |
| `clusters` | os acordes lidos, fechados, com as lacunas maiores que um tom preenchidas por segundas | Cowell 1930 |
| `estatica` | um acorde só por seção, mudando uma cor a cada dois acordes | Feldman |
| `espectral` | a cor de cada acorde vem dos parciais medidos no corpo da referência (`score.spectra`): o 7º parcial vira 7ª menor, o 11º vira ♯11 etc. | Grisey, Murail; Fineberg 2000 |
| `bartok` | o sistema de eixos (cada acorde vai a outro ponto do seu eixo de 3ªs menores); a cor da escala acústica 7(♯11); chegada pelo contrapolo ♭II | Lendvai 1971 |
| `scriabin` | o acorde místico 7(9)(♯11)(13) andando por trítonos e 3ªs menores, sem resolver | *Prometeu*, 1910 |
| `villa-lobos` | três técnicas: as cordas soltas (quartas) sobre o pedal; a sequência de quintas das *Bachianas*; o paralelismo dos *Choros* | §6 |
| `moacir` | Moacir Santos (*Coisas*, 1965): dórico im7(9)–IV7(9)–♭VII7M ou mixolídio I7sus(9)–♭VII7M, com baixo em pedal | §6 |
| `guinga` | vozes internas cromáticas: 7M(♯5), m(7M), o diminuto; chegada meio tom acima (♭II7(♯11) → I7M(9)) | §6 |

### A condução

`Harmony.cpp`, passo 5:

- **quartal e Villa-Lobos:** as notas do acorde ordenadas pelo ciclo de
  quartas a partir da fundamental (0, 5, 10, 3, 8…) e empilhadas;
- **clusters:** o acorde fechado com as lacunas preenchidas por segundas
  (até sete notas);
- **Scriabin, politonal, octatônica, espectral:** em bloco, como o
  *planing* impressionista, porque a cor é o que se ouve e a condução
  parcimoniosa a desmancharia;
- **as outras:** a condução a quatro partes (§5).

### Testado

`HarmonyWritingTests` §12 cobre três coisas:

- **o nome:** o nome de cada linguagem lido de volta (a leitura aceita o
  nome de exibição);
- **a marca de cada linguagem:**
  - o ♭VII e o ♭II do modal;
  - o V(♭9) andaluz;
  - a mediante e a troca de modo da cromática;
  - o sus4 em todo acorde quartal;
  - o místico em todo acorde de Scriabin;
  - o ♯11 politonal;
  - as raízes octatônicas;
  - a fundamental fixa da estática;
  - o 7(♯11) de Bartók;
  - o pedal de Villa-Lobos;
  - a 9ª de Moacir;
  - o 7M(♯5) e o m(7M) de Guinga;
- **o espectral:** um corpo com o 7º e o 11º parciais vira 7(♯11).

### A escuta

Há um exemplo por linguagem em `escutas/2026-10-17-linguagens-modernas/`,
todos sobre a mesma referência, com `--seed 6` e `--rasgo 0`. O nome de
cada arquivo é `linguagem-conjunto-modo.wav`.

### Ainda não

- **A microtonal (Partch):** entrou depois, ver §10.
- **Gismonti, o frevo e o maracatu:** ficam para a próxima leva.

### O arquivo de cadências (o desenho de §3.3)

`data/harmonia/arquivo_trioio_cadencias.txt` tem 45 cadências. Ele é
gerado por `tools/gerar_arquivo_de_cadencias.py`, que descreve o formato e
recusa um acorde mal escrito. Cada cadência traz:

- o nome, a família e a linguagem da máquina que a escreve;
- os graus e os acordes, em forma legível pelo core (intervalo, qualidade,
  cores, baixo);
- a voz de cima, o baixo e o efeito;
- a fonte.

O core o lê com `parseCadenceArchive` (`HarmonyWriting.h`). Para listar:

```
trioio_biblioteca cadencias [família|linguagem]
```

**Testado** (`HarmonyWritingTests` §13): todas as cadências são legíveis e
têm fonte; a autêntica perfeita termina V → I; o leitor recusa um intervalo
fora de 0–11.

**O sorteio** (`drawArchiveCadences`): depois de cada linguagem escrever as
suas chegadas, a máquina troca 40 % delas por uma cadência do arquivo com a
mesma `linguagem:`, transposta à tonalidade da marca, com o baixo que ela
pede. Assim o autor acrescenta uma cadência editando o gerador (ou o texto)
sem recompilar. A troca só acontece quando há tempo para a cadência inteira
sem invadir a anterior. Testado em §14: com chance 1, toda chegada andaluza
vira a do arquivo e a linha do tempo continua contínua.

## 9. Pernambuco e Gismonti: o frevo, o maracatu (2026-10-17)

Três linguagens a mais (`frevo`, `maracatu`, `gismonti`) completam o
pedido brasileiro de §6.

- **Frevo** (Recife; V. de Oliveira, *Frevo, capoeira e passo*, 1971):
  tonal e maior, de harmonia rápida.
  - Cada acorde lido de dois tempos ou mais é preparado pela sua dominante
    (o V7 de x) ou, em 30 % das vezes, por um diminuto de passagem meio tom
    abaixo.
  - A chegada é ii–V7(♭9)–I6.
- **Maracatu de baque virado** (C. Guerra-Peixe, *Os Maracatus do Recife*,
  1955):
  - as toadas modais: eólia (i–♭VII–♭VI–♭VII) ou mixolídia (I–♭VII);
  - um acorde para cada dois lidos;
  - o bordão da tônica em toda a seção, como a alfaia;
  - a chegada ♭VII → i, sem sensível.
- **Egberto Gismonti** (as gravações *Água e vinho*, 1972, e *Dança das
  cabeças*, 1977):
  - o pedal da tônica, como a corda solta do violão de dez cordas;
  - acordes que trocam de modo sobre ele: I(9), ♭VII(9), IV7M(9),
    ♭VI7M(♯11), IIm7(9);
  - a disposição em bloco;
  - a chegada ♭VI7M(♯11) → I(9).
  - É uma leitura de escuta das gravações, não de uma análise publicada: o
    autor deve julgar se reconhece Gismonti nela.

O bordão das linguagens com pedal passou a continuar também sob a chegada.

O arquivo de cadências ganhou cinco entradas, e agora são 50: duas do
frevo, duas do maracatu e uma de Gismonti.

**Testado** (`HarmonyWritingTests` §12):

- **frevo:** mais acordes que os lidos, o V7(♭9) e a tônica maior no fim;
- **maracatu:** o pedal em todo acorde e o ♭VII;
- **Gismonti:** o pedal e o ♭VI7M.

## 10. A microtonal: Partch (2026-10-17)

A linguagem `partch` (H. Partch, *Genesis of a Music*, 1949) escreve
harmonia fora dos 12 semitons, em entonação justa no limite 11:

- **A otonalidade:** a série harmônica sobre uma unidade (1/1, 5/4, 3/2,
  7/4, 9/8, 11/8), o "maior" estendido. O 7/4 fica 31 cents abaixo da 7ª
  menor temperada e o 11/8 fica entre a 4ª e a 4ª aumentada.
- **A utonalidade:** o espelho da otonalidade, sob a unidade (1/1, 8/5, 4/3,
  8/7, 16/9, 16/11).
- **As unidades:** vêm do diamante de tonalidades (1/1, 9/8, 8/7, 6/5, 5/4,
  4/3, 11/8, 3/2, 8/5, 5/3, 7/4, 16/9), contadas a partir da tônica lida.
- **A escolha:** cada acorde lido escolhe a unidade mais perto. O acorde
  maior vira O e o menor vira U.
- **O pivô de Partch:** quando a unidade se repete, em 60 % das vezes o
  acorde passa para o outro lado (O ↔ U).

**Como funciona:** `ChordSpan` ganhou `microPcs`, as classes de altura com
fração, e `microName` (O 7/4, U 3/2…). Daí em diante as frações seguem para
o resto da peça:

- o alvo leva as frações;
- o baixo toca a unidade;
- o motivo cai nas notas justas nos ataques fortes;
- a voz de harmonia toca o acorde em bloco, sem arredondar.

Num render de câmara, 251 das 576 notas da voz de harmonia e 139 da melodia
ficaram fora dos 12 semitons. As outras ficam quase inteiras porque o 3/2 e
o 9/8 estão a 2 a 4 cents do temperado.

**Fica de fora:** o arquivo de cadências guarda intervalos inteiros e não
tem cadências de Partch. A harmonia de Partch é de movimento entre
identidades, não de cadência.

**Testado** (`HarmonyWritingTests` §12):

- todo acorde tem seis notas justas;
- a otonalidade tem o 7º harmônico a 968,8 cents da unidade;
- a utonalidade tem o espelho, a 231,2 cents.

**A escuta:** `escutas/2026-10-17-linguagens-modernas/partch-camara-seguir.wav`
e `partch-orquestra-transfigurar.wav`.


## 11. A mistura conceitual de cadências (2026-10-18)

Da pesquisa sobre a recombinação de estilos (Atlas de Referência do RASGO,
entrada de 18/10). A base é A. Zacharakis, M. Kaliakatsos-Papakostas e E.
Cambouropoulos, "Conceptual Blending in Music Cadences: A Formal Model and
Subjective Evaluation" (ISMIR 2015; projeto COINVENT), sobre a mistura
conceitual de G. Fauconnier e M. Turner (*The Way We Think*, 2002).

**Como funciona** (`blendCadences`, `HarmonyWriting.h`):

1. **O final fica** (a tônica). Só misturam cadências que chegam à tônica.
2. **Os acordes penúltimos viram traços com saliência:**
   - **as notas relativas à tônica:** a sensível que sobe à tônica e o ♭2
     que desce valem 3; a fundamental e a nota que resolve por semitom na
     terça do final valem 2; as outras, 1;
   - **o tipo do acorde, os intervalos sobre a fundamental:** no acorde de
     dominante, a 3ª e a 7ª menor (o trítono) valem 3; os outros, 1.
3. **Os candidatos:** as 12 fundamentais × as tríades e tétrades usuais,
   sem ♭9, porque os acordes grandes acumulariam traços só por terem muitas
   notas.
4. **As condições:**
   - a mistura guarda ao menos uma nota própria de cada entrada (fora do
     espaço genérico): uma variante de uma só não é mistura;
   - não pode ser igual a nenhuma entrada;
   - não pode ter o próprio final como penúltimo.
5. **A nota:** a soma das saliências mantidas (o que as duas têm conta
   metade da segunda vez), menos 0,5 por nota que não vem de nenhuma.

**O resultado do artigo, reproduzido** (`HarmonyWritingTests` §15): da
perfeita (V7 → I) com a frígia modal do século XV (♭vii → I, acrescentada
ao arquivo, agora com 51 cadências) emerge a **substituição de trítono**
(`Dó#7 → Dó`, ♭II7 → I). É o acorde que o jazz inventou séculos depois, e
que o sistema acha sozinho. A **backdoor** (`Sib7`) aparece mais abaixo,
como mistura mais fraca, também como no artigo.

**Na máquina de harmonia:** das chegadas sorteadas no arquivo (40 % das
cadências), metade é **inventada**. Duas cadências da linguagem se
misturam, e uma das três melhores misturas vira a chegada. A forma diz
qual: "mistura de "backdoor" com "substituição de trítono": Sol7 → Dó7M".
É curioso: misturar os dois substitutos do V7 reconstruiu o próprio V7.

**Para experimentar:**

```
trioio_biblioteca misturar-cadencias "autêntica perfeita com cadencial 6/4" "frígia modal"
trioio_biblioteca misturar-cadencias "ii–V–I" "baião"
```

**A saliência pelo arquivo** (desde 2026-10-18, depois): as prioridades
seguem a regra acima. No artigo, um especialista as dava a cada cadência, e
o arquivo agora aceita o mesmo. A linha `saliência: 11:3, 5:2` (a nota
relativa à tônica e o peso, de 1 a 3) vence a regra para as notas que nomeia.
A perfeita com o 6/4 e a frígia modal trazem os pesos do artigo; as outras
cadências o autor afina quando quiser.
