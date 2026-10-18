// As regras do autor em texto simples (docs/REGRAS.md). Uma por linha;
// '#' começa comentário; acentos opcionais nas palavras-chave.
//
//   proibido timbre <família>      proibido olho <escolha>      proibido sala <sala>
//   obrigatório olho <escolha>
//   máximo salto <semitons>        âmbito <grave> <agudo>       (MIDI)
//   mínimo memória <passagens>     exceção <0..1>
//   condicionado olho <escolha> se <condição>
//   incompatível olho <escolha> e <escolha>

#include "Composer.h"
#include "ThirdEye.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace trioio::composition
{
namespace
{
std::string lower(std::string s)
{
    for (auto& c : s)
        if (static_cast<unsigned char>(c) < 128)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string trim(const std::string& s)
{
    const auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos)
        return {};
    const auto b = s.find_last_not_of(" \t\r");
    return s.substr(a, b - a + 1);
}

// palavras-chave com e sem acento
bool is(const std::string& w, std::initializer_list<const char*> forms)
{
    return std::any_of(forms.begin(), forms.end(), [&](const char* f) { return w == f; });
}

// o resto da linha é um nome: compara sem caixa. Aceita, nesta ordem, o
// nome exato, o início do nome ("SPACE" para "SPACE (Rasgo Modular)",
// "chip" para "chip AY") ou um trecho dele ("trioio" para "sala do Trioio"),
// desde que só um nome sirva
template <typename NameOf>
int findName(const std::string& wanted, int count, NameOf nameOf)
{
    const auto w = lower(wanted);
    if (w.empty())
        return -1;
    int prefixHit = -1, prefixCount = 0, innerHit = -1, innerCount = 0;
    for (int k = 0; k < count; ++k)
    {
        const auto n = lower(nameOf(k));
        if (n == w)
            return k;
        if (n.rfind(w, 0) == 0)
        {
            prefixHit = k;
            ++prefixCount;
        }
        else if (n.find(w) != std::string::npos)
        {
            innerHit = k;
            ++innerCount;
        }
    }
    if (prefixCount == 1)
        return prefixHit;
    return prefixCount == 0 && innerCount == 1 ? innerHit : -1;
}
} // namespace

const char* conditionName(Condition c)
{
    switch (c)
    {
        case Condition::none: return "";
        case Condition::energyHigh: return "energia alta";
        case Condition::energyLow: return "energia baixa";
        case Condition::energyRising: return "energia subindo";
        case Condition::energyFalling: return "energia caindo";
        case Condition::noisy: return "ruído";
        case Condition::dense: return "denso";
        case Condition::sparse: return "rarefeito";
        case Condition::changeAhead: return "mudança";
        case Condition::stasis: return "campo parado";
        case Condition::strongChord: return "acorde forte";
    }
    return "?";
}

namespace
{
// "ruido" e "mudanca" sem acento também valem
Condition parseCondition(const std::string& s)
{
    const auto w = lower(trim(s));
    for (int k = 1; k < kConditions; ++k)
        if (w == lower(conditionName(static_cast<Condition>(k))))
            return static_cast<Condition>(k);
    if (w == "ruido") return Condition::noisy;
    if (w == "mudanca") return Condition::changeAhead;
    return Condition::none;
}

int eyeByName(const std::string& s)
{
    return findName(trim(s), kEyeChoices, [](int i) { return std::string(eyeChoiceName(i)); });
}
} // namespace

Constraints parseConstraints(const std::string& text, std::vector<std::string>& errors)
{
    Constraints c;
    std::istringstream in(text);
    std::string raw;
    int lineNo = 0;
    while (std::getline(in, raw))
    {
        ++lineNo;
        const auto hash = raw.find('#');
        const auto line = trim(hash == std::string::npos ? raw : raw.substr(0, hash));
        if (line.empty())
            continue;
        std::istringstream ws(line);
        std::string a, b;
        ws >> a >> b;
        a = lower(a);
        b = lower(b);
        std::string rest;
        std::getline(ws, rest);
        rest = trim(rest);
        const auto fail = [&](const std::string& why) {
            errors.push_back("linha " + std::to_string(lineNo) + ": " + why + " (\"" + line + "\")");
        };
        bool ok = true;
        if (is(a, { "proibido", "obrigatório", "obrigatorio" }))
        {
            const bool must = a != "proibido";
            if (is(b, { "timbre" }) && !must)
            {
                const int k = findName(rest, kTimbreCount, [](int i) { return std::string(timbreName(static_cast<Timbre>(i))); });
                if (k < 0) { fail("família de timbre desconhecida"); ok = false; }
                else c.noTimbre[static_cast<std::size_t>(k)] = true;
            }
            else if (is(b, { "olho" }))
            {
                const int k = findName(rest, kEyeChoices, [](int i) { return std::string(eyeChoiceName(i)); });
                if (k < 0) { fail("escolha do Terceiro Olho desconhecida"); ok = false; }
                else (must ? c.mustEye : c.noEye)[static_cast<std::size_t>(k)] = true;
            }
            else if (is(b, { "sala" }) && !must)
            {
                const int k = findName(rest, kRooms, [](int i) { return std::string(roomName(static_cast<Room>(i))); });
                if (k < 0) { fail("sala desconhecida"); ok = false; }
                else c.noRoom[static_cast<std::size_t>(k)] = true;
            }
            else { fail(must ? "obrigatório só vale para 'olho'" : "use timbre, olho ou sala"); ok = false; }
        }
        else if (is(a, { "condicionado" }) && is(b, { "olho" }))
        {
            const auto se = rest.rfind(" se ");
            const int k = se == std::string::npos ? -1 : eyeByName(rest.substr(0, se));
            const Condition cond = se == std::string::npos ? Condition::none : parseCondition(rest.substr(se + 4));
            if (se == std::string::npos) { fail("use: condicionado olho <escolha> se <condição>"); ok = false; }
            else if (k < 0) { fail("escolha do Terceiro Olho desconhecida"); ok = false; }
            else if (cond == Condition::none) { fail("condição desconhecida (energia alta, energia baixa, energia subindo, energia caindo, ruído, denso, rarefeito, mudança, campo parado, acorde forte)"); ok = false; }
            else c.when[static_cast<std::size_t>(k)] = cond;
        }
        else if (is(a, { "incompatível", "incompativel" }) && is(b, { "olho" }))
        {
            const auto e = rest.find(" e ");
            const int x = e == std::string::npos ? -1 : eyeByName(rest.substr(0, e));
            const int y = e == std::string::npos ? -1 : eyeByName(rest.substr(e + 3));
            if (x < 0 || y < 0 || x == y) { fail("use: incompatível olho <escolha> e <outra escolha>"); ok = false; }
            else c.incompatible.push_back({ x, y });
        }
        else if (is(a, { "máximo", "maximo" }) && is(b, { "salto" }))
        {
            try { c.maxLeap = std::max(1, std::stoi(rest)); }
            catch (...) { fail("salto máximo precisa de um número de semitons"); ok = false; }
        }
        else if (is(a, { "âmbito", "ambito" }))
        {
            std::istringstream ns(b + " " + rest);
            float lo = 0, hi = 0;
            if (!(ns >> lo >> hi) || hi - lo < 6.0f) { fail("âmbito precisa de grave e agudo em MIDI, com ao menos 6 semitons"); ok = false; }
            else { c.lowest = lo; c.highest = hi; }
        }
        else if (is(a, { "mínimo", "minimo" }) && is(b, { "memória", "memoria" }))
        {
            try { c.minMemory = std::clamp(std::stoi(rest), 0, 8); }
            catch (...) { fail("mínimo de memória precisa de um número de passagens"); ok = false; }
        }
        else if (is(a, { "exceção", "excecao" }))
        {
            try { c.exception = std::clamp(std::stof(b), 0.0f, 1.0f); }
            catch (...) { fail("exceção precisa de uma chance entre 0 e 1"); ok = false; }
        }
        else { fail("regra desconhecida"); ok = false; }
        if (ok)
            c.lines.push_back(line);
    }
    return c;
}

} // namespace trioio::composition
