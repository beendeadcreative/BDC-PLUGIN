#include "InputActivityDetector.h"

void InputActivityDetector::prepare (double sampleRate)
{
    attackCoeff  = (float) std::exp (-1.0 / (0.01 * sampleRate));
    releaseCoeff = (float) std::exp (-1.0 / (0.4 * sampleRate));
    reset();
}

void InputActivityDetector::reset()
{
    envelope = 0.0f;
    active = false;
}

bool InputActivityDetector::updateAndIsActive (const juce::AudioBuffer<float>& input, int numSamples)
{
    const int numChannels = input.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float rectified = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            rectified = juce::jmax (rectified, std::abs (input.getSample (ch, i)));

        float coeff = rectified > envelope ? attackCoeff : releaseCoeff;
        envelope = coeff * envelope + (1.0f - coeff) * rectified;
    }

    if (! active && envelope > onThreshold)
        active = true;
    else if (active && envelope < offThreshold)
        active = false;

    return active;
}
