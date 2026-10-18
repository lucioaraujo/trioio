#pragma once

// Memória pesquisável por timbre (Atlas de Referência RASGO §62, CataRT /
// MuBu, IRCAM: síntese concatenativa por corpus — "a memória pode ser
// pesquisável").
//
// O oráculo (OracleMemory.h) lembra a referência pela ORDEM: o que veio
// depois do quê. O mosaico a lembra pelo SOM: cada ataque da referência vira
// uma unidade com descritores (energia, brilho, ruído, altura, duração), e a
// cada passo a resposta procura no acervo as unidades que soam como o momento
// pede — "seguir", parecidas com a referência daquele instante; "contrapor",
// energia e ruído opostos; "transfigurar", brilho invertido; "interromper",
// em fragmentos com respiros. A escolha soma o custo do alvo e o custo da
// emenda (salto de altura até a unidade anterior), como na seleção de
// unidades de D. Schwarz.
//
// Cada nota do mosaico guarda de onde veio (Note::source): a família
// espectral toca com o corpo do instante de origem, não o do instante em que
// soa — o trecho do minuto 1 que volta no minuto 3 traz o timbre do minuto 1.
//
// Referências (públicas; implementação própria, do zero):
//   D. Schwarz, "Corpus-based concatenative synthesis", IEEE Signal
//   Processing Magazine 24(2), 2007;
//   D. Schwarz et al., "Real-time corpus-based concatenative synthesis with
//   CataRT", DAFx 2006.
// Especificação: docs/MOSAICO.md.

#include "Composer.h"

#include <vector>

namespace trioio::composition
{

// Acrescenta passagens de mosaico (voice 5). Quantas dependem da
// imprevisibilidade (0 = nenhuma: a composição fica idêntica); evitam as
// passagens do oráculo. Marca cada passagem na forma.
void descriptorMosaic(Score& score, const std::vector<perception::Frame>& frames,
                      const std::vector<perception::Onset>& onsets, const Options& options);

} // namespace trioio::composition
