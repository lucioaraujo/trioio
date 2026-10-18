#include "Listening.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>

namespace trioio::composition
{

const char* verdictName(Verdict v)
{
    switch (v)
    {
        case Verdict::accept: return "aceitar";
        case Verdict::reject: return "rejeitar";
        case Verdict::transform: return "transformar";
    }
    return "?";
}

bool parseVerdict(const std::string& s, Verdict& out)
{
    if (s == "aceitar") { out = Verdict::accept; return true; }
    if (s == "rejeitar") { out = Verdict::reject; return true; }
    if (s == "transformar") { out = Verdict::transform; return true; }
    return false;
}

int roomFromMark(const std::string& mark)
{
    const std::string prefix = "sala: ";
    if (mark.rfind(prefix, 0) != 0)
        return -1;
    const auto name = mark.substr(prefix.size());
    for (int r = 0; r < kRooms; ++r)
        if (name == roomName(static_cast<Room>(r)))
            return r;
    return -1;
}

void describeResponse(const std::vector<std::string>& formMarks, const std::vector<int>& noteTimbres,
                      Judgement& out)
{
    std::array<double, kTimbreCount> count {};
    for (int t : noteTimbres)
        if (t >= 0 && t < kTimbreCount)
            count[static_cast<std::size_t>(t)] += 1.0;
    out.timbres.clear();
    for (int t = 0; t < kTimbreCount; ++t)
        if (count[static_cast<std::size_t>(t)] > 0.0)
            out.timbres.push_back({ t, count[static_cast<std::size_t>(t)] });
    out.eyeChoices.clear();
    out.room = -1;
    for (const auto& m : formMarks)
    {
        if (const int c = eyeChoiceFromMark(m); c >= 0)
            out.eyeChoices.push_back(c);
        if (const int r = roomFromMark(m); r >= 0)
            out.room = r;
    }
}

double responseEnd(const std::vector<NoteAt>& notes)
{
    double end = 0.0;
    for (const auto& n : notes)
        end = std::max(end, n.time + std::max(0.0, n.duration));
    return end;
}

void describeSpan(const std::vector<MarkAt>& marks, const std::vector<NoteAt>& notes, double a, double b,
                  Judgement& out)
{
    // cada nota conta 1, como no julgamento inteiro; a que atravessa a
    // fronteira conta pela fração que soa dentro (pesar por segundos daria
    // ao pad sustentado o trecho todo)
    std::array<double, kTimbreCount> secs {};
    for (const auto& n : notes)
        if (n.timbre >= 0 && n.timbre < kTimbreCount)
        {
            double part = 0.0;
            if (n.duration > 0.0)
                part = std::max(0.0, std::min(b, n.time + n.duration) - std::max(a, n.time)) / n.duration;
            else
                part = n.time >= a && n.time < b ? 1.0 : 0.0;
            if (part > 0.0)
                secs[static_cast<std::size_t>(n.timbre)] += part;
        }
    out.timbres.clear();
    for (int t = 0; t < kTimbreCount; ++t)
        if (secs[static_cast<std::size_t>(t)] > 0.0)
            out.timbres.push_back({ t, secs[static_cast<std::size_t>(t)] });
    out.eyeChoices.clear();
    out.room = -1;
    for (const auto& m : marks)
    {
        if (const int c = eyeChoiceFromMark(m.text); c >= 0 && m.time >= a && m.time < b)
            out.eyeChoices.push_back(c);
        if (const int r = roomFromMark(m.text); r >= 0)
            out.room = r;
    }
    const double end = responseEnd(notes);
    out.spanStart = a;
    out.spanEnd = b;
    out.roomWeight = end > 0.0 ? std::clamp((b - a) / end, 0.0, 1.0) : 1.0;
}

namespace
{
bool parseTime(std::string t, double end, double& out)
{
    while (!t.empty() && t.front() == ' ') t.erase(t.begin());
    while (!t.empty() && t.back() == ' ') t.pop_back();
    if (t == "fim")
    {
        out = end;
        return true;
    }
    try
    {
        std::size_t used = 0;
        const auto colon = t.find(':');
        if (colon != std::string::npos)
        {
            const double m = std::stod(t.substr(0, colon), &used);
            if (used != colon) return false;
            const auto rest = t.substr(colon + 1);
            const double sec = std::stod(rest, &used);
            if (used != rest.size() || sec >= 60.0) return false;
            out = m * 60.0 + sec;
        }
        else
        {
            out = std::stod(t, &used);
            if (used != t.size()) return false;
        }
    }
    catch (...)
    {
        return false;
    }
    return std::isfinite(out) && out >= 0.0;
}
} // namespace

bool parseSpan(const std::string& text, double end, double& a, double& b)
{
    const auto dash = text.find('-');
    if (dash == std::string::npos)
        return false;
    if (!parseTime(text.substr(0, dash), end, a) || !parseTime(text.substr(dash + 1), end, b))
        return false;
    b = std::min(b, end);
    return a < b;
}

std::vector<std::pair<double, double>> sectionSpans(const std::vector<MarkAt>& marks, double end,
                                                    const std::string& query)
{
    // marcas "seção N (início–fim s): PAPEL · …"; o papel começa por "A · "
    // ou "retorno de A" na forma por repetição
    struct Sec { int number; char letter; double start; };
    std::vector<Sec> secs;
    const std::string prefix = "seção ";
    for (const auto& m : marks)
    {
        if (m.text.rfind(prefix, 0) != 0)
            continue;
        Sec s { std::atoi(m.text.c_str() + prefix.size()), 0, m.time };
        if (const auto colon = m.text.find("): "); colon != std::string::npos)
        {
            const auto role = m.text.substr(colon + 3);
            const std::string ret = "retorno de ", dot = " · ";
            if (role.size() > dot.size() && role[0] >= 'A' && role[0] <= 'Z' && role.compare(1, dot.size(), dot) == 0)
                s.letter = role[0];
            else if (role.rfind(ret, 0) == 0 && role.size() > ret.size())
                s.letter = role[ret.size()];
        }
        secs.push_back(s);
    }
    std::vector<std::pair<double, double>> out;
    if (query.empty())
        return out;
    const bool byLetter = query.size() == 1 && std::isalpha(static_cast<unsigned char>(query[0]));
    const char letter = byLetter ? static_cast<char>(std::toupper(static_cast<unsigned char>(query[0]))) : 0;
    const int number = byLetter ? 0 : std::atoi(query.c_str());
    for (std::size_t i = 0; i < secs.size(); ++i)
        if (byLetter ? secs[i].letter == letter : secs[i].number == number && number > 0)
        {
            const double b = std::min(end, i + 1 < secs.size() ? secs[i + 1].start : end);
            if (secs[i].start < b)
                out.push_back({ secs[i].start, b });
        }
    return out;
}

// Cada julgamento soma ao placar do que a resposta usou: aceitar +1,
// transformar +0,3 (gostou do material, não do resultado), rejeitar −1.
// Uma família pesa pela fração das notas que tocou; cada escolha do olho e a
// sala pesam inteiras. O julgamento mais novo pesa 1, o anterior 0,85, e
// assim por diante: o instrumento esquece aos poucos. Tendência =
// exp(1,2 × placar), limitada a 0,5..2: puxa, nunca proíbe nem impõe.
Tendencies tendenciesFrom(const std::vector<Judgement>& history)
{
    Tendencies t;
    std::array<double, kTimbreCount> st {};
    std::array<double, kEyeChoices> se {};
    std::array<double, kRooms> sr {};
    const std::size_t n = history.size();
    // a idade conta atos: trechos da mesma resposta julgados no mesmo minuto
    // são um só ato e têm a mesma idade (julgamentos inteiros, um ato cada)
    std::vector<int> act(n, 0);
    for (std::size_t i = n; i-- > 0;)
    {
        if (i + 1 == n)
            continue;
        const auto& x = history[i];
        const auto& y = history[i + 1];
        const bool same = x.hasSpan() && y.hasSpan() && x.derivationId == y.derivationId
                       && x.when.substr(0, 16) == y.when.substr(0, 16);
        act[i] = act[i + 1] + (same ? 0 : 1);
    }
    for (std::size_t i = 0; i < n; ++i)
    {
        const auto& j = history[i];
        const double age = std::pow(0.85, static_cast<double>(act[i]));
        const double v = j.verdict == Verdict::accept ? 1.0 : (j.verdict == Verdict::reject ? -1.0 : 0.3);
        double total = 0.0;
        for (const auto& [fam, c] : j.timbres)
            total += c;
        for (const auto& [fam, c] : j.timbres)
            if (fam >= 0 && fam < kTimbreCount && total > 0.0)
                st[static_cast<std::size_t>(fam)] += age * v * c / total;
        for (int c : j.eyeChoices)
            if (c >= 0 && c < kEyeChoices)
                se[static_cast<std::size_t>(c)] += age * v;
        if (j.room >= 0 && j.room < kRooms)
            sr[static_cast<std::size_t>(j.room)] += age * v * std::clamp(j.roomWeight, 0.0, 1.0);
    }
    const auto toWeight = [](double score) {
        return static_cast<float>(std::clamp(std::exp(1.2 * score), 0.5, 2.0));
    };
    for (std::size_t k = 0; k < st.size(); ++k) t.timbre[k] = toWeight(st[k]);
    for (std::size_t k = 0; k < se.size(); ++k) t.eye[k] = toWeight(se[k]);
    for (std::size_t k = 0; k < sr.size(); ++k) t.room[k] = toWeight(sr[k]);
    t.judgements = static_cast<int>(n);
    return t;
}

} // namespace trioio::composition
