#include "AnalogGlueModule.h"

void AnalogGlueModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (auto& f : dcBlockers)
    {
        // Not tone shaping - just cleans up any DC creep from the
        // saturation curve below, well below anything audible.
        f.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (spec.sampleRate, 20.0f);
        f.prepare (spec);
    }

    reset();
}

void AnalogGlueModule::reset()
{
    for (auto& f : dcBlockers)
        f.reset();
}

void AnalogGlueModule::setAmount (float amount01)
{
    amount = juce::jlimit (0.0f, 1.0f, amount01);
}

void AnalogGlueModule::process (juce::AudioBuffer<float>& buffer)
{
    if (amount <= 0.0001f)
        return;

    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    // A gentle, ear-safe drive range: even at 100% this stays a soft
    // round-off of peaks rather than an audible distortion effect. The
    // shaped signal is normalised back to unity gain at the drive point,
    // then blended in by `amount` so it eases in rather than switching on.
    const float drive = 1.0f + amount * 2.0f;
    const float normalise = std::tanh (drive);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto& dc = dcBlockers[(size_t) ch];

        for (int i = 0; i < numSamples; ++i)
        {
            const float shaped = std::tanh (data[i] * drive) / normalise;
            const float blended = data[i] * (1.0f - amount) + shaped * amount;
            data[i] = dc.processSample (blended);
        }
    }
}
