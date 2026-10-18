#include "Prompts.h"

#include "Ensemble.h"
#include "HarmonyWriting.h"
#include "LibraryImporter.h"
#include "ManifestJson.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cstdlib>

namespace trioio::library
{
namespace
{
using namespace composition;

juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.c_str()); }
std::string s8(const juce::var& v) { return v.toString().toStdString(); }

juce::var fieldsJson(const IntentFields& f)
{
    auto v = juce::var(new juce::DynamicObject());
    auto* o = v.getDynamicObject();
    o->setProperty("energy", f.energy);
    o->setProperty("density", f.density);
    o->setProperty("brightness", f.brightness);
    o->setProperty("harmonicity", f.harmonicity);
    o->setProperty("bpm", f.bpm);
    o->setProperty("mood", f.mood);
    o->setProperty("moodName", f.mood >= 0 ? u8(intentionName(static_cast<Intention>(f.mood))) : juce::String());
    o->setProperty("ensemble", f.ensemble);
    o->setProperty("ensembleName", f.ensemble >= 0 ? u8(ensembleName(static_cast<Ensemble>(f.ensemble))) : juce::String());
    o->setProperty("harmonyLanguage", f.harmonyLanguage);
    o->setProperty("voice", f.voice);
    // os campos de 2026-10-18 (antes eles se perdiam ao regravar o prompt)
    o->setProperty("rockStyle", f.rockStyle);
    o->setProperty("narrative", f.narrative);
    o->setProperty("dance", f.dance);
    o->setProperty("freedom", f.freedom);
    o->setProperty("text", u8(intentFieldsText(f)));
    return v;
}

IntentFields fieldsFrom(const juce::var& v)
{
    IntentFields f;
    if (!v.isObject())
        return f;
    f.energy = static_cast<float>(static_cast<double>(v["energy"]));
    f.density = static_cast<float>(static_cast<double>(v["density"]));
    f.brightness = static_cast<float>(static_cast<double>(v["brightness"]));
    f.harmonicity = static_cast<float>(static_cast<double>(v["harmonicity"]));
    f.bpm = static_cast<float>(static_cast<double>(v["bpm"]));
    f.mood = static_cast<int>(v["mood"]);
    f.ensemble = static_cast<int>(v["ensemble"]);
    f.harmonyLanguage = static_cast<int>(v["harmonyLanguage"]);
    f.voice = static_cast<int>(v["voice"]);
    // os arquivos de antes não os têm: ficam "não ditos" (−1), nunca 0
    if (v.hasProperty("rockStyle")) f.rockStyle = static_cast<int>(v["rockStyle"]);
    if (v.hasProperty("narrative")) f.narrative = static_cast<int>(v["narrative"]);
    if (v.hasProperty("dance")) f.dance = static_cast<float>(static_cast<double>(v["dance"]));
    if (v.hasProperty("freedom")) f.freedom = static_cast<float>(static_cast<double>(v["freedom"]));
    return f;
}

// o manifesto da referência: promptOriginalProvided e promptsRef
void markManifest(const std::string& dataRoot, const std::string& referenceId, bool provided, std::vector<std::string>& problems)
{
    const auto mf = juce::File(u8(referenceManifestDir(dataRoot))).getChildFile(u8(referenceId) + ".json");
    auto read = fromJson(mf.loadFileAsString().toStdString());
    if (!read.manifest)
    {
        problems.push_back("manifesto não encontrado: " + referenceId);
        return;
    }
    read.manifest->promptOriginalProvided = provided;
    read.manifest->promptsRef = provided ? "indice/prompts/" + referenceId + ".json" : std::string();
    if (!mf.replaceWithText(u8(toJson(*read.manifest))))
        problems.push_back("falha ao gravar o manifesto de " + referenceId);
}
} // namespace

std::string promptsPath(const std::string& dataRoot, const std::string& referenceId)
{
    return juce::File(u8(dataRoot)).getChildFile("indice").getChildFile("prompts").getChildFile(u8(referenceId) + ".json")
        .getFullPathName().toStdString();
}

bool loadPrompt(const std::string& dataRoot, const std::string& referenceId, PromptRecord& r)
{
    const juce::File f(u8(promptsPath(dataRoot, referenceId)));
    if (!f.existsAsFile())
        return false;
    const auto v = juce::JSON::parse(f.loadFileAsString());
    if (!v.isObject() || s8(v["schemaVersion"]) != "trioio.prompts/1")
        return false;
    r = {};
    r.referenceId = referenceId;
    if (const auto& o = v["promptOriginal"]; o.isObject())
    {
        r.hasOriginal = true;
        r.original = s8(o["text"]);
        r.source = s8(o["source"]);
        r.providedAt = s8(o["providedAt"]);
    }
    const auto& d = v["declaredIntent"];
    r.declared.fields = fieldsFrom(d["fields"]);
    r.declared.revised = static_cast<bool>(d["revised"]);
    if (const auto* a = d["matches"].getArray())
        for (const auto& m : *a)
            r.declared.matches.push_back({ s8(m["word"]), s8(m["field"]), s8(m["value"]) });
    if (const auto* a = d["unread"].getArray())
        for (const auto& w : *a)
            r.declared.unread.push_back(s8(w));
    if (v["analysisInferred"].isObject())
    {
        r.hasInferred = true;
        r.inferred = fieldsFrom(v["analysisInferred"]);
    }
    if (const auto* a = v["history"].getArray())
        for (const auto& h : *a)
            r.history.push_back(s8(h));
    return true;
}

bool savePrompt(const std::string& dataRoot, const PromptRecord& r, std::vector<std::string>& problems)
{
    auto root = juce::var(new juce::DynamicObject());
    auto* o = root.getDynamicObject();
    o->setProperty("schemaVersion", "trioio.prompts/1");
    o->setProperty("referenceId", u8(r.referenceId));
    if (r.hasOriginal)
    {
        auto p = juce::var(new juce::DynamicObject());
        p.getDynamicObject()->setProperty("text", u8(r.original));
        p.getDynamicObject()->setProperty("source", u8(r.source));
        p.getDynamicObject()->setProperty("providedAt", u8(r.providedAt));
        o->setProperty("promptOriginal", p);
    }
    auto d = juce::var(new juce::DynamicObject());
    d.getDynamicObject()->setProperty("fields", fieldsJson(r.declared.fields));
    d.getDynamicObject()->setProperty("revised", r.declared.revised);
    juce::Array<juce::var> ms, un, hi;
    for (const auto& m : r.declared.matches)
    {
        auto x = juce::var(new juce::DynamicObject());
        x.getDynamicObject()->setProperty("word", u8(m.word));
        x.getDynamicObject()->setProperty("field", u8(m.field));
        x.getDynamicObject()->setProperty("value", u8(m.value));
        ms.add(x);
    }
    for (const auto& w : r.declared.unread)
        un.add(u8(w));
    d.getDynamicObject()->setProperty("matches", ms);
    d.getDynamicObject()->setProperty("unread", un);
    o->setProperty("declaredIntent", d);
    if (r.hasInferred)
        o->setProperty("analysisInferred", fieldsJson(r.inferred));
    for (const auto& h : r.history)
        hi.add(u8(h));
    o->setProperty("history", hi);
    const juce::File f(u8(promptsPath(dataRoot, r.referenceId)));
    f.getParentDirectory().createDirectory();
    juce::TemporaryFile tmp(f);
    if (!tmp.getFile().replaceWithText(juce::JSON::toString(root)) || !tmp.overwriteTargetFileWithTemporary())
    {
        problems.push_back("falha ao gravar " + f.getFullPathName().toStdString());
        return false;
    }
    return true;
}

bool providePrompt(const std::string& dataRoot, const std::string& referenceId, const std::string& text, const std::string& source,
                   const std::string& now, std::vector<std::string>& problems)
{
    PromptRecord r;
    const bool existed = loadPrompt(dataRoot, referenceId, r);
    r.referenceId = referenceId;
    r.hasOriginal = true;
    r.original = text;
    r.source = source.empty() ? "autor" : source;
    r.providedAt = now;
    r.declared = translatePrompt(text);
    r.history.push_back(now + ": " + (existed ? "prompt trocado" : "prompt fornecido") + " (" + r.source + ")");
    if (!savePrompt(dataRoot, r, problems))
        return false;
    markManifest(dataRoot, referenceId, true, problems);
    return problems.empty();
}

bool reviseIntent(const std::string& dataRoot, const std::string& referenceId, const std::string& edits, const std::string& now,
                  std::vector<std::string>& problems)
{
    PromptRecord r;
    if (!loadPrompt(dataRoot, referenceId, r))
        r.referenceId = referenceId;  // uma intenção pode ser declarada sem prompt
    auto& f = r.declared.fields;
    juce::StringArray parts;
    parts.addTokens(u8(edits), ",", "");
    for (auto p : parts)
    {
        p = p.trim();
        const auto k = p.upToFirstOccurrenceOf("=", false, false).trim().toLowerCase().toStdString();
        const auto v = p.fromFirstOccurrenceOf("=", false, false).trim().toLowerCase().toStdString();
        const float x = static_cast<float>(std::atof(v.c_str()));
        bool ok = true;
        if (k == "energia" || k == "energy") f.energy = std::clamp(x, 0.0f, 1.0f);
        else if (k == "densidade" || k == "density") f.density = std::clamp(x, 0.0f, 1.0f);
        else if (k == "brilho" || k == "brightness") f.brightness = std::clamp(x, 0.0f, 1.0f);
        else if (k == "harmonicidade" || k == "harmonicity") f.harmonicity = std::clamp(x, 0.0f, 1.0f);
        else if (k == "andamento" || k == "bpm") f.bpm = x;
        else if (k == "voz" || k == "voice") f.voice = (v == "sim" || v == "1") ? 1 : 0;
        else if (k == "humor" || k == "mood")
        {
            Intention i;
            ok = parseIntention(v, i);
            if (ok) f.mood = static_cast<int>(i);
        }
        else if (k == "estilo" || k == "conjunto")
        {
            Ensemble e;
            ok = parseEnsemble(v, e);
            if (ok) f.ensemble = static_cast<int>(e);
        }
        else if (k == "harmonia" || k == "linguagem")
        {
            HarmonyLanguage h;
            ok = parseHarmonyLanguage(v, h);
            if (ok) f.harmonyLanguage = static_cast<int>(h);
        }
        else
            ok = false;
        if (!ok)
        {
            problems.push_back("não entendi \"" + p.toStdString() + "\" (energia, densidade, brilho, harmonicidade, andamento, humor, estilo, harmonia, voz)");
            return false;
        }
    }
    r.declared.revised = true;
    r.history.push_back(now + ": tradução revisada (" + edits + ")");
    return savePrompt(dataRoot, r, problems);
}

bool removePrompt(const std::string& dataRoot, const std::string& referenceId, std::vector<std::string>& problems)
{
    const juce::File f(u8(promptsPath(dataRoot, referenceId)));
    if (f.existsAsFile() && !f.deleteFile())
    {
        problems.push_back("falha ao apagar " + f.getFullPathName().toStdString());
        return false;
    }
    markManifest(dataRoot, referenceId, false, problems);
    return problems.empty();
}

} // namespace trioio::library
