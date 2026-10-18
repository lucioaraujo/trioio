# TRIOIO — as regras do autor (restrições musicais)

**Estado:** implementado em 2026-10-08 (`src/core/Constraints.cpp`,
`tests/ConstraintsTests.cpp`). Exemplo: `exemplos/regras.txt`.

## De onde vem

O Atlas de Referência RASGO, na §55 ("Restrições musicais", das bibliotecas
de *constraints* do OpenMusic), propõe incorporar regras: permitido,
proibido, condicionado, obrigatório, incompatível, exceção. Com isso a
música passa a depender também "do conjunto de regras vigentes", o que
aproxima o instrumento de "composição algorítmica, jogo, negociação,
conflito, exceção".

No Trioio, as regras são o jeito direto de o autor **dirigir** a resposta
sem mexer em código. Elas complementam a escuta (`docs/ESCUTA.md`): a
escuta **puxa** tendências aos poucos, e as regras **mandam**.

## Como se usa

Um arquivo de texto, uma regra por linha, `#` para comentário, acentos
opcionais nas palavras-chave. O `responder` usa `<dados>/regras.txt` se o
arquivo existir, ou outro arquivo com `--regras ARQUIVO`. `--regras nao`
desliga.

```text
proibido timbre <família>       # ex.: chip AY, espectral, corda Bioma
proibido olho <escolha>         # ex.: silêncio súbito, cluster, congelar
proibido sala <sala>            # trioio, HALL ou SPACE
obrigatório olho <escolha>      # acontece ao menos uma vez
máximo salto <semitons>         # maior intervalo melódico do motivo
âmbito <grave> <agudo>          # em MIDI (60 = Dó central)
mínimo memória <passagens>      # passagens da memória do oráculo
exceção <0..1>                  # chance de o olho quebrar uma proibição
condicionado olho <escolha> se <condição>   # só quando a condição vale
incompatível olho <escolha> e <escolha>     # nunca soam juntas
```

**Condições** (lidas pelo Terceiro Olho na leitura do momento, `ThirdEye.cpp`):

| condição | quando vale |
| --- | --- |
| energia alta | energia ≥ 0,6 nos próximos 4 s da referência |
| energia baixa | energia ≤ 0,35 |
| energia subindo / energia caindo | a energia de agora menos a dos 4 s anteriores > +0,08 (subindo) ou < −0,08 (caindo) |
| ruído | planura espectral ≥ 0,3 |
| denso / rarefeito | a partitura tem ≥ 3 notas/s / ≤ 1 nota/s nos próximos 4 s |
| mudança | novidade à frente ≥ 0,6 (a seção vai mudar) |
| campo parado | o campo harmônico de antes ≈ o de depois (≥ 0,7) |
| acorde forte | um acorde que se destacou nos últimos 3 s (≥ 0,5) |

Os nomes são os que aparecem na forma. Vale o nome inteiro, o começo dele
("chip" para "chip AY") ou um trecho ("trioio" para "sala do Trioio"),
desde que só um nome sirva.

**Uma regra que não é entendida impede a resposta** e diz a linha. Nunca sai
uma versão que ignorou uma regra em silêncio.

## Como cada regra age

- **Proibido timbre:** a família pesa 0 no sorteio de cada seção, no baixo,
  na memória e nas trocas de timbre e no eco do Terceiro Olho. A
  consequência "o corpo da referência vira o timbre" não acontece se o
  espectral for proibido.
- **Proibido olho:** a escolha pesa 0 na leitura do momento, salvo
  **exceção**. Com a chance declarada, o olho pode quebrar a proibição, e
  a marca diz "· exceção à regra".
- **Obrigatório olho:** se a escolha não acontecer naturalmente, ela é
  imposta no momento de maior novidade que ainda não tem intervenção por
  perto. A marca diz "leitura: obrigatório pelas regras".
- **Proibido sala:** a sala pesa 0; se todas forem proibidas, fica a do
  Trioio.
- **Máximo salto e âmbito:** aplicados por último, depois do Terceiro Olho,
  que também move alturas. Primeiro dobra por oitavas; se ainda passar do
  salto máximo, encurta o salto até o limite. Valem para motivo, eco e
  memória; o baixo fica livre. A forma diz quantas alturas foram
  ajustadas.
- **Condicionado:** fora da condição, a escolha pesa 0. Uma escolha
  "obrigatória" e condicionada só é imposta num momento em que a condição
  vale. A exceção não se aplica: a condição é o próprio contexto.
- **Incompatível:** enquanto uma das duas soa (e até meio segundo depois),
  a outra pesa 0.
- **Mínimo memória:** a memória do oráculo entra mesmo com pouca
  imprevisibilidade, insiste mais para achar lugar e, em último caso,
  aceita ficar perto de outra passagem.

A forma lista as regras que valeram ("regras do autor: …"), e o
`decisoes.json` as guarda.

**Sem regras, a composição é idêntica à de antes.** Isso foi conferido na
gravação real: a forma ficou igual à da v11.

## Validação

`tests/ConstraintsTests.cpp`, sobre uma referência melódica de 40 s e 24
seeds, com uma regra de cada tipo:

- **leitura:** acentos opcionais, nomes pelo início ou por um trecho, 3
  erros apontados pela linha;
- **resultado:** 0 notas de timbre proibido, 0 intervenções proibidas, 0
  vezes a sala proibida, "congelar" em 24 de 24, 0 saltos acima do máximo,
  0 notas fora do âmbito e ao menos 3 memórias em 24 de 24;
- **exceção:** com chance 1, 11 quebras marcadas;
- **condicionado** (referência com trechos fortes e fracos alternados,
  "cluster se energia alta"): 15 clusters, todos em trecho forte;
- **incompatível** ("drone e nuvem granular"): 9 drones e 7 nuvens, nenhum
  par sobreposto.

**Gravação real** (v12, em
`/media/luc/4tb_21042021/TRIOIO_respostas/v12-regras/` (apagada em 2026-10-16: arquivo de teste), com o
`regras.txt` ao lado):

- só salas HALL e SPACE;
- nenhum chip AY nem silêncio súbito;
- o corpo da referência em todas: em uma delas, imposto pelas regras;
- de 93 a 209 alturas ajustadas para o âmbito e o salto máximo.

## Limites e próximos passos

- **Regras por seção** ("no fim, só o corpo da referência").
- **Salto máximo encurtado** pode tirar a nota do campo harmônico. Uma
  versão melhor procuraria a nota do campo mais próxima dentro do limite.
