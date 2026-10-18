# Cópia fixada do DSP do Rasgo Modular

Estes 12 headers vêm do **Rasgo Modular** (`RASGO_MODULAR/src/core` e
`src/dsp`), mesma autoria (Lúcio Araújo) e mesmo workspace RASGO. São
tocados pelo Trioio como famílias de timbre (`src/core/ModularVoice.cpp`,
`docs/SINTESE.md`). As pastas `core/` e `dsp/` repetem as da origem,
para os includes internos (`"core/SignalGraph.hpp"`) funcionarem sem
alteração.

## Por que uma cópia

Decisão do autor (7 out. 2026): "o Trioio será autônomo, com código
próprio, sem depender de nenhum outro projeto do RASGO; se quiser
aproveitar os códigos, faça uma cópia para o Trioio". Até esta cópia, o
Trioio lia esses headers direto de `../RASGO_MODULAR` por include path, e
isso foi desfeito.

## Origem

- **Repositório:** `lucioaraujo/rasgo-modular`, commit
  `fcb711aaadf5fd4d46da259efb5a308346f5ebef` (release v0.1.6, 7 out. 2026).
  Cópia feita com `git show <commit>:<caminho>`.
- **Conferido:** os arquivos são idênticos aos da pasta de trabalho no
  momento da cópia.
- **Adaptar é permitido e esperado** (regra RASGO, `GOVERNANCA_E_TRANSVERSALIDADE.md §7.0`):
  a cópia é código do Trioio e pode ser transformada para os conceitos
  musicais dele. Cada adaptação entra na seção "Adaptações" abaixo, para
  ficar claro o que veio da origem (checksums) e o que mudou aqui.

## Licença

**AGPL-3.0-or-later**, a mesma do Trioio. O Rasgo Modular foi escrito
estudando comportamentos e técnicas publicadas, sem copiar código. As
fontes estudadas estão no comentário de cada módulo e nos dossiês do Rasgo
Modular (`dossies/`). Entre elas:

- Mutable Instruments Rings e Elements, de Émilie Gillet (MIT: estudo de
  comportamento);
- Cook, *Real Sound Synthesis*;
- Fletcher e Rossing;
- Buchla 259/258 e Serge Wave Multipliers;
- PolyBLEP (Välimäki e Huovilainen; Martin Finke);
- Karplus e Strong (1983);
- J. O. Smith, *Physical Audio Signal Processing*;
- Jaffe e Smith;
- síntese pulsar (Curtis Roads).

## Adaptações

Nenhuma até agora (7 out. 2026): os arquivos estão como na origem. A
adaptação está no código do Trioio que os toca (`src/core/`), não neles.

## Checksums (SHA-256) no momento da cópia

```
d2c1246ae0163445432d38c35e85095f923ff3b7d32f9af6da80f991d5577a7c  core/Graph.hpp
088bd4af93d47f5c8efea6b555cda92e035e180ab93e84e26ce11a8a35879f45  core/Panel.hpp
026ad1a2ed7a263dfffabba7b1718ccdac7e351d1623a23f85db5883376cc219  core/SignalGraph.hpp
3b59c15a6ce708b5c157a4e540744b8b9776f76f45e27134ab80d494674bfa64  dsp/Drum.hpp
079366eeb48e300646b3aa1ea4ea0a2f68035669a9bde808f1fc534c5dff0b61  dsp/Formant.hpp
dc1af55119a8b8d567a5b8d26fb65de4cc8c50bb2482a38526a1ca2d618cc442  dsp/Matter.hpp
bf222eb3219c8f52073ca2999b54b4183524278c79722e16a242ad1af52edddc  dsp/Oscillator.hpp
30bcd9d6d61081248cd7a52f1806d1fa92ea56020ec3bb246f4df0c8f2f534ee  dsp/Oversampler.hpp
a59b0c73648942e0551a61837387a60b539863f323d8616efe01e40507774135  dsp/Pulsar.hpp
c1acbd23281a264071cac92127b630f247985b674b84304d628e5fed5b07ae38  dsp/Shape.hpp
f76c04c9e0f2bd761af674a043f211d1726975c70af2bf762ea0b56084ea93bc  dsp/StringVoice.hpp
776460b77adf247afe2ec0b859251d5efcc0499da5def50b04b77e9e84bd19b3  dsp/Wavetable.hpp
```

## Segunda leva (7 out. 2026, mesmo commit)

`RESONATOR`, `HALL` e `SPACE`, copiados do mesmo commit da primeira cópia e conferidos
idênticos à pasta de trabalho:

```
ede360d4e728f0c65dcb03dce7a4c3e0db95e0394d4e7b4db61a3d62a717b77d  dsp/Resonator.hpp
2c01cf2e4bf08814c9e4b63f92dd7f964720dd0a0a0319f7d53fde63449c012f  dsp/Hall.hpp
242ae0a1983154282919cc56ed2f0209e6a1670a47d1a0c6400f7dc34e452281  dsp/Space.hpp
```

## Terceira leva (8 out. 2026, mesmo commit)

`OPERATOR` e `LPG`, do mesmo commit, conferidos idênticos à pasta de trabalho:

```
3e167ab18c2979f9963e0f760816d9c9e954483ccc137215d0d33d12a2325041  dsp/Operator.hpp
092277ef2f14721c01b1e96e7533b191155fa466991b7346aa79eca1e49c094a  dsp/Lpg.hpp
```
