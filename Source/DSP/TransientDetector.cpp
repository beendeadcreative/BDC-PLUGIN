#include "TransientDetector.h"

namespace
{
    float coeffFor (float timeMs, double sampleRate)
    {
        return 1.0f - std::exp (-1.0f / (0.001f * timeMs * (float) sampleRate));
    }
}

void TransientDetector::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    fastAttackCoeff  = coeffFor (0.5f, sampleRate);
    fastReleaseCoeff = coeffFor (20.0f, sampleRate);
    slowAttackCoeff  = coeffFor (50.0f, sampleRate);
    slowReleaseCoeff = coeffFor (300.0f, sampleRate);

    reset();
}

void TransientDetector::reset()
{
    fastEnvelope = 0.0f;
    slowEnvelope = 0.0f;
    refractorySamplesRemaining = 0;
}

bool TransientDetector::updateAndDetectOnset (const juce::AudioBuffer<float>& input, int numSamples)
{
    const int numChannels = input.getNumChannels();
    bool onsetDetected = false;

    for (int i = 0; i < numSamples; ++i)
    {
        float rectified = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            rectified = juce::jmax (rectified, std::abs (input.getSample (ch, i)));

        fastEnvelope += (rectified > fastEnvelope ? fastAttackCoeff : fastReleaseCoeff) * (rectified - fastEnvelope);
        slowEnvelope += (rectified > slowEnvelope ? slowAttackCoeff : slowReleaseCoeff) * (rectified - slowEnvelope);

        if (refractorySamplesRemaining > 0)
        {
            --refractorySamplesRemaining;
        }
        else if (fastEnvelope > onsetFloor && fastEnvelope > slowEnvelope * onsetRatio)
        {
            onsetDetected = true;
            refractorySamplesRemaining = (int) (0.001f * refractoryMs * (float) sampleRate);

            // Snap the baseline partway up towards the hit so the tail of
            // this same transient doesn't immediately queue up another
            // onset the moment the refractory period ends.
            slowEnvelope = juce::jmax (slowEnvelope, fastEnvelope * 0.7f);
        }
    }

    return onsetDetected;
}
