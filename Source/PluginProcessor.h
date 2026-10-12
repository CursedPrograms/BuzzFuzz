#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "circuit/Fuzz14.h"
#include "circuit/Diodes.h"

// The whole pedal, solved part by part every sample (oversampled to at least 88.2 kHz).
//   INPUT    how hard the guitar hits it: 0 dB = full scale is 1 V at the input jack (a pickup
//            gives 0.1-1 V; a mic preamp often far less, so a voice wants +18 dB or more)
//   BATTERY  fresh 9 V to a dying one (lower voltage, more internal resistance)
//   VOLUME   what reaches the amp: 0 dB = 1 V at the output jack is full scale
//   D1-D3    which diode sits in each spot (pull D1 and D2 out and nothing biases the transistor)
//   BYPASS   true bypass (the clean signal, delayed to line up); MUTE: silence
class Fuzz14Processor : public juce::AudioProcessor
{
public:
    Fuzz14Processor();

    void prepareToPlay (double sampleRate, int maxBlockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                         { return true; }

    const juce::String getName() const override             { return "Fuzz14"; }
    bool acceptsMidi() const override                       { return false; }
    bool producesMidi() const override                      { return false; }
    double getTailLengthSeconds() const override            { return 0.0; }

    int getNumPrograms() override                           { return 1; }
    int getCurrentProgram() override                        { return 0; }
    void setCurrentProgram (int) override                   {}
    const juce::String getProgramName (int) override        { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    juce::AudioProcessorParameter* getBypassParameter() const override;

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static double batteryVolts (float battery)              { return 9.0 - 3.0 * battery; }    // 9 V .. 6 V
    static double batteryOhms (float battery)               { return 2.0 + 78.0 * battery * battery; }

    juce::AudioProcessorValueTreeState params;

    // how hard each diode conducts (amps, peak since the window last looked)
    std::array<std::atomic<float>, 3> diodeCurrent {};
    std::atomic<float> inputVolts { 0.0f };               // peak volts at the input jack

private:
    Fuzz14 fuzz;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::SmoothedValue<float> inputGain, outputGain, battery;
    std::vector<float> mono, dry, dryDelay;
    size_t dryPos = 0;
    juce::SmoothedValue<float> wetMix, level;            // bypass crossfade, mute fade
    double dcIn = 0.0, dcOut = 0.0, dcPole = 0.999;   // the amp's input cap: blocks DC, like a real amp
    std::atomic<float>* inputParam = nullptr;
    std::atomic<float>* batteryParam = nullptr;
    std::atomic<float>* volumeParam = nullptr;
    std::atomic<float>* d1Param = nullptr;
    std::atomic<float>* d2Param = nullptr;
    std::atomic<float>* d3Param = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* muteParam = nullptr;
    int d1Type = 0, d2Type = 0, d3Type = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Fuzz14Processor)
};
