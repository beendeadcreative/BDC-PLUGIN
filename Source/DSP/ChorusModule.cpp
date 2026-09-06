#include "ChorusModule.h"

float ChorusModule::triangleWave (float phase01) noexcept
{
    const float t = phase01 - std::floor (phase01);
    return 4.0f * std::abs (t - 0.5f) - 1.0f; // -1..1
}

void ChorusModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples ((int) (spec.sampleRate * 0.05)); // 50ms is plenty of headroom

    wetLowpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (spec.sampleRate, 5500.0f);
    wetLowpass.prepare (spec);

    reset();
}

void ChorusModule::reset()
{
    delayLine.reset();
    wetLowpass.reset();
    lfoPhase = 0.0f;
}

void ChorusModule::setParameters (float newRateHz, float depth01, float mix01)
{
    rateHz = juce::jmax (0.01f, newRateHz);
    depthMs = juce::jlimit (0.0f, 1.0f, depth01) * 6.0f;
    mix = juce::jlimit (0.0f, 1.0f, mix01);
}

void ChorusModule::process (juce::AudioBuffer<float>& buffer)
{
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    if (numChannels < 2)
        return; // the L/R phase-inversion trick needs a stereo signal to widen

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        // A single BBD-style delay tap fed from the mono sum of the input,
        // like a real chorus circuit working on one signal path - the
        // stereo width below comes from how that one tap is recombined,
        // not from running two independent delays.
        const float monoIn = 0.5f * (left[i] + right[i]);

        const float lfo = triangleWave (lfoPhase);
        const float modulatedDelayMs = baseDelayMs + lfo * depthMs * 0.5f;
        const float modulatedDelaySamples = juce::jmax (1.0f, modulatedDelayMs * 0.001f * (float) currentSampleRate);

        delayLine.pushSample (0, monoIn);
        delayLine.setDelay (modulatedDelaySamples);
        float wet = delayLine.popSample (0);

        // BBD chips inherently roll off the top end of whatever passes
        // through them - without this the comb-filtered swirl below reads
        // as thin/metallic instead of warm.
        wet = wetLowpass.processSample (wet);

        // The classic trick: sum (not crossfade) the same wet tap back
        // into dry as +wet on the left, -wet on the right. The moving
        // comb-filter notches land in different places per channel,
        // which is what makes this sound wide rather than just "chorused
        // then panned."
        const float chorusLeft = left[i] + wet * wetGain;
        const float chorusRight = right[i] - wet * wetGain;

        left[i] = left[i] * (1.0f - mix) + chorusLeft * mix;
        right[i] = right[i] * (1.0f - mix) + chorusRight * mix;

        lfoPhase += rateHz / (float) currentSampleRate;
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;
    }
}
