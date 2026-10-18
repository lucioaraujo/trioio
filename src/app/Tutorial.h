#pragma once

// O TUTORIAL do Trioio (2026-10-18, o autor: "não criou o tutorial ainda",
// "nem os idiomas"). O padrão da família RASGO (Antitotem, Navalha 2): um
// cartão com o sumário dos capítulos à esquerda, o texto com rolagem, a DICA
// num quadro à parte, anterior/próximo; abre na língua do instrumento (sem
// botão de idioma próprio). O conteúdo segue docs/COMO_TOCAR.md e os nomes
// da interface (UiLanguage.h), nas quatro línguas.

#include "UiLanguage.h"

#include <array>

namespace trioio::app::tutorial
{

struct Chapter
{
    L4 title, body, tip;
};

inline const L4 openButton { "TUTORIAL", "TUTORIAL", "TUTORIEL", "TUTORIAL" };
inline const L4 contents { "CONTENTS", "SUMÁRIO", "SOMMAIRE", "SUMARIO" };
inline const L4 tipHeading { "TIP", "DICA", "ASTUCE", "CONSEJO" };
inline const L4 previous { "< previous", "< anterior", "< précédent", "< anterior" };
inline const L4 next { "next >", "próximo >", "suivant >", "siguiente >" };

inline const std::array<Chapter, 11> chapters {{
    // 1
    { { "What playing Trioio means", "O que é tocar o Trioio", "Ce que veut dire jouer Trioio", "Qué es tocar Trioio" },
      { "Trioio is not played note by note. It is a conversation in four moves:\n\n"
        "1. You bring a recording: the reference.\n"
        "2. Trioio listens to it and composes a response: a new file, entirely synthesised, that never contains the reference's audio.\n"
        "3. You listen and judge: the whole response, one section or a stretch.\n"
        "4. Trioio learns from your judgements. The next response changes because of your listening.\n\n"
        "Playing means choosing the reference, the mode, the language and what counts. The instrument composes; you lead by listening.",
        "O Trioio não se toca nota a nota. É uma conversa em quatro tempos:\n\n"
        "1. Você traz uma gravação: a referência.\n"
        "2. O Trioio a escuta e compõe uma resposta: um arquivo novo, todo sintetizado, que nunca contém o áudio da referência.\n"
        "3. Você ouve e julga: a resposta inteira, uma seção ou um trecho.\n"
        "4. O Trioio aprende com os seus julgamentos. A próxima resposta muda por causa da sua escuta.\n\n"
        "Tocar é escolher a referência, o modo, a linguagem e o que vale. O instrumento compõe; você conduz pela escuta.",
        "Trioio ne se joue pas note à note. C'est une conversation en quatre temps :\n\n"
        "1. Vous apportez un enregistrement : la référence.\n"
        "2. Trioio l'écoute et compose une réponse : un fichier nouveau, entièrement synthétisé, qui ne contient jamais l'audio de la référence.\n"
        "3. Vous écoutez et jugez : la réponse entière, une section ou un passage.\n"
        "4. Trioio apprend de vos jugements. La réponse suivante change à cause de votre écoute.\n\n"
        "Jouer, c'est choisir la référence, le mode, le langage et ce qui compte. L'instrument compose ; vous conduisez par l'écoute.",
        "Trioio no se toca nota a nota. Es una conversación en cuatro tiempos:\n\n"
        "1. Traes una grabación: la referencia.\n"
        "2. Trioio la escucha y compone una respuesta: un archivo nuevo, todo sintetizado, que nunca contiene el audio de la referencia.\n"
        "3. Escuchas y juzgas: la respuesta entera, una sección o un fragmento.\n"
        "4. Trioio aprende de tus juicios. La próxima respuesta cambia por tu escucha.\n\n"
        "Tocar es elegir la referencia, el modo, el lenguaje y lo que vale. El instrumento compone; tú conduces con la escucha." },
      { "Leaving every choice as it is is also a choice: Trioio then decides from what it hears.",
        "Deixar tudo como está também é uma escolha: o Trioio decide a partir da escuta.",
        "Tout laisser tel quel est aussi un choix : Trioio décide alors à partir de l'écoute.",
        "Dejar todo como está también es una elección: Trioio decide a partir de la escucha." } },
    // 2
    { { "Bring and authorise a recording", "Trazer e autorizar uma gravação", "Apporter et autoriser un enregistrement", "Traer y autorizar una grabación" },
      { "1. Click \"choose a reference\": it takes you to the Library.\n"
        "2. Click \"bring a recording…\" and pick the file.\n"
        "3. A new recording starts as pending. Click \"authorise…\" and state its origin (your own, a capture, another instrument, AI, licensed, public domain…), with what right it may be used and who declares it.\n"
        "4. Click \"authorise\".\n\n"
        "Without authorisation Trioio does not compose on a recording. This protects provenance: every response keeps the record of where its reference came from.\n\n"
        "If the app opens in demonstration mode, use \"open data folder…\" to choose where Trioio keeps its data.",
        "1. Clique em \"escolher referência\": ele leva à Biblioteca.\n"
        "2. Clique em \"trazer gravação…\" e escolha o arquivo.\n"
        "3. Uma gravação nova começa pendente. Clique em \"autorizar…\" e diga a origem (autoral, captura, outro instrumento, IA, licenciado, domínio público…), com que direito ela pode ser usada e quem declara.\n"
        "4. Clique em \"autorizar\".\n\n"
        "Sem autorização, o Trioio não compõe sobre a gravação. Isso protege a proveniência: cada resposta guarda o registro de onde veio a sua referência.\n\n"
        "Se o app abrir em demonstração, use \"abrir pasta de dados…\" para escolher onde o Trioio guarda os dados.",
        "1. Cliquez sur « choisir une référence » : cela mène à la Bibliothèque.\n"
        "2. Cliquez sur « apporter un enregistrement… » et choisissez le fichier.\n"
        "3. Un nouvel enregistrement commence en attente. Cliquez sur « autoriser… » et indiquez son origine (personnelle, captation, autre instrument, IA, sous licence, domaine public…), avec quel droit il peut être utilisé et qui le déclare.\n"
        "4. Cliquez sur « autoriser ».\n\n"
        "Sans autorisation, Trioio ne compose pas sur l'enregistrement. Cela protège la provenance : chaque réponse garde la trace de l'origine de sa référence.\n\n"
        "Si l'app s'ouvre en démonstration, utilisez « ouvrir le dossier de données… » pour choisir où Trioio garde ses données.",
        "1. Haz clic en «elegir una referencia»: lleva a la Biblioteca.\n"
        "2. Haz clic en «traer una grabación…» y elige el archivo.\n"
        "3. Una grabación nueva empieza pendiente. Haz clic en «autorizar…» e indica su origen (propia, captura, otro instrumento, IA, con licencia, dominio público…), con qué derecho puede usarse y quién lo declara.\n"
        "4. Haz clic en «autorizar».\n\n"
        "Sin autorización, Trioio no compone sobre la grabación. Eso protege la procedencia: cada respuesta guarda el registro de dónde vino su referencia.\n\n"
        "Si la app abre en demostración, usa «abrir carpeta de datos…» para elegir dónde Trioio guarda sus datos." },
      { "Start with a short recording of your own, 30 s to 2 min: the first listening is quicker and you know the material.",
        "Comece com uma gravação curta e sua, de 30 s a 2 min: a primeira escuta é mais rápida e você conhece a matéria.",
        "Commencez par un enregistrement court et à vous, de 30 s à 2 min : la première écoute est plus rapide et vous connaissez la matière.",
        "Empieza con una grabación corta y tuya, de 30 s a 2 min: la primera escucha es más rápida y conoces el material." } },
    // 3
    { { "Listen to the reference", "Ouvir a referência", "Écouter la référence", "Escuchar la referencia" },
      { "On the \"Listening and response\" page, click \"▶ reference\".\n\n"
        "The first analysis takes a little while; the status line says Trioio is listening. When it is done, the REFERENCE strip shows the sections Trioio read: where the material changes, the pulse, the key and the language it recognised.\n\n"
        "This reading is what the response will talk to. It is kept in the data folder, so the next time it is instant.",
        "Na aba \"Escuta e resposta\", clique em \"▶ referência\".\n\n"
        "A primeira análise demora um pouco; a linha de estado diz que o Trioio está ouvindo. Quando termina, a faixa REFERÊNCIA mostra as seções que o Trioio leu: onde a matéria muda, o pulso, a tonalidade e a linguagem que ele reconheceu.\n\n"
        "É com essa leitura que a resposta vai conversar. Ela fica guardada na pasta de dados, e da próxima vez é instantânea.",
        "Dans la page « Écoute et réponse », cliquez sur « ▶ référence ».\n\n"
        "La première analyse prend un peu de temps ; la ligne d'état indique que Trioio écoute. Ensuite, la bande RÉFÉRENCE montre les sections que Trioio a lues : où la matière change, la pulsation, la tonalité et le langage reconnu.\n\n"
        "C'est avec cette lecture que la réponse dialoguera. Elle reste dans le dossier de données ; la fois suivante, elle est instantanée.",
        "En la página «Escucha y respuesta», haz clic en «▶ referencia».\n\n"
        "El primer análisis tarda un poco; la línea de estado dice que Trioio está escuchando. Al terminar, la franja REFERENCIA muestra las secciones que Trioio leyó: dónde cambia el material, el pulso, la tonalidad y el lenguaje que reconoció.\n\n"
        "Con esa lectura conversará la respuesta. Queda guardada en la carpeta de datos, y la próxima vez es instantánea." },
      { "", "", "", "" } },
    // 4
    { { "Choose the mode", "Escolher o modo", "Choisir le mode", "Elegir el modo" },
      { "The mode is the attitude of the response:\n\n"
        "• follow: it goes along with the reference, same field, same pulse;\n"
        "• transfigure: the same matter, another figure;\n"
        "• oppose: it goes against it, another register, another density;\n"
        "• interrupt: it cuts in, enters and leaves.\n\n"
        "The four buttons sit at the top of the window. The mode also decides how a style read in the reference is answered: follow plays in that style, transfigure goes to a neighbour, oppose to its contrary.",
        "O modo é a atitude da resposta:\n\n"
        "• seguir: acompanha a referência, mesmo campo, mesmo pulso;\n"
        "• transfigurar: a mesma matéria, outra figura;\n"
        "• contrapor: vai contra, outro registro, outra densidade;\n"
        "• interromper: corta, entra e sai.\n\n"
        "Os quatro botões ficam no alto da janela. O modo também decide como responder a um estilo lido na referência: seguir toca no estilo, transfigurar vai a um vizinho, contrapor vai ao contrário.",
        "Le mode est l'attitude de la réponse :\n\n"
        "• suivre : elle accompagne la référence, même champ, même pulsation ;\n"
        "• transfigurer : la même matière, une autre figure ;\n"
        "• opposer : elle va contre, autre registre, autre densité ;\n"
        "• interrompre : elle coupe, entre et sort.\n\n"
        "Les quatre boutons sont en haut de la fenêtre. Le mode décide aussi comment répondre à un style lu dans la référence : suivre joue dans ce style, transfigurer va vers un voisin, opposer vers son contraire.",
        "El modo es la actitud de la respuesta:\n\n"
        "• seguir: acompaña la referencia, mismo campo, mismo pulso;\n"
        "• transfigurar: la misma materia, otra figura;\n"
        "• contraponer: va en contra, otro registro, otra densidad;\n"
        "• interrumpir: corta, entra y sale.\n\n"
        "Los cuatro botones están arriba de la ventana. El modo también decide cómo responder a un estilo leído en la referencia: seguir toca en ese estilo, transfigurar va a un vecino, contraponer a su contrario." },
      { "Compose the same reference in follow and then in oppose: the difference between the two teaches more than any description.",
        "Componha a mesma referência em seguir e depois em contrapor: a diferença entre as duas ensina mais que qualquer descrição.",
        "Composez la même référence en suivre puis en opposer : la différence entre les deux en apprend plus que toute description.",
        "Compón la misma referencia en seguir y luego en contraponer: la diferencia entre las dos enseña más que cualquier descripción." } },
    // 5
    { { "Compose", "Compor", "Composer", "Componer" },
      { "Click \"compose again\".\n\n"
        "While Trioio works, a bar at the bottom of the window shows each stage (listening, reading the choices, the network when it is on, the score, the voices one by one, the master, saving) and the time spent. A response takes from a few seconds to about a minute, depending on its length and on the ensemble.\n\n"
        "The response appears in the RESPONSE strip. Each new response is a new version: nothing is overwritten, the earlier ones stay in \"Your listening\".",
        "Clique em \"compor de novo\".\n\n"
        "Enquanto o Trioio trabalha, uma barra no rodapé da janela mostra cada etapa (a escuta, as escolhas, a rede quando está ligada, a partitura, as vozes uma a uma, o master, a gravação) e o tempo decorrido. Uma resposta leva de alguns segundos a cerca de um minuto, conforme a duração e o conjunto.\n\n"
        "A resposta aparece na faixa RESPOSTA. Cada resposta nova é uma versão nova: nada é sobrescrito, as anteriores ficam em \"Sua escuta\".",
        "Cliquez sur « composer à nouveau ».\n\n"
        "Pendant que Trioio travaille, une barre en bas de la fenêtre montre chaque étape (l'écoute, les choix, le réseau quand il est activé, la partition, les voix une à une, le master, l'enregistrement) et le temps écoulé. Une réponse prend de quelques secondes à environ une minute, selon sa durée et la formation.\n\n"
        "La réponse apparaît dans la bande RÉPONSE. Chaque nouvelle réponse est une nouvelle version : rien n'est écrasé, les précédentes restent dans « Votre écoute ».",
        "Haz clic en «componer de nuevo».\n\n"
        "Mientras Trioio trabaja, una barra al pie de la ventana muestra cada etapa (la escucha, las elecciones, la red cuando está activada, la partitura, las voces una a una, el máster, la grabación) y el tiempo transcurrido. Una respuesta tarda de unos segundos a cerca de un minuto, según su duración y el conjunto.\n\n"
        "La respuesta aparece en la franja RESPUESTA. Cada respuesta nueva es una versión nueva: nada se sobrescribe, las anteriores quedan en «Tu escucha»." },
      { "", "", "", "" } },
    // 6
    { { "Listen, and the Third Eye", "Ouvir, e o Terceiro Olho", "Écouter, et le Troisième Œil", "Escuchar, y el Tercer Ojo" },
      { "• ▶ plays the response.\n"
        "• \"▶ together\" plays the reference and the response at the same time, for listening only.\n"
        "• The master sets the volume of everything.\n\n"
        "The Third Eye is Trioio's unpredictable part: the gestures, the deviations, the choices that do not come from the reference. It stays hidden until you have listened, so it does not steer your ear. Then click \"reveal after listening\" to see what it did and where.\n\n"
        "The amount of unpredictability is set in the Language panel.",
        "• ▶ toca a resposta.\n"
        "• \"▶ juntos\" toca a referência e a resposta ao mesmo tempo, só para ouvir.\n"
        "• O master regula o volume de tudo.\n\n"
        "O Terceiro Olho é a parte imprevisível do Trioio: os gestos, os desvios, as escolhas que não vêm da referência. Ele fica escondido até você ouvir, para não guiar a sua escuta. Depois, clique em \"revelar depois de ouvir\" para ver o que ele fez e onde.\n\n"
        "A quantidade de imprevisibilidade se regula no painel Linguagem.",
        "• ▶ joue la réponse.\n"
        "• « ▶ ensemble » joue la référence et la réponse en même temps, pour l'écoute seulement.\n"
        "• Le master règle le volume de l'ensemble.\n\n"
        "Le Troisième Œil est la part imprévisible de Trioio : les gestes, les écarts, les choix qui ne viennent pas de la référence. Il reste caché jusqu'à ce que vous ayez écouté, pour ne pas guider votre oreille. Ensuite, cliquez sur « révéler après l'écoute » pour voir ce qu'il a fait et où.\n\n"
        "La part d'imprévisibilité se règle dans le panneau Langage.",
        "• ▶ toca la respuesta.\n"
        "• «▶ juntos» toca la referencia y la respuesta a la vez, solo para escuchar.\n"
        "• El máster regula el volumen de todo.\n\n"
        "El Tercer Ojo es la parte imprevisible de Trioio: los gestos, los desvíos, las elecciones que no vienen de la referencia. Queda oculto hasta que escuchas, para no guiar tu oído. Después, haz clic en «revelar tras escuchar» para ver qué hizo y dónde.\n\n"
        "La cantidad de imprevisibilidad se regula en el panel Lenguaje." },
      { "Listen once without revealing, judge, and only then reveal: you will learn what the Eye does by ear before reading it.",
        "Ouça uma vez sem revelar, julgue e só então revele: você aprende o que o Olho faz pelo ouvido antes de ler.",
        "Écoutez une fois sans révéler, jugez, et seulement ensuite révélez : vous apprendrez ce que fait l'Œil à l'oreille avant de le lire.",
        "Escucha una vez sin revelar, juzga y solo entonces revela: aprenderás lo que hace el Ojo de oído antes de leerlo." } },
    // 7
    { { "Judge", "Julgar", "Juger", "Juzgar" },
      { "First choose what you judge:\n\n"
        "• click outside the selection: the whole response;\n"
        "• click a block of the RESPONSE strip: that section;\n"
        "• drag across the RESPONSE strip: the marked stretch.\n\n"
        "Write a note if you like: your sentence about that listening, such as \"the chip sound got too hard\". It is kept with the judgement and shown in \"Your listening\".\n\n"
        "Then click accept, transform or reject. The judgement weighs on the next responses, and on the rules of the game: accepted rules stay in memory, rejected ones go to the compost.",
        "Primeiro escolha o que julgar:\n\n"
        "• clicar fora da seleção: a resposta inteira;\n"
        "• clicar num bloco da faixa RESPOSTA: aquela seção;\n"
        "• arrastar sobre a faixa RESPOSTA: o trecho marcado.\n\n"
        "Escreva uma nota se quiser: a sua frase sobre aquela escuta, como \"o chip ficou duro demais\". Ela fica com o julgamento e aparece em \"Sua escuta\".\n\n"
        "Depois clique em aceitar, transformar ou rejeitar. O julgamento pesa nas próximas respostas e nas regras do jogo: as regras aceitas ficam na memória, as rejeitadas vão para o adubo.",
        "Choisissez d'abord ce que vous jugez :\n\n"
        "• cliquer hors de la sélection : la réponse entière ;\n"
        "• cliquer sur un bloc de la bande RÉPONSE : cette section ;\n"
        "• glisser sur la bande RÉPONSE : le passage marqué.\n\n"
        "Écrivez une note si vous voulez : votre phrase sur cette écoute, comme « le son chip est devenu trop dur ». Elle est gardée avec le jugement et apparaît dans « Votre écoute ».\n\n"
        "Puis cliquez sur accepter, transformer ou rejeter. Le jugement pèse sur les réponses suivantes et sur les règles du jeu : les règles acceptées restent en mémoire, les rejetées vont au compost.",
        "Primero elige qué juzgar:\n\n"
        "• hacer clic fuera de la selección: la respuesta entera;\n"
        "• hacer clic en un bloque de la franja RESPUESTA: esa sección;\n"
        "• arrastrar sobre la franja RESPUESTA: el fragmento marcado.\n\n"
        "Escribe una nota si quieres: tu frase sobre esa escucha, como «el chip quedó demasiado duro». Queda con el juicio y aparece en «Tu escucha».\n\n"
        "Luego haz clic en aceptar, transformar o rechazar. El juicio pesa en las próximas respuestas y en las reglas del juego: las aceptadas quedan en la memoria, las rechazadas van al abono." },
      { "Judging a stretch is more useful than judging everything: Trioio learns which choice worked where.",
        "Julgar um trecho ensina mais que julgar tudo: o Trioio aprende qual escolha funcionou onde.",
        "Juger un passage apprend davantage que juger le tout : Trioio apprend quel choix a fonctionné où.",
        "Juzgar un fragmento enseña más que juzgar todo: Trioio aprende qué elección funcionó dónde." } },
    // 8
    { { "Compare versions", "Comparar versões", "Comparer des versions", "Comparar versiones" },
      { "On the \"Your listening\" page:\n\n"
        "1. Click a version in the list: it becomes A.\n"
        "2. Click another: it becomes the new A, and the previous one becomes B. Green and blue badges show which is which.\n"
        "3. Use \"▶ compare A × B\" and \"switch A ↔ B\". Switching jumps to the other version at the same moment of the music.\n\n"
        "To the right you see what your listening taught (how much each choice rose or fell with your judgements), the rules of the game in version A, and the box \"the next response takes the listening into account\".",
        "Na aba \"Sua escuta\":\n\n"
        "1. Clique numa versão da lista: ela vira o A.\n"
        "2. Clique em outra: ela vira o novo A, e a anterior vira o B. Selos verde e azul mostram quem é quem.\n"
        "3. Use \"▶ comparar A × B\" e \"alternar A ↔ B\". Alternar troca de versão no mesmo instante da música.\n\n"
        "À direita ficam o que a escuta ensinou (o quanto cada escolha subiu ou desceu com os seus julgamentos), as regras do jogo na versão A e a caixa \"a próxima resposta leva a escuta em conta\".",
        "Dans la page « Votre écoute » :\n\n"
        "1. Cliquez sur une version de la liste : elle devient A.\n"
        "2. Cliquez sur une autre : elle devient le nouveau A, et la précédente devient B. Des pastilles verte et bleue montrent qui est qui.\n"
        "3. Utilisez « ▶ comparer A × B » et « alterner A ↔ B ». Alterner change de version au même instant de la musique.\n\n"
        "À droite : ce que l'écoute a appris (combien chaque choix a monté ou baissé avec vos jugements), les règles du jeu dans la version A et la case « la prochaine réponse tient compte de l'écoute ».",
        "En la página «Tu escucha»:\n\n"
        "1. Haz clic en una versión de la lista: se vuelve A.\n"
        "2. Haz clic en otra: se vuelve el nuevo A, y la anterior pasa a B. Sellos verde y azul muestran cuál es cuál.\n"
        "3. Usa «▶ comparar A × B» y «alternar A ↔ B». Alternar cambia de versión en el mismo instante de la música.\n\n"
        "A la derecha están lo que enseñó la escucha (cuánto subió o bajó cada elección con tus juicios), las reglas del juego en la versión A y la casilla «la próxima respuesta tiene en cuenta la escucha»." },
      { "", "", "", "" } },
    // 9
    { { "The Language panel", "O painel Linguagem", "Le panneau Langage", "El panel Lenguaje" },
      { "\"Language ▸\" opens the choices. All of them are optional, and each one cites its source in the form of the response.\n\n"
        "• Mixture: scales to combine, by name or by family, separated by + or commas (\"rāga Yaman + maqam Hijaz\", \"Greek modes, African scales\").\n"
        "• Pitches, technique, seam, cycle, voicing, intention, form, tempo.\n"
        "• Ensemble: the instruments of each voice by style (choro, samba, jazz, forró, tango, rock, orchestra…), or read from the reference.\n"
        "• Harmony: the language of the harmony machine.\n"
        "• Unpredictability (the Third Eye) and the modular tear (from the instrument to the unheard).\n"
        "• Dialogue (answer in the pauses), game (Trioio invents its own rules), film effects, the sound of the aura, stems.",
        "\"Linguagem ▸\" abre as escolhas. Todas são opcionais, e cada uma cita a fonte na forma da resposta.\n\n"
        "• Mistura: escalas para combinar, pelo nome ou pela família, separadas por + ou vírgula (\"rāga Yaman + maqam Hijaz\", \"modos gregos, escalas africanas\").\n"
        "• Alturas, técnica, costura, ciclo, voicing, intenção, forma, andamento.\n"
        "• Conjunto: os instrumentos de cada voz pelo estilo (choro, samba, jazz, forró, tango, rock, orquestra…), ou lido na referência.\n"
        "• Harmonia: a linguagem da máquina de harmonia.\n"
        "• Imprevisibilidade (o Terceiro Olho) e o rasgo modular (do instrumento ao inaudito).\n"
        "• Diálogo (responder nas pausas), jogo (o Trioio inventa as próprias regras), efeitos de cinema, som da aura, stems.",
        "« Langage ▸ » ouvre les choix. Tous sont facultatifs, et chacun cite sa source dans la forme de la réponse.\n\n"
        "• Mélange : des gammes à combiner, par nom ou par famille, séparées par + ou des virgules (« rāga Yaman + maqam Hijaz », « modes grecs, gammes africaines »).\n"
        "• Hauteurs, technique, couture, cycle, voicing, intention, forme, tempo.\n"
        "• Formation : les instruments de chaque voix par style (choro, samba, jazz, forró, tango, rock, orchestre…), ou lue dans la référence.\n"
        "• Harmonie : le langage de la machine harmonique.\n"
        "• Imprévisibilité (le Troisième Œil) et la déchirure modulaire (de l'instrument à l'inouï).\n"
        "• Dialogue (répondre dans les pauses), jeu (Trioio invente ses règles), effets de cinéma, son de l'aura, stems.",
        "«Lenguaje ▸» abre las elecciones. Todas son opcionales, y cada una cita su fuente en la forma de la respuesta.\n\n"
        "• Mezcla: escalas para combinar, por nombre o por familia, separadas por + o comas («rāga Yaman + maqam Hijaz», «modos griegos, escalas africanas»).\n"
        "• Alturas, técnica, costura, ciclo, voicing, intención, forma, tempo.\n"
        "• Conjunto: los instrumentos de cada voz por estilo (choro, samba, jazz, forró, tango, rock, orquesta…), o leído en la referencia.\n"
        "• Armonía: el lenguaje de la máquina armónica.\n"
        "• Imprevisibilidad (el Tercer Ojo) y el rasgo modular (del instrumento a lo inaudito).\n"
        "• Diálogo (responder en las pausas), juego (Trioio inventa sus reglas), efectos de cine, sonido del aura, stems." },
      { "Change one choice at a time between two responses: that way you hear what each one does.",
        "Mude uma escolha de cada vez entre duas respostas: assim você ouve o que cada uma faz.",
        "Changez un seul choix à la fois entre deux réponses : vous entendrez ce que fait chacun.",
        "Cambia una sola elección por vez entre dos respuestas: así oyes lo que hace cada una." } },
    // 10
    { { "Words, influences and gesture", "Palavras, influências e gesto", "Mots, influences et geste", "Palabras, influencias y gesto" },
      { "Words: write a few words about the response you want, in any language (\"dançante, maracatu, virada\", \"cold rain, slow\"). Trioio reads intention, form, dance and the nodes of the influence tree, and shows in the form what it understood and what it did not. Left empty, Trioio follows its own criteria. Accents, capitals, punctuation and small typing mistakes are tolerated.\n\n"
        "The box \"look up unread words online\" sends only the words Trioio did not read to public services (Wikidata, Datamuse) and calculates from their answers. It is off by default, because the words leave the computer; answers are remembered, so the second time is instant.\n\n"
        "Influences: nodes of the tree (rock/noise, brasil/nordeste/maracatu) or auto; the strength goes from the reference (0) to the tree (1). Without: roles to leave out (percussion, bass).\n\n"
        "Gesture: draw the intensity curve of the piece, or write it as position:intensity.\n\n"
        "Artists and bands: Trioio does not bring the poetics of artists and bands; it does not invent how someone composes. A name written in Words only reaches the network as loose words. To make an artist an influence, write it yourself, with your reading and your sources, in the file influencias_do_autor.txt in the data folder (Trioio creates it with a commented example). The line \"também:\" gives the names as you write them, for example \"também: júpiter maçã\"; from then on the name calls that node in Words and in Influences.",
        "Palavras: escreva algumas palavras sobre a resposta que você quer, em qualquer língua (\"dançante, maracatu, virada\", \"chuva fria, lento\"). O Trioio lê a intenção, a forma, a dança e os nós da árvore de influências, e mostra na forma o que entendeu e o que não entendeu. Vazio, o Trioio segue os seus critérios. Acentos, maiúsculas, pontuação e pequenos erros de digitação são tolerados.\n\n"
        "A caixa \"consultar na rede as palavras não lidas\" manda só as palavras que o Trioio não leu para serviços públicos (Wikidata, Datamuse) e calcula a partir das respostas. Vem desligada, porque as palavras saem do computador; as respostas ficam na memória, e da segunda vez é instantâneo.\n\n"
        "Influências: nós da árvore (rock/noise, brasil/nordeste/maracatu) ou auto; a força vai da referência (0) à árvore (1). Sem: os papéis que saem (percussão, baixo).\n\n"
        "Gesto: desenhe a curva de intensidade da peça, ou escreva como posição:intensidade.\n\n"
        "Artistas e bandas: o Trioio não traz a poética de artistas e bandas; ele não inventa como alguém compõe. Um nome escrito em Palavras só chega à rede como palavras soltas. Para fazer de um artista uma influência, escreva-o você, com a sua leitura e as suas fontes, no arquivo influencias_do_autor.txt da pasta de dados (o Trioio o cria com um exemplo comentado). A linha \"também:\" dá os nomes como você os escreve, por exemplo \"também: júpiter maçã\"; daí em diante o nome chama esse nó nas Palavras e nas Influências.",
        "Mots : écrivez quelques mots sur la réponse que vous voulez, dans n'importe quelle langue (« dançante, maracatu, virada », « pluie froide, lent »). Trioio lit l'intention, la forme, la danse et les nœuds de l'arbre des influences, et montre dans la forme ce qu'il a compris ou non. Vide, Trioio suit ses propres critères. Accents, majuscules, ponctuation et petites fautes de frappe sont tolérés.\n\n"
        "La case « consulter en ligne les mots non lus » envoie seulement les mots que Trioio n'a pas lus à des services publics (Wikidata, Datamuse) et calcule à partir des réponses. Elle est désactivée par défaut, car les mots quittent l'ordinateur ; les réponses sont gardées en mémoire, la deuxième fois est instantanée.\n\n"
        "Influences : des nœuds de l'arbre (rock/noise, brasil/nordeste/maracatu) ou auto ; la force va de la référence (0) à l'arbre (1). Sans : les rôles retirés (percussion, basse).\n\n"
        "Geste : dessinez la courbe d'intensité de la pièce, ou écrivez-la en position:intensité.\n\n"
        "Artistes et groupes : Trioio n'apporte pas la poétique des artistes et des groupes ; il n'invente pas la manière de composer de quelqu'un. Un nom écrit dans Mots n'arrive au réseau que comme des mots isolés. Pour faire d'un artiste une influence, écrivez-le vous-même, avec votre lecture et vos sources, dans le fichier influencias_do_autor.txt du dossier de données (Trioio le crée avec un exemple commenté). La ligne « também: » donne les noms tels que vous les écrivez, par exemple « também: júpiter maçã » ; dès lors, le nom appelle ce nœud dans Mots et dans Influences.",
        "Palabras: escribe algunas palabras sobre la respuesta que quieres, en cualquier idioma («dançante, maracatu, virada», «lluvia fría, lento»). Trioio lee la intención, la forma, la danza y los nodos del árbol de influencias, y muestra en la forma lo que entendió y lo que no. Vacío, Trioio sigue sus criterios. Se toleran acentos, mayúsculas, puntuación y pequeños errores de tecleo.\n\n"
        "La casilla «consultar en la red las palabras no leídas» envía solo las palabras que Trioio no leyó a servicios públicos (Wikidata, Datamuse) y calcula a partir de las respuestas. Viene desactivada, porque las palabras salen del ordenador; las respuestas quedan en la memoria, y la segunda vez es instantánea.\n\n"
        "Influencias: nodos del árbol (rock/noise, brasil/nordeste/maracatu) o auto; la fuerza va de la referencia (0) al árbol (1). Sin: los papeles que salen (percusión, bajo).\n\n"
        "Gesto: dibuja la curva de intensidad de la pieza, o escríbela como posición:intensidad.\n\n"
        "Artistas y bandas: Trioio no trae la poética de artistas y bandas; no inventa cómo compone alguien. Un nombre escrito en Palabras solo llega a la red como palabras sueltas. Para hacer de un artista una influencia, escríbelo tú, con tu lectura y tus fuentes, en el archivo influencias_do_autor.txt de la carpeta de datos (Trioio lo crea con un ejemplo comentado). La línea «também:» da los nombres como los escribes, por ejemplo «também: júpiter maçã»; desde entonces el nombre llama a ese nodo en Palabras y en Influencias." },
      { "The influences are tendencies, never rules: \"rock\" makes drums likely, not compulsory. Mix two distant nodes to get somewhere new.",
        "As influências são tendências, nunca regras: \"rock\" torna a bateria provável, não obrigatória. Misture dois nós distantes para chegar a um lugar novo.",
        "Les influences sont des tendances, jamais des règles : « rock » rend la batterie probable, pas obligatoire. Mélangez deux nœuds éloignés pour arriver ailleurs.",
        "Las influencias son tendencias, nunca reglas: «rock» hace probable la batería, no obligatoria. Mezcla dos nodos lejanos para llegar a un lugar nuevo." } },
    // 11
    { { "A first exercise", "Um primeiro exercício", "Un premier exercice", "Un primer ejercicio" },
      { "1. Use a short recording of your own.\n"
        "2. Compose in follow. Listen, judge the whole response.\n"
        "3. Compose again in oppose.\n"
        "4. In \"Your listening\", compare the two with \"switch A ↔ B\".\n"
        "5. Turn on the game in \"Language ▸\" and compose again.\n"
        "6. Read the rules in \"Your listening\". Judge by stretch where a rule worked well or badly.\n"
        "7. Compose two or three more times. The responses should start to change with what you accepted and rejected.\n\n"
        "To take a response to a DAW, turn on \"stems\" in the Language panel: next to the mix, Trioio saves one WAV per voice.",
        "1. Use uma gravação curta e sua.\n"
        "2. Componha em seguir. Ouça, julgue a resposta inteira.\n"
        "3. Componha de novo em contrapor.\n"
        "4. Em \"Sua escuta\", compare as duas com \"alternar A ↔ B\".\n"
        "5. Ligue o jogo em \"Linguagem ▸\" e componha de novo.\n"
        "6. Leia as regras em \"Sua escuta\". Julgue por trecho onde uma regra agiu bem ou mal.\n"
        "7. Componha mais duas ou três vezes. As respostas devem começar a mudar com o que você aceitou e rejeitou.\n\n"
        "Para levar uma resposta à DAW, ligue \"stems\" no painel Linguagem: ao lado da mistura, o Trioio grava um WAV por voz.",
        "1. Utilisez un enregistrement court et à vous.\n"
        "2. Composez en suivre. Écoutez, jugez la réponse entière.\n"
        "3. Composez à nouveau en opposer.\n"
        "4. Dans « Votre écoute », comparez les deux avec « alterner A ↔ B ».\n"
        "5. Activez le jeu dans « Langage ▸ » et composez à nouveau.\n"
        "6. Lisez les règles dans « Votre écoute ». Jugez par passage là où une règle a bien ou mal agi.\n"
        "7. Composez encore deux ou trois fois. Les réponses devraient commencer à changer avec ce que vous avez accepté et rejeté.\n\n"
        "Pour emporter une réponse dans une STAN, activez « stems » dans le panneau Langage : à côté du mixage, Trioio enregistre un WAV par voix.",
        "1. Usa una grabación corta y tuya.\n"
        "2. Compón en seguir. Escucha, juzga la respuesta entera.\n"
        "3. Compón de nuevo en contraponer.\n"
        "4. En «Tu escucha», compara las dos con «alternar A ↔ B».\n"
        "5. Activa el juego en «Lenguaje ▸» y compón de nuevo.\n"
        "6. Lee las reglas en «Tu escucha». Juzga por fragmento dónde una regla funcionó bien o mal.\n"
        "7. Compón dos o tres veces más. Las respuestas deberían empezar a cambiar con lo que aceptaste y rechazaste.\n\n"
        "Para llevar una respuesta a la DAW, activa «stems» en el panel Lenguaje: junto a la mezcla, Trioio graba un WAV por voz." },
      { "", "", "", "" } },
}};

} // namespace trioio::app::tutorial
