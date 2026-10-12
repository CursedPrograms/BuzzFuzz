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

private:
    void timerCallback() override;

    Fuzz14Processor& pedal;
    InkLook look;
    std::unique_ptr<juce::Drawable> skin;
    juce::Slider input, battery, volume;
    juce::ComboBox d1, d3;
    juce::TextButton bypass { "BYPASS" }, mute { "MUTE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment, batteryAttachment, volumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> d1Attachment, d3Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment, muteAttachment;
    float diodeGlow[3] {};
    float inputGlow = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Fuzz14Editor)
};
