# O que os outros instrumentos RASGO já fazem — mapa para o Trioio

**Criado em 2026-10-14**, a pedido do autor: "é bom que você conheça bem o que
já codamos em outros instrumentos. isso ajudaria muito".

**Escopo.** O mapa reúne o que interessa ao Trioio; ele não substitui a
documentação de cada instrumento. Tudo foi lido sem editar nada. O Rasgo
Synth e o Aquorbium ficam fora do alcance das sessões do Trioio (o autor
trabalha neles com o Codex).

**Regras para trazer qualquer coisa daqui:**

- **Ideias** passam pelo portão conceitual e pelo protocolo de
  reaproveitamento do Método RASGO (`JOGO.md` §10).
- **Código** só entra como cópia fixada, com proveniência, em `third_party/`
  (`GOVERNANCA_E_TRANSVERSALIDADE.md` §7.0).

## 0. O método comum

`RASGO_SYNTH_STUDIO/docs/pt/Metodo_RASGO.md` — "genealogia comum,
personalidades próprias". É a leitura obrigatória antes de qualquer frente
nova.

- **"Mesma genealogia não significa mesma personalidade; diferença de
  instrumento não significa ausência de método comum."**
- **O que cada instrumento decide por si:**
  - a unidade musical;
  - o tempo;
  - se compõe, reage, habita, corta, escuta, negocia ou performa;
  - a agência;
  - os corpos sonoros.
- **"O TRIOIO pode perceber simultaneamente evento, contexto e regra sem
  transformar todo motor em Terceiro Olho."**
- **"Memória e regras podem ser comuns; no Aquorbium aparecem como hábitos de
  um ecossistema, no TRIOIO como estratégia situada e no Synth como política
  de uma composição por seed."**
- **O jogo numa topologia que pode ser rasgada:**
  - jogadas que abrem casas, criam rotas e transformam a regra que as
    permitiu;
  - módulos como força, peça, território, regra, recurso, memória,
    obstáculo, testemunha ou abertura;
  - não é preciso vencedor.
- **O portão conceitual antes do código** e o **protocolo de reaproveitamento**
  em 6 pontos.

## 1. Rasgo Synth Studio — composição integral por seed

**O catálogo de mecanismos:** `docs/pt/_catalogo_mecanismos.md`, 142
mecanismos em 14 categorias.

**O que o Trioio já trouxe** (cópia fixada do commit `0dc468a`,
`third_party/rasgo_synth_dsp`): Gendyn, VOSIM, Walsh, theremin, flauta FM,
scat, chip AY, galo, coro, Rungler, nuvem granular, objeto espectral, o filtro
multibanda e o excitador de sopro.

**Ideias trazidas, reescritas** (`JOGO.md` §10):

- o "Arquivo apócrifo" → "citar a si mesma";
- a "Leitura apócrifa" → as regras que quase entraram.

**Ideias que interessam ao Trioio** (para os próximos passos, cada uma pelo
portão):

| mecanismo do Studio | por que interessa ao Trioio |
|---|---|
| Plano de jogo composicional (GamePlan): 6 estratégias, 6 regras, 5 recursos | o estado do jogo (`JOGO.md` §5); o Studio consultou o Trioio como genealogia, sem copiar personalidades |
| Pesos de Caillois; coesão de álbum por trajetória no simplex | o Studio já faz Caillois; o Trioio não deve repetir: Caillois serve para descrever, não para pesar |
| Agon (disputa de poder entre vozes) | as linhas por frequência como vozes que disputam |
| Sinal de blefe e confiança (BluffSignal) | o apócrifo; a confiança do autor nas regras reveladas |
| Mercado de probabilidades (estado persistido entre peças) | o apodrecer das regras usadas demais |
| Ruptura por excesso de coerência (entropia de Shannon) | um gatilho para o "confunde-te a ti mesmo" |
| Atropelo (a disputa da vez; a frase interrompida lembrada) | a troca de lugar no diálogo |
| Orquestração de agência material (temperamentos: ousadia, persistência, resposta ao silêncio) | as linhas por frequência como agentes, não só filtros |
| Topologia composicional mutável (regiões, passagens, cismo) | o jogo numa topologia rasgável (Método) |
| Arco narrativo por Markov; gramáticas plugáveis (Freytag, ki-shō-ten-ketsu) | a dramaturgia do Trioio, que hoje tem quatro curvas fixas |
| Accelerando rumo ao clímax; ritardando real na coda; morte térmica | a dramaturgia (o tempo, não só a dinâmica e o registro) |
| Heisenberg (o ritmo preciso contra a afinação livre, e vice-versa) | uma regra do jogo possível |
| Nota de rodapé (o gesto fora da escala, uma vez por peça) | uma ação do olho ou do jogo |
| Self-epitaph (texto no WAV com a seed e o percurso) | gravar no WAV do Trioio a proveniência e as regras |
| Humanização rítmica; condução de voz; tratamento de dissonância; graus estáveis nos tempos fortes; modulação real | composição: o groove, a harmonia escrita (auditoria §6) |
| Cadeia de masterização; PCM24 true-peak | o Trioio já tem o próprio master (§ `SINTESE.md`); comparar sem uniformizar |

**Outros documentos do Studio para ler:** `Manifesto.md` (os quatro jogos, o
limite, o álbum apócrifo), `Plano_de_Upgrade_Instrumento_Generativo.md`,
`Palavras_Efemeras_Interface.md`, `Kosmische_Pericia_Tecnica.md`.

## 2. Rasgo Modular — modular de patch, o músico toca

**Identidade:** o músico toca e o instrumento propõe patches como ponto de
partida. Ele não toca sozinho.

**O que o Trioio já trouxe** (`third_party/rasgo_modular_dsp`): as salas HALL e
SPACE, RESONATOR, OPERATOR, LPG.

**Módulos que dialogam com o Trioio:**

| módulo | relação |
|---|---|
| DECISION (a fonte de escolhas a cada pulso) | o acaso das regras |
| TURING (melodias por acaso que se guardam com LOCK) | as regras que "ficam" (aceitas) |
| MEMORY (os últimos 3 s em grãos) | a memória de curto prazo; o "imitar o que acabou de soar" |
| CHAOS, DRIFT (o imprevisível; o movimento de minutos) | a dramaturgia lenta |
| STAGES, FUNCTION (curvas, envelopes) | a curva de tensão |
| LOGIC, SWITCH, MATRIX (combinar, alternar e distribuir sinais) | a topologia: rotas entre linhas |
| SCOPE (medir o som e devolver ao patch) | já mapeado como candidato a receber a escuta do Trioio (resposta ao autor, 2026-10-14) |

**No Atlas de Referência:** §21 "Patching como jogo" e §22 "Informação
imperfeita".

## 3. Aquorbium — um ecossistema, um bioma

**Identidade:** organismos que vivem, comem energia e se reproduzem num bioma.
As regras são hábitos de um ecossistema.

**O que o Trioio já trouxe** (`third_party/aquorbium_dsp`): o ressonador
modal, a corda física e o striker granular.

**Para o Trioio:** a ideia de **hábito** (comportamento que persiste e se
desgasta) é vizinha do "apodrece e vira adubo". Mas o Trioio não deve virar
ecossistema.

## 4. Antitotem — objeto sonoro, hardware CMOS de 2013

**Identidade:** um instrumento de hardware autoral (modular, CMOS e
discreto), transposto para um app: cinco osciladores, um scanner.

**O que o Trioio já trouxe** (`third_party/antitotem_dsp`): a voz CMOS e os
campos de ruído.

## 5. Navalha 2 — corte, gravação, saída

**Identidade:** parte do Navalha de Glerm Soares (2009); recorte e sequência
de fatias na mão de quem toca, gravação e saída (dither TPDF,
masterização).

**Para o Trioio:** o exemplo de *découpe* no dossiê
`docs/REAPROPRIACAO.md`. Fica fora: a resposta do Trioio é só síntese.

## 6. Maravi — transcrição

**Identidade:** o irmão do Trioio na bifurcação de 2026-08-20 (o antigo
"Maravi Tri Oio"); transcrição monofônica (altura, ataques).

**Para o Trioio:** a escuta (`LEITURA_E_GABARITO.md` cita o tempo forte e a
correção de oitava do Maravi como candidatos).

## 7. Em incubação

- **Trucmetry:** um instrumento sobretudo de ritmo; o conceito
  geométrico/rítmico; os metrônomos circulares; a bateria e a percussão
  expressivas. Ainda documental. **Para o Trioio:** os ciclos rítmicos e o
  groove.
- **Risco:** a escrita, os caracteres, o grafismo e a matemática como
  matéria de composição: escrever, riscar, sobrepor, apagar, percorrer,
  reler. Ainda documental. **Para o Trioio:** o "Matema — Dissolve e
  coagula" do arquivo de Vitoriamario e o "riscar" como gesto.

## 8. Próximo

Antes de cada novo passo do jogo, voltar a este mapa e ao Método.

- **O estado do jogo:** ler o GamePlan do Studio e o Mercado de
  probabilidades.
- **O "confunde-te a ti mesmo":** ler a Ruptura por excesso de coerência.
- **A troca de lugar no diálogo:** ler o Atropelo.
