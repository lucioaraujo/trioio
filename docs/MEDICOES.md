# Medições: tempo, CPU e memória por perfil

```
tools/medir_perfis.sh <dados> <ref> [saida.tsv]
```

O script mede uma resposta por perfil, uma de cada vez, com `nice` e
`/usr/bin/time`. Antes de cada uma, confere que há pelo menos 5 GB livres em
`/`, e no fim manda as sessões para a lixeira. Para ver as etapas do render,
use `TRIOIO_TEMPOS=1 trioio_biblioteca responder …`: a síntese, a mistura e a
sala, e cada barramento de voz com o seu tempo e o número de notas.

## 2026-10-18: vocalise (82 s), semente 5

A máquina: 4 núcleos e 23 GB.

**A latência:** o Trioio responde offline. "Latência" aqui é o tempo do
pedido até a resposta pronta (WAV, MIDI, decisões), com a análise em cache.

| perfil | antes | depois | memória máx. |
|---|---|---|---|
| motor de energia (sem partitura) | 10,1 s | 10,2 s | 249 MB |
| partitura (o padrão) | 37,0 s | **22,3 s** | 257 MB |
| conjunto jazz, `--rasgo 0` | 126,5 s | **22,0 s** | 294 MB |
| jogo com critérios no som | 46,7 s | **19,6 s** | 306 MB |
| stems | 39,1 s | **23,2 s** | 502 MB |
| linhas (3) | 102,9 s | **60,3 s** | 430 MB |

**A CPU:** a resposta usa entre 99 % e 112 % de um núcleo. A síntese roda em
paralelo, mas a escuta, a composição e a mistura não.

## O que mudou (e por que o som é o mesmo)

**1. O piano.** O modelo calculava, por amostra, por parcial e por corda, um
`pow` (a curva de altura), dois `exp` (os envelopes) e um `sin`. No jazz, a
voz de harmonia (o piano) levava 98,8 de 103,5 s de síntese.

- **A mudança:**
  - a curva de altura é calculada uma vez por amostra;
  - os envelopes, por recorrência;
  - o seno, por rotação complexa quando a nota não tem curva de altura.
- **O som:** o resíduo diante do antigo ficou 153 dB abaixo do sinal; na
  peça inteira, 103 dB.

**2. A palheta livre (sanfona, bandoneón) e os metais (trompete, trombone,
naipe).** Eram um `sin` e um `pow` por harmônico e por amostra. Os
harmônicos são múltiplos exatos da fase da palheta, então o
sin(kθ) sai da recorrência de Chebyshev:

sin(kθ) = 2·cos θ·sin((k−1)θ) − sin((k−2)θ)

Fica um seno e um cosseno por palheta, e E^(1+0,45(k−1)) vira um produto
acumulado.

- **O tempo por nota:** de 0,38 s para 0,076 s (sanfona) e de 0,39 s para
  0,10 s (naipe de metais).
- **O som:** o resíduo ficou 148 dB ou mais abaixo do sinal, comparado com
  a versão anterior compilada à parte; idêntico em dois dos seis timbres.

**3. Os números desnormalizados.** As caudas dos filtros IIR no silêncio
decaem até ~1e-38, e a CPU x86 processa esses números até 100 vezes mais
devagar.

- **A mudança:** `ScopedNoDenormals` (`src/core/Denormals.h`) liga FTZ e
  DAZ no render, em cada thread de trabalho e na resposta inteira.
- **O tempo:** a mistura de uma resposta caiu de 12,9 s para 1,1 s.
- **O som:** o WAV de 24 bits saiu idêntico, bit a bit, ao anterior.

**4. Os barramentos em paralelo.** A síntese das 8 vozes roda em lotes de
até 3 threads (núcleos − 1; `TRIOIO_THREADS=1` desliga), com no máximo um
lote na memória. A mistura segue na ordem das vozes, então o resultado é o
mesmo do sequencial, amostra a amostra (`ProductionTests`). Sozinho, ganhou
pouco (3 a 8 %), porque um barramento dominava; agora conta mais.

## As threads (Marco 4)

| o quê | onde roda |
|---|---|
| a interface | a thread de mensagens do JUCE |
| a análise, a composição, a síntese de uma resposta | uma thread de trabalho por pedido (`juce::Thread::launch`), fora da interface |
| a síntese dos barramentos | até 3 threads, dentro do render |
| a leitura do disco na reprodução | `TimeSliceThread` "Trioio: leitura", com leitura antecipada de ~1,4 s (desde 2026-10-18; antes o callback lia o disco) |
| o callback de áudio | só lê dos buffers: sem I/O, sem alocação, sem trava |

A escuta ao vivo, quando vier, usa a memória curta (`ShortMemory.h`), que
recebe os quadros sem alocar.

## O que ainda pesa

- **As linhas (60 s):** três composições e três renders, mais os filtros de
  banda. Daria para compor as linhas em paralelo, mas a memória sobe; fica
  para quando for preciso.
- **O padrão (22 s):** a síntese ainda leva ~9 s, dominada pela voz de
  harmonia, que tem 256 notas.
