#!/usr/bin/env python3
"""Gera o arquivo de escalas do Trioio (data/escalas/arquivo_trioio.txt).

Escrito pelo Trioio a partir do conhecimento teórico: razões, cents e
construções publicadas. Nenhum arquivo de terceiros é lido ou copiado; cada
escala cita a fonte do conhecimento, como numa pesquisa acadêmica.
Rodar de novo regenera o arquivo:

    python3 tools/gerar_arquivo_de_escalas.py data/escalas/arquivo_trioio.txt

Formato (um bloco por escala; linhas com "#" são comentário):

    = nome
    família: …
    período: 1200            (cents, ou razão: 2/1, 3/1)
    graus: 0 203.910 …       (cents, ou razões 9/8, dentro do período)
    fonte: …
    nota: …                  (opcional: modelo, aproximação, variações)

"Modelo" quer dizer uma abstração teórica declarada, não uma medição de
uma tradição viva, cuja entonação é contextual.
"""

import math
import sys
from fractions import Fraction as F

PC = 1200 * math.log2(531441 / 524288)  # coma pitagórica, 23,460 cents
SC = 1200 * math.log2(81 / 80)          # coma sintônica, 21,506 cents
SCHISMA = PC - SC                       # 1,954 cents

blocks = []


def cents(r):
    return 1200 * math.log2(float(r))


def add(name, family, steps_cents, source, note="", period=1200.0, ratios=None):
    steps = sorted({round(x % period, 3) for x in steps_cents})
    if steps[0] != 0.0:
        raise ValueError(name)
    b = [f"= {name}", f"família: {family}", f"período: {period:.3f}",
         "graus: " + " ".join(f"{x:.3f}" for x in steps)]
    if ratios:
        b.append("razões: " + " ".join(str(r) for r in ratios))
    b.append(f"fonte: {source}")
    if note:
        b.append(f"nota: {note}")
    blocks.append("\n".join(b))


def from_ratios(name, family, ratios, source, note="", period=F(2)):
    rs = [F(r) for r in ratios]
    add(name, family, [cents(r) for r in rs], source, note, period=cents(period), ratios=[str(r) for r in rs])


# ---- Grécia antiga: gêneros de tetracorde ---------------------------------
# A oitava dórica: tetracorde + tom disjuntivo 9/8 + tetracorde, com os
# intervalos de cada tetracorde de baixo para cima (o menor embaixo). Cada
# tetracorde soma 4/3 (conferido abaixo).
PTOL = "Ptolomeu, Harmônicos (séc. II d.C.), I.13–II.15; trad. A. Barker, Greek Musical Writings II, Cambridge UP, 1989"
genera = [
    ("Arquitas enarmônico", (F(28, 27), F(36, 35), F(5, 4)), "Arquitas de Tarento (séc. IV a.C.), segundo " + PTOL),
    ("Arquitas cromático", (F(28, 27), F(243, 224), F(32, 27)), "Arquitas de Tarento, segundo " + PTOL),
    ("Arquitas diatônico", (F(28, 27), F(8, 7), F(9, 8)), "Arquitas de Tarento, segundo " + PTOL),
    ("Dídimo enarmônico", (F(32, 31), F(31, 30), F(5, 4)), "Dídimo (séc. I a.C.), segundo " + PTOL),
    ("Dídimo cromático", (F(16, 15), F(25, 24), F(6, 5)), "Dídimo, segundo " + PTOL),
    ("Dídimo diatônico", (F(16, 15), F(10, 9), F(9, 8)), "Dídimo, segundo " + PTOL),
    ("Ptolomeu enarmônico", (F(46, 45), F(24, 23), F(5, 4)), PTOL),
    ("Ptolomeu cromático suave", (F(28, 27), F(15, 14), F(6, 5)), PTOL),
    ("Ptolomeu cromático intenso", (F(22, 21), F(12, 11), F(7, 6)), PTOL),
    ("Ptolomeu diatônico suave", (F(21, 20), F(10, 9), F(8, 7)), PTOL),
    ("Ptolomeu diatônico tônico", (F(28, 27), F(8, 7), F(9, 8)), PTOL),
    ("Ptolomeu diatônico igual", (F(12, 11), F(11, 10), F(10, 9)), PTOL),
    ("Ptolomeu diatônico intenso", (F(16, 15), F(9, 8), F(10, 9)), PTOL + "; a espécie de oitava de Dó deste gênero é a escala de G. Zarlino, Le istitutioni harmoniche, 1558 (ver a entrada \"justa diatônica de Zarlino\")"),
    ("Ptolomeu diatônico ditônico (pitagórico)", (F(256, 243), F(9, 8), F(9, 8)), PTOL),
]
# As 7 espécies de oitava, pela posição de partida na oitava dórica (Mi–Mi):
# Mi dórica, Fá hipolídia, Sol hipofrígia, Lá hipodórica, Si mixolídia,
# Dó lídia, Ré frígia (Cleônides, Eisagogē harmonikē, séc. II d.C.; Ptolomeu,
# Harmônicos II.3).
SPECIES = ["dórica", "hipolídia", "hipofrígia", "hipodórica", "mixolídia", "lídia", "frígia"]
SPECIES_SRC = "; espécies de oitava segundo Cleônides, Eisagogē harmonikē (séc. II d.C.)"


def species(name, family, cents_dorian, src, note=""):
    """As 7 rotações de uma oitava dórica (7 graus, em cents)."""
    for k, sp in enumerate(SPECIES):
        rot = [((c - cents_dorian[k]) % 1200) for c in cents_dorian]
        add(f"{name}, espécie {sp}" if k else name, family + ("" if k == 0 else ", espécie de oitava"), rot,
            src + ("" if k == 0 else SPECIES_SRC), note)


for name, tet, src in genera:
    assert tet[0] * tet[1] * tet[2] == F(4, 3), name
    lower = [F(1), tet[0], tet[0] * tet[1]]
    upper = [F(3, 2) * r for r in lower]
    rs = lower + [F(4, 3)] + upper
    species(name, "Grécia antiga (gênero de tetracorde, oitava dórica)", [cents(r) for r in rs], src)

# Aristóxeno: o tom em 12 partes (a quarta = 30 partes = 500 cents)
ARIST = "Aristóxeno, Elementos Harmônicos (séc. IV a.C.), II; trad. A. Barker, Greek Musical Writings II, 1989"
for name, parts in [("Aristóxeno enarmônico", (3, 3, 24)), ("Aristóxeno cromático suave", (4, 4, 22)),
                    ("Aristóxeno cromático hemiólio", (4.5, 4.5, 21)), ("Aristóxeno cromático tônico", (6, 6, 18)),
                    ("Aristóxeno diatônico suave", (6, 9, 15)), ("Aristóxeno diatônico intenso", (6, 12, 12))]:
    assert abs(sum(parts) - 30) < 1e-9
    c = [0, parts[0] * 100 / 6, (parts[0] + parts[1]) * 100 / 6]
    species(name, "Grécia antiga (Aristóxeno, partes iguais)", c + [500] + [700 + x for x in c], ARIST,
            "o tom de 200 cents dividido em 12 partes iguais (1 parte ≈ 16,7 cents); a quarta tem 30 partes")

# ---- entonação justa e série harmônica ---------------------------------------
from_ratios("Partch 43 tons", "entonação justa (limite 11)",
            ["1", "81/80", "33/32", "21/20", "16/15", "12/11", "11/10", "10/9", "9/8", "8/7", "7/6", "32/27", "6/5", "11/9",
             "5/4", "14/11", "9/7", "21/16", "4/3", "27/20", "11/8", "7/5", "10/7", "16/11", "40/27", "3/2", "32/21", "14/9",
             "11/7", "8/5", "18/11", "5/3", "27/16", "12/7", "7/4", "16/9", "9/5", "20/11", "11/6", "15/8", "40/21", "64/33",
             "160/81"],
            "H. Partch, Genesis of a Music, University of Wisconsin Press, 1949")
from_ratios("justa diatônica de Zarlino", "entonação justa (limite 5)",
            ["1", "9/8", "5/4", "4/3", "3/2", "5/3", "15/8"],
            "G. Zarlino, Le istitutioni harmoniche, 1558 (o diatônico intenso de Ptolomeu na espécie de Dó)")
from_ratios("justa 5-limite cromática", "entonação justa (limite 5)",
            ["1", "16/15", "9/8", "6/5", "5/4", "4/3", "45/32", "3/2", "8/5", "5/3", "9/5", "15/8"],
            "escala cromática justa usual; ver J. M. Barbour, Tuning and Temperament, Michigan State College Press, 1951")
from_ratios("justa 7-limite", "entonação justa (limite 7)",
            ["1", "8/7", "7/6", "5/4", "4/3", "7/5", "3/2", "8/5", "5/3", "7/4"],
            "construção a partir das razões de limite 7; ver H. Partch, Genesis of a Music, 1949")
from_ratios("série harmônica 8–16", "série harmônica (modo 8)",
            [F(k, 8) for k in range(8, 16)], "a série harmônica (parciais 8 a 15 sobre a fundamental 8); H. von Helmholtz, Die Lehre von den Tonempfindungen, 1863")
from_ratios("série harmônica 16–32", "série harmônica (modo 16)",
            [F(k, 16) for k in range(16, 32)], "a série harmônica (parciais 16 a 31); H. von Helmholtz, 1863")
from_ratios("série subarmônica 16–8", "série subarmônica",
            [F(16, k) for k in range(16, 8, -1)], "a série subarmônica (16/16 a 16/9); H. Partch, Genesis of a Music, 1949 (utonalidade)")
from_ratios("otonalidade 1-3-5-7-9-11", "entonação justa (hexade de Partch)",
            ["1", "9/8", "5/4", "11/8", "3/2", "7/4"], "H. Partch, Genesis of a Music, 1949 (otonalidade de limite 11)")

# ---- temperamentos históricos: pelo ciclo de quintas -----------------------
# As quintas de Mib a Lá♭ (Eb Bb F C G D A E B F# C# G#), cada uma pura
# (701,955) menos a fração de coma indicada.
NAMES = ["Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#"]
PC_OF = {"C": 0, "C#": 1, "D": 2, "Eb": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "Bb": 10, "B": 11}
PURE5 = cents(F(3, 2))


def from_fifths(name, temper, source, note=""):
    """temper: dict 'C-G' -> cents a tirar da quinta pura; as outras puras."""
    pos = {"Eb": 0.0}
    for a, b in zip(NAMES, NAMES[1:]):
        pos[b] = pos[a] + PURE5 - temper.get(f"{a}-{b}", 0.0)
    c0 = pos["C"]
    steps = [(pos[n] - c0) % 1200 for n in NAMES]
    add(name, "temperamento histórico (sobre Dó)", steps, source, note)


def meantone(frac, name, source):
    from_fifths(name, {f"{a}-{b}": frac * SC for a, b in zip(NAMES, NAMES[1:])}, source,
                f"quintas estreitadas {frac:g} da coma sintônica; a quinta do lobo fica entre Sol# e Mib")


from_fifths("pitagórica (Mib a Sol#)", {}, "a cadeia de quintas puras 3/2; Boécio, De institutione musica (séc. VI)",
            "a quinta do lobo fica entre Sol# e Mib")
meantone(1 / 4, "mesotônico de 1/4 de coma", "P. Aron, Toscanello in musica, 1523")
meantone(1 / 3, "mesotônico de 1/3 de coma", "F. Salinas, De musica libri septem, 1577")
meantone(2 / 7, "mesotônico de 2/7 de coma", "G. Zarlino, Le istitutioni harmoniche, 1558")
meantone(1 / 5, "mesotônico de 1/5 de coma", "temperamento de 1/5 de coma; ver J. M. Barbour, Tuning and Temperament, 1951")
meantone(1 / 6, "mesotônico de 1/6 de coma", "temperamento de 1/6 de coma (Silbermann); ver J. M. Barbour, 1951")
from_fifths("Werckmeister III", {"C-G": PC / 4, "G-D": PC / 4, "D-A": PC / 4, "B-F#": PC / 4},
            "A. Werckmeister, Musicalische Temperatur, 1691")
from_fifths("Kirnberger III", {"C-G": SC / 4, "G-D": SC / 4, "D-A": SC / 4, "A-E": SC / 4, "F#-C#": SCHISMA},
            "J. P. Kirnberger, Die Kunst des reinen Satzes in der Musik, 1771–1779")
from_fifths("Vallotti", {k: PC / 6 for k in ["Bb-F", "F-C", "C-G", "G-D", "D-A", "A-E"]},
            "F. A. Vallotti, Della scienza teorica e pratica della moderna musica, 1779",
            "seis quintas (Fá a Si) estreitadas 1/6 da coma pitagórica; as outras puras")
from_fifths("Young II", {k: PC / 6 for k in ["C-G", "G-D", "D-A", "A-E", "E-B", "B-F#"]},
            "T. Young, \"Outlines of experiments and inquiries respecting sound and light\", Philosophical Transactions 90, 1800")

# ---- divisões iguais ----------------------------------------------------------
EDO_SRC = {
    5: "aproxima o slendro javanês e a afinação do amadinda de Uganda; ver as entradas próprias",
    7: "aproxima a afinação tradicional tailandesa; ver a entrada própria",
    10: "ver I. Wyschnegradsky e a teoria xenarmônica",
    13: "", 15: "", 16: "", 17: "",
    19: "J. Yasser, A Theory of Evolving Tonality, 1932; J. M. Mandelbaum, 19-Tone Equal Temperament (tese), 1961",
    22: "P. Erlich, \"Tuning, Tonality, and Twenty-Two-Tone Temperament\", Xenharmonikôn 17, 1998",
    24: "A. Hába, Neue Harmonielehre, 1927; I. Wyschnegradsky, Manuel d'harmonie à quarts de ton, 1932",
    31: "C. Huygens, Novus cyclus harmonicus, 1691; A. D. Fokker, Just Intonation and the 31-Tone Equal Temperament, 1949",
    41: "", 43: "J. Sauveur, Système général des intervalles des sons, 1701",
    53: "N. Mercator (séc. XVII), segundo W. Holder, A Treatise of the Natural Grounds… of Harmony, 1694",
    72: "E. Sims e J. Maneri (Boston Microtonal Society), séc. XX",
}
for n in [k for k in range(5, 101) if k != 12]:  # 12-EDO é o temperamento igual comum (está no catálogo)
    src = EDO_SRC.get(n, "")
    add(f"{n}-EDO", "divisão igual da oitava", [1200 * k / n for k in range(n)],
        src or "divisão da oitava em partes iguais (construção geral)")

add("Bohlen-Pierce igual (13 de 3/1)", "não oitavante", [cents(3) * k / 13 for k in range(13)],
    "H. Bohlen, \"13 Tonstufen in der Duodezime\", Acustica 39, 1978; M. Mathews e J. R. Pierce et al., Computer Music Journal 13(1), 1989",
    period=cents(3))
from_ratios("Bohlen-Pierce justa", "não oitavante",
            ["1", "27/25", "25/21", "9/7", "7/5", "75/49", "5/3", "9/5", "49/25", "15/7", "7/3", "63/25", "25/9"],
            "H. Bohlen, 1978; Mathews e Pierce, 1989", period=F(3))
for name, step in [("alfa de Carlos", 77.965), ("beta de Carlos", 63.833), ("gama de Carlos", 35.099)]:
    add(name, "não oitavante", [0.0],
        "W. Carlos, \"Tuning: At the Crossroads\", Computer Music Journal 11(1), 1987",
        "um passo igual repetido, sem oitava: o período é o próprio passo", period=step)

# ---- maqam árabe: o modelo de 24 quartos de tom ----------------------------
CAIRO = ("Congresso de Música Árabe do Cairo, 1932 (Recueil des travaux du Congrès de musique arabe, 1934); "
         "H. H. Touma, The Music of the Arabs, Amadeus, 1996")
MAQ_NOTE = "modelo teórico em quartos de tom; o grau neutro real varia por região, intérprete e direção do sayr"
for name, steps in [("maqam Rast", [0, 200, 350, 500, 700, 900, 1050]), ("maqam Bayati", [0, 150, 300, 500, 700, 800, 1000]),
                    ("maqam Saba", [0, 150, 300, 400, 700, 800, 1000]), ("maqam Hijaz", [0, 100, 400, 500, 700, 800, 1000]),
                    ("maqam Sikah", [0, 150, 350, 550, 700, 850, 1050]), ("maqam Nahawand", [0, 200, 300, 500, 700, 800, 1100]),
                    ("maqam Kurd", [0, 100, 300, 500, 700, 800, 1000]), ("maqam Ajam", [0, 200, 400, 500, 700, 900, 1100]),
                    ("maqam Nikriz", [0, 200, 300, 600, 700, 900, 1000]), ("maqam Hijazkar", [0, 100, 400, 500, 700, 800, 1100]),
                    ("maqam Huzam", [0, 150, 350, 450, 750, 850, 1050])]:
    add(name, "maqam árabe (modelo de 24 partes)", steps, CAIRO, MAQ_NOTE)

# ---- makam turco: o sistema Arel–Ezgi–Uzdilek (coma de Holder, 53 partes) ---
AEU = ("H. S. Arel, Türk Musikisi Nazariyatı Dersleri, 1943–1948 (sistema Arel–Ezgi–Uzdilek); "
       "K. Signell, Makam: Modal Practice in Turkish Art Music, Asian Music Publications, 1977")
for name, commas in [("makam Rast", (9, 8, 5, 9, 9, 8, 5)), ("makam Uşşak", (8, 5, 9, 9, 4, 9, 9)),
                     ("makam Hüseyni", (8, 5, 9, 9, 8, 5, 9)), ("makam Hicaz", (5, 12, 5, 9, 8, 5, 9)),
                     ("makam Kürdi", (4, 9, 9, 9, 4, 9, 9)), ("makam Buselik", (9, 4, 9, 9, 4, 9, 9))]:
    assert sum(commas) == 53, name
    acc, steps = 0, [0.0]
    for c in commas[:-1]:
        acc += c
        steps.append(1200 * acc / 53)
    add(name, "makam turco (53 comas de Holder)", steps, AEU,
        "intervalos em comas de Holder (1200/53 ≈ 22,6 cents): " + " ".join(map(str, commas))
        + "; a prática difere da teoria em vários graus")

# ---- Índia ----------------------------------------------------------------------
from_ratios("22 shrutis (reconstrução justa)", "Índia (shruti)",
            ["1", "256/243", "16/15", "10/9", "9/8", "32/27", "6/5", "5/4", "81/64", "4/3", "27/20", "45/32", "729/512",
             "3/2", "128/81", "8/5", "5/3", "27/16", "16/9", "9/5", "15/8", "243/128"],
            "Bharata, Nāṭyaśāstra (c. séc. II a.C.–II d.C.), cap. 28; reconstrução em razões de A. Daniélou, "
            "Introduction to the Study of Musical Scales, India Society, 1943",
            "uma das reconstruções modernas em razões; os shrutis são debatidos")
JUST = {0: "1", 1: "16/15", 2: "9/8", 3: "6/5", 4: "5/4", 5: "4/3", 6: "45/32", 7: "3/2", 8: "8/5", 9: "5/3", 10: "9/5", 11: "15/8"}
BHAT = "V. N. Bhatkhande, Hindustānī Saṅgīt Paddhati, 1909–1932 (sistema dos 10 thāts)"
for name, pcs in [("thāt Bilāval", [0, 2, 4, 5, 7, 9, 11]), ("thāt Kalyān", [0, 2, 4, 6, 7, 9, 11]),
                  ("thāt Khamāj", [0, 2, 4, 5, 7, 9, 10]), ("thāt Kāfī", [0, 2, 3, 5, 7, 9, 10]),
                  ("thāt Āsāvarī", [0, 2, 3, 5, 7, 8, 10]), ("thāt Bhairavī", [0, 1, 3, 5, 7, 8, 10]),
                  ("thāt Bhairav", [0, 1, 4, 5, 7, 8, 11]), ("thāt Mārvā", [0, 1, 4, 6, 7, 9, 11]),
                  ("thāt Pūrvī", [0, 1, 4, 6, 7, 8, 11]), ("thāt Toḍī", [0, 1, 3, 6, 7, 8, 11])]:
    from_ratios(name, "Índia hindustani (thāt)", [JUST[p] for p in pcs], BHAT,
                "entonação justa como modelo; na rāga a afinação de cada svara é contextual (movimento, ornamento)")
# melakarta: S R G M P D N, 72 combinações (Venkatamakhin)
RG = [(1, 2), (1, 3), (1, 4), (2, 3), (2, 4), (3, 4)]
DN = [(8, 9), (8, 10), (8, 11), (9, 10), (9, 11), (10, 11)]
MELA_NAMES = {8: "Hanumatoḍi", 15: "Māyāmāḷavagauḷa", 20: "Naṭabhairavi", 21: "Kīravāṇi", 22: "Kharaharapriya",
              28: "Harikāmbhoji", 29: "Dhīraśaṅkarābharaṇam", 65: "Mecakalyāṇi"}
VENK = ("Venkatamakhin, Caturdaṇḍīprakāśikā, 1660 (o sistema dos 72 melakartas); "
        "ver P. Sambamoorthy, South Indian Music, Indian Music Publishing House, 1958–1969")
for m in range(1, 73):
    ma = 5 if m <= 36 else 6
    k = (m - 1) % 36
    r, g = RG[k // 6]
    d, n = DN[k % 6]
    pcs = [0, r, g, ma, 7, d, n]
    nm = f"melakarta {m}" + (f" {MELA_NAMES[m]}" if m in MELA_NAMES else "")
    add(nm, "Índia carnática (melakarta)", [p * 100 for p in pcs], VENK,
        "modelo em 12 sons iguais; a gamaka e a entonação real são parte da rāga")

# ---- Sudeste asiático, África, Leste asiático ------------------------------------
KUNST = "J. Kunst, Music in Java, Martinus Nijhoff, 1949 (3.ª ed. 1973)"
add("slendro (modelo igual)", "Indonésia (gamelão)", [240 * k for k in range(5)], KUNST,
    "aproximação equidistante de 5 partes; cada gamelão tem afinação própria (embat), medida em Kunst e em C. McPhee, Music in Bali, 1966")
add("pelog (modelo em 9 partes)", "Indonésia (gamelão)", [1200 * k / 9 for k in (0, 1, 2, 4, 5, 6, 7)], KUNST,
    "aproximação teórica de 7 graus sobre 9 partes iguais (passos 1 1 2 1 1 1 2); não é uma medição")
add("pelog bem (modelo pentatônico)", "Indonésia (gamelão)", [1200 * k / 9 for k in (0, 1, 2, 5, 6)], KUNST,
    "os 5 graus principais do pathet de pelog sobre o modelo de 9 partes; aproximação")
add("tailandesa (7 partes iguais)", "Tailândia", [1200 * k / 7 for k in range(7)],
    "D. Morton, The Traditional Music of Thailand, University of California Press, 1976",
    "modelo equidistante de 7 graus; as medições mostram desvios por instrumento")
add("amadinda (5 partes iguais)", "Uganda (xilofone kiganda)", [240 * k for k in range(5)],
    "G. Kubik, \"The Structure of Kiganda Xylophone Music\", African Music 2(3), 1960",
    "modelo equidistante; cada instrumento é afinado de ouvido")
add("timbila (7 partes iguais)", "Moçambique (xilofone chopi)", [1200 * k / 7 for k in range(7)],
    "H. Tracey, Chopi Musicians: Their Music, Poetry, and Instruments, Oxford UP, 1948",
    "modelo equidistante aproximado")
LUSHI = "Lüshi Chunqiu (239 a.C.), o método sanfen sunyi (gerar por quintas 3/2 e quartas 4/3)"
from_ratios("pentatônica chinesa (gong shang jue zhi yu)", "China", ["1", "9/8", "81/64", "3/2", "27/16"], LUSHI)
from_ratios("12 lü (pitagóricos)", "China", ["1", "2187/2048", "9/8", "19683/16384", "81/64", "177147/131072", "729/512",
                                                 "3/2", "6561/4096", "27/16", "59049/32768", "243/128"], LUSHI,
            "os 12 lü gerados pela cadeia de quintas; Zhu Zaiyu (1584) propôs depois a divisão igual")
UEHARA = "R. Uehara, Zokugaku senritsu kō, 1895 (a teoria dos modos in e yō)"
add("in (japonesa)", "Japão", [0, 100, 500, 700, 800], UEHARA, "modelo em 12 sons iguais")
add("yō (japonesa)", "Japão", [0, 200, 500, 700, 900], UEHARA, "modelo em 12 sons iguais")

# ---- Brasil -----------------------------------------------------------------------
SIQ = "J. Siqueira, O sistema modal na música folclórica do Brasil, João Pessoa, 1981"
add("modo nordestino (lídio com sétima menor)", "Brasil (Nordeste)", [0, 200, 400, 600, 700, 900, 1000], SIQ,
    "modelo em 12 sons iguais; a 4.ª aumentada e a 7.ª menor juntas")
add("modo nordestino (mixolídio)", "Brasil (Nordeste)", [0, 200, 400, 500, 700, 900, 1000], SIQ, "modelo em 12 sons iguais")
add("modo nordestino (dórico)", "Brasil (Nordeste)", [0, 200, 300, 500, 700, 900, 1000], SIQ, "modelo em 12 sons iguais")


# ---- escalas MOS (moment of symmetry) ------------------------------------------
# Um gerador g repetido dentro de n partes iguais; a cadeia de L notas é MOS
# quando tem exatamente dois tamanhos de passo. Ex.: g = 7 de 12 → 5 notas
# (pentatônica) e 7 notas (diatônica).
MOS_SRC = ("E. Wilson, cartas e diagramas sobre moment of symmetry (1975 em diante), em K. Narushima, "
           "Microtonality and the Tuning Systems of Erv Wilson, Routledge, 2018; N. Carey e D. Clampitt, "
           "\"Aspects of Well-Formed Scales\", Music Theory Spectrum 11(2), 1989")
mos_seen = set()
mos_count = 0
for n in range(5, 54):
    for g in range(1, n // 2 + 1):
        if math.gcd(g, n) != 1:
            continue
        for L in range(5, 13):
            if L >= n:
                break
            pcs = sorted({(g * k) % n for k in range(L)})
            steps = [(pcs[(i + 1) % L] - pcs[i]) % n or n for i in range(L)]
            if len(set(steps)) != 2:
                continue
            key = (n, tuple(pcs))
            if key in mos_seen:
                continue
            mos_seen.add(key)
            a, b = sorted(set(steps), reverse=True)
            big, small = steps.count(a), steps.count(b)
            add(f"MOS {L} notas ({big}G+{small}p), gerador {g}\\{n}", "MOS (escala bem formada)",
                [1200 * p / n for p in pcs], MOS_SRC,
                f"gerador de {1200 * g / n:.1f} cents em {n}-EDO; passos de {a} e {b} partes")
            mos_count += 1

# ---- genera de Euler-Fokker -------------------------------------------------------
# Todos os produtos dos fatores de um multiconjunto (com 1), reduzidos à oitava.
EF_SRC = ("L. Euler, Tentamen novae theoriae musicae, 1739 (genera musica); A. D. Fokker, "
          "Unsere Musik und die Musik der Zukunft / Les mathématiques et la musique, 1947 (genera de Euler-Fokker)")


def reduce_oct(r):
    while r >= 2:
        r /= 2
    while r < 1:
        r *= 2
    return r


import itertools

ef_seen = set()
for size in range(2, 5):
    for combo in itertools.combinations_with_replacement([3, 5, 7, 11], size):
        prods = {F(1)}
        for f in combo:
            prods |= {p * f for p in prods}
        rs = sorted({reduce_oct(F(p)) for p in prods})
        if not 4 <= len(rs) <= 24 or tuple(rs) in ef_seen:
            continue
        ef_seen.add(tuple(rs))
        from_ratios("Euler-Fokker [" + " ".join(map(str, combo)) + "]", "genus de Euler-Fokker (entonação justa)", rs, EF_SRC)

# ---- conjuntos de produto combinatório (CPS) --------------------------------
# Escolhe k fatores de um conjunto de n e multiplica: hexany (2 de 4),
# dekany (2 ou 3 de 5), eikosany (3 de 6); reduzidos à oitava.
CPS_SRC = "E. Wilson, combination product sets (1960–1990), em K. Narushima, Microtonality and the Tuning Systems of Erv Wilson, Routledge, 2018"
CPS_NAME = {(4, 2): "hexany", (5, 2): "dekany 2)5", (5, 3): "dekany 3)5", (6, 3): "eikosany"}
cps_seen = set()
for (n, k), cname in CPS_NAME.items():
    for factors in itertools.combinations([1, 3, 5, 7, 9, 11, 13], n):
        prods = [reduce_oct(F(math.prod(c))) for c in itertools.combinations(factors, k)]
        rs = sorted(set(prods))
        base = rs[0]
        rs = sorted({reduce_oct(r / base) for r in rs})
        if tuple(rs) in cps_seen or len(rs) < 5:
            continue
        cps_seen.add(tuple(rs))
        from_ratios(f"{cname} {{{' '.join(map(str, factors))}}}", "conjunto de produto combinatório (Wilson)", rs, CPS_SRC,
                    "reduzido à oitava e tomado a partir do menor produto")

# ---- segmentos da série harmônica e subarmônica ----------------------------
for m in range(5, 33):
    if m in (8, 16):
        continue  # já estão acima
    from_ratios(f"série harmônica {m}–{2 * m}", "série harmônica (modo de Partch/Johnston)",
                [F(k, m) for k in range(m, 2 * m)],
                "a série harmônica (parciais m a 2m−1); H. Partch, Genesis of a Music, 1949; B. Johnston, \"Scalar Order as a Compositional Resource\", Perspectives of New Music 2(2), 1964")
    from_ratios(f"série subarmônica {2 * m}–{m}", "série subarmônica",
                [F(2 * m, k) for k in range(2 * m, m, -1)],
                "a série subarmônica; H. Partch, Genesis of a Music, 1949 (utonalidade)")

# ---- mais mesotônicos (frações de coma) ---------------------------------------
for frac, label in [(1 / 7, "1/7"), (1 / 8, "1/8"), (1 / 9, "1/9"), (1 / 10, "1/10"), (1 / 11, "1/11"), (2 / 9, "2/9"), (3 / 10, "3/10")]:
    meantone(frac, f"mesotônico de {label} de coma", "família dos mesotônicos regulares; ver J. M. Barbour, Tuning and Temperament, 1951")

# ---- maqamat por ajnas (gêneros de 4 notas) ------------------------------------
# Um jins embaixo (na tônica) e outro em cima, na quinta (disjunto, com um
# tom inteiro no meio): o modelo de 24 partes. Os nomes compostos indicam a
# construção; vários coincidem com maqamat conhecidos.
MARCUS = ("S. Marcus, Arab Music Theory in the Modern Period (tese), UCLA, 1989; "
          "H. H. Touma, The Music of the Arabs, 1996")
AJNAS = {"rast": [0, 200, 350, 500], "bayati": [0, 150, 300, 500], "hijaz": [0, 100, 400, 500],
         "nahawand": [0, 200, 300, 500], "kurd": [0, 100, 300, 500], "ajam": [0, 200, 400, 500],
         "saba": [0, 150, 300, 400], "sikah (trichorde)": [0, 150, 350]}
for lo, a in AJNAS.items():
    for hi, b in AJNAS.items():
        if lo.startswith("saba") and hi != "hijaz":
            continue  # saba não fecha a quarta; só a combinação tradicional com hijaz
        add(f"jins {lo} + jins {hi} (na quinta)", "maqam árabe (ajnas combinados, modelo de 24 partes)",
            a + [700 + x for x in b], MARCUS,
            "construção por ajnas; o nome do maqam real depende do sayr e da tradição")

# ---- Messiaen: modos de transposição limitada ----------------------------------
MESS = "O. Messiaen, Technique de mon langage musical, Leduc, 1944"
for i, pcs in enumerate([[0, 2, 4, 6, 8, 10], [0, 1, 3, 4, 6, 7, 9, 10], [0, 2, 3, 4, 6, 7, 8, 10, 11],
                         [0, 1, 2, 5, 6, 7, 8, 11], [0, 1, 5, 6, 7, 11], [0, 2, 4, 5, 6, 8, 10, 11],
                         [0, 1, 2, 3, 5, 6, 7, 8, 9, 11]], start=1):
    add(f"Messiaen modo {i}", "modo de transposição limitada", [100 * p for p in pcs], MESS)

# ---- conjuntos de classes de altura (forma prima, 5 a 9 notas) ---------------
FORTE = ("A. Forte, The Structure of Atonal Music, Yale UP, 1973; forma prima pelo algoritmo de "
         "J. Rahn, Basic Atonal Theory, Longman, 1980")


def prime_form(pcs):
    best = None
    for inv in (pcs, [(-p) % 12 for p in pcs]):
        s = sorted(inv)
        for r in range(len(s)):
            rot = s[r:] + [x + 12 for x in s[:r]]
            norm = [x - rot[0] for x in rot]
            # Rahn: compara do intervalo externo para dentro
            key = tuple(reversed(norm))
            if best is None or key < best[0]:
                best = (key, norm)
    return tuple(best[1])


primes = set()
for mask in range(1 << 12):
    pcs = [p for p in range(12) if mask >> p & 1]
    if 5 <= len(pcs) <= 9:
        primes.add(prime_form(pcs))
for pf in sorted(primes, key=lambda x: (len(x), x)):
    add("conjunto [" + ",".join(f"{p:X}" if p > 9 else str(p) for p in pf) + "]",
        f"conjunto de classes de altura ({len(pf)} notas)", [100 * p for p in pf], FORTE,
        "forma prima; A = 10, B = 11")


# ---- comportamento das escalas (RASGO_DOCUMENTATION/teoria/…, §6.12) -------
# Quando a fonte descreve como a linguagem USA a escala: subida e descida,
# notas de apoio (a primeira é a principal), percurso (as regiões que a
# melodia visita ao longo da peça), frases-assinatura, ornamento, cadência e
# textura (bordão ou acordes). Valores em cents a partir da tônica (podem
# passar da oitava ou ficar abaixo da tônica). Frases construídas pelo
# Trioio, e não transcritas da fonte, vêm marcadas na nota do comportamento.

def behavior(name, source, note="", **fields):
    keys = {"subida": "subida", "descida": "descida", "apoio": "apoio", "percurso": "percurso",
            "assinatura": "assinatura", "ornamento": "ornamento", "cadencia": "cadência", "textura": "textura",
            "ritmo": "ritmo", "vamp": "vamp", "voicing": "voicing"}
    for i, b in enumerate(blocks):
        if b.startswith(f"= {name}\n"):
            lines = [b]
            for k, label in keys.items():
                if k in fields:
                    v = fields[k]
                    if isinstance(v, (list, tuple)) and v and isinstance(v[0], (list, tuple)):
                        v = " ; ".join(" ".join(f"{x:.3f}" for x in ph) for ph in v)
                    elif k == "vamp":
                        v = " ".join(str(int(x)) for x in v)
                    elif isinstance(v, (list, tuple)):
                        v = " ".join(f"{x:.3f}" for x in v)
                    lines.append(f"{label}: {v}")
            lines.append(f"comportamento-fonte: {source}")
            if note:
                lines.append(f"comportamento-nota: {note}")
            blocks[i] = "\n".join(lines)
            return
    raise KeyError(name)


FARRAJ = ("J. Farraj e S. Abu Shumays, Inside Arabic Music, Oxford UP, 2019; S. Marcus, Arab Music Theory in the Modern Period, 1989; "
          "a heterofonia do takht: A. J. Racy, Making Music in the Arab World: The Culture and Artistry of Ṭarab, Cambridge UP, 2003")
BUILT = "as frases-assinatura foram construídas pelo Trioio a partir da descrição (não são transcrições)"
behavior("maqam Rast", FARRAJ, BUILT + "; na descida o 7.º grau neutro costuma virar Sib (jins nahawand sobre Sol)",
         subida=[0, 200, 350, 500, 700, 900, 1050, 1200], descida=[1200, 1000, 900, 700, 500, 350, 200, 0],
         apoio=[0, 700, 350], percurso=[0, 350, 700, 1200, 700, 0],
         assinatura=[[0, 200, 350, 500, 350, 200, 0], [700, 900, 1050, 1200, 1000, 900, 700]],
         cadencia=[500, 350, 200, 0], textura="heterofonia", ritmo="maqsum")
behavior("maqam Bayati", FARRAJ, BUILT + "; o ghammaz (nota de apoio) é a quarta",
         subida=[0, 150, 300, 500, 700, 800, 1000, 1200], descida=[1200, 1000, 800, 700, 500, 300, 150, 0],
         apoio=[0, 500, 300], percurso=[0, 300, 500, 700, 500, 0],
         assinatura=[[0, 150, 300, 150, 0], [500, 300, 150, 0]], cadencia=[300, 150, 0], textura="heterofonia", ritmo="baladi")
behavior("maqam Hijaz", FARRAJ, BUILT + "; a segunda aumentada (Mib–Fá#) é a identidade do jins hijaz",
         subida=[0, 100, 400, 500, 700, 800, 1000, 1200], descida=[1200, 1000, 800, 700, 500, 400, 100, 0],
         apoio=[0, 500, 400], percurso=[0, 400, 500, 700, 500, 0],
         assinatura=[[0, 100, 400, 500, 400, 100, 0], [500, 700, 800, 700, 500]], cadencia=[400, 100, 0], textura="heterofonia", ritmo="maqsum")
behavior("maqam Saba", FARRAJ, BUILT + "; o ghammaz é a terça; o Saba não fecha a oitava, e o âmbito é curto",
         subida=[0, 150, 300, 400, 700, 800, 1000], descida=[1000, 800, 700, 400, 300, 150, 0],
         apoio=[300, 0], percurso=[0, 300, 400, 300, 0],
         assinatura=[[0, 150, 300, 400, 300, 150, 0]], cadencia=[300, 150, 0], textura="heterofonia", ritmo="wahda")

# rāgas: novas entradas, em entonação justa como os thāts
JAIR = "N. A. Jairazbhoy, The Rāgs of North Indian Music, Faber, 1971; V. N. Bhatkhande, Hindustānī Saṅgīt Paddhati, 1909–1932"
YAMAN = [0, cents(F(9, 8)), cents(F(5, 4)), cents(F(45, 32)), cents(F(3, 2)), cents(F(5, 3)), cents(F(15, 8))]
add("rāga Yaman", "Índia hindustani (rāga)", YAMAN, JAIR,
    "thāt Kalyān; primeira parte da noite; entonação justa como modelo")
N_ = YAMAN[6] - 1200  # Ni da oitava de baixo
P_ = YAMAN[4] - 1200
behavior("rāga Yaman", JAIR, "pakaḍ usual: ṇi re ga re, ṇi re sa, pa̤ re sa; vādī Ga, saṃvādī Ni; a subida evita Sa e Pa",
         subida=[N_, YAMAN[1], YAMAN[2], YAMAN[3], YAMAN[5], YAMAN[6], 1200],
         descida=[1200, YAMAN[6], YAMAN[5], YAMAN[4], YAMAN[3], YAMAN[2], YAMAN[1], 0],
         apoio=[YAMAN[2], YAMAN[6], 0], percurso=[0, YAMAN[2], YAMAN[4], YAMAN[6], 1200, YAMAN[2], 0],
         assinatura=[[N_, YAMAN[1], YAMAN[2], YAMAN[1]], [N_, YAMAN[1], 0], [P_, YAMAN[1], 0]],
         cadencia=[YAMAN[2], YAMAN[1], 0], ornamento="meend", textura="bordão", ritmo="tīntāl")
BHAIRAV = [0, cents(F(16, 15)), cents(F(5, 4)), cents(F(4, 3)), cents(F(3, 2)), cents(F(8, 5)), cents(F(15, 8))]
add("rāga Bhairav", "Índia hindustani (rāga)", BHAIRAV, JAIR, "thāt Bhairav; o amanhecer; entonação justa como modelo")
behavior("rāga Bhairav", JAIR, "pakaḍ usual: ga ma dha dha pa, ga ma re re sa; vādī dha (komal), saṃvādī re (komal); "
         "andolan (oscilação lenta) em re e dha, aqui aproximada por inflexão",
         subida=BHAIRAV + [1200], descida=[1200] + BHAIRAV[::-1][:-1] + [0],
         apoio=[BHAIRAV[5], BHAIRAV[1], 0], percurso=[0, BHAIRAV[2], BHAIRAV[5], 1200, BHAIRAV[5], BHAIRAV[1], 0],
         assinatura=[[BHAIRAV[2], BHAIRAV[3], BHAIRAV[5], BHAIRAV[5], BHAIRAV[4]],
                     [BHAIRAV[2], BHAIRAV[3], BHAIRAV[1], BHAIRAV[1], 0]],
         cadencia=[BHAIRAV[2], BHAIRAV[1], 0], ornamento="andolan", textura="bordão", ritmo="ektāl")

# carnático: o gamaka (kampita) como parte da nota
behavior("melakarta 29 Dhīraśaṅkarābharaṇam", "P. Sambamoorthy, South Indian Music, Indian Music Publishing House, 1958–1969",
         "o kampita (oscilação) em ri, ga, dha e ni; Sa e Pa firmes; sem frases-assinatura (a rāga real escolhe as suas)",
         apoio=[0, 700, 400], ornamento="gamaka", textura="bordão", ritmo="ādi tāla")

# pathet do slendro (modelo de 5 partes iguais: 1=0, 2=240, 3=480, 5=720, 6=960)
HOOD = "M. Hood, The Nuclear Theme as a Determinant of Patet in Javanese Music, Groningen, 1954"
for nm, sup in [("slendro pathet nem", [960, 240, 720]), ("slendro pathet sanga", [720, 240, 0]),
                ("slendro pathet manyura", [960, 480, 240])]:
    add(nm, "Indonésia (gamelão, pathet)", [240 * k for k in range(5)], KUNST + "; " + HOOD,
        "modelo equidistante de 5 partes; os tons de cadência de cada pathet A CONFERIR em Hood 1954")
    behavior(nm, HOOD + "; a elaboração em camadas: J. Becker, Gamelan Stories, 1993",
             "tons de apoio e de gong do pathet (a conferir); o primeiro é o tom final; textura em camadas (heterofonia)",
             apoio=sup, cadencia=[sup[1], sup[0]], percurso=[sup[0], sup[1], sup[2], sup[0]], textura="heterofonia",
             ritmo="ketawang" if nm.endswith("manyura") else "lancaran")

# blues: as notas inflexionadas
add("blues (com blue notes)", "blues", [0, 300, 500, 600, 700, 1000],
    "G. Kubik, Africa and the Blues, University Press of Mississippi, 1999",
    "pentatônica menor com a quinta diminuta; as blue notes são inflexões, não graus fixos")
behavior("blues (com blue notes)", "G. Kubik, Africa and the Blues, 1999",
         "a terça, a quinta e a sétima chegam de baixo, arrastadas (ornamento blue); frases construídas pelo Trioio",
         apoio=[0, 700, 300], assinatura=[[700, 1000, 700, 500, 300, 0]], cadencia=[300, 0], ornamento="blue",
         textura="acordes", ritmo="shuffle")

# o modo de Mi andaluz e a cadência frígia
add("modo de Mi andaluz", "flamenco (frígio com terça maior)", [0, 100, 400, 500, 700, 800, 1000],
    "P. Manuel, \"Modal Harmony in Andalusian, Eastern European, and Turkish Syncretic Musics\", Yearbook for Traditional Music 21, 1989")
behavior("modo de Mi andaluz", "P. Manuel, 1989",
         "a melodia desce ao final pelo semitom de cima (Fá–Mi), sobre a cadência andaluza; frases construídas pelo Trioio",
         apoio=[0, 500, 800], assinatura=[[800, 700, 500, 400, 100, 0]], cadencia=[500, 400, 100, 0], textura="acordes", ritmo="compás de 12 (soleá, bulerías)")


# ---- mais linguagens com comportamento (2026-10-11) ---------------------------
# maqamat (Farraj e Abu Shumays): o ghammaz é a nota de apoio do jins de cima
behavior("maqam Nahawand", FARRAJ, BUILT + "; sobe com o jins hijaz sobre a quinta (a sensível) e desce pelo jins "
         "kurd/nahawand (a sétima menor): a mesma escala muda com a direção",
         subida=[0, 200, 300, 500, 700, 800, 1100, 1200], descida=[1200, 1000, 800, 700, 500, 300, 200, 0],
         apoio=[0, 700, 300], percurso=[0, 300, 500, 700, 1200, 700, 0],
         assinatura=[[0, 200, 300, 500, 300, 200, 0], [700, 800, 1100, 1200, 1000, 800, 700]],
         cadencia=[300, 200, 0], textura="heterofonia", ritmo="maqsum")
behavior("maqam Kurd", FARRAJ, BUILT + "; o jins kurd (semitom no 2.º grau) desce à tônica pelo semitom; ghammaz na quarta",
         apoio=[0, 500, 300], percurso=[0, 300, 500, 700, 500, 0],
         assinatura=[[500, 300, 100, 0], [0, 100, 300, 500, 300, 100, 0]], cadencia=[300, 100, 0],
         textura="heterofonia", ritmo="wahda")
behavior("maqam Ajam", FARRAJ, BUILT + "; o jins ajam (maior) com ghammaz na quinta; a cadência chega à tônica de cima",
         apoio=[0, 700, 400], percurso=[0, 400, 700, 1200, 700, 0],
         assinatura=[[0, 200, 400, 500, 700, 500, 400, 200, 0], [700, 900, 1100, 1200, 1100, 900, 700]],
         cadencia=[400, 200, 0], textura="heterofonia", ritmo="maqsum")
behavior("maqam Sikah", FARRAJ, BUILT + "; a tônica é a nota de três quartos (Mi meio-bemol no modelo); ghammaz no 3.º grau "
         "(o jins rast acima do sikah); âmbito estreito ao redor da tônica",
         apoio=[0, 350, 700], percurso=[0, 350, 700, 350, 0],
         assinatura=[[0, 150, 350, 150, 0], [350, 550, 700, 550, 350, 150, 0]], cadencia=[150, 0],
         textura="heterofonia", ritmo="wahda")

# rāgas novas (Bhatkhande; Jairazbhoy): vādī e saṃvādī usuais, que variam entre escolas
RAGAS = [
    ("rāga Bhūpālī", [0, cents(F(9, 8)), cents(F(5, 4)), cents(F(3, 2)), cents(F(5, 3))],
     "thāt Kalyān, pentatônica (sem ma e ni); primeira parte da noite", 2, 4,
     "pakaḍ usual: ga re sa ḍha, sa re ga, pa ga, ḍha pa ga re sa; vādī Ga, saṃvādī Dha",
     [[2, 1, 0, -1], [0, 1, 2, 3, 2], [4, 3, 2, 1, 0]], "meend", "tīntāl"),
    ("rāga Mālkauns", [0, cents(F(6, 5)), cents(F(4, 3)), cents(F(8, 5)), cents(F(9, 5))],
     "thāt Bhairavī, pentatônica (sem re e pa); tarde da noite", 2, 0,
     "pakaḍ usual: ma ga ma dha ni dha ma ga ma ga sa; vādī Ma, saṃvādī Sa; andolan em ga e dha",
     [[2, 1, 2, 3, 4, 3, 2, 1, 2, 1, 0]], "andolan", "tīntāl"),
    ("rāga Bhairavī", [0, cents(F(16, 15)), cents(F(6, 5)), cents(F(4, 3)), cents(F(3, 2)), cents(F(8, 5)), cents(F(9, 5))],
     "thāt Bhairavī (todos os graus móveis bemóis); manhã, e o fecho tradicional de um concerto", 3, 0,
     "vādī Ma, saṃvādī Sa (há escolas que dão Dha/Ga); em performance livre admite notas de fora",
     [[3, 2, 1, 0], [4, 5, 4, 3, 2, 1, 0]], "meend", "tīntāl"),
    ("rāga Kāfī", [0, cents(F(9, 8)), cents(F(6, 5)), cents(F(4, 3)), cents(F(3, 2)), cents(F(5, 3)), cents(F(9, 5))],
     "thāt Kāfī (ga e ni bemóis); a meia-noite; base de muitas formas leves (ṭhumrī, holī)", 4, 0,
     "vādī Pa, saṃvādī Sa (variam entre escolas)",
     [[0, 1, 2, 3, 4, 3, 2, 1, 0], [4, 5, 6, 7, 6, 5, 4]], "meend", "tīntāl"),
]
for nm, steps, desc, vadi, samvadi, pakad, frases, orn, tala in RAGAS:
    add(nm, "Índia hindustani (rāga)", steps, JAIR, desc + "; entonação justa como modelo")
    deg = lambda k: (k // len(steps)) * 1200 + steps[k % len(steps)]
    behavior(nm, JAIR, pakad + "; frases construídas pelo Trioio a partir do pakaḍ (não são transcrições)",
             apoio=[steps[vadi], steps[samvadi] if samvadi else 0, 0],
             assinatura=[[deg(k) for k in f] for f in frases],
             cadencia=[deg(2), deg(1), 0] if len(steps) > 5 else [deg(1), 0],
             ornamento=orn, textura="bordão", ritmo=tala)

# os modos nordestinos (Siqueira): a nota característica no apoio, o baião
behavior("modo nordestino (mixolídio)", SIQ, "a 7.ª menor é a cor do aboio, da cantoria e do baião; cadência bVII–I; "
         "frases construídas pelo Trioio", apoio=[0, 1000, 700], assinatura=[[700, 900, 1000, 900, 700, 400, 200, 0]],
         cadencia=[-200, 0], textura="acordes", ritmo="baião")
behavior("modo nordestino (lídio com sétima menor)", SIQ, "a 4.ª aumentada e a 7.ª menor juntas (o \"modo nordestino\" por "
         "excelência; L. Gonzaga, Asa Branca, 1947, ronda essa cor); frases construídas pelo Trioio",
         apoio=[0, 600, 1000], assinatura=[[0, 200, 400, 600, 700, 600, 400, 200, 0], [700, 1000, 900, 700, 600, 400]],
         cadencia=[600, 400, 200, 0], textura="acordes", ritmo="baião")
behavior("modo nordestino (dórico)", SIQ, "a 6.ª maior sobre o menor; frases construídas pelo Trioio",
         apoio=[0, 900, 700], assinatura=[[0, 300, 500, 700, 900, 700, 500, 300, 0]], cadencia=[300, 200, 0],
         textura="acordes", ritmo="baião")

# Japão (Koizumi): tetracordes com tons nucleares a uma quarta; o in desce ao núcleo por semitom
KOIZUMI = "F. Koizumi, Nihon dentō ongaku no kenkyū (Estudos da música tradicional japonesa), Ongaku no Tomo, 1958"
behavior("in (japonesa)", KOIZUMI, "tons nucleares 0, 500 e 1200 (as quartas); no miyako-bushi a melodia cai ao núcleo pelo "
         "semitom de cima; frases construídas pelo Trioio",
         apoio=[0, 500, 700], assinatura=[[700, 500, 100, 0], [1200, 800, 700, 500]], cadencia=[100, 0],
         textura="heterofonia")
behavior("yō (japonesa)", KOIZUMI, "tons nucleares 0, 500 e 1200; tetracordes sem semitom (min'yō); frases construídas pelo Trioio",
         apoio=[0, 500, 700], assinatura=[[700, 500, 200, 0], [1200, 900, 700, 500]], cadencia=[200, 0],
         textura="heterofonia")

# China: a pentatônica e o seu modo de chegada
behavior("pentatônica chinesa (gong shang jue zhi yu)",
         "F. Lau, Music in China: Experiencing Music, Expressing Culture, Oxford UP, 2008",
         "o modo gong (sobre a primeira nota) com apoio em zhi (a quinta); melodia por graus conjuntos da pentatônica, "
         "elaborada em heterofonia pelo conjunto; frases construídas pelo Trioio",
         apoio=[0, 701.955, 407.82], assinatura=[[0, 203.91, 407.82, 701.955, 407.82, 203.91, 0]],
         cadencia=[203.91, 0], textura="heterofonia")


# ---- a prática modal: nota característica, cadência modal, vamp e pedal ------
# Os modos como linguagem, não só como conjunto: a nota característica (o
# grau que distingue o modo do maior ou do menor) vira apoio; a cadência
# modal evita a sensível; o vamp (dois acordes do modo alternando sobre um
# pedal na tônica) sustenta o modo sem a função tonal. "vamp" são os graus
# (0 = I) das fundamentais dos dois acordes.
PERS = ("V. Persichetti, Twentieth-Century Harmony, Norton, 1961 (modos, nota característica, cadências modais); "
        "M. Levine, The Jazz Theory Book, Sher Music, 1995 (jazz modal, vamps)")
MODAL = [
    ("modo dórico (prática modal)", [0, 200, 300, 500, 700, 900, 1000], 900, [900, 700, 300, 0], [0, 3],
     "a 6.ª maior é a nota característica; vamp i–IV (a tônica menor e o IV maior, como em \"Oye Como Va\", T. Puente, 1962); "
     "o jazz modal sustenta o dórico por longos trechos (M. Davis, \"So What\", Kind of Blue, 1959)"),
    ("modo frígio (prática modal)", [0, 100, 300, 500, 700, 800, 1000], 100, [300, 100, 0], [0, 1],
     "a 2.ª menor é a nota característica; cadência bII–i, o b2 descendo à tônica"),
    ("modo lídio (prática modal)", [0, 200, 400, 600, 700, 900, 1100], 600, [600, 700, 400, 0], [0, 1],
     "a 4.ª aumentada é a nota característica; vamp I–II (o II maior traz o #4)"),
    ("modo mixolídio (prática modal)", [0, 200, 400, 500, 700, 900, 1000], 1000, [-200, 0], [0, 6],
     "a 7.ª menor é a nota característica; cadência bVII–I, sem sensível"),
    ("modo eólio (prática modal)", [0, 200, 300, 500, 700, 800, 1000], 800, [800, 700, -200, 0], [0, 5],
     "a 6.ª menor é a nota característica; vamp i–bVI e a chegada pelo bVII"),
    ("modo lócrio (prática modal)", [0, 100, 300, 500, 600, 800, 1000], 600, [100, 0], [0, 1],
     "a 5.ª diminuta é a nota característica (a tônica é instável); vamp i°–bII"),
]
for nm, steps, char, cad, vamp, note in MODAL:
    add(nm, "modo (prática modal)", steps, PERS, "modelo em 12 sons iguais")
    extra = {"modo dórico (prática modal)": {"voicing": "sowhat"}, "modo mixolídio (prática modal)": {"voicing": "quartal"}}.get(nm, {})
    behavior(nm, PERS, note + ("; voicing " + extra["voicing"] + " (jazz modal)" if extra else ""),
             apoio=[0, char, 700 if 700 in steps else steps[4]], cadencia=cad, textura="vamp", vamp=vamp, **extra)


def main():
    out = sys.argv[1]
    header = (
        "# Arquivo de escalas do Trioio\n"
        "# Escrito pelo Trioio a partir do conhecimento teórico (razões, cents, construções),\n"
        "# com a fonte citada em cada escala. Nenhum arquivo de terceiros foi copiado.\n"
        "# Gerado por tools/gerar_arquivo_de_escalas.py; formato descrito ali.\n"
        f"# {len(blocks)} escalas.\n"
    )
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(header + "\n" + "\n\n".join(blocks) + "\n")
    print(f"{len(blocks)} escalas em {out}")


if __name__ == "__main__":
    main()
