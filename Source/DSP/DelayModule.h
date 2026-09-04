#pragma once

#include <JuceHeader.h>

// Feedback delay line with a one-pole lowpass in the feedback path (so
// repeats darken over time rather than looping forever at full brightness).
class DelayModule
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setParameters (float delayMs, float feedback01, float mix01);
    void process (juce::AudioBuffer<float>& buffer);

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 192000 };
    std::array<juce::dsp::IIR::Filter<float>, 2> feedbackFilters;

    float delayInSamples = 0.0f;
    float feedback = 0.35f;
    float mix = 0.3f;
    double currentSampleRate = 44100.0;
};
