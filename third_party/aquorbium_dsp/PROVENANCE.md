# Cópia fixada do DSP do Aquorbium

Três corpos sonoros do **Aquorbium** (`AQUORBIUM/src/core/`), mesma
autoria (Lúcio Araújo), tocados pelo Trioio como famílias de timbre
(`src/core/AquorbiumVoice.cpp`):

- `BiomaModalResonator`: ressonador modal de 4 modos, com material, dureza
  e posição do golpe;
- `BiomaPhysicalString`: corda física (laço de atraso fracionário, perda,
  posição do pluck);
- `BiomaGranularStriker`: golpe de até 12 partículas ressonantes, com
  espalhamento espectral e irregularidade.

A pasta `core/` repete a da origem para os includes internos
(`"core/Numeric.h"`) funcionarem sem alteração.

## Origem

- **Commit:** `e234a256cc6696bcbdf192e404456199833eae71` (2026-08-21), o
  último commit que tocou estes arquivos no repositório principal
  `rasgo-instruments`. Cópia feita com `git show <commit>:<caminho>`.
- **Autorização:** pedido do autor em 8 out. 2026 ("faça todos", trazer
  ao Trioio o que faltava, inclusive do Aquorbium). O Aquorbium é
  trabalhado pelo autor com o Codex. Nada nele foi editado; estes sete
  arquivos estavam sem alterações pendentes e são idênticos à pasta de
  trabalho no momento da cópia. Outros arquivos do Aquorbium tinham
  alterações não commitadas do Codex, e nenhum deles foi copiado.
- **Regra:** instrumentos RASGO autônomos
  (`RASGO_DOCUMENTATION/GOVERNANCA_E_TRANSVERSALIDADE.md §7.0`). A cópia é
  do Trioio e pode ser adaptada.

## Licença

**AGPL-3.0-or-later** (`AQUORBIUM/LICENSE-AGPL-3.0.txt`), a mesma do
Trioio. Conceito e fontes estudadas de cada corpo estão nos documentos de
projeto do Aquorbium: `MODAL_RESONATOR_DESIGN.md`,
`PHYSICAL_STRING_DESIGN.md` e `GRANULAR_STRIKER_DESIGN.md`.

## Adaptações

Nenhuma no código copiado (8 out. 2026). O Trioio só define os parâmetros
e o golpe por nota, no código que os toca.

## Checksums (SHA-256) no momento da cópia

```
e715ff187b5c1eb8a6ea190b8e7ad0dc80295ab02ecb0f2e2f1c5dc39170794e  core/BiomaModalResonator.h
95240a47dc8ea99801b0fbf97b78b4ba5f71e376aa7274bdfa5a7be161ed418e  core/BiomaModalResonator.cpp
7212828c3e74ffb9812813235f91be4e253df0efc06622f4c44a316a1d5593c3  core/BiomaPhysicalString.h
d28f6e22c4406e7eed1574636754dce2a25e4320eedcf0e2875ecdd45d4f7ad3  core/BiomaPhysicalString.cpp
d74e6641c121b64dc236b0209298879aee35b60f549575621d223205bbccbd0b  core/BiomaGranularStriker.h
4b97b7e97db3b84b16146e375f2388bf997157bbb3cf6859c485ddce9a4e92a7  core/BiomaGranularStriker.cpp
bc91f30e8106fdde034bf0c3c93abb0a7911665c32be7714327efe128e9501a7  core/Numeric.h
```
