// A retenção das sessões (src/library/Retention.h): a julgada e as recentes
// ficam; as de teste vão para a lixeira do dia, de onde podem voltar; a
// lixeira só se esvazia depois de D dias.
#include "ManifestJson.h"
#include "Response.h"
#include "Retention.h"
#include "LibraryImporter.h"

#include <juce_core/juce_core.h>

#include <cstdio>
#include <cstdlib>

using namespace trioio::library;

static int falhas = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FALHOU %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++falhas;                                                            \
        }                                                                        \
    } while (0)

static void sessao(const juce::File& root, const std::string& id, const std::string& der)
{
    const auto d = root.getChildFile("sessoes").getChildFile(id).getChildFile("saidas");
    d.createDirectory();
    d.getChildFile("x.wav").replaceWithText("som");
    trioio::provenance::Manifest m;
    m.id = der;
    m.kind = trioio::provenance::ManifestKind::derivation;
    trioio::provenance::Derivation dv;
    dv.sessionId = id;
    dv.from = { "ref-x" };
    m.derivation = dv;
    const auto dir = juce::File(juce::String(derivationManifestDir(root.getFullPathName().toStdString())));
    dir.createDirectory();
    dir.getChildFile(juce::String(der) + ".json").replaceWithText(juce::String::fromUTF8(toJson(m).c_str()));
}

int main()
{
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("trioio_retencao_" + juce::String(juce::Random().nextInt64()));
    root.createDirectory();
    const std::string dados = root.getFullPathName().toStdString();
    sessao(root, "sessao-20261001000000-seguir-s1", "der-a");  // julgada
    sessao(root, "sessao-20261002000000-seguir-s1", "der-b");  // teste
    sessao(root, "sessao-20261003000000-seguir-s1", "der-c");  // teste
    sessao(root, "sessao-20261004000000-seguir-s1", "der-d");  // recente
    root.getChildFile("escuta.json").replaceWithText(R"({"schema":"x","judgements":[{"derivation":"der-a","verdict":"aceitar"}]})");

    const auto st = sessionStatus(dados, 1);
    CHECK(st.size() == 4 && st.front().id == "sessao-20261004000000-seguir-s1" && st.front().recent);
    int julgadas = 0;
    for (const auto& s : st)
        julgadas += s.judged ? 1 : 0;
    CHECK(julgadas == 1 && st.back().judged);

    const auto moved = moveDisposableToTrash(dados, "2026-10-17", 1);
    CHECK(moved.size() == 2);
    CHECK(root.getChildFile("sessoes/sessao-20261001000000-seguir-s1").isDirectory());  // a julgada fica
    CHECK(root.getChildFile("sessoes/sessao-20261004000000-seguir-s1").isDirectory());  // a recente fica
    CHECK(root.getChildFile("lixeira/2026-10-17/sessao-20261002000000-seguir-s1/saidas/x.wav").existsAsFile());  // reversível
    CHECK(root.getChildFile("indice/derivacoes/der-b.json").existsAsFile());  // o registro fica no índice

    // a lixeira: com 7 dias, a pasta de hoje fica; 8 dias depois, sai
    CHECK(emptyTrash(dados, "2026-10-20", 7).empty() && root.getChildFile("lixeira/2026-10-17").isDirectory());
    CHECK(emptyTrash(dados, "2026-10-25", 7).size() == 1 && !root.getChildFile("lixeira/2026-10-17").exists());
    // a virada de mês conta certo
    root.getChildFile("lixeira/2026-09-28").createDirectory();
    CHECK(emptyTrash(dados, "2026-10-04", 7).empty());
    CHECK(emptyTrash(dados, "2026-10-06", 7).size() == 1);

    // o cache: o de uma referência do índice fica; o órfão sai
    {
        trioio::provenance::Manifest r;
        r.id = "ref-x";
        r.identity.contentSha256 = "aaaa";
        const juce::File rdir(juce::String(referenceManifestDir(dados)));
        rdir.createDirectory();
        rdir.getChildFile("ref-x.json").replaceWithText(juce::String::fromUTF8(toJson(r).c_str()));
        const auto cdir = root.getChildFile("cache/analise");
        cdir.createDirectory();
        cdir.getChildFile("aaaa.json").replaceWithText("{}");
        cdir.getChildFile("bbbb.json").replaceWithText("{}");
        int orfaos = 0;
        for (const auto& c : cacheStatus(dados))
            orfaos += c.orphan ? 1 : 0;
        CHECK(orfaos == 1);
        CHECK(removeCaches(dados, false).size() == 1 && cdir.getChildFile("aaaa.json").existsAsFile() && !cdir.getChildFile("bbbb.json").exists());
        CHECK(removeCaches(dados, true).size() == 1 && !cdir.getChildFile("aaaa.json").exists());
    }
    // as ações de uma resposta pelo app: exportar e mandar uma sessão à lixeira
    {
        const auto ses = root.getChildFile("sessoes").getChildFile("sessao-20261019000000-seguir-s7");
        ses.getChildFile("saidas").getChildFile("stems").createDirectory();
        (void) ses.getChildFile("saidas").getChildFile("r.wav").replaceWithText("wav");
        (void) ses.getChildFile("saidas").getChildFile("stems").getChildFile("1-motivo.wav").replaceWithText("stem");
        (void) ses.getChildFile("decisoes.json").replaceWithText("{}");
        const auto fora = root.getChildFile("exportadas");
        fora.createDirectory();
        std::string criada, erro;
        CHECK(exportSession(dados, "sessao-20261019000000-seguir-s7", fora.getFullPathName().toStdString(), criada, erro));
        const auto alvo = juce::File(juce::String::fromUTF8(criada.c_str()));
        CHECK(alvo.getChildFile("r.wav").existsAsFile() && alvo.getChildFile("stems").getChildFile("1-motivo.wav").existsAsFile()
              && alvo.getChildFile("decisoes.json").existsAsFile());
        std::string criada2;
        CHECK(exportSession(dados, "sessao-20261019000000-seguir-s7", fora.getFullPathName().toStdString(), criada2, erro)
              && criada2 != criada && juce::String::fromUTF8(criada2.c_str()).endsWith("-2"));  // nada sobrescrito
        CHECK(!exportSession(dados, "../x", fora.getFullPathName().toStdString(), criada2, erro));
        CHECK(!exportSession(dados, "sessao-20261019000000-seguir-s7", ses.getFullPathName().toStdString(), criada2, erro));
        CHECK(moveSessionToTrash(dados, "sessao-20261019000000-seguir-s7", "2026-10-19", erro) && !ses.exists());
        CHECK(root.getChildFile("lixeira").getChildFile("2026-10-19").getChildFile("sessao-20261019000000-seguir-s7")
                  .getChildFile("saidas").getChildFile("r.wav").existsAsFile());
        CHECK(!moveSessionToTrash(dados, "sessao-20261019000000-seguir-s7", "2026-10-19", erro));  // já não existe
        CHECK(!moveSessionToTrash(dados, "sessao-x/../../y", "2026-10-19", erro));
    }
    root.deleteRecursively();
    if (falhas)
    {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", falhas);
        return EXIT_FAILURE;
    }
    std::puts("retenção: todas as checagens passaram");
    return EXIT_SUCCESS;
}
