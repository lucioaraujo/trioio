# Situação de licença: Trioio e JUCE

**Situação atual:** código próprio sob AGPL-3.0-or-later. O JUCE 9.0.0 é
usado sob AGPL-3.0-only, só na camada da Biblioteca (`src/library/`). Não
é necessária licença comercial do JUCE.

É convenção da família RASGO registrar por escrito sob qual opção de
licença cada projeto compila contra o JUCE. Precedentes:
`RASGO_MODULAR/apps/juce/LICENSE_STATUS.md` e
`NAVALHA2_JUCE/docs/LICENSE_STATUS.md`.

## A combinação

O código do Trioio já é AGPL-3.0-or-later, da mesma família de licença do
JUCE em AGPLv3. O "or later" cobre a AGPL-3.0, e a obra combinada é
distribuída coerentemente sob AGPL.

O núcleo (`src/core/`) não usa o JUCE e pode ser compilado e reutilizado
sem ele, sob a licença do próprio Trioio.

## Efeito prático

- **Cláusula de rede (Seção 13 da AGPLv3).** Se uma versão modificada
  oferecer interação remota a usuários por uma rede, a fonte precisa ser
  oferecida a eles. O Trioio é um instrumento desktop local, então hoje ela
  não é acionada. Passa a valer se algum dia houver um serviço hospedado.
- **Ao distribuir um binário:** levar o `LICENSE` junto e oferecer também a
  fonte do JUCE na versão usada, registrando essa versão na release.
