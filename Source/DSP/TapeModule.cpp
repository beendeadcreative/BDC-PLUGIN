#include "TapeModule.h"

void TapeModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    for (auto& cs : channels)
    {
        cs.wobbleDelay.prepare (spec);
        cs.wobbleDelay.setMaximumDelayInSamples (2048);
        cs.lowpass.prepare (spec);
        cs.highpass.prepare (spec);
        cs.midBump.prepare (spec);
    }

    setAmount (amount); // establish initial filter coefficients
    reset();
}

void TapeModule::reset()
{
    for (auto& cs : channels)
    {
        cs.wobbleDelay.reset();
        cs.lowpass.reset();
        cs.highpass.reset();
        cs.midBump.reset();
    }

    wowPhase = 0.0f;
    flutterPhase = 0.0f;
}

void TapeModule::setAmount (float amount01)
{
    amount = juce::jlimit (0.0f, 1.0f, amount01);

    // A clean-ish top end at 0% dulling down to a boxy, rolled-off cassette
    // response at 100%, plus a small low-mid bump for that boxy warmth and
    // a rising highpass to thin out the sub-bass tape can't really hold.
    float lowpassCutoff = juce::jmap (amount, 0.0f, 1.0f, 16000.0f, 5200.0f);
    float highpassCutoff = juce::jmap (amount, 0.0f, 1.0f, 20.0f, 100.0f);
    float midBumpGainDb = juce::jmap (amount, 0.0f, 1.0f, 0.0f, 4.5f);

    for (auto& cs : channels)
    {
        cs.lowpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, lowpassCutoff);
        cs.highpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, highpassCutoff);
        cs.midBump.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 400.0f, 0.9f, juce::Decibels::decibelsToGain (midBumpGainDb));
    }
}

void TapeModule::process (juce::AudioBuffer<float>& buffer)
{
    if (amount <= 0.0005f)
        return; // fully clean - skip the whole chain

    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    const float wobbleDepthMs = amount * 1.6f;          // tape speed instability
    const float driveAmount = 1.0f + amount * 5.0f;      // saturation drive
    const float saturationNorm = std::tanh (driveAmount);
    const float hissLevel = amount * amount * 0.01f;     // audible mainly at higher amounts
    const float baseDelaySamples = 5.0f;                 // headroom so wobble can swing both ways

    for (int i = 0; i < numSamples; ++i)
    {
        float wobbleMs = wobbleDepthMs * (0.7f * std::sin (wowPhase) + 0.3f * std::sin (flutterPhase));
        float modulatedDelay = juce::jmax (0.5f, baseDelaySamples + wobbleMs * 0.001f * (float) currentSampleRate);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& cs = channels[(size_t) ch];
            auto* data = buffer.getWritePointer (ch);

            cs.wobbleDelay.pushSample (ch, data[i]);
            cs.wobbleDelay.setDelay (modulatedDelay);
            float wobbled = cs.wobbleDelay.popSample (ch);

            float toned = cs.highpass.processSample (cs.lowpass.processSample (cs.midBump.processSample (wobbled)));
            float saturated = std::tanh (toned * driveAmount) / saturationNorm;
            float hiss = hissLevel * (noiseRandom.nextFloat() * 2.0f - 1.0f);

            float wet = saturated + hiss;
            data[i] = data[i] * (1.0f - amount) + wet * amount;
        }

        wowPhase += juce::MathConstants<float>::twoPi * wowRateHz / (float) currentSampleRate;
        if (wowPhase > juce::MathConstants<float>::twoPi)
            wowPhase -= juce::MathConstants<float>::twoPi;

        flutterPhase += juce::MathConstants<float>::twoPi * flutterRateHz / (float) currentSampleRate;
        if (flutterPhase > juce::MathConstants<float>::twoPi)
            flutterPhase -= juce::MathConstants<float>::twoPi;
    }
}
