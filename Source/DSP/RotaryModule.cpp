#include "RotaryModule.h"

void RotaryModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    drum.filter.prepare (spec);
    drum.filter.setType (juce::dsp::LinkwitzRileyFilter<float>::Type::lowpass);
    drum.filter.setCutoffFrequency (800.0f);
    drum.vibratoDelay.prepare (spec);
    drum.vibratoDelay.setMaximumDelayInSamples (4096);

    horn.filter.prepare (spec);
    horn.filter.setType (juce::dsp::LinkwitzRileyFilter<float>::Type::highpass);
    horn.filter.setCutoffFrequency (800.0f);
    horn.vibratoDelay.prepare (spec);
    horn.vibratoDelay.setMaximumDelayInSamples (4096);

    speedSmoothed.reset (spec.sampleRate, 1.5); // ~1.5s motor spin-up/down
    speedSmoothed.setCurrentAndTargetValue (0.0f);

    lowBuffer.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);
    highBuffer.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);

    reset();
}

void RotaryModule::reset()
{
    drum.filter.reset();
    drum.vibratoDelay.reset();
    horn.filter.reset();
    horn.vibratoDelay.reset();
}

void RotaryModule::setParameters (float speed01, float mix01)
{
    speedSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, speed01));
    mix = juce::jlimit (0.0f, 1.0f, mix01);
}

void RotaryModule::processRotor (Rotor& rotor, juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float phase = rotor.phase[(size_t) juce::jmin (ch, 1)];

            float ampLfo = std::sin (phase);
            float gain = 1.0f - rotor.ampDepth + rotor.ampDepth * (0.5f + 0.5f * ampLfo);

            float delaySamples = rotor.baseDelaySamples + rotor.vibratoDepthSamples * std::sin (phase);

            rotor.vibratoDelay.pushSample (ch, buffer.getSample (ch, i));
            rotor.vibratoDelay.setDelay (juce::jmax (1.0f, delaySamples));
            float vibrated = rotor.vibratoDelay.popSample (ch);

            buffer.setSample (ch, i, vibrated * gain);
        }

        for (auto& p : rotor.phase)
        {
            p += juce::MathConstants<float>::twoPi * rotor.rateHz / (float) sampleRate;
            if (p > juce::MathConstants<float>::twoPi)
                p -= juce::MathConstants<float>::twoPi;
        }
    }
}

void RotaryModule::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();

    lowBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);
    highBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);
    lowBuffer.makeCopyOf (buffer, true);
    highBuffer.makeCopyOf (buffer, true);

    {
        juce::dsp::AudioBlock<float> block (lowBuffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        drum.filter.process (ctx);
    }
    {
        juce::dsp::AudioBlock<float> block (highBuffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        horn.filter.process (ctx);
    }

    // Advance the motor speed once per block (smooth ramp between slow/fast).
    float speed = speedSmoothed.getNextValue();
    drum.rateHz = juce::jmap (speed, 0.0f, 1.0f, drumSlowHz, drumFastHz);
    horn.rateHz = juce::jmap (speed, 0.0f, 1.0f, hornSlowHz, hornFastHz);

    processRotor (drum, lowBuffer, currentSampleRate);
    processRotor (horn, highBuffer, currentSampleRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* dry = buffer.getWritePointer (ch);
        auto* lo = lowBuffer.getReadPointer (ch);
        auto* hi = highBuffer.getReadPointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float wet = lo[i] + hi[i];
            dry[i] = dry[i] * (1.0f - mix) + wet * mix;
        }
    }
}
