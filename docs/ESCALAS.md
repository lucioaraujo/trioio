# TRIOIO — escalas e afinações

**Estado:** catálogo (45) e banco próprio (2576 escalas, fontes citadas; 32 com comportamento desde 2026-10-11)
desde 2026-10-08/09; máquina de escalas desde 2026-10-09; recurso
transversal do RASGO. Antes: banco de escalas implementado em 2026-10-08
(`src/core/ScaleBank.*`, `tests/ScaleBankTests.cpp`). A "máquina de
escalas" (variantes conforme a harmonia, encadeamentos e desvios; harmonia
nascida das linhas) está em desenho, no fim desta página.

## Antes

O Trioio tinha quatro fontes de altura:

- **referência:** as classes mais fortes do campo medido;
- **modal:** um de **6** modos diatônicos sorteado;
- **espectral:** os picos medidos;
- **microtonal:** graus desafinados ao acaso.

## Agora (`--alturas`)

| Fonte | O que faz |
| --- | --- |
| `referencia` | o campo medido (sem mudança) |
| `catalogo` (ou `modal`) | a escala do **catálogo de 45** que melhor veste o campo da seção, sorteada entre as 3 melhores |
| `espectral` | os picos medidos (sem mudança) |
| `microtonal` | graus desafinados ao acaso (sem mudança) |
| `inventada` | um **crivo de Xenakis** por peça, sempre fora do catálogo |
| `edo` | um crivo sobre **19, 22 ou 31** divisões da oitava: graus fora dos 12 semitons |
| `justa` | o campo reafinado em **afinação justa 5-limit** |
| `pitagorica` | o campo reafinado em **quintas puras** |
| `quartos` | o campo com metade das classes subida **um quarto de tom** |

A forma registra a escala, por exemplo "campo: menor harmônica sobre Sol:
Sol Lá Sib Dó Ré Mib Fá#" ou "crivo inventado (p%3==0 ∪ p%4==1): …".

**O catálogo** inclui:

- modos diatônicos e da menor melódica e harmônica;
- escalas simétricas (tons inteiros, diminutas, aumentada);
- pentatônicas e suas rotações;
- blues e bebop;
- húngaras e bizantina;
- japonesas (hirajoshi, in, kumoi);
- maior harmônica, menor napolitana, Prometeu e cromática.

**O ajuste ao campo** dá uma nota a cada uma das 45 escalas, em cada uma
das 12 tônicas:

- soma a fração do campo que a escala cobre;
- desconta 0,06 por nota que a referência não toca;
- acrescenta o peso da tônica.

Um campo de Sol menor harmônica escolhe a menor harmônica sobre Sol 30
vezes em 30.

## De onde vem

As tabelas e os algoritmos vêm do **Rasgo Synth Studio**: cópia fixada em
`third_party/rasgo_synth_dsp/sequencer/`, com checksums, créditos e
adaptações em `third_party/rasgo_synth_dsp/PROVENANCE.md`. Os créditos das
ideias são:

- **Xenakis:** o crivo;
- **Ptolomeu e Partch:** a afinação justa;
- **Hába e Wyschnegradsky:** os quartos de tom;
- **Blackwood:** 19-EDO;
- **Huygens e Fokker:** 31-EDO.

Adaptações próprias do Trioio:

- no crivo em EDO, os graus **são** a escala;
- o gerador próprio dá a mesma escala para a mesma seed em qualquer
  sistema.

## Validação

`tests/ScaleBankTests.cpp`:

- 45 escalas distintas;
- ajuste ao campo de Sol menor harmônica: 30/30;
- 300 inventadas: 108 coleções distintas, nenhuma do catálogo, todas com 4
  a 9 notas, reprodutíveis;
- crivo em EDO: 60/60 com graus fora dos 12 semitons, as 3 divisões
  usadas;
- terça justa a 386 cents, pitagórica a 408;
- no compositor, cada fonte marca a escala na forma, e as microtonais tocam
  alturas fora dos 12 semitons.

## O arquivo de escalas do Trioio

**2026-10-08.** Um arquivo **próprio**, escrito pelo Trioio a partir do
conhecimento teórico público. Escalas não têm dono: são patrimônio comum.
Cada uma cita a fonte, como numa pesquisa acadêmica, e diz quando é
**modelo** ou **aproximação** e não medição. Nenhum arquivo de terceiros é
copiado ou distribuído (decisão do autor).

- **Arquivo:** `data/escalas/arquivo_trioio.txt`, com 187 escalas.
- **Gerador:** `tools/gerar_arquivo_de_escalas.py`. A construção de cada
  família fica no código: os tetracordes somam 4/3, as quintas do ciclo
  são temperadas pela fração de coma e os melakartas seguem a regra de
  combinação.

| Família | Escalas | Fonte principal |
| --- | --- | --- |
| melakartas carnáticos | 72 | Venkatamakhin, *Caturdaṇḍīprakāśikā*, 1660 |
| divisões iguais da oitava (5 a 72) | 31 | Huygens, Fokker, Yasser, Hába, Wyschnegradsky, Erlich… |
| gêneros gregos (Arquitas, Dídimo, Ptolomeu) | 14 | Ptolomeu, *Harmônicos*, trad. Barker, 1989 |
| Aristóxeno (partes iguais) | 6 | Aristóxeno, *Elementos Harmônicos* |
| maqam árabe (modelo de 24 partes) | 11 | Congresso do Cairo, 1932; Touma, 1996 |
| thāts hindustani + 22 shrutis | 11 | Bhatkhande, 1909–1932; Daniélou, 1943 |
| temperamentos históricos | 10 | Aron, Salinas, Zarlino, Werckmeister, Kirnberger, Vallotti, Young |
| entonação justa e séries harmônicas | 8 | Partch, 1949; Helmholtz, 1863; Zarlino, 1558 |
| makam turco (53 comas de Holder) | 6 | Arel, 1943–1948; Signell, 1977 |
| não oitavantes (Bohlen-Pierce, Carlos) | 5 | Bohlen, 1978; Mathews e Pierce, 1989; Carlos, 1987 |
| gamelão, tailandesa, amadinda, timbila | 6 | Kunst, 1949; Morton, 1976; Kubik, 1960; Tracey, 1948 |
| chinesas, japonesas | 4 | *Lüshi Chunqiu*; Uehara, 1895 |
| modos nordestinos | 3 | J. Siqueira, 1981 |

**Formato** (um bloco por escala, legível e editável):

```text
= Werckmeister III
família: temperamento histórico (sobre Dó)
período: 1200.000
graus: 0.000 90.225 192.180 …       (cents, ou razões como 9/8)
fonte: A. Werckmeister, Musicalische Temperatur, 1691
nota: …                             (opcional)
```

**Uso:**

- `--escala pelog` (ou `maqam`, `melakarta`, `Werckmeister III`,
  `Bohlen-Pierce justa`…) escolhe, entre as escalas cujo nome, família ou
  fonte contém o termo, a que melhor veste o campo da referência inteira,
  e a tônica;
- `--alturas arquivo` sem termo: entre todas as de oitava com 5 a 24
  graus;
- `--escala minha.scl`: um arquivo no formato Scala, que é aberto;
- **escalas do autor:** blocos no mesmo formato em `<dados>/escalas/*.txt`,
  ou arquivos `.scl` ali, entram junto.

A escala vale para a peça inteira. A forma registra o nome, a família, a
tônica, a fonte e quantas candidatas havia. **Escalas não oitavantes**
funcionam: o compositor passou a ter um período (a Bohlen-Pierce repete na
duodécima, 3/1).

**Validação** (`tests/ScaleArchiveTests.cpp`):

- as 187 escalas são legíveis, citadas e com os graus em ordem;
- valores conferidos com a literatura: Werckmeister III (Mi 390,225, Si
  1092,180), Kirnberger III (Mi 386,314, Fá# 590,224), mesotônico de 1/4
  de coma (Ré 193,157);
- a terça de 6/5 no diatônico intenso de Ptolomeu (oitava dórica) e a de
  5/4 em Zarlino;
- makam Rast a 9 comas de Holder, maqam Rast com a terça neutra de 350;
- os 43 graus de Partch, a Bohlen-Pierce com período de 1901,955 e os 72
  melakartas;
- um campo de Hijaz sobre Ré escolhe o maqam Hijaz sobre Ré.

Na gravação real (v17), a faixa do Suno (Sol menor harmônica) com `--escala
maqam` escolheu **Hijazkar sobre Ré**: Sol menor harmônica tocada a partir
de Ré é a família do Hijaz.

**Limites:**

- os temperamentos históricos são transpostos para a tônica escolhida.
  Para manter a "cor das tonalidades", a tônica deveria ficar em Dó;
- a harmonia em movimento fica desligada nas escalas do arquivo (podem
  ser microtonais);
- a escala do arquivo ainda não se transforma ao longo da peça (é a
  máquina de escalas, abaixo).

## O banco ampliado por regra (2026-10-09)

O arquivo passou de 187 para **2559 escalas**, geradas por construções
teóricas citadas, todas no gerador:

| Família | Escalas | Construção e fonte |
| --- | --- | --- |
| MOS (escalas bem formadas) | 1753 | um gerador repetido em n partes iguais (5 a 53), cadeias de 5 a 12 notas com exatamente dois tamanhos de passo; Wilson; Carey e Clampitt, 1989 |
| gêneros gregos nas 7 espécies de oitava | 140 | os 20 gêneros, cada um girado nas espécies dórica, hipolídia, hipofrígia, hipodórica, mixolídia, lídia e frígia; Cleônides; Ptolomeu |
| divisões iguais da oitava | 95 | de 5 a 100 partes (o 12 está no catálogo) |
| conjuntos de produto combinatório | 84 | hexany, dekany e eikosany de {1 3 5 7 9 11 13}; Wilson, em Narushima, 2018 |
| melakartas | 72 | Venkatamakhin, 1660 |
| genera de Euler-Fokker | 61 | produtos de 2 a 4 fatores de {3 5 7 11}, reduzidos à oitava; Euler, 1739; Fokker |
| ajnas combinados | 57 | jins na tônica + jins na quinta, em 24 partes; Marcus, 1989 |
| séries harmônica e subarmônica | 55 | segmentos m a 2m (5 ≤ m ≤ 32); Partch; Johnston, 1964 |
| formas primas de 5 a 9 notas | 167 | todos os conjuntos de classes de altura (38, 50, 38, 29 e 12, como em Forte, 1973; algoritmo de Rahn, 1980) |
| temperamentos (com os mesotônicos de 1/7 a 1/11 de coma) | 17 | Barbour, 1951 |
| modos de Messiaen | 7 | *Technique de mon langage musical*, 1944 |
| as outras da primeira versão | ~50 | ver acima |

## A máquina de escalas (2026-10-09)

Pedido do autor: "a máquina de criar outras escalas, ou que gere desvios
propiciando novos encadeamentos harmônicos". Fica em
`src/core/ScaleMachine.*` e é ligada com `--maquina sim`.

**As operações** valem para qualquer escala, em cents e com período:

- **rotação:** um modo da mesma escala;
- **alteração:** um grau sobe ou desce um semitom, um quarto de tom ou
  uma coma;
- **espelho:** os intervalos invertidos; o dual da escala (o frígio do
  jônio);
- **interpolação:** cada grau caminha uma fração até a escala de destino.
  São estados intermediários entre duas escalas, o Frames (Atlas §40)
  aplicado à altura;
- **pivô:** a mesma escala noutra tônica, a que guarda mais notas comuns
  (modulação por notas comuns);
- **acréscimo e supressão:** um grau novo no maior intervalo, ou some o
  grau mais fraco no campo;
- **alargamento:** o período estica ou encolhe de 1 a 3 %, sem acumular
  de seção em seção.

**O encadeamento.** Da 2.ª seção em diante, a escala nasce da anterior,
guiada pelo modo e pelo campo da seção:

- **seguir:** desvios pequenos (alteração do grau mais fraco, pivô ou
  rotação). No teste, 125 de 140 notas ficaram em comum em 20 passos;
- **transfigurar:** interpolação de 35 a 75 % até uma escala do banco
  que veste a seção, com tamanho parecido (±2 graus); às vezes alarga o
  período;
- **contrapor:** o espelho; às vezes ganha ou perde um grau;
- **interromper:** o salto para a escala do banco com menos notas em
  comum.

Os destinos são sorteados **por família** (primeiro a família, depois a
escala). Sem isso, as 1753 MOS tomavam quase todos os sorteios.

A forma registra cada passo, por exemplo: "máquina de escalas: pivô por
notas comuns: a escala vai para Fá (6 de 7 notas em comum)".

**A harmonia passa a ser feita dos graus da escala.** Com a máquina, ou
numa escala microtonal ou não oitavante, os acordes da harmonia em
movimento (`docs/HARMONIA.md`) são **tríades de graus** da escala da
seção (grau, +2, +4). Em cada tempo, entra a tríade que melhor responde
ao acorde lido na referência. Assim qualquer escala tem harmonia, e cada
desvio da máquina muda a cor dos acordes: é o encadeamento harmônico
novo. Na afinação justa, as terças da voz de harmonia são de 5/4 (3,86
semitons).

**Validação** (`tests/ScaleMachineTests.cpp`):

- operações conferidas com a teoria:
  - a rotação do jônio dá o dórico sobre Ré;
  - o espelho do jônio dá o frígio;
  - o 4.º grau alterado dá o lídio;
  - a interpolação do maior ao menor harmônico (50 % e 100 %) confere;
  - o pivô do Dó maior num campo de Sol vai a Sol maior (6 notas
    comuns);
- o encadeamento é reprodutível;
- na peça, cada seção após a 1.ª marca o seu desvio, e a voz de harmonia
  sai "pelos graus das escalas da máquina".

**Na gravação real** (v18, `/media/luc/4tb_21042021/TRIOIO_respostas/v18-maquina-de-escalas/` (apagada em 2026-10-16: arquivo de teste)):

- **Suno, seguir:** rotações e alterações de um grau (6 ou 7 de 7 notas
  comuns);
- **Suno, contrapor:** espelhos, um que perde e outro que ganha um grau;
- **Suno, transfigurar:** interpolações até conjuntos de 8 e 9 notas;
- **Suno, interromper com maqam:** saltos (justa de limite 7, mesotônico
  de 3/10);
- **vocalise:** seguir; e transfigurar a partir do pelog.

**Variantes da escala por acorde** (2026-10-09; chord-scale, G. Russell,
*The Lydian Chromatic Concept of Tonal Organization*, 1953):

- sob cada acorde da harmonia em movimento, a nota do acorde que falta na
  escala da seção toma o lugar do grau vizinho mais próximo (até 150
  cents; a tônica não se move);
- as notas do motivo e do eco nesse grau seguem a alteração enquanto o
  acorde dura. Exemplos:
  - Lá menor natural sob Mi maior: Sol → Sol#, a menor harmônica;
  - Dó maior sob Ré maior: Fá → Fá#;
  - maqam Rast sob Dó maior: o Mi meio-bemol vai a Mi;
- vale para a escala da referência, do catálogo, do banco e da máquina;
- a forma registra quantas notas seguiram a variante e um exemplo ("sob
  Sol: Sib → Si").

**Limites:**

- num campo cheio (mistura do Suno, vocalise cromático), os destinos
  tendem a conjuntos cromáticos densos, coerente com o campo, mas pouco
  variado;
- a máquina age por seção, não dentro dela;
- as variantes por acorde mudam só as notas que caem no grau alterado;
  o modo inteiro do acorde (a escala-acorde completa de Russell) ainda
  não é escolhido.

**Recurso transversal** (declaração do autor, 2026-10-09): o banco e a
máquina servem a todos os instrumentos RASGO. Estão registrados no
inventário global como `KNOW-ESCALAS` e no Atlas. Outro instrumento copia
o arquivo (texto), o gerador e o leitor, com proveniência, e os adapta.

## Afeto e intenção (2026-10-09)

Estudo transversal com as fontes:
[`RASGO_DOCUMENTATION/teoria/ESCALAS_AFETO_INTENCAO_E_FUNCAO.md`](../../../RASGO_DOCUMENTATION/teoria/ESCALAS_AFETO_INTENCAO_E_FUNCAO.md).

**Mapa afetivo** (`src/core/ScaleAffect.*`). Propriedades estruturais de
qualquer escala, inclusive microtonal: brilho, terça (maior, menor ou
neutra), tensão, estabilidade, ambiguidade, familiaridade e uma valência
estimada. São tendências declaradas, não verdades.

- **Teste de sanidade:** a valência ordena os modos como Temperley e Tan
  (2013) mediram com ouvintes. Jônio +0,77 > lídio +0,72 > mixolídio
  +0,64 > dórico −0,08 > eólio −0,21 > frígio −0,39 > lócrio −0,63.
- O lídio é o mais "agudo", mas o trítono sobre a tônica pesa, como no
  estudo.

**Intenção** (`--intencao luminosa|sombria|tensa|serena|suspensa|ambigua|arco`):
um alvo no espaço afetivo que guia a máquina de escalas (e a liga):

- **seguir:** entre todos os desvios pequenos (cada grau e cada
  alteração, cada rotação, o pivô), escolhe o que mais se aproxima do
  alvo;
- **transfigurar:** o destino é avaliado pela escala **interpolada** que
  vai soar, não pelo destino em si;
- **contrapor:** o espelho, ganhando ou perdendo grau conforme o alvo;
- **arco:** a tensão pedida sobe até 2/3 da peça e resolve.

Cada seção registra o afeto estimado da sua escala na forma.

**No teste:**

- luminosa, a partir do eólio: valência −0,21 → +0,76;
- sombria, a partir do jônio: +0,77 → −0,40;
- arco: tensão 0,22 → 0,80 no clímax → 0,27.

**Na gravação real** (v19, `/media/luc/4tb_21042021/TRIOIO_respostas/v19-intencao/` (apagada em 2026-10-16: arquivo de teste)):

| Áudio e intenção | Resultado |
| --- | --- |
| Suno, luminosa | valência +0,52 → +0,76, tensão 0,65 → 0,22 |
| Suno, sombria | valência → −0,5 |
| Suno, arco | tensão 0,40 → 0,60 → 0,80 → 0,60 |
| Suno, tensa | tensão até 1,0 |
| vocalise, serena | tensão 0,53 → 0,14 |
| vocalise, ambígua | ambiguidade até 0,60 |

**Próximo:** o comportamento das escalas, conforme o §6.12 do estudo:

- subida e descida, notas de apoio;
- percurso (sayr, seyir, ālāp);
- frases-assinatura (pakaḍ, cengkok);
- ornamentos (gamaka, blue notes);
- cadências.

## Comportamento: a escala como a linguagem a usa (2026-10-09)

Estudo, §6:
[`RASGO_DOCUMENTATION/teoria/ESCALAS_AFETO_INTENCAO_E_FUNCAO.md`](../../../RASGO_DOCUMENTATION/teoria/ESCALAS_AFETO_INTENCAO_E_FUNCAO.md).
Na maioria das tradições, a escala é um **comportamento**, e não só um
conjunto de notas. O arquivo ganhou campos opcionais, preenchidos quando
a fonte descreve o comportamento:

```text
subida:    −111.731 203.910 386.314 590.224 884.359 1088.269 1200.000
descida:   1200.000 1088.269 884.359 701.955 590.224 386.314 203.910 0.000
apoio:     386.314 1088.269 0.000                (a 1.ª é a principal)
percurso:  0.000 386.314 701.955 1088.269 1200.000 386.314 0.000
assinatura: −111.731 203.910 386.314 203.910 ; −111.731 203.910 0.000 ; …
ornamento: meend                               (meend, andolan, blue)
cadência:  386.314 203.910 0.000
textura:   bordão                              (bordão, heterofonia ou acordes)
ritmo:     tīntāl                              (o ciclo usual: docs/RITMO.md, "Ciclos das culturas")
comportamento-fonte: …
comportamento-nota: …                          (o que foi construído pelo Trioio)
```

**Escalas com comportamento:**

| Escala | Comportamento | Fonte |
| --- | --- | --- |
| maqam Rast | percurso tônica → terça → quinta → oitava → volta; Sib na descida | Farraj e Abu Shumays, 2019; Marcus, 1989 |
| maqam Bayati | apoio na quarta | idem |
| maqam Hijaz | a segunda aumentada como identidade | idem |
| maqam Saba | apoio na terça, âmbito curto | idem |
| rāga Yaman | āroha sem Sa e Pa, pakaḍ ṇi re ga re · ṇi re sa · pa̤ re sa, vādī Ga, saṃvādī Ni, meend | Jairazbhoy, 1971; Bhatkhande |
| rāga Bhairav | vādī dha, saṃvādī re, pakaḍ, andolan | idem |
| slendro pathet nem, sanga, manyura | tons de cadência (**a conferir** em Hood) | Hood, 1954 |
| blues | blue notes arrastadas | Kubik, 1999 |
| modo de Mi andaluz | descida ao final pelo semitom de cima | Manuel, 1989 |
| modos dórico, frígio, lídio, mixolídio, eólio, lócrio | nota característica, cadência modal, vamp | Persichetti, 1961; Levine, 1995 |
| maqam Nahawand *(2026-10-11)* | sobe com a sensível (jins hijaz na quinta), desce com a 7.ª menor (kurd/nahawand); ghammaz na quinta | Farraj e Abu Shumays, 2019 |
| maqam Kurd *(idem)* | desce à tônica pelo semitom; ghammaz na quarta | idem |
| maqam Ajam *(idem)* | ghammaz na quinta; chega à tônica de cima | idem |
| maqam Sikah *(idem)* | tônica de três quartos; ghammaz no 3.º grau; âmbito estreito | idem |
| rāga Bhūpālī, Mālkauns, Bhairavī, Kāfī *(novas, idem)* | vādī e saṃvādī usuais (variam entre escolas), pakaḍ, meend ou andolan, tīntāl | Jairazbhoy, 1971; Bhatkhande |
| modos nordestinos: mixolídio, lídio com 7.ª menor, dórico *(idem)* | a nota característica no apoio; cadências; baião | Siqueira, 1981 |
| in e yō japonesas *(idem)* | tons nucleares a uma quarta; no in, a queda ao núcleo pelo semitom | Koizumi, 1958 |
| pentatônica chinesa *(idem)* | modo gong com apoio em zhi; graus conjuntos; heterofonia | Lau, 2008 |

As frases construídas pelo Trioio, e não transcritas, vêm marcadas.

**Sayr: o movimento numa linguagem linear (2026-10-11).** Ao ouvir as
respostas sobre Recording_187, o autor percebeu mais variação de harmonia
no modo andaluz (textura de acordes) que no Hijaz (heterofonia, sem
acordes). A diferença é a das tradições.

- **Condição:** quando a escala foi escolhida **automaticamente** e a
  textura de casa é heterofonia, as seções do meio **alternam** com uma
  escala vizinha do arquivo.
- **A vizinha:**
  - tem comportamento e a mesma textura;
  - tem tônica no ghammaz, a 2.ª nota de apoio;
  - tem no máximo uma nota diferente da escala de casa.
- **Casa:** a primeira e a última seção ficam na escala de casa.
- **Escala pedida pelo nome** (`--escala`): fica fixa.

Assim o maqam visita outra região e volta, sem acordes. É uma forma
simples do *sayr* e da modulação por ajnas (Farraj e Abu Shumays, 2019).

- **Recording_187, seed 1:** Hijaz sobre Lá ↔ **Nahawand sobre Ré**, com
  as mesmas 7 notas e o centro no ghammaz.
  - A subida/descida passa a agir em 4 notas: a sensível que o Nahawand
    usa só na subida.
  - Para ouvir: `TRIOIO_respostas/v31-linguagem-recording187/5-hijaz-com-sayr-s1.wav`.

**Como o Trioio aplica** (`src/core/ScaleIdiom.*`), seção a seção, sobre
a melodia:

1. **percurso:** o registro de cada seção vai à região correspondente do
   caminho;
2. **apoio:** os ataques fortes (depois de pausa, ou notas longas) vão às
   notas de apoio;
3. **assinatura:** frases que abrem a seção e o meio dela, com o ritmo do
   motivo;
4. **cadência:** as últimas notas da seção;
5. **subida e descida, por último:** cada nota vai à forma da escala na
   direção em que a melodia anda, sem inverter a direção e sem mexer nas
   frases da tradição;
6. **ornamento:**
   - meend: desliza até a nota seguinte;
   - andolan: inflexão lenta, aproximada por um glide curto;
   - blue: a nota chega de baixo, arrastada;
7. **textura bordão:** a harmonia em movimento dá lugar a um bordão de
   tônica e quinta, e o baixo vira pedal, como na prática da rāga e do
   maqam.

**Validação** (`tests/ScaleIdiomTests.cpp`):

- **Yaman:** de 232 subidas, só 9 vão a Pa ou a Sa sem vir do Ni (as
  entradas das frases da tradição); bordão só de tônica e quinta; meend
  presente;
- **Rast:** descendo, 31 notas em Sib contra 2 no Si meio-bemol; subindo,
  o meio-bemol (37);
- **blues:** blue notes com glide para cima;
- **sem comportamento** (Werckmeister III): nada muda.

**Na gravação real** (v20, `/media/luc/4tb_21042021/TRIOIO_respostas/v20-comportamento/` (apagada em 2026-10-16: arquivo de teste)):

- vocalise com Yaman, Bhairav e Rast;
- Suno com Hijaz (Sol menor harmônica a partir de Ré), modo de Mi andaluz,
  blues e slendro pathet manyura.

**Curva de altura e heterofonia (2026-10-09, segunda etapa):**

- **A nota ganhou uma curva de altura** (`Note::pitchAt`): pontos
  interpolados e uma oscilação (profundidade, frequência, atraso). Os 7
  pontos da síntese que liam a altura passaram a usá-la. Soa nas famílias
  de altura contínua: aditiva, sopro, FM, espectral, os módulos do Rasgo
  Modular, as vozes do Rasgo Synth e a CMOS. As de corpo fixo (corda
  pinçada, membrana, metal) não deslizam; nota ornamentada nelas passa ao
  sopro, e a forma registra.
- **Ornamentos reais:**
  - **andolan:** oscilação lenta e larga (±0,25 semitom a 1,1 Hz) nos
    apoios komal da Bhairav;
  - **gamaka** (kampita): oscilação rápida (±0,55 a 6 Hz), com Sa e Pa
    firmes; no melakarta 29 Dhīraśaṅkarābharaṇam (Sambamoorthy);
  - **blue:** a curva começa meio tom abaixo e chega à nota em 30 % da
    duração.
- **Medido no áudio** (`tests/SynthFamiliesTests.cpp`): ±1 semitom a 2 Hz
  aparece como razão de 1,09–1,10 entre o alto e o baixo (teórico 1,122;
  a janela de análise suaviza os picos); a curva de −2 semitons vai de 198
  a 221 Hz.
- **Heterofonia** (textura dos maqamat e dos pathet: A. J. Racy, *Making
  Music in the Arab World*, 2003; J. Becker, 1993): uma segunda voz toca a
  mesma melodia 20–80 ms depois, com corpo contrastante (corda pinçada sob
  melodia soprada, sopro sob as outras), apojaturas pelo grau vizinho,
  algumas oitavas e omissões; **sem acordes nem baixo**.

**Limites:**

- os tons de cadência dos pathet precisam ser conferidos em Hood (1954);
- o comportamento ainda não guia as memórias (oráculo, mosaico);
- o gamaka real tem muitas formas (kampita, jāru, sphurita…); só a
  oscilação foi feita.

## Misturas: recombinar culturas, linguagens e estilos (2026-10-09)

Pedido do autor: "aprimorar as recombinações e misturas de culturas,
linguagens, estilos; os mashups, as colagens, entre outras técnicas de
composição que propiciem novas sonoridades". Estudo, §6.13. Módulo
`src/core/Recombination.*`.

Uso: `--mistura "rāga Yaman + maqam Hijaz + blues" --tecnica colagem|hibrido|camadas|transicao`.
Cada termo escolhe, no arquivo, a escala que melhor veste a referência,
com o comportamento dela.

| Técnica | O que faz | Raiz |
| --- | --- | --- |
| **colagem** | cada seção toma uma cultura da lista, em ciclo, com escala, comportamento e **textura** própria: uma seção com bordão de rāga, a seguinte com heterofonia de maqam, a outra com acordes e blue notes; o corte entre seções faz parte do som | Berio, *Sinfonia* (1968); Zimmermann |
| **híbrido** | uma escala nova: os graus da primeira abaixo da quinta e os da segunda da quinta para cima, sobre a tônica da primeira; comportamento e textura da primeira, **ornamento enxertado** da segunda | a montagem por tetracordes (gêneros gregos, ajnas) atravessando culturas |
| **camadas** | a melodia numa escala; a segunda voz e a harmonia noutra, **sobre o mesmo centro**. Se a textura da primeira não tem segunda voz, uma voz heterofônica é acrescentada (mantendo bordão e acordes) | a polimodalidade de Bartók; o acorde de *Petrushka* (Stravinsky, 1911) |
| **transição** | a escala de cada seção é um estado intermediário entre as da lista, na ordem dada (0 %, 50 %, 100 %…); o comportamento é o da mais próxima | a interpolação da máquina de escalas; o Frames (Atlas §40) |

As **texturas passaram a valer por trecho**: bordão, heterofonia e acordes
convivem numa peça. O comportamento passou a ser por seção.

**Validação** (`tests/RecombinationTests.cpp`):

- **colagem:** as seções alternam Yaman → Hijaz → blues → Yaman, com
  bordão, heterofonia e comportamento;
- **híbrido:** os graus conferem (Yaman embaixo, Hijaz em cima) e o
  ornamento é enxertado;
- **camadas:** o bordão fica e a segunda voz canta na outra escala;
- **transição:** chega a 100 %;
- **termo sem escala:** avisa e segue.

**Na gravação real** (v22, `/media/luc/4tb_21042021/TRIOIO_respostas/v22-misturas/` (apagada em 2026-10-16: arquivo de teste)):

| Render | Mistura |
| --- | --- |
| Suno, colagem | Yaman + Hijaz + blues |
| Suno, híbrido | Bhairav embaixo + blues em cima |
| Suno, camadas | Hijaz na melodia, pathet manyura na segunda voz (401 notas) |
| Suno, transição | modo de Mi andaluz → Hijaz → Bhairav |
| vocalise, colagem | Saba + melakarta 29 + pathet nem |
| vocalise, camadas | Yaman com o Rast na segunda voz (136 notas) |

**Costuras** (`--costura`, 2026-10-09; `seamSections` em
`src/core/Recombination.*`). Como passar de uma seção (uma cultura) à
seguinte:

| Costura | Na fronteira |
| --- | --- |
| `corte` | o corte seco da colagem (padrão) |
| `pivo` | no último compasso, a melodia fica nas **notas comuns** às duas escalas, e uma nota comum sustentada atravessa a fronteira (modulação por nota comum); sem comuns, a tônica de quem entra |
| `ponte` | um compasso de cada lado toca a **escala intermediária**, a metade do caminho entre as duas (interpolação) |
| `sobreposicao` | a percussão e a harmonia de quem sai continuam um compasso dentro de quem entra, a 60 % (o crossfade do DJ) |

**Validação** (`tests/RecombinationTests.cpp`), de Dó maior para Lá♭
maior:

- **pivô:** a melodia fica só em Dó, Fá e Sol (as 3 notas comuns), e a
  nota sustentada atravessa a fronteira;
- **ponte:** todas as notas na escala intermediária;
- **sobreposição:** 8 golpes copiados, mais baixos.

**Na gravação real** (v28, `/media/luc/4tb_21042021/TRIOIO_respostas/v28-costuras/` (apagada em 2026-10-16: arquivo de teste)):
a colagem Yaman + Hijaz + blues do Suno com cada costura.

| Costura | O que aconteceu nas 4 fronteiras |
| --- | --- |
| pivô | 13 notas comuns, 4 notas sustentadas |
| ponte | 31 notas movidas |
| sobreposição | 26 golpes e acordes |

**Limites:**
- o mashup de **materiais** (trechos de várias referências ao mesmo tempo)
  ainda não existe: as misturas são de linguagens sobre uma referência;
- o ritmo de cada cultura (tala, iqa', clave, colotomia do gamelão) ainda
  não acompanha a escala.

## Prática modal (2026-10-09)

Pedido do autor: "música modal, etc.". O banco ganhou os modos como
**linguagem**, e não só como conjunto de notas: "modo dórico (prática
modal)", frígio, lídio, mixolídio, eólio e lócrio. Fontes: V. Persichetti,
*Twentieth-Century Harmony*, 1961; M. Levine, *The Jazz Theory Book*, 1995.

| Modo | Nota característica (apoio) | Cadência modal | Vamp |
| --- | --- | --- | --- |
| dórico | 6.ª maior | 6 → 5 → 3 → 1 | i–IV ("Oye Como Va", Puente, 1962) |
| frígio | 2.ª menor | 3 → b2 → 1 | i–bII |
| lídio | 4.ª aumentada | #4 → 5 → 3 → 1 | I–II |
| mixolídio | 7.ª menor | b7 → 1 (sem sensível) | I–bVII |
| eólio | 6.ª menor | b6 → 5 → b7 → 1 | i–bVI |
| lócrio | 5.ª diminuta | b2 → 1 | i°–bII |

- **Apoio:** a nota característica fica ao lado da tônica e da quinta, nos
  ataques fortes.
- **Textura vamp** (`vampLayer`, `src/core/ScaleIdiom.*`): os dois
  acordes (tríades dos graus do modo) alternam a cada compasso, no
  registro médio, sobre o baixo em pedal na tônica. O modo se sustenta
  sem a função tonal. Funciona por trecho, também na colagem.
- **Validação** (`tests/ScaleIdiomTests.cpp`):
  - o vamp dórico tem exatamente dois acordes, {0,3,7} e {0,5,9}, em 37
    tempos;
  - o baixo fica sempre na tônica;
  - o frígio fecha com o b2 descendo à tônica (8 chegadas).
- **Na gravação real** (v26, `/media/luc/4tb_21042021/TRIOIO_respostas/v26-modal/` (apagada em 2026-10-16: arquivo de teste)):
  - Suno em dórico, mixolídio, frígio e numa colagem lídio → dórico →
    frígio;
  - vocalise em eólio e lídio.

**Limites:**

- os vamps são de tríades; os voicings quartais do jazz modal (o "So What
  chord") ainda não existem (o `MelodyVoicingBank` do Studio tem
  quartais, a copiar);
- a troca de modo dentro da peça (a passagem de um modo ao vizinho, como
  na ponte de "So What", meio tom acima) pode ser feita com a máquina de
  escalas (pivô, rotação) e com a colagem.
