#!/usr/bin/env python3
"""Gera o arquivo de cadências do Trioio (data/harmonia/arquivo_trioio_cadencias.txt).

Escrito pelo Trioio a partir da teoria publicada; nenhum arquivo de
terceiros é copiado. Cada cadência cita a fonte, como numa pesquisa
(docs/MAQUINA_DE_HARMONIA.md §1, §3.3, §6 e §8).

    python3 tools/gerar_arquivo_de_cadencias.py data/harmonia/arquivo_trioio_cadencias.txt

Formato (um bloco por cadência):

    = autêntica perfeita
    família: funcional
    linguagem: funcional          (a linguagem da máquina que a escreve, ou "—")
    graus: IV – V – I
    acordes: 5 M | 7 M | 0 M      (lido pelo core: ver abaixo)
    soprano: 1 – 2 – 1            (os graus da voz de cima, ou "—")
    baixo: 4 – 5 – 1
    efeito: o ponto final
    fonte: …

Os acordes, separados por "|": o intervalo da fundamental em semitons acima
da tônica (0–11), a qualidade (M, m, dim, aug) e as cores, em qualquer ordem:
7, 7M, 9, b9, #11, 13, 6, sus4. Opcional: "/n", o baixo em semitons acima da
tônica (a inversão ou o pedal).
"""

import re
import sys

blocks = []
QUAL = {"M", "m", "dim", "aug"}
COLORS = {"7", "7M", "9", "b9", "#11", "13", "6", "sus4"}


def add(name, family, language, degrees, chords, soprano, bass, effect, source, saliencia=""):
    for c in chords.split("|"):
        toks = c.split()
        assert 0 <= int(toks[0]) <= 11, (name, c)
        assert toks[1] in QUAL, (name, c)
        for t in toks[2:]:
            assert t in COLORS or re.fullmatch(r"/\d{1,2}", t), (name, t)
    assert len(chords.split("|")) == len(degrees.split("–")), (name, "graus e acordes")
    blocks.append("\n".join([
        f"= {name}", f"família: {family}", f"linguagem: {language}", f"graus: {degrees}",
        f"acordes: {chords}", f"soprano: {soprano}", f"baixo: {bass}", f"efeito: {effect}", f"fonte: {source}"]
        + ([f"saliência: {saliencia}"] if saliencia else [])))


PISTON = "W. Piston, Harmony, 1941 (5ª ed., M. DeVoto, 1987); A. Schoenberg, Harmonielehre, 1911"
ALDWELL = "E. Aldwell e C. Schachter, Harmony and Voice Leading, 4ª ed., 2011"
LEVINE = "M. Levine, The Jazz Theory Book, Sher Music, 1995"
CHEDIAK = "A. Chediak, Harmonia e improvisação, Lumiar, 1986"
SEVE = "M. Sève, Vocabulário do choro, Lumiar, 1999"
SIQUEIRA = "J. Siqueira, O Sistema Modal na Música Folclórica do Brasil, João Pessoa, 1981"
COHN = "R. Cohn, Audacious Euphony, Oxford UP, 2012"
MANUEL = "P. Manuel, \"From Scarlatti to 'Guantanamera'\", JAMS 55(2), 2002"
DEVOTO = "M. DeVoto, Debussy and the Veil of Tonality, Pendragon, 2004"
MESSIAEN = "O. Messiaen, Technique de mon langage musical, 1944"
LENDVAI = "E. Lendvai, Béla Bartók: An Analysis of His Music, Kahn & Averill, 1971"
BAKER = "J. Baker, The Music of Alexander Scriabin, Yale UP, 1986"
TYMOCZKO = "D. Tymoczko, A Geometry of Music, Oxford UP, 2011"

# ---- funcional (a prática comum) ---------------------------------------------------------------
add("autêntica perfeita", "funcional", "funcional", "IV – V – I", "5 M | 7 M | 0 M", "1 – 2 – 1", "4 – 5 – 1",
    "o ponto final: fundamental no baixo, tônica em cima", PISTON)
add("autêntica perfeita com cadencial 6/4", "funcional", "funcional", "ii6 – I6/4 – V7 – I",
    "2 m /5 | 0 M /7 | 7 M 7 | 0 M", "4 – 3 – 2 – 1", "4 – 5 – 5 – 1",
    "o I6/4 adia a dominante; o fim mais solene", ALDWELL,
    saliencia="7:2, 11:3, 2:1, 5:2")
add("autêntica imperfeita", "funcional", "funcional", "V – I6", "7 M | 0 M /4", "2 – 1", "5 – 3",
    "a vírgula forte: a terça no baixo", PISTON)
add("suspensiva (meia cadência)", "funcional", "funcional", "ii – V", "2 m | 7 M", "4 – 2", "2 – 5",
    "a pergunta", PISTON)
add("frígia", "funcional", "funcional", "iv6 – V", "5 m /8 | 7 M", "1 – 2", "♭6 – 5",
    "a pergunta arcaica, barroca, no menor", ALDWELL)
add("frígia modal (século XV)", "modal", "modal", "♭vii – I", "10 m /1 | 0 M", "2 – 1", "♭2 – 1",
    "o ♭2 que desce à tônica: a sensível de cima, sem a de baixo",
    "E. Cambouropoulos et al., a cadência frígia como entrada da mistura conceitual (ISMIR 2015); ver MAQUINA_DE_HARMONIA §11",
    saliencia="10:1, 1:3, 5:2")
add("plagal", "funcional", "funcional", "IV – I", "5 M | 0 M", "1 – 1", "4 – 1", "o \"amém\"", PISTON)
add("plagal menor", "funcional", "funcional", "iv – I", "5 m | 0 M", "1 – 1", "4 – 1",
    "o \"amém\" com o empréstimo do menor: o ♭6 que desce", ALDWELL)
add("de engano", "funcional", "funcional", "V – vi", "7 M | 9 m", "2 – 1", "5 – 6",
    "a surpresa: o ouvido espera a tônica", PISTON)
add("evitada", "funcional", "funcional", "V7 – V7/IV", "7 M 7 | 0 M 7", "4 – 3", "5 – 1",
    "o fim adiado: a tônica vira dominante", PISTON)
add("picarda", "funcional", "funcional", "V – I (no menor)", "7 M | 0 M", "2 – 3", "5 – 1",
    "uma peça menor que termina no I maior: a luz no fim", ALDWELL)
add("napolitana", "funcional", "funcional", "♭II6 – V – i", "1 M /5 | 7 M | 0 m", "♭2 – 7 – 1", "4 – 5 – 1",
    "o ♭2 que desce à sensível", ALDWELL)
add("sexta aumentada alemã", "funcional", "funcional", "Ger+6 – I6/4 – V – i",
    "8 M 7 | 0 m /7 | 7 M | 0 m", "1 – 1 – 7 – 1", "♭6 – 5 – 5 – 1",
    "as duas notas que abrem para a oitava da dominante", ALDWELL)

# ---- jazz -----------------------------------------------------------------------------------------
add("ii–V–I", "jazz", "jazz", "IIm7 – V7 – I7M", "2 m 7 | 7 M 7 | 0 M 7M", "1 – 7 – 7", "2 – 5 – 1",
    "a cadência do jazz: as 7ªs que descem às 3ªs", LEVINE)
add("ii–V–i menor", "jazz", "jazz", "IIm7(♭5) – V7(♭9) – Im7", "2 dim 7 | 7 M 7 b9 | 0 m 7", "♭6 – ♭6 – 5", "2 – 5 – 1",
    "o ii–V do menor, com a ♭9", LEVINE)
add("substituição de trítono", "jazz", "jazz", "IIm7 – ♭II7 – I7M", "2 m 7 | 1 M 7 | 0 M 7M", "1 – 7 – 7", "2 – ♭2 – 1",
    "o baixo desce por semitom", LEVINE)
add("backdoor", "jazz", "jazz", "IVm7 – ♭VII7 – I7M", "5 m 7 | 10 M 7 | 0 M 7M", "♭6 – ♭6 – 5", "4 – ♭7 – 1",
    "a dominante pela porta dos fundos", LEVINE)
add("turnaround", "jazz", "jazz", "I – VI7 – IIm7 – V7", "0 M 6 | 9 M 7 | 2 m 7 | 7 M 7", "3 – ♯1 – 1 – 7",
    "1 – 6 – 2 – 5", "a volta ao começo", LEVINE)

# ---- brasileira -----------------------------------------------------------------------------------
add("ii–V–I6/9", "brasileira", "brasileira", "IIm7 – V7(♭9) – I6(9)", "2 m 7 | 7 M 7 b9 | 0 M 6 9",
    "1 – ♭6 – 5", "2 – 5 – 1", "a cadência do samba e da bossa", CHEDIAK)
add("diminuto de passagem", "brasileira", "brasileira", "I – ♯I° – IIm7 – V7", "0 M | 1 dim 6 | 2 m 7 | 7 M 7",
    "5 – 5 – 4 – 4", "1 – ♯1 – 2 – 5", "o baixo cromático do choro", SEVE)
add("V7/IV do choro", "brasileira", "brasileira", "I7 – IV – ♯IV° – I/5", "0 M 7 | 5 M | 6 dim 6 | 0 M /7",
    "♭7 – 6 – 6 – 5", "1 – 4 – ♯4 – 5", "a ida à subdominante e a volta pela baixaria", SEVE)
add("7M(♯11) de chegada", "brasileira", "brasileira", "♭VII7 – I7M(♯11)", "10 M 7 | 0 M 7M #11", "4 – ♯4", "♭7 – 1",
    "o empréstimo do menor e a cor lídia", CHEDIAK)

# ---- nordestina, mineira, Hermeto ------------------------------------------------------------------
add("baião ♭VII → I7", "nordestina", "nordestina", "♭VII – I7", "10 M | 0 M 7", "2 – 1", "♭7 – 1",
    "o mixolídio: a tônica com a 7ª menor", SIQUEIRA)
add("IV7 → I7", "nordestina", "nordestina", "IV7 – I7", "5 M 7 | 0 M 7", "♭7 – 1", "4 – 1",
    "o plagal do baião, com os dois acordes de 7ª", SIQUEIRA)
add("lídia II → I", "nordestina", "nordestina", "II – I", "2 M | 0 M", "♯4 – 3", "2 – 1",
    "o modo nordestino (lídio-mixolídio): a quarta aumentada", SIQUEIRA)
add("Vm7 → I", "mineira", "mineira", "Vm7 – I", "7 m 7 | 0 M", "♭7 – 1", "5 – 1",
    "a dominante sem sensível do Clube da Esquina", "ver MAQUINA_DE_HARMONIA §6 (o Clube da Esquina)")
add("IVsus(9) → I", "mineira", "mineira", "IVsus(9) – I", "5 M sus4 9 | 0 M", "5 – 5", "4 – 1",
    "o plagal suspenso", "ver MAQUINA_DE_HARMONIA §6")
add("♭II7M(♯11) → I", "mineira", "mineira", "♭II7M(♯11) – I7M(9)", "1 M 7M #11 | 0 M 7M 9", "5 – 5", "♭2 – 1",
    "a chegada meio tom acima, sem dominante", "ver MAQUINA_DE_HARMONIA §6")
add("♭VI7M(♯11) → I", "Hermeto", "hermeto", "♭VI7M(♯11) – I7M(9)", "8 M 7M #11 | 0 M 7M 9", "2 – 2", "♭6 – 1",
    "a mediante com a cor lídia", "A. G. Arrais, A música de Hermeto Pascoal, USP; ver MAQUINA_DE_HARMONIA §6")

# ---- modal, andaluza, cromática -------------------------------------------------------------------
add("vamp dórico", "modal", "modal", "im7 – IV7", "0 m 7 | 5 M 7", "♭3 – 3", "1 – 4",
    "o IV maior do dórico: a 6ª maior no menor", LEVINE)
add("vamp mixolídio", "modal", "modal", "I – ♭VII", "0 M | 10 M", "3 – 2", "1 – ♭7", "a tônica sem sensível", LEVINE)
add("vamp frígio", "modal", "modal", "i – ♭II7M", "0 m | 1 M 7M", "1 – 1", "1 – ♭2", "o ♭2 do frígio", LEVINE)
add("andaluza", "andaluza", "andaluza", "i – ♭VII – ♭VI – V", "0 m | 10 M | 8 M | 7 M b9", "1 – 1 – 1 – 7",
    "1 – ♭7 – ♭6 – 5", "o baixo que desce ao V: a dupla tonicidade do flamenco", MANUEL)
add("mediante cromática", "cromática romântica", "cromatica", "♭VI – I", "8 M | 0 M", "1 – 1", "♭6 – 1",
    "uma nota comum, as outras por semitom", COHN)
add("ciclo hexatônico", "cromática romântica", "cromatica", "I – i – ♭VI – ♭vi", "0 M | 0 m | 8 M | 8 m",
    "3 – ♭3 – ♭3 – ♭3", "1 – 1 – ♭6 – ♭6", "P e L alternados: a máxima parcimônia", COHN)
add("ciclo octatônico", "cromática romântica", "cromatica", "I – i – ♭III – ♭iii", "0 M | 0 m | 3 M | 3 m",
    "5 – 5 – 5 – 5", "1 – 1 – ♭3 – ♭3", "P e R alternados", COHN)

# ---- o século XX ----------------------------------------------------------------------------------
add("planing de nonas", "impressionista", "impressionista", "I(9) – II(9) – I(9)", "0 M 7 9 | 2 M 7 9 | 0 M 7 9",
    "2 – 3 – 2", "1 – 2 – 1", "o acorde movido em bloco (\"Nuages\", \"La cathédrale engloutie\")", DEVOTO)
add("tons inteiros", "impressionista", "impressionista", "I+ – II+ – I", "0 aug | 2 aug | 0 M", "3 – ♯4 – 3", "1 – 2 – 1",
    "a escala de tons inteiros: nenhuma função", DEVOTO)
add("quartal ♭VII → I", "quartal", "quartal", "♭VII7sus4 – I7sus4(9)", "10 M sus4 7 | 0 M sus4 7 9", "4 – 4", "♭7 – 1",
    "as quartas empilhadas (So What)", "P. Hindemith, Unterweisung im Tonsatz, 1937; " + LEVINE)
add("Petrushka", "politonal", "politonal", "I7(♭9)(♯11)", "0 M 7 b9 #11", "♯4", "1",
    "duas tríades a um trítono ao mesmo tempo", "I. Stravinsky, Petrushka, 1911; " + TYMOCZKO)
add("dominantes octatônicas", "octatônica", "octatonica", "I7(♭9) – ♭III7(♭9) – ♯IV7(♭9) – VI7(♭9)",
    "0 M 7 b9 | 3 M 7 b9 | 6 M 7 b9 | 9 M 7 b9", "—", "1 – ♭3 – ♯4 – 6",
    "a coleção octatônica (modo 2): as raízes por 3ªs menores", MESSIAEN)
add("eixo de Bartók", "Bartók", "bartok", "♭II7(♯11) – I7(♯11)", "1 M 7 #11 | 0 M 7 #11", "5 – 5", "♭2 – 1",
    "a dominante do contrapolo; a escala acústica", LENDVAI)
add("acorde místico", "Scriabin", "scriabin", "I7(9)(♯11)(13) – ♯IV7(9)(♯11)(13)", "0 M 7 9 #11 13 | 6 M 7 9 #11 13",
    "—", "1 – ♯4", "o acorde de Prometeu, andando ao trítono, sem resolver", BAKER)
add("Bachianas: quintas descendentes", "Villa-Lobos", "villa-lobos", "VI7 – II7 – V7 – I",
    "9 M 7 | 2 M 7 | 7 M 7 | 0 M", "—", "6 – 2 – 5 – 1", "a sequência barroca à brasileira",
    "H. Villa-Lobos, Bachianas Brasileiras, 1930–45; ver MAQUINA_DE_HARMONIA §6")
add("Moacir Santos: dórico", "Moacir Santos", "moacir", "im7(9) – IV7(9)", "0 m 7 9 | 5 M 7 9", "2 – 2", "1 – 4",
    "o modal afro-brasileiro sobre o ostinato", "M. Santos, Coisas, Forma, 1965; ver MAQUINA_DE_HARMONIA §6")
add("Guinga: meio tom acima", "Guinga", "guinga", "I7M(♯5) – ♭II7(♯11) – I7M(9)", "0 aug 7M | 1 M 7 #11 | 0 M 7M 9",
    "♯5 – 5 – 5", "1 – ♭2 – 1", "as vozes internas cromáticas", "ver MAQUINA_DE_HARMONIA §6")

# ---- Pernambuco e Gismonti -------------------------------------------------------------------------
GPEIXE = "C. Guerra-Peixe, Os Maracatus do Recife, 1955 (2ª ed., Irmãos Vitale, 1980)"
add("frevo: ii–V7(♭9)–I6", "frevo", "frevo", "IIm7 – V7(♭9) – I6", "2 m 7 | 7 M 7 b9 | 0 M 6", "4 – ♭6 – 5", "2 – 5 – 1",
    "a chegada das orquestras de frevo, depois das dominantes encadeadas",
    "V. de Oliveira, Frevo, capoeira e passo, Recife, 1971; ver MAQUINA_DE_HARMONIA §9")
add("frevo: diminuto de passagem", "frevo", "frevo", "I – ♯I° – IIm7 – V7", "0 M | 1 dim 6 | 2 m 7 | 7 M 7", "3 – 3 – 4 – 4",
    "1 – ♯1 – 2 – 5", "o baixo cromático das fanfarras", "V. de Oliveira, Frevo, capoeira e passo, Recife, 1971")
add("maracatu: ♭VII → i", "maracatu", "maracatu", "♭VII – i", "10 M | 0 m", "2 – 1", "♭7 – 1",
    "a toada eólia sobre a alfaia: a tônica sem sensível", GPEIXE)
add("maracatu: ♭VI–♭VII–i", "maracatu", "maracatu", "♭VI – ♭VII – i", "8 M | 10 M | 0 m", "1 – 2 – 1", "♭6 – ♭7 – 1",
    "a subida eólia", GPEIXE)
add("Gismonti: ♭VI7M(♯11) → I(9)", "Gismonti", "gismonti", "♭VI7M(♯11) – I(9)", "8 M 7M #11 /0 | 0 M 9", "2 – 2", "1 – 1",
    "a mistura de modos sobre o pedal da corda solta",
    "E. Gismonti, Água e vinho, Odeon, 1972; Dança das cabeças, ECM, 1977 (as gravações)")


def main():
    out = sys.argv[1]
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Arquivo de cadências do Trioio\n"
                "# Escrito pelo Trioio a partir da teoria publicada, com a fonte em cada cadência.\n"
                "# Gerado por tools/gerar_arquivo_de_cadencias.py; formato descrito ali.\n"
                f"# {len(blocks)} cadências.\n\n" + "\n\n".join(blocks) + "\n")
    print(f"{len(blocks)} cadências em {out}")


if __name__ == "__main__":
    main()
