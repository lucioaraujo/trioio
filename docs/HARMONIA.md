# TRIOIO — harmonia em movimento

**Estado:** implementado em 2026-10-08 (`src/core/Harmony.*`,
`tests/HarmonyTests.cpp`). É a camada 1 do roteiro de musicalidade
(`docs/MUSICALIDADE.md`).

## Por quê

Diante de uma canção (a faixa do Suno: Sol menor harmônica, i–V–iv–III–VI),
o Trioio tocava um "campo" de notas por seção. O baixo ficava na tônica, e
nada acompanhava a progressão de acordes que faz a música andar.

## Como funciona

1. **Leitura dos acordes, tempo a tempo.** O croma de cada tempo, pesado
   pela amplitude, vota nos 36 acordes (maiores, menores e diminutos;
   fundamental com peso 1, terça e quinta com 0,8). Um **Viterbi**
   encontra o melhor caminho, com um custo de 0,18 para cada troca de
   acorde: é a abordagem de reconhecimento de acordes por HMM de A. Sheh e
   D. Ellis (ISMIR 2003), em implementação própria. Trechos com confiança
   média abaixo de 0,55 ficam sem acorde.
   - **O ritmo harmônico sai da música.** A primeira versão lia por
     compasso. Como a fase do compasso medido não coincidia com a troca,
     cada janela misturava duas harmonias, e a mistura inventava um
     terceiro acorde (Dóm + Sib → "Mib").
   - **A harmonia nasce da linha.** Numa referência de voz só, cada tempo
     tem uma nota. O Viterbi soma as notas de vários tempos até formar o
     acorde: uma linha que arpeja a progressão é lida certa em 32 de 32
     segundos.
2. **Transformação conforme o modo** (depois do deslocamento de tônica
   que o modo já dá à melodia: transfigurar +7, contrapor +6):
   - **seguir:** a progressão como está;
   - **transfigurar:** uma transformação **neo-riemanniana** por peça (P,
     L, R ou composta: LP, RP, PL, LR) aplicada a cada acorde. Cada
     operação simples mantém duas notas comuns e muda a cor, por exemplo
     Solm → Mib por L (Riemann; Cohn, Lewin). Usa a cópia fixada de
     `NeoRiemannian.hpp` do Studio;
   - **contrapor:** **rearmonização**. A dominante que resolve uma quinta
     abaixo vira a **substituta de trítono**, e 35 % dos outros acordes
     saltam por **mediante cromática** (±3 ou ±4 semitons). São as
     técnicas do `HarmonicWanderer` do Studio, reescritas com o gerador
     do Trioio;
   - **interromper:** a mesma progressão; a voz de harmonia toca 40 % de
     cada acorde, e um acorde em cada quatro cala.
3. **Aplicação:**
   - **baixo (voz 1):** as notas são partidas nas trocas de acorde, e cada
     parte toca a fundamental, na oitava mais perto da nota original
     (MIDI 31–55);
   - **motivo (voz 0):** nos ataques fortes (início do acorde, nota de um
     tempo ou mais, ou acento ≥ 0,7), uma nota fora do acorde vai à nota
     do acorde mais próxima (até 2 semitons); as outras ficam como
     passagem;
   - **voz de harmonia (voz 6, canal 7 do MIDI):** o acorde sustentado, em
     **condução parcimoniosa**: de cada acorde para o seguinte, a inversão
     que menos se move, no registro médio (MIDI 50–74). Timbre por peça:
     aditivo, sopro, flauta ou espectral, conforme as regras.

**Em qualquer escala (2026-10-09).** Com a máquina de escalas
(`--maquina sim`), ou numa escala microtonal ou não oitavante (`justa`,
`edo`, `arquivo`…), os acordes são **tríades dos graus da escala** da
seção (grau, +2, +4; em escalas de menos de 5 graus, graus vizinhos). Em
cada trecho entra a que melhor responde ao acorde lido na referência, com
nomes em graus romanos (I, IV, II…). O baixo, o motivo e a voz de
harmonia seguem essas alturas, inclusive fracionárias. As transformações
neo-riemanniana e de rearmonização ficam só nos 12 sons; nos graus, quem
transforma é a máquina, de seção em seção (`docs/ESCALAS.md`).

`--harmonia nao` desliga a camada.

A forma registra uma linha, por exemplo: "harmonia em movimento: 69
acordes lidos (Mi Solm Mibm Dóm Ré…) · como a referência: … · baixo na
fundamental (83 notas), 182 notas do motivo levadas ao acorde, voz de
harmonia em espectral".

## Validação

`tests/HarmonyTests.cpp`, com Solm Ré Solm Dóm Sib Mib Ré Solm (2 s cada,
duas voltas):

- **leitura:** 32 de 32 segundos certos com os acordes, e 32 de 32 com uma
  linha que só arpeja;
- **seguir:** baixo na fundamental 52/52; ataques fortes do motivo no
  acorde ≥ 90 %; voz de harmonia com 2,9 semitons de movimento médio por
  troca (somadas as 3 vozes);
- **transfigurar e contrapor:** marcados ("neo-riemanniana P", "6
  substituições de trítono, 5 mediantes cromáticas");
- **microtonal ou desligada:** sem voz de harmonia.

O teste do Terceiro Olho passou a aceitar no congelamento as notas que
**soavam** nos 3 s anteriores. Um acorde sustentado da voz de harmonia
pode ter começado antes disso e ainda estar soando, e o olho, com razão, o
congela.

**Na gravação real** (v16, `/media/luc/4tb_21042021/TRIOIO_respostas/v16-harmonia-*` (apagada em 2026-10-16: arquivo de teste)):

- **Suno:** 69 acordes lidos ("Mi Solm Mibm Dóm Ré Mibm Ré Mibm Sol Mib Ré
  Solm"). Solm, Ré e Dóm aparecem, mas "Mibm" deve ser Mib maior
  confundido pelo Fá# da voz na mistura;
- **vocalise:** 74 acordes, que andam cromaticamente como a voz;
- **comparação:** `1-seguir` e `1b-seguir-sem-harmonia` são o mesmo seed,
  com e sem a camada.

## Limites e próximos passos

- **Erros de leitura em misturas cheias**, como na faixa do Suno: a
  melodia entra no croma. Ajudariam pesar o grave (o baixo define a
  fundamental), usar sétimas e suspensões nos moldes e acrescentar uma
  tonalidade global que favoreça os acordes do tom (o "contexto" do
  HMM).
- **Só tríades.** Faltam sétimas, nonas, suspensões e o voicing do banco
  do Studio (quartal, planing).
- **Função harmônica:** cadências, dominantes secundárias e preparação de
  modulação ainda não são escritas, só lidas e transformadas.
- **Dissonância:** o tratamento do Studio (apojatura, nota errada, cor)
  entraria aqui.

## Voicings (2026-10-09)

Como empilhar o acorde (`src/core/Voicing.*`). Reescrito a partir dos
princípios do `MelodyVoicingBank` do Rasgo Synth Studio (o "Eixo B" da
flauta, commit `babe6e8`; lido, não copiado: o arquivo do Studio é um
sintetizador com osciladores próprios) e estendido para respeitar a escala
da seção.

| `--voicing` | Construção | Referência |
| --- | --- | --- |
| `tercas` | a tríade com condução parcimoniosa (padrão) | — |
| `quartal` | quartas empilhadas (0 5 10), sem terça | McCoy Tyner |
| `sowhat` | três quartas e uma terça maior (0 5 10 15 19) | Bill Evans e Miles Davis, "So What", *Kind of Blue* (1959) |
| `planing` | a forma do 1.º acorde movida em paralelo, cromática | Debussy, "Voiles" (1909) |
| `bitonal` | duas tríades a um trítono | Stravinsky, *Petrushka* (1911); Milhaud |
| `aberto` | quintas abertas (0 7 14) | o bordão com a quinta (o dam do duduk) |
| `cluster` | três graus vizinhos da escala | H. Cowell, *New Musical Resources* (1930); Ligeti |

- **Ajuste à escala:** só os desvios microtonais (até 60 cents) são
  corrigidos; uma nota fora da escala por um semitom é cor do voicing.
  - **Correção registrada:** a primeira versão ajustava até um semitom.
    Numa escala pentatônica, a quarta Dó–Fá virava a terça Dó–Mi, e o
    quartal perdia a identidade (só 8 de 22 intervalos em quartas). Agora
    são 22 de 22.
- **Onde vale:** na voz de harmonia e no vamp modal. O dórico do banco usa
  So What e o mixolídio usa quartal, pelo campo `voicing:` do
  comportamento.
- **Validação** (`tests/VoicingTests.cpp`):
  - So What sobre Ré dórico: Ré Sol Dó Fá Lá;
  - sobre Sol: Sol Dó Fá Sib Ré (o Sib fica);
  - quartal, aberto, bitonal, cluster e planing (Mib maior em paralelo a
    Dó maior, não menor);
  - a quarta de 22-EDO puxa a quarta justa;
  - na peça, 22 de 22 intervalos em quartas.
- **Na gravação real** (v27, `/media/luc/4tb_21042021/TRIOIO_respostas/v27-voicings/` (apagada em 2026-10-16: arquivo de teste)):
  - o Suno nos 7 voicings, com a mesma seed, para comparar;
  - o dórico modal com vamp So What;
  - o mashup da voz do vocalise sobre o Suno, em quartal.

## Harmonia escrita (2026-10-17)

`src/core/HarmonyWriting.h/.cpp` e o passo 4b de `Harmony.cpp`. É o item 6
da auditoria de 14/10, pedido pelo autor ("avance com a harmonia escrita").

Até aqui a harmonia era lida na referência e transformada pelo modo, mas a
função harmônica não era escrita. A camada nova entra entre a leitura (e a
transformação do modo) e a aplicação: o baixo, o motivo e a voz de harmonia
seguem o que for escrito. Vale nos 12 sons temperados; pelos graus da
escala (a máquina de escalas, as afinações microtonais) a harmonia fica
como antes.

1. **A tonalidade,** pelos perfis de Krumhansl e Kessler (*Psychological
   Review* 89(4), 1982) sobre os acordes, pesados pela duração.
   - **A da peça:** uma tonalidade global.
   - **A de cada seção:** só quando ela muda de verdade, numa seção de 8
     tempos ou mais cujo tom case claramente melhor (+0,15 de correlação)
     que o da peça.
   - **Corrigido no caminho:** na primeira versão, cada seção tinha o seu
     tom, e no rec187 quase toda seção "modulava" (6 modulações, 1
     cadência).
   - **Modulação** é só a troca de tônica; Lá maior e Lá menor não contam.
2. **As cadências** no fim de cada seção, pelo modo (W. Piston, *Harmony*,
   1987):

   | modo | cadência |
   |---|---|
   | seguir | autêntica (V → I, 60 %) ou suspensiva (termina em V) |
   | transfigurar | às vezes plagal (IV → I) |
   | contrapor | de engano (V → vi; no menor, V → VI) |
   | interromper | suspensiva |
   | o fim da peça | sempre perfeita (IV → V → I) |
   | jazz | ii → V → I |
3. **As dominantes secundárias:** antes de um acorde que não é a tônica, a
   dominante dele (V/x) às vezes toma a segunda metade do acorde anterior
   (de 20 % a 45 %, conforme o modo). Os acordes de estilo (`styleVoicing`)
   já reconhecem essa dominante e lhe dão a sétima.
4. **A modulação preparada:** quando a seção seguinte muda de tônica, o fim
   da seção chega à dominante do tom novo.
5. **A dissonância tratada** (o passo 4b): no tempo forte, numa nota longa
   do motivo que é nota do acorde, às vezes entra a apojatura.
   - **Quanto:** de 10 % a 30 %, conforme o modo.
   - **Que nota:** a de cima (um tom; meio tom sobre a terça maior, o
     retardo 4–3).
   - **Como:** toma ~40 % do valor, acentuada, e resolve por grau, mais
     suave.

**Onde liga:** ligada no app e na CLI (`--harmonia-escrita nao` desliga);
no núcleo, `Options::writeHarmony`, desligada, para os testes antigos.

**Testado** (`tests/HarmonyWritingTests.cpp`):

- Dó maior e Lá menor reconhecidos;
- o fim IV–V–I;
- a cadência de engano ao contrapor;
- a modulação para Sol preparada por Ré;
- as dominantes secundárias em 9 de 10 sementes;
- o ii–V–I no jazz.

**No rec187:**

| resposta | o que a escrita fez |
|---|---|
| choro, seguir | Lá maior com um trecho em Lá menor: 4 cadências autênticas, 2 suspensivas, 1 modulação, 5 dominantes secundárias |
| jazz, seguir | o mesmo, em ii–V–I |
| bolero, contrapor | a rearmonização do contrapor espalha a harmonia por tons distantes, e a escrita a lê como 6 modulações preparadas, o que combina com o caráter do modo |

**A escuta:** `AUDIOS_RASGO/Trioio/escutas/2026-10-17-harmonia-escrita/`.

**Na mesma data:**

- **A melodia fecha na cadência** (`CadenceMark`). A última nota do motivo
  antes do fim do acorde de chegada vai para a nota mais próxima entre as
  da cadência, e se alonga até o fim do acorde (o ponto final da frase):

  | cadência | a nota de chegada |
  |---|---|
  | autêntica, plagal, o fim | a tônica ou a terça |
  | suspensiva, modulação | uma nota da dominante (a 5ª, a 2ª, a sensível) |
  | de engano | o vi ou a tônica |
- **O retardo preparado** na voz de harmonia. Na troca de acorde, a nota do
  acorde anterior que está um grau acima de uma nota do novo, e não
  pertence a ele, fica presa nos primeiros 45 % (a dissonância) e desce
  para resolver: o 4–3 e o 9–8 da escrita coral (Piston, cap. 9). Acontece
  em 35 % dos casos possíveis, em acordes de dois tempos ou mais.
- **Testado:** as cadências registradas (a suspensiva no meio ao
  interromper, a autêntica no fim, terminando no fim da peça).
- **No rec187** (seguir): 6 frases fechadas na cadência e 13 retardos.
- **Onde o retardo aparece:** nos conjuntos com figuras de estilo, a voz de
  harmonia é rearticulada nos trechos com pulso, e o retardo fica só nos
  trechos sem pulso. Na orquestra e na câmara, que não têm figuras, ele
  aparece na peça toda.

**A condução de vozes a quatro partes** (`voiceLead`, na mesma data): o
baixo (a fundamental) e três vozes de cima, numa busca entre todas as
disposições (as permutações das notas e as oitavas de cada voz, entre Dó 3
e Fá 5), pontuadas pelas regras clássicas (Piston, cap. 5–6; D. Huron,
*Music Perception* 19(1), 2001):

- o movimento mínimo, com as notas comuns ficando;
- sem quintas nem oitavas paralelas entre quaisquer duas vozes, o baixo
  incluído;
- sem cruzamento, e no máximo uma oitava entre vozes vizinhas;
- a sensível sobe à tônica no V → I (e também no I → IV, em que o I é a
  dominante do IV).

**Testado:** I–IV–V–I–vi–ii–V–I sem paralelas, com a sensível subindo nas 3
vezes.

**No rec187** (a orquestra): 112 acordes conduzidos, sem nenhuma paralela.

**A escuta:** `orquestra-seguir-v3-quatro-partes.wav`.
