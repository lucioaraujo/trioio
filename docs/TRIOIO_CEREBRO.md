# TRIOIO — Cérebro do Terceiro Olho

**Estado:** conceito e arquitetura inicial  
**Data:** 2026-08-12  
**Projeto:** RASGO  
**Identidade:** Vitoriamario

## 1. Função

O Cérebro é a camada que transforma escuta em decisão musical. Ele não é um
chatbot, um sequenciador convencional nem um gerador que substitui a síntese.
É o sistema que observa relações, mantém contexto, formula uma estratégia e
publica intenções para os motores de energia.

```text
escuta → interpretação → memória → estratégia → decisão → síntese
```

O Cérebro deve permitir que o TRIOIO:

- funcione sem MIDI;
- escute áudio e prompt;
- reconheça mudanças de energia e comportamento;
- mantenha memória sem repetir mecanicamente;
- proponha ou aceite regras de jogo;
- desvie de uma referência sem perder coerência;
- traduza intenção conceitual em parâmetros sonoros;
- continue operando mesmo sem uma IA externa.

## 2. Terceiro Olho

O Terceiro Olho é o princípio de organização do Cérebro. Ele percebe três
níveis ao mesmo tempo:

1. o evento sonoro presente;
2. o contexto e a trajetória que produziram o evento;
3. a regra ou possibilidade que pode ser alterada.

Ele não é um personagem separado. É a capacidade de Vitoriamario de participar,
observar e transformar o jogo.

## 3. Camadas

### 3.1 Percepção

Recebe áudio, prompt, gesto, MIDI opcional e telemetria da própria síntese.
Extrai descritores sem tomar decisões musicais finais:

- energia e dinâmica;
- transientes e ataques;
- frequência fundamental provável;
- densidade e atividade;
- centroide e espalhamento espectral;
- harmonicidade e ruído;
- pulsação e periodicidade;
- largura, movimento e profundidade estéreo;
- estabilidade, repetição e novidade.

### 3.2 Contexto

Organiza os descritores em uma janela temporal. O Cérebro deve compreender
trajetórias, não apenas valores instantâneos.

O contexto inclui:

- memória curta de eventos recentes;
- memória média de padrões recorrentes;
- estado atual do jogo;
- intenção do performer;
- estado dos organismos e motores de energia;
- grau de previsibilidade;
- regras explícitas e regras ocultas ativas.

### 3.3 Memória

A memória não é um gravador integral. Ela armazena assinaturas, relações,
decisões e consequências:

```cpp
struct MusicalMemory
{
    float energyProfile;
    float densityProfile;
    float spectralProfile;
    float gestureProfile;
    float coherence;
    float novelty;
    float responseAccepted;
    uint64_t seed;
};
```

O sistema deve lembrar sem congelar a música. Memória pode reforçar, deformar,
contrastar ou ser deliberadamente esquecida.

### 3.4 Estratégia

A estratégia decide como o sistema irá agir sobre o contexto. Ela pode:

- seguir;
- acompanhar com atraso;
- condensar;
- expandir;
- trocar o material;
- preservar energia e mudar timbre;
- preservar gesto e negar tonalidade;
- repetir com diferença;
- ocultar uma transformação;
- interromper e abrir espaço;
- mudar a regra do jogo.

Estratégia não é uma lista fixa de presets. É uma hipótese temporária que pode
ser confirmada, abandonada ou contradita pelo próximo evento.

### 3.5 Decisão

A decisão transforma a estratégia em intenção musical, sem comandar amostras
diretamente.

```cpp
struct BrainDecision
{
    float density;
    float energy;
    float harmonicity;
    float brightness;
    float transientRetention;
    float memoryAmount;
    float mutation;
    float spatialActivity;
    float autonomy;
    float confidence;
};
```

Os valores são alvos suavizados e limitados. A camada de síntese decide como
realizá-los dentro das características de cada motor.

## 4. Prompt como intenção

O Cérebro pode receber prompts em linguagem natural, vocabulário controlado,
gestos ou combinações desses sinais. O prompt não deve virar controle direto de
frequência.

Exemplos:

- “preservar os ataques e corroer o corpo”;
- “aumentar a tensão sem aumentar o volume”;
- “lembrar de forma incompleta”;
- “seguir a pulsação, mas negar a tonalidade”;
- “deixar o sistema descobrir uma saída”.

O interpretador converte essas frases em intenções intermediárias. Um LLM
externo poderá auxiliar, mas nunca será obrigatório.

### Proveniência da referência

O Cérebro deve distinguir o que veio do arquivo do que foi inferido. A entrada
pode ser uma gravação própria, áudio licenciado, síntese de outro instrumento,
obra gerada por IA, stem, MIDI ou captura do performer.

```text
audioSource       = áudio recebido
metadataImported  = tags e dados técnicos encontrados
promptOriginal    = prompt fornecido pelo usuário/manifesto
analysisInferred  = interpretação produzida pelo TRIOIO
```

Um prompt ausente nunca será tratado como prompt recuperado. O Cérebro pode
inferir uma descrição musical a partir do áudio, mas deve marcá-la como
inferência e preservar a distinção entre fato, dado importado e interpretação.

Quando o usuário fornecer um prompt de origem externa, ele será uma
`declaredIntent`, não uma
janela para os parâmetros internos do serviço. O Cérebro poderá comparar a
intenção com a análise acústica:

```text
declaredIntent × observedAudio
```

Essa discrepância será uma entrada estratégica: seguir a intenção, corrigi-la,
contradizê-la ou transformá-la em uma regra do jogo.

Um conjunto autorizado de versões geradas por assistentes de composição pode ser
recebido como um arquivo de possibilidades. O Cérebro poderá comparar versões,
extrair operações recorrentes, identificar contradições e recombinar intenções
em uma decisão própria. Cada versão preserva sua proveniência; o Cérebro não
copia o modelo que a produziu.

### Referência como início do jogo composicional

Uma versão musical fornecida pelo usuário, acompanhada de seu prompt de origem
quando disponível, pode iniciar uma sessão do TRIOIO. O arquivo não entra como
modelo a ser reproduzido: ele é uma referência situada, entregue para que o
Terceiro Olho escute suas relações e proponha uma consequência sonora própria.

```text
versão autorizada + prompt de origem + metadados/autorização
                         ↓
                 acervo de referência
                         ↓
      escuta de relações e estratégia do Terceiro Olho
                         ↓
              síntese energética própria do TRIOIO
                         ↓
          saída derivada + decisão e proveniência registradas
```

O prompt fornecido é a `declaredIntent`: aquilo que se pediu à ferramenta de
origem. A versão de áudio é `observedAudio`: aquilo que de fato ocorreu. A
leitura do TRIOIO é `analysisInferred`: sua interpretação musical explícita e
separada das duas anteriores. O instrumento nunca afirma recuperar parâmetros
internos, prompts ausentes ou o processo secreto de outra ferramenta.

O jogo começa, em especial, na distância entre intenção declarada e som
observado. Um pedido de rarefação pode ter produzido muitos ataques; uma
intenção estável pode revelar harmonia instável e energia crescente. Essa
diferença permite ao TRIOIO preservar uma relação e transformar outra: manter
energia e negar timbre, reter ataques e deslocar tonalidade, conservar a forma
temporal e criar matéria inteiramente nova. A estratégia escolhida — seguir,
transfigurar, contrapor ou interromper — e sua consequência deverão acompanhar
a saída no registro da sessão.

O manifesto de cada referência e derivação deverá guardar, no mínimo:

- arquivo de áudio e identificação da versão;
- prompt de origem, quando fornecido;
- autoria/origem e autorização declarada;
- metadados técnicos importados;
- análise inferida pelo TRIOIO;
- decisões estratégicas e seed;
- saída derivada e sua relação declarada com a referência.

### Biblioteca de música e sessões

O TRIOIO terá uma Biblioteca de Música: um conjunto de diretórios de acervo
escolhidos pelo usuário, distinto dos diretórios de código do instrumento. A
Biblioteca carrega referências sem movê-las, renomeá-las ou sobrescrevê-las. A
sessão é o espaço em que o Terceiro Olho escuta, decide e cria derivações.

```text
TRIOIO_LIBRARY/
├── referencias/
│   ├── autorais/
│   ├── autorizadas/
│   └── geradas-autorizadas/
├── stems/
├── sessoes/
│   └── <sessão>/
│       ├── manifesto.json
│       ├── decisoes.json
│       └── saidas/
└── imports-pendentes/
```

A superfície `LIBRARY` permitirá adicionar diretórios, indexar formatos de
áudio suportados em thread de trabalho e mostrar a identidade técnica de cada
faixa — duração, canais, taxa, origem e estado de autorização. Prompt e
metadados poderão acompanhar a referência em manifesto associado. Uma mesma
faixa pode retornar em sessões distintas, cada qual preservando suas decisões,
seed e saídas, sem apagar a origem.

### Leitura progressiva e resposta rápida

O TRIOIO não precisa esperar a análise integral de uma faixa para começar a
compor. O drop deve preparar a identidade técnica e a leitura do arquivo para a
interface; uma janela curta de escuta pode produzir a primeira decisão; a
análise profunda continua em segundo plano e atualiza contexto e estratégia sem
interromper a síntese.

```text
soltar arquivo na Biblioteca
          ↓
identidade técnica, waveform e cache
          ↓
janela curta de escuta → primeira BrainDecision → síntese
          ↓
análise contínua de relações e memória
          ↓
revisão progressiva da estratégia e da saída
```

O instrumento oferecerá três modos de trabalho:

- `RESPONDER AGORA`: inicia a síntese a partir da leitura do presente e
  descobre a referência enquanto ela avança;
- `LER ANTES`: analisa a obra completa para criar uma resposta orientada por
  sua forma global;
- `HÍBRIDO`: começa vivo e revisa sua estratégia à medida que reconhece novas
  seções, recorrências e mudanças.

Decodificação completa, descritores custosos e comparação entre versões não
rodam na thread de áudio. O áudio recebe somente `BrainDecision` leves,
versionadas e contínuas. Mapas de análise e waveforms podem ser mantidos em
cache para que uma referência já conhecida retorne à sessão com menor espera.

### Armazenamento, memória, aprendizagem e prompts

O acervo sustenta memória e aprendizagem local. O TRIOIO não aprende uma faixa
por cópia de áudio nem treina automaticamente um modelo externo: conserva
registros compactos, auditáveis e removíveis das relações percebidas e das
escolhas do performer.

```text
referência autorizada + prompt declarado
                 ↓
descritores + mapa de relações + proveniência
                 ↓
sessão: estratégia, BrainDecision, seed, saída e escuta anotada
                 ↓
memória musical local e tendências revisáveis
                 ↓
decisão futura, sempre contestável pelo performer
```

Há quatro camadas distintas de persistência:

- **cache técnico reconstruível**: waveform, descritores e mapas temporais;
- **memória de sessão**: referência, prompt, intenção traduzida, análise,
  estratégia, seed, parâmetros, saída e anotações de escuta;
- **memória musical consolidada**: relações recorrentes, estratégias eficazes,
  preferências e graus de confiança;
- **preferências explícitas do performer**: aceitar, rejeitar, preservar,
  esquecer ou contrariar uma resposta.

O prompt tem seu próprio ciclo de vida. `promptOriginal` preserva o texto
fornecido; `declaredIntent` é sua tradução para o vocabulário interno do
TRIOIO; `analysisInferred` descreve somente o que foi inferido do som. Nenhuma
dessas camadas substitui a outra. Uma sessão também registra que prompt ou
intenção levou a cada decisão, permitindo comparar versões e perceber como uma
instrução foi transformada musicalmente.

Aprender significa ajustar tendências locais, nunca fixar uma regra ou alegar
compreender a ferramenta de origem. O performer pode apagar uma sessão,
invalidar seu aprendizado, limpar cache, editar/remover um prompt ou limitar a
memória a determinada Biblioteca. Persistência, consolidação e esquecimento
ocorrem fora do callback de áudio; a síntese lê somente a decisão atual.

Para o acervo autoral do usuário, o fluxo poderá preservar a cadeia de criação:
áudio próprio, prompt opcional, metadados, stems, MIDI e versões derivadas. A
autorização deve acompanhar o manifesto. Conteúdo de terceiros fica fora do
escopo. Nenhum serviço externo é requisito do Cérebro.

## 5. Modos de operação

### Autônomo

O Cérebro trabalha a partir de energia, memória, acaso determinístico e regras
do próprio instrumento.

### Performance

O performer conduz o sistema por gestos, áudio, prompt e controles. A resposta
deve ser imediata, mas não necessariamente literal.

### Híbrido

O sistema negocia entre intenção autônoma e intervenção humana. O parâmetro
`autonomy` deve ser contínuo e suavizado.

### Escuta e resposta

O instrumento analisa uma referência e escolhe entre reconstruir, transfigurar,
contrapor ou interromper a trajetória percebida.

## 6. Jogo

O Cérebro mantém recursos e regras do jogo:

- energia disponível;
- memória utilizável;
- previsibilidade;
- risco de saturação;
- grau de liberdade;
- informação ainda oculta;
- confiança na interpretação.

O Terceiro Olho pode revelar, proteger, substituir ou quebrar uma regra. A
trapaça é uma possibilidade expressiva, não o objetivo único.

## 7. Barramento de energia

O Cérebro não deve conhecer detalhes específicos de cada oscilador. Publica um
contrato comum para os motores:

```text
BrainDecision
        ↓
EnergyControlBus
        ↓
OrganismVoiceEngine / síntese própria
```

O barramento pode carregar:

- energia;
- densidade;
- brilho;
- harmonicidade;
- rugosidade;
- dureza;
- duração;
- memória;
- mutação;
- espacialidade;
- intenção de ataque;
- grau de silêncio.

Cada espécie interpreta esses campos de modo próprio. O mesmo aumento de
`tension` pode abrir um banco modal, aumentar partículas granulares, alterar a
excitação de uma corda ou deslocar o feedback de um ressonador.

## 8. Tempo real e threads

O Cérebro completo não roda no callback de áudio.

### Thread de áudio

- lê snapshots atômicos;
- aplica alvos já calculados;
- suaviza parâmetros;
- executa síntese fixa;
- não aloca, bloqueia, faz I/O ou interpreta texto.

### Thread de análise

- calcula descritores de áudio;
- atualiza contexto e memória;
- avalia novidade e trajetória;
- publica snapshots.

### Thread de estratégia

- interpreta prompt;
- escolhe regras e modos;
- calcula decisões de mais alto nível;
- pode usar modelos externos ou processos mais pesados.

### GUI

- mostra estado e telemetria;
- envia intenção por atomics e filas fixas;
- nunca acessa diretamente o estado mutável do Cérebro.

## 9. Determinismo e acaso

O acaso deve ser uma fonte controlada de descoberta, não um comportamento
impossível de reproduzir. O Cérebro usará seed explícita, subfluxos separados
por função e registro dos eventos estratégicos.

Uma mesma sessão pode ser reproduzida quando seed, entrada e decisões forem
iguais. A opção de mutação deve ser explícita e auditável.

## 10. Aprendizado

A primeira versão usará adaptação local, não treinamento de uma grande rede:

- atualizar preferências a partir de escolhas do performer;
- registrar respostas aceitas ou rejeitadas;
- reforçar ou enfraquecer estratégias;
- comparar assinatura de referência e resultado;
- esquecer gradualmente para evitar cristalização.

O aprendizado deve alterar tendências, não aprisionar o instrumento em um perfil
de gosto.

## 11. Critérios de excelência

O Cérebro só será considerado funcional quando demonstrar:

- decisões audíveis e não apenas números na interface;
- diferença clara entre seguir, transformar e contrariar;
- memória sem repetição mecânica;
- resposta coerente a áudio, prompt e gesto combinados;
- funcionamento autônomo sem MIDI;
- estabilidade numérica e ausência de alocação no áudio;
- custo de CPU e memória medidos por modo;
- gravação de alta qualidade e renders reproduzíveis;
- escuta humana documentada;
- relação verificável entre conceito, decisão e consequência sonora.

## 12. Próxima etapa

Antes do código, definir o primeiro vocabulário de intenções e construir um
protótipo offline mínimo:

```text
áudio + prompt → descritores → BrainDecision → síntese de energia
```

O protótipo deverá produzir relatório de decisões e render A/B para que a
escuta avalie não somente se o resultado é agradável, mas se o Cérebro realmente
compreendeu e transformou a relação musical investigada.

## 13. Referências

- Magenta RealTime 2, documentação oficial: contexto de áudio, texto/MusicCoCa,
  MIDI e geração frame a frame — referência de interação multimodal.
- Magenta DDSP: análise-resíntese por DSP interpretável — referência para
  separar decisão musical de realização sonora.
- `AQUORBIUM/src/core/BiomaBrain.*`, `AutonomousEnergy.*`,
  `OrganismVoiceEngine.*` e `FixedMpscQueue.h` — código próprio consultado para
  determinismo, energia e segurança de thread.
- `RASGO_SYNTH/rasgo-synth-core/src` — código próprio consultado para síntese,
  composição generativa e memória de render.
- `NAVALHA2_JUCE/src/core` — código próprio consultado para gravação, saída
  segura e validação de WAV.

Links externos e distinção de licenças devem acompanhar qualquer futura adoção
de um componente externo. Nenhuma referência autoriza copiar sua implementação.

O Cérebro só processa conteúdo livre ou expressamente autorizado para o uso
previsto. A proveniência de áudio, metadados, prompts, versões e análises deve
ser preservada; autoria, licença, referência e transformação são campos
distintos. O projeto TRIOIO adota GNU AGPLv3 ou posterior como licença habitual
do código próprio, sem alterar a licença de qualquer componente externo.
