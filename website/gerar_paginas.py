#!/usr/bin/env python3
"""Gera as quatro páginas do site do Trioio (index.html em português, a
canônica, e en/fr/es.html) a partir de um modelo único e dos textos de cada
idioma, para as quatro nunca divergirem na estrutura. Depois, `python3
seo.py` injeta os metadados (o bloco SEO não é escrito aqui).

    python3 gerar_paginas.py && python3 seo.py

Editar os TEXTOS aqui, não nos .html (eles são derivados).
"""

import html
import pathlib

AQUI = pathlib.Path(__file__).parent
PORTAL = "https://rasgoinstruments.arquiviagem.net/"
REPO = "https://github.com/lucioaraujo/trioio"
BLOB = "https://github.com/lucioaraujo/trioio/blob/main"
# a versão publicada e os pacotes do release (a CI: .github/workflows/package.yml)
VERSAO = "0.1.0"
RELEASE = "https://github.com/lucioaraujo/trioio/releases/tag/v" + VERSAO
DL = "https://github.com/lucioaraujo/trioio/releases/download/v" + VERSAO + "/"
PACOTES = [
    ("Windows", [("trioio-%s-win64.exe" % VERSAO, "exe"), ("trioio-%s-win64.zip" % VERSAO, "zip")]),
    ("macOS", [("trioio-%s-Darwin.dmg" % VERSAO, "dmg")]),
    ("Linux", [("trioio-%s-Linux.deb" % VERSAO, "deb"), ("trioio-%s-linux-x86_64.AppImage" % VERSAO, "AppImage"),
               ("trioio-%s-linux-x86_64.tar.gz" % VERSAO, "tar.gz")]),
]

PAGINAS = [("index.html", "pt-BR", "PT"), ("en.html", "en", "EN"), ("fr.html", "fr", "FR"), ("es.html", "es", "ES")]

T = {
    "pt-BR": {
        "title": "Trioio — escuta, memória e resposta",
        "desc": "Trioio: instrumento que escuta uma referência autorizada, reconhece pulso, harmonia, melodia e linguagem musical, e compõe uma resposta própria. Com o Terceiro Olho.",
        "strip": "RASGO — família de instrumentos e pesquisa sonora",
        "skip": "Pular para o conteúdo",
        "langnav": "Idioma",
        "pill": "Versão 0.1.0 — prévia, para Windows, macOS e Linux",
        "lead": "Trioio é um instrumento que escuta. Você traz uma referência — uma gravação sua, a saída de outro instrumento, uma faixa autorizada — e ele reconhece o pulso, a harmonia, a melodia e a linguagem musical dela. Depois compõe uma resposta própria, sintetizada do zero, e aprende com o seu julgamento.",
        "s1": "Escuta e resposta",
        "steps": [
            ("Autorizar", "Toda referência entra pela Biblioteca com a sua proveniência: quem fez, com que direito, para quais usos. Sem autorização, nada é analisado nem derivado."),
            ("Escutar", "O Trioio lê o pulso e o compasso, os acordes e a tonalidade, a melodia dentro da mistura e a linguagem — maqam, rāga, modo, flamenco… — pelo arquivo de escalas que ele mesmo escreveu."),
            ("Responder", "Em quatro modos — seguir, transfigurar, contrapor, interromper —, compõe motivo, frases, forma e harmonia, e sintetiza com famílias de timbre próprias. Nada é amostra: tudo é síntese."),
            ("Julgar", "Você aceita, transforma ou rejeita a resposta, inteira ou por trecho. O julgamento vira memória: puxa ou afasta tendências, sem aprisionar o instrumento num gosto."),
        ],
        "s2": "O Terceiro Olho",
        "eye": "O primeiro olho participa; o segundo observa; o Terceiro Olho percebe a estrutura e pode transformá-la. No Trioio ele intervém na resposta — um objeto sonoro, uma regra oculta — e algumas intervenções deixam consequências que seguem até o fim. Ele fica escondido até você ouvir: a escuta vem antes da explicação.",
        "s3": "Linguagens e culturas",
        "lang": "O Trioio carrega um arquivo de escalas e ciclos escrito por ele mesmo a partir do conhecimento publicado, com a fonte citada em cada entrada, como numa pesquisa — nenhum arquivo de terceiros é copiado.",
        "facts": [
            "2576 escalas: gêneros gregos, temperamentos históricos, entonação justa, maqamat, rāgas e melakartas, gamelão, modos nordestinos, Messiaen e outras;",
            "32 delas trazem o comportamento da linguagem: subida e descida, notas de apoio, frases-assinatura, cadências, ornamentos e textura;",
            "26 ciclos rítmicos: iqa', tala, usul e aksak, claves, colotomia do gamelão, compás flamenco, baião;",
            "misturas entre culturas — colagem, híbrido, camadas, transição — com costuras entre as seções.",
            "a máquina de harmonia: 25 linguagens — da funcional e do jazz às brasileiras (Hermeto, Villa-Lobos, Moacir Santos, Guinga, Gismonti, frevo, maracatu), às modernas e à microtonal de Partch —, com condução a quatro vozes;",
            "15 conjuntos — choro, samba, jazz, forró, tango, rock, orquestra, a cappella… —, cada voz com o instrumento e a figura do estilo.",
        ],
        "example": "Exemplo: numa gravação com a cadência andaluza (Rém–Dó–Sib–Lá), o Trioio reconhece o Lá frígio dominante e responde, conforme a semente, em maqam Hijaz — heterofonia, ciclo maqsum — ou no modo de Mi andaluz — acordes, compás de 12. No Hijaz, sem acordes, o movimento vem do sayr: a melodia visita o maqam vizinho no ghammaz e volta.",
        "s4": "A interface",
        "shot_alt": "A tela Escuta e resposta do Trioio: a referência com os acordes lidos, o Terceiro Olho revelado com as consequências, e a resposta em seções com o rolo de notas.",
        "shot_cap": "A tela Escuta e resposta: três faixas alinhadas no tempo — a referência, o Terceiro Olho e a resposta —, a Biblioteca, a linguagem da resposta e a barra de julgamento. Em quatro idiomas, de 960×600 a 2560×1440.",
        "s5": "Proveniência",
        "prov": "Cada referência tem um manifesto: origem, fundamento, usos autorizados, histórico. O Trioio só deriva do que foi autorizado para derivar, e cada resposta registra de onde veio. O código reaproveitado de outros instrumentos RASGO — Rasgo Synth, Rasgo Modular, Antitotem, Aquorbium — é uma cópia fixada, com commit de origem e checksums.",
        "s6": "Créditos",
        "credits": "Lúcio de Araújo — concepção, composição, escuta, interface e código. Licença GNU AGPLv3 ou posterior; JUCE sob a via AGPLv3. Fontes e créditos completos:",
        "credits_link": "CREDITS_AND_SOURCES.md",
        "s7": "Estado",
        "state": "A versão 0.1.0 é uma prévia. Os pacotes de Windows e macOS são construídos e testados pela integração contínua, mas ainda não foram abertos pelo autor em máquinas reais; o Linux é testado também na abertura, em quatro distribuições. O instrumento continua em desenvolvimento, aberto.",
        "contact": "Contato",
        "footer_credits": "Créditos",
        "footer_license": "Licença AGPLv3+",
        "dialog_title": "CONTATO",
        "dialog_copy_hint": "Copie o endereço de e-mail:",
        "dialog_copy": "Copiar e-mail",
        "dialog_close": "Fechar janela de contato",
        "copied": "E-mail copiado.",
        "fallback": "Selecione e copie o endereço acima.",
        "s_dl": "Baixar",
        "dl_intro": "A versão 0.1.0 é uma prévia: o instrumento faz o essencial e continua aberto, em desenvolvimento. Escolha o pacote do seu sistema.",
        "dl_os": {"Windows": "Windows 10 ou 11 — instalador (.exe) ou sem instalar (.zip)", "macOS": "macOS 10.15 ou mais novo, Intel e Apple Silicon", "Linux": ".deb para Ubuntu, Debian e Mint; AppImage ou .tar.gz para as outras distribuições"},
        "dl_warn": "Na primeira abertura, o Windows e o macOS mostram um aviso de segurança: é esperado, porque o app não tem a assinatura paga das duas empresas. O guia de instalação diz onde clicar.",
        "dl_install": "Guia de instalação",
        "dl_all": "Todos os pacotes e as notas da versão",
        "s_words": "Palavras, influências e gesto",
        "words": [["Palavras", "Escreva, se quiser, algumas palavras sobre a resposta — em qualquer língua, com ou sem acento, com erros de digitação. O Trioio lê a intenção, a forma e a dança, e mostra o que entendeu. Em branco, ele segue os próprios critérios."], ["A rede, opcional", "As palavras que o Trioio não conhece podem ser consultadas em serviços públicos (Wikidata, Datamuse). O cálculo é dele: os serviços só devolvem palavras. Vem desligada, porque as palavras saem do computador."], ["A árvore de influências", "Estilos não são gavetas: são ramos de uma árvore aberta, com contextos sociais, históricos, tecnológicos e de geração, e traços que são tendências, nunca regras. O Trioio cruza ramos distantes para chegar ao inesperado; as poéticas de artistas e bandas, quem escreve é o músico."], ["O gesto", "Desenhe a curva de intensidade da peça, e a resposta segue essa forma."]],
        "tutorial": "O app traz um tutorial em quatro idiomas, do primeiro passo ao primeiro exercício.",
    },
    "en": {
        "title": "Trioio — listening, memory and response",
        "desc": "Trioio: an instrument that listens to an authorised reference, recognises pulse, harmony, melody and musical language, and composes a response of its own. With the Third Eye.",
        "strip": "RASGO — a family of instruments and sound research",
        "skip": "Skip to content",
        "langnav": "Language",
        "pill": "Version 0.1.0 — preview, for Windows, macOS and Linux",
        "lead": "Trioio is an instrument that listens. You bring a reference — a recording of yours, the output of another instrument, an authorised track — and it recognises its pulse, harmony, melody and musical language. Then it composes a response of its own, synthesised from scratch, and learns from your judgement.",
        "s1": "Listening and response",
        "steps": [
            ("Authorise", "Every reference enters through the Library with its provenance: who made it, by what right, for which uses. Without authorisation, nothing is analysed or derived."),
            ("Listen", "Trioio reads the pulse and the metre, the chords and the key, the melody inside the mix and the language — maqam, rāga, mode, flamenco… — through the archive of scales it wrote itself."),
            ("Respond", "In four modes — follow, transfigure, oppose, interrupt — it composes motif, phrases, form and harmony, and synthesises them with timbre families of its own. Nothing is a sample: everything is synthesis."),
            ("Judge", "You accept, transform or reject the response, whole or by passage. The judgement becomes memory: it pulls or pushes tendencies, without locking the instrument into a taste."),
        ],
        "s2": "The Third Eye",
        "eye": "The first eye takes part; the second observes; the Third Eye perceives the structure and may transform it. In Trioio it intervenes in the response — a sound object, a hidden rule — and some interventions leave consequences that run to the end. It stays hidden until you have listened: listening comes before explanation.",
        "s3": "Languages and cultures",
        "lang": "Trioio carries an archive of scales and cycles it wrote itself from published knowledge, citing the source of every entry as in research — no third-party archive is copied.",
        "facts": [
            "2576 scales: Greek genera, historical temperaments, just intonation, maqamat, rāgas and melakartas, gamelan, the modes of north-eastern Brazil, Messiaen and others;",
            "32 of them carry the behaviour of the language: ascent and descent, resting tones, signature phrases, cadences, ornaments and texture;",
            "26 rhythmic cycles: iqa', tala, usul and aksak, claves, gamelan colotomy, flamenco compás, baião;",
            "mixtures between cultures — collage, hybrid, layers, transition — with seams between the sections.",
            "the harmony machine: 25 languages — from functional harmony and jazz to the Brazilian ones (Hermeto, Villa-Lobos, Moacir Santos, Guinga, Gismonti, frevo, maracatu), the modern ones and Partch's microtonality —, with four-part voice leading;",
            "15 ensembles — choro, samba, jazz, forró, tango, rock, orchestra, a cappella… —, each voice with the instrument and the figure of the style.",
        ],
        "example": "Example: in a recording with the Andalusian cadence (Dm–C–B♭–A), Trioio recognises A Phrygian dominant and responds, depending on the seed, in maqam Hijaz — heterophony, maqsum cycle — or in the Andalusian mode on E — chords, 12-beat compás. In Hijaz, without chords, the movement comes from sayr: the melody visits the neighbouring maqam on the ghammaz and returns.",
        "s4": "The interface",
        "shot_alt": "Trioio's Listening and response screen: the reference with the chords it read, the revealed Third Eye with its consequences, and the response in sections with the piano roll.",
        "shot_cap": "The Listening and response screen: three tracks aligned in time — the reference, the Third Eye and the response —, the Library, the language of the response and the judgement bar. In four languages, from 960×600 to 2560×1440.",
        "s5": "Provenance",
        "prov": "Every reference has a manifest: origin, basis, authorised uses, history. Trioio only derives from what was authorised for derivation, and every response records where it came from. Code reused from other RASGO instruments — Rasgo Synth, Rasgo Modular, Antitotem, Aquorbium — is a pinned copy, with source commit and checksums.",
        "s6": "Credits",
        "credits": "Lúcio de Araújo — concept, composition, listening, interface and code. Licence GNU AGPLv3 or later; JUCE under its AGPLv3 option. Full sources and credits:",
        "credits_link": "CREDITS_AND_SOURCES.md",
        "s7": "Status",
        "state": "Version 0.1.0 is a preview. The Windows and macOS packages are built and tested by continuous integration but have not yet been opened by the author on real machines; Linux is also launch-tested on four distributions. The instrument remains in development, open.",
        "contact": "Contact",
        "footer_credits": "Credits",
        "footer_license": "AGPLv3+ licence",
        "dialog_title": "CONTACT",
        "dialog_copy_hint": "Copy the email address:",
        "dialog_copy": "Copy email",
        "dialog_close": "Close contact window",
        "copied": "Email copied.",
        "fallback": "Select and copy the address above.",
        "s_dl": "Download",
        "dl_intro": "Version 0.1.0 is a preview: the instrument does the essential and remains open, in development. Choose the package for your system.",
        "dl_os": {"Windows": "Windows 10 or 11 — installer (.exe) or no install (.zip)", "macOS": "macOS 10.15 or later, Intel and Apple Silicon", "Linux": ".deb for Ubuntu, Debian and Mint; AppImage or .tar.gz for other distributions"},
        "dl_warn": "On first launch, Windows and macOS show a security warning: it is expected, because the app does not carry the two companies' paid signature. The installation guide says where to click.",
        "dl_install": "Installation guide",
        "dl_all": "All packages and release notes",
        "s_words": "Words, influences and gesture",
        "words": [["Words", "Write, if you like, a few words about the response — in any language, with or without accents, with typing mistakes. Trioio reads the intention, the form and the dance, and shows what it understood. Left blank, it follows its own criteria."], ["The network, optional", "Words Trioio does not know can be looked up in public services (Wikidata, Datamuse). The calculation is its own: the services only return words. It comes switched off, because the words leave the computer."], ["The influence tree", "Styles are not drawers: they are branches of an open tree, with social, historical, technological and generational contexts, and traits that are tendencies, never rules. Trioio crosses distant branches to reach the unexpected; the poetics of artists and bands are written by the musician."], ["The gesture", "Draw the intensity curve of the piece, and the response follows that shape."]],
        "tutorial": "The app includes a tutorial in four languages, from the first step to a first exercise.",
    },
    "fr": {
        "title": "Trioio — écoute, mémoire et réponse",
        "desc": "Trioio : un instrument qui écoute une référence autorisée, en reconnaît la pulsation, l'harmonie, la mélodie et le langage musical, et compose sa propre réponse. Avec le Troisième Œil.",
        "strip": "RASGO — famille d'instruments et de recherche sonore",
        "skip": "Aller au contenu",
        "langnav": "Langue",
        "pill": "Version 0.1.0 — aperçu, pour Windows, macOS et Linux",
        "lead": "Trioio est un instrument qui écoute. Vous apportez une référence — un de vos enregistrements, la sortie d'un autre instrument, une piste autorisée — et il en reconnaît la pulsation, l'harmonie, la mélodie et le langage musical. Puis il compose sa propre réponse, synthétisée de zéro, et apprend de votre jugement.",
        "s1": "Écoute et réponse",
        "steps": [
            ("Autoriser", "Chaque référence entre par la Bibliothèque avec sa provenance : qui l'a faite, à quel titre, pour quels usages. Sans autorisation, rien n'est analysé ni dérivé."),
            ("Écouter", "Trioio lit la pulsation et la mesure, les accords et la tonalité, la mélodie au sein du mixage et le langage — maqam, rāga, mode, flamenco… — grâce à l'archive de gammes qu'il a lui-même écrite."),
            ("Répondre", "En quatre modes — suivre, transfigurer, opposer, interrompre —, il compose motif, phrases, forme et harmonie, et les synthétise avec ses propres familles de timbre. Rien n'est échantillon : tout est synthèse."),
            ("Juger", "Vous acceptez, transformez ou rejetez la réponse, entière ou par passage. Le jugement devient mémoire : il attire ou éloigne des tendances, sans enfermer l'instrument dans un goût."),
        ],
        "s2": "Le Troisième Œil",
        "eye": "Le premier œil participe ; le deuxième observe ; le Troisième Œil perçoit la structure et peut la transformer. Dans Trioio, il intervient dans la réponse — un objet sonore, une règle cachée — et certaines interventions laissent des conséquences qui vont jusqu'à la fin. Il reste caché jusqu'à votre écoute : l'écoute précède l'explication.",
        "s3": "Langages et cultures",
        "lang": "Trioio porte une archive de gammes et de cycles qu'il a écrite lui-même à partir du savoir publié, en citant la source de chaque entrée comme dans une recherche — aucune archive de tiers n'est copiée.",
        "facts": [
            "2576 gammes : genres grecs, tempéraments historiques, intonation juste, maqamat, rāgas et melakartas, gamelan, modes du Nordeste brésilien, Messiaen et d'autres ;",
            "32 d'entre elles portent le comportement du langage : montée et descente, notes d'appui, phrases-signatures, cadences, ornements et texture ;",
            "26 cycles rythmiques : iqa', tala, usul et aksak, claves, colotomie du gamelan, compás flamenco, baião ;",
            "des mélanges entre cultures — collage, hybride, couches, transition — avec des coutures entre les sections.",
            "la machine harmonique : 25 langages — de l'harmonie fonctionnelle et du jazz aux langages brésiliens (Hermeto, Villa-Lobos, Moacir Santos, Guinga, Gismonti, frevo, maracatu), aux modernes et à la microtonalité de Partch —, avec conduite à quatre voix ;",
            "15 formations — choro, samba, jazz, forró, tango, rock, orchestre, a cappella… —, chaque voix avec l'instrument et la figure du style.",
        ],
        "example": "Exemple : dans un enregistrement avec la cadence andalouse (Rém–Do–Si♭–La), Trioio reconnaît La phrygien dominant et répond, selon la graine, en maqam Hijaz — hétérophonie, cycle maqsum — ou dans le mode de Mi andalou — accords, compás de 12. Dans le Hijaz, sans accords, le mouvement vient du sayr : la mélodie visite le maqam voisin sur le ghammaz et revient.",
        "s4": "L'interface",
        "shot_alt": "L'écran Écoute et réponse de Trioio : la référence avec les accords lus, le Troisième Œil révélé avec ses conséquences, et la réponse en sections avec le rouleau de notes.",
        "shot_cap": "L'écran Écoute et réponse : trois pistes alignées dans le temps — la référence, le Troisième Œil et la réponse —, la Bibliothèque, le langage de la réponse et la barre de jugement. En quatre langues, de 960×600 à 2560×1440.",
        "s5": "Provenance",
        "prov": "Chaque référence a un manifeste : origine, fondement, usages autorisés, historique. Trioio ne dérive que de ce qui a été autorisé à la dérivation, et chaque réponse enregistre d'où elle vient. Le code réutilisé d'autres instruments RASGO — Rasgo Synth, Rasgo Modular, Antitotem, Aquorbium — est une copie figée, avec le commit d'origine et les sommes de contrôle.",
        "s6": "Crédits",
        "credits": "Lúcio de Araújo — conception, composition, écoute, interface et code. Licence GNU AGPLv3 ou ultérieure ; JUCE sous sa voie AGPLv3. Sources et crédits complets :",
        "credits_link": "CREDITS_AND_SOURCES.md",
        "s7": "État",
        "state": "La version 0.1.0 est un aperçu. Les paquets Windows et macOS sont construits et testés par l'intégration continue, mais n'ont pas encore été ouverts par l'auteur sur de vraies machines ; Linux est aussi testé au lancement, sur quatre distributions. L'instrument reste en développement, ouvert.",
        "contact": "Contact",
        "footer_credits": "Crédits",
        "footer_license": "Licence AGPLv3+",
        "dialog_title": "CONTACT",
        "dialog_copy_hint": "Copiez l'adresse e-mail :",
        "dialog_copy": "Copier l'e-mail",
        "dialog_close": "Fermer la fenêtre de contact",
        "copied": "E-mail copié.",
        "fallback": "Sélectionnez et copiez l'adresse ci-dessus.",
        "s_dl": "Télécharger",
        "dl_intro": "La version 0.1.0 est un aperçu : l'instrument fait l'essentiel et reste ouvert, en développement. Choisissez le paquet de votre système.",
        "dl_os": {"Windows": "Windows 10 ou 11 — installateur (.exe) ou sans installation (.zip)", "macOS": "macOS 10.15 ou plus récent, Intel et Apple Silicon", "Linux": ".deb pour Ubuntu, Debian et Mint ; AppImage ou .tar.gz pour les autres distributions"},
        "dl_warn": "Au premier lancement, Windows et macOS affichent un avertissement de sécurité : c'est attendu, car l'app n'a pas la signature payante des deux entreprises. Le guide d'installation indique où cliquer.",
        "dl_install": "Guide d'installation",
        "dl_all": "Tous les paquets et les notes de version",
        "s_words": "Mots, influences et geste",
        "words": [["Mots", "Écrivez, si vous voulez, quelques mots sur la réponse — dans n'importe quelle langue, avec ou sans accents, avec des fautes de frappe. Trioio lit l'intention, la forme et la danse, et montre ce qu'il a compris. Vide, il suit ses propres critères."], ["Le réseau, facultatif", "Les mots que Trioio ne connaît pas peuvent être consultés dans des services publics (Wikidata, Datamuse). Le calcul est le sien : les services ne renvoient que des mots. Il est désactivé par défaut, car les mots quittent l'ordinateur."], ["L'arbre des influences", "Les styles ne sont pas des tiroirs : ce sont les branches d'un arbre ouvert, avec des contextes sociaux, historiques, technologiques et générationnels, et des traits qui sont des tendances, jamais des règles. Trioio croise des branches éloignées pour atteindre l'inattendu ; les poétiques d'artistes et de groupes, c'est le musicien qui les écrit."], ["Le geste", "Dessinez la courbe d'intensité de la pièce, et la réponse suit cette forme."]],
        "tutorial": "L'app comprend un tutoriel en quatre langues, du premier pas à un premier exercice.",
    },
    "es": {
        "title": "Trioio — escucha, memoria y respuesta",
        "desc": "Trioio: un instrumento que escucha una referencia autorizada, reconoce su pulso, armonía, melodía y lenguaje musical, y compone una respuesta propia. Con el Tercer Ojo.",
        "strip": "RASGO — familia de instrumentos e investigación sonora",
        "skip": "Saltar al contenido",
        "langnav": "Idioma",
        "pill": "Versión 0.1.0 — vista previa, para Windows, macOS y Linux",
        "lead": "Trioio es un instrumento que escucha. Traes una referencia — una grabación tuya, la salida de otro instrumento, una pista autorizada — y reconoce su pulso, armonía, melodía y lenguaje musical. Luego compone una respuesta propia, sintetizada desde cero, y aprende de tu juicio.",
        "s1": "Escucha y respuesta",
        "steps": [
            ("Autorizar", "Toda referencia entra por la Biblioteca con su procedencia: quién la hizo, con qué derecho, para qué usos. Sin autorización, nada se analiza ni se deriva."),
            ("Escuchar", "Trioio lee el pulso y el compás, los acordes y la tonalidad, la melodía dentro de la mezcla y el lenguaje — maqam, rāga, modo, flamenco… — mediante el archivo de escalas que él mismo escribió."),
            ("Responder", "En cuatro modos — seguir, transfigurar, contraponer, interrumpir —, compone motivo, frases, forma y armonía, y los sintetiza con familias de timbre propias. Nada es muestra: todo es síntesis."),
            ("Juzgar", "Aceptas, transformas o rechazas la respuesta, entera o por pasaje. El juicio se vuelve memoria: atrae o aleja tendencias, sin encerrar al instrumento en un gusto."),
        ],
        "s2": "El Tercer Ojo",
        "eye": "El primer ojo participa; el segundo observa; el Tercer Ojo percibe la estructura y puede transformarla. En Trioio interviene en la respuesta — un objeto sonoro, una regla oculta — y algunas intervenciones dejan consecuencias que siguen hasta el final. Permanece oculto hasta que escuchas: la escucha va antes que la explicación.",
        "s3": "Lenguajes y culturas",
        "lang": "Trioio lleva un archivo de escalas y ciclos escrito por él mismo a partir del conocimiento publicado, con la fuente citada en cada entrada, como en una investigación — no se copia ningún archivo de terceros.",
        "facts": [
            "2576 escalas: géneros griegos, temperamentos históricos, entonación justa, maqamat, rāgas y melakartas, gamelán, modos del Nordeste brasileño, Messiaen y otras;",
            "32 de ellas llevan el comportamiento del lenguaje: ascenso y descenso, notas de apoyo, frases-firma, cadencias, ornamentos y textura;",
            "26 ciclos rítmicos: iqa', tala, usul y aksak, claves, colotomía del gamelán, compás flamenco, baião;",
            "mezclas entre culturas — collage, híbrido, capas, transición — con costuras entre las secciones.",
            "la máquina armónica: 25 lenguajes — de la armonía funcional y el jazz a los brasileños (Hermeto, Villa-Lobos, Moacir Santos, Guinga, Gismonti, frevo, maracatu), los modernos y la microtonalidad de Partch —, con conducción a cuatro voces;",
            "15 conjuntos — choro, samba, jazz, forró, tango, rock, orquesta, a cappella… —, cada voz con el instrumento y la figura del estilo.",
        ],
        "example": "Ejemplo: en una grabación con la cadencia andaluza (Rem–Do–Si♭–La), Trioio reconoce La frigio dominante y responde, según la semilla, en maqam Hijaz — heterofonía, ciclo maqsum — o en el modo de Mi andaluz — acordes, compás de 12. En el Hijaz, sin acordes, el movimiento viene del sayr: la melodía visita el maqam vecino en el ghammaz y vuelve.",
        "s4": "La interfaz",
        "shot_alt": "La pantalla Escucha y respuesta de Trioio: la referencia con los acordes leídos, el Tercer Ojo revelado con sus consecuencias, y la respuesta en secciones con el rollo de notas.",
        "shot_cap": "La pantalla Escucha y respuesta: tres pistas alineadas en el tiempo — la referencia, el Tercer Ojo y la respuesta —, la Biblioteca, el lenguaje de la respuesta y la barra de juicio. En cuatro idiomas, de 960×600 a 2560×1440.",
        "s5": "Procedencia",
        "prov": "Cada referencia tiene un manifiesto: origen, fundamento, usos autorizados, historial. Trioio solo deriva de lo que se autorizó para derivar, y cada respuesta registra de dónde vino. El código reutilizado de otros instrumentos RASGO — Rasgo Synth, Rasgo Modular, Antitotem, Aquorbium — es una copia fijada, con commit de origen y sumas de verificación.",
        "s6": "Créditos",
        "credits": "Lúcio de Araújo — concepción, composición, escucha, interfaz y código. Licencia GNU AGPLv3 o posterior; JUCE bajo su vía AGPLv3. Fuentes y créditos completos:",
        "credits_link": "CREDITS_AND_SOURCES.md",
        "s7": "Estado",
        "state": "La versión 0.1.0 es una vista previa. Los paquetes de Windows y macOS se construyen y prueban en la integración continua, pero el autor aún no los abrió en máquinas reales; Linux también se prueba al abrir, en cuatro distribuciones. El instrumento sigue en desarrollo, abierto.",
        "contact": "Contacto",
        "footer_credits": "Créditos",
        "footer_license": "Licencia AGPLv3+",
        "dialog_title": "CONTACTO",
        "dialog_copy_hint": "Copia la dirección de correo:",
        "dialog_copy": "Copiar correo",
        "dialog_close": "Cerrar ventana de contacto",
        "copied": "Correo copiado.",
        "fallback": "Selecciona y copia la dirección de arriba.",
        "s_dl": "Descargar",
        "dl_intro": "La versión 0.1.0 es una vista previa: el instrumento hace lo esencial y sigue abierto, en desarrollo. Elige el paquete de tu sistema.",
        "dl_os": {"Windows": "Windows 10 u 11 — instalador (.exe) o sin instalar (.zip)", "macOS": "macOS 10.15 o más reciente, Intel y Apple Silicon", "Linux": ".deb para Ubuntu, Debian y Mint; AppImage o .tar.gz para las demás distribuciones"},
        "dl_warn": "En la primera apertura, Windows y macOS muestran un aviso de seguridad: es lo esperado, porque la app no lleva la firma de pago de las dos empresas. La guía de instalación dice dónde hacer clic.",
        "dl_install": "Guía de instalación",
        "dl_all": "Todos los paquetes y las notas de la versión",
        "s_words": "Palabras, influencias y gesto",
        "words": [["Palabras", "Escribe, si quieres, algunas palabras sobre la respuesta — en cualquier idioma, con o sin acentos, con errores de tecleo. Trioio lee la intención, la forma y la danza, y muestra lo que entendió. En blanco, sigue sus propios criterios."], ["La red, opcional", "Las palabras que Trioio no conoce pueden consultarse en servicios públicos (Wikidata, Datamuse). El cálculo es suyo: los servicios solo devuelven palabras. Viene desactivada, porque las palabras salen del ordenador."], ["El árbol de influencias", "Los estilos no son cajones: son ramas de un árbol abierto, con contextos sociales, históricos, tecnológicos y de generación, y rasgos que son tendencias, nunca reglas. Trioio cruza ramas lejanas para llegar a lo inesperado; las poéticas de artistas y bandas las escribe el músico."], ["El gesto", "Dibuja la curva de intensidad de la pieza, y la respuesta sigue esa forma."]],
        "tutorial": "La app trae un tutorial en cuatro idiomas, del primer paso a un primer ejercicio.",
    },
}


def pagina(arquivo, lang):
    t = T[lang]
    e = lambda s: html.escape(s, quote=False)
    a = lambda s: html.escape(s, quote=True)
    langs = "\n".join('      <a href="%s"%s>%s</a>' % (f, ' aria-current="true"' if f == arquivo else "", c) for f, _, c in PAGINAS)
    passos = "\n".join('    <li><h3>%s</h3><p>%s</p></li>' % (e(h), e(p)) for h, p in t["steps"])
    fatos = "\n".join("    <li>%s</li>" % e(f) for f in t["facts"])
    pacotes = "\n".join(
        '    <li><h3>%s</h3><p>%s</p><p class="dl-links">%s</p></li>' % (
            e(so), e(t["dl_os"][so]), " ".join('<a class="button" href="%s%s">%s</a>' % (DL, a(f), e(rot)) for f, rot in arqs))
        for so, arqs in PACOTES)
    palavras = "\n".join('    <li><h3>%s</h3><p>%s</p></li>' % (e(h), e(p)) for h, p in t["words"])
    return f"""<!doctype html>
<html lang="{lang}">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>{e(t["title"])}</title>
<meta name="description" content="{a(t["desc"])}" />
<link rel="icon" href="assets/identity/trioio-olho.svg" type="image/svg+xml" />
<link rel="stylesheet" href="styles.css" />
<script src="assets/contact.js" defer></script>
</head>
<body>
<div class="rasgo-strip"><a href="{PORTAL}">{e(t["strip"])}</a></div>
<a class="skip-link" href="#conteudo">{e(t["skip"])}</a>

<div class="site-header">
  <a class="brand" href="#top" aria-label="Trioio">
    <img src="assets/identity/trioio-olho.svg" alt="" width="36" height="36" />
    <span>TRIOIO</span>
  </a>
  <nav class="lang-switch" aria-label="{a(t["langnav"])}">
{langs}
  </nav>
</div>

<main id="conteudo">
<section class="hero" id="top">
  <span class="status-pill">{e(t["pill"])}</span>
  <h1>Trioio</h1>
  <p class="lead">{e(t["lead"])}</p>
</section>

<section class="section">
  <h2>{e(t["s4"])}</h2>
  <figure class="shot">
    <img src="assets/interface.jpg" alt="{a(t["shot_alt"])}" width="1920" height="1080" loading="lazy" />
    <figcaption>{e(t["shot_cap"])}</figcaption>
  </figure>
</section>

<section class="section section-dark download" id="baixar">
  <h2>{e(t["s_dl"])}</h2>
  <p>{e(t["dl_intro"])}</p>
  <ul class="steps downloads">
{pacotes}
  </ul>
  <p class="example">{e(t["dl_warn"])}</p>
  <div class="contact-row">
    <a class="button button-primary" href="{BLOB}/INSTALL.md">{e(t["dl_install"])}</a>
    <a class="button" href="{RELEASE}">{e(t["dl_all"])}</a>
  </div>
</section>

<section class="section">
  <h2>{e(t["s1"])}</h2>
  <ol class="steps">
{passos}
  </ol>
</section>

<section class="section eye-section">
  <h2>{e(t["s2"])}</h2>
  <p>{e(t["eye"])}</p>
</section>

<section class="section section-dark">
  <h2>{e(t["s3"])}</h2>
  <p>{e(t["lang"])}</p>
  <ul class="facts">
{fatos}
  </ul>
  <p class="example">{e(t["example"])}</p>
</section>

<section class="section">
  <h2>{e(t["s_words"])}</h2>
  <ul class="steps">
{palavras}
  </ul>
  <p>{e(t["tutorial"])}</p>
</section>

<section class="section section-dark">
  <h2>{e(t["s5"])}</h2>
  <p>{e(t["prov"])}</p>
</section>

<section class="section">
  <h2>{e(t["s6"])}</h2>
  <p>{e(t["credits"])} <a href="{BLOB}/CREDITS_AND_SOURCES.md">{e(t["credits_link"])}</a>.</p>
</section>

<section class="section section-dark">
  <h2>{e(t["s7"])}</h2>
  <p>{e(t["state"])}</p>
  <div class="contact-row">
    <button type="button" class="button button-primary" data-contact-trigger>{e(t["contact"])}</button>
    <a class="button" href="{REPO}">GitHub</a>
  </div>
</section>
</main>

<footer class="site-footer">
  <span>© Lúcio de Araújo · Trioio 2026 · RASGO</span>
  <nav>
    <a href="{REPO}">GitHub</a> ·
    <a href="{BLOB}/CREDITS_AND_SOURCES.md">{e(t["footer_credits"])}</a> ·
    <a href="{BLOB}/LICENSE">{e(t["footer_license"])}</a>
  </nav>
</footer>
<dialog class="contact-dialog" data-contact-dialog aria-labelledby="contact-title">
  <button class="dialog-close" type="button" data-contact-close aria-label="{a(t["dialog_close"])}">×</button>
  <h2 id="contact-title">{e(t["dialog_title"])}</h2>
  <p>{e(t["dialog_copy_hint"])}</p>
  <code data-contact-address></code>
  <button class="button button-primary" type="button" data-contact-copy>{e(t["dialog_copy"])}</button>
  <p class="contact-feedback" data-contact-feedback data-copied="{a(t["copied"])}" data-fallback="{a(t["fallback"])}" aria-live="polite"></p>
</dialog>
</body>
</html>
"""


def main():
    for arquivo, lang, _ in PAGINAS:
        caminho = AQUI / arquivo
        novo = pagina(arquivo, lang)
        caminho.write_text(novo, encoding="utf-8")
    print("gerar_paginas.py: %d páginas" % len(PAGINAS))


if __name__ == "__main__":
    main()
