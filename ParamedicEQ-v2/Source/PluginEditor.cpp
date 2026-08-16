#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
const juce::Colour BG(0xff080807);
const juce::Colour PANEL(0xff11100e);
const juce::Colour PANEL2(0xff171512);
const juce::Colour GRID(0xff37332b);
const juce::Colour GOLD(0xffd6a62f);
const juce::Colour GOLD2(0xffffcf5a);
const juce::Colour GOLD_DARK(0xff70501a);
const juce::Colour TEXT(0xffeee9dc);
const juce::Colour MUTED(0xffa9a18f);
const juce::Colour BLACK(0xff060606);

static float logX(float f, float left, float width)
{
    const float a = std::log10(20.0f), b = std::log10(20000.0f);
    f = juce::jlimit(20.0f, 20000.0f, f);
    return left + width * ((std::log10(f) - a) / (b - a));
}

static juce::File getPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("Paramedic EQ")
                    .getChildFile("Library")
                    .getChildFile("Presets");

    dir.createDirectory();
    return dir;
}

static juce::String fmtFreq(float f)
{
    if (f >= 1000.0f)
    {
        if (f >= 10000.0f) return juce::String(f / 1000.0f, 0) + "k";
        return juce::String(f / 1000.0f, f < 2000.0f ? 2 : 1).trimCharactersAtEnd("0") + "k";
    }
    if (f < 100.0f) return juce::String(f, 1).trimCharactersAtEnd("0");
    return juce::String(f, 0);
}

static void metalPanel(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff201e1a), r.getX(), r.getY(),
                                           juce::Colour(0xff0b0b0a), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(juce::Colour(0xff514a3d));
    g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
    g.setColour(juce::Colour(0xff080807));
    g.drawRoundedRectangle(r.reduced(4.0f), 3.0f, 1.0f);
}

class GoldLook : public juce::LookAndFeel_V4
{
public:
    GoldLook()
    {
        setColour(juce::Slider::rotarySliderFillColourId, GOLD);
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff302a20));
        setColour(juce::Slider::thumbColourId, GOLD2);
        setColour(juce::Slider::textBoxTextColourId, TEXT);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff080807));
        setColour(juce::Slider::textBoxOutlineColourId, GOLD_DARK);
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff12110f));
        setColour(juce::ComboBox::outlineColourId, GOLD_DARK);
        setColour(juce::ComboBox::textColourId, GOLD2);
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161411));
        setColour(juce::TextButton::textColourOffId, TEXT);
        setColour(juce::ToggleButton::textColourId, TEXT);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        const float radius = juce::jmin(w, h) * 0.32f;
        const auto c = juce::Point<float>(x + w * 0.5f, y + h * 0.43f);
        g.setColour(juce::Colour(0xff090909));
        g.fillEllipse(c.x - radius - 5, c.y - radius - 5, (radius + 5) * 2, (radius + 5) * 2);
        g.setColour(juce::Colour(0xff27231d));
        g.drawEllipse(c.x - radius, c.y - radius, radius * 2, radius * 2, 4.0f);

        juce::Path arc;
        arc.addCentredArc(c.x, c.y, radius + 1, radius + 1, 0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colour(0xff4c3a1d));
        g.strokePath(arc, juce::PathStrokeType(2.0f));

        const float a = startAngle + sliderPos * (endAngle - startAngle);
        juce::Path valueArc;
        valueArc.addCentredArc(c.x, c.y, radius + 1, radius + 1, 0.0f, startAngle, a, true);
        g.setColour(GOLD);
        g.strokePath(valueArc, juce::PathStrokeType(3.0f));

        const auto p = juce::Point<float>(c.x + std::cos(a) * (radius - 5),
                                          c.y + std::sin(a) * (radius - 5));
        g.setColour(GOLD2);
        g.fillEllipse(p.x - 2.2f, p.y - 2.2f, 4.4f, 4.4f);
    }
};

static GoldLook goldLook;

static void title(juce::Graphics& g, const juce::String& s, float x, float y, float w)
{
    g.setColour(GOLD);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(s, juce::roundToInt(x), juce::roundToInt(y), juce::roundToInt(w), 18, juce::Justification::centred);
}

static void valueText(juce::Graphics& g, const juce::String& s, float x, float y, float w)
{
    g.setColour(TEXT);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(s, juce::roundToInt(x), juce::roundToInt(y), juce::roundToInt(w), 18, juce::Justification::centred);
}
}

ParamedicEQAudioProcessorEditor::ParamedicEQAudioProcessorEditor(ParamedicEQAudioProcessor& x)
    : AudioProcessorEditor(&x), p(x)
{
    setLookAndFeel(&goldLook);
    setSize(1536, 1020);
    setResizable(false, false);

    for (int i = 0; i < p.getNumPrograms(); ++i)
        preset.addItem(p.getProgramName(i), i + 1);
    preset.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
    preset.onChange = [this] { loadPreset(preset.getSelectedId() - 1); };

    for (auto* b : { &prev, &next, &save, &loadButton, &addButton, &deleteButton, &menuButton })
        addAndMakeVisible(*b);
    addAndMakeVisible(preset);
    addAndMakeVisible(power);

    prev.onClick = [this] { loadPreset(juce::jmax(0, p.getCurrentProgram() - 1)); };
    next.onClick = [this] { loadPreset(juce::jmin(p.getNumPrograms() - 1, p.getCurrentProgram() + 1)); };

    // Preset file operations. The buttons were previously only visual controls;
    // they had no callbacks, so Save/Open appeared to do nothing.
    save.onClick = [this] { savePresetFile(); };
    loadButton.onClick = [this] { openPresetFile(); };
    menuButton.onClick = [this] { showPresetMenu(); };
    deleteButton.onClick = [this] { resetCurrentPreset(); };
    addButton.onClick = [this] { savePresetFile(); };

    for (int i = 0; i < 29; ++i)
    {
        configureKnob(freq[(size_t)i], 20.0, 20000.0);
        configureKnob(gain[(size_t)i], -18.0, 18.0);
        configureKnob(q[(size_t)i], 0.1, 10.0);
        freq[(size_t)i].setSkewFactorFromMidPoint(1000.0);
        q[(size_t)i].setSkewFactorFromMidPoint(1.0);

        addAndMakeVisible(freq[(size_t)i]);
        addAndMakeVisible(gain[(size_t)i]);
        addAndMakeVisible(q[(size_t)i]);
        addAndMakeVisible(bypass[(size_t)i]);

        bypass[(size_t)i].setButtonText("");
        bypass[(size_t)i].setClickingTogglesState(true);
        bypass[(size_t)i].setColour(juce::ToggleButton::tickColourId, GOLD);
        bypass[(size_t)i].setColour(juce::ToggleButton::tickDisabledColourId, GOLD_DARK);

        attachments[(size_t)i * 3 + 0] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.state(), "F" + juce::String(i + 1), freq[(size_t)i]);
        attachments[(size_t)i * 3 + 1] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.state(), "G" + juce::String(i + 1), gain[(size_t)i]);
        attachments[(size_t)i * 3 + 2] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.state(), "Q" + juce::String(i + 1), q[(size_t)i]);
        bypassAttachments[(size_t)i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            p.state(), "B" + juce::String(i + 1), bypass[(size_t)i]);

        for (auto* s : { &freq[(size_t)i], &gain[(size_t)i], &q[(size_t)i] })
            s->onDragStart = [this, i] { selectBand(i); };
    }

    auto attachGlobal = [this](juce::Slider& s, const char* id, double lo, double hi, int idx)
    {
        configureKnob(s, lo, hi);
        addAndMakeVisible(s);
        globalAttachments[(size_t)idx] =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state(), id, s);
    };

    attachGlobal(input, "INPUT", -24, 24, 0);
    attachGlobal(output, "OUTPUT", -24, 12, 1);
    attachGlobal(rmix, "RMIX", 0, 100, 2);
    attachGlobal(rpre, "RPRE", 0, 200, 3);
    attachGlobal(rdec, "RDEC", .1, 4, 4);
    attachGlobal(rsize, "RSIZE", 0, 100, 5);
    attachGlobal(rdamp, "RDAMP", 0, 100, 6);
    attachGlobal(rlow, "RLOW", 20, 1000, 7);
    attachGlobal(rhigh, "RHIGH", 2000, 20000, 8);
    attachGlobal(rwidth, "RWIDTH", 0, 100, 9);

    for (auto* s : { &input, &output })
        s->setTextValueSuffix(" dB");

    addAndMakeVisible(reverbPreset);
    reverbPreset.addItem("VOCAL ROOM", 1);
    reverbPreset.addItem("VOCAL PLATE", 2);
    reverbPreset.addItem("VOCAL HALL", 3);
    reverbPreset.setSelectedId(1);

    // These controls have different concrete JUCE button types
    // (TextButton and ToggleButton).  Store them through the common
    // Component base so the initializer list has one consistent type.
    for (juce::Component* b : {
             static_cast<juce::Component*>(&specButton),
             static_cast<juce::Component*>(&curveButton),
             static_cast<juce::Component*>(&bothButton),
             static_cast<juce::Component*>(&analyzerGear),
             static_cast<juce::Component*>(&analyzerHold),
             static_cast<juce::Component*>(&analyzerOff),
             static_cast<juce::Component*>(&compressor),
             static_cast<juce::Component*>(&limiter),
             static_cast<juce::Component*>(&masterMix),
             static_cast<juce::Component*>(&masterWidth) })
        addAndMakeVisible(*b);

    compressor.setClickingTogglesState(true);
    limiter.setClickingTogglesState(true);
    analyzerHold.setClickingTogglesState(true);
    masterMix.setButtonText("ON");
    masterWidth.setButtonText("100%");
    compressor.setButtonText("ON");
    limiter.setButtonText("ON");

    power.setButtonText("");
    power.setClickingTogglesState(true);
    power.setToggleState(true, juce::dontSendNotification);

    startTimerHz(30);
}

ParamedicEQAudioProcessorEditor::~ParamedicEQAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void ParamedicEQAudioProcessorEditor::configureKnob(juce::Slider& s, double lo, double hi)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setRange(lo, hi, 0.01);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 17);
    s.setColour(juce::Slider::textBoxTextColourId, TEXT);
    s.setColour(juce::Slider::textBoxBackgroundColourId, BLACK);
    s.setColour(juce::Slider::textBoxOutlineColourId, GOLD_DARK);
}

void ParamedicEQAudioProcessorEditor::loadPreset(int i)
{
    p.setCurrentProgram(i);
    preset.setSelectedId(i + 1, juce::dontSendNotification);
    repaint();
}

void ParamedicEQAudioProcessorEditor::savePresetFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Paramedic EQ Preset",
        getPresetDirectory()
            .getChildFile(p.presetName().replaceCharacter(' ', '_') + ".peq"),
        "*.peq");

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                           | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (file.getFullPathName().isEmpty())
            {
                fileChooser.reset();
                return;
            }

            juce::MemoryBlock stateData;
            p.getStateInformation(stateData);
            if (stateData.getSize() > 0)
            {
                file.replaceWithData(stateData.getData(), stateData.getSize());
            }

            fileChooser.reset();
        });
}

void ParamedicEQAudioProcessorEditor::openPresetFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Open Paramedic EQ Preset",
        getPresetDirectory(),
        "*.peq");

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                           | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                juce::MemoryBlock stateData;
                if (file.loadFileAsData(stateData))
                {
                    p.setStateInformation(stateData.getData(),
                                          static_cast<int>(stateData.getSize()));
                    syncPresetSelector();
                    repaint();
                }
            }

            fileChooser.reset();
        });
}

void ParamedicEQAudioProcessorEditor::syncPresetSelector()
{
    const auto name = p.presetName();
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        if (p.getProgramName(i).equalsIgnoreCase(name))
        {
            preset.setSelectedId(i + 1, juce::dontSendNotification);
            return;
        }
    }

    preset.setSelectedId(0, juce::dontSendNotification);
    preset.setText(name, juce::dontSendNotification);
}

void ParamedicEQAudioProcessorEditor::resetCurrentPreset()
{
    p.loadPreset(p.getCurrentProgram());
    preset.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
    repaint();
}

void ParamedicEQAudioProcessorEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Open Preset...");
    menu.addItem(2, "Save Preset As...");
    menu.addSeparator();
    menu.addItem(3, "Reset Current Preset");

    menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result)
    {
        switch (result)
        {
            case 1: openPresetFile(); break;
            case 2: savePresetFile(); break;
            case 3: resetCurrentPreset(); break;
            default: break;
        }
    });
}

int ParamedicEQAudioProcessorEditor::bandAtGraphPosition(juce::Point<float> pos) const
{
    // The graph's draggable EQ-node area matches the visual curve exactly.
    constexpr float left = 178.0f;
    constexpr float top = 158.0f;
    constexpr float right = 1358.0f;
    constexpr float bottom = 376.0f;

    if (pos.x < left - 14.0f || pos.x > right + 14.0f ||
        pos.y < top - 14.0f || pos.y > bottom + 14.0f)
        return -1;

    int closest = -1;
    float bestDistance = 18.0f;

    for (int i = 0; i < 29; ++i)
    {
        const float x = logX(freq[(size_t)i].getValue(), left, right - left);
        const float db = juce::jlimit(-24.0f, 18.0f,
                                      static_cast<float>(gain[(size_t)i].getValue()));
        const float y = juce::jmap(db, -24.0f, 18.0f, bottom, top);
        const float d = pos.getDistanceFrom({ x, y });

        if (d < bestDistance)
        {
            bestDistance = d;
            closest = i;
        }
    }

    return closest;
}

void ParamedicEQAudioProcessorEditor::moveGraphBand(int band, juce::Point<float> pos)
{
    if (band < 0 || band >= 29)
        return;

    constexpr float left = 178.0f;
    constexpr float right = 1358.0f;
    constexpr float top = 158.0f;
    constexpr float bottom = 376.0f;

    const float x = juce::jlimit(left, right, pos.x);
    const float y = juce::jlimit(top, bottom, pos.y);

    // X is logarithmic (20 Hz -> 20 kHz), matching the EQ graph.
    const float t = juce::jlimit(0.0f, 1.0f, (x - left) / (right - left));
    const float frequency = std::pow(10.0f,
                                     std::log10(20.0f) +
                                     t * (std::log10(20000.0f) - std::log10(20.0f)));

    // Y maps directly to the visible graph range (-24 dB -> +18 dB).
    const float db = juce::jlimit(-18.0f, 18.0f,
                                      juce::jmap(y, bottom, top, -24.0f, 18.0f));

    freq[(size_t)band].setValue(frequency, juce::sendNotificationSync);
    gain[(size_t)band].setValue(db, juce::sendNotificationSync);
    selectedBand = band;
    repaint();
}

void ParamedicEQAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isPopupMenu() && e.position.x >= 160.0f && e.position.x <= 1380.0f &&
        e.position.y >= 145.0f && e.position.y <= 390.0f)
    {
        draggedBand = bandAtGraphPosition(e.position);

        if (draggedBand >= 0)
        {
            draggingGraphBand = true;
            selectedBand = draggedBand;
            if (auto* parameter = p.state().getParameter(
                    "F" + juce::String(draggedBand + 1)))
                parameter->beginChangeGesture();

            if (auto* parameter = p.state().getParameter(
                    "G" + juce::String(draggedBand + 1)))
                parameter->beginChangeGesture();
            repaint();
        }
    }

    AudioProcessorEditor::mouseDown(e);
}

void ParamedicEQAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingGraphBand && draggedBand >= 0)
    {
        moveGraphBand(draggedBand, e.position);
        return;
    }

    AudioProcessorEditor::mouseDrag(e);
}

void ParamedicEQAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    if (draggingGraphBand && draggedBand >= 0)
    {
        if (auto* parameter = p.state().getParameter(
                "F" + juce::String(draggedBand + 1)))
            parameter->endChangeGesture();

        if (auto* parameter = p.state().getParameter(
                "G" + juce::String(draggedBand + 1)))
            parameter->endChangeGesture();
        draggedBand = -1;
        draggingGraphBand = false;
        repaint();
        return;
    }

    AudioProcessorEditor::mouseUp(e);
}

void ParamedicEQAudioProcessorEditor::selectBand(int i)
{
    selectedBand = juce::jlimit(0, 28, i);
    repaint();
}

juce::String ParamedicEQAudioProcessorEditor::formatFreq(float f) const { return fmtFreq(f); }

void ParamedicEQAudioProcessorEditor::timerCallback()
{
    analyzerPhase += 0.045f;
    repaint();
}

void ParamedicEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(BG);

    // Header
    metalPanel(g, {7, 7, 1522, 91});
    g.setColour(GOLD2);
    g.setFont(juce::FontOptions(35.0f, juce::Font::bold));
    g.drawText("PARAMEDIC EQ", 30, 14, 390, 40, juce::Justification::left);
    g.setColour(GOLD);
    g.setFont(juce::FontOptions(16.0f));
    g.drawText("Dvinesoul Edition", 122, 51, 240, 22, juce::Justification::centred);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("29 BAND PARAMETRIC EQUALISER", 60, 76, 330, 18, juce::Justification::left);

    title(g, "PRESET", 460, 13, 390);
    g.setColour(GOLD2);
    g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
    g.drawText(p.presetName().toUpperCase(), 515, 42, 290, 28, juce::Justification::centred);

    // right logo: source-derived X-Core mark, recoloured gold for this UI
    auto logo = juce::ImageCache::getFromMemory(BinaryData::xcore_logo_gold_png, BinaryData::xcore_logo_gold_pngSize);
    if (logo.isValid())
        g.drawImageWithin(logo, 1110, 14, 155, 74, juce::RectanglePlacement::centred, false);
    g.setColour(GOLD2);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("DVINESOUL", 1265, 31, 190, 30, juce::Justification::left);
    g.setColour(TEXT);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("POWER", 1395, 18, 65, 20, juce::Justification::centred);
    g.setColour(MUTED);
    g.drawText("v2.0.5", 1460, 79, 55, 14, juce::Justification::right);

    // Main EQ area
    metalPanel(g, {7, 104, 1522, 344});
    metalPanel(g, {18, 112, 112, 324});
    metalPanel(g, {1406, 112, 112, 324});

    title(g, "INPUT", 22, 121, 102);
    title(g, "OUTPUT", 1412, 121, 100);
    valueText(g, juce::String(input.getValue(), 2) + " dB", 22, 140, 102);
    valueText(g, juce::String(output.getValue(), 2) + " dB", 1412, 140, 100);

    // meter wells
    for (auto x : { 40, 1430 })
    {
        g.setColour(BLACK);
        g.fillRoundedRectangle((float)x, 167, 67, 190, 3);
        for (int i=0;i<9;++i)
        {
            const float yy = 177 + i*19.0f;
            g.setColour(i < 6 ? juce::Colour(0xff786321) : juce::Colour(0xff43371e));
            g.fillRect((float)x + 7.0f, yy, 53.0f, 11.0f);
        }
        g.setColour(juce::Colour(0xff1f1b12));
        g.drawRoundedRectangle((float)x, 167, 67, 190, 3, 1);
    }

    // Graph
    const juce::Rectangle<float> gr(145, 116, 1240, 307);
    g.setColour(juce::Colour(0xff090b0b));
    g.fillRoundedRectangle(gr, 4);
    g.setColour(juce::Colour(0xff292d2c));
    g.drawRoundedRectangle(gr, 4, 1);
    title(g, "EQ CURVE", 205, 121, 110);

    for (int db=-24; db<=18; db+=6)
    {
        const float y = juce::jmap((float)db, -24.f, 18.f, 376.f, 158.f);
        g.setColour(db == 0 ? juce::Colour(0xff5c5647) : juce::Colour(0xff202522));
        g.drawHorizontalLine((int)y, 175, 1360);
        g.setColour(MUTED);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(juce::String(db), 155, y-7, 32, 14, juce::Justification::right);
    }

    const std::array<int,18> labels {20,30,40,60,80,100,200,300,400,600,800,1000,2000,3000,4000,6000,8000,10000};
    for (auto f : labels)
    {
        const float x = logX((float)f, 178, 1180);
        g.setColour(juce::Colour(0xff1b201e));
        g.drawVerticalLine((int)x, 158, 377);
        g.setColour(MUTED);
        g.setFont(juce::FontOptions(9.0f));
        g.drawText(fmtFreq((float)f), x-25, 141, 50, 14, juce::Justification::centred);
    }
    g.setColour(MUTED);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("1k", 980, 141, 35, 14, juce::Justification::centred);
    g.drawText("15k", 1250, 141, 35, 14, juce::Justification::centred);
    g.drawText("20k", 1330, 141, 35, 14, juce::Justification::centred);

    // analyzer
    juce::Path spec;
    const float base = 365;
    for (int x=0; x<=1180; ++x)
    {
        const float t=(float)x/1180.f;
        float amp = 35.f + 18.f*std::sin(t*18.f) + 11.f*std::sin(t*57.f+analyzerPhase)
                    + 8.f*std::sin(t*143.f) + 4.f*std::sin(t*311.f);
        amp *= 0.65f + 0.35f*std::sin(juce::MathConstants<float>::pi*t);
        const float y=base-juce::jmax(2.f,amp);
        if(x==0) spec.startNewSubPath(178,base); else spec.lineTo(178+x,y);
    }
    spec.lineTo(1358,base); spec.closeSubPath();
    g.setColour(juce::Colour(0xff2b2924)); g.fillPath(spec);
    g.setColour(juce::Colour(0xff77736a)); g.strokePath(spec, juce::PathStrokeType(0.8f));

    // EQ curve
    juce::Path curve;
    for (int x=0; x<=1180; ++x)
    {
        const float t=(float)x/1180.f;
        const float f=std::pow(10.f, std::log10(20.f)+t*(std::log10(20000.f)-std::log10(20.f)));
        float db=0;
        for(int i=0;i<29;++i)
        {
            const float bf=freq[(size_t)i].getValue();
            const float bgain=gain[(size_t)i].getValue();
            const float o=std::log2(juce::jmax(20.f,f)/juce::jmax(20.f,bf));
            db += bgain*std::exp(-0.85f*o*o);
        }
        db=juce::jlimit(-24.f,18.f,db);
        const float y=juce::jmap(db,-24.f,18.f,376.f,158.f);
        if(x==0) curve.startNewSubPath(178,y); else curve.lineTo(178+x,y);
    }
    g.setColour(juce::Colour(0xffdedbd4)); g.strokePath(curve, juce::PathStrokeType(1.6f));

    const std::array<juce::Colour,10> bandColours = {
        juce::Colour(0xffb83a22), juce::Colour(0xffcb5c24), juce::Colour(0xffd58a20),
        juce::Colour(0xffd0a41d), juce::Colour(0xffa9b61e), juce::Colour(0xff6da82d),
        juce::Colour(0xff2d9e70), juce::Colour(0xff20a5aa), juce::Colour(0xff7869c4),
        juce::Colour(0xffc04aa8)
    };
    for(int i=0;i<29;++i)
    {
        const float f=freq[(size_t)i].getValue();
        const float x=logX(f,178,1180);
        const float db = juce::jlimit(-18.0f, 18.0f,
                                      static_cast<float>(gain[(size_t)i].getValue()));
        const float y=juce::jmap(db,-24.f,18.f,376.f,158.f);
        g.setColour(i==selectedBand ? GOLD2 : bandColours[(size_t)i*bandColours.size()/29]);
        g.fillEllipse(x-9,y-9,18,18);
        g.setColour(BLACK);
        g.setFont(juce::FontOptions(9.0f,juce::Font::bold));
        g.drawText(juce::String(i+1),x-10,y-6,20,12,juce::Justification::centred);
    }

    // matrix
    const float mx=12, my=454, mw=1512, mh=179;
    metalPanel(g,{mx,my,mw,mh});
    const float rowX=65, cellW=(1440.f)/29.f;
    const std::array<juce::String,29> freqs = {
        "20","25","31","40","50","63","100","125","125","160","200","250","315","400","500",
        "630","800","1k","1.25k","1.6k","2k","2.5k","3.15k","4k","5k","6.3k","8k","12k","20k"
    };
    const std::array<juce::String,29> types = {
        "∿","∿","∿","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁",
        "⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁","⌁"
    };

    const float labelW=50;
    g.setColour(GOLD);
    g.setFont(juce::FontOptions(11.0f,juce::Font::bold));
    g.drawText("BAND",17,468,42,18,juce::Justification::centred);
    g.drawText("FREQ (Hz)",10,497,55,18,juce::Justification::centred);
    g.drawText("GAIN (dB)",10,524,55,18,juce::Justification::centred);
    g.drawText("Q",20,552,30,18,juce::Justification::centred);
    g.drawText("TYPE",12,579,42,18,juce::Justification::centred);
    g.drawText("BYPASS",9,607,48,18,juce::Justification::centred);

    for(int i=0;i<29;++i)
    {
        const float x=rowX+i*cellW;
        if(i==selectedBand)
        {
            g.setColour(juce::Colour(0xff171c1b));
            g.fillRect(x,my+3,cellW,mh-6);
            g.setColour(GOLD2);
            g.drawRect(x, my + 3.0f, cellW, mh - 6.0f, 1.0f);
        }
        g.setColour(GRID);
        g.drawVerticalLine((int)x,(int)my+4,(int)my+mh-4);
        g.setColour(TEXT);
        g.setFont(juce::FontOptions(11.0f,juce::Font::bold));
        g.drawText(juce::String(i+1),x,my+9,cellW,17,juce::Justification::centred);
        valueText(g,freqs[(size_t)i],x,my+38,cellW);
        valueText(g,juce::String(gain[(size_t)i].getValue(),1),x,my+66,cellW);
        valueText(g,juce::String(q[(size_t)i].getValue(),2),x,my+94,cellW);
        g.setColour(GOLD);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(types[(size_t)i],x,my+120,cellW,17,juce::Justification::centred);
        g.setColour(juce::Colour(0xff25231f));
        g.fillEllipse(x+cellW/2-10,my+146,20,20);
        g.setColour(juce::Colour(0xff5e553e));
        g.drawEllipse(x+cellW/2-10,my+146,20,20,1);
    }

    // Lower six panels
    const float y=646, gap=7;
    const std::array<float,6> ws={184,305,267,329,195,208};
    float x=7;
    for(float w:ws){ metalPanel(g,{x,y,w,309}); x+=w+gap; }

    title(g,"GLOBAL",15,657,168);
    title(g,"ANALYZER",199,657,289);
    title(g,"BAND SELECT",511,657,251);
    title(g,"REVERB ENGINE",785,657,315);
    title(g,"PRESETS",1109,657,181);
    title(g,"MASTER SECTION",1301,657,222);

    // Global
    valueText(g,"INPUT GAIN",19,688,76); valueText(g,"OUTPUT GAIN",105,688,76);
    valueText(g,"MIX",61,799,74);
    // Analyzer
    g.setColour(BLACK); g.fillRect(226,699,236,154); g.setColour(juce::Colour(0xff4b3c16)); g.drawRect(226,699,236,154,1);
    juce::Path a2; a2.startNewSubPath(228,836);
    for(int xx=0;xx<232;++xx){ float t=xx/232.f; float yy=830-juce::jmax(2.f, 65.f*std::abs(std::sin(t*25))+20.f*std::abs(std::sin(t*77+analyzerPhase))); a2.lineTo(228+xx,yy);}
    g.setColour(juce::Colour(0xffc49419)); g.strokePath(a2,juce::PathStrokeType(1.1f));
    title(g,"PRE",245,862,70); title(g,"POST",325,862,70); title(g,"HOLD",405,862,55);

    // Band select
    g.setColour(juce::Colour(0xff21a7b3)); g.setFont(juce::FontOptions(38.0f,juce::Font::bold));
    g.drawText(juce::String(selectedBand+1),550,697,190,48,juce::Justification::centred);
    valueText(g,fmtFreq(freq[(size_t)selectedBand].getValue())+" Hz",550,744,190);
    valueText(g,"GAIN",535,782,80); valueText(g,"Q",625,782,80); valueText(g,"TYPE",696,782,70);
    valueText(g,juce::String(gain[(size_t)selectedBand].getValue(),2)+" dB",535,856,80);
    valueText(g,juce::String(q[(size_t)selectedBand].getValue(),2),625,856,80);
    valueText(g,"PEAK",696,856,70);

    // Reverb
    valueText(g,"VOCAL ROOM",850,693,205);
    valueText(g,"MIX",810,719,60); valueText(g,"PRE-DELAY",880,719,70); valueText(g,"DECAY",965,719,65); valueText(g,"SIZE",1040,719,55);
    valueText(g,"DAMPING",805,811,70); valueText(g,"LOW CUT",900,811,70); valueText(g,"HIGH CUT",980,811,75); valueText(g,"WIDTH",1060,811,55);

    // Preset list
    const std::array<juce::String,12> presetNames={"Vocal Clean","Vocal Presence","Vocal Air","Vocal Warm","Vocal Bright","Vocal Radio","Vocal Power","Vocal Rap","Vocal Trap","Vocal RnB","Vocal Smooth","Vocal Intimate"};
    for(size_t i=0;i<presetNames.size();++i)
    {
        g.setColour(presetNames[i]==p.presetName()?GOLD2:TEXT);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String("☆  ") + presetNames[i],1120,695+(int)i*21,160,18,juce::Justification::left);
    }

    // Master
    valueText(g,"SATURATION",1320,692,88); valueText(g,"STEREO WIDTH",1425,692,90);
    valueText(g,"COMPRESSOR",1318,800,92); valueText(g,"LIMITER",1430,800,70);
    valueText(g,"-18.0 dB",1325,871,80); valueText(g,"-1.0 dB",1430,871,70);

    // Footer
    g.setColour(GOLD);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("PARAMEDIC EQ v2.0.5",28,979,220,22,juce::Justification::left);
    g.drawText("BUILT BY DVINESOUL  •  ENGINEERED FOR PROFESSIONALS",510,979,520,22,juce::Justification::centred);
    g.setFont(juce::FontOptions(24.0f,juce::Font::italic));
    g.drawText("Dvinesoul",1370,971,130,30,juce::Justification::right);
}

void ParamedicEQAudioProcessorEditor::resized()
{
    prev.setBounds(470, 32, 40, 38);
    preset.setBounds(510, 32, 350, 38);
    next.setBounds(860, 32, 40, 38);
    save.setBounds(906, 32, 52, 38);
    addButton.setBounds(964, 32, 52, 38);
    deleteButton.setBounds(1022, 32, 52, 38);
    menuButton.setBounds(1080, 32, 40, 38);
    loadButton.setBounds(1140, 32, 1, 1);
    power.setBounds(1460, 24, 54, 54);

    input.setBounds(42, 357, 64, 66);
    output.setBounds(1430, 357, 64, 66);

    const float rowX=65, cellW=1440.f/29.f;
    for(int i=0;i<29;++i)
    {
        const int x=juce::roundToInt(rowX+i*cellW+2);
        const int w=juce::jmax(34,juce::roundToInt(cellW-4));
        freq[(size_t)i].setBounds(x, 480, w, 34);
        gain[(size_t)i].setBounds(x, 508, w, 34);
        q[(size_t)i].setBounds(x, 536, w, 34);
        freq[(size_t)i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        gain[(size_t)i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        q[(size_t)i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        bypass[(size_t)i].setBounds(x+8, 592, w-16, 30);
    }

    rmix.setBounds(42, 820, 72, 62);
    input.setTextValueSuffix(" dB");
    output.setTextValueSuffix(" dB");

    rpre.setBounds(818, 736, 65, 72);
    rdec.setBounds(895, 736, 65, 72);
    rsize.setBounds(972, 736, 65, 72);
    rwidth.setBounds(1045, 736, 65, 72);
    rdamp.setBounds(818, 836, 65, 72);
    rlow.setBounds(895, 836, 65, 72);
    rhigh.setBounds(972, 836, 65, 72);
    rwidth.setBounds(1045, 836, 65, 72);

    specButton.setBounds(1115, 113, 80, 25);
    curveButton.setBounds(1198, 113, 78, 25);
    bothButton.setBounds(1279, 113, 76, 25);
    analyzerGear.setBounds(1358, 113, 25, 25);

    analyzerHold.setBounds(245, 875, 60, 28);
    analyzerOff.setBounds(310, 875, 60, 28);

    masterMix.setBounds(1328, 726, 82, 70);
    masterWidth.setBounds(1430, 726, 82, 70);
    compressor.setBounds(1328, 825, 82, 35);
    limiter.setBounds(1430, 825, 82, 35);

    reverbPreset.setBounds(842, 688, 220, 26);
}

void ParamedicEQAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    // Gold rings around lower rotary controls and bypass wells.
    for (int i=0;i<29;++i)
    {
        const float x=65+i*(1440.f/29.f)+(1440.f/29.f)*0.5f;
        g.setColour(juce::Colour(0xff28241d));
        g.drawEllipse(x-11, 591, 22, 22, 1.0f);
    }
}
