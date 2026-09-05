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

void DelayModule::setParameters (float delayMs, float feedback01, float mix01, int taps, float tapSpread01)
{
    delayInSamples = (float) (delayMs * 0.001 * currentSampleRate);
    feedback = juce::jlimit (0.0f, 0.95f, feedback01);
    mix = juce::jlimit (0.0f, 1.0f, mix01);
    numTaps = juce::jlimit (1, maxTaps, taps);
    tapSpread = juce::jlimit (0.0f, 1.0f, tapSpread01);
}

void DelayModule::process (juce::AudioBuffer<float>& buffer)
{
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    std::array<float, maxTaps> tapDelaySamples {};
    std::array<float, maxTaps> tapLevel {};
    std::array<float, maxTaps> tapLeftGain {};
    std::array<float, maxTaps> tapRightGain {};

    for (int i = 0; i < numSamples; ++i)
    {
        // Tape motor speed instability: a slow wow plus a faster flutter,
        // modulating the delay time itself - this is the signature tape
        // echo "wobble" a clean digital delay doesn't have.
        float wobbleMs = wowDepthMs * std::sin (wowPhase) + flutterDepthMs * std::sin (flutterPhase);
        float modulatedDelaySamples = juce::jmax (1.0f, delayInSamples + wobbleMs * 0.001f * (float) currentSampleRate);

        // Lay out the taps: evenly spaced fractions of the delay time,
        // e.g. numTaps == 3 reads at 1/3, 2/3, and 1x. Only the last
        // (full-time) tap is centred and feeds the feedback/tone path;
        // earlier taps are quieter pre-echoes, panned alternately by
        // tapSpread for width instead of stacking up in the centre.
        for (int k = 0; k < numTaps; ++k)
        {
            const bool isMainTap = (k == numTaps - 1);
            const float ratio = (float) (k + 1) / (float) numTaps;

            tapDelaySamples[(size_t) k] = juce::jmax (1.0f, modulatedDelaySamples * ratio);
            tapLevel[(size_t) k] = isMainTap ? 1.0f : 0.7f * ratio;

            const float pan = isMainTap ? 0.0f : ((k % 2 == 0) ? -tapSpread : tapSpread);
            tapLeftGain[(size_t) k] = juce::jlimit (0.0f, 1.0f, 1.0f - pan);
            tapRightGain[(size_t) k] = juce::jlimit (0.0f, 1.0f, 1.0f + pan);
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            auto& lp = feedbackLowpass[(size_t) ch];
            auto& hp = feedbackHighpass[(size_t) ch];

            float wetSum = 0.0f;
            float mainDelayed = 0.0f;

            for (int k = 0; k < numTaps; ++k)
            {
                const bool isMainTap = (k == numTaps - 1);
                float tapSample = delayLine.popSample (ch, tapDelaySamples[(size_t) k], isMainTap);
                float gain = (ch == 0) ? tapLeftGain[(size_t) k] : tapRightGain[(size_t) k];
                wetSum += tapSample * tapLevel[(size_t) k] * gain;

                if (isMainTap)
                    mainDelayed = tapSample;
            }

            // Band-limit like tape heads, then soft-saturate: repeats
            // darken and gently compress/glow rather than looping forever
            // clean, and heavy feedback settles into a saturated ceiling
            // instead of clipping or screeching away.
            float toned = hp.processSample (lp.processSample (mainDelayed));
            float saturated = std::tanh (toned * 1.4f) * 0.85f;

            float toWrite = data[i] + saturated * feedback;
            delayLine.pushSample (ch, toWrite);

            data[i] = data[i] * (1.0f - mix) + wetSum * mix;
        }

        wowPhase += juce::MathConstants<float>::twoPi * wowRateHz / (float) currentSampleRate;
        if (wowPhase > juce::MathConstants<float>::twoPi)
            wowPhase -= juce::MathConstants<float>::twoPi;

        flutterPhase += juce::MathConstants<float>::twoPi * flutterRateHz / (float) currentSampleRate;
        if (flutterPhase > juce::MathConstants<float>::twoPi)
            flutterPhase -= juce::MathConstants<float>::twoPi;
    }
}
