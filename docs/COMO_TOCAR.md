# Como tocar o Trioio

Criado em 2026-10-14, a pedido do autor ("preciso aprender a tocar o
instrumento"). Os nomes citados aqui são os da interface em português,
conferidos no código (`src/app/MainView.cpp`, `src/app/UiLanguage.h`).

## O que é tocar o Trioio

O Trioio não se toca nota a nota. É uma conversa em quatro tempos:

1. **Você traz uma gravação**, a referência.
2. **O Trioio a escuta e compõe uma resposta**: um arquivo novo, todo
   sintetizado, que nunca contém o áudio da referência.
3. **Você ouve e julga**, a resposta inteira, uma seção ou um trecho.
4. **O Trioio aprende com os seus julgamentos.** A próxima resposta muda
   por causa da sua escuta.

Tocar é escolher a referência, o modo, a linguagem e o que vale. O
instrumento compõe; você conduz pela escuta.

## Abrir

```
build/trioio_app_artefacts/Release/Trioio --dados <pasta de dados>
```

Sem `--dados`, o app pode abrir em **demonstração (sem raiz de dados)**.
Nesse modo não se compõe nada. Use **abrir pasta de dados…** para escolher a
pasta de dados.

Os dados das escutas de teste ficam em
`/media/luc/4tb_21042021/AUDIOS_RASGO/Trioio/dados`.

## O tutorial no app

O botão **TUTORIAL**, ao lado de **SOBRE** (ou no menu ☰, na janela
estreita), abre os 11 capítulos deste guia dentro do app:

- abre na língua do instrumento: EN, PT, FR ou ES;
- tem o sumário à esquerda e uma **DICA** em destaque nos capítulos que
  têm uma;
- navega com **anterior/próximo** e com as setas do teclado; Esc fecha.

O conteúdo está em `src/app/Tutorial.h`. Quando este guia mudar, o
tutorial muda junto.

## As três abas

| aba | para quê |
|---|---|
| **Escuta e resposta** | onde se toca: a referência em cima, o Terceiro Olho e a resposta embaixo; os modos, o play, o master e o julgamento |
| **Biblioteca** | as gravações, para trazer, autorizar e escolher |
| **Sua escuta** | as versões já compostas, a comparação A × B, o que os seus julgamentos ensinaram e as regras do jogo |

Numa janela larga, a Biblioteca fica sempre visível à esquerda.

## Passo a passo

### 1. Trazer uma gravação

1. Clique em **escolher referência**, que leva à Biblioteca.
2. Clique em **trazer gravação…** e escolha o arquivo.

### 2. Autorizar

Uma gravação nova começa **pendente**.

1. Clique em **autorizar…** e diga:
   - a **origem**: autoral, captura, outro instrumento, IA, licenciado,
     domínio público…;
   - **com que direito** ela pode ser usada;
   - quem declara.
2. Clique em **autorizar**.

Sem autorização, o Trioio não compõe sobre a gravação. Isso protege a
proveniência.

### 3. Ouvir a referência

1. Na aba **Escuta e resposta**, clique em **▶ referência**.
2. A primeira análise demora um pouco. O status mostra "ouvindo…".
3. Quando terminar, a faixa **REFERÊNCIA — o que soa** mostra as seções
   lidas.

### 4. Escolher o modo

O modo é a atitude da resposta:

| modo | a resposta… |
|---|---|
| **seguir** | acompanha a referência: mesmo campo, mesmo pulso |
| **transfigurar** | transforma a referência: a mesma matéria, outra figura |
| **contrapor** | vai contra: outro registro, outra densidade |
| **interromper** | corta, entra e sai |

### 5. Escolher a linguagem (opcional)

Em **Linguagem ▸**, ficam:

- a **mistura** de escalas, por exemplo "rāga Yaman + maqam Hijaz";
- as **alturas**, a **técnica**, a **costura**, o **ciclo**, o **voicing**,
  a **intenção**, a **forma** e o **andamento**;
- a **imprevisibilidade (o Terceiro Olho)**;
- o **conjunto**: o estilo da formação. Ele dá a cada voz o instrumento do
  estilo (no choro: flauta, bandolim, 7 cordas, cavaquinho e pandeiro) e
  toca com as figuras e o fraseado dele.
  - **automático:** o Trioio escolhe os timbres, sem estilo;
  - **lido na referência:** o Trioio escolhe o estilo pelo que escutou, e o
    modo responde (seguir toca no estilo lido, transfigurar vai ao
    vizinho, contrapor vai ao contrário);
  - **os estilos:** choro, samba, escola de samba, jazz, manouche, bolero,
    salsa, charanga, forró, tango, rock, câmara, orquestra e *a cappella*.
- **efeitos de cinema na dramaturgia:** *riser*, impacto e *braam* no
  clímax, *whoosh* nas seções, ambiências nos trechos calmos;
- duas caixas:
  - **diálogo: responder nas pausas da referência**;
  - **jogo: o Trioio inventa as próprias regras**.

Deixar tudo como está também é uma escolha: o Trioio decide a partir da
escuta.

### 6. Compor

1. Clique em **compor de novo**. Leva cerca de meio minuto.
2. A resposta aparece na faixa **RESPOSTA — o que o Trioio compõe**.
3. Clique em **revelar depois de ouvir** para ver o que o Terceiro Olho fez:
   ele fica escondido até você ouvir, para não guiar a sua escuta.

### 7. Ouvir

- **▶** toca a resposta.
- **▶ juntos** toca a referência e a resposta ao mesmo tempo, só para ouvir.
- O **master** regula o volume de tudo.
- **arquivo ▾**, junto da resposta escolhida (2026-10-19):
  - **mostrar na pasta** abre o gerenciador de arquivos na sessão da
    resposta;
  - **exportar…** copia o WAV, o MIDI, os stems, as linhas e
    `decisoes.json` para a pasta que você escolher, numa subpasta com o
    nome da sessão. Nada é sobrescrito;
  - **mover para a lixeira…** pede confirmação e manda a sessão para
    `lixeira/AAAA-MM-DD/`, de onde ela pode voltar para `sessoes/`. O
    julgamento, se houver, continua na sua escuta.

### 8. Julgar

Escolha o que julgar:

| ação | o que fica escolhido |
|---|---|
| clicar fora da seleção | a resposta inteira |
| clicar num bloco da faixa da resposta | aquela seção |
| arrastar sobre a faixa da resposta | o trecho marcado |

Depois:

1. Escreva uma **nota** se quiser. É opcional. Ela é a sua frase sobre
   aquela escuta, por exemplo "o chip ficou duro demais" ou "boa matéria,
   forma fraca":
   - fica gravada junto do julgamento;
   - aparece na lista de versões da aba **Sua escuta**;
   - fica no arquivo, como registro do porquê.

   O Trioio ainda **não lê** a nota para compor: quem pesa é o veredito
   (`docs/ESCUTA.md`, "Limites").
2. Clique em **aceitar**, **transformar** ou **rejeitar**.

O julgamento "pesa nas próximas respostas". Vale também para as regras do
jogo:

- **Aceitas** ficam na memória (até 8).
- **Rejeitadas** vão para o adubo.

### 9. Comparar versões (aba Sua escuta)

1. Clique numa versão da lista **Versões sobre esta referência**: ela é o
   **A**.
2. Clique em outra: ela vira o novo **A**, e a anterior vira o **B**.
3. A lista mostra quem é quem:
   - um selo verde **A** e um selo azul **B** nas duas versões;
   - **▶** no selo da versão que está tocando;
   - acima dos botões, a linha "A: … · B: …".
4. Use **▶ comparar A × B** e **alternar A ↔ B**. A alternância troca de
   versão no mesmo momento da música.

À direita da lista estão:

- **O que a escuta ensinou**: o quanto cada escolha subiu ou desceu com os
  seus julgamentos;
- **Regras do jogo na versão A**;
- a caixa **a próxima resposta leva a escuta em conta**.

### 10. Levar para a DAW: os stems (linha de comando)

```
trioio_biblioteca responder <dados> <ref> --modo seguir --stems sim
```

Grava, ao lado da mistura, um WAV estéreo de 24 bits por voz que soou, em
`sessoes/<sessão>/saidas/stems/`:

- `1-motivo`, `2-baixo`, `3-eco`, `4-memoria`, `5-percussao`, `6-mosaico`,
  `7-harmonia`: cada voz já processada (equalização, compressão e lugar no
  palco do conjunto), seca;
- `8-gestos`: os gestos do Terceiro Olho e os efeitos de cinema;
- `9-sala`: a reverberação de todas as vozes.

**Os stems não são para ouvir soltos.** Cada um só tem som onde a sua voz
toca: numa peça de jazz, o eco soa em 2 % do tempo, o mosaico em 3 % e os
gestos em 4 %. Todos ficam no nível em que entram na mistura, sem
normalização. Para ouvir, use a mistura. O `LEIA-ME.txt` da pasta dos stems
diz quanto cada voz soa e em que nível.

**Como se somam:** os stems têm o mesmo ganho final da mistura e passam pela
mesma curva do master (`--master sim`), então somados em 0 dB refazem a
mistura. Só a cola de barramento da mistura fica de fora: na soma, cerca de
0,7 dB e correlação de 0,9997 (`ProductionTests`).

**O custo** (Recording_187, 224 s, conjunto jazz): 245 s em vez de 220 s, e
1,6 GB de memória em vez de 0,87 GB.

**No app:** a caixa "stems: também um WAV por voz", no fim do painel
Linguagem.

**No modo linhas** (`--linhas 200,2000 --stems sim`): os stems por voz somam
as linhas, cada uma filtrada na sua banda e com o seu ganho final. Os
`saidas/linhas/` continuam sendo um WAV por banda. Enquanto compõe, o Trioio
guarda os stems de cada linha num temporário da pasta de dados
(`tmp-stems-…`, apagado no fim), para não ter todas as linhas na memória.
Conferido no vocalise com três linhas: a soma dos stems refaz a mistura com
correlação de 0,9999 e −0,9 dB (a cola de cada linha). Levou 118 s e 0,64 GB.

**Para ouvir um exemplo:** `escutas/2026-10-17-stems-jazz/` (os nove stems
e `0-mistura.wav`).

## Um primeiro exercício

1. Use uma gravação curta, de 30 s a 2 min, sua.
2. Componha no modo **seguir**. Ouça, julgue a resposta inteira.
3. Componha de novo no modo **contrapor**.
4. Em **Sua escuta**, compare as duas com **alternar A ↔ B**.
5. Ligue o **jogo** em **Linguagem ▸** e componha de novo.
6. Leia as regras na aba **Sua escuta**. Julgue por trecho onde uma regra
   agiu bem ou mal.
7. Componha mais duas ou três vezes. As respostas devem começar a mudar com o
   que você aceitou e rejeitou.
