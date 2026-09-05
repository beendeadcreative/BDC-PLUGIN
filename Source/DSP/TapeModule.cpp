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
        cs.warmthShelf.prepare (spec);
    }

    // Fixed gentle lowpass that shapes the hiss into a soft "whoosh"
    // instead of bright, full-bandwidth white noise.
    noiseLowpassAlpha = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 4000.0f / (float) currentSampleRate);

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
        cs.warmthShelf.reset();
        cs.noiseLowpassState = 0.0f;
    }

    wowPhase = 0.0f;
    flutterPhase = 0.0f;
}

void TapeModule::setAmount (float amount01)
{
    amount = juce::jlimit (0.0f, 1.0f, amount01);

    // A clean-ish top end at 0% dulling down to a boxy, rolled-off cassette
    // response at 100%, plus a small low-mid bump and a low-shelf body
    // boost for warmth, and a rising highpass (applied post-saturation, see
    // process()) to thin out the sub-bass tape can't really hold and to
    // mop up any DC drift from the asymmetric saturation.
    float lowpassCutoff = juce::jmap (amount, 0.0f, 1.0f, 16000.0f, 5000.0f);
    float highpassCutoff = juce::jmap (amount, 0.0f, 1.0f, 15.0f, 60.0f);
    float midBumpGainDb = juce::jmap (amount, 0.0f, 1.0f, 0.0f, 4.5f);
    float warmthShelfGainDb = juce::jmap (amount, 0.0f, 1.0f, 0.0f, 3.5f);

    for (auto& cs : channels)
    {
        cs.lowpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, lowpassCutoff);
        cs.highpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, highpassCutoff);
        cs.midBump.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 400.0f, 0.9f, juce::Decibels::decibelsToGain (midBumpGainDb));
        cs.warmthShelf.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            currentSampleRate, 200.0f, 0.7f, juce::Decibels::decibelsToGain (warmthShelfGainDb));
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

            float toned = cs.warmthShelf.processSample (cs.lowpass.processSample (cs.midBump.processSample (wobbled)));

            // Asymmetric drive (gentler on the negative half) instead of a
            // symmetric tanh: symmetric clipping only adds odd harmonics,
            // which reads as harsh/transistor-y. The asymmetry adds even
            // harmonics too, which is what actually sounds warm/tube-like.
            float driven = toned * driveAmount;
            float saturated = (driven >= 0.0f ? std::tanh (driven) : std::tanh (driven * 0.75f)) / saturationNorm;

            // DC blocking + final tone shaping happens after saturation, so
            // it also mops up any DC drift the asymmetry introduces.
            float shaped = cs.highpass.processSample (saturated);

            // Hiss shaped into a soft whoosh (lowpassed) rather than bright
            // white noise - much closer to real tape hiss.
            float rawNoise = noiseRandom.nextFloat() * 2.0f - 1.0f;
            cs.noiseLowpassState += noiseLowpassAlpha * (rawNoise - cs.noiseLowpassState);
            float hiss = hissLevel * cs.noiseLowpassState * 3.0f; // compensate for lowpass energy loss

            float wet = shaped + hiss;
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
