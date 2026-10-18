# TRIOIO — estrutura do projeto

O TRIOIO seguirá a organização de engenharia já validada no Navalha, adaptada
ao seu núcleo de escuta, estratégia e síntese. A organização separa o motor
musical da aplicação, das ferramentas de pesquisa e da validação.

```text
TRIOIO/
├── LICENSE                 # GNU AGPLv3 ou posterior
├── CREDITS.md              # autoria, referências e licenças verificadas
├── THIRD_PARTY_NOTICES.md  # dependências e obrigações de distribuição
├── docs/                   # conceito, arquitetura, manual e decisões
├── src/
│   ├── core/               # Cérebro, energia, DSP e contratos portáteis
│   ├── app/                # JUCE, interface e integração de dispositivos
│   ├── tools/              # análise offline, importação e conversores
│   └── validation/         # verificadores executáveis e relatórios
├── tests/                  # testes unitários e concorrência
├── validation/             # renders, métricas e evidências versionáveis
├── assets/                 # somente materiais licenciados e catalogados
├── examples/               # exemplos autorais ou com licença registrada
└── packaging/              # builds e empacotamento por plataforma
```

O núcleo atual já está em `src/core` e seus testes em `tests`. Os demais
diretórios serão criados quando houver conteúdo real. Não serão copiados
arquivos do Navalha: a semelhança é apenas organizacional e metodológica.

Qualquer migração de documentos da raiz para `docs/` ocorrerá em um marco
próprio, depois de atualizar referências, CMake, scripts e links.

## Biblioteca de Música: acervo externo, não código-fonte

As faixas usadas como referência não pertencem, por padrão, ao repositório nem
são copiadas para dentro dele. O usuário aponta uma ou mais raízes de Biblioteca
na aplicação. O TRIOIO indexa esses diretórios em segundo plano, conserva os
arquivos originais intactos e registra no manifesto apenas a referência de
origem, a identidade técnica e a autorização declarada.

Uma organização possível do acervo é:

```text
TRIOIO_LIBRARY/
├── referencias/
│   ├── autorais/
│   ├── autorizadas/
│   └── geradas-autorizadas/
├── stems/
├── imports-pendentes/
└── sessoes/
    └── <sessão>/
        ├── manifesto.json
        ├── prompts.json
        ├── decisoes.json
        ├── escuta.json
        ├── analise-cache/
        └── saidas/
```

`referencias/` contém material que pode ser carregado para escuta; `stems/`
mantém suas partes quando elas existirem; `imports-pendentes/` é uma área de
triagem sem autorização ainda declarada; e `sessoes/` guarda os resultados do
jogo composicional. A sessão nunca escreve sobre a faixa de origem.

O cache técnico — waveform, descritores e mapa de análise — poderá ficar na
sessão ou em cache de aplicação, vinculado à identidade do arquivo. Ele é
reconstruível e não substitui o áudio. A primeira leitura prepara rapidamente
a interface e uma janela curta para resposta; análises profundas, indexação e
comparações correm fora da thread de áudio.

O contrato operacional da Biblioteca, da memória local e dos manifestos está em
[`BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md`](BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md).

Além do acervo, a aplicação poderá manter uma memória local e editável, em uma
raiz de dados escolhida pelo usuário:

```text
TRIOIO_DATA/
├── cache/             # waveform e análise reconstruíveis
├── memoria/
│   ├── sessoes/       # decisões, seeds, escutas e relações por sessão
│   └── preferencias/  # escolhas explícitas e tendências locais
└── indice/             # catálogo das Bibliotecas configuradas
```

`prompts.json` separa texto original, intenção declarada traduzida e análise
inferida do áudio. `escuta.json` preserva a avaliação humana da resposta.
Memória e aprendizagem guardam relações e decisões, não cópias ocultas das
referências. O usuário poderá limpar cache, apagar sessões, invalidar tendências
aprendidas, editar/remover prompts e limitar a memória a uma Biblioteca.

## Onde ficam os áudios e os dados (o padrão de 2026-10-16)

O autor pediu: "há muitos arquivos wav espalhados por todo canto, evite isso,
precisamos de um padrão de criação de diretórios".

Tudo o que é áudio ou dado do Trioio fica em
`/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/`, segundo o `AGENTS.md` do RASGO
(`AUDIOS_RASGO/<instrumento>`):

| pasta | o que guarda |
|---|---|
| `dados/` | a pasta de dados que o app abre: autorizações, julgamentos, sessões |
| `dados-2026-10-13/` | a pasta de dados anterior, preservada |
| `referencias/` | atalhos para as gravações-fonte |
| `escutas/AAAA-MM-DD-tema/` | o que se gera para ouvir |

O `LEIA-ME.md` dessa pasta tem as regras; `ORGANIZACAO_2026-10-16.md` tem o
registro do que foi movido e apagado.

**Na prática:**

- **Nada de áudio** no repositório, no disco do sistema, em `tmp-builds` ou
  em pastas soltas.
- **As audições:** com `TRIOIO_ESCUTAS=/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/escutas`,
  um nome de arquivo sem pasta vai para `escutas/AAAA-MM-DD/`
  (`trioio_biblioteca timbres` e `efeitos`).
- **As sessões sem julgamento e as escutas superadas** são arquivos de
  teste, e podem ser apagadas.
- **Os julgamentos** (`escuta.json`) **e as autorizações** (`indice/`)
  nunca são apagados.

**A retenção das sessões** (2026-10-17, `src/library/Retention.h`):

```
trioio_biblioteca limpar <dados> [--manter 3] [--executar sim]
trioio_biblioteca esvaziar-lixeira <dados> [--dias 7] [--executar sim]
```

- **Sem `--executar sim`, os dois só mostram** o que fariam.
- **`limpar`:**
  - deixa intocadas as sessões julgadas (as que têm uma derivação com
    julgamento no `escuta.json`) e as 3 mais recentes;
  - move as outras para `dados/lixeira/AAAA-MM-DD/`. É reversível: basta
    mover de volta para `sessoes/`.
- **`esvaziar-lixeira`:** apaga só as pastas da lixeira com mais de 7 dias.
- **O índice fica:** os manifestos de derivação continuam em `indice/`, como
  registro do que foi feito. A interface esconde as versões cuja sessão não
  existe mais.
- **Testado** em `RetentionTests`: a julgada e a recente ficam; a de teste
  vai para a lixeira com os arquivos; o índice fica; a lixeira respeita os
  dias, inclusive na virada do mês.
- **Em 17/10:** a pedido do autor ("pode apagar os testes"), as 56 sessões
  não julgadas de 16 e 17/10 foram apagadas, 3 GB. Ficaram as três julgadas.
  Os dados caíram para 385 MB.

