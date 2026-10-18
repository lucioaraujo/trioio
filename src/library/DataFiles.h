#pragma once

// Os arquivos de dados do Trioio (data/: escalas, ritmos, cadências) num
// binário instalado em outra máquina (2026-10-18: antes, só pelo caminho do
// código-fonte embutido na compilação). A busca, na ordem:
//   1. TRIOIO_DATA_DIR (a pasta que contém escalas/, ritmos/, harmonia/);
//   2. data/ ao lado do executável (Windows, o tar.gz, o build);
//   3. ../Resources/data (o .app do macOS);
//   4. ../share/trioio/data (a instalação Linux, o AppImage);
//   5. o código-fonte (TRIOIO_SOURCE_DIR/data), no desenvolvimento.
// Devolve o primeiro que existe; se nenhum, o do código-fonte (para a
// mensagem de erro dizer onde procurou).

#include <juce_core/juce_core.h>

namespace trioio::library
{

[[nodiscard]] juce::File trioioDataFile(const juce::String& relative);

} // namespace trioio::library
