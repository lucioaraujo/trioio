#include "ListeningStore.h"

#include "ManifestJson.h"
#include "Response.h"

#include <juce_core/juce_core.h>

namespace trioio::library
{
using namespace trioio::composition;

namespace
{
juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.data(), static_cast<int>(s.size())); }

bool writeAtomically(const juce::File& target, const juce::String& text)
{
    target.getParentDirectory().createDirectory();
    juce::TemporaryFile tmp(target);
    return tmp.getFile().replaceWithText(text, false, false, "\n") && tmp.overwriteTargetFileWithTemporary();
}

juce::var toVar(const Judgement& j)
{
    auto v = juce::var(new juce::DynamicObject());
    auto* o = v.getDynamicObject();
    o->setProperty("derivation", u8(j.derivationId));
    o->setProperty("verdict", verdictName(j.verdict));
    o->setProperty("note", u8(j.note));
    o->setProperty("when", u8(j.when));
    juce::Array<juce::var> timbres, eye;
    for (const auto& [fam, c] : j.timbres)
    {
        auto t = juce::var(new juce::DynamicObject());
        t.getDynamicObject()->setProperty("family", fam);
        t.getDynamicObject()->setProperty("name", timbreName(static_cast<Timbre>(fam)));
        t.getDynamicObject()->setProperty("notes", c);
        timbres.add(t);
    }
    for (int c : j.eyeChoices)
        eye.add(c);
    o->setProperty("timbres", timbres);
    o->setProperty("eyeChoices", eye);
    o->setProperty("room", j.room);
    if (j.room >= 0)
        o->setProperty("roomName", roomName(static_cast<Room>(j.room)));
    if (j.hasSpan())
    {
        juce::Array<juce::var> span;
        span.add(j.spanStart);
        span.add(j.spanEnd);
        o->setProperty("span", span);
        if (!j.section.empty())
            o->setProperty("section", u8(j.section));
        o->setProperty("roomWeight", j.roomWeight);
    }
    return v;
}

bool fromVar(const juce::var& v, Judgement& j)
{
    if (!v.isObject())
        return false;
    j.derivationId = v["derivation"].toString().toStdString();
    if (!parseVerdict(v["verdict"].toString().toStdString(), j.verdict))
        return false;
    j.note = v["note"].toString().toStdString();
    j.when = v["when"].toString().toStdString();
    if (const auto* a = v["timbres"].getArray())
        for (const auto& t : *a)
            j.timbres.push_back({ static_cast<int>(t["family"]), static_cast<double>(t["notes"]) });
    if (const auto* a = v["eyeChoices"].getArray())
        for (const auto& c : *a)
            j.eyeChoices.push_back(static_cast<int>(c));
    j.room = v.hasProperty("room") ? static_cast<int>(v["room"]) : -1;
    if (const auto* a = v["span"].getArray(); a != nullptr && a->size() == 2)
    {
        j.spanStart = static_cast<double>((*a)[0]);
        j.spanEnd = static_cast<double>((*a)[1]);
        j.section = v["section"].toString().toStdString();
        j.roomWeight = v.hasProperty("roomWeight") ? static_cast<double>(v["roomWeight"]) : 1.0;
    }
    return true;
}
} // namespace

std::string listeningPath(const std::string& dataRoot)
{
    return juce::File(u8(dataRoot)).getChildFile("escuta.json").getFullPathName().toStdString();
}

std::vector<Judgement> loadJudgements(const std::string& dataRoot)
{
    std::vector<Judgement> out;
    const juce::File f(u8(listeningPath(dataRoot)));
    if (!f.existsAsFile())
        return out;
    const auto root = juce::JSON::parse(f.loadFileAsString());
    if (const auto* a = root["judgements"].getArray())
        for (const auto& v : *a)
        {
            Judgement j;
            if (fromVar(v, j))
                out.push_back(std::move(j));
        }
    return out;
}

JudgementResult recordJudgement(const std::string& dataRoot, const std::string& derivationId, Verdict verdict,
                                const std::string& note, const std::string& nowArg, const std::string& spanArg,
                                const std::string& sectionArg)
{
    JudgementResult r;
    const std::string now = nowArg.empty() ? juce::Time::getCurrentTime().toISO8601(true).toStdString() : nowArg;
    // 1. a derivação e a sua sessão
    const juce::File mf = juce::File(u8(derivationManifestDir(dataRoot))).getChildFile(u8(derivationId) + ".json");
    if (!mf.existsAsFile())
    {
        r.problems.push_back("derivação não encontrada: " + derivationId + " (em " + derivationManifestDir(dataRoot) + ")");
        return r;
    }
    const auto lido = fromJson(mf.loadFileAsString().toStdString());
    if (!lido.manifest || !lido.manifest->derivation)
    {
        r.problems.push_back("manifesto ilegível ou sem derivação: " + mf.getFullPathName().toStdString());
        return r;
    }
    const auto& d = *lido.manifest->derivation;
    const auto decisions = juce::File(u8(dataRoot)).getChildFile("sessoes").getChildFile(u8(d.sessionId))
                               .getChildFile(u8(d.decisionsRef.empty() ? std::string("decisoes.json") : d.decisionsRef));
    if (!decisions.existsAsFile())
    {
        r.problems.push_back("decisões da sessão não encontradas: " + decisions.getFullPathName().toStdString());
        return r;
    }
    // 2. o que a resposta usou
    const auto dj = juce::JSON::parse(decisions.loadFileAsString());
    std::vector<std::string> marks;
    std::vector<int> timbres;
    std::vector<MarkAt> marksAt;
    std::vector<NoteAt> notesAt;
    if (const auto* a = dj["form"].getArray())
        for (const auto& m : *a)
        {
            marks.push_back(m["text"].toString().toStdString());
            marksAt.push_back({ static_cast<double>(m["time"]), marks.back() });
        }
    if (const auto* a = dj["notes"].getArray())
        for (const auto& n : *a)
        {
            timbres.push_back(static_cast<int>(n["timbre"]));
            notesAt.push_back({ static_cast<double>(n["time"]), static_cast<double>(n["duration"]), timbres.back() });
        }
    Judgement base;
    base.derivationId = derivationId;
    base.verdict = verdict;
    base.note = note;
    base.when = now;
    // 2b. a resposta inteira, um trecho ou as seções pedidas
    std::vector<Judgement> made;
    if (!spanArg.empty() && !sectionArg.empty())
    {
        r.problems.push_back("use --trecho ou --secao, não os dois");
        return r;
    }
    const double end = responseEnd(notesAt);
    if (!spanArg.empty() || !sectionArg.empty())
    {
        if (notesAt.empty())
        {
            r.problems.push_back("a resposta não tem partitura gravada (motor de energia): só pode ser julgada inteira");
            return r;
        }
        std::vector<std::pair<double, double>> spans;
        if (!spanArg.empty())
        {
            double a = 0.0, b = 0.0;
            if (!parseSpan(spanArg, end, a, b))
            {
                r.problems.push_back("trecho ilegível ou fora da resposta: \"" + spanArg + "\" (ex.: 0:00-1:00, 1:41-fim; a resposta tem "
                                     + std::to_string(static_cast<int>(end)) + " s)");
                return r;
            }
            spans.push_back({ a, b });
        }
        else
        {
            spans = sectionSpans(marksAt, end, sectionArg);
            if (spans.empty())
            {
                r.problems.push_back("seção \"" + sectionArg + "\" não existe nesta resposta (número da seção, ou letra na forma por repetição)");
                return r;
            }
        }
        for (const auto& [a, b] : spans)
        {
            Judgement j = base;
            describeSpan(marksAt, notesAt, a, b, j);
            j.section = sectionArg;
            made.push_back(std::move(j));
        }
    }
    else
    {
        Judgement j = base;
        describeResponse(marks, timbres, j);
        made.push_back(std::move(j));
    }
    // 3. acrescentar ao escuta.json
    auto history = loadJudgements(dataRoot);
    history.insert(history.end(), made.begin(), made.end());
    auto root = juce::var(new juce::DynamicObject());
    root.getDynamicObject()->setProperty("schema", "trioio.escuta/2");
    juce::Array<juce::var> arr;
    for (const auto& h : history)
        arr.add(toVar(h));
    root.getDynamicObject()->setProperty("judgements", arr);
    if (!writeAtomically(juce::File(u8(listeningPath(dataRoot))), juce::JSON::toString(root)))
    {
        r.problems.push_back("falha ao gravar " + listeningPath(dataRoot));
        return r;
    }
    r.written = true;
    r.judgement = made.front();
    r.judgements = std::move(made);
    return r;
}

} // namespace trioio::library
