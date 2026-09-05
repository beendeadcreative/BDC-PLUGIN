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
    // JUCE's raw depth range swings into audibly-dissonant "out of tune"
    // pitch modulation well before the knob reaches halfway; scale it down
    // so the full range stays in lush-chorus territory instead.
    chorus.setDepth (depth01 * 0.45f);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.1f);
    chorus.setMix (mix01);
}

void ChorusModule::process (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    chorus.process (context);
}
