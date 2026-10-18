# TRIOIO — dossiê conceitual inicial

**Estado:** investigação, arquitetura inicial e protótipo de core C++20
**Data:** 2026-08-12  
**Projeto:** RASGO  
**Identificador arquivístico:** `ARQ-TRI-001`
**Identidade:** Vitoriamario

> Este documento registra hipóteses de trabalho. Não é ainda uma especificação
> definitiva de produto nem autoriza implementação apressada.

O Trioio deriva da bifurcação do projeto anterior “Maravi Tri Oio”, em
2026-08-20. No catálogo, Maravi conserva `ARQ-MAR-001` e Trioio recebe
`ARQ-TRI-001`; documentos históricos sob o nome anterior preservam a
proveniência comum sem confundir os dois projetos atuais.

## 1. Identidade e Terceiro Olho

`TRIOIO` é a forma anagramática de `VITORIAMARIO`. O nome é uma
transformação da identidade central, não uma entidade separada.

O Terceiro Olho é a capacidade de perceber simultaneamente o evento, o sistema
que organiza o evento e a possibilidade de transformar esse sistema. Não é um
personagem independente nem se reduz à trapaça: pode significar percepção,
intuição, antecipação, memória, cuidado, mudança de perspectiva e criação de
relações não evidentes.

```text
olho 1       participa
olho 2       observa
Terceiro Olho percebe a estrutura e pode transformá-la
```

## 2. Composição musical e escuta

Este é um eixo fundamental do projeto e deve acompanhar todas as decisões de
design e engenharia. O instrumento será desenvolvido por investigação
aprofundada dos territórios de:

- composição musical e formas de organização temporal;
- teoria musical, sem restringir o sistema à tradição tonal;
- gesto musical, articulação, intenção e consequência sonora;
- comportamento musical, incluindo repetição, variação, espera, tensão,
  repouso, insistência, ruptura e silêncio;
- cultura musical, contexto, memória coletiva, convenções e desvios;
- escuta situada, escuta corporal, escuta crítica e escuta do próprio sistema;
- relações entre material, forma, performance, ambiente e tecnologia;
- composição algorítmica, improvisação, jogo e coautoria.

O objetivo não é ensinar teoria musical ao usuário de maneira escolar, mas
construir um instrumento que compreenda música como relação entre eventos,
expectativas, gestos, culturas e contextos.

Uma nota, um ruído ou um ataque não será tratado como unidade autossuficiente.
Seu sentido depende da trajetória que o antecede, do campo que o envolve e da
consequência que produz.

```text
escutar → reconhecer relações → formular hipótese
→ agir musicalmente → ouvir a consequência → reformular
```

O sistema deve ser capaz de responder não apenas “o que está soando?”, mas:

- o que está tentando acontecer;
- que expectativa foi criada;
- qual gesto a alterou;
- que cultura ou convenção está sendo evocada;
- quando uma repetição é estrutura e quando vira automatismo;
- quando o silêncio é ausência e quando é ação;
- que tipo de escuta o próprio instrumento está propondo.

## 3. Jogo e composição

O jogo será a lógica de composição e performance, não uma gamificação da GUI.
Participam dele o performer, o áudio de referência, a síntese de energia, o
acaso, a memória e Vitoriamario como agente estratégico.

Recursos: energia, tempo, densidade, memória, coerência, surpresa, informação
oculta, risco de colapso e liberdade de desvio.

Ações: escutar, imitar, transformar, atrasar, ocultar, repetir com diferença,
interromper, trocar a regra, responder e produzir uma consequência inesperada.

```text
referência → leitura → estratégia → desvio → nova referência
```

O Terceiro Olho pode revelar uma regra, propor outra, proteger o equilíbrio,
interromper um padrão ou transformar uma falha em material. A trapaça é apenas
um modo possível dessa operação.

## 4. Síntese sonora energética

A síntese de energia é núcleo do instrumento. O TRIOIO deve gerar música
sem depender de MIDI, de uma IA externa ou de conexão de rede.

Famílias a investigar e comparar: síntese aditiva e espectral, subtrativa,
waveshaping, FM, pulsos, granularidade, modelos modais, waveguides, cordas
físicas, ressonadores acoplados, síntese estocástica, feedback, caos,
análise-resíntese e DSP diferenciável como referência conceitual.

Cada módulo precisa ter conceito, comportamento, memória, gesto, limites e uma
consequência sonora verificável. A sofisticação estará nas relações entre os
módulos, não na quantidade de osciladores.

## 5. Escuta, prompt e áudio

O instrumento poderá escutar uma música, extrair sua organização energética e
reencená-la por síntese própria. O objetivo é reconstruir ou transformar sua
lógica acústica, não copiar sua forma de onda.

```text
áudio → análise de energia e estrutura → mapa de relações
→ estratégia Vitoriamario → parâmetros de síntese → nova forma musical
```

Descritores possíveis incluem energia, dinâmica, frequência fundamental,
transientes, densidade, centroide espectral, harmonicidade, ruído, pulsação,
andamento, campo estéreo, profundidade e relações entre eventos.

O prompt deve expressar intenções composicionais — rarefazer matéria, manter
ataques e trocar o corpo, negar a tonalidade, preservar tensão ou deformar uma
memória — e não apenas controlar frequências isoladas.

## 6. Magenta como referência opcional

O Magenta RealTime 2 confirma a viabilidade de combinar texto, áudio e MIDI em
geração musical contínua. Estudaremos contexto deslizante, prompt contínuo,
mistura de sinais e resposta frame a frame, sem copiar código, modelo ou
interface.

O núcleo será nosso: prompt estruturado, estratégia Vitoriamario, memória e
sintese energética RASGO. Uma IA conversacional externa poderá ser opcional,
mas o instrumento funcionará sem ela.

## 7. Memória e aprendizado

A primeira versão usará memória adaptativa local, registrando descritores e
decisões do usuário em vez de treinar imediatamente uma rede neural gigante.

```text
escutar → interpretar → sintetizar → registrar decisões → adaptar
```

Não capturaremos nem treinaremos com conteúdo obtido de serviços de streaming ou
plataformas externas. Para análise profunda, usar áudio próprio, autorizado, de
domínio público ou licenciado.

### Proveniência de áudio e prompts

O TRIOIO deverá aceitar referências de origens diversas: gravações próprias,
arquivos licenciados, sínteses de outros instrumentos, obras geradas por IA,
stems, MIDI e áudio capturado pelo próprio performer. Nenhuma plataforma é uma
dependência do projeto.

O importador separará quatro camadas:

- `audioSource`: o áudio efetivamente recebido;
- `metadataImported`: tags e informações técnicas encontradas no arquivo;
- `promptOriginal`: prompt fornecido explicitamente pelo usuário ou por um
  manifesto associado;
- `analysisInferred`: leitura produzida pelo próprio TRIOIO.

O sistema nunca afirmará ter recuperado um prompt a partir da forma de onda. Se
o prompt original não estiver disponível, o Cérebro produzirá uma interpretação
própria, claramente marcada como inferida. A proveniência, autorização e
limitações de uso acompanharão a referência sem serem confundidas com a análise
musical.

Prompts fornecidos voluntariamente pelo usuário podem servir como intenções
declaradas quando estiverem associados ao próprio acervo autoral. Eles não serão
tratados como acesso aos
parâmetros internos ou ao processo secreto de geração do serviço. O TRIOIO
traduzirá o texto para seu próprio vocabulário e comparará:

```text
intenção declarada × comportamento acústico observado
```

A diferença entre os dois poderá ser preservada, corrigida ou explorada como
material do Terceiro Olho.

Uma versão musical fornecida pelo usuário, com seu prompt de origem quando ele
existir, pode inaugurar o jogo composicional. A entrada não é uma ordem para
reproduzir a faixa nem um modelo a ser copiado. É uma referência autorizada a
partir da qual o instrumento lê relações e decide como responder por síntese
própria:

```text
versão autorizada + prompt + metadados/autorização
                         ↓
        intenção declarada × comportamento acústico observado
                         ↓
          estratégia do Terceiro Olho e síntese energética
                         ↓
          saída derivada + decisão e proveniência registradas
```

O potencial composicional está também na discrepância: um prompt que pede
calma pode produzir grande atividade de ataques; uma descrição de estabilidade
pode resultar em harmonia instável. O TRIOIO pode conservar energia e mudar
timbre, preservar ataques e negar tonalidade, manter a forma temporal e
inventar outra matéria. Cada escolha entre seguir, transfigurar, contrapor ou
interromper deverá ser registrada com a saída resultante.

### Biblioteca e velocidade de resposta

Uma Biblioteca de Música reunirá diretórios de acervo escolhidos pelo usuário.
Ela é externa ao código do instrumento e não altera os arquivos de referência:
indexa as faixas, apresenta sua identidade técnica e associa seus manifestos.
As derivações vivem em sessões separadas, junto de decisões, seed e saídas.

O início do jogo deve ser rápido. Ao soltar uma faixa, o TRIOIO prepara sua
leitura e pode escutar uma janela curta para publicar a primeira decisão e
iniciar a síntese. A análise de forma completa continua fora da thread de áudio
e transforma progressivamente contexto, memória e estratégia. Assim existem
três atitudes de escuta: `RESPONDER AGORA`, para descoberta em movimento;
`LER ANTES`, para resposta guiada pela forma inteira; e `HÍBRIDO`, que começa
imediatamente e se reorganiza ao reconhecer novas seções.

Waveform, descritores e mapas de uma referência conhecida podem ser
armazenados em cache. A síntese só recebe decisões leves e contínuas; nunca
espera decodificação, análise pesada ou comparação de versões.

O fluxo autoral previsto é:

```text
áudio autoral → prompt opcional + metadados → análise TRIOIO
→ estratégia do Terceiro Olho → nova síntese autoral
```

Plataformas externas podem ser apenas exemplos de origem. Nenhuma plataforma,
serviço ou método externo é finalidade ou dependência do TRIOIO. O escopo não
inclui importar, raspar ou reaproveitar prompts, letras ou áudios de terceiros.

Prompts e versões geradas por assistentes de composição poderão alimentar o
Terceiro Olho como material autoral de referência, quando fornecidos pelo
usuário ou cobertos por autorização. O TRIOIO poderá comparar versões,
recombinar intenções, observar divergências entre prompt e resultado e produzir
uma nova síntese própria. Isso é investigação composicional, não cópia de
modelo, treinamento automático sobre serviço externo ou reprodução de conteúdo
de terceiros.

## 8. Engenharia e excelência

O núcleo JUCE/C++ será multiplataforma, com perfis Essencial, Avançado e
Laboratório. Modelos externos poderão depender de hardware específico, mas não
serão requisito para a identidade do instrumento.

Regras: nenhum modelo pesado no callback; threads próprias para análise;
buffers lock-free; fallbacks; qualidade adaptativa; medição de CPU, RAM e
latência; testes em Windows, macOS e Linux; gravação protegida contra clipping
e falhas de tempo real.

Cada módulo seguirá:

```text
conceito → pesquisa → hipótese → protótipo → teste técnico
→ escuta → refinamento → integração
```

O resultado deverá oferecer síntese autoral, performance generativa, gravação
estéreo de alta qualidade, exportação float32/PCM24, stems reproduzíveis,
headroom, saneamento numérico e validação humana.

## 9. Próximo passo

Elaborar o dossiê do motor do Terceiro Olho: estados, memória, recursos, ações,
regras explícitas e ocultas, critérios de mudança e tradução dessas decisões
para o barramento de controle da síntese.

## 10. Referências consultadas

### Externas

- Magenta RealTime 2 — repositório e documentação oficial: geração contínua
  condicionada por texto, áudio e MIDI; usado como referência de interação, não
  como código ou dependência.
- Magenta DDSP — pesquisa sobre síntese digital diferenciável e combinação de
  análise com osciladores, filtros e reverberação; referência metodológica para
  análise-resíntese interpretável.

### Próprias do RASGO

- `TRIOIO_PROPOSTA.html` — proposta geral;
- `TRIOIO_CEREBRO.md` — arquitetura decisória;
- `TRIOIO_INVENTARIO_RASGO.md` — mapa de código próprio reutilizável;
- dossiês de energia, organismos e segurança realtime do AQUORBIUM;
- documentação de síntese generativa do RASGO_SYNTH;
- documentação de gravação e saída segura do NAVALHA2_JUCE.

As referências externas informam conceitos e critérios de investigação. A
implementação do TRIOIO permanece autoral.

O TRIOIO segue a metodologia RASGO de conteúdo livre: só entram materiais com
licença de uso verificável e compatível com a finalidade prevista. Autores,
pesquisadores, fontes, licenças e transformações recebem crédito explícito;
referência conceitual não é código incorporado nem autoria compartilhada.
Para o novo instrumento C++/JUCE, a licença habitual do projeto é GNU AGPLv3 ou
posterior, respeitando separadamente os termos de cada dependência.
