#include "DelayModule.h"

void DelayModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples ((int) (spec.sampleRate * 2.0)); // up to 2s

    for (auto& f : feedbackFilters)
    {
        f.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (spec.sampleRate, 4500.0f);
        f.prepare (spec);
    }

    reset();
}

void DelayModule::reset()
{
    delayLine.reset();
    for (auto& f : feedbackFilters)
        f.reset();
}

void DelayModule::setParameters (float delayMs, float feedback01, float mix01)
{
    delayInSamples = (float) (delayMs * 0.001 * currentSampleRate);
    feedback = juce::jlimit (0.0f, 0.95f, feedback01);
    mix = juce::jlimit (0.0f, 1.0f, mix01);
}

void DelayModule::process (juce::AudioBuffer<float>& buffer)
{
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    delayLine.setDelay (delayInSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto& filter = feedbackFilters[(size_t) ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float delayed = delayLine.popSample (ch);
            float filtered = filter.processSample (delayed);

            float toWrite = data[i] + filtered * feedback;
            delayLine.pushSample (ch, toWrite);

            data[i] = data[i] * (1.0f - mix) + delayed * mix;
        }
    }
}
