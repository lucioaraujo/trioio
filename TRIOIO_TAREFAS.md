# TRIOIO — tarefas

## Biblioteca e arquivos — 2026-10-31

Objetivo: tornar localizáveis as gravações registradas e as respostas, com
gestão pela própria interface. Em `src/app/MainView.{h,cpp}` e
`src/app/UiLanguage.h`, foi adicionado **áudio de origem ▾** ao cartão da
referência: mostrar o original na pasta ou guardar uma cópia em
`<pasta de dados>/fontes/`. A cópia roda fora da thread da interface, usa
o nome legível do original, arquivo parcial no mesmo volume e registra o novo caminho no manifesto pela
identidade SHA-256 existente; o original permanece. **Pasta da biblioteca ▾**
agora distingue mostrar a pasta atual de escolher outra; ao trocar, a seleção
e a lista de respostas da biblioteca anterior são limpas. Um arquivo de áudio
ou MIDI pode ser arrastado para a janela e passa pelo mesmo registro e pedido
de autorização de **Trazer gravação**. A resposta continua com **arquivo ▾**
para mostrar/exportar WAV, MIDI e stems. O seletor de entrada abre na pasta da
referência selecionada, quando ela ainda está disponível. A cópia só começa
com espaço suficiente no volume para o arquivo mais uma margem de 256 MiB.

Pendente: compilar e verificar fluxo com arquivo pequeno, duplicata,
autorização conservada, pasta indisponível e layout compacto/completo. O
preflight encontrou 7,7 GiB de memória disponíveis mas swap em 1,9/2 GiB;
o procedimento de segurança bloqueia o build acima de 50% de swap usada.
Não anunciar o recurso como publicado até essa validação. Próximo avanço:
seleção múltipla de arquivos com fila limitada e opção de importar cópia já
na entrada; por ora o arrastar aceita um arquivo de cada vez.

**Preflight após limpeza da swap:** a swap caiu para 2 MiB, mas a memória
disponível oscilou entre 5,7 e 6,1 GiB e a swap subiu a 157 MiB em poucos
minutos. O procedimento exige 6 GiB disponíveis e veda build durante
crescimento rápido da swap; a compilação segue pendente. Retomar quando a
memória estabilizar acima de 6 GiB, com `-j1`, e conferir importação, cópia
e layout. `git diff --check` passou.

**Build concluído após novo preflight:** memória disponível 8,9 GiB, swap
336/2047 MiB, disco 964 GiB livres. `timeout 600 cmake --build build -j1
--target trioio_app` passou e gerou
`build/trioio_app_artefacts/Release/Trioio` em 31/10 às 19:37. Ao terminar,
a memória disponível era 9,1 GiB e a swap permaneceu em 336 MiB.
`git diff --check` passou. Pendente a prova interativa do arrastar/soltar,
da cópia registrada e do layout da Biblioteca nos três regimes; o build
comprova compilação, não essas ações de interface.

**Inspeção visual subsequente:** o modo `--instantaneos ... --pagina 1`
gerou a Biblioteca em 960×600, 1024×650, 1366×768, 1920×1080 e 2560×1440;
os 12 controles essenciais ficaram dentro da tela nos cinco tamanhos. Na
inspeção das imagens de 1366 e 1920, o botão do áudio de origem estava longe
da lista. Foi movido para junto de **trazer gravação…**, recompilado com `-j1`
e os cinco instantâneos passaram novamente. Ainda falta a prova interativa
de arrastar um arquivo real e de copiar uma fonte na biblioteca temporária.

**Prova funcional com raiz temporária:** foi criado um WAV sintético curto em
`/tmp`, separado de uma biblioteca vazia. Os instantâneos com `--dados`
confirmaram a seleção dessa raiz e mostraram 0 referências. Já na tentativa
interativa, a janela observada exibiu referências do acervo habitual; a
instância foi fechada antes de registrar ou copiar qualquer áudio. As
preferências do usuário permaneceram apontando para `/home/luc/Music/Trioio`.
Repetir a prova interativa com perfil isolado e confirmar a raiz na tela
antes de acionar a importação.

O botão **pasta da biblioteca ▾** passou a mostrar o caminho absoluto como
tooltip para facilitar essa confirmação; Release recompilado com `-j1`.

**Tentativa interativa adicional:** a raiz temporária foi confirmada no
rodapé antes de clicar. A automação do seletor de arquivos, porém, escolheu
um MP3 da pasta Música e confirmou a autorização, em vez do WAV sintético;
o manifesto e a análise foram escritos somente na raiz de teste em `/tmp`.
O arquivo de origem permaneceu intocado. O app foi fechado. A preferência
`dados`, que o app havia mudado para a raiz temporária, foi restaurada e
verificada como `/home/luc/Music/Trioio`. A automação não é confiável para
validar a cópia do arquivo; não alegar que essa ação passou. Próximo teste:
um harness de UI controlado ou interação manual acompanhada, com seleção
confirmada antes de autorizar.

**Validação da identidade e do caminho com fixture WAV sintética:** no
diretório temporário, `trioio_biblioteca varrer <origem> <dados>` criou uma
referência pendente; uma cópia em `fontes/` foi reconhecida como o mesmo
conteúdo e moveu o caminho canônico sem duplicar o manifesto. Depois de
autorizar a fixture, nova varredura retornou uma referência sem mudança e o
manifesto manteve `status=authorized` e `canonicalPath` na cópia. Isso valida
o registro, a identidade e a persistência da autorização no importador; a
cópia feita por shell não valida a ação do botão de UI nem o drag-and-drop.

**Correção do override de sessão:** `src/app/Main.cpp` agora ignora o sinal
`onDataRootChanged` durante a abertura inicial. O argumento `--dados` escolhe
a raiz desta execução sem sobrescrever a preferência permanente. Build
Release passou com `-j1`; ao abrir e fechar com a raiz temporária, o valor
persistido continuou `/home/luc/Music/Trioio`. `git diff --check` passou.

**Validação final desta etapa:** o teste automatizado do importador agora
exercita a cópia gerenciada de uma fixture WAV: verifica o arquivo em
`fontes/`, a identidade já registrada, a autorização preservada e o novo
`canonicalPath`. `trioio_library_importer_tests` e
`trioio_window_policy_tests` passaram (2/2); `git diff --check` passou. O
build Release do app também passou. A validação automatizada da cópia cobre o
mesmo helper usado pelo botão; a interação real do seletor e do drag-and-drop
continua sem prova confiável porque a automação gráfica selecionou um arquivo
alheio na tentativa anterior. Nenhuma origem do usuário foi alterada. Próximo
passo: validar manualmente o arrastar/soltar e o seletor em sessão normal.

**Tutorial, idiomas e operação:** o capítulo de importação em
`src/app/Tutorial.h` foi alinhado aos controles atuais e explica arrastar um
áudio/MIDI, revelar ou copiar a fonte e escolher a pasta da biblioteca em
inglês, português, francês e espanhol. `docs/BIBLIOTECA_OPERACAO.md` agora
documenta `fontes/`, a cópia sem mover o original e a exigência de autorização.
Build Release incremental passou após a alteração do tutorial; `git diff
--check` passou.

## Pendências de 2026-10-27

- [x] Biblioteca: "esconder recusados (N)" (`138d946`): tira da vista,
      nunca apaga o manifesto; guardado em `esconder_recusados`. Sai na
      próxima versão.
- [x] A captura do autor em 1920×1006 (`docs/design/trioio-interface-1920x1006.png`,
      app em FR) é a do card do portal RASGO (`trioio-1920.jpg`).
- [ ] **Idioma misturado:** com a interface em francês, o Terceiro Olho e
      os nomes das seções saem em português ("reste : a escala fica
      mutada", "a música segue para onde o gesto levou", "retorno de D
      (como na seção…)"). Esses textos vêm do compositor/dramaturgia sem
      passar pela tradução. Conferir as quatro línguas.

## ▶ Ponto de retomada (2026-10-18, manhã)

**Estado:**

- tudo commitado localmente, sem push: são 103 commits do Trioio à frente
  de `origin/main`, e push é decisão do autor;
- ctest: 46/46;
- o app e a CLI (`trioio_biblioteca`) estão compilados em `build/`, com
  `data/` ao lado.

**O que existe e funciona:**

- **A escuta:** pulso, compasso, seções, acordes e tonalidade, linguagem e
  ciclo rítmico (`docs/LEITURA_E_GABARITO.md`). Também a memória curta, que
  guarda só descritores (`ShortMemory.h`).
- **A composição:** motivo, frases, forma, Terceiro Olho, diálogo e
  dramaturgia.
- **A máquina de harmonia** (`docs/MAQUINA_DE_HARMONIA.md`):
  - 26 linguagens, inclusive as brasileiras e Partch;
  - a condução a quatro vozes;
  - 50 cadências;
  - o som da aura.
- **A máquina de timbres** (`docs/MAQUINA_DE_TIMBRES.md`): os modelos
  físicos, os 15 conjuntos, a produção, os efeitos e o rasgo modular.
- **O jogo** (`docs/JOGO.md` §9): critérios no som, estado, julgamento por
  trecho, regras do jogo e do autor, troca de lugar.
- **A intenção e o modo híbrido** (`docs/PROMPTS_E_INTENCOES.md`):
  - áudio + prompt (`prompts.json`, as três camadas) + gesto (`--gesto`,
    o campo e o editor no app);
  - a comparação entre versões (`versoes`, `docs/VERSOES.md`).
- **A saída:** WAV, MIDI e stems por voz, também no modo linhas.
- **A Biblioteca** (`docs/BIBLIOTECA_OPERACAO.md`, `docs/FORMATOS_DE_DADOS.md`):
  a retenção (`limpar`, `esvaziar-lixeira`, `limpar-cache`), o registro das
  saídas no manifesto e as referências por módulo.
- **O desempenho** (`docs/MEDICOES.md`): o jazz caiu de 126 para 22 s com o
  mesmo som (piano, palheta, metais, desnormalizados, barramentos em
  paralelo). A reprodução não faz I/O no callback de áudio.
- **O release** (`docs/MULTIPLATAFORMA.md`):
  - os dados viajam com o binário;
  - o runtime estático do MSVC, o Universal 2 e o alvo 10.15;
  - o pacote de fontes com manifesto e teste de restauração.
- **Os dados:** só em `/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/`. A
  lixeira de 18/10 (cerca de 3 GB de testes) pode ser esvaziada depois de
  7 dias (`esvaziar-lixeira`).

**Esperando o autor:**

1. **Escutar e julgar** (`AUDIOS_RASGO/Trioio/escutas/`):
   - `2026-10-17-linguagens-modernas/` (17 linguagens + Partch);
   - `2026-10-17-maquina-de-harmonia/`;
   - `2026-10-17-jogo-troca-de-lugar/` e `2026-10-17-jogo-criterios-no-som/`;
   - `2026-10-17-stems-jazz/` (para ouvir, a mistura);
   - `2026-10-18-gesto/` (`2026-10-18-intencao-e-consequencia/` foi escutada:
     "senti as diferenças");
   - a nota escrita como regra ("quando cresce, o baixo cala") entra no
     jogo.
2. **No app:** confirmar a reprodução (a leitura antecipada nova) e
   desenhar um gesto com o mouse.
3. **Decidir:**
   - o push;
   - a versão estável (`tools/empacotar_release.sh`);
   - o ícone do app, para o CPack e a CI dos três sistemas.

**Para o Claude, sem o autor:** a reorganização ao vivo do modo híbrido;
compor as linhas em paralelo.

**Atualização da tarde de 18/10:**

- **A árvore de influências** (`docs/INFLUENCIAS.md`), também no app;
- **a pesquisa sobre a IA** no Atlas (o commit do Atlas fica com o Codex);
- **a mistura conceitual de cadências** (`MAQUINA_DE_HARMONIA.md` §11);
- **o mapa** da busca pelo não ocupado.

Escutar `escutas/2026-10-18-influencias/`. As poéticas de artistas e bandas
entram pela escrita do autor (`tools/gerar_arquivo_de_influencias.py`).

## ⏸ Ponto de retomada anterior (pausa em 2026-10-06; histórico)

**Pausa decidida pelo autor** para resolver o Rasgo Modular no Windows. O
Trioio para num estado estável, com tudo commitado (último commit
`879b3be`) e 12/12 testes passando.

**Onde paramos:**

- **A cadeia completa funciona offline:**
  - Biblioteca: varrer, autorizar e analisar;
  - escuta: descritores, croma, picos, f0, pulso, compasso, seções e campo;
  - compositor: motivo da referência, sentença, variação em
    desenvolvimento, forma e orquestração em seis timbres;
  - o Terceiro Olho: objetos sonoros e regras ocultas;
  - saída em WAV, MIDI e a forma em texto.

  Comando:

  ```sh
  build/trioio_biblioteca_artefacts/Release/trioio_biblioteca responder <dados> <id> --modo M [--seed N] [--alturas …] [--tempo …] [--imprevisibilidade 0..1]
  ```

  Ver `docs/COMPOSICAO.md` (§Etapa 3).
- **A última escuta do autor foi sobre a v2.** Comentário: "interessante,
  porém contido, como arquivos de midi". A **v3 ainda não foi escutada**:
  são seis versões em `~/Music/TRIOIO_respostas/v3-fora-da-caixa/`, com
  imprevisibilidade de 0 a 1.
- **Dados locais do autor:** `~/TRIOIO_DATA`, com 22 gravações do Rasgo
  Synth autorizadas **só para `analyze`**. Para `responder` sobre elas é
  preciso redeclarar com `--usos derive`.
- **Ferramenta compilada** em `TRIOIO/build/`, pasta ignorada pelo git.

**2026-10-07 — escolha contextual feita.** O Terceiro Olho lê o momento
(referência e partitura) antes de cada intervenção e escolhe por afinidade.
A imprevisibilidade passou a dosar quanto a escolha obedece a essa leitura.
O congelamento usa o acorde que acabou de se destacar. Cada marca da forma
diz o motivo. Ver `docs/COMPOSICAO.md` §"Leitura do momento". 12/12 testes.
A **v4** está em `~/Music/TRIOIO_respostas/v4-leitura-do-momento/`: mesma
gravação e mesmas configurações da v3, para comparar.

**2026-10-07 — escuta da v4 e consequências.** O autor ouviu a v4: "há
variações, mas ficam meio equivalentes... avance no desenvolvimento e iremos
acertando com o tempo". Resposta: as intervenções do Terceiro Olho agora
podem **ficar até o fim**. O acorde congelado vira o campo, o cânone
continua, o drone vira pedal, a melodia segue o gesto. A forma vira
trajetória. Ver `docs/COMPOSICAO.md` §"Consequências". A **v5** está em
`~/Music/TRIOIO_respostas/v5-consequencias/`. 12/12 testes.

**2026-10-07 — 14 famílias de timbre** (pedido do autor: espectral, mais
"um bom apanhado de tudo que temos codado em síntese"):

- **espectral:** o corpo medido da referência, ressintetizado;
- **7 famílias do Rasgo Modular hospedadas** (tabela de ondas, pulsar,
  modal, guia de onda, tambor, voz formante, dobra), usando os módulos
  reais por include path, sem cópia. O Trioio vira o segundo consumidor
  desse DSP;
- **inventário** de toda a síntese RASGO, com licenças, em
  `docs/SINTESE.md`. Rasgo Synth (cerca de 50 vozes, GPL-3.0) e Aquorbium
  (AGPL-3.0, `LICENSE-AGPL-3.0.txt`) dependem de uma decisão do autor:
  fixar uma versão. A primeira versão do inventário disse, por engano, que
  o Aquorbium não tinha licença;
- 13/13 testes;
- **v6** em `~/Music/TRIOIO_respostas/v6-familias/`.

**2026-10-07 — 22 famílias, Trioio autônomo.**

- **Decisão do autor:** "o Trioio será autônomo, com código próprio, sem
  depender de nenhum outro projeto do RASGO; se quiser aproveitar os
  códigos, faça uma cópia".
- O Rasgo Modular deixou de entrar por include path e virou **cópia
  fixada** (`third_party/rasgo_modular_dsp`, commit `fcb711a`).
- Entraram **8 vozes do Rasgo Synth** por cópia fixada
  (`third_party/rasgo_synth_dsp`, commit `0dc468a`): Gendyn, VOSIM, Walsh,
  teremim, flauta FM, scat, chip AY e galo.
- Commit e checksums em cada `PROVENANCE.md`; créditos em
  `CREDITS_AND_SOURCES.md`.
- 13/13 testes, com as 22 famílias conferidas. A **v7** está em
  `~/Music/TRIOIO_respostas/v7-22-familias/`.

**2026-10-07 — corpo que se move e objeto "corpo da referência".**

- O perfil espectral é medido em janelas de 3 s a cada 1,5 s. Cada nota
  espectral se transforma do corpo do seu início no do seu fim.
- O Terceiro Olho ganhou um 8º objeto: sustenta os parciais medidos da
  referência, e se a intervenção fica, o timbre da melodia vira o
  espectral.
- 13/13 testes, com o movimento conferido por energia de parcial
  (Goertzel).
- A **v8** está em `~/Music/TRIOIO_respostas/v8-corpo-que-se-move/`.

**2026-10-07 — 26 famílias e três salas.**

- Entraram o ressonador soprado (Rasgo Modular `RESONATOR` com o ruído do
  Antitotem), CMOS (Antitotem `CmosVoice`, primeira cópia do Antitotem), coro
  e Rungler (Rasgo Synth).
- Salas HALL e SPACE do Rasgo Modular, escolhidas pelo caráter da
  referência.
- Corrigido no caminho:
  - o ressonador saía 8 semitons acima da nota (o modo mais forte não era a
    fundamental): passou a usar a saída `low`, com estrutura pouco esticada;
  - o teste das salas olhava a cauda tarde demais para um SPACE curto.
- 13/13 testes. A **v9** está em
  `~/Music/TRIOIO_respostas/v9-26-familias-salas/`.

**2026-10-08 — 33 famílias: todo o inventário de síntese pedido.**

- Pedido do autor: "faça todos".
- Entraram:
  - do Rasgo Modular: operador FM (`OPERATOR`) e LPG (`OSC` → `LPG`);
  - do Rasgo Synth: nuvem granular e objeto espectral;
  - do Aquorbium (primeira cópia): ressonador Bioma, corda Bioma e striker
    granular. Copiados do commit `e234a25`; nada foi editado no
    Aquorbium.
- Os `.cpp` copiados compilam sem avisos, com o flag correto em cada
  compilador.
- 13/13 testes. A **v10** está em `~/Music/TRIOIO_respostas/v10-33-familias/`.

**2026-10-08 — a escuta do autor vira tendências.** `escutar <dados>
der-… aceitar|rejeitar|transformar [--nota]` grava em `escuta.json` o
julgamento e o que a resposta usou: famílias, escolhas do Terceiro Olho e
sala. As respostas seguintes puxam o aceito (até ×2) e afastam o rejeitado
(até ×0,5), com esquecimento gradual. Sem escuta, a composição é idêntica.
Ver `docs/ESCUTA.md`. Também: as famílias percussivas hospedadas saíram de
−12 dB para −3/−4 dB em relação ao aditivo. 14/14 testes.
**Disco (8 out. 2026):** o disco do sistema (`/`) chegou a 98 %. O autor
moveu `~/Music/Rasgo Synth` para liberar espaço; ela tinha a referência de
todas as versões, `rasgo_integrated_20261004_191505.wav`. Os testes de
ponta a ponta passaram a usar uma referência sintética própria. **Renders
novos vão para `AUDIOS_RASGO/Trioio/escutas/AAAA-MM-DD-tema/`** (a pasta
`TRIOIO_respostas`, usada até ali, foi desfeita em 2026-10-28: as três
referências viraram arquivos reais em `AUDIOS_RASGO/Trioio/referencias/`, os
gabaritos já estavam em `dados/gabaritos/`, e o resto foi para a Lixeira do
disco). Antes de
qualquer render ou build, conferir que `/` tem pelo menos 5 GB livres. Os
1,8 GB de renders antigos em `~/Music/TRIOIO_respostas` aguardam a decisão
do autor.

**2026-10-08 — memória performativa (Atlas §63, OMax).**

- A referência é aprendida num **oráculo de fatores**, implementação
  própria a partir do algoritmo publicado (Allauzen, Crochemore e Raffinot;
  Assayag e Dubnov).
- Volta recombinada em 1 a 4 passagens numa voz própria, com o corpo
  espectral da referência: seguida, transposta ou espelhada conforme o
  modo.
- 96% dos pares seguidos são pares reais da referência, e 100% das alturas
  vêm dela.
- Ver `docs/MEMORIA.md` e a entrada no Atlas. 15/15 testes.
- A **v11** está em
  `/media/luc/4tb_21042021/TRIOIO_respostas/v11-memoria-oraculo/` (apagada em 2026-10-16: arquivo de teste) (renders
  agora no disco de 4 TB).
- **Próximos candidatos do Atlas:**
  - CataRT/MuBu (§62): memória pesquisável por descritores, mosaico;
  - restrições musicais (§55): permitido, proibido, condicionado,
    exceção;
  - Grids (§45): ritmo como campo;
  - Frames (§40): estados e interpolação;
  - Streams (§46): dinâmica dependente do sinal;
  - Antescofo (§60.1): acompanhar a referência ao vivo.

**2026-10-08 — regras do autor (Atlas §55).**

- `regras.txt`: proibido timbre, olho ou sala; obrigatório olho; máximo
  salto; âmbito; mínimo memória; exceção marcada.
- Uma regra não entendida impede a resposta e aponta a linha.
- Sem regras, a forma ficou idêntica à da v11 (conferido na gravação
  real), o que mostra que a reorganização do Terceiro Olho em
  `intervene(t, forçada)` não mudou nada.
- Ver `docs/REGRAS.md` e `exemplos/regras.txt`. 16/16 testes.
- A **v12** está em `/media/luc/4tb_21042021/TRIOIO_respostas/v12-regras/` (apagada em 2026-10-16: arquivo de teste).

**2026-10-08 — regras condicionado e incompatível.** Completam os tipos
da §55. `condicionado olho <escolha> se <condição>` usa a leitura do
momento: energia alta, baixa, subindo ou caindo; ruído; denso; rarefeito;
mudança; campo parado; acorde forte. `incompatível olho <a> e <b>` impede
que as duas soem juntas. Testes: 15 clusters condicionados, todos em
trecho forte; nenhuma sobreposição incompatível. 16/16.

**2026-10-08 — ritmo como campo (Atlas §45, Grids).**

- É a primeira camada rítmica do Trioio (voz 4).
- O campo é feito dos compassos da referência (densidade × síncope), e a
  resposta navega por ele conforme o modo:
  - seguir: 16 % fora do tempo;
  - transfigurar: 85 % fora do tempo;
  - contrapor: mais denso.
- Só entra nos trechos com pulso. Tambor no grave; striker, LPG ou corda no
  agudo.
- Ver `docs/RITMO.md`. 17/17 testes.
- A **v13** está em
  `/media/luc/4tb_21042021/TRIOIO_respostas/v13-ritmo-campo/` (apagada em 2026-10-16: arquivo de teste).

**2026-10-08 — memória pesquisável por timbre (Atlas §62, CataRT).**

- É a segunda memória do Trioio (voz 5, canal 6 do MIDI): cada ataque da
  referência vira uma unidade (energia, brilho, ruído, altura, duração), e
  a resposta busca as unidades que soam como o momento pede:
  - seguir: o mesmo som;
  - contrapor: energia e ruído opostos;
  - transfigurar: brilho invertido;
  - interromper: em fragmentos.
- Custo do alvo mais custo da emenda (seleção de unidades de Schwarz).
- Cada nota guarda de onde veio (`Note::source`) e toca com o corpo
  espectral **daquele instante**; a memória do oráculo passou a fazer o
  mesmo.
- Teste em trechos alternados claros e escuros: seguir 174/174 do mesmo
  tipo de trecho, contrapor 1/137, transfigurar 15/117.
- `--mosaico nao` desliga. Ver `docs/MOSAICO.md`. 18/18 testes.
- A **v14** está em `/media/luc/4tb_21042021/TRIOIO_respostas/v14-mosaico/` (apagada em 2026-10-16: arquivo de teste).
  O render 5 é o mesmo seed do 1 sem mosaico, para comparar.
- **Próximos candidatos do Atlas:** Frames (§40), estados e interpolação;
  Antescofo (§60.1), acompanhar ao vivo; Streams (§46); o oráculo com
  símbolos de timbre (ordem × som).

**2026-10-08 — vários formatos de áudio (pedido do autor).**

- A Biblioteca aceita WAV, AIFF, FLAC, Ogg, Opus e MP3 pelo JUCE, e
  M4A/AAC, WebM, WMA e outros pelo ffmpeg, se instalado. O ffmpeg
  decodifica para um WAV temporário e apagado; o original fica intacto.
- **O formato é lido pelo conteúdo.** O `Recording_215.mp3` do vocalise é
  MP4/AAC de gravador Android; antes, o leitor de MP3 do JUCE decodificava
  lixo (98,9 s em vez de 81,8 s). O manifesto ganha um `aviso`.
- **Analisador v4:** a planura passou a ser medida entre 50 Hz e 10 kHz,
  com piso de −80 dB. Os codecs com perda faziam ruído parecer tom
  (planura 0,00 em MP3, Ogg e AAC; agora de 0,52 a 0,55). Os renders
  anteriores à v15 usaram o analisador v3.
- Ver `docs/FORMATOS_DE_AUDIO.md`. 18/18 testes; o mosaico relaxou o limite
  de "contrapor" para 15 % (14/137).
- A **v15** está em `/media/luc/4tb_21042021/TRIOIO_respostas/v15-vocalise/` (apagada em 2026-10-16: arquivo de teste)
  e `v15-suno/`: as duas referências novas do autor, nos 4 modos.
- **Pendente:** o Frames (§40) foi desenhado, ainda sem código. Estados por
  seção com interpolação, e a largura da passagem medida na referência:
  quem segue corta onde ela corta e interpola onde ela muda devagar;
  transfigurar e contrapor invertem.

**2026-10-08 — banco de escalas (pedido do autor).**

- O Trioio tinha 6 modos. Agora traz o catálogo de 45 escalas, as escalas
  inventadas por crivo de Xenakis e as afinações justa, pitagórica e de
  quartos de tom, copiados do Studio (cópia fixada, `PROVENANCE.md`).
- Adaptação própria: um crivo em 19, 22 ou 31-EDO com graus fora dos 12
  semitons.
- `--alturas catalogo|inventada|edo|justa|pitagorica|quartos`. Ver
  `docs/ESCALAS.md`. 19/19 testes.
- **Em desenho:** a máquina de escalas (`.scl`/Scala; escala ↔ harmonia;
  encadeamentos e desvios; harmonia nascida das linhas), no fim de
  `docs/ESCALAS.md`.

**2026-10-08 — roteiro de musicalidade (pedido do autor).** "Precisamos
de mais musicalidade, técnicas e astúcias de composição; investigar a fundo
teoria musical, linguagens, comportamentos, estilos, culturas."
`docs/MUSICALIDADE.md` traz o diagnóstico (a faixa do Suno: Sol menor
harmônica, i–V–iv–III–VI, ~85 BPM; o Trioio não lê acordes, dobra o pulso
e não acha repetição), o inventário dos motores do Studio a copiar e o
dossiê a preencher. Ordem das camadas:

1. **harmonia em movimento** (em andamento): acordes da referência ou da
   linha, baixo na fundamental, melodia nas notas do acorde, cada modo
   transforma a progressão;
2. escuta: pulso sem dobro, forma por repetição, grave e voz;
3. máquina de escalas: `.scl`, variantes por acorde, encadeamentos e
   desvios;
4. voicings e tratamento da dissonância (Studio);
5. técnicas do dossiê, uma por vez.

Continuam pendentes o Frames (§40) e a decisão sobre os 1,8 GB de renders
antigos em `~/Music/TRIOIO_respostas`.

**2026-10-08 — camada 1: harmonia em movimento.**

- Acordes lidos tempo a tempo por Viterbi (Sheh e Ellis): 32/32 com
  acordes e 32/32 com uma linha só (a harmonia nasce da linha).
- Modos: transfigurar por transformação neo-riemanniana (cópia fixada do
  Studio); contrapor por substituição de trítono e mediantes cromáticas;
  interromper em fragmentos.
- O baixo segue a fundamental; o motivo cai no acorde nos ataques fortes;
  a voz de harmonia (6) tem condução parcimoniosa (2,9 semitons por
  troca).
- `--harmonia nao` desliga. Ver `docs/HARMONIA.md`. 20/20 testes.
- A **v16** está em `/media/luc/4tb_21042021/TRIOIO_respostas/v16-harmonia-suno/` (apagada em 2026-10-16: arquivo de teste)
  e `v16-harmonia-vocalise/`. `1b` é o mesmo seed sem harmonia.
- **Limite visto no Suno:** a voz na mistura confunde acordes ("Mibm" por
  Mib).
- **Próximo:** grave pesado, sétimas e tonalidade global na leitura; depois
  a camada 2 (pulso sem dobro, forma por repetição).

**2026-10-08 — arquivo de escalas próprio.**

- O arquivo do Scala (mais de 5000 escalas, Op de Coul) foi examinado.
  **Decisão do autor:** não usar nem distribuir arquivos de terceiros;
  escrever o nosso a partir do conhecimento público, citando as fontes como
  numa pesquisa.
- `data/escalas/arquivo_trioio.txt` tem 187 escalas: gregas, temperamentos,
  justa, EDOs, não oitavantes, maqam, makam, shrutis, thāts, 72
  melakartas, gamelão, Tailândia, Uganda, Moçambique, China, Japão,
  Nordeste. É gerado por `tools/gerar_arquivo_de_escalas.py`.
- `--escala TERMO|arquivo.scl` e `--alturas arquivo`; escalas do autor em
  `<dados>/escalas/`. Escalas não oitavantes funcionam (período no
  compositor).
- Ver `docs/ESCALAS.md`. 21/21 testes.
- A **v17** está em `/media/luc/4tb_21042021/TRIOIO_respostas/v17-arquivo-de-escalas/` (apagada em 2026-10-16: arquivo de teste):
  vocalise com pelog, maqam e melakarta; Suno com maqam (escolheu Hijazkar
  sobre Ré), Werckmeister III, Bohlen-Pierce e todas.
- **Próximo:** ampliar o arquivo (dastgah persa, echoi bizantinos,
  modos de Messiaen, mais rāgas com as fontes); temperamentos com tônica
  fixa em Dó; a máquina de escalas (variantes por acorde, encadeamentos).

**2026-10-09 — banco por regra, máquina de escalas, harmonia em qualquer escala.**

- **Banco:** 2559 escalas próprias, geradas por regra e com fontes: MOS,
  espécies gregas, EDO 5–100, CPS, Euler-Fokker, ajnas, séries, Forte,
  Messiaen, mesotônicos.
- **Máquina** (`--maquina sim`): rotação, alteração, espelho,
  interpolação, pivô, acréscimo, supressão, alargamento, encadeados por
  seção conforme o modo.
- **Harmonia** pelos graus de qualquer escala (microtonal, não oitavante).
- 22/22 testes. A **v18** está em
  `/media/luc/4tb_21042021/TRIOIO_respostas/v18-maquina-de-escalas/` (apagada em 2026-10-16: arquivo de teste).
- **Recurso transversal** (declaração do autor): `KNOW-ESCALAS` no
  inventário global e no Atlas.
- **Variantes por acorde** (mesmo dia): sob cada acorde, a escala se curva
  para conter as notas dele (Lá menor sob Mi → Sol#), e o motivo e o eco
  seguem.
- **Afeto e intenção** (mesmo dia): mapa afetivo das escalas
  (`ScaleAffect`), que ordena os modos como Temperley e Tan; e
  `--intencao` guiando a máquina. 23/23 testes; v19 em
  `/media/luc/4tb_21042021/TRIOIO_respostas/v19-intencao/` (apagada em 2026-10-16: arquivo de teste). O estudo
  transversal com as fontes está em
  `RASGO_DOCUMENTATION/teoria/ESCALAS_AFETO_INTENCAO_E_FUNCAO.md`, com as
  linguagens, estilos e técnicas de improvisação (§6).
- **Comportamento das escalas** (mesmo dia): subida e descida, apoio,
  percurso, assinatura, cadência, ornamento e textura bordão, lidos do
  arquivo e aplicados por `ScaleIdiom`. Primeiras escalas: 4 maqamat,
  Yaman, Bhairav, 3 pathet (a conferir), blues e modo de Mi. 24/24
  testes; v20 em
  `/media/luc/4tb_21042021/TRIOIO_respostas/v20-comportamento/` (apagada em 2026-10-16: arquivo de teste).
- **Curva de altura e heterofonia** (mesmo dia): `Note::pitchAt` com
  pontos e oscilação; andolan, gamaka e blue notes reais (medidos no
  áudio); heterofonia para maqamat e pathet (Racy, 2003). 24/24 testes;
  v21 em `/media/luc/4tb_21042021/TRIOIO_respostas/v21-ornamentos-heterofonia/` (apagada em 2026-10-16: arquivo de teste).
- **Misturas** (mesmo dia; pedido do autor depois de ouvir a v21):
  colagem, híbrido, camadas e transição entre culturas
  (`--mistura … --tecnica …`); textura e comportamento por seção. 25/25
  testes; v22 em `/media/luc/4tb_21042021/TRIOIO_respostas/v22-misturas/` (apagada em 2026-10-16: arquivo de teste).
- **Ciclos das culturas** (mesmo dia): arquivo de 26 ciclos próprios com
  fontes (iqa', tala, usul e aksak, claves, colotomia, compás, shuffle);
  cada escala chama o seu; `--ciclo` força um ciclo (mashup de ritmo).
  26/26 testes; v23 em `/media/luc/4tb_21042021/TRIOIO_respostas/v23-ciclos-ritmicos/` (apagada em 2026-10-16: arquivo de teste).
- **Chegadas e tihai** (mesmo dia): a melodia chega no sam, no gong e no
  dum, na nota de apoio; os tala fecham as seções com tihai. 26/26 testes;
  v24 em `/media/luc/4tb_21042021/TRIOIO_respostas/v24-chegadas-tihai/` (apagada em 2026-10-16: arquivo de teste).
- **Mashup de materiais** (mesmo dia): `--com OUTRO_ID --da-outra
  harmonia,ritmo,memoria,corpo`; B reescalada no tempo de A, com o mesmo
  portão de autorização e as duas fontes na proveniência. 27/27 testes; v25
  em `/media/luc/4tb_21042021/TRIOIO_respostas/v25-mashup/` (apagada em 2026-10-16: arquivo de teste) (a voz do
  vocalise sobre o Suno).
- **Prática modal** (mesmo dia): os seis modos como linguagem (nota
  característica, cadência modal, textura vamp sobre pedal; Persichetti,
  Levine). 27/27 testes; v26 em `/media/luc/4tb_21042021/TRIOIO_respostas/v26-modal/` (apagada em 2026-10-16: arquivo de teste).
- **Voicings** (mesmo dia): terças, quartal, So What, planing, bitonal,
  quintas abertas, cluster (`--voicing`; princípios do `MelodyVoicingBank`
  do Studio). 28/28 testes; v27 em
  `/media/luc/4tb_21042021/TRIOIO_respostas/v27-voicings/` (apagada em 2026-10-16: arquivo de teste).
- **Costuras** (mesmo dia): `--costura corte|pivo|ponte|sobreposicao` nas
  fronteiras entre seções. 28/28 testes; v28 em
  `/media/luc/4tb_21042021/TRIOIO_respostas/v28-costuras/` (apagada em 2026-10-16: arquivo de teste).
- **Andamento pela escuta** (mesmo dia): `--andamento metade|dobro`. A
  medida mostrou que o pulso do Suno é genuinamente ambíguo (172 em todas
  as bandas), o que corrige o "~85 BPM" do diagnóstico. A regra
  automática de nível métrico foi tentada e desfeita, por não agir em
  nenhum caso testável (`docs/MUSICALIDADE.md`). v29. Depois o autor
  confirmou: "a contagem é feita mais lenta". A preferência do estimador
  passou de 120 para 100 BPM (Parncutt 1994; London 2004), e o Suno sai a
  85 sozinho.
- **Forma por repetição** (2026-10-09/10): `--forma repeticao`
  (novidade de Foote; letras pela diagonal da matriz de
  autossimilaridade, Müller); a seção que volta retoma o material. O A B A
  B construído sai certo; no Suno e no vocalise, nenhuma repetição clara.
  Falta a forma real de uma canção para calibrar o limiar. v30.
- **Próximo:** calibrar a forma com uma referência de forma conhecida;
  mukhṛā e tihais maiores;
  andamentos próprios (vilambit, drut); costuras entre culturas na colagem;
  mashup de materiais; conferir os pathet em Hood; mais rāgas e maqamat
  com fonte;
  as outras formas de gamaka; o comportamento nas memórias; dastgah, echoi,
  rāgas contextuais; um segundo consumidor (módulo de escala no Rasgo
  Modular); depois a camada 2 (pulso sem dobro, forma por repetição).

**Interface (proposta, não decidida, 2026-10-07):**

- três faixas alinhadas no tempo: referência, Terceiro Olho e resposta;
- o olho escondido até "revelar depois de ouvir", e as consequências
  desenhadas como linhas até o fim;
- a biblioteca com o estado de proveniência;
- "sua escuta" com comparação de versões e aceitar, rejeitar ou
  transformar.

Fica para depois do `escuta.json`, com identidade visual própria do Trioio.

**2026-10-10:** o esboço segue as diretrizes de layout
(INTERFACES_E_LAYOUTS §4–§6) em [`docs/design.md`](docs/design.md):

- as seis perguntas, o mapa de tarefas e a classificação dos controles;
- os regimes completo (1920×1080), compacto (1024×650) e emergência
  (960×600), com a conta de pixels das superfícies mínimas;
- o canvas privado em https://claude.ai/artifact/WUkdez7xVzVkdGMy1KPczM,
  em estado `exploration`.

Nada foi compilado: o autor está renderizando um álbum. **Próximo:**

1. o autor escolhe a direção visual;
2. ~~desenhar as superfícies Biblioteca e Sua escuta~~ (feito em
   2026-10-10, no compacto 1024×650);
3. o protótipo JUCE no regime compacto.

**2026-10-10, julgamento por trecho especificado** (`docs/ESCUTA.md`):

- `--trecho m:ss-m:ss|fim` e `--secao N|letra`;
- o timbre pesa pelos segundos que soam dentro do trecho, e o olho pelas
  marcas dentro dele;
- a sala pesa pela fração da peça; os trechos do mesmo ato têm a mesma
  idade;
- o esquema passa a `trioio.escuta/2` e continua lendo o `/1`;
- seis testes previstos.

**Implementado no mesmo dia, depois do álbum.**

- Validação:
  - os seis testes novos;
  - uma mutação no agrupamento por ato faz o teste falhar;
  - a suíte 28/28;
  - uma prova de ponta a ponta numa resposta real.
- O peso por segundos foi trocado por notas fracionárias, porque o pad
  dominava.
- **Próximo:**
  1. guardar nas marcas do olho a duração das consequências, para que o
     trecho possa contar a intervenção anterior que ainda soa nele;
  2. o protótipo JUCE no regime compacto.

**Ao retomar, nesta ordem:**

1. **Ouvir a v5 com o autor**, ao lado da v4. A pergunta: as consequências
   tiraram a sensação de "equivalente"? O autor prefere seguir
   desenvolvendo sem parar a cada escuta.
2. ~~**Escolha contextual das intervenções do Terceiro Olho**~~ (feito em
   2026-10-07).
3. ~~**Escuta do autor registrada em `escuta.json`**~~ (feito em 2026-10-08,
   `docs/ESCUTA.md`): aceitar, rejeitar ou
   transformar, ajustando tendências locais.
4. **Versão ao vivo:** vozes pré-alocadas, a escuta ao vivo (o
   `FrameAnalyzer` já é compartilhado) e o modo RESPONDER AGORA.
5. **Pendências menores:**
   - compasso de 3 ou 6 com acento interno;
   - o brilho em contrapor sai positivo na música real;
   - `prompts.json` e o vocabulário de intenções, que ainda não existem.

## Continuidade de acervo — proporcional ao desenvolvimento

- [ ] Quando houver uma versão estável escolhida para release, publicação,
  migração ou entrega, definir o recorte de fontes e documentação, gerar
  manifesto/checksum e testar a restauração do pacote. — 2026-10-18: a ferramenta está pronta e testada (`tools/empacotar_release.sh`: recorte pelo git, MANIFESTO.sha256, restauração com os testes do core; `docs/MULTIPLATAFORMA.md`). Falta a escolha da versão pelo autor
- Enquanto o Trioio estiver em investigação e prototipagem, registrar decisões,
  testes e próximo passo neste arquivo ou nos dossiês locais. Não criar pacote
  de preservação a cada experimento.

## Marco 1 — fundação do Cérebro

- [x] Registrar proposta conceitual e eixo de composição musical/escuta
- [x] Definir Cérebro, Terceiro Olho, memória, estratégia e decisão
- [x] Criar contrato portátil `BrainDecision`
- [x] Criar `EnergyControlBus` sem dependência de JUCE
- [x] Sanitizar valores não finitos e limitar o barramento a `[0, 1]`
- [x] Criar primeiro teste automatizado de segurança numérica
- [x] Inventariar código próprio reutilizável do RASGO e separar lacunas do TRIOIO
- [x] Provar publicação/leitura concorrente em teste dedicado
- [x] Definir e testar snapshot versionado para múltiplas decisões coerentes
- [x] Definir manifesto de proveniência para áudio, metadados, prompt original,
      análise inferida e autorização de uso — 2026-10-05:
      [`docs/MANIFESTO_DE_PROVENIENCIA.md`](docs/MANIFESTO_DE_PROVENIENCIA.md)
      (esquema `trioio.manifesto/1`) e contrato executável
      `src/core/Provenance.h/.cpp`: modelo, `validate()` campo a campo,
      portão `permits()` por uso (listen → analyze → learn/derive →
      publishDerivative; pendente só ouve) e `canPublishDerivation()`.
      `tests/ProvenanceTests.cpp`: 12 cenários (autoral, IA com termos de
      serviço, licença, escopo parcial, revogação, arquivo alterado/ausente,
      derivação com fonte revogada). Conferido por mutação: quebrar a regra
      de origem não verificada faz o teste falhar.
      **Achado junto:** o `trioio_brain_tests` usava `assert` e, em build
      Release, o NDEBUG o desligava — passava sem verificar nada. Os testes
      agora compilam com `-UNDEBUG` (`/UNDEBUG` no MSVC); conferido que uma
      asserção quebrada aborta em Release. Validação: Release e Debug, 2/2.
      **Próximo passo:** Marco 2 — escolher a biblioteca de JSON (licença
      compatível com AGPL), ler/gravar `manifesto.json` e o importador que
      cria o manifesto `pending` no drop.

**Marco 1 concluído em 2026-10-05.**

**2026-10-05 — licença e `manifesto.json` em disco.** Decisão do autor:
"usar as licenças que temos usado habitualmente, C++ e JUCE, software livre".
`LICENSE` (AGPL-3.0-or-later, mesmo texto do Rasgo Modular),
`CREDITS_AND_SOURCES.md` (formato da família, que substitui os
`CREDITS.md`/`THIRD_PARTY_NOTICES.md` previstos: não há código de terceiros
a listar) e `docs/LICENSE_STATUS.md` (JUCE 9.0.0 sob AGPLv3). O JSON usa o
`juce::JSON`, sem dependência nova; a camada `src/library/` usa o JUCE, e o
núcleo continua sem ele (`-DTRIOIO_JUCE_PATH`, opcional). CMake: projeto
passa a `VERSION 0.0.1` e `LANGUAGES C CXX` (exigências do JUCE).
`ManifestJson`: ida e volta exata (UTF-8, seed de 64 bits em texto), erro
em valor desconhecido, recusa de outra versão de esquema; o teste lê o
exemplo publicado na especificação. Validação: com JUCE 3/3, só núcleo 2/2,
sem avisos; mutações conferidas (seed como número e categoria errada no
exemplo do documento fazem o teste falhar).

## Marco 2 — percepção e contexto

- [x] Criar dossiê operacional da Biblioteca: indexação, importação por drop,
      manifestos, sessões, cache, memória local, prompts e aprendizagem — 2026-10-18: `docs/BIBLIOTECA_OPERACAO.md` (a pasta de dados, o fluxo, os estados, a memória e o que o performer desfaz, os recursos)
- [x] Definir raízes configuráveis da Biblioteca e diretório de dados local,
      mantendo o acervo fora do repositório e sem alterar arquivos de origem
      — 2026-10-05: `scanLibrary(biblioteca, dados)`; dados dentro da
      Biblioteca é recusado; Biblioteca conferida intacta por teste
- [x] Definir formatos versionados de `manifesto.json`, `prompts.json`,
      `decisoes.json` e `escuta.json` — 2026-10-18: `docs/FORMATOS_DE_DADOS.md`
      (`trioio.manifesto/1`, `trioio.decisoes/1` com todos os campos, inclusive
      os do jogo, `trioio.escuta/2`; a regra de versão: campo novo opcional não
      muda o número; mudança de sentido sobe). `prompts.json` só nasce com os
      prompts (Marco 2)
- [x] Definir identidade estável de referência para reuso seguro de cache e
      associação de sessões sem depender apenas do caminho do arquivo —
      2026-10-05: o conteúdo (SHA-256); ver `docs/IMPORTADOR_DA_BIBLIOTECA.md`
- [x] Definir estados de importação: pendente, autorizado, analisando,
      disponível, indisponível e alterado desde a análise — 2026-10-05: `authorization.status` (pending/authorized/refused/revoked) separado de `analysis` (none/indexing/analyzing/cached/changed/unavailable); `docs/MANIFESTO_DE_PROVENIENCIA.md` §4
- [x] Definir política de retenção e limpeza para cache, sessões, prompts e
      tendências aprendidas, sempre reversível pelo performer — 2026-10-17,
      para as sessões: `trioio_biblioteca limpar` (julgadas e recentes ficam;
      as de teste vão para a lixeira do dia, reversível) e
      `esvaziar-lixeira` (só depois de 7 dias); ver
      `docs/ESTRUTURA_DO_PROJETO.md`. 2026-10-18: o cache (`limpar-cache`), os prompts (`prompt --revisar|--apagar`) e as tendências (recalculadas de `escuta.json`, desligáveis por resposta), em `docs/BIBLIOTECA_OPERACAO.md`
- [x] Definir descritores mínimos de energia, densidade, ataques e espectro
      — 2026-10-05: `docs/DESCRITORES.md`
- [x] Criar janela temporal e memória curta sem armazenar áudio integral — 2026-10-18: `core/ShortMemory.h` (anel pré-alocado de descritores, sem áudio, `push` sem alocação; o resumo da janela: nível, tendência, ataques/s, f0); a base da escuta ao vivo, ainda sem uso no app
- [x] Produzir relatório offline de relações musicais — 2026-10-05,
      primeira versão: resumo, linha do tempo e "onde muda mais"
      (`trioio_biblioteca analisar`); relações entre materiais ainda não
- [x] Definir vocabulário inicial de intenções de prompt — 2026-10-18: `core/Intent.h` (energia, densidade, brilho, harmonicidade, andamento, humor, estilo, linguagem harmônica, voz; léxico pt/en/fr/es); `docs/PROMPTS_E_INTENCOES.md`
- [x] Importar WAV, stems e MIDI sem pressupor uma plataforma ou serviço
      específico como origem — 2026-10-05: `trioio_biblioteca varrer` (wav/aif/flac/mid; stems por pasta)
- [x] Separar prompt original disponível de prompt inferido pelo TRIOIO — 2026-10-18: `prompts.json` com `promptOriginal` (só se fornecido), `declaredIntent` e `analysisInferred` separados (`library/Prompts.h`)
- [x] Traduzir prompts externos fornecidos pelo usuário para intenções próprias
      (`declaredIntent`) sem alegar recuperar parâmetros internos do gerador — 2026-10-18: `translatePrompt`, com a palavra de cada campo à vista e as não lidas; revisável (`prompt --revisar`)
- [x] Comparar intenção declarada e comportamento acústico observado como entrada
      do Terceiro Olho — 2026-10-18: `compareIntent`; a distância média abre o Terceiro Olho (imprevisibilidade até +0,25) em `applyIntent`
- [x] Aceitar conjunto autorizado de prompts e versões geradas como material de
      referência, preservando proveniência e separando recombinação autoral de
      cópia ou treinamento de serviço externo — 2026-10-18: cada versão é uma referência autorizada com o seu `prompts.json` (fonte `gerador`), comparadas por `versoes`; recombinação autoral em síntese própria, sem cópia nem treinamento (`docs/VERSOES.md`)
- [x] Criar comparação entre versões: invariantes, divergências e operações
      possíveis para a síntese energética — 2026-10-18: `trioio_biblioteca versoes` (`core/Versions.h`, `docs/VERSOES.md`): andamento, tom, forma, progressão, duração e intenção ouvida; as operações (mashup, contrapor, linhas)
- [x] Manter referências externas e internas registradas por módulo, separando
      inspiração conceitual, código próprio e licença — 2026-10-18: `docs/REFERENCIAS_POR_MODULO.md`, gerado por `tools/gerar_referencias_por_modulo.py` (62 módulos: função, fontes externas citadas, referências internas, origem do código)
- [x] Criar `CREDITS.md`/inventário de terceiros por marco, com autores, fontes,
      licenças, permissões, transformações e avisos exigidos — 2026-10-05: `CREDITS_AND_SOURCES.md` (nenhum código de terceiros; JUCE em AGPLv3)
- [x] Definir fluxo geral de referência autoral: áudio + prompt opcional +
      metadados → manifesto → análise e síntese TRIOIO — 2026-10-18: `docs/BIBLIOTECA_OPERACAO.md`, "O fluxo de uma referência autoral" (varrer → autorizar → analisar → prompt → responder → escutar)
- [x] Registrar versões, stems, MIDI e decisões derivadas do acervo próprio — 2026-10-18: o manifesto de cada derivação registra no histórico as decisões, a partitura MIDI, os stems e a intenção usada, com o caminho na sessão; as versões se comparam por `versoes`
- [x] Rejeitar no importador materiais de terceiros sem autorização explícita — 2026-10-05: tudo entra pendente; análise e derivação barradas sem `analyze`/`derive` (`permits`)

## Marco 3 — estratégia e síntese

- [x] Implementar modos seguir, transfigurar, contrapor e interromper —
      2026-10-05, primeira versão (`src/core/Strategy`, `docs/CADEIA_MINIMA.md`)
- [x] Conectar decisões a um primeiro motor de síntese energética próprio —
      2026-10-05, `src/core/EnergySynth` (simples, mensurável, sem alocação)
- [x] Renderizar comparação A/B com decisões registradas — 2026-10-05, primeira forma: `responder` grava WAV + MIDI + `decisoes.json` por modo/seed/opções e mede a correlação com a referência; falta a comparação lado a lado numa ferramenta
- [x] Validar por escuta a diferença entre intenção e consequência — 2026-10-18: o material em `escutas/2026-10-18-intencao-e-consequencia/` (A sem intenção × B com uma intenção oposta ao que soa); o autor escutou: "senti as diferenças". A intenção declarada muda a resposta de modo audível. Ainda não julgado: se B soa como a intenção pedida (tensa, rock, energia alta)

## Marco 4 — performance e engenharia

- [x] Integrar áudio, prompt e gesto em modo híbrido — 2026-10-18 (offline): o áudio dá a matéria, o prompt a intenção, o gesto (`--gesto "0:0.1,0.75:1,1:0.2"`) a forma; `docs/PROMPTS_E_INTENCOES.md`, "O modo híbrido". No app: o campo "Gesto" e o editor desenhável no painel Linguagem, sincronizados (o mouse ainda sem teste); falta a reorganização ao vivo
- [x] Separar threads de análise, estratégia e áudio — 2026-10-18: a interface, o trabalho de cada resposta (análise, composição, síntese), a síntese dos barramentos em paralelo (até 3), a leitura do disco na reprodução (TimeSliceThread; antes o callback lia o disco) e o callback de áudio só com buffers; `docs/MEDICOES.md`
- [x] Medir CPU, RAM, latência e memória por perfil — 2026-10-18: `tools/medir_perfis.sh` e `TRIOIO_TEMPOS=1`; `docs/MEDICOES.md`. A medição achou o piano (98,8 de 103,5 s no jazz), a palheta e os metais e os desnormalizados: o jazz caiu de 126 para 22 s, com o mesmo som
- [x] Preparar gravação estéreo de alta qualidade e stems — 2026-10-17:
      `responder --stems sim`, um WAV de 24 bits por voz, os efeitos e a sala,
      com o ganho e a curva do master da mistura (`docs/COMO_TOCAR.md` §10);
      2026-10-18: a caixa no app e os stems por voz também no modo linhas
- [ ] Validar Windows, macOS e Linux — 2026-10-18: Linux validado; o core limpo no clang; o runtime estático do MSVC, o Universal 2 e o alvo 10.15; os dados junto do binário (`DataFiles.h`). Falta o portão de release: ícone, CPack, CI dos três sistemas (decisão de push) e o teste do autor (`docs/MULTIPLATAFORMA.md`)

## Marco transversal — organização do projeto

- [x] Definir estrutura inspirada na organização do Navalha, sem copiar código
- [x] Renomear de "Maravi Tri Oio" para Trioio (apelido: 3oio), 2026-08-20 —
      autor: "o instrumento maravi tri oio, pretendo renomea-lo passaria a se
      chamar '3oio' ou 'trioio', e o 'Maravi' passaria a ser o nome do novo
      projeto rasgo para audio-to-score que vamos implementar". Verificado sem
      conflito com nenhum synth/plugin/projeto musical existente (nem "3oio"
      nem "Trioio"); diretório `MARAVI_TRI_OIO/` → `TRIOIO/`, CMake
      (`project(MaraviTriOio)`→`Trioio`, alvo `maravi_core`→`trioio_core`,
      teste `maravi_brain_tests`→`trioio_brain_tests`), código
      (`MaraviBrain.h/.cpp`→`TrioioBrain.h/.cpp`, `namespace maravi`→
      `namespace trioio`), 6 docs internos + HTML da proposta, e ~16 arquivos
      de referência cruzada no resto do RASGO. Build limpo
      (`-Wall -Wextra -Wpedantic`), `trioio_brain_tests` passando via `ctest`.
      `MARAVI_TRI_OIO_PROPOSTA.pdf` só teve o NOME do arquivo atualizado
      (`TRIOIO_PROPOSTA.pdf`) - é binário, conteúdo textual interno continua
      dizendo "Maravi Tri Oio" até ser regenerado a partir de uma fonte
      editável (não localizada). `0_BRAINSTORM/maravi_tri_oio.txt` deixado
      INTOCADO de propósito - é o registro histórico original (mesmo
      precedente já usado noutros materiais de `0_BRAINSTORM/` no RASGO,
      nunca editados retroativamente), e nele encontrei a origem real do nome
      antigo: "anagrama vitoriamario - maravi tri oio" - ou seja, "Maravi" foi
      concebido originalmente como um anagrama de "Vitoriamario" (a
      identidade central do projeto, ver `TRIOIO_CONCEITO.md` seção 1), não
      como abreviação de "maravilha" (a origem que o autor deu pro NOVO uso
      do nome, num contexto diferente). A frase `TRIOIO_CONCEITO.md`:13
      ("`TRIOIO` é a forma anagramática de `VITORIAMARIO`") foi herdada
      mecanicamente da troca de "MARAVI"→"TRIOIO" - conferido que continua
      literalmente verdadeira (as letras T,R,I,I,O,O todas estão em
      "VITORIAMARIO"), mas o autor não pediu essa afirmação especificamente;
      fica sinalizado caso prefira reescrever/remover.
- [x] Criar `LICENSE` e créditos do TRIOIO — 2026-10-05: `LICENSE`
      (AGPL-3.0-or-later), `CREDITS_AND_SOURCES.md`, `docs/LICENSE_STATUS.md`
- [x] Migrar documentação da raiz para `docs/` após atualizar referências — 2026-10-18: `TRIOIO_CONCEITO.md`, `TRIOIO_CEREBRO.md` e `TRIOIO_INVENTARIO_RASGO.md` para `docs/` (git mv: o histórico fica); na raiz, um aviso com o novo caminho para os links de fora (RASGO_DOCUMENTATION, MARAVI); as referências do Trioio atualizadas. Ficam na raiz por convenção: README, LICENSE, CREDITS_AND_SOURCES e este TAREFAS (a continuidade)
- [ ] Criar `src/app`, `src/tools`, `src/validation`, `validation` e
      `packaging` somente quando cada diretório tiver conteúdo real — 2026-10-18: `src/app` e `src/tools` existem com conteúdo; `src/validation`, `validation` e `packaging` ainda não têm conteúdo real e não foram criados (o `packaging` nasce com o ícone e o CPack, `docs/MULTIPLATAFORMA.md`)

## Registro — 2026-10-05: importador da Biblioteca

`src/library/LibraryImporter.h/.cpp` e a ferramenta `trioio_biblioteca`
(`varrer`, `listar`), documentados em `docs/IMPORTADOR_DA_BIBLIOTECA.md`.
Material novo entra pendente; identidade pelo SHA-256 do conteúdo (mover
preserva a autorização, substituir conteúdo não herda); ausentes e
substituídos viram `unavailable`, nada é apagado; manifesto ilegível nunca é
sobrescrito; gravação atômica; limites e cancelamento. Módulos JUCE usados:
`juce_core`, `juce_audio_formats`, `juce_cryptography` (AGPLv3).
Validação: 4/4 testes (Release, sem avisos), mutações conferidas; pasta real
`/home/luc/Music/Rasgo Synth` (720 MB) lida só para leitura: 21 WAV em 25 s,
segunda passada sem mudança, pasta intacta. **Próximo passo:**
`trioio_biblioteca autorizar` (declarar origem/fundamento/escopo com
`validate()` antes de gravar) e depois os descritores mínimos.

**2026-10-05 — declaração de autorização.** `src/library/Declaration.h/.cpp`
e os comandos `autorizar`, `recusar` e `revogar` da `trioio_biblioteca`. Nada
é gravado sem passar em `validate()`; recusar só pendente, revogar só
autorizado; voltar a autorizar fica no histórico. Validação: 5/5 testes
(Release, sem avisos), mutação conferida, ponta a ponta pela ferramenta.
**Próximo passo:** descritores mínimos (energia, densidade, ataques,
espectro) para material com `analyze` liberado, cache ligado ao
`contentSha256`, e o primeiro relatório offline.

## Registro — 2026-10-05: descritores mínimos e relatório offline

`src/core/Descriptors` (núcleo, sem JUCE: FFT própria, RMS, centroide,
espalhamento, planura, fluxo log, ataques por limiar adaptativo, resumos por
trecho e contraste) e `src/library/Analysis` (portão `analyze`, SHA-256 antes
de analisar, cache `trioio.analise/1` idêntico na releitura, relatório).
Comando `trioio_biblioteca analisar`. Especificação e validação em
`docs/DESCRITORES.md`. Três defeitos achados e corrigidos no caminho:
precisão do cache (o parâmetro do `JSON::toString` são casas decimais),
arquivo reescrito no mesmo segundo passando pela checagem de data (agora
SHA-256) e linha do relatório cortada. Duas decisões de escuta: brilho em
oitavas; silêncio não tem timbre. Validação: 7/7 com JUCE e 3/3 só núcleo,
sem avisos; mutação conferida; duas gravações reais do Rasgo Synth
analisadas só para leitura. Candidato a inventário global
(`Descriptors` é reaproveitável, mas fica local até haver segundo
consumidor). **Próximo passo:** janela curta ao vivo (RESPONDER AGORA) ou
os descritores restantes do §3.1 (f0, harmonicidade, pulsação, estéreo,
novidade) — decisão do autor.

**2026-10-05 — Biblioteca local pronta para o autor.** Pedido: "compile e
deixe pronto". Ferramenta compilada em `TRIOIO/build/` (ignorado pelo git;
binário `build/trioio_biblioteca_artefacts/Release/trioio_biblioteca`,
commit 6a51c17). Raiz de dados: `~/TRIOIO_DATA` (fora do repositório e da
pasta de música). As 22 gravações de `/home/luc/Music/Rasgo Synth` foram
registradas e autorizadas **só para `analyze`** (origem `otherInstrument`,
ferramenta "Rasgo Synth", fundamento `ownWork`, declarado em nome de Lúcio
de Araújo, com a restrição "declarado pelo Claude Code a pedido do autor").
Pasta de música conferida intacta. Para derivar ou publicar, o autor
redeclara com `--usos derive` ou `publishDerivative`.

## Registro — 2026-10-05: cadeia mínima audível (o TRIOIO faz som)

Pedido do autor: "siga com a cadeia mínima audível" (a escuta ao vivo fica
depois; o `FrameAnalyzer` compartilhado já foi feito, commit 4151104).
`Strategy` (quatro modos), `EnergySynth` (motor próprio, sem alocação) e
`Response` (comando `trioio_biblioteca responder`, exige `derive`): grava
WAV, `decisoes.json` e manifesto de derivação pendente na raiz de dados, e
mede no som o quanto a resposta segue ou contraria a referência. Defeito
achado: relógio de eventos preso à taxa do instante do disparo (corrigido
com acumulador de fase). Validação: 9/9 testes, sem avisos; Dilettante
faixa 7 respondida nos quatro modos (dados descartáveis), correlações
coerentes com cada modo, pasta de música intacta. Ferramenta do autor em
`TRIOIO/build` recompilada com `responder`. **Pendente:** escuta do autor
(o ouvido julga o som; a correlação só prova a estratégia); para gerar
respostas em `~/TRIOIO_DATA`, as referências precisam de `--usos derive`.

## Registro — 2026-10-05: composição (motivo, desenvolvimento, forma)

Escuta do autor sobre a cadeia mínima: "ainda meio robótico… não vejo ainda
muita qualidade musical". Decisões do autor: alturas derivadas da
referência, pulso da referência, motivo e desenvolvimento primeiro,
"levando em consideração as demais alternativas também" (por isso as fontes
de altura modal, espectral e microtonal e o tempo livre ficaram como opções
reais, e o tempo `auto` escolhe por seção). Feito: escuta da estrutura
(croma e picos em janela longa, pulso com ciclo de compasso e refinamento
por regressão, grade rítmica, seções, campo), compositor (motivo da
referência, sentença, variação em desenvolvimento, ritmo vivo, dinâmica e
registro locais, respiração, forma com recapitulação, hoqueto em
contrapor), voz com timbre que evolui, sala, MIDI. Defeitos achados e
corrigidos no caminho: croma com acordes graves fundidos (janela curta),
fase do pulso com 45 ms de erro (deriva do período), tresillo lido como 80
BPM, metrônomo lido como 97,6 BPM (envelope de um quadro), confiança do
pulso sobrescrita. Validação: 11/11 testes, sem avisos; música real
composta nos quatro modos. Seis respostas para o autor ouvir em
`~/Music/TRIOIO_respostas/v2-composicao/` (WAV, MIDI e a forma em texto).
**Próximo passo:** escuta do autor; depois f0 (contorno real), compasso
ternário, e timbre/orquestração.

## Registro — 2026-10-05: melodia real, compasso, orquestração, Terceiro Olho

Escuta do autor sobre a v2: interessante mas "contido, como arquivos de
midi"; pediu "algo mais fora da caixa… música sem previsibilidades" e
trabalhar nas limitações. Feito (ver `docs/COMPOSICAO.md` §Etapa 3): f0 por
YIN e contorno do motivo tirado da melodia real; compasso de 2 a 7 tempos;
seis famílias de timbre próprias com orquestração por seção; Terceiro Olho
(objetos sonoros + regras ocultas, `--imprevisibilidade 0..1`, reprodutível
pela seed). Defeitos achados: altura lida no ataque ouvia a nota anterior;
valsa convertida em compasso de 4; escala espectral espalhada em quartos de
tom. Validação: 12/12 testes; seis versões de uma gravação real em
`~/Music/TRIOIO_respostas/v3-fora-da-caixa/`. **Próximo passo:** escuta do
autor; depois, escolha contextual das intervenções (leitura musical em vez
de sorteio guiado por novidade) e versão ao vivo.


**2026-10-10, escuta das referências: leitura, gabarito e HPSS**
(`docs/LEITURA_E_GABARITO.md`).

- **Feito:**
  - os comandos `ler`, `gabarito` e `avaliar`;
  - a tonalidade lida pelos acordes;
  - o HPSS (analisador v5): pulso pelo fluxo percussivo, e croma do que
    se sustenta quando a fração percussiva é ≥ 0,40.
- **Suno:**
  - tonalidade Sol menor; antes, Ré maior;
  - acordes de 36 para 27 trechos;
  - confiança do pulso de 0,21 para 0,24.
- **Validação:**
  - a suíte 29/29;
  - `ReadingTests`: gabarito, métricas e a faixa sintética com e sem HPSS;
  - `ComposerTests §8` pegou o croma harmônico apagando a melodia
    sozinha, e foi isso que levou à decisão por peça.
- **Pendente com o autor:** preencher os gabaritos em
  `AUDIOS_RASGO/Trioio/dados/gabaritos/` (Suno e vocalise; até 2026-10-28
  havia uma cópia idêntica em `TRIOIO_respostas/gabaritos/`).
  Depois:
  1. `trioio_biblioteca avaliar <dados> <id> --gabarito ARQUIVO`;
  2. calibrar os limiares (fração percussiva 0,40; forma 0,8).
- **Próximo na escuta:** croma do grave e croma NNLS; melodia na mistura;
  tempo forte; reconhecer ciclos e escalas.

**2026-10-10, analisador v6, feito sem os gabaritos:**

- **Croma do grave**, que vota nos acordes.
  - Na faixa de teste Dó6 × Lám7: 100 % com o grave e 50 % sem ele, com e
    sem bateria.
  - Limite: numa voz sozinha, o vocalise sobe de 78 para 93 trechos de
    acorde.
- **Altura predominante** por saliência harmônica (o núcleo do MELODIA):
  - melodia na mistura: 91 % (Sol4–Sol5) e 86 % (Lá3–Sol4), contra 0–1 %
    do YIN;
  - Suno: MIDI 58–78, mediana em Lá4.
- **Validação:** a suíte 29/29.
- **Próximo:**
  1. o compositor usar a altura predominante quando o YIN falhar;
  2. os contornos do MELODIA;
  3. o tempo forte (porte do `downbeat` do Maravi);
  4. reconhecer ciclos e escalas.

**2026-10-10, avaliação sem gabarito.** O autor não vai preencher
gabaritos ("tente outro método"), e o autor trouxe uma referência nova,
`Recording_187 (Cover)(10).wav`. Ver `docs/LEITURA_E_GABARITO.md`.

- **Autogabarito** (`trioio_biblioteca autoavaliar`): o Trioio compõe,
  sintetiza, escuta o próprio áudio e compara com a partitura.

  | Referência | Peças | Andamento | Tonalidade | Acordes | Melodia (classe) | Seções F |
  |---|---|---|---|---|---|---|
  | Suno | 8 | 5/8 | 1,00 | 58 % | 45 % (66 %) | 0,52 |
  | Recording_187 | 4 | 2/4 | 0,65 | 39 % | 37 % (59 %) | 0,39 |

  - Os erros de oitava da melodia são todos para baixo, provavelmente as
    vozes de acompanhamento. A regra de oitava foi testada e descartada.
  - O pulso pela parte percussiva foi mantido. A amostra é pequena e o
    resultado, misto.
- **Reconhecer a linguagem** no `ler`, com o campo estrutural (acordes +
  melodia) e só as 330 escalas de linguagens:
  - Recording_187: Hijaz, melakarta 14 ou modo andaluz, **sobre Lá**,
    confirmado pela cadência Rém–Dó–Sib–Lá;
  - Suno: Hijaz sobre Ré, as notas de Sol menor harmônica.
- **Validação:** a suíte 29/29, com `ScaleArchiveTests` reconhecendo um
  campo de Lá frígio dominante.
- **Próximo:**
  1. o teste por transformação (reamostragem) nas gravações reais;
  2. os contornos do MELODIA;
  3. o compositor usar a altura predominante;
  4. reconhecer ciclos rítmicos;
  5. o tempo forte.

**2026-10-10, teste por transformação** (`trioio_biblioteca estabilidade`):
reamostrar por 2^(k/12) e conferir se a leitura se transpõe junto.

| Gravação | Andamento | Tonalidade | Linguagem | Acordes | Melodia |
|---|---|---|---|---|---|
| Recording_187 | 3/4 | 4/4 | 4/4 | 84 % | 91 % |
| Suno | 3/4 | 4/4 | 3/4 | 71 % | 76 % |
| vocalise | 2/4 | 1/4 | 4/4 | 53 % | 74 % |

- **Achado:** o nível do pulso salta (×2, ×1,5, ×0,65) quando o andamento
  passa pela preferência em 100 BPM.
- Duas correções rápidas, nível pela periodicidade mais forte e pente de
  múltiplos, quebraram a faixa sintética e o `ComposerTests` e foram
  **revertidas**.
- **Validação:** a suíte 29/29 e o teste de reamostragem (440 Hz vira
  493,9 Hz).
- **Próximo:**
  1. o nível do pulso pela programação dinâmica de Ellis 2007;
  2. os contornos do MELODIA;
  3. o compositor usar a altura predominante;
  4. reconhecer ciclos rítmicos.

**2026-10-10, a melodia da mistura chega ao compositor.**

- `pitchAt` recorre à altura predominante quando o YIN falha.
  - Suno: ataques com altura de 46 % para 100 %.
  - A resposta passa de "contorno: inventado" a "contorno: da melodia da
    referência", e o oráculo aprende mais notas (10/16/15 contra
    10/10/7).
- **Nível do pulso:** a estimativa de Ellis (TPS2/TPS3) foi tentada em três
  ajustes. Quebrou o metrônomo de 120 (lido como 60) e foi revertida.
  Antes de mexer de novo, montar uma bateria de andamentos conhecidos.
- **Validação:** a suíte 29/29.
- **Próximo:**
  1. a bateria de andamentos (o autogabarito com o andamento variando)
     para o nível do pulso;
  2. os contornos do MELODIA;
  3. reconhecer ciclos rítmicos.

**2026-10-10, bancada de andamentos** (`build/trioio_bancada_andamento`,
fora da suíte). São 6 estilos de 60 a 180 BPM.

- **Estimador atual:** 68/120 certos e 21 saltos de nível.
  - Sólido no metrônomo, no shuffle e na valsa.
  - Erra o rock (⅔ em 120–127 BPM) e a bossa nova (3/20).
- **Variantes medidas e revertidas:**
  - Ellis TPS: 72–73 certos, 21 saltos, e quebra o `ComposerTests`;
  - saliência da grade: 53 certos, 60 saltos.
- **Próximo no pulso:** o acompanhamento por programação dinâmica, medido
  nesta bancada.
- **Próximo em geral:** reconhecer ciclos rítmicos.

**2026-10-10, reconhecer o ciclo rítmico** (`rankCycles`, no `ler`;
experimental).

- **Método:** o ciclo previsto na linha do tempo inteira, mais grave e
  agudo (o dum contra o tak), com o nível do pulso e o período afinado.
- **Ciclos sintéticos:** 4/8 em 1.º e 5/8 entre os 3 primeiros (maqsum,
  baladi, sa'idi e wahda certos).
- **Gravações reais:** nenhum ciclo claro, como esperado para canções.
- **Achado:** o detector de ataques perde golpes próximos. O envelope do
  fluxo foi tentado e piorou.
- **Validação:** a suíte 29/29. O teste guarda a linha de base 4/8 e 5/8.
- **Próximo:**
  1. ataques por banda (grave e agudo);
  2. o acompanhamento por programação dinâmica para o pulso (bancada);
  3. os contornos do MELODIA.

**2026-10-10, analisador v7: ataques e pulso por banda (grave < 200 Hz,
aguda > 2 kHz).**

- **Pulso:** escuta as duas bandas normalizadas.
  - Bancada: **92/120 certos e 6 saltos**, contra 68 e 21.
  - Suno em 84,9 BPM; Recording_187 estável, com acordes de 84 % para
    91 %.
  - O vocalise piora, mas não tem pulso.
  - Nas mesmas peças do autogabarito, empate.
- **Ciclos:** 6/8 em 1.º e 7/8 entre os 3 primeiros, contra 4 e 5.
- **Validação:** a suíte 29/29. Os testes de ciclo guardam 6 e 7.
- **Próximo:**
  1. separar melodia de acompanhamento (contornos);
  2. o tempo forte do compasso;
  3. a ruchenitsa e o shuffle nos ciclos.

**2026-10-11, analisador v8: a linha da melodia.** São 3 candidatos por
quadro, e um Viterbi com custo de salto e preferência pelo agudo
(`trackMelody`).

- **Nas mesmas peças do autogabarito:**
  - Suno: altura de 47 % para 53 %, erros de oitava de 15 % para 7 %;
  - Recording_187: altura de 43 % para 44 %, oitava de 10 % para 7 %.
  - A classe de altura não muda (~64 %).
- **Faixa sintética:** melodia aguda 100 %; a grave, entre as notas do
  pad, cai de 86 % para 74 %.
- **Validação:** a suíte 29/29.
- **Próximo:**
  1. a decisão de voz e os contornos do MELODIA, para os erros de classe;
  2. o tempo forte do compasso;
  3. a ruchenitsa e o shuffle nos ciclos.

**2026-10-11, fase do pulso e tempo forte.**

- **A fase do pulso estava errada** no shuffle e na bossa (0/15). Vinha da
  média circular dos ataques, puxada pelos ataques fora do tempo. Agora
  vem da grade que soma mais envelope: shuffle 15/15, bossa 5/15. A suíte
  continua 29/29.
- **Tempo forte** (`estimateDownbeat`, no `ler`): o grave dominante mais a
  mudança de acorde.
  - Pop (acordes no 1): 12/13.
  - Bateria sem harmonia: ambígua (1 contra 3).
  - A valsa é lida em 4 tempos.
  - Gravações reais: confiança 0,01–0,05, sem decisão.
  - O autogabarito não serve, porque o 1 das peças é arbitrário.
- A bancada ganhou o estilo pop e as medidas de fase e de tempo forte.
- **Próximo:**
  1. detectar o compasso de 3;
  2. o tempo forte no compositor, quando a confiança for alta;
  3. a decisão de voz da melodia.

**2026-10-11, o compasso de 3** (`estimateMeter`: correlação do grave e
semelhança do croma nos atrasos de 2, 3 e 4 tempos; margem de 0,05).

- **Bancada:** o compasso fica certo em 102/105. A valsa é lida em 3 em
  13/16, sem nenhum 4/4 virar 3.
- **Tempo forte da valsa:** de 0/16 para 13/16.
- **No pulso:** só substitui o 4 padrão.
- **Validação:** a suíte 29/29. As gravações seguem em 4.
- **Próximo:**
  1. a decisão de voz da melodia;
  2. o tempo forte no compositor, com confiança alta;
  3. a ruchenitsa e o shuffle nos ciclos.

**2026-10-11, melodia: penalizar o sustentado foi medido e descartado.**

- Nas mesmas peças: Suno 44 % → 44/43/41 %; Recording_187 29 % →
  29/28/25 %. A melodia do Trioio também segura notas.
- O código foi revertido; fica o registro em `docs/LEITURA_E_GABARITO.md`.
- **Próximo na melodia:** o timbre de cada voz, ou contornos com começo e
  fim de nota.

**2026-10-11, a linguagem reconhecida chega ao compositor.**

- **Defeito:** com `--alturas arquivo`, o compositor escolhia pelo croma
  bruto entre todo o arquivo. Recording_187 recebia um mesotônico de 12
  notas nas 7 seções; o Suno, modos de Messiaen.
- **Correção:**
  - `languageField` (acordes lidos + melodia, o campo do `ler`);
  - a escolha automática só entre linguagens de 5 a 9 graus;
  - o sorteio só entre as quase empatadas.
- **Agora:**
  - Recording_187: melakarta 14 ou jins hijaz + kurd sobre Lá;
  - Suno: Hijaz ou melakarta 14 sobre Ré, as notas de Sol menor
    harmônica.
- **Validação:** a suíte 29/29.
- **Próximo:**
  1. o comportamento da linguagem (ornamentos, cadências, frases de
     assinatura) nas respostas sobre Recording_187: conferir se o idioma
     do Hijaz entra;
  2. a separação da melodia por timbre.

**2026-10-11, o comportamento da linguagem entra nas respostas.** Entre as
escalas quase empatadas, o sorteio prefere as que têm comportamento.

- Recording_187: maqam Hijaz (heterofonia, assinaturas, cadências, ciclo
  maqsum) ou modo andaluz (assinaturas, cadências, compás de 12), conforme
  a seed.
- O teste garante 8/8 escolhas com comportamento.
- **Próximo:**
  1. **ouvir** as respostas: Hijaz e andaluz sobre Recording_187 estão em
     `/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/dados/sessoes/`;
  2. a separação da melodia por timbre;
  3. comportamento para mais escalas do arquivo (hoje são 18).

**2026-10-11, escuta do autor das respostas sobre Recording_187 (v31).**

- O autor achou "bom no geral" e percebeu "mais variação de harmonia" no
  andaluz. Confere com os dados:
  - **Hijaz (textura heterofonia):** sem baixo nem voz de harmonia; 605
    notas na segunda voz (a melodia repetida com atraso, apojaturas e
    oitavas);
  - **andaluz (textura acordes):** baixo com 136 notas, voz de harmonia
    com 279, e 100 acordes lidos andando pelos graus (I IV I II I II III
    I VII VI I VII…).
- **A diferença é a das tradições:** o maqam é linear (o takht toca a
  mesma melodia, cada um com sua variação; Racy 2003), e o flamenco é
  harmônico (a cadência andaluza; Manuel 1986).
- **Ideia para depois:** no Hijaz, a variação pode vir de outro lugar,
  como modulações a ajnas vizinhos (o percurso do maqam: sayr) ou uma voz
  de bordão. Mantém o caráter linear e dá mais movimento.

**2026-10-11, os três itens listados.**

1. **Melodia pelo timbre:** medido e descartado. Nenhum efeito nas mesmas
   peças: Suno 44 %, Recording_187 29 % com qualquer peso. A melodia do
   Trioio troca de timbre a cada seção. O código foi revertido.
2. **Comportamento para mais 14 linguagens:** são 32 escalas com
   comportamento, antes 18 (commit cad4eee):
   - Nahawand, Kurd, Ajam e Sikah;
   - as rāgas Bhūpālī, Mālkauns, Bhairavī e Kāfī;
   - os três modos nordestinos, com o baião;
   - in e yō;
   - a pentatônica chinesa.
3. **Sayr no Hijaz:** numa linguagem linear escolhida automaticamente, as
   seções do meio alternam com a vizinha no ghammaz. Em Recording_187,
   Hijaz sobre Lá ↔ Nahawand sobre Ré.
   - Para ouvir: `v31-linguagem-recording187/5-hijaz-com-sayr-s1.wav`.

- **Validação:** a suíte 29/29. O `ScaleIdiomTests` garantiu que uma
  escala pedida pelo nome fica fixa e que a vizinha mantém a textura.
- **Próximo:** a escuta do autor do sayr.

**2026-10-11, protótipo v0 da interface (`trioio_app`, `docs/design.md
§7.1`).**

- **O que tem:**
  - a tela Escuta e resposta com as três faixas e o olho revelável;
  - Biblioteca, Linguagem e a barra de julgamento (tocar, aceitar,
    transformar, rejeitar, compor de novo);
  - a janela pela política comum, com teste;
  - os três regimes.
- **Validação:**
  - os instantâneos em 5 superfícies, com 10/10 controles essenciais em
    cada uma;
  - a janela real aberta, com 1690×950 num monitor de 1920×1080;
  - a suíte 30/30, com o teste novo da política de janela.
- **Para abrir:**
  `TRIOIO/build/trioio_app_artefacts/Release/Trioio --dados <raiz>`.
- **Próximo:**
  1. o autor usa e diz o que muda;
  2. a identidade visual;
  3. o julgamento por trecho na faixa;
  4. a página Sua escuta;
  5. a escala de DPI.

**2026-10-11, interface em 4 idiomas, com SOBRE e crédito** (pedido do
autor; `docs/design.md §7.1`).

- **Idiomas:** EN, PT, FR e ES. Na primeira abertura vale o do sistema; o
  botão do cabeçalho troca e a escolha é guardada.
- **SOBRE:** cartão sobre véu.
- **Crédito:** "© LÚCIO DE ARAÚJO · TRIOIO 2026 · LICENÇA AGPLv3+ · v…".
- **Validação:** 4 idiomas × 5 superfícies, 11/11 controles em todas; a
  suíte 30/30.
- **Próximo:** o website (`TRIOIO/website/`), sem publicar até o
  instrumento ser publicado.

**2026-10-12, site em preparação** (`website/`, `website/README.md`).

- **Feito:** quatro páginas — PT canônica, EN, FR e ES — geradas por
  `gerar_paginas.py` com SEO e sitemap pelo `seo.py` comum.
  - Seções: escuta e resposta, o Terceiro Olho, linguagens e culturas
    (com o exemplo da cadência andaluza), a interface, proveniência,
    créditos e estado/contato.
  - Paleta dos tokens da interface; fontes DejaVu e `contact.js` copiados
    e fixados a partir do Antitotem.
  - Links do GitHub já no caminho novo, `PROJETOS_AUTORAIS/TRIOIO`
    (reorganização de 2026-10-11).
- **Não publicado:** noindex e selo "em preparação" até o instrumento sair.
- **Validação:** links locais, hreflang e sitemap conferidos; capturas no
  Chrome a 1440 px e 390 px.
- **Próximo:** o autor revê o texto do site e usa o protótipo
  (`docs/design.md §7.1`). Depois, a identidade visual, que vale para o
  app e o site.

**2026-10-12, julgamento por trecho na interface** (`docs/design.md §7.1`,
`docs/ESCUTA.md`).

- **Feito:** arrastar sobre a resposta marca um trecho; clicar num bloco
  escolhe a seção; aceitar/transformar/rejeitar julgam o que estiver
  marcado. O núcleo já aceitava trechos desde 2026-10-10.
- **Validação:** 11/11 controles nos instantâneos com seleção; a suíte
  30/30; os formatos gravados numa cópia descartável.
  - Binário: `build/trioio_app_artefacts/Release/Trioio`, 2026-10-12
    18:40.
- **Falta:** o autor arrastar de verdade.
- **Próximo:**
  1. a página "Sua escuta";
  2. a escala de DPI;
  3. a identidade visual (decisão do autor).

**2026-10-13, a primeira abertura e o "trazer gravação"** (`docs/design.md
§7.1`).

- **Pedido:** o autor não sabia como tocar. O app aberto sem `--dados`
  caía na demonstração muda e parecia travado.
- **Feito:**
  - tela vazia com roteiro;
  - "trazer gravação…" (registra um arquivo e cria `~/Música/Trioio`);
  - o cartão de autorização;
  - "autorizar…" para pendentes;
  - "▶ referência";
  - a raiz e quem declara guardados entre sessões.
  - O importador aceita um arquivo só.
  - O símbolo virou a logo de três olhos do autor (triângulos), no app e
    no site.
- **Validação:** o teste do importador, a suíte 30/30 e os instantâneos
  (raiz vazia, pendente, cartão a 960×600).
  - Binário: `build/trioio_app_artefacts/Release/Trioio`, 2026-10-13
    00:41.
  - A instância antiga (demonstração) foi fechada.
- **Falta:** o autor percorrer o fluxo com o mouse. As mensagens de
  validação ainda só existem em português.
- **Próximo:**
  1. o retorno do autor sobre o fluxo;
  2. a página "Sua escuta";
  3. a escala de DPI.

**2026-10-13, master** (pedido do autor).

- **Feito:** volume master na barra de baixo (−60…+6 dB, guardado,
  aplicado no ganho do transporte).
- **Validação:** 12/12 controles essenciais em 5 superfícies × PT/FR.
  - Binário refeito.
  - Não foi ouvido aqui: o autor confere o ganho de ouvido.

**2026-10-13, linhas por região de frequência, etapa 1** (`docs/LINHAS.md`).

- **Pedido:** múltiplas linhas de referência por região de frequência, e
  criações autônomas sobre elas.
- **Feito:**
  - o divisor `BandSplit`: Linkwitz-Riley de 4.ª ordem, fase zero; a soma
    reconstrói a referência;
  - `ler --linhas 200,2000` (de 1 a 4 cortes);
  - as regras do que vale em cada linha.
- **Medido:**
  - **Recording_187:** o médio reconhece o Hijaz sobre Lá; o grave dá as
    fundamentais Lá · Ré · Fá; as três linhas leem o mesmo pulso.
  - **Suno:** as linhas leem um pulso fraco, mas que concorda com a peça;
    com 4 linhas, o grave lê compasso de 3.
- **Validação:**
  - `trioio_band_split_tests`, novo;
  - a suíte (31 testes).
- **Próximo:** a etapa 2 — um compositor por linha, o regente (pulso comum
  ou linhas livres) e os stems.

**2026-10-13, linhas por região de frequência, etapa 2** (`docs/LINHAS.md`).

- **Feito:**
  - `responder --linhas` (com `--linhas-modos`, `--linhas-livres sim`,
    `--linhas-filtro nao`);
  - um compositor por linha, com modo e semente próprios;
  - o empréstimo de alturas da peça onde as da linha não valem;
  - o pulso comum (`Options::pulse`) ou as linhas livres;
  - o registro e o filtro por banda;
  - o equilíbrio pela energia da referência e o teto de −1 dBFS;
  - os stems em `saidas/linhas/`, `lines` e `line` no `decisoes.json`, e o
    manifesto declarando as linhas.
- **Respostas para ouvir** (raiz de teste
  `/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/dados`), refeitas depois
  da correção do UTF-8 (ver "Próximo").
- **Validação:**
  - `ResponseTests` §7 e `BandSplitTests` §5–6;
  - a suíte;
  - a análise dos WAVs: bandas, pico, soma dos stems.
- **Próximo:**
  1. a escuta do autor;
  2. a etapa 3 (faixas por linha na interface, julgamento por linha).

**2026-10-14, a bancada completa dos ciclos** (`docs/LEITURA_E_GABARITO.md`).

- **Pergunta do autor:** "algo limitado a 8 modelos". Era a bancada de
  reconhecimento de ciclo, que só cobria 8 dos 26 ciclos do arquivo.
- **Feito:**
  - a bancada cobre os 26 ciclos, com o mesmo ruído para cada um; antes, o
    resultado dependia da ordem;
  - o teste mostra a posição do ciclo certo quando ele fica fora dos 5.
- **Medido:**
  - os 26: 19 em 1.º e 21 entre os 3;
  - os 8 de antes: 6 em 1.º (com o ruído corrido entre os ciclos, tinham dado
    8 de 8, por sorte).
- **Descartado:** a varredura de fase dentro da unidade (sem ganho em 1.º,
  +40 % de custo).
- **Corrigido:** `docs/SINTESE.md` dizia "8 famílias" do Rasgo Synth; hoje
  são 13 vozes.
- **Próximo:**
  1. a etapa 3 das linhas (interface);
  2. o ciclo continua experimental: comparar a periodicidade do sinal com o
     comprimento do ciclo, quando a composição usar o reconhecimento.

**2026-10-14, auditoria** (`docs/AUDITORIA_2026-10-14.md`), pedida pelo
autor.

- **O que ela cobre:**
  - os conceitos cogitados e não feitos (ao vivo, prompt, jogo, gesto,
    expectativa, memória das próprias respostas);
  - o que está raso;
  - os limites de composição (grade sem groove, sem dramaturgia, peça
    paralela em vez de diálogo);
  - os limites de som (sem master nem mixagem, espaço estático).
- **Prioridade recomendada:**
  1. o barramento de master;
  2. o groove e o microtempo da referência;
  3. a dramaturgia.
  - O "tocar junto / pergunta e resposta" pede a decisão do autor.

**2026-10-14, barramento de master** (auditoria, item 1; `docs/SINTESE.md`).

- **Feito:** passa-altas → cola relativa → −16 LUFS (BS.1770) → limitador de
  pico verdadeiro −1 dBTP. Os stems seguem a mesma curva. `--master nao`
  desliga.
- **Corrigido na auditoria:** as respostas não eram "baixas". Os números de
  antes eram RMS; em LUFS, ficam entre −16 e −17.
- **Validação:**
  - `MasterTests`, novo;
  - `ResponseTests`: as correlações seguir/contrapor não mudaram;
  - a suíte.
- **A/B para ouvir** (raiz de teste):
  - `sessao-20261013231251` (sem master);
  - `sessao-20261013231414` (com master);
  - `sessao-20261013231519` (3 linhas, com master).
- **Próximo:** item 2, o groove e o microtempo da referência.

**2026-10-14, groove e microtempo da referência** (auditoria, item 2;
`docs/RITMO.md`).

- **Feito:**
  - `measureGroove`: swing, microtempo por semicolcheia, acento;
  - `applyGroove`: a grade deformada, só nas notas da grade e nos trechos
    com pulso;
  - por modo: seguir e interromper 100 %, transfigurar 60 %, contrapor
    reto.
- **Corrigido:** os dois vieses da primeira medição (a janela larga
  inventava swing; o pulso fraco não tem grade).
- **Medido:**
  - Recording_187: sem swing, microtempo até 20 ms;
  - Suno e vocalise: sem groove (pulso fraco).
- **Validação:**
  - `GrooveTests`, novo;
  - as suítes de composição sem mudança;
  - a suíte completa.
- **Para ouvir:** `sessao-20261013232941` (Recording_187, seguir, com
  groove e master).
- **Próximo:** item 3, a dramaturgia.

**2026-10-14, dramaturgia** (auditoria, item 3; `docs/COMPOSICAO.md`).

- **Feito:**
  - a curva de tensão por modo (a energia da referência, o arco áureo, a
    invertida, os cortes);
  - ela molda a dinâmica (0,85–1,15×), rareia os enfeites nos vales e põe
    uma respiração de um tempo antes do clímax (peças ≥ 60 s).
- **Corrigido pelo teste:** o clímax pegava o começo do platô dos
  percentis.
- **Medido:** Recording_187 nos 4 modos (clímax 2:53, arco 2:18, sem clímax
  em contrapor).
- **Limite:** o efeito é modesto.
- **Para ouvir** (raiz de teste, seed 3, com master e groove):
  - `sessao-20261013234105-seguir-s3`;
  - `sessao-20261013234149-transfigurar-s3`;
  - `sessao-20261013234233-contrapor-s3`;
  - `sessao-20261013234316-interromper-s3`.
- **Próximo:**
  1. a escuta do autor dos itens 1–3 (master, groove, dramaturgia);
  2. a decisão sobre o item 4 (tocar junto / pergunta e resposta).

**2026-10-14, diálogo e "▶ juntos"** (auditoria, item 4, sem mudar a
proveniência; `docs/COMPOSICAO.md`).

- **Feito:**
  - `--dialogo sim` e a caixa no painel "Linguagem": as vozes que conduzem
    calam nas frases da referência e respondem nas pausas; na música
    contínua, alternância de 2 compassos;
  - "▶ juntos" no app: a resposta e a referência ao mesmo tempo, só para
    ouvir, sem gravar.
- **Corrigido pelo teste:** a pausa precisa ser funda (12 dB abaixo do
  nível forte).
- **Medido:**
  - vocalise: 20 pedidos e 20 respostas;
  - Recording_187: alternância.
- **Validação:**
  - `DialogueTests`, novo;
  - os instantâneos (12/12 controles, com "▶ juntos" e a caixa do diálogo);
  - a suíte.
- **Para ouvir:**
  - `sessao-20261013235327-seguir-s4` (vocalise, diálogo);
  - `sessao-20261013235347-seguir-s4` (Recording_187, alternância).
  - No app, escolha a resposta e use "▶ juntos".
  - As sessões `235149` e `235206` saíram sem diálogo: foram rodadas com o
    binário antigo e servem de comparação.
- **Pendente com o autor:** gravar ou publicar uma mistura com a referência.
- **Próximo:**
  1. a escuta;
  2. a resposta que imita o pedido;
  3. a dramaturgia guiando o registro e o olho.

**2026-10-14, decisão do autor: a resposta é sempre um arquivo novo.**

- **A decisão:** "vamos criar um arquivo novo — não usar o áudio da própria
  referência na resposta"; "a música é uma variação criativa da versão de
  referência".
- **Fecha duas pendências:**
  - a mistura gravada com a referência: não;
  - as amostras no mosaico: não.
- **Conferido no código:** nada da referência entra no som.
  - A leitura do áudio na resposta serve só à escuta das linhas.
  - O corpo da referência e o mosaico ressintetizam.
  - O "▶ juntos" não grava.
- **Registrado em:** `docs/COMPOSICAO.md` (Diálogo), `docs/MOSAICO.md` e a
  auditoria.

**2026-10-14, dossiê da reapropriação criativa** (`docs/REAPROPRIACAO.md`).

- **O autor:**
  - mantém a decisão de hoje (só síntese);
  - pede o dossiê para rever quando o instrumento estiver mais
    desenvolvido;
  - o Navalha 2 foi só um exemplo, para pensar caminhos dentro do caráter
    do Trioio.
- **O que o dossiê traz:**
  - os termos e as fontes (Debord e Wolman, Burroughs e Gysin, Schaeffer,
    Oswald, Cutler, Bourriaud, Oswald de Andrade);
  - a gradação que o Trioio já percorre (abstração → gesto → ressíntese;
    a citação fica fora);
  - os caminhos A–E;
  - a ética e a proveniência (o uso "citar" que faltaria);
  - o que mudaria;
  - as perguntas e o critério de retomada.
- **Atlas:** a entrada de estudo de 2026-10-14 (não adotado).

**2026-10-14, o Atlas do Jogo** (`RASGO_DOCUMENTATION/ATLAS_DO_JOGO.md`).

- **O que é:** um atlas próprio das questões conceituais do jogo, a pedido
  do autor, com universidades, bibliografia e centros de pesquisa,
  inclusive a Sorbonne Paris Nord (Henriot, EXPERICE, a revista *Sciences
  du jeu*).
- **A forma mais concreta para o Trioio:** a matriz de ganhos de Xenakis
  (*Duel*) entre o autor e o instrumento (§4.A).
- **A chave conceitual:** Henriot, o jogo como atitude e metáfora (§3.1).
- **Próximo:** a leitura (§5) e a decisão com o autor sobre a primeira forma
  do jogo.

**2026-10-14, a resposta retoma o gesto do pedido** (`imitateCalls`;
`docs/COMPOSICAO.md`, "Diálogo").

- **Feito:** com o diálogo ligado, a voz do motivo toca o fim da frase que
  a referência acabou de tocar, ajustado à escala e ao registro da resposta.
  - Por modo: seguir imita; transfigurar inverte; contrapor toca o
    retrógrado invertido; interromper cita um fragmento e corta.
- **Medido:** vocalise, 12 de 20 respostas com o gesto.
- **Validação:**
  - `DialogueTests` §4 (a cauda Fá-Sol-Lá-Si, a inversão, o fragmento, o
    pedido sem altura);
  - a suíte.
- **Para ouvir:**
  - `sessao-20261014004938-seguir-s4` (vocalise, imitação);
  - `sessao-20261014004957-transfigurar-s4` (inversão).
  - No app, use "▶ juntos".
- **Próximo:**
  1. a dramaturgia guiando o registro e o olho;
  2. a decisão sobre a primeira forma do jogo (Atlas do Jogo, §4).

**2026-10-14, a dramaturgia guia o registro** (`docs/COMPOSICAO.md`).

- **Feito:** o motivo sobe uma oitava no clímax e desce nos vales
  profundos, em janelas contínuas.
- **Validação:** `DramaturgyTests` e a suíte, 35/35.
- **Para ouvir:**
  - `sessao-20261014005620-seguir-s3`;
  - `sessao-20261014005704-transfigurar-s3`.
- **Próximo:**
  1. a escuta do autor (master, groove, dramaturgia, diálogo com imitação);
  2. o olho guiado pela tensão;
  3. a decisão sobre a primeira forma do jogo (Atlas do Jogo, §4).

**2026-10-14, o olho guiado pela tensão** (trabalho autônomo, com o autor
dormindo; `docs/COMPOSICAO.md`).

- **Feito:**
  - o Terceiro Olho intervém mais nos trechos tensos (até 1,5×) e menos nos
    vales (0,5×);
  - nunca na respiração antes do clímax (`Options::tensionAt`, `breath`).
- **Validação:**
  - `ThirdEyeTests` §9: 19 intervenções com tensão 0 contra 43 com tensão 1;
    nenhuma na respiração;
  - a suíte.
- **Erro de processo, corrigido no commit seguinte.**
  - O commit `5c34d2f` entrou com 1 teste falhando (`rhythm_field`): o
    comando encadeado não conferiu o resultado da suíte.
  - A causa não é um defeito: o olho, agora guiado pela tensão, escolhe
    outros momentos, e uma regra dele (tempo deformado 2:3) tira golpes da
    grade de propósito. O teste do campo rítmico passava antes por sorte das
    sementes.
  - Correção: o teste mede o campo com a dramaturgia desligada (as condições
    de antes).
  - Daqui em diante, cada commit só entra se a suíte responder "100% tests
    passed".

**2026-10-14, rubato nas seções de tempo livre** (trabalho autônomo;
`docs/RITMO.md`).

- **Feito:** o arco de frase de Todd nas frases das seções livres.
  - Por modo: seguir 30 %, transfigurar 45 %, interromper 15 %, contrapor
    reto.
- **Validação:** `GrooveTests` §5 e a suíte, 35/35.
- **Para ouvir:**
  - `sessao-20261014011721-seguir-s3`;
  - `sessao-20261014011803-transfigurar-s3`.
  - A Recording_187 tem as seções 3–6 em tempo livre.

**2026-10-14, ponto de retomada da noite autônoma.**

- **Feito nesta sequência:**
  - o olho guiado pela tensão;
  - o rubato nas seções livres;
  - a correção do erro de processo (`81019d4`).
- **Interrompido pelo limite de uso, sem código começado:**
  1. a página "Sua escuta" na interface (desenho em `docs/design.md` §2 e
     §4, prancha 5);
  2. o documento de desenho da matriz de jogo à maneira de Xenakis
     (Atlas do Jogo §4.A; proposta para o autor decidir).
- **Estado:**
  - a suíte com 35/35;
  - nada enviado ao GitHub;
  - o app e o CLI compilados;
  - nenhuma janela aberta.

**2026-10-14, a página "Sua escuta"** (`docs/design.md` §7.1).

- **Feito:**
  - as versões sobre a referência, com o veredito em cor e a nota;
  - "o que a escuta ensinou", em barras log centradas em ×1, com o
    esquecimento declarado;
  - a chave de levar a escuta em conta no "compor de novo";
  - a aba no compacto, o menu na emergência, as abas no completo;
  - `--pagina` nos instantâneos.
- **Validação:** 12/12 controles nas 5 superfícies; a suíte.
- **Falta:**
  - comparar A × B e alternar no mesmo ponto;
  - os nomes de timbres e do olho nos 4 idiomas;
  - a rolagem das barras.

**2026-10-14, a proposta do jogo** (`docs/JOGO.md`).

- **O que traz:**
  - o *Duel* de Xenakis como ponto de partida; as táticas dele já existem
    no Trioio (nuvem, glissandos, drone, percussão, sopros, silêncio, mais o
    motivo e a memória);
  - três formas: A, o duelo dentro do instrumento (as linhas); B, pelo
    julgamento; C, ao vivo;
  - a matriz (Caillois ou Xenakis) e a leitura de Henriot.
- **Recomendação:** a forma A, atrás de `--duelo`.
- **Pendente:** as 4 perguntas do §6 para o autor. Nada implementado.
- **Feito no mesmo dia:** os nomes de timbres, do olho e das salas nos 4
  idiomas, na página "Sua escuta" (`trName`).

**2026-10-14, achados do autor no app:**

- as abas passavam despercebidas; a aba ativa ganhou a cor da resposta;
- a troca de idioma deixava o seletor, a leitura e a ficha no idioma
  anterior; corrigido com `refreshReferenceTexts`.
- **Validação:** 12/12 controles nos instantâneos; a suíte.
- A janela aberta pelo autor é anterior a esta correção: é preciso fechar e
  abrir de novo.

**2026-10-14, comparar A × B na "Sua escuta"** (`docs/design.md`).

- **Feito:**
  - A é a versão escolhida agora e B a anterior;
  - "▶ comparar A × B" toca A, e "alternar A ↔ B" troca para a outra no
    mesmo instante;
  - corrigida a corrida da parada assíncrona na troca de fonte.
- **Validação:** 12/12 controles; a suíte.
- **Não testado com áudio aqui:** a troca no mesmo instante precisa da
  escuta do autor.
- **Feito no mesmo dia:** as barras da "Sua escuta" rolam com a roda do
  mouse, e "↓ N …" diz quantas faltam.

**2026-10-14, o jogo redesenhado com as respostas do autor**
(`docs/JOGO.md`).

- **As respostas do autor:**
  - o jogo compreende os parâmetros, mas não se limita a eles;
  - regras novas entram se forem musicalmente interessantes;
  - o jogo contém as questões de Vitoriamario;
  - Xenakis e Caillois são referências, não normas: o Trioio cria as
    próprias regras.
- **O que mudou:**
  - o jogo é o de Vitoriamario, com os três olhos como papéis;
  - as regras ganham uma forma comum (condição, ação, alcance, custo,
    oculta, fica) e um ciclo de vida (proposta → em jogo → julgada → fica,
    volta ou é esquecida; variada, combinada ou quebrada);
  - cinco caminhos de invenção;
  - os critérios do "musicalmente interessante" (audível, coerente, novidade
    que se aprende, custo, escuta do autor);
  - o estado do jogo com os recursos do conceito.
- **Primeiro passo proposto (§7):** a forma comum de regra, a invenção pela
  regra lida e pela regra variada, os critérios medidos, e a "Sua escuta"
  mostrando as regras.
- **Pendente:** as 3 perguntas do §8.

**2026-10-14, os princípios de Vitoriamario no jogo** (`docs/JOGO.md` §1.1).

- **De onde vêm:** do arquivo "Apodrece e vira adubo"
  (apodrece.arquiviagem.net), a pedido do autor.
- **Os princípios:**
  - **"confunde-te a ti mesmo"**, que nasce no Trioio, dito pelo autor, da
    frase "VITORIAMARIO CONFUNDE-TE POR MUITO MENOS";
  - **"tudo apodrece e vira adubo"**: as regras apodrecem e alimentam regras
    novas;
  - **"tudo é vitoriamario"**: a apropriação de regras, não de som;
  - **"emissor e receptor se tornam vitoriamario"**: a troca de lugar no
    diálogo;
  - **"brincar de avançar na evolução da percepção"**;
  - **"Dogma único: fodam-se os dogmas" / "crie o seu mandamento"**.
- **No ciclo de vida:** "apodrece → adubo → regra nova" substitui
  "esquecida". Os caminhos de invenção passam a sete.
- **Atlas do Jogo:** §3.2, o arquivo de Vitoriamario como fonte própria.
- **O autor, no mesmo dia:** "vitoriamario é um ser autônomo — libertário —
  que se reinventa — se não existia passou a existir". Isso entrou como
  abertura do §1.1:
  - **autônomo:** decide por si, não executa um gosto;
  - **libertário:** nenhuma autoridade fixa;
  - **que se reinventa:** apodrece e vira adubo;
  - **se não existia, passou a existir:** os princípios podem nascer no
    Trioio.

**2026-10-14, o jogo, primeiro passo** (`docs/JOGO.md` §9;
`src/core/Game.*`).

- **Feito:**
  - a forma comum de regra;
  - dez ações (com "citar a si mesma");
  - cinco origens: lida, volta, variada, adubo, contra si mesmo;
  - os critérios na partitura;
  - o apócrifo, com a verdade guardada, e a leitura apócrifa;
  - a memória (as regras que ficam, o adubo);
  - `--jogo sim` e a caixa no app;
  - as regras na "Sua escuta".
- **Corrigido:** a troca de referência deixava a resposta da anterior na
  faixa.
- **Validação:** `GameTests`, novo; os instantâneos; a suíte.
- **Para ouvir:**
  - `sessao-20261014223159-seguir-s5` (Recording_187);
  - `sessao-20261014223243-contrapor-s5` (vocalise).
- **Próximo:**
  1. os critérios no som;
  2. o estado do jogo;
  3. a troca de lugar no diálogo;
  4. o mapa do que os outros instrumentos já fazem (pedido do autor).

**2026-10-14, o mapa dos outros instrumentos e o portão conceitual do jogo**
(a pedido do autor).

- **`docs/OUTROS_INSTRUMENTOS.md`:**
  - o Método RASGO (genealogia comum, personalidades próprias);
  - o catálogo do Studio (142 mecanismos), com o GamePlan, o BluffSignal, o
    Mercado de probabilidades, a Ruptura por coerência e o Atropelo, que
    interessam ao jogo;
  - os módulos do Modular;
  - Aquorbium, Antitotem, Navalha, Maravi, Trucmetry e Risco;
  - o que o Trioio já trouxe de cada um.
- **`docs/JOGO.md` §10:**
  - o portão conceitual respondido (o primeiro passo entrou antes dele, e
    isso fica corrigido);
  - o protocolo de reaproveitamento aplicado às duas ideias trazidas do
    Studio.
- **Próximo:** cada passo do jogo começa por este mapa e pelo portão.

**2026-10-14, o guia de como tocar e A × B à vista** (a pedido do autor).

- **`docs/COMO_TOCAR.md`:** o guia passo a passo da interface.
- **A e B à vista na aba Sua escuta.** O autor não via qual versão era qual:
  - **na lista:** selos A (verde) e B (azul) nas versões, e ▶ no selo da que
    está tocando;
  - **sobre os botões:** a linha "A: … · B: …";
  - **sem escolha ainda:** A é a resposta que está na faixa;
  - **quando a lista é relida:** A e B seguem as mesmas versões (pelo id),
    mesmo depois de uma resposta nova; numa referência nova, somem.
- **Validação:**
  - ctest 36/36;
  - instantâneos da aba Sua escuta em 5 tamanhos, com o selo A e a linha
    conferidos na imagem.
- **Pendente:** o selo B não foi visto em imagem, porque os instantâneos não
  escolhem uma segunda versão. Fica para a escuta do autor.

**2026-10-14, a máquina de timbres, primeira leva** (pedido do autor:
"timbres de instrumentos, talvez menos sintéticos", estilos, vozes, efeitos
de filme, "precisamos criar nossos sons").

- **Feito:**
  - **cinco corpos acústicos por modelo físico próprio**
    (`src/core/AcousticVoice.cpp`): arco, palheta, marimba/vibrafone, piano e
    violão; eles entram no sorteio do motivo;
  - **o comando de audição** `trioio_biblioteca timbres <wav> [família…]`;
  - **o desenho da máquina** e o mapa dos estilos em
    `docs/MAQUINA_DE_TIMBRES.md`.
- **Validação:**
  - a altura conferida nas cinco (±0,06 semitom);
  - ctest 36/36;
  - a audição medida (o violão foi escurecido depois da primeira medida).
- **Áudio:** `/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/escutas/2026-10-14-a-16-timbres/` (local, fora
  do Git).
- **Pendente:** a escuta do autor.
- **Próximo:** a segunda leva (baixo acústico, flauta transversal,
  percussões), depois as receitas e o conjunto por estilo.
- **Adiado:** o julgamento por trecho pesando em cada regra do jogo, já
  desenhado: guardar `firedAt` em cada regra e decidir "fica ou adubo" pelo
  trecho julgado que cobre o disparo.

**2026-10-15, a máquina de timbres, segunda leva.**

- **Feito:**
  - baixo acústico, flauta transversal, surdo, conga, pandeiro e ganzá
    (`AcousticVoice.cpp`);
  - **o teste de afinação em toda a extensão** dos sete instrumentos com
    altura (§2c). Ele achou a flauta saltando de registro, corrigida por
    varredura do jato, e a marimba sem fundamental no agudo, corrigida no
    filtro da baqueta;
  - **nas respostas:** o baixo acústico no baixo, a flauta no motivo, e as
    percussões na camada rítmica (sorteio próprio);
  - **três respostas reais:** sementes 6, 7 e 8 em Recording_187.
- **Validação:** ctest 36/36.
- **Áudio:** a audição `segunda-leva-2026-10-15.wav`.
  - Apaguei por engano a versão de 14/10, com a flauta ainda desafinada,
    que eu mesmo tinha gerado minutos antes. Fica registrado.
- **Pendente:** a escuta do autor.
- **Próximo:**
  - as receitas (cavaquinho, bandolim, 7 cordas e manouche, a partir do
    violão);
  - o conjunto por estilo em **Linguagem ▸**.

**2026-10-15, as receitas da corda dedilhada.**

- **Feito:**
  - o violão virou receita (`pluckString` + `Pluck`), com mais cinco:
    violão de 7 cordas, requinto, cavaquinho, bandolim (ordens duplas) e
    guitarra manouche;
  - a extensão de cada instrumento: fora dela, a nota soa na oitava que
    cabe.
- **Validação:**
  - a afinação em toda a extensão;
  - ctest 36/36;
  - a audição `cordas-dedilhadas-2026-10-15.wav`.
- **Próximo:** o conjunto por estilo (choro, samba, jazz…) em **Linguagem
  ▸**, que põe cada receita no seu papel.

**2026-10-15, o conjunto por estilo.**

- **Feito:**
  - `Ensemble.h/.cpp`: choro, samba, jazz, manouche, bolero, charanga,
    câmara e orquestra; cada voz recebe o instrumento do seu papel;
  - **no app:** **Linguagem ▸ Conjunto**;
  - **na CLI:** `--conjunto`;
  - **o painel Linguagem** agora rola quando a tela é baixa: com a linha
    nova, a caixa do jogo saía da tela em 1366×768.
- **Validação:**
  - ctest 37/37, com o novo `trioio_ensemble_tests`;
  - as capturas de tela do painel (`--linguagem`, uma opção nova das
    capturas), conferidas a 1366×768 e 1920×1080;
  - cinco respostas reais (choro, jazz, samba, manouche, câmara).
- **Pendente:** a escuta do autor.
- **Próximo:**
  - as figuras de cada estilo (baixaria, *la pompe*, *tumbao*);
  - os instrumentos que faltam: metais, sax, bateria de jazz, voz;
  - os efeitos de filme.

**2026-10-15, as figuras de cada estilo.**

- **Feito:**
  - `StyleFigures.h/.cpp`: a *la pompe*, o *walking* e o Charleston com
    swing, a batida do cavaquinho e a baixaria do 7 cordas, o bolero, o
    *montuno* e o *tumbao*;
  - o manouche sem percussão (a *pompe* é o ritmo).
- **Validação:**
  - ctest 37/37, com as figuras testadas nota a nota;
  - seis respostas reais.
- **Pendente:** a escuta do autor (os padrões são aproximações de partida).
- **Próximo:** os instrumentos que faltam (metais, sax, bateria de jazz,
  voz) ou os efeitos de filme.

**2026-10-15, sopros, bateria e a percussão de cada estilo.**

- **Feito:**
  - os instrumentos: sax (palheta com reflexo sem inversão), trompete e
    trombone (modelo de sinal de Risset e Mathews), prato de condução,
    chimbal, caixa, caixa com escovinha, bumbo, bongô e guiro;
  - **a percussão de cada estilo:** a bateria de jazz com swing, a conga, o
    bongô e o guiro da salsa, o surdo, o pandeiro e o ganzá do samba;
  - **o conjunto salsa** (metais);
  - **o jazz** com sax e trompete;
  - **o comando de audição** ganhou `--notas`.
- **Validação:**
  - os 15 instrumentos com altura afinados na extensão (o teste mede o som
    seco; ver `MAQUINA_DE_TIMBRES.md` §3.3);
  - ctest 37/37;
  - três respostas reais.
- **Registrado:**
  - o lábio físico não oscilou: fica para um passo futuro;
  - a sala embola notas graves sustentadas: a ouvir.
- **Pendente:** a escuta do autor.
- **Próximo:**
  - a voz (o trato vocal);
  - os elétricos (guitarra e baixo, órgão);
  - os efeitos de filme.

**2026-10-15, a voz.**

- **Feito:**
  - voz cantada, sussurro e boca chiusa (a fonte glotal de Rosenberg e os
    formantes de Klatt, com as vogais de Peterson e Barney);
  - a voz no motivo do bolero e do samba;
  - o conjunto *a cappella*.
- **Validação:** ctest 37/37; a afinação de MIDI 40 a 83.
- **Próximo:** a bateria completa (tons, prato de ataque, a levada de
  rock), a guitarra e o baixo elétricos, o conjunto rock e a bateria de
  escola de samba.

**2026-10-15, a bateria completa, os elétricos e o rock** (o autor:
"bateria").

- **Feito:**
  - **o kit:** tom-tom e prato de ataque;
  - **os elétricos:** guitarra elétrica, guitarra distorcida e baixo
    elétrico (a corda de aço, o captador, o amplificador valvulado e a
    caixa);
  - **o conjunto rock** com a levada: *power chords*, baixo em colcheias e
    a bateria com o prato de ataque e a virada nos tons.
- **Validação:**
  - ctest 37/37;
  - os elétricos afinados na extensão;
  - uma resposta real.
- **Próximo:**
  - a bateria de escola de samba;
  - os efeitos de filme;
  - o órgão;
  - a escuta do autor das levas de hoje.

**2026-10-15, a bateria de escola de samba.**

- **Feito:**
  - tamborim, repique, agogô, cuíca (fricção) e chocalho;
  - os surdos de primeira, segunda e terceira;
  - o conjunto **escola de samba** com a batucada.
- **Validação:**
  - ctest 37/37;
  - uma resposta real (−16 LUFS, −1,1 dBTP).
- **Próximo:**
  - os efeitos de filme;
  - o órgão;
  - as bossas e paradinhas da bateria;
  - a escuta do autor.

**2026-10-15, os efeitos de cinema.**

- **Feito:**
  - vento, chuva, trovão, água, *whoosh*, *riser*, impacto e *braam*, por
    síntese procedural;
  - postos na dramaturgia: o clímax, as mudanças de seção, os trechos
    rarefeitos;
  - a caixa em **Linguagem ▸**, `--efeitos sim` e o comando
    `trioio_biblioteca efeitos`.
- **Validação:**
  - ctest 38/38, com o novo `trioio_film_effects_tests`;
  - duas respostas reais com o master saudável;
  - a captura de tela do painel.
- **Próximo:**
  - o órgão;
  - as bossas e paradinhas;
  - mais ambiências (fogo, multidão, cidade);
  - a escuta do autor.

**2026-10-15, o órgão, a palheta livre, o forró e o tango.**

- **Feito:**
  - os instrumentos: órgão de rodas fônicas com a Leslie, sanfona (musette),
    bandoneón, zabumba e triângulo;
  - os conjuntos forró (baião) e tango (*marcato* e 3-3-2);
  - o órgão no rock.
- **Validação:**
  - ctest 38/38;
  - a afinação na extensão (o órgão uma oitava abaixo, pelo 16');
  - três respostas reais.
- **Próximo:**
  - as bossas e paradinhas;
  - mais ambiências;
  - a escuta do autor.

**2026-10-15, as bossas, as paradinhas e mais ambiências.**

- **Feito:**
  - a paradinha da escola (chamada, bossa em uníssono, volta);
  - o breque do samba;
  - fogo, multidão e cidade.
- **Validação:**
  - ctest 38/38;
  - três respostas reais;
  - a audição dos onze efeitos.
- **Próximo:** a escuta do autor das levas de 14 e 15/10, para corrigir o
  que soar mecânico ou sintético.

**2026-10-15, o fraseado de cada estilo.**

- **Feito:** a melodia tocada com a articulação do estilo: swing e
  aproximações (jazz), apojaturas e corridas (choro), síncopes (samba,
  forró, salsa), portamento e vibrato (bolero, *a cappella*, cordas),
  *marcato* e arrastre (tango), bends (rock).
- **Validação:** ctest 38/38; quatro respostas reais.
- **Próximo:** a escuta do autor. Sem ela, o passo seguinte seria o
  vocabulário melódico próprio de cada estilo.

**2026-10-15, o conjunto lido na referência.**

- **Feito:** `readEnsemble`: o estilo pela escuta (ciclo, swing,
  andamento, caráter, voz) e a resposta do modo (vizinho ao transfigurar,
  contrário ao contrapor); **Linguagem ▸ Conjunto ▸ lido na referência**,
  `--conjunto lido`.
- **Validação:**
  - ctest 38/38;
  - as três referências lidas: vocalise → *a cappella*, suno → tango,
    rec187 → bolero;
  - o guia `COMO_TOCAR.md` atualizado.
- **Próximo:** a escuta do autor, para ajustar as regras de leitura e os
  timbres.

**2026-10-15, os naipes e a pasta de escuta.**

- **Feito:**
  - naipe de cordas, naipe de metais, coro de vozes; usados na orquestra,
    na salsa e na escola;
  - **a pasta de escuta**
    `/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/escutas/2026-10-15-estilos/`,
    com cópias (os originais ficam nas sessões) da resposta mais recente de
    cada estilo, com nomes claros.
- **A escuta do autor** (15/10, depois de ouvir os timbres e os estilos):
  "ainda muito midi — se possível trazer mais da síntese modular criativa
  para dentro dos timbres, sons inauditos, não protocolares".
  - **Rumo:** hibridizar os corpos acústicos com processos modulares, em vez
    de buscar mais realismo.

**2026-10-16, o rasgo modular** (resposta à escuta de 15/10: "muito midi").

- **Feito:**
  - os patches: dobra, anel inarmônico, deslocamento de frequência, portão
    caótico, gagueira granular e ressonância simpática;
  - as quimeras: prato friccionado, corda cantada e pele soprada;
  - a distribuição por voz e por seção, movida pela tensão;
  - **Linguagem ▸ Rasgo modular** (padrão 0,5) e `--rasgo`.
- **Validação:** ctest 39/39; respostas `rasgo-*.wav` na pasta de escuta.
- **A escuta seguinte do autor** (16/10): "está muito amador, precisamos de
  sonoridade mais sofisticada, amadurecida, profissional […] para que gere
  emoção". Sobre a síntese realista: "creio que fx né, confirma?".
  - **Diagnóstico:** a mistura é primitiva. Todas as vozes somam num par
    estéreo, passam por uma reverberação de 4 linhas com entrada mono e
    pela mesma quantidade de sala, sem equalização, sem profundidade, sem
    palco.
- **Próximo:** a produção. Barramentos por voz, equalização e dinâmica por
  papel, o palco estéreo por conjunto, uma reverberação de qualidade (FDN
  de 8 linhas com reflexões iniciais, decaimento por frequência e linhas
  moduladas) como envio, e uma sala por estilo.

**2026-10-16, a produção.**

- **Feito:**
  - os barramentos por papel (equalização, compressão, largura, envio);
  - a reverberação de envio (FDN de 8 linhas, difusão, reflexões iniciais,
    caudas descorrelacionadas, envio filtrado);
  - a sala por estilo (as salas HALL/SPACE viraram perfis dela);
  - a cola da soma.
- **Validação:**
  - ctest 40/40, com o novo `trioio_production_tests`: o RT60 a menos de
    10 % do pedido, a cauda descorrelacionada, o grave cortado onde deve;
  - a pasta `escutas/2026-10-16-producao`.
- **Pendente:** a escuta do autor.
- **Próximo:** a humanização fina por nota e o palco por instrumento.
- **A humanização fina, também em 16/10:**
  - tempo, força e afinação por nota, em passeio lento por voz;
  - ligada no app e na CLI;
  - ctest 40/40;
  - as versões `-humano` na pasta de escuta, ao lado das sem humanização.

**2026-10-16, o palco e o idioma** (a escuta do autor: "não soa como
jazz").

- **Feito:**
  - um barramento por voz e o palco por conjunto (lado, largura,
    profundidade);
  - o Terceiro Olho no idioma (sem objetos e sem as regras que quebram o
    estilo, metade da imprevisibilidade; inteiro com o rasgo ≥ 0,5);
  - os acordes de jazz (sem fundamental, com 7ª, 9ª e 13ª) e os de Django;
  - as notas do olho de volta ao instrumento do conjunto.
- **Validação:**
  - ctest 40/40;
  - o jazz do rec187 só com instrumentos de jazz (`jazz-v2-idioma.wav`).
- **Pendente:** a escuta do autor do jazz novo; o timbre do sax e do
  trompete.

**2026-10-16, os pratos refeitos** (a escuta do autor: "o som do prato não
está parecido com o habitual do jazz").

- **Feito:** o modelo denso (70–140 modos, o estalo, a lavagem que cresce),
  com cache; prato de condução, chimbal e ataque.
- **Validação:**
  - ctest 40/40;
  - o centróide do prato de condução foi de 2,8 para 4,65 kHz;
  - `pratos-2026-10-16b.wav`, `jazz-v3-pratos.wav`.
- **Pendente:** a escuta do autor.

**2026-10-16, o padrão de diretórios** (o autor: "há muitos arquivos wav
espalhados por todo canto […] pra não avacalhar o HD").

- **Feito:**
  - tudo em `/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/`: `dados/` (a
    pasta de dados, movida de `tmp-builds`), `dados-2026-10-13/`,
    `referencias/` e `escutas/AAAA-MM-DD-tema/`;
  - os caminhos nos manifestos e na configuração do app foram reescritos;
  - **apagados os arquivos de teste:**
    - 119 sessões sem julgamento (~7,5 GB);
    - as respostas antigas `v2`–`v31` (5,7 GB);
    - `tmp-builds`.
  - **mantidos:** os julgamentos (14 + 9), as autorizações, os gabaritos,
    as 3 sessões julgadas e as gravações-fonte do autor;
  - **a CLI:** `TRIOIO_ESCUTAS` manda as audições para
    `escutas/AAAA-MM-DD/`;
  - **o registro:** `ORGANIZACAO_2026-10-16.md`, na pasta.
- **Verificado:** `trioio_biblioteca listar` com as 3 referências
  autorizadas nos caminhos novos.
- **Para abrir o app:**
  `build/trioio_app_artefacts/Release/Trioio` (a configuração já aponta
  para `AUDIOS_RASGO/Trioio/dados`).

**2026-10-16, a expressão.**

- **Feito:** o arco das frases (Todd), a carga melódica (KTH), o
  *ritardando* final e a fermata.
- **A resposta pode passar do comprimento da referência:** o final não é
  mais cortado. No modo de linhas, só o arco.
- **Validação:**
  - ctest 40/40;
  - jazz e orquestra em `escutas/2026-10-16-expressao/`, terminando em
    silêncio.

**2026-10-16, o piano e o sax do jazz.**

- **Feito:** o tampo harmônico e o abafador no piano; o ar e o subtone no
  sax.
- **Validação:**
  - ctest 40/40;
  - `escutas/2026-10-16-expressao/jazz-piano-sax.wav`.

**2026-10-16, a cor dos acordes em todos os estilos.**

- **Feito:** choro, samba, escola, bolero, tango, salsa, charanga e forró
  com os acordes do idioma; as faixas de registro com uma oitava inteira.
- **Validação:**
  - ctest 40/40, com o teste das doze fundamentais;
  - `escutas/2026-10-16-acordes/`.

**2026-10-17, duas medidas.**

- **O *aliasing* medido:** baixo; a sobreamostragem foi descartada.
- **Os conjuntos de estilo no app:** com o "arquivo" (o padrão), passam a
  usar o campo tonal da referência, não a escala rara do arquivo.
- **Registrado:** a CLI já era tonal por padrão, então o jazz que o autor
  ouviu não tinha o modo de Messiaen; a escala não era a causa ali.
- **ctest 40/40.**

**2026-10-17, a harmonia escrita** (o autor: "avance com a harmonia
escrita").

- **Feito** (`HarmonyWriting.h/.cpp`; `docs/HARMONIA.md`, "Harmonia
  escrita"):
  - a tonalidade da peça e das seções (Krumhansl–Kessler, com a
    estabilidade);
  - as cadências por modo (o ii–V–I no jazz, a perfeita no fim);
  - as dominantes secundárias;
  - as modulações preparadas;
  - as apojaturas no motivo.
- **Validação:**
  - ctest 41/41, com o novo `trioio_harmony_writing_tests`;
  - choro, jazz e bolero em `escutas/2026-10-17-harmonia-escrita/`.
- **Próximo:**
  - o retardo preparado;
  - a cadência alinhada à frase da melodia;
  - a condução de vozes a quatro partes.

**2026-10-17, a melodia fecha na cadência; o retardo preparado.**

- **Feito:**
  - a última nota da frase vai para a nota da cadência e se alonga;
  - o retardo 4–3 e 9–8 na voz de harmonia.
- **Validação:**
  - ctest 41/41;
  - choro e orquestra (`-v2`) em `escutas/2026-10-17-harmonia-escrita/`.
- **Próximo na harmonia:** a condução de vozes a quatro partes.

**2026-10-17, a condução de vozes a quatro partes.**

- **Feito:** `voiceLead`: sem paralelas, sem cruzamento, a sensível que
  sobe.
- **Validação:**
  - ctest 41/41;
  - na orquestra do rec187, 112 acordes sem nenhuma paralela.
- **O autor, em seguida:** "sugiro um estudo aprofundado de harmonia,
  cadências, variações usuais e não usuais, e por que não pensarmos numa
  máquina de harmonia — harmonia funcional, harmonia experimental, etc."
  - **Próximo:** o estudo e o desenho da máquina de harmonia
    (`docs/MAQUINA_DE_HARMONIA.md`).

**2026-10-17, o estudo e o desenho da máquina de harmonia**
(`docs/MAQUINA_DE_HARMONIA.md`).

- **O estudo:**
  - dez famílias de linguagem harmônica, cada uma com as fontes (Rameau,
    Riemann, Piston, Schoenberg, Rohrmeier, Steedman, Levine, Sève,
    Chediak, Manuel, Cohn, Tymoczko, Hindemith, Messiaen, Cowell, Forte,
    Fineberg, Partch);
  - o repertório de cadências usuais e não usuais;
  - a harmonia própria do Trioio.
- **O que já existe** no Trioio, no Studio e no Modular (lidos, não
  copiados).
- **O desenho da máquina:**
  - as linguagens: vocabulário, sintaxe, pontuação, condução, ritmo
    harmônico;
  - as operações;
  - o arquivo de cadências;
  - a escolha por conjunto, por modo e por seção;
  - o jogo e a escuta.
- **O portão conceitual,** e as cinco fases.
- **Próximo:** a fase 1 (o arquivo de cadências; as linguagens funcional,
  jazz e brasileira; substituir e interpolar; a escolha por conjunto e
  modo).

**2026-10-17, a máquina de harmonia: a fase 1 e a impressionista** (o
autor: "faça as duas juntas, avance").

- **Feito:**
  - o acorde com cores, a qualidade aumentada e o baixo próprio;
  - quatro linguagens: funcional, jazz, brasileira e impressionista
    (Debussy), com as cadências usuais e não usuais;
  - a condução a três ou quatro vozes; o *planing* em bloco;
  - **Linguagem ▸ Harmonia** e `--linguagem-harmonica`;
  - a escolha automática pelo conjunto e pelo modo.
- **Validação:**
  - ctest 41/41, com os testes das linguagens;
  - quatro respostas em `escutas/2026-10-17-maquina-de-harmonia/`.
- **Próximo:** as fases 2 e 3 (modal e cromática; quartal, politonal,
  octatônica, *clusters*, espectral, microtonal; Bartók, Scriabin).

**2026-10-17, as harmonias brasileiras: o Nordeste, Minas, Guerra-Peixe e
Hermeto.**

- **O estudo,** com as fontes verificadas: o trimodal de José Siqueira, a
  cantoria, Guerra-Peixe e os maracatus, o Armorial, o *Calendário do Som*
  e o "som da aura", o Clube da Esquina.
- **Feito:** as linguagens nordestina (baião, lídio, trimodal,
  repente/aboio, Guerra-Peixe/Armorial), mineira e Hermeto; o forró pede a
  nordestina.
- **Validação:**
  - ctest com os testes §10;
  - quatro respostas em `escutas/2026-10-17-maquina-de-harmonia/`.
- **Próximo:** o "som da aura" de Hermeto; Villa-Lobos, Gismonti, Moacir
  Santos, Guinga; o frevo e o maracatu.

**2026-10-17, o som da aura (Hermeto Pascoal).**

- **Feito:** a fala da referência, sílaba a sílaba, vira a melodia, dobrada
  em uníssono, com um acorde por sílaba em que a nota é uma cor;
  `--som-da-aura sim` e a caixa em **Linguagem ▸**.
- **Validação:** ctest 41/41; o vocalise em
  `escutas/2026-10-17-maquina-de-harmonia/`.
- **O autor, em seguida:** "avance numa sequência grande de tarefas, sem
  interrupção e de modo autônomo".

**2026-10-17, madrugada: as linguagens modernas e as dos compositores; o
arquivo de cadências** (a sequência autônoma pedida pelo autor).

- **Feito:**
  - 14 linguagens novas na máquina de harmonia (`writeModern`):
    - modal, andaluza, cromática romântica (Cohn), quartal, politonal,
      octatônica, clusters, estática (Feldman), espectral (os parciais da
      referência);
    - Bartók, Scriabin, Villa-Lobos, Moacir Santos, Guinga.
  - A condução por quartas (quartal, Villa-Lobos), por clusters e em bloco
    (Scriabin, politonal, octatônica, espectral).
  - **Linguagem ▸ Harmonia** e `--linguagem-harmonica` com as 22.
  - O arquivo de cadências: 45 cadências com fonte, gerador,
    `parseCadenceArchive` e `trioio_biblioteca cadencias`.
  - Doc: `docs/MAQUINA_DE_HARMONIA.md` §8.
- **Validação:**
  - `HarmonyWritingTests` §12–13;
  - ctest (ver o commit);
  - um exemplo por linguagem em `escutas/2026-10-17-linguagens-modernas/`.
- **Pendente:**
  - a escuta do autor;
  - a microtonal (Partch);
  - os itens do jogo.

**2026-10-17, madrugada (cont.): o frevo, o maracatu e Gismonti; o sorteio
no arquivo de cadências.**

- **Feito:**
  - as linguagens `frevo` (as dominantes encadeadas, o diminuto de
    passagem), `maracatu` (as toadas eólia e mixolídia sobre o bordão) e
    `gismonti` (os acordes de 9ª trocando de modo sobre o pedal); agora são
    25 linguagens;
  - `drawArchiveCadences`: 40 % das chegadas sorteadas no arquivo da mesma
    linguagem;
  - 50 cadências no arquivo;
  - o bordão continua sob a chegada.
  - Doc: `MAQUINA_DE_HARMONIA.md` §8 (o sorteio) e §9.
- **Validação:**
  - `HarmonyWritingTests` §12–14;
  - ctest (ver o commit);
  - os três em `escutas/2026-10-17-linguagens-modernas/`.
- **Pendente:**
  - a escuta do autor, sobretudo se reconhece Gismonti (é uma leitura de
    escuta das gravações, sem análise publicada citada);
  - a microtonal;
  - o jogo.
- **Limpeza a decidir pelo autor:** 39 sessões de teste de 17/10, nenhuma
  julgada, cerca de 2,4 GB.
  - Ficam em `AUDIOS_RASGO/Trioio/dados/sessoes/sessao-20261017*`.
  - As cópias de escuta delas já estão em `escutas/2026-10-17-*`.
  - A remoção automática foi bloqueada pela verificação de segurança da
    sessão, por isso não foram apagadas. Para apagar:
    `rm -rf /media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/dados/sessoes/sessao-20261017*`.
- **Medição dos 14 exemplos:** picos de −1,1 a −4,2 dBFS, RMS em torno de
  −20 dB, nenhum clipping.

**2026-10-17, tarde: a harmonia microtonal (Partch)** (o autor: "pode
continuar").

- **Feito:**
  - a linguagem `partch`: otonalidades e utonalidades no limite 11 sobre as
    unidades do diamante, com o pivô O ↔ U;
  - as frações de semitom levadas até o baixo, o motivo e a voz de harmonia
    (`ChordSpan::microPcs`);
  - são agora 26 linguagens;
  - doc em `MAQUINA_DE_HARMONIA.md` §10.
- **Validação:**
  - `HarmonyWritingTests` §12;
  - ctest 41/41;
  - dois renders em `escutas/2026-10-17-linguagens-modernas/`;
  - as frações conferidas no `decisoes.json`.
- **Pendente:**
  - a escuta do autor de todas as linguagens;
  - os itens do jogo;
  - a limpeza das sessões de teste de 17/10 (ver acima), agora também as
    duas de Partch.

**2026-10-17, noite: o julgamento por trecho pesa em cada regra do jogo**
(o item adiado desde 14/10).

- **Feito:**
  - `GameRule::firedAt`, gravado no `decisoes.json`;
  - `judgeGameRule`: o trecho julgado que cobre o disparo decide; a maioria
    dos disparos decide a regra; sem trecho, vale a resposta inteira;
  - `loadGameMemory` passou a usá-lo;
  - "Sua escuta" mostra os instantes dos disparos;
  - doc em `docs/JOGO.md` §9.1.
- **Validação:**
  - `GameTests`;
  - ctest 41/41;
  - um `responder --jogo sim` com os disparos no `decisoes.json`.
- **Pendente:**
  - **do jogo:** o estado do jogo (§5); os caminhos 3 e 4; a troca de lugar
    no diálogo; os critérios medidos no som;
  - **da escuta do autor:** as linguagens de harmonia e o Partch;
  - **a limpeza:** as sessões de teste de 17/10 (ver acima), mais
    `sessao-20261017190542-seguir-s7` (o teste do jogo).

**2026-10-17, noite (cont.): o estado do jogo e a regra que nasce do jogo**
(`docs/JOGO.md` §9.2).

- **Feito:**
  - `GameState` com os oito recursos do §5, gastos e ganhos por cada
    disparo e devolvidos pelo tempo;
  - o caminho 3: o primeiro esgotamento cria uma regra ("sem surpresa
    disponível, só imitar"; "perto do colapso, sustentar");
  - o estado nas marcas da forma e no `decisoes.json` (`gameState`).
- **Validação:**
  - `GameTests`;
  - ctest 41/41;
  - Recording_187 em seguir e interromper (semente 7). Em interromper, a
    surpresa se esgota aos 8 s e nasce a regra de imitar.
- **Pendente:**
  - **do jogo:** o caminho 4 (a regra da nota do autor); a troca de lugar
    no diálogo; os critérios no som; o estado entre respostas;
  - **a limpeza:** mais duas sessões de teste de 17/10
    (`sessao-20261017191856-*`, `sessao-20261017192024-*`).

**2026-10-17, noite (cont.): a regra que nasce do autor** (o caminho 4,
`docs/JOGO.md` §9.3).

- **Feito:**
  - `ruleFromNote` lê ação, condição e material na nota livre do
    julgamento;
  - a nota mais recente que diz uma ação entra no jogo, depois da regra
    lida.
- **Validação:** `GameTests`; ctest 41/41.
- **Para o autor:** escrever a nota do julgamento como regra ("quando
  cresce, o baixo cala"; "sempre que ficar parado, corta tudo") faz o jogo
  jogá-la na próxima resposta.
- **Pendente do jogo:** a troca de lugar no diálogo; os critérios no som; o
  estado entre respostas.

**2026-10-17, noite (cont.): a troca de lugar no diálogo** (princípio 4,
`docs/JOGO.md` §9.4).

- **Feito:**
  - com o diálogo e o jogo ligados, um trecho do meio da peça troca de
    papel (a resposta fala quando a referência fala) ou fica sem vez (as
    duas falam juntas, o atropelo);
  - a marca na forma.
- **Validação:**
  - `DialogueTests`;
  - ctest 41/41;
  - dois renders em `escutas/2026-10-17-jogo-troca-de-lugar/`.
- **Pendente do jogo:**
  - os critérios medidos no som;
  - o estado entre respostas.
- **Pendente da limpeza:** as sessões de teste de 17/10, agora também
  `sessao-20261017203235-*` e `sessao-20261017203431-*`.

**2026-10-17, noite (cont.): o estado do jogo entre respostas** (a sessão
longa, `docs/JOGO.md` §9.2).

- **Feito:** a resposta nova parte, pela metade, do estado em que o jogo da
  anterior terminou; o oculto recomeça em zero.
- **Validação:**
  - `GameTests`;
  - ctest 41/41;
  - conferido de ponta a ponta: surpresa 0,29 no fim da anterior, 0,47 no
    início da nova.
- **Pendente do jogo:**
  - os critérios medidos no som (compor com e sem a regra, sintetizar,
    comparar; pesado: um render a mais por candidata);
  - o estado decidir a escolha das regras.
- **Pendente da limpeza:** mais `sessao-20261017204138-*`.

**2026-10-17, noite (cont.): o estado decide a escolha das regras.**

- **Feito:**
  - perto do colapso (≥ 0,5 no começo), as regras que quebram ficam de
    fora;
  - sem surpresa (≤ 0,15), as regras são ditas, não ocultas;
  - a regra do autor passa sempre.
- **Validação:** `GameTests`; ctest 41/41.
- **Pendente do jogo:** os critérios medidos no som. É pesado (um render a
  mais por candidata): pedir decisão ao autor antes.

**2026-10-17, noite (cont.): os critérios do jogo medidos no som** (o
autor: "sim medir os critérios no som"; `docs/JOGO.md` §9.5).

- **Feito:**
  - `soundCheck`: as janelas dos disparos sintetizadas com e sem a regra
    num só render; nível, brilho, ataques e a distância espectral (bandas
    de 1/6 de oitava, FFT própria);
  - os critérios audível, coerente e nova julgados no som;
  - `--jogo-no-som nao` para comparar.
- **Tempo** (pedido do autor): 89 s só na partitura, 102 s com o som (+12 s,
  +14 %, cerca de 2,5 s por candidata).
- **Validação:**
  - `GameTests`, com a calibração;
  - ctest 41/41;
  - as duas versões em `escutas/2026-10-17-jogo-criterios-no-som/`.
- **Pendente da limpeza:** mais `sessao-20261017222039-*` e
  `sessao-20261017222208-*`.

**2026-10-17, noite (cont.): a limpeza dos testes e a retenção das sessões.**

- **Feito:**
  - a pedido do autor ("pode apagar os testes"), as 56 sessões não julgadas
    de 16 e 17/10 foram apagadas (3 GB). Ficaram as três julgadas, e os
    dados caíram para 385 MB;
  - os 50 manifestos de derivação delas ficam no índice, e a interface
    esconde as versões sem sessão;
  - `trioio_biblioteca limpar` e `esvaziar-lixeira` (`Retention.h`):
    reversível, julgadas e recentes ficam.
- **Validação:**
  - `RetentionTests`, novo;
  - ctest 42/42;
  - `limpar` em modo de mostrar sobre os dados reais (só as três julgadas).
- **Daqui em diante:** depois de uma rodada de testes, rodar `limpar …
  --executar sim`. O pedido de autorização se resolve com a lixeira, que é
  reversível.

**2026-10-17, noite (cont.): os stems por voz.**

- **Feito:**
  - `renderScore` guarda cada barramento processado e a sala com o mesmo
    ganho final;
  - `responder --stems sim` os grava em `saidas/stems/`, passando pela
    curva do master.
- **Validação:**
  - `ProductionTests`: a mistura idêntica com e sem stems; soma dos stems
    com correlação de 0,9997 e diferença de 0,7 dB;
  - ctest 42/42;
  - nove stems reais, em `escutas/2026-10-17-stems-jazz/`.
- **Custo:** +25 s (+11 %) e +0,75 GB de memória numa peça de 224 s.
- **Pendente:**
  - um botão "stems" no app;
  - o stem por linha e por voz juntos, no modo linhas.

**2026-10-18: os stems no app e no modo linhas.**

- **Feito:**
  - a caixa "stems" em **Linguagem ▸** (conferida no instantâneo de
    1920×1080);
  - no modo linhas, os stems por voz somam as linhas, filtradas e com o
    ganho de cada uma, por um temporário em disco.
- **Validação:**
  - ctest 42/42;
  - o vocalise com três linhas: a soma dos stems refaz a mistura com
    correlação de 0,9999 e −0,9 dB;
  - 118 s e 0,64 GB.

**2026-10-18, tarde: dos subestilos do rock à árvore de influências.**

- **A escuta do autor:**
  - **intenção × consequência:** "um pouco mais rock, mas mais pra
    diferente de A que com uma cara de rock", e o rock "tem uma gama
    bastante larga de subestilos e intenções";
  - **o gesto** (`escutas/2026-10-18-gesto/`): "há alterações entre os 3
    audios, seguem um pouco a mesma linha" (mais diferente do que nítido).
- **Feito:**
  - os subestilos do rock (`--rock`), como pontos de entrada;
  - **a árvore de influências** (`src/core/Influences.h`,
    `data/influencias/`, `docs/INFLUENCIAS.md`):
    - os traços como tendências, os conceitos, os contextos;
    - a polinização, a mutação e a memória;
    - a repetição que o conceito pede;
    - os eixos por cruzamento, não pela média;
    - o caráter (dança, peso) em qualquer conjunto.
  - O princípio ficou na memória do agente (vale para todo o RASGO).
- **Validação:**
  - `InfluencesTests`;
  - ctest 47/47;
  - três respostas em `escutas/2026-10-18-influencias/`.
- **Próximo:**
  - a pesquisa sobre como a IA (Suno e outros) recombina estilos pelo
    prompt, para o Atlas;
  - as poéticas do autor;
  - as influências no app.

**2026-10-18, tarde (cont.): a pesquisa sobre a recombinação de estilos na
IA, a força e a exclusão.**

- **O Atlas de Referência do RASGO** ganhou a entrada "2026-10-18: a
  recombinação de estilos na IA generativa":
  - o Suno: o campo de estilo, os *mashups* da v4.5, o *prompt
    enhancement*, Weirdness, Style Influence e Exclude;
  - MusicLM e MuLan, MusicGen e o *guidance*, Jukebox, Composer Vector;
  - Every Noise at Once e o estudo de 101.953 canções do Suno e do Udio;
  - Boden, a mistura conceitual e a das cadências (COINVENT).
  O Atlas tinha alterações de outro fluxo, então a entrada foi acrescentada
  sem commit do arquivo.
- **Feito:** `--forca-influencias` e `--sem`, com testes.
- **Validação:** `InfluencesTests`; ctest 47/47.
- **Próximo:** a mistura conceitual de cadências; o mapa das respostas nos
  eixos; as poéticas do autor; as influências no app.

**2026-10-18, tarde (cont.): a mistura conceitual de cadências**
(`docs/MAQUINA_DE_HARMONIA.md` §11).

- **Feito:**
  - `blendCadences`: os traços salientes das notas e do tipo de acorde, o
    espaço genérico e as restrições;
  - metade das chegadas sorteadas no arquivo é inventada por mistura;
  - `trioio_biblioteca misturar-cadencias`;
  - a frígia modal no arquivo (51 cadências).
- **Validação:**
  - `HarmonyWritingTests` §15: a substituição de trítono emerge da perfeita
    com a frígia, e a backdoor vem mais fraca, como no artigo;
  - ctest 47/47;
  - numa resposta de jazz, a mistura da backdoor com a substituição de
    trítono deu V7 → I7M.
- **Próximo:** o mapa das respostas nos eixos (a busca pelo não ocupado).

**2026-10-18, tarde (cont.): o mapa das influências (a busca pelo não
ocupado)** (`docs/INFLUENCIAS.md`).

- **Feito:** os pontos das respostas em `dados/memoria/mapa_influencias.json`;
  cinco sorteios por resposta, e fica o mais longe do já feito; a repetição
  pedida pelo conceito faz a busca ceder.
- **Validação:** `InfluencesTests`; ctest 47/47; quatro respostas `auto`,
  todas em regiões diferentes.
- **Depois, no mesmo dia:** as influências no app, no painel Linguagem
  (campo, força e "sem"), conferidas no instantâneo.
- **Próximo:** as poéticas do autor no arquivo; a saliência das cadências
  pelo arquivo; a escuta de `escutas/2026-10-18-influencias/`.

**2026-10-18, fim da tarde: a saliência pelo arquivo; o timbre e a
densidade nas influências. PAUSA pedida pelo autor.**

- **Feito:**
  - a linha `saliência:` no arquivo de cadências, com os pesos do artigo
    na perfeita e na frígia;
  - o traço `timbre:papel:família` (40 no arquivo) e o eixo `densidade` nas
    influências.
- **Validação:**
  - `HarmonyWritingTests`, `InfluencesTests`;
  - ctest 47/47;
  - a segunda leva de escuta em `escutas/2026-10-18-influencias-v2/`.
- **Ao voltar:**
  1. a escuta de `2026-10-18-influencias/` e `-v2/`: as influências ficaram
     nítidas, ou ainda só diferentes?
  2. as poéticas do autor no arquivo de influências;
  3. o commit do Atlas, com o Codex.

**2026-10-18, noite: as gramáticas narrativas; os gestos do olho passam a
seguir a forma.**

- **O commit do Atlas** pelo Codex foi conferido: `269090f`, com a entrada
  da recombinação de estilos.
- **Feito:**
  - as gramáticas narrativas (`--narrativa`, e o traço `forma:` nas
    influências): freytag, kishotenketsu (com a virada), ondas, plato,
    arco; duas se misturam;
  - **achado e corrigido pela medição dos stems:** os gestos do Terceiro
    Olho não seguiam a curva da peça e mascaravam a forma, também no gesto
    e na intenção ("mais diferente que nítido"). Agora seguem
    (`shapeGestures`), e as curvas explícitas impõem o nível.
  - Doc: `docs/COMPOSICAO.md`.
- **Validação:**
  - `DramaturgyTests`;
  - ctest 47/47;
  - `escutas/2026-10-18-narrativas/`, onde a Freytag vai de −24 a −17 e a
    −28 dB.
- **Para o autor:** ouvir as narrativas, e refazer a escuta do gesto e da
  intenção com os gestos corrigidos, se quiser comparar.
- **Depois:** o gesto refeito com a correção, em `escutas/2026-10-18-gesto-v2/`
  (com LEIA-ME e a comparação por quarto). A forma do gesto agora se mede:
  - **C** (cheio, vazio no meio, cheio): −18,1, −19,9, −22,8, −20,5 dB; na
    primeira versão o fim não voltava;
  - **B** (cresce): começa em −23,9 e sobe.
- **A escuta do autor** (narrativas, gesto-v2 e influências-v2): "estão
  interessantes - vejo progresso". É o primeiro retorno positivo depois de
  "mais diferente que nítido". A correção dos gestos do olho e as curvas
  explícitas impondo o nível foram na direção certa.

**2026-10-18, noite (cont.): a forma e as influências cruzadas**
(`docs/INFLUENCIAS.md`).

- **Feito:** os eixos de caráter seguem a curva da peça; a virada do
  ki-shō-ten-ketsu traz os timbres e os eixos do galho distante.
- **Validação:** `InfluencesTests`; ctest 47/47.
- **Próximo** (pedido do autor): um campo de prompt opcional no Trioio,
  para palavras-chave; vazio, o Trioio segue os próprios critérios.

**2026-10-18, noite (cont.): as palavras da resposta (o prompt opcional)**
(`docs/PROMPTS_E_INTENCOES.md`).

- **Feito:**
  - o campo **Palavras** no app e `--palavras`: a intenção, a forma, a
    dança e os nós da árvore pelas palavras; vazio, os critérios do Trioio;
  - o que foi entendido e o que não foi, na forma.
- **Validação:**
  - `IntentTests`, `InfluencesTests`;
  - ctest 47/47;
  - o instantâneo do app;
  - um exemplo em `escutas/2026-10-18-palavras/`.
- **A pergunta do autor** ("como o app vai interpretar as palavras? há uma
  tecnologia para transformar em cálculo?"): respondida no doc, com três
  caminhos (léxico ampliado pelo autor, embeddings locais, espaço
  texto-áudio). **Decidido pelo autor:** a consulta na rede, leve, com uma
  lista de serviços (abaixo).

**2026-10-18, noite (cont.): a ponte semântica pela rede**
(`docs/PROMPTS_E_INTENCOES.md`, "A ponte semântica pela rede").

- **Feito:**
  - `src/library/SemanticBridge.h/.cpp`: as palavras não lidas vão aos
    serviços de `data/semantica/fontes.txt` (lista editável). A Wikidata
    traduz para o inglês, o Datamuse traz as associações e
    `intentFromAssociations` (core) calcula o caráter;
  - em paralelo, com um tempo limite para tudo, em lote e com memória em
    `dados/memoria/semantica.json`;
  - opcional e desligada por padrão: a caixa no app, que consulta em
    segundo plano enquanto se escreve, e `--semantica sim` na resposta;
  - o comando `semantica` só consulta e calcula.
- **Medido:** chuva, Regen e pluie → "rain" (energia 0,47, densidade 0,65,
  brilho 0,33); saudade → humor sombria. Cinco palavras em cerca de 3,1 s
  na primeira vez; depois, instantâneo.
- **Achado:** uma rajada de 30 buscas paralelas recebeu 429 da Wikimedia.
  Agora são duas por vez, em lote, com o User-Agent da política.
- **Limites:** "tempestade" ainda não tem tradução na Wikidata. A API do
  ConceptNet estava fora do ar (fica comentada na lista).
- **Próximo:**
  - o autor experimenta a caixa no app, com palavras e metáforas suas;
  - se a cobertura da Wikidata faltar, avaliar mais uma fonte de tradução
    livre para a lista.

**2026-10-18, noite (cont.): a escrita do autor como vier**
(`docs/PROMPTS_E_INTENCOES.md`).

- **O primeiro uso da rede no app** mostrou falhas: "post rock" em duas
  palavras, "sinfonica" sem acento, as associações de estilo descartadas e a
  Mistura que só aceitava "+" e o nome exato.
- **Feito** (`src/core/TextFold.h`): caixa, acento faltando ou indevido,
  pontuação e emoji, números, termos de duas palavras, erros de digitação
  (1 letra; 2 nos longos; letras vizinhas invertidas) e a flexão. Vale no
  léxico, na árvore e nas escalas.
- **Feito também:**
  - as associações da rede que são nós da árvore viram influências;
  - a Mistura aceita vírgula e regiões ("modos gregos", "escalas
    africanas").
- **Validação:** testes novos em `IntentTests`, `InfluencesTests` e
  `ScaleArchiveTests`; ctest 48/48; refeito com as palavras exatas do
  autor.
- **Próximo:** o autor fecha e reabre o app (o binário foi recompilado) e
  experimenta.

**2026-10-18, noite (cont.): o tutorial, os idiomas, a barra e as correções
do uso no app.**

- **Feito:**
  - o **TUTORIAL** no app (`src/app/Tutorial.h`), no padrão da família:
    11 capítulos a partir de `docs/COMO_TOCAR.md`, nas quatro línguas, com
    sumário, DICA, anterior/próximo e as setas do teclado; abre na língua
    do instrumento;
  - a **barra de trabalho em andamento** ("pra não dar impressão que
    travou"): a etapa e o tempo; a síntese avança voz por voz;
  - as etapas e a linha da rede nos 4 idiomas;
  - as **teclas mortas** compostas no app ("bai~ao" → baião; o JUCE no
    Linux não as compõe);
  - na rede: a ortografia ("janaese" → japanese), a memória do que as
    fontes não conhecem (7 dias), só as palavras completas enquanto se
    escreve, e o aviso que distingue "não conhece" de "não respondeu";
  - na Mistura: "escalas medievais" → os modos da prática modal; "harmonia
    funcional" vai para a harmonia.
- **Investigado:** a falha "sem resposta" nas consultas do app. Não era o
  SIGCHLD (conferido em `/proc`). As palavras eram desconhecidas das fontes
  e a mensagem antiga confundia; houve também o tempo limite com a máquina
  carregada.
- **Validação:**
  - ctest 48/48;
  - instantâneos do tutorial (`--instantaneos <pasta> --tutorial`) nas 4
    línguas × 5 tamanhos.
- **Pendente:** os textos da forma da resposta (as linhas de intenção,
  influências e seções) e os nomes das seções na demonstração saem em
  português em qualquer idioma: são gerados pelo núcleo.
- **Próximo:** o autor reabre o app e experimenta o tutorial e a barra.


**2026-10-18, noite (cont.): o segundo uso no app.** A resposta
`sessao-20261018212824` mostrou três falhas:

- "rock 60" foi ignorado em Influências;
- "dim dom" e "miles davis" não acharam escala na Mistura;
- "cromatica" foi para a harmonia. Esse erro era meu, da rodada anterior.

- **Feito:**
  - a escrita livre em Influências: as palavras pela árvore e a década pelo
    contexto histórico, `docs/INFLUENCIAS.md`;
  - as escalas diminuta e dórica na Mistura;
  - os nomes que também são escala ficam escala.
- **Validação:**
  - ctest 48/48;
  - as entradas do autor refeitas: rock/psicodélico; cromática justa,
    Messiaen 2, dórico e blues.
- **Próximo:** o autor reabre o app (binário das 21:41) e segue
  experimentando.

**2026-10-18, noite (cont.): o terceiro uso no app.** A resposta
`sessao-20261018220951` usou "tango improvisado, coral a capela" e a
Mistura "coco de roda, jupiter maça. ondas do mar".

- **O que estava errado:**
  - "coral" foi para a rede como *coral reef*, e "capela" como *chapel*;
  - "improvisado" não deu nada;
  - os termos da Mistura se perderam.
- **Feito:**
  - os nós `vocal/coral`, `mundo/tango` e `brasil/nordeste/coco`;
  - no léxico, "coral" e "a capela";
  - o campo **liberdade** ("improvisado", "aleatório"), que abre o Terceiro
    Olho;
  - o que a Mistura não reconhece como escala é lido como Palavras;
  - na rede: as palavras curtas sem correção ortográfica ("mar" virava
    "mark"); os nós da árvore fora da rede; o tempo limite cresce com o
    número de palavras ("mar" → *sea*).
- **Validação:**
  - ctest 48/48;
  - refeito: tango com a harmonia em coro, liberdade 0,80, forma ondas.
- **Próximo:** o autor reabre o app e segue experimentando.

**2026-10-18, noite (cont.): os nomes compostos e as influências do autor.**

- **Feito:**
  - a linha `também:` nos nós ("coco de roda" chama o coco e não manda
    "roda" para a rede);
  - `<dados>/influencias_do_autor.txt`, onde o autor escreve os artistas e
    as bandas com a sua leitura;
  - o tutorial explica isso no capítulo 10 (4 línguas).
- **Validação:** ctest 48/48.
- **Próximo** (com o autor): escrever os primeiros nós de artistas, por
  exemplo Júpiter Maçã, com as fontes dele.

**2026-10-19, madrugada: a preparação da publicação (0.1.0).** O autor
aprovou o repositório próprio e escolheu o ícone ("sim, os 3 olhos do
cabeçalho"). Depois pediu: "verifique os possíveis bugs antes de criar os
instaladores".

- **Feito:**
  - a verificação com os sanitizadores (ASan e UBSan) na suíte, em respostas
    completas e no app, sem nenhum erro;
  - a revisão do código achou quatro defeitos, corrigidos;
  - o ícone, a instalação, o CPack, os scripts Linux (com proveniência), o
    fluxo da CI (não rodado) e o `INSTALL.md`;
  - o `.deb` e o `.tar.gz` locais, testados: o app abre e lê os dados da
    instalação. Detalhes em `docs/MULTIPLATAFORMA.md`.
- **Validação:** ctest 48/48 (normal e com sanitizadores).
- **Esperando o autor:**
  - autorizar a criação do repositório público `lucioaraujo/trioio`, com
    histórico limpo, e o envio;
  - a tag `v0.1.0`.

**2026-10-19: publicado.** O repositório público é `lucioaraujo/trioio`.
Ele tem histórico limpo; o anterior fica no `rasgo-instruments` até
`3fd0908`. O release
[v0.1.0](https://github.com/lucioaraujo/trioio/releases/tag/v0.1.0) é um
pré-lançamento, com os seis pacotes. A CI achou e o Claude corrigiu três
defeitos fora do Linux (`docs/MULTIPLATAFORMA.md`).

- **Daqui em diante:** os commits e o push do Trioio são feitos nesta
  pasta, que é um repositório próprio. A CI só roda nas tags `v*`.
- **Próximo** (o autor):
  - instalar e abrir no Windows e no Mac reais;
  - seguir tocando e julgando as escutas.


**2026-10-19: o site no ar** — https://lucioaraujo.github.io/trioio/
(GitHub Pages, `.github/workflows/deploy-site.yml`).

- **Feito:**
  - a imagem da interface fica logo depois da abertura, acima de Baixar
    (pedido do autor);
  - Baixar traz os seis pacotes da v0.1.0;
  - a seção nova "Palavras, influências e gesto";
  - o `noindex` saiu.
- **Portal RASGO:** o card do Trioio está pronto, e o pacote
  `RASGO_WEBSITE/rasgo-website.zip` está gerado. **O envio à HostGator é
  do autor.**
- **Numa versão nova:** mudar `VERSAO` em `website/gerar_paginas.py` e em
  `website/seo.py`, e gerar as páginas de novo.

**2026-10-19: as menções a Vitoriamario no repositório público.** São 82, em
9 arquivos. Entre elas:

- o anagrama, em `docs/TRIOIO_CONCEITO.md`, na proposta e nas tarefas;
- "Identidade: Vitoriamario";
- o jogo de Vitoriamario, em `docs/JOGO.md`.

**Decisão do autor:** "deixar como está". Os documentos de desenvolvimento
são o registro do processo. A regra de não trazer o anagrama vale para os
textos de apresentação: o site e o README em quatro idiomas, que não o
trazem.

**2026-10-19: v0.1.1 publicada** (pré-lançamento): o botão **arquivo ▾**
(mostrar na pasta, exportar, lixeira).

- A CI passou na primeira rodada.
- O site e o README apontam para a 0.1.1.
- O `CHANGELOG.md` começa aqui.
- **Numa versão nova:** o número da versão está em `CMakeLists.txt`,
  `README.md`, `INSTALL.md`, `website/gerar_paginas.py` e
  `website/seo.py`.
- **A ordem do envio:**
  1. enviar só a tag;
  2. esperar o release;
  3. enviar o `main`, que publica o site sem links quebrados.
