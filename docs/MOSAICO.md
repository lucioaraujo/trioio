# TRIOIO — memória pesquisável por timbre (mosaico)

**Estado:** implementado em 2026-10-08 (`src/core/Mosaic.*`,
`tests/MosaicTests.cpp`).

## De onde vem

O Atlas de Referência RASGO, na §62 (CataRT / MuBu, IRCAM), diz que "a
memória pode ser pesquisável": a síntese concatenativa por corpus guarda
pedaços de som com descritores e os busca pelo que eles **soam**, não pela
ordem em que vieram.

Referências públicas, sem código delas:

- D. Schwarz, "Corpus-based concatenative synthesis", *IEEE Signal
  Processing Magazine* 24(2), 2007;
- D. Schwarz et al., "Real-time corpus-based concatenative synthesis with
  CataRT", DAFx 2006.

O Trioio passa a ter duas memórias da referência:

- **oráculo** (`docs/MEMORIA.md`, §63): lembra pela **ordem**, o que veio
  depois do quê;
- **mosaico**: lembra pelo **som**, o que soa como o momento pede.

## Como funciona

1. **O acervo:** cada ataque da referência vira uma unidade, do ataque até
   o seguinte (entre 0,08 s e 1,2 s). A unidade guarda:
   - energia, brilho e ruído, normalizados pela extensão do próprio
     acervo;
   - altura (YIN; sem altura definida, o pico espectral mais forte);
   - duração.

   Unidades sem som ou sem altura ficam de fora.
2. **O alvo:** a cada passo da resposta, os mesmos descritores da
   referência naquele instante (janela de ±0,75 s), transformados pelo
   modo:
   - **seguir:** como estão;
   - **contrapor:** energia e ruído opostos;
   - **transfigurar:** brilho invertido;
   - **interromper:** como estão, mas em fragmentos (meia duração, um
     respiro de 0,6 s a cada três unidades).

   Quando a referência cala, o mosaico também cala.
3. **A escolha:** cada unidade recebe um custo, a soma de duas partes (a
   seleção de unidades de Schwarz):
   - **custo do alvo:** a distância ao alvo, com peso dobrado nos eixos
     que o modo inverte;
   - **custo da emenda:** o salto de altura desde a unidade anterior.

   A mesma unidade não se repete em seguida. A escolha é sorteada entre as
   *k* mais baratas, e *k* vai de 1 a 5 com a imprevisibilidade.
4. **O corpo de origem:** cada nota guarda de onde veio (`Note::source`).
   A família espectral toca com o corpo **do instante de origem**, não com
   o do instante em que soa: o trecho do minuto 1 que volta no minuto 3
   traz o timbre do minuto 1. Esse é o gesto próprio da concatenação. A
   memória do oráculo passou a usar o mesmo mecanismo.

**Quanto entra:**

- 1 passagem de 5 a 10 s; 2 passagens com imprevisibilidade acima de 0,5;
- nenhuma com imprevisibilidade 0, e a composição fica idêntica;
- `--mosaico nao` desliga o mosaico.

As passagens evitam as do oráculo. É a voz 5 (canal 6 do MIDI), no lado
oposto do panorama. O manifesto de decisões grava `source` em cada nota de
memória.

A forma registra uma linha por passagem, por exemplo: "memória (mosaico):
32 unidades da referência escolhidas pelo som · alvo: o momento da
referência · vindas em média de 22 s de distância".

**Regras do autor:** proibida a família espectral, o mosaico usa o aditivo
ou a primeira família permitida. O âmbito vale para ele, e o Terceiro Olho
pode transformá-lo como transforma as outras vozes.

## Validação

`tests/MosaicTests.cpp` usa uma referência de 60 s em trechos alternados de
10 s: claros, fortes e ruidosos (dente de serra com chiado) contra escuros,
fracos e puros (seno uma oitava abaixo). Com 8 seeds e imprevisibilidade
0,8, longe das fronteiras, eis a fração de unidades que vêm do mesmo tipo
de trecho do momento:

| Modo | Mesmo tipo de trecho |
| --- | --- |
| seguir | **174 de 174** |
| contrapor | **1 de 137** |
| transfigurar | **15 de 117** (só o brilho é invertido; energia e ruído ainda puxam para o mesmo trecho) |

O teste confere também que:

- toda nota é um ataque real da referência;
- nenhuma cai nas passagens do oráculo;
- todas usam a família espectral;
- com `proibido timbre espectral`, nenhuma usa;
- sem imprevisibilidade ou com o mosaico desligado, não há notas.

Antes do peso dobrado, "transfigurar" escolhia o mesmo tipo de trecho em
134 de 139 unidades: num acervo em que energia, brilho e ruído andam
juntos, o eixo invertido perdia para os outros dois.

**Na gravação real** (v14, `/media/luc/4tb_21042021/TRIOIO_respostas/v14-mosaico/` (apagada em 2026-10-16: arquivo de teste)):

- de 12 a 41 unidades por passagem;
- as unidades vêm, em média, de 11 a 36 s de distância do instante em que
  soam;
- pico de −1,1 dBFS, RMS de −18 a −21 dBFS;
- o render 5 é o mesmo seed do 1 sem o mosaico, para comparação direta.

## Limites e próximos passos

- **Unidades tocadas por síntese, não por amostra.** O CataRT toca o
  próprio trecho gravado. Aqui, a unidade empresta altura, duração, força
  e corpo espectral, e a família espectral a ressintetiza. Tocar a amostra
  exigiria levar o áudio da referência até a síntese.
  - **Decidido pelo autor em 2026-10-14: não.** A resposta é um arquivo
    novo, uma variação criativa feita só por síntese; o som da referência
    nunca entra nela (`docs/COMPOSICAO.md`, "Diálogo com a referência").
- **Três descritores.** Energia, brilho e ruído. Densidade de ataques,
  periodicidade e croma ficaram de fora para manter o espaço legível.
- **Alvo vindo da escuta:** o alvo poderia vir das unidades que o autor
  aceitou (`docs/ESCUTA.md`) em vez do momento da referência.
- **Oráculo × mosaico:** um oráculo cujo símbolo é a classe de timbre (as
  unidades agrupadas) uniria a ordem e o som, como no OMax com descritores.
