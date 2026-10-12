#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::StringArray diodeNames()
{
    juce::StringArray names;
    for (const auto& d : diodeTypes())
        names.add (d.name);
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    const auto db = [] (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB"; };
    return {
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "input", 1 }, "Input",
                                                     juce::NormalisableRange<float> (-24.0f, 36.0f, 0.1f), 0.0f,
                                                     juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "battery", 1 }, "Battery", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return juce::String (Fuzz14Processor::batteryVolts (v), 1) + " V"; })),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "volume", 1 }, "Volume",
                                                     juce::NormalisableRange<float> (-36.0f, 12.0f, 0.1f), 0.0f,
                                                     juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)),
        std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "d1", 1 }, "D1", diodeNames(), 0),
        std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "d3", 1 }, "D3", diodeNames(), 0),
        std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false),
        std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "mute", 1 }, "Mute", false),
    };
}

void atomicMax (std::atomic<float>& a, float v)
{
    for (auto old = a.load(); v > old && ! a.compare_exchange_weak (old, v);)
        ;
}
}

Fuzz14Processor::Fuzz14Processor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::mono(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      params (*this, nullptr, "Fuzz14", createLayout())
{
    inputParam = params.getRawParameterValue ("input");
    batteryParam = params.getRawParameterValue ("battery");
    volumeParam = params.getRawParameterValue ("volume");
    d1Param = params.getRawParameterValue ("d1");
    d3Param = params.getRawParameterValue ("d3");
    bypassParam = params.getRawParameterValue ("bypass");
    muteParam = params.getRawParameterValue ("mute");
}

juce::AudioProcessorParameter* Fuzz14Processor::getBypassParameter() const
{
    return params.getParameter ("bypass");
}

bool Fuzz14Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    const auto ok = [] (const juce::AudioChannelSet& s) { return s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo(); };
    return ok (in) && ok (out);
}

void Fuzz14Processor::prepareToPlay (double sampleRate, int maxBlockSize)
{
    const auto factor = sampleRate < 88200.0 ? 1 : 0;                   // 2x at 44.1 / 48 kHz
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        1, (size_t) factor, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing ((size_t) maxBlockSize);
    setLatencySamples (juce::roundToInt (oversampling->getLatencyInSamples()));

    d1Type = (int) d1Param->load();
    d3Type = (int) d3Param->load();
    fuzz.circuit.setDiode (fuzz.d1, diodeTypes()[(size_t) d1Type].model);
    fuzz.circuit.setDiode (fuzz.d3, diodeTypes()[(size_t) d3Type].model);

    battery.reset (sampleRate, 0.3);
    battery.setCurrentAndTargetValue (batteryParam->load());
    fuzz.circuit.setSource (fuzz.battery, batteryVolts (battery.getCurrentValue()));
    fuzz.circuit.setSourceOhms (fuzz.battery, batteryOhms (battery.getCurrentValue()));
    fuzz.circuit.prepare (sampleRate * (1 << factor));                // settles the bias

    inputGain.reset (sampleRate, 0.05);
    inputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (inputParam->load()));
    outputGain.reset (sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (volumeParam->load()));
    mono.assign ((size_t) juce::jmax (1, maxBlockSize), 0.0f);
    dry.assign (mono.size(), 0.0f);
    dryDelay.assign ((size_t) getLatencySamples() + 1, 0.0f);     // the clean signal, lined up with the pedal
    dryPos = 0;
    wetMix.reset (sampleRate, 0.02);
    wetMix.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
    level.reset (sampleRate, 0.02);
    level.setCurrentAndTargetValue (muteParam->load() > 0.5f ? 0.0f : 1.0f);
    dcPole = std::exp (-juce::MathConstants<double>::twoPi * 5.0 / sampleRate);
    dcIn = fuzz.circuit.voltage (fuzz.output);           // start settled: no thump
    dcOut = 0.0;
}

void Fuzz14Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto ins = getTotalNumInputChannels(), outs = getTotalNumOutputChannels();
    const auto n = buffer.getNumSamples();
    if (n == 0 || oversampling == nullptr)
        return;
    if ((size_t) n > mono.size())                                   // a host broke its promise
    {
        buffer.clear();
        return;
    }

    // guitar in, volts at the input jack
    inputGain.setTargetValue (juce::Decibels::decibelsToGain (inputParam->load()));
    float inputPeak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        auto x = ins > 0 ? buffer.getSample (0, i) : 0.0f;
        if (ins > 1)
            x = 0.5f * (x + buffer.getSample (1, i));
        dryDelay[dryPos] = x;
        dryPos = (dryPos + 1) % dryDelay.size();
        dry[(size_t) i] = dryDelay[dryPos];
        mono[(size_t) i] = x * inputGain.getNextValue();
        inputPeak = std::max (inputPeak, std::abs (mono[(size_t) i]));
    }
    atomicMax (inputVolts, inputPeak);

    // swap a diode, like pulling one out of the breadboard and pushing another in
    if (const auto t = (int) d1Param->load(); t != d1Type)
        fuzz.circuit.setDiode (fuzz.d1, diodeTypes()[(size_t) (d1Type = t)].model);
    if (const auto t = (int) d3Param->load(); t != d3Type)
        fuzz.circuit.setDiode (fuzz.d3, diodeTypes()[(size_t) (d3Type = t)].model);

    battery.setTargetValue (batteryParam->load());
    float* channels[] = { mono.data() };
    juce::dsp::AudioBlock<float> block (channels, 1, (size_t) n);
    auto up = oversampling->processSamplesUp (block);
    auto* s = up.getChannelPointer (0);
    const auto factor = (int) (up.getNumSamples() / (size_t) n);
    auto& c = fuzz.circuit;
    float current[3] = {};
    for (size_t i = 0; i < up.getNumSamples(); ++i)
    {
        if (i % (size_t) factor == 0 && battery.isSmoothing())       // battery sag, smoothly
        {
            const auto b = battery.getNextValue();
            c.setSource (fuzz.battery, batteryVolts (b));
            c.setSourceOhms (fuzz.battery, batteryOhms (b));
        }
        c.setSource (fuzz.input, (double) s[i]);
        c.step();
        auto out = c.voltage (fuzz.output);
        if (! std::isfinite (out))                                      // never pass garbage on
        {
            c.prepare (getSampleRate() * factor);
            out = 0.0;
        }
        s[i] = (float) out;
        current[0] = std::max (current[0], (float) c.diodeCurrent (fuzz.d1));
        current[1] = std::max (current[1], (float) c.diodeCurrent (fuzz.d2));
        current[2] = std::max (current[2], (float) c.diodeCurrent (fuzz.d3));
    }
    if (! battery.isSmoothing())
        battery.setCurrentAndTargetValue (battery.getTargetValue());
    oversampling->processSamplesDown (block);
    for (size_t d = 0; d < 3; ++d)
        atomicMax (diodeCurrent[d], current[d]);

    // volts at the output jack to the amp
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (volumeParam->load()));
    wetMix.setTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
    level.setTargetValue (muteParam->load() > 0.5f ? 0.0f : 1.0f);
    for (int i = 0; i < n; ++i)
    {
        // A leaky diode (germanium, Schottky) in D3 puts DC on the output; the amp's input cap
        // would block it, so do the same: a 5 Hz high-pass
        const auto x = (double) mono[(size_t) i];
        dcOut = x - dcIn + dcPole * dcOut;
        dcIn = x;
        const auto wet = juce::jlimit (-2.0f, 2.0f, (float) dcOut * outputGain.getNextValue());
        const auto mix = wetMix.getNextValue();
        const auto y = (mix * wet + (1.0f - mix) * dry[(size_t) i]) * level.getNextValue();
        for (int ch = 0; ch < outs; ++ch)
            buffer.setSample (ch, i, y);
    }
}

juce::AudioProcessorEditor* Fuzz14Processor::createEditor()
{
    return new Fuzz14Editor (*this);
}

void Fuzz14Processor::getStateInformation (juce::MemoryBlock& destData)
{
    if (const auto xml = params.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void Fuzz14Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (params.state.getType()))
            params.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Fuzz14Processor();
}
