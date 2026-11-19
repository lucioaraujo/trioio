"""Textos das páginas Tutorial, Aprender e Instalação do site do Trioio.

Nivelamento dos sites da família (autor, 19/11/2026: "nivele pelos mais
completos"). O Tutorial vem do próprio app (src/app/Tutorial.h), lido pelo
gerar_paginas.py. O Aprender segue docs/COMO_TOCAR.md e a Instalação segue
INSTALL.md; os nomes dos botões em cada língua são os de src/app/UiLanguage.h.

Marcação mínima nos textos: **negrito** e `código`. Blocos: ("p", texto),
("ul", [itens]), ("ol", [itens]), ("tab", [(coluna 1, coluna 2), ...]),
("code", texto).
"""

LANGS = ("pt-BR", "en", "fr", "es")

NAV = {
    "pt-BR": {"sobre": "Sobre", "tutorial": "Tutorial", "aprender": "Aprender", "instalacao": "Instalação", "baixar": "Baixar", "menu": "Páginas"},
    "en": {"sobre": "About", "tutorial": "Tutorial", "aprender": "Learn", "instalacao": "Installation", "baixar": "Download", "menu": "Pages"},
    "fr": {"sobre": "À propos", "tutorial": "Tutoriel", "aprender": "Apprendre", "instalacao": "Installation", "baixar": "Télécharger", "menu": "Pages"},
    "es": {"sobre": "Acerca de", "tutorial": "Tutorial", "aprender": "Aprender", "instalacao": "Instalación", "baixar": "Descargar", "menu": "Páginas"},
}

TUTORIAL = {
    "pt-BR": {"title": "Tutorial — Trioio", "h1": "Tutorial",
              "lead": "Os onze capítulos do tutorial que o Trioio traz dentro da janela (botão TUTORIAL), do primeiro arquivo ao primeiro exercício.",
              "desc": "Tutorial do Trioio: trazer e autorizar uma gravação, escolher o modo, compor, ouvir, julgar e comparar versões.",
              "contents": "Sumário", "tip": "Dica"},
    "en": {"title": "Tutorial — Trioio", "h1": "Tutorial",
           "lead": "The eleven chapters of the tutorial Trioio carries inside its window (TUTORIAL button), from the first file to the first exercise.",
           "desc": "Trioio tutorial: bring and authorise a recording, choose the mode, compose, listen, judge and compare versions.",
           "contents": "Contents", "tip": "Tip"},
    "fr": {"title": "Tutoriel — Trioio", "h1": "Tutoriel",
           "lead": "Les onze chapitres du tutoriel que Trioio porte dans sa fenêtre (bouton TUTORIEL), du premier fichier au premier exercice.",
           "desc": "Tutoriel de Trioio : apporter et autoriser un enregistrement, choisir le mode, composer, écouter, juger et comparer les versions.",
           "contents": "Sommaire", "tip": "Astuce"},
    "es": {"title": "Tutorial — Trioio", "h1": "Tutorial",
           "lead": "Los once capítulos del tutorial que Trioio lleva dentro de su ventana (botón TUTORIAL), del primer archivo al primer ejercicio.",
           "desc": "Tutorial de Trioio: traer y autorizar una grabación, elegir el modo, componer, escuchar, juzgar y comparar versiones.",
           "contents": "Sumario", "tip": "Consejo"},
}

APRENDER = {
    "pt-BR": {
        "title": "Aprender — Trioio", "h1": "Aprender",
        "lead": "Cada parte da janela do Trioio e o que ela faz. Para tocar a primeira vez, siga o Tutorial.",
        "desc": "As abas, as faixas, os modos, o painel Linguagem, o julgamento, a comparação de versões e os stems do Trioio.",
        "secoes": [
            ("As três abas", [
                ("tab", [("**Escuta e resposta**", "onde se toca: a referência em cima, o Terceiro Olho e a resposta embaixo; os modos, o play, o master e o julgamento"),
                         ("**Biblioteca**", "as gravações: trazer, autorizar e escolher. Numa janela larga, ela fica sempre à esquerda"),
                         ("**Sua escuta**", "as versões compostas, a comparação A × B, o que os seus julgamentos ensinaram e as regras do jogo")]),
            ]),
            ("As três faixas", [
                ("ul", ["**REFERÊNCIA — o que soa**: a gravação que você trouxe, com as seções que o Trioio leu nela. **▶ referência** toca e, na primeira vez, analisa.",
                        "**TERCEIRO OLHO**: o que o instrumento decidiu por conta própria. Fica escondido até você ouvir; **revelar depois de ouvir** mostra o que ele fez.",
                        "**RESPOSTA — o que o Trioio compõe**: o arquivo novo, todo sintetizado, em blocos por seção."]),
            ]),
            ("Os modos", [
                ("p", "O modo é a atitude da resposta diante da referência."),
                ("tab", [("**seguir**", "acompanha: mesmo campo, mesmo pulso"),
                         ("**transfigurar**", "transforma: a mesma matéria, outra figura"),
                         ("**contrapor**", "vai contra: outro registro, outra densidade"),
                         ("**interromper**", "corta, entra e sai")]),
            ]),
            ("O painel Linguagem ▸", [
                ("ul", ["a **mistura** de escalas, por exemplo rāga Yaman + maqam Hijaz;",
                        "as **alturas**, a **técnica**, a **costura**, o **ciclo**, o **voicing**, a **intenção**, a **forma** e o **andamento**;",
                        "a **imprevisibilidade**, que é o Terceiro Olho;",
                        "o **conjunto**: o estilo da formação, que dá a cada voz o instrumento e o fraseado do estilo. **automático** deixa o Trioio escolher os timbres; **lido na referência** escolhe o estilo pelo que ele escutou, e o modo responde; e há os estilos: choro, samba, escola de samba, jazz, manouche, bolero, salsa, charanga, forró, tango, rock, câmara, orquestra e a cappella;",
                        "**efeitos de cinema na dramaturgia**: riser, impacto e braam no clímax, whoosh nas seções, ambiências nos trechos calmos;",
                        "**diálogo: responder nas pausas da referência**;",
                        "**jogo: o Trioio inventa as próprias regras**;",
                        "**stems: também um WAV por voz (para a DAW)**."]),
                ("p", "Deixar tudo como está também é uma escolha: o Trioio decide a partir da escuta."),
            ]),
            ("Ouvir e guardar", [
                ("ul", ["**▶** toca a resposta; **▶ juntos** toca a referência e a resposta ao mesmo tempo, para ouvir a conversa.",
                        "O **master** regula o volume de tudo.",
                        "**arquivo ▾**, junto da resposta: **mostrar na pasta**; **exportar…** copia o WAV, o MIDI, os stems e as decisões para a pasta que você escolher, numa subpasta com o nome da sessão; **mover para a lixeira…** guarda a sessão em `lixeira/`, de onde ela pode voltar."]),
            ]),
            ("Julgar", [
                ("tab", [("clicar fora da seleção", "julga a resposta inteira"),
                         ("clicar num bloco da resposta", "julga aquela seção"),
                         ("arrastar sobre a resposta", "julga o trecho marcado")]),
                ("p", "Escreva uma **nota**, se quiser: a sua frase sobre aquela escuta fica gravada com o julgamento e aparece na lista de versões. Depois clique em **aceitar**, **transformar** ou **rejeitar**. O julgamento pesa nas próximas respostas: as aceitas ficam na memória (até 8) e as rejeitadas viram adubo. Vale também para as regras do jogo."),
            ]),
            ("Comparar versões (Sua escuta)", [
                ("ol", ["Clique numa versão da lista **Versões sobre esta referência**: ela é o **A**.",
                        "Clique em outra: ela vira o novo **A**, e a anterior vira o **B**.",
                        "**▶ comparar A × B** toca as duas; **alternar A ↔ B** troca de versão no mesmo instante da música."]),
                ("p", "Ao lado ficam **O que a escuta ensinou** (o quanto cada escolha subiu ou desceu com os seus julgamentos) e **Regras do jogo na versão A**."),
            ]),
            ("Os stems", [
                ("p", "Com a caixa de stems ligada, a resposta grava, ao lado da mistura, um WAV estéreo de 24 bits por voz que soou, em `sessoes/<sessão>/saidas/stems/`:"),
                ("ul", ["`1-motivo`, `2-baixo`, `3-eco`, `4-memoria`, `5-percussao`, `6-mosaico`, `7-harmonia`: cada voz já processada, seca;",
                        "`8-gestos`: os gestos do Terceiro Olho e os efeitos de cinema;",
                        "`9-sala`: a reverberação de todas as vozes."]),
                ("p", "Cada stem soa só onde a sua voz toca e fica no nível em que entra na mistura. Somados em 0 dB na DAW, eles refazem a mistura. Para ouvir a peça, use a mistura."),
            ]),
            ("Onde ficam os dados", [
                ("p", "Gravações, respostas, julgamentos e memória ficam na **pasta de dados** que você escolhe em **abrir pasta de dados…**; o Trioio lembra dela na próxima vez. O Trioio compõe sem internet: só a caixa de consulta de palavras na rede usa serviços públicos, e ela vem desligada."),
            ]),
        ],
    },
    "en": {
        "title": "Learn — Trioio", "h1": "Learn",
        "lead": "Every part of Trioio's window and what it does. To play for the first time, follow the Tutorial.",
        "desc": "Trioio's tabs, lanes, modes, Language panel, judging, version comparison and stems.",
        "secoes": [
            ("The three tabs", [
                ("tab", [("**Listening and response**", "where you play: the reference on top, the Third Eye and the response below; the modes, play, master and judging"),
                         ("**Library**", "the recordings: bring, authorise and choose. In a wide window it stays on the left"),
                         ("**Your listening**", "the composed versions, the A × B comparison, what your judgements taught and the rules of the game")]),
            ]),
            ("The three lanes", [
                ("ul", ["**REFERENCE — what sounds**: the recording you brought, with the sections Trioio read in it. **▶ reference** plays it and, the first time, analyses it.",
                        "**THIRD EYE**: what the instrument decided on its own. It stays hidden until you listen; **reveal after listening** shows what it did.",
                        "**RESPONSE**: the new file Trioio composes, entirely synthesised, in blocks by section."]),
            ]),
            ("The modes", [
                ("p", "The mode is the response's attitude towards the reference."),
                ("tab", [("**follow**", "accompanies: same field, same pulse"),
                         ("**transfigure**", "transforms: the same matter, another figure"),
                         ("**oppose**", "goes against: another register, another density"),
                         ("**interrupt**", "cuts, comes in and out")]),
            ]),
            ("The Language ▸ panel", [
                ("ul", ["the **mixture** of scales, for example rāga Yaman + maqam Hijaz;",
                        "the **pitches**, the **technique**, the **seam**, the **cycle**, the **voicing**, the **intention**, the **form** and the **tempo**;",
                        "the **unpredictability**, which is the Third Eye;",
                        "the **ensemble**: the style of the band, which gives each voice the instrument and phrasing of the style. **automatic** lets Trioio choose the timbres; **read in the reference** chooses the style from what it heard, and the mode answers; and there are the styles: choro, samba, samba school, jazz, manouche, bolero, salsa, charanga, forró, tango, rock, chamber, orchestra and a cappella;",
                        "**film effects in the drama**: riser, impact and braam at the climax, whoosh at the sections, ambiences in calm passages;",
                        "**dialogue: answer in the reference's pauses**;",
                        "**game: Trioio invents its own rules**;",
                        "**stems: one WAV per voice too (for the DAW)**."]),
                ("p", "Leaving everything as it is is also a choice: Trioio then decides from what it hears."),
            ]),
            ("Listen and keep", [
                ("ul", ["**▶** plays the response; **▶ together** plays the reference and the response at the same time, to hear the conversation.",
                        "The **master** sets the volume of everything.",
                        "**file ▾**, next to the response: **show in folder**; **export…** copies the WAV, the MIDI, the stems and the decisions to the folder you choose, in a subfolder named after the session; **move to the trash…** keeps the session in `lixeira/`, from where it can come back."]),
            ]),
            ("Judging", [
                ("tab", [("click outside the selection", "judges the whole response"),
                         ("click a block of the response", "judges that section"),
                         ("drag over the response", "judges the marked stretch")]),
                ("p", "Write a **note** if you like: your sentence about that listening is saved with the judgement and appears in the list of versions. Then click **accept**, **transform** or **reject**. The judgement weighs on the next responses: accepted ones stay in memory (up to 8) and rejected ones become compost. This also holds for the rules of the game."),
            ]),
            ("Comparing versions (Your listening)", [
                ("ol", ["Click a version in the list **Versions on this reference**: it is **A**.",
                        "Click another: it becomes the new **A**, and the previous one becomes **B**.",
                        "**▶ compare A × B** plays both; **switch A ↔ B** changes version at the same moment of the music."]),
                ("p", "Beside them are **What your listening taught** (how much each choice went up or down with your judgements) and **Rules of the game in version A**."),
            ]),
            ("The stems", [
                ("p", "With the stems box on, the response saves, next to the mix, one 24-bit stereo WAV per voice that sounded, in `sessoes/<session>/saidas/stems/`:"),
                ("ul", ["`1-motivo`, `2-baixo`, `3-eco`, `4-memoria`, `5-percussao`, `6-mosaico`, `7-harmonia`: each voice already processed, dry;",
                        "`8-gestos`: the Third Eye's gestures and the film effects;",
                        "`9-sala`: the reverberation of all the voices."]),
                ("p", "Each stem sounds only where its voice plays and stays at the level at which it enters the mix. Summed at 0 dB in the DAW, they rebuild the mix. To listen to the piece, use the mix."),
            ]),
            ("Where the data lives", [
                ("p", "Recordings, responses, judgements and memory live in the **data folder** you choose with **open data folder…**; Trioio remembers it the next time. Trioio composes offline: only the online word lookup box uses public services, and it starts switched off."),
            ]),
        ],
    },
    "fr": {
        "title": "Apprendre — Trioio", "h1": "Apprendre",
        "lead": "Chaque partie de la fenêtre de Trioio et ce qu'elle fait. Pour jouer la première fois, suivez le Tutoriel.",
        "desc": "Les onglets, les pistes, les modes, le panneau Langage, le jugement, la comparaison des versions et les stems de Trioio.",
        "secoes": [
            ("Les trois onglets", [
                ("tab", [("**Écoute et réponse**", "là où l'on joue : la référence en haut, le Troisième Œil et la réponse en dessous ; les modes, la lecture, le master et le jugement"),
                         ("**Bibliothèque**", "les enregistrements : apporter, autoriser et choisir. Dans une fenêtre large, elle reste à gauche"),
                         ("**Votre écoute**", "les versions composées, la comparaison A × B, ce que vos jugements ont appris et les règles du jeu")]),
            ]),
            ("Les trois pistes", [
                ("ul", ["**RÉFÉRENCE — ce qui sonne** : l'enregistrement que vous avez apporté, avec les sections que Trioio y a lues. **▶ référence** le joue et, la première fois, l'analyse.",
                        "**TROISIÈME ŒIL** : ce que l'instrument a décidé de lui-même. Il reste caché jusqu'à votre écoute ; **révéler après l'écoute** montre ce qu'il a fait.",
                        "**RÉPONSE** : le nouveau fichier que Trioio compose, entièrement synthétisé, en blocs par section."]),
            ]),
            ("Les modes", [
                ("p", "Le mode est l'attitude de la réponse face à la référence."),
                ("tab", [("**suivre**", "accompagne : même champ, même pulsation"),
                         ("**transfigurer**", "transforme : la même matière, une autre figure"),
                         ("**opposer**", "va contre : un autre registre, une autre densité"),
                         ("**interrompre**", "coupe, entre et sort")]),
            ]),
            ("Le panneau Langage ▸", [
                ("ul", ["le **mélange** de gammes, par exemple rāga Yaman + maqam Hijaz ;",
                        "les **hauteurs**, la **technique**, la **couture**, le **cycle**, le **voicing**, l'**intention**, la **forme** et le **tempo** ;",
                        "l'**imprévisibilité**, qui est le Troisième Œil ;",
                        "la **formation** : le style de l'ensemble, qui donne à chaque voix l'instrument et le phrasé du style. **automatique** laisse Trioio choisir les timbres ; **lu dans la référence** choisit le style d'après ce qu'il a entendu, et le mode répond ; et il y a les styles : choro, samba, école de samba, jazz, manouche, boléro, salsa, charanga, forró, tango, rock, chambre, orchestre et a cappella ;",
                        "**effets de cinéma dans la dramaturgie** : riser, impact et braam au climax, whoosh aux sections, ambiances dans les passages calmes ;",
                        "**dialogue : répondre dans les pauses de la référence** ;",
                        "**jeu : Trioio invente ses propres règles** ;",
                        "**stems : aussi un WAV par voix (pour la STAN)**."]),
                ("p", "Tout laisser tel quel est aussi un choix : Trioio décide alors à partir de l'écoute."),
            ]),
            ("Écouter et garder", [
                ("ul", ["**▶** joue la réponse ; **▶ ensemble** joue la référence et la réponse en même temps, pour entendre la conversation.",
                        "Le **master** règle le volume de tout.",
                        "**fichier ▾**, près de la réponse : **afficher dans le dossier** ; **exporter…** copie le WAV, le MIDI, les stems et les décisions dans le dossier choisi, dans un sous-dossier au nom de la session ; **mettre à la corbeille…** garde la session dans `lixeira/`, d'où elle peut revenir."]),
            ]),
            ("Juger", [
                ("tab", [("cliquer hors de la sélection", "juge la réponse entière"),
                         ("cliquer un bloc de la réponse", "juge cette section"),
                         ("glisser sur la réponse", "juge le passage marqué")]),
                ("p", "Écrivez une **note** si vous voulez : votre phrase sur cette écoute est gardée avec le jugement et apparaît dans la liste des versions. Puis cliquez **accepter**, **transformer** ou **rejeter**. Le jugement pèse sur les réponses suivantes : les acceptées restent en mémoire (jusqu'à 8) et les rejetées deviennent du compost. Cela vaut aussi pour les règles du jeu."),
            ]),
            ("Comparer les versions (Votre écoute)", [
                ("ol", ["Cliquez une version dans la liste **Versions sur cette référence** : c'est **A**.",
                        "Cliquez-en une autre : elle devient le nouvel **A**, et la précédente devient **B**.",
                        "**▶ comparer A × B** joue les deux ; **alterner A ↔ B** change de version au même instant de la musique."]),
                ("p", "À côté se trouvent **Ce que l'écoute a appris** (combien chaque choix est monté ou descendu avec vos jugements) et **Règles du jeu dans la version A**."),
            ]),
            ("Les stems", [
                ("p", "Avec la case stems cochée, la réponse enregistre, à côté du mixage, un WAV stéréo 24 bits par voix qui a sonné, dans `sessoes/<session>/saidas/stems/` :"),
                ("ul", ["`1-motivo`, `2-baixo`, `3-eco`, `4-memoria`, `5-percussao`, `6-mosaico`, `7-harmonia` : chaque voix déjà traitée, sèche ;",
                        "`8-gestos` : les gestes du Troisième Œil et les effets de cinéma ;",
                        "`9-sala` : la réverbération de toutes les voix."]),
                ("p", "Chaque stem sonne seulement là où sa voix joue et reste au niveau où il entre dans le mixage. Additionnés à 0 dB dans la STAN, ils refont le mixage. Pour écouter la pièce, utilisez le mixage."),
            ]),
            ("Où vivent les données", [
                ("p", "Enregistrements, réponses, jugements et mémoire vivent dans le **dossier de données** choisi avec **ouvrir le dossier de données…** ; Trioio s'en souvient la fois suivante. Trioio compose hors ligne : seule la case de recherche de mots en ligne utilise des services publics, et elle est désactivée au départ."),
            ]),
        ],
    },
    "es": {
        "title": "Aprender — Trioio", "h1": "Aprender",
        "lead": "Cada parte de la ventana de Trioio y lo que hace. Para tocar la primera vez, sigue el Tutorial.",
        "desc": "Las pestañas, las pistas, los modos, el panel Lenguaje, el juicio, la comparación de versiones y los stems de Trioio.",
        "secoes": [
            ("Las tres pestañas", [
                ("tab", [("**Escucha y respuesta**", "donde se toca: la referencia arriba, el Tercer Ojo y la respuesta abajo; los modos, el play, el máster y el juicio"),
                         ("**Biblioteca**", "las grabaciones: traer, autorizar y elegir. En una ventana ancha, queda siempre a la izquierda"),
                         ("**Tu escucha**", "las versiones compuestas, la comparación A × B, lo que tus juicios enseñaron y las reglas del juego")]),
            ]),
            ("Las tres pistas", [
                ("ul", ["**REFERENCIA — lo que suena**: la grabación que trajiste, con las secciones que Trioio leyó en ella. **▶ referencia** la toca y, la primera vez, la analiza.",
                        "**TERCER OJO**: lo que el instrumento decidió por su cuenta. Queda oculto hasta que escuchas; **revelar tras escuchar** muestra lo que hizo.",
                        "**RESPUESTA**: el archivo nuevo que Trioio compone, todo sintetizado, en bloques por sección."]),
            ]),
            ("Los modos", [
                ("p", "El modo es la actitud de la respuesta ante la referencia."),
                ("tab", [("**seguir**", "acompaña: mismo campo, mismo pulso"),
                         ("**transfigurar**", "transforma: la misma materia, otra figura"),
                         ("**contraponer**", "va en contra: otro registro, otra densidad"),
                         ("**interrumpir**", "corta, entra y sale")]),
            ]),
            ("El panel Lenguaje ▸", [
                ("ul", ["la **mezcla** de escalas, por ejemplo rāga Yaman + maqam Hijaz;",
                        "las **alturas**, la **técnica**, la **costura**, el **ciclo**, el **voicing**, la **intención**, la **forma** y el **tempo**;",
                        "la **imprevisibilidad**, que es el Tercer Ojo;",
                        "el **conjunto**: el estilo de la formación, que da a cada voz el instrumento y el fraseo del estilo. **automático** deja que Trioio elija los timbres; **leído en la referencia** elige el estilo por lo que escuchó, y el modo responde; y están los estilos: choro, samba, escuela de samba, jazz, manouche, bolero, salsa, charanga, forró, tango, rock, cámara, orquesta y a cappella;",
                        "**efectos de cine en la dramaturgia**: riser, impacto y braam en el clímax, whoosh en las secciones, ambientes en los pasajes tranquilos;",
                        "**diálogo: responder en las pausas de la referencia**;",
                        "**juego: Trioio inventa sus propias reglas**;",
                        "**stems: también un WAV por voz (para la DAW)**."]),
                ("p", "Dejar todo como está también es una elección: Trioio decide a partir de la escucha."),
            ]),
            ("Escuchar y guardar", [
                ("ul", ["**▶** toca la respuesta; **▶ juntos** toca la referencia y la respuesta a la vez, para oír la conversación.",
                        "El **máster** regula el volumen de todo.",
                        "**archivo ▾**, junto a la respuesta: **mostrar en la carpeta**; **exportar…** copia el WAV, el MIDI, los stems y las decisiones a la carpeta que elijas, en una subcarpeta con el nombre de la sesión; **mover a la papelera…** guarda la sesión en `lixeira/`, de donde puede volver."]),
            ]),
            ("Juzgar", [
                ("tab", [("hacer clic fuera de la selección", "juzga la respuesta entera"),
                         ("hacer clic en un bloque de la respuesta", "juzga esa sección"),
                         ("arrastrar sobre la respuesta", "juzga el fragmento marcado")]),
                ("p", "Escribe una **nota** si quieres: tu frase sobre esa escucha se guarda con el juicio y aparece en la lista de versiones. Después haz clic en **aceptar**, **transformar** o **rechazar**. El juicio pesa en las próximas respuestas: las aceptadas quedan en la memoria (hasta 8) y las rechazadas se vuelven abono. Vale también para las reglas del juego."),
            ]),
            ("Comparar versiones (Tu escucha)", [
                ("ol", ["Haz clic en una versión de la lista **Versiones sobre esta referencia**: es la **A**.",
                        "Haz clic en otra: pasa a ser la nueva **A**, y la anterior pasa a ser la **B**.",
                        "**▶ comparar A × B** toca las dos; **alternar A ↔ B** cambia de versión en el mismo instante de la música."]),
                ("p", "Al lado están **Lo que enseñó la escucha** (cuánto subió o bajó cada elección con tus juicios) y **Reglas del juego en la versión A**."),
            ]),
            ("Los stems", [
                ("p", "Con la casilla de stems marcada, la respuesta graba, junto a la mezcla, un WAV estéreo de 24 bits por cada voz que sonó, en `sessoes/<sesión>/saidas/stems/`:"),
                ("ul", ["`1-motivo`, `2-baixo`, `3-eco`, `4-memoria`, `5-percussao`, `6-mosaico`, `7-harmonia`: cada voz ya procesada, seca;",
                        "`8-gestos`: los gestos del Tercer Ojo y los efectos de cine;",
                        "`9-sala`: la reverberación de todas las voces."]),
                ("p", "Cada stem suena solo donde toca su voz y queda en el nivel con que entra en la mezcla. Sumados a 0 dB en la DAW, rehacen la mezcla. Para escuchar la pieza, usa la mezcla."),
            ]),
            ("Dónde viven los datos", [
                ("p", "Grabaciones, respuestas, juicios y memoria viven en la **carpeta de datos** que eliges con **abrir carpeta de datos…**; Trioio la recuerda la próxima vez. Trioio compone sin internet: solo la casilla de consulta de palabras en la red usa servicios públicos, y viene desactivada."),
            ]),
        ],
    },
}

INSTALACAO = {
    "pt-BR": {
        "title": "Instalação — Trioio", "h1": "Instalação",
        "lead": "Como instalar o Trioio no Windows, no macOS e no Linux, o que fazer na primeira abertura e como resolver os problemas mais comuns.",
        "desc": "Instalar o Trioio no Windows, macOS e Linux, passo a passo, com os avisos de segurança, a primeira abertura e os requisitos.",
        "secoes": [
            ("Antes de começar", [
                ("p", "Os pacotes estão em **Baixar**, na página inicial, e na página de releases do GitHub. Os avisos de segurança do Windows e do macOS são esperados e aparecem só na primeira vez."),
            ]),
            ("Windows", [
                ("ol", ["Baixe `trioio-<versão>-win64.exe` e abra.",
                        "O Windows mostra **\"O Windows protegeu o computador\"**: clique em **\"Mais informações\"** e depois em **\"Executar assim mesmo\"**.",
                        "Se pedir permissão de administrador, clique em **\"Sim\"**.",
                        "Clique em **\"Avançar\"** até o fim. O Trioio fica no menu Iniciar e na área de trabalho."]),
                ("p", "**Sem instalar:** descompacte `trioio-<versão>-win64.zip` e abra `Trioio.exe`, com a pasta `data` ao lado dele."),
            ]),
            ("macOS", [
                ("ol", ["Baixe `trioio-<versão>-Darwin.dmg`. Ele funciona em Macs Intel e Apple Silicon.",
                        "Abra e arraste o Trioio para **Aplicativos**.",
                        "Abra o Trioio. Na primeira vez, o macOS o bloqueia, porque o app tem assinatura ad hoc, sem o Developer ID pago da Apple. Clique em **\"OK\"**.",
                        "Libere o app uma vez: em **Ajustes do Sistema → Privacidade e Segurança**, clique em **\"Abrir Mesmo Assim\"** e confirme. No macOS 14 ou anterior: botão direito no app, no Finder → **Abrir → Abrir**."]),
                ("p", "Se o botão não aparecer, use o Terminal. Ele tira a marca de quarentena que o macOS põe em todo arquivo baixado:"),
                ("code", 'xattr -dr com.apple.quarantine "/Applications/Trioio.app"'),
            ]),
            ("Linux", [
                ("ul", ["**Ubuntu 22.04+, Debian 12+, Mint 21+:** dois cliques no `.deb`, ou `sudo apt install ./trioio-<versão>-Linux.deb`.",
                        "**Outras distribuições, AppImage:** torne-o executável (Propriedades → permitir executar, ou `chmod +x`) e abra.",
                        "**Outras distribuições, `.tar.gz`:** descompacte e rode `./install.sh`. Ele instala para o seu usuário, sem root; `./install.sh --remove` desfaz."]),
            ]),
            ("Na primeira abertura", [
                ("ol", ["Escolha uma **pasta de dados** em **abrir pasta de dados…**. As suas gravações, respostas, julgamentos e memória ficam nela. Uma resposta com stems ocupa algumas centenas de MB, então escolha um disco com espaço.",
                        "O botão **TUTORIAL**, ao lado de **SOBRE**, explica o instrumento passo a passo, na língua da interface.",
                        "O Trioio compõe sem internet. A caixa de consulta de palavras na rede é a única que usa serviços públicos (Wikidata, Datamuse), e vem desligada."]),
            ]),
            ("Problemas e soluções", [
                ("tab", [("**Abre em demonstração e não compõe**", "escolha a pasta de dados em **abrir pasta de dados…**; o Trioio lembra dela"),
                         ("**Não compõe sobre a minha gravação**", "uma gravação nova começa pendente: clique em **autorizar…**, declare a origem e clique em **autorizar**"),
                         ("**Sem som**", "o Trioio toca pela saída de áudio padrão do sistema: escolha a saída nos ajustes de som, reabra o Trioio e confira o **master**"),
                         ("**Parece travado ao compor**", "a barra no rodapé mostra cada etapa e o tempo; uma resposta leva de alguns segundos a cerca de um minuto"),
                         ("**Linux: a janela não abre**", "instale o `.deb` com o `apt`, que traz as dependências; para o AppImage, instale `libfuse2` ou abra com `--appimage-extract-and-run`")]),
                ("p", "Para outra coisa, escreva para **rasgo.instruments@gmail.com** com o seu sistema, a versão, o que você fez e a mensagem que apareceu."),
            ]),
            ("Requisitos", [
                ("tab", [("**Windows**", "Windows 10 ou 11, 64 bits"),
                         ("**macOS**", "10.15 ou mais novo, Intel e Apple Silicon"),
                         ("**Linux**", "x86-64 com glibc 2.35 ou mais nova (distribuições de 2022 em diante)"),
                         ("**Processador**", "64 bits, dois núcleos ou mais; medido num Intel Core i5-6500: cerca de 20 s por resposta, usando cerca de um núcleo"),
                         ("**Memória**", "cerca de 250 a 500 MB ao compor; 4 GB no computador bastam"),
                         ("**Disco**", "cerca de 25 MB para o app; as respostas vão para a pasta de dados")]),
            ]),
        ],
    },
    "en": {
        "title": "Installation — Trioio", "h1": "Installation",
        "lead": "How to install Trioio on Windows, macOS and Linux, what to do the first time you open it and how to solve the most common problems.",
        "desc": "Install Trioio on Windows, macOS and Linux, step by step, with the security warnings, the first opening and the requirements.",
        "secoes": [
            ("Before you start", [
                ("p", "The packages are under **Download**, on the home page, and on the GitHub releases page. The Windows and macOS security warnings are expected and appear only the first time."),
            ]),
            ("Windows", [
                ("ol", ["Download `trioio-<version>-win64.exe` and open it.",
                        "Windows shows **\"Windows protected your PC\"**: click **\"More info\"**, then **\"Run anyway\"**.",
                        "If Windows asks for administrator permission, click **\"Yes\"**.",
                        "Click **\"Next\"** to the end. Trioio is then in the Start menu and on the desktop."]),
                ("p", "**Without installing:** unzip `trioio-<version>-win64.zip` and open `Trioio.exe`, with the `data` folder next to it."),
            ]),
            ("macOS", [
                ("ol", ["Download `trioio-<version>-Darwin.dmg`. It works on Intel and Apple Silicon Macs.",
                        "Open it and drag Trioio into **Applications**.",
                        "Open Trioio. The first time, macOS blocks it, because the app is signed ad hoc, without a paid Apple Developer ID. Click **\"OK\"**.",
                        "Allow the app once: in **System Settings → Privacy & Security**, click **\"Open Anyway\"** and confirm. On macOS 14 or earlier: right-click the app in Finder → **Open → Open**."]),
                ("p", "If the button does not appear, use Terminal. It removes the quarantine mark macOS puts on every downloaded file:"),
                ("code", 'xattr -dr com.apple.quarantine "/Applications/Trioio.app"'),
            ]),
            ("Linux", [
                ("ul", ["**Ubuntu 22.04+, Debian 12+, Mint 21+:** double-click the `.deb`, or `sudo apt install ./trioio-<version>-Linux.deb`.",
                        "**Other distributions, AppImage:** make it executable (Properties → allow executing, or `chmod +x`) and open it.",
                        "**Other distributions, `.tar.gz`:** unpack it and run `./install.sh`. It installs for your user, without root; `./install.sh --remove` undoes it."]),
            ]),
            ("The first time you open it", [
                ("ol", ["Choose a **data folder** with **open data folder…**. Your recordings, responses, judgements and memory live there. A response with stems takes a few hundred MB, so choose a disk with room.",
                        "The **TUTORIAL** button, next to **ABOUT**, explains the instrument step by step, in the interface language.",
                        "Trioio composes offline. The online word lookup box is the only one that uses public services (Wikidata, Datamuse), and it starts switched off."]),
            ]),
            ("Troubleshooting", [
                ("tab", [("**It opens in demonstration mode and does not compose**", "choose the data folder with **open data folder…**; Trioio remembers it"),
                         ("**It does not compose on my recording**", "a new recording starts as pending: click **authorise…**, declare its origin and click **authorise**"),
                         ("**No sound**", "Trioio plays through the system's default audio output: choose the output in the sound settings, reopen Trioio and check the **master**"),
                         ("**It seems frozen while composing**", "the bar at the bottom shows each stage and the time; a response takes from a few seconds to about a minute"),
                         ("**Linux: the window does not open**", "install the `.deb` with `apt`, which brings the dependencies; for the AppImage, install `libfuse2` or start it with `--appimage-extract-and-run`")]),
                ("p", "For anything else, write to **rasgo.instruments@gmail.com** with your system, the version, what you did and the message you saw."),
            ]),
            ("Requirements", [
                ("tab", [("**Windows**", "Windows 10 or 11, 64-bit"),
                         ("**macOS**", "10.15 or later, Intel and Apple Silicon"),
                         ("**Linux**", "x86-64 with glibc 2.35 or newer (2022+ distributions)"),
                         ("**Processor**", "64-bit, two cores or more; measured on an Intel Core i5-6500: about 20 s per response, using about one core"),
                         ("**Memory**", "about 250–500 MB while composing; 4 GB in the computer is enough"),
                         ("**Disk**", "about 25 MB for the app; the responses go to the data folder")]),
            ]),
        ],
    },
    "fr": {
        "title": "Installation — Trioio", "h1": "Installation",
        "lead": "Comment installer Trioio sous Windows, macOS et Linux, que faire à la première ouverture et comment résoudre les problèmes les plus courants.",
        "desc": "Installer Trioio sous Windows, macOS et Linux, pas à pas, avec les avertissements de sécurité, la première ouverture et la configuration requise.",
        "secoes": [
            ("Avant de commencer", [
                ("p", "Les paquets sont dans **Télécharger**, sur la page d'accueil, et sur la page des releases GitHub. Les avertissements de sécurité de Windows et de macOS sont attendus et n'apparaissent que la première fois."),
            ]),
            ("Windows", [
                ("ol", ["Téléchargez `trioio-<version>-win64.exe` et ouvrez-le.",
                        "Windows affiche **« Windows a protégé votre ordinateur »** : cliquez sur **« Informations complémentaires »**, puis sur **« Exécuter quand même »**.",
                        "Si Windows demande l'autorisation d'administrateur, cliquez sur **« Oui »**.",
                        "Cliquez sur **« Suivant »** jusqu'à la fin. Trioio est alors dans le menu Démarrer et sur le bureau."]),
                ("p", "**Sans installer :** décompressez `trioio-<version>-win64.zip` et ouvrez `Trioio.exe`, avec le dossier `data` à côté."),
            ]),
            ("macOS", [
                ("ol", ["Téléchargez `trioio-<version>-Darwin.dmg`. Il fonctionne sur les Mac Intel et Apple Silicon.",
                        "Ouvrez-le et glissez Trioio dans **Applications**.",
                        "Ouvrez Trioio. La première fois, macOS le bloque, car l'app a une signature ad hoc, sans Developer ID Apple payant. Cliquez sur **« OK »**.",
                        "Autorisez l'app une fois : dans **Réglages Système → Confidentialité et sécurité**, cliquez sur **« Ouvrir quand même »** et confirmez. Sous macOS 14 ou antérieur : clic droit sur l'app dans le Finder → **Ouvrir → Ouvrir**."]),
                ("p", "Si le bouton n'apparaît pas, utilisez le Terminal. La commande retire la marque de quarantaine que macOS pose sur chaque fichier téléchargé :"),
                ("code", 'xattr -dr com.apple.quarantine "/Applications/Trioio.app"'),
            ]),
            ("Linux", [
                ("ul", ["**Ubuntu 22.04+, Debian 12+, Mint 21+ :** double-cliquez sur le `.deb`, ou `sudo apt install ./trioio-<version>-Linux.deb`.",
                        "**Autres distributions, AppImage :** rendez-le exécutable (Propriétés → autoriser l'exécution, ou `chmod +x`) et ouvrez-le.",
                        "**Autres distributions, `.tar.gz` :** décompressez et lancez `./install.sh`. Il installe pour votre utilisateur, sans root ; `./install.sh --remove` annule."]),
            ]),
            ("À la première ouverture", [
                ("ol", ["Choisissez un **dossier de données** avec **ouvrir le dossier de données…**. Vos enregistrements, réponses, jugements et mémoire y vivent. Une réponse avec stems occupe quelques centaines de Mo : choisissez un disque avec de la place.",
                        "Le bouton **TUTORIEL**, à côté d'**À PROPOS**, explique l'instrument pas à pas, dans la langue de l'interface.",
                        "Trioio compose hors ligne. La case de recherche de mots en ligne est la seule qui utilise des services publics (Wikidata, Datamuse), et elle est désactivée au départ."]),
            ]),
            ("Problèmes et solutions", [
                ("tab", [("**S'ouvre en démonstration et ne compose pas**", "choisissez le dossier de données avec **ouvrir le dossier de données…** ; Trioio s'en souvient"),
                         ("**Ne compose pas sur mon enregistrement**", "un nouvel enregistrement commence en attente : cliquez **autoriser…**, déclarez l'origine et cliquez **autoriser**"),
                         ("**Pas de son**", "Trioio joue par la sortie audio par défaut du système : choisissez la sortie dans les réglages du son, rouvrez Trioio et vérifiez le **master**"),
                         ("**Semble figé pendant la composition**", "la barre du bas montre chaque étape et le temps ; une réponse prend de quelques secondes à environ une minute"),
                         ("**Linux : la fenêtre ne s'ouvre pas**", "installez le `.deb` avec `apt`, qui apporte les dépendances ; pour l'AppImage, installez `libfuse2` ou lancez-le avec `--appimage-extract-and-run`")]),
                ("p", "Pour autre chose, écrivez à **rasgo.instruments@gmail.com** avec votre système, la version, ce que vous avez fait et le message affiché."),
            ]),
            ("Configuration requise", [
                ("tab", [("**Windows**", "Windows 10 ou 11, 64 bits"),
                         ("**macOS**", "10.15 ou plus récent, Intel et Apple Silicon"),
                         ("**Linux**", "x86-64 avec glibc 2.35 ou plus récente (distributions de 2022 et après)"),
                         ("**Processeur**", "64 bits, deux cœurs ou plus ; mesuré sur un Intel Core i5-6500 : environ 20 s par réponse, sur environ un cœur"),
                         ("**Mémoire**", "environ 250 à 500 Mo pendant la composition ; 4 Go dans l'ordinateur suffisent"),
                         ("**Disque**", "environ 25 Mo pour l'app ; les réponses vont dans le dossier de données")]),
            ]),
        ],
    },
    "es": {
        "title": "Instalación — Trioio", "h1": "Instalación",
        "lead": "Cómo instalar Trioio en Windows, macOS y Linux, qué hacer la primera vez que lo abres y cómo resolver los problemas más comunes.",
        "desc": "Instalar Trioio en Windows, macOS y Linux, paso a paso, con los avisos de seguridad, la primera apertura y los requisitos.",
        "secoes": [
            ("Antes de empezar", [
                ("p", "Los paquetes están en **Descargar**, en la página de inicio, y en la página de releases de GitHub. Los avisos de seguridad de Windows y macOS son esperables y aparecen solo la primera vez."),
            ]),
            ("Windows", [
                ("ol", ["Descarga `trioio-<versión>-win64.exe` y ábrelo.",
                        "Windows muestra **\"Windows protegió su PC\"**: haz clic en **\"Más información\"** y luego en **\"Ejecutar de todas formas\"**.",
                        "Si Windows pide permiso de administrador, haz clic en **\"Sí\"**.",
                        "Haz clic en **\"Siguiente\"** hasta el final. Trioio queda en el menú Inicio y en el escritorio."]),
                ("p", "**Sin instalar:** descomprime `trioio-<versión>-win64.zip` y abre `Trioio.exe`, con la carpeta `data` al lado."),
            ]),
            ("macOS", [
                ("ol", ["Descarga `trioio-<versión>-Darwin.dmg`. Funciona en Mac Intel y Apple Silicon.",
                        "Ábrelo y arrastra Trioio a **Aplicaciones**.",
                        "Abre Trioio. La primera vez, macOS lo bloquea, porque la app tiene firma ad hoc, sin el Developer ID de pago de Apple. Haz clic en **\"OK\"**.",
                        "Autoriza la app una vez: en **Ajustes del Sistema → Privacidad y seguridad**, haz clic en **\"Abrir igualmente\"** y confirma. En macOS 14 o anterior: clic derecho en la app en el Finder → **Abrir → Abrir**."]),
                ("p", "Si el botón no aparece, usa la Terminal. El comando quita la marca de cuarentena que macOS pone a todo archivo descargado:"),
                ("code", 'xattr -dr com.apple.quarantine "/Applications/Trioio.app"'),
            ]),
            ("Linux", [
                ("ul", ["**Ubuntu 22.04+, Debian 12+, Mint 21+:** doble clic en el `.deb`, o `sudo apt install ./trioio-<versión>-Linux.deb`.",
                        "**Otras distribuciones, AppImage:** hazlo ejecutable (Propiedades → permitir ejecutar, o `chmod +x`) y ábrelo.",
                        "**Otras distribuciones, `.tar.gz`:** descomprímelo y ejecuta `./install.sh`. Instala para tu usuario, sin root; `./install.sh --remove` lo deshace."]),
            ]),
            ("La primera vez que lo abres", [
                ("ol", ["Elige una **carpeta de datos** con **abrir carpeta de datos…**. Tus grabaciones, respuestas, juicios y memoria viven en ella. Una respuesta con stems ocupa algunos cientos de MB, así que elige un disco con espacio.",
                        "El botón **TUTORIAL**, junto a **ACERCA DE**, explica el instrumento paso a paso, en el idioma de la interfaz.",
                        "Trioio compone sin internet. La casilla de consulta de palabras en la red es la única que usa servicios públicos (Wikidata, Datamuse), y viene desactivada."]),
            ]),
            ("Problemas y soluciones", [
                ("tab", [("**Abre en demostración y no compone**", "elige la carpeta de datos con **abrir carpeta de datos…**; Trioio la recuerda"),
                         ("**No compone sobre mi grabación**", "una grabación nueva empieza pendiente: haz clic en **autorizar…**, declara el origen y haz clic en **autorizar**"),
                         ("**Sin sonido**", "Trioio suena por la salida de audio predeterminada del sistema: elige la salida en los ajustes de sonido, vuelve a abrir Trioio y revisa el **máster**"),
                         ("**Parece bloqueado al componer**", "la barra de abajo muestra cada etapa y el tiempo; una respuesta tarda de unos segundos a cerca de un minuto"),
                         ("**Linux: la ventana no se abre**", "instala el `.deb` con `apt`, que trae las dependencias; para el AppImage, instala `libfuse2` o ábrelo con `--appimage-extract-and-run`")]),
                ("p", "Para otra cosa, escribe a **rasgo.instruments@gmail.com** con tu sistema, la versión, lo que hiciste y el mensaje que apareció."),
            ]),
            ("Requisitos", [
                ("tab", [("**Windows**", "Windows 10 u 11, 64 bits"),
                         ("**macOS**", "10.15 o posterior, Intel y Apple Silicon"),
                         ("**Linux**", "x86-64 con glibc 2.35 o más reciente (distribuciones de 2022 en adelante)"),
                         ("**Procesador**", "64 bits, dos núcleos o más; medido en un Intel Core i5-6500: unos 20 s por respuesta, usando cerca de un núcleo"),
                         ("**Memoria**", "unos 250 a 500 MB al componer; 4 GB en el ordenador bastan"),
                         ("**Disco**", "unos 25 MB para la app; las respuestas van a la carpeta de datos")]),
            ]),
        ],
    },
}
