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

    samplesUntilNextDropout = 0.0;
    dropoutSamplesRemaining = 0;
    dropoutDepth = 0.0f;
}

void TapeModule::setAmount (float amount01)
{
    amount = juce::jlimit (0.0f, 1.0f, amount01);

    // A clean-ish top end at 0% dulling down to a warm, boxy, narrow-band
    // response at 100% - more "1960s AM radio" than crisp cassette: a
    // tighter lowpass, a bigger low-mid boxiness bump, and a bigger
    // low-shelf body boost, plus a rising highpass (applied post-
    // saturation, see process()) to thin out the sub-bass an AM speaker
    // can't reproduce and to mop up any DC drift from the asymmetric
    // saturation.
    float lowpassCutoff = juce::jmap (amount, 0.0f, 1.0f, 16000.0f, 3800.0f);
    float highpassCutoff = juce::jmap (amount, 0.0f, 1.0f, 15.0f, 90.0f);
    float midBumpGainDb = juce::jmap (amount, 0.0f, 1.0f, 0.0f, 6.0f);
    float warmthShelfGainDb = juce::jmap (amount, 0.0f, 1.0f, 0.0f, 6.0f);

    // Hiss darkens right along with the tone - a dull, warm radio-static
    // "whoosh" rather than a bright, constant fizz.
    float noiseLowpassCutoff = juce::jmap (amount, 0.0f, 1.0f, 6000.0f, 2200.0f);
    noiseLowpassAlpha = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * noiseLowpassCutoff / (float) currentSampleRate);

    for (auto& cs : channels)
    {
        cs.lowpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, lowpassCutoff);
        cs.highpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, highpassCutoff);
        cs.midBump.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 500.0f, 0.8f, juce::Decibels::decibelsToGain (midBumpGainDb));
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
    const float hissLevel = amount * amount * 0.014f;    // audible mainly at higher amounts
    const float baseDelaySamples = 5.0f;                 // headroom so wobble can swing both ways

    for (int i = 0; i < numSamples; ++i)
    {
        float wobbleMs = wobbleDepthMs * (0.7f * std::sin (wowPhase) + 0.3f * std::sin (flutterPhase));
        float modulatedDelay = juce::jmax (0.5f, baseDelaySamples + wobbleMs * 0.001f * (float) currentSampleRate);

        // Tape dropouts: occasional brief volume dips, like a worn tape
        // momentarily losing head contact - randomly timed (Poisson-ish
        // inter-arrival) and more frequent the more "tape" is dialed in.
        float dropoutGain = 1.0f;
        if (dropoutSamplesRemaining > 0)
        {
            float progress = 1.0f - (float) dropoutSamplesRemaining / (float) dropoutTotalSamples;
            float dip = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * progress));
            dropoutGain = 1.0f - dropoutDepth * dip;
            --dropoutSamplesRemaining;
        }
        else
        {
            samplesUntilNextDropout -= 1.0;
            if (samplesUntilNextDropout <= 0.0 && amount > 0.02f)
            {
                float eventsPerSecond = amount * 2.2f;
                float meanIntervalSamples = (float) currentSampleRate / juce::jmax (0.05f, eventsPerSecond);
                samplesUntilNextDropout = meanIntervalSamples * (0.3f + dropoutRandom.nextFloat() * 1.4f);

                dropoutTotalSamples = juce::jmax (1, (int) (currentSampleRate * (0.008 + dropoutRandom.nextDouble() * 0.03)));
                dropoutSamplesRemaining = dropoutTotalSamples;
                dropoutDepth = 0.35f + dropoutRandom.nextFloat() * 0.5f;
            }
        }

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

            float wet = (shaped + hiss) * dropoutGain;
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
