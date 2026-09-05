#include "DelayModule.h"

void DelayModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples ((int) (spec.sampleRate * 2.0)); // up to 2s

    for (auto& f : feedbackLowpass)
    {
        f.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (spec.sampleRate, 3200.0f);
        f.prepare (spec);
    }

    for (auto& f : feedbackHighpass)
    {
        f.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (spec.sampleRate, 140.0f);
        f.prepare (spec);
    }

    reset();
}

void DelayModule::reset()
{
    delayLine.reset();
    for (auto& f : feedbackLowpass)
        f.reset();
    for (auto& f : feedbackHighpass)
        f.reset();

    wowPhase = 0.0f;
    flutterPhase = 0.0f;
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

    for (int i = 0; i < numSamples; ++i)
    {
        // Tape motor speed instability: a slow wow plus a faster flutter,
        // modulating the delay time itself - this is the signature tape
        // echo "wobble" a clean digital delay doesn't have.
        float wobbleMs = wowDepthMs * std::sin (wowPhase) + flutterDepthMs * std::sin (flutterPhase);
        float modulatedDelaySamples = juce::jmax (1.0f, delayInSamples + wobbleMs * 0.001f * (float) currentSampleRate);
        delayLine.setDelay (modulatedDelaySamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            auto& lp = feedbackLowpass[(size_t) ch];
            auto& hp = feedbackHighpass[(size_t) ch];

            float delayed = delayLine.popSample (ch);

            // Band-limit like tape heads, then soft-saturate: repeats
            // darken and gently compress/glow rather than looping forever
            // clean, and heavy feedback settles into a saturated ceiling
            // instead of clipping or screeching away.
            float toned = hp.processSample (lp.processSample (delayed));
            float saturated = std::tanh (toned * 1.4f) * 0.85f;

            float toWrite = data[i] + saturated * feedback;
            delayLine.pushSample (ch, toWrite);

            data[i] = data[i] * (1.0f - mix) + delayed * mix;
        }

        wowPhase += juce::MathConstants<float>::twoPi * wowRateHz / (float) currentSampleRate;
        if (wowPhase > juce::MathConstants<float>::twoPi)
            wowPhase -= juce::MathConstants<float>::twoPi;

        flutterPhase += juce::MathConstants<float>::twoPi * flutterRateHz / (float) currentSampleRate;
        if (flutterPhase > juce::MathConstants<float>::twoPi)
            flutterPhase -= juce::MathConstants<float>::twoPi;
    }
}
