#include "ThirdEye.h"

#include "Listening.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>

namespace trioio::composition
{
using namespace trioio::perception;

namespace
{
struct Rng
{
    std::uint64_t s;
    std::uint64_t next()
    {
        s += 0x9E3779B97F4A7C15ull;
        std::uint64_t x = s;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) / 9007199254740992.0; }
    int range(int lo, int hi) { return lo + static_cast<int>(uniform() * (hi - lo + 1) - 1e-9); }
    bool chance(double p) { return uniform() < p; }
    float between(float a, float b) { return a + static_cast<float>(uniform()) * (b - a); }
};

enum class Rule { mirror, warp, mutate, silence, echo, retimbre, thicken, thin, freeze };

const char* ruleName(Rule r)
{
    switch (r)
    {
        case Rule::mirror: return "registro invertido";
        case Rule::warp: return "tempo deformado";
        case Rule::mutate: return "escala mutante";
        case Rule::silence: return "silêncio súbito";
        case Rule::echo: return "eco em cânone";
        case Rule::retimbre: return "troca de timbre";
        case Rule::thicken: return "densificar";
        case Rule::thin: return "rarefazer";
        case Rule::freeze: return "congelar";
    }
    return "?";
}

bool inSpan(double t, double a, double b) { return t >= a && t < b; }

// as classes de altura mais fortes do trecho, numa oitava em volta de `center`
std::vector<float> chordAround(const std::vector<Frame>& frames, double a, double b, float center, int count)
{
    const auto field = harmonicField(frames, a, b);
    std::vector<int> pcs(12);
    for (int i = 0; i < 12; ++i)
        pcs[static_cast<std::size_t>(i)] = i;
    std::sort(pcs.begin(), pcs.end(), [&](int x, int y) { return field[static_cast<std::size_t>(x)] > field[static_cast<std::size_t>(y)]; });
    std::vector<float> out;
    for (int i = 0; i < count; ++i)
    {
        float p = static_cast<float>(pcs[static_cast<std::size_t>(i % 12)]) + 12.0f * std::floor(center / 12.0f);
        if (p < center - 6.0f) p += 12.0f;
        if (p > center + 6.0f) p -= 12.0f;
        out.push_back(p + 12.0f * static_cast<float>(i / 12));
    }
    return out;
}

// Leva cada altura à classe de altura mais próxima do acorde (empate: para
// baixo). Serve para o acorde congelado virar o campo do resto da peça.
float snapToChord(float pitch, const std::vector<float>& chord)
{
    bool in[12] {};
    for (float c : chord)
        in[((static_cast<int>(std::lround(c)) % 12) + 12) % 12] = true;
    const int r = static_cast<int>(std::lround(pitch));
    for (int d = 0; d <= 6; ++d)
    {
        if (in[((r - d) % 12 + 12) % 12]) return static_cast<float>(r - d);
        if (in[((r + d) % 12 + 12) % 12]) return static_cast<float>(r + d);
    }
    return pitch;
}

// ---- leitura do momento ---------------------------------------------------
// Antes de cada intervenção, o olho LÊ o contexto: a referência em volta de t
// e a partitura como está até ali. O que fazer sai dessa leitura (afinidades),
// e não mais de um sorteio uniforme entre objetos e regras. Pedido do roteiro
// (TRIOIO_TAREFAS, retomada de 6 out. 2026): "congelar justamente o acorde que
// acabou de soar mais forte".
struct Reading
{
    float energy = 0.0f, density = 0.0f, brightness = 0.0f, noisiness = 0.0f;  // referência, t..t+4 s
    float trend = 0.0f;          // energia depois − antes (−1..1): sobe ou cai
    float changeAhead = 0.0f;    // novidade máxima nos próximos 4 s (0..1): seção mudando
    float notesPerSecond = 0.0f; // partitura (motivo), t..t+4 s
    float span = 0.0f;           // âmbito das alturas do motivo em volta de t, semitons
    float repetition = 0.0f;     // 0..1: quanto as últimas alturas se repetem
    float stasis = 0.0f;         // 0..1: campo harmônico parado (antes ≈ depois)
    bool sameTimbre = false;     // o motivo está num timbre só há 8 s
    float body = 0.0f;           // 0..1: a referência tem corpo espectral definido aqui
    float peak = 0.0f;           // força do acorde mais forte dos últimos 3 s (0 = nenhum)
    std::vector<float> peakChord;  // as alturas que soavam juntas nele
};

float clamp01(float x) { return std::clamp(x, 0.0f, 1.0f); }

float cosine(const PitchField& a, const PitchField& b)
{
    double ab = 0, aa = 0, bb = 0;
    for (std::size_t i = 0; i < 12; ++i) { ab += a[i] * b[i]; aa += a[i] * a[i]; bb += b[i] * b[i]; }
    return aa > 0 && bb > 0 ? static_cast<float>(ab / std::sqrt(aa * bb)) : 0.0f;
}

Reading readMoment(const Score& score, const std::vector<Frame>& frames, const std::vector<Onset>& onsets,
                   double t, const std::function<float(double)>& noveltyAt)
{
    Reading r;
    const double end = score.duration;
    const auto now = summarize(frames, onsets, t, std::min(end, t + 4.0));
    const auto before = summarize(frames, onsets, std::max(0.0, t - 4.0), t);
    r.energy = now.energy;
    r.density = now.density;
    r.brightness = now.brightness;
    r.noisiness = hasTimbre(now) ? now.noisiness : 0.0f;
    r.trend = std::clamp(now.energy - before.energy, -1.0f, 1.0f);
    for (double s = t + 0.5; s < std::min(end, t + 4.0); s += 0.5)
        r.changeAhead = std::max(r.changeAhead, noveltyAt(s));
    if (const auto* sp = spectralAt(score, t); sp && sp->ratios.size() >= 3 && sp->noise < 0.35f)
        r.body = 1.0f - sp->noise / 0.35f;
    r.stasis = clamp01((cosine(harmonicField(frames, std::max(0.0, t - 6.0), t),
                               harmonicField(frames, t, std::min(end, t + 6.0))) - 0.6f) / 0.4f);

    int ahead = 0;
    float lo = 200.0f, hi = -200.0f;
    std::vector<float> recent;
    int firstTimbre = -1;
    bool oneTimbre = true;
    const Note* strongest = nullptr;
    for (const auto& n : score.notes)
    {
        if (n.voice != 0)
            continue;
        if (n.time >= t && n.time < t + 4.0) ++ahead;
        if (n.time >= t - 4.0 && n.time < t + 4.0) { lo = std::min(lo, n.pitch); hi = std::max(hi, n.pitch); }
        if (n.time >= t - 4.0 && n.time < t) recent.push_back(n.pitch);
        if (n.time >= t - 8.0 && n.time < t + 4.0)
        {
            const int tb = static_cast<int>(n.timbre);
            if (firstTimbre < 0) firstTimbre = tb;
            else if (tb != firstTimbre) oneTimbre = false;
        }
        if (n.time >= t - 3.0 && n.time < t && (!strongest || n.velocity > strongest->velocity))
            strongest = &n;
    }
    r.notesPerSecond = static_cast<float>(ahead) / 4.0f;
    r.span = hi > lo ? hi - lo : 0.0f;
    r.sameTimbre = firstTimbre >= 0 && oneTimbre;
    if (recent.size() >= 4)
    {
        std::vector<int> pcs;
        for (float p : recent) pcs.push_back(static_cast<int>(std::lround(p)) % 12);
        std::sort(pcs.begin(), pcs.end());
        const auto distinct = std::unique(pcs.begin(), pcs.end()) - pcs.begin();
        r.repetition = 1.0f - static_cast<float>(distinct) / static_cast<float>(recent.size());
    }
    if (strongest)
    {
        // o acorde: tudo o que soava (motivo, baixo, eco) no ataque da nota mais forte
        const double at = strongest->time;
        for (const auto& n : score.notes)
            if (n.time <= at + 0.03 && n.time + n.duration > at)
                r.peakChord.push_back(n.pitch);
        // "forte" é em relação à vizinhança: quanto a nota se destaca da
        // média dos 8 s em volta. Pela velocidade absoluta, quase todo trecho
        // tinha um "acorde forte" e o congelamento virou a escolha de sempre.
        double sum = 0.0;
        int count = 0;
        for (const auto& n : score.notes)
            if (n.voice == 0 && n.time >= t - 6.0 && n.time < t + 2.0) { sum += n.velocity; ++count; }
        const float mean = count ? static_cast<float>(sum / count) : strongest->velocity;
        const float prominence = clamp01((strongest->velocity - mean) / 0.25f);
        r.peak = r.peakChord.size() >= 2 ? prominence : prominence * 0.5f;
    }
    return r;
}

// As 17 escolhas do olho: 0..7 = objetos sonoros (ordem de GestureKind),
// 8..16 = regras ocultas (ordem de Rule).
constexpr int kChoices = 17;
constexpr int kObjects = 8;
static_assert(kChoices == kEyeChoices, "Composer.h kEyeChoices acompanha as escolhas do olho");

const char* reasonOf(int c)
{
    static const char* reasons[kChoices] = {
        "textura densa ou ruidosa",               // nuvem granular
        "a seção vai mudar",                      // massa de glissandos
        "a referência está ruidosa",              // faixa de ruído
        "energia alta e tonal",                   // cluster
        "a energia está mudando de direção",      // iteração
        "pouca energia, música rarefeita",        // drone
        "um acorde forte acabou de soar",         // congelamento (objeto)
        "a referência tem um corpo estável",      // corpo da referência
        "melodia repetitiva num âmbito estreito", // registro invertido
        "pulso regular e repetitivo",             // tempo deformado
        "o campo harmônico está parado",          // escala mutante
        "energia alta antes de uma mudança",      // silêncio súbito
        "melodia esparsa e tonal",                // eco em cânone
        "o mesmo timbre há muito tempo",          // troca de timbre
        "a música está rarefeita",                // densificar
        "a música está densa",                    // rarefazer
        "um acorde forte acabou de soar",         // congelar (regra)
    };
    return c >= 0 && c < kChoices ? reasons[c] : "?";
}

std::array<float, kChoices> affinities(const Reading& r)
{
    const float sparse = clamp01((1.5f - r.notesPerSecond) / 1.5f);
    const float dense = clamp01((r.notesPerSecond - 2.0f) / 3.0f);
    // planura espectral: 0,15 já é som com ar; 0,55 é ruído franco
    const float noisy = clamp01((r.noisiness - 0.15f) / 0.4f);
    const float tonal = 1.0f - noisy;
    const float narrow = clamp01(1.0f - r.span / 24.0f);
    // o olho já intervém mais onde a referência muda (o QUANDO); por isso,
    // "a seção vai mudar" só conta quando a mudança é clara. Sem esse
    // desconto, a massa de glissandos virou a escolha mais comum na v4.
    const float change = clamp01((r.changeAhead - 0.4f) / 0.6f);
    return {
        clamp01(0.6f * noisy + 0.4f * std::max(dense, r.density)),          // nuvem granular
        change,                                                                // massa de glissandos
        noisy,                                                                 // faixa de ruído
        clamp01(r.energy * tonal),                                             // cluster
        clamp01(std::abs(r.trend) * 2.5f),                                     // iteração
        clamp01(0.6f * (1.0f - r.energy) + 0.4f * sparse),                     // drone
        clamp01(r.peak * (1.0f - 0.5f * dense)),                               // congelamento
        clamp01(r.body * (0.35f + 0.65f * r.stasis) * (1.0f - 0.5f * dense)), // corpo da referência
        clamp01(r.repetition * (0.4f + 0.6f * narrow)),                        // registro invertido
        clamp01(r.repetition * r.density * 2.0f),                              // tempo deformado
        // dentro de uma seção o campo quase sempre está parado e o timbre é um
        // só: pesam menos, senão viram a escolha padrão
        0.7f * r.stasis,                                                       // escala mutante
        clamp01(r.energy * change * (1.0f - sparse) * 1.5f),                   // silêncio súbito
        clamp01(sparse * tonal),                                               // eco em cânone
        r.sameTimbre ? 0.45f : 0.1f,                                           // troca de timbre
        sparse,                                                                // densificar
        dense,                                                                 // rarefazer
        clamp01(r.peak * 0.8f),                                                // congelar (regra)
    };
}
} // namespace

const char* gestureName(GestureKind k)
{
    switch (k)
    {
        case GestureKind::cloud: return "nuvem granular";
        case GestureKind::glissMass: return "massa de glissandos";
        case GestureKind::noiseBand: return "faixa de ruído";
        case GestureKind::cluster: return "cluster";
        case GestureKind::iteration: return "iteração";
        case GestureKind::drone: return "drone";
        case GestureKind::freeze: return "congelamento";
        case GestureKind::body: return "corpo da referência";
        case GestureKind::wind: return "vento";
        case GestureKind::rain: return "chuva";
        case GestureKind::thunder: return "trovão";
        case GestureKind::water: return "água";
        case GestureKind::whoosh: return "whoosh";
        case GestureKind::riser: return "riser";
        case GestureKind::impact: return "impacto";
        case GestureKind::braam: return "braam";
        case GestureKind::fire: return "fogo";
        case GestureKind::crowd: return "multidão";
        case GestureKind::city: return "cidade";
    }
    return "?";
}

const char* eyeChoiceName(int c)
{
    if (c >= 0 && c < kObjects)
        return gestureName(static_cast<GestureKind>(c));
    if (c >= kObjects && c < kChoices)
        return ruleName(static_cast<Rule>(c - kObjects));
    return "?";
}

int eyeChoiceFromMark(const std::string& mark)
{
    const std::string obj = "Terceiro Olho · objeto sonoro: ", rule = "Terceiro Olho · regra oculta: ";
    const auto startsWith = [&](const std::string& rest, const char* name) {
        return rest.rfind(name, 0) == 0;
    };
    if (mark.rfind(obj, 0) == 0)
    {
        const auto rest = mark.substr(obj.size());
        for (int k = 0; k < kObjects; ++k)
            if (startsWith(rest, gestureName(static_cast<GestureKind>(k))))
                return k;
    }
    if (mark.rfind(rule, 0) == 0)
    {
        const auto rest = mark.substr(rule.size());
        for (int k = 0; k < kChoices - kObjects; ++k)
            if (startsWith(rest, ruleName(static_cast<Rule>(k))))
                return kObjects + k;
    }
    return -1;
}

void thirdEye(Score& score, const std::vector<Frame>& frames, const std::vector<Onset>& onsets, const Options& opt)
{
    const float u = std::clamp(opt.unpredictability, 0.0f, 1.0f);
    if (u <= 0.0f || score.duration < 6.0)
        return;
    Rng rng { opt.seed * 0xD1B54A32D192ED03ull + 0x3E };

    // novidade por segundo: o olho se interessa por onde a referência muda
    const auto win = segment(frames, onsets, score.duration, 1.0);
    std::vector<float> novelty(win.size(), 0.0f);
    float maxN = 1e-6f;
    for (std::size_t i = 1; i < win.size(); ++i)
    {
        novelty[i] = contrast(win[i - 1], win[i]);
        maxN = std::max(maxN, novelty[i]);
    }
    const std::function<float(double)> noveltyAt = [&](double t) {
        const auto i = static_cast<std::size_t>(std::clamp(t, 0.0, score.duration - 1e-6));
        return i < novelty.size() ? novelty[i] / maxN : 0.0f;
    };

    // QUANDO: candidatos a cada ~5/u s, aceitos com 0,4 + 0,6 × novidade.
    // O QUÊ: a leitura do momento (abaixo).
    std::vector<std::pair<double, double>> silences;
    int lastChoice = -1;

    // CONSEQUÊNCIAS (2026-10-07). Escuta do autor sobre a v4: as variações
    // "ficam meio equivalentes". Cada intervenção durava de 3 a 15 s e a
    // música voltava ao mesmo lugar. Agora uma intervenção pode FICAR até o
    // fim: o acorde congelado vira o campo, a escala mutada fica, o cânone
    // continua, o drone vira pedal, a melodia segue para onde o gesto levou.
    // A forma vira trajetória, e cada seed leva a peça a outro rumo. Poucas
    // por peça (1 + 3 × imprevisibilidade), para a música não se desfazer.
    const int maxLasting = 1 + static_cast<int>(std::lround(3.0 * u));
    int lasting = 0;
    const auto willLast = [&]() {
        if (lasting >= maxLasting || !rng.chance(0.35 + 0.3 * u))
            return false;
        ++lasting;
        return true;
    };
    auto& notes = score.notes;
    const auto thickenSpan = [&](double a, double b) {
        std::vector<Note> add;
        for (auto& x : notes)
            if (x.voice == 0 && inSpan(x.time, a, b) && x.duration > 0.15)
            {
                x.duration /= 2.0;
                Note k = x;
                k.time += x.duration;
                k.pitch += rng.chance(0.5) ? 1.0f : -1.0f;
                add.push_back(k);
            }
        notes.insert(notes.end(), add.begin(), add.end());
    };
    const auto thinSpan = [&](double a, double b) {
        std::vector<Note> kept;
        int i = 0;
        for (auto x : notes)
        {
            if (x.voice == 0 && inSpan(x.time, a, b))
            {
                if (i++ % 3 != 0)
                    continue;
                x.duration *= 2.5;
            }
            kept.push_back(x);
        }
        notes.swap(kept);
    };
    const auto snapFrom = [&](double from, const std::vector<float>& chord) {
        for (auto& x : notes)
            if (x.time >= from && x.voice != 4)  // a percussão não vira altura do campo
            {
                x.pitch = snapToChord(x.pitch, chord);
                x.glide = 0.0f;
            }
    };
    const Constraints& rules = opt.constraints;
    std::array<bool, kChoices> used {};
    // regras "condicionado": a condição lida no momento
    const auto holds = [](Condition c, const Reading& r) {
        switch (c)
        {
            case Condition::none: return true;
            case Condition::energyHigh: return r.energy >= 0.6f;
            case Condition::energyLow: return r.energy <= 0.35f;
            case Condition::energyRising: return r.trend > 0.08f;
            case Condition::energyFalling: return r.trend < -0.08f;
            case Condition::noisy: return r.noisiness >= 0.3f;
            case Condition::dense: return r.notesPerSecond >= 3.0f;
            case Condition::sparse: return r.notesPerSecond <= 1.0f;
            case Condition::changeAhead: return r.changeAhead >= 0.6f;
            case Condition::stasis: return r.stasis >= 0.7f;
            case Condition::strongChord: return r.peak >= 0.5f;
        }
        return true;
    };
    // regras "incompatível": o que está soando agora (escolha, início, fim)
    struct Span { int choice; double a, b; };
    std::vector<Span> spans;
    const auto clashes = [&](int c, double t) {
        for (const auto& [x, y] : rules.incompatible)
        {
            const int other = c == x ? y : (c == y ? x : -1);
            if (other >= 0 && std::any_of(spans.begin(), spans.end(), [&](const Span& s) {
                    return s.choice == other && t >= s.a && t < s.b + 0.5;
                }))
                return true;
        }
        return false;
    };
    // Uma intervenção no instante t. `forced` ≥ 0 impõe a escolha (regras
    // "obrigatório", docs/REGRAS.md); −1 deixa a leitura do momento decidir.
    // Devolve false se nada pôde ser feito (todas as escolhas proibidas).
    const auto intervene = [&](const double t, const int forced) -> bool {
        const auto local = summarize(frames, onsets, t, std::min(score.duration, t + 6.0));
        const float center = 48.0f + 30.0f * local.brightness;
        char text[400];

        // Peso de cada escolha = exp(afinidade / temperatura). Com pouca
        // imprevisibilidade o olho faz o que o momento pede; com 1 o sorteio
        // quase se abre de novo (o inusitado continua possível). Repetir a
        // escolha anterior pesa menos, para o olho não virar tique.
        const Reading reading = readMoment(score, frames, onsets, t, noveltyAt);
        const auto aff = affinities(reading);
        const double temperature = 0.08 + 0.5 * u;
        std::array<double, kChoices> weight {};
        double total = 0.0;
        for (std::size_t c = 0; c < weight.size(); ++c)
        {
            weight[c] = std::exp(static_cast<double>(aff[c]) / temperature)
                        * (static_cast<int>(c) == lastChoice ? 0.35 : 1.0)
                        * static_cast<double>(opt.tendencies.eye[c]);  // a escuta do autor
            total += weight[c];
        }
        // regras: uma escolha proibida pesa 0, salvo exceção (a chance de o
        // olho quebrar a regra, marcada na forma). Sem regras, nenhum sorteio
        // a mais: a composição fica idêntica.
        if (rules.anyEyeForbidden())
            for (std::size_t c = 0; c < weight.size(); ++c)
                if (rules.noEye[c] && !(rules.exception > 0.0f && rng.chance(rules.exception)))
                {
                    total -= weight[c];
                    weight[c] = 0.0;
                }
        // condicionado e incompatível: sem exceção (são o próprio contexto)
        for (std::size_t c = 0; c < weight.size(); ++c)
            if (weight[c] > 0.0
                && (!holds(rules.when[c], reading) || (!rules.incompatible.empty() && clashes(static_cast<int>(c), t))))
            {
                total -= weight[c];
                weight[c] = 0.0;
            }
        int choice = forced;
        if (choice < 0)
        {
            if (total <= 0.0)
                return false;
            choice = kChoices - 1;
            double pick = rng.uniform() * total;
            for (std::size_t c = 0; c < weight.size(); ++c)
            {
                pick -= weight[c];
                if (pick < 0.0)
                {
                    choice = static_cast<int>(c);
                    break;
                }
            }
        }
        lastChoice = choice;
        used[static_cast<std::size_t>(choice)] = true;
        std::string whyText = forced >= 0 ? std::string("obrigatório pelas regras") : std::string(reasonOf(choice));
        if (forced < 0 && rules.noEye[static_cast<std::size_t>(choice)])
            whyText += " · exceção à regra";
        const char* why = whyText.c_str();
        // às vezes (mais com imprevisibilidade alta) o olho contraria a leitura
        // na direção do gesto, não na escolha
        const bool contrary = rng.chance(0.5 * u);

        // para onde o gesto deriva: o registro dos próximos segundos, desviado
        // pela imprevisibilidade (antes era só sorte)
        const auto next = summarize(frames, onsets, std::min(score.duration, t + 4.0), std::min(score.duration, t + 10.0));
        const float nextCenter = hasTimbre(next) ? 48.0f + 30.0f * next.brightness : center;
        const std::vector<float>& chord = reading.peakChord;

        if (choice < kObjects)
        {
            // ---- objeto sonoro --------------------------------------------
            Gesture g;
            g.kind = static_cast<GestureKind>(choice);
            g.time = t;
            g.duration = std::min(score.duration - t, 2.0 + rng.uniform() * 10.0 * (0.5 + u));
            g.level = std::clamp(0.25f + 0.45f * local.energy * rng.between(0.6f, 1.3f), 0.1f, 0.9f);
            g.seed = rng.next();
            g.pan = rng.between(-0.8f, 0.8f);
            g.panEnd = rng.between(-0.8f, 0.8f);
            const float width = rng.between(3.0f, 18.0f);
            const float drift = (nextCenter - center) * (1.0f - u) + rng.between(-18.0f, 18.0f) * u;
            // abre quando a energia sobe, fecha quando cai (ou o contrário, se contrário)
            const bool opens = (reading.trend >= 0.0f) != contrary;
            switch (g.kind)
            {
                case GestureKind::cloud:
                    g.lo = center - width; g.hi = center + width;
                    g.loEnd = g.lo + drift; g.hiEnd = g.hi + drift * 0.5f;
                    g.density = rng.between(6.0f, 25.0f); g.densityEnd = rng.between(6.0f, 60.0f);
                    break;
                case GestureKind::glissMass:  // fechado → aberto na subida; aberto → fechado na queda
                    if (opens) { g.lo = center - 1.0f; g.hi = center + 1.0f; g.loEnd = center - 24.0f; g.hiEnd = center + 24.0f; }
                    else { g.lo = center - 24.0f; g.hi = center + 24.0f; g.loEnd = center + drift - 1.0f; g.hiEnd = center + drift + 1.0f; }
                    g.density = g.densityEnd = static_cast<float>(rng.range(6, 24));
                    break;
                case GestureKind::noiseBand:
                    g.lo = center - width * 0.5f; g.hi = center + width * 0.5f;
                    g.loEnd = g.lo + drift; g.hiEnd = g.hi + drift + rng.between(-6.0f, 12.0f);
                    break;
                case GestureKind::cluster:
                    g.lo = center - 3.0f; g.hi = center + 3.0f; g.loEnd = g.lo + drift * 0.3f; g.hiEnd = g.hi + drift * 0.3f;
                    g.density = g.densityEnd = static_cast<float>(rng.range(8, 20));
                    break;
                case GestureKind::iteration:  // acelera na subida, desacelera na queda
                    g.lo = g.loEnd = center; g.hi = g.hiEnd = center + 12.0f;
                    if (opens) { g.density = 2.0f; g.densityEnd = rng.between(12.0f, 30.0f); }
                    else { g.density = rng.between(12.0f, 30.0f); g.densityEnd = 1.5f; }
                    break;
                case GestureKind::drone:
                    g.lo = g.hi = g.loEnd = g.hiEnd = 28.0f + static_cast<float>(rng.range(0, 11));
                    g.level *= 0.7f;
                    break;
                case GestureKind::freeze:  // o acorde que acabou de soar; sem ele, o campo
                    g.pitches = chord.size() >= 2 ? chord : chordAround(frames, t, t + g.duration, center, rng.range(3, 6));
                    break;
                case GestureKind::body:  // o corpo medido da referência neste instante
                {
                    if (const auto* sp = spectralAt(score, t))
                    {
                        g.ratios = sp->ratios;
                        g.weights = sp->weights;
                    }
                    // a fundamental: a nota mais grave do acorde recente, ou
                    // uma oitava abaixo do registro do momento
                    float root = center - 12.0f;
                    if (!chord.empty())
                        root = *std::min_element(chord.begin(), chord.end());
                    while (root > 60.0f) root -= 12.0f;
                    while (root < 36.0f) root += 12.0f;
                    g.pitches = { root };
                    g.level = std::clamp(g.level, 0.25f, 0.7f);
                    break;
                }
                default:  // os efeitos de cinema: o olho não os sorteia (FilmEffects.h)
                    break;
            }
            score.gestures.push_back(g);

            // a consequência do gesto, se ele ficar
            std::string stays;
            const double after = g.time + g.duration;
            const float shift = std::clamp(std::round(0.5f * ((g.loEnd + g.hiEnd) - (g.lo + g.hi))), -12.0f, 12.0f);
            const bool moves = g.kind == GestureKind::cloud || g.kind == GestureKind::glissMass
                               || g.kind == GestureKind::noiseBand || g.kind == GestureKind::cluster;
            if ((!moves || std::abs(shift) >= 2.0f) && after < score.duration - 1.0 && willLast())
            {
                char buf[96];
                switch (g.kind)
                {
                    case GestureKind::freeze:
                        snapFrom(after, g.pitches);
                        stays = "o acorde vira o campo";
                        break;
                    case GestureKind::body:  // a melodia passa a soar com o corpo da referência
                        if (rules.noTimbre[static_cast<std::size_t>(Timbre::spectral)])
                            break;  // proibido pelas regras: o corpo soa, mas não fica
                        for (auto& x : notes)
                            if (x.voice != 1 && x.voice != 4 && x.time >= after)
                                x.timbre = Timbre::spectral;
                        stays = "o corpo da referência vira o timbre";
                        break;
                    case GestureKind::drone:
                        for (auto& x : notes)
                            if (x.voice == 1 && x.time >= after)
                                x.pitch = snapToChord(x.pitch, { g.lo });
                        stays = "o drone vira pedal do baixo";
                        break;
                    case GestureKind::iteration:
                        if (g.densityEnd > g.density) { thickenSpan(after, score.duration + 1.0); stays = "a textura segue acelerada"; }
                        else { thinSpan(after, score.duration + 1.0); stays = "a textura segue rarefeita"; }
                        break;
                    default:  // nuvem, glissandos, ruído, cluster: a melodia vai junto
                        for (auto& x : notes)
                            if (x.voice != 1 && x.voice != 4 && x.time >= after)
                                x.pitch = std::clamp(x.pitch + shift, 24.0f, 108.0f);
                        std::snprintf(buf, sizeof buf, "a música segue para onde o gesto levou (%+.0f semitons)", shift);
                        stays = buf;
                        break;
                }
            }
            std::snprintf(text, sizeof text, "Terceiro Olho · objeto sonoro: %s por %.1f s%s%s · leitura: %s",
                          gestureName(g.kind), g.duration, stays.empty() ? "" : " · fica: ", stays.c_str(), why);
            score.marks.push_back({ t, text });
            spans.push_back({ choice, t, t + g.duration });
        }
        else
        {
            // ---- regra oculta ---------------------------------------------
            const auto rule = static_cast<Rule>(choice - kObjects);
            const double d = rule == Rule::silence ? 0.8 + rng.uniform() * 2.5
                                                   : std::min(score.duration - t, 3.0 + rng.uniform() * 15.0 * u);
            const double a = t, b = t + d;
            std::string detail;
            // se a regra FICA, o trecho vai até o fim da peça (silêncio e tempo
            // deformado são sempre momentâneos)
            const bool canLast = rule != Rule::silence && rule != Rule::warp && b < score.duration - 1.0;
            const bool stays = canLast && willLast();
            const double bb = stays ? score.duration + 1.0 : b;
            const char* staysText = "";
            switch (rule)
            {
                case Rule::mirror:
                {
                    float sum = 0.0f;
                    int n = 0;
                    for (const auto& x : notes)
                        if (x.voice == 0 && inSpan(x.time, a, b)) { sum += x.pitch; ++n; }
                    const float axis = n ? sum / static_cast<float>(n) : center;
                    for (auto& x : notes)
                        if (x.voice != 1 && x.voice != 4 && inSpan(x.time, a, bb))
                            x.pitch = std::clamp(2.0f * axis - x.pitch, 24.0f, 108.0f);
                    staysText = "o registro fica invertido";
                    break;
                }
                case Rule::warp:
                {
                    // mais rápido quando a energia sobe, mais lento quando cai
                    const bool faster = (reading.trend >= 0.0f) != contrary;
                    const double ratio = faster ? 2.0 / 3.0 : 3.0 / 2.0;
                    detail = faster ? " (3:2, mais rápido)" : " (2:3, mais lento)";
                    std::vector<Note> kept;
                    for (auto x : notes)
                    {
                        if (x.voice != 1 && inSpan(x.time, a, b))
                        {
                            x.time = a + (x.time - a) * ratio;
                            x.duration *= ratio;
                            if (x.time >= b)
                                continue;
                        }
                        kept.push_back(x);
                    }
                    notes.swap(kept);
                    break;
                }
                case Rule::mutate:
                {
                    static const float shifts[] = { 0.5f, -0.5f, 1.0f, -1.0f, 6.0f, 0.25f };
                    const float sh = shifts[static_cast<std::size_t>(rng.range(0, 5))];
                    char buf[48];
                    std::snprintf(buf, sizeof buf, " (%+.2f semitom)", sh);
                    detail = buf;
                    for (auto& x : notes)
                        if (x.voice != 1 && x.voice != 4 && inSpan(x.time, a, bb))
                            x.pitch += sh;
                    staysText = "a escala fica mutada";
                    break;
                }
                case Rule::silence:
                {
                    std::vector<Note> kept;
                    for (auto x : notes)
                    {
                        if (inSpan(x.time, a, b))
                            continue;
                        if (x.time < a && x.time + x.duration > a)
                            x.duration = a - x.time;  // corta o que soava
                        kept.push_back(x);
                    }
                    notes.swap(kept);
                    for (auto& g : score.gestures)
                        if (g.time < a && g.time + g.duration > a)
                            g.duration = a - g.time;
                    silences.push_back({ a, b });
                    break;
                }
                case Rule::echo:
                {
                    const double delay = 0.2 + rng.uniform() * 0.6;
                    // campo tonal: quinta, quarta ou oitava; ruidoso ou contrário: o
                    // quarto de tom e a terça entram no sorteio
                    static const float consonant[] = { 7.0f, -5.0f, 12.0f, -12.0f };
                    static const float any[] = { 7.0f, -5.0f, 12.0f, -12.0f, 4.0f, 0.5f };
                    const float iv = (reading.noisiness < 0.4f && !contrary)
                                         ? consonant[static_cast<std::size_t>(rng.range(0, 3))]
                                         : any[static_cast<std::size_t>(rng.range(0, 5))];
                    auto other = static_cast<Timbre>(rng.range(0, kTimbreCount - 1));
                    for (int tries = 0; tries < 16 && rules.noTimbre[static_cast<std::size_t>(other)]; ++tries)
                        other = static_cast<Timbre>(rng.range(0, kTimbreCount - 1));
                    std::vector<Note> add;
                    for (const auto& x : notes)
                        if (x.voice == 0 && inSpan(x.time, a, bb))
                        {
                            Note e = x;
                            e.time += delay;
                            e.pitch = std::clamp(e.pitch + iv, 24.0f, 108.0f);
                            e.velocity *= 0.6f;
                            e.voice = 2;
                            e.timbre = other;
                            if (e.time < score.duration)
                                add.push_back(e);
                        }
                    notes.insert(notes.end(), add.begin(), add.end());
                    char buf[64];
                    std::snprintf(buf, sizeof buf, " (%.2f s, %+.1f semitons)", delay, iv);
                    detail = buf;
                    staysText = "o cânone continua até o fim";
                    break;
                }
                case Rule::retimbre:
                {
                    // um timbre diferente do que vinha soando
                    int current = -1;
                    for (const auto& x : notes)
                        if (x.voice == 0 && x.time < a) current = static_cast<int>(x.timbre);
                    int tb = rng.range(0, kTimbreCount - 1);
                    if (tb == current) tb = (tb + 1 + rng.range(0, kTimbreCount - 2)) % kTimbreCount;
                    for (int tries = 0; tries < 16 && rules.noTimbre[static_cast<std::size_t>(tb)]; ++tries)
                        tb = rng.range(0, kTimbreCount - 1);
                    for (auto& x : notes)
                        if (x.voice != 1 && x.voice != 4 && inSpan(x.time, a, bb))
                            x.timbre = static_cast<Timbre>(tb);
                    staysText = "o timbre novo fica";
                    break;
                }
                case Rule::thicken:
                    thickenSpan(a, bb);
                    staysText = "a textura fica densa";
                    break;
                case Rule::thin:
                    thinSpan(a, bb);
                    staysText = "a textura fica rarefeita";
                    break;
                case Rule::freeze:
                {
                    std::vector<Note> kept;
                    for (const auto& x : notes)
                        if (!(x.voice != 1 && inSpan(x.time, a, b)))
                            kept.push_back(x);
                    notes.swap(kept);
                    Gesture g;
                    g.kind = GestureKind::freeze;
                    g.time = a;
                    g.duration = d;
                    g.level = 0.35f + 0.3f * local.energy;
                    // o acorde que acabou de soar mais forte; sem ele, o campo
                    g.pitches = chord.size() >= 2 ? chord : chordAround(frames, a, b, center, rng.range(3, 5));
                    g.seed = rng.next();
                    score.gestures.push_back(g);
                    if (stays)
                        snapFrom(b, g.pitches);
                    staysText = "o acorde vira o campo";
                    break;
                }
            }
            std::snprintf(text, sizeof text, "Terceiro Olho · regra oculta: %s%s por %.1f s%s%s · leitura: %s",
                          ruleName(rule), detail.c_str(), d, stays ? " · fica: " : "", stays ? staysText : "", why);
            score.marks.push_back({ t, text });
            spans.push_back({ choice, t, t + d });
        }
        return true;
    };

    double cursor = 2.0 + rng.uniform() * 4.0;
    while (cursor < score.duration - 2.0)
    {
        const double gap = -std::log(1.0 - rng.uniform() * 0.999) * (5.0 / u);
        // a dramaturgia (Options::tensionAt): mais intervenções onde a peça
        // está tensa, menos nos vales (0,5× a 1,5×); nenhuma na respiração
        const double tension = opt.tensionAt ? 0.5 + std::clamp(static_cast<double>(opt.tensionAt(cursor)), 0.0, 1.0) : 1.0;
        const bool inBreath = cursor >= opt.breath.first - 1.0 && cursor < opt.breath.second && opt.breath.second > 0.0;
        if (inBreath || !rng.chance(std::min(1.0, (0.4 + 0.6 * noveltyAt(cursor)) * tension)))
        {
            cursor += gap;
            continue;
        }
        intervene(cursor, -1);
        cursor += std::max(gap, 1.5);
    }

    // regras "obrigatório": o que não aconteceu acontece no momento de maior
    // novidade que ainda não tem intervenção por perto
    for (int c = 0; c < kChoices; ++c)
    {
        if (!rules.mustEye[static_cast<std::size_t>(c)] || used[static_cast<std::size_t>(c)])
            continue;
        double best = -1.0, at = -1.0;
        for (double x = 2.0; x < score.duration - 2.0; x += 0.5)
        {
            const bool near = std::any_of(score.marks.begin(), score.marks.end(), [&](const Mark& m) {
                return std::abs(m.time - x) < 1.5 && m.text.rfind("Terceiro Olho", 0) == 0;
            });
            const int cc = c;
            const bool allowed = (rules.when[static_cast<std::size_t>(cc)] == Condition::none
                                  || holds(rules.when[static_cast<std::size_t>(cc)], readMoment(score, frames, onsets, x, noveltyAt)))
                                 && !clashes(cc, x);
            if (!near && allowed && noveltyAt(x) > best)
            {
                best = noveltyAt(x);
                at = x;
            }
        }
        if (at >= 0.0)
            intervene(at, c);
    }

    // o silêncio vale para tudo o que vier depois dentro dele, inclusive o
    // que outras regras moveram para lá
    std::vector<Note> kept;
    for (const auto& x : score.notes)
        if (std::none_of(silences.begin(), silences.end(), [&](const auto& s) { return inSpan(x.time, s.first, s.second); }))
            kept.push_back(x);
    score.notes.swap(kept);
    std::vector<Gesture> gk;
    for (const auto& g : score.gestures)
        if (std::none_of(silences.begin(), silences.end(), [&](const auto& s) { return inSpan(g.time, s.first, s.second); }))
            gk.push_back(g);
    score.gestures.swap(gk);

    std::sort(score.notes.begin(), score.notes.end(), [](const Note& x, const Note& y) {
        return x.time < y.time || (x.time == y.time && x.voice < y.voice);
    });
    std::stable_sort(score.marks.begin(), score.marks.end(), [](const Mark& x, const Mark& y) { return x.time < y.time; });
}

} // namespace trioio::composition
