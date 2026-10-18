#!/usr/bin/env python3
"""Gera o arquivo de influências do Trioio (data/influencias/arquivo_trioio_influencias.txt).

Não é um catálogo de gêneros: é uma árvore ABERTA de influências (classes,
subclasses, maneiras de fazer, e assim por diante), de onde o Trioio tira
parâmetros, cruza galhos distantes e muta, para que a música não se repita
nem se feche num rótulo (docs/INFLUENCIAS.md; o autor, 2026-10-18: "nos
nutrimos com informações para criarmos algo novo").

Os traços são TENDÊNCIAS, nunca regras ("não tenho como afirmar que todo
rock há bateria"): o número é a chance de o traço entrar numa peça (ou, nos
eixos, o valor de 0 a 1). Os filhos herdam os traços dos pais e podem
mudá-los. Os contextos (social, histórico, tecnológico, de geração) ficam
como texto, com a fonte, como numa pesquisa.

As poéticas de artistas e bandas não são inventadas aqui: o autor as escreve,
no mesmo formato, com as suas fontes, em <dados>/influencias_do_autor.txt (o
Trioio o cria com um modelo comentado e nunca o sobrescreve).

    python3 tools/gerar_arquivo_de_influencias.py data/influencias/arquivo_trioio_influencias.txt

Formato (um bloco por nó):

    = rock/pesado
    também: heavy metal, rock pesado   (outros nomes, como se escrevem nas Palavras)
    contexto: social: …; histórico: …; tecnológico: …; geração: …
    traço: presença:percussão 0.9      (a chance de a voz estar presente)
    traço: conjunto:rock 0.9           (a chance de escolher esse conjunto)
    traço: rock:pesado 0.8             (o subestilo do rock)
    traço: harmonia:modal 0.6          (a linguagem harmônica)
    traço: humor:tensa 0.6             (a intenção da máquina de escalas)
    traço: escala:frígio 0.3           (um termo do arquivo de escalas, para a mistura)
    traço: ciclo:shuffle 0.3           (um ciclo do arquivo de ritmos)
    traço: eixo:dança 0.4              (o valor do eixo, 0..1: dança, peso, ruído)
    traço: eixo:rasgo 0.6              (o rasgo modular, 0..1)
    traço: timbre:motivo:sax 0.5       (a família de timbre de um papel: motivo,
                                        baixo, contracanto, memória, mosaico, harmonia)
    traço: eixo:densidade 0.8          (abaixo do meio rareia, acima parte as notas)
    traço: forma:kishotenketsu 0.3     (a gramática narrativa: freytag, kishotenketsu,
                                        ondas, plato, arco)
    traço: eixo:repetição 0.9          (o conceito pede a repetição: a memória
                                        anti-repetição cede, a mutação diminui)
    fonte: …

A linha `conceito:` (antes do contexto) diz a ideia do nó: "os
instrumentos rasgo precisam dos conceitos". A repetição só vale quando o
conceito a pede, "para exprimir uma música que requer a repetição".

Presenças: percussão, baixo, harmonia, contracanto, memória, mosaico.
"""

import re
import sys

blocks = []
CATS = {"presença", "conjunto", "rock", "harmonia", "humor", "escala", "ciclo", "eixo", "timbre", "forma"}
EIXOS = {"dança", "peso", "ruído", "rasgo", "repetição", "densidade"}
PAPEIS = {"motivo", "baixo", "contracanto", "memória", "mosaico", "harmonia"}
# os nomes das famílias de timbre do Trioio (timbreName, src/core)
TIMBRES = set("""aditivo|sopro|metal|corda pinçada|membrana|FM|espectral|tabela de ondas|pulsar|modal|guia de onda|tambor|voz formante|dobra|Gendyn|VOSIM|Walsh|teremim|flauta FM|scat|chip AY|galo|ressonador soprado|CMOS|coro|Rungler|operador FM|LPG|nuvem granular|objeto espectral|ressonador Bioma|corda Bioma|striker granular|arco|palheta|marimba|piano|violão|baixo acústico|flauta transversal|surdo|conga|pandeiro|ganzá|violão de 7 cordas|requinto|cavaquinho|bandolim|guitarra manouche|sax|trompete|trombone|prato de condução|chimbal|caixa|caixa com escovinha|bumbo|bongô|guiro|voz|sussurro|boca chiusa|tom-tom|prato de ataque|guitarra elétrica|guitarra distorcida|baixo elétrico|tamborim|repique|agogô|cuíca|chocalho|órgão|sanfona|bandoneón|zabumba|triângulo|naipe de cordas|naipe de metais|coro de vozes|prato friccionado|corda cantada|pele soprada""".split("|"))

# os timbres e a densidade de cada nó (tendências; o autor acrescenta)
EXTRA = {
    "rock": "timbre:motivo:guitarra distorcida 0.5; timbre:baixo:baixo elétrico 0.7",
    "rock/punk": "eixo:densidade 0.8; forma:plato 0.3",
    "rock/pesado/lento": "eixo:densidade 0.2",
    "rock/post-rock": "timbre:motivo:guitarra elétrica 0.6; eixo:densidade 0.3; forma:ondas 0.6",
    "rock/psicodélico": "timbre:contracanto:órgão 0.6; timbre:harmonia:órgão 0.3",
    "rock/noise": "timbre:motivo:guitarra distorcida 0.6; timbre:mosaico:Gendyn 0.4; forma:kishotenketsu 0.3",
    "rock/introspectivo": "timbre:motivo:guitarra elétrica 0.5; eixo:densidade 0.25",
    "jazz": "timbre:motivo:sax 0.5; timbre:contracanto:trompete 0.4; timbre:harmonia:piano 0.6; timbre:baixo:baixo acústico 0.7",
    "jazz/bebop": "eixo:densidade 0.85",
    "jazz/modal": "eixo:densidade 0.45",
    "jazz/livre": "eixo:densidade 0.7; forma:kishotenketsu 0.3",
    "brasil/choro": "timbre:motivo:bandolim 0.4; timbre:motivo:flauta transversal 0.3; timbre:harmonia:cavaquinho 0.4; timbre:baixo:violão de 7 cordas 0.5",
    "brasil/samba/bossa": "timbre:harmonia:violão 0.7; eixo:densidade 0.4",
    "brasil/nordeste": "timbre:motivo:sanfona 0.6; timbre:mosaico:triângulo 0.3",
    "brasil/clube-da-esquina": "timbre:harmonia:violão 0.6; timbre:contracanto:voz 0.3",
    "erudita": "timbre:motivo:arco 0.5; timbre:harmonia:naipe de cordas 0.5; timbre:contracanto:flauta transversal 0.3; forma:freytag 0.4; forma:arco 0.3",
    "erudita/impressionismo": "timbre:harmonia:piano 0.6; timbre:motivo:flauta transversal 0.4",
    "erudita/minimalismo": "timbre:harmonia:marimba 0.5; timbre:motivo:piano 0.3; eixo:densidade 0.8; forma:plato 0.7",
    "erudita/espectral": "timbre:harmonia:objeto espectral 0.5",
    "eletrônica": "timbre:motivo:FM 0.4; timbre:harmonia:tabela de ondas 0.4; timbre:baixo:operador FM 0.4",
    "eletrônica/ambiente": "timbre:harmonia:nuvem granular 0.6; eixo:densidade 0.15",
    "eletrônica/pista": "eixo:densidade 0.75; forma:ondas 0.5",
    "experimental/ruído": "timbre:motivo:Gendyn 0.5; timbre:contracanto:chip AY 0.3; timbre:harmonia:Rungler 0.4",
    "experimental/drone": "timbre:harmonia:prato friccionado 0.4; timbre:harmonia:corda cantada 0.3; eixo:densidade 0.05; forma:plato 0.8",
    "mundo/indiana": "timbre:harmonia:corda cantada 0.4",
    "mundo/flamenco": "timbre:harmonia:violão 0.7",
}

# os conceitos: a ideia de cada fazer musical ("os instrumentos rasgo precisam
# dos conceitos"); o conceito governa como os traços se combinam
CONCEITOS = {
    "rock": "o corpo e a eletricidade: o volume, o riff que volta e a banda como unidade",
    "rock/punk": "a urgência e a recusa: o mínimo de meios, o máximo de energia",
    "rock/pesado": "a potência e o peso como experiência física",
    "rock/progressivo": "forma:kishotenketsu 0.35; a forma longa e a ambição de obra dentro do rock",
    "rock/post-rock": "o instrumento do rock usado para a textura e a forma longa, não para a canção",
    "rock/psicodélico": "a expansão da percepção: o som que se desfaz e se transforma",
    "rock/noise": "o ruído como matéria e a canção como pretexto para rasgá-la",
    "rock/dançante": "o corpo na pista: o pulso que puxa a dança",
    "rock/introspectivo": "a escuta para dentro: o espaço e a voz baixa",
    "jazz": "a improvisação sobre uma forma comum: a conversa entre músicos",
    "jazz/livre": "a liberdade da forma e do tempo como posição",
    "brasil/samba": "a roda: o corpo, a síncope, o coletivo",
    "brasil/tropicalismo": "a antropofagia: devorar o de fora e o de dentro para fazer outra coisa",
    "erudita/minimalismo": "a repetição como processo: a mudança lenta que só se ouve porque o resto se repete",
    "erudita/espectral": "o timbre gera a harmonia: o acorde tirado do som",
    "eletrônica/pista": "o corpo coletivo: o pulso que não para, a transformação por camadas sobre a repetição",
    "eletrônica/ambiente": "a música como ambiente: a atenção que entra e sai",
    "experimental/ruído": "o que a música exclui (o ruído) vira o centro",
    "experimental/drone": "o som que permanece: escutar por dentro de uma nota",
    "vocal/coral": "muitas vozes como uma: a respiração comum e o acorde cantado",
    "mundo/tango": "o abraço e a pausa: o marcato que arrasta e a frase que se suspende",
    "brasil/nordeste/coco": "a roda que responde: o refrão do coro à embolada do tirador, a palma e o pé",
}
# a repetição que o conceito pede (para exprimir uma música que requer a
# repetição); onde não está, vale a busca do que não se repete
REPETICAO = {
    "rock": 0.5, "rock/post-rock": 0.65, "rock/dançante": 0.7, "erudita/minimalismo": 0.95,
    "eletrônica/pista": 0.85, "eletrônica/ambiente": 0.7, "experimental/drone": 1.0, "mundo/indiana": 0.6,
}


# os outros nomes (com mais de uma palavra, o nome inteiro chama o nó e as
# suas palavras não seguem avulsas para a leitura nem para a rede)
TAMBEM = {
    "brasil/nordeste/coco": "coco de roda, coco de embolada, coco de praia, coco de zambê, samba de coco",
    "mundo/tango": "tango nuevo, tango argentino, orquestra típica",
    "vocal/coral": "canto coral, coro misto",
    "brasil/samba/bossa": "bossa nova",
    "brasil/nordeste/frevo": "frevo de rua, frevo-canção",
}


def add(path, contexto, tracos, fonte):
    assert re.fullmatch(r"[a-zà-ú0-9\-]+(/[a-zà-ú0-9\-]+)*", path), path
    lines = [f"= {path}"]
    if path in CONCEITOS:
        lines.append(f"conceito: {CONCEITOS[path]}")
    lines.append(f"contexto: {contexto}")
    if path in TAMBEM:
        lines.append(f"também: {TAMBEM[path]}")
    if path in REPETICAO:
        tracos += f"; eixo:repetição {REPETICAO[path]}"
    if path in EXTRA:
        tracos += "; " + EXTRA[path]
    for t in tracos.split(";"):
        t = t.strip()
        if not t:
            continue
        key, val = t.rsplit(" ", 1)
        cat = key.split(":", 1)[0]
        assert cat in CATS, (path, key)
        if cat == "forma":
            assert key.split(":", 1)[1] in {"freytag", "kishotenketsu", "ondas", "plato", "arco"}, (path, key)
        if cat == "timbre":
            _, papel, nome = key.split(":", 2)
            assert papel in PAPEIS and nome in TIMBRES, (path, key)
        if cat == "eixo":
            assert key.split(":", 1)[1] in EIXOS, (path, key)
        v = float(val)
        assert 0.0 <= v <= 1.0, (path, t)
        lines.append(f"traço: {key} {val}")
    lines.append(f"fonte: {fonte}")
    blocks.append("\n".join(lines))


MIDDLETON = "R. Middleton, Studying Popular Music, Open University Press, 1990"

# ---- rock ------------------------------------------------------------------------------------------
add("rock",
    "social: música jovem e de banda, do baile ao estádio; histórico: anos 1950, do rhythm and blues e do country; "
    "tecnológico: a guitarra elétrica, o amplificador valvulado, a gravação multipista; geração: o pós-guerra e as seguintes",
    "presença:percussão 0.85; presença:baixo 0.9; presença:harmonia 0.8; conjunto:rock 0.8; harmonia:funcional 0.5; "
    "eixo:dança 0.55; eixo:peso 0.6; eixo:ruído 0.35; eixo:rasgo 0.45",
    MIDDLETON)
add("rock/clássico",
    "social: a banda de palco e de rádio; histórico: fim dos anos 1960 e 1970; tecnológico: o amplificador saturado, o riff "
    "de power chord; geração: os que cresceram com o rock dos anos 1950",
    "rock:clássico 0.8; eixo:dança 0.6; eixo:peso 0.6",
    MIDDLETON)
add("rock/punk",
    "social: o faça-você-mesmo, contra o virtuosismo e a indústria; histórico: Nova York e Londres, 1975–1978; "
    "tecnológico: gravação barata, equipamento simples; geração: a juventude da crise dos anos 1970",
    "rock:punk 0.85; eixo:dança 0.7; eixo:peso 0.65; eixo:ruído 0.5; harmonia:funcional 0.6",
    "J. Savage, England's Dreaming, Faber, 1991")
add("rock/pesado",
    "social: a comunidade de fãs e a potência do volume; histórico: do fim dos anos 1960 às suas muitas ramificações; "
    "tecnológico: a distorção extrema, a afinação baixa, o bumbo duplo; geração: várias, cada uma com o seu peso",
    "rock:pesado 0.8; humor:tensa 0.5; harmonia:modal 0.5; escala:frígio 0.3; eixo:peso 0.95; eixo:dança 0.35; eixo:ruído 0.55",
    "R. Walser, Running with the Devil, Wesleyan UP, 1993")
add("rock/pesado/lento",
    "social: o rito lento e grave; histórico: do início dos anos 1970 em diante; tecnológico: a afinação muito baixa, o "
    "amplificador no limite; geração: as que buscaram o peso pela lentidão",
    "presença:percussão 0.7; humor:sombria 0.6; eixo:dança 0.15; eixo:peso 1.0",
    "R. Walser, Running with the Devil, Wesleyan UP, 1993")
add("rock/blues-rock",
    "social: o bar, a jam; histórico: anos 1960, a releitura do blues elétrico; tecnológico: a guitarra saturada, o "
    "bend; geração: os que descobriram o blues pelos discos",
    "rock:blues 0.85; ciclo:shuffle 0.4; eixo:dança 0.6",
    "G. Kubik, Africa and the Blues, UP of Mississippi, 1999")
add("rock/progressivo",
    "social: o álbum como obra; histórico: fim dos anos 1960 e anos 1970; tecnológico: o sintetizador, o mellotron, o "
    "estúdio como instrumento; geração: a que trouxe o erudito e o folclore ao rock",
    "rock:progressivo 0.8; harmonia:quartal 0.4; eixo:dança 0.3; eixo:rasgo 0.55",
    "E. Macan, Rocking the Classics, Oxford UP, 1997")
add("rock/post-rock",
    "social: o instrumental, a escuta longa; histórico: anos 1990 (o termo é de S. Reynolds, 1994); tecnológico: os "
    "pedais de delay e reverb, a textura; geração: a que saiu do rock pelo som e pela forma",
    "rock:post-rock 0.85; presença:percussão 0.6; harmonia:estática 0.5; humor:suspensa 0.5; eixo:dança 0.2; eixo:peso 0.45",
    "S. Reynolds, Rip It Up and Start Again, Faber, 2005")
add("rock/psicodélico",
    "social: a contracultura; histórico: meados dos anos 1960; tecnológico: o estúdio, a fita ao contrário, o órgão, o "
    "fuzz; geração: a de 1967",
    "rock:psicodélico 0.85; harmonia:modal 0.5; humor:suspensa 0.4; eixo:dança 0.5; eixo:rasgo 0.65",
    MIDDLETON)
add("rock/noise",
    "social: o underground, a recusa do som limpo; histórico: dos anos 1980 em diante; tecnológico: a microfonia, as "
    "afinações alteradas, o pedal no limite; geração: a do pós-punk e a seguinte",
    "rock:noise 0.85; harmonia:clusters 0.5; eixo:ruído 0.95; eixo:peso 0.7; eixo:rasgo 0.8",
    "P. Hegarty, Noise/Music: A History, Continuum, 2007")
add("rock/dançante",
    "social: a pista; histórico: do rock'n'roll de baile ao pós-punk dançante; tecnológico: o bumbo marcado, o baixo "
    "em colcheias; geração: muitas",
    "presença:percussão 0.95; rock:punk 0.3; rock:clássico 0.4; eixo:dança 0.9; eixo:peso 0.45",
    "S. Reynolds, Rip It Up and Start Again, Faber, 2005")
add("rock/introspectivo",
    "social: a escuta íntima; histórico: muitas épocas; tecnológico: a guitarra limpa, o quarto; geração: muitas",
    "presença:percussão 0.5; rock:post-rock 0.4; humor:serena 0.4; humor:sombria 0.3; eixo:dança 0.15; eixo:peso 0.3",
    MIDDLETON)

# ---- jazz ------------------------------------------------------------------------------------------
GIOIA = "T. Gioia, The History of Jazz, Oxford UP, 1997"
add("jazz",
    "social: a música afro-americana, do clube ao festival; histórico: do início do século XX em diante; tecnológico: "
    "o disco, o rádio; geração: cada uma reinventou o jazz",
    "conjunto:jazz 0.8; harmonia:jazz 0.7; presença:percussão 0.8; presença:baixo 0.9; eixo:dança 0.5; eixo:peso 0.35",
    GIOIA)
add("jazz/bebop", "social: os músicos entre si, o clube depois da hora; histórico: anos 1940; tecnológico: o disco de 78 rpm; "
    "geração: a do pós-swing", "harmonia:jazz 0.9; eixo:dança 0.4; humor:tensa 0.3", GIOIA)
add("jazz/modal", "social: a escuta concentrada; histórico: fim dos anos 1950; tecnológico: o LP (as faixas longas); "
    "geração: a que saiu da harmonia funcional", "harmonia:modal 0.7; harmonia:quartal 0.3; humor:suspensa 0.4; eixo:dança 0.3", GIOIA)
add("jazz/livre", "social: a liberdade como posição; histórico: anos 1960; tecnológico: o selo independente; geração: a dos "
    "direitos civis", "presença:percussão 0.7; harmonia:clusters 0.5; eixo:ruído 0.6; eixo:rasgo 0.7; eixo:dança 0.2", GIOIA)
add("jazz/fusão", "social: o jazz e o rock juntos; histórico: fim dos anos 1960 e anos 1970; tecnológico: o piano "
    "elétrico, o sintetizador; geração: a do rock", "conjunto:rock 0.3; harmonia:quartal 0.4; eixo:dança 0.6; eixo:peso 0.5", GIOIA)

# ---- música brasileira ----------------------------------------------------------------------------------
TINHORAO = "J. R. Tinhorão, Pequena história da música popular, Vozes, 1974"
add("brasil", "social: o encontro de matrizes africanas, europeias e indígenas; histórico: do século XIX em diante; "
    "tecnológico: o rádio, o disco; geração: muitas", "presença:percussão 0.85; eixo:dança 0.65", TINHORAO)
add("brasil/choro", "social: as rodas; histórico: Rio de Janeiro, fim do século XIX; tecnológico: o violão de 7 cordas; "
    "geração: os chorões", "conjunto:choro 0.85; harmonia:brasileira 0.7; eixo:dança 0.5", TINHORAO)
add("brasil/samba", "social: as rodas, os terreiros, o carnaval; histórico: Rio de Janeiro, início do século XX; tecnológico: "
    "o rádio; geração: muitas", "conjunto:samba 0.7; conjunto:escola 0.2; harmonia:brasileira 0.6; eixo:dança 0.85",
    "H. Vianna, O mistério do samba, Zahar, 1995")
add("brasil/samba/bossa", "social: o apartamento, a voz baixa; histórico: Rio, fim dos anos 1950; tecnológico: o microfone "
    "próximo; geração: a classe média carioca", "conjunto:samba 0.6; harmonia:brasileira 0.8; humor:serena 0.4; eixo:dança 0.45; "
    "eixo:peso 0.15", "R. Castro, Chega de saudade, Companhia das Letras, 1990")
add("brasil/nordeste", "social: a feira, o forró, a cantoria; histórico: o sertão; tecnológico: a sanfona; geração: a da "
    "migração nordestina", "conjunto:forró 0.7; harmonia:nordestina 0.7; ciclo:baião 0.4; eixo:dança 0.8",
    "J. Siqueira, O Sistema Modal na Música Folclórica do Brasil, João Pessoa, 1981")
add("brasil/nordeste/frevo", "social: o carnaval de rua do Recife; histórico: fim do século XIX; tecnológico: as bandas de "
    "metais; geração: muitas", "harmonia:frevo 0.8; conjunto:orquestra 0.3; eixo:dança 0.95; eixo:peso 0.5",
    "V. de Oliveira, Frevo, capoeira e passo, Recife, 1971")
add("brasil/nordeste/maracatu", "social: as nações, o cortejo; histórico: Recife, raízes no século XVIII; tecnológico: a "
    "alfaia; geração: muitas, e a do manguebeat", "harmonia:maracatu 0.8; conjunto:escola 0.3; eixo:dança 0.75; eixo:peso 0.6",
    "C. Guerra-Peixe, Os Maracatus do Recife, 1955")
add("brasil/tropicalismo", "social: a antropofagia, a mistura; histórico: 1967–1968; tecnológico: a guitarra elétrica e o "
    "estúdio na MPB; geração: a de 1968", "conjunto:rock 0.4; harmonia:brasileira 0.5; eixo:rasgo 0.6; eixo:dança 0.55",
    "C. Veloso, Verdade tropical, Companhia das Letras, 1997")
add("brasil/clube-da-esquina", "social: a amizade, Minas Gerais; histórico: anos 1970; tecnológico: o violão e o estúdio; "
    "geração: a dos anos 1970", "harmonia:mineira 0.8; humor:suspensa 0.3; eixo:dança 0.35",
    "ver docs/MAQUINA_DE_HARMONIA.md §6 (o Clube da Esquina)")

# ---- erudita ------------------------------------------------------------------------------------------
add("erudita", "social: a sala de concerto, a partitura; histórico: séculos de tradição escrita; tecnológico: a notação, "
    "a orquestra; geração: muitas", "conjunto:câmara 0.4; conjunto:orquestra 0.4; presença:percussão 0.2; eixo:dança 0.15",
    "D. J. Grout e C. V. Palisca, A History of Western Music, Norton, 1960 (e edições)")
add("erudita/impressionismo", "social: a Paris do fim do século; histórico: 1890–1920; tecnológico: o piano moderno; "
    "geração: a de Debussy", "harmonia:impressionista 0.8; humor:suspensa 0.4; eixo:peso 0.2",
    "M. DeVoto, Debussy and the Veil of Tonality, Pendragon, 2004")
add("erudita/minimalismo", "social: o loft, a galeria; histórico: anos 1960 e 1970; tecnológico: a fita em loop, o órgão "
    "elétrico; geração: a do pós-serialismo", "harmonia:estática 0.7; eixo:dança 0.45; presença:percussão 0.3",
    "M. Nyman, Experimental Music: Cage and Beyond, Cambridge UP, 1974")
add("erudita/espectral", "social: o IRCAM, a pesquisa; histórico: Paris, anos 1970; tecnológico: a análise espectral por "
    "computador; geração: a de Grisey e Murail", "harmonia:espectral 0.8; presença:percussão 0.2; eixo:rasgo 0.6",
    "J. Fineberg, \"Guide to the basic concepts and techniques of spectral music\", Contemporary Music Review 19(2), 2000")

# ---- eletrônica e experimental --------------------------------------------------------------------------
HOLMES = "T. Holmes, Electronic and Experimental Music, Routledge, 2002 (e edições)"
add("eletrônica", "social: o estúdio, o clube; histórico: do século XX em diante; tecnológico: o sintetizador, o "
    "sequenciador, o computador; geração: muitas", "eixo:rasgo 0.75; presença:percussão 0.6", HOLMES)
add("eletrônica/pista", "social: a rave, o clube; histórico: Detroit e Chicago nos anos 1980, depois o mundo; tecnológico: "
    "a caixa de ritmos, o sampler; geração: a da cultura rave", "presença:percussão 0.95; eixo:dança 0.95; eixo:rasgo 0.7",
    "S. Reynolds, Energy Flash, Picador, 1998")
add("eletrônica/ambiente", "social: a escuta de fundo, a instalação; histórico: anos 1970 em diante; tecnológico: a fita, "
    "o sintetizador; geração: muitas", "presença:percussão 0.15; harmonia:estática 0.6; humor:suspensa 0.6; eixo:dança 0.05",
    HOLMES)
add("experimental/ruído", "social: a recusa da música como ordem; histórico: dos futuristas (L. Russolo, 1913) em diante; "
    "tecnológico: o ruído como material; geração: muitas", "harmonia:clusters 0.6; eixo:ruído 1.0; eixo:rasgo 0.9; eixo:dança 0.1",
    "P. Hegarty, Noise/Music: A History, Continuum, 2007")
add("experimental/drone", "social: a escuta longa, o rito; histórico: anos 1960 em diante, e as tradições de bordão; "
    "tecnológico: a amplificação, o oscilador; geração: muitas", "presença:percussão 0.1; harmonia:estática 0.8; eixo:dança 0.0",
    "M. Nyman, Experimental Music: Cage and Beyond, Cambridge UP, 1974")

# ---- músicas do mundo (as tradições do arquivo de escalas e de ritmos) ---------------------------------
add("mundo/árabe", "social: o tarab, a improvisação no maqam; histórico: séculos; tecnológico: o oud, o qanun; geração: muitas",
    "escala:maqam 0.7; ciclo:maqsum 0.5; presença:harmonia 0.3",
    "J. Farraj e S. Abu Shumays, Inside Arabic Music, Oxford UP, 2019")
add("mundo/indiana", "social: o raga, a casa de concerto e o templo; histórico: séculos; tecnológico: o tanpura, a tabla; "
    "geração: muitas", "escala:rāga 0.7; ciclo:tīntāl 0.5; presença:harmonia 0.2; harmonia:estática 0.4",
    "M. Clayton, Time in Indian Music, Oxford UP, 2000")
# ---- acrescentados em 2026-10-18, pedidos pelo uso do autor no app ("tango improvisado, coral a
# capela", "coco de roda"): a voz em grupo como influência que atravessa os conjuntos ------------------
add("vocal", "social: o canto como prática comum; histórico: todas as épocas; tecnológico: o corpo, o microfone; "
    "geração: muitas", "timbre:motivo:voz 0.5; presença:percussão 0.3",
    "J. Potter (org.), The Cambridge Companion to Singing, Cambridge UP, 2000")
add("vocal/coral", "social: a igreja, a escola, o coral da comunidade; histórico: do canto gregoriano à polifonia e aos "
    "corais de hoje; tecnológico: só a voz, a acústica da sala; geração: muitas",
    "timbre:harmonia:coro 0.8; timbre:memória:voz 0.5; timbre:contracanto:voz 0.4; presença:percussão 0.15; "
    "eixo:dança 0.2; eixo:peso 0.3",
    "J. Potter (org.), The Cambridge Companion to Singing, Cambridge UP, 2000")
add("mundo/tango", "social: o salão, a milonga, o abraço; histórico: Buenos Aires e Montevidéu, fim do século XIX, e o "
    "tango nuevo dos anos 1950 e 1960; tecnológico: o bandoneón, a orquestra típica; geração: muitas, e a de Piazzolla",
    "conjunto:tango 0.8; harmonia:funcional 0.5; humor:tensa 0.3; eixo:dança 0.6; eixo:peso 0.45",
    "S. Collier, A. Cooper, M. S. Azzi e R. Martin, ¡Tango!, Thames & Hudson, 1995")
add("brasil/nordeste/coco", "social: a roda, a umbigada, o sapateado; histórico: o litoral nordestino, raízes africanas e "
    "indígenas; tecnológico: o ganzá, o pandeiro, a palma, o tamanco; geração: muitas",
    "presença:percussão 0.9; harmonia:nordestina 0.5; conjunto:forró 0.3; ciclo:baião 0.3; timbre:motivo:voz 0.5; "
    "eixo:dança 0.85; eixo:repetição 0.6",
    "M. de Andrade, Os cocos (org. O. Alvarenga), Duas Cidades, 1984")
add("mundo/flamenco", "social: o tablao, a família; histórico: Andaluzia, século XIX; tecnológico: o violão; geração: muitas",
    "harmonia:andaluza 0.7; ciclo:compás 0.4; eixo:dança 0.65",
    "P. Manuel, \"From Scarlatti to 'Guantanamera'\", JAMS 55(2), 2002")


def main():
    out = sys.argv[1]
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Arquivo de influências do Trioio\n"
                "# Uma árvore aberta: os traços são tendências (a chance, ou o valor do eixo), nunca regras.\n"
                "# O autor acrescenta e corrige; as poéticas de artistas e bandas entram pela escrita dele.\n"
                "# Gerado por tools/gerar_arquivo_de_influencias.py; formato descrito ali.\n"
                f"# {len(blocks)} nós.\n\n" + "\n\n".join(blocks) + "\n")
    print(f"{len(blocks)} nós em {out}")


if __name__ == "__main__":
    main()
