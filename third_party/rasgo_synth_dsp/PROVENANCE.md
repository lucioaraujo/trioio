# Cópia fixada do DSP do Rasgo Synth

Estes 14 headers vêm do **Rasgo Synth** (`RASGO_SYNTH/rasgo-synth-core/src/dsp/`),
mesma autoria (Lúcio Araújo) e mesmo workspace RASGO. São tocados pelo
Trioio como famílias de timbre (`src/core/RasgoSynthVoice.cpp`,
`docs/SINTESE.md`).

## Por que uma cópia, e não um include path

O Rasgo Synth está em desenvolvimento ativo, com o autor e o Codex. Se o
Trioio apontasse para a pasta viva, cada mudança lá poderia quebrar o build
e mudar o som do Trioio sem aviso. A cópia fixa a versão. Atualizar é uma
decisão explícita: copiar de novo de um commit novo e atualizar esta
página e os checksums.

## Origem

- **Commit:** `0dc468a8fed15bc4628dac3b9d0d431e4db8efb1`, o push do autor
  em 7 out. 2026, "feat(studio): consolida direção musical, álbum e
  lifecycle seguro". Cópia feita com `git show <commit>:<caminho>`.
- **Autorização:** o autor pausou o trabalho com o Codex no Rasgo Synth e
  liberou o acesso aos documentos e ao código do Studio e do Performance
  (7 out. 2026). Nada foi editado em `RASGO_SYNTH/`.
- **Conferido:** os 14 arquivos são idênticos aos da pasta de trabalho no
  momento da cópia.
- **Adaptar é permitido e esperado** (regra RASGO, `GOVERNANCA_E_TRANSVERSALIDADE.md §7.0`):
  a cópia é código do Trioio e pode ser transformada para os conceitos
  musicais dele. Cada adaptação entra na seção "Adaptações" abaixo, para
  ficar claro o que veio da origem (checksums) e o que mudou aqui.

## Licença

O Rasgo Synth é **GPL-3.0** (`RASGO_SYNTH/LICENSE`). O Trioio é
**AGPL-3.0-or-later**. A combinação é permitida pela seção 13 das duas
licenças: estes arquivos seguem sob a GPL-3.0, e a obra combinada é
distribuída nos termos da AGPL-3.0.

## Créditos (de `RASGO_SYNTH/CREDITS.md`)

Todas são implementações originais do Rasgo Synth. Os créditos são das
ideias e técnicas de origem:

- `GendynOscillator`: síntese estocástica dinâmica (GENDYN) de **Iannis
  Xenakis**, reimplementada a partir do algoritmo publicado;
- `VosimOscillator`: VOSIM, do Instituto de Sonologia de Utrecht,
  associado a **Werner Kaegi**, com análise espectral de **Stan
  Tempelaars**. As duas contribuições são distintas;
- `WalshOscillator`: funções de **Joseph L. Walsh**;
- `ModalResonator`: inspirado no Rings, de **Émilie Gillet / Mutable
  Instruments** (banco ressonante original);
- `BreathExciter`: inspirado no Elements, de **Émilie Gillet / Mutable
  Instruments** (excitador original);
- `AY8910Chip`: modelo simplificado e original da arquitetura publicamente
  documentada do **AY-3-8910 da General Instrument** (família Yamaha
  YM2149);
- `Oscillator`: antialias PolyBLEP de **Vesa Välimäki** e **Antti
  Huovilainen**.

## Adaptações

Nenhuma até agora (7 out. 2026): os arquivos estão como na origem. A
adaptação está no código do Trioio que os toca (`src/core/`), não neles.

## Checksums (SHA-256) no momento da cópia

```
b03986bcb23240a78cf1996f7e340bde5da254f008a8f5b7bf7708b02ef564ca  AY8910Chip.hpp
794e9eaf88e4a8848ac0c1b176c2593fc60b488b8f99da6d2d36cc4320dedf48  BreathExciter.hpp
ca1c9300ae0bd8727ee5c18c3a0054d67072cdfa82333513c33b7ea810693014  Envelope.hpp
9f8247fc5db3b6535fb57cee5c04664d065e21338526da0c24224fdf7123386c  FmFluteVoice.hpp
84aea1857c227b0022c2a53d3f0a607e7071bc95b41b53f79e273f7e770c414a  GaloVoice.hpp
228c3de2e707fd40f5a75e93753fb4630e8a099a69fd77db144a39f7c46f1b06  GendynOscillator.hpp
324b737587c71f1bc9892cf9b92b4db5d1a7bed043b413809ac7ecde456ca5d6  LfoBank.hpp
5e87d761f3f9b07ee337234b4e402fb6571e2dba44ce09df4f1593649628b996  ModalResonator.hpp
af1fd0f7c1cc308a3fc603d8e079b91e5eb3171dbc3b79c55f32a511e6eaff76  MultibandFilter.hpp
28348a5341620fb2c5d54e3dd29cff7c67919437349cdc427a153606bd7b0885  Oscillator.hpp
e8921369409cdb46a7690e9adf239d3ed5d3298c3815f45a5b9e55ce2bd0cf36  ScatVoice.hpp
ca5b5a97d855bc758111db38f625bd836d901e7992145766ba6c4185c998f7fd  ThereminVoice.hpp
16fcd7e0de8402db33e82d588cddb2deb83ad2ecb5800429b0c9870be7735407  VosimOscillator.hpp
3161082f18ca18373e7d6cff5f12938035c0552caa2fac9e2c6a9e34948263f6  WalshOscillator.hpp
```

## Segunda leva (7 out. 2026, mesmo commit)

`ChoirEnsemble` e `Rungler`, copiados do mesmo commit da primeira cópia e conferidos
idênticos à pasta de trabalho:

```
e6a99c8ecb8fc5bf2b57851a0e77eb764b4a7957ae6a4e24d9e451dad581dba3  ChoirEnsemble.hpp
2ab0d6f79e6ea24ff70e32364be52d686637894d87f1e185bd59817c814f4dac  Rungler.hpp
```

## Terceira leva (8 out. 2026, mesmo commit)

`GranularCloud` e `SpectralObject`, do mesmo commit, conferidos idênticos à pasta de trabalho:

```
4da04cf05916dc75666f2f0db6eb06efeef3307423e3a5d95264d10d0e20d8a8  GranularCloud.hpp
29b0f542cdbc232d4b942d87a47aaea254cabebaa4c20c5a418eb373eccfd233  SpectralObject.hpp
```

## Quarta leva (8 out. 2026, mesmo commit): escalas e afinações

Pedido do autor: "o Trioio está limitado a algumas escalas; o Studio já
criou códigos para trabalhar com uma gama bastante larga de escalas,
inclusive inventando escalas próprias". Três headers de
`rasgo-synth-core/src/sequencer/`, do mesmo commit `0dc468a`, conferidos
idênticos à pasta de trabalho, em `sequencer/`:

```
71c60744b79f45d5d4f1eb24493715107fe7fd0705704533fcc7bfcdc5fb09af  sequencer/MicrotonalTuning.hpp
d4d7e3c05b52544a146bd182d2bd66c48837adeb8834b05f940d420a28af2fd8  sequencer/Scales.hpp
a3502f87b9d74e5f82cf1e06a546208448f3363fd1828bd1afaf5c9c4d7562c3  sequencer/SieveScale.hpp
```

Créditos (dos comentários da origem):

- o catálogo de 45 escalas, escrito e revisado pelo Studio a partir do
  estudo do `Sequence.java` do Kosmische, com três erros da origem
  corrigidos e documentados;
- o crivo é a teoria de **Iannis Xenakis** (*Formalized Music*);
- as afinações seguem Ptolomeu e **Harry Partch** (justa 5-limit), a pilha
  pitagórica de quintas e **Alois Hába** e **Ivan Wyschnegradsky** (quartos
  de tom); as EDOs finas, **Easley Blackwood** (19), a teoria xenarmônica
  (22) e **Huygens/Fokker** (31).

**Adaptações no Trioio** (`src/core/ScaleBank.cpp`; os headers seguem
intocados):

- o Trioio usa as tabelas de `Scales.hpp`, `cataloguedPitchClassSets()` e
  os desvios justo e pitagórico de `MicrotonalTuning`;
- os geradores de crivo foram reescritos com o gerador do Trioio, porque
  `std::mt19937` com `std::uniform_real_distribution` dá números diferentes
  em libstdc++, libc++ e MSVC, e a mesma seed precisa dar a mesma peça em
  qualquer sistema;
- no crivo em EDO fina, os graus admitidos **são** a escala (graus fora
  dos 12 semitons), em vez de só desviar os 12 semitons como no Studio;
- os módulos vão de 3 a edo/3, de 5 a 9 graus, para a melodia continuar
  cantável;
- os nomes das escalas foram traduzidos para o português.

## Quinta leva (8 out. 2026, mesmo commit): transformações neo-riemannianas

`sequencer/NeoRiemannian.hpp`, do mesmo commit `0dc468a`, conferido idêntico
à pasta de trabalho, para a harmonia em movimento (`src/core/Harmony.cpp`,
`docs/HARMONIA.md`):

```
b6fc47be2cd0cba9150ba7dcee83542f88faacef6a01b10d1a73e08f88ca899d  sequencer/NeoRiemannian.hpp
```

Créditos: a teoria de **Hugo Riemann**, na formalização transformacional de
**Richard Cohn** e **David Lewin** (P, L, R). Sem adaptação no header. As
técnicas do `HarmonicWanderer` (substituição de trítono, mediantes
cromáticas) foram reescritas no Trioio, sem copiar o arquivo, porque ele
sorteia com `std::mt19937`.
