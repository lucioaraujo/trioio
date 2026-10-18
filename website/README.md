# Site do Trioio

**Estado: em preparação — NÃO publicado.** O site de um instrumento RASGO só é
publicado junto com o instrumento. Até lá, as páginas têm
`<meta name="robots" content="noindex">` e o selo "em preparação". Ao
publicar: tirar o noindex em `gerar_paginas.py`, conferir a URL base em
`seo.py` (provisória: `https://lucioaraujo.github.io/trioio/`) e ligar o card
no portal RASGO.

## Como gerar

```sh
python3 gerar_paginas.py && python3 seo.py
```

- `gerar_paginas.py` é a fonte única: um modelo e os textos dos quatro idiomas.
  Gera `index.html` (português, canônica), `en.html`, `fr.html` e `es.html`.
  **Os .html são derivados.** Editar os textos no script.
- `seo.py` injeta os metadados (descrição, Open Graph, hreflang, JSON-LD) e
  escreve o `sitemap.xml`. É o código comum da família; só a configuração do
  topo é do Trioio.

## Conteúdo e regras

- O texto público não traz o nome como anagrama de identidade pessoal
  (`docs/TRIOIO_CONCEITO.md`).
- A faixa fixa `rasgo-strip` aponta para o portal; o cabeçalho próprio vem logo
  abaixo. Regra da família RASGO.
- As fontes são livres e embutidas, nunca fontes de sistema nomeadas.
- O contato usa o diálogo de copiar o endereço, não `mailto:`.
- A paleta vem dos tokens da interface (`src/app/MainView.h`). Ela é
  provisória, como a da interface (`docs/design.md §6`).

## Imagens (derivadas, próprias)

| Arquivo | Origem |
|---|---|
| `assets/interface.jpg` | `docs/design/prototipo-v0-1920x1080-completo.png`, instantâneo do `trioio_app` (dados de demonstração, olho revelado; refeito em 2026-10-13 com o símbolo novo), JPEG q84 |
| `assets/og-trioio.jpg` | o mesmo instantâneo, recorte central 1200×630, JPEG q85 |
| `assets/identity/trioio-olho.svg` | da logo dos três olhos do autor (2026-10-13), só os olhos, a pedido dele: o Terceiro Olho em âmbar em cima, dois olhos embaixo; traço, cores dos tokens |

## Proveniência das cópias

Cópia fixada de `ANTITOTEM/website`, no commit `b285505` (2026-08-27):

| Arquivo | sha256 | Licença |
|---|---|---|
| `assets/fonts/dejavu-sans-400.woff2` | `78a6fff8c1c80c36308959c615fc8be4c1fc0382ebbffbfa3090d22dd7071e8a` | DejaVu (Bitstream Vera, livre) |
| `assets/fonts/dejavu-sans-700.woff2` | `1a1984b5e3fa20c8b2b292d65ead462134d2d970377438888d485e108294a248` | idem |
| `assets/fonts/dejavu-sans-mono-400.woff2` | `07bed8326332033d814124dc5ab4cbb1171432caffb5fb150d4ac674676ba05a` | idem |
| `assets/fonts/dejavu-sans-mono-700.woff2` | `8a1e8269d844898145b1f8dbbe6e2221f0e061cdc1b10be8eccfb184fdca2782` | idem |
| `assets/contact.js` | `55f650ea3bd79a2ccf0af306489bf1fdc90131d6005626416e001f6f92783d58` | AGPLv3+ (RASGO) |
| `seo.py` | `b1076d…` na origem; só a configuração mudou | AGPLv3+ (RASGO) |

Os subconjuntos das fontes são os mesmos do Antitotem: latim e a pontuação
comum. Os acentos franceses e espanhóis, ♭, ×, · e — foram conferidos nas
capturas.

## Validação (2026-10-12)

- 4 páginas geradas, cada uma com 5 `hreflang`.
- O `sitemap.xml` tem 4 URLs.
- Nenhum link local quebrado.
- O noindex está presente nas quatro páginas, e o anagrama está ausente.
- Capturas no Chrome headless:
  - desktop 1440 px (PT);
  - celular 390 px (FR, o idioma de texto mais longo).
  - Nos dois tamanhos não há rolagem horizontal, e os passos se empilham.
