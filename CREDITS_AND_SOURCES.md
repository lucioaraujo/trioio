# Trioio — créditos, fontes e licenças

Documento voltado à publicação: o que alguém precisa ler antes de usar,
redistribuir ou derivar do Trioio. Segue o formato do Rasgo Modular e do
Antitotem.

## Autoria e licença

**Lúcio de Araújo**, 2026. Código sob **GNU AGPL-3.0-or-later**: ver
[`LICENSE`](LICENSE). É a licença habitual da família RASGO, confirmada
pelo autor para o Trioio em 2026-10-05: C++, JUCE, software livre.

A camada que usa o JUCE (hoje a Biblioteca, `src/library/`; depois a
aplicação) o usa sob **AGPLv3**, a opção livre do JUCE, sem licença
comercial. Detalhe em [`docs/LICENSE_STATUS.md`](docs/LICENSE_STATUS.md).

O núcleo (`src/core/`) não depende do JUCE nem de nenhuma outra biblioteca.

## Código de terceiros incorporado

**Nenhum.** O JSON dos manifestos usa o `juce::JSON` do próprio JUCE, sem
biblioteca adicional. O JUCE não é versionado neste diretório: é um
checkout local apontado por `-DTRIOIO_JUCE_PATH`, como nos outros
instrumentos RASGO.

Quando algum componente externo entrar, ele ganha uma linha aqui com
autor, origem, versão, licença e local de uso, e a mesma triagem por cor
dos projetos irmãos:

- **verde:** domínio público, licença livre compatível ou código do autor;
- **amarelo:** copyleft compatível, com crédito e decisão registrados;
- **vermelho:** proprietário ou não comercial, que serve só de referência
  conceitual.

## Código próprio do RASGO

O reaproveitamento previsto de outros instrumentos do autor está em
[`docs/TRIOIO_INVENTARIO_RASGO.md`](docs/TRIOIO_INVENTARIO_RASGO.md), e o
inventário de síntese em [`docs/SINTESE.md`](docs/SINTESE.md). Todo o
código abaixo é do mesmo autor (verde).

| Origem | O quê | Como entra | Licença | Onde |
| --- | --- | --- | --- | --- |
| **Rasgo Modular** (`RASGO_MODULAR/src/core`, `src/dsp`) | módulos `WAVETABLE`, `PULSAR`, `MATTER`, `STRING`, `DRUM`, `OSC`, `FORMANT`, `SHAPE`, `RESONATOR`, `HALL`, `SPACE`, `OPERATOR`, `LPG` e o núcleo `SignalGraph` | **cópia fixada** do commit `fcb711a` (v0.1.6), com checksums: [`third_party/rasgo_modular_dsp/PROVENANCE.md`](third_party/rasgo_modular_dsp/PROVENANCE.md). O Trioio é autônomo: nenhum caminho aponta para fora dele | AGPL-3.0-or-later, a mesma do Trioio | `src/core/ModularVoice.cpp` (desde 2026-10-07) |
| **Rasgo Synth** (`RASGO_SYNTH/rasgo-synth-core/src/dsp`) | `GendynOscillator`, `VosimOscillator`, `WalshOscillator`, `ThereminVoice`, `FmFluteVoice`, `ScatVoice`, `AY8910Chip`, `GaloVoice`, `ChoirEnsemble`, `Rungler`, `GranularCloud`, `SpectralObject` e dependências (18 headers) | **cópia fixada** do commit `0dc468a`, com checksums: [`third_party/rasgo_synth_dsp/PROVENANCE.md`](third_party/rasgo_synth_dsp/PROVENANCE.md) | GPL-3.0; combinada com a AGPL-3.0 pela seção 13 das duas | `src/core/RasgoSynthVoice.cpp` (desde 2026-10-07) |
| **Antitotem** (`ANTITOTEM/src/core`) | `CmosVoice` (voz CMOS: 5 osciladores, PLL 4046) e `NoiseFields` (ruídos coloridos) | **cópia fixada** do commit `eabde9f`, com checksums: [`third_party/antitotem_dsp/PROVENANCE.md`](third_party/antitotem_dsp/PROVENANCE.md) | AGPL-3.0-or-later | `src/core/AntitotemVoice.cpp`, `src/core/ModularVoice.cpp` (desde 2026-10-07) |
| **Aquorbium** (`AQUORBIUM/src/core`) | `BiomaModalResonator`, `BiomaPhysicalString`, `BiomaGranularStriker` e `Numeric.h` | **cópia fixada** do commit `e234a25`, com checksums: [`third_party/aquorbium_dsp/PROVENANCE.md`](third_party/aquorbium_dsp/PROVENANCE.md). Nada editado no Aquorbium | AGPL-3.0-or-later | `src/core/AquorbiumVoice.cpp` (desde 2026-10-08) |

Os créditos das ideias de origem (Xenakis, Kaegi e Tempelaars, Walsh,
Gillet/Mutable Instruments, General Instrument, Välimäki e Huovilainen)
seguem em `PROVENANCE.md`, como no `CREDITS.md` do Rasgo Synth. Os módulos
do Rasgo Modular trazem os créditos nos próprios dossiês
(`RASGO_MODULAR/dossies/`).

## Material musical

As referências de áudio, stems, MIDI e prompts **não fazem parte do
código** e não estão neste diretório. Cada uma tem a própria proveniência e
autorização registradas no seu manifesto
([`docs/MANIFESTO_DE_PROVENIENCIA.md`](docs/MANIFESTO_DE_PROVENIENCIA.md)).
A licença do código não se estende a elas.
