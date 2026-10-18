# TRIOIO — cadeia mínima audível (versão 1)

**Estado:** implementada em 2026-10-05. Primeira vez que o TRIOIO faz som.

```
referência autorizada (derive) → escuta (descritores, trechos de 0,5 s)
  → estratégia (trajetória de BrainDecision) → síntese energética
  → WAV + decisoes.json + manifesto da derivação → escuta da própria resposta
```

```sh
trioio_biblioteca responder <dados> <id> --modo seguir|transfigurar|contrapor|interromper [--seed N]
```

Onde estão as partes:

- **Núcleo, sem JUCE:**
  - `src/core/Strategy.h/.cpp`: os quatro modos;
  - `src/core/EnergySynth.h/.cpp`: o motor;
  - `perception::correlation`: a medida da resposta.
- **Biblioteca:** `src/library/Response.h/.cpp`.
- **Testes:**
  - `tests/ChainTests.cpp`: a cadeia no núcleo, mais a prova de que o motor
    não aloca memória;
  - `tests/ResponseTests.cpp`: a cadeia com arquivos, portão e manifesto.

## 1. O que cada modo faz

| Modo | Decisão por trecho |
| --- | --- |
| **seguir** | mesma energia, densidade, brilho e harmonicidade da referência (silêncio vira silêncio) |
| **transfigurar** | mantém energia e densidade; inverte o brilho; tom vira granuloso e ruído vira tom; mais espaço e mutação |
| **contrapor** | energia, densidade e brilho invertidos **dentro da faixa da própria peça**, porque uma peça que só varia entre 0,55 e 0,65 precisa que essa faixa seja invertida para a inversão soar; no silêncio da referência, a resposta fala |
| **interromper** | segue, mas corta nos trechos de maior mudança (um a cada 12 trechos, no mínimo um) e volta com um bloco denso, brilhante e de ataque duro |

O seed muda pequenas variações por trecho e todos os sorteios do motor.
O mesmo seed e o mesmo modo geram o mesmo áudio, bit a bit.

## 2. O motor (`EnergySynth`)

Cada escala da decisão tem um efeito **mensurável**, na mesma escala da
escuta:

| Decisão | Som |
| --- | --- |
| energia | nível RMS de −60 dB (0) a 0 dB (1). Um nivelador lento (cerca de 1 s) mede o conteúdo e o leva ao nível pedido, sem achatar os ataques |
| densidade | eventos por segundo, `4d/(1−d)`, com no máximo 16/s e ±40 % de variação no intervalo; a camada contínua abaixa quando há muitos eventos |
| brilho | centroide de 100 Hz a 8 kHz, em oitavas. Quatro parciais 1/k com f0 = c·H₄/4, deslocados por uma escala pentatônica pequena |
| harmonicidade | mistura entre parciais e ruído filtrado (SVF passa-banda no centroide) |
| retenção de ataque | ataque de 1 ms (1) a 40 ms (0) |
| atividade espacial | largura do pan por evento |

A saída passa por saturação suave (`tanh`). Nos blocos do **interromper**,
os picos chegam perto de 0 dBFS: é saturação suave, não corte duro.

**Defeito achado no caminho:** o intervalo até o próximo evento era
calculado no disparo, com a densidade daquele instante. O primeiro evento,
disparado enquanto o suavizador ainda subia, marcava o seguinte para 15 s
depois, e a resposta ficava quase sem ataques (correlação de densidade de
+0,04). Agora um acumulador de fase faz a taxa valer a cada amostra, e a
correlação de densidade passou a +0,86.

## 3. O que é gravado (só na raiz de dados)

- `sessoes/<sessão>/saidas/<ref>-<modo>-s<seed>.wav`: estéreo, 48 kHz, 24
  bits, com a duração da referência.
- `sessoes/<sessão>/decisoes.json` (`trioio.decisoes/1`): a trajetória de
  decisões, trecho a trecho.
- `indice/derivacoes/der-<sha>.json`: o manifesto da derivação, com a
  origem TRIOIO, a relação `derived_from`, a estratégia, o seed, a relação
  declarada em palavras e o estado **pendente**. Publicar exige
  `publishDerivative` na referência (`canPublishDerivation`).

## 4. A medida da resposta

Depois de sintetizar, o TRIOIO **escuta a própria saída** com os mesmos
descritores e mede a correlação, trecho a trecho, com a referência.
Seguir deve dar valores positivos e contrapor, negativos.

| Material | Modo | Energia | Densidade | Brilho |
| --- | --- | --- | --- | --- |
| referência sintética (teste) | seguir | +0,98 | +0,86 | +0,90 |
| | contrapor | −0,98 | −0,80 | −0,77 |
| | transfigurar | +0,95 | — | −0,82 |
| Dilettante, faixa 7 (3:42, real) | seguir | +0,99 | +0,88 | +0,94 |
| | contrapor | −0,97 | −0,62 | −0,84 |
| | transfigurar | +0,99 | +0,93 | −0,84 |
| | interromper | +0,80 | +0,81 | +0,86 |

Uma resposta de 3:42 leva cerca de 6 s para gerar.

**Limite conhecido:** acima de cerca de 6 ataques por segundo, a escuta
satura, porque a regra de pico de ±3 quadros impõe esse teto. O motor toca
até 16 eventos por segundo, mais rápido do que a escuta consegue contar.

**A correlação prova que a estratégia chega ao som. Não prova que o som é
bom.** Esse juízo é do ouvido do autor (`escuta.json`, Marco 2).

## 5. Próximos passos

- **Escuta do autor:** aceitar, rejeitar ou transformar cada resposta, com
  registro em `escuta.json`.
- **Matéria sonora:** o motor é deliberadamente simples. Os corpos sonoros
  do inventário (`TRIOIO_INVENTARIO_RASGO.md`) são o caminho para ganhar
  timbre, sempre com a origem registrada.
- **Ao vivo:** a mesma cadeia, alimentada pela escuta ao vivo (o
  `FrameAnalyzer` já é compartilhado).
