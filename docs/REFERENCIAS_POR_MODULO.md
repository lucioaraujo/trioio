# Referências por módulo

Gerado por `tools/gerar_referencias_por_modulo.py` a partir dos comentários dos fontes. Separa a inspiração conceitual (as fontes externas citadas: teoria, artigos, gravações), as referências internas (os instrumentos RASGO irmãos) e a origem do código. Licença de todo o código: AGPL-3.0-or-later; código de terceiros incorporado: nenhum ([`CREDITS_AND_SOURCES.md`](../CREDITS_AND_SOURCES.md)). Extração heurística: o que não está no comentário do fonte não aparece aqui.

64 módulos.

## `src/core/AcousticVoice`

Corpos de instrumentos acústicos (docs/MAQUINA_DE_TIMBRES.md): modelos físicos escritos para o Trioio a partir da teoria. Não há amostras: a resposta continua sendo só síntese. Pedido do autor (2026-10-14): "precisamos incluir mais sons e timbres, timbres de instrumentos, talvez menos sintéticos".

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (36, as 8 primeiras):
  - guia de onda com a curva de atrito do arco (Smith 1986
  - McIntyre, Schumacher e Woodhouse 1983) e o corpo de violino, viola ou violoncelo pelo registro (Woodhouse 2014) palheta  — clarinete
  - tubo cilíndrico (fechado-aberto) e a palheta como válvula de pressão (Smith 1986) marimba  — barras afinadas 1
  - nas notas longas, vibrafone com o motor (Fletcher e Rossing 1998, cap. 19) piano    — três cordas rígidas (inarmonicidade), o feltro do martelo e o decaimento em dois estágios (Weinreich 1977
  - Chaigne e Askenfelt 1994) violão   — Karplus-Strong estendido (Jaffe e Smith 1983) com o lugar do toque e os modos do tampo (Karjalainen, Välimäki e Tolonen 1998)
  - Giordano, "Simple model of a piano soundboard", JASA 102(2), 1997
  - Suzuki, "Vibration and sound radiation of a piano soundboard", JASA 80(6), 1986).
  - Jaffe e Smith 1983

## `src/core/AntitotemVoice`

A voz CMOS do Antitotem como família de timbre do Trioio. DSP de uma cópia fixada (third_party/antitotem_dsp, AGPL-3.0, mesma autoria). Ver docs/SINTESE.md.

- **código:** próprio
- **referências internas:** Antitotem
- **fontes externas citadas:** nenhuma

## `src/core/AquorbiumVoice`

Corpos do Aquorbium (ressonador modal, corda física, striker granular) como famílias de timbre do Trioio. DSP de uma cópia fixada (third_party/aquorbium_dsp, AGPL-3.0, mesma autoria). Ver docs/SINTESE.md.

- **código:** próprio
- **referências internas:** Aquorbium
- **fontes externas citadas:** nenhuma

## `src/core/BandSplit`

Linhas de referência por região de frequência (docs/LINHAS.md, etapa 1).

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (2):
  - Linkwitz, JAES 24(1), 1976).
  - fração da energia da referência que está nesta linha "grave (< 200 Hz)", "200–2000 Hz", "agudo (> 2000 Hz)"

## `src/core/Composer`

Compositor: escreve uma PARTITURA a partir da escuta da referência. Motivo tirado da referência (ritmo real de um compasso + alturas do campo harmônico), desenvolvido em frases (sentença) e organizado na forma das seções da referência, com recapitulação. Especificação: docs/COMPOSICAO.md.

- **código:** próprio
- **referências internas:** Antitotem, Aquorbium, Rasgo Modular, Rasgo Synth, Studio
- **fontes externas citadas** (1):
  - Abu Shumays, Inside Arabic Music, 2019 (sayr e modulação por ajnas).

## `src/core/Denormals`

Os números desnormalizados (2026-10-18): as caudas dos filtros IIR no silêncio decaem até ~1e-38, e a CPU x86 os processa até 100× mais devagar (a mistura de uma resposta caiu de 12,9 s para 1,1 s, com o mesmo áudio em 24 bits). ScopedNoDenormals liga FTZ e DAZ na thread atual e restaura ao sair. O MXCSR é por thread: cada thread de trabalho cria o seu. Sem SSE (ARM), não faz nada (o áudio em ARM já trata os desnormalizados).

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/Descriptors`

Percepção, primeira camada: descritores mínimos de um sinal mono. Especificação: docs/DESCRITORES.md.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (6):
  - Gómez, IEEE TASLP 2012
  - Klapuri, ISMIR 2006)
  - Fitzgerald, "Harmonic/Percussive Separation using Median Filtering", DAFx 2010)
  - a função de saliência (Salamon e Gómez 2012, eq. 2–3) em `work`
  - cada pico vota em f/h (h = 1..20) com peso α^(h−1), espalhado ±1 semitom por cos² (Salamon e Gómez 2012, eq. 2–3)
  - Altura fundamental por YIN (de Cheveigné e Kawahara, 2002) sobre as últimas 4096 amostras reduzidas a ~12 kHz

## `src/core/Dialogue`

O diálogo com a referência (docs/AUDITORIA_2026-10-14.md §6, item 4, a metade que não muda a proveniência; docs/COMPOSICAO.md "Diálogo"): a resposta deixa de correr em paralelo e passa a conversar — escuta enquanto a referência toca uma frase e responde nas pausas dela.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (1):
  - Floyd Jr., The Power of Black Music, Oxford UP, 1995

## `src/core/Dramaturgy`

A dramaturgia da peça (docs/AUDITORIA_2026-10-14.md §6, item 3; docs/COMPOSICAO.md "Dramaturgia"): uma curva de tensão para a peça inteira que molda a resposta depois de composta — a dinâmica, a densidade das vozes de enfeite e um silêncio composto antes do clímax. Antes, a forma vinha só das seções da referência e cada seção recebia uma operação: havia variedade, mas pouca direção.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (3):
  - Krumhansl, "Modeling tonal tension", Music Perception 24(4), 2007
  - Farbood, "A parametric, temporal model of musical tension", Music Perception 29(4), 2012)
  - Howat, Debussy in Proportion, Cambridge UP, 1983).

## `src/core/EnergySynth`

Síntese energética, primeira versão: um motor simples e próprio, dirigido pela BrainDecision. Especificação: docs/CADEIA_MINIMA.md §3.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/Ensemble`

O conjunto por estilo (docs/MAQUINA_DE_TIMBRES.md §4): o estilo não é um timbre, é uma formação — cada voz do Trioio recebe o instrumento do seu papel naquele estilo. No choro: a flauta no motivo, o bandolim no contracanto, o violão de 7 cordas na baixaria, o cavaquinho na harmonia, o pandeiro na percussão. A extensão de cada instrumento (AcousticVoice.h) põe as notas na oitava que cabe.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/FilmEffects`

Os efeitos de cinema na forma (docs/MAQUINA_DE_TIMBRES.md §3.7). Pedido do autor (2026-10-14): "sons de efeitos sonoros como de filmes". O Trioio não os espalha ao acaso: eles marcam a dramaturgia da resposta, como numa trilha — - o riser sobe nos segundos antes do clímax, e o impacto e o braam caem no clímax; - o whoosh passa nas mudanças de seção (no máximo um a cada 20 s); - nos trechos rarefeitos (tensão baixa por 8 s ou mais), uma ambiência pelo brilho da referência: escura → vento ou fogo; média → água ou cidade; clara → chuva (às vezes com um trovão) ou multidão. Os efeitos são gestos (…

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (1):
  - Os efeitos são gestos (objetos sonoros), sintetizados por processo em ScoreSynth.cpp (Farnell, Designing Sound, 2010).

## `src/core/Game`

O jogo (docs/JOGO.md, §2–§4 e §7): o Trioio inventa as próprias regras e as joga sobre a resposta composta. Uma regra tem a forma comum (GameRule em Composer.h): quando <condição lida na referência>, faça <ação> sobre <material>, por <alcance>.

- **código:** próprio
- **referências internas:** Rasgo Synth, Studio
- **fontes externas citadas:** nenhuma

## `src/core/Groove`

O groove da referência (docs/AUDITORIA_2026-10-14.md §6, item 2; docs/RITMO.md "Groove e microtempo"): a resposta deixa de ser uma grade quantizada e toca com o balanço medido nos ataques da referência.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (3):
  - jazz eighth-notes as expressive microrhythm", Ethnomusicology 50(1), 2006)
  - Danielsen, Musical Rhythm in the Age of Digital Reproduction, Ashgate, 2010
  - Berdahl, "Towards machine learning of expressive microtiming in Brazilian drumming", ICMC 2006).

## `src/core/Harmony`

Harmonia em movimento (docs/HARMONIA.md; camada 1 de docs/MUSICALIDADE.md).

- **código:** próprio
- **referências internas:** Rasgo Synth, Studio
- **fontes externas citadas** (4):
  - 2. o melhor caminho pelos 36 acordes (Viterbi, como no reconhecimento de acordes por HMM de Sheh e Ellis, 2003)
  - 2b. a variante da escala sob cada acorde (chord-scale, Russell 1953)
  - meio tom sobre a terça maior, o retardo 4–3) toma ~40 % do valor, acentuada, e resolve por grau na nota do acorde, mais suave (Piston 1987, cap. 9
  - a nota do acorde anterior que está um grau acima desta, e não pertence ao acorde novo, fica presa no começo dele (a dissonância) e desce para resolver — o 4–3, o 9–8 da escrita coral (Piston 1987, cap. 9)

## `src/core/HarmonyWriting`

A harmonia escrita (2026-10-17; docs/HARMONIA.md, "Harmonia escrita"; auditoria de 14/10, item 6). Até aqui a harmonia era LIDA na referência e transformada pelo modo; a função harmônica não era escrita. Esta camada, nos 12 sons, entre a leitura e a aplicação (o baixo, o motivo e a voz de harmonia seguem o que for escrito): 1. a TONALIDADE de cada seção, pelos perfis de Krumhansl e Kessler ("Tracing the dynamic changes in perceived tonal organization in a spatial representation of musical keys", Psychological Review 89(4), 1982) sobre os acordes da seção, pesados pela duração; 2. as CADÊNCIAS …

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (10, as 8 primeiras):
  - 1. a TONALIDADE de cada seção, pelos perfis de Krumhansl e Kessler ("Tracing the dynamic changes in perceived tonal organization in a spatial representation of musical keys", Psychological Review 89(4), 1982) sobre os…
  - Piston, Harmony, 5ª ed., Norton, 1987 (as cadências, as dominantes secundárias, a modulação)
  - Schoenberg, Structural Functions of Harmony, 1954.
  - o baixo (a fundamental) e três vozes de cima, numa busca entre todas as disposições (as permutações das notas do acorde e as oitavas de cada voz, entre Dó 3 e Fá 5), pontuadas pelas regras clássicas (Piston 1987, cap.…
  - a derivation of the rules of voice-leading from perceptual principles", Music Perception 19(1), 2001)
  - i–♭VII–♭VI–V (Manuel 2002)
  - Partch (Genesis of a Music, 1949)
  - Egberto Gismonti ("Água e vinho", 1972

## `src/core/Intent`

A intenção (docs/BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md, "prompts.json"; docs/PROMPTS_E_INTENCOES.md): três camadas que nunca se substituem.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/Lines`

As linhas por região de frequência na composição (docs/LINHAS.md).

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (1):
  - confiança ≥ 0,25 e ≥ 0,5 ataque/s o pulso no nível da peça (ou metade/dobro), mesmo fraco cobre parte de 400–2000 Hz e ≥ 25 % dos quadros com altura cobre parte de 300–1500 Hz abaixo de 300 Hz

## `src/core/Listening`

A escuta do autor como memória do instrumento (docs/TRIOIO_CEREBRO.md §10: "adaptação local, não treinamento"; "alterar tendências, não aprisionar o instrumento em um perfil de gosto"). Cada julgamento — aceitar, rejeitar ou transformar uma resposta — guarda o que aquela resposta usou: as famílias de timbre, as escolhas do Terceiro Olho e a sala. O histórico vira TENDÊNCIAS: multiplicadores nos sorteios do compositor, do olho e da sala, limitados a 0,5..2 e com os julgamentos antigos pesando cada vez menos (esquecimento gradual). Sem julgamentos, tudo é 1 e a composição é idêntica à de antes. …

- **código:** próprio
- **referências internas:** Rasgo Modular
- **fontes externas citadas:** nenhuma

## `src/core/Mashup`

Mashup de materiais (docs/COMPOSICAO.md, "Mashup"; estudo transversal §6.13): duas referências compostas juntas. A melodia, a forma e o olho nascem da referência A; da referência B podem vir: harmonia   os acordes lidos em B (a melodia de A se curva a eles: o mashup clássico, a voz de uma canção sobre o instrumental de outra) ritmo      o groove de B (o campo rítmico dos compassos de B) memória    os fragmentos recombinados pelo oráculo e pelo mosaico corpo      o corpo espectral (a família espectral toca com os parciais de B) B é reescalada no tempo para a duração de A (estica ou encolhe por …

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/Master`

O barramento de master da resposta (docs/AUDITORIA_2026-10-14.md §6, item 1; docs/SINTESE.md "Master"): o que faltava entre a soma das vozes e o WAV.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (2):
  - passa-altas 30 Hz (linear) → compressão de cola (2:1, joelho suave) → ganho para o alvo de loudness (ITU-R BS.1770
  - Loudness integrada (LUFS) de um estéreo, BS.1770-4.

## `src/core/ModularRasgo`

O rasgo modular (docs/MAQUINA_DE_TIMBRES.md §5.0). A escuta do autor (2026-10-15), depois de ouvir os timbres e os estilos: "ainda muito midi — se possível trazer mais da síntese modular criativa para dentro dos timbres, sons inauditos, não protocolares". O rumo deixa de ser o realismo: o corpo acústico é o PONTO DE PARTIDA, e o Trioio o atravessa com processos de síntese modular — cada voz, em cada seção, recebe um "patch", com a profundidade movida pela tensão da peça: dobra       — wavefolder movido pelo envelope do próprio som (o West Coast: Buchla 259; o SHAPE do Rasgo Modular) anel      …

- **código:** próprio
- **referências internas:** Rasgo Modular, Rasgo Synth
- **fontes externas citadas:** nenhuma

## `src/core/ModularVoice`

Corpos de voz do Rasgo Modular (AGPL-3.0, mesma autoria e licença do Trioio), a partir de uma CÓPIA fixada em third_party/rasgo_modular_dsp (commit e checksums em PROVENANCE.md): o Trioio é autônomo. Cada família de timbre hospedada é uma pequena cadeia de módulos reais do Rasgo Modular — fonte e, às vezes, um processador —, tocada nota a nota, offline. O Trioio é o SEGUNDO consumidor desse DSP (regra do inventário global: segundo consumidor antes de promover a código comum). Especificação: docs/SINTESE.md.

- **código:** próprio
- **referências internas:** Antitotem, Rasgo Modular
- **fontes externas citadas:** nenhuma

## `src/core/Mosaic`

Memória pesquisável por timbre (Atlas de Referência RASGO §62, CataRT / MuBu, IRCAM: síntese concatenativa por corpus — "a memória pode ser pesquisável").

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (2):
  - Schwarz, "Corpus-based concatenative synthesis", IEEE Signal Processing Magazine 24(2), 2007
  - Schwarz et al., "Real-time corpus-based concatenative synthesis with CataRT", DAFx 2006.

## `src/core/OracleMemory`

Memória performativa (Atlas de Referência RASGO §63, OMax/IRCAM: "o sistema escuta, aprende, reorganiza, responde"; aplicações: "repetir deformado", "reconstruir", "gerar memória falsa").

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (2):
  - Dubnov, "Using factor oracles for machine improvisation", Soft Computing 8, 2004 (a base do OMax).
  - Construção incremental (Allauzen, Crochemore, Raffinot 1999)

## `src/core/Production`

A produção (docs/MAQUINA_DE_TIMBRES.md §6): a mistura como um engenheiro a faria. A escuta do autor (2026-10-16): "está muito amador, precisamos de sonoridade mais sofisticada, amadurecida, profissional […] para que gere emoção". Antes, todas as vozes somavam num par estéreo e passavam pela mesma reverberação de 4 linhas com entrada mono, sem equalização nem profundidade. Agora: 1. BARRAMENTOS por papel (melodia, baixo, harmonia e camadas, percussão, efeitos), cada um processado à parte — o passa-altas que limpa, o corte da "lama" (300 Hz) na harmonia, a presência (3 kHz) na melodia, a compres…

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (7):
  - Chaigne, "Digital delay networks for designing artificial reverberators", AES 90, 1991), difusão de entrada por passa-tudos e linhas levemente moduladas (J.
  - Izhaki, Mixing Audio, 3ª ed., Routledge, 2018
  - Owsinski, The Mixing Engineer's Handbook, 4ª ed., 2017.
  - Hennig et al., "The nature and perception of fluctuations in human musical rhythms", PLoS ONE 6(10), 2011
  - Todd, "The dynamics of dynamics", JASA 91(6), 1992)
  - Friberg et al., "Overview of the KTH rule system for musical performance", Advances in Cognitive Psychology 2(2–3), 2006)
  - um ritardando nos últimos ~6 s (o tempo alarga até ~25 %, a curva do "final ritard" de Friberg e Sundberg, "Does music performance allude to locomotion?", JASA 105(3), 1999) e a fermata no último acorde.

## `src/core/Provenance`

Manifesto de proveniência do TRIOIO, versão 1 — o modelo em memória e as regras que decidem o que uma referência pode fazer dentro do instrumento. Especificação: docs/MANIFESTO_DE_PROVENIENCIA.md.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/RasgoSynthVoice`

Vozes do Rasgo Synth como famílias de timbre do Trioio. O DSP vem de uma cópia fixada (third_party/rasgo_synth_dsp, commit 0dc468a, GPL-3.0, mesma autoria), não da pasta viva do Rasgo Synth. Especificação: docs/SINTESE.md.

- **código:** próprio
- **referências internas:** Rasgo Synth
- **fontes externas citadas:** nenhuma

## `src/core/Reading`

A leitura de uma referência e a sua avaliação contra um gabarito do autor (docs/LEITURA_E_GABARITO.md). Mede, não decide.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (5):
  - Kessler, Psychological Review 89, 1982
  - Krumhansl, Cognitive Foundations of Musical Pitch, Oxford UP, 1990), correlacionados com o croma médio da peça. - Leitura
  - Raffel et al., "mir_eval", ISMIR 2014), reimplementadas aqui
  - Dixon, JNMR 36, 2007), tonalidade com acerto parcial para quinta, relativa e homônima, acordes pela fração do tempo, fronteiras de seção com ±3 s.
  - Krumhansl e Kessler (1982), perfis de tonalidade de Dó maior e Dó menor

## `src/core/Recombination`

Recombinação de culturas, linguagens e estilos (docs/ESCALAS.md, "Misturas"; RASGO_DOCUMENTATION/teoria/ESCALAS_AFETO_INTENCAO_E_FUNCAO.md, §6.13). Quatro técnicas, cada uma com raiz numa prática documentada:

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (1):
  - Berio, Sinfonia, 1968, 3.º movimento

## `src/core/RhythmCycles`

Os ciclos rítmicos de cada cultura (docs/RITMO.md, "Ciclos das culturas"; data/ritmos/arquivo_trioio_ritmos.txt, gerado por tools/gerar_arquivo_de_ritmos.py): iqa' árabes, tala hindustani e carnático, usul turco e aksak, claves e linhas-guia, a colotomia do gamelão, o compás do flamenco e o shuffle. Cada ciclo cita a fonte.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (1):
  - Clayton, Time in Indian Music, 2000).

## `src/core/RhythmField`

Ritmo como campo (Atlas de Referência RASGO §45, Grids: "pensar ritmo não como uma sequência fixa de passos, mas como navegação em um campo de padrões e densidades": campo → posição → densidade → probabilidade → evento).

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/ScaleAffect`

Afeto e intenção das escalas (RASGO_DOCUMENTATION/teoria/ ESCALAS_AFETO_INTENCAO_E_FUNCAO.md, §4–5; TRIOIO/docs/ESCALAS.md).

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (3):
  - a afetividade vem da peça inteira, e muito dela é aprendido (Temperley e Tan 2013
  - graus mais altos, mais "alegre" (Temperley e Tan 2013) terça           +1 maior, −1 menor, 0 neutra ou ausente (Hevner 1936) tensão          semitons vizinhos, trítonos, b2, trítono sobre a tônica, quinta ausente (Hur…
  - Lerdahl 2001), 0..1 estabilidade    quinta, quarta e terça sobre a tônica, 0..1 ambiguidade     parte dos graus a mais de 30 c do temperamento igual familiaridade   notas comuns com o jônio na mesma tônica, 0..1 valên…

## `src/core/ScaleArchive`

O arquivo de escalas do Trioio (docs/ESCALAS.md, "Arquivo de escalas").

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (2):
  - os modelos de maqam (Cairo, 1932) e de makam (Arel–Ezgi–Uzdilek)
  - Werckmeister, Musicalische Temperatur, 1691" "modelo…", "aproximação…" (pode ser vazio)

## `src/core/ScaleBank`

O banco de escalas do Trioio (docs/ESCALAS.md), adaptado do Rasgo Synth Studio (cópia fixada em third_party/rasgo_synth_dsp/sequencer/, com proveniência):

- **código:** adaptado (ver o comentário do fonte)
- **referências internas:** Rasgo Synth, Studio
- **fontes externas citadas:** nenhuma

## `src/core/ScaleIdiom`

A escala tocada como a linguagem a usa (RASGO_DOCUMENTATION/teoria/ ESCALAS_AFETO_INTENCAO_E_FUNCAO.md, §6; TRIOIO/docs/ESCALAS.md, "Comportamento").

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (3):
  - Racy, Making Music in the Arab World, 2003
  - dois acordes do modo — tríades dos graus de `vamp` — alternando a cada `barLength` segundos sobre um pedal na tônica (voz 6 e baixo), sem a função tonal (Persichetti 1961
  - Levine 1995).

## `src/core/ScaleMachine`

A máquina de escalas (docs/ESCALAS.md, "A máquina de escalas").

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (1):
  - Russell, The Lydian Chromatic Concept of Tonal Organization, 1953)

## `src/core/ScoreSynth`

Toca uma partitura (Composer) offline: vozes aditivas com timbre que evolui e uma sala simples. Especificação: docs/COMPOSICAO.md §"Síntese".

- **código:** próprio
- **referências internas:** Antitotem, Aquorbium, Rasgo Modular, Rasgo Synth
- **fontes externas citadas** (3):
  - Farnell, Designing Sound, MIT Press, 2010
  - K. van den Doel, "Physically based models for liquid sounds", ACM TAP 2(4), 2005
  - Mag. 16, 1933).

## `src/core/ShortMemory`

A janela temporal e a memória curta (TRIOIO_TAREFAS, Marco 2; docs/ BIBLIOTECA_MEMORIA_E_PROVENIENCIA.md, "RESPONDER AGORA"): os últimos N segundos da escuta como DESCRITORES (os quadros e os ataques), nunca o áudio. O buffer é um anel pré-alocado: push() não aloca nem trava, e a memória não cresce com a duração da escuta. É a base da escuta ao vivo: o "o que acabou de soar" que o diálogo e o Terceiro Olho leem.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/SomDaAura`

O "som da aura" (2026-10-17; docs/MAQUINA_DE_HARMONIA.md §7). Hermeto Pascoal chama assim a fala ouvida como melodia: ele transcreve a entonação de quem fala, sílaba por sílaba, dobra essa melodia com os instrumentos e harmoniza cada nota ("a vibração sonora da alma de cada pessoa, refletida na sua fala"; L. Costa-Lima Neto, "The first 'Sound of the Aura' of Hermeto Pascoal (1984): hearing the spoken voice as a sung melody", IASPM Journal; o disco Lagoa da Canoa, Município de Arapiraca, 1984).

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (2):
  - Costa-Lima Neto, "The first 'Sound of the Aura' of Hermeto Pascoal (1984)
  - o disco Lagoa da Canoa, Município de Arapiraca, 1984).

## `src/core/Strategy`

Estratégia, primeira versão: transforma a escuta de uma referência (resumos por trecho) numa trajetória de BrainDecision, segundo um dos quatro modos do Terceiro Olho. Especificação: docs/CADEIA_MINIMA.md.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/Structure`

Escuta da estrutura: pulso, grade rítmica, campo harmônico e seções de uma referência, a partir dos quadros e ataques de Descriptors. É o material do compositor (docs/COMPOSICAO.md). Mede, não decide.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (7):
  - Parncutt 1994, London 2004
  - Plumbley, "A spectral difference approach to downbeat extraction in musical audio", EUSIPCO 2006
  - Papadopoulos, IEEE TASLP 2011.
  - Foote, "Automatic audio segmentation using a measure of audio novelty", ICME 2000)
  - Müller, Fundamentals of Music Processing, Springer, 2015, cap. 4). `labels[k]` é a letra da seção k (0 = A, 1 = B…). a semelhança entre os segmentos da última chamada (z médio na diagonal), para diagnóstico e calibração
  - Parncutt, "A perceptual model of pulse salience and metrical accent in musical rhythms", Music Perception 11(4), 1994
  - London, Hearing in Time, 2004).

## `src/core/StyleFigures`

As figuras de cada estilo (docs/MAQUINA_DE_TIMBRES.md §4.2): o conjunto troca os corpos; as figuras trocam o JEITO de tocar. A harmonia (voz 6) e o baixo (voz 1) que o Trioio compôs — os acordes sustentados e as fundamentais — são rearticulados no compasso e no pulso lidos na referência, só nos trechos com pulso:

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (2):
  - Levine, The Jazz Piano Book, Sher Music, 1989 (as disposições A/B sem fundamental)
  - Dregni, Django, 2004.

## `src/core/ThirdEye`

O Terceiro Olho: o agente que percebe a estrutura da peça e a transforma. Age DEPOIS do compositor, sobre a partitura pronta: em momentos sorteados (mais prováveis onde a referência muda mais) faz irromper objetos sonoros ou impõe regras ocultas por um tempo. Especificação: docs/COMPOSICAO.md §"Terceiro Olho".

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas** (1):
  - Pedido do roteiro (TRIOIO_TAREFAS, retomada de 6 out. 2026)

## `src/core/TrioioBrain`

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/Versions`

A comparação entre versões (TRIOIO_TAREFAS, Marco 2: "invariantes, divergências e operações possíveis"; docs/VERSOES.md): várias gerações do mesmo prompt, várias gravações da mesma peça, as tomadas de uma sessão. O que não muda entre elas é o que a peça "é"; o que muda é o espaço onde o Trioio pode jogar. Lê o som (a escuta de cada uma), nunca um parâmetro interno de um gerador.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/core/Voicing`

Voicings: como empilhar o acorde (docs/HARMONIA.md, "Voicings"). Reescrito no Trioio a partir dos princípios do MelodyVoicingBank do Rasgo Synth Studio (o "Eixo B" da flauta; RASGO_SYNTH/rasgo-synth-core/src/dsp/ MelodyVoicingBank.hpp, commit babe6e8), estendidos para respeitar a escala da seção (inclusive microtonal):

- **código:** próprio
- **referências internas:** Studio
- **fontes externas citadas** (3):
  - o acorde de "So What" (Bill Evans e Miles Davis, Kind of Blue, 1959) planing    a forma do primeiro acorde movida em paralelo, sem condução e sem função (Debussy, "Voiles", 1909) bitonal    duas tríades a um trítono (…
  - duas tônicas ao mesmo tempo (Stravinsky, Petrushka, 1911
  - Cowell, New Musical Resources, 1930

## `src/core/Constraints`

As regras do autor em texto simples (docs/REGRAS.md). Uma por linha; '#' começa comentário; acentos opcionais nas palavras-chave.

- **código:** próprio
- **referências internas:** Rasgo Modular
- **fontes externas citadas:** nenhuma

## `src/library/Analysis`

Análise de um material registrado: lê o áudio em blocos, mede os descritores (perception::Analyzer) e guarda o resultado em cache ligado ao conteúdo. Especificação: docs/DESCRITORES.md.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/library/AudioSource`

Leitura de áudio em vários formatos (docs/FORMATOS_DE_AUDIO.md).

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/library/DataFiles`

Os arquivos de dados do Trioio (data/: escalas, ritmos, cadências) num binário instalado em outra máquina (2026-10-18: antes, só pelo caminho do código-fonte embutido na compilação). A busca, na ordem: 1. TRIOIO_DATA_DIR (a pasta que contém escalas/, ritmos/, harmonia/); 2. data/ ao lado do executável (Windows, o tar.gz, o build); 3. ../Resources/data (o .app do macOS); 4. ../share/trioio/data (a instalação Linux, o AppImage); 5. o código-fonte (TRIOIO_SOURCE_DIR/data), no desenvolvimento. Devolve o primeiro que existe; se nenhum, o do código-fonte (para a mensagem de erro dizer onde procurou)…

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/library/Declaration`

Declaração do performer sobre um material registrado: origem, fundamento e usos liberados — ou recusa/revogação. É a única forma prevista de um manifesto sair de `pending`. Especificação: docs/MANIFESTO_DE_PROVENIENCIA.md.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/library/LibraryImporter`

Importador da Biblioteca: varre uma raiz escolhida pelo performer e mantém um manifesto (trioio.manifesto/1) por material encontrado. Regras: docs/IMPORTADOR_DA_BIBLIOTECA.md.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/library/ListeningStore`

A escuta do autor gravada na raiz de dados (escuta.json). O cálculo das tendências é do núcleo (core/Listening.h); aqui só se lê e grava. Especificação: docs/ESCUTA.md.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/library/ManifestJson`

Leitura e escrita de `manifesto.json` (esquema trioio.manifesto/1). Especificação: docs/MANIFESTO_DE_PROVENIENCIA.md §4.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/library/Prompts`

O prompts.json de cada referência (docs/PROMPTS_E_INTENCOES.md; esquema trioio.prompts/1, em <dados>/indice/prompts/<ref>.json): as três camadas da intenção (core/Intent.h), separadas e nunca substituídas.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/library/Response`

A cadeia mínima audível, offline: referência autorizada → escuta → estratégia → síntese → WAV + decisões + manifesto da derivação → escuta da própria resposta. Especificação: docs/CADEIA_MINIMA.md.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/library/Retention`

A retenção das sessões (TRIOIO_TAREFAS, Marco 2: "política de retenção e limpeza ..., sempre reversível pelo performer"; docs/ESTRUTURA_DO_PROJETO.md):

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/app/AppModel`

O que a interface mostra, lido da raiz de dados que a CLI já grava (manifestos, cache de análise, decisoes.json, escuta.json). A interface não guarda estado musical próprio (docs/design.md §7).

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/app/MainView`

A tela do Trioio (docs/design.md): "Escuta e resposta" com as três faixas (referência, Terceiro Olho, resposta), a Biblioteca, a Linguagem e a barra de julgamento, nos regimes completo, compacto e emergência. Cada regime recalcula as áreas (sem zoom); alvos de pelo menos 44 px.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/app/UiLanguage`

Os quatro idiomas da família RASGO (a mesma convenção do Antitotem, do Navalha 2 e do Rasgo Modular): inglês, português, francês e espanhol. Na primeira abertura, o idioma do sistema (qualquer outro cai no inglês); depois, o escolhido no botão EN/PT/FR/ES. Todo texto da interface vem daqui. As marcas da forma que o compositor escreve (as seções, o Terceiro Olho) ficam em português: são o registro do núcleo, não a interface.

- **código:** próprio
- **referências internas:** Antitotem, Aquorbium, Navalha, Rasgo Modular, Rasgo Synth
- **fontes externas citadas:** nenhuma

## `src/app/WindowPolicy`

A política de janela do Trioio (docs/design.md §5; contrato responsivo de RASGO_DOCUMENTATION/design/INTERFACES_E_LAYOUTS.md §5.1). Pura e sem JUCE, para ser testada na suíte. Escrita para o Trioio (o utilitário comum candidato é o do AQUORBIUM, que só será copiado depois de G4).

- **código:** próprio
- **referências internas:** AQUORBIUM
- **fontes externas citadas:** nenhuma

## `src/app/Main`

O aplicativo do Trioio (docs/design.md): a janela pelo contrato comum (WindowPolicy.h), o transporte de áudio, e o modo de instantâneos que desenha a tela nas superfícies da matriz sem abrir janela:

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

## `src/tools/BibliotecaCli`

trioio_biblioteca — o importador da Biblioteca na linha de comando.

- **código:** próprio
- **referências internas:** nenhuma
- **fontes externas citadas:** nenhuma

