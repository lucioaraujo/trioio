# Design — interface do Trioio

**Escopo:** a interface do instrumento. As decisões aqui são específicas do
Trioio e não valem como regra RASGO comum; uma promoção ao comum só passa
por G4.
**Estado:** `exploration`. Mapa de tarefas, wireframe, esboço visual e, desde
2026-10-11, o **protótipo v0** (`trioio_app`, §7.1), com a identidade
provisória. A identidade visual ainda não foi decidida pelo autor.
**Bases lidas:**
- em `RASGO_DOCUMENTATION/design/`: `README.md`, `INTERFACES_E_LAYOUTS.md`,
  `IDENTIDADE_VISUAL.md` e `AUDITORIA_MIGRACAO_RESPONSIVA.md`;
- o brief em `RASGO_DOCUMENTATION/templates/BRIEF_DE_DESIGN.md`;
- o documento irmão `RASGO_MODULAR/apps/panel/design.md`, como exemplo de
  formato. A aparência dele não foi copiada.

**Esboço visual:** canvas privado "Trioio — interface",
https://claude.ai/artifact/WUkdez7xVzVkdGMy1KPczM. É uma cópia de acesso:
este arquivo é a fonte.
**Data:** 2026-10-10.

---

## 1. As seis perguntas (INTERFACES_E_LAYOUTS §1)

- **Gesto central:**
  1. escutar uma referência;
  2. pedir uma resposta;
  3. escutar a resposta;
  4. julgar a resposta: aceitar, transformar ou rejeitar.

  É o laço do `TRIOIO_CONCEITO.md`: escutar, reconhecer relações, formar uma
  hipótese, agir, ouvir a consequência e reformular. O "tocar" do Trioio é
  esse diálogo. Ele não toca notas.
- **O que precisa ser entendido antes de soar:**
  - qual referência está escolhida;
  - se ela está autorizada para o uso pedido (o portão `permits()` da
    proveniência);
  - em que modo o Trioio vai responder.
- **Parâmetros:** a classificação está no §3. Os de performance são poucos:
  o modo, a imprevisibilidade, "compor de novo", revelar o olho e o
  julgamento.
- **O que fica visível sob pressão:**
  - a referência atual e o estado dela;
  - o tempo e a posição;
  - o modo;
  - a forma da resposta, em letras de seção;
  - os três botões de julgamento.
- **Relações reais com os irmãos:** nenhuma de aparência. O Trioio usa
  Scales, SieveScale, MicrotonalTuning e NeoRiemannian do Rasgo Synth, mas
  como cópia fixada, e isso não aparece para quem toca. A gramática Eurorack
  (§3.1) entra só onde explica uma relação musical: as consequências do
  Terceiro Olho e o mashup entre duas referências são ligações com fonte,
  destino e profundidade.
- **Metáforas só do Trioio:**
  - o Terceiro Olho, escondido até a escuta;
  - as três faixas, que são os três "olhos": o que soa, o que o olho
    percebe e o que se responde;
  - a forma como um jogo de relações e não como uma timeline de DAW;
  - a escuta do autor como memória do instrumento.

## 2. Cenário e mapa de tarefas

**Cenário:** o autor está no estúdio, com fones e uma tela de notebook ou de
monitor. Traz uma faixa: uma gravação própria, uma faixa de outro instrumento
ou de IA (com termos de serviço) ou um material licenciado. Quer ouvir o que
o Trioio responde, comparar versões e guardar as que interessam. Mais tarde
haverá uma versão ao vivo, com o modo RESPONDER AGORA, que ainda não existe.

| Tarefa | Frequência | Comando hoje (CLI) | Onde fica na interface |
|---|---|---|---|
| varrer a biblioteca | rara | `varrer` | Biblioteca, botão de sistema |
| autorizar, recusar ou revogar | por material | `autorizar` / `recusar` / `revogar` | Biblioteca, ficha do material |
| analisar | por material | `analisar` | automático depois de autorizar; estado na ficha |
| escolher a referência | por sessão | `<id>` | Biblioteca, ou o seletor no topo |
| escolher o modo | por resposta | `--modo` | sempre visível, no topo das faixas |
| ajustar a linguagem | por resposta | `--escala`, `--mistura`, `--ciclo`, `--voicing`… | painel Linguagem |
| compor | por resposta | `responder` | "compor de novo", sempre visível |
| ouvir e comparar | contínua | (fora do Trioio) | transporte, mais as versões em "Sua escuta" |
| revelar o olho | por escuta | (ler `decisoes.json`) | faixa do meio, depois de ouvir |
| julgar | por resposta | `escutar … aceitar\|transformar\|rejeitar` | barra de julgamento, sempre visível |
| ver o que a escuta ensinou | rara | `escuta` | Sua escuta |

## 3. Classificação dos controles (§4, passo 2)

- **Performance:**
  - tocar e parar;
  - o modo: seguir, transfigurar, contrapor ou interromper;
  - a imprevisibilidade;
  - compor de novo;
  - revelar o olho;
  - aceitar, transformar ou rejeitar.
- **Criação** (o painel Linguagem):
  - as alturas e a escala;
  - a mistura, a técnica e a costura;
  - o ciclo;
  - o voicing;
  - a intenção;
  - a forma;
  - o andamento;
  - o mashup ("com", "da outra");
  - o motor;
  - o tempo;
  - as chaves de ritmo, mosaico, harmonia e máquina.
- **Preparação** (Biblioteca):
  - varrer;
  - a ficha de proveniência;
  - autorizar, recusar e revogar;
  - a análise e o estado dela.
- **Sistema:**
  - o dispositivo de áudio;
  - a pasta de dados;
  - as regras (`regras.txt`);
  - ligar ou desligar a escuta;
  - exportar a resposta;
  - a seção Sobre.

## 4. Estrutura das telas

Há uma tela principal, **Escuta e resposta**. Duas superfícies de apoio,
**Biblioteca** e **Sua escuta**, abrem em coluna ou em página conforme o
regime.

Na tela principal, de cima para baixo:

1. **Topo:** a referência, com o estado de proveniência, o modo, o pulso,
   o compasso e a forma lida.
2. **Referência:** a energia, o brilho e os acordes lidos, alinhados no
   tempo.
3. **Terceiro Olho:** coberto por uma trama até "revelar depois de ouvir".
   Revelado, mostra as intervenções nos pontos do tempo e as consequências
   como linhas que seguem até o fim (as ligações do §3.1: fonte, destino e
   intensidade).
4. **Resposta:** as seções com a letra, a linguagem, a textura e o ciclo.
   Abaixo vem o rolo de notas por voz, com as chegadas (sam, tihai)
   marcadas.
5. **Barra de julgamento:**
   - o transporte;
   - aceitar, transformar e rejeitar, com um campo de nota;
   - compor de novo.

**Biblioteca** (prancha 4, compacto). À esquerda, a lista com busca,
"varrer" e o filtro todos / pendentes / autorizados; cada item mostra a
origem e o estado. À direita, a ficha do material, com estes elementos:

- origem, fundamento, quem declarou e as restrições (do `manifesto.json`);
- o portão `permits()` como cadeia ouvir › analisar › aprender › derivar ›
  publicar derivado. Cada uso traz os anteriores, e o que está fechado
  aparece tracejado;
- o resumo da análise e das derivações;
- os botões "usar como referência", "usar no mashup" e "revogar…".

O material pendente só pode ser ouvido. Autorizar pede origem, fundamento,
usos e quem declara.

**Sua escuta** (prancha 5, compacto). À esquerda, as versões sobre a
referência: ouvir, o modo, a seed, o veredito e a nota, mais dois botões,
"comparar A × B" e "alternar no mesmo ponto". À direita, "o que a escuta
ensinou": as tendências do `escuta.json` (timbre, Terceiro Olho e sala)
como barras em escala logarítmica centradas em ×1, de ×0,5 a ×2, com o
esquecimento declarado e a chave "a próxima resposta leva a escuta em
conta" (`--escuta`). A barra de julgamento continua embaixo.

**Julgar um trecho** (prancha 1, especificado em `docs/ESCUTA.md`, ainda
não implementado):

- arrastar sobre a resposta seleciona um trecho, e clicar numa seção
  seleciona a seção;
- a barra diz "julgar: trecho 1:41–2:18 · seção A′". Esc volta para a
  resposta inteira;
- os trechos já julgados ficam como filetes na cor do veredito, no alto
  da faixa.

No compacto e na emergência, o trecho aparece como um chip logo acima da
barra, para não roubar a largura dos botões.

Os valores das pranchas são ilustrativos. Os campos vêm dos contratos
reais.

Cada escolha de linguagem mostra a fonte citada, como nos arquivos de
escalas e de ritmos. A interface não esconde a proveniência do conhecimento.

## 5. Regimes (contrato responsivo, §5.1)

- **Canvas de referência:** 1920×1080.
- **Primeira abertura:** cerca de 88% da área útil do monitor escolhido,
  centralizada num monitor só.

| Regime | Largura útil | Organização |
|---|---|---|
| completo | ≥ 1600 | a Biblioteca em coluna à esquerda; as três faixas ao centro; o painel Linguagem à direita; a barra de julgamento embaixo |
| compacto | 1024–1599 | a Biblioteca vira página (aba no topo); a Linguagem vira gaveta com abas (Escala · Ritmo · Forma · Mashup); as três faixas e a barra ficam |
| emergência | 960–1023, ou altura < 650 | as faixas empilhadas com rolagem vertical; a barra de julgamento e "compor de novo" fixas embaixo; a Linguagem em páginas |

- **O que nunca some, em nenhum regime:**
  - a referência e o estado dela;
  - o modo;
  - tocar e parar;
  - compor de novo;
  - aceitar, transformar e rejeitar.
- **Conta de pixels nas superfícies mínimas** (esboço; ainda sem código
  que a meça):
  - **compacto 1024×650:**
    - cabeçalho de 56: o nome, a referência (até 270) e os quatro modos
      (~410), cerca de 860 de 1024;
    - abas de páginas de 48;
    - rodapé de 64: tocar, tempo, nota, os três julgamentos e "compor de
      novo", cerca de 700;
    - sobram 462 de altura para as três faixas: referência ~95, olho 56 e
      resposta ~295;
    - a gaveta Linguagem tem 340 de largura, cobre a borda direita das
      faixas e não as empurra.
  - **emergência 960×600:**
    - cabeçalho de 52: a referência, os quatro modos e um menu (Biblioteca,
      Linguagem, Sua escuta e sistema), cerca de 720;
    - rodapé fixo de 64: tocar, aceitar, transformar, rejeitar e "compor de
      novo", cerca de 530;
    - as faixas e o resumo da Linguagem rolam na vertical nos 484 do meio;
    - nada essencial fica atrás da rolagem.
- **Dimensões:** os alvos têm no mínimo 44 px lógicos. O texto do corpo
  nunca fica abaixo de 13 px. Não há escala afim: cada regime recalcula as
  áreas, e as faixas recebem o espaço que sobra.

## 6. Identidade (`exploration`, a decidir com o autor)

- **Paleta por função:**
  - `background`: noite fria;
  - `ref`: cinza-azulado, o que soa;
  - `eye`: âmbar, o Terceiro Olho, que só aparece quando revelado;
  - `response`: verde-água, o que o Trioio compõe;
  - `action`: claro sobre escuro;
  - `danger` e `warning`: para a proveniência pendente ou revogada.
- **Tipografia proposta** (todas com licença OFL e embutidas no app, sem
  fontes de sistema):
  - Syne para o display;
  - Atkinson Hyperlegible para o corpo, pela legibilidade com pouca luz;
  - JetBrains Mono para números e tempo.

  Não usa a IBM Plex do Rasgo Synth ("Cobre"), para não herdar a identidade
  de um irmão.
- **Símbolo:** um círculo aberto com um ponto âmbar, o olho. É só um esboço.

- **Símbolo (2026-10-13):** "a logo do Trioio tem 3 olhos" (o autor).
  - O autor mostrou a logo numa imagem dele: em cima, um triângulo
    invertido com um olho e raios que descem até a ponta; embaixo, dois
    triângulos em pé, cada um com seu olho. Desenho em traço.
  - Mostrou também uma caixa de fósforos "Olho Triplo" como referência
    de clima. Essa imagem traz um nome pessoal, que não vai para texto
    público.
  - Redesenhado em traço claro sobre o fundo do Trioio. O Terceiro Olho
    (em cima) e os raios ficam em âmbar (`TRIOIO_CONCEITO.md §1`).
  - Depois, o autor disse: "não dá pra ver os olhos com o contorno —
    deixe só os olhos".
    - Ficaram só os três olhos, maiores, na mesma disposição: o
      Terceiro Olho em âmbar em cima, e os outros dois embaixo.
    - Saíram os triângulos e os raios.
  - O mesmo desenho está no cabeçalho do app (`MainView::paint`, grade 64)
    e no site (`website/assets/identity/trioio-olho.svg`).
  - Continua provisório até o autor dar um desenho próprio.

## 7. Implementação (quando sair do esboço)

- O app será JUCE, com a política de janela do contrato comum (áreas
  semânticas e restauração de monitor e bounds). Ele usará o utilitário
  candidato do AQUORBIUM quando esse passar por G4, sem depender de pasta
  irmã.
- A interface lê o que a CLI já grava: `manifesto.json`, `analise`,
  `decisoes.json` e `escuta.json`. Ela não guarda um estado musical próprio.
- Texto visível em UTF-8 explícito (`CharPointer_UTF8`), por causa de nomes
  como tīntāl, rāga, maqam e sa'idi.

## 7.1 Protótipo v0 (2026-10-11): `trioio_app`

O autor pediu o protótipo ("avance a parte do layout, parte gráfica do
instrumento") e escolheu a interface antes da identidade visual. É um app
JUCE funcional, com a identidade **provisória** (§6): as cores são tokens
num só lugar (`src/app/MainView.h`, `Tokens`) e a fonte é a padrão do
JUCE.

- **Fontes:** embutir as OFL propostas (Syne, Atkinson Hyperlegible,
  JetBrains Mono) seria trazer arquivos de terceiros ao repositório, e
  isso espera a decisão do autor.

**O que faz:**

- **Escuta e resposta:**
  - **referência:** a energia, os acordes lidos e, no título, o BPM, a
    tonalidade e a linguagem;
  - **Terceiro Olho:** coberto por uma trama até "revelar depois de
    ouvir". Revelado, mostra as intervenções no tempo e as consequências
    como linhas até o fim, com o texto "fica: …" escalonado em alturas e
    omitido onde não cabe;
  - **resposta:** as seções, com letra, papel e escala (o *sayr* aparece),
    o rolo de notas (motivo, baixo, demais vozes) e o cursor de reprodução.
- **Biblioteca:** as referências, com a origem, o estado de proveniência e
  a duração; a ficha; abrir outra pasta de dados.
- **Linguagem:** as escolhas que o "compor de novo" leva (alturas,
  mistura, técnica, costura, ciclo, voicing, intenção, forma, andamento,
  imprevisibilidade).
- **Barra de julgamento:**
  - tocar e parar, pelo dispositivo de áudio;
  - aceitar, transformar e rejeitar, que gravam no `escuta.json` por
    `recordJudgement`;
  - a nota;
  - compor de novo, que chama `respond` numa thread de trabalho e recarrega.
- **Janela** (`src/app/WindowPolicy.*`, pura, com teste):
  - abre a 88 % da área útil do monitor principal. Medido: 1690×950 num
    monitor de 1920×1080;
  - salva e revalida os limites; um monitor removido ou o título fora da
    tela levam de volta à área útil.
- **Regimes** recalculados por área (sem zoom):
  - **completo:** Biblioteca à esquerda e Linguagem à direita;
  - **compacto:** abas Escuta/Biblioteca e a Linguagem como gaveta;
  - **emergência:** o cabeçalho com menu ☰ e as faixas com rolagem.
  - A barra de julgamento fica sempre.

**Validação sem abrir janela:**

```sh
Trioio --instantaneos <pasta> [--dados <raiz>] [--referencia <id>] [--revelar]
```

O comando desenha a tela em 960×600, 1024×650, 1366×768, 1920×1080 e
2560×1440, grava os PNGs e confere a geometria dos 10 controles
essenciais: referência, os quatro modos, tocar, aceitar, transformar,
rejeitar e compor de novo. Cada um precisa estar visível, dentro da tela,
com ≥ 44 px e sem sobrepor outro. Resultado: **10/10 nas 5 superfícies**,
com a demonstração e com dados reais (Recording_187 com a resposta Hijaz e
o *sayr*).

**Defeitos achados nas imagens e corrigidos:**

1. o botão da referência estreito no compacto (largura calculada depois de
   cortar a faixa);
2. o símbolo do olho cortado na emergência;
3. os textos das consequências sobrepostos;
4. o botão "revelar" cobrindo o fim da faixa do olho; foi para a linha
   do seletor de respostas;
5. o cabeçalho da resposta com a marca errada; agora mostra a escala e o
   *sayr*.

**Evidências:** `docs/design/prototipo-v0-1920x1080-completo.png`,
`prototipo-v0-1366x768-compacto.png` e
`prototipo-v0-960x600-emergencia.png`.

**Quatro idiomas, SOBRE e crédito (2026-10-11, pedido do autor):**

- **Idiomas** (`src/app/UiLanguage.h`): inglês, português, francês e
  espanhol, na convenção L4 da família (Antitotem, Navalha 2, Rasgo
  Modular).
  - Na primeira abertura vale o idioma do sistema; depois, o escolhido no
    botão EN/PT/FR/ES do cabeçalho, guardado nas preferências.
  - Todo texto da interface muda, inclusive os itens dos combos, sem
    perder a escolha.
  - **Fica em português:** o que vem do núcleo (os nomes das seções, as
    tonalidades, as consequências do olho). É o registro do compositor.
  - Os nomes dos ciclos (maqsum, tīntāl…) não se traduzem.
- **SOBRE:** um cartão sobre véu escuro, com o título, o que o Trioio é, o
  princípio de proveniência (nada de terceiros, fontes citadas, cópias
  fixadas do código RASGO), as licenças e o crédito.
  - Fecha com o botão, com Esc ou clicando fora.
  - Na emergência, fica no menu ☰.
  - O nome como anagrama de uma identidade pessoal (`TRIOIO_CONCEITO.md`)
    não vai para o texto público.
- **Crédito:** "© LÚCIO DE ARAÚJO · TRIOIO 2026 · LICENÇA AGPLv3+ · vX",
  à direita da linha de estado, como no Rasgo Modular.
- **Validação:** os instantâneos nos 4 idiomas × 5 superfícies, com 11/11
  controles essenciais (agora com o botão de idioma) em todas.
  - Evidências: `docs/design/prototipo-v0-fr-1024x650-compacto.png` (o
    francês tem os textos mais longos),
    `prototipo-v0-es-960x600-emergencia.png` e `prototipo-v0-fr-sobre.png`.
  - O modo de instantâneos ganhou `--idioma xx` e `--sobre`.

- **Falta:**
  - a escala de DPI de 125 a 200 %;
  - o teste com dois monitores de verdade;
  - a identidade visual escolhida pelo autor;
  - as fontes embutidas;
  - o teclado e o foco revisados.
- **Julgamento por trecho (2026-10-12):** na faixa da resposta, arrastar
  marca um trecho (≥ 1 s) e clicar num bloco escolhe a seção. Clicar fora
  volta à resposta inteira, e trocar de resposta limpa a seleção. O rótulo
  da barra diz o que vai ser julgado ("julgar só · trecho 0:30–0:55",
  "· seção B"), e a dica explica o gesto.
  - Os instantâneos ganharam `--trecho a-b` e `--secao N`.
  - Evidências: `docs/design/prototipo-v0-trecho-1920x1080.png` e
    `prototipo-v0-fr-secao-1024x650.png`.
  - Validação:
    - 11/11 controles essenciais em todas as superfícies, com a seleção;
    - a suíte 30/30;
    - os três formatos que a interface manda (trecho, seção, `-fim`)
      gravados por `escutar` numa cópia descartável de uma raiz real.
  - Falta o arrasto com o mouse de verdade (o autor).
- **Primeira abertura e trazer gravação (2026-10-13):** o autor abriu o
  app sem `--dados`. Ele caiu na demonstração, onde nada toca nem compõe,
  e pareceu travado ("não sei como tocar").
  - **Sem raiz de dados:** o app não mostra mais a demonstração. Mostra a
    tela vazia com o roteiro (1 traga uma gravação · 2 diga de onde vem ·
    3 componha · 4 ouça, revele, julgue) e o botão "trazer gravação…".
    A demonstração fica só nos instantâneos.
  - **Trazer gravação:** escolhe um arquivo, cria a raiz padrão
    (`~/Música/Trioio`, a pasta de música do sistema) se não houver, e
    registra só aquele arquivo. O importador aceita um arquivo como
    Biblioteca, sem varrer a pasta vizinha. Depois abre o cartão de
    autorização.
  - **Cartão de autorização** (`AuthOverlay`):
    - origem e direito, este sugerido pela origem;
    - só os campos pedidos: ferramenta (IA, outro instrumento), licença,
      evidência (licença, permissão, termos);
    - quem declara, lembrado entre sessões;
    - "pode ouvir, analisar, aprender e compor respostas" (ligado) e "as
      respostas podem ser publicadas" (desligado);
    - recusar, agora não, autorizar.
    - O que faltar volta de `validate()` no cartão, sem fechar. As
      mensagens de `validate()` ainda são só em português.
  - **Pendente na Biblioteca:** a faixa orienta ("diga de onde ela vem") e
    aparece o botão "autorizar…"; a análise não é tentada.
  - **"▶ referência":** toca a gravação original pelo `openAudio`, que usa
    o ffmpeg para M4A/AAC. Abre fora da thread da tela.
  - **A raiz escolhida** em "abrir pasta de dados…" passa a ficar salva;
    antes, perdia-se ao fechar.
  - Nesta máquina, `~/Music/Trioio` é um link para
    `/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio`, como os outros
    instrumentos (o disco do sistema está cheio).
  - Os instantâneos ganharam `--autorizar`.
  - Evidências: `docs/design/prototipo-v0-primeira-abertura-1366x768.png`
    e `prototipo-v0-autorizar-960x600.png`.
  - Validação:
    - o importador com arquivo único (novo bloco 11 em
      `LibraryImporterTests`);
    - a suíte 30/30;
    - 11/11 controles nos instantâneos, inclusive a raiz vazia e a
      pendente;
    - o cartão aberto em 960×600.
  - Falta o autor percorrer o fluxo com o mouse: trazer, autorizar,
    compor, ouvir.
- **Master (2026-10-13):** "falta um controle de master" (o autor).
  - Um volume na barra de baixo, ao lado do tempo, de −60 dB (mudo) a
    +6 dB. O meio do curso fica perto de −12 dB; duplo clique volta a
    0 dB.
  - Age no ganho do transporte, então vale para a resposta e para a
    referência. Fica guardado entre sessões (`master` nas preferências).
  - Entrou nos controles essenciais, que agora são 12, todos aprovados
    nas 5 superfícies.
  - Na emergência (960×600), o rótulo do julgamento cede o espaço ao
    master; a dica continua no tooltip.
- **Sua escuta (2026-10-14, prancha 5):** a terceira página.
  - **Como se chega a ela:**
    - no compacto, uma aba;
    - na emergência, um item do menu ☰;
    - no completo, uma linha de abas sobre a coluna do meio, que alterna
      entre "Escuta e resposta" e "Sua escuta", porque a Biblioteca já fica
      à esquerda.
  - **À esquerda, as versões sobre a referência:**
    - o modo, a seed e a sessão;
    - o veredito, numa faixa de cor (aceita em verde-água, rejeitada em
      laranja, a transformar em âmbar, sem julgamento em cinza);
    - quantos trechos foram julgados, e a nota.
    - Escolher uma versão a carrega na tela e no ▶.
  - **À direita, "o que a escuta ensinou":**
    - as tendências do `escuta.json` (`tendenciesFrom`) que saíram do
      neutro, em grupos (timbre, Terceiro Olho, sala), as maiores primeiro;
    - barras em escala log2 centradas em ×1, de ×0,5 a ×2;
    - o número de julgamentos e o esquecimento declarado (cada julgamento
      anterior pesa 0,85×);
    - a chave "a próxima resposta leva a escuta em conta"
      (`ResponseRequest::useListening` no "compor de novo").
  - Os instantâneos ganharam `--pagina 0|1|2`.
  - **Evidências:** `docs/design/prototipo-v0-sua-escuta-1920x1080.png` e
    `prototipo-v0-fr-sua-escuta-960x600.png`, feitas numa cópia descartável
    da raiz de teste, com 3 julgamentos de teste.
  - **Validação:** 12/12 controles essenciais nas 5 superfícies.
  - **Falta:**
    - ~~"comparar A × B" e "alternar no mesmo ponto"~~ (feito em
      2026-10-14):
      - A é a versão escolhida agora e B a anterior;
      - "▶ comparar A × B" toca A;
      - "alternar A ↔ B" troca para a outra versão no mesmo instante
        (`onPlayAt`; o transporte abre a nova fonte nessa posição);
      - a parada da fonte anterior não marca mais o fim da reprodução
        (`starting_` no transporte).
    - ~~os nomes de timbres e de escolhas do olho nos 4 idiomas~~ (feito em
      2026-10-14: `trName` em `UiLanguage.h`, 41 nomes descritivos
      traduzidos; os nomes próprios passam como estão);
    - ~~a rolagem das barras nas telas pequenas~~ (feito em 2026-10-14):
      a roda do mouse rola as barras, e uma linha "↓ N …" diz quantas
      faltam.
- **Abas e idioma (2026-10-14, achados do autor):**
  - **As abas passavam despercebidas:** "não achei sua escuta". A aba ativa
    agora fica na cor da resposta, e a linha de abas do completo ficou da
    altura mínima de alvo (44 px).
  - **A troca de idioma deixava textos no idioma anterior** (visto numa
    captura da janela real: "suivre · seed", "4 temps", a ficha da
    Biblioteca em francês com a interface em português).
    `refreshReferenceTexts` refaz agora o seletor de respostas (mantendo a
    escolhida), a linha de leitura e a ficha a cada troca.
- **Site (2026-10-12):** `website/` usa os mesmos tokens. Ele está em
  preparação, com noindex, e não é publicado antes do instrumento
  (`website/README.md`).

## 8. Registro

- **2026-10-10, primeiro esboço:**
  - o autor pediu trabalho leve enquanto renderiza um álbum, sem build nem
    render, e depois "veja as diretrizes de layout";
  - o primeiro esboço (1440×900, estilo antes do mapa) foi refeito para
    seguir o processo do §4: primeiro o mapa e a classificação, depois o
    wireframe e os três regimes nas superfícies da matriz;
  - os testes de matriz, DPI e multimonitor ficam para quando houver
    código.
- **2026-10-10, Biblioteca e Sua escuta:** desenhadas no regime compacto
  1024×650, que é o mínimo completo. Para caber na altura, a lista de
  versões mostra cinco itens e rola a partir daí.
- **Próximo:**
  1. o autor escolhe a direção visual;
  2. depois do álbum, o protótipo JUCE no regime compacto, começando pela
     tela Escuta e resposta lendo os arquivos que a CLI já grava;
  3. ainda falta o julgamento por trecho ("o minuto 1 sim, o final não"):
     o núcleo ainda não o tem (`docs/ESCUTA.md`).
