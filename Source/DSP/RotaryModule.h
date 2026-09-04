#pragma once

#include <JuceHeader.h>

// Simplified Leslie-style rotary speaker simulation: splits the signal into
// a low ("drum") and high ("horn") band, spins each with its own amplitude
// tremolo + pitch vibrato (via a modulated short delay line) at different
// rates, and ramps between slow/fast speeds the way a real motor
// accelerates rather than snapping instantly.
class RotaryModule
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    // speed01: 0 = slow (chorale), 1 = fast (tremolo/brake).
    void setParameters (float speed01, float mix01);
    void process (juce::AudioBuffer<float>& buffer);

private:
    struct Rotor
    {
        juce::dsp::LinkwitzRileyFilter<float> filter;
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> vibratoDelay { 4096 };
        std::array<float, 2> phase { { 0.0f, juce::MathConstants<float>::halfPi } };
        float rateHz = 1.0f;
        float ampDepth = 0.3f;
        float vibratoDepthSamples = 2.0f;
        float baseDelaySamples = 10.0f;
    };

    void processRotor (Rotor& rotor, juce::AudioBuffer<float>& buffer, double sampleRate);

    Rotor drum;  // low band
    Rotor horn;  // high band

    juce::AudioBuffer<float> lowBuffer, highBuffer;

    juce::SmoothedValue<float> speedSmoothed { 0.0f };
    float mix = 0.6f;
    double currentSampleRate = 44100.0;

    static constexpr float drumSlowHz = 0.6f, drumFastHz = 5.0f;
    static constexpr float hornSlowHz = 0.8f, hornFastHz = 6.5f;
};
