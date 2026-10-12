#pragma once
#include "PluginProcessor.h"

// Knobs drawn in ink, like the rest of the schematic
class InkLook : public juce::LookAndFeel_V4
{
public:
    InkLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

// The window is the schematic: the knobs sit next to the parts they belong to, and the diodes
// glow red while they conduct.
class Fuzz14Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Fuzz14Editor (Fuzz14Processor&);
    ~Fuzz14Editor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;          // click a diode: pick another
    void mouseMove (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    int diodeType (int spot) const;             // which diode sits in D1 / D2 / D3
    int diodeAt (juce::Point<int>) const;       // the diode under the mouse, or -1
    void showDiodeMenu (int spot);

    Fuzz14Processor& pedal;
    InkLook look;
    std::unique_ptr<juce::Drawable> skin;
    juce::Slider input, battery, volume;
    juce::ComboBox d1, d2, d3;
    juce::TextButton bypass { "BYPASS" }, mute { "MUTE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment, batteryAttachment, volumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> d1Attachment, d2Attachment, d3Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment, muteAttachment;
    float diodeGlow[3] {};
    float inputGlow = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Fuzz14Editor)
};
