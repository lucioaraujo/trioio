#pragma once

// Os quatro idiomas da família RASGO (a mesma convenção do Antitotem,
// do Navalha 2 e do Rasgo Modular): inglês, português, francês e espanhol.
// Na primeira abertura, o idioma do sistema (qualquer outro cai no inglês);
// depois, o escolhido no botão EN/PT/FR/ES. Todo texto da interface vem
// daqui. As marcas da forma que o compositor escreve (as seções, o Terceiro
// Olho) ficam em português: são o registro do núcleo, não a interface.

#include <juce_core/juce_core.h>

#include <array>
#include <string>

namespace trioio::app
{

enum class Lang { en = 0, pt, fr, es };

struct L4
{
    const char* en;
    const char* pt;
    const char* fr;
    const char* es;
};

inline juce::String tr(const L4& v, Lang l)
{
    const std::array<const char*, 4> a { v.en, v.pt, v.fr, v.es };
    return juce::String::fromUTF8(a[static_cast<std::size_t>(l)]);
}

inline Lang langFromCode(const juce::String& code)
{
    const auto c = code.substring(0, 2).toLowerCase();
    if (c == "pt") return Lang::pt;
    if (c == "fr") return Lang::fr;
    if (c == "es") return Lang::es;
    return Lang::en;
}
inline juce::String langCode(Lang l) { return std::array<const char*, 4> { "en", "pt", "fr", "es" }[static_cast<std::size_t>(l)]; }
inline juce::String langLabel(Lang l) { return langCode(l).toUpperCase(); }
inline Lang nextLang(Lang l) { return static_cast<Lang>((static_cast<int>(l) + 1) % 4); }

namespace S
{
// cabeçalho e páginas
inline const L4 chooseReference { "choose a reference", "escolher referência", "choisir une référence", "elegir una referencia" };
inline const L4 modes[4] { { "follow", "seguir", "suivre", "seguir" },
                           { "transfigure", "transfigurar", "transfigurer", "transfigurar" },
                           { "oppose", "contrapor", "opposer", "contraponer" },
                           { "interrupt", "interromper", "interrompre", "interrumpir" } };
inline const L4 pageListen { "Listening and response", "Escuta e resposta", "Écoute et réponse", "Escucha y respuesta" };
inline const L4 pageLibrary { "Library", "Biblioteca", "Bibliothèque", "Biblioteca" };
inline const L4 drawer { "Language ▸", "Linguagem ▸", "Langage ▸", "Lenguaje ▸" };
inline const L4 menuLanguage { "Musical language", "Linguagem", "Langage musical", "Lenguaje musical" };
inline const L4 about { "ABOUT", "SOBRE", "À PROPOS", "ACERCA DE" };
inline const L4 languageTip { "Interface language", "Idioma da interface", "Langue de l'interface", "Idioma de la interfaz" };
inline const L4 beats { "beats", "tempos", "temps", "tiempos" };

// as três faixas
inline const L4 refCaption { "REFERENCE — what sounds", "REFERÊNCIA — o que soa", "RÉFÉRENCE — ce qui sonne", "REFERENCIA — lo que suena" };
inline const L4 refEmpty { "choose a reference in the Library", "escolha uma referência na Biblioteca",
                           "choisissez une référence dans la Bibliothèque", "elige una referencia en la Biblioteca" };
inline const L4 listening { "listening to the reference…", "ouvindo a referência…", "écoute de la référence…", "escuchando la referencia…" };
inline const L4 eyeHidden { "THIRD EYE — hidden until you listen", "TERCEIRO OLHO — escondido até você ouvir",
                            "TROISIÈME ŒIL — caché jusqu'à votre écoute", "TERCER OJO — oculto hasta que escuches" };
inline const L4 eyeRevealed { "THIRD EYE — revealed", "TERCEIRO OLHO — revelado", "TROISIÈME ŒIL — révélé", "TERCER OJO — revelado" };
inline const L4 stays { "stays: ", "fica: ", "reste : ", "queda: " };
inline const L4 reveal { "reveal after listening", "revelar depois de ouvir", "révéler après l'écoute", "revelar tras escuchar" };
inline const L4 hideEye { "hide the eye", "esconder o olho", "cacher l'œil", "ocultar el ojo" };
inline const L4 respCaption { "RESPONSE — what Trioio composes", "RESPOSTA — o que o Trioio compõe",
                              "RÉPONSE — ce que Trioio compose", "RESPUESTA — lo que compone Trioio" };
inline const L4 respEmpty { "no response yet: \"compose again\"", "nenhuma resposta ainda: \"compor de novo\"",
                            "aucune réponse encore : « composer à nouveau »", "aún no hay respuesta: «componer de nuevo»" };
inline const L4 sayrWith { " · sayr with ", " · sayr com ", " · sayr avec ", " · sayr con " };
inline const L4 noResponses { "no response yet", "nenhuma resposta ainda", "aucune réponse encore", "aún no hay respuestas" };
inline const L4 pickResponse { "the responses to this reference", "as respostas desta referência",
                               "les réponses à cette référence", "las respuestas a esta referencia" };

// Biblioteca
inline const L4 openData { "open data folder…", "abrir pasta de dados…", "ouvrir le dossier de données…", "abrir carpeta de datos…" };
inline const L4 dataRootTitle { "Trioio data root", "A raiz de dados do Trioio", "Racine de données de Trioio", "Raíz de datos de Trioio" };
inline const L4 bring { "bring a recording…", "trazer gravação…", "apporter un enregistrement…", "traer una grabación…" };
inline const L4 chooseRecording { "A recording for Trioio to listen to", "Uma gravação para o Trioio ouvir",
                                  "Un enregistrement que Trioio écoutera", "Una grabación para que Trioio escuche" };
inline const L4 authoriseButton { "authorise…", "autorizar…", "autoriser…", "autorizar…" };
inline const L4 registering { "registering ", "registrando ", "enregistrement de ", "registrando " };
inline const L4 alreadyKnown { " is already in the Library", " já está na Biblioteca", " est déjà dans la Bibliothèque", " ya está en la Biblioteca" };
inline const L4 dataKept { "data kept in ", "dados guardados em ", "données conservées dans ", "datos guardados en " };
inline const L4 playRef { "▶ reference", "▶ referência", "▶ référence", "▶ referencia" };
inline const L4 dialogueToggle { "dialogue: answer in the reference's pauses", "diálogo: responder nas pausas da referência",
                                 "dialogue : répondre dans les pauses de la référence", "diálogo: responder en las pausas de la referencia" };
inline const L4 filmFxToggle { "film effects in the drama", "efeitos de cinema na dramaturgia", "effets de cinéma dans la dramaturgie",
                               "efectos de cine en la dramaturgia" };
inline const L4 stemsToggle { "stems: one WAV per voice too (for the DAW)", "stems: também um WAV por voz (para a DAW)",
                              "stems : aussi un WAV par voix (pour la STAN)", "stems: también un WAV por voz (para la DAW)" };
inline const L4 somDaAuraToggle { "sound of the aura (Hermeto): the voice becomes harmonized melody", "som da aura (Hermeto): a fala vira melodia harmonizada",
                                  "son de l'aura (Hermeto) : la voix devient mélodie harmonisée", "sonido del aura (Hermeto): la voz se vuelve melodía armonizada" };
inline const L4 gameToggle { "game: Trioio invents its own rules", "jogo: o Trioio inventa as próprias regras",
                             "jeu : Trioio invente ses propres règles", "juego: Trioio inventa sus propias reglas" };
inline const L4 gameRulesTitle { "Rules of the game in version A", "Regras do jogo na versão A", "Règles du jeu dans la version A",
                                 "Reglas del juego en la versión A" };
inline const L4 compareAB { "▶ compare A × B", "▶ comparar A × B", "▶ comparer A × B", "▶ comparar A × B" };
inline const L4 switchAB { "switch A ↔ B", "alternar A ↔ B", "alterner A ↔ B", "alternar A ↔ B" };
inline const L4 abHint { "A = the version chosen now, B = the one before; switching keeps the same moment",
                         "A = a versão escolhida agora, B = a anterior; alternar mantém o mesmo instante",
                         "A = la version choisie maintenant, B = la précédente ; alterner garde le même instant",
                         "A = la versión elegida ahora, B = la anterior; alternar mantiene el mismo instante" };
inline const L4 playingA { "playing A: ", "tocando A: ", "lecture A : ", "sonando A: " };
inline const L4 abNone { "click another version", "clique noutra versão", "cliquez une autre version", "haz clic en otra versión" };
inline const L4 playingB { "playing B: ", "tocando B: ", "lecture B : ", "sonando B: " };
inline const L4 needTwo { "choose two versions in the list (A = now, B = before)", "escolha duas versões na lista (A = agora, B = antes)",
                          "choisissez deux versions dans la liste (A = maintenant, B = avant)", "elige dos versiones en la lista (A = ahora, B = antes)" };
inline const L4 pageYourListening { "Your listening", "Sua escuta", "Votre écoute", "Tu escucha" };
inline const L4 versionsTitle { "Versions on this reference", "Versões sobre esta referência", "Versions sur cette référence",
                                "Versiones sobre esta referencia" };
inline const L4 taughtTitle { "What your listening taught", "O que a escuta ensinou", "Ce que l'écoute a appris",
                              "Lo que enseñó la escucha" };
inline const L4 noJudgements { "no judgement yet: accept, transform or reject responses to teach Trioio",
                               "nenhum julgamento ainda: aceite, transforme ou rejeite respostas para ensinar o Trioio",
                               "aucun jugement encore : acceptez, transformez ou rejetez des réponses pour apprendre à Trioio",
                               "ningún juicio aún: acepta, transforma o rechaza respuestas para enseñar a Trioio" };
inline const L4 notJudged { "not judged", "sem julgamento", "non jugée", "sin juicio" };
inline const L4 verdictShort[3] { { "accepted", "aceita", "acceptée", "aceptada" }, { "rejected", "rejeitada", "rejetée", "rechazada" },
                                  { "to transform", "a transformar", "à transformer", "por transformar" } };
inline const L4 judgementsCount { " judgement(s); the newest weighs 1, each older one 0.85× the next (forgetting)",
                                  " julgamento(s); o mais novo pesa 1, cada anterior 0,85× o seguinte (esquecimento)",
                                  " jugement(s) ; le plus récent pèse 1, chaque précédent 0,85× le suivant (oubli)",
                                  " juicio(s); el más reciente pesa 1, cada anterior 0,85× el siguiente (olvido)" };
inline const L4 useListeningToggle { "the next response takes the listening into account", "a próxima resposta leva a escuta em conta",
                                     "la prochaine réponse tient compte de l'écoute", "la próxima respuesta tiene en cuenta la escucha" };
inline const L4 groupTimbre { "timbre", "timbre", "timbre", "timbre" };
inline const L4 groupEye { "Third Eye", "Terceiro Olho", "Troisième Œil", "Tercer Ojo" };
inline const L4 groupRoom { "room", "sala", "salle", "sala" };
inline const L4 playTogether { "▶ together", "▶ juntos", "▶ ensemble", "▶ juntos" };
inline const L4 togetherTip { "The reference and the response at the same time (listening only; nothing is saved)",
                              "A referência e a resposta ao mesmo tempo (só para ouvir; nada é gravado)",
                              "La référence et la réponse en même temps (écoute seulement ; rien n'est enregistré)",
                              "La referencia y la respuesta a la vez (solo para escuchar; no se graba nada)" };
inline const L4 stopRef { "■ stop", "■ parar", "■ arrêter", "■ parar" };
inline const L4 guide { "1 · bring a recording\n2 · say where it comes from and what Trioio may do with it\n3 · compose a response\n4 · listen (▶), reveal the Third Eye, judge",
                        "1 · traga uma gravação\n2 · diga de onde ela vem e o que o Trioio pode fazer com ela\n3 · componha uma resposta\n4 · ouça (▶), revele o Terceiro Olho, julgue",
                        "1 · apportez un enregistrement\n2 · dites d'où il vient et ce que Trioio peut en faire\n3 · composez une réponse\n4 · écoutez (▶), révélez le Troisième Œil, jugez",
                        "1 · trae una grabación\n2 · di de dónde viene y qué puede hacer Trioio con ella\n3 · compón una respuesta\n4 · escucha (▶), revela el Tercer Ojo, juzga" };
inline const L4 nowCompose { "authorised — now: compose a response", "autorizada — agora: compor uma resposta",
                             "autorisée — maintenant : composer une réponse", "autorizada — ahora: componer una respuesta" };
inline const L4 pendingHint { "pending: say where it comes from (authorise…)", "pendente: diga de onde ela vem (autorizar…)",
                              "en attente : dites d'où il vient (autoriser…)", "pendiente: di de dónde viene (autorizar…)" };
// a autorização (docs/MANIFESTO_DE_PROVENIENCIA.md)
inline const L4 authTitle { "Where does this recording come from?", "De onde vem esta gravação?", "D'où vient cet enregistrement ?",
                            "¿De dónde viene esta grabación?" };
inline const L4 originLabel { "Origin", "Origem", "Origine", "Origen" };
inline const L4 basisLabel { "On what right", "Com que direito", "À quel titre", "Con qué derecho" };
inline const L4 originItems[6] { { "my own work", "obra minha", "mon œuvre", "obra mía" },
                                 { "a performer's capture", "captura de um intérprete", "captation d'un interprète", "captura de un intérprete" },
                                 { "another instrument", "outro instrumento", "un autre instrument", "otro instrumento" },
                                 { "generated by AI", "gerada por IA", "générée par IA", "generada por IA" },
                                 { "licensed", "licenciada", "sous licence", "con licencia" },
                                 { "public domain", "domínio público", "domaine public", "dominio público" } };
inline const L4 basisItems[5] { { "own work", "obra própria", "œuvre propre", "obra propia" }, { "licence", "licença", "licence", "licencia" },
                                { "public domain", "domínio público", "domaine public", "dominio público" },
                                { "written permission", "permissão escrita", "autorisation écrite", "permiso escrito" },
                                { "service terms", "termos do serviço", "conditions du service", "términos del servicio" } };
inline const L4 toolLabel { "Tool or instrument (name, version)", "Ferramenta ou instrumento (nome, versão)",
                            "Outil ou instrument (nom, version)", "Herramienta o instrumento (nombre, versión)" };
inline const L4 licenceLabel { "Licence", "Licença", "Licence", "Licencia" };
inline const L4 evidenceLabel { "Evidence (link, file, terms)", "Evidência (link, arquivo, termos)", "Preuve (lien, fichier, conditions)",
                                "Evidencia (enlace, archivo, términos)" };
inline const L4 byLabel { "Declared by", "Declarado por", "Déclaré par", "Declarado por" };
inline const L4 deriveToggle { "Trioio may listen, analyse, learn and compose responses from it",
                               "o Trioio pode ouvir, analisar, aprender e compor respostas a partir dela",
                               "Trioio peut l'écouter, l'analyser, apprendre et composer des réponses",
                               "Trioio puede escuchar, analizar, aprender y componer respuestas a partir de ella" };
inline const L4 publishToggle { "the responses may be published", "as respostas podem ser publicadas", "les réponses peuvent être publiées",
                                "las respuestas pueden publicarse" };
inline const L4 authorise { "authorise", "autorizar", "autoriser", "autorizar" };
inline const L4 refuse { "refuse", "recusar", "refuser", "rechazar" };
inline const L4 notNow { "not now", "agora não", "pas maintenant", "ahora no" };
inline const L4 origin { "origin: ", "origem: ", "origine : ", "origen: " };
inline const L4 state { "state: ", "estado: ", "état : ", "estado: " };
inline const L4 uses { "uses: listen › analyse › learn › derive", "usos: ouvir › analisar › aprender › derivar",
                       "usages : écouter › analyser › apprendre › dériver", "usos: escuchar › analizar › aprender › derivar" };
inline const L4 notDerivable { "cannot be derived yet", "ainda não pode ser derivada", "ne peut pas encore être dérivée",
                               "aún no se puede derivar" };

// Linguagem
inline const L4 languageTitle { "Language of the response", "Linguagem da resposta", "Langage de la réponse", "Lenguaje de la respuesta" };
inline const L4 wordsLabel { "Words (optional prompt: intention, form, dance, influences)", "Palavras (prompt opcional: intenção, forma, dança, influências)",
                             "Mots (prompt facultatif : intention, forme, danse, influences)", "Palabras (prompt opcional: intención, forma, danza, influencias)" };
inline const L4 wordsEmpty { "empty: Trioio follows its own criteria (e.g. dançante, maracatu, virada)",
                             "vazio: o Trioio segue os seus critérios (ex.: dançante, maracatu, virada)",
                             "vide : Trioio suit ses critères (ex. : dançante, maracatu, virada)",
                             "vacío: Trioio sigue sus criterios (ej.: dançante, maracatu, virada)" };
inline const L4 semanticToggle { "look up unread words online (Wikidata, Datamuse)", "consultar na rede as palavras não lidas (Wikidata, Datamuse)",
                                 "consulter en ligne les mots non lus (Wikidata, Datamuse)", "consultar en la red las palabras no leídas (Wikidata, Datamuse)" };
inline const L4 semanticTip { "Optional and off by default: the words leave this computer. Sources in data/semantica/fontes.txt; answers kept in dados/memoria/semantica.json.",
                              "Opcional e desligado por padrão: as palavras saem do computador. As fontes em data/semantica/fontes.txt; as respostas guardadas em dados/memoria/semantica.json.",
                              "Facultatif et désactivé par défaut : les mots quittent l'ordinateur. Sources dans data/semantica/fontes.txt ; réponses gardées dans dados/memoria/semantica.json.",
                              "Opcional y desactivado por defecto: las palabras salen del ordenador. Fuentes en data/semantica/fontes.txt; respuestas guardadas en dados/memoria/semantica.json." };
inline const L4 semanticAsking { "online: looking up…", "rede: consultando…", "en ligne : consultation…", "red: consultando…" };
inline const L4 semanticNothing { "online: nothing to look up (every word was read)", "rede: nada a consultar (todas as palavras foram lidas)",
                                  "en ligne : rien à consulter (tous les mots ont été lus)", "red: nada que consultar (todas las palabras fueron leídas)" };
// as ações de uma resposta (o botão "arquivo ▾", 2026-10-19)
inline const L4 fileMenu { "file ▾", "arquivo ▾", "fichier ▾", "archivo ▾" };
inline const L4 fileMenuTip { "Show this response's files, export them, or move the response to the trash",
                              "Mostrar os arquivos desta resposta, exportá-los ou mover a resposta para a lixeira",
                              "Afficher les fichiers de cette réponse, les exporter ou mettre la réponse à la corbeille",
                              "Mostrar los archivos de esta respuesta, exportarlos o mover la respuesta a la papelera" };
inline const L4 showInFolder { "show in folder", "mostrar na pasta", "afficher dans le dossier", "mostrar en la carpeta" };
inline const L4 exportResponse { "export… (WAV, MIDI, stems)", "exportar… (WAV, MIDI, stems)", "exporter… (WAV, MIDI, stems)",
                                 "exportar… (WAV, MIDI, stems)" };
inline const L4 exportTitle { "Where to export this response", "Para onde exportar esta resposta", "Où exporter cette réponse",
                              "Dónde exportar esta respuesta" };
inline const L4 exportedTo { "exported to ", "exportada em ", "exportée dans ", "exportada en " };
inline const L4 trashResponse { "move to the trash…", "mover para a lixeira…", "mettre à la corbeille…", "mover a la papelera…" };
inline const L4 trashTitle { "Move this response to the trash?", "Mover esta resposta para a lixeira?", "Mettre cette réponse à la corbeille ?",
                             "¿Mover esta respuesta a la papelera?" };
inline const L4 trashBody { "It goes to the trash folder of your data folder (lixeira/) and can be brought back by moving it to sessoes/.",
                            "Ela vai para a lixeira da sua pasta de dados (lixeira/) e pode voltar: basta movê-la de novo para sessoes/.",
                            "Elle va dans la corbeille de votre dossier de données (lixeira/) et peut revenir : il suffit de la remettre dans sessoes/.",
                            "Va a la papelera de su carpeta de datos (lixeira/) y puede volver: basta moverla de nuevo a sessoes/." };
inline const L4 trashJudged { " Your judgement of it stays in your listening.", " O seu julgamento dela continua na sua escuta.",
                              " Votre jugement reste dans votre écoute.", " Su juicio sobre ella permanece en su escucha." };
inline const L4 trashOk { "move to the trash", "mover para a lixeira", "mettre à la corbeille", "mover a la papelera" };
inline const L4 cancel { "cancel", "cancelar", "annuler", "cancelar" };
inline const L4 trashedMsg { "moved to the trash: ", "movida para a lixeira: ", "mise à la corbeille : ", "movida a la papelera: " };
inline const L4 noResponseSelected { "choose a response first", "escolha uma resposta primeiro", "choisissez d'abord une réponse",
                                     "elija primero una respuesta" };

inline const L4 semanticPrefix { "online: ", "rede: ", "en ligne : ", "red: " };
// as etapas da barra de andamento (a resposta as manda em português: a chave)
struct StageName { const char* key; L4 text; };
inline const StageName stages[] {
    { "escutando a referência", { "listening to the reference", "escutando a referência", "écoute de la référence", "escuchando la referencia" } },
    { "lendo as escolhas", { "reading the choices", "lendo as escolhas", "lecture des choix", "leyendo las elecciones" } },
    { "consultando a rede", { "looking up online", "consultando a rede", "consultation en ligne", "consultando la red" } },
    { "compondo a partitura", { "composing the score", "compondo a partitura", "composition de la partition", "componiendo la partitura" } },
    { "sintetizando as vozes", { "synthesising the voices", "sintetizando as vozes", "synthèse des voix", "sintetizando las voces" } },
    { "compondo e sintetizando as linhas", { "composing and synthesising the lines", "compondo e sintetizando as linhas",
                                            "composition et synthèse des lignes", "componiendo y sintetizando las líneas" } },
    { "master", { "master", "master", "master", "máster" } },
    { "gravando", { "saving", "gravando", "enregistrement", "grabando" } },
    { "escutando a resposta", { "listening to the response", "escutando a resposta", "écoute de la réponse", "escuchando la respuesta" } },
};
inline const L4 influencesLabel { "Influences (tree nodes, comma-separated, or auto)", "Influências (nós da árvore, por vírgula, ou auto)",
                                  "Influences (nœuds de l'arbre, par virgule, ou auto)", "Influencias (nodos del árbol, por coma, o auto)" };
inline const L4 influencesEmpty { "none (e.g. rock/noise, brasil/nordeste/maracatu)", "nenhuma (ex.: rock/noise, brasil/nordeste/maracatu)",
                                  "aucune (ex. : rock/noise, brasil/nordeste/maracatu)", "ninguna (ej.: rock/noise, brasil/nordeste/maracatu)" };
inline const L4 influenceStrengthLabel { "Influence strength (0 = the reference, 1 = the tree)", "Força das influências (0 = a referência, 1 = a árvore)",
                                         "Force des influences (0 = la référence, 1 = l'arbre)", "Fuerza de las influencias (0 = la referencia, 1 = el árbol)" };
inline const L4 excludeLabel { "Without (roles to leave out)", "Sem (os papéis que saem)", "Sans (les rôles retirés)", "Sin (los papeles que salen)" };
inline const L4 excludeEmpty { "nothing (e.g. percussão, baixo)", "nada (ex.: percussão, baixo)", "rien (ex. : percussão, baixo)",
                               "nada (ej.: percussão, baixo)" };
inline const L4 gestureLabel { "Gesture (the form: position:intensity, 0 to 1)", "Gesto (a forma: posição:intensidade, de 0 a 1)",
                               "Geste (la forme : position:intensité, de 0 à 1)", "Gesto (la forma: posición:intensidad, de 0 a 1)" };
inline const L4 gestureEmpty { "none: the mode's curve (e.g. 0:0.1, 0.75:1, 1:0.2)", "nenhum: a curva do modo (ex.: 0:0.1, 0.75:1, 1:0.2)",
                               "aucun : la courbe du mode (ex. : 0:0.1, 0.75:1, 1:0.2)", "ninguno: la curva del modo (ej.: 0:0.1, 0.75:1, 1:0.2)" };
inline const L4 gestureDraw { "draw the gesture: click or drag (double-click removes)", "desenhe o gesto: clique ou arraste (duplo clique tira)",
                              "dessinez le geste : cliquez ou glissez (double-clic retire)", "dibuje el gesto: haga clic o arrastre (doble clic quita)" };
inline const L4 gestureDrawMore { "one more point to make a curve", "mais um ponto para fazer a curva", "encore un point pour la courbe",
                                  "un punto más para la curva" };
inline const L4 mixLabel { "Mixture (e.g. rāga Yaman + maqam Hijaz)", "Mistura (ex.: rāga Yaman + maqam Hijaz)",
                           "Mélange (ex. : rāga Yaman + maqam Hijaz)", "Mezcla (ej.: rāga Yaman + maqam Hijaz)" };
inline const L4 mixEmpty { "none: the scale of the field", "nenhuma: a escala do campo", "aucun : la gamme du champ",
                           "ninguna: la escala del campo" };
inline const L4 pitchLabel { "Pitches", "Alturas", "Hauteurs", "Alturas" };
inline const L4 techniqueLabel { "Technique", "Técnica", "Technique", "Técnica" };
inline const L4 seamLabel { "Seam", "Costura", "Couture", "Costura" };
inline const L4 cycleLabel { "Cycle", "Ciclo", "Cycle", "Ciclo" };
inline const L4 voicingLabel { "Voicing", "Voicing", "Voicing", "Voicing" };
inline const L4 ensembleLabel { "Ensemble (the instruments of each voice, by style)", "Conjunto (os instrumentos de cada voz, pelo estilo)",
                                "Formation (les instruments de chaque voix, par style)", "Conjunto (los instrumentos de cada voz, por estilo)" };
inline const L4 ensembleAuto { "automatic (Trioio chooses)", "automático (o Trioio escolhe)", "automatique (Trioio choisit)",
                               "automático (Trioio elige)" };
inline const L4 rasgoLabel { "Modular tear (0 = the instrument, 1 = the unheard)", "Rasgo modular (0 = o instrumento, 1 = o inaudito)",
                             "Déchirure modulaire (0 = l'instrument, 1 = l'inouï)", "Rasgo modular (0 = el instrumento, 1 = lo inaudito)" };
inline const L4 harmonyLangLabel { "Harmony (the language of the harmony machine)", "Harmonia (a linguagem da máquina de harmonia)",
                                   "Harmonie (le langage de la machine harmonique)", "Armonía (el lenguaje de la máquina armónica)" };
inline const L4 harmonyAuto { "automatic (by ensemble and mode)", "automática (pelo conjunto e pelo modo)", "automatique (par formation et mode)",
                              "automática (por conjunto y modo)" };
inline const L4 harmonyFunctional { "functional (tonal)", "funcional (tonal)", "fonctionnelle (tonale)", "funcional (tonal)" };
inline const L4 harmonyBrazilian { "Brazilian (choro, samba, bossa)", "brasileira (choro, samba, bossa)", "brésilienne (choro, samba, bossa)",
                                   "brasileña (choro, samba, bossa)" };
inline const L4 harmonyNortheastern { "Northeastern (baião, repente, Armorial)", "nordestina (baião, repente, armorial)",
                                      "nordestine (baião, repente, armorial)", "nordestina (baião, repente, armorial)" };
inline const L4 harmonyMineira { "Minas (Clube da Esquina)", "mineira (Clube da Esquina)", "Minas (Clube da Esquina)", "minera (Clube da Esquina)" };
inline const L4 harmonyModal { "modal (Dorian, Mixolydian, Phrygian vamps)", "modal (vamps dórico, mixolídio, frígio)",
                              "modale (vamps dorien, mixolydien, phrygien)", "modal (vamps dórico, mixolidio, frigio)" };
inline const L4 harmonyAndalusian { "Andalusian (i–♭VII–♭VI–V)", "andaluza (i–♭VII–♭VI–V)", "andalouse (i–♭VII–♭VI–V)", "andaluza (i–♭VII–♭VI–V)" };
inline const L4 harmonyChromatic { "Romantic chromatic (mediants, Cohn cycles)", "cromática romântica (mediantes, ciclos de Cohn)",
                                   "chromatique romantique (médiantes, cycles de Cohn)", "cromática romántica (mediantes, ciclos de Cohn)" };
inline const L4 harmonyQuartal { "quartal (Hindemith, McCoy Tyner)", "quartal (Hindemith, McCoy Tyner)", "quartale (Hindemith, McCoy Tyner)",
                                 "cuartal (Hindemith, McCoy Tyner)" };
inline const L4 harmonyPolytonal { "polytonal (Petrushka, Milhaud)", "politonal (Petrushka, Milhaud)", "polytonale (Petrouchka, Milhaud)",
                                   "politonal (Petrushka, Milhaud)" };
inline const L4 harmonyOctatonic { "octatonic (Stravinsky, Messiaen)", "octatônica (Stravinsky, Messiaen)", "octatonique (Stravinsky, Messiaen)",
                                   "octatónica (Stravinsky, Messiaen)" };
inline const L4 harmonyClusters { "clusters (Cowell)", "clusters (Cowell)", "clusters (Cowell)", "clusters (Cowell)" };
inline const L4 harmonyStasis { "static (Feldman)", "estática (Feldman)", "statique (Feldman)", "estática (Feldman)" };
inline const L4 harmonySpectral { "spectral (the reference's body)", "espectral (o corpo da referência)", "spectrale (le corps de la référence)",
                                  "espectral (el cuerpo de la referencia)" };
inline const L4 harmonyImpressionist { "impressionist (Debussy)", "impressionista (Debussy)", "impressionniste (Debussy)", "impresionista (Debussy)" };
inline const L4 ensembleRead { "read in the reference (the mode answers)", "lido na referência (o modo responde)",
                               "lu dans la référence (le mode répond)", "leído en la referencia (el modo responde)" };
inline const L4 ensembleChamber { "chamber (string quartet)", "câmara (quarteto de cordas)", "chambre (quatuor à cordes)",
                                  "cámara (cuarteto de cuerdas)" };
inline const L4 ensembleOrchestra { "orchestra", "orquestra", "orchestre", "orquesta" };
inline const L4 intentionLabel { "Intention", "Intenção", "Intention", "Intención" };
inline const L4 formLabel { "Form", "Forma", "Forme", "Forma" };
inline const L4 tempoLabel { "Tempo", "Andamento", "Tempo", "Tempo" };
inline const L4 unpredLabel { "Unpredictability (the Third Eye)", "Imprevisibilidade (o Terceiro Olho)",
                              "Imprévisibilité (le Troisième Œil)", "Imprevisibilidad (el Tercer Ojo)" };
inline const L4 languageNote { "Each choice cites its source in the form of the response.", "Cada escolha cita a fonte na forma da resposta.",
                               "Chaque choix cite sa source dans la forme de la réponse.", "Cada elección cita su fuente en la forma de la respuesta." };
inline const L4 pitchItems[6] { { "archive (the recognised language)", "arquivo (a linguagem reconhecida)", "archive (le langage reconnu)", "archivo (el lenguaje reconocido)" },
                                { "reference", "referência", "référence", "referencia" },
                                { "catalogue", "catálogo", "catalogue", "catálogo" },
                                { "microtonal", "microtonal", "microtonal", "microtonal" },
                                { "invented", "inventada", "inventée", "inventada" },
                                { "just intonation", "justa", "juste", "justa" } };
inline const L4 techniqueItems[4] { { "collage", "colagem", "collage", "collage" }, { "hybrid", "híbrido", "hybride", "híbrido" },
                                    { "layers", "camadas", "couches", "capas" }, { "transition", "transição", "transition", "transición" } };
inline const L4 seamItems[4] { { "cut", "corte", "coupe", "corte" }, { "pivot", "pivô", "pivot", "pivote" },
                               { "bridge", "ponte", "pont", "puente" }, { "overlap", "sobreposição", "superposition", "superposición" } };
inline const L4 fromScale { "the scale's own", "o da escala", "celui de la gamme", "el de la escala" };
inline const L4 voicingItems[4] { { "thirds", "terças", "tierces", "terceras" }, { "quartal", "quartal", "quartal", "cuartal" },
                                  { "So What", "So What", "So What", "So What" }, { "cluster", "cluster", "cluster", "cluster" } };
inline const L4 intentionItems[7] { { "none", "nenhuma", "aucune", "ninguna" }, { "arc", "arco", "arc", "arco" },
                                    { "luminous", "luminosa", "lumineuse", "luminosa" }, { "sombre", "sombria", "sombre", "sombría" },
                                    { "tense", "tensa", "tendue", "tensa" }, { "serene", "serena", "sereine", "serena" },
                                    { "suspended", "suspensa", "suspendue", "suspendida" } };
inline const L4 formItems[2] { { "contrast", "contraste", "contraste", "contraste" }, { "repetition", "repetição", "répétition", "repetición" } };
inline const L4 tempoItems[3] { { "as read", "como lido", "tel que lu", "como se leyó" }, { "half", "metade", "moitié", "mitad" },
                                { "double", "dobro", "double", "doble" } };

// barra de julgamento
inline const L4 masterTip { "Master volume (everything Trioio plays)", "Volume master (tudo o que o Trioio toca)",
                            "Volume général (tout ce que joue Trioio)", "Volumen master (todo lo que suena en Trioio)" };
inline const L4 playTip { "Play the response", "Tocar a resposta", "Écouter la réponse", "Reproducir la respuesta" };
inline const L4 judgeWhole { "judge the whole response — or drag a passage", "julgar a resposta inteira — ou arraste um trecho",
                             "juger toute la réponse — ou glissez sur un passage", "juzgar toda la respuesta — o arrastra un pasaje" };
inline const L4 judgeOnly { "judge only", "julgar só", "juger seulement", "juzgar solo" };
inline const L4 judgeSpan { " · passage ", " · trecho ", " · passage ", " · pasaje " };
inline const L4 judgeSection { " · section ", " · seção ", " · section ", " · sección " };
inline const L4 spanEnd { "end", "fim", "fin", "fin" };
inline const L4 judgeHint { "Drag over the response to judge a passage; click a section block to judge that section; click outside to judge the whole response.",
                            "Arraste sobre a resposta para julgar um trecho; clique num bloco para julgar a seção; clique fora para julgar a resposta inteira.",
                            "Glissez sur la réponse pour juger un passage ; cliquez sur un bloc pour juger la section ; cliquez ailleurs pour juger toute la réponse.",
                            "Arrastra sobre la respuesta para juzgar un pasaje; haz clic en un bloque para juzgar la sección; haz clic fuera para juzgar toda la respuesta." };
inline const L4 noteEmpty { "note (optional)…", "nota (opcional)…", "note (facultative)…", "nota (opcional)…" };
inline const L4 accept { "accept", "aceitar", "accepter", "aceptar" };
inline const L4 transform { "transform", "transformar", "transformer", "transformar" };
inline const L4 reject { "reject", "rejeitar", "rejeter", "rechazar" };
inline const L4 compose { "compose again", "compor de novo", "composer à nouveau", "componer de nuevo" };

// mensagens
inline const L4 dataStatus { "data: ", "dados: ", "données : ", "datos: " };
inline const L4 references { " reference(s)", " referência(s)", " référence(s)", " referencia(s)" };
inline const L4 listeningTo { "listening to ", "ouvindo ", "écoute de ", "escuchando " };
inline const L4 firstAnalysis { " (the first analysis takes a while)", " (a primeira análise demora)", " (la première analyse prend du temps)",
                                " (el primer análisis tarda)" };
inline const L4 referenceRead { "reference read", "referência lida", "référence lue", "referencia leída" };
inline const L4 demo { "demonstration (no data root)", "demonstração (sem raiz de dados)", "démonstration (sans racine de données)",
                       "demostración (sin raíz de datos)" };
inline const L4 nothingToJudge { "no response to judge", "nenhuma resposta para julgar", "aucune réponse à juger", "ninguna respuesta que juzgar" };
inline const L4 judged { "listening recorded: ", "escuta registrada: ", "écoute enregistrée : ", "escucha registrada: " };
inline const L4 judgedTail { " (weighs on the next responses)", " (pesa nas próximas respostas)", " (pèse sur les prochaines réponses)",
                             " (pesa en las próximas respuestas)" };
inline const L4 verdicts[3] { { "accepted", "aceitar", "acceptée", "aceptada" }, { "rejected", "rejeitar", "rejetée", "rechazada" },
                              { "to transform", "transformar", "à transformer", "por transformar" } };
inline const L4 chooseAuthorised { "choose an authorised reference in the Library", "escolha uma referência autorizada na Biblioteca",
                                   "choisissez une référence autorisée dans la Bibliothèque", "elige una referencia autorizada en la Biblioteca" };
inline const L4 cannotDerive { "this reference cannot be derived yet (authorise it in the Library)",
                               "esta referência ainda não pode ser derivada (autorizar na Biblioteca)",
                               "cette référence ne peut pas encore être dérivée (l'autoriser dans la Bibliothèque)",
                               "esta referencia aún no se puede derivar (autorízala en la Biblioteca)" };
inline const L4 composing { "composing… (about half a minute)", "compondo… (meio minuto, mais ou menos)", "composition… (environ une demi-minute)",
                            "componiendo… (medio minuto, más o menos)" };
inline const L4 composeFailed { "composition failed", "falha ao compor", "échec de la composition", "falló la composición" };
inline const L4 newResponse { "new response: ", "resposta nova: ", "nouvelle réponse : ", "respuesta nueva: " };

// os estados e origens que vêm dos manifestos (chaves em português)
inline const L4 statusAuthorised { "authorised", "autorizado", "autorisée", "autorizada" };
inline const L4 statusPending { "pending", "pendente", "en attente", "pendiente" };
// a Biblioteca sem as recusadas (o autor, 2026-10-27): só a vista; o
// manifesto fica (a proveniência nunca se apaga)
inline const L4 hideRefused { "hide refused", "esconder recusados", "masquer les refusées", "ocultar rechazadas" };
inline const L4 hideRefusedTip { "Hides the refused references from the list. Nothing is deleted: the record stays, and unticking shows them again.",
                                 "Tira da lista as referências recusadas. Nada é apagado: o registro fica, e desmarcar mostra de novo.",
                                 "Retire de la liste les références refusées. Rien n'est effacé : le registre reste, et décocher les montre à nouveau.",
                                 "Quita de la lista las referencias rechazadas. Nada se borra: el registro queda, y desmarcar las muestra de nuevo." };
inline const L4 statusRefused { "refused", "recusado", "refusée", "rechazada" };
inline const L4 statusRevoked { "revoked", "revogado", "révoquée", "revocada" };
inline const L4 statusUnavailable { "unavailable", "indisponível", "indisponible", "no disponible" };
inline const L4 originAuthorial { "authorial", "autoral", "d'auteur", "de autor" };
inline const L4 originCapture { "capture", "captura", "captation", "captura" };
inline const L4 originOther { "other instrument", "outro instrumento", "autre instrument", "otro instrumento" };
inline const L4 originAi { "AI", "IA", "IA", "IA" };
inline const L4 originLicensed { "licensed", "licenciado", "sous licence", "con licencia" };
inline const L4 originPublic { "public domain", "domínio público", "domaine public", "dominio público" };
inline const L4 originUnknown { "unknown origin", "origem desconhecida", "origine inconnue", "origen desconocido" };

// SOBRE e o crédito (o padrão da família: Antitotem, Rasgo Modular)
inline const L4 close { "close", "fechar", "fermer", "cerrar" };
inline const L4 aboutTitle { "TRIOIO — LISTENING, MEMORY, STRATEGY", "TRIOIO — ESCUTA, MEMÓRIA, ESTRATÉGIA",
                             "TRIOIO — ÉCOUTE, MÉMOIRE, STRATÉGIE", "TRIOIO — ESCUCHA, MEMORIA, ESTRATEGIA" };
inline const L4 aboutBody {
    "Trioio is an instrument that listens to an authorised reference, recognises its pulse, harmony, melody and musical "
    "language, and composes a response of its own, synthesised from scratch. Its Third Eye perceives the structure "
    "and may transform it: it stays hidden until you have listened, and your judgement (accept, transform, reject) "
    "becomes the instrument's memory.\n\n"
    "Every scale, rhythmic cycle and method is written by Trioio from published knowledge, citing its source as "
    "in research; no third-party archive is copied. Reused RASGO code (Rasgo Synth, Rasgo Modular, Antitotem, "
    "Aquorbium) is a pinned copy with recorded provenance.\n\n"
    "Licence: GNU AGPLv3 or later; JUCE under AGPLv3. Sources and credits: TRIOIO/CREDITS_AND_SOURCES.md.",
    "O Trioio é um instrumento que escuta uma referência autorizada, reconhece o pulso, a harmonia, a melodia e a "
    "linguagem musical dela, e compõe uma resposta própria, sintetizada do zero. O Terceiro Olho percebe a estrutura "
    "e pode transformá-la: fica escondido até você ouvir, e o seu julgamento (aceitar, transformar, rejeitar) vira "
    "a memória do instrumento.\n\n"
    "Cada escala, ciclo rítmico e método é escrito pelo Trioio a partir do conhecimento publicado, com a fonte citada "
    "como numa pesquisa; nenhum arquivo de terceiros é copiado. O código RASGO reaproveitado (Rasgo Synth, Rasgo "
    "Modular, Antitotem, Aquorbium) é uma cópia fixada, com a proveniência registrada.\n\n"
    "Licença: GNU AGPLv3 ou posterior; JUCE sob AGPLv3. Fontes e créditos: TRIOIO/CREDITS_AND_SOURCES.md.",
    "Trioio est un instrument qui écoute une référence autorisée, en reconnaît la pulsation, l'harmonie, la mélodie "
    "et le langage musical, et compose sa propre réponse, synthétisée de zéro. Son Troisième Œil perçoit la "
    "structure et peut la transformer : il reste caché jusqu'à votre écoute, et votre jugement (accepter, transformer, "
    "rejeter) devient la mémoire de l'instrument.\n\n"
    "Chaque gamme, cycle rythmique et méthode est écrit par Trioio à partir du savoir publié, avec la source citée "
    "comme dans une recherche ; aucune archive de tiers n'est copiée. Le code RASGO réutilisé (Rasgo Synth, Rasgo "
    "Modular, Antitotem, Aquorbium) est une copie figée, à la provenance enregistrée.\n\n"
    "Licence : GNU AGPLv3 ou ultérieure ; JUCE sous AGPLv3. Sources et crédits : TRIOIO/CREDITS_AND_SOURCES.md.",
    "Trioio es un instrumento que escucha una referencia autorizada, reconoce su pulso, armonía, melodía y lenguaje "
    "musical, y compone una respuesta propia, sintetizada desde cero. Su Tercer Ojo percibe la estructura y puede "
    "transformarla: permanece oculto hasta que escuchas, y tu juicio (aceptar, transformar, rechazar) se vuelve la "
    "memoria del instrumento.\n\n"
    "Cada escala, ciclo rítmico y método lo escribe Trioio a partir del conocimiento publicado, con la fuente citada "
    "como en una investigación; no se copia ningún archivo de terceros. El código RASGO reutilizado (Rasgo Synth, "
    "Rasgo Modular, Antitotem, Aquorbium) es una copia fijada, con la procedencia registrada.\n\n"
    "Licencia: GNU AGPLv3 o posterior; JUCE bajo AGPLv3. Fuentes y créditos: TRIOIO/CREDITS_AND_SOURCES.md." };
inline const L4 footerCredit { "© LÚCIO DE ARAÚJO  ·  TRIOIO 2026  ·  AGPLv3+ LICENSE  ·  v",
                               "© LÚCIO DE ARAÚJO  ·  TRIOIO 2026  ·  LICENÇA AGPLv3+  ·  v",
                               "© LÚCIO DE ARAÚJO  ·  TRIOIO 2026  ·  LICENCE AGPLv3+  ·  v",
                               "© LÚCIO DE ARAÚJO  ·  TRIOIO 2026  ·  LICENCIA AGPLv3+  ·  v" };
} // namespace S

// um estado ou origem do manifesto (chave em português) no idioma da interface
inline juce::String trStage(const std::string& key, Lang l)
{
    for (const auto& st : S::stages)
        if (key == st.key)
            return tr(st.text, l);
    return juce::String::fromUTF8(key.c_str());
}

inline juce::String trKey(const std::string& key, Lang l)
{
    static const std::pair<const char*, const L4*> map[] = {
        { "autorizado", &S::statusAuthorised }, { "pendente", &S::statusPending }, { "recusado", &S::statusRefused },
        { "revogado", &S::statusRevoked }, { "indisponível", &S::statusUnavailable }, { "autoral", &S::originAuthorial },
        { "captura", &S::originCapture }, { "outro instrumento", &S::originOther }, { "IA", &S::originAi },
        { "licenciado", &S::originLicensed }, { "domínio público", &S::originPublic }, { "origem desconhecida", &S::originUnknown },
        { "seguir", &S::modes[0] }, { "transfigurar", &S::modes[1] }, { "contrapor", &S::modes[2] }, { "interromper", &S::modes[3] } };
    for (const auto& [k, v] : map)
        if (key == k)
            return tr(*v, l);
    return juce::String::fromUTF8(key.c_str());
}


// Os nomes do núcleo (timbres, escolhas do Terceiro Olho, salas) vêm em
// português; a interface os mostra no idioma escolhido. Nomes próprios (FM,
// VOSIM, Gendyn, Rungler, HALL, SPACE…) passam como estão.
inline juce::String trName(const std::string& ptName, Lang l)
{
    struct Entry { const char* pt; L4 names; };
    static const Entry table[] = {
        { "aditivo", { "additive", "aditivo", "additif", "aditivo" } },
        { "sopro", { "breath", "sopro", "souffle", "soplo" } },
        { "metal", { "metal", "metal", "métal", "metal" } },
        { "corda pinçada", { "plucked string", "corda pinçada", "corde pincée", "cuerda pulsada" } },
        { "membrana", { "membrane", "membrana", "membrane", "membrana" } },
        { "espectral", { "spectral", "espectral", "spectral", "espectral" } },
        { "tabela de ondas", { "wavetable", "tabela de ondas", "table d'ondes", "tabla de ondas" } },
        { "pulsar", { "pulsar", "pulsar", "pulsar", "pulsar" } },
        { "modal", { "modal", "modal", "modal", "modal" } },
        { "guia de onda", { "waveguide", "guia de onda", "guide d'onde", "guía de onda" } },
        { "tambor", { "drum", "tambor", "tambour", "tambor" } },
        { "voz formante", { "formant voice", "voz formante", "voix formantique", "voz formante" } },
        { "dobra", { "fold", "dobra", "repli", "pliegue" } },
        { "teremim", { "theremin", "teremim", "thérémine", "theremín" } },
        { "flauta FM", { "FM flute", "flauta FM", "flûte FM", "flauta FM" } },
        { "galo", { "rooster", "galo", "coq", "gallo" } },
        { "ressonador soprado", { "blown resonator", "ressonador soprado", "résonateur soufflé", "resonador soplado" } },
        { "coro", { "choir", "coro", "chœur", "coro" } },
        { "operador FM", { "FM operator", "operador FM", "opérateur FM", "operador FM" } },
        { "nuvem granular", { "granular cloud", "nuvem granular", "nuage granulaire", "nube granular" } },
        { "objeto espectral", { "spectral object", "objeto espectral", "objet spectral", "objeto espectral" } },
        { "ressonador Bioma", { "Bioma resonator", "ressonador Bioma", "résonateur Bioma", "resonador Bioma" } },
        { "corda Bioma", { "Bioma string", "corda Bioma", "corde Bioma", "cuerda Bioma" } },
        { "arco", { "bowed string", "arco", "corde frottée", "arco" } },
        { "palheta", { "reed", "palheta", "anche", "lengüeta" } },
        { "marimba", { "marimba", "marimba", "marimba", "marimba" } },
        { "piano", { "piano", "piano", "piano", "piano" } },
        { "violão", { "nylon guitar", "violão", "guitare classique", "guitarra clásica" } },
        { "baixo acústico", { "upright bass", "baixo acústico", "contrebasse", "contrabajo" } },
        { "flauta transversal", { "transverse flute", "flauta transversal", "flûte traversière", "flauta traversa" } },
        { "surdo", { "surdo", "surdo", "surdo", "surdo" } },
        { "conga", { "conga", "conga", "conga", "conga" } },
        { "pandeiro", { "pandeiro", "pandeiro", "pandeiro", "pandeiro" } },
        { "ganzá", { "shaker (ganzá)", "ganzá", "ganzá", "ganzá" } },
        { "violão de 7 cordas", { "7-string guitar", "violão de 7 cordas", "guitare à 7 cordes", "guitarra de 7 cuerdas" } },
        { "requinto", { "requinto", "requinto", "requinto", "requinto" } },
        { "cavaquinho", { "cavaquinho", "cavaquinho", "cavaquinho", "cavaquinho" } },
        { "bandolim", { "mandolin (bandolim)", "bandolim", "mandoline (bandolim)", "mandolina (bandolim)" } },
        { "guitarra manouche", { "manouche guitar", "guitarra manouche", "guitare manouche", "guitarra manouche" } },
        { "sax", { "saxophone", "sax", "saxophone", "saxofón" } },
        { "trompete", { "trumpet", "trompete", "trompette", "trompeta" } },
        { "trombone", { "trombone", "trombone", "trombone", "trombón" } },
        { "prato de condução", { "ride cymbal", "prato de condução", "cymbale ride", "platillo ride" } },
        { "chimbal", { "hi-hat", "chimbal", "charleston", "hi-hat" } },
        { "caixa", { "snare drum", "caixa", "caisse claire", "caja" } },
        { "caixa com escovinha", { "snare with brushes", "caixa com escovinha", "caisse claire aux balais", "caja con escobillas" } },
        { "bumbo", { "bass drum", "bumbo", "grosse caisse", "bombo" } },
        { "bongô", { "bongos", "bongô", "bongos", "bongó" } },
        { "guiro", { "güiro", "guiro", "güiro", "güiro" } },
        { "voz", { "singing voice", "voz", "voix chantée", "voz cantada" } },
        { "sussurro", { "whisper", "sussurro", "chuchotement", "susurro" } },
        { "boca chiusa", { "humming (bocca chiusa)", "boca chiusa", "bouche fermée", "boca cerrada" } },
        { "tom-tom", { "tom-tom", "tom-tom", "tom", "tom" } },
        { "prato de ataque", { "crash cymbal", "prato de ataque", "cymbale crash", "platillo crash" } },
        { "guitarra elétrica", { "electric guitar", "guitarra elétrica", "guitare électrique", "guitarra eléctrica" } },
        { "guitarra distorcida", { "distorted guitar", "guitarra distorcida", "guitare saturée", "guitarra distorsionada" } },
        { "baixo elétrico", { "electric bass", "baixo elétrico", "basse électrique", "bajo eléctrico" } },
        { "tamborim", { "tamborim", "tamborim", "tamborim", "tamborim" } },
        { "repique", { "repinique", "repique", "repinique", "repinique" } },
        { "agogô", { "agogô bells", "agogô", "agogô", "agogó" } },
        { "cuíca", { "cuíca (friction drum)", "cuíca", "cuíca", "cuíca" } },
        { "chocalho", { "chocalho (jingle shaker)", "chocalho", "chocalho", "chocalho" } },
        { "órgão", { "tonewheel organ", "órgão", "orgue à roues phoniques", "órgano" } },
        { "sanfona", { "accordion (sanfona)", "sanfona", "accordéon", "acordeón" } },
        { "bandoneón", { "bandoneon", "bandoneón", "bandonéon", "bandoneón" } },
        { "zabumba", { "zabumba", "zabumba", "zabumba", "zabumba" } },
        { "triângulo", { "triangle", "triângulo", "triangle", "triángulo" } },
        { "naipe de cordas", { "string section", "naipe de cordas", "pupitre de cordes", "sección de cuerdas" } },
        { "naipe de metais", { "brass section", "naipe de metais", "pupitre de cuivres", "sección de metales" } },
        { "coro de vozes", { "vocal choir", "coro de vozes", "chœur", "coro de voces" } },
        { "prato friccionado", { "bowed cymbal", "prato friccionado", "cymbale frottée", "platillo frotado" } },
        { "corda cantada", { "sung string", "corda cantada", "corde chantée", "cuerda cantada" } },
        { "pele soprada", { "blown skin", "pele soprada", "peau soufflée", "parche soplado" } },
        { "striker granular", { "granular striker", "striker granular", "percuteur granulaire", "percutor granular" } },
        { "massa de glissandos", { "mass of glissandi", "massa de glissandos", "masse de glissandi", "masa de glissandos" } },
        { "faixa de ruído", { "noise band", "faixa de ruído", "bande de bruit", "banda de ruido" } },
        { "cluster", { "cluster", "cluster", "cluster", "clúster" } },
        { "iteração", { "iteration", "iteração", "itération", "iteración" } },
        { "drone", { "drone", "drone", "bourdon", "dron" } },
        { "congelamento", { "freeze", "congelamento", "gel", "congelación" } },
        { "corpo da referência", { "body of the reference", "corpo da referência", "corps de la référence", "cuerpo de la referencia" } },
        { "registro invertido", { "inverted register", "registro invertido", "registre inversé", "registro invertido" } },
        { "tempo deformado", { "warped time", "tempo deformado", "temps déformé", "tiempo deformado" } },
        { "escala mutante", { "mutating scale", "escala mutante", "gamme mutante", "escala mutante" } },
        { "silêncio súbito", { "sudden silence", "silêncio súbito", "silence soudain", "silencio súbito" } },
        { "eco em cânone", { "canon echo", "eco em cânone", "écho en canon", "eco en canon" } },
        { "troca de timbre", { "timbre swap", "troca de timbre", "changement de timbre", "cambio de timbre" } },
        { "densificar", { "densify", "densificar", "densifier", "densificar" } },
        { "rarefazer", { "rarefy", "rarefazer", "raréfier", "rarificar" } },
        { "congelar", { "freeze (rule)", "congelar", "geler", "congelar" } },
        { "sala do Trioio", { "Trioio room", "sala do Trioio", "salle de Trioio", "sala de Trioio" } }
    };
    for (const auto& e : table)
        if (ptName == e.pt)
            return tr(e.names, l);
    return juce::String::fromUTF8(ptName.c_str());
}

} // namespace trioio::app
