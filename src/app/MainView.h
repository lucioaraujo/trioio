#pragma once

#include <atomic>
#include <memory>

// A tela do Trioio (docs/design.md): "Escuta e resposta" com as três faixas
// (referência, Terceiro Olho, resposta), a Biblioteca, a Linguagem e a
// barra de julgamento, nos regimes completo, compacto e emergência. Cada
// regime recalcula as áreas (sem zoom); alvos de pelo menos 44 px.
//
// Identidade visual PROVISÓRIA (design.md §6, `exploration`): as cores são
// tokens daqui (Tokens), a fonte é a padrão do JUCE. A direção visual é do
// autor; trocar a paleta é trocar os tokens.

#include "AppModel.h"
#include "UiLanguage.h"
#include "WindowPolicy.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace trioio::app
{

// u8: todo texto visível não ASCII passa por aqui (regra UTF-8 do RASGO)
inline juce::String u8(const char* s) { return juce::String(juce::CharPointer_UTF8(s)); }
inline juce::String u8(const std::string& s) { return juce::String::fromUTF8(s.data(), static_cast<int>(s.size())); }

namespace Tokens
{
inline const juce::Colour background { 0xff0e1014 }, surface { 0xff161920 }, recessed { 0xff11131a }, line { 0xff272b35 },
    text { 0xffece8de }, text2 { 0xffa3a9b6 }, ref { 0xff8e9bb0 }, eye { 0xffe2a23b }, eyeBg { 0xff19160f },
    response { 0xff4fb3a5 }, responseBg { 0xff10201e }, responseBlock { 0xff14302d }, warn { 0xffd9774a };
inline constexpr int minTarget = 44;
inline constexpr float bodyFont = 14.0f, smallFont = 13.0f, titleFont = 18.0f;
} // namespace Tokens

class ReferenceTrack;
class EyeTrack;
class ResponseTrack;
class GestureEditor;

class MainView : public juce::Component, private juce::ListBoxModel
{
public:
    MainView();
    ~MainView() override;

    // os dados: uma raiz de dados real, ou a demonstração (instantâneos)
    void openDataRoot(const std::string& root);
    void showDemo();
    // a primeira abertura, sem raiz de dados: a tela vazia com o roteiro
    // (trazer gravação → autorizar → compor → ouvir e julgar)
    void showEmpty();
    // a aplicação guarda a raiz escolhida e o nome de quem declara
    std::function<void(const std::string& root)> onDataRootChanged;
    void setDeclarer(const juce::String& name) { declarer_ = name; }
    std::function<void(const juce::String&)> onDeclarerChanged;
    // onde a raiz de dados é criada quando o performer traz a primeira gravação
    juce::File defaultDataRoot;

    // o transporte é da aplicação (dispositivo de áudio); a tela só pede
    std::function<void(const std::string& wavPath)> onPlay;
    // tocar a partir de um instante (o "alternar A ↔ B" da Sua escuta)
    std::function<void(const std::string& wavPath, double fromSeconds)> onPlayAt;
    std::function<void()> onStop;
    // "▶ juntos": a resposta e a referência ao mesmo tempo (só para ouvir)
    std::function<void(const std::string& response, const std::string& reference)> onPlayTogether;
    std::function<double()> playPosition;  // segundos, < 0 = parado
    void setPlaying(bool playing);
    // o volume master, em dB (−60 = mudo … +6); a aplicação aplica e guarda
    void setMasterDb(double db) { master_.setValue(db, juce::dontSendNotification); master_.updateText(); }
    std::function<void(double db)> onMasterChanged;
    // o idioma da interface (UiLanguage.h); a aplicação guarda a escolha
    void setLanguage(Lang l);
    [[nodiscard]] Lang language() const noexcept { return lang_; }
    std::function<void(Lang)> onLanguageChanged;
    void showAbout(bool show);
    void showTutorial(bool show);
    // lê a referência selecionada na hora (os instantâneos não têm o laço de mensagens)
    void loadReferenceNow();
    // os instantâneos: escolher a referência pelo id e revelar o olho
    void selectReferenceById(const std::string& id);
    void revealEye(bool reveal);
    // os instantâneos: marcar um trecho [a, b) em segundos, ou a seção N (1…), como o arrasto/clique faria
    void selectSpan(double a, double b, int section = 0);
    // os instantâneos: o cartão de autorização da referência escolhida
    void openAuthorisation() { showAuth(true); }
    // os instantâneos: abrir uma página (0 escuta, 1 biblioteca, 2 sua escuta)
    void openPage(int page) { setPage(page); }
    void openLanguage() { drawerButton_.setToggleState(true, juce::dontSendNotification); drawerOpen_ = true; resized(); }

    void paint(juce::Graphics&) override;
    void resized() override;

    [[nodiscard]] Regime regime() const noexcept { return regime_; }
    // os controles essenciais (design.md §5: nunca somem), para o teste de geometria
    [[nodiscard]] std::vector<std::pair<juce::String, juce::Component*>> essentials();

private:
    // ListBoxModel (Biblioteca)
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged(int lastRowSelected) override;

    void selectReference(int index);
    void bringRecording();                 // escolher o arquivo, registrar, pedir a autorização
    bool ensureDataRoot();                 // cria a raiz padrão se não houver
    void reloadReferences(const std::string& selectId);
    void showAuth(bool show);
    void updateLibraryButtons();
    void toggleReferencePlayback();
    void togglePlayTogether();
    void showResponse(int index);
    void judge(int verdict);  // 0 aceitar, 1 rejeitar, 2 transformar; o trecho ou a seção marcados na resposta
    [[nodiscard]] juce::String judgeTarget() const;  // " · trecho 1:10–1:45", " · seção 3" ou vazio
    void updateJudgeLabel();
    void composeAgain();
    void setStatus(const juce::String& s);
    void setPage(int page);   // 0 escuta, 1 biblioteca (compacto/emergência), 2 sua escuta (todos)
    void refreshListeningPage();
    void refreshReferenceTexts();  // o seletor, a leitura e a ficha no idioma atual
    void applyLanguage();     // todos os textos no idioma atual
    void layoutTracks(juce::Rectangle<int> area);
    void layoutLanguage(juce::Rectangle<int> area);
    void layoutFooter(juce::Rectangle<int> area, bool compact);

    AppModel model_;
    std::vector<ReferenceItem> refs_;
    std::vector<ResponseItem> responses_;
    int refIndex_ = -1, responseIndex_ = -1;
    Regime regime_ = Regime::compact;
    int page_ = 0;
    bool drawerOpen_ = false;
    int mode_ = 0;
    bool demo_ = false;
    Lang lang_ = Lang::en;
    juce::TextButton langButton_, aboutButton_, tutorialButton_;
    juce::Label creditLabel_;
    juce::TooltipWindow tooltips_ { this, 700 };
    std::unique_ptr<juce::Component> about_, auth_, tutorial_;
    juce::TextButton bringButton_, authButton_, bringBigButton_, refPlayButton_, togetherButton_;
    juce::String declarer_;
    bool playingReference_ = false, playingTogether_ = false;
    std::unique_ptr<juce::Component> listeningPage_;
    bool useListening_ = true;
    int versionA_ = -1, versionB_ = -1, playingVersion_ = -1;  // a comparação A × B
    void compareVersions();
    void updateVersionMarks();
    void switchVersions();
    bool importing_ = false;

    juce::Label title_;
    juce::TextButton refButton_, menuButton_, drawerButton_;
    juce::TextButton modeButtons_[4];
    juce::Label readingLabel_;
    juce::TextButton pageButtons_[3];  // escuta e resposta, Biblioteca, Sua escuta
    std::unique_ptr<ReferenceTrack> refTrack_;
    std::unique_ptr<EyeTrack> eyeTrack_;
    std::unique_ptr<ResponseTrack> responseTrack_;
    std::unique_ptr<GestureEditor> gestureEditor_;  // o gesto desenhado (o modo híbrido)
    juce::Component tracksHolder_;
    juce::Viewport tracksViewport_;
    juce::TextButton revealButton_;
    juce::ComboBox responsePicker_;

    juce::ListBox library_;
    juce::Label libraryTitle_, libraryCard_;
    juce::TextButton openButton_;

    struct Panel : juce::Component
    {
        void paint(juce::Graphics& g) override
        {
            g.fillAll(Tokens::surface);
            g.setColour(Tokens::line);
            g.drawVerticalLine(0, 0.0f, static_cast<float>(getHeight()));
        }
    } languagePanel_;
    // o painel rola quando a tela é baixa (2026-10-15: com o conjunto, a
    // caixa do jogo saía da tela em 1366×768)
    juce::Viewport languageScroll_;
    juce::Component languageContent_;
    juce::Label languageTitle_, languageNote_;
    juce::Label mixLabel_, pitchLabel_, techniqueLabel_, seamLabel_, cycleLabel_, voicingLabel_, intentionLabel_,
        formLabel_, tempoLabel_, unpredLabel_, ensembleLabel_, rasgoLabel_, harmonyLangLabel_, gestureLabel_, influencesLabel_,
        influenceStrengthLabel_, excludeLabel_, wordsLabel_;
    juce::TextEditor mix_, gesture_;  // gesture_: o gesto do modo híbrido, posição:intensidade
    juce::TextEditor influences_, exclude_;
    juce::TextEditor words_;  // as palavras da resposta (o prompt opcional)  // a árvore de influências (core/Influences.h) e a exclusão do autor
    juce::Slider influenceStrength_;
    juce::ComboBox pitch_, technique_, seam_, cycle_, voicing_, intention_, form_, tempo_, ensemble_, harmonyLang_;
    juce::Slider unpredictability_, rasgo_;
    juce::ToggleButton semantic_;  // a ponte semântica (library/SemanticBridge.h): desligada por padrão
    juce::Label semanticStatus_;
    int semanticGeneration_ = 0;   // a consulta enquanto se escreve: só a última vale
    std::shared_ptr<std::atomic<bool>> semanticBusy_ = std::make_shared<std::atomic<bool>>(false);  // uma consulta por vez
    void prefetchSemantic();
    juce::ToggleButton dialogue_, game_, filmFx_, somDaAura_, stems_;  // o diálogo (core/Dialogue.h): responder nas pausas da referência

    juce::Slider master_;
    juce::TextButton playButton_, acceptButton_, transformButton_, rejectButton_, composeButton_;
    juce::Label timeLabel_, judgeLabel_, statusLabel_;
    // a barra do trabalho em andamento (o autor, 2026-10-18: "pra não dar
    // impressão que travou"): a etapa da resposta e o tempo decorrido
    double progress_ = -1.0;
    juce::ProgressBar progressBar_ { progress_ };
    juce::String progressStage_;
    double composeStartMs_ = 0.0;
    juce::TimedCallback progressClock_ { [this] { updateProgressText(); } };
    void updateProgressText();
    void showProgress(bool on);
    juce::TextEditor note_;
    bool playing_ = false;

    struct Timer : juce::Timer
    {
        std::function<void()> fn;
        void timerCallback() override { fn(); }
    } timer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainView)
};

} // namespace trioio::app
