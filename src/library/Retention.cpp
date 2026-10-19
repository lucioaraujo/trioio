#include "Retention.h"

#include "ListeningStore.h"
#include "LibraryImporter.h"
#include "ManifestJson.h"
#include "Response.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <set>

namespace trioio::library
{
namespace
{
juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.c_str()); }

double megabytesOf(const juce::File& dir)
{
    juce::int64 total = 0;
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, true, "*"))
        total += f.getSize();
    return static_cast<double>(total) / (1024.0 * 1024.0);
}

// AAAA-MM-DD → dias desde uma origem (para comparar datas sem fuso)
int dayNumber(const std::string& ymd)
{
    if (ymd.size() < 10)
        return 0;
    const int y = std::atoi(ymd.substr(0, 4).c_str()), m = std::atoi(ymd.substr(5, 2).c_str()), d = std::atoi(ymd.substr(8, 2).c_str());
    // dias civis (H. Hinnant, "chrono-compatible low-level date algorithms")
    const int yy = y - (m <= 2 ? 1 : 0);
    const int era = (yy >= 0 ? yy : yy - 399) / 400;
    const int yoe = yy - era * 400;
    const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}
} // namespace

std::vector<SessionStatus> sessionStatus(const std::string& dataRoot, int keepRecent)
{
    // as sessões julgadas: as das derivações com julgamento
    std::set<std::string> judgedDerivations, judgedSessions;
    for (const auto& j : loadJudgements(dataRoot))
        judgedDerivations.insert(j.derivationId);
    for (const auto& f : juce::File(u8(derivationManifestDir(dataRoot))).findChildFiles(juce::File::findFiles, false, "*.json"))
    {
        const auto read = fromJson(f.loadFileAsString().toStdString());
        if (read.manifest && read.manifest->derivation && judgedDerivations.count(read.manifest->id))
            judgedSessions.insert(read.manifest->derivation->sessionId);
    }
    std::vector<SessionStatus> out;
    for (const auto& d : juce::File(u8(dataRoot)).getChildFile("sessoes").findChildFiles(juce::File::findDirectories, false, "sessao-*"))
    {
        SessionStatus s;
        s.id = d.getFileName().toStdString();
        s.judged = judgedSessions.count(s.id) > 0;
        s.megabytes = megabytesOf(d);
        out.push_back(s);
    }
    std::sort(out.begin(), out.end(), [](const SessionStatus& a, const SessionStatus& b) { return a.id > b.id; });
    for (std::size_t i = 0; i < out.size() && static_cast<int>(i) < keepRecent; ++i)
        out[i].recent = true;
    return out;
}

std::vector<std::string> moveDisposableToTrash(const std::string& dataRoot, const std::string& today, int keepRecent)
{
    std::vector<std::string> moved;
    const juce::File root(u8(dataRoot));
    const auto trash = root.getChildFile("lixeira").getChildFile(u8(today));
    for (const auto& s : sessionStatus(dataRoot, keepRecent))
    {
        if (s.judged || s.recent)
            continue;
        if (!trash.isDirectory() && !trash.createDirectory())
            break;
        const auto from = root.getChildFile("sessoes").getChildFile(u8(s.id));
        if (from.moveFileTo(trash.getChildFile(u8(s.id))))
            moved.push_back(s.id);
    }
    return moved;
}

std::vector<CacheStatus> cacheStatus(const std::string& dataRoot)
{
    std::set<std::string> live;
    for (const auto& f : juce::File(u8(referenceManifestDir(dataRoot))).findChildFiles(juce::File::findFiles, false, "*.json"))
    {
        const auto read = fromJson(f.loadFileAsString().toStdString());
        if (read.manifest)
            live.insert(read.manifest->identity.contentSha256);
    }
    std::vector<CacheStatus> out;
    for (const auto& f : juce::File(u8(dataRoot)).getChildFile("cache").getChildFile("analise").findChildFiles(juce::File::findFiles, false, "*.json"))
    {
        CacheStatus c;
        c.file = f.getFileName().toStdString();
        c.orphan = live.count(f.getFileNameWithoutExtension().toStdString()) == 0;
        c.megabytes = static_cast<double>(f.getSize()) / (1024.0 * 1024.0);
        out.push_back(c);
    }
    return out;
}

std::vector<std::string> removeCaches(const std::string& dataRoot, bool all)
{
    std::vector<std::string> removed;
    const auto dir = juce::File(u8(dataRoot)).getChildFile("cache").getChildFile("analise");
    for (const auto& c : cacheStatus(dataRoot))
        if ((all || c.orphan) && dir.getChildFile(u8(c.file)).deleteFile())
            removed.push_back(c.file);
    return removed;
}

std::vector<std::string> emptyTrash(const std::string& dataRoot, const std::string& today, int days)
{
    std::vector<std::string> removed;
    const auto trash = juce::File(u8(dataRoot)).getChildFile("lixeira");
    const int now = dayNumber(today);
    for (const auto& d : trash.findChildFiles(juce::File::findDirectories, false, "????" "-" "??" "-" "??"))
    {
        const auto name = d.getFileName().toStdString();
        if (now - dayNumber(name) > days && d.deleteRecursively())
            removed.push_back(name);
    }
    return removed;
}

namespace
{
// só o nome de uma pasta de sessão: nada de caminhos
bool validSessionId(const std::string& id)
{
    return id.rfind("sessao-", 0) == 0 && id.find('/') == std::string::npos && id.find('\\') == std::string::npos
           && id.find("..") == std::string::npos;
}
} // namespace

bool moveSessionToTrash(const std::string& dataRoot, const std::string& sessionId, const std::string& today, std::string& error)
{
    if (!validSessionId(sessionId))
    {
        error = "sessão inválida: " + sessionId;
        return false;
    }
    const auto root = juce::File(u8(dataRoot));
    const auto from = root.getChildFile("sessoes").getChildFile(u8(sessionId));
    if (!from.isDirectory())
    {
        error = "a sessão não existe: " + sessionId;
        return false;
    }
    const auto trash = root.getChildFile("lixeira").getChildFile(u8(today));
    if (!trash.isDirectory() && !trash.createDirectory())
    {
        error = "não consegui criar " + trash.getFullPathName().toStdString();
        return false;
    }
    auto to = trash.getChildFile(u8(sessionId));
    for (int n = 2; to.exists(); ++n)
        to = trash.getChildFile(u8(sessionId) + "-" + juce::String(n));
    if (!from.moveFileTo(to))
    {
        error = "não consegui mover para " + to.getFullPathName().toStdString();
        return false;
    }
    return true;
}

bool exportSession(const std::string& dataRoot, const std::string& sessionId, const std::string& destination,
                   std::string& createdDir, std::string& error)
{
    if (!validSessionId(sessionId))
    {
        error = "sessão inválida: " + sessionId;
        return false;
    }
    const auto session = juce::File(u8(dataRoot)).getChildFile("sessoes").getChildFile(u8(sessionId));
    const auto outputs = session.getChildFile("saidas");
    const auto dest = juce::File(u8(destination));
    if (!outputs.isDirectory())
    {
        error = "a sessão não tem saídas: " + sessionId;
        return false;
    }
    if (!dest.isDirectory() || dest == session || dest.isAChildOf(session))
    {
        error = "destino inválido: " + destination;
        return false;
    }
    auto target = dest.getChildFile(u8(sessionId));
    for (int n = 2; target.exists(); ++n)
        target = dest.getChildFile(u8(sessionId) + "-" + juce::String(n));
    if (!outputs.copyDirectoryTo(target))
    {
        error = "não consegui copiar para " + target.getFullPathName().toStdString();
        return false;
    }
    if (const auto d = session.getChildFile("decisoes.json"); d.existsAsFile())
        (void) d.copyFileTo(target.getChildFile("decisoes.json"));
    createdDir = target.getFullPathName().toStdString();
    return true;
}

} // namespace trioio::library
