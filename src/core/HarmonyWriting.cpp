#include "HarmonyWriting.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>

namespace trioio::composition
{
namespace
{
int mod12(int x) { return ((x % 12) + 12) % 12; }

struct Rng
{
    std::uint64_t s;
    double next()
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return static_cast<double>(s >> 11) / 9007199254740992.0;
    }
};

double correlate(const std::array<double, 12>& a, const std::array<double, 12>& b)
{
    double ma = 0, mb = 0;
    for (int i = 0; i < 12; ++i)
    {
        ma += a[static_cast<std::size_t>(i)];
        mb += b[static_cast<std::size_t>(i)];
    }
    ma /= 12;
    mb /= 12;
    double num = 0, da = 0, db = 0;
    for (int i = 0; i < 12; ++i)
    {
        const double x = a[static_cast<std::size_t>(i)] - ma, y = b[static_cast<std::size_t>(i)] - mb;
        num += x * y;
        da += x * x;
        db += y * y;
    }
    return da > 0 && db > 0 ? num / std::sqrt(da * db) : 0.0;
}

// os acordes de grau na tonalidade
Chord degree(const KeyEstimate& k, int which)
{
    // which: 1 I, 2 ii, 4 IV, 5 V (sempre maior: a sensível), 6 vi
    const int t = k.tonic;
    switch (which)
    {
        case 1: return { t, k.major ? ChordQuality::major : ChordQuality::minor };
        case 2: return { mod12(t + 2), k.major ? ChordQuality::minor : ChordQuality::diminished };
        case 4: return { mod12(t + 5), k.major ? ChordQuality::major : ChordQuality::minor };
        case 5: return { mod12(t + 7), ChordQuality::major };
        case 6: return k.major ? Chord { mod12(t + 9), ChordQuality::minor } : Chord { mod12(t + 8), ChordQuality::major };
        default: return { t, ChordQuality::major };
    }
}

// divide o acorde i em partes iguais (presas ao tempo) com os acordes dados
void splitInto(std::vector<ChordSpan>& spans, std::size_t i, const std::vector<Chord>& chords, double beat)
{
    const ChordSpan base = spans[i];
    const double len = base.end - base.start;
    const auto n = chords.size();
    std::vector<ChordSpan> parts;
    double t = base.start;
    for (std::size_t k = 0; k < n; ++k)
    {
        double e = k + 1 == n ? base.end : base.start + len * static_cast<double>(k + 1) / static_cast<double>(n);
        if (k + 1 < n && beat > 0.0)
            e = base.start + std::max(beat, std::round((e - base.start) / beat) * beat);
        e = std::min(e, base.end);
        if (e <= t + 1e-6)
            continue;
        parts.push_back({ t, e, chords[k], base.confidence });
        t = e;
    }
    if (parts.empty())
        return;
    parts.back().end = base.end;
    spans.erase(spans.begin() + static_cast<std::ptrdiff_t>(i));
    spans.insert(spans.begin() + static_cast<std::ptrdiff_t>(i), parts.begin(), parts.end());
}
} // namespace

static const std::array<double, 12> kMajorP { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
static const std::array<double, 12> kMinorP { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };

std::array<double, 12> histogram(const std::vector<ChordSpan>& spans, double t0, double t1)
{
    std::array<double, 12> h {};
    for (const auto& s : spans)
        if (s.start >= t0 - 1e-6 && s.start < t1)
        {
            const double w = s.end - s.start;
            const auto pcs = s.chord.pitchClasses();
            for (std::size_t k = 0; k < pcs.size(); ++k)
                h[static_cast<std::size_t>(mod12(pcs[k]))] += w * (k == 0 ? 1.0 : 0.8);
        }
    return h;
}

double keyScore(const std::array<double, 12>& h, int tonic, bool major)
{
    std::array<double, 12> rot {};
    for (int i = 0; i < 12; ++i)
        rot[static_cast<std::size_t>(mod12(i + tonic))] = (major ? kMajorP : kMinorP)[static_cast<std::size_t>(i)];
    return correlate(h, rot);
}

KeyEstimate findKey(const std::vector<ChordSpan>& spans, double t0, double t1)
{
    const auto& majorP = kMajorP;
    const auto& minorP = kMinorP;
    std::array<double, 12> h {};
    for (const auto& s : spans)
        if (s.start >= t0 - 1e-6 && s.start < t1)
        {
            const double w = s.end - s.start;
            const auto pcs = s.chord.pitchClasses();
            for (std::size_t k = 0; k < pcs.size(); ++k)
                h[static_cast<std::size_t>(mod12(pcs[k]))] += w * (k == 0 ? 1.0 : 0.8);
        }
    KeyEstimate best;
    best.score = -2.0;
    for (int t = 0; t < 12; ++t)
        for (bool maj : { true, false })
        {
            std::array<double, 12> rot {};
            for (int i = 0; i < 12; ++i)
                rot[static_cast<std::size_t>(mod12(i + t))] = (maj ? majorP : minorP)[static_cast<std::size_t>(i)];
            const double c = correlate(h, rot);
            if (c > best.score)
                best = { t, maj, c };
        }
    return best;
}

std::string keyName(const KeyEstimate& k)
{
    // o nome da tônica (sem o "m" do acorde menor) e o modo: "Lá menor"
    return Chord { k.tonic, ChordQuality::major }.name() + (k.major ? " maior" : " menor");
}

const char* harmonyLanguageName(HarmonyLanguage l)
{
    switch (l)
    {
        case HarmonyLanguage::automatic: return "automática";
        case HarmonyLanguage::functional: return "funcional";
        case HarmonyLanguage::jazz: return "jazz";
        case HarmonyLanguage::brazilian: return "brasileira";
        case HarmonyLanguage::impressionist: return "impressionista";
        case HarmonyLanguage::northeastern: return "nordestina";
        case HarmonyLanguage::mineira: return "mineira (Clube da Esquina)";
        case HarmonyLanguage::hermeto: return "Hermeto";
        case HarmonyLanguage::modal: return "modal";
        case HarmonyLanguage::andalusian: return "andaluza";
        case HarmonyLanguage::chromatic: return "cromática romântica";
        case HarmonyLanguage::quartal: return "quartal";
        case HarmonyLanguage::polytonal: return "politonal";
        case HarmonyLanguage::octatonic: return "octatônica";
        case HarmonyLanguage::clusters: return "clusters";
        case HarmonyLanguage::stasis: return "estática";
        case HarmonyLanguage::spectral: return "espectral";
        case HarmonyLanguage::bartok: return "Bartók";
        case HarmonyLanguage::scriabin: return "Scriabin";
        case HarmonyLanguage::villaLobos: return "Villa-Lobos";
        case HarmonyLanguage::moacir: return "Moacir Santos";
        case HarmonyLanguage::guinga: return "Guinga";
        case HarmonyLanguage::frevo: return "frevo";
        case HarmonyLanguage::maracatu: return "maracatu";
        case HarmonyLanguage::gismonti: return "Gismonti";
        case HarmonyLanguage::partch: return "Partch";
    }
    return "?";
}

bool parseHarmonyLanguage(const std::string& raw, HarmonyLanguage& out)
{
    // aceita também o nome de exibição: minúsculas (ASCII) e só a primeira palavra
    std::string n = raw.substr(0, raw.find(' '));
    for (auto& ch : n)
        if (ch >= 'A' && ch <= 'Z')
            ch = static_cast<char>(ch - 'A' + 'a');
    if (n == "auto" || n == "automatica" || n == "automática") out = HarmonyLanguage::automatic;
    else if (n == "funcional") out = HarmonyLanguage::functional;
    else if (n == "jazz") out = HarmonyLanguage::jazz;
    else if (n == "brasileira") out = HarmonyLanguage::brazilian;
    else if (n == "impressionista" || n == "debussy") out = HarmonyLanguage::impressionist;
    else if (n == "nordestina" || n == "baiao" || n == "baião" || n == "armorial") out = HarmonyLanguage::northeastern;
    else if (n == "mineira" || n == "clube") out = HarmonyLanguage::mineira;
    else if (n == "hermeto") out = HarmonyLanguage::hermeto;
    else if (n == "modal") out = HarmonyLanguage::modal;
    else if (n == "andaluza" || n == "flamenco") out = HarmonyLanguage::andalusian;
    else if (n == "cromatica" || n == "cromática" || n == "romantica" || n == "romântica") out = HarmonyLanguage::chromatic;
    else if (n == "quartal" || n == "hindemith") out = HarmonyLanguage::quartal;
    else if (n == "politonal" || n == "milhaud" || n == "petrushka") out = HarmonyLanguage::polytonal;
    else if (n == "octatonica" || n == "octatônica" || n == "stravinsky") out = HarmonyLanguage::octatonic;
    else if (n == "clusters" || n == "cluster" || n == "cowell") out = HarmonyLanguage::clusters;
    else if (n == "estatica" || n == "estática" || n == "feldman") out = HarmonyLanguage::stasis;
    else if (n == "espectral") out = HarmonyLanguage::spectral;
    else if (n == "bartok" || n == "bartók") out = HarmonyLanguage::bartok;
    else if (n == "scriabin") out = HarmonyLanguage::scriabin;
    else if (n == "villa-lobos" || n == "villalobos") out = HarmonyLanguage::villaLobos;
    else if (n == "moacir" || n == "moacir-santos") out = HarmonyLanguage::moacir;
    else if (n == "guinga") out = HarmonyLanguage::guinga;
    else if (n == "frevo") out = HarmonyLanguage::frevo;
    else if (n == "maracatu") out = HarmonyLanguage::maracatu;
    else if (n == "gismonti" || n == "egberto") out = HarmonyLanguage::gismonti;
    else if (n == "partch" || n == "microtonal" || n == "otonal") out = HarmonyLanguage::partch;
    else return false;
    return true;
}

HarmonyLanguage resolveLanguage(int requested, int ensemble, strategy::Mode mode)
{
    if (requested > 0 && requested < kHarmonyLanguages)
        return static_cast<HarmonyLanguage>(requested);
    // pelo conjunto (a ordem de Ensemble.h)
    switch (ensemble)
    {
        case 3: case 4: return HarmonyLanguage::jazz;                                  // jazz, manouche
        case 1: case 2: case 12: return HarmonyLanguage::brazilian;                     // choro, samba, escola
        case 13: return HarmonyLanguage::northeastern;                                  // forró
        case 5: case 6: case 9: case 14: case 11: return HarmonyLanguage::functional;  // bolero, charanga, salsa, tango, rock
        case 7: case 8: case 10:                                                        // câmara, orquestra, a cappella
            return mode == strategy::Mode::transfigure ? HarmonyLanguage::impressionist : HarmonyLanguage::functional;
        default: break;
    }
    // sem conjunto: pelo modo
    switch (mode)
    {
        case strategy::Mode::transfigure: return HarmonyLanguage::impressionist;
        case strategy::Mode::counter: return HarmonyLanguage::jazz;
        default: return HarmonyLanguage::functional;
    }
}

namespace
{
Chord colored(Chord c, std::uint8_t color)
{
    c.color = color;
    return c;
}

// a cor de cada acorde pela função, na linguagem (o jazz e a brasileira)
void colorize(std::vector<ChordSpan>& spans, const std::vector<std::pair<double, double>>& sections, const std::vector<KeyEstimate>& keys,
              HarmonyLanguage lang, Rng& rng)
{
    for (std::size_t i = 0; i < spans.size(); ++i)
    {
        auto& c = spans[i].chord;
        if (c.color != 0)
            continue;  // já escrito (as cadências da linguagem)
        std::size_t sec = 0;
        while (sec + 1 < sections.size() && spans[i].start >= sections[sec + 1].first - 1e-6)
            ++sec;
        const auto& k = keys[sec];
        const bool next4th = i + 1 < spans.size() && mod12(spans[i + 1].chord.root - c.root) == 5;
        const bool dominant = c.quality == ChordQuality::major && (next4th || c.root == mod12(k.tonic + 7));
        const bool nextMinor = i + 1 < spans.size() && spans[i + 1].chord.quality == ChordQuality::minor;
        if (lang == HarmonyLanguage::jazz)
        {
            if (c.quality == ChordQuality::diminished)
                c.color = kSeventh;  // m7(♭5)
            else if (c.quality == ChordQuality::minor)
                c.color = kSeventh | kNinth;
            else if (dominant)
                c.color = kSeventh | (nextMinor ? kFlatNinth : (rng.next() < 0.5 ? kNinth : kThirteenth));
            else if (c.root == mod12(k.tonic + 5) && rng.next() < 0.4)
                c.color = kMajorSeventh | kSharpEleventh;  // o IV lídio
            else
                c.color = kMajorSeventh | kNinth;
        }
        else if (lang == HarmonyLanguage::brazilian)
        {
            if (c.quality == ChordQuality::diminished)
                c.color = kSixth;  // o diminuto com a 7ª diminuta
            else if (c.quality == ChordQuality::minor)
                c.color = c.root == k.tonic ? kSixth : kSeventh;  // a tônica menor m6; os outros m7
            else if (dominant)
                c.color = kSeventh | (nextMinor ? kFlatNinth : kNinth);
            else if (c.root == k.tonic)
                c.color = kSixth | kNinth;  // a tônica 6(9)
            else
                c.color = kMajorSeventh;
        }
    }
}
} // namespace

namespace
{
// a cor a partir das razões medidas do corpo espectral (a harmonia
// espectral): cada parcial vira a classe de altura mais perto, relativa à
// fundamental, e a classe vira um bit de cor
std::uint8_t colorFromSpectrum(const std::vector<SpectralProfile>* spectra, double t)
{
    std::uint8_t c = 0;
    if (!spectra)
        return kSeventh | kNinth | kSharpEleventh | kThirteenth;  // a série harmônica: o acorde acústico
    for (const auto& sp : *spectra)
    {
        if (t < sp.start - 1e-6 || t >= sp.end)
            continue;
        for (std::size_t k = 0; k < sp.ratios.size(); ++k)
        {
            if (k < sp.weights.size() && sp.weights[k] < 0.03f)
                continue;
            const int pc = mod12(static_cast<int>(std::lround(12.0 * std::log2(std::max(0.01f, sp.ratios[k])))));
            switch (pc)
            {
                case 1: c |= kFlatNinth; break;
                case 2: c |= kNinth; break;
                case 6: c |= kSharpEleventh; break;
                case 9: c |= kThirteenth; break;
                case 10: c |= kSeventh; break;
                case 11: c |= kMajorSeventh; break;
                case 5: c |= kSus4; break;
                default: break;
            }
        }
        break;
    }
    return c ? c : static_cast<std::uint8_t>(kSeventh | kNinth);
}

// as linguagens modernas e as dos compositores (2026-10-17): por seção,
// cada uma transforma os acordes e escreve a sua chegada
void writeModern(std::vector<ChordSpan>& spans, const std::vector<std::pair<double, double>>& sections, const std::vector<KeyEstimate>& keys,
                 double beat, HarmonyLanguage lang, Rng& rng, std::map<std::string, int>& count, std::vector<CadenceMark>* cadences,
                 const std::vector<SpectralProfile>* spectra)
{
    using Q = ChordQuality;
    const auto sectionOf = [&](double t) {
        std::size_t sec = 0;
        while (sec + 1 < sections.size() && t >= sections[sec + 1].first - 1e-6)
            ++sec;
        return sec;
    };
    struct Slot { int iv; Q q; std::uint8_t color; };
    const auto nearestSlot = [&](int root, int tonic, const std::vector<Slot>& slots) {
        int best = 0, bestD = 99;
        for (std::size_t k = 0; k < slots.size(); ++k)
        {
            const int pc = mod12(tonic + slots[k].iv);
            const int d = std::min(mod12(pc - root), mod12(root - pc));
            if (d < bestD || (d == bestD && rng.next() < 0.5))
            {
                bestD = d;
                best = static_cast<int>(k);
            }
        }
        return slots[static_cast<std::size_t>(best)];
    };
    for (std::size_t sIdx = 0; sIdx < sections.size(); ++sIdx)
    {
        const auto& key = keys[sIdx];
        const int T = key.tonic;
        const double u = rng.next();
        std::vector<std::size_t> idx;
        for (std::size_t i = 0; i < spans.size(); ++i)
            if (sectionOf(spans[i].start) == sIdx)
                idx.push_back(i);
        if (idx.empty())
            continue;
        std::vector<Chord> cad;  // a chegada da seção (vazia: sem cadência escrita)
        auto kind = CadenceMark::Kind::plagal;
        bool pedal = false;
        std::string tech;
        switch (lang)
        {
            case HarmonyLanguage::modal:
            {
                // os vamps: dórico i–IV, mixolídio I–♭VII, frígio i–♭II, eólio i–♭VI–♭VII
                std::vector<Slot> slots;
                if (u < 0.3) { tech = "vamp dórico (im7–IV7)"; slots = { { 0, Q::minor, kSeventh }, { 5, Q::major, kSeventh } }; }
                else if (u < 0.55) { tech = "vamp mixolídio (I–♭VII)"; slots = { { 0, Q::major, 0 }, { 10, Q::major, 0 } }; }
                else if (u < 0.75) { tech = "vamp frígio (i–♭II)"; slots = { { 0, Q::minor, 0 }, { 1, Q::major, kMajorSeventh } }; }
                else { tech = "eólio (i–♭VI–♭VII)"; slots = { { 0, Q::minor, 0 }, { 8, Q::major, 0 }, { 10, Q::major, 0 } }; }
                // o vamp alterna os seus acordes, um por acorde lido
                for (std::size_t k = 0; k < idx.size(); ++k)
                {
                    const auto& sl = slots[k % slots.size()];
                    spans[idx[k]].chord = { mod12(T + sl.iv), sl.q, sl.color };
                }
                pedal = rng.next() < 0.5;  // o bordão
                break;
            }
            case HarmonyLanguage::andalusian:
            {
                // a cadência andaluza descendo sem parar: i–♭VII–♭VI–V (Manuel 2002)
                tech = "a andaluza (i–♭VII–♭VI–V)";
                const Chord seq[4] = { { T, Q::minor, 0 }, { mod12(T + 10), Q::major, 0 }, { mod12(T + 8), Q::major, 0 },
                                       { mod12(T + 7), Q::major, kFlatNinth } };
                for (std::size_t k = 0; k < idx.size(); ++k)
                    spans[idx[k]].chord = seq[k % 4];
                cad = { { mod12(T + 8), Q::major, 0 }, { mod12(T + 7), Q::major, kFlatNinth } };
                kind = CadenceMark::Kind::half;
                break;
            }
            case HarmonyLanguage::chromatic:
            {
                // o cromatismo romântico: os ciclos de Cohn (hexatônico: P e L
                // alternados; octatônico: P e R) e as mediantes
                const bool hexa = u < 0.5;
                tech = hexa ? "ciclo hexatônico (P–L, Cohn)" : "ciclo octatônico (P–R, Cohn)";
                Chord c { T, key.major ? Q::major : Q::minor, 0 };
                for (std::size_t k = 0; k < idx.size(); ++k)
                {
                    spans[idx[k]].chord = c;
                    // P: troca o modo; L (no maior: a fundamental sobe uma 3ª maior e vira
                    // menor) e R (no maior: a relativa) pela teoria neo-riemanniana
                    const bool stepP = k % 2 == 0;
                    if (stepP)
                        c.quality = c.quality == Q::major ? Q::minor : Q::major;
                    else if (hexa)
                        c = c.quality == Q::major ? Chord { mod12(c.root + 4), Q::minor, 0 } : Chord { mod12(c.root + 8), Q::major, 0 };
                    else
                        c = c.quality == Q::major ? Chord { mod12(c.root + 9), Q::minor, 0 } : Chord { mod12(c.root + 3), Q::major, 0 };
                }
                cad = { { mod12(T + 8), Q::major, 0 }, { T, key.major ? Q::major : Q::minor, 0 } };  // ♭VI → I, a mediante
                break;
            }
            case HarmonyLanguage::quartal:
            {
                tech = "quartal (Hindemith; o So What)";
                for (auto i : idx)
                    spans[i].chord = { spans[i].chord.root, Q::major, static_cast<std::uint8_t>(kSus4 | kSeventh | (rng.next() < 0.5 ? kNinth : 0)) };
                cad = { { mod12(T + 10), Q::major, kSus4 | kSeventh }, { T, Q::major, kSus4 | kSeventh | kNinth } };
                break;
            }
            case HarmonyLanguage::polytonal:
            {
                // dois tons ao mesmo tempo: o acorde de Petrushka (Dó e Fá♯:
                // 7(♭9)(♯11)) ou as tríades a um tom (Ré sobre Dó: 9, ♯11, 13; Milhaud)
                const bool petrushka = u < 0.5;
                tech = petrushka ? "politonal ao trítono (o acorde de Petrushka)" : "politonal a um tom (Milhaud)";
                for (auto i : idx)
                    spans[i].chord = { spans[i].chord.root, Q::major,
                                       static_cast<std::uint8_t>(petrushka ? (kSeventh | kFlatNinth | kSharpEleventh) : (kNinth | kSharpEleventh | kThirteenth)) };
                break;
            }
            case HarmonyLanguage::octatonic:
            {
                // a coleção octatônica: as dominantes com ♭9 a uma 3ª menor
                // (Stravinsky; o modo 2 de Messiaen)
                tech = "octatônica (as dominantes 7(♭9) por 3ªs menores)";
                for (std::size_t k = 0; k < idx.size(); ++k)
                    spans[idx[k]].chord = { mod12(T + 3 * static_cast<int>(k % 4)), Q::major,
                                            static_cast<std::uint8_t>(kSeventh | kFlatNinth | (k % 2 ? kThirteenth : 0)) };
                break;
            }
            case HarmonyLanguage::clusters:
                tech = "clusters (Cowell)";  // a disposição em segundas fica com a voz de harmonia
                break;
            case HarmonyLanguage::stasis:
            {
                // a harmonia estática (Feldman): poucos acordes longos, que mudam
                // uma nota de cada vez
                tech = "estática (Feldman)";
                Chord c { T, key.major ? Q::major : Q::minor, kMajorSeventh };
                static const std::uint8_t toggles[] = { kNinth, kSixth, kSharpEleventh, kNinth, kMajorSeventh };
                for (std::size_t k = 0; k < idx.size(); ++k)
                {
                    spans[idx[k]].chord = c;
                    if (k % 2 == 1)
                        c.color ^= toggles[(k / 2) % 5];
                }
                break;
            }
            case HarmonyLanguage::spectral:
            {
                // o acorde do corpo da referência: as cores pelos parciais medidos
                tech = "espectral (o corpo da referência)";
                for (auto i : idx)
                    spans[i].chord = { spans[i].chord.root, Q::major, colorFromSpectrum(spectra, spans[i].start) };
                pedal = rng.next() < 0.5;
                break;
            }
            case HarmonyLanguage::bartok:
            {
                // o sistema de eixos (E. Lendvai): as funções T, S e D, cada uma
                // num eixo de 3ªs menores; o acorde vai a outro ponto do seu eixo;
                // a cor da escala acústica (7(♯11))
                tech = "Bartók (os eixos; a escala acústica)";
                for (auto i : idx)
                {
                    auto& c = spans[i].chord;
                    const int axisStep = 3 * static_cast<int>(rng.next() * 4.0);
                    c = { mod12(c.root + axisStep), c.quality == Q::diminished ? Q::minor : c.quality,
                          static_cast<std::uint8_t>(c.quality == Q::minor ? (kSeventh | kNinth) : (kSeventh | kSharpEleventh)) };
                }
                // a chegada pelo polo oposto: o D do contrapolo (o trítono) → T
                cad = { { mod12(T + 1), Q::major, kSeventh | kSharpEleventh }, { T, Q::major, kSeventh | kSharpEleventh } };
                break;
            }
            case HarmonyLanguage::scriabin:
            {
                // o acorde místico (Dó Fá♯ Si♭ Mi Lá Ré: 7(9)(♯11)(13)), andando
                // por trítonos e 3ªs menores, sem resolver (o Prometeu)
                tech = "Scriabin (o acorde místico)";
                for (std::size_t k = 0; k < idx.size(); ++k)
                    spans[idx[k]].chord = { mod12(T + (k % 2 ? 6 : 0) + 3 * static_cast<int>((k / 2) % 2)), Q::major,
                                            kSeventh | kNinth | kSharpEleventh | kThirteenth };
                break;
            }
            case HarmonyLanguage::villaLobos:
            {
                if (u < 0.4)
                {
                    // as cordas soltas do violão (Mi Lá Ré Sol Si Mi) sobre um pedal,
                    // deslizando em bloco
                    tech = "Villa-Lobos: as cordas soltas sobre o pedal";
                    for (auto i : idx)
                    {
                        spans[i].chord = { spans[i].chord.root, Q::major, kSus4 | kSeventh | kNinth };
                        spans[i].bassPc = T;
                    }
                }
                else if (u < 0.7)
                {
                    // as Bachianas: a sequência de quintas descendentes com 7ªs
                    tech = "Villa-Lobos: as Bachianas (a sequência de quintas)";
                    int r = mod12(T + 9);
                    for (auto i : idx)
                    {
                        spans[i].chord = { r, Q::major, kSeventh };
                        r = mod12(r + 5);
                    }
                    cad = { { mod12(T + 7), Q::major, kSeventh }, { T, key.major ? Q::major : Q::minor, 0 } };
                    kind = CadenceMark::Kind::authentic;
                }
                else
                {
                    // os Choros: o paralelismo de acordes com 7ª e o pedal
                    tech = "Villa-Lobos: os Choros (o paralelismo)";
                    for (auto i : idx)
                        spans[i].chord = { spans[i].chord.root, Q::major, kSeventh | kNinth };
                    pedal = true;
                }
                break;
            }
            case HarmonyLanguage::moacir:
            {
                // Moacir Santos (Coisas): o modal afro-brasileiro, dórico ou
                // mixolídio, os acordes sus e 7(9) dos metais, o baixo em ostinato
                const bool dorian = u < 0.5;
                tech = dorian ? "Moacir Santos: dórico (im7(9), IV7(9))" : "Moacir Santos: mixolídio (I7sus(9), ♭VII7M)";
                const std::vector<Slot> slots = dorian ? std::vector<Slot> { { 0, Q::minor, kSeventh | kNinth }, { 5, Q::major, kSeventh | kNinth },
                                                                             { 10, Q::major, kMajorSeventh } }
                                                       : std::vector<Slot> { { 0, Q::major, kSus4 | kSeventh | kNinth }, { 10, Q::major, kMajorSeventh },
                                                                             { 5, Q::major, kSeventh } };
                for (auto i : idx)
                {
                    const auto sl = nearestSlot(spans[i].chord.root, T, slots);
                    spans[i].chord = { mod12(T + sl.iv), sl.q, sl.color };
                }
                pedal = rng.next() < 0.5;
                break;
            }
            case HarmonyLanguage::guinga:
            {
                // Guinga: as vozes internas cromáticas — o 7M(♯5), o m(7M), o
                // diminuto — e a chegada meio tom acima
                tech = "Guinga (as vozes internas cromáticas)";
                for (std::size_t k = 0; k < idx.size(); ++k)
                {
                    auto& c = spans[idx[k]].chord;
                    const double v = rng.next();
                    if (c.quality == Q::minor)
                        c.color = v < 0.5 ? (kMajorSeventh | kNinth) : (kSeventh | kNinth);
                    else if (v < 0.35)
                        c = { c.root, Q::augmented, kMajorSeventh };
                    else if (v < 0.55)
                        c = { c.root, Q::diminished, kSixth };
                    else
                        c.color = kMajorSeventh | kNinth;
                }
                cad = { { mod12(T + 1), Q::major, kSeventh | kSharpEleventh }, { T, key.major ? Q::major : Q::minor, kMajorSeventh | kNinth } };
                break;
            }
            case HarmonyLanguage::frevo:
            {
                // o frevo (Recife): tonal, maior, rápido; cada acorde lido é
                // preparado pela sua dominante (o V7 de x, na metade de antes) e
                // o diminuto de passagem liga as chegadas; a cadência ii–V7(♭9)–I
                tech = "frevo (as dominantes encadeadas, o diminuto de passagem)";
                for (std::size_t k = idx.size(); k-- > 0;)
                {
                    const std::size_t i = idx[k];
                    auto c = spans[i].chord;
                    if (c.root == T && k + 1 == idx.size())
                        continue;  // a tônica final fica para a cadência
                    if (c.quality == Q::diminished)
                        c.quality = Q::minor;
                    const double len = spans[i].end - spans[i].start;
                    if (len >= 2.0 * beat - 1e-6)
                    {
                        const Chord prep = rng.next() < 0.3 ? Chord { mod12(c.root - 1), Q::diminished, kSixth }  // o dim de passagem
                                                            : Chord { mod12(c.root + 7), Q::major, kSeventh };     // o V7 de x
                        splitInto(spans, i, { prep, c }, beat);
                    }
                }
                std::vector<std::size_t> again;  // os índices mudaram: refaz a seção
                for (std::size_t i = 0; i < spans.size(); ++i)
                    if (sectionOf(spans[i].start) == sIdx)
                        again.push_back(i);
                idx = again;
                cad = { { mod12(T + 2), Q::minor, kSeventh }, { mod12(T + 7), Q::major, kSeventh | kFlatNinth }, { T, Q::major, kSixth } };
                kind = CadenceMark::Kind::authentic;
                break;
            }
            case HarmonyLanguage::maracatu:
            {
                // o maracatu de baque virado: as toadas modais (o eólio, o
                // mixolídio), um acorde para dois lidos, o bordão da alfaia na
                // tônica; a chegada ♭VII → i (Guerra-Peixe, Os Maracatus do Recife)
                const bool aeolian = u < 0.6;
                tech = aeolian ? "maracatu: a toada eólia (i–♭VII–♭VI)" : "maracatu: a toada mixolídia (I–♭VII)";
                const std::vector<Chord> loop = aeolian ? std::vector<Chord> { { T, Q::minor, 0 }, { mod12(T + 10), Q::major, 0 },
                                                                               { mod12(T + 8), Q::major, 0 }, { mod12(T + 10), Q::major, 0 } }
                                                        : std::vector<Chord> { { T, Q::major, 0 }, { mod12(T + 10), Q::major, 0 } };
                for (std::size_t k = 0; k < idx.size(); ++k)
                    spans[idx[k]].chord = loop[(k / 2) % loop.size()];
                pedal = true;
                cad = { { mod12(T + 10), Q::major, 0 }, { T, aeolian ? Q::minor : Q::major, 0 } };
                break;
            }
            case HarmonyLanguage::partch:
            {
                // H. Partch (Genesis of a Music, 1949): a entonação justa no
                // limite 11. A otonalidade é a série harmônica sobre a unidade
                // (1/1 5/4 3/2 7/4 9/8 11/8: o "maior" estendido); a utonalidade
                // é o seu espelho, sob a unidade (1/1 8/5 4/3 8/7 16/9 16/11).
                // As unidades vêm do diamante de tonalidades (1/1, 9/8, 8/7, 6/5,
                // 5/4, 4/3, 11/8, 3/2, 8/5, 5/3, 7/4, 16/9); o acorde lido escolhe
                // a unidade mais perto e o modo (maior → O, menor → U); de vez em
                // quando a mesma unidade troca de O para U (o pivô de Partch)
                tech = "Partch (otonalidades e utonalidades no limite 11)";
                static const double kUnity[] = { 1.0, 9.0 / 8, 8.0 / 7, 6.0 / 5, 5.0 / 4, 4.0 / 3, 11.0 / 8, 3.0 / 2, 8.0 / 5, 5.0 / 3, 7.0 / 4, 16.0 / 9 };
                static const char* const kUnityName[] = { "1/1", "9/8", "8/7", "6/5", "5/4", "4/3", "11/8", "3/2", "8/5", "5/3", "7/4", "16/9" };
                static const double kOtonal[] = { 1.0, 5.0 / 4, 3.0 / 2, 7.0 / 4, 9.0 / 8, 11.0 / 8 };
                const auto cents = [](double r) { return 1200.0 * std::log2(r); };
                bool prevOtonal = true;
                int prevUnity = -1;
                for (std::size_t k = 0; k < idx.size(); ++k)
                {
                    auto& sp = spans[idx[k]];
                    const int rel = mod12(sp.chord.root - T);
                    int best = 0;
                    double bd = 1e9;
                    for (int j = 0; j < 12; ++j)
                    {
                        double d = std::abs(cents(kUnity[j]) - 100.0 * rel);
                        d = std::min(d, 1200.0 - d);
                        if (d < bd) { bd = d; best = j; }
                    }
                    bool otonal = sp.chord.quality != Q::minor && sp.chord.quality != Q::diminished;
                    if (best == prevUnity && rng.next() < 0.6)
                        otonal = !prevOtonal;  // o pivô: a mesma unidade, o outro lado
                    const double unity = static_cast<double>(T) * 100.0 + cents(kUnity[best]);
                    sp.microPcs.clear();
                    for (double r : kOtonal)
                    {
                        const double c = unity + (otonal ? cents(r) : -cents(r));
                        sp.microPcs.push_back(static_cast<float>(std::fmod(std::fmod(c / 100.0, 12.0) + 12.0, 12.0)));
                    }
                    sp.microName = std::string(otonal ? "O " : "U ") + kUnityName[best];
                    // o acorde de 12 semitons mais perto, para o que lê classes inteiras
                    sp.chord = { mod12(static_cast<int>(std::lround(unity / 100.0))), otonal ? Q::major : Q::minor,
                                 static_cast<std::uint8_t>(kSeventh | kNinth) };
                    prevOtonal = otonal;
                    prevUnity = best;
                }
                break;
            }
            case HarmonyLanguage::gismonti:
            {
                // Egberto Gismonti ("Água e vinho", 1972; Dança das cabeças, 1977):
                // o pedal da tônica (a corda solta do violão de dez cordas) sob
                // acordes que mudam de modo — I(9), ♭VII, IV(9), ♭VI7M — com a 9ª
                // e a 4ª abertas; a chegada ♭VI7M → I(9)
                tech = "Gismonti (os acordes de 9ª sobre o pedal, a mistura de modos)";
                const std::vector<Slot> slots { { 0, Q::major, kNinth }, { 10, Q::major, kNinth }, { 5, Q::major, kNinth | kMajorSeventh },
                                                { 8, Q::major, kMajorSeventh | kSharpEleventh }, { 2, Q::minor, kSeventh | kNinth } };
                for (auto i : idx)
                {
                    const auto sl = nearestSlot(spans[i].chord.root, T, slots);
                    spans[i].chord = { mod12(T + sl.iv), sl.q, sl.color };
                }
                pedal = true;
                cad = { { mod12(T + 8), Q::major, kMajorSeventh | kSharpEleventh }, { T, Q::major, kNinth } };
                break;
            }
            default: break;
        }
        ++count[tech];
        const std::size_t last = idx.back();
        const double sectionEnd = spans[last].end;
        if (!cad.empty())
        {
            if (spans[last].end - spans[last].start >= static_cast<double>(cad.size()) * beat - 1e-6)
                splitInto(spans, last, cad, beat);
            else
                spans[last].chord = cad.back();
        }
        if (pedal)
        {
            // o bordão, também sob a chegada
            for (auto& x : spans)
                if (x.start >= spans[idx.front()].start - 1e-6 && x.end <= sectionEnd + 1e-6)
                    x.bassPc = T;
            ++count["pedais"];
        }
        if (cad.empty())
            continue;
        if (cadences)
            for (std::size_t i = spans.size(); i-- > 0;)
                if (spans[i].end <= sectionEnd + 1e-6 && spans[i].start < sectionEnd - 1e-6)
                {
                    cadences->push_back({ spans[i].start, spans[i].end, kind, key });
                    break;
                }
    }
}
} // namespace

std::string writeHarmony(std::vector<ChordSpan>& spans, const std::vector<std::pair<double, double>>& sectionsIn, double beat,
                         strategy::Mode mode, HarmonyLanguage lang, std::uint64_t seed, std::vector<CadenceMark>* cadences,
                         const std::vector<SpectralProfile>* spectra)
{
    if (spans.size() < 2)
        return {};
    if (lang == HarmonyLanguage::automatic)
        lang = HarmonyLanguage::functional;
    std::sort(spans.begin(), spans.end(), [](const ChordSpan& a, const ChordSpan& b) { return a.start < b.start; });
    auto sections = sectionsIn;
    if (sections.empty())
        sections = { { spans.front().start, spans.back().end } };
    Rng rng { seed * 0x9E3779B97F4A7C15ull + 0x4A2Dull };
    const bool jazz = lang == HarmonyLanguage::jazz, brazil = lang == HarmonyLanguage::brazilian;
    const bool impression = lang == HarmonyLanguage::impressionist;
    // a tonalidade da peça, e a de cada seção só quando ela muda de verdade:
    // seção longa (≥ 8 tempos) e o tom dela casando claramente melhor (+0,15)
    // que o da peça; senão, o tom da peça (as seções curtas oscilavam)
    const auto global = findKey(spans, spans.front().start, spans.back().end + 1.0);
    std::vector<KeyEstimate> keys;
    for (const auto& s : sections)
    {
        const auto local = findKey(spans, s.first, s.second);
        const double globalHere = keyScore(histogram(spans, s.first, s.second), global.tonic, global.major);
        const bool longEnough = s.second - s.first >= 8.0 * beat;
        keys.push_back(longEnough && local.score > globalHere + 0.15 ? local : global);
    }
    std::map<std::string, int> count;
    const auto sectionOf = [&](double t) {
        std::size_t sec = 0;
        while (sec + 1 < sections.size() && t >= sections[sec + 1].first - 1e-6)
            ++sec;
        return sec;
    };

    // ---- as linguagens modernas e as dos compositores (writeModern)
    if (static_cast<int>(lang) >= static_cast<int>(HarmonyLanguage::modal))
        writeModern(spans, sections, keys, beat, lang, rng, count, cadences, spectra);
    // ---- a impressionista (Debussy): a técnica de cada seção, o pedal, as
    //      cadências sem V → I
    else if (impression)
    {
        for (std::size_t sIdx = 0; sIdx < sections.size(); ++sIdx)
        {
            const auto& key = keys[sIdx];
            const double u = rng.next();
            // 0 planing real (o mesmo acorde de nona deslizando), 1 planing
            // diatônico (as qualidades da escala, com a 9ª), 2 tons inteiros
            // (acordes aumentados), 3 a sonoridade pentatônica (6/9)
            const int tech = u < 0.3 ? 0 : u < 0.6 ? 1 : u < 0.8 ? 2 : 3;
            const bool pedal = rng.next() < 0.35;
            static const char* const kTech[] = { "planing real (nonas)", "planing diatônico", "tons inteiros", "pentatônica (6/9)" };
            ++count[kTech[tech]];
            if (pedal)
                ++count["pedal"];
            static const int majorScale[7] = { 0, 2, 4, 5, 7, 9, 11 }, minorScale[7] = { 0, 2, 3, 5, 7, 8, 10 };
            const int* scale = key.major ? majorScale : minorScale;
            for (auto& s : spans)
            {
                if (sectionOf(s.start) != sIdx)
                    continue;
                auto& c = s.chord;
                switch (tech)
                {
                    case 0:
                        c.quality = ChordQuality::major;
                        c.color = kSeventh | kNinth;  // a nona de dominante, sem resolver
                        break;
                    case 1:
                    {
                        // a fundamental na escala, e a qualidade do grau
                        int best = 0, bestD = 99;
                        for (int d = 0; d < 7; ++d)
                        {
                            const int pc = mod12(key.tonic + scale[d]);
                            const int dist = std::min(mod12(pc - c.root), mod12(c.root - pc));
                            if (dist < bestD)
                            {
                                bestD = dist;
                                best = d;
                            }
                        }
                        c.root = mod12(key.tonic + scale[best]);
                        const int third = mod12(scale[(best + 2) % 7] - scale[best]), fifth = mod12(scale[(best + 4) % 7] - scale[best]);
                        c.quality = fifth == 6 ? ChordQuality::diminished : third == 4 ? ChordQuality::major : ChordQuality::minor;
                        c.color = kNinth;
                        break;
                    }
                    case 2:
                        // os tons inteiros: a fundamental na coleção da tônica
                        if (mod12(c.root - key.tonic) % 2 != 0)
                            c.root = mod12(c.root + 1);
                        c.quality = ChordQuality::augmented;
                        c.color = kSeventh;
                        break;
                    default:
                        c.quality = c.quality == ChordQuality::minor ? ChordQuality::minor : ChordQuality::major;
                        c.color = kSixth | kNinth;
                        break;
                }
                if (pedal)
                    s.bassPc = key.tonic;
            }
            // a cadência: nunca V → I — modal (♭VII → I), plagal (IV → I) ou
            // evitada (a frase para numa nona sem resolver)
            std::size_t last = spans.size();
            for (std::size_t i = 0; i < spans.size(); ++i)
                if (sectionOf(spans[i].start) == sIdx)
                    last = i;
            if (last == spans.size())
                continue;
            const bool final = sIdx + 1 == sections.size();
            const double v = rng.next();
            std::vector<Chord> cad;
            CadenceMark::Kind kind = CadenceMark::Kind::plagal;
            const Chord I = colored({ key.tonic, key.major ? ChordQuality::major : ChordQuality::minor }, kSixth | kNinth);
            if (final || v < 0.45)
            {
                cad = { colored({ mod12(key.tonic + 10), ChordQuality::major }, kNinth), I };  // ♭VII(add9) → I 6/9
                ++count["cadência modal ♭VII → I"];
            }
            else if (v < 0.75)
            {
                cad = { colored({ mod12(key.tonic + 5), key.major ? ChordQuality::major : ChordQuality::minor }, kMajorSeventh), I };
                ++count["cadência plagal"];
            }
            else
            {
                cad = { colored({ mod12(key.tonic + 7), ChordQuality::major }, kSeventh | kNinth) };  // a nona que não resolve
                kind = CadenceMark::Kind::half;
                ++count["cadência evitada"];
            }
            const double sectionEnd = spans[last].end;
            if (spans[last].end - spans[last].start >= static_cast<double>(cad.size()) * beat - 1e-6)
                splitInto(spans, last, cad, beat);
            else
                spans[last].chord = cad.back();
            if (cadences)
                for (std::size_t i = spans.size(); i-- > 0;)
                    if (spans[i].end <= sectionEnd + 1e-6 && spans[i].start < sectionEnd - 1e-6)
                    {
                        cadences->push_back({ spans[i].start, spans[i].end, kind, key });
                        break;
                    }
        }
    }
    // ---- as linguagens brasileiras modais e a de Hermeto (2026-10-17)
    else if (lang == HarmonyLanguage::northeastern || lang == HarmonyLanguage::mineira || lang == HarmonyLanguage::hermeto)
    {
        // o acorde da linguagem para cada grau (relativo à tônica da seção)
        struct Slot { int iv; ChordQuality q; std::uint8_t color; };
        for (std::size_t sIdx = 0; sIdx < sections.size(); ++sIdx)
        {
            const auto& key = keys[sIdx];
            const double u = rng.next();
            std::vector<Slot> slots;
            bool pedal = false;
            std::string tech;
            if (lang == HarmonyLanguage::northeastern)
            {
                // o sistema trimodal de José Siqueira (1981): o mixolídio do
                // baião, o lídio, e a mistura dos dois; e o repente/aboio com
                // bordão; e o modal dissonante de Guerra-Peixe / do Armorial
                if (u < 0.3)
                {
                    tech = "baião (mixolídio: I7, IV7, ♭VII, Vm7)";
                    slots = { { 0, ChordQuality::major, kSeventh }, { 5, ChordQuality::major, kSeventh }, { 10, ChordQuality::major, 0 },
                              { 7, ChordQuality::minor, kSeventh }, { 2, ChordQuality::minor, kSeventh } };
                }
                else if (u < 0.5)
                {
                    tech = "lídio (I7M(♯11), o II maior)";
                    slots = { { 0, ChordQuality::major, kMajorSeventh | kSharpEleventh }, { 2, ChordQuality::major, 0 },
                              { 7, ChordQuality::major, 0 }, { 4, ChordQuality::minor, kSeventh } };
                }
                else if (u < 0.7)
                {
                    tech = "trimodal de Siqueira (lídio-mixolídio: I7(♯11))";
                    slots = { { 0, ChordQuality::major, kSeventh | kSharpEleventh }, { 2, ChordQuality::major, kSeventh },
                              { 10, ChordQuality::major, 0 }, { 7, ChordQuality::minor, kSeventh } };
                }
                else if (u < 0.85)
                {
                    tech = "repente e aboio (o bordão, os acordes sus4 sobre o pedal)";
                    slots = { { 0, ChordQuality::major, kSus4 }, { 0, ChordQuality::major, 0 }, { 10, ChordQuality::major, kSus4 },
                              { 5, ChordQuality::major, kSus4 } };
                    pedal = true;
                }
                else
                {
                    tech = "Guerra-Peixe / Armorial (o modal com as notas acrescentadas)";
                    slots = { { 0, ChordQuality::major, kSus4 | kNinth }, { 10, ChordQuality::major, kNinth },
                              { 5, ChordQuality::major, kSeventh | kSharpEleventh }, { 2, ChordQuality::minor, kNinth | kSeventh } };
                    pedal = rng.next() < 0.5;
                }
            }
            else if (lang == HarmonyLanguage::mineira)
            {
                // o Clube da Esquina: o dórico e o mixolídio, o sus4, o Vm7, o
                // ♭II7M(♯11) como dominante, os pedais
                const bool dorian = u < 0.5;
                tech = dorian ? "mineira dórica (im7, IV7, ♭VII7M, Vm7)" : "mineira mixolídia (I7sus4, ♭VII7M, Vm7, IVsus)";
                slots = dorian ? std::vector<Slot> { { 0, ChordQuality::minor, kSeventh | kNinth }, { 5, ChordQuality::major, kSeventh },
                                                     { 10, ChordQuality::major, kMajorSeventh }, { 7, ChordQuality::minor, kSeventh },
                                                     { 3, ChordQuality::major, kMajorSeventh } }
                               : std::vector<Slot> { { 0, ChordQuality::major, kSeventh | kSus4 }, { 10, ChordQuality::major, kMajorSeventh },
                                                     { 7, ChordQuality::minor, kSeventh }, { 5, ChordQuality::major, kSus4 | kNinth },
                                                     { 2, ChordQuality::minor, kSeventh } };
                pedal = rng.next() < 0.4;
            }
            else
                tech = "Hermeto (a rearmonização densa, o baixo cromático)";
            ++count[tech];
            if (pedal)
                ++count["pedais"];
            // as fundamentais da seção levadas ao grau mais perto da linguagem
            for (std::size_t i = 0; i < spans.size(); ++i)
            {
                auto& s = spans[i];
                if (sectionOf(s.start) != sIdx)
                    continue;
                if (lang == HarmonyLanguage::hermeto)
                {
                    // as cores ricas de Hermeto, sobre a fundamental lida (o
                    // Calendário do Som: a melodia primeiro, a cifra depois)
                    static const std::uint8_t rich[] = { kMajorSeventh | kNinth | kSharpEleventh, kSeventh | kNinth | kThirteenth,
                                                         kSeventh | kFlatNinth | kSharpEleventh, kSeventh | kNinth, kSus4 | kSeventh | kNinth,
                                                         kMajorSeventh | kNinth };
                    auto& c = s.chord;
                    if (c.quality == ChordQuality::diminished)
                        c.color = kSeventh | kNinth;
                    else if (c.quality == ChordQuality::minor)
                        c.color = rng.next() < 0.5 ? (kSeventh | kNinth) : (kMajorSeventh | kNinth);  // o m(7M) também
                    else
                        c.color = rich[static_cast<std::size_t>(rng.next() * 6.0) % 6];
                    continue;
                }
                int best = 0, bestD = 99;
                for (std::size_t k = 0; k < slots.size(); ++k)
                {
                    const int pc = mod12(key.tonic + slots[k].iv);
                    const int d = std::min(mod12(pc - s.chord.root), mod12(s.chord.root - pc));
                    if (d < bestD || (d == bestD && rng.next() < 0.5))
                    {
                        bestD = d;
                        best = static_cast<int>(k);
                    }
                }
                const auto& sl = slots[static_cast<std::size_t>(best)];
                s.chord = { mod12(key.tonic + sl.iv), sl.q, sl.color };
                if (pedal)
                    s.bassPc = key.tonic;
            }
            // Hermeto: o acorde de chegada cromática (meio tom acima do
            // próximo) na segunda metade dos acordes longos, e o baixo que
            // anda por semitons até a próxima fundamental
            if (lang == HarmonyLanguage::hermeto)
                for (std::size_t i = 0; i + 1 < spans.size(); ++i)
                {
                    if (sectionOf(spans[i].start) != sIdx || spans[i].end - spans[i].start < 2.0 * beat - 1e-6 || rng.next() > 0.6)
                        continue;
                    const auto next = spans[i + 1].chord;
                    Chord approach { mod12(next.root + 1), ChordQuality::major,
                                     static_cast<std::uint8_t>(rng.next() < 0.5 ? (kMajorSeventh | kSharpEleventh) : (kSeventh | kSharpEleventh)) };
                    const auto keep = spans[i].chord;
                    const int bass0 = keep.root;
                    splitInto(spans, i, { keep, approach }, beat);
                    // o baixo: um passo cromático entre as duas fundamentais
                    if (i + 1 < spans.size())
                        spans[i + 1].bassPc = mod12(bass0 + (mod12(next.root - bass0) <= 6 ? 1 : -1));
                    ++count["chegadas cromáticas (o acorde meio tom acima)"];
                    ++i;
                }
            // a cadência da linguagem
            std::size_t last = spans.size();
            for (std::size_t i = 0; i < spans.size(); ++i)
                if (sectionOf(spans[i].start) == sIdx)
                    last = i;
            if (last == spans.size())
                continue;
            const bool final = sIdx + 1 == sections.size();
            std::vector<Chord> cad;
            CadenceMark::Kind kind = CadenceMark::Kind::plagal;
            const double v = rng.next();
            if (lang == HarmonyLanguage::northeastern)
            {
                const Chord I7 { key.tonic, ChordQuality::major, kSeventh };
                if (v < 0.5 || final)
                {
                    cad = { Chord { mod12(key.tonic + 10), ChordQuality::major, 0 }, I7 };  // ♭VII → I7 (o mixolídio)
                    ++count["cadência ♭VII → I7"];
                }
                else if (v < 0.8)
                {
                    cad = { Chord { mod12(key.tonic + 5), ChordQuality::major, kSeventh }, I7 };  // IV7 → I7
                    ++count["cadência IV7 → I7"];
                }
                else
                {
                    cad = { Chord { mod12(key.tonic + 2), ChordQuality::major, 0 }, Chord { key.tonic, ChordQuality::major, kSharpEleventh } };
                    ++count["cadência lídia II → I"];
                }
            }
            else if (lang == HarmonyLanguage::mineira)
            {
                const Chord I { key.tonic, key.major ? ChordQuality::major : ChordQuality::minor,
                                static_cast<std::uint8_t>(key.major ? (kMajorSeventh | kNinth) : (kSeventh | kNinth)) };
                if (v < 0.4 || final)
                {
                    cad = { Chord { mod12(key.tonic + 1), ChordQuality::major, kMajorSeventh | kSharpEleventh }, I };
                    ++count["♭II7M(♯11) → I"];
                }
                else if (v < 0.7)
                {
                    cad = { Chord { mod12(key.tonic + 7), ChordQuality::minor, kSeventh }, I };
                    ++count["Vm7 → I"];
                }
                else
                {
                    cad = { Chord { mod12(key.tonic + 5), ChordQuality::major, kSus4 | kNinth }, I };
                    ++count["IVsus(9) → I"];
                }
            }
            else
            {
                const Chord I { key.tonic, key.major ? ChordQuality::major : ChordQuality::minor,
                                static_cast<std::uint8_t>(key.major ? (kMajorSeventh | kNinth) : (kMajorSeventh | kNinth)) };
                if (v < 0.5 || final)
                {
                    cad = { Chord { mod12(key.tonic + 1), ChordQuality::major, kMajorSeventh | kSharpEleventh }, I };
                    ++count["♭II7M(♯11) → I7M(9)"];
                }
                else
                {
                    cad = { Chord { mod12(key.tonic + 8), ChordQuality::major, kMajorSeventh | kSharpEleventh }, I };  // ♭VI7M(♯11) → I
                    ++count["♭VI7M(♯11) → I"];
                }
            }
            const double sectionEnd = spans[last].end;
            if (spans[last].end - spans[last].start >= static_cast<double>(cad.size()) * beat - 1e-6)
                splitInto(spans, last, cad, beat);
            else
                spans[last].chord = cad.back();
            if (cadences)
                for (std::size_t i = spans.size(); i-- > 0;)
                    if (spans[i].end <= sectionEnd + 1e-6 && spans[i].start < sectionEnd - 1e-6)
                    {
                        cadences->push_back({ spans[i].start, spans[i].end, kind, key });
                        break;
                    }
        }
    }
    else
    {
        // ---- as funcionais (a funcional, o jazz, a brasileira)
        // 1. as dominantes secundárias (no jazz, às vezes com o seu ii)
        const double pSec = mode == strategy::Mode::transfigure ? 0.45 : mode == strategy::Mode::counter ? 0.3
                          : mode == strategy::Mode::interrupt ? 0.2 : 0.25;
        for (std::size_t i = 1; i < spans.size(); ++i)
        {
            const std::size_t sec = sectionOf(spans[i].start);
            const auto& key = keys[sec];
            const auto x = spans[i].chord;
            const auto prev = spans[i - 1];
            const bool lastOfSection = i + 1 >= spans.size() || spans[i + 1].start >= sections[sec].second - 1e-6;
            if (lastOfSection || x.root == key.tonic || x.quality == ChordQuality::diminished)
                continue;
            if (prev.end - prev.start < 2.0 * beat - 1e-6 || (prev.chord.root == mod12(x.root + 7) && prev.chord.quality == ChordQuality::major))
                continue;
            if (rng.next() >= pSec)
                continue;
            const Chord dom { mod12(x.root + 7), ChordQuality::major };
            if (jazz && prev.end - prev.start >= 4.0 * beat - 1e-6 && rng.next() < 0.5)
            {
                splitInto(spans, i - 1, { Chord { mod12(x.root + 2), ChordQuality::minor }, dom }, beat);  // o ii–V para o x
                ++count["ii–V secundários"];
            }
            else if (brazil && x.quality == ChordQuality::minor && mod12(x.root - prev.chord.root) == 2
                     && rng.next() < 0.5)
            {
                // o diminuto de passagem: I – ♯I° – ii (o choro)
                splitInto(spans, i - 1, { prev.chord, Chord { mod12(prev.chord.root + 1), ChordQuality::diminished, kSixth } }, beat);
                ++count["diminutos de passagem"];
            }
            else
            {
                splitInto(spans, i - 1, { prev.chord, dom }, beat);
                ++count["dominantes secundárias"];
            }
            ++i;
        }
        // 2. as cadências e as preparações de modulação
        for (std::size_t sIdx = 0; sIdx < sections.size(); ++sIdx)
        {
            const auto& key = keys[sIdx];
            const bool final = sIdx + 1 == sections.size();
            std::size_t last = spans.size();
            for (std::size_t i = 0; i < spans.size(); ++i)
                if (spans[i].start >= sections[sIdx].first - 1e-6 && spans[i].start < sections[sIdx].second - 1e-6)
                    last = i;
            if (last == spans.size())
                continue;
            const Chord I = degree(key, 1), IV = degree(key, 4), V = degree(key, 5), ii = degree(key, 2), vi = degree(key, 6);
            std::vector<Chord> cad;
            auto kind = CadenceMark::Kind::authentic;
            std::string name;
            // (a mesma tônica em maior e menor não é modulação)
            if (!final && keys[sIdx + 1].tonic != key.tonic)
            {
                cad = { I, degree(keys[sIdx + 1], 5) };
                kind = CadenceMark::Kind::modulation;
                name = "modulações preparadas";
            }
            else if (final)
            {
                const double u = rng.next();
                if (jazz)
                {
                    cad = u < 0.3 ? std::vector<Chord> { colored({ mod12(key.tonic + 5), ChordQuality::minor }, kSeventh),
                                                         colored({ mod12(key.tonic + 10), ChordQuality::major }, kSeventh | kNinth), I }  // a backdoor
                                  : std::vector<Chord> { ii, V, I };
                    name = u < 0.3 ? "backdoor (iv7–♭VII7–I)" : "ii–V–I";
                }
                else if (brazil)
                {
                    cad = { colored(ii, kSeventh), colored(V, kSeventh | kFlatNinth), colored(I, key.major ? (kSixth | kNinth) : kSixth) };
                    name = "IIm7–V7(♭9)–I6(9)";
                }
                else if (!key.major && u < 0.4)
                {
                    cad = { Chord { mod12(key.tonic + 1), ChordQuality::major }, V, I };  // a napolitana
                    name = "napolitana (♭II⁶–V–i)";
                }
                else
                {
                    cad = { IV, V, I };
                    name = "cadências perfeitas (IV–V–I)";
                }
                // a picarda: a peça menor que termina maior
                if (!key.major && !jazz && rng.next() < 0.5)
                {
                    cad.back().quality = ChordQuality::major;
                    name += " com a terça picarda";
                }
            }
            else
            {
                const double u = rng.next();
                switch (mode)
                {
                    case strategy::Mode::counter:
                        cad = { V, vi };
                        kind = CadenceMark::Kind::deceptive;
                        name = "cadências de engano";
                        break;
                    case strategy::Mode::interrupt:
                        cad = { I, V };
                        kind = CadenceMark::Kind::half;
                        name = "cadências suspensivas";
                        break;
                    default:
                        if (mode == strategy::Mode::transfigure && u < 0.25)
                        {
                            cad = brazil ? std::vector<Chord> { IV, colored({ mod12(key.tonic + 5), ChordQuality::minor }, kSixth), I }
                                         : std::vector<Chord> { IV, I };
                            kind = CadenceMark::Kind::plagal;
                            name = brazil ? "plagais com o iv menor" : "cadências plagais";
                        }
                        else if (u < 0.5)
                        {
                            if (jazz && rng.next() < 0.35)
                            {
                                cad = { ii, colored({ mod12(key.tonic + 1), ChordQuality::major }, kSeventh), I };  // o trítono
                                name = "ii–♭II7–I (o trítono)";
                            }
                            else
                            {
                                cad = (jazz || brazil) ? std::vector<Chord> { ii, V, I } : std::vector<Chord> { V, I };
                                name = (jazz || brazil) ? "ii–V–I" : "cadências autênticas";
                            }
                        }
                        else if (u < 0.6 && !jazz && !brazil)
                        {
                            cad = { V, I };
                            kind = CadenceMark::Kind::authentic;
                            name = "autênticas imperfeitas (o I com a terça no baixo)";
                        }
                        else if (!key.major && u < 0.8 && !jazz)
                        {
                            cad = { Chord { mod12(key.tonic + 5), ChordQuality::minor }, V };  // a frígia: iv⁶ → V
                            kind = CadenceMark::Kind::half;
                            name = "meias cadências frígias (iv⁶–V)";
                        }
                        else if (jazz)
                        {
                            cad = { I, vi, ii, V };  // o turnaround
                            kind = CadenceMark::Kind::half;
                            name = "turnarounds (I–vi–ii–V)";
                        }
                        else
                        {
                            cad = { I, V };
                            kind = CadenceMark::Kind::half;
                            name = "cadências suspensivas";
                        }
                        break;
                }
            }
            ++count[name];
            const double sectionEnd = spans[last].end;
            const double len = spans[last].end - spans[last].start;
            const std::size_t firstNew = last;
            if (len >= static_cast<double>(cad.size()) * beat - 1e-6)
                splitInto(spans, last, cad, beat);
            else
            {
                std::size_t k = cad.size();
                std::size_t i = last + 1;
                while (k > 0 && i > 0)
                {
                    --i;
                    --k;
                    if (spans[i].start < sections[sIdx].first - 1e-6)
                        break;
                    spans[i].chord = cad[k];
                }
            }
            // as inversões da cadência: a frígia com o ♭6 no baixo; a autêntica
            // imperfeita com a terça no baixo do I
            for (std::size_t i = firstNew; i < spans.size() && spans[i].start < sectionEnd - 1e-6; ++i)
            {
                if (name.rfind("meias cadências frígias", 0) == 0 && spans[i].chord.root == mod12(key.tonic + 5))
                    spans[i].bassPc = mod12(key.tonic + 8);
                if (name.rfind("autênticas imperfeitas", 0) == 0 && spans[i].chord.root == key.tonic)
                    spans[i].bassPc = mod12(key.tonic + (key.major ? 4 : 3));
            }
            if (cadences)
                for (std::size_t i = spans.size(); i-- > 0;)
                    if (spans[i].end <= sectionEnd + 1e-6 && spans[i].start < sectionEnd - 1e-6)
                    {
                        cadences->push_back({ spans[i].start, spans[i].end, kind, key });
                        break;
                    }
        }
        // 3. as cores da linguagem
        if (jazz || brazil)
            colorize(spans, sections, keys, lang, rng);
    }

    std::string keysText;
    for (std::size_t i = 0; i < keys.size() && i < 6; ++i)
        keysText += (i ? ", " : "") + keyName(keys[i]);
    if (keys.size() > 6)
        keysText += "…";
    std::string what;
    for (const auto& [k, n] : count)
        what += (what.empty() ? "" : ", ") + std::to_string(n) + " " + k;
    return std::string("máquina de harmonia, linguagem ") + harmonyLanguageName(lang) + " (tonalidades por Krumhansl–Kessler: "
           + keysText + "): " + what;
}

namespace
{
// o baixo numa altura (para medir a direção): a fundamental perto do Dó 2
float bassPitch(int pc) { return 36.0f + static_cast<float>(mod12(pc)); }

// o movimento do baixo de uma fundamental à outra: o menor caminho
float bassMove(int from, int to)
{
    int d = mod12(to - from);
    if (d > 6)
        d -= 12;
    return static_cast<float>(d);
}

bool parallelPair(float a0, float a1, float b0, float b1)
{
    // duas vozes que se movem no mesmo sentido, de uma quinta (ou oitava) a
    // outra quinta (ou oitava)
    const float ma = a1 - a0, mb = b1 - b0;
    if (std::abs(ma) < 0.01f || std::abs(mb) < 0.01f || (ma > 0) != (mb > 0))
        return false;
    const int i0 = mod12(static_cast<int>(std::lround(b0 - a0))), i1 = mod12(static_cast<int>(std::lround(b1 - a1)));
    return (i0 == 7 && i1 == 7) || (i0 == 0 && i1 == 0);
}
} // namespace

int countParallels(const std::vector<float>& prev, const std::vector<float>& next, int prevBassPc, int bassPc)
{
    if (prev.size() != next.size())
        return 0;
    std::vector<float> a { bassPitch(prevBassPc) }, b { bassPitch(prevBassPc) + bassMove(prevBassPc, bassPc) };
    a.insert(a.end(), prev.begin(), prev.end());
    b.insert(b.end(), next.begin(), next.end());
    int n = 0;
    for (std::size_t i = 0; i < a.size(); ++i)
        for (std::size_t j = i + 1; j < a.size(); ++j)
            n += parallelPair(a[i], b[i], a[j], b[j]) ? 1 : 0;
    return n;
}

std::vector<float> voiceLead(const std::vector<float>& prev, const std::vector<int>& chordPcs, int prevBassPc, int bassPc, int leadingPc)
{
    // três vozes de cima na tríade; quatro nos acordes com cor (a 7ª, a 9ª…)
    std::vector<int> pcs = chordPcs;
    while (pcs.size() < 3)
        pcs.push_back(pcs.empty() ? 0 : pcs.front());  // a fundamental dobrada
    if (pcs.size() > 4)
        pcs.resize(4);
    const std::size_t n = pcs.size();
    std::vector<float> best;
    double bestScore = 1e18;
    std::vector<int> perm(n);
    for (std::size_t i = 0; i < n; ++i)
        perm[i] = static_cast<int>(i);
    std::vector<float> cand(n);
    do
    {
        // as oitavas de cada voz: as que caem entre Dó 3 (48) e Fá 5 (77)
        std::vector<std::vector<float>> opts(n);
        for (std::size_t v = 0; v < n; ++v)
            for (int o = 3; o <= 6; ++o)
            {
                const float p = static_cast<float>(12 * (o + 1) + mod12(pcs[static_cast<std::size_t>(perm[v])]));
                if (p >= 48.0f && p <= 77.0f)
                    opts[v].push_back(p);
            }
        // a busca, voz a voz, de baixo para cima (sem cruzamento, no máximo
        // uma oitava entre vizinhas)
        std::function<void(std::size_t)> go = [&](std::size_t v) {
            if (v == n)
            {
                double score = 0.0;
                if (prev.size() == n)
                {
                    for (std::size_t k = 0; k < n; ++k)
                        score += std::abs(cand[k] - prev[k]);
                    score += 12.0 * countParallels(prev, cand, prevBassPc, bassPc);
                    if (leadingPc >= 0)
                        for (std::size_t k = 0; k < n; ++k)
                            if (mod12(static_cast<int>(std::lround(prev[k]))) == mod12(leadingPc) && std::abs(cand[k] - prev[k] - 1.0f) > 0.01f)
                                score += 8.0;  // a sensível não subiu à tônica
                }
                else if (!prev.empty())
                {
                    // de três para quatro vozes (ou o contrário): perto do centro anterior
                    double mp = 0.0, mc = 0.0;
                    for (float p : prev) mp += p;
                    for (float c : cand) mc += c;
                    score += std::abs(mc / n - mp / prev.size()) * 2.0;
                }
                else
                    score += std::abs(cand[n / 2] - 62.0f);  // o primeiro acorde em volta do Ré central
                if (score < bestScore)
                {
                    bestScore = score;
                    best = cand;
                }
                return;
            }
            for (float p : opts[v])
            {
                if (v > 0 && (p <= cand[v - 1] || p - cand[v - 1] > 12.0f))
                    continue;
                cand[v] = p;
                go(v + 1);
            }
        };
        go(0);
    } while (std::next_permutation(perm.begin(), perm.end()));
    return best;
}
std::vector<CadenceEntry> parseCadenceArchive(const std::string& text, std::vector<std::string>& errors)
{
    std::vector<CadenceEntry> out;
    std::size_t pos = 0;
    int lineNo = 0;
    const auto trim = [](std::string v) {
        const auto a = v.find_first_not_of(" \t\r");
        const auto b = v.find_last_not_of(" \t\r");
        return a == std::string::npos ? std::string() : v.substr(a, b - a + 1);
    };
    const auto parseChord = [&](const std::string& spec, Chord& c, int& bass) {
        std::istringstream in(spec);
        int iv = -1;
        std::string q, tok;
        if (!(in >> iv >> q) || iv < 0 || iv > 11)
            return false;
        c = { iv, ChordQuality::major, 0 };
        bass = -1;
        if (q == "m") c.quality = ChordQuality::minor;
        else if (q == "dim") c.quality = ChordQuality::diminished;
        else if (q == "aug") c.quality = ChordQuality::augmented;
        else if (q != "M") return false;
        while (in >> tok)
        {
            if (tok == "7") c.color |= kSeventh;
            else if (tok == "7M") c.color |= kMajorSeventh;
            else if (tok == "9") c.color |= kNinth;
            else if (tok == "b9") c.color |= kFlatNinth;
            else if (tok == "#11") c.color |= kSharpEleventh;
            else if (tok == "13") c.color |= kThirteenth;
            else if (tok == "6") c.color |= kSixth;
            else if (tok == "sus4") c.color |= kSus4;
            else if (tok.size() >= 2 && tok[0] == '/') bass = std::atoi(tok.c_str() + 1) % 12;
            else return false;
        }
        return true;
    };
    while (pos <= text.size())
    {
        const auto nl = text.find('\n', pos);
        const std::string line = trim(text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos));
        pos = nl == std::string::npos ? text.size() + 1 : nl + 1;
        ++lineNo;
        if (line.empty() || line[0] == '#')
            continue;
        if (line[0] == '=')
        {
            out.push_back({});
            out.back().name = trim(line.substr(1));
            continue;
        }
        const auto colon = line.find(':');
        if (out.empty() || colon == std::string::npos)
        {
            errors.push_back("linha " + std::to_string(lineNo) + ": fora de um bloco");
            continue;
        }
        auto& e = out.back();
        const std::string key = line.substr(0, colon), value = trim(line.substr(colon + 1));
        if (key == "família") e.family = value;
        else if (key == "linguagem") e.language = value;
        else if (key == "graus") e.degrees = value;
        else if (key == "soprano") e.soprano = value;
        else if (key == "baixo") e.bass = value;
        else if (key == "efeito") e.effect = value;
        else if (key == "fonte") e.source = value;
        else if (key == "saliência")
        {
            std::istringstream in(value);
            std::string item;
            while (std::getline(in, item, ','))
            {
                const auto c2 = item.find(':');
                const int pc = c2 == std::string::npos ? -1 : std::atoi(item.substr(0, c2).c_str());
                const int w = c2 == std::string::npos ? 0 : std::atoi(item.substr(c2 + 1).c_str());
                if (pc < 0 || pc > 11 || w < 1 || w > 3)
                    errors.push_back("linha " + std::to_string(lineNo) + ": saliência ilegível em \"" + e.name + "\" (nota:peso, 0..11 : 1..3)");
                else
                    e.salience[pc] = w;
            }
        }
        else if (key == "acordes")
        {
            std::size_t a = 0;
            while (a <= value.size())
            {
                const auto bar = value.find('|', a);
                Chord c;
                int bass = -1;
                if (!parseChord(value.substr(a, bar == std::string::npos ? std::string::npos : bar - a), c, bass))
                    errors.push_back("linha " + std::to_string(lineNo) + ": acorde ilegível em \"" + e.name + "\"");
                else
                {
                    e.chords.push_back(c);
                    e.bassPcs.push_back(bass);
                }
                a = bar == std::string::npos ? value.size() + 1 : bar + 1;
            }
        }
        else
            errors.push_back("linha " + std::to_string(lineNo) + ": campo desconhecido \"" + key + "\"");
    }
    for (const auto& e : out)
        if (e.chords.empty() || e.source.empty())
            errors.push_back("\"" + e.name + "\": sem acordes ou sem fonte");
    return out;
}

int drawArchiveCadences(std::vector<ChordSpan>& spans, std::vector<CadenceMark>& cadences, const std::vector<CadenceEntry>& archive,
                        HarmonyLanguage language, double beat, std::uint64_t seed, double chance, double blendChance,
                        std::vector<std::string>* invented)
{
    std::vector<const CadenceEntry*> pool;
    for (const auto& e : archive)
    {
        HarmonyLanguage l;
        if (parseHarmonyLanguage(e.language, l) && l == language && !e.chords.empty())
            pool.push_back(&e);
    }
    if (pool.empty() || spans.empty())
        return 0;
    Rng rng { seed * 0x9E3779B97F4A7C15ull + 0xCADE };
    int drawn = 0;
    double prevEnd = 0.0;
    for (auto& mark : cadences)
    {
        const double markEnd = mark.end;
        const bool take = rng.next() < chance;
        const CadenceEntry* pick = pool[static_cast<std::size_t>(rng.next() * static_cast<double>(pool.size())) % pool.size()];
        // a chegada inventada: a mistura conceitual desta com outra do arquivo
        CadenceEntry blended;
        if (take && pool.size() >= 2 && rng.next() < blendChance)
        {
            const CadenceEntry* other = pick;
            for (int tries = 0; tries < 8 && other == pick; ++tries)
                other = pool[static_cast<std::size_t>(rng.next() * static_cast<double>(pool.size())) % pool.size()];
            const auto bl = other != pick ? blendCadences(*pick, *other, 3) : std::vector<CadenceBlend> {};
            if (!bl.empty())
            {
                const auto& b = bl[static_cast<std::size_t>(rng.next() * static_cast<double>(bl.size())) % bl.size()];
                blended.name = "mistura de \"" + pick->name + "\" com \"" + other->name + "\": " + b.penultimate.name() + " → "
                             + b.final.name();
                blended.chords = { b.penultimate, b.final };
                blended.bassPcs = { -1, -1 };
                pick = &blended;
            }
        }
        const CadenceEntry& e = *pick;
        if (take)
        {
            // o último acorde da cadência marcada
            std::size_t last = spans.size();
            for (std::size_t i = 0; i < spans.size(); ++i)
                if (std::abs(spans[i].end - markEnd) < 1e-6)
                    last = i;
            if (last < spans.size())
            {
                // volta até ter acordes ou tempo para a cadência inteira, sem
                // passar da cadência anterior
                const std::size_t n = e.chords.size();
                std::size_t first = last;
                while (first > 0 && spans[first - 1].start >= prevEnd - 1e-6 && last - first + 1 < n
                       && markEnd - spans[first].start < static_cast<double>(n) * beat - 1e-6)
                    --first;
                if (markEnd - spans[first].start >= static_cast<double>(n) * beat - 1e-6)
                {
                    ChordSpan merged = spans[first];
                    merged.end = spans[last].end;
                    spans.erase(spans.begin() + static_cast<std::ptrdiff_t>(first) + 1, spans.begin() + static_cast<std::ptrdiff_t>(last) + 1);
                    spans[first] = merged;
                    std::vector<Chord> cs;
                    for (const auto& c : e.chords)
                        cs.push_back({ mod12(c.root + mark.key.tonic), c.quality, c.color });
                    splitInto(spans, first, cs, beat);
                    for (std::size_t k = 0; k < n && first + k < spans.size(); ++k)
                        spans[first + k].bassPc = e.bassPcs[k] < 0 ? -1 : mod12(e.bassPcs[k] + mark.key.tonic);
                    mark.start = spans[first + n - 1].start;
                    ++drawn;
                    if (invented && pick == &blended)
                        invented->push_back(blended.name);  // só a que entrou de fato
                }
            }
        }
        prevEnd = markEnd;
    }
    return drawn;
}

namespace
{
struct Salient
{
    std::map<int, int> key;   // nota relativa à tônica (keyNote) → saliência
    std::map<int, int> type;  // intervalo relativo à fundamental do acorde (chordNote) → saliência
};

Salient salience(const Chord& pen, const Chord& fin, const std::map<int, int>& given = {})
{
    Salient s;
    const auto pcs = pen.pitchClasses();
    const auto fpcs = fin.pitchClasses();
    const int finalThird = fpcs.size() >= 2 ? fpcs[1] : 4;
    for (int pc : pcs)
    {
        int p = 1;
        if (pc == 11 || pc == 1)
            p = 3;  // a sensível de baixo e a de cima: chegam à tônica por semitom
        else if (pc == pen.root || std::abs(mod12(pc - finalThird)) == 1 || mod12(finalThird - pc) == 1)
            p = 2;  // a fundamental do penúltimo; a que resolve por semitom na terça do final
        s.key[pc] = std::max(s.key[pc], p);
    }
    // a saliência dada pelo autor no arquivo vence a regra
    for (const auto& [pc, w] : given)
        if (s.key.count(pc))
            s.key[pc] = w;
    // o tipo: os intervalos do acorde; no de dominante (3ª maior e 7ª menor),
    // o trítono vale 3 em cada ponta, como no artigo
    const bool dominant = (pen.color & kSeventh) && pen.quality == ChordQuality::major;
    for (int pc : pcs)
    {
        const int iv = mod12(pc - pen.root);
        s.type[iv] = dominant && (iv == 4 || iv == 10) ? 3 : 1;
    }
    return s;
}

std::string noteName(int pc)
{
    static const char* const k[] = { "1", "♭2", "2", "♭3", "3", "4", "♯4", "5", "♭6", "6", "♭7", "7" };
    return k[mod12(pc)];
}
} // namespace

std::vector<CadenceBlend> blendCadences(const CadenceEntry& a, const CadenceEntry& b, int maxResults)
{
    std::vector<CadenceBlend> out;
    // só cadências que chegam à tônica (o final fixo é a premissa do modelo)
    if (a.chords.size() < 2 || b.chords.size() < 2 || a.chords.back().root != 0 || b.chords.back().root != 0)
        return out;
    const Chord& penA = a.chords[a.chords.size() - 2];
    const Chord& penB = b.chords[b.chords.size() - 2];
    const Chord fin = a.chords.back();
    const auto sa = salience(penA, fin, a.salience), sb = salience(penB, b.chords.back(), b.salience);
    using Q = ChordQuality;
    // os tipos usuais (tríades e tétrades), como no artigo: sem a 2ª menor
    // contra a fundamental (o ♭9) e sem acordes grandes que acumulam traços
    // só por terem muitas notas
    static const std::uint8_t colors[] = { 0, kSeventh, kMajorSeventh, kSixth };
    for (int root = 0; root < 12; ++root)
        for (Q q : { Q::major, Q::minor, Q::diminished, Q::augmented })
            for (std::uint8_t col : colors)
            {
                Chord c { root, q, col };
                if (root == fin.root)
                    continue;  // o penúltimo não é o próprio final
                if (c == penA && c.color == penA.color)
                    continue;
                if (c == penB && c.color == penB.color)
                    continue;
                const auto pcs = c.pitchClasses();
                const auto has = [&](int pc) { return std::find(pcs.begin(), pcs.end(), mod12(pc)) != pcs.end(); };
                double score = 0.0;
                int fromA = 0, fromB = 0, foreign = 0;
                std::vector<std::string> kept;
                std::set<int> counted;
                for (const auto& [pc, p] : sa.key)
                    if (has(pc))
                    {
                        score += p;
                        counted.insert(pc);
                        if (!sb.key.count(pc))
                            ++fromA;  // fora do espaço genérico: só a entrada A o tem
                        if (p >= 3)
                            kept.push_back(noteName(pc) + " (de " + a.name + ")");
                    }
                for (const auto& [pc, p] : sb.key)
                    if (has(pc))
                    {
                        if (!counted.count(pc))
                            score += p;
                        else
                            score += 0.5 * p;  // no espaço genérico: as duas o têm
                        counted.insert(pc);
                        if (!sa.key.count(pc))
                            ++fromB;
                        if (p >= 3)
                            kept.push_back(noteName(pc) + " (de " + b.name + ")");
                    }
                // o tipo do acorde: os intervalos que as entradas têm (o comum vale metade da segunda vez)
                std::set<int> ivs;
                for (int pc : pcs)
                    ivs.insert(mod12(pc - root));
                bool tritoneKept = false;
                for (const auto* st : { &sa.type, &sb.type })
                    for (const auto& [iv, p] : *st)
                        if (ivs.count(iv))
                        {
                            const bool twice = st == &sb.type && sa.type.count(iv);
                            score += twice ? 0.5 * p : p;
                            tritoneKept = tritoneKept || p >= 3;
                        }
                tritoneKept = tritoneKept && ivs.count(4) && ivs.count(10);  // as duas pontas
                if (tritoneKept)
                    kept.push_back("o trítono do acorde de dominante");
                for (int pc : pcs)
                    if (!sa.key.count(pc) && !sb.key.count(pc))
                        ++foreign;
                score -= 0.5 * foreign;
                if (fromA == 0 || fromB == 0)
                    continue;  // a mistura guarda algo próprio de cada entrada (não só o espaço genérico)
                CadenceBlend bl;
                bl.penultimate = c;
                bl.final = fin;
                bl.score = score;
                for (std::size_t k = 0; k < kept.size(); ++k)
                    bl.kept += (k ? ", " : "") + kept[k];
                out.push_back(bl);
            }
    std::stable_sort(out.begin(), out.end(), [](const CadenceBlend& x, const CadenceBlend& y) { return x.score > y.score; });
    if (static_cast<int>(out.size()) > maxResults)
        out.resize(static_cast<std::size_t>(maxResults));
    return out;
}

} // namespace trioio::composition
