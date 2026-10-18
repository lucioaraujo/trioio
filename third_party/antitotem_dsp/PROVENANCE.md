# Cópia fixada do DSP do Antitotem

`CmosVoice.h` (a voz CMOS: cinco osciladores, núcleos Schmitt, formas de
função e deriva sem buffer, PLL 4046 "caçando", realimentação entre eles) e
`NoiseFields.h` (paleta de ruídos coloridos) vêm do **Antitotem**
(`lucioaraujo/antitotem`, `src/core/`), mesma autoria (Lúcio Araújo). São
tocados pelo Trioio na família **CMOS** e como excitação da família
**ressonador soprado** (`src/core/AntitotemVoice.cpp` e
`src/core/ModularVoice.cpp`).

## Origem

- **Commit:** `eabde9f5cbadf7b1ba3254354002cb5e723cba40`. Cópia feita com `git show <commit>:<caminho>`.
- **Conferido:** os arquivos são idênticos aos da pasta de trabalho no
  momento da cópia.
- **Regra:** instrumentos RASGO autônomos
  (`RASGO_DOCUMENTATION/GOVERNANCA_E_TRANSVERSALIDADE.md §7.0`). A cópia é
  do Trioio e pode ser adaptada; cada adaptação é registrada abaixo.

## Licença

**AGPL-3.0-or-later**, a mesma do Trioio. Nas palavras do próprio
`CmosVoice.h`: "estudos digitais originais de comportamentos de circuito,
não emulações de componente". Os circuitos de referência são os CMOS 4093,
4020 e 4046, de documentação pública.

## Adaptações

Nenhuma (7 out. 2026). O Trioio só converte a nota para a escala de
controle da voz (0..1, curva exponencial), no código que a toca.

## Checksums (SHA-256) no momento da cópia

```
afd716ddb78d3c6df060a6f7cace3fe464f295223c0e5510d5be90f478a2fcc8  CmosVoice.h
39a2136c7e68904e0088d0028e411ff0dda0b7003a390a52a7638becb787235c  NoiseFields.h
```
