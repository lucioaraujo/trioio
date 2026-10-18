# TRIOIO — importador da Biblioteca

**Estado:** implementado em 2026-10-05 (`src/library/LibraryImporter.h/.cpp`,
ferramenta `trioio_biblioteca`, teste `tests/LibraryImporterTests.cpp`)
**Depende de:** [`MANIFESTO_DE_PROVENIENCIA.md`](MANIFESTO_DE_PROVENIENCIA.md)

## O que faz

Varre uma pasta escolhida pelo performer (a Biblioteca) e mantém **um
manifesto por material**: áudio (WAV, AIFF, FLAC, Ogg, Opus, MP3 e, com o
ffmpeg, M4A/AAC, WebM, WMA e outros; o formato é lido pelo conteúdo, ver
[`FORMATOS_DE_AUDIO.md`](FORMATOS_DE_AUDIO.md)), stem
(qualquer áudio dentro de uma pasta `stems/`) e MIDI (`mid`, `midi`).

```sh
trioio_biblioteca varrer <biblioteca> <dados>   # registra e atualiza
trioio_biblioteca listar <dados>                # mostra o que está registrado
```

## Declarar origem e autorização

O material sai de `pending` por uma declaração do performer
(`src/library/Declaration.h/.cpp`):

```sh
trioio_biblioteca autorizar <dados> <id> --origem authorial --fundamento ownWork \
    --usos derive --por "Nome"
trioio_biblioteca autorizar <dados> <id> --origem aiGenerated --fundamento serviceTerms \
    --ferramenta "serviço X" --evidencia "termos de 2026-09-30, docs-locais/termos-x.pdf" \
    --usos analyze,learn --por "Nome"
trioio_biblioteca recusar <dados> <id> --por "Nome" --motivo "não é meu"
trioio_biblioteca revogar <dados> <id> --por "Nome" --motivo "..."
```

- Os nomes são os do JSON. Os usos fecham pela cadeia: `--usos derive`
  grava `listen`, `analyze` e `derive`.
- **Nada é gravado** se o resultado não passar em `validate()`. A ferramenta
  lista o que falta (quem declarou, a evidência, o fundamento compatível
  com a origem...), e o arquivo em disco fica como estava.
- **Recusar** vale só para o que está pendente. **Revogar** vale só para o
  que está autorizado. Recusado ou revogado pode voltar a ser autorizado, e
  isso fica no histórico como "autorização redeclarada".

## Garantias

- **A Biblioteca só é lida.** Os manifestos vão para
  `<dados>/indice/referencias/<id>.json`. Se a pasta de dados estiver dentro
  da Biblioteca, a varredura é recusada.
- **Todo material novo entra pendente**, com a origem "não verificada". O
  importador nunca presume autoria nem autorização: quem declara é o
  performer. Enquanto está pendente, o material pode ser ouvido e nada mais.
- **A identidade é o conteúdo.** O `id` é `ref-` seguido dos 16 primeiros
  dígitos do SHA-256 do arquivo, e o hash completo fica em
  `identity.contentSha256`. Por isso:
  - **mover ou renomear** atualiza o caminho, e a autorização acompanha o
    material;
  - **colocar outro conteúdo no mesmo caminho** não herda nada. O manifesto
    antigo vira `unavailable` com o evento "conteúdo substituído", e o novo
    entra pendente com a relação `version_of` apontando para o antigo;
  - **cópias idênticas** são contadas e registradas uma vez só no histórico
    do original, sem criar um manifesto novo.
- **O que some** vira `unavailable` (nem ouvir), com o evento "ausente".
  Se voltar, é "reencontrado". Nada é apagado: o histórico só cresce.
- **Manifesto ilegível** (editado à mão e quebrado, por exemplo) é relatado
  e **nunca sobrescrito**. O arquivo correspondente é pulado até que alguém
  conserte o manifesto.
- **Gravação atômica.** Cada manifesto é escrito num arquivo temporário e
  depois trocado pelo definitivo. Um manifesto sem mudança não é regravado.
- **Fica fora da varredura:** arquivos ocultos e a pasta `sessoes/`, porque
  as saídas do próprio TRIOIO são registradas pela sessão, como derivações.
- **Limites:** até 20 000 arquivos por varredura e 4 GB por arquivo
  (configuráveis em `ImportLimits`). Há cancelamento entre arquivos, e uma
  varredura cancelada não grava nada. O hash é calculado em blocos, sem
  carregar o arquivo inteiro na memória. Tudo roda fora da thread de áudio.

## Metadados

Os metadados que o leitor de áudio do JUCE encontra no arquivo (cabeçalhos
BWF, listas INFO e afins) vão para `metadataImported` como vieram, sem
edição. Duração, taxa e canais vão para `identity`. No MIDI, a duração vem
do último evento.

## Validação (2026-10-05)

- **Teste automático, com uma Biblioteca gerada na hora:** 4 materiais
  registrados e 1 arquivo quebrado pulado. Mover com renomeação preserva a
  autorização. A cópia idêntica é registrada uma única vez. Ausente e
  reencontrado funcionam. A substituição do conteúdo não herda a
  autorização e cria `version_of`. A pasta de dados dentro da Biblioteca é
  recusada. O manifesto ilegível é preservado. O cancelamento não grava
  nada. O limite de tamanho é respeitado. A Biblioteca é conferida por hash
  e data antes e depois e sai intacta.
- **Mutações:** permitir a pasta de dados dentro da Biblioteca e não
  atualizar o caminho ao mover fazem o teste falhar.
- **Declaração** (`tests/DeclarationTests.cpp`, 9 cenários): uma declaração
  incompleta não grava nada; o escopo fecha pela cadeia; revogar e recusar
  seguem as regras; material de IA sem evidência é barrado. Desligar a
  validação antes de gravar faz 7 checagens falharem. Ponta a ponta pela
  ferramenta: varrer, depois autorizar sem `--por` (barrado, nada gravado),
  depois autorizar completo e listar.
- **Pasta real, só leitura** (`/home/luc/Music/Rasgo Synth`, 720 MB, 62
  arquivos):
  - os 21 WAV foram registrados em 25 s, e os 41 textos e JSON foram
    ignorados;
  - a segunda passada não mudou nada;
  - a pasta ficou intacta (lista de caminhos, tamanhos e datas idêntica);
  - os manifestos foram gravados no rascunho da sessão e apagados depois.

## Próximos passos

- **Descritores mínimos** (energia, densidade, ataques, espectro), só para
  material com `analyze` liberado, com cache ligado ao `contentSha256`.

## Um arquivo só (2026-10-13)

`scanLibrary` aceita um arquivo como Biblioteca. É o "trazer gravação…" da
interface e o `trioio_biblioteca varrer <arquivo> <dados>`.

- Registra só aquele arquivo.
- A pasta vizinha não é varrida, e nenhum material de outras varreduras é
  dado como ausente.
- A raiz de dados pode ficar ao lado do arquivo.
- Um arquivo que não é áudio nem MIDI é pulado, sem gravar nada.
- Teste: bloco 11 de `tests/LibraryImporterTests.cpp`.

