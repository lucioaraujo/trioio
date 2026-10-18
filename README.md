# Trioio

Localização atual (2026-10-11): `RASGO/PROJETOS_AUTORAIS/TRIOIO/`.
O caminho antigo na raiz é um alias de compatibilidade, não outra fonte.
Criação e desenvolvimento são a finalidade; autoria, licença e identidade
permanecem próprias. Ler [as instruções RASGO](../../AGENTS.md) e a continuidade
local antes de trabalhar. Organização e validações no
[registro da migração](../../RASGO_DOCUMENTATION/architecture/MIGRACAO_PROJETOS_AUTORAIS.md).
Aplicar o [procedimento de recursos](../../../PROCEDIMENTO_SEGURANCA_RECURSOS.md)
antes de tarefas controladas; publicação continua uma decisão separada.


**Identificador arquivístico:** `ARQ-TRI-001`
**Estado:** protótipo funcional offline (escuta → composição → Terceiro
Olho → WAV/MIDI), em desenvolvimento ativo desde 2026-10-07.

- **Foco atual (2026-10-18):** a musicalidade e a escuta do autor.
  - **A máquina de harmonia:** 26 linguagens, a condução a quatro vozes, o
    arquivo de cadências e Partch microtonal.
  - **A máquina de timbres:** modelos físicos próprios, conjuntos por estilo
    e produção.
  - **O jogo de Vitoriamario:** implementado, com critérios medidos no som,
    estado, regras do autor e troca de lugar no diálogo.
  - **A saída:** stems por voz e retenção das sessões de teste.
  - **A intenção e o modo híbrido:** prompts traduzidos para o vocabulário
    do Trioio, o pedido × o ouvido abrindo o Terceiro Olho, o gesto do
    autor como forma, e a comparação entre versões
    ([`docs/PROMPTS_E_INTENCOES.md`](docs/PROMPTS_E_INTENCOES.md),
    [`docs/VERSOES.md`](docs/VERSOES.md)).
  - **O desempenho e o release:** a resposta de jazz de 126 para 22 s com
    o mesmo som ([`docs/MEDICOES.md`](docs/MEDICOES.md)); os dados viajam
    com o binário; o pacote de fontes com teste de restauração
    ([`docs/MULTIPLATAFORMA.md`](docs/MULTIPLATAFORMA.md)).
  - A escuta das referências segue medida pelas quatro réguas de
    [`docs/LEITURA_E_GABARITO.md`](docs/LEITURA_E_GABARITO.md).
- **Interface gráfica:** o app JUCE (`trioio_app`), com as abas de compor e
  "Sua escuta" (A × B, julgamento por trecho); ver
  [`docs/COMO_TOCAR.md`](docs/COMO_TOCAR.md).
- **Última revisão:** 2026-10-18.
**Autoria:** Lúcio de Araújo
**Licença:** GNU AGPL-3.0-or-later ([`LICENSE`](LICENSE)); JUCE sob AGPLv3
na camada da Biblioteca — ver [`CREDITS_AND_SOURCES.md`](CREDITS_AND_SOURCES.md)

## Apresentação

Trioio é um instrumento de escuta, memória, estratégia e síntese energética.
Seu “Terceiro Olho” relaciona áudio autorizado, metadados e intenções
declaradas para produzir decisões e síntese próprias; não copia áudio, prompts
ausentes ou processos internos de plataformas externas.

O projeto deriva da bifurcação de “Maravi Tri Oio”, em 2026-08-20. Esse nome é
um antecedente histórico compartilhado; Maravi é hoje o projeto de
transcrição, identificado por `ARQ-MAR-001`, e Trioio é o instrumento atual
identificado por `ARQ-TRI-001`.

## Retomada e fontes locais

O ponto de retomada, com o estado, os comandos e a ordem dos próximos
passos, está em [`TRIOIO_TAREFAS.md`](TRIOIO_TAREFAS.md) (as entradas mais
recentes ficam no fim da seção de estado). O roteiro atual é
[`docs/MUSICALIDADE.md`](docs/MUSICALIDADE.md).

Como o Trioio compõe e soa:

- [`docs/COMPOSICAO.md`](docs/COMPOSICAO.md): motivo, frases, forma,
  Terceiro Olho;
- [`docs/HARMONIA.md`](docs/HARMONIA.md): harmonia em movimento
  (acordes lidos, transformados pelo modo, baixo, voz de harmonia);
- [`docs/ESCALAS.md`](docs/ESCALAS.md): catálogo de 45 escalas,
  inventadas, EDO, afinações, o **arquivo de escalas próprio** (2576, com
  fontes citadas e o comportamento de cada linguagem, em `data/escalas/`) e
  a **máquina de escalas** (recurso transversal do RASGO);
- [`docs/MEMORIA.md`](docs/MEMORIA.md): oráculo (ordem);
- [`docs/MOSAICO.md`](docs/MOSAICO.md): mosaico por timbre (som);
- [`docs/RITMO.md`](docs/RITMO.md): ritmo como campo, e o arquivo de 26
  ciclos de culturas (iqa', tala, usul, aksak, claves, gamelão, flamenco),
  com as chegadas (sam, tihai), em `data/ritmos/`;
- [`docs/REGRAS.md`](docs/REGRAS.md): regras do autor;
- [`docs/ESCUTA.md`](docs/ESCUTA.md): a escuta do autor, sobre a resposta
  inteira ou só um trecho ou seção (`escutar --trecho|--secao`);
- [`docs/MAQUINA_DE_HARMONIA.md`](docs/MAQUINA_DE_HARMONIA.md): o estudo de
  harmonia e a máquina de harmonia. São 26 linguagens: funcional, jazz,
  brasileiras, impressionista, modal, século XX, compositores, espectral e
  Partch. Inclui o arquivo de 50 cadências em `data/harmonia/`
  (`trioio_biblioteca cadencias`).
- [`docs/MAQUINA_DE_TIMBRES.md`](docs/MAQUINA_DE_TIMBRES.md): a máquina de
  timbres (corpos acústicos por modelo físico, receitas, conjuntos por estilo).
- [`docs/COMO_TOCAR.md`](docs/COMO_TOCAR.md): como tocar o Trioio, passo a
  passo (trazer, autorizar, compor, ouvir, julgar, comparar).
- [`docs/OUTROS_INSTRUMENTOS.md`](docs/OUTROS_INSTRUMENTOS.md): o que os outros
  instrumentos RASGO já fazem e interessa ao Trioio (o Método RASGO, os 142
  mecanismos do Studio, os módulos do Modular, Aquorbium, Antitotem, Navalha,
  Maravi, Trucmetry, Risco).
- [`docs/JOGO.md`](docs/JOGO.md): o jogo de Vitoriamario. O Trioio inventa as
  próprias regras (a forma comum, o ciclo de vida, os critérios musicais), e
  as teorias ficam como referência. Implementado (§9): regras lidas,
  variadas, do adubo, do jogo e do autor; critérios medidos no som; estado
  do jogo; julgamento por trecho; troca de lugar no diálogo.
- [`docs/REAPROPRIACAO.md`](docs/REAPROPRIACAO.md): dossiê da reapropriação
  criativa (*détournement*, *découpe*, colagem). É estudo, não adotado: a
  resposta é só síntese até nova decisão do autor.
- [`docs/AUDITORIA_2026-10-14.md`](docs/AUDITORIA_2026-10-14.md): o que falta,
  o que foi pensado e não feito, os limites de composição e de som, e as
  prioridades (master, groove, dramaturgia).
- [`docs/LINHAS.md`](docs/LINHAS.md): as linhas por região de frequência
  (divisor Linkwitz-Riley de fase zero, leitura por linha e o que vale em
  cada uma; etapa 1 de 3).
- [`docs/LEITURA_E_GABARITO.md`](docs/LEITURA_E_GABARITO.md): a leitura
  das referências (pulso, tonalidade pelos acordes, separação
  harmônico/percussivo) e o gabarito do autor, com as métricas
  (`ler`, `gabarito`, `avaliar`);
- [`docs/design.md`](docs/design.md): o desenho da interface, com o mapa de
  tarefas e os regimes completo, compacto e emergência.

  O esboço visual é uma cópia de acesso em canvas privado, e a fonte é o
  documento.
- [`docs/SINTESE.md`](docs/SINTESE.md): 33 famílias de timbre e 3 salas;
- [`docs/FORMATOS_DE_AUDIO.md`](docs/FORMATOS_DE_AUDIO.md): formatos
  aceitos.
- [`docs/FORMATOS_DE_DADOS.md`](docs/FORMATOS_DE_DADOS.md): os JSON da pasta
  de dados (`decisoes.json`, `escuta.json`, o índice), campo a campo, e a
  regra de versão.


- [`docs/TRIOIO_CONCEITO.md`](docs/TRIOIO_CONCEITO.md) — identidade, escopo e limites;
- [`docs/TRIOIO_CEREBRO.md`](docs/TRIOIO_CEREBRO.md) — arquitetura de decisão e memória;
- [`docs/TRIOIO_INVENTARIO_RASGO.md`](docs/TRIOIO_INVENTARIO_RASGO.md) — candidatos de
  reutilização e fronteiras com instrumentos irmãos;
- [`TRIOIO_TAREFAS.md`](TRIOIO_TAREFAS.md) — marcos, pendências e validações;
- [`docs/BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md`](docs/BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md)
  — regras para referências, autorização, prompts e dados locais.

O core atual contém `BrainDecision`, `EnergyControlBus` e o manifesto de
proveniência (`Provenance.h`, esquema `trioio.manifesto/1`, especificado em
[`docs/MANIFESTO_DE_PROVENIENCIA.md`](docs/MANIFESTO_DE_PROVENIENCIA.md)),
com testes próprios. Marco 1 concluído em 2026-10-05. No Marco 2 já existem a
leitura e escrita de `manifesto.json` e o importador da Biblioteca
([`docs/IMPORTADOR_DA_BIBLIOTECA.md`](docs/IMPORTADOR_DA_BIBLIOTECA.md),
ferramenta `trioio_biblioteca`), a declaração de autorização e os primeiros
descritores de escuta com relatório offline
([`docs/DESCRITORES.md`](docs/DESCRITORES.md)) e a **cadeia mínima audível**:
escuta → estratégia → síntese → resposta em WAV
([`docs/CADEIA_MINIMA.md`](docs/CADEIA_MINIMA.md)). A
primeira validação de build/teste está registrada nas tarefas; novas
integrações devem preservar o contrato de realtime e a separação entre
referência autorizada, análise inferida e síntese derivada.

## Compilar e testar

```sh
cmake -S . -B build -DTRIOIO_JUCE_PATH=/caminho/para/JUCE   # sem o JUCE: só o núcleo
cmake --build build -j1
ctest --test-dir build --output-on-failure
```

O núcleo (`src/core/`) não depende de nada; a Biblioteca (`src/library/`,
leitura e escrita de `manifesto.json`) usa o `juce_core`. O JUCE não é
versionado aqui, como nos outros instrumentos RASGO.

As referências musicais ficam fora do repositório e não devem ser movidas ou
copiadas para ele.

## Próxima tarefa arquivística

Em um marco estável de release, publicação, migração ou entrega, definir o
recorte de fontes/documentação e então gerar manifesto/checksum e testar
restauração. Até lá, as tarefas e os dossiês locais registram a continuidade
sem criar pacote de preservação a cada experimento.
