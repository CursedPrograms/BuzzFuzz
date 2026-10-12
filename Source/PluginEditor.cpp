#include "PluginEditor.h"
#include "BinaryData.h"
#if JucePlugin_Build_Standalone
 #include <juce_audio_utils/juce_audio_utils.h>
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

using namespace juce;

namespace
{
const Colour ink { 0xff1d1d1f }, paper { 0xffffffff }, grey { 0xff666666 }, red { 0xffc0392b }, glow { 0xffff3b2f };
constexpr int skinWidth = 860, skinHeight = 520;

Font font (float height, bool bold = false)
{
    return Font (FontOptions ("Segoe UI", height, bold ? Font::bold : Font::plain));
}

// how bright a diode looks: 1 uA barely, 1 mA fully
float brightness (float amps)
{
    return amps <= 0.0f ? 0.0f : jlimit (0.0f, 1.0f, (std::log10 (amps) + 6.0f) / 3.0f);
}
}

//==============================================================================
InkLook::InkLook()
{
    setColour (Slider::textBoxTextColourId, grey);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (ComboBox::backgroundColourId, paper);
    setColour (ComboBox::outlineColourId, ink);
    setColour (ComboBox::textColourId, ink);
    setColour (ComboBox::arrowColourId, ink);
    setColour (PopupMenu::backgroundColourId, paper);
    setColour (PopupMenu::textColourId, ink);
    setColour (PopupMenu::headerTextColourId, red);
    setColour (PopupMenu::highlightedBackgroundColourId, red.withAlpha (0.15f));
    setColour (PopupMenu::highlightedTextColourId, ink);
}

void InkLook::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const auto on = b.getToggleState();
    g.setColour (on ? red : over ? Colour (0xfff3f3f0) : paper);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? red : ink);
    g.drawRoundedRectangle (r, 4.0f, down ? 2.5f : 1.8f);
}

void InkLook::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setColour (b.getToggleState() ? paper : ink);
    g.setFont (font (13.0f, true));
    g.drawText (b.getButtonText(), b.getLocalBounds(), Justification::centred);
}

Font InkLook::getComboBoxFont (ComboBox&)   { return font (13.0f); }
Font InkLook::getPopupMenuFont()            { return font (14.0f); }

void InkLook::drawRotarySlider (Graphics& g, int x, int y, int width, int height, float pos,
                                float startAngle, float endAngle, Slider&)
{
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
    const auto r = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f - 6.0f;
    const auto c = bounds.getCentre();

    // tick marks, like a pot's dial
    for (int i = 0; i <= 10; ++i)
    {
        const auto a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const auto inner = r + 2.0f, outer = r + (i % 5 == 0 ? 7.0f : 5.0f);
        g.setColour (i % 5 == 0 ? ink : grey);
        g.drawLine (c.x + inner * std::sin (a), c.y - inner * std::cos (a), c.x + outer * std::sin (a), c.y - outer * std::cos (a),
                    i % 5 == 0 ? 2.0f : 1.2f);
    }

    g.setColour (paper);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (ink);
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 2.5f);
    g.drawEllipse (c.x - r * 0.72f, c.y - r * 0.72f, r * 1.44f, r * 1.44f, 1.0f);

    const auto a = startAngle + pos * (endAngle - startAngle);
    g.setColour (red);
    g.drawLine (c.x + r * 0.15f * std::sin (a), c.y - r * 0.15f * std::cos (a),
                c.x + r * 0.95f * std::sin (a), c.y - r * 0.95f * std::cos (a), 3.0f);
    g.setColour (ink);
    g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
}

Label* InkLook::createSliderTextBox (Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (font (12.0f));
    return label;
}

//==============================================================================
Fuzz14Editor::Fuzz14Editor (Fuzz14Processor& p)
    : AudioProcessorEditor (p), pedal (p)
{
    setLookAndFeel (&look);
    skin = Drawable::createFromImageData (BinaryData::skin_svg, BinaryData::skin_svgSize);

    const auto setUp = [this] (Slider& s, const char* id, std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>& attachment)
    {
        s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
        s.onValueChange = [this] { repaint(); };
        s.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (s);
        attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (pedal.params, id, s);
    };
    setUp (input, "input", inputAttachment);
    setUp (battery, "battery", batteryAttachment);
    setUp (volume, "volume", volumeAttachment);
    // the diode pickers, grouped by kind
    for (auto* box : { &d1, &d3 })
    {
        String kind;
        int id = 1;
        for (const auto& d : diodeTypes())
        {
            if (kind != d.kind)
            {
                kind = d.kind;
                box->addSectionHeading (kind);
            }
            box->addItem (d.name, id++);
        }
        addAndMakeVisible (*box);
    }
    d1.setTooltip ("D1: which diode sits in the first collector-to-base spot");
    d3.setTooltip ("D3: which diode clips from the output back to the base");
    d1Attachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (pedal.params, "d1", d1);
    d3Attachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (pedal.params, "d3", d3);

    for (auto* b : { &bypass, &mute })
    {
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }
    bypass.setTooltip ("True bypass: the clean signal, straight through");
    mute.setTooltip ("Silence, for tuning or between songs");
    bypassAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (pedal.params, "bypass", bypass);
    muteAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (pedal.params, "mute", mute);

   #if JucePlugin_Build_Standalone
    // The standalone app starts with the input muted "to avoid a feedback loop" and a bar across
    // the top; this is an instrument effect, so open with the input live (MUTE is right here).
    if (p.wrapperType == AudioProcessor::wrapperType_Standalone)
        if (auto* holder = StandalonePluginHolder::getInstance())
            holder->getMuteInputValue().setValue (false);
   #endif

    input.setDoubleClickReturnValue (true, 0.0);
    battery.setDoubleClickReturnValue (true, 0.0);
    volume.setDoubleClickReturnValue (true, 0.0);

    setSize (skinWidth, skinHeight);
    startTimerHz (30);
}

Fuzz14Editor::~Fuzz14Editor()
{
    setLookAndFeel (nullptr);
}

void Fuzz14Editor::resized()
{
    battery.setBounds (712, 92, 96, 96);
    volume.setBounds (712, 316, 96, 96);
    input.setBounds (40, 342, 96, 96);
    d1.setBounds (150, 118, 132, 24);
    d3.setBounds (150, 188, 132, 24);
    bypass.setBounds (668, 440, 76, 28);
    mute.setBounds (752, 440, 76, 28);
}

void Fuzz14Editor::timerCallback()
{
    // light up fast, fade like a filament
    const auto fade = [] (float& shown, float target) { shown = target > shown ? target : shown * 0.82f; };
    for (size_t d = 0; d < 3; ++d)
        fade (diodeGlow[d], brightness (pedal.diodeCurrent[d].exchange (0.0f)));
    // the input jack glows from 10 mV (still clean) to 100 mV and up (full fuzz)
    const auto volts = pedal.inputVolts.exchange (0.0f);
    fade (inputGlow, volts <= 0.0f ? 0.0f : jlimit (0.0f, 1.0f, std::log10 (volts) + 2.0f));
    repaint (370, 105, 50, 115);
    repaint (84, 256, 52, 52);
}

void Fuzz14Editor::paint (Graphics& g)
{
    g.fillAll (paper);
    if (skin != nullptr)
        skin->drawWithin (g, Rectangle<float> (0.0f, 0.0f, (float) skinWidth, (float) skinHeight), RectanglePlacement::stretchToFit, 1.0f);

    // knob labels, with a dashed lead to the part they belong to
    const auto label = [&g] (Slider& knob, const String& name, const String& note)
    {
        const auto r = knob.getBounds().toFloat();
        g.setColour (ink);
        g.setFont (font (14.0f, true));
        g.drawText (name, r.withY (r.getY() - 22.0f).withHeight (18.0f), Justification::centred);
        g.setColour (red);
        g.setFont (font (12.0f, true));
        g.drawText (knob.getTextFromValue (knob.getValue()), r.withY (r.getBottom() - 2.0f).withHeight (15.0f).expanded (30.0f, 0.0f),
                    Justification::centred);
        g.setColour (grey);
        g.setFont (font (11.0f));
        g.drawText (note, r.withY (r.getBottom() + 12.0f).withHeight (14.0f).expanded (30.0f, 0.0f), Justification::centred);
    };
    label (battery, "BATTERY", "fresh  to  dying");
    label (volume, "VOLUME", "into the amp");
    label (input, "INPUT", "how hard you hit it");

    // D2 can't be changed: it's what keeps the transistor biased
    g.setColour (ink);
    g.setFont (font (13.0f));
    g.drawText ("1N4148", Rectangle<int> (150, 153, 132, 24), Justification::centredLeft);
    g.setColour (grey);
    g.setFont (font (11.0f, true));
    for (const auto& [y, name] : { std::pair<int, const char*> { 118, "D1" }, { 153, "D2" }, { 188, "D3" } })
        g.drawText (name, Rectangle<int> (118, y, 28, 24), Justification::centredRight);

    const float dash[] = { 4.0f, 4.0f };
    g.setColour (grey.withAlpha (0.6f));
    g.drawDashedLine ({ 712.0f, 140.0f, 620.0f, 62.0f }, dash, 2, 1.2f);       // to +9 V
    g.drawDashedLine ({ 760.0f, 316.0f, 742.0f, 250.0f }, dash, 2, 1.2f);      // to OUT
    g.drawDashedLine ({ 88.0f, 318.0f, 106.0f, 300.0f }, dash, 2, 1.2f);       // to IN

    g.setColour (Colour (0xff888888));
    g.setFont (font (11.0f));
    g.drawText (String ("Fuzz14 v" FUZZ14_VERSION)
                    + String (CharPointer_UTF8 ("  \xc2\xb7  Farica\xe2\x98\x86Kimora  (c) Cursed Entertainment")),
                Rectangle<int> (skinWidth - 420, skinHeight - 18, 400, 16), Justification::centredRight);
}

void Fuzz14Editor::paintOverChildren (Graphics& g)
{
    // the diodes glow while they conduct (same spots as in tools/draw_schematic.py)
    const auto diode = [&g] (float cx, float y, float level)
    {
        if (level <= 0.01f)
            return;
        g.setColour (glow.withAlpha (0.35f * level));
        g.fillEllipse (cx - 18.0f, y - 18.0f, 36.0f, 36.0f);
        Path p;
        p.addTriangle (cx + 9.0f, y - 10.0f, cx + 9.0f, y + 10.0f, cx - 8.0f, y);
        g.setColour (glow.withAlpha (level));
        g.fillPath (p);
        g.drawLine (cx - 9.0f, y - 10.0f, cx - 9.0f, y + 10.0f, 3.0f);
    };
    diode (395.0f, 130.0f, diodeGlow[0]);
    diode (395.0f, 165.0f, diodeGlow[1]);
    diode (395.0f, 200.0f, diodeGlow[2]);

    if (inputGlow > 0.01f)
    {
        g.setColour (glow.withAlpha (0.35f * inputGlow));
        g.fillEllipse (110.0f - 20.0f, 282.0f - 20.0f, 40.0f, 40.0f);
        g.setColour (glow.withAlpha (inputGlow));
        g.drawEllipse (110.0f - 9.0f, 282.0f - 9.0f, 18.0f, 18.0f, 3.0f);
    }
}
