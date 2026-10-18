# TRIOIO — a escuta do autor como memória do instrumento

**Estado:** implementado em 2026-10-08 (`src/core/Listening.*`,
`src/library/ListeningStore.*`, `tests/ListeningTests.cpp`).

## Por quê

Até aqui o Trioio não aprendia nada com o que o autor ouvia. Cada versão
nascia só da referência, do modo e da seed. O autor descreveu o processo
assim: "avance em mais desenvolvimento e iremos acertando com o tempo". Para
que esse acerto fique no instrumento, e não só na conversa, a escuta
precisa ser registrada e pesar nas próximas respostas.

O `TRIOIO_CEREBRO.md §10` dá os limites:

- adaptação local, sem treinar uma rede;
- reforçar ou enfraquecer estratégias a partir das escolhas do performer;
- esquecer aos poucos para evitar cristalização;
- "alterar tendências, não aprisionar o instrumento em um perfil de gosto".

## Como se usa

```sh
# 1. gerar uma resposta (a derivação der-… aparece na saída)
trioio_biblioteca responder <dados> <ref> --modo seguir --seed 1

# 2. ouvir e julgar
trioio_biblioteca escutar <dados> der-… aceitar     --nota "o corpo espectral e a sala"
trioio_biblioteca escutar <dados> der-… rejeitar    --nota "o chip ficou duro demais"
trioio_biblioteca escutar <dados> der-… transformar --nota "boa matéria, forma fraca"

# 3. ver o que o instrumento tirou disso
trioio_biblioteca escuta <dados>

# as próximas respostas já levam a escuta em conta; para ouvir sem ela:
trioio_biblioteca responder <dados> <ref> --modo seguir --escuta nao
```

## O que é guardado (`<dados>/escuta.json`, esquema `trioio.escuta/1`)

Cada julgamento guarda:

- a derivação julgada e o veredito;
- a nota do autor, livre;
- a data;
- **o que aquela resposta usou**, lido do `decisoes.json` da sessão:
  - as famílias de timbre, com quantas notas cada uma tocou;
  - as escolhas do Terceiro Olho, lidas das marcas da forma;
  - a sala.

O arquivo é texto legível e é do autor. Apagar uma entrada desfaz o seu
efeito.

## Como o julgamento vira tendência (`tendenciesFrom`)

- **Placar:** aceitar soma +1, transformar +0,3 ("gostei da matéria, não do
  resultado") e rejeitar −1.
- **O que entra no placar:**
  - a família de timbre, na proporção das notas que tocou naquela
    resposta;
  - cada escolha do Terceiro Olho e a sala, inteiras.
- **Esquecimento:** o julgamento mais novo pesa 1, o anterior 0,85, o
  outro 0,72, e assim por diante.
- **Tendência** = exp(1,2 × placar), **limitada entre 0,5 e 2**. Ela puxa ou
  afasta, mas nunca proíbe nem impõe: uma família rejeitada várias vezes
  ainda pode aparecer, só que com metade da chance.

As tendências multiplicam os pesos de três sorteios:

1. a família de timbre de cada seção (`motifTimbreFor`);
2. a sala da peça;
3. as 17 escolhas do Terceiro Olho, junto com a leitura do momento.

**Sem nenhum julgamento, tudo vale 1, e a composição é idêntica** à de antes
para a mesma seed. O teste confirma isso nota por nota.

O `decisoes.json` de cada resposta registra quantos julgamentos pesaram
(`listeningJudgements`), para a resposta poder ser refeita.

## Validação

`tests/ListeningTests.cpp`:

- **neutro:** sem escuta, tudo vale 1;
- **direção:** aceitar puxa (×2), rejeitar afasta (×0,5), transformar puxa
  pouco (×1,43), e os limites valem mesmo depois de 30 julgamentos iguais;
- **esquecimento:** uma rejeição antiga pesa ×0,76, e uma nova pesa ×0,5;
- **leitura das marcas:** 52 de 52 intervenções do olho e 12 de 12 salas;
- **efeito:** com o espectral aceito, as notas espectrais dobram em 24 seeds
  (de 156 para 312), e a sala aceita aparece mais;
- **identidade:** sem escuta, a mesma seed dá a mesma partitura.

## Limites e próximos passos

- A escuta pesa sobre **o que foi usado**, não sobre **como** foi usado. Se o
  autor aceita uma versão pelo desenvolvimento do motivo, as famílias
  daquela versão também ganham peso. A nota livre guarda a intenção, mas
  ainda não é interpretada.
- O julgamento por trecho ("o minuto 1 sim, o final não") está descrito
  abaixo.
- Na interface proposta, aceitar, rejeitar e transformar viram botões na
  faixa "sua escuta".

## Julgamento por trecho

**Estado:** implementado em 2026-10-10:

- no núcleo, `describeSpan`, `parseSpan`, `sectionSpans`, `responseEnd` e
  a idade por ato em `tendenciesFrom`;
- em `ListeningStore` e em `escutar --trecho|--secao`;
- na interface (2026-10-12, `docs/design.md §7.1`): arrastar sobre a faixa
  da resposta marca um trecho; clicar num bloco escolhe a seção; clicar fora
  volta à resposta inteira. Os botões aceitar/transformar/rejeitar julgam o
  que estiver marcado — a interface manda `m:ss-m:ss` (segundos inteiros,
  sem depender do locale), `m:ss-fim` quando o trecho chega ao fim, ou o
  número da seção.

Validação: os seis testes em `tests/ListeningTests.cpp`, a suíte 28/28 e
uma prova de ponta a ponta numa resposta real (ver "Caso real").

### Por quê

Uma resposta raramente é toda boa ou toda ruim. Julgar só a resposta
inteira obriga o autor a escolher entre perder o que funcionou e reforçar
o que não funcionou.

### Uso

```sh
trioio_biblioteca escutar <dados> der-… aceitar  --trecho 0:00-1:00 --nota "o ālāp"
trioio_biblioteca escutar <dados> der-… rejeitar --trecho 1:41-fim  --nota "o retorno pesou"
trioio_biblioteca escutar <dados> der-… aceitar  --secao 3          # a 3.ª seção
trioio_biblioteca escutar <dados> der-… rejeitar --secao A          # toda seção A (forma por repetição)
```

- O `--trecho` aceita `m:ss-m:ss`, segundos (`12.5-40`) e `fim`.
- O `--secao` aceita um número ou uma letra.
  - O intervalo vem da própria marca: `seção N (início–fim s): …`.
  - A letra só existe na forma por repetição (`A · …`, `retorno de A`).
    Como pode se repetir, uma letra julga todas as seções com ela no
    mesmo ato.
- Sem nenhum dos dois, o julgamento vale para a resposta inteira, como
  hoje.
- Vários trechos da mesma resposta podem ser julgados, cada um com o seu
  veredito.

### O que o trecho [a, b) conta

Tudo é lido do `decisoes.json`, que já tem o tempo de cada nota e de cada
marca.

- **Timbre:** cada nota conta 1, como no julgamento inteiro. A nota que
  atravessa a fronteira conta pela fração que soa dentro. O trecho
  [0, fim] dá exatamente o julgamento inteiro, e há um teste para isso.
  - A primeira versão pesava pelos segundos soando. Na resposta real, o
    pad espectral sustentado somou 90 s de notas num trecho de 21 s e
    dominou o julgamento.
  - A contagem por notas foi trocada no mesmo dia, antes de qualquer uso.
- **Terceiro Olho:** contam as intervenções cuja marca cai dentro do
  trecho.
  - Uma intervenção anterior ao trecho, cuja consequência ainda soa nele,
    não entra.
  - As marcas ainda não guardam a duração das consequências.
  - Isso já apareceu no caso real (ver "Caso real").
  - Esse limite é aceito por enquanto. A interface vai mostrar qual
    intervenção ficou de fora.
- **Sala:** a sala é da peça inteira. Por isso entra com peso
  (b − a) / D, em que D é o fim da última nota.
  - Rejeitar o último minuto de uma peça de quatro minutos pesa um quarto
    de rejeição na sala.
- **Esquecimento:** os trechos julgados no mesmo ato contam como um só
  julgamento na idade. O ato é a mesma derivação dentro do mesmo minuto.
  - Julgar três trechos de uma vez não faz o primeiro "envelhecer" em
    relação ao terceiro.

### Formato

O esquema passa a ser `trioio.escuta/2`. Cada julgamento ganha estes
campos opcionais:

- `span`: `[a, b]`, em segundos. Em `timbres`, `notes` pode ser
  fracionário;
- `section`: o número ou a letra, quando o trecho veio de `--secao`;
- `roomWeight`: a fração da sala, de 0 a 1.

O `trioio.escuta/1` continua legível: um julgamento sem `span` vale para a
resposta inteira, com `roomWeight` = 1. Um julgamento antigo de resposta
inteira contava timbre por número de notas. Ele continua assim, para não
mudar retroativamente as tendências que o autor já formou.

### Interface (`docs/design.md`)

- Arrastar sobre a faixa da resposta seleciona um trecho, e clicar numa
  seção seleciona a seção.
- A barra de julgamento passa a dizer o que será julgado: "a resposta
  inteira" ou "trecho 1:41–2:18 (seção A′)". Esc volta para a resposta
  inteira.
- Os trechos já julgados ficam marcados sobre a faixa, na cor do veredito.

### Caso real (2026-10-10)

A resposta foi gerada sobre a faixa Suno com `--forma repeticao --seed 1`.
Ficou com as seções A a F, sem repetição clara. O teste foi:

```sh
escutar … rejeitar --secao C
escutar … aceitar  --trecho 0:00-0:20
```

1. A seção C foi resolvida pela marca como 0:20–0:41, com a sala em ×0,15.
2. A marca da seção anuncia o motivo em "chip AY", mas o motivo soou
   espectral. Aos 8,4 s, o Terceiro Olho fez o "corpo da referência", cuja
   consequência é "o corpo da referência vira o timbre".
   - O julgamento atribuiu ao espectral, que é o que de fato soou, e não ao
     que a marca planejava.
   - A intervenção que causou a troca fica fora do trecho. É o limite
     descrito acima, agora visto num caso real.
3. Os erros não gravam nada:
   - seção inexistente;
   - trecho depois do fim;
   - `--trecho` e `--secao` juntos.

### Testes (`tests/ListeningTests.cpp`)

1. **Compatibilidade:** sem trecho, as tendências são idênticas às de
   hoje, tanto no histórico `/1` quanto no `/2`.
2. **Localidade:** aceitar só um trecho espectral não puxa o chip que só
   toca fora dele.
3. **Fronteira:** uma nota que atravessa a borda conta só a fração de
   dentro, e [0, fim] é igual à resposta inteira.
4. **Sala:** a rejeição do último quarto da peça pesa um quarto.
5. **Mesmo ato:** três trechos julgados juntos têm a mesma idade.
6. **Seção:** `--secao 3` resolve o intervalo pela marca. `--secao A`
   julga todas as seções A, inclusive os retornos. Um número ou letra
   inexistente é recusado sem gravar nada.
