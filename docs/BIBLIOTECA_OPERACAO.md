# A Biblioteca na prática (dossiê operacional)

Este é o "como se opera" da Biblioteca do Trioio, como ela existe hoje
(2026-10-18). Os princípios ficam em
[`BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md`](BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md),
os formatos em [`FORMATOS_DE_DADOS.md`](FORMATOS_DE_DADOS.md) e o passo a passo
do app em [`COMO_TOCAR.md`](COMO_TOCAR.md). Os comandos são do
`trioio_biblioteca`; o app faz o mesmo pela interface.

## A pasta de dados

`/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/dados/` é a raiz que o app abre
(`~/.Trioio/Trioio.settings`):

| pasta / arquivo | o que guarda | apagar? |
|---|---|---|
| `indice/referencias/` | o manifesto de cada referência: origem, autorização, identidade técnica | nunca (a proveniência) |
| `indice/derivacoes/` | o manifesto de cada resposta: de onde veio e o que gravou | nunca (o registro) |
| `indice/prompts/` | o `prompts.json` de cada referência com prompt | pelo autor: `prompt --apagar` |
| `escuta.json` | os julgamentos do autor | **nunca** |
| `gabaritos/` | os gabaritos de leitura do autor | **nunca** |
| `cache/analise/` | a análise de cada conteúdo (pelo SHA-256) | sim: é reconstruível (`limpar-cache`) |
| `sessoes/` | cada resposta: WAV, MIDI, stems, `decisoes.json` | as de teste: `limpar` |
| `lixeira/AAAA-MM-DD/` | as sessões tiradas por `limpar` (reversível) | `esvaziar-lixeira`, depois de 7 dias |

O original de cada referência **fica onde estava**. O índice guarda só o
caminho canônico, o tamanho, a data e o SHA-256 do conteúdo.

## O fluxo de uma referência autoral

```text
áudio (+ prompt opcional) (+ metadados)
  → varrer          o arquivo entra PENDENTE; nada é analisado sem autorização
  → autorizar       origem, fundamento, usos (analyze, derive…), quem autorizou
  → analisar        descritores, pulso, acordes, seções → cache (pelo SHA-256)
  → prompt          (opcional) o texto original, a tradução para a intenção
  → responder       escuta + intenção + escolhas → composição → síntese própria
                    → sessoes/<sessão>/ (WAV, MIDI, stems, decisoes.json)
                    → indice/derivacoes/der-….json (de onde veio, o que gravou)
  → escutar         o julgamento do autor (inteiro ou por trecho) → escuta.json
  → a próxima resposta leva a escuta em conta (as tendências, o jogo)
```

**Os comandos, na ordem:**

```
trioio_biblioteca varrer <biblioteca> <dados>
trioio_biblioteca autorizar <dados> <id> --origem O --fundamento F --usos analyze,derive --por NOME
trioio_biblioteca analisar <dados> <id>
trioio_biblioteca prompt <dados> <id> "texto" --fonte autor|gerador|metadados
trioio_biblioteca responder <dados> <id> --modo seguir [--stems sim] [--jogo sim] …
trioio_biblioteca escutar <dados> <der-id> aceitar|rejeitar|transformar [--nota "…"] [--trecho 1:00-1:30 | --secao 3]
```

**O que muda o estado:**

- **um arquivo que muda depois de analisado** fica `changed`: o cache é
  invalidado e a próxima análise o refaz;
- **um arquivo que some** fica `unavailable`, e o histórico é preservado;
- **`revogar`** tira a autorização: nada mais é derivado daquela
  referência, mas o que já foi feito continua registrado;
- **`recusar`** marca a referência como recusada;
- **sem autorização com `derive`,** `responder` recusa.

## As versões e os geradores

As versões de um gerador (várias saídas do mesmo prompt) entram como
referências comuns:

1. cada uma é autorizada;
2. o prompt vai com `--fonte gerador`;
3. o comando `versoes` compara as versões ([`VERSOES.md`](VERSOES.md)).

Recombina-se por autoria, em síntese própria: o áudio não é copiado, e o
serviço gerador não é treinado nem alimentado.

## Memória, aprendizagem e o que o performer pode desfazer

| camada | onde | como desfazer |
|---|---|---|
| cache técnico | `cache/analise/` | `limpar-cache [--tudo sim] --executar sim` (a análise refaz) |
| sessões | `sessoes/` | `limpar --executar sim` (julgadas e as 3 mais recentes ficam; o resto vai para a lixeira, reversível) |
| prompts e intenções | `indice/prompts/` | `prompt --revisar …` (corrige a tradução) e `prompt --apagar` |
| tendências da escuta | calculadas de `escuta.json` a cada resposta, nunca guardadas à parte | desligar por resposta: `--escuta nao` (CLI) ou a caixa "a próxima resposta leva a escuta em conta" (app) |
| memória do jogo | calculada de `escuta.json` e dos `decisoes.json` | `--jogo nao`; as regras apodrecem sozinhas (só as 8 mais recentes ficam) |
| julgamentos | `escuta.json` | não se apagam: é a palavra do autor. Um julgamento novo sobre a mesma resposta vale mais que o antigo |

**Como a aprendizagem funciona:** as tendências nunca viram regra fixa,
porque são recalculadas a partir dos julgamentos a cada resposta. Por isso
não há um "estado aprendido" escondido para invalidar: basta desligar a
escuta numa resposta, ou julgar de novo.

## Os recursos

- **Disco:** tudo fica no disco de 4 TB. Mantenha pelo menos 5 GB livres em
  `/`.
- **O tamanho de uma sessão:** 60 a 250 MB. Os stems somam 9 WAVs de 24
  bits.
- **O tempo** (desde as otimizações de 2026-10-18,
  [`MEDICOES.md`](MEDICOES.md)): cerca de 20 a 25 s por resposta para 82 s
  de música, e 60 s com três linhas.
- **A memória:** 0,6 a 0,9 GB por resposta, 1,6 GB com stems.
