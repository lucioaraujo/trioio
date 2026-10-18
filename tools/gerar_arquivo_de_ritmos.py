#!/usr/bin/env python3
"""Gera o arquivo de ciclos rítmicos do Trioio (data/ritmos/arquivo_trioio_ritmos.txt).

Escrito pelo Trioio a partir do conhecimento publicado sobre cada tradição;
nenhum arquivo de terceiros é copiado. Cada ciclo cita a fonte, como numa
pesquisa, e diz quando é uma simplificação (a performance real varia,
ornamenta e improvisa sobre o ciclo).

    python3 tools/gerar_arquivo_de_ritmos.py data/ritmos/arquivo_trioio_ritmos.txt

Formato (um bloco por ciclo):

    = maqsum
    família: iqa' árabe
    unidade: colcheia            (colcheia | colcheia-ternária | semicolcheia | tempo)
    golpes: D T - T D - T -       (um símbolo por unidade; ver abaixo)
    divisões: 4 4                 (opcional: vibhāg, agrupamento aksak…)
    acentos: 1                    (opcional: posições fortes, a partir de 1)
    vazios: 9                     (opcional: khālī, o tempo "vazio" do tala)
    fonte: …
    nota: …

Símbolos dos golpes:
    D  grave, aberto (dum, dha, bombo)      T  agudo, aberto (tak, tin)
    k  leve, seco (ka, tek, ghost)          B  grave + agudo juntos (dha)
    x  golpe genérico (clave, sino)         G  gong grande
    N  kenong (gong horizontal médio)       P  kempul (gong suspenso pequeno)
    t  kethuk (gong pequeno, seco)          -  silêncio
"""

import sys

blocks = []


def add(name, family, unit, strokes, source, note="", divisions=None, accents=None, empties=None):
    s = strokes.split()
    assert all(c in "DTkBxGNPt-" for c in s), name
    if divisions:
        assert sum(divisions) == len(s), (name, divisions, len(s))
    b = [f"= {name}", f"família: {family}", f"unidade: {unit}", "golpes: " + " ".join(s)]
    if divisions:
        b.append("divisões: " + " ".join(map(str, divisions)))
    if accents:
        b.append("acentos: " + " ".join(map(str, accents)))
    if empties:
        b.append("vazios: " + " ".join(map(str, empties)))
    b.append(f"fonte: {source}")
    if note:
        b.append(f"nota: {note}")
    blocks.append("\n".join(b))


# ---- iqa' árabes ------------------------------------------------------------------
IQA = ("J. Farraj e S. Abu Shumays, Inside Arabic Music, Oxford UP, 2019 (os iqa'at); "
       "A. J. Racy, Making Music in the Arab World, Cambridge UP, 2003")
IQA_NOTE = "o esqueleto do iqa' (dum e tak); a percussão real preenche com golpes leves e ornamentos"
add("maqsum", "iqa' árabe", "colcheia", "D T - T D - T -", IQA, IQA_NOTE, accents=[1])
add("baladi", "iqa' árabe", "colcheia", "D D - T D - T -", IQA, IQA_NOTE, accents=[1])
add("sa'idi", "iqa' árabe", "colcheia", "D T - D D - T -", IQA, IQA_NOTE, accents=[1])
add("wahda", "iqa' árabe", "colcheia", "D - - - T - - -", IQA, IQA_NOTE, accents=[1])
add("samai thaqil", "iqa' árabe", "colcheia", "D - - T - D D T - -", IQA,
    IQA_NOTE + "; o ciclo de 10 do samai (forma instrumental)", accents=[1])

# ---- tala hindustani e carnático ------------------------------------------------------
CLAYTON = "M. Clayton, Time in Indian Music: Rhythm, Metre and Form in North Indian Rāg Performance, Oxford UP, 2000"
TALA_NOTE = ("o ṭhekā simplificado em grave/agudo (dha = B, dhin = B, tin/ta/na = T, ge = D, ke = k); "
             "sam (1) é o tempo forte; khālī é o tempo 'vazio', sem grave")
# tīntāl: dha dhin dhin dha | dha dhin dhin dha | dha tin tin ta | ta dhin dhin dha
add("tīntāl", "tala hindustani", "tempo", "B B B B B B B B B T T T T B B B", CLAYTON,
    TALA_NOTE + "; no tīntāl o 9 é dhā no ṭhekā usual, e o khālī se ouve pela falta do grave em tin tin tā tā (10 a 13)",
    divisions=[4, 4, 4, 4], accents=[1, 5, 13], empties=[9])
# jhaptāl: dhi na | dhi dhi na | ti na | dhi dhi na
add("jhaptāl", "tala hindustani", "tempo", "B T B B T T T B B T", CLAYTON, TALA_NOTE,
    divisions=[2, 3, 2, 3], accents=[1, 3, 8], empties=[6])
# rūpak: tin tin na | dhi na | dhi na  (o sam é khālī)
add("rūpak", "tala hindustani", "tempo", "T T T B T B T", CLAYTON, TALA_NOTE + "; no rūpak o próprio sam é khālī",
    divisions=[3, 2, 2], accents=[1, 4, 6], empties=[1])
# ektāl: dhin dhin | dhage tirakita | tu na | kat ta | dhage tirakita | dhi na
add("ektāl", "tala hindustani", "tempo", "B B B k T T k T B k B T", CLAYTON, TALA_NOTE,
    divisions=[2, 2, 2, 2, 2, 2], accents=[1, 5, 9, 11], empties=[3, 7])
SAMBA = "P. Sambamoorthy, South Indian Music, Indian Music Publishing House, 1958–1969"
add("ādi tāla", "tala carnático", "tempo", "B T T T B T B T", SAMBA,
    "o laghu de 4 e dois drutam de 2 (4+2+2); contado com palma (grave) e dedos ou aceno (agudo)",
    divisions=[4, 2, 2], accents=[1, 5, 7])

# ---- usul turco e aksak ----------------------------------------------------------------
SIGNELL = "K. Signell, Makam: Modal Practice in Turkish Art Music, Asian Music Publications, 1977"
add("sofyan", "usul turco", "colcheia", "D - T k", SIGNELL, "düm, te-ke, tek: o usul de 4", accents=[1])
add("curcuna", "usul turco", "colcheia", "D - k T - D - T - -", SIGNELL, "10/8 em 3+2+2+3",
    divisions=[3, 2, 2, 3], accents=[1, 4, 6, 8])
add("aksak (9/8)", "usul turco / aksak", "colcheia", "D - T - D - T - -", SIGNELL + "; C. Brăiloiu, \"Le rythme aksak\", Revue de musicologie 33, 1951",
    "9/8 em 2+2+2+3: o tempo 'manco'", divisions=[2, 2, 2, 3], accents=[1, 3, 5, 7])
BRAILOIU = "C. Brăiloiu, \"Le rythme aksak\", Revue de musicologie 33, 1951"
add("ruchenitsa (7/8)", "aksak balcânico", "colcheia", "D - T - D - -", BRAILOIU, "7/8 em 2+2+3 (Bulgária)",
    divisions=[2, 2, 3], accents=[1, 3, 5])
add("kopanitsa (11/8)", "aksak balcânico", "colcheia", "D - T - D - - T - D -", BRAILOIU, "11/8 em 2+2+3+2+2",
    divisions=[2, 2, 3, 2, 2], accents=[1, 3, 5, 8, 10])

# ---- claves e linhas-guia ------------------------------------------------------------------
TOUSSAINT = "G. T. Toussaint, The Geometry of Musical Rhythm, CRC Press, 2013; K. Agawu, The African Imagination in Music, Oxford UP, 2016"
add("clave son 3-2", "clave afro-cubana", "semicolcheia", "x - - x - - x - - - x - x - - -", TOUSSAINT)
add("clave rumba 3-2", "clave afro-cubana", "semicolcheia", "x - - x - - - x - - x - x - - -", TOUSSAINT)
add("clave bossa nova", "Brasil (bossa nova)", "semicolcheia", "x - - x - - x - - - x - - x - -", TOUSSAINT,
    "a 'clave' da bossa nova (o padrão do aro da caixa)")
add("tresillo", "linha-guia afro-americana", "semicolcheia", "x - - x - - x -", TOUSSAINT, "3+3+2: o tresillo")
add("cinquillo", "linha-guia afro-cubana", "semicolcheia", "x - x x - x x -", TOUSSAINT)
add("sino padrão (12/8)", "linha-guia oeste-africana", "colcheia-ternária", "x - x - x x - x - x - x", TOUSSAINT,
    "o 'standard pattern' de 7 golpes em 12, do sino (gankogui, agogô)")
add("baião", "Brasil (Nordeste)", "semicolcheia", "D - - D - - T - D - - D - - T -",
    "J. Siqueira, O sistema modal na música folclórica do Brasil, 1981; o baião de Luiz Gonzaga (anos 1940)",
    "o desenho da zabumba (tresillo no grave, o bacalhau no agudo), simplificado")

# ---- gamelão: colotomia ----------------------------------------------------------------------
LINDSAY = "J. Lindsay, Javanese Gamelan, Oxford UP, 1979; J. Becker, Gamelan Stories, 1993"
GAM = "estrutura colotômica: os gongs marcam o ciclo; a melodia-núcleo corre por cima"
# lancaran: 16 tempos; kethuk nos ímpares, kempul em 6, 10, 14; kenong em 4, 8, 12, 16; gong em 16
add("lancaran", "gamelão (colotomia)", "tempo", "t - t N t P t N t P t N t P t G", LINDSAY,
    GAM + "; o kenong também soa no 16 junto com o gong", accents=[16])
add("ketawang", "gamelão (colotomia)", "tempo", "- - t - - - t N - - t P - - t G", LINDSAY,
    GAM + "; ciclo de 16 com kenong em 8 e 16, kempul em 12", accents=[16])

# ---- flamenco -----------------------------------------------------------------------------------
MANUEL = "P. Manuel, \"Evolution and Structure in Flamenco Harmony\", Current Musicology 42, 1986"
add("compás de 12 (soleá, bulerías)", "flamenco", "tempo", "x - x - - x - x - x - x", MANUEL,
    "a contagem de 12 com acentos em 12, 3, 6, 8 e 10 (aqui a partir do 12)", accents=[1, 4, 7, 9, 11])

# ---- blues ----------------------------------------------------------------------------------------
add("shuffle", "blues", "colcheia-ternária", "D - T D - T D - T D - T", "G. Kubik, Africa and the Blues, 1999",
    "a divisão ternária (12/8) do shuffle: golpe longo e curto; simplificado")


def main():
    out = sys.argv[1]
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Arquivo de ciclos rítmicos do Trioio\n"
                "# Escrito pelo Trioio a partir do conhecimento publicado, com a fonte em cada ciclo.\n"
                "# Gerado por tools/gerar_arquivo_de_ritmos.py; formato descrito ali.\n"
                f"# {len(blocks)} ciclos.\n\n" + "\n\n".join(blocks) + "\n")
    print(f"{len(blocks)} ciclos em {out}")


if __name__ == "__main__":
    main()
