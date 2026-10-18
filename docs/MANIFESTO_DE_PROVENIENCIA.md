# TRIOIO — manifesto de proveniência (versão 1)

**Estado:** definido em 2026-10-05; contrato executável em
`src/core/Provenance.h/.cpp`, testado em `tests/ProvenanceTests.cpp`
**Esquema:** `trioio.manifesto/1`
**Fecha:** Marco 1, último item ("manifesto de proveniência para áudio,
metadados, prompt original, análise inferida e autorização de uso")

## 1. Para que serve

Cada material que entra no TRIOIO (áudio, stem ou MIDI) ganha um manifesto, e
cada saída derivada também. O manifesto responde a três perguntas, nesta
ordem:

1. **O que é e de onde veio.** Identidade técnica do arquivo, origem, autoria
   e metadados encontrados.
2. **O que pode ser feito com ele.** A autorização: fundamento, evidência e
   quais usos estão liberados.
3. **O que o TRIOIO fez com ele.** O estado da análise, as relações com
   outras versões e, nas derivações, a sessão, o seed, a estratégia e a
   relação declarada com a referência.

As quatro camadas definidas no conceito ficam separadas e nenhuma substitui
outra:

| Camada | Onde fica |
| --- | --- |
| `audioSource` | `identity`: o arquivo efetivamente recebido |
| `metadataImported` | `metadataImported`: como veio do arquivo, sem edição |
| `promptOriginal` | `prompts.json`; o manifesto guarda só o vínculo (`promptsRef`) |
| `analysisInferred` | `analysis` (estado) e o cache; nunca no lugar dos dados importados |

O manifesto é um registro de decisões, não uma cópia: não guarda áudio, e o
original nunca é movido, renomeado ou reescrito pelo TRIOIO.

## 2. Usos e o portão

A autorização libera usos, do mais leve ao mais comprometedor:

| Uso | O que libera |
| --- | --- |
| `listen` | tocar e ver a forma de onda |
| `analyze` | descritores, mapa de relações, cache |
| `learn` | entrar na memória consolidada e nas tendências locais |
| `derive` | servir de referência para síntese derivada |
| `publishDerivative` | publicar uma saída derivada dela |

Os usos formam uma cadeia: `learn` e `derive` exigem `analyze`, `analyze`
exige `listen`, e `publishDerivative` exige `derive`. Um escopo que pule um
elo é inválido. Assim, "pode publicar mas não pode analisar" não é algo que
o manifesto consiga dizer.

O portão (`permits`) libera um uso a uma referência autorizada apenas quando
**todas** as condições a seguir valem:

- o manifesto é válido (§5);
- a autorização está `authorized`;
- o arquivo não está `unavailable`;
- o uso está no escopo;
- se o arquivo mudou desde a análise (`changed`), só `listen` e `analyze`
  passam: nada pode se apoiar num mapa velho até que o arquivo seja
  reanalisado.

`pending` libera só `listen`: o material em triagem pode ser ouvido, porque é
ouvindo que o performer decide se autoriza. Ele não é analisado, não entra na
memória e não serve de referência. `refused` e `revoked` não liberam nada, nem
mesmo ouvir.

Uma saída derivada só é publicável (`canPublishDerivation`) se a própria
derivação for válida e **todas** as referências de que ela partiu
permitirem `publishDerivative`. Se uma fonte for revogada depois, a derivação
deixa de ser publicável; o arquivo dela continua existindo, pois o TRIOIO
não apaga nada sozinho.

## 3. Origem e fundamento

| `origin.category` | Fundamentos aceitos para autorizar |
| --- | --- |
| `authorial`: obra do performer | `ownWork` |
| `performerCapture`: gravação ou microfone do performer | `ownWork` (ou licença/permissão, se capturou obra alheia) |
| `otherInstrument`: saída de outro instrumento, inclusive do RASGO | `ownWork`, licença ou permissão; nomear o instrumento |
| `aiGenerated`: gerado por ferramenta de IA | `serviceTerms`, `license` ou `writtenPermission`; nunca `ownWork`; nomear a ferramenta |
| `licensed`: de terceiros, sob licença | `license` (com a licença e a evidência) |
| `publicDomain` | `publicDomain` |
| `thirdPartyUnverified`: chegou sem origem conhecida | **nenhum**: fica `pending` até a origem ser esclarecida |

Sobre material gerado por IA: os direitos sobre essas saídas variam conforme
o serviço e o país, e mudam com o tempo. Por isso o manifesto não decide se
há direito autoral. Registra **em que** o performer se apoia (os termos do
serviço na data da geração) e **onde** está a prova (`evidence`). Isso faz
de "gerado por IA" uma declaração verificável, e não uma presunção.

`license`, `writtenPermission` e `serviceTerms` exigem `evidence`: um
caminho, uma URL ou a descrição do documento. `license` exige também a
própria licença (SPDX quando houver, como `CC-BY-4.0`).

## 4. Formato (`manifesto.json`)

Exemplo de referência: uma versão gerada por IA, autorizada para análise e
derivação, mas não para publicação.

```json
{
  "schemaVersion": "trioio.manifesto/1",
  "id": "ref-2026-10-05-0007",
  "createdAt": "2026-10-05T10:00:00Z",
  "updatedAt": "2026-10-05T10:00:00Z",
  "kind": "reference",
  "material": "audio",
  "identity": {
    "canonicalPath": "/home/performer/TRIOIO_LIBRARY/referencias/geradas-autorizadas/versao-3.wav",
    "sizeBytes": 10584044,
    "modifiedAt": "2026-09-30T21:14:00Z",
    "format": "wav",
    "durationSeconds": 60.0,
    "sampleRate": 44100,
    "channels": 2,
    "contentSha256": "9f2c…"
  },
  "origin": {
    "category": "aiGenerated",
    "authors": [{ "name": "Performer", "role": "autor do prompt e da seleção" }],
    "toolName": "serviço de geração X",
    "toolVersion": "v4",
    "createdAt": "2026-09-30",
    "notes": "terceira de cinco versões do mesmo prompt"
  },
  "authorization": {
    "status": "authorized",
    "basis": "serviceTerms",
    "scope": ["listen", "analyze", "learn", "derive"],
    "license": "",
    "evidence": "termos do serviço em 2026-09-30, cópia local em docs-locais/termos-x.pdf",
    "declaredBy": "Performer",
    "declaredAt": "2026-10-05T10:00:00Z",
    "restrictions": "não publicar derivações antes de reler os termos"
  },
  "metadataImported": { "title": "versao 3", "encoder": "…" },
  "promptOriginalProvided": true,
  "promptsRef": "prompts.json#ref-2026-10-05-0007",
  "analysis": "cached",
  "relations": [
    { "type": "version_of", "target": "ref-2026-10-05-0005" },
    { "type": "same_prompt_as", "target": "ref-2026-10-05-0006" }
  ],
  "history": [
    { "at": "2026-10-05T09:58:00Z", "event": "registrado", "by": "Performer", "note": "drop em imports-pendentes" },
    { "at": "2026-10-05T10:00:00Z", "event": "autorizado", "by": "Performer", "note": "" }
  ]
}
```

Uma derivação acrescenta `"kind": "derivation"` e o bloco:

```json
"derivation": {
  "from": ["ref-2026-10-05-0007"],
  "sessionId": "sessao-2026-10-05-01",
  "decisionsRef": "decisoes.json#12",
  "seed": "42",
  "strategy": "transfigure",
  "declaredRelation": "mantém a curva de energia, troca toda a matéria sonora",
  "trioioVersion": "0.1.0"
}
```

Valores fechados:

- **`kind`:** `reference`, `derivation`.
- **`material`:** `audio`, `stem`, `midi`.
- **`analysis`:** `none`, `indexing`, `analyzing`, `cached`, `changed`, `unavailable`.
- **`authorization.status`:** `pending`, `authorized`, `refused`, `revoked`.
- **`strategy`:** `follow`, `transfigure`, `counter`, `interrupt`, isto é, seguir, transfigurar, contrapor e interromper.
- **`relations[].type`:** `version_of`, `stem_of`, `midi_of`, `same_prompt_as`, `derived_from`.

Os oito estados da Biblioteca (`docs/BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md`)
vêm da combinação de dois campos que aqui ficam separados:
`pending_authorization` e `authorized` vêm de `authorization.status`; os
demais (`indexing`, `available`, `analyzing`, `cached`, `changed` e
`unavailable`) vêm de `analysis`. Um arquivo pode estar autorizado e ausente,
ou presente e sem autorização. Juntar os dois num campo só esconderia um
desses casos.

## 5. Regras de validade

Um manifesto inválido não entra em uso nenhum. `validate()` aponta, campo a
campo, o que falta:

1. `schemaVersion` conhecido, `id` e `createdAt` presentes.
2. Referência tem `identity.canonicalPath` e `sizeBytes`.
3. `aiGenerated` e `otherInstrument` nomeiam a ferramenta (`toolName`).
4. O escopo respeita a cadeia de usos (§2).
5. Uma autorização `authorized` exige:
   - fundamento, escopo não vazio, quem declarou e quando;
   - origem que não seja `thirdPartyUnverified`;
   - fundamento compatível com a origem (§3);
   - evidência e licença onde o fundamento as exige.
6. `promptOriginalProvided` exige `promptsRef`.
7. Derivação exige `from`, `sessionId` e `declaredRelation`. Uma
   referência não carrega o bloco `derivation`.
8. `history` nunca é vazio, porque o registro já é o primeiro evento, e só
   cresce: revogar acrescenta um evento e não apaga os anteriores.

## 6. Leitura e escrita

`src/library/ManifestJson.h/.cpp` (2026-10-05) lê e grava este formato
usando o `juce::JSON`, do próprio JUCE e sob AGPLv3. Nenhuma biblioteca de
terceiros foi acrescentada. Regras da leitura:

- **Valor desconhecido** num campo fechado (categoria, estado, uso,
  estratégia) é erro e não é ignorado, porque um manifesto mal lido não pode
  virar autorização.
- **Outra versão de esquema** não é lida campo a campo.
- **O `seed` é gravado como texto.** O número do JSON é um double e não
  guarda 64 bits com exatidão em outros leitores.
- **Ler não é validar.** Um manifesto pendente e incompleto pode ser gravado
  e relido durante a triagem. A coerência das regras é verificada à parte,
  por `validate()`.

O teste `tests/ManifestJsonTests.cpp` lê o exemplo da §4 **direto deste
documento**: se a especificação e o formato divergirem, o teste falha.

## 7. O que fica para o Marco 2
- **`prompts.json`, `decisoes.json` e `escuta.json`:** seguem o mesmo
  padrão (`schemaVersion`, `id`, datas e histórico).
- **Identidade estável para o cache:** a regra de quando calcular
  `contentSha256`, sempre fora da thread de áudio.
- **O importador:** feito em 2026-10-05. Ver
  [`IMPORTADOR_DA_BIBLIOTECA.md`](IMPORTADOR_DA_BIBLIOTECA.md). Ele cria o
  manifesto `pending` e usa o conteúdo como identidade. Falta declarar a
  autorização pela ferramenta.
