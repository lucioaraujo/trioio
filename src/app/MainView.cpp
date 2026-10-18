#include "MainView.h"

#include "Dramaturgy.h"

#include "Declaration.h"
#include "LibraryImporter.h"
#include "ListeningStore.h"
#include "Response.h"
#include "SemanticBridge.h"
#include "Tutorial.h"
#include "Intent.h"
#include "ThirdEye.h"

#include <juce_cryptography/juce_cryptography.h>

#include <cmath>

namespace trioio::app
{

namespace
{
// As teclas mortas (o til, os acentos) no Linux: o TextEditor do JUCE não as
// compõe (sem método de entrada), e "baião" chegava como "bai~ao" (o autor,
// 2026-10-18). O acento sozinho seguido da letra vira a letra acentuada,
// enquanto se digita; o cursor fica no lugar.
juce::String composeDeadKeys(const juce::String& in, int& caret)
{
    struct Row { juce::juce_wchar mark; const char* from; const char* to; };
    static const Row rows[] = {
        { '~', "aonAON", "ãõñÃÕÑ" },
        { 0x00B4, "aeiouAEIOUcC", "áéíóúÁÉÍÓÚçÇ" },  // ´
        { '`', "aeiouAEIOU", "àèìòùÀÈÌÒÙ" },
        { '^', "aeiouAEIOU", "âêîôûÂÊÎÔÛ" },
        { 0x00A8, "aeiouAEIOU", "äëïöüÄËÏÖÜ" },  // ¨
    };
    juce::String out;
    int newCaret = caret;
    for (int i = 0; i < in.length(); ++i)
    {
        const auto c = in[i];
        if (i + 1 < in.length())
            for (const auto& r : rows)
                if (c == r.mark)
                {
                    const int k = juce::String(r.from).indexOfChar(in[i + 1]);
                    if (k >= 0)
                    {
                        out << juce::String(juce::CharPointer_UTF8(r.to))[k];
                        if (i < caret)
                            --newCaret;
                        ++i;
                        goto next;
                    }
                }
        out << juce::String::charToString(c);
    next:;
    }
    caret = newCaret;
    return out;
}

void attachDeadKeys(juce::TextEditor& e)
{
    const auto previous = e.onTextChange;
    e.onTextChange = [&e, previous] {
        int caret = e.getCaretPosition();
        const auto fixed = composeDeadKeys(e.getText(), caret);
        if (fixed != e.getText())
        {
            e.setText(fixed, false);
            e.setCaretPosition(caret);
        }
        if (previous)
            previous();
    };
}

juce::String clock(double s)
{
    if (s < 0.0)
        s = 0.0;
    return juce::String(static_cast<int>(s) / 60) + ":" + juce::String(static_cast<int>(s) % 60).paddedLeft('0', 2);
}

float xOf(double t, double duration, juce::Rectangle<float> r)
{
    return r.getX() + static_cast<float>(duration > 0.0 ? t / duration : 0.0) * r.getWidth();
}

void caption(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& left, const juce::String& right, juce::Colour c)
{
    g.setColour(c);
    g.setFont(juce::FontOptions(Tokens::smallFont));
    g.drawText(left, r, juce::Justification::topLeft, true);
    g.setColour(Tokens::text2);
    g.drawText(right, r, juce::Justification::topRight, true);
}
} // namespace

// ---- as três faixas -----------------------------------------------------------

class ReferenceTrack : public juce::Component
{
public:
    Lang lang = Lang::en;
    ReferenceView view;
    double duration = 1.0;
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour(Tokens::surface);
        g.fillRoundedRectangle(r.toFloat(), 8.0f);
        r.reduce(14, 10);
        juce::String info;
        if (view.ok)
            info = juce::String(view.bpm, 1) + " BPM · " + u8(view.key) + (view.language.empty() ? "" : u8(" · ") + u8(view.language));
        caption(g, r.removeFromTop(18), tr(S::refCaption, lang), info, Tokens::ref);
        if (!view.ok)
        {
            g.setColour(Tokens::text2);
            if (view.problem == "guide")
            {
                // o roteiro da primeira abertura; o botão "trazer gravação…" fica embaixo
                g.setFont(juce::FontOptions(Tokens::bodyFont));
                g.drawFittedText(tr(S::guide, lang), r.withTrimmedBottom(Tokens::minTarget + 6), juce::Justification::centredLeft, 6);
                return;
            }
            g.drawText(view.problem.empty() ? tr(S::refEmpty, lang) : view.problem == "…" ? tr(S::listening, lang) : u8(view.problem), r,
                       juce::Justification::centred);
            return;
        }
        auto chords = r.removeFromBottom(std::min(24, r.getHeight() / 3)).toFloat();
        auto env = r.reduced(0, 4).toFloat();
        juce::Path p;
        for (std::size_t i = 0; i < view.energy.size(); ++i)
        {
            const float x = xOf(static_cast<double>(i) * view.energyStep, duration, env);
            const float y = env.getBottom() - view.energy[i] * env.getHeight();
            i == 0 ? p.startNewSubPath(x, y) : p.lineTo(x, y);
        }
        g.setColour(Tokens::ref);
        g.strokePath(p, juce::PathStrokeType(1.5f));
        g.setFont(juce::FontOptions(12.0f));
        for (const auto& c : view.chords)
        {
            const float x0 = xOf(c.start, duration, chords), x1 = xOf(c.end, duration, chords);
            const auto box = juce::Rectangle<float>(x0, chords.getY(), std::max(1.0f, x1 - x0 - 1.0f), chords.getHeight());
            g.setColour(juce::Colour(0xff222735));
            g.fillRoundedRectangle(box, 2.0f);
            if (box.getWidth() > 34.0f)
            {
                g.setColour(juce::Colour(0xffc8cfdb));
                g.drawText(u8(c.chord.name()), box.reduced(4.0f, 0.0f), juce::Justification::centredLeft, false);
            }
        }
    }
};

class EyeTrack : public juce::Component
{
public:
    Lang lang = Lang::en;
    ResponseView view;
    double duration = 1.0;
    bool revealed = false;
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour(Tokens::eyeBg);
        g.fillRoundedRectangle(r.toFloat(), 8.0f);
        g.setColour(juce::Colour(0xff5c4a22));
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 8.0f, 1.0f);
        r.reduce(14, 10);
        caption(g, r.removeFromTop(18), revealed ? tr(S::eyeRevealed, lang) : tr(S::eyeHidden, lang),
                {}, Tokens::eye);
        auto lane = r.toFloat();
        if (!revealed)
        {
            g.saveState();
            g.reduceClipRegion(lane.toNearestInt());
            g.setColour(juce::Colour(0xff2a2312));
            for (float x = lane.getX() - lane.getHeight(); x < lane.getRight(); x += 12.0f)
                g.drawLine(x, lane.getBottom(), x + lane.getHeight(), lane.getY(), 5.0f);
            g.restoreState();
            return;
        }
        const float boxH = std::min(22.0f, lane.getHeight() * 0.45f);
        g.setFont(juce::FontOptions(12.0f));
        int stayIndex = 0;
        // as consequências que ficam, em ordem: cada uma ocupa uma altura e o
        // texto vai até onde começa a seguinte
        std::vector<double> stayStarts;
        for (const auto& e : view.eye)
            if (!e.stays.empty())
                stayStarts.push_back(e.time);
        for (const auto& e : view.eye)
        {
            const float x0 = xOf(e.time, duration, lane), x1 = xOf(e.time + std::max(e.duration, 0.5), duration, lane);
            const auto box = juce::Rectangle<float>(x0, lane.getY(), std::max(4.0f, x1 - x0), boxH);
            g.setColour(e.stays.empty() ? juce::Colour(0xff3a2f17) : Tokens::eye);
            g.fillRoundedRectangle(box, 3.0f);
            if (!e.stays.empty())
            {
                // a consequência segue até o fim, cada uma na sua altura
                const int levels = std::max(1, static_cast<int>((lane.getHeight() - boxH) / 16.0f));
                const float y = lane.getY() + boxH + 10.0f + static_cast<float>(stayIndex % levels) * 16.0f;
                const double nextStart = static_cast<std::size_t>(stayIndex) + 1 < stayStarts.size()
                                             ? stayStarts[static_cast<std::size_t>(stayIndex) + 1] : duration;
                ++stayIndex;
                juce::Path p;
                p.startNewSubPath(box.getRight(), box.getCentreY());
                p.cubicTo(box.getRight() + 20.0f, box.getCentreY(), box.getRight() + 20.0f, y, box.getRight() + 40.0f, y);
                p.lineTo(lane.getRight(), y);
                g.setColour(Tokens::eye);
                g.strokePath(p, juce::PathStrokeType(2.0f));
                g.setColour(juce::Colour(0xffd8b57a));
                // o texto só onde cabe até a consequência seguinte (≥ 120 px)
                const float textLeft = box.getRight() + 44.0f;
                const float textRight = std::min(levels > 1 ? lane.getRight() : xOf(nextStart, duration, lane), lane.getRight());
                if (textRight - textLeft >= 120.0f)
                    g.drawText(tr(S::stays, lang) + u8(e.stays), juce::Rectangle<float>(textLeft, y - 15.0f, textRight - textLeft - 4.0f, 14.0f),
                               juce::Justification::centredLeft, true);
            }
        }
    }
};

class ResponseTrack : public juce::Component
{
public:
    Lang lang = Lang::en;
    ResponseView view;
    double duration = 1.0, playhead = -1.0;
    // julgamento por trecho (docs/ESCUTA.md): arrastar marca [selStart, selEnd);
    // clicar num bloco escolhe a seção (selSection = número, 1…); clicar fora
    // da seleção volta à resposta inteira
    double selStart = -1.0, selEnd = -1.0;
    int selSection = 0;
    std::function<void()> onSelection;
    [[nodiscard]] bool hasSelection() const noexcept { return selEnd > selStart && selStart >= 0.0; }
    void clearSelection()
    {
        selStart = selEnd = -1.0;
        selSection = 0;
        repaint();
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        dragFrom_ = timeAt(static_cast<float>(e.x));
        dragging_ = false;
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!view.ok || dragFrom_ < 0.0 || e.getDistanceFromDragStart() < 6)
            return;
        dragging_ = true;
        const double t = timeAt(static_cast<float>(e.x));
        selStart = std::min(dragFrom_, t);
        selEnd = std::max(dragFrom_, t);
        selSection = 0;
        repaint();
    }
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (!view.ok)
            return;
        if (dragging_)
        {
            // menos de um segundo não é trecho que se julgue
            if (selEnd - selStart < 1.0)
                clearSelection();
        }
        else
        {
            const double t = timeAt(static_cast<float>(e.x));
            int hit = 0;
            if (secsRect_.contains(e.position))
                for (std::size_t i = 0; i < view.sections.size(); ++i)
                    if (t >= view.sections[i].start && t < view.sections[i].end)
                        hit = static_cast<int>(i) + 1;
            if (hit > 0 && hit != selSection)
            {
                selSection = hit;
                selStart = view.sections[static_cast<std::size_t>(hit - 1)].start;
                selEnd = view.sections[static_cast<std::size_t>(hit - 1)].end;
            }
            else
                selStart = selEnd = -1.0, selSection = 0;
            repaint();
        }
        dragging_ = false;
        if (onSelection)
            onSelection();
    }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour(Tokens::responseBg);
        g.fillRoundedRectangle(r.toFloat(), 8.0f);
        r.reduce(14, 10);
        juce::String scaleLine;
        for (const auto& h : view.header)
            if (h.rfind("escala do arquivo: ", 0) == 0 || h.rfind("escala da mistura: ", 0) == 0)
                scaleLine = u8(h).upToFirstOccurrenceOf(" · fonte", false, false).fromFirstOccurrenceOf(": ", false, false);
        for (const auto& h : view.header)
            if (h.rfind("sayr: ", 0) == 0)
                scaleLine << tr(S::sayrWith, lang) << u8(h).fromFirstOccurrenceOf(" alternam com ", false, false).upToFirstOccurrenceOf(" (", false, false);
        caption(g, r.removeFromTop(18), tr(S::respCaption, lang), scaleLine, Tokens::response);
        if (!view.ok)
        {
            g.setColour(Tokens::text2);
            g.drawText(view.problem.empty() ? tr(S::respEmpty, lang) : u8(view.problem), r,
                       juce::Justification::centred);
            return;
        }
        auto secs = r.removeFromTop(std::min(46, r.getHeight() / 3)).toFloat();
        secsRect_ = secs;
        timeArea_ = r.toFloat().withY(secs.getY()).withBottom(static_cast<float>(r.getBottom()));
        r.removeFromTop(6);
        g.setFont(juce::FontOptions(12.0f));
        for (std::size_t i = 0; i < view.sections.size(); ++i)
        {
            const auto& s = view.sections[i];
            const float x0 = xOf(s.start, duration, secs), x1 = xOf(s.end, duration, secs);
            const auto box = juce::Rectangle<float>(x0, secs.getY(), std::max(2.0f, x1 - x0 - 3.0f), secs.getHeight());
            g.setColour(Tokens::responseBlock);
            g.fillRoundedRectangle(box, 5.0f);
            if (box.getWidth() > 40.0f)
            {
                g.setColour(Tokens::text);
                const juce::String letter = juce::String::charToString(static_cast<juce::juce_wchar>('A' + static_cast<int>(i % 26)));
                g.drawText(letter + "  " + u8(s.label), box.reduced(6.0f, 4.0f), juce::Justification::topLeft, true);
            }
        }
        auto roll = r.toFloat();
        float lo = 127.0f, hi = 0.0f;
        for (const auto& n : view.notes)
            if (n.voice != 4)
            {
                lo = std::min(lo, n.pitch);
                hi = std::max(hi, n.pitch);
            }
        if (hi < lo)
            return;
        lo -= 2.0f;
        hi += 2.0f;
        const float noteH = std::clamp(roll.getHeight() / (hi - lo), 2.0f, 7.0f);
        for (const auto& n : view.notes)
        {
            if (n.voice == 4)
                continue;
            const float x0 = xOf(n.time, duration, roll), x1 = xOf(n.time + n.duration, duration, roll);
            const float y = roll.getBottom() - (n.pitch - lo) / (hi - lo) * roll.getHeight();
            g.setColour(n.voice == 0 ? Tokens::response : n.voice == 1 ? juce::Colour(0xff2e6f68) : juce::Colour(0x664fb3a5));
            g.fillRoundedRectangle(x0, y - noteH * 0.5f, std::max(1.5f, x1 - x0), noteH, 1.5f);
        }
        if (hasSelection())
        {
            const float x0 = xOf(selStart, duration, roll), x1 = xOf(selEnd, duration, roll);
            const juce::Rectangle<float> band(x0, secs.getY(), std::max(2.0f, x1 - x0), roll.getBottom() - secs.getY());
            g.setColour(Tokens::text.withAlpha(0.08f));
            g.fillRect(band);
            g.setColour(Tokens::text.withAlpha(0.7f));
            g.drawRect(band, 1.5f);
        }
        if (playhead >= 0.0)
        {
            const float x = xOf(playhead, duration, roll);
            g.setColour(Tokens::text);
            g.drawLine(x, secs.getY(), x, roll.getBottom(), 1.5f);
        }
    }

private:
    double timeAt(float x) const
    {
        if (timeArea_.getWidth() <= 0.0f)
            return -1.0;
        const double f = std::clamp((x - timeArea_.getX()) / timeArea_.getWidth(), 0.0f, 1.0f);
        return std::min(f * duration, view.duration);
    }
    juce::Rectangle<float> secsRect_, timeArea_;
    double dragFrom_ = -1.0;
    bool dragging_ = false;
};

// ---- AUTORIZAR: de onde vem a gravação e o que o Trioio pode fazer com ela ----------
// (docs/MANIFESTO_DE_PROVENIENCIA.md). Só os campos que a origem e o
// fundamento pedem ficam visíveis; o que faltar volta de validate() e
// aparece no cartão, sem fechar.

class AuthOverlay : public juce::Component
{
public:
    std::function<void(const library::AuthorizationRequest&)> onAuthorise;
    std::function<void()> onRefuse, onClose;
    juce::Label title, file, originL, basisL, toolL, licenceL, evidenceL, byL, problems;
    juce::ComboBox origin, basis;
    juce::TextEditor tool, licence, evidence, by;
    juce::ToggleButton derive, publish;
    juce::TextButton ok, refuseB, cancel;
    std::string id;
    Lang lang = Lang::en;

    AuthOverlay()
    {
        title.setFont(juce::FontOptions(Tokens::titleFont, juce::Font::bold));
        title.setColour(juce::Label::textColourId, Tokens::text);
        file.setColour(juce::Label::textColourId, Tokens::ref);
        problems.setColour(juce::Label::textColourId, Tokens::warn);
        problems.setJustificationType(juce::Justification::topLeft);
        for (auto* l : { &title, &file, &originL, &basisL, &toolL, &licenceL, &evidenceL, &byL, &problems })
            addAndMakeVisible(l);
        for (auto* l : { &originL, &basisL, &toolL, &licenceL, &evidenceL, &byL })
        {
            l->setFont(juce::FontOptions(Tokens::smallFont));
            l->setColour(juce::Label::textColourId, Tokens::text2);
        }
        for (auto* e : { &tool, &licence, &evidence, &by })
        {
            e->setFont(juce::FontOptions(Tokens::bodyFont));
            addAndMakeVisible(e);
        }
        addAndMakeVisible(origin);
        addAndMakeVisible(basis);
        derive.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(derive);
        addAndMakeVisible(publish);
        origin.onChange = [this] {
            // o fundamento mais provável para a origem; o performer pode trocar
            const int byOrigin[] = { 1, 4, 1, 5, 2, 3 };  // obra própria, permissão, própria, termos, licença, domínio
            const int o = origin.getSelectedItemIndex();
            if (o >= 0)
                basis.setSelectedId(byOrigin[o], juce::dontSendNotification);
            refreshFields();
        };
        basis.onChange = [this] { refreshFields(); };
        ok.setColour(juce::TextButton::buttonColourId, Tokens::responseBlock);
        ok.onClick = [this] { submit(); };
        refuseB.onClick = [this] { if (onRefuse) onRefuse(); };
        cancel.onClick = [this] { if (onClose) onClose(); };
        for (auto* b : { &ok, &refuseB, &cancel })
            addAndMakeVisible(b);
        setWantsKeyboardFocus(true);
    }

    void open(const std::string& refId, const juce::String& name, const juce::String& declarer, Lang l)
    {
        id = refId;
        lang = l;
        title.setText(tr(S::authTitle, l), juce::dontSendNotification);
        file.setText(name, juce::dontSendNotification);
        originL.setText(tr(S::originLabel, l), juce::dontSendNotification);
        basisL.setText(tr(S::basisLabel, l), juce::dontSendNotification);
        toolL.setText(tr(S::toolLabel, l), juce::dontSendNotification);
        licenceL.setText(tr(S::licenceLabel, l), juce::dontSendNotification);
        evidenceL.setText(tr(S::evidenceLabel, l), juce::dontSendNotification);
        byL.setText(tr(S::byLabel, l), juce::dontSendNotification);
        derive.setButtonText(tr(S::deriveToggle, l));
        publish.setButtonText(tr(S::publishToggle, l));
        ok.setButtonText(tr(S::authorise, l));
        refuseB.setButtonText(tr(S::refuse, l));
        cancel.setButtonText(tr(S::notNow, l));
        origin.clear(juce::dontSendNotification);
        for (int i = 0; i < 6; ++i)
            origin.addItem(tr(S::originItems[i], l), i + 1);
        basis.clear(juce::dontSendNotification);
        for (int i = 0; i < 5; ++i)
            basis.addItem(tr(S::basisItems[i], l), i + 1);
        origin.setSelectedId(1, juce::dontSendNotification);
        basis.setSelectedId(1, juce::dontSendNotification);
        tool.clear();
        licence.clear();
        evidence.clear();
        by.setText(declarer, false);
        derive.setToggleState(true, juce::dontSendNotification);
        publish.setToggleState(false, juce::dontSendNotification);
        problems.setText({}, juce::dontSendNotification);
        refreshFields();
    }

    void showProblems(const std::vector<std::string>& list)
    {
        juce::String t;
        for (const auto& p : list)
            t << u8("• ") << u8(p) << "\n";
        problems.setText(t.trim(), juce::dontSendNotification);
    }

    bool keyPressed(const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey && onClose)
        {
            onClose();
            return true;
        }
        return false;
    }

    juce::Rectangle<int> card() const
    {
        return getLocalBounds().withSizeKeepingCentre(std::min(640, getWidth() - 24), std::min(560, getHeight() - 24));
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.62f));
        g.setColour(Tokens::surface);
        g.fillRoundedRectangle(card().toFloat(), 10.0f);
        g.setColour(Tokens::line);
        g.drawRoundedRectangle(card().toFloat(), 10.0f, 1.0f);
    }
    void resized() override
    {
        auto r = card().reduced(22, 16);
        title.setBounds(r.removeFromTop(28));
        file.setBounds(r.removeFromTop(20));
        r.removeFromTop(6);
        auto buttons = r.removeFromBottom(Tokens::minTarget);
        ok.setBounds(buttons.removeFromRight(150));
        buttons.removeFromRight(8);
        cancel.setBounds(buttons.removeFromRight(130));
        refuseB.setBounds(buttons.removeFromLeft(120));
        r.removeFromBottom(6);
        const auto pair = [&r](juce::Label& la, juce::Component& a, juce::Label* lb, juce::Component* b) {
            auto row = r.removeFromTop(20 + Tokens::minTarget);
            r.removeFromTop(4);
            auto left = b != nullptr ? row.removeFromLeft((row.getWidth() - 10) / 2) : row;
            la.setBounds(left.removeFromTop(20));
            a.setBounds(left);
            if (b != nullptr)
            {
                row.removeFromLeft(10);
                lb->setBounds(row.removeFromTop(20));
                b->setBounds(row);
            }
        };
        pair(originL, origin, &basisL, &basis);
        const bool t = tool.isVisible(), l = licence.isVisible(), e = evidence.isVisible();
        if (t && l)
            pair(toolL, tool, &licenceL, &licence);
        else if (t)
            pair(toolL, tool, nullptr, nullptr);
        else if (l)
            pair(licenceL, licence, nullptr, nullptr);
        if (e)
            pair(evidenceL, evidence, nullptr, nullptr);
        pair(byL, by, nullptr, nullptr);
        derive.setBounds(r.removeFromTop(Tokens::minTarget));
        publish.setBounds(r.removeFromTop(Tokens::minTarget));
        problems.setBounds(r);
    }

private:
    void refreshFields()
    {
        const int o = origin.getSelectedItemIndex(), b = basis.getSelectedItemIndex();
        const bool needsTool = o == 2 || o == 3;              // outro instrumento, IA
        const bool needsLicence = b == 1;                     // licença
        const bool needsEvidence = b == 1 || b == 3 || b == 4;  // licença, permissão, termos
        tool.setVisible(needsTool);
        toolL.setVisible(needsTool);
        licence.setVisible(needsLicence);
        licenceL.setVisible(needsLicence);
        evidence.setVisible(needsEvidence);
        evidenceL.setVisible(needsEvidence);
        resized();
    }
    void submit()
    {
        using namespace trioio::provenance;
        static const OriginCategory origins[] = { OriginCategory::authorial, OriginCategory::performerCapture, OriginCategory::otherInstrument,
                                                  OriginCategory::aiGenerated, OriginCategory::licensed, OriginCategory::publicDomain };
        static const AuthorizationBasis bases[] = { AuthorizationBasis::ownWork, AuthorizationBasis::license, AuthorizationBasis::publicDomain,
                                                    AuthorizationBasis::writtenPermission, AuthorizationBasis::serviceTerms };
        library::AuthorizationRequest q;
        q.id = id;
        q.origin = origins[std::clamp(origin.getSelectedItemIndex(), 0, 5)];
        q.basis = bases[std::clamp(basis.getSelectedItemIndex(), 0, 4)];
        q.scope = static_cast<UseSet>(Use::listen);
        if (derive.getToggleState())
            q.scope = Use::listen | Use::analyze | Use::learn | Use::derive;
        if (publish.getToggleState())
            q.scope = q.scope | Use::listen | Use::analyze | Use::derive | Use::publishDerivative;
        if (tool.isVisible())
        {
            // "Nome 1.2" → ferramenta e versão (a última palavra, se tiver dígito)
            const auto t = tool.getText().trim();
            const auto last = t.fromLastOccurrenceOf(" ", false, false);
            if (t.containsChar(' ') && last.containsAnyOf("0123456789"))
            {
                q.toolName = t.upToLastOccurrenceOf(" ", false, false).trim().toStdString();
                q.toolVersion = last.toStdString();
            }
            else
                q.toolName = t.toStdString();
        }
        if (licence.isVisible())
            q.license = licence.getText().trim().toStdString();
        if (evidence.isVisible())
            q.evidence = evidence.getText().trim().toStdString();
        q.declaredBy = by.getText().trim().toStdString();
        if (onAuthorise)
            onAuthorise(q);
    }
};

// ---- SUA ESCUTA: as versões sobre a referência e o que a escuta ensinou -----------
// (docs/design.md §4, prancha 5). À esquerda as versões (modo, seed, veredito,
// nota); à direita as tendências do escuta.json em barras logarítmicas
// centradas em ×1 (×0,5 a ×2), com o esquecimento declarado e a chave "a
// próxima resposta leva a escuta em conta".

// O gesto desenhado (o modo híbrido, docs/PROMPTS_E_INTENCOES.md): a curva
// de intensidade da peça, da esquerda (o começo) à direita (o fim), de baixo
// (vazio) a cima (pleno). Clicar ou arrastar põe ou move um ponto; o duplo
// clique tira o ponto mais perto. O texto do campo Gesto acompanha.
class GestureEditor : public juce::Component
{
public:
    std::function<void()> onChange;
    std::vector<std::pair<float, float>> points;

    void setPoints(std::vector<std::pair<float, float>> p)
    {
        std::sort(p.begin(), p.end());
        points = std::move(p);
        repaint();
    }
    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        g.setColour(Tokens::recessed);
        g.fillRoundedRectangle(b, 4.0f);
        g.setColour(Tokens::line);
        g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.0f);
        if (points.size() < 2)
        {
            g.setColour(Tokens::text2);
            g.setFont(Tokens::smallFont);
            g.drawText(points.empty() ? hint : hintOne, getLocalBounds().reduced(8, 0), juce::Justification::centredLeft);
        }
        juce::Path curve;
        for (std::size_t k = 0; k < points.size(); ++k)
        {
            const auto p = toScreen(points[k]);
            if (k == 0)
                curve.startNewSubPath(p);
            else
                curve.lineTo(p);
        }
        g.setColour(Tokens::response);
        if (points.size() >= 2)
            g.strokePath(curve, juce::PathStrokeType(2.0f));
        for (const auto& pt : points)
        {
            const auto p = toScreen(pt);
            g.fillEllipse(p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        dragging_ = -1;
        const auto v = fromScreen(e.position);
        for (std::size_t k = 0; k < points.size(); ++k)
            if (std::abs(points[k].first - v.first) < 0.03f)
                dragging_ = static_cast<int>(k);
        if (dragging_ < 0)
        {
            points.push_back(v);
            std::sort(points.begin(), points.end());
            for (std::size_t k = 0; k < points.size(); ++k)
                if (points[k] == v)
                    dragging_ = static_cast<int>(k);
        }
        else
            points[static_cast<std::size_t>(dragging_)].second = v.second;
        changed();
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragging_ < 0 || dragging_ >= static_cast<int>(points.size()))
            return;
        auto& p = points[static_cast<std::size_t>(dragging_)];
        const auto v = fromScreen(e.position);
        // o ponto não passa os vizinhos
        const float lo = dragging_ > 0 ? points[static_cast<std::size_t>(dragging_ - 1)].first + 0.01f : 0.0f;
        const float hi = dragging_ + 1 < static_cast<int>(points.size()) ? points[static_cast<std::size_t>(dragging_ + 1)].first - 0.01f : 1.0f;
        p = { std::clamp(v.first, lo, hi), v.second };
        changed();
    }
    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        const auto v = fromScreen(e.position);
        std::size_t best = points.size();
        float bd = 0.05f;
        for (std::size_t k = 0; k < points.size(); ++k)
            if (std::abs(points[k].first - v.first) < bd)
            {
                bd = std::abs(points[k].first - v.first);
                best = k;
            }
        if (best < points.size())
        {
            points.erase(points.begin() + static_cast<std::ptrdiff_t>(best));
            dragging_ = -1;
            changed();
        }
    }
    juce::String hint, hintOne;

private:
    int dragging_ = -1;
    juce::Point<float> toScreen(std::pair<float, float> p) const
    {
        const auto b = getLocalBounds().toFloat().reduced(6.0f);
        return { b.getX() + p.first * b.getWidth(), b.getBottom() - p.second * b.getHeight() };
    }
    std::pair<float, float> fromScreen(juce::Point<float> s) const
    {
        const auto b = getLocalBounds().toFloat().reduced(6.0f);
        const float x = std::clamp((s.x - b.getX()) / std::max(1.0f, b.getWidth()), 0.0f, 1.0f);
        const float y = std::clamp((b.getBottom() - s.y) / std::max(1.0f, b.getHeight()), 0.0f, 1.0f);
        return { std::round(x * 100.0f) / 100.0f, std::round(y * 100.0f) / 100.0f };
    }
    void changed()
    {
        repaint();
        if (onChange)
            onChange();
    }
};

class ListeningPage : public juce::Component, private juce::ListBoxModel
{
public:
    struct Row
    {
        juce::String title, detail;
        int verdict = -1;  // −1 sem julgamento; 0 aceita, 1 rejeitada, 2 a transformar
    };
    struct Bar
    {
        juce::String group, name;
        float factor = 1.0f;
    };
    std::function<void(int)> onSelect;
    std::function<void(bool)> onToggle;
    std::function<void()> onCompare, onSwitch;
    Lang lang = Lang::en;

    ListeningPage()
    {
        list.setModel(this);
        list.setRowHeight(52);
        list.setColour(juce::ListBox::backgroundColourId, Tokens::recessed);
        addAndMakeVisible(list);
        toggle.setToggleState(true, juce::dontSendNotification);
        toggle.onClick = [this] { if (onToggle) onToggle(toggle.getToggleState()); };
        addAndMakeVisible(toggle);
        compare.onClick = [this] { if (onCompare) onCompare(); };
        swap.onClick = [this] { if (onSwitch) onSwitch(); };
        addAndMakeVisible(compare);
        addAndMakeVisible(swap);
    }
    ~ListeningPage() override { list.setModel(nullptr); }

    std::vector<juce::String> rules;  // as regras do jogo da versão A (reveladas)

    // A e B à vista: um selo em cada linha (A na cor da resposta, B na da
    // referência), ▶ na que está tocando, e a linha "A: … · B: …" sobre os
    // botões — o autor não via qual versão era qual (2026-10-14)
    void setMarks(int a, int b, int playingRow)
    {
        markA = a;
        markB = b;
        playing = playingRow;
        list.repaint();
        repaint();
    }

    void set(std::vector<Row> r, std::vector<Bar> b, int judgements, int selected, bool useListening)
    {
        rows = std::move(r);
        bars = std::move(b);
        count = judgements;
        offset = std::clamp(offset, 0, std::max(0, static_cast<int>(bars.size()) - 1));
        list.updateContent();
        if (selected >= 0 && selected < static_cast<int>(rows.size()))
            list.selectRow(selected, false, true);
        toggle.setToggleState(useListening, juce::dontSendNotification);
        toggle.setButtonText(tr(S::useListeningToggle, lang));
        compare.setButtonText(tr(S::compareAB, lang));
        swap.setButtonText(tr(S::switchAB, lang));
        compare.setTooltip(tr(S::abHint, lang));
        swap.setTooltip(tr(S::abHint, lang));
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(Tokens::background);
        auto r = getLocalBounds().reduced(16, 12);
        auto left = r.removeFromLeft(std::min(460, r.getWidth() / 2));
        r.removeFromLeft(16);
        g.setColour(Tokens::text);
        g.setFont(juce::FontOptions(Tokens::titleFont, juce::Font::bold));
        g.drawText(tr(S::versionsTitle, lang), left.removeFromTop(28), juce::Justification::centredLeft);
        {
            left.removeFromBottom(Tokens::minTarget + 4);
            auto line = left.removeFromBottom(22);
            g.setFont(juce::FontOptions(Tokens::smallFont, juce::Font::bold));
            const auto name = [this](int i) {
                return i >= 0 && i < static_cast<int>(rows.size()) ? rows[static_cast<std::size_t>(i)].title : tr(S::abNone, lang);
            };
            auto half = line.removeFromLeft(line.getWidth() / 2);
            g.setColour(Tokens::response);
            g.drawText(u8("A: ") + name(markA), half, juce::Justification::centredLeft, true);
            g.setColour(Tokens::ref);
            g.drawText(u8("B: ") + name(markB), line, juce::Justification::centredLeft, true);
        }
        g.drawText(tr(S::taughtTitle, lang), r.removeFromTop(28), juce::Justification::centredLeft);
        // as regras do jogo da versão A ficam embaixo, acima da chave, mesmo
        // sem julgamento ainda (elas não dependem da escuta)
        r.removeFromBottom(Tokens::minTarget + 8);
        if (!rules.empty())
        {
            // as regras do jogo da versão A, reveladas, acima da chave
            auto box = r.removeFromBottom(std::min(r.getHeight() / 2, 22 + 34 * static_cast<int>(rules.size())));
            g.setColour(Tokens::eye);
            g.setFont(juce::FontOptions(Tokens::smallFont, juce::Font::bold));
            g.drawText(tr(S::gameRulesTitle, lang), box.removeFromTop(20), juce::Justification::centredLeft);
            g.setFont(juce::FontOptions(12.0f));
            g.setColour(Tokens::text);
            for (const auto& line : rules)
            {
                if (box.getHeight() < 30)
                    break;
                g.drawFittedText(line, box.removeFromTop(34), juce::Justification::topLeft, 2);
            }
        }
        g.setFont(juce::FontOptions(Tokens::smallFont));
        g.setColour(Tokens::text2);
        auto info = r.removeFromTop(20);
        if (count == 0)
        {
            g.drawFittedText(tr(S::noJudgements, lang), r.removeFromTop(60), juce::Justification::topLeft, 3);
            return;
        }
        g.drawText(juce::String(count) + tr(S::judgementsCount, lang), info, juce::Justification::centredLeft, true);
        r.removeFromTop(6);
        // as barras: o nome à esquerda, a barra log2 centrada em ×1
        const int rowH = 22;
        const int nameW = std::min(220, r.getWidth() / 2);
        barsArea = r;
        juce::String lastGroup;
        shown = 0;
        for (std::size_t bi = static_cast<std::size_t>(offset); bi < bars.size(); ++bi)
        {
            const auto& b = bars[bi];
            // a última linha que cabe fica para o aviso, se ainda houver barras depois
            if (r.getHeight() < 2 * rowH && bi + 1 < bars.size())
            {
                g.setColour(Tokens::text2);
                g.drawText(u8("↓ ") + juce::String(static_cast<int>(bars.size() - bi)) + u8(" …"), r.removeFromTop(rowH),
                           juce::Justification::centredLeft);
                break;
            }
            ++shown;
            if (b.group != lastGroup)
            {
                lastGroup = b.group;
                g.setColour(Tokens::eye);
                g.drawText(b.group, r.removeFromTop(rowH), juce::Justification::bottomLeft);
                if (r.getHeight() < rowH)
                    break;
            }
            auto row = r.removeFromTop(rowH);
            g.setColour(Tokens::text);
            g.drawText(b.name, row.removeFromLeft(nameW), juce::Justification::centredLeft, true);
            auto valueBox = row.removeFromRight(52);  // o valor fora da barra (a barra cheia não o cobre)
            auto bar = row.reduced(4, 6).toFloat();
            const float mid = bar.getCentreX();
            g.setColour(Tokens::line);
            g.fillRect(bar);
            g.setColour(Tokens::text2);
            g.drawVerticalLine(static_cast<int>(mid), bar.getY() - 3.0f, bar.getBottom() + 3.0f);
            const float v = std::clamp(std::log2(b.factor), -1.0f, 1.0f);  // ×0,5 … ×2
            const float w = v * bar.getWidth() * 0.5f;
            g.setColour(v >= 0.0f ? Tokens::response : Tokens::warn);
            g.fillRect(juce::Rectangle<float>(std::min(mid, mid + w), bar.getY(), std::abs(w), bar.getHeight()));
            g.setColour(Tokens::text2);
            g.drawText(u8("×") + juce::String(b.factor, 2), valueBox, juce::Justification::centredRight);
        }
    }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (!barsArea.contains(e.getPosition()) || bars.empty())
            return;
        const int step = w.deltaY < 0.0f ? 3 : -3;
        offset = std::clamp(offset + step, 0, std::max(0, static_cast<int>(bars.size()) - shown));
        repaint();
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced(16, 12);
        auto left = r.removeFromLeft(std::min(460, r.getWidth() / 2));
        r.removeFromLeft(16);
        left.removeFromTop(32);
        auto ab = left.removeFromBottom(Tokens::minTarget);
        left.removeFromBottom(4 + 22 + 4);  // a linha "A: … · B: …"
        compare.setBounds(ab.removeFromLeft(ab.getWidth() / 2 - 4));
        ab.removeFromLeft(8);
        swap.setBounds(ab);
        list.setBounds(left);
        toggle.setBounds(r.removeFromBottom(Tokens::minTarget));
    }

private:
    int getNumRows() override { return static_cast<int>(rows.size()); }
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= static_cast<int>(rows.size()))
            return;
        const auto& r = rows[static_cast<std::size_t>(row)];
        auto box = juce::Rectangle<int>(0, 0, w, h).reduced(4, 3).toFloat();
        g.setColour(selected ? Tokens::responseBlock : Tokens::surface);
        g.fillRoundedRectangle(box, 5.0f);
        const juce::Colour vc = r.verdict == 0 ? Tokens::response : r.verdict == 1 ? Tokens::warn : r.verdict == 2 ? Tokens::eye : Tokens::line;
        g.setColour(vc);
        g.fillRoundedRectangle(box.removeFromLeft(5.0f), 2.0f);
        if (row == markA || row == markB)
        {
            const bool isA = row == markA;
            const juce::Colour c = isA ? Tokens::response : Tokens::ref;
            auto badge = box.removeFromRight(46.0f).withSizeKeepingCentre(row == playing ? 40.0f : 26.0f, 26.0f);
            g.setColour(c);
            g.fillRoundedRectangle(badge, 13.0f);
            g.setColour(Tokens::background);
            g.setFont(juce::FontOptions(Tokens::bodyFont, juce::Font::bold));
            g.drawText(row == playing ? (isA ? u8("▶ A") : u8("▶ B")) : (isA ? u8("A") : u8("B")), badge,
                       juce::Justification::centred);
            g.setColour(c);
            g.drawRoundedRectangle(juce::Rectangle<int>(0, 0, w, h).reduced(4, 3).toFloat(), 5.0f, 1.5f);
        }
        auto t = box.toNearestInt().reduced(10, 4);
        g.setColour(Tokens::text);
        g.setFont(juce::FontOptions(Tokens::bodyFont));
        g.drawText(r.title, t.removeFromTop(t.getHeight() / 2), juce::Justification::centredLeft, true);
        g.setColour(Tokens::text2);
        g.setFont(juce::FontOptions(Tokens::smallFont));
        g.drawText(r.detail, t, juce::Justification::centredLeft, true);
    }
    void selectedRowsChanged(int last) override
    {
        if (last >= 0 && onSelect)
            onSelect(last);
    }

    juce::ListBox list;
    juce::ToggleButton toggle;
    juce::TextButton compare, swap;
    juce::Rectangle<int> barsArea;
    int offset = 0, shown = 0;  // a rolagem das barras (roda do mouse)
    int markA = -1, markB = -1, playing = -1;
    std::vector<Row> rows;
    std::vector<Bar> bars;
    int count = 0;
};

// ---- SOBRE: véu escuro sobre a tela e um cartão (o padrão da família) ------------

class AboutOverlay : public juce::Component
{
public:
    std::function<void()> onClose;
    juce::Label title;
    juce::TextEditor body;
    juce::TextButton close;
    AboutOverlay()
    {
        title.setFont(juce::FontOptions(Tokens::titleFont, juce::Font::bold));
        title.setColour(juce::Label::textColourId, Tokens::eye);
        addAndMakeVisible(title);
        body.setMultiLine(true, true);
        body.setReadOnly(true);
        body.setCaretVisible(false);
        body.setScrollbarsShown(true);
        body.setFont(juce::FontOptions(Tokens::bodyFont));
        body.setColour(juce::TextEditor::backgroundColourId, Tokens::surface);
        body.setColour(juce::TextEditor::outlineColourId, Tokens::surface);
        addAndMakeVisible(body);
        close.onClick = [this] { if (onClose) onClose(); };
        addAndMakeVisible(close);
        setWantsKeyboardFocus(true);
    }
    juce::Rectangle<int> card() const
    {
        return getLocalBounds().withSizeKeepingCentre(std::min(720, getWidth() - 32), std::min(520, getHeight() - 32));
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.62f));
        g.setColour(Tokens::surface);
        g.fillRoundedRectangle(card().toFloat(), 10.0f);
        g.setColour(Tokens::line);
        g.drawRoundedRectangle(card().toFloat(), 10.0f, 1.0f);
    }
    void resized() override
    {
        auto r = card().reduced(24, 20);
        title.setBounds(r.removeFromTop(30));
        r.removeFromTop(8);
        close.setBounds(r.removeFromBottom(Tokens::minTarget).removeFromRight(140));
        r.removeFromBottom(10);
        body.setBounds(r);
    }
    bool keyPressed(const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey && onClose)
        {
            onClose();
            return true;
        }
        return false;
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!card().contains(e.getPosition()) && onClose)
            onClose();
    }
};

// ---- TUTORIAL: o padrão da família (Antitotem, Navalha 2) no cartão do SOBRE ------
// o sumário à esquerda, o texto com rolagem, a DICA num quadro à parte,
// anterior/próximo; a língua é a do instrumento (src/app/Tutorial.h)

class TutorialOverlay : public juce::Component
{
public:
    std::function<void()> onClose;
    TutorialOverlay()
    {
        heading.setFont(juce::FontOptions(Tokens::titleFont, juce::Font::bold));
        heading.setColour(juce::Label::textColourId, Tokens::eye);
        addAndMakeVisible(heading);
        contentsHeading.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        contentsHeading.setColour(juce::Label::textColourId, Tokens::text2);
        addAndMakeVisible(contentsHeading);
        for (std::size_t i = 0; i < chapterButtons.size(); ++i)
        {
            auto& b = chapterButtons[i];
            b.setClickingTogglesState(false);
            b.setColour(juce::TextButton::buttonOnColourId, Tokens::responseBlock);
            b.onClick = [this, i] { select(static_cast<int>(i)); };
            addAndMakeVisible(b);
        }
        body.setMultiLine(true, true);
        body.setReadOnly(true);
        body.setCaretVisible(false);
        body.setScrollbarsShown(true);
        body.setFont(juce::FontOptions(15.0f));
        body.setColour(juce::TextEditor::backgroundColourId, Tokens::recessed);
        body.setColour(juce::TextEditor::outlineColourId, Tokens::recessed);
        body.setIndents(14, 12);
        body.setLineSpacing(1.3f);
        addAndMakeVisible(body);
        tipHeading.setFont(juce::FontOptions(12.5f, juce::Font::bold));
        tipHeading.setColour(juce::Label::textColourId, Tokens::eye);
        addAndMakeVisible(tipHeading);
        tipBody.setFont(juce::FontOptions(14.5f));
        tipBody.setColour(juce::Label::textColourId, Tokens::text);
        tipBody.setJustificationType(juce::Justification::topLeft);
        tipBody.setMinimumHorizontalScale(1.0f);
        addAndMakeVisible(tipBody);
        previous.onClick = [this] { select(current - 1); };
        next.onClick = [this] { select(current + 1); };
        close.onClick = [this] { if (onClose) onClose(); };
        for (auto* b : { &previous, &next, &close })
            addAndMakeVisible(*b);
        setWantsKeyboardFocus(true);
    }
    void setLanguage(Lang l)
    {
        lang = l;
        refresh();
    }
    juce::Rectangle<int> card() const
    {
        return getLocalBounds().withSizeKeepingCentre(std::min(1000, getWidth() - 32), std::min(660, getHeight() - 32));
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.62f));
        g.setColour(Tokens::surface);
        g.fillRoundedRectangle(card().toFloat(), 10.0f);
        g.setColour(Tokens::line);
        g.drawRoundedRectangle(card().toFloat(), 10.0f, 1.0f);
        if (!tipBounds.isEmpty())
        {
            g.setColour(Tokens::eyeBg);
            g.fillRoundedRectangle(tipBounds.toFloat(), 8.0f);
            g.setColour(Tokens::eye.withAlpha(0.7f));
            g.fillRect(tipBounds.getX(), tipBounds.getY() + 8, 3, tipBounds.getHeight() - 16);
        }
    }
    void resized() override
    {
        auto r = card().reduced(22, 18);
        auto nav = r.removeFromBottom(Tokens::minTarget);
        r.removeFromBottom(12);
        const bool narrow = r.getWidth() < 640;
        auto side = r.removeFromLeft(narrow ? 170 : 250);
        r.removeFromLeft(16);
        contentsHeading.setBounds(side.removeFromTop(24));
        const int bh = std::min(Tokens::minTarget, side.getHeight() / static_cast<int>(chapterButtons.size()));
        for (auto& b : chapterButtons)
            b.setBounds(side.removeFromTop(bh).reduced(0, 2));
        heading.setBounds(r.removeFromTop(34));
        r.removeFromTop(8);
        const bool tip = tipBody.getText().isNotEmpty();
        tipHeading.setVisible(tip);
        tipBody.setVisible(tip);
        if (tip)
        {
            tipBounds = r.removeFromBottom(112);
            r.removeFromBottom(10);
            auto t = tipBounds.reduced(18, 10);
            tipHeading.setBounds(t.removeFromTop(20));
            tipBody.setBounds(t);
        }
        else
            tipBounds = {};
        body.setBounds(r);
        close.setBounds(nav.removeFromRight(130));
        nav.removeFromRight(16);
        next.setBounds(nav.removeFromRight(150));
        nav.removeFromRight(8);
        previous.setBounds(nav.removeFromRight(150));
    }
    bool keyPressed(const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey && onClose)
        {
            onClose();
            return true;
        }
        if (k == juce::KeyPress::rightKey || k == juce::KeyPress::pageDownKey)
        {
            select(current + 1);
            return true;
        }
        if (k == juce::KeyPress::leftKey || k == juce::KeyPress::pageUpKey)
        {
            select(current - 1);
            return true;
        }
        return false;
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!card().contains(e.getPosition()) && onClose)
            onClose();
    }

private:
    void select(int i)
    {
        current = std::clamp(i, 0, static_cast<int>(tutorial::chapters.size()) - 1);
        refresh();
    }
    void refresh()
    {
        const auto& c = tutorial::chapters[static_cast<std::size_t>(current)];
        heading.setText(juce::String(current + 1) + ". " + tr(c.title, lang), juce::dontSendNotification);
        contentsHeading.setText(tr(tutorial::contents, lang), juce::dontSendNotification);
        tipHeading.setText(tr(tutorial::tipHeading, lang), juce::dontSendNotification);
        tipBody.setText(tr(c.tip, lang), juce::dontSendNotification);
        for (std::size_t i = 0; i < chapterButtons.size(); ++i)
        {
            chapterButtons[i].setButtonText(juce::String(static_cast<int>(i) + 1) + ". " + tr(tutorial::chapters[i].title, lang));
            chapterButtons[i].setToggleState(static_cast<int>(i) == current, juce::dontSendNotification);
        }
        body.setText(tr(c.body, lang), false);
        body.moveCaretToTop(false);
        previous.setButtonText(tr(tutorial::previous, lang));
        next.setButtonText(tr(tutorial::next, lang));
        close.setButtonText(tr(S::close, lang));
        previous.setEnabled(current > 0);
        next.setEnabled(current + 1 < static_cast<int>(tutorial::chapters.size()));
        resized();
        repaint();
    }
    Lang lang = Lang::pt;
    int current = 0;
    juce::Label heading, contentsHeading, tipHeading, tipBody;
    std::array<juce::TextButton, tutorial::chapters.size()> chapterButtons;
    juce::TextEditor body;
    juce::TextButton previous, next, close;
    juce::Rectangle<int> tipBounds;
};

// ---- a tela ---------------------------------------------------------------------

MainView::MainView()
{
    auto& lf = getLookAndFeel();
    lf.setColour(juce::TextButton::buttonColourId, Tokens::recessed);
    lf.setColour(juce::TextButton::buttonOnColourId, Tokens::responseBlock);
    lf.setColour(juce::TextButton::textColourOffId, Tokens::text);
    lf.setColour(juce::TextButton::textColourOnId, Tokens::text);
    lf.setColour(juce::ComboBox::backgroundColourId, Tokens::recessed);
    lf.setColour(juce::ComboBox::outlineColourId, Tokens::line);
    lf.setColour(juce::ComboBox::textColourId, Tokens::text);
    lf.setColour(juce::TextEditor::backgroundColourId, Tokens::recessed);
    lf.setColour(juce::TextEditor::outlineColourId, Tokens::line);
    lf.setColour(juce::TextEditor::textColourId, Tokens::text);
    lf.setColour(juce::Label::textColourId, Tokens::text);
    lf.setColour(juce::ListBox::backgroundColourId, Tokens::background);
    lf.setColour(juce::PopupMenu::backgroundColourId, Tokens::surface);
    lf.setColour(juce::Slider::thumbColourId, Tokens::eye);
    lf.setColour(juce::Slider::trackColourId, Tokens::eye.withAlpha(0.6f));

    title_.setText("TRIOIO", juce::dontSendNotification);
    title_.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    addAndMakeVisible(title_);
    refButton_.setButtonText(u8("escolher referência"));
    refButton_.onClick = [this] { setPage(1); };
    addAndMakeVisible(refButton_);
    const char* modes[] = { "seguir", "transfigurar", "contrapor", "interromper" };
    for (int i = 0; i < 4; ++i)
    {
        modeButtons_[i].setButtonText(u8(modes[i]));
        modeButtons_[i].setClickingTogglesState(true);
        modeButtons_[i].setRadioGroupId(1001);
        modeButtons_[i].onClick = [this, i] { mode_ = i; };
        addAndMakeVisible(modeButtons_[i]);
    }
    modeButtons_[0].setToggleState(true, juce::dontSendNotification);
    readingLabel_.setColour(juce::Label::textColourId, Tokens::text2);
    readingLabel_.setFont(juce::FontOptions(Tokens::smallFont));
    addAndMakeVisible(readingLabel_);
    const char* pages[] = { "Escuta e resposta", "Biblioteca", "Sua escuta" };
    for (int i = 0; i < 3; ++i)
    {
        pageButtons_[i].setButtonText(u8(pages[i]));
        pageButtons_[i].setClickingTogglesState(true);
        pageButtons_[i].setRadioGroupId(1002);
        pageButtons_[i].onClick = [this, i] { setPage(i); };
        // a aba ativa na cor da resposta: as páginas não passam despercebidas
        pageButtons_[i].setColour(juce::TextButton::buttonOnColourId, Tokens::response);
        pageButtons_[i].setColour(juce::TextButton::textColourOnId, Tokens::background);
        addChildComponent(pageButtons_[i]);
    }
    pageButtons_[0].setToggleState(true, juce::dontSendNotification);
    {
        auto page = std::make_unique<ListeningPage>();
        page->onSelect = [this](int i) {
            // A = a escolhida agora, B = a anterior
            if (i != versionA_)
            {
                versionB_ = versionA_;
                versionA_ = i;
            }
            if (i != responsePicker_.getSelectedItemIndex())
                responsePicker_.setSelectedItemIndex(i);  // → showResponse
            updateVersionMarks();
        };
        page->onCompare = [this] { compareVersions(); };
        page->onSwitch = [this] { switchVersions(); };
        page->onToggle = [this](bool on) { useListening_ = on; };
        listeningPage_ = std::move(page);
        addChildComponent(*listeningPage_);
    }
    drawerButton_.setButtonText(u8("Linguagem ▸"));
    drawerButton_.setClickingTogglesState(true);
    drawerButton_.onClick = [this] { drawerOpen_ = drawerButton_.getToggleState(); resized(); };
    addChildComponent(drawerButton_);
    menuButton_.setButtonText(u8("☰"));
    menuButton_.setTooltip(u8("Biblioteca, Linguagem"));
    menuButton_.onClick = [this] {
        juce::PopupMenu m;
        m.addItem(1, tr(S::pageListen, lang_));
        m.addItem(2, tr(S::pageLibrary, lang_));
        m.addItem(5, tr(S::pageYourListening, lang_));
        m.addItem(3, tr(S::menuLanguage, lang_));
        m.addItem(6, tr(tutorial::openButton, lang_));
        m.addItem(4, tr(S::about, lang_));
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menuButton_), [this](int r) {
            if (r == 1 || r == 2) setPage(r - 1);
            if (r == 5) setPage(2);
            if (r == 3) { drawerOpen_ = !drawerOpen_; resized(); }
            if (r == 4) showAbout(true);
            if (r == 6) showTutorial(true);
        });
    };
    addChildComponent(menuButton_);
    langButton_.onClick = [this] {
        setLanguage(nextLang(lang_));
        if (onLanguageChanged)
            onLanguageChanged(lang_);
    };
    addAndMakeVisible(langButton_);
    aboutButton_.onClick = [this] { showAbout(true); };
    addAndMakeVisible(aboutButton_);
    tutorialButton_.onClick = [this] { showTutorial(true); };
    addAndMakeVisible(tutorialButton_);
    creditLabel_.setFont(juce::FontOptions(10.5f));
    creditLabel_.setColour(juce::Label::textColourId, Tokens::text2.withAlpha(0.8f));
    creditLabel_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(creditLabel_);

    refTrack_ = std::make_unique<ReferenceTrack>();
    eyeTrack_ = std::make_unique<EyeTrack>();
    responseTrack_ = std::make_unique<ResponseTrack>();
    tracksHolder_.addAndMakeVisible(*refTrack_);
    tracksHolder_.addAndMakeVisible(*eyeTrack_);
    tracksHolder_.addAndMakeVisible(*responseTrack_);
    responseTrack_->onSelection = [this] { updateJudgeLabel(); };
    revealButton_.setButtonText(u8("revelar depois de ouvir"));
    revealButton_.setClickingTogglesState(true);
    revealButton_.setColour(juce::TextButton::textColourOffId, Tokens::eye);
    revealButton_.onClick = [this] {
        eyeTrack_->revealed = revealButton_.getToggleState();
        revealButton_.setButtonText(eyeTrack_->revealed ? tr(S::hideEye, lang_) : tr(S::reveal, lang_));
        eyeTrack_->repaint();
    };
    tracksHolder_.addAndMakeVisible(revealButton_);
    tracksHolder_.addChildComponent(bringBigButton_);
    refPlayButton_.onClick = [this] { toggleReferencePlayback(); };
    tracksHolder_.addChildComponent(refPlayButton_);
    togetherButton_.onClick = [this] { togglePlayTogether(); };
    tracksHolder_.addChildComponent(togetherButton_);
    responsePicker_.onChange = [this] { showResponse(responsePicker_.getSelectedItemIndex()); };
    responsePicker_.setTextWhenNoChoicesAvailable(u8("nenhuma resposta ainda"));
    responsePicker_.setTextWhenNothingSelected(u8("as respostas desta referência"));
    tracksHolder_.addAndMakeVisible(responsePicker_);
    tracksViewport_.setViewedComponent(&tracksHolder_, false);
    tracksViewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(tracksViewport_);

    libraryTitle_.setText(u8("Biblioteca"), juce::dontSendNotification);
    libraryTitle_.setFont(juce::FontOptions(Tokens::titleFont, juce::Font::bold));
    addAndMakeVisible(libraryTitle_);
    library_.setModel(this);
    library_.setRowHeight(56);
    addAndMakeVisible(library_);
    libraryCard_.setColour(juce::Label::textColourId, Tokens::text2);
    libraryCard_.setFont(juce::FontOptions(Tokens::smallFont));
    libraryCard_.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(libraryCard_);
    openButton_.setButtonText(u8("abrir pasta de dados…"));
    openButton_.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>(tr(S::dataRootTitle, lang_));
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                             [this, chooser](const juce::FileChooser& fc) {
                                 if (fc.getResult().isDirectory())
                                     openDataRoot(fc.getResult().getFullPathName().toStdString());
                             });
    };
    addAndMakeVisible(openButton_);
    bringButton_.setColour(juce::TextButton::buttonColourId, Tokens::responseBlock);
    bringButton_.onClick = [this] { bringRecording(); };
    addAndMakeVisible(bringButton_);
    bringBigButton_.setColour(juce::TextButton::buttonColourId, Tokens::responseBlock);
    bringBigButton_.onClick = [this] { bringRecording(); };
    authButton_.setColour(juce::TextButton::textColourOffId, Tokens::eye);
    authButton_.onClick = [this] { showAuth(true); };
    addChildComponent(authButton_);

    // a Linguagem: as escolhas que o "compor de novo" leva
    languageTitle_.setText(u8("Linguagem da resposta"), juce::dontSendNotification);
    languageTitle_.setFont(juce::FontOptions(Tokens::titleFont, juce::Font::bold));
    languageContent_.addAndMakeVisible(languageTitle_);
    const auto label = [this](juce::Label& l, const char* t) {
        l.setText(u8(t), juce::dontSendNotification);
        l.setFont(juce::FontOptions(Tokens::smallFont));
        l.setColour(juce::Label::textColourId, Tokens::text2);
        languageContent_.addAndMakeVisible(l);
    };
    const auto combo = [this](juce::ComboBox& c, std::initializer_list<const char*>) {
        languageContent_.addAndMakeVisible(c);  // os itens vêm de applyLanguage()
    };
    // as palavras: o prompt opcional da resposta; vazio, o Trioio segue os seus critérios
    label(wordsLabel_, "Palavras");
    languageContent_.addAndMakeVisible(words_);
    // a ponte semântica: consulta enquanto o autor escreve (um respiro de
    // 0,8 s depois da última tecla), em segundo plano; a resposta usa a memória
    languageContent_.addAndMakeVisible(semantic_);
    semanticStatus_.setFont(juce::FontOptions(Tokens::smallFont));
    semanticStatus_.setColour(juce::Label::textColourId, Tokens::text2);
    languageContent_.addAndMakeVisible(semanticStatus_);
    semantic_.onClick = [this] { prefetchSemantic(); };
    words_.onTextChange = [this] {
        const int gen = ++semanticGeneration_;
        juce::Component::SafePointer<MainView> self(this);
        juce::Timer::callAfterDelay(1200, [self, gen] {
            if (self && gen == self->semanticGeneration_)
                self->prefetchSemantic();
        });
    };
    label(mixLabel_, "Mistura (ex.: rāga Yaman + maqam Hijaz)");
    mix_.setTextToShowWhenEmpty(u8("nenhuma: a escala do campo"), Tokens::text2);
    languageContent_.addAndMakeVisible(mix_);
    label(pitchLabel_, "Alturas");
    combo(pitch_, { "arquivo (a linguagem reconhecida)", "referência", "catálogo", "microtonal", "inventada", "justa" });
    label(techniqueLabel_, "Técnica");
    combo(technique_, { "colagem", "híbrido", "camadas", "transição" });
    label(seamLabel_, "Costura");
    combo(seam_, { "corte", "pivô", "ponte", "sobreposição" });
    label(cycleLabel_, "Ciclo");
    combo(cycle_, { "o da escala", "maqsum", "tīntāl", "clave son 3-2", "compás de 12 (soleá, bulerías)", "baião" });
    label(voicingLabel_, "Voicing");
    combo(voicing_, { "o da escala", "terças", "quartal", "So What", "cluster" });
    label(intentionLabel_, "Intenção");
    combo(intention_, { "nenhuma", "arco", "luminosa", "sombria", "tensa", "serena", "suspensa" });
    label(formLabel_, "Forma");
    combo(form_, { "contraste", "repetição" });
    label(tempoLabel_, "Andamento");
    combo(tempo_, { "como lido", "metade", "dobro" });
    label(ensembleLabel_, "Conjunto");
    combo(ensemble_, {});
    label(harmonyLangLabel_, "Harmonia");
    combo(harmonyLang_, {});
    label(unpredLabel_, "Imprevisibilidade (o Terceiro Olho)");
    unpredictability_.setSliderStyle(juce::Slider::LinearHorizontal);
    unpredictability_.setRange(0.0, 1.0, 0.05);
    unpredictability_.setValue(0.5, juce::dontSendNotification);
    unpredictability_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 24);
    languageContent_.addAndMakeVisible(unpredictability_);
    label(rasgoLabel_, "Rasgo modular");
    rasgo_.setSliderStyle(juce::Slider::LinearHorizontal);
    rasgo_.setRange(0.0, 1.0, 0.05);
    rasgo_.setValue(0.5, juce::dontSendNotification);  // a escuta do autor de 15/10: "ainda muito midi"
    rasgo_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 24);
    languageContent_.addAndMakeVisible(rasgo_);
    languageContent_.addAndMakeVisible(dialogue_);
    languageContent_.addAndMakeVisible(game_);
    languageContent_.addAndMakeVisible(filmFx_);
    languageContent_.addAndMakeVisible(somDaAura_);
    languageContent_.addAndMakeVisible(stems_);
    // a árvore de influências: os nós (ou "auto"), a força e a exclusão
    label(influencesLabel_, "Influências");
    languageContent_.addAndMakeVisible(influences_);
    label(influenceStrengthLabel_, "Força das influências");
    influenceStrength_.setSliderStyle(juce::Slider::LinearHorizontal);
    influenceStrength_.setRange(0.0, 1.0, 0.05);
    influenceStrength_.setValue(1.0, juce::dontSendNotification);
    influenceStrength_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 24);
    languageContent_.addAndMakeVisible(influenceStrength_);
    label(excludeLabel_, "Sem");
    languageContent_.addAndMakeVisible(exclude_);
    label(gestureLabel_, "Gesto");
    languageContent_.addAndMakeVisible(gesture_);
    gestureEditor_ = std::make_unique<GestureEditor>();
    languageContent_.addAndMakeVisible(*gestureEditor_);
    // o desenho escreve o texto; o texto redesenha a curva
    gestureEditor_->onChange = [this] {
        juce::String t;
        for (const auto& p : gestureEditor_->points)
            t << (t.isEmpty() ? "" : ", ") << juce::String(p.first, 2) << ":" << juce::String(p.second, 2);
        gesture_.setText(t, juce::dontSendNotification);
    };
    gesture_.onTextChange = [this] {
        std::vector<std::pair<float, float>> pts;
        if (trioio::composition::parseGesture(gesture_.getText().toStdString(), pts) || gesture_.getText().trim().isEmpty())
            gestureEditor_->setPoints(pts);
    };
    // as teclas mortas compostas em todos os campos de texto do autor
    for (auto* e : { &words_, &mix_, &influences_, &exclude_, &note_ })
        attachDeadKeys(*e);
    languageNote_.setText(u8("Cada escolha cita a fonte na forma da resposta."), juce::dontSendNotification);
    languageNote_.setFont(juce::FontOptions(12.0f));
    languageNote_.setColour(juce::Label::textColourId, Tokens::text2);
    languageContent_.addAndMakeVisible(languageNote_);
    languageScroll_.setViewedComponent(&languageContent_, false);
    languageScroll_.setScrollBarsShown(true, false);
    languageScroll_.setScrollBarThickness(8);
    languagePanel_.addAndMakeVisible(languageScroll_);
    addAndMakeVisible(languagePanel_);

    playButton_.setButtonText(u8("▶"));
    playButton_.setTooltip(u8("Tocar a resposta"));
    playButton_.onClick = [this] {
        if (playing_ && !playingReference_ && !playingTogether_) { if (onStop) onStop(); setPlaying(false); return; }
        playingReference_ = playingTogether_ = false;
        playingVersion_ = -1;
        if (responseIndex_ >= 0 && responseIndex_ < static_cast<int>(responses_.size()) && onPlay)
        {
            onPlay(responses_[static_cast<std::size_t>(responseIndex_)].wavPath);
            setPlaying(true);
        }
    };
    addAndMakeVisible(playButton_);
    timeLabel_.setFont(juce::FontOptions(Tokens::bodyFont));
    addAndMakeVisible(timeLabel_);
    // master: −60 dB vale mudo; o meio do curso fica perto de −12 dB
    master_.setSliderStyle(juce::Slider::LinearHorizontal);
    master_.setRange(-60.0, 6.0, 0.5);
    master_.setSkewFactorFromMidPoint(-12.0);
    master_.setValue(0.0, juce::dontSendNotification);
    master_.setDoubleClickReturnValue(true, 0.0);
    master_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 24);
    master_.textFromValueFunction = [](double v) { return v <= -60.0 ? juce::String(u8("−∞ dB")) : juce::String(v, 1) + " dB"; };
    master_.valueFromTextFunction = [](const juce::String& t) { return t.retainCharacters("-0123456789.").getDoubleValue(); };
    master_.setColour(juce::Slider::trackColourId, Tokens::response);
    master_.setColour(juce::Slider::thumbColourId, Tokens::text);
    master_.setColour(juce::Slider::textBoxOutlineColourId, Tokens::line);
    master_.onValueChange = [this] { if (onMasterChanged) onMasterChanged(master_.getValue()); };
    master_.updateText();  // o texto com "dB" (as funções vêm depois do setTextBoxStyle)
    addAndMakeVisible(master_);
    judgeLabel_.setFont(juce::FontOptions(Tokens::smallFont));
    judgeLabel_.setColour(juce::Label::textColourId, Tokens::text2);
    judgeLabel_.setText(u8("julgar a resposta inteira"), juce::dontSendNotification);
    addAndMakeVisible(judgeLabel_);
    note_.setTextToShowWhenEmpty(u8("nota (opcional)…"), Tokens::text2);
    addAndMakeVisible(note_);
    acceptButton_.setButtonText(u8("aceitar"));
    transformButton_.setButtonText(u8("transformar"));
    rejectButton_.setButtonText(u8("rejeitar"));
    acceptButton_.onClick = [this] { judge(0); };
    rejectButton_.onClick = [this] { judge(1); };
    transformButton_.onClick = [this] { judge(2); };
    acceptButton_.setColour(juce::TextButton::buttonColourId, Tokens::responseBlock);
    transformButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a2f17));
    addAndMakeVisible(acceptButton_);
    addAndMakeVisible(transformButton_);
    addAndMakeVisible(rejectButton_);
    composeButton_.setButtonText(u8("compor de novo"));
    composeButton_.setColour(juce::TextButton::buttonColourId, Tokens::text);
    composeButton_.setColour(juce::TextButton::textColourOffId, Tokens::background);
    composeButton_.onClick = [this] { composeAgain(); };
    addAndMakeVisible(composeButton_);
    statusLabel_.setFont(juce::FontOptions(12.0f));
    statusLabel_.setColour(juce::Label::textColourId, Tokens::text2);
    addAndMakeVisible(statusLabel_);
    progressBar_.setColour(juce::ProgressBar::foregroundColourId, Tokens::response);
    progressBar_.setColour(juce::ProgressBar::backgroundColourId, Tokens::recessed);
    addChildComponent(progressBar_);

    timer_.fn = [this] {
        if (!playPosition)
            return;
        const double p = playPosition();
        if (p < 0.0 && playing_)
            setPlaying(false);
        if (playingReference_)
            return;
        responseTrack_->playhead = p;
        responseTrack_->repaint();
        timeLabel_.setText(clock(std::max(0.0, p)) + " / " + clock(responseTrack_->duration), juce::dontSendNotification);
    };
    applyLanguage();
    setSize(kCompactW, kCompactH);
}

MainView::~MainView()
{
    library_.setModel(nullptr);
}

void MainView::setLanguage(Lang l)
{
    lang_ = l;
    applyLanguage();
    resized();
    repaint();
}

void MainView::applyLanguage()
{
    refTrack_->lang = eyeTrack_->lang = responseTrack_->lang = lang_;
    refreshReferenceTexts();
    langButton_.setButtonText(langLabel(lang_));
    langButton_.setTooltip(tr(S::languageTip, lang_));
    aboutButton_.setButtonText(tr(S::about, lang_));
    tutorialButton_.setButtonText(tr(tutorial::openButton, lang_));
    if (auto* t = dynamic_cast<TutorialOverlay*>(tutorial_.get()))
        t->setLanguage(lang_);
    if (refIndex_ < 0)
        refButton_.setButtonText(tr(S::chooseReference, lang_));
    else if (!demo_)
    {
        const auto& ref = refs_[static_cast<std::size_t>(refIndex_)];
        refButton_.setButtonText(u8(ref.name) + u8(" · ") + trKey(ref.status, lang_));
    }
    for (int i = 0; i < 4; ++i)
        modeButtons_[i].setButtonText(tr(S::modes[i], lang_));
    pageButtons_[0].setButtonText(tr(S::pageListen, lang_));
    pageButtons_[1].setButtonText(tr(S::pageLibrary, lang_));
    pageButtons_[2].setButtonText(tr(S::pageYourListening, lang_));
    if (auto* lp = dynamic_cast<ListeningPage*>(listeningPage_.get()))
        lp->lang = lang_;
    if (page_ == 2)
        refreshListeningPage();
    drawerButton_.setButtonText(tr(S::drawer, lang_));
    menuButton_.setTooltip(tr(S::pageLibrary, lang_) + ", " + tr(S::menuLanguage, lang_) + ", " + tr(S::about, lang_));
    revealButton_.setButtonText(eyeTrack_->revealed ? tr(S::hideEye, lang_) : tr(S::reveal, lang_));
    responsePicker_.setTextWhenNoChoicesAvailable(tr(S::noResponses, lang_));
    responsePicker_.setTextWhenNothingSelected(tr(S::pickResponse, lang_));
    libraryTitle_.setText(tr(S::pageLibrary, lang_), juce::dontSendNotification);
    openButton_.setButtonText(tr(S::openData, lang_));
    bringButton_.setButtonText(tr(S::bring, lang_));
    bringBigButton_.setButtonText(tr(S::bring, lang_));
    authButton_.setButtonText(tr(S::authoriseButton, lang_));
    refPlayButton_.setButtonText(playingReference_ ? tr(S::stopRef, lang_) : tr(S::playRef, lang_));
    togetherButton_.setButtonText(playingTogether_ ? tr(S::stopRef, lang_) : tr(S::playTogether, lang_));
    togetherButton_.setTooltip(tr(S::togetherTip, lang_));
    dialogue_.setButtonText(tr(S::dialogueToggle, lang_));
    game_.setButtonText(tr(S::gameToggle, lang_));
    filmFx_.setButtonText(tr(S::filmFxToggle, lang_));
    somDaAura_.setButtonText(tr(S::somDaAuraToggle, lang_));
    stems_.setButtonText(tr(S::stemsToggle, lang_));
    semantic_.setButtonText(tr(S::semanticToggle, lang_));
    semantic_.setTooltip(tr(S::semanticTip, lang_));
    gestureLabel_.setText(tr(S::gestureLabel, lang_), juce::dontSendNotification);
    influencesLabel_.setText(tr(S::influencesLabel, lang_), juce::dontSendNotification);
    influences_.setTextToShowWhenEmpty(tr(S::influencesEmpty, lang_), Tokens::text2);
    influenceStrengthLabel_.setText(tr(S::influenceStrengthLabel, lang_), juce::dontSendNotification);
    excludeLabel_.setText(tr(S::excludeLabel, lang_), juce::dontSendNotification);
    exclude_.setTextToShowWhenEmpty(tr(S::excludeEmpty, lang_), Tokens::text2);
    gesture_.setTextToShowWhenEmpty(tr(S::gestureEmpty, lang_), Tokens::text2);
    gestureEditor_->hint = tr(S::gestureDraw, lang_);
    gestureEditor_->hintOne = tr(S::gestureDrawMore, lang_);
    gestureEditor_->repaint();
    languageTitle_.setText(tr(S::languageTitle, lang_), juce::dontSendNotification);
    mixLabel_.setText(tr(S::mixLabel, lang_), juce::dontSendNotification);
    wordsLabel_.setText(tr(S::wordsLabel, lang_), juce::dontSendNotification);
    words_.setTextToShowWhenEmpty(tr(S::wordsEmpty, lang_), Tokens::text2);
    mix_.setTextToShowWhenEmpty(tr(S::mixEmpty, lang_), Tokens::text2);
    pitchLabel_.setText(tr(S::pitchLabel, lang_), juce::dontSendNotification);
    techniqueLabel_.setText(tr(S::techniqueLabel, lang_), juce::dontSendNotification);
    seamLabel_.setText(tr(S::seamLabel, lang_), juce::dontSendNotification);
    cycleLabel_.setText(tr(S::cycleLabel, lang_), juce::dontSendNotification);
    voicingLabel_.setText(tr(S::voicingLabel, lang_), juce::dontSendNotification);
    intentionLabel_.setText(tr(S::intentionLabel, lang_), juce::dontSendNotification);
    formLabel_.setText(tr(S::formLabel, lang_), juce::dontSendNotification);
    tempoLabel_.setText(tr(S::tempoLabel, lang_), juce::dontSendNotification);
    ensembleLabel_.setText(tr(S::ensembleLabel, lang_), juce::dontSendNotification);
    harmonyLangLabel_.setText(tr(S::harmonyLangLabel, lang_), juce::dontSendNotification);
    rasgoLabel_.setText(tr(S::rasgoLabel, lang_), juce::dontSendNotification);
    unpredLabel_.setText(tr(S::unpredLabel, lang_), juce::dontSendNotification);
    languageNote_.setText(tr(S::languageNote, lang_), juce::dontSendNotification);
    // os combos: os itens no idioma novo, a escolha mantida
    const auto refill = [this](juce::ComboBox& c, std::initializer_list<juce::String> items) {
        const int keep = std::max(0, c.getSelectedItemIndex());
        c.clear(juce::dontSendNotification);
        int id = 1;
        for (const auto& it : items)
            c.addItem(it, id++);
        c.setSelectedItemIndex(std::min(keep, c.getNumItems() - 1), juce::dontSendNotification);
    };
    const auto L = [this](const L4& v) { return tr(v, lang_); };
    refill(pitch_, { L(S::pitchItems[0]), L(S::pitchItems[1]), L(S::pitchItems[2]), L(S::pitchItems[3]), L(S::pitchItems[4]), L(S::pitchItems[5]) });
    refill(technique_, { L(S::techniqueItems[0]), L(S::techniqueItems[1]), L(S::techniqueItems[2]), L(S::techniqueItems[3]) });
    refill(seam_, { L(S::seamItems[0]), L(S::seamItems[1]), L(S::seamItems[2]), L(S::seamItems[3]) });
    // os ciclos são nomes próprios das tradições: não se traduzem
    refill(cycle_, { L(S::fromScale), u8("maqsum"), u8("tīntāl"), u8("clave son 3-2"), u8("compás de 12 (soleá, bulerías)"), u8("baião") });
    refill(voicing_, { L(S::fromScale), L(S::voicingItems[0]), L(S::voicingItems[1]), L(S::voicingItems[2]), L(S::voicingItems[3]) });
    refill(intention_, { L(S::intentionItems[0]), L(S::intentionItems[1]), L(S::intentionItems[2]), L(S::intentionItems[3]),
                         L(S::intentionItems[4]), L(S::intentionItems[5]), L(S::intentionItems[6]) });
    refill(form_, { L(S::formItems[0]), L(S::formItems[1]) });
    refill(tempo_, { L(S::tempoItems[0]), L(S::tempoItems[1]), L(S::tempoItems[2]) });
    // as linguagens da máquina de harmonia (HarmonyLanguage, na ordem)
    refill(harmonyLang_, { L(S::harmonyAuto), L(S::harmonyFunctional), u8("jazz"), L(S::harmonyBrazilian), L(S::harmonyImpressionist),
                           L(S::harmonyNortheastern), L(S::harmonyMineira), u8("Hermeto Pascoal"),
                           L(S::harmonyModal), L(S::harmonyAndalusian), L(S::harmonyChromatic), L(S::harmonyQuartal), L(S::harmonyPolytonal),
                           L(S::harmonyOctatonic), L(S::harmonyClusters), L(S::harmonyStasis), L(S::harmonySpectral), u8("Bartók (eixos)"),
                           u8("Scriabin (acorde místico)"), u8("Villa-Lobos"), u8("Moacir Santos"), u8("Guinga"),
                           u8("frevo"), u8("maracatu"), u8("Egberto Gismonti"),
                           u8("Partch (microtonal, entonação justa)") });
    // os conjuntos (core/Ensemble.h, na ordem de Ensemble); os nomes dos estilos não se traduzem
    refill(ensemble_, { L(S::ensembleAuto), L(S::ensembleRead), u8("choro"), u8("samba"), u8("jazz"), u8("manouche"), u8("bolero"),
                        u8("charanga (salsa)"), L(S::ensembleChamber), L(S::ensembleOrchestra), u8("salsa"), u8("a cappella"), u8("rock"), u8("escola de samba"), u8("forró"), u8("tango") });
    playButton_.setTooltip(tr(S::playTip, lang_));
    master_.setTooltip(tr(S::masterTip, lang_));
    updateJudgeLabel();
    note_.setTextToShowWhenEmpty(tr(S::noteEmpty, lang_), Tokens::text2);
    acceptButton_.setButtonText(tr(S::accept, lang_));
    transformButton_.setButtonText(tr(S::transform, lang_));
    rejectButton_.setButtonText(tr(S::reject, lang_));
    composeButton_.setButtonText(tr(S::compose, lang_));
    creditLabel_.setText(tr(S::footerCredit, lang_) + TRIOIO_VERSION, juce::dontSendNotification);
    if (auto* a = dynamic_cast<AboutOverlay*>(about_.get()))
    {
        a->title.setText(tr(S::aboutTitle, lang_), juce::dontSendNotification);
        a->body.setText(tr(S::aboutBody, lang_) + "\n\n" + tr(S::footerCredit, lang_) + TRIOIO_VERSION, false);
        a->close.setButtonText(tr(S::close, lang_));
    }
    // a lista de respostas e a biblioteca, com os nomes no idioma novo
    library_.repaint();
    for (int i = 0; i < responsePicker_.getNumItems() && i < static_cast<int>(responses_.size()); ++i)
    {
        const auto& r = responses_[static_cast<std::size_t>(i)];
        responsePicker_.changeItemText(i + 1, trKey(r.mode, lang_) + u8(" · seed ") + juce::String(static_cast<juce::int64>(r.seed)) + u8(" · ")
                                                  + u8(r.sessionId).fromFirstOccurrenceOf("sessao-", false, false).substring(0, 12));
    }
    tracksHolder_.repaint();
}

void MainView::showTutorial(bool show)
{
    if (!show)
    {
        if (tutorial_)
            tutorial_->setVisible(false);
        return;
    }
    if (!tutorial_)
    {
        auto t = std::make_unique<TutorialOverlay>();
        t->onClose = [this] { showTutorial(false); };
        t->setLanguage(lang_);
        tutorial_ = std::move(t);
        addChildComponent(*tutorial_);
    }
    tutorial_->setBounds(getLocalBounds());
    tutorial_->setVisible(true);
    tutorial_->toFront(true);
    tutorial_->grabKeyboardFocus();
}

void MainView::showAbout(bool show)
{
    if (!show)
    {
        if (about_)
            about_->setVisible(false);
        return;
    }
    if (!about_)
    {
        auto a = std::make_unique<AboutOverlay>();
        a->onClose = [this] { showAbout(false); };
        about_ = std::move(a);
        addChildComponent(*about_);
        applyLanguage();
    }
    about_->setBounds(getLocalBounds());
    about_->setVisible(true);
    about_->toFront(true);
    about_->grabKeyboardFocus();
}

void MainView::setPlaying(bool playing)
{
    playing_ = playing;
    if (!playing)
    {
        playingReference_ = playingTogether_ = false;
        playingVersion_ = -1;
        updateVersionMarks();
    }
    playButton_.setButtonText(playing && !playingReference_ && !playingTogether_ ? u8("■") : u8("▶"));
    refPlayButton_.setButtonText(playingReference_ ? tr(S::stopRef, lang_) : tr(S::playRef, lang_));
    togetherButton_.setButtonText(playingTogether_ ? tr(S::stopRef, lang_) : tr(S::playTogether, lang_));
    playing ? timer_.startTimerHz(20) : timer_.stopTimer();
}

void MainView::openDataRoot(const std::string& root)
{
    demo_ = false;
    model_.setDataRoot(root);
    if (onDataRootChanged)
        onDataRootChanged(root);
    refs_ = model_.references();
    library_.updateContent();
    library_.repaint();
    if (refs_.empty())
    {
        refTrack_->view = {};
        refTrack_->view.problem = "guide";
        refTrack_->repaint();
    }
    updateLibraryButtons();
    setStatus(tr(S::dataStatus, lang_) + u8(root) + " · " + juce::String(static_cast<int>(refs_.size())) + tr(S::references, lang_));
    for (int i = 0; i < static_cast<int>(refs_.size()); ++i)
        if (refs_[static_cast<std::size_t>(i)].canDerive)
        {
            library_.selectRow(i);
            break;
        }
}

void MainView::showEmpty()
{
    demo_ = false;
    refs_.clear();
    responses_.clear();
    refIndex_ = responseIndex_ = -1;
    library_.updateContent();
    refTrack_->view = {};
    refTrack_->view.problem = "guide";
    responseTrack_->view = {};
    eyeTrack_->view = {};
    refButton_.setButtonText(tr(S::refEmpty, lang_));
    libraryCard_.setText({}, juce::dontSendNotification);
    setStatus(tr(S::guide, lang_).upToFirstOccurrenceOf("\n", false, false));
    updateLibraryButtons();
    repaint();
}

bool MainView::ensureDataRoot()
{
    if (!demo_ && !model_.dataRoot().empty())
        return true;
    const auto root = defaultDataRoot != juce::File() ? defaultDataRoot
                                                      : juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Trioio");
    if (!root.createDirectory())
    {
        setStatus(u8("não foi possível criar ") + root.getFullPathName());
        return false;
    }
    openDataRoot(root.getFullPathName().toStdString());
    setStatus(tr(S::dataKept, lang_) + root.getFullPathName());
    return true;
}

void MainView::bringRecording()
{
    if (importing_)
        return;
    auto chooser = std::make_shared<juce::FileChooser>(tr(S::chooseRecording, lang_),
                                                       juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                                       "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.opus;*.m4a;*.aac;*.webm;*.mid;*.midi");
    juce::Component::SafePointer<MainView> self(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [self, chooser](const juce::FileChooser& fc) {
        const auto f = fc.getResult();
        if (!self || !f.existsAsFile() || !self->ensureDataRoot())
            return;
        self->importing_ = true;
        self->setStatus(tr(S::registering, self->lang_) + f.getFileName() + u8("…"));
        const auto root = self->model_.dataRoot();
        // registrar e calcular o id (o SHA-256 do conteúdo, como o importador) fora da thread da tela
        juce::Thread::launch([self, f, root] {
            const auto rep = library::scanLibrary(f.getFullPathName().toStdString(), root);
            std::string id;
            juce::FileInputStream in(f);
            if (in.openedOk())
                id = "ref-" + juce::SHA256(in, -1).toHexString().substring(0, 16).toStdString();
            juce::MessageManager::callAsync([self, rep, id, f] {
                if (!self)
                    return;
                self->importing_ = false;
                if (!rep.errors.empty() || (rep.registered == 0 && rep.unchanged == 0 && rep.relocated == 0 && rep.duplicates == 0
                                            && rep.reappeared == 0))
                {
                    const auto why = !rep.errors.empty() ? rep.errors.front() : !rep.skipped.empty() ? rep.skipped.front() : std::string("?");
                    self->setStatus(u8(why));
                    return;
                }
                self->reloadReferences(id);
                if (self->refIndex_ >= 0 && self->refs_[static_cast<std::size_t>(self->refIndex_)].status == "pendente")
                    self->showAuth(true);
                else if (rep.registered == 0)
                    self->setStatus(f.getFileName() + tr(S::alreadyKnown, self->lang_));
            });
        });
    });
}

void MainView::reloadReferences(const std::string& selectId)
{
    refs_ = model_.references();
    library_.updateContent();
    library_.repaint();
    for (int i = 0; i < static_cast<int>(refs_.size()); ++i)
        if (refs_[static_cast<std::size_t>(i)].id == selectId)
        {
            library_.selectRow(i);  // → selectReference (lê, se puder)
            break;
        }
    updateLibraryButtons();
}

void MainView::showAuth(bool show)
{
    if (!show)
    {
        if (auth_)
            auth_->setVisible(false);
        return;
    }
    if (refIndex_ < 0 || demo_)
        return;
    if (!auth_)
    {
        auto a = std::make_unique<AuthOverlay>();
        a->onClose = [this] { showAuth(false); };
        a->onAuthorise = [this](const library::AuthorizationRequest& q) {
            const auto r = library::authorize(model_.dataRoot(), q);
            auto* overlay = dynamic_cast<AuthOverlay*>(auth_.get());
            if (!r.written)
            {
                if (overlay)
                    overlay->showProblems(r.problems);
                return;
            }
            if (q.declaredBy != declarer_.toStdString())
            {
                declarer_ = u8(q.declaredBy);
                if (onDeclarerChanged)
                    onDeclarerChanged(declarer_);
            }
            showAuth(false);
            reloadReferences(q.id);
            setStatus(tr(S::nowCompose, lang_));
        };
        a->onRefuse = [this] {
            auto* overlay = dynamic_cast<AuthOverlay*>(auth_.get());
            const auto r = library::refuse(model_.dataRoot(), overlay ? overlay->id : std::string(), declarer_.toStdString(), {});
            if (!r.written)
            {
                if (overlay)
                    overlay->showProblems(r.problems);
                return;
            }
            showAuth(false);
            reloadReferences(overlay ? overlay->id : std::string());
        };
        auth_ = std::move(a);
        addChildComponent(*auth_);
    }
    const auto& ref = refs_[static_cast<std::size_t>(refIndex_)];
    if (auto* overlay = dynamic_cast<AuthOverlay*>(auth_.get()))
        overlay->open(ref.id, u8(ref.name), declarer_, lang_);
    auth_->setBounds(getLocalBounds());
    auth_->setVisible(true);
    auth_->toFront(true);
    auth_->grabKeyboardFocus();
}

void MainView::updateLibraryButtons()
{
    const bool hasRef = !demo_ && refIndex_ >= 0 && refIndex_ < static_cast<int>(refs_.size());
    const auto status = hasRef ? refs_[static_cast<std::size_t>(refIndex_)].status : std::string();
    authButton_.setVisible(hasRef && (status == "pendente" || status == "recusado" || status == "revogado")
                           && (regime_ == Regime::full || page_ == 1));
    bringBigButton_.setVisible(!demo_ && refs_.empty());
    refPlayButton_.setVisible(hasRef && status == "autorizado" && !refs_[static_cast<std::size_t>(refIndex_)].path.empty());
    togetherButton_.setVisible(refPlayButton_.isVisible() && responseIndex_ >= 0 && responseIndex_ < static_cast<int>(responses_.size())
                               && !responses_[static_cast<std::size_t>(responseIndex_)].wavPath.empty());
}

void MainView::togglePlayTogether()
{
    if (playingTogether_)
    {
        if (onStop)
            onStop();
        setPlaying(false);
        return;
    }
    if (refIndex_ < 0 || responseIndex_ < 0 || responseIndex_ >= static_cast<int>(responses_.size()) || !onPlayTogether)
        return;
    playingReference_ = false;
    onPlayTogether(responses_[static_cast<std::size_t>(responseIndex_)].wavPath, refs_[static_cast<std::size_t>(refIndex_)].path);
    playingTogether_ = true;
    setPlaying(true);
}

void MainView::toggleReferencePlayback()
{
    if (playingReference_)
    {
        if (onStop)
            onStop();
        setPlaying(false);
        return;
    }
    if (refIndex_ < 0 || !onPlay)
        return;
    onPlay(refs_[static_cast<std::size_t>(refIndex_)].path);
    playingTogether_ = false;
    playingReference_ = true;
    setPlaying(true);
}

void MainView::showDemo()
{
    demo_ = true;
    refs_ = { { "ref-demo", "Track-68.mp3", "", "autorizado", "IA", 138.0, true },
              { "ref-demo2", "Recording_187.wav", "", "autorizado", "autoral", 224.0, true },
              { "ref-demo3", "sem-titulo.wav", "", "pendente", "origem desconhecida", 65.0, false } };
    library_.updateContent();
    refIndex_ = 0;
    refButton_.setButtonText(u8("Track-68.mp3 · ") + trKey("autorizado", lang_));
    refTrack_->view = demoReference();
    responseTrack_->view = demoResponse();
    eyeTrack_->view = responseTrack_->view;
    refTrack_->duration = responseTrack_->duration = eyeTrack_->duration = 138.0;
    readingLabel_.setText(u8("84,9 BPM · 4 ") + tr(S::beats, lang_) + u8(" · Sol menor · maqam Hijaz sobre Ré"), juce::dontSendNotification);
    responsePicker_.clear(juce::dontSendNotification);
    responsePicker_.addItem(tr(S::modes[0], lang_) + u8(" · seed 1 (demo)"), 1);
    responsePicker_.setSelectedId(1, juce::dontSendNotification);
    timeLabel_.setText("1:12 / 2:18", juce::dontSendNotification);
    libraryCard_.setText(tr(S::origin, lang_) + trKey("IA", lang_) + "\n" + tr(S::uses, lang_), juce::dontSendNotification);
    setStatus(tr(S::demo, lang_));
    repaint();
}

int MainView::getNumRows() { return static_cast<int>(refs_.size()); }

void MainView::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row < 0 || row >= static_cast<int>(refs_.size()))
        return;
    const auto& r = refs_[static_cast<std::size_t>(row)];
    auto box = juce::Rectangle<int>(0, 0, width, height).reduced(2, 3).toFloat();
    g.setColour(selected ? Tokens::responseBlock : Tokens::background);
    g.fillRoundedRectangle(box, 6.0f);
    g.setColour(r.canDerive ? Tokens::line : Tokens::warn);
    g.drawRoundedRectangle(box, 6.0f, 1.0f);
    g.setColour(Tokens::text);
    g.setFont(juce::FontOptions(Tokens::bodyFont));
    g.drawText(u8(r.name), box.reduced(10.0f, 6.0f), juce::Justification::topLeft, true);
    g.setColour(r.canDerive ? Tokens::text2 : Tokens::warn);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(trKey(r.origin, lang_) + u8(" · ") + trKey(r.status, lang_) + u8(" · ") + clock(r.duration), box.reduced(10.0f, 6.0f),
               juce::Justification::bottomLeft, true);
}

void MainView::selectedRowsChanged(int last)
{
    if (!demo_)
        selectReference(last);
}

void MainView::selectReference(int index)
{
    if (index < 0 || index >= static_cast<int>(refs_.size()))
        return;
    refIndex_ = index;
    const auto ref = refs_[static_cast<std::size_t>(index)];
    // A e B seguem as mesmas versões (pelo id) quando a lista é relida — uma
    // resposta nova muda as posições; noutra referência, elas somem
    const auto idOf = [this](int i) {
        return i >= 0 && i < static_cast<int>(responses_.size()) ? responses_[static_cast<std::size_t>(i)].derivationId : std::string();
    };
    const auto idA = idOf(versionA_), idB = idOf(versionB_);
    responses_ = model_.responsesFor(ref.id);
    const auto indexOf = [this](const std::string& id) {
        for (int i = 0; i < static_cast<int>(responses_.size()); ++i)
            if (!id.empty() && responses_[static_cast<std::size_t>(i)].derivationId == id)
                return i;
        return -1;
    };
    versionA_ = indexOf(idA);
    versionB_ = indexOf(idB);
    playingVersion_ = -1;
    refreshReferenceTexts();
    updateLibraryButtons();
    if (ref.status == "pendente" || ref.status == "recusado" || ref.status == "revogado")
    {
        refTrack_->view = {};
        refTrack_->view.problem = tr(S::pendingHint, lang_).toStdString();
        refTrack_->repaint();
        responseTrack_->view = {};
        responseTrack_->repaint();
        setStatus(tr(S::pendingHint, lang_));
        return;
    }
    refTrack_->view = {};
    refTrack_->view.problem = "…";  // "ouvindo a referência…", no idioma da faixa
    refTrack_->repaint();
    setStatus(tr(S::listeningTo, lang_) + u8(ref.name) + tr(S::firstAnalysis, lang_));
    setPage(0);
    juce::Component::SafePointer<MainView> self(this);
    const auto model = model_;
    const auto id = ref.id;
    juce::Thread::launch([self, model, id] {
        auto view = model.loadReference(id);
        juce::MessageManager::callAsync([self, view, id] {
            if (!self || self->refIndex_ < 0 || self->refs_[static_cast<std::size_t>(self->refIndex_)].id != id)
                return;
            self->refTrack_->view = view;
            self->refTrack_->duration = std::max(view.duration, self->responseTrack_->view.duration);
            if (view.ok)
                self->refreshReferenceTexts();
            else
                self->readingLabel_.setText(u8(view.problem), juce::dontSendNotification);
            self->setStatus(view.ok ? tr(S::referenceRead, self->lang_) : u8(view.problem));
            self->refTrack_->repaint();
        });
    });
    // a primeira resposta da referência nova, explicitamente: o seletor pode
    // já estar no índice 0 (refreshReferenceTexts restaura a posição), e um
    // setSelectedItemIndex igual não avisa — a faixa ficava com a resposta da
    // referência anterior
    if (!responses_.empty())
    {
        responsePicker_.setSelectedItemIndex(0, juce::dontSendNotification);
        showResponse(0);
    }
    else
        showResponse(-1);
}

void MainView::selectReferenceById(const std::string& id)
{
    for (int i = 0; i < static_cast<int>(refs_.size()); ++i)
        if (refs_[static_cast<std::size_t>(i)].id == id)
            library_.selectRow(i);
}

void MainView::revealEye(bool reveal)
{
    revealButton_.setToggleState(reveal, juce::sendNotificationSync);
    eyeTrack_->revealed = reveal;
    revealButton_.setButtonText(reveal ? tr(S::hideEye, lang_) : tr(S::reveal, lang_));
}

void MainView::loadReferenceNow()
{
    if (refIndex_ < 0 || demo_ || refs_[static_cast<std::size_t>(refIndex_)].status != "autorizado")
        return;  // pendente/recusada: fica a orientação de selectReference
    const auto view = model_.loadReference(refs_[static_cast<std::size_t>(refIndex_)].id);
    setStatus(view.ok ? tr(S::referenceRead, lang_) : u8(view.problem));
    refTrack_->view = view;
    refTrack_->duration = std::max(view.duration, responseTrack_->view.duration);
    refreshReferenceTexts();
}

// os textos que dependem da referência carregada E do idioma: refeitos ao
// escolher a referência e ao trocar o idioma (antes, a troca deixava o
// seletor, a leitura e a ficha no idioma anterior)
void MainView::refreshReferenceTexts()
{
    if (demo_ || refIndex_ < 0 || refIndex_ >= static_cast<int>(refs_.size()))
        return;
    const auto& ref = refs_[static_cast<std::size_t>(refIndex_)];
    refButton_.setButtonText(u8(ref.name) + u8(" · ") + trKey(ref.status, lang_));
    libraryCard_.setText(tr(S::origin, lang_) + trKey(ref.origin, lang_) + "\n" + tr(S::state, lang_) + trKey(ref.status, lang_) + "\n"
                             + (ref.canDerive ? tr(S::uses, lang_) : tr(S::notDerivable, lang_)),
                         juce::dontSendNotification);
    const int selected = responsePicker_.getSelectedItemIndex();
    responsePicker_.clear(juce::dontSendNotification);
    for (int i = 0; i < static_cast<int>(responses_.size()); ++i)
    {
        const auto& r = responses_[static_cast<std::size_t>(i)];
        responsePicker_.addItem(trKey(r.mode, lang_) + u8(" · seed ") + juce::String(static_cast<juce::int64>(r.seed)) + u8(" · ")
                                    + u8(r.sessionId).fromFirstOccurrenceOf("sessao-", false, false).substring(0, 12),
                                i + 1);
    }
    if (selected >= 0 && selected < static_cast<int>(responses_.size()))
        responsePicker_.setSelectedItemIndex(selected, juce::dontSendNotification);
    const auto& view = refTrack_->view;
    if (view.ok)
        readingLabel_.setText(juce::String(view.bpm, 1) + u8(" BPM · ") + juce::String(view.beatsPerBar) + " " + tr(S::beats, lang_)
                                  + u8(" · ") + u8(view.key) + u8(" · ") + u8(view.language),
                              juce::dontSendNotification);
}

void MainView::showResponse(int index)
{
    responseIndex_ = index;
    responseTrack_->clearSelection();
    updateJudgeLabel();
    updateLibraryButtons();
    if (index < 0 || index >= static_cast<int>(responses_.size()))
    {
        responseTrack_->view = {};
        eyeTrack_->view = {};
    }
    else
    {
        responseTrack_->view = model_.loadResponse(responses_[static_cast<std::size_t>(index)]);
        eyeTrack_->view = responseTrack_->view;
    }
    const double d = std::max(refTrack_->view.duration, responseTrack_->view.duration);
    refTrack_->duration = responseTrack_->duration = eyeTrack_->duration = d > 0.0 ? d : 1.0;
    timeLabel_.setText("0:00 / " + clock(responseTrack_->view.duration), juce::dontSendNotification);
    eyeTrack_->revealed = false;
    revealButton_.setToggleState(false, juce::dontSendNotification);
    revealButton_.setButtonText(tr(S::reveal, lang_));
    tracksHolder_.repaint();
    refTrack_->repaint();
    eyeTrack_->repaint();
    responseTrack_->repaint();
    if (page_ == 2)
        refreshListeningPage();  // as regras do jogo são da versão escolhida
}

void MainView::judge(int verdict)
{
    if (demo_ || responseIndex_ < 0)
    {
        setStatus(tr(S::nothingToJudge, lang_));
        return;
    }
    const auto v = verdict == 0 ? composition::Verdict::accept : verdict == 1 ? composition::Verdict::reject : composition::Verdict::transform;
    // o trecho vai em m:ss (segundos inteiros, sem depender da vírgula decimal do locale)
    std::string span, section;
    if (responseTrack_->selSection > 0)
        section = std::to_string(responseTrack_->selSection);
    else if (responseTrack_->hasSelection())
    {
        const double end = responseTrack_->view.duration;
        span = clock(std::floor(responseTrack_->selStart)).toStdString() + "-"
             + (responseTrack_->selEnd >= end - 0.5 ? std::string("fim") : clock(std::ceil(responseTrack_->selEnd)).toStdString());
    }
    const auto r = library::recordJudgement(model_.dataRoot(), responses_[static_cast<std::size_t>(responseIndex_)].derivationId, v,
                                            note_.getText().toStdString(), {}, span, section);
    setStatus(r.written ? tr(S::judged, lang_) + tr(S::verdicts[verdict], lang_) + judgeTarget() + tr(S::judgedTail, lang_)
                        : u8(r.problems.empty() ? std::string("?") : r.problems.front()));
    if (r.written)
    {
        note_.clear();
        responseTrack_->clearSelection();
        updateJudgeLabel();
        if (page_ == 2)
            refreshListeningPage();
    }
}

juce::String MainView::judgeTarget() const
{
    const auto& t = *responseTrack_;
    if (t.selSection > 0)
        // a letra do bloco desenhado (A, B, …); o núcleo recebe o número da seção
        return tr(S::judgeSection, lang_) + juce::String::charToString(static_cast<juce::juce_wchar>('A' + (t.selSection - 1) % 26));
    if (t.hasSelection())
        return tr(S::judgeSpan, lang_) + clock(std::floor(t.selStart)) + u8("–")
             + (t.selEnd >= t.view.duration - 0.5 ? tr(S::spanEnd, lang_) : clock(std::ceil(t.selEnd)));
    return {};
}

void MainView::selectSpan(double a, double b, int section)
{
    auto& t = *responseTrack_;
    if (section > 0 && section <= static_cast<int>(t.view.sections.size()))
    {
        a = t.view.sections[static_cast<std::size_t>(section - 1)].start;
        b = t.view.sections[static_cast<std::size_t>(section - 1)].end;
    }
    else
        section = 0;
    t.selStart = std::max(0.0, a);
    t.selEnd = std::min(b, t.view.duration);
    t.selSection = section;
    if (!t.hasSelection())
        t.clearSelection();
    t.repaint();
    updateJudgeLabel();
}

void MainView::updateJudgeLabel()
{
    const auto target = judgeTarget();
    judgeLabel_.setText(target.isEmpty() ? tr(S::judgeWhole, lang_) : tr(S::judgeOnly, lang_) + target, juce::dontSendNotification);
    judgeLabel_.setColour(juce::Label::textColourId, target.isEmpty() ? Tokens::text2 : Tokens::text);
    judgeLabel_.setTooltip(tr(S::judgeHint, lang_));
}

void MainView::composeAgain()
{
    if (demo_ || refIndex_ < 0)
    {
        setStatus(tr(S::chooseAuthorised, lang_));
        return;
    }
    const auto& ref = refs_[static_cast<std::size_t>(refIndex_)];
    if (!ref.canDerive)
    {
        setStatus(tr(S::cannotDerive, lang_));
        return;
    }
    library::ResponseRequest q;
    q.id = ref.id;
    const strategy::Mode modes[] = { strategy::Mode::follow, strategy::Mode::transfigure, strategy::Mode::counter,
                                     strategy::Mode::interrupt };
    q.mode = modes[mode_];
    q.seed = static_cast<std::uint64_t>(juce::Time::currentTimeMillis() % 100000);
    q.unpredictability = static_cast<float>(unpredictability_.getValue());
    q.rasgo = static_cast<float>(rasgo_.getValue());
    q.humanize = true;  // a humanização fina (Production.h)
    q.writeHarmony = true;  // a harmonia escrita (HarmonyWriting.h)
    q.harmonyLanguage = std::max(0, harmonyLang_.getSelectedItemIndex());
    const composition::PitchSource pitches[] = { composition::PitchSource::archive, composition::PitchSource::reference,
                                                 composition::PitchSource::modal, composition::PitchSource::microtonal,
                                                 composition::PitchSource::invented, composition::PitchSource::just };
    q.pitch = pitches[std::max(0, pitch_.getSelectedItemIndex())];
    q.mix = mix_.getText().toStdString();
    q.promptText = words_.getText().trim().toStdString();
    q.semantic = semantic_.getToggleState();
    const composition::MixTechnique techniques[] = { composition::MixTechnique::collage, composition::MixTechnique::hybrid,
                                                     composition::MixTechnique::layers, composition::MixTechnique::transition };
    q.mixTechnique = techniques[std::max(0, technique_.getSelectedItemIndex())];
    if (cycle_.getSelectedItemIndex() > 0)
        q.cycle = cycle_.getText().toStdString();
    q.formByRepetition = form_.getSelectedItemIndex() == 1;
    q.seam = std::max(0, seam_.getSelectedItemIndex());  // corte pivô ponte sobreposição = a ordem de Seam
    const int voicingOf[] = { -1, 0, 1, 2, 6 };          // o da escala, terças, quartal, So What, cluster (Voicing)
    if (const int vi = voicing_.getSelectedItemIndex(); vi > 0 && vi < 5)
        q.voicing = voicingOf[vi];
    const composition::Intention intentions[] = { composition::Intention::none, composition::Intention::arc,
                                                  composition::Intention::luminous, composition::Intention::somber,
                                                  composition::Intention::tense, composition::Intention::serene,
                                                  composition::Intention::suspended };
    q.intention = intentions[std::max(0, intention_.getSelectedItemIndex())];
    q.tempoScale = tempo_.getSelectedItemIndex() == 1 ? 0.5 : tempo_.getSelectedItemIndex() == 2 ? 2.0 : 1.0;
    q.dialogue = dialogue_.getToggleState();
    q.game = game_.getToggleState();
    // o item 1 é "lido na referência" (−1); os outros seguem a ordem de Ensemble
    if (const int ei = ensemble_.getSelectedItemIndex(); ei == 1)
        q.ensemble = -1;
    else
        q.ensemble = ei > 1 ? ei - 1 : 0;
    q.filmFx = filmFx_.getToggleState();
    q.somDaAura = somDaAura_.getToggleState();
    q.stems = stems_.getToggleState();
    // as influências: a lista separada por vírgulas (ou "auto"), a força, o que sai
    const auto splitList = [](const juce::String& text) {
        std::vector<std::string> out;
        juce::StringArray parts;
        parts.addTokens(text, ",", "");
        for (auto p : parts)
            if (p.trim().isNotEmpty())
                out.push_back(p.trim().toStdString());
        return out;
    };
    q.influences = splitList(influences_.getText());
    q.influenceStrength = static_cast<float>(influenceStrength_.getValue());
    q.exclude = splitList(exclude_.getText());
    // o gesto: só entra se estiver bem escrito (senão, a curva do modo)
    if (!trioio::composition::parseGesture(gesture_.getText().toStdString(), q.gesture))
        q.gesture.clear();
    q.useListening = useListening_;
    composeButton_.setEnabled(false);
    setStatus(tr(S::composing, lang_));
    juce::Component::SafePointer<MainView> self(this);
    const auto root = model_.dataRoot();
    // a barra: cada etapa da resposta chega da thread dela para esta
    q.progress = [self](float f, const std::string& what) {
        juce::MessageManager::callAsync([self, f, what] {
            if (!self)
                return;
            self->progress_ = f;
            self->progressStage_ = trStage(what, self->lang_);
            self->updateProgressText();
        });
    };
    showProgress(true);
    juce::Thread::launch([self, root, q] {
        const auto result = library::respond(root, q);
        juce::MessageManager::callAsync([self, result] {
            if (!self)
                return;
            self->showProgress(false);
            self->composeButton_.setEnabled(true);
            if (!result.ok)
            {
                self->setStatus(result.problems.empty() ? tr(S::composeFailed, self->lang_) : u8(result.problems.front()));
                return;
            }
            self->setStatus(tr(S::newResponse, self->lang_) + u8(result.derivationId));
            self->selectReference(self->refIndex_);
        });
    });
}

// a ponte semântica enquanto se escreve: só as palavras que o léxico não
// leu, em segundo plano; o que vier fica na memória (a resposta não espera)
void MainView::prefetchSemantic()
{
    if (!semantic_.getToggleState() || demo_ || model_.dataRoot().empty())
    {
        semanticStatus_.setText({}, juce::dontSendNotification);
        return;
    }
    // só as palavras completas: a última, enquanto não vier um separador, ainda
    // está sendo escrita ("jana" não vai para a rede antes de "janaese,")
    auto typed = words_.getText();
    if (typed.isNotEmpty() && juce::CharacterFunctions::isLetterOrDigit(typed.getLastCharacter()))
    {
        int cut = -1;
        for (int i = typed.length() - 1; i >= 0 && cut < 0; --i)
            if (!juce::CharacterFunctions::isLetterOrDigit(typed[i]) && typed[i] != '-' && typed[i] != '~' && typed[i] != '^')
                cut = i;
        typed = cut < 0 ? juce::String() : typed.substring(0, cut);
    }
    const auto text = typed.trim().toStdString();
    const auto unread = composition::translatePrompt(text).unread;
    if (text.empty() || unread.empty())
    {
        semanticStatus_.setText(text.empty() ? juce::String() : tr(S::semanticNothing, lang_), juce::dontSendNotification);
        return;
    }
    // uma consulta por vez: com a rede lenta, as pausas da escrita não empilham
    // consultas (a resposta completa o que faltar quando for composta)
    if (semanticBusy_->exchange(true))
        return;
    semanticStatus_.setText(tr(S::semanticAsking, lang_), juce::dontSendNotification);
    const int gen = semanticGeneration_;
    juce::Component::SafePointer<MainView> self(this);
    const auto root = model_.dataRoot();
    std::vector<std::string> ask(unread.begin(), unread.begin() + std::min<size_t>(5, unread.size()));
    juce::Thread::launch([self, root, ask, gen, busy = semanticBusy_] {
        const auto res = library::lookupSemantic(root, ask, library::defaultSemanticSources(), 6000);
        busy->store(false);
        juce::String line;
        for (const auto& r : res)
            line << (line.isEmpty() ? "" : "   ") << u8(r.word) << (r.ok ? u8(" → ") + u8(r.english) : juce::String(" ?"));
        juce::MessageManager::callAsync([self, line, gen] {
            if (self && gen == self->semanticGeneration_)
                self->semanticStatus_.setText(tr(S::semanticPrefix, self->lang_) + line, juce::dontSendNotification);
        });
    });
}

void MainView::showProgress(bool on)
{
    progress_ = on ? 0.0 : -1.0;
    progressStage_ = on ? tr(S::composing, lang_) : juce::String();
    composeStartMs_ = juce::Time::getMillisecondCounterHiRes();
    progressBar_.setVisible(on);
    on ? progressClock_.startTimer(500) : progressClock_.stopTimer();
    updateProgressText();
    resized();
}

void MainView::updateProgressText()
{
    const int secs = static_cast<int>((juce::Time::getMillisecondCounterHiRes() - composeStartMs_) / 1000.0);
    progressBar_.setTextToDisplay(progressStage_ + u8(" · ") + juce::String(secs) + " s");
}

void MainView::setStatus(const juce::String& s) { statusLabel_.setText(s, juce::dontSendNotification); }

void MainView::setPage(int page)
{
    page_ = std::clamp(page, 0, 2);
    pageButtons_[page_].setToggleState(true, juce::dontSendNotification);
    if (page_ == 2)
        refreshListeningPage();
    resized();
}

void MainView::compareVersions()
{
    const int n = static_cast<int>(responses_.size());
    if (versionA_ < 0 || versionB_ < 0 || versionA_ >= n || versionB_ >= n || !onPlayAt)
    {
        setStatus(tr(S::needTwo, lang_));
        return;
    }
    playingReference_ = playingTogether_ = false;
    playingVersion_ = versionA_;
    onPlayAt(responses_[static_cast<std::size_t>(versionA_)].wavPath, 0.0);
    setPlaying(true);
    updateVersionMarks();
    setStatus(tr(S::playingA, lang_) + trKey(responses_[static_cast<std::size_t>(versionA_)].mode, lang_) + u8(" · seed ")
              + juce::String(static_cast<juce::int64>(responses_[static_cast<std::size_t>(versionA_)].seed)));
}

void MainView::switchVersions()
{
    const int n = static_cast<int>(responses_.size());
    if (versionA_ < 0 || versionB_ < 0 || versionA_ >= n || versionB_ >= n || !onPlayAt)
    {
        setStatus(tr(S::needTwo, lang_));
        return;
    }
    // o mesmo instante na outra versão (as respostas têm a duração da referência)
    const double at = playPosition ? std::max(0.0, playPosition()) : 0.0;
    playingVersion_ = playingVersion_ == versionA_ ? versionB_ : versionA_;
    const bool isA = playingVersion_ == versionA_;
    playingReference_ = playingTogether_ = false;
    onPlayAt(responses_[static_cast<std::size_t>(playingVersion_)].wavPath, at);
    setPlaying(true);
    updateVersionMarks();
    const auto& r = responses_[static_cast<std::size_t>(playingVersion_)];
    setStatus((isA ? tr(S::playingA, lang_) : tr(S::playingB, lang_)) + trKey(r.mode, lang_) + u8(" · seed ")
              + juce::String(static_cast<juce::int64>(r.seed)) + u8(" · ") + juce::String(at, 1) + " s");
}

void MainView::refreshListeningPage()
{
    auto* page = dynamic_cast<ListeningPage*>(listeningPage_.get());
    if (!page)
        return;
    page->lang = lang_;
    std::vector<composition::Judgement> history;
    if (!demo_ && !model_.dataRoot().empty())
        history = library::loadJudgements(model_.dataRoot());
    // as versões desta referência, com o último veredito e a nota de cada uma
    std::vector<ListeningPage::Row> rows;
    for (const auto& r : responses_)
    {
        ListeningPage::Row row;
        row.title = trKey(r.mode, lang_) + u8(" · seed ") + juce::String(static_cast<juce::int64>(r.seed)) + u8(" · ")
                  + u8(r.sessionId).fromFirstOccurrenceOf("sessao-", false, false).substring(0, 12);
        juce::String note;
        int parts = 0;
        for (const auto& j : history)
            if (j.derivationId == r.derivationId)
            {
                if (j.hasSpan())
                    ++parts;
                else
                    row.verdict = static_cast<int>(j.verdict);
                if (!j.note.empty())
                    note = u8(j.note);
                if (row.verdict < 0)
                    row.verdict = static_cast<int>(j.verdict);
            }
        row.detail = (row.verdict < 0 ? tr(S::notJudged, lang_) : tr(S::verdictShort[row.verdict], lang_))
                   + (parts > 0 ? u8(" · ") + juce::String(parts) + u8(" trecho(s)") : juce::String())
                   + (note.isNotEmpty() ? u8(" · “") + note + u8("”") : juce::String());
        rows.push_back(row);
    }
    // o que a escuta ensinou: as tendências que saíram do neutro, as maiores primeiro
    const auto tend = composition::tendenciesFrom(history);
    std::vector<ListeningPage::Bar> bars;
    const auto add = [&](const juce::String& group, std::vector<ListeningPage::Bar> items) {
        std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
            return std::abs(std::log(a.factor)) > std::abs(std::log(b.factor));
        });
        for (auto& b : items)
        {
            b.group = group;
            bars.push_back(b);
        }
    };
    std::vector<ListeningPage::Bar> tb, eb, rb;
    for (int i = 0; i < composition::kTimbreCount; ++i)
        if (std::abs(std::log(tend.timbre[static_cast<std::size_t>(i)])) > 0.01f)
            tb.push_back({ {}, trName(composition::timbreName(static_cast<composition::Timbre>(i)), lang_), tend.timbre[static_cast<std::size_t>(i)] });
    for (int i = 0; i < composition::kEyeChoices; ++i)
        if (std::abs(std::log(tend.eye[static_cast<std::size_t>(i)])) > 0.01f)
            eb.push_back({ {}, trName(composition::eyeChoiceName(i), lang_), tend.eye[static_cast<std::size_t>(i)] });
    for (int i = 0; i < composition::kRooms; ++i)
        if (std::abs(std::log(tend.room[static_cast<std::size_t>(i)])) > 0.01f)
            rb.push_back({ {}, trName(composition::roomName(static_cast<composition::Room>(i)), lang_), tend.room[static_cast<std::size_t>(i)] });
    add(tr(S::groupTimbre, lang_), tb);
    add(tr(S::groupEye, lang_), eb);
    add(tr(S::groupRoom, lang_), rb);
    page->rules.clear();
    for (const auto& line : responseTrack_->view.gameRules)
        page->rules.push_back(u8(line));
    // sem escolha ainda, A é a resposta que está na faixa
    if (versionA_ < 0 && responseIndex_ >= 0 && responseIndex_ < static_cast<int>(responses_.size()))
        versionA_ = responseIndex_;
    page->set(std::move(rows), std::move(bars), static_cast<int>(history.size()), responseIndex_, useListening_);
    updateVersionMarks();
}

void MainView::updateVersionMarks()
{
    if (auto* page = dynamic_cast<ListeningPage*>(listeningPage_.get()))
        page->setMarks(versionA_, versionB_, playing_ && !playingReference_ && !playingTogether_ ? playingVersion_ : -1);
}

std::vector<std::pair<juce::String, juce::Component*>> MainView::essentials()
{
    return { { u8("referência"), &refButton_ }, { u8("seguir"), &modeButtons_[0] }, { u8("transfigurar"), &modeButtons_[1] },
             { u8("contrapor"), &modeButtons_[2] }, { u8("interromper"), &modeButtons_[3] }, { u8("tocar"), &playButton_ },
             { u8("aceitar"), &acceptButton_ }, { u8("transformar"), &transformButton_ }, { u8("rejeitar"), &rejectButton_ },
             { u8("compor de novo"), &composeButton_ }, { u8("idioma"), &langButton_ }, { u8("master"), &master_ } };
}

void MainView::paint(juce::Graphics& g)
{
    g.fillAll(Tokens::background);
    g.setColour(Tokens::line);
    const int headerH = regime_ == Regime::full ? 72 : regime_ == Regime::compact ? 56 : 52;
    g.drawHorizontalLine(headerH, 0.0f, static_cast<float>(getWidth()));
    // o símbolo: a logo dos três olhos do autor (2026-10-13), só os olhos —
    // em cima o Terceiro Olho (âmbar), embaixo o que participa e o que observa
    // (docs/TRIOIO_CONCEITO.md §1). A mesma geometria de
    // website/assets/identity/trioio-olho.svg (grade 64).
    // Na emergência o cabeçalho é só a referência, os modos e o menu.
    if (!title_.isVisible())
        return;
    const auto box = title_.getBounds().withWidth(34).withX(title_.getX() - 40).toFloat().withSizeKeepingCentre(34.0f, 34.0f);
    const float k = box.getWidth() / 64.0f;
    const auto at = [&](float x, float y) { return juce::Point<float>(box.getX() + x * k, box.getY() + y * k); };
    const auto eye = [&](float cx, float cy, float halfW, float pupil) {
        juce::Path p;
        p.startNewSubPath(at(cx - halfW, cy));
        p.quadraticTo(at(cx, cy - halfW), at(cx + halfW, cy));
        p.quadraticTo(at(cx, cy + halfW), at(cx - halfW, cy));
        g.strokePath(p, juce::PathStrokeType(2.8f * k, juce::PathStrokeType::curved));
        g.fillEllipse(juce::Rectangle<float>(2.0f * pupil * k, 2.0f * pupil * k).withCentre(at(cx, cy)));
    };
    g.setColour(Tokens::text);
    eye(16.0f, 44.0f, 13.0f, 4.0f);
    eye(48.0f, 44.0f, 13.0f, 4.0f);
    g.setColour(Tokens::eye);
    eye(32.0f, 21.0f, 15.0f, 4.6f);
}

void MainView::layoutTracks(juce::Rectangle<int> area)
{
    tracksViewport_.setBounds(area);
    const bool scroll = regime_ == Regime::emergency;
    const int w = area.getWidth() - (scroll ? 10 : 0);
    const int h = scroll ? std::max(area.getHeight(), 520) : area.getHeight();
    tracksHolder_.setSize(w, h);
    auto r = juce::Rectangle<int>(0, 0, w, h).reduced(regime_ == Regime::full ? 20 : 10, 8);
    // na primeira abertura a faixa da referência carrega o roteiro e o botão: mais alta
    const int refH = !demo_ && refs_.empty() ? std::clamp(h / 3, 170, 240) : std::clamp(h / 5, 96, 150);
    const int eyeH = std::clamp(h / 7, 64, 100);
    auto pick = r.removeFromTop(Tokens::minTarget);
    responsePicker_.setBounds(pick.removeFromRight(std::min(360, pick.getWidth() / 2)));
    pick.removeFromRight(8);
    refPlayButton_.setBounds(pick.removeFromLeft(std::min(170, pick.getWidth() / 4)));
    pick.removeFromLeft(6);
    togetherButton_.setBounds(pick.removeFromLeft(std::min(140, pick.getWidth() / 5)));
    r.removeFromTop(6);
    refTrack_->setBounds(r.removeFromTop(refH));
    bringBigButton_.setBounds(refTrack_->getBounds().reduced(14, 8).removeFromBottom(Tokens::minTarget).removeFromLeft(220));
    r.removeFromTop(8);
    auto eye = r.removeFromTop(eyeH);
    eyeTrack_->setBounds(eye);
    // revelar fica na linha de cima, junto da resposta escolhida: a faixa do
    // olho inteira fica livre para as intervenções
    revealButton_.setBounds(pick.removeFromRight(std::min(220, pick.getWidth())));
    r.removeFromTop(8);
    responseTrack_->setBounds(r);
}

void MainView::layoutLanguage(juce::Rectangle<int> area)
{
    languagePanel_.setBounds(area);
    languageScroll_.setBounds(languagePanel_.getLocalBounds().withTrimmedLeft(1));
    // a altura de que o conteúdo precisa; se o painel for mais baixo, rola
    constexpr int rowH = 20 + Tokens::minTarget + 6;
    const int needed = 12 + 26 + 6 + 20 + 2 * Tokens::minTarget + 20 + 8 + 6 * rowH + 7 * (20 + Tokens::minTarget) + 5 * Tokens::minTarget + 60 + 6 + 36 + 12;
    const bool scrolls = needed > languageScroll_.getHeight();
    languageContent_.setSize(languageScroll_.getWidth() - (scrolls ? languageScroll_.getScrollBarThickness() : 0),
                             std::max(needed, languageScroll_.getHeight()));
    auto r = languageContent_.getLocalBounds().reduced(14, 12);
    languageTitle_.setBounds(r.removeFromTop(26));
    r.removeFromTop(6);
    wordsLabel_.setBounds(r.removeFromTop(20));
    words_.setBounds(r.removeFromTop(Tokens::minTarget));
    semantic_.setBounds(r.removeFromTop(Tokens::minTarget));
    semanticStatus_.setBounds(r.removeFromTop(20));
    mixLabel_.setBounds(r.removeFromTop(20));
    mix_.setBounds(r.removeFromTop(Tokens::minTarget));
    r.removeFromTop(8);
    juce::Label* labels[] = { &pitchLabel_, &techniqueLabel_, &seamLabel_, &cycleLabel_, &voicingLabel_, &intentionLabel_, &formLabel_, &tempoLabel_ };
    juce::ComboBox* combos[] = { &pitch_, &technique_, &seam_, &cycle_, &voicing_, &intention_, &form_, &tempo_ };
    const int colW = (r.getWidth() - 10) / 2;
    for (int i = 0; i < 8; i += 2)
    {
        auto row = r.removeFromTop(rowH);
        for (int k = 0; k < 2; ++k)
        {
            auto cell = row.removeFromLeft(colW);
            if (k == 0)
                row.removeFromLeft(10);
            labels[i + k]->setBounds(cell.removeFromTop(20));
            combos[i + k]->setBounds(cell.removeFromTop(Tokens::minTarget));
        }
    }
    {
        // o conjunto: a linha inteira (os instrumentos de cada voz, pelo estilo)
        auto row = r.removeFromTop(rowH);
        ensembleLabel_.setBounds(row.removeFromTop(20));
        ensemble_.setBounds(row.removeFromTop(Tokens::minTarget));
    }
    {
        // a harmonia: a linguagem da máquina de harmonia
        auto row = r.removeFromTop(rowH);
        harmonyLangLabel_.setBounds(row.removeFromTop(20));
        harmonyLang_.setBounds(row.removeFromTop(Tokens::minTarget));
    }
    rasgoLabel_.setBounds(r.removeFromTop(20));
    rasgo_.setBounds(r.removeFromTop(Tokens::minTarget));
    unpredLabel_.setBounds(r.removeFromTop(20));
    unpredictability_.setBounds(r.removeFromTop(Tokens::minTarget));
    dialogue_.setBounds(r.removeFromTop(Tokens::minTarget));
    game_.setBounds(r.removeFromTop(Tokens::minTarget));
    filmFx_.setBounds(r.removeFromTop(Tokens::minTarget));
    somDaAura_.setBounds(r.removeFromTop(Tokens::minTarget));
    stems_.setBounds(r.removeFromTop(Tokens::minTarget));
    influencesLabel_.setBounds(r.removeFromTop(20));
    influences_.setBounds(r.removeFromTop(Tokens::minTarget));
    influenceStrengthLabel_.setBounds(r.removeFromTop(20));
    influenceStrength_.setBounds(r.removeFromTop(Tokens::minTarget));
    excludeLabel_.setBounds(r.removeFromTop(20));
    exclude_.setBounds(r.removeFromTop(Tokens::minTarget));
    gestureLabel_.setBounds(r.removeFromTop(20));
    gesture_.setBounds(r.removeFromTop(Tokens::minTarget));
    r.removeFromTop(4);
    gestureEditor_->setBounds(r.removeFromTop(56));
    r.removeFromTop(6);
    languageNote_.setBounds(r.removeFromTop(36));
}

void MainView::layoutFooter(juce::Rectangle<int> f, bool compact)
{
    f.reduce(compact ? 10 : 24, compact ? 8 : 16);
    playButton_.setBounds(f.removeFromLeft(48).withSizeKeepingCentre(48, 48));
    f.removeFromLeft(10);
    timeLabel_.setBounds(f.removeFromLeft(compact ? 92 : 110));
    master_.setBounds(f.removeFromLeft(compact ? 170 : 210).withSizeKeepingCentre(compact ? 170 : 210, Tokens::minTarget));
    f.removeFromLeft(10);
    composeButton_.setBounds(f.removeFromRight(compact ? 150 : 170).withSizeKeepingCentre(compact ? 150 : 170, Tokens::minTarget + 4));
    f.removeFromRight(12);
    rejectButton_.setBounds(f.removeFromRight(compact ? 92 : 110).withSizeKeepingCentre(compact ? 92 : 110, Tokens::minTarget + 4));
    f.removeFromRight(6);
    transformButton_.setBounds(f.removeFromRight(compact ? 112 : 130).withSizeKeepingCentre(compact ? 112 : 130, Tokens::minTarget + 4));
    f.removeFromRight(6);
    acceptButton_.setBounds(f.removeFromRight(compact ? 92 : 110).withSizeKeepingCentre(compact ? 92 : 110, Tokens::minTarget + 4));
    f.removeFromRight(10);
    // o que sobra: o rótulo do julgamento e a nota, se couberem
    const bool roomForNote = f.getWidth() >= 220;
    note_.setVisible(roomForNote);
    judgeLabel_.setVisible(f.getWidth() >= 140);
    if (roomForNote)
    {
        note_.setBounds(f.removeFromRight(std::min(320, f.getWidth() - (f.getWidth() >= 360 ? 140 : 0))).withSizeKeepingCentre(
            std::min(320, f.getWidth()), Tokens::minTarget));
    }
    judgeLabel_.setBounds(f);
}

void MainView::resized()
{
    regime_ = classify(getWidth(), getHeight());
    auto r = getLocalBounds();
    const bool full = regime_ == Regime::full, compact = regime_ == Regime::compact, emergency = regime_ == Regime::emergency;

    // cabeçalho
    auto header = r.removeFromTop(full ? 72 : compact ? 56 : 52);
    header.reduce(full ? 24 : 12, 0);
    header.removeFromLeft(44);
    title_.setVisible(!emergency);
    if (!emergency)
        title_.setBounds(header.removeFromLeft(full ? 120 : 96));
    else
        title_.setBounds(header.getX() - 36, header.getY(), 0, header.getHeight());
    menuButton_.setVisible(emergency);
    if (emergency)
    {
        menuButton_.setBounds(header.removeFromRight(Tokens::minTarget).withSizeKeepingCentre(Tokens::minTarget, Tokens::minTarget));
        header.removeFromRight(4);
    }
    // o SOBRE (no menu, na emergência) e o idioma
    aboutButton_.setVisible(!emergency);
    tutorialButton_.setVisible(!emergency);
    if (!emergency)
    {
        const int aw = lang_ == Lang::es ? 104 : lang_ == Lang::fr ? 96 : 80;
        aboutButton_.setBounds(header.removeFromRight(aw).withSizeKeepingCentre(aw, Tokens::minTarget));
        header.removeFromRight(4);
        const int tw = lang_ == Lang::fr ? 100 : 92;
        tutorialButton_.setBounds(header.removeFromRight(tw).withSizeKeepingCentre(tw, Tokens::minTarget));
        header.removeFromRight(4);
    }
    langButton_.setBounds(header.removeFromRight(Tokens::minTarget).withSizeKeepingCentre(Tokens::minTarget, Tokens::minTarget));
    header.removeFromRight(full ? 16 : 8);
    int modeW[4] = { 74, 112, 96, 108 };
    int total = 0;
    for (int w : modeW) total += w + 4;
    auto modes = header.removeFromRight(total);
    for (int i = 0; i < 4; ++i)
    {
        modeButtons_[i].setBounds(modes.removeFromLeft(modeW[i]).withSizeKeepingCentre(modeW[i], Tokens::minTarget));
        modes.removeFromLeft(4);
    }
    header.removeFromRight(12);
    const int refW = std::min(full ? 380 : 300, header.getWidth());
    refButton_.setBounds(header.removeFromLeft(refW).withSizeKeepingCentre(refW, Tokens::minTarget));
    header.removeFromLeft(12);
    readingLabel_.setVisible(full);
    readingLabel_.setBounds(header);

    // rodapé (a barra de julgamento): sempre
    auto footer = r.removeFromBottom(full ? 84 : 64);
    layoutFooter(footer, !full);
    {
        // a linha de estado: a mensagem à esquerda, o crédito à direita (o
        // padrão da família RASGO: © autor · instrumento ano · licença · versão)
        auto row = r.removeFromBottom(20).reduced(full ? 24 : 12, 0);
        creditLabel_.setBounds(row.removeFromRight(std::min(460, row.getWidth() / 2)));
        if (progressBar_.isVisible())
        {
            progressBar_.setBounds(row.removeFromLeft(std::min(340, row.getWidth() * 2 / 3)).reduced(0, 1));
            row.removeFromLeft(10);
        }
        statusLabel_.setBounds(row);
    }
    if (about_ && about_->isVisible())
        about_->setBounds(getLocalBounds());
    if (tutorial_ && tutorial_->isVisible())
        tutorial_->setBounds(getLocalBounds());
    if (auth_ && auth_->isVisible())
        auth_->setBounds(getLocalBounds());
    updateLibraryButtons();

    // páginas (compacto e emergência)
    for (auto& b : pageButtons_)
        b.setVisible(compact);
    drawerButton_.setVisible(compact);
    if (compact)
    {
        auto tabs = r.removeFromTop(48).reduced(12, 2);
        pageButtons_[0].setBounds(tabs.removeFromLeft(170));
        tabs.removeFromLeft(4);
        pageButtons_[1].setBounds(tabs.removeFromLeft(120));
        tabs.removeFromLeft(4);
        pageButtons_[2].setBounds(tabs.removeFromLeft(130));
        drawerButton_.setBounds(tabs.removeFromRight(140));
    }

    const bool libraryPage = !full && page_ == 1;
    if (full)
    {
        auto lib = r.removeFromLeft(320).reduced(20, 16);
        libraryTitle_.setBounds(lib.removeFromTop(28));
        bringButton_.setBounds(lib.removeFromTop(Tokens::minTarget));
        lib.removeFromTop(8);
        openButton_.setBounds(lib.removeFromBottom(Tokens::minTarget));
        lib.removeFromBottom(8);
        authButton_.setBounds(lib.removeFromBottom(Tokens::minTarget));
        lib.removeFromBottom(6);
        libraryCard_.setBounds(lib.removeFromBottom(96));
        library_.setBounds(lib);
        layoutLanguage(r.removeFromRight(380));
        // no completo, a Biblioteca já está à esquerda: as abas só alternam
        // a coluna do meio entre a escuta e a resposta e a Sua escuta
        auto tabs = r.removeFromTop(Tokens::minTarget + 8).reduced(20, 4);
        pageButtons_[0].setVisible(true);
        pageButtons_[2].setVisible(true);
        pageButtons_[0].setBounds(tabs.removeFromLeft(190));
        tabs.removeFromLeft(4);
        pageButtons_[2].setBounds(tabs.removeFromLeft(150));
        for (juce::Component* c : std::initializer_list<juce::Component*> { &libraryTitle_, &library_, &libraryCard_, &openButton_, &bringButton_, &languagePanel_ })
            c->setVisible(true);
        const bool yours = page_ == 2;
        tracksViewport_.setVisible(!yours);
        listeningPage_->setVisible(yours);
        if (yours)
            listeningPage_->setBounds(r);
        else
            layoutTracks(r);
        return;
    }
    // compacto e emergência: a página de escuta ou a da Biblioteca; a
    // Linguagem como gaveta por cima
    for (juce::Component* c : std::initializer_list<juce::Component*> { &libraryTitle_, &library_, &libraryCard_, &openButton_, &bringButton_ })
        c->setVisible(libraryPage);
    const bool yoursPage = page_ == 2;
    tracksViewport_.setVisible(!libraryPage && !yoursPage);
    listeningPage_->setVisible(yoursPage);
    if (yoursPage)
        listeningPage_->setBounds(r);
    if (libraryPage)
    {
        auto lib = r.reduced(16, 10);
        libraryTitle_.setBounds(lib.removeFromTop(28));
        auto card = lib.removeFromRight(std::min(380, lib.getWidth() / 2));
        lib.removeFromRight(12);
        bringButton_.setBounds(card.removeFromTop(Tokens::minTarget));
        card.removeFromTop(8);
        openButton_.setBounds(card.removeFromBottom(Tokens::minTarget));
        card.removeFromBottom(6);
        authButton_.setBounds(card.removeFromBottom(Tokens::minTarget));
        libraryCard_.setBounds(card);
        library_.setBounds(lib);
    }
    else if (!yoursPage)
        layoutTracks(r);
    languagePanel_.setVisible(drawerOpen_);
    if (drawerOpen_)
    {
        layoutLanguage(r.removeFromRight(std::min(360, r.getWidth())));
        languagePanel_.toFront(false);
    }
}

} // namespace trioio::app
