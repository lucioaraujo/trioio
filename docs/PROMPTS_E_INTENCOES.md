# Prompts e intenções

Como o Trioio recebe o prompt de uma referência e o que faz com ele. É a
implementação do desenho em
[`BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md`](BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md)
("prompts.json").

## As três camadas (nunca se substituem)

| camada | o que é | quem escreve |
|---|---|---|
| `promptOriginal` | o texto, como veio, com a fonte (`autor`, `gerador`, `metadados`) e a data; só existe quando foi fornecido | o autor (`prompt … "texto"`) |
| `declaredIntent` | a **tradução** do prompt para o vocabulário do Trioio. Cada palavra que deu um campo fica à vista, e as que não deram ficam listadas como "não lidas". O autor pode revisar | `translatePrompt`; o autor (`--revisar`) |
| `analysisInferred` | o que a **escuta do áudio** mostra, no mesmo vocabulário | `inferIntent`, a cada resposta |

**O que o Trioio não faz:**

- **Não alega recuperar os parâmetros internos de um gerador.** A tradução
  lê palavras, e a inferência lê o som.
- **Não guarda prompt inventado:** sem prompt fornecido, não há
  `promptOriginal`. Uma intenção pode ser declarada direto (`--revisar`),
  sem prompt.

## O vocabulário

Cada campo é opcional:

- **energia, densidade, brilho, harmonicidade:** de 0 a 1;
- **andamento:** em BPM;
- **humor:** as intenções da máquina de escalas (luminosa, sombria, tensa,
  serena, suspensa, ambígua);
- **estilo:** um dos 15 conjuntos;
- **linguagem harmônica:** uma das 26;
- **voz:** cantada ou instrumental.

**O léxico** (`src/core/Intent.cpp`) lê português, inglês, francês e
espanhol por radicais:

- "energ" pega energético, energetic, énergique, enérgico;
- "calm", "soft" e "doux" dão energia baixa;
- "dark", "escuro" e "sombre" dão brilho baixo;
- "dreamy" e "ambient" dão humor suspenso;
- "baião", "forró" e "xote" dão o conjunto de forró;
- "120 bpm" dá o andamento.

**As palavras que não deram campo** ("saxophone", "folk") ficam listadas
como não lidas. O Trioio diz o que não entendeu, em vez de inventar.

## O pedido × o que soa

`compareIntent` compara, campo a campo, o declarado e o ouvido:

- **os números:** a distância de 0 a 1;
- **o andamento:** a razão, em que o dobro e a metade contam como o mesmo
  pulso em outro nível;
- **o humor:** igual ou outro.

Cada item vira uma frase: "energia: pedida 0,20, ouvida 0,70 — a gravação é
mais intensa que o prompt".

## Na resposta

Com um `prompts.json`, `responder` usa a intenção (desligue com
`--prompt nao`). Isto é `applyIntent`:

1. **O que o autor não escolheu na interface vem da intenção declarada:** o
   humor guia a escala, o estilo escolhe o conjunto e a linguagem harmônica
   entra. O que o autor escolheu fica.
2. **A distância média entre o pedido e o que soa abre o Terceiro Olho:**
   a imprevisibilidade sobe até +0,25. Onde o gerador não fez o que o
   prompt pedia, o Trioio tem mais espaço para propor.
3. **Tudo vai para a forma** ("intenção · …") e para o `decisoes.json`. A
   camada ouvida é regravada no `prompts.json`.

## Os comandos

```
trioio_biblioteca prompt <dados> <ref> "texto" [--fonte autor|gerador|metadados]
trioio_biblioteca prompt <dados> <ref> --mostrar
trioio_biblioteca prompt <dados> <ref> --revisar "energia=0.3,humor=serena,estilo=jazz,harmonia=modal,andamento=90,voz=sim"
trioio_biblioteca prompt <dados> <ref> --apagar
trioio_biblioteca responder … [--prompt nao]
```

**Onde fica:** o arquivo é `<dados>/indice/prompts/<ref>.json` (esquema
`trioio.prompts/1`, ver [`FORMATOS_DE_DADOS.md`](FORMATOS_DE_DADOS.md)). O
manifesto da referência ganha `promptOriginalProvided` e `promptsRef`, e
apagar o prompt desfaz as duas marcas.

## Testado

- **`IntentTests`:**
  - um prompt de gerador em inglês, campo a campo, com as não lidas;
  - português e francês;
  - um prompt sem campos não inventa nada;
  - a inferência sobre quadros sintéticos;
  - a comparação, com o dobro do andamento contando como perto;
  - na resposta, só preenche o que o autor não escolheu e abre o olho até
    +0,25.
- **`PromptsTests`:**
  - fornecer, revisar e recusar o que não entende;
  - a camada ouvida separada;
  - apagar desmarca o manifesto.
- **De ponta a ponta** (2026-10-18, `suno.mp3`, com um prompt de teste
  apagado depois e o manifesto restaurado byte a byte):
  - pedido: "calm acoustic folk ballad, 70 bpm, warm and nostalgic";
  - ouvido: energia 0,70, brilho 0,89, humor tenso;
  - distância de 0,55, imprevisibilidade de 0,50 para 0,64.

## O modo híbrido: áudio + prompt + gesto (2026-10-18)

Três entradas compõem a mesma resposta, cada uma com o seu papel:

| entrada | o que dá | onde |
|---|---|---|
| o áudio da referência | a matéria: motivo, acordes, pulso, seções, corpo espectral | a escuta |
| o prompt (a intenção) | o humor, o estilo, a linguagem harmônica; a distância entre o pedido e o ouvido abre o Terceiro Olho | `prompts.json`, `applyIntent` |
| o gesto do autor | a forma: a curva de intensidade da peça | `--gesto`, `gestureCurve` |

**O gesto** é escrito como `--gesto "0:0.1,0.75:1,1:0.2"`: pares
posição:intensidade, de 0 a 1, com pelo menos dois pontos, interpolados.
Ele substitui a curva automática do modo na dramaturgia, no rasgo modular e
nos efeitos de cinema:

- a dinâmica vai de 0,6 a 1,4×, mais larga que os 0,85 a 1,15× da curva
  automática, porque o gesto é uma intenção explícita;
- as vozes de enfeite rareiam nos vales, até 80 % no fundo;
- o motivo sobe uma oitava no clímax (o ponto mais alto do gesto) e desce
  nos vales;
- há uma respiração antes do clímax.

**Testado** (`DramaturgyTests`):

- a curva interpolada, o clímax no ponto mais alto, a leitura do texto e a
  recusa do texto mal escrito;
- um gesto que sobe deixa a força média do fim maior que a do começo (0,63
  contra 0,37).

**A escuta:** `escutas/2026-10-18-gesto/` (sem gesto, crescendo até ¾,
esvaziando no meio), com LEIA-ME. Medido por quarto da peça: com o gesto que
cresce, o terceiro quarto sobe de −22,4 para −18,9 dB.

**No app** (desde 2026-10-18), no fim do painel **Linguagem ▸**:

- **o campo "Gesto":** aceita a mesma escrita da linha de comando. Um gesto
  mal escrito é ignorado, e fica a curva do modo;
- **o editor logo abaixo:** clicar ou arrastar põe ou move um ponto (da
  esquerda, o começo, à direita, o fim; de baixo, vazio, a cima, pleno), e o
  duplo clique tira o ponto mais perto;
- **o campo e o editor ficam sincronizados:** o desenho escreve o texto, e
  o texto redesenha a curva.

Conferido nos instantâneos em francês, com a dica traduzida. O uso com o
mouse ainda não foi testado (o modo de instantâneos não clica): fica para
a escuta do autor.

**Ainda não:**

- **a escuta ao vivo com reorganização em curso** (o "HÍBRIDO" do desenho
  da Biblioteca: começar já e reorganizar ao reconhecer seções). A base é a
  memória curta (`ShortMemory.h`), e o resto pede a escuta em tempo real.

## As palavras da resposta: o prompt opcional (2026-10-18)

O autor: "pode trabalhar com uma ideia de prompt no trioio para que seja
escritas algumas palavras chave, algo como uma ferramenta opcional? caso não
seja escrito nada o trioio trabalha com seus critérios e cálculos."

**Onde:** o campo **Palavras**, no topo do painel **Linguagem ▸**, ou
`--palavras "…"`. Vale para aquela resposta e vence o prompt guardado da
referência. Vazio, nada muda.

**O que as palavras dão:**

- **a intenção,** pelo léxico: energia, densidade, brilho, harmonicidade,
  andamento, humor, estilo, subestilo do rock, linguagem harmônica, voz;
- **a forma:** "drama" ou "clímax" dão Freytag; "virada", "twist" ou
  "surpresa" dão ki-shō-ten-ketsu; "ondas"; "hipnótico" ou "trance" dão
  platô; "arco";
- **a dança:** "dançante", "groovy" ou "suingado" sobem o eixo;
  "introspectivo", "contemplativo" ou "meditativo" o baixam;
- **os nós da árvore de influências** cujo nome aparece: "maracatu",
  "post-rock", "minimalismo", "jazz". Os nós qualificativos (dançante,
  introspectivo, livre…) só entram com o pai no texto: "dançante" sozinha é
  o eixo; "rock dançante" é o nó.

**Tudo fica à vista,** na forma e no `decisoes.json`: o que foi entendido,
os nós chamados e as palavras não lidas.

**Exemplo:** "algo dançante e escuro com maracatu, uma virada no meio,
cheiro de chuva".

- **o que foi entendido:**
  - a dança, o brilho baixo, a harmonia do maracatu e a forma
    ki-shō-ten-ketsu;
  - o nó `brasil/nordeste/maracatu`;
  - a virada trouxe o galho polinizado (o ruído experimental);
- **o que não foi:** "cheiro de chuva", listada como não lida (ver abaixo);
- **a escuta:** `escutas/2026-10-18-palavras/`.

**Testado:** `IntentTests` (a forma e a dança pelas palavras) e
`InfluencesTests` (as palavras chamam os nós; o qualificativo pede o pai).

### Como o Trioio transforma as palavras em cálculo (e as alternativas)

**Hoje: um léxico.** É uma tabela própria de radicais em português, inglês,
francês e espanhol. Cada radical dá um campo e um valor ("escur" → brilho
0,2; "virada" → ki-shō-ten-ketsu), e os nomes da árvore de influências
também valem.

- **As vantagens:**
  - é transparente: cada palavra e o que ela deu aparecem;
  - é determinístico e offline;
  - não depende de modelo nem de serviço externo;
  - o autor pode ampliar a tabela.
- **O limite:** só entende o que está na tabela. A metáfora ("cheiro de
  chuva") e o sinônimo fora dela ficam como não lidos.

**As alternativas (para decidir com o autor; nada adotado):**

1. **Ampliar o léxico,** com sinônimos e metáforas escritos pelo autor num
   arquivo: "chuva" pode dar brilho baixo, densidade em grãos, humor
   suspenso. É barato e fiel à poética dele, mas cresce à mão.
2. **Embeddings de texto locais:** um modelo pequeno de frases,
   multilíngue, rodando no computador. Ele põe cada palavra num espaço de
   sentido, e a palavra não lida vai para a entrada do léxico mais perto
   ("garoa" fica perto de "chuva"). O custo é carregar um modelo (dezenas a
   centenas de MB), com a sua licença e proveniência, e o resultado fica
   menos explicável.
3. **Um espaço comum de texto e áudio** (o tipo MuLan ou CLAP, Atlas de
   18/10): compara as palavras diretamente com o som da referência e da
   resposta. É o mais forte, mas também o mais pesado, e os dados de treino
   desses modelos vêm de música de terceiros.

**A recomendação:** começar pelo 1, coerente com "não incluir arquivos de
terceiros", com o 2 como fallback opcional se o autor quiser. O 3 só com
uma decisão sobre a proveniência.

**A decisão do autor (2026-10-18): a consulta na rede.** "Não gostaria de
deixar o app muito pesado com um recurso de texto, ele seria interessante
para o app se fosse uma consulta linguística (semântica) na internet. E o
app fosse capaz de calcular com base nessas consultas." E depois: "o app
pode ter uma lista de sites que prestam esse serviço. Fazer a conexão de
busca e retorno rápido, sem ocupar muito tempo do trabalho de composição.
Esse recurso deve ser ágil, prático e não tornar o uso do app monolítico."
Daí a ponte semântica, abaixo. Os embeddings locais (o 2) e o espaço de
texto e áudio (o 3) ficam de fora: pesam o app.

### A ponte semântica pela rede (2026-10-18)

A palavra que o léxico não leu é consultada em serviços públicos. **Quem
calcula é o Trioio:** os serviços só devolvem palavras, e o léxico decide os
campos. O app não embute léxico grande nem modelo; o código é
`src/library/SemanticBridge.h/.cpp`, e o cálculo é
`intentFromAssociations` em `src/core/Intent.h`.

```text
"chuva" (qualquer língua)
  → Wikidata, lexemas (CC0): o lexema L648257 (pt) → o sentido traduzido
    (P5972) L3325 "rain"; sem tradução no sentido, o conceito dele (P5137)
    e o rótulo inglês (Regen L405265 de → Q7925 "rain")
  → Datamuse: os adjetivos que descrevem "rain" (rel_jjb: heavy, cold,
    gentle, steady, torrential…) e os sentidos próximos (ml: downpour,
    drizzle, storm…), cada um com o seu peso
  → o léxico do Trioio lê cada associação; os campos numéricos são a média
    ponderada; o humor e a forma são o mais votado (≥ 25 % do peso)
  → chuva: energia 0,47, densidade 0,65, brilho 0,33
```

**A lista de serviços** é o arquivo `data/semantica/fontes.txt`. Tem uma
linha por fonte: o papel (tradução ou associação), o formato da resposta, o
endereço-modelo e o peso. O autor tira, troca ou acrescenta fontes sem
recompilar. Os formatos conhecidos são `wikidata-lexema`, `datamuse` e
`conceptnet`. A API pública do ConceptNet (CC BY-SA 4.0; Speer, Chin &
Havasi, 2017) estava fora do ar em 18/10 (502) e fica comentada até voltar.

**Ágil, sem prender a composição:**

- **Paralelo e com prazo.** As fontes de um papel são consultadas em
  paralelo, dentro de **um** tempo limite para tudo: 4 s na resposta, por
  padrão. A tradução usa até 70 % desse tempo. O que não respondeu a tempo
  fica de fora, e a resposta segue sem esperar.
- **Em lote.** Com a Wikidata são poucas requisições para todas as
  palavras: uma busca por palavra, duas por vez; uma requisição com os
  sentidos de todos os lexemas; os lemas e os conceitos juntos.
- **Sem rajada.** A Wikimedia limita rajadas de clientes sem conta: uma
  rajada de 30 buscas paralelas recebeu 429 em 18/10. Por isso o pedido
  segue a sua política: o User-Agent identifica o Trioio e o projeto, e
  as buscas vão duas por vez.
- **Com memória.** O que já foi consultado fica em
  `<dados>/memoria/semantica.json`, na pasta de dados da pessoa e nunca no
  repositório. A segunda vez é instantânea e não usa a rede.
- **Enquanto se escreve.** No app, a caixa "consultar na rede as palavras
  não lidas" consulta em segundo plano 0,8 s depois da última tecla. A
  linha abaixo das Palavras mostra `rede: chuva → rain   saudade →
  saudade`. Quando o autor pede a resposta, as palavras normalmente já
  estão na memória.
- **Só o que falta.** São consultadas no máximo 5 palavras não lidas. O
  que vem da rede só preenche os campos que as palavras lidas deixaram em
  aberto. As associações dão o **caráter** (energia, densidade, brilho,
  harmonicidade, dança, humor, forma), nunca o estilo: "heavy rain" é
  chuva forte, não rock pesado.
- **Opcional e desligada por padrão,** porque as palavras saem do
  computador. Sem internet, ou com a fonte fora do ar, o Trioio segue só
  com o léxico, como antes.

**Os comandos:**

```text
trioio_biblioteca semantica <dados> "chuva, saudade, Regen, pluie" [ms]   # só a consulta e o cálculo
trioio_biblioteca responder <dados> <id> --palavras "chuva fina" --semantica sim [--semantica-tempo 4000]
TRIOIO_SEMANTICA_DEBUG=1 …   # cada requisição, o tempo e o tamanho, no stderr
```

**Medido em 2026-10-18,** primeira consulta, sem memória, 5 palavras:

| Palavra | Caminho | Resultado |
| --- | --- | --- |
| chuva | Wikidata L648257 (pt) → L3325 "rain" | energia 0,47, densidade 0,65, brilho 0,33 |
| saudade | Wikidata L619182 (pt) → Q1338509; Datamuse | humor sombria (nostalgia) |
| Regen | L405265 (de) → Q7925 "rain" | igual a chuva |
| pluie | L12049 (fr) → Q7925 "rain" | igual a chuva |
| tempestade | sem tradução na Wikidata; o Datamuse não conhece a palavra portuguesa | fica não lida |

As cinco levaram cerca de 3,1 s no total. A mesma consulta, já na memória,
é instantânea.

**Os limites:**

- A cobertura da Wikidata é desigual: "tempestade" ainda não tem tradução
  lá.
- O Datamuse é um serviço de língua inglesa, e as associações dele refletem
  o uso do inglês. A tradução é o que permite o uso multilíngue.
- O léxico precisa dos descritores ingleses para ler as associações. Eles
  foram ampliados em 18/10: cold, gentle, steady, torrential, gloomy…

**As fontes:**

- Wikidata, dados CC0: https://www.wikidata.org/wiki/Wikidata:Licensing
- Datamuse API, gratuita e sem chave: https://www.datamuse.com/api/
- Speer, R., Chin, J., & Havasi, C. (2017). ConceptNet 5.5: An Open
  Multilingual Graph of General Knowledge. AAAI.
- Wikimedia API rate limits:
  https://www.mediawiki.org/wiki/Wikimedia_APIs/Rate_limits

### A escrita do autor como vier (2026-10-18)

**O primeiro uso no app.** O autor escreveu `macumba,, maculele, sinfonica,
post rock` em Palavras e `modos gregos, escalas africanas` em Mistura. A
resposta mostrou quatro falhas:

- "post rock" foi lido como duas palavras. "post" foi para a rede, e a
  Wikidata achou o alemão *Post* (correio).
- "sinfonica", sem acento, não foi lida.
- As associações de "macumba" (maracatu, samba, baião, afoxé) eram estilos,
  e o cálculo da rede descarta o estilo.
- A Mistura só aceitava "+" e o nome exato de uma escala.

Depois o autor pediu que o Trioio reconheça "falta de acentos, acentos
indevidos, separação de palavras, erros gramaticais", "caixa alta, caixa
baixa, números e caracteres especiais". O código está em
`src/core/TextFold.h`, usado pelo léxico, pela árvore e pelo arquivo de
escalas.

| A escrita | O que o Trioio faz |
| --- | --- |
| Caixa alta ou baixa; acento faltando ou indevido | Os dois lados da comparação ficam sem caixa e sem acento: `MELANCÓLICO`, `sinfonica`, `Dancánte` |
| Pontuação, aspas tipográficas, travessões, `#`, `@`, emoji | Separam palavras: `“Pós Rock”!!! 🎉 #ruidoso` |
| Números | `120bpm` e `120 bpm` dão o andamento; o número solto (`1998`) é ignorado e não vai para a rede |
| Separação | `post rock`, `postrock` e `post-rock` são o mesmo termo, no léxico e na árvore |
| Erro de digitação ou de grafia | Até 1 letra de diferença (trocada, faltando, sobrando ou invertida); até 2 nos radicais de 9+ letras |
| Duas letras vizinhas invertidas | Aceitas também nas palavras de 4 letras: `rokc` → rock |
| Flexão (plural, gênero) | `sambas` → samba |

A correção aparece na forma, como `melacolico → correção ≈ melanc`.

**Prudência.** Os radicais curtos do léxico só valem exatos: "escur"
(escuro) não corrige "escuta". As palavras de 3 letras nunca são
corrigidas (`jaz` fica não lida). Os nós qualificativos da árvore ("rock
dançante") não são corrigidos.

**A rede e a árvore.** As associações da rede que são nós da árvore
entram como influências, até dois por palavra: macumba → brasil/nordeste/
maracatu e brasil/samba. O cálculo do caráter continua sem o estilo.

**A Mistura.** Aceita vírgula além de "+". Também aceita regiões e
famílias escritas pelo autor: "modos gregos" → Grécia antiga (140
escalas); "escalas africanas" → Uganda e Moçambique (amadinda, timbila);
"árabe" → maqam; "indiana" → Índia; "japonesa" → Japão. As palavras
genéricas ("escalas", "modos") saem da busca.

**Refeito com as palavras exatas do autor:** post rock → rock/post-rock;
sinfonica → orquestra; macumba → maracatu e samba pela rede; a Mistura →
Aristóxeno cromático (Grécia antiga) e timbila (Moçambique).

**A Mistura, depois do segundo uso (2026-10-18).**

- "dim dom" → a escala diminuta (octatônica), que no arquivo é o modo 2 de
  Messiaen.
- "miles davis" → os modos dóricos (a prática modal de *Kind of Blue*).
- "cromatica", "modal", "octatônica", "andaluza", "quartal" e "espectral"
  sem "harmonia" na frente ficam como escala. Na rodada anterior,
  "cromatica" tinha ido para a harmonia, o que era um erro.
- "harmonia cromática" (com o prefixo) vai para a harmonia.
