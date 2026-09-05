#pragma once

#include <JuceHeader.h>

// Cassette 4-track emulation (Tascam Porta 02 MkII vibe): a single 0-1
// "amount" continuously scales tape wow/flutter pitch wobble, a
// dulled/boxy frequency response, soft saturation, and tape hiss - fully
// transparent at 0, fully lo-fi cassette-colored at 1. Meant to sit at the
// end of the chain, as if the whole mix were bounced to tape.
class TapeModule
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setAmount (float amount01);
    void process (juce::AudioBuffer<float>& buffer);

private:
    struct ChannelState
    {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> wobbleDelay { 2048 };
        juce::dsp::IIR::Filter<float> lowpass;
        juce::dsp::IIR::Filter<float> highpass;
        juce::dsp::IIR::Filter<float> midBump;
    };

    std::array<ChannelState, 2> channels;

    float amount = 0.0f;
    double currentSampleRate = 44100.0;

    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
    static constexpr float wowRateHz = 0.9f;
    static constexpr float flutterRateHz = 7.0f;

    juce::Random noiseRandom { 3 };
};
