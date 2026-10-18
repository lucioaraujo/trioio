#pragma once

// A escuta do autor gravada na raiz de dados (escuta.json). O cálculo das
// tendências é do núcleo (core/Listening.h); aqui só se lê e grava.
// Especificação: docs/ESCUTA.md.

#include "Listening.h"

#include <string>
#include <vector>

namespace trioio::library
{

[[nodiscard]] std::string listeningPath(const std::string& dataRoot);  // <dados>/escuta.json

// O histórico, do mais antigo ao mais novo (vazio se não houver arquivo).
[[nodiscard]] std::vector<composition::Judgement> loadJudgements(const std::string& dataRoot);

struct JudgementResult
{
    bool written = false;
    std::vector<std::string> problems;
    composition::Judgement judgement;               // o primeiro (ou único) julgamento gravado
    std::vector<composition::Judgement> judgements;  // todos: uma letra de seção pode julgar várias seções
};

// Julga a resposta `derivationId` (der-…): lê o que ela usou no decisoes.json
// da sua sessão e acrescenta o julgamento ao escuta.json. `span` ("0:00-1:00",
// "1:41-fim") ou `section` ("3", "A") julgam só um trecho (docs/ESCUTA.md,
// "Julgamento por trecho"); vazios, a resposta inteira. Nada é gravado se o
// trecho ou a seção não existirem.
[[nodiscard]] JudgementResult recordJudgement(const std::string& dataRoot, const std::string& derivationId,
                                              composition::Verdict verdict, const std::string& note,
                                              const std::string& now = {}, const std::string& span = {},
                                              const std::string& section = {});

} // namespace trioio::library
