#pragma once

#include <JuceHeader.h>

// Thin wrapper around JUCE's built-in chorus DSP so the processor has one
// consistent module interface (prepare/setParameters/process) like the
// hand-built effects.
class ChorusModule
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setParameters (float rateHz, float depth01, float mix01);
    void process (juce::AudioBuffer<float>& buffer);

private:
    juce::dsp::Chorus<float> chorus;
};
