# Trioio — Installation Guide / Guia de Instalação

The installation steps for Windows and macOS follow the Antitotem guide
(`lucioaraujo/antitotem`, `INSTALL.md`), checked on real machines by the
author; the requirements below were measured for the Trioio itself.

- [English](#english)
- [Português](#português)

## English

### Installing, step by step

Packages for every system are on the
[releases page](https://github.com/lucioaraujo/trioio/releases); the newest
one is at the top. The security warnings below **are expected**. They do not
mean anything is wrong, and they only appear the first time.

#### Windows

1. Download `trioio-<version>-win64.exe`.
2. Open it. Windows shows a blue **"Windows protected your PC"** box: click
   **"More info"**, then **"Run anyway"**.
3. If Windows asks for administrator permission, click **"Yes"**.
4. Click **"Next"** to the end. The last page has **"Run Trioio"** already
   ticked. Trioio is then in the **Start menu** and on the desktop.

**Without installing:** unzip `trioio-<version>-win64.zip` and open
`Trioio.exe`. Keep the `data` folder next to it.

#### macOS

1. Download `trioio-<version>-Darwin.dmg`. It works on Intel and on Apple
   Silicon Macs.
2. Open it and **drag Trioio into Applications**.
3. Open it. The first time, macOS **blocks** the app with a message saying
   that Apple could not confirm it is free of malicious software. You may see
   "cannot verify the developer" or "is damaged and can't be opened" instead.
   In every case:
   - This is **not a defect or a virus**: the app is signed ad hoc, without
     a paid Apple Developer ID.
   - Click **"OK"** or **"Done"**, **not** "Move to Trash".
4. Allow the app, once, in one of two ways:
   - **In System Settings:** go to **Privacy & Security**, scroll to the
     security section, click **"Open Anyway"**, confirm with your password
     or Touch ID, and click **"Open"**. On macOS 14 or earlier: right-click
     the app in Finder → **Open → Open**.
   - **In Terminal**, if the button does not appear:

     ```sh
     xattr -dr com.apple.quarantine "/Applications/Trioio.app"
     ```

     If nothing is printed, it worked. The command only removes the
     "quarantine mark" macOS puts on every downloaded file.

#### Linux

- **Ubuntu 22.04+, Debian 12+, Mint 21+:** double-click the `.deb`, or run
  `sudo apt install ./trioio-<version>-Linux.deb`.
- **Fedora, Arch, openSUSE and other distributions:**
  - **AppImage:** make it executable (Properties → allow executing, or
    `chmod +x`) and open it.
  - **`.tar.gz`:** unpack it and run `./install.sh`. It installs for your
    user only, without root, and `./install.sh --remove` undoes it.

### The first time you open it

1. Trioio opens in demonstration mode until you choose a **data folder**
   with **open data folder…**. Your
   recordings, responses, judgements and memory live there, never inside
   the app. Choose an empty folder on a disk with room. A response with
   stems takes a few hundred MB.
2. The **TUTORIAL** button, next to **ABOUT**, explains the instrument step
   by step, in English, Portuguese, French or Spanish. It follows the
   interface language.
3. **The internet is optional.** Trioio composes offline. Only the box "look
   up unread words online" sends words to public services (Wikidata,
   Datamuse), and it is off by default.

### Requirements

| | |
|---|---|
| **Windows** | Windows 10 or 11, 64-bit |
| **macOS** | 10.15 or later; Intel and Apple Silicon (Universal 2) |
| **Linux** | x86-64 with glibc 2.35 or newer (2022+ distributions) |
| **Processor** | 64-bit, two cores or more. **Measured** on an Intel Core i5-6500 (2015, 4 cores): a response takes about 20 s (60 s with three lines) and uses about one core |
| **Memory** | about 250–500 MB while composing (measured); 4 GB in the computer is enough |
| **Disk** | about 25 MB for the app; the responses go to your data folder |
| **Audio** | any system audio output |

The processor and memory figures were measured on the author's machine
(`docs/MEDICOES.md`). Weaker machines have not been tested.

### Building from source

```sh
git clone https://github.com/lucioaraujo/trioio.git
git clone --branch 9.0.3 https://github.com/juce-framework/JUCE.git
cmake -S trioio -B trioio/build -DCMAKE_BUILD_TYPE=Release -DTRIOIO_JUCE_PATH=$PWD/JUCE
cmake --build trioio/build --parallel 2
(cd trioio/build && ctest --output-on-failure)
```

On Linux, the JUCE build needs the ALSA, FreeType, Fontconfig, X11 and GTK
development packages. The workflow `.github/workflows/package.yml` lists
them.

### Status

Version 0.1.0 is a **preview**. The Windows and macOS packages are built,
tested and checked by continuous integration: no dependency on the Visual
C++ Redistributable, and a sealed Universal 2 bundle on macOS. They have not
yet been opened by the author on real Windows and Mac machines.

### License

AGPL-3.0-or-later (`LICENSE`). Credits and sources: `CREDITS_AND_SOURCES.md`.
Problems: **rasgo.instruments@gmail.com**.

## Português

### Instalar, passo a passo

Os pacotes de cada sistema estão na
[página de releases](https://github.com/lucioaraujo/trioio/releases); o
mais novo fica em cima. Os avisos de segurança abaixo **são esperados**.
Não indicam defeito e só aparecem na primeira vez.

#### Windows

1. Baixe `trioio-<versão>-win64.exe`.
2. Abra. O Windows mostra uma caixa azul **"O Windows protegeu o
   computador"**: clique em **"Mais informações"** e depois em
   **"Executar assim mesmo"**.
3. Se pedir permissão de administrador, clique em **"Sim"**.
4. Clique em **"Avançar"** até o fim. A última página já vem com **"Executar
   Trioio"** marcado. O Trioio fica no **menu Iniciar** e na área de
   trabalho.

**Sem instalar:** descompacte `trioio-<versão>-win64.zip` e abra
`Trioio.exe`. Mantenha a pasta `data` ao lado dele.

#### macOS

1. Baixe `trioio-<versão>-Darwin.dmg`. Ele funciona em Macs Intel e Apple
   Silicon.
2. Abra e **arraste o Trioio para Aplicativos**.
3. Abra. Na primeira vez, o macOS **bloqueia** o app com uma mensagem
   dizendo que a Apple não pôde confirmar que ele está livre de software
   malicioso. Também pode aparecer "não é possível verificar o
   desenvolvedor" ou "está danificado". Em todos os casos:
   - **Não é defeito nem vírus**: o app é assinado ad hoc, sem o Developer
     ID pago da Apple.
   - Clique em **"OK"**, **não** em "Mover para o Lixo".
4. Libere o app uma vez, de um destes dois jeitos:
   - **Em Ajustes do Sistema:** vá em **Privacidade e Segurança**, desça até
     a seção de segurança, clique em **"Abrir Mesmo Assim"**, confirme com a
     senha ou o Touch ID e clique em **"Abrir"**. No macOS 14 ou anterior:
     clique com o botão direito no app no Finder → **Abrir → Abrir**.
   - **No Terminal**, se o botão não aparecer:

     ```sh
     xattr -dr com.apple.quarantine "/Applications/Trioio.app"
     ```

     Se nada for impresso, funcionou. O comando só tira a "marca de
     quarentena" que o macOS põe em todo arquivo baixado.

#### Linux

- **Ubuntu 22.04+, Debian 12+, Mint 21+:** dois cliques no `.deb`, ou
  `sudo apt install ./trioio-<versão>-Linux.deb`.
- **Fedora, Arch, openSUSE e outras distribuições:**
  - **AppImage:** torne-o executável (Propriedades → permitir executar, ou
    `chmod +x`) e abra.
  - **`.tar.gz`:** descompacte e rode `./install.sh`. Ele instala só para o
    seu usuário, sem root, e `./install.sh --remove` desfaz.

### Na primeira abertura

1. O Trioio abre em modo de demonstração até você escolher uma **pasta de
   dados** em **abrir pasta de dados…**. As
   suas gravações, respostas, julgamentos e memória ficam lá, nunca dentro
   do app. Escolha uma pasta vazia num disco com espaço. Uma resposta com
   stems ocupa algumas centenas de MB.
2. O botão **TUTORIAL**, ao lado de **SOBRE**, explica o instrumento passo
   a passo, em português, inglês, francês ou espanhol. Ele segue o idioma
   da interface.
3. **A internet é opcional.** O Trioio compõe sem rede. Só a caixa
   "consultar na rede as palavras não lidas" manda palavras a serviços
   públicos (Wikidata, Datamuse), e ela vem desligada.

### Requisitos

| | |
|---|---|
| **Windows** | Windows 10 ou 11, 64 bits |
| **macOS** | 10.15 ou mais novo; Intel e Apple Silicon (Universal 2) |
| **Linux** | x86-64 com glibc 2.35 ou mais nova (distribuições de 2022 em diante) |
| **Processador** | 64 bits, dois núcleos ou mais. **Medido** num Intel Core i5-6500 (2015, 4 núcleos): uma resposta leva cerca de 20 s (60 s com três linhas) e usa cerca de um núcleo |
| **Memória** | cerca de 250 a 500 MB enquanto compõe (medido); um computador com 4 GB basta |
| **Disco** | cerca de 25 MB para o app; as respostas vão para a sua pasta de dados |
| **Áudio** | qualquer saída de áudio do sistema |

Os números de processador e memória foram medidos na máquina do autor
(`docs/MEDICOES.md`). Máquinas mais fracas não foram testadas.

### Compilar a partir do código

```sh
git clone https://github.com/lucioaraujo/trioio.git
git clone --branch 9.0.3 https://github.com/juce-framework/JUCE.git
cmake -S trioio -B trioio/build -DCMAKE_BUILD_TYPE=Release -DTRIOIO_JUCE_PATH=$PWD/JUCE
cmake --build trioio/build --parallel 2
(cd trioio/build && ctest --output-on-failure)
```

No Linux, a compilação do JUCE pede os pacotes de desenvolvimento de ALSA,
FreeType, Fontconfig, X11 e GTK. O fluxo `.github/workflows/package.yml`
lista todos.

### Estado

A versão 0.1.0 é uma **prévia**. Os pacotes de Windows e macOS são
construídos, testados e conferidos pela integração contínua: sem
dependência do Visual C++ Redistributable, e o pacote macOS selado,
Universal 2. Eles ainda não foram abertos pelo autor em máquinas Windows e
Mac reais.

### Licença

AGPL-3.0-or-later (`LICENSE`). Créditos e fontes: `CREDITS_AND_SOURCES.md`.
Problemas: **rasgo.instruments@gmail.com**.
