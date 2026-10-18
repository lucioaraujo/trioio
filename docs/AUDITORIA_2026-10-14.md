# Auditoria do Trioio — 2026-10-14

Pedido do autor: "faça uma auditoria do que ainda está sem implementação
elaborada. Limitações a serem resolvidas. Conceitos que já foram cogitados mas
que ainda não foram desenvolvidos. Limitações reais de composição e limitações
sonoras; o que o Trioio precisa para ficar mais interessante musicalmente e
sonoramente."

## Método

- **Fontes cruzadas:**
  - o que foi pensado: `TRIOIO_CONCEITO.md`, `TRIOIO_CEREBRO.md`;
  - o que foi registrado: os `docs/`, `TRIOIO_TAREFAS.md`;
  - o que existe: busca no `src/` por cada conceito.
- **Marcação:** o que não foi conferido no código está marcado **(a
  verificar)**; o resto foi conferido.
- **O que esta auditoria não é:** ela não substitui a escuta. Os pontos
  sonoros são leituras do código e das medições de nível; a prioridade final
  vem da escuta do autor.

## 1. Em uma frase

O Trioio é hoje um compositor **offline** forte na **escuta** e no
**vocabulário** (33 timbres, 2576 escalas, 26 ciclos, Terceiro Olho, linhas
por frequência). Ele é fraco no que faz a música respirar: tempo expressivo,
dramaturgia, diálogo com a referência, mixagem e espaço. E não tem ainda o lado
**instrumento**: ao vivo, gesto, prompt, jogo.

## 2. Conceitos cogitados e ainda não desenvolvidos

Do conceito e do cérebro, conferidos no código:

| Conceito (onde foi pensado) | O que existe | Falta |
|---|---|---|
| **Ao vivo** — modos *Performance* e *Híbrido*, `RESPONDER AGORA` e `HÍBRIDO` com leitura progressiva (CÉREBRO §4–5) | o contrato `EnergyControlBus`/`BrainDecision` (seqlock, sem alocação); o oráculo incremental; o analisador com modo ao vivo | nenhum motor ao vivo consome o barramento; o app só compõe offline e toca o WAV. Faltam o HPSS causal, as vozes pré-alocadas para os objetos do olho e a revisão progressiva da estratégia |
| **Prompt como intenção** ("preservar os ataques e corroer o corpo"…) (CONCEITO §5, CÉREBRO §4) | só o vínculo `promptOriginalProvided`/`promptsRef` no manifesto; as 7 intenções afetivas (`--intencao`) | o interpretador de frases → intenções intermediárias; o `declaredIntent`; o registro de que prompt levou a que decisão |
| **O jogo** — recursos (energia, memória, previsibilidade, risco de saturação, informação oculta) e ações (CONCEITO §3, CÉREBRO §6) | os 4 modos (seguir, transfigurar, contrapor, interromper) e o olho, que revela e esconde | nenhum estado de jogo: não há recursos que se gastam, risco, regra trocada no meio, consequência como placar |
| **Gesto e performer** — `autonomy` contínuo, intervenção humana (CÉREBRO §5) | o campo `autonomy` no `BrainDecision` | entrada de gesto, MIDI ou controle ao vivo; negociação humano × sistema |
| **Expectativa** — "que expectativa foi criada", "quando a repetição vira automatismo", "quando o silêncio é ação" (CONCEITO §2) | a novidade (Foote) para a forma; a leitura do momento para o olho | um modelo de expectativa (probabilidade do próximo evento, surpresa, à maneira do IDyOM); o silêncio como gesto composto |
| **Memória consolidada e esquecimento** — quatro camadas, esquecer gradualmente, memória por Biblioteca (CÉREBRO §4, §10) | `escuta.json` → tendências com idade por ato; julgamento por trecho | uma memória das **próprias respostas** aprovadas (o OMax completo); a nota livre interpretada; o esquecimento controlado pelo autor; a memória separada por Biblioteca |
| **Análise-ressíntese quadro a quadro** (CONCEITO §4–5, SÍNTESE "próximos") | o corpo espectral em janelas de 3 s; picos sem amplitude | a ressíntese que acompanha o corpo da referência ao longo do tempo, com amplitudes |
| **Campo estéreo e profundidade** como descritores (CONCEITO §5) | a análise é mono | ler a imagem estéreo e a profundidade, e responder a elas |
| **Etapa 3 das linhas** (`docs/LINHAS.md`) | as linhas na linha de comando | as faixas por linha na interface; ▶ e julgamento por linha |

## 3. Implementado, mas raso — limitações a resolver

### Escuta

- **Acordes:**
  - só tríades; sétimas, nonas e suspensões não são lidas;
  - erram em misturas cheias (a melodia entra no croma);
  - candidato: o croma NNLS (Mauch e Dixon 2010).
- **Melodia:**
  - ~45–53 % da altura exata nas peças do Trioio;
  - não decide onde **há** melodia: faltam os contornos do MELODIA.
- **Tempo forte:** a confiança fica ≤ 0,05 nas gravações reais; por isso
  não entra na composição.
- **Nível do pulso:**
  - estável na bancada (105/140), mas instável sob mudança de andamento;
  - falta o acompanhamento por programação dinâmica (Ellis 2007) ou pulso
    local.
- **Ciclos:**
  - 19 de 26 em 1.º na bancada sintética; nenhum ciclo claro nas gravações
    reais;
  - segue experimental e não alimenta a composição.
- **Limiares sem gabarito humano:** a fração percussiva (0,40) e a forma
  por repetição (0,8) foram calibradas em casos construídos.
- **A escuta é mono e offline:** o HPSS é centrado no tempo.

### Composição e aprendizagem

- **A escuta do autor pesa o que foi usado, não como foi usado.** A nota
  livre é guardada, mas não interpretada. A percussão não aprende a posição
  no campo.
- **As afinidades do olho e das escolhas são heurísticas escritas à mão**,
  calibradas em poucos casos.
- **Regras:**
  - não há regras por seção ("no fim, só o corpo");
  - o salto máximo encurtado pode tirar a nota do campo.
- **Harmonia:** a função é lida e transformada, mas **não escrita**
  (cadências, dominantes secundárias, preparação de modulação). Falta o
  tratamento da dissonância (apojatura, cor).
- **Memórias:**
  - o oráculo aprende só a altura (sem ritmo nem timbre por evento);
  - o mosaico usa 3 descritores e ressintetiza, não toca a amostra (tocar a
    amostra é decisão do autor).
- **Campo rítmico:** duas faixas, sem a membrana média.
- **Linhas:** cada linha é uma composição inteira dobrada para o seu
  registro. Não há divisão de papéis por linha.

### Interface e entrega

- **Interface:**
  - falta a página "Sua escuta";
  - DPI de 125 a 200 % e dois monitores reais não testados;
  - teclado e foco não revisados;
  - fontes não embutidas;
  - identidade visual provisória;
  - as mensagens de validação da autorização são só em português.
- **Empacotamento:** nenhum (nem CPack, nem macOS Universal, nem
  instalador Windows), apesar do padrão multiplataforma do RASGO. O site
  segue em preparação.
- **Critérios de excelência do cérebro ainda não medidos:** o custo de CPU e
  memória por modo; a escuta humana documentada de forma sistemática.

## 4. Limitações reais de composição

Estas não são falhas de código. São o que hoje mais limita o interesse
musical de uma resposta.

1. **O tempo é uma grade.**
   - Não há microtempo, swing, groove nem rubato.
   - O único "groove" do código é a cópia dos compassos de outra referência
     no mashup.
   - Uma resposta com pulso soa quantizada mesmo quando a referência
     respira. O desvio dos ataques da referência em relação à grade já está
     medido (ataques e fase), mas não é aplicado.
2. **Não há dramaturgia de longo prazo.**
   - A forma vem das seções da referência (contraste ou repetição), e cada
     seção recebe uma operação: inversão, retrógrado, sayr.
   - Não há uma curva planejada de tensão: preparar, chegar a um clímax,
     esvaziar, respirar. A intenção "arco" só guia a máquina de escalas.
   - O silêncio não é composto como gesto.
3. **A resposta é uma peça paralela, não um diálogo.**
   - Ela tem a mesma duração e corre ao lado da referência.
   - Não há pergunta e resposta (entrar quando a referência pausa), tocar
     **junto** com a referência (acompanhar, contrapor em sobreposição),
     nem retomar um gesto dela.
   - O "escuta e resposta" do nome ainda é tradução, não conversa.
4. **O desenvolvimento motívico é procedural.**
   - As transformações clássicas (inversão, retrógrado, dobra, aditivo)
     encadeadas por sorteio dão variedade, mas pouca direção.
   - A frase não "quer chegar" a lugar nenhum, a não ser pelas cadências
     das linguagens com comportamento (32 de 2576).
5. **A relação entre vozes é fraca.**
   - Heterofonia, eco e harmonia sustentada existem.
   - Mas não há condução de vozes (contraponto, notas comuns, movimento
     contrário) nem pergunta e resposta entre as vozes da própria resposta.
6. **A dinâmica segue a energia da referência** (dinâmica relativa).
   Faltam o fraseado dinâmico próprio (crescendo para a cadência, acento de
   frase) e a articulação (legato, staccato) como escolha composicional
   **(a verificar em detalhe)**.

## 5. Limitações sonoras

1. **Não havia mixagem nem master.**
   - Cada nota é sintetizada, nivelada pelo pico e somada.
   - A peça inteira era normalizada pelo pico a −1 dBFS: um único pico
     decidia o volume de tudo, e os picos entre amostras não eram vistos.
   - Não havia compressão de barramento, limitador de *true peak* nem alvo de
     loudness.
   - *Correção no mesmo dia:* os "−19 a −23 dBFS" eram RMS, não loudness.
     Medidas em LUFS, as respostas ficam entre −16 e −17 LUFS: não eram
     baixas.
   - **O master foi feito no mesmo dia**: §8 e `docs/SINTESE.md`, "O
     barramento de master". Seu ganho real é a consistência entre respostas,
     a cola leve e a segurança de pico. Ainda falta o EQ por família.
2. **O espaço é estático.**
   - Cada voz tem pan fixo, e uma das 3 salas recebe o meio do estéreo.
   - Não há envio de reverb por voz, profundidade (distância, primeiras
     reflexões) nem movimento no estéreo.
3. **As vozes não interagem.**
   - As notas são renderizadas isoladas e somadas.
   - Não há ressonância por simpatia, mascaramento tratado ou
     acoplamento entre corpos.
4. **O timbre não evolui dentro da nota.**
   - Há glissando, curvas de ornamento e oscilação.
   - Mas o timbre não acompanha a frase (brilho que abre no crescendo, sopro
     que entra na nota longa) **(a verificar família por família)**.
5. **A família espectral é a mais "da referência", e ainda é estática.** O
   corpo vem em janelas de 3 s, com picos sem amplitude.
6. **Aliasing não auditado** nas famílias copiadas (osciladores com formas
   ricas). As aditivas próprias são seguras por construção.

## 6. O que tornaria o Trioio mais interessante

Em ordem de impacto por esforço:

| # | Proposta | Impacto | Esforço | Por quê |
|---|---|---|---|---|
| 1 | **Barramento de master:** EQ por família, compressão suave de cola, limitador de *true peak*, alvo de loudness (~−16 LUFS), e o mesmo tratamento por linha | sonoro, alto | baixo–médio | é a diferença mais audível entre "demo" e "peça"; não toca a composição |
| 2 | **Groove e microtempo da referência:** medir o desvio médio dos ataques por posição do compasso (o "feel") e aplicá-lo à resposta; rubato nas seções de tempo livre | musical, alto | médio | tira a grade; usa a escuta que já existe |
| 3 | **Dramaturgia:** uma curva de tensão para a peça (preparar, clímax, esvaziar, silêncio composto) guiando densidade, registro, dinâmica e o olho | musical, alto | médio | dá direção ao que hoje é variedade |
| 4 | **Diálogo com a referência:** modos "tocar junto" (a resposta sobre a referência, com o mix das duas) e "pergunta e resposta" (entrar nas pausas, retomar gestos dela) | conceitual e musical, alto | médio–alto | é o "escuta e resposta" literal |
| 5 | **Espaço:** envios de sala por voz, profundidade, movimento de pan; ler o estéreo da referência | sonoro, médio | médio | dá corpo e lugar às vozes |
| 6 | **Harmonia escrita:** sétimas e suspensões na leitura e na escrita, cadências e dominantes secundárias, condução de vozes | musical, médio | médio | as canções andam pela harmonia |
| 7 | **Expressão por nota:** articulação, dinâmica dentro da frase, timbre que segue a frase | sonoro e musical, médio | médio | as famílias passam a "tocar", não só a soar |
| 8 | **Memória das próprias respostas aprovadas** (OMax completo) e a nota livre interpretada | identidade, médio | médio | o instrumento passa a ter repertório próprio |
| 9 | **Prompt como intenção** (vocabulário controlado primeiro, sem LLM) | conceitual, médio | médio | o que está no conceito desde o início |
| 10 | **Ao vivo** (`RESPONDER AGORA`, performance) | instrumento, muito alto | alto | o contrato existe; é uma etapa arquitetural própria |

Recomendação: **1 → 2 → 3**, nessa ordem. O master é rápido e muda a
escuta de tudo o que vier depois. O groove e a dramaturgia atacam o que mais
falta musicalmente. O 4 é a grande decisão conceitual seguinte e pede uma
conversa com o autor (tocar junto com a referência muda a relação de
proveniência e de publicação). O 10 é um projeto à parte.

## 7. Validação que falta

- **Gabarito humano:** nenhum. Os números das faixas reais são comparações
  entre versões.
- **Escuta sistemática do autor**, com julgamento, sobre as versões. Hoje ela
  é ocasional (Recording_187, linhas).
- **Medições de CPU e memória por modo** (critério do cérebro).
- **Auditoria de aliasing** e de *true peak* nas famílias copiadas.
- **Empacotamento e o teste nas três plataformas**, quando houver
  publicação.

## 8. Andamento das prioridades

- **Item 1 — barramento de master: feito em 2026-10-14** (`src/core/Master.*`,
  `docs/SINTESE.md`, "O barramento de master").
  - A cadeia: passa-altas 30 Hz → cola 2:1 relativa à peça → −16 LUFS
    (BS.1770) → limitador de pico verdadeiro −1 dBTP.
  - Os stems seguem a mesma curva de ganho, e a soma deles continua sendo a
    resposta.
  - Falta o EQ por família.
- **Item 2 — groove e microtempo: feito em 2026-10-14** (`src/core/Groove.*`,
  `docs/RITMO.md`, "Groove e microtempo da referência").
  - O swing, o microtempo por posição e o acento são medidos nos ataques e
    aplicados à grade, por modo.
  - Dois vieses da primeira medição foram corrigidos (a janela larga
    inventava swing; o pulso fraco não tem grade).
  - O rubato nas seções livres foi feito em 2026-10-14 (`applyRubato`).
- **Item 3 — dramaturgia: primeira versão em 2026-10-14**
  (`src/core/Dramaturgy.*`, `docs/COMPOSICAO.md`, "Dramaturgia").
  - A curva de tensão por modo molda a dinâmica, os enfeites e o silêncio
    antes do clímax.
  - O efeito é modesto: a curva ainda não guia o registro, as vozes
    principais nem o olho.
- **Item 4 — a metade que não muda a proveniência: feita em 2026-10-14**
  (`src/core/Dialogue.*`, `docs/COMPOSICAO.md`, "Diálogo").
  - O diálogo (a resposta escuta nas frases e responde nas pausas, ou
    alterna compassos) e o "▶ juntos" no app (ouvir sem gravar).
  - **Decidido pelo autor:** não se grava nem se publica uma mistura com o
    áudio da referência. A resposta é um arquivo novo, uma variação criativa
    feita só por síntese. O mesmo vale para tocar amostras no mosaico.
- **Próximo:**
  - a escuta do autor dos itens 1–4;
  - ~~a resposta que imita o pedido~~ (feita em 2026-10-14:
    `imitateCalls`, `docs/COMPOSICAO.md`);
  - a dramaturgia guiando o registro e o olho (feitos em 2026-10-14).

