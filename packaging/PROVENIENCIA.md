# Proveniência do empacotamento

Os scripts de `linux/` (`empacotar.sh`, `install.sh`,
`testar-arranque.sh`) foram **copiados** do Antitotem
(`PROJETOS_AUTORAIS/ANTITOTEM/packaging/linux/`, commit `6c88ab4`) em
2026-10-18. Eles são a versão genérica do padrão multiplataforma do RASGO
(`RASGO_DOCUMENTATION/PADRAO_DISTRIBUICAO_MULTIPLATAFORMA.md`). A
regra dos instrumentos autônomos manda copiar, nunca depender da pasta irmã.

- **Autor:** Lúcio Araújo (os dois projetos).
- **Licença:** AGPL-3.0-or-later (a mesma nos dois).
- **Adaptação ao Trioio:** o `install.sh` copia e remove também
  `share/trioio`, com os dados que o app acha em `../share/trioio/data`
  (`src/library/DataFiles.h`). O `trioio.desktop` e o
  `README-tarball.txt` foram escritos para o Trioio, no formato dos do
  Antitotem.
- **Ferramenta de terceiros no caminho do release:** o `linuxdeploy`, só
  na CI, fixado por versão e SHA-256 no `empacotar.sh`. Não fica no
  repositório.

O ícone (`icon/`) foi gerado em 2026-10-18 a partir de
`website/assets/identity/trioio-olho.svg`, a logo dos três olhos do autor,
com o Inkscape:

- `trioio-1024` até `trioio-48` saem do SVG original;
- 32, 24 e 16 px saem de `trioio-olho-pequeno.svg`, o mesmo desenho com o
  traço mais grosso, para continuar legível em tamanho pequeno;
- `trioio.ico` é a junção dos tamanhos, para o Windows.

Checksums dos originais copiados (Antitotem @ 6c88ab4):

```text
c2dc50b470bf1f56aa46ad304552d8b1f3c513cecf0902b21e1133f18b51d814  empacotar.sh
33fa272fcaa693f354b33d0f2398dd5d8a1fb505b6f2f20c8fabd62e6c987678  install.sh
52632772917a059840022982e4e7cabc51fc13782f56bf4026841db9ca3cc3c0  testar-arranque.sh
e858b570b739b6e8415b7503171a5739f52be4fbd47cdb4bfca291455f704f12  README-tarball.txt
1e6105bec546242fc6efaf35adec2edd291aeaf64c9c4249e9605e6400ce4df9  antitotem.desktop
```
