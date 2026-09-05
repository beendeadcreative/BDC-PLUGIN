#pragma once

#include <JuceHeader.h>

// Cassette 4-track emulation (Tascam Porta 02 MkII vibe, leaning warm -
// closer to 1960s AM radio than crisp hi-fi tape): a single 0-1 "amount"
// continuously scales tape wow/flutter pitch wobble, a warm/boxy narrow-
// band frequency response, asymmetric (even-harmonic) saturation, a warm
// lowpassed hiss bed, and the occasional brief volume dropout that gives
// worn tape/old-radio playback its glitchy, physical feel - fully
// transparent at 0, fully lo-fi at 1. Meant to sit at the end of the
// chain, as if the whole mix were bounced to tape.
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
        juce::dsp::IIR::Filter<float> highpass; // post-saturation: tone shaping + DC blocking
        juce::dsp::IIR::Filter<float> midBump;
        juce::dsp::IIR::Filter<float> warmthShelf; // low-shelf body/warmth boost
        float noiseLowpassState = 0.0f;            // shapes hiss into a soft "whoosh" instead of white noise
    };

    std::array<ChannelState, 2> channels;

    float amount = 0.0f;
    double currentSampleRate = 44100.0;

    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
    static constexpr float wowRateHz = 0.9f;
    static constexpr float flutterRateHz = 7.0f;

    float noiseLowpassAlpha = 1.0f;
    juce::Random noiseRandom { 3 };

    // Tape dropouts: brief random volume dips like a worn tape losing head
    // contact for a moment, giving the static a "glitchy," physical feel
    // rather than a constant, unchanging noise floor.
    double samplesUntilNextDropout = 0.0;
    int dropoutSamplesRemaining = 0;
    int dropoutTotalSamples = 1;
    float dropoutDepth = 0.0f;
    juce::Random dropoutRandom { 5 };
};
