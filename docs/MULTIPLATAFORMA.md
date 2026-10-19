# Windows, macOS e Linux: o que está pronto e o que falta

O padrão é o do RASGO
([`PADRAO_DISTRIBUICAO_MULTIPLATAFORMA.md`](../../../RASGO_DOCUMENTATION/PADRAO_DISTRIBUICAO_MULTIPLATAFORMA.md)).
A regra de custo: a validação do dia a dia é local, e o build nos três
sistemas é portão de release, não de rotina. A CI cobrada só roda com uma
decisão de push.

## Pronto (2026-10-18)

- **Linux:** a suíte local inteira (46 testes) e o app e a CLI compilados.
- **O core passa limpo no clang** (`-Wall -Wextra`), além do GCC. O clang
  é mais próximo do Apple clang. Achou duas variáveis sem uso nos testes e
  uma no core, corrigidas.
- **Windows:** o runtime estático (`CMAKE_MSVC_RUNTIME_LIBRARY`, /MT) desde
  já. O `/utf-8` do MSVC já estava. O código sem JUCE não usa API POSIX: o
  único `setenv` (num teste) tem a versão `_putenv_s`.
- **macOS:**
  - o binário Universal 2 (`x86_64;arm64`) e o alvo de implantação 10.15,
    antes do `project()`;
  - nada de `std::filesystem` nem de `from_chars`/`to_chars` com float (as
    lacunas da libc++ da Apple), nem `optional::value()`;
  - o FTZ/DAZ (`Denormals.h`) só entra em x86; em arm64 não faz nada.
- **Os dados viajam com o binário** (`src/library/DataFiles.h`). Antes, o
  app achava as escalas, os ritmos e as cadências só pelo caminho do
  código-fonte embutido na compilação, e instalado em outra máquina não os
  acharia. Agora:
  - o build copia `data/` para junto do executável, e no `.app` para
    `Contents/Resources/data`;
  - a busca segue `TRIOIO_DATA_DIR` → ao lado do executável → `Resources`
    → `share/trioio` → o fonte;
  - conferido: uma cópia da CLI em outra pasta leu o `data/` ao lado dela.

## O portão de release (2026-10-19, versão 0.1.0)

**Feito (local, sem custo):**

1. **O ícone:** a logo dos três olhos do autor, escolhida por ele (`packaging/icon/`,
   gerado do `website/assets/identity/trioio-olho.svg`; nos tamanhos de 16 a
   32 px, o traço é mais grosso). Ele está no `juce_add_gui_app` e no
   `.ico` do instalador.
2. **A instalação e o CPack:**
   - NSIS com o atalho do Menu Iniciar (`CPACK_PACKAGE_EXECUTABLES`), o
     `.exe` na raiz, e a pasta e o registro sem a versão;
   - DragNDrop no macOS;
   - `.deb` no Linux;
   - o AppImage e o `.tar.gz` por `packaging/linux/empacotar.sh`, copiado do
     Antitotem (`packaging/PROVENIENCIA.md`).

   Os dados vão onde `DataFiles.h` os procura.
3. **O fluxo da CI** (`.github/workflows/package.yml`), copiado do Antitotem
   e adaptado. A suíte de testes roda nos três sistemas, e a CI confere os
   dados dentro do `.app`, o runtime estático (`dumpbin`), o selo ad hoc
   dos dois slices, os atalhos do NSIS e a abertura do app em Debian 12,
   Ubuntu 24.04, Fedora e Arch. **Ainda não rodou.**
4. **O guia** `INSTALL.md` (en, pt).

**Verificado antes dos instaladores** (o autor: "verifique os possíveis
bugs antes de criar os instaladores"):

- a suíte inteira com AddressSanitizer e UndefinedBehaviorSanitizer:
  48/48, nenhum erro;
- quatro respostas completas nessa compilação, sem erros: a simples, as
  três entradas do autor (com rede, mistura e stems) e uma com jogo,
  diálogo e linhas;
- o app no modo de instantâneos nessa compilação, em sete telas, sem
  erros;
- a revisão do código achou quatro defeitos, corrigidos:
  - o User-Agent com a versão escrita à mão;
  - os outros nomes de uma palavra ignorados;
  - os campos novos perdidos no `prompts.json`;
  - as consultas da rede sobrepostas no app.

**Testado no Linux, local:** a árvore instalada abre o app (Xvfb e a
janela conferida), e o app lê os dados da instalação
(`usr/share/trioio/data`, conferido com o `strace`). O `.deb` e o
`.tar.gz` foram gerados localmente, em `TRIOIO_respostas/pacotes-0.1.0/`.
O `.deb` local pede a glibc 2.38 desta máquina; o `.deb` publicado sai da
CI, no Ubuntu 22.04.

**Publicado (2026-10-19):** o repositório público `lucioaraujo/trioio`,
com histórico limpo, e o release
[v0.1.0](https://github.com/lucioaraujo/trioio/releases/tag/v0.1.0),
marcado como pré-lançamento. Ele traz o `.exe` e o `.zip` do Windows, o
`.dmg` do macOS e o `.deb`, o AppImage e o `.tar.gz` do Linux.

**A CI achou três defeitos que o Linux x86 não mostra.** Foram precisas
três rodadas até ficar toda verde:

1. **macOS (ARM):** o seqlock do `EnergyControlBus` aceitava uma decisão
   rasgada. Faltavam duas cercas de memória (Boehm, MSPC 2012).
2. **Windows:** `M_PI` só existe com `_USE_MATH_DEFINES` no MSVC.
3. **Windows:** o teste da Biblioteca comparava a data das pastas, que o
   NTFS atualiza de forma preguiçosa. O defeito era do teste, não do
   importador.

Na rodada final, os 48 testes passaram nos três sistemas. A CI também
conferiu:

- o runtime estático no Windows e os atalhos do instalador;
- o `.app` Universal 2 selado, com os dados dentro;
- a abertura do app em Debian 12, Ubuntu 24.04, Fedora e Arch.

Depois, os pacotes publicados foram baixados e inspecionados: o `.deb`
pede a glibc 2.35, e os dados estão no `.zip` e no `.tar.gz`.

**Falta:** o teste do autor no Windows e no Mac reais.

## O pacote de fontes, o manifesto e a restauração

```
tools/empacotar_release.sh <pasta no disco de 4 TB> [--sem-teste]
```

**O que o script faz:**

1. **O recorte:** os arquivos versionados no git do Trioio (`git ls-files`),
   sem o site, que tem publicação própria. O que não está no git não entra:
   nada de `build/`, áudio, dados locais ou `0_BRAINSTORM`.
2. **O manifesto:** o `MANIFESTO.sha256` dentro do pacote, com o SHA-256 de
   cada arquivo, e o `.sha256` do próprio `tar.gz`, ao lado.
3. **A restauração:** extrai numa pasta nova, confere os dois checksums,
   configura só o core (sem JUCE), compila e roda os testes do core.

**Testado em 2026-10-18** (versão 0.0.1):

- 291 arquivos, 2,9 MB;
- os checksums ok;
- os 39 testes do core passaram no pacote restaurado, o que também prova
  que `data/` vai dentro;
- a pasta do teste foi apagada depois.

**Quando o autor escolher a versão estável:** subir `project(Trioio VERSION
…)`, rodar o script e guardar o `tar.gz` e o `.sha256` como cópia de
preservação (`RASGO_ARQUIVO`).

