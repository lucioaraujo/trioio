# A árvore de influências

```
trioio_biblioteca responder <dados> <ref> --influencias "rock/noise, brasil/nordeste/maracatu"
trioio_biblioteca responder <dados> <ref> --influencias auto
```

## Por que não um catálogo de estilos

O autor, em 18/10/2026, enquanto o rock ganhava subestilos:

- "todos os estilos, não somente o rock, possuem variações";
- "nuances, intenções, que vem muito da poética de cada artista ou banda";
- "contextos sociais, contextos históricos, tecnológicos, de geração";
- "dentro de uma classe há diversas subclasses, que possuem diversas formas
  de se fazer musica, e assim sucessivamente, a ideia na verdade é mais
  criar algo que não se repita ou se limite em algo engessado";
- "nos nutrimos com informações para criarmos algo novo", "uma das bases
  dos instrumentos rasgo";
- "não tenho como afirmar que todo rock há bateria";
- "os instrumentos rasgo precisam dos conceitos";
- "a repetição pode existir se o conceito precisar da repetição".

Daí a árvore. Não é uma lista de gêneros para imitar: é uma árvore aberta
de influências, de onde cada resposta tira parâmetros, cruza galhos
distantes e muta. O objetivo é o inusitado, não a fidelidade a um rótulo.
Os subestilos do rock (`--rock`) e os conjuntos (`--conjunto`) continuam,
como pontos de entrada.

## O arquivo

`data/influencias/arquivo_trioio_influencias.txt` é gerado por
`tools/gerar_arquivo_de_influencias.py`, que descreve o formato. Na versão
inicial são 38 nós: rock, jazz, músicas brasileiras, erudita, eletrônica,
experimental e tradições do mundo, cada um com a sua fonte. Cada nó é um
caminho (`rock/pesado/lento`) e traz:

- **o conceito:** a ideia daquele fazer musical (19 nós já têm);
- **o contexto:** social, histórico, tecnológico e de geração, como texto;
- **os traços, que são TENDÊNCIAS:**
  - `presença:percussão 0.85` é a chance de a percussão estar;
  - `conjunto:rock 0.8`, `harmonia:modal 0.6`, `humor:tensa 0.5`,
    `escala:frígio 0.3` e `ciclo:shuffle 0.4` são escolhas com a sua chance;
  - `eixo:dança|peso|ruído|rasgo|repetição` dão o valor de 0 a 1;
- **a fonte.**

**Os filhos herdam os traços dos pais** e podem mudá-los: `rock/pesado/lento`
tem percussão 0,7 e peso 1,0.

**As poéticas de artistas e bandas** não são inventadas pelo Trioio: o
autor as escreve no mesmo formato, com as suas fontes.

## O sorteio (`drawInfluences`, `src/core/Influences.h`)

1. **Os nós:** os pedidos (pelo caminho ou pelo fim dele: "pesado" acha
   `rock/pesado`). Com `auto`, um nó escolhido pela raridade.
2. **A polinização:** um galho de outra raiz, com peso 0,35, com chance de
   (1 − repetição)·(0,3 + 0,5·imprevisibilidade).
3. **As escolhas** (conjunto, subestilo do rock, harmonia, humor, escala,
   ciclo):
   - entre os candidatos, pela tendência vezes a raridade;
   - e só com a chance da tendência: às vezes não se cumpre.
4. **As presenças:** cada voz pela sua chance. O motivo sempre fica.
5. **Os eixos, por cruzamento:** cada eixo vem inteiro de um dos nós,
   sorteado pelo peso, com uma folga.
   - Antes era a média, que puxava tudo para o neutro: o noise com o
     maracatu dava dança +0,08.
   - Com o cruzamento, o mesmo par dá dança +0,24 e peso +0,19.
6. **A mutação:** com chance de 0,35·imprevisibilidade·(1 − repetição),
   uma escolha troca por um valor de qualquer galho.
7. **A memória** (`dados/memoria/influencias.json`): cada chave usada conta
   mais uma, e o muito usado fica raro. É o "mercado de probabilidades" do
   Studio, reescrito.

**A repetição que o conceito pede:** o eixo repetição (o minimalismo, o
drone, a pista) faz a raridade, a polinização e a mutação cederem,
"se a repetição for necessária para exprimir e manifestar uma música que
requer a repetição".

## Na resposta

**Antes de compor** (`applyInfluenceOptions`): o que o autor não escolheu
vem do sorteio (o conjunto, o subestilo, a harmonia, o humor, a escala da
mistura e o ciclo). O rasgo e o ruído puxam o rasgo modular.

**Depois das figuras do estilo** (`applyInfluenceScore`):

- **as presenças:** a voz ausente sai;
- **os eixos de caráter, em qualquer conjunto:**
  - **dança:** acima do meio, a percussão mais acentuada e o baixo e a
    harmonia mais curtos (o groove); abaixo, a percussão rareia e as notas
    se alongam (o introspectivo);
  - **peso:** a força, e o baixo e a harmonia uma oitava abaixo (pesado) ou
    acima (leve).

**Na forma:** tudo vai para as marcas ("influências · …", "caráter: …").

## Testado (`InfluencesTests`)

- **o arquivo:** legível;
- **a herança;**
- **as tendências:** em 200 sorteios do rock, a percussão faltou em 30, a
  tendência e não a regra;
- **a polinização:** 67 em 200;
- **a raridade:** com o rock muito usado, a árvore livre o escolheu 2 vezes
  em 100;
- **a repetição:** o drone (repetição 1,0), mesmo com imprevisibilidade 1,
  não teve novidades;
- **as opções:** o que o autor escolheu fica;
- **o caráter:** a dança encurta o baixo, o introspectivo rareia a
  percussão, o peso desce o baixo uma oitava.

## A escuta

`escutas/2026-10-18-influencias/` traz três respostas ao vocalise, com
LEIA-ME:

- noise com maracatu (e jazz livre polinizado);
- minimalismo com pista;
- jazz livre.

## A força e a exclusão (2026-10-18, a partir da pesquisa)

O autor pediu uma pesquisa sobre como o Suno e outros sistemas de IA
recombinam estilos pelo prompt. Ela está no Atlas de Referência do RASGO,
na entrada "2026-10-18: a recombinação de estilos na IA generativa". Dela
vieram duas ideias, reescritas:

- **`--forca-influencias 0..1`**, o análogo do "Style Influence" do Suno e
  do *guidance* dos modelos abertos. Com força 1, as tendências do arquivo
  entram inteiras; com menos, as escolhas acontecem menos e os eixos voltam
  ao meio, e a escuta da referência pesa mais. Medido: o subestilo do punk
  entra em 86 de 100 sorteios com força 1 e em 20 com força 0,2.
- **`--sem "percussão, baixo"`**, o análogo do "Exclude". A exclusão é do
  autor e vence qualquer tendência: na pista, a percussão de 0,95 sai.
  Sozinho, sem `--influencias`, só exclui; a árvore não sorteia nada.

**Uma lição da pesquisa:** a interpolação de *embeddings* (a média) é o
mesmo achatamento que levava os eixos ao neutro, e que leva a IA à
homogeneização. Por isso o Trioio cruza os eixos em vez de fazer a média.

## Próximo

- ~~A mistura conceitual de cadências~~: feita em 18/10,
  `MAQUINA_DE_HARMONIA.md` §11.
- ~~O mapa das respostas nos eixos~~: feito, na seção abaixo.

## No app (2026-10-18)

No painel **Linguagem ▸**, acima do gesto:

- **"Influências":** os nós separados por vírgula, ou `auto`;
- **"Força das influências":** de 0 (a referência) a 1 (a árvore);
- **"Sem":** os papéis que saem (percussão, baixo, harmonia, contracanto,
  memória, mosaico).

Conferido no instantâneo de 2560×1440.

## O mapa: a busca pelo não ocupado (2026-10-18)

A ideia é a busca pela novidade de J. Lehman e K. O. Stanley ("Abandoning
objectives: Evolution through the search for novelty alone", *Evolutionary
Computation* 19(2), 2011), reescrita para o Trioio:

- **O ponto:** cada resposta com influências vira um ponto no mapa
  (`dados/memoria/mapa_influencias.json`, os 300 mais recentes), feito dos
  eixos (dança, peso, ruído, rasgo), do conjunto e da harmonia.
- **A escolha:** a próxima resposta sorteia cinco candidatos e fica com o
  que cai mais longe do vizinho mais perto já feito (`pickNovelDraw`).
- **A distância:** euclidiana nos eixos, mais 0,5 se o conjunto difere e
  0,5 se a harmonia difere.
- **A repetição:** se o conceito a pede (repetição ≥ 0,6), a busca cede.

**Conferido** com quatro respostas seguidas com `--influencias auto`:
ruído experimental, post-rock, música brasileira e erudita. Cada uma caiu a
mais de 1,0 da vizinha mais perto, e a do post-rock, cujo conceito pede
repetição, não forçou a novidade.

**Testado** (`InfluencesTests`): entre um candidato perto do já feito e um
longe, fica o longe; com a repetição pedida, fica o primeiro; sem passado,
o primeiro.
- **As poéticas do autor** no arquivo.
- **No app:** um campo de influências e a árvore navegável.
- **Mais traços:** o timbre de cada voz, o andamento relativo, a forma, a
  produção.

## O timbre e a densidade (2026-10-18, depois)

A escuta do autor achou a intenção e o gesto "mais diferentes que
nítidos". Por isso as influências ganharam dois traços que se ouvem mais:

- **`timbre:papel:família`:** a família de timbre de um papel (motivo,
  baixo, contracanto, memória, mosaico, harmonia), escolhida pela
  tendência. Por exemplo, `timbre:motivo:sax 0.5` no jazz e
  `timbre:harmonia:corda cantada 0.3` no drone. O arquivo traz 40 desses
  traços.
- **O eixo `densidade`:** abaixo do meio, as vozes de enfeite e a harmonia
  rareiam; acima, as notas longas do motivo, do contracanto e da harmonia se
  partem em ataques repetidos.

**Testado** (`InfluencesTests`): o jazz põe o motivo no sax em 56 de 100
sorteios; a voz troca de timbre; a densidade parte e rareia.

**A escuta:** `escutas/2026-10-18-influencias-v2/`, as mesmas combinações
da primeira leva e mais o drone, com LEIA-ME.

## A forma e as influências cruzadas (2026-10-18, noite)

- **Os eixos seguem a forma:** dança, peso e densidade não são mais
  constantes. Sobem até +0,2 no clímax da curva da peça (a gramática, o
  gesto ou a curva do modo) e descem nos vales. Na Freytag, o baixo fica
  mais curto e a textura mais densa no clímax que na exposição.
- **A virada traz o galho distante:** com a gramática ki-shō-ten-ketsu, a
  virada (de 50 a 75 % da peça) usa os timbres (a família de maior
  tendência por papel) e os eixos do nó polinizado, ou do segundo nó
  pedido. Isso só vale nesse trecho. A virada deixa de ser só um
  deslocamento e vira a entrada de outra influência.
- **A gramática pode vir da árvore:** o traço `forma:` já escolhe a
  gramática quando o autor não escolhe.

**Testado** (`InfluencesTests`): na Freytag, o baixo é mais curto no clímax;
na virada, só o trecho de 50 a 75 % ganha o timbre do galho distante.

## A escrita livre no campo Influências (2026-10-18)

O autor escreveu "rock 60" no campo Influências, e o nome não estava na
árvore. Agora, quando o texto não é um nó, o Trioio:

1. procura as palavras na árvore, com a mesma tolerância das Palavras
   (acento, caixa, erro de digitação, "post rock");
2. lê uma **década**, se houver ("60", "60s", "anos 60", "1970"). Entre os
   nós do ramo pedido (ou de toda a árvore, se não houver palavra), fica com
   os que citam essa década no contexto histórico, e o sorteio escolhe um.

A forma mostra o caminho:

```text
"rock 60" → rock/psicodélico (os anos 1960 no contexto: 5 nó(s))
```

É o contexto histórico de cada nó (ver "contexto:" no arquivo) que vira
parâmetro de escolha, como pede o princípio da árvore: contextos sociais,
históricos, tecnológicos e de geração.

## Os nós pedidos pelo uso (2026-10-18)

O autor escreveu "tango improvisado, coral a capela" e "coco de roda", mas a
árvore não tinha esses ramos. Foram acrescentados pelo gerador
(`tools/gerar_arquivo_de_influencias.py`), cada um com o seu conceito, o
contexto e a fonte:

| nó | conceito | fonte |
| --- | --- | --- |
| `vocal`, `vocal/coral` | muitas vozes como uma: a respiração comum e o acorde cantado | J. Potter (org.), *The Cambridge Companion to Singing*, 2000 |
| `mundo/tango` | o abraço e a pausa: o marcato que arrasta e a frase que se suspende | S. Collier et al., *¡Tango!*, 1995 |
| `brasil/nordeste/coco` | a roda que responde: o refrão do coro à embolada do tirador, a palma e o pé | M. de Andrade, *Os cocos*, 1984 |

O coral é uma influência que **atravessa** os conjuntos. Ele não troca o
tango por um coro *a cappella*: dá a tendência de voz e coro aos papéis
(a harmonia em coro, a memória e o contracanto em voz), dentro do conjunto
que estiver em jogo. Refeito com as palavras do autor: tango com a harmonia
em coro.

## Os outros nomes e as influências do autor (2026-10-18)

**`também:`** é uma linha opcional do nó, com outros nomes separados por
vírgula, como se escrevem nas Palavras: `coco de roda, samba de coco`,
`tango nuevo`, `bossa nova`. Um nome de mais de uma palavra chama o nó
inteiro, e as suas palavras não seguem avulsas. Por exemplo, "roda" não vai
mais para a rede como *wheel* quando faz parte de "coco de roda".

**`<dados>/influencias_do_autor.txt`** é o arquivo do autor. O autor
lembrou: "se não é possível escrever nome de artistas e bandas talvez seja
necessário explicar isso no tutorial".

- O Trioio não inventa a poética de artistas e bandas. O autor a escreve
  nesse arquivo, no mesmo formato, com a sua leitura e as suas fontes.
- O Trioio cria o arquivo com um exemplo comentado na primeira resposta
  que usa Palavras ou Influências. Depois disso, nunca o sobrescreve.
- Um nó do autor com o mesmo caminho de um nó do Trioio substitui o nó do
  Trioio.
- O arquivo fica na pasta de dados da pessoa, nunca no repositório.

O tutorial explica isso no capítulo 10, nas quatro línguas.
