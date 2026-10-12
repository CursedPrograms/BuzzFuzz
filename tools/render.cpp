// Offline check without a DAW: plays a synthetic guitar through the pedal with every diode in
// D1 and D3, a dying battery, and the defaults; writes WAVs, timing, and the window as PNGs.
//   Fuzz14Render <out dir>
#include "../Source/PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <chrono>
#include <iostream>

namespace
{
constexpr double rate = 48000.0;
constexpr int blockSize = 256;
int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  ok   " : "  FAIL ") << what << '\n';
    failures += ok ? 0 : 1;
}

// a plucked low E, then an A power chord, about as loud as a pickup (0.3 V peaks = -10 dBFS)
juce::AudioBuffer<float> guitar (double seconds)
{
    const int n = (int) (seconds * rate);
    juce::AudioBuffer<float> out (1, n);
    out.clear();
    juce::Random rnd (7);
    const auto pluck = [&] (double start, double hz, float level)
    {
        for (int k = 1; k <= 30; ++k)
        {
            const auto f = hz * k;
            if (f > 0.4 * rate)
                break;
            const auto phase = rnd.nextDouble() * juce::MathConstants<double>::twoPi;
            for (int i = (int) (start * rate); i < n; ++i)
            {
                const auto t = i / rate - start;
                out.addSample (0, i, level / (float) k * (float) (std::exp (-t * (1.5 + 0.4 * k))
                                                                 * std::sin (juce::MathConstants<double>::twoPi * f * t + phase)));
            }
        }
    };
    pluck (0.0, 82.41, 0.18f);
    pluck (seconds / 2, 110.0, 0.12f);
    pluck (seconds / 2, 164.81, 0.1f);
    pluck (seconds / 2, 220.0, 0.08f);
    return out;
}

void writeWav (const juce::File& file, const juce::AudioBuffer<float>& buffer)
{
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
    auto writer = juce::WavAudioFormat().createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                                    .withSampleRate (rate)
                                                                    .withNumChannels (buffer.getNumChannels())
                                                                    .withBitsPerSample (24));
    writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
}

void set (Fuzz14Processor& p, const char* id, float value)
{
    auto* param = p.params.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

struct Result
{
    juce::AudioBuffer<float> audio;
    double seconds = 0.0;
};

Result play (Fuzz14Processor& p, const juce::AudioBuffer<float>& input)
{
    p.setPlayConfigDetails (1, 2, rate, blockSize);
    p.prepareToPlay (rate, blockSize);
    Result r;
    r.audio.setSize (2, input.getNumSamples());
    juce::AudioBuffer<float> block (2, blockSize);
    juce::MidiBuffer midi;
    const auto start = std::chrono::steady_clock::now();
    for (int pos = 0; pos < input.getNumSamples(); pos += blockSize)
    {
        const auto n = std::min (blockSize, input.getNumSamples() - pos);
        block.setSize (2, n, false, false, true);
        block.clear();
        block.copyFrom (0, 0, input, 0, pos, n);
        p.processBlock (block, midi);
        for (int ch = 0; ch < 2; ++ch)
            r.audio.copyFrom (ch, pos, block, ch, 0, n);
    }
    r.seconds = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();
    return r;
}

bool finite (const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (ch, i)))
                return false;
    return true;
}

juce::String levels (const juce::AudioBuffer<float>& b)
{
    return "peak " + juce::String (juce::Decibels::gainToDecibels (b.getMagnitude (0, b.getNumSamples())), 1)
           + " dBFS, rms " + juce::String (juce::Decibels::gainToDecibels (b.getRMSLevel (0, 0, b.getNumSamples())), 1) + " dBFS";
}
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    const auto input = guitar (4.0);
    writeWav (outDir.getChildFile ("in_guitar.wav"), input);

    std::cout << "defaults (the 2014 board):\n";
    {
        Fuzz14Processor p;
        const auto r = play (p, input);
        check (finite (r.audio), levels (r.audio));
        check (r.seconds < 4.0 * 0.5, juce::String (4.0 / r.seconds, 1) + "x real time at 48 kHz (2x oversampled)");
        check (p.getLatencySamples() >= 0, "latency " + juce::String (p.getLatencySamples()) + " samples");
        writeWav (outDir.getChildFile ("out_default.wav"), r.audio);
    }

    const auto& types = diodeTypes();
    for (const char* spot : { "d1", "d2", "d3" })
    {
        std::cout << "every diode in " << juce::String (spot).toUpperCase() << ":\n";
        for (size_t t = 0; t < types.size(); ++t)
        {
            Fuzz14Processor p;
            set (p, spot, (float) t);
            const auto r = play (p, input);
            check (finite (r.audio), juce::String (types[t].name).paddedRight (' ', 13) + levels (r.audio));
            if (t == 3 || t == 10 || t == 15)                // germanium, red LED, pulled out
                writeWav (outDir.getChildFile (juce::String ("out_") + spot + "_" + juce::String (types[t].name).replaceCharacter (' ', '_') + ".wav"), r.audio);
        }
    }

    std::cout << "D1 and D2 both pulled out (nothing biases the transistor):\n";
    {
        Fuzz14Processor p;
        set (p, "d1", 15.0f);
        set (p, "d2", 15.0f);
        const auto r = play (p, input);
        check (finite (r.audio), levels (r.audio));
    }

    std::cout << "dying battery:\n";
    {
        Fuzz14Processor p;
        set (p, "battery", 1.0f);
        const auto r = play (p, input);
        check (finite (r.audio), levels (r.audio));
        writeWav (outDir.getChildFile ("out_dying_battery.wav"), r.audio);
    }

    std::cout << "bypass and mute:\n";
    {
        Fuzz14Processor p;
        set (p, "bypass", 1.0f);
        const auto r = play (p, input);
        const auto latency = p.getLatencySamples();
        float worst = 0.0f;
        for (int i = latency; i < input.getNumSamples(); ++i)
            worst = std::max (worst, std::abs (r.audio.getSample (0, i) - input.getSample (0, i - latency)));
        check (worst < 1e-6f, "bypass gives back the clean input, " + juce::String (latency) + " samples late like the pedal");

        Fuzz14Processor q;
        set (q, "mute", 1.0f);
        const auto m = play (q, input);
        check (m.audio.getMagnitude (0, m.audio.getNumSamples()) == 0.0f, "mute is silent");
    }

    std::cout << "swapping diodes and turning the battery down while it plays:\n";
    {
        Fuzz14Processor p;
        p.setPlayConfigDetails (1, 2, rate, blockSize);
        p.prepareToPlay (rate, blockSize);
        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;
        bool ok = true;
        for (int pos = 0, k = 0; pos + blockSize <= input.getNumSamples(); pos += blockSize, ++k)
        {
            if (k % 40 == 0)
            {
                set (p, "d1", (float) ((k / 40) % (int) types.size()));
                set (p, "d3", (float) ((k / 40 + 7) % (int) types.size()));
                set (p, "battery", (float) ((k / 40) % 5) / 4.0f);
            }
            block.clear();
            block.copyFrom (0, 0, input, 0, pos, blockSize);
            p.processBlock (block, midi);
            ok = ok && finite (block) && block.getMagnitude (0, blockSize) <= 2.0f;
        }
        check (ok, "no NaNs, never past the +-2.0 safety clip");
    }

    // the window: a loud chord so the diodes glow
    for (const auto& [name, d1, d2, d3] : { std::tuple<const char*, int, int, int> { "gui.png", 0, 0, 0 }, { "gui_led.png", 10, 12, 3 },
                                             { "gui_blue.png", 13, 15, 0 } })
    {
        Fuzz14Processor p;
        set (p, "d1", (float) d1);
        set (p, "d2", (float) d2);
        set (p, "d3", (float) d3);
        set (p, "input", 12.0f);
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        play (p, input);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
        const auto file = outDir.getChildFile (name);
        file.deleteFile();
        juce::FileOutputStream stream (file);
        juce::PNGImageFormat().writeImageToStream (image, stream);
    }

    std::cout << (failures == 0 ? "all passed\n" : juce::String (failures) + " FAILED\n");
    return failures == 0 ? 0 : 1;
}
