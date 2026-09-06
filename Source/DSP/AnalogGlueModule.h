#pragma once

#include <JuceHeader.h>

// A subtle, always-in-the-chain saturation stage sitting after everything
// else - like a mixing console's output transformer or a tape machine's
// output stage gently rounding off whatever passes through it, regardless
// of which effects are engaged. Meant to stay understated: even at its
// maximum setting this should read as "glue" holding the mix together,
// not as an audible distortion effect.
class AnalogGlueModule
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setAmount (float amount01);
    void process (juce::AudioBuffer<float>& buffer);

private:
    float amount = 0.0f;
    std::array<juce::dsp::IIR::Filter<float>, 2> dcBlockers;
};
