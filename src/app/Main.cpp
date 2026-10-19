// O aplicativo do Trioio (docs/design.md): a janela pelo contrato comum
// (WindowPolicy.h), o transporte de áudio, e o modo de instantâneos que
// desenha a tela nas superfícies da matriz sem abrir janela:
//
//     trioio_app [--dados <raiz>]
//     trioio_app --instantaneos <pasta> [--dados <raiz>] [--referencia <id>] [--revelar] [--idioma en|pt|fr|es] [--sobre] [--tutorial]
//                  [--trecho <a>-<b> (segundos) | --secao <N>] [--autorizar] [--pagina 0|1|2]

#include "AudioSource.h"
#include "MainView.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace trioio::app
{

namespace
{
// os instantâneos: a tela em cada superfície da matriz, em PNG, mais a
// geometria dos controles essenciais (dentro da tela, ≥ 44 px, sem
// sobreposição)
int snapshots(const juce::File& out, const juce::String& dataRoot, const juce::String& referenceId, bool reveal,
              const juce::String& language, bool about, const juce::String& span, bool authorise, int page, bool languagePanel, bool tutorial)
{
    out.createDirectory();
    MainView view;
    view.setLanguage(langFromCode(language.isNotEmpty() ? language : juce::String("pt")));
    if (dataRoot.isNotEmpty())
    {
        view.openDataRoot(dataRoot.toStdString());
        if (referenceId.isNotEmpty())
            view.selectReferenceById(referenceId.toStdString());
        view.loadReferenceNow();  // nos instantâneos, sem a thread de trabalho
    }
    else
        view.showDemo();
    view.revealEye(reveal);
    if (about)
        view.showAbout(true);
    if (tutorial)
        view.showTutorial(true);
    if (authorise)
        view.openAuthorisation();
    if (page >= 0)
        view.openPage(page);
    if (languagePanel)
        view.openLanguage();
    if (span.startsWith("secao:"))
        view.selectSpan(0.0, 0.0, span.fromFirstOccurrenceOf(":", false, false).getIntValue());
    else if (span.contains("-"))
        view.selectSpan(span.upToFirstOccurrenceOf("-", false, false).getDoubleValue(),
                        span.fromFirstOccurrenceOf("-", false, false).getDoubleValue());
    const juce::String tag = tutorial ? "-tutorial-" : languagePanel ? "-linguagem-" : about ? "-sobre-" : authorise ? "-autorizar-" : page == 2 ? "-suaescuta-" : page == 1 ? "-biblioteca-" : span.isNotEmpty() ? "-trecho-" : "-";
    struct Size { int w, h; };
    const Size sizes[] = { { 960, 600 }, { 1024, 650 }, { 1366, 768 }, { 1920, 1080 }, { 2560, 1440 } };
    int failures = 0;
    for (const auto& s : sizes)
    {
        view.setSize(s.w, s.h);
        const auto image = view.createComponentSnapshot(view.getLocalBounds(), true, 1.0f);
        const auto file = out.getChildFile("trioio-" + langCode(view.language()) + tag + juce::String(s.w) + "x"
                                           + juce::String(s.h) + ".png");
        file.deleteFile();
        juce::FileOutputStream stream(file);
        juce::PNGImageFormat().writeImageToStream(image, stream);
        juce::String report = juce::String(s.w) + "x" + juce::String(s.h) + " (" + regimeName(view.regime()) + "): ";
        const auto essentials = view.essentials();
        int bad = 0;
        // com o SOBRE aberto, os controles ficam sob o véu: só a imagem conta
        for (std::size_t i = 0; i < (about || authorise || tutorial ? 0 : essentials.size()); ++i)
        {
            auto* c = essentials[i].second;
            const auto b = c->getBounds();
            juce::String why;
            if (!c->isVisible()) why = "invisível";
            else if (!view.getLocalBounds().contains(b)) why = "fora da tela";
            else if (b.getHeight() < Tokens::minTarget || b.getWidth() < Tokens::minTarget) why = "menor que 44 px";
            for (std::size_t k = 0; k < i && why.isEmpty(); ++k)
                if (b.intersects(essentials[k].second->getBounds()))
                    why = "sobre " + essentials[k].first;
            if (why.isNotEmpty())
            {
                report << essentials[i].first << " " << why << "; ";
                ++bad;
            }
        }
        const std::string tail = about || authorise ? "cartão aberto (só a imagem)"
                                 : bad == 0         ? "os " + std::to_string(essentials.size()) + " controles essenciais ok"
                                                    : "";
        std::printf("%s%s\n", report.toRawUTF8(), tail.c_str());
        failures += bad;
    }
    return failures == 0 ? 0 : 1;
}

class MainWindow : public juce::DocumentWindow, private juce::ChangeListener
{
public:
    MainWindow(juce::PropertiesFile& props, const juce::String& dataRoot)
        : DocumentWindow("Trioio", Tokens::background, DocumentWindow::allButtons), props_(props)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, false);
        auto* view = new MainView();
        setContentOwned(view, false);
        formats_.registerBasicFormats();
        // a leitura do disco fica numa thread própria (o callback de áudio só
        // lê do buffer de leitura antecipada: sem I/O na thread de áudio)
        readAhead_.startThread(juce::Thread::Priority::normal);
        const auto error = devices_.initialiseWithDefaultDevices(0, 2);
        devices_.addAudioCallback(&player_);
        // duas fontes misturadas: a resposta (transport_) e, no "▶ juntos", a
        // referência (transport2_), começando juntas
        mixer_.addInputSource(&transport_, false);
        mixer_.addInputSource(&transport2_, false);
        player_.setSource(&mixer_);
        transport_.addChangeListener(this);
        if (error.isNotEmpty())
            std::fprintf(stderr, "áudio: %s\n", error.toRawUTF8());
        // o idioma: o escolhido antes; na primeira abertura, o do sistema
        // (qualquer outro que não pt, fr, es cai no inglês)
        const auto savedLanguage = props_.getValue("idioma");
        view->setLanguage(langFromCode(savedLanguage.isNotEmpty() ? savedLanguage : juce::SystemStats::getUserLanguage()));
        view->onLanguageChanged = [this](Lang l) {
            props_.setValue("idioma", langCode(l));
            props_.saveIfNeeded();
        };
        view->onPlay = [this](const std::string& path) { play(path); };
        view->onPlayAt = [this](const std::string& path, double from) { play(path, {}, from); };
        // master: o ganho do transporte (vale para a resposta e a referência), guardado
        const double masterDb = props_.getDoubleValue("master", 0.0);
        view->setMasterDb(masterDb);
        masterGain_ = juce::Decibels::decibelsToGain(static_cast<float>(masterDb), -60.0f);
        applyGains();
        view->onMasterChanged = [this](double db) {
            masterGain_ = juce::Decibels::decibelsToGain(static_cast<float>(db), -60.0f);
            applyGains();
            props_.setValue("master", db);
            props_.saveIfNeeded();
        };
        view->onStop = [this] { transport_.stop(); transport2_.stop(); };
        // soltar os arquivos (antes de mover uma resposta para a lixeira)
        view->onRelease = [this] {
            ++playToken_;  // uma abertura em curso não volta a prender o arquivo
            transport_.stop();
            transport2_.stop();
            transport_.setSource(nullptr);
            transport2_.setSource(nullptr);
            reader_.reset();
            reader2_.reset();
            decoded_.reset();
            decoded2_.reset();
        };
        view->onPlayTogether = [this](const std::string& response, const std::string& reference) { play(response, reference); };
        view->playPosition = [this] { return transport_.isPlaying() ? transport_.getCurrentPosition() : -1.0; };
        view->onDataRootChanged = [this](const std::string& root) {
            props_.setValue("dados", juce::String::fromUTF8(root.c_str()));
            props_.saveIfNeeded();
        };
        view->setDeclarer(props_.getValue("declarante", juce::SystemStats::getFullUserName()));
        view->onDeclarerChanged = [this](const juce::String& name) {
            props_.setValue("declarante", name);
            props_.saveIfNeeded();
        };
        // sem raiz de dados: a tela vazia com o roteiro; a raiz é criada em
        // ~/Música/Trioio quando a primeira gravação chegar
        const auto root = dataRoot.isNotEmpty() ? dataRoot : props_.getValue("dados");
        if (root.isNotEmpty() && juce::File(root).isDirectory())
            view->openDataRoot(root.toStdString());
        else
            view->showEmpty();

        // a janela: o monitor e os limites salvos, revalidados; senão ~88 %
        // da área útil do monitor principal (WindowPolicy.h)
        std::vector<Rect> areas;
        for (const auto& d : juce::Desktop::getInstance().getDisplays().displays)
        {
            const auto a = d.userBounds.toNearestInt();
            areas.push_back({ a.getX(), a.getY(), a.getWidth(), a.getHeight() });
            if (d.isMain)
                std::swap(areas.front(), areas.back());
        }
        Rect saved;
        const auto s = juce::StringArray::fromTokens(props_.getValue("janela"), ",", "");
        if (s.size() == 4)
            saved = { s[0].getIntValue(), s[1].getIntValue(), s[2].getIntValue(), s[3].getIntValue() };
        const auto r = revalidate(saved, areas);
        const auto& main = areas.empty() ? Rect { 0, 0, kCompactW, kCompactH } : areas.front();
        setResizeLimits(std::min(kMinW, main.w), std::min(kMinH, main.h), 8192, 8192);
        setBounds(r.x, r.y, r.w, r.h);
        setVisible(true);
    }

    ~MainWindow() override
    {
        const auto b = getBounds();
        props_.setValue("janela", juce::String(b.getX()) + "," + juce::String(b.getY()) + "," + juce::String(b.getWidth()) + ","
                                      + juce::String(b.getHeight()));
        props_.saveIfNeeded();
        transport_.removeChangeListener(this);
        transport_.setSource(nullptr);
        transport2_.setSource(nullptr);
        player_.setSource(nullptr);
        mixer_.removeAllInputs();
        devices_.removeAudioCallback(&player_);
        readAhead_.stopThread(2000);
    }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

private:
    // a resposta (WAV) ou a referência (qualquer formato da Biblioteca: o
    // JUCE, ou o ffmpeg para M4A/AAC e outros — AudioSource.h); com `second`,
    // as duas juntas ("▶ juntos": a referência e a resposta, só para ouvir —
    // nada é gravado). Abrir pode decodificar: fora da thread da tela.
    void play(const std::string& path, const std::string& second = {}, double from = 0.0)
    {
        transport_.stop();
        transport2_.stop();
        transport_.setSource(nullptr);
        transport2_.setSource(nullptr);
        reader_.reset();
        reader2_.reset();
        decoded_.reset();
        decoded2_.reset();
        together_ = !second.empty();
        starting_ = true;  // a parada acima avisa a tela depois: não é o fim da reprodução
        applyGains();
        const auto token = ++playToken_;
        juce::Component::SafePointer<MainWindow> self(this);
        const juce::File file(juce::String::fromUTF8(path.c_str()));
        const juce::File file2(juce::String::fromUTF8(second.c_str()));
        const bool two = !second.empty();
        juce::Thread::launch([self, file, file2, two, token, from] {
            auto opened = std::make_shared<library::OpenedAudio>(library::openAudio(file, 1800.0, 120000));
            auto opened2 = std::make_shared<library::OpenedAudio>();
            if (two)
                *opened2 = library::openAudio(file2, 1800.0, 120000);
            juce::MessageManager::callAsync([self, opened, opened2, two, token, from] {
                if (!self || token != self->playToken_)
                    return;
                self->starting_ = false;
                if (!opened->ok() || (two && !opened2->ok()))
                {
                    std::fprintf(stderr, "áudio: %s\n", (!opened->ok() ? opened->error : opened2->error).c_str());
                    if (auto* v = dynamic_cast<MainView*>(self->getContentComponent()))
                        v->setPlaying(false);
                    return;
                }
                const double sr = opened->reader->sampleRate;
                self->decoded_ = std::move(opened->decoded);
                self->reader_ = std::make_unique<juce::AudioFormatReaderSource>(opened->reader.release(), true);
                self->transport_.setSource(self->reader_.get(), kReadAhead, &self->readAhead_, sr);
                self->transport_.setPosition(std::max(0.0, from));
                if (two)
                {
                    const double sr2 = opened2->reader->sampleRate;
                    self->decoded2_ = std::move(opened2->decoded);
                    self->reader2_ = std::make_unique<juce::AudioFormatReaderSource>(opened2->reader.release(), true);
                    self->transport2_.setSource(self->reader2_.get(), kReadAhead, &self->readAhead_, sr2);
                    self->transport2_.setPosition(0.0);
                    self->transport2_.start();
                }
                self->transport_.start();
            });
        });
    }
    // o master nas duas fontes; juntas, cada uma a −3 dB (a soma não estoura)
    void applyGains()
    {
        const float g = masterGain_ * (together_ ? 0.708f : 1.0f);
        transport_.setGain(g);
        transport2_.setGain(g);
    }
    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        if (starting_)
            return;  // a troca de fonte (alternar A ↔ B) não é o fim
        if (!transport_.isPlaying())
            transport2_.stop();
        if (!transport_.isPlaying())
            if (auto* v = dynamic_cast<MainView*>(getContentComponent()))
                v->setPlaying(false);
    }

    static constexpr int kReadAhead = 1 << 16;  // ~1,4 s a 48 kHz lidos antes, fora do callback
    juce::PropertiesFile& props_;
    juce::TimeSliceThread readAhead_ { "Trioio: leitura" };
    juce::AudioDeviceManager devices_;
    juce::AudioFormatManager formats_;
    juce::AudioSourcePlayer player_;
    juce::MixerAudioSource mixer_;
    juce::AudioTransportSource transport_, transport2_;
    std::unique_ptr<juce::AudioFormatReaderSource> reader_, reader2_;
    std::unique_ptr<juce::TemporaryFile> decoded_, decoded2_;  // o WAV do ffmpeg, enquanto toca
    float masterGain_ = 1.0f;
    bool together_ = false, starting_ = false;
    int playToken_ = 0;
};
} // namespace

class TrioioApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Trioio"; }
    const juce::String getApplicationVersion() override { return TRIOIO_VERSION; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override
    {
        const auto args = juce::StringArray::fromTokens(commandLine, true);
        juce::String dataRoot;
        if (const int i = args.indexOf("--dados"); i >= 0 && i + 1 < args.size())
            dataRoot = args[i + 1].unquoted();
        if (const int i = args.indexOf("--instantaneos"); i >= 0 && i + 1 < args.size())
        {
            juce::String ref;
            if (const int k = args.indexOf("--referencia"); k >= 0 && k + 1 < args.size())
                ref = args[k + 1].unquoted();
            juce::String idioma;
            if (const int k = args.indexOf("--idioma"); k >= 0 && k + 1 < args.size())
                idioma = args[k + 1].unquoted();
            juce::String span;
            if (const int k = args.indexOf("--trecho"); k >= 0 && k + 1 < args.size())
                span = args[k + 1].unquoted();
            if (const int k = args.indexOf("--secao"); k >= 0 && k + 1 < args.size())
                span = "secao:" + args[k + 1].unquoted();
            setApplicationReturnValue(snapshots(juce::File(args[i + 1].unquoted()), dataRoot, ref, args.contains("--revelar"), idioma,
                                                args.contains("--sobre"), span, args.contains("--autorizar"),
                                                args.contains("--pagina") ? args[args.indexOf("--pagina") + 1].getIntValue() : -1,
                                                args.contains("--linguagem"), args.contains("--tutorial")));
            quit();
            return;
        }
        juce::PropertiesFile::Options o;
        o.applicationName = "Trioio";
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
        props_ = std::make_unique<juce::PropertiesFile>(o);
        window_ = std::make_unique<MainWindow>(*props_, dataRoot);
    }

    void shutdown() override
    {
        window_.reset();
        props_.reset();
    }
    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<juce::PropertiesFile> props_;
    std::unique_ptr<MainWindow> window_;
};

} // namespace trioio::app

START_JUCE_APPLICATION(trioio::app::TrioioApplication)
