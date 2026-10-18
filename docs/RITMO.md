# TRIOIO — ritmo como campo

**Estado:** implementado em 2026-10-08 (`src/core/RhythmField.*`,
`tests/RhythmFieldTests.cpp`).

## De onde vem

O Atlas de Referência RASGO, na §45 ("Grids → ritmo como campo"), propõe
"pensar ritmo não apenas como uma sequência fixa de passos, mas como
navegação em um campo de padrões e densidades": campo → posição →
densidade → probabilidade → evento. A referência é o Mutable Instruments
Grids, de Émilie Gillet (MIT). Dele veio só o comportamento: nenhum código
e nenhum mapa. No Grids, o mapa é fixo, treinado em baterias. No Trioio,
**o mapa é a própria referência**.

Até aqui o Trioio tinha melodia, baixo, eco, memória e objetos sonoros,
mas nenhuma camada rítmica.

## Como funciona

1. **O campo:** cada compasso da referência com ataques (a grade rítmica já
   medida pela escuta) vira um ponto num mapa de duas dimensões:
   - **densidade:** ataques por passo;
   - **síncope:** a fração dos ataques fora do tempo.

   O ponto guarda o padrão de ataques do compasso, com a força de cada
   passo.
2. **O caminho:** a cada compasso da resposta, a posição parte do compasso
   da referência naquele instante e é transformada pelo modo:
   - **seguir:** fica onde está;
   - **transfigurar:** a síncope espelhada (1 − s);
   - **contrapor:** a densidade complementar (1 − d);
   - **interromper:** um compasso em cada quatro fica vazio.

   Um passeio aleatório, mais solto com a imprevisibilidade, desvia a
   posição.
3. **As probabilidades:** os 4 padrões mais próximos da posição se misturam,
   pesados pela distância. Se a posição pede mais (ou menos) síncope do que
   esses padrões têm, golpes são **deslocados** do tempo para o contratempo
   (ou o contrário), na proporção da diferença. Sem isso, numa referência
   de compassos iguais, mover a posição não mudava o som.
4. **Os golpes:** cada passo soa com a sua probabilidade, escalada pela
   densidade da posição. Forte e no tempo vai para a faixa **grave**, o
   tambor. O resto vai para a faixa **aguda**, que muda de peça para peça:
   striker granular, LPG ou corda pinçada.

A camada é a voz 4 (canal 5 do MIDI). Ela só entra nos trechos **com
pulso**: as seções em tempo livre ficam sem percussão. A forma registra
uma linha, por exemplo "campo rítmico: 47 compassos, 114 golpes · posição
média: densidade 0,52, síncope 0,60 · tambor/striker granular".

**Com as outras camadas:**

- o Terceiro Olho não transforma a percussão em altura (espelho, escala
  mutante, acorde que vira campo, melodia que segue o gesto, troca de
  timbre), mas silêncio súbito, tempo deformado e congelar valem para ela;
- as regras de âmbito e salto máximo não se aplicam a ela, e as famílias
  proibidas são respeitadas;
- `--ritmo nao` desliga a camada.

## Validação

`tests/RhythmFieldTests.cpp`, com uma referência a 120 BPM, golpes quase
todos no tempo e 6 seeds:

- **seguir:** 290 golpes em 90 compassos, todos na grade; 288 nos passos
  que a referência usa; 16 % fora do tempo;
- **transfigurar:** 85 % fora do tempo (a síncope espelhada);
- **contrapor:** 403 golpes contra 290 (a referência é rala, e a densidade
  complementar a adensa);
- **tempo livre e `--ritmo nao`:** nenhum golpe;
- **regra "proibido timbre tambor":** o grave usa outra família.

**Gravação real** (v13, em
`/media/luc/4tb_21042021/TRIOIO_respostas/v13-ritmo-campo/` (apagada em 2026-10-16: arquivo de teste)):

- 47 compassos nas versões com pulso forçado, de 11 a 15 nas que têm
  seções em tempo livre;
- síncope média de 0,33 a 0,73, conforme o modo;
- a faixa aguda em LPG, corda pinçada ou striker.

## Limites e próximos passos

- **Duas faixas.** O Grids tem três (bumbo, caixa e chimbal), e uma faixa
  média de membrana completaria isso.
- **Posição a cada compasso.** Ela poderia ser guiada também pela energia
  da própria resposta, ou pela escuta do autor.
- **A percussão não aprende com a escuta.** As famílias dela entram no
  julgamento como as outras, mas a posição no campo não.

## Ciclos das culturas (2026-10-09)

O campo rítmico acima vem da própria referência. Para as misturas de
culturas (`docs/ESCALAS.md`, "Misturas"), o Trioio ganhou também um
**arquivo de ciclos rítmicos próprio**:
`data/ritmos/arquivo_trioio_ritmos.txt`, com 26 ciclos, gerado por
`tools/gerar_arquivo_de_ritmos.py`. Foi escrito a partir das fontes, cada
ciclo com a citação e com a marca de simplificação quando é o caso.

| Família | Ciclos | Fonte |
| --- | --- | --- |
| iqa' árabe | maqsum, baladi, sa'idi, wahda, samai thaqil | Farraj e Abu Shumays, 2019; Racy, 2003 |
| tala hindustani | tīntāl (16), jhaptāl (10), rūpak (7), ektāl (12) | M. Clayton, *Time in Indian Music*, 2000 |
| tala carnático | ādi tāla (4+2+2) | Sambamoorthy |
| usul turco / aksak | sofyan, curcuna (3+2+2+3), aksak 9/8; ruchenitsa 7/8, kopanitsa 11/8 | Signell, 1977; Brăiloiu, "Le rythme aksak", 1951 |
| claves e linhas-guia | clave son e rumba, clave da bossa nova, tresillo, cinquillo, sino padrão 12/8 | Toussaint, 2013; Agawu, 2016 |
| Brasil | baião (zabumba) | Siqueira, 1981 |
| gamelão | lancaran, ketawang (colotomia: kethuk, kenong, kempul, gong) | Lindsay, 1979; Becker, 1993 |
| flamenco | compás de 12 (soleá, bulerías) | Manuel, 1986 |
| blues | shuffle | Kubik, 1999 |

**Formato:** `golpes:` tem um símbolo por unidade. Os símbolos são:

- D grave; T agudo; k leve; B grave e agudo (dhā); x clave ou sino;
- G gong; N kenong; P kempul; t kethuk; - silêncio.

A `unidade:` é tempo, colcheia, colcheia ternária ou semicolcheia. Há
também `divisões:`, `acentos:` (o sam e os tempos fortes) e `vazios:` (o
khālī).

**Como toca** (`src/core/RhythmCycles.*`). Quando a escala de uma seção
pede o seu ciclo (o campo `ritmo:` do comportamento), ou quando
`--ciclo NOME` o força para a peça, a percussão (voz 4) daquele trecho
toca o ciclo, ancorado no pulso:

- os golpes do campo rítmico no trecho são substituídos;
- os acentos ficam mais fortes;
- cada símbolo tem o seu corpo: tambor, membrana, LPG, madeira (MATTER) ou
  gongs metálicos.

Na colagem, cada seção toca o ciclo da sua cultura.

Associações usuais no banco de escalas (não obrigatórias):

| Escala | Ciclo |
| --- | --- |
| Rast, Hijaz | maqsum |
| Bayati | baladi |
| Saba | wahda |
| Yaman | tīntāl |
| Bhairav | ektāl |
| melakarta 29 | ādi tāla |
| pathet nem e sanga | lancaran |
| pathet manyura | ketawang |
| blues | shuffle |
| modo de Mi | compás de 12 |

**Mashup de ritmo:** `--ciclo` sobre qualquer escala (um maqam sobre
clave cubana, um maqam sobre aksak búlgaro).

**Validação** (`tests/RhythmCyclesTests.cpp`):

- o arquivo é legível e citado;
- maqsum com 20 de 20 golpes nas posições;
- tīntāl com sam acentuado e o khālī sem grave em tin tin tā tā;
- lancaran com o gong no 16;
- a Yaman chama o tīntāl; `--ciclo` força a clave; termo desconhecido é
  avisado.

**Correção registrada:** a primeira descrição dizia que o 9.º tempo do
tīntāl (o khālī) era sem grave. No ṭhekā usual o 9 é dhā; o khālī se ouve
em tin tin tā tā (10 a 13).

**Na gravação real** (v23, `/media/luc/4tb_21042021/TRIOIO_respostas/v23-ciclos-ritmicos/` (apagada em 2026-10-16: arquivo de teste)):

- Suno em colagem: tīntāl, maqsum e shuffle por seção;
- Hijaz sobre clave son;
- pathet manyura com ketawang;
- modo de Mi com o compás de 12;
- vocalise: Bhairav em ektāl; Rast sobre kopanitsa 11/8.

**Limites:**

- os ciclos são esqueletos; a percussão real ornamenta e improvisa sobre
  eles;
- o ciclo segue o pulso medido da referência, sem o andamento próprio de
  cada tradição (vilambit, madhya, drut);
- ~~a melodia ainda não se alinha ao ciclo~~: feito, ver abaixo.

### A melodia chega no ciclo (2026-10-09)

Nas tradições de ciclo, as frases **convergem** no tempo principal: o
**sam** do tala, o **gong** do gamelão, o **dum** do iqa'.
`alignMelodyToCycles` (`src/core/RhythmCycles.*`) faz isso, depois de os
ciclos tocarem:

- **chegada:** em cada volta do ciclo, a nota da melodia mais próxima do
  tempo principal (o 1.º acento do ciclo: o sam no tala; o 16 no
  lancaran e no ketawang; o dum no iqa') passa a começar nele, na **nota
  de apoio** da escala (vādī, ghammaz, tom de gong do pathet), ou na
  tônica, com acento;
- **tihai** (só nos tala): antes do último sam da seção, a frase de três
  notas que vinha da melodia soa **três vezes**, com um respiro de uma
  unidade, e a última nota cai exatamente no sam (M. Clayton, *Time in
  Indian Music*, 2000).

**Validação** (`tests/RhythmCyclesTests.cpp`):

- lancaran: as duas chegadas no gong (7,5 e 15,5 s), no tom de apoio;
- tīntāl: o tihai em 11,0 11,5 12,0 · 13,0 13,5 14,0 · 15,0 15,5 16,0, a
  última no sam (16 s);
- na peça, a Yaman marca "a melodia chega no ciclo";
- **correção registrada:** uma nota original logo depois do sam ficava
  sobreposta à nota final do tihai; agora o trecho é limpo até o fim
  dela.

**Na gravação real** (v24, `/media/luc/4tb_21042021/TRIOIO_respostas/v24-chegadas-tihai/` (apagada em 2026-10-16: arquivo de teste)):

| Render | Chegadas | Tihai |
| --- | --- | --- |
| Suno, Yaman (tīntāl) | 18 | 5 |
| Suno, Bhairav (ektāl) | 26 | 5 |
| Suno, pathet sanga (lancaran) | 19, no gong | — |
| Suno, colagem Yaman + Hijaz + pathet nem | 45 | 2 |
| vocalise, Yaman | 2 | 4 |
| vocalise, melakarta 29 (ādi tāla) | 8 | 4 |

**Limites:**

- o tihai usa sempre 3 notas de uma unidade; tihais maiores (de frase
  longa, "bedam" e "dumdar") e a mukhṛā (a frase que leva ao sam) ainda
  não existem;
- no vocalise, de tempo livre, as chegadas são poucas: há pouco pulso
  para ancorar.

## Groove e microtempo da referência (2026-10-14)

`src/core/Groove.*`, `Options::groove` (padrão ligado); auditoria §6, item 2.

A resposta deixava de ter balanço: toda nota caía na grade de semicolcheias.
Agora o groove é medido nos ataques da referência e aplicado às notas da
resposta que estão na grade, nos trechos com pulso.

**Três medidas** (`measureGroove`):

1. **Swing.** Onde caem os contratempos dentro do tempo: reto é 0,5;
   ternário, 0,667 (Benadon 2006, a razão das colcheias no jazz). Também o
   swing das semicolcheias dentro de cada metade.
2. **Microtempo por posição.** O resíduo mediano de cada semicolcheia do
   compasso depois do swing, descontada a deriva lenta (a mediana em ±4
   tempos). Por exemplo, a caixa atrás do tempo nos 2 e 4 (Danielsen 2010;
   Wright e Berdahl 2006, sobre o microtempo do samba). Só com ≥ 4 ataques na
   posição; abaixo de 8 ms, zero; limitado a ±60 ms.
3. **Acento por posição:** a raiz da força média sobre a média, entre 0,8 e
   1,2. O acento molda a dinâmica, não a substitui.

**A aplicação** (`applyGroove`) deforma a grade dentro de cada tempo:

- a deformação é linear por metade, para que 0,5 vá para 0,5 + swing;
- soma o microtempo da posição e multiplica a força pelo acento;
- o fim de cada nota fica onde estava;
- só nas notas da grade (±5 % do passo) e nos trechos com pulso; o tempo
  livre, os ornamentos e os objetos do olho ficam como estão.

**Por modo:**

- seguir e interromper: 100 %;
- transfigurar: 60 %;
- contrapor: a grade reta, contra o balanço; a forma anota quando a
  referência balança e a resposta não.

**Dois vieses achados e corrigidos na primeira medição real:**

- **A janela larga inventava swing.** A primeira versão tomava a mediana dos
  contratempos entre 0,40 e 0,71 do tempo. Com ataques soltos (o tempo
  expressivo de uma gravação), essa mediana cai no meio da janela, ~0,555,
  e inventava 24–34 % de swing em todas as gravações.
  - Agora o swing só existe quando os contratempos se **concentram** na
    região ternária (0,58–0,72), em clara maioria (1,5×) sobre a reta
    (0,42–0,58), e são ≥ 1/6 dos ataques.
  - O teste "solto" (colcheias com ±12 % de desvio aleatório) guarda isso.
- **Pulso fraco não tem grade.** Abaixo de 0,25 de confiança, a mesma régua
  das linhas (`core/Lines.h`), não há groove.

**Medido:**

| referência | pulso | groove |
|---|---|---|
| Recording_187 | 74,4 BPM, confiança 0,58 | sem swing; microtempo até 20 ms na 2.ª semicolcheia do compasso, nas 830 notas da grade |
| Suno | 84,9, confiança 0,21 | nenhum (pulso fraco) |
| vocalise | 108,7, confiança 0,07 | nenhum (pulso fraco) |

**Testes** (`tests/GrooveTests.cpp`):

- o ternário e a caixa +20 ms recuperados (0,167 de swing, +20 ms nos tempos
  2 e 4, acentos);
- colcheias retas levadas ao balanço;
- 50 % pela metade;
- a nota fora da grade intacta;
- a referência reta e os ataques soltos sem swing;
- o pulso fraco sem groove.

As suítes de composição, ciclos (o tihai de tempos exatos), campo rítmico,
escalas e olho passam sem mudança: as referências sintéticas são retas e
ficam abaixo dos limiares.

**Limites:**

- O microtempo é por posição do compasso, não por instrumento. Na linha
  grave e na aguda das linhas por frequência, cada uma mede o seu: o
  bumbo e o chimbal com balanços próprios. Isso já vale nas linhas, porque
  cada linha compõe com os próprios ataques.
- O swing é um só para a peça inteira. Um swing que muda por seção pediria a
  medida por seção.
- O rubato nas seções de tempo livre foi feito em 2026-10-14 (abaixo).

## Rubato nas seções de tempo livre (2026-10-14)

`applyRubato` (`src/core/Groove.*`), junto com o groove (`Options::groove`).

O groove só vale nos trechos com pulso. Nas seções de tempo livre, as notas
eram tocadas no espaçamento exato que o compositor escreveu. Agora cada frase
livre tem o **arco de frase** de N. P. Todd ("The dynamics of dynamics: a
model of musical expression", *JASA* 91(6), 1992): o tempo se apressa um
pouco no meio e se alarga nas pontas.

- **As frases:** as notas do motivo separadas por pausas de mais de 0,6 s,
  dentro de cada seção livre (as seções que não estão em `pulsedSpans`).
  Frases de menos de 1 s ficam como estão.
- **A deformação:** w(u) = (u + β((u − ½)³ + ⅛)) / (1 + β/4).
  - Os extremos ficam no lugar.
  - O tempo local é mínimo no meio e máximo nas pontas, com razão 1 + 3β/4.
  - Todas as vozes da frase seguem a mesma deformação, e o fim de cada nota
    também é deformado.
- **Por modo:**
  - seguir: β = 0,4, pontas 30 % mais lentas;
  - transfigurar: β = 0,6, pontas 45 % mais lentas;
  - interromper: β = 0,2;
  - contrapor: reto.
- **Medido (Recording_187, seed 3):**
  - seguir: 7 frases, 100 notas;
  - transfigurar: 9 frases, 114 notas.
- **Teste** (`GrooveTests` §5): 16 notas a cada 0,25 s viram 0,287 s nas
  pontas e 0,227 s no meio; os extremos ficam onde estavam; a nota de fora
  da seção fica intacta; contrapor fica reto.
- **Limite:** o arco é o mesmo para toda frase livre. Um arco que segue a
  dinâmica da frase (o crescendo com o acelerando, como no modelo completo
  de Todd) é o passo seguinte.

