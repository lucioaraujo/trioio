# TRIOIO — Biblioteca, memória e proveniência

**Estado:** especificação operacional inicial  
**Data:** 2026-08-12  
**Escopo:** acervo musical, importação, sessões, prompts, cache e aprendizagem local.

## Princípio

A Biblioteca de Música é o acervo escolhido pelo performer. Não é uma pasta de
código e não altera as faixas originais. O TRIOIO indexa referências
autorizadas, escuta relações e produz síntese própria em sessões separadas.

```text
Biblioteca externa → referência autorizada → sessão do Terceiro Olho
       ↓                      ↓                       ↓
 arquivo preservado       análise/cache       decisão + síntese + saída
```

Material sem autorização pode ser mantido em triagem, mas não entra em análise
musical, aprendizagem consolidada ou síntese derivada.

## Raízes e diretórios

O usuário pode configurar várias raízes de Biblioteca. Uma organização sugerida:

```text
TRIOIO_LIBRARY/
├── referencias/
│   ├── autorais/
│   ├── autorizadas/
│   └── geradas-autorizadas/
├── stems/
├── imports-pendentes/
└── sessoes/
    └── <session-id>/
        ├── manifesto.json
        ├── prompts.json
        ├── decisoes.json
        ├── escuta.json
        └── saidas/
```

O original permanece no local de origem. Uma cópia portátil só pode ocorrer por
ação explícita do performer e deve ser registrada como cópia de preservação.

Dados internos ficam numa raiz independente e configurável:

```text
TRIOIO_DATA/
├── indice/             # catálogo das raízes e referências
├── cache/              # waveform e análise reconstruíveis
├── memoria/
│   ├── sessoes/        # relações e decisões consolidadas
│   └── preferencias/   # escolhas e tendências locais
└── logs/               # auditoria técnica
```

## Estados e velocidade de importação

| Estado | Significado |
| --- | --- |
| `pending_authorization` | Arquivo encontrado, uso ainda não confirmado. |
| `authorized` | Pode ser indexado e usado em sessão. |
| `indexing` | Identidade técnica e waveform sendo preparados. |
| `available` | Pronto para iniciar o jogo composicional. |
| `analyzing` | Análise profunda em curso; resposta curta é possível. |
| `cached` | Mapa reutilizável disponível. |
| `changed` | Arquivo mudou; cache invalidado. |
| `unavailable` | Arquivo ausente/ilegível; histórico preservado. |

O drop não bloqueia a síntese:

```text
drop → identificação técnica → janela curta → BrainDecision → síntese
                                         ↓
                        waveform e análise profunda em segundo plano
```

- `RESPONDER AGORA`: começa pela janela curta e descobre a referência em curso.
- `LER ANTES`: espera o mapa completo para responder à forma global.
- `HÍBRIDO`: começa imediatamente e reorganiza a estratégia ao reconhecer novas seções.

Decodificação, indexação, cache, persistência e análise pesada nunca rodam no
callback de áudio; ele recebe somente `BrainDecision` leves e coerentes.

## Identidade, cache e manifestos

A referência combina caminho canônico, tamanho, data de modificação, duração,
taxa, canais, formato e, quando necessário, assinatura de conteúdo calculada
fora do áudio. Divergência torna o item `changed` e invalida seu cache.

Todo documento é versionado e traz `schemaVersion`, `id` e datas. Os formatos
iniciais são:

- `manifesto.json`: origem, autorização, identidade técnica e estado de análise
  — definido em [`MANIFESTO_DE_PROVENIENCIA.md`](MANIFESTO_DE_PROVENIENCIA.md)
  (`trioio.manifesto/1`);
- `prompts.json`: `promptOriginal`, `declaredIntent`, `analysisInferred` e revisão da tradução;
- `decisoes.json`: seed, modo, estratégia e trajetória de `BrainDecision`;
- `escuta.json`: avaliações humanas de aceitação, rejeição ou transformação.

`promptOriginal` só existe quando foi fornecido. `declaredIntent` é a tradução
editável para o vocabulário TRIOIO. `analysisInferred` descreve a escuta do
áudio. As três camadas nunca se substituem nem alegam recuperar parâmetros
internos de outra ferramenta.

## Memória e aprendizagem local

O TRIOIO aprende relações e preferências locais, não copia a forma de onda nem
treina automaticamente um serviço externo. A persistência possui quatro níveis:

1. cache técnico reconstruível;
2. memória de sessão: referência, prompts, análise, decisão, seed, saída e escuta;
3. memória musical consolidada: relações recorrentes, estratégias e confiança;
4. preferências explícitas: aceitar, rejeitar, preservar, esquecer ou contrariar.

Aceitações e rejeições ajustam tendências revisáveis, nunca criam regras fixas.
O performer pode apagar sessões, limpar cache, editar/remover prompts,
invalidar tendências, limitar memória a uma Biblioteca ou desligar aprendizagem.
Persistir, consolidar e esquecer é sempre trabalho fora da thread de áudio.

## Primeiro marco implementável

1. Selecionar uma raiz e indexar WAV autorizado.
2. Criar `manifesto.json` e `prompts.json` mínimos.
3. Gerar waveform e descritores de janela curta numa thread de trabalho.
4. Publicar a primeira `BrainDecision` em `RESPONDER AGORA`.
5. Persistir seed, estratégia e saída em `decisoes.json`.
6. Registrar aceitação/rejeição em `escuta.json`.
7. Reabrir a referência e reutilizar cache corretamente.
8. Alterar o arquivo e demonstrar invalidação segura do cache.

O marco se encerra quando a cadeia completa puder ser escutada e auditada:
referência autorizada → prompt/escuta → decisão → síntese → saída → avaliação
humana → tendência local revisável.
