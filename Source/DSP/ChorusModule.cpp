#include "ChorusModule.h"

void ChorusModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    chorus.prepare (spec);
    reset();
}

void ChorusModule::reset()
{
    chorus.reset();
}

void ChorusModule::setParameters (float rateHz, float depth01, float mix01)
{
    chorus.setRate (rateHz);
    chorus.setDepth (depth01);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.15f);
    chorus.setMix (mix01);
}

void ChorusModule::process (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    chorus.process (context);
}
