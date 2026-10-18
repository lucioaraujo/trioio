# TRIOIO — síntese: famílias de timbre e o inventário RASGO

**Estado:** 2026-10-07. Pedido do autor: "seguir com a família de timbre
espectral, e também ampliar os módulos de síntese, com um bom apanhado de
tudo o que temos codado em termos de síntese em todos os nossos
instrumentos, para ficar bem rico".

## Princípio

Todo som do Trioio é **síntese**, sem amostras e sem instrumento MIDI. O
compositor escreve uma partitura, e o `ScoreSynth` a toca. O MIDI é só uma
exportação. A escuta da referência decide o que tocar. Desde a família
espectral, ela também dá o **corpo** do som.

## As 33 famílias (`Timbre`, `src/core/Composer.h`)

| Família | De onde vem | O que é |
| --- | --- | --- |
| aditivo | Trioio (`ScoreSynth`) | 8 parciais harmônicos, agudos decaem antes |
| sopro | Trioio | parciais suaves + ar filtrado |
| metal | Trioio | parciais de sino (1; 2,76; 5,40; 8,93; 13,34) |
| corda pinçada | Trioio | Karplus-Strong |
| membrana | Trioio | modos de pele, altura que cai no golpe |
| FM | Trioio | 2 operadores, razão 1,41 |
| **espectral** | **Trioio, a partir da referência** | os parciais **medidos** da referência (razões e pesos), ressintetizados |
| tabela de ondas | Rasgo Modular `WAVETABLE` | posição e dobra da tabela sorteadas por nota |
| pulsar | Rasgo Modular `PULSAR` | síntese pulsar (Roads): formante e janela |
| modal | Rasgo Modular `MATTER` | 24 modos golpeados: estrutura de corda a sino |
| guia de onda | Rasgo Modular `STRING` | corda com posição de excitação e saturação no laço |
| tambor | Rasgo Modular `DRUM` | tambor com queda de altura e estalo |
| voz formante | Rasgo Modular `OSC` (serra) → `FORMANT` | vogais, com ditongo ao longo da nota |
| dobra | Rasgo Modular `OSC` (senoide) → `SHAPE` | dobra de onda da costa oeste, que fecha ao longo da nota |
| Gendyn | Rasgo Synth `GendynOscillator` | síntese estocástica dinâmica (Xenakis): pontos em passeio aleatório; a altura vagueia ±25 %, por técnica |
| VOSIM | Rasgo Synth `VosimOscillator` | trens de pulsos sin² com formante própria (Kaegi; Tempelaars) |
| Walsh | Rasgo Synth `WalshOscillator` | soma de funções de Walsh com pesos sorteados: ondas em degraus |
| teremim | Rasgo Synth `ThereminVoice` | senoide que nunca salta: desliza até a nota, com vibrato |
| flauta FM | Rasgo Synth `FmFluteVoice` | FM quase harmônica com o "chiff" do ataque |
| scat | Rasgo Synth `ScatVoice` | pulso glotal em três formantes de vogal real |
| chip AY | Rasgo Synth `AY8910Chip` | ondas quadradas do AY-3-8910: a nota e a oitava levemente desafinada |
| galo | Rasgo Synth `GaloVoice` | corpo modal (6 modos esticados) soprado, com o contorno do chamado |
| ressonador soprado | Rasgo Modular `RESONATOR`, soprado pelo ruído rosa do Antitotem (`NoisePalette`) | banco de 8 a 20 modos que soa enquanto é soprado, com golpe no ataque |
| CMOS | Antitotem `CmosVoice` | 5 osciladores CMOS (núcleos Schmitt, formas de função ou deriva sem buffer), batimentos e o PLL 4046 caçando a nota uma oitava acima |
| coro | Rasgo Synth `ChoirEnsemble` | 4 vozes de vogal (scat) num acorde em volta da nota (maior 7, menor 7, sus ou quartal), cada uma entrando com atraso próprio |
| Rungler | Rasgo Synth `Rungler` + `Oscillator` | registrador de deslocamento caótico (Rob Hordijk) que faz a altura saltar em degraus em volta da nota |
| operador FM | Rasgo Modular `OPERATOR` | FM de 4 operadores: algoritmo (8) e razões sorteados por nota, índice pela dinâmica |
| LPG | Rasgo Modular `OSC` → `LPG` | serra ou triângulo num LPG golpeado: o "bongô" de vactrol (Buchla 292), brilho e volume caindo juntos |
| nuvem granular | Rasgo Synth `GranularCloud` | o tom da nota alimenta uma nuvem de grãos que relê o passado recente com desvio de altura |
| objeto espectral | Rasgo Synth `SpectralObject` | parciais admitidos por um crivo (Xenakis), cada um pulsando num período proporcional ao seu índice: a mesma proporção vira altura, ritmo e timbre |
| ressonador Bioma | Aquorbium `BiomaModalResonator` | 4 modos com material, dureza e posição do golpe |
| corda Bioma | Aquorbium `BiomaPhysicalString` | corda física (atraso fracionário, perda, posição do pluck) |
| striker granular | Aquorbium `BiomaGranularStriker` | golpe de até 12 partículas ressonantes em volta da nota; notas longas são golpeadas de novo a cada 0,35 s |

### As salas

A sala da peça inteira é escolhida pelo caráter da referência, com sorteio
próprio (não desloca os outros sorteios), e anotada na forma (`sala: …`):

- **sala do Trioio:** a rede de 4 linhas de atraso de sempre;
- **HALL** (Rasgo Modular): cauda longa e modulada, preferida em
  referências rarefeitas e tonais;
- **SPACE** (Rasgo Modular): ecos multitap com difusão, preferido em
  referências densas ou ruidosas.

HALL e SPACE recebem o meio do estéreo e devolvem o molhado somado. O SPACE
é mono, então o retorno vai aos dois lados com 11 ms de defasagem.

Os 8 objetos sonoros do Terceiro Olho seguem em `ScoreSynth`: nuvem, massa
de glissandos, faixa de ruído, cluster, iteração, drone, congelamento e,
desde 2026-10-07, o **corpo da referência**. Esse último sustenta os
parciais medidos da gravação naquele instante sobre uma fundamental (a nota
mais grave do acorde recente), cada parcial com respiração e fase próprias,
espalhado no estéreo do grave ao agudo. O olho o escolhe quando a
referência tem corpo tonal estável. Se a intervenção fica, a melodia passa
a tocar com o timbre espectral até o fim.

### Família espectral

`spectralProfile()` (`Composer.cpp`) monta o perfil de cada seção da
referência:

- em cada quadro com som, os 4 picos espectrais são divididos pela
  fundamental. A fundamental é o YIN, refinado pelo pico mais próximo (a
  menos de 8 %), porque o YIN erra alguns % quando há parciais inarmônicos;
- as razões entram num histograma (1/8 de razão por casa), com peso pela
  potência do quadro;
- os 10 máximos locais mais fortes são os parciais, e a fundamental é
  garantida. A planura média dá a parte de ruído.

A nota toca esses parciais sobre a sua própria fundamental. Cada parcial
decai mais cedo quanto mais agudo e tem um batimento lento próprio, e o
ruído da referência entra como ar filtrado na região do parcial mais forte.

- **O corpo se move (2026-10-07):** o perfil não é mais um por seção, e
  sim um por janela de 3 s, a cada 1,5 s, ao longo de toda a referência.
  Cada nota parte do corpo do seu início e se transforma no corpo do seu
  fim, cruzando os dois conjuntos de parciais. O timbre acompanha a
  referência. Teste: uma nota que atravessa os corpos 1; 2,76 e 1; 3,5 tem,
  no início, o 2,76× com 93 vezes a energia do 3,5×, e no fim 0,024 vez.
- **Limite:** os quadros guardam 4 picos sem amplitude, então o peso é
  "quanto aparece entre os mais fortes". O movimento é de janela a janela,
  não uma ressíntese quadro a quadro.
- **Teste** (`tests/SynthFamiliesTests.cpp`): uma referência de 200 Hz com
  parciais 1, 2 e 2,76 volta como 1,00, 2,00 e 2,75. A nota espectral em
  Lá 220 tem picos em 220, 440 e 605 Hz.

### Código aproveitado: cópias fixadas

**O Trioio é autônomo** (decisão do autor, 7 out. 2026): nada no build
aponta para outro projeto RASGO. O DSP aproveitado entra como **cópia
fixada** em `third_party/`, com o commit de origem, os checksums, a
licença e os créditos em `PROVENANCE.md`. A cópia passa a ser código do
Trioio e pode ser **adaptada** aos conceitos musicais dele. Cada adaptação
fica registrada no `PROVENANCE.md`. Essa é a regra de todos os
instrumentos RASGO (`GOVERNANCA_E_TRANSVERSALIDADE.md §7.0`).

| Cópia | Origem | Licença |
| --- | --- | --- |
| `third_party/rasgo_modular_dsp/` (17 headers) | Rasgo Modular, commit `fcb711a` (v0.1.6) | AGPL-3.0-or-later |
| `third_party/rasgo_synth_dsp/` (18 headers) | Rasgo Synth, commit `0dc468a` | GPL-3.0, combinada com a AGPL pela seção 13 das duas |
| `third_party/antitotem_dsp/` (2 headers) | Antitotem, commit `eabde9f` | AGPL-3.0-or-later |
| `third_party/aquorbium_dsp/` (7 arquivos: 3 corpos + `Numeric.h`) | Aquorbium, commit `e234a25` (repositório principal) | AGPL-3.0-or-later |

### Famílias do Rasgo Modular

`ModularVoice.cpp` cria a cadeia de módulos reais do Rasgo Modular para
cada nota:

- define `freq` (ou `tone`) e aplica o glissando bloco a bloco;
- manda o golpe (`strike`, `pluck` ou `gate`) nas famílias percussivas;
- move a vogal e a dobra ao longo da nota;
- usa envelope nas sustentadas e deixa as percussivas decaírem sozinhas.

O nível de cada nota é levado ao das famílias próprias.

- **Segundo consumidor:** o Trioio é o **segundo consumidor** desse DSP,
  que é a condição do inventário global para promovê-lo a código comum.
  Promover de fato ainda exige contrato e decisão.

### Famílias do Rasgo Synth

`RasgoSynthVoice.cpp` toca cada voz amostra a amostra, com a frequência da
nota (glissando incluído). As vozes de envelope próprio são flauta, scat e
galo; as outras recebem ataque curto e release. Há um bloqueador de DC,
porque a saída do VOSIM é só positiva. Ajustes de cada família:

- **Gendyn:** a duração média de cada ponto é 1 / (f × pontos);
- **AY:** o tom alterna a cada ciclo, então o chip recebe 2 × a frequência;
- **teremim:** desliza até a nota, com o portamento dele.

**Teste** (`tests/SynthFamiliesTests.cpp`): as 33 famílias soam (entre −18
e −7 dB) e ficam finitas. As de altura definida tocam a nota pedida (Lá
220). As famílias hospedadas vêm de fato do Rasgo Modular e do Rasgo
Synth.

## Inventário: o que mais há de síntese nos instrumentos RASGO

Levantamento só de leitura, de 2026-10-07. Rasgo Synth e Aquorbium são
trabalhados pelo autor com o Codex: nada neles foi editado.

| Instrumento | Licença | Síntese disponível | Situação para o Trioio |
| --- | --- | --- | --- |
| **Rasgo Modular** | AGPL-3.0 | 58 módulos. Fontes ainda não hospedadas: `OPERATOR` (FM), `ADDITIVE` (aditivo com stretch, odd e comb), `SPECTRA`, `RESONATOR` (banco de parciais excitável), `CHAOS`, `NOISE`, `MEMORY` (granular), `PLANAR`, `SWIRL`. Processadores: `FILTER`, `WASP`, `LPG`, `SHIFTER`, `CRUSH`, `VOCODER`, `HALL`, `SPACE` | **em uso** (7 famílias, cópia fixada). Próximos candidatos: `RESONATOR` com ruído (corpo soprado), `OPERATOR`, `LPG` na saída das percussivas, `HALL`/`SPACE` no lugar da sala única |
| **Rasgo Synth** | GPL-3.0 (combinável com AGPL) | cerca de 50 vozes e processadores em `rasgo-synth-core/src/dsp/`: `GendynOscillator` (Xenakis), `VosimOscillator`, `WalshOscillator`, `ThereminVoice`, `ChoirEnsemble`, `ScatVoice`, `FmFluteVoice`, `AtariPunkConsole`, `AY8910Chip`, `Rungler`, `GranularCloud`, `SpectralGhost`, `SpectralObject`, `ModalResonator`, `MorphOscillator`, `HardSyncVoice`, `AcidBasslineVoice`, `DrumVoices`, filtros (`MoogFilter`, `PolivoksFilter`, `SallenKeyFilter`) e saturadores | **em uso** (cópia fixada do commit `0dc468a`, 7 out. 2026, com o Codex pausado e acesso liberado pelo autor): começou com 8 famílias e hoje são 13 vozes do Rasgo Synth entre os 33 timbres do Trioio (Gendyn, VOSIM, Walsh, theremin, flauta FM, scat, chip AY, galo, coro, rungler, nuvem granular, objeto espectral e o filtro multibanda; conferido em 2026-10-14). Candidatos que ainda não vieram: `AtariPunkConsole`, `HardSyncVoice`, `MorphOscillator`, `AcidBasslineVoice`; filtros `MoogFilter` e `PolivoksFilter` como timbre das vozes. O Rasgo Synth está fora do alcance desta sessão (o autor trabalha nele com o Codex): uma cópia nova pede liberação do autor |
| **Aquorbium** | AGPL-3.0-or-later (`AQUORBIUM/LICENSE-AGPL-3.0.txt`) | ressonador modal, corda física, striker e captura granular, `HarmonicMorphOscillator`, `SynthFMVoice`, `SchoolSpectralBank`, `BiomaPulsar`, `EcologicalNoiseBed` | **candidato, mesma licença.** Mesma situação do Rasgo Synth: é área do autor com o Codex e muda o tempo todo; usar só com uma versão fixada. *Correção de 2026-10-07: a primeira versão deste inventário disse "sem LICENSE" porque só procurou o nome `LICENSE`; o arquivo existe desde o primeiro commit do núcleo, e o texto é o mesmo AGPL-3.0 dos outros instrumentos (difere só em `http`/`https` nos links da FSF).* |
| **Antitotem** | AGPL-3.0 | `CmosVoice` (vozes CMOS), `MaterialFilter`, `NoiseFields` | candidato: o caráter CMOS e os campos de ruído não existem em outro lugar |
| **Navalha 2** | GPL-3.0 | fatiamento e montagem de áudio, masterização (`LookaheadLimiter`, `MasteringProcessor`) | **não é síntese.** Interessa para a escuta e a mixagem final, não para as famílias |

## O barramento de master (2026-10-14)

`src/core/Master.*`, `ResponseRequest::master` (padrão ligado),
`responder --master nao`.

A auditoria (`docs/AUDITORIA_2026-10-14.md` §5–6) apontou o que faltava entre
a soma das vozes e o WAV. A resposta era normalizada pelo pico da amostra a
−1 dBFS: um único golpe decidia o volume de tudo, e os picos entre amostras
não eram vistos. A cadeia:

1. **Passa-altas de 30 Hz** (Butterworth 2.ª ordem): tira o grave inaudível
   que só gasta margem.
2. **Cola:**
   - 2:1, joelho de 6 dB, ataque de 20 ms, soltura de 250 ms, ligada nos
     dois canais;
   - o limiar é **relativo à peça**: 3 dB acima da loudness dela. Com um
     limiar fixo (−20 dBFS), a primeira versão comprimia 5,5 dB uma resposta
     que já estava em −16 LUFS; com o limiar relativo, ficam 3,4 dB, só nos
     trechos fortes.
3. **Alvo de −16 LUFS:**
   - pela ITU-R BS.1770-4: ponderação K com os coeficientes da norma a
     48 kHz, blocos de 400 ms, portas de −70 LUFS e −10 LU;
   - o ganho fica limitado a +18 dB.
4. **Limitador de pico verdadeiro de −1 dBTP:**
   - o pico é medido em 4× sobreamostragem (sinc com janela de Blackman);
   - antecipação de 5 ms, rampa sem degrau, soltura de 80 ms, duas
     passadas;
   - se o limitador cortar os picos, a peça cai abaixo do alvo: o master
     mede de novo e completa o ganho, até 3 vezes, **sem deixar o limitador
     cortar mais de 8 dB**. A dinâmica vale mais que o alvo.

**A dinâmica é uma curva de ganho só, tirada da mistura.** Aplicada também
aos stems das linhas, junto com o passa-altas, que é linear, a soma dos
stems continua sendo a resposta: diferença de 3,6e-7 no `ResponseTests` §7.

**O relatório** vai para `decisoes.json` → `master` e para a saída do
`responder`: loudness antes e depois, pico verdadeiro, redução máxima da cola
e do limitador, ganho.

**Medido (Recording_187):**

| resposta | antes | depois |
|---|---|---|
| seguir, seed 3 | −15,9 LUFS, −1,0 dBTP | −16,0 LUFS, −2,7 dBTP, cola até 3,4 dB |
| 3 linhas | −16,9 LUFS, −1,0 dBTP | −16,0 LUFS, −1,2 dBTP, cola até 3,5 dB |

**Testes** (`tests/MasterTests.cpp`):

- 997 Hz a −20 dBFS dá −20,00 LUFS (a norma);
- um canal só dá −23,01;
- a peça de golpes moderados vai de −24,8 a −16,2 LUFS, com −1,1 dBTP;
- golpes acima do zero digital ficam sob o teto, com o limitador ≤ 8 dB;
- a soma dos stems é preservada;
- o silêncio passa intacto.

**Falta:** o EQ por família (cada timbre no seu lugar do espectro), e a
escuta do autor do A/B. O A/B está na raiz de teste: `sessao-20261013231251`
é sem master; `sessao-20261013231414` é com master.

## Próximos passos

1. **Ouvir a v7** (`~/Music/TRIOIO_respostas/v7-22-familias/`): a mesma
   gravação e as mesmas configurações das versões anteriores, agora com 22
   famílias.
2. Hospedar `RESONATOR` (ressonância soprada por ruído), `OPERATOR` e o
   `LPG` como saída das percussivas. Trocar a sala única por `HALL`/`SPACE`
   do Rasgo Modular.
3. Mais vozes do Rasgo Synth por nova cópia: coro, Rungler, console Atari,
   objeto espectral e nuvem granular. Do Aquorbium (AGPL-3.0, por cópia
   fixada, quando o autor liberar): o ressonador modal, a corda física e o
   striker granular.
4. **Família espectral, próximo passo:** análise e ressíntese quadro a
   quadro, com amplitudes dos picos, para o corpo se mover como na
   referência em vez de ser a média da seção.
