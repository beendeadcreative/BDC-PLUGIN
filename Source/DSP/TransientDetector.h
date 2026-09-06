#pragma once

#include <JuceHeader.h>

// Lightweight broadband onset ("hit") detector, distinct from
// InputActivityDetector (which tracks a sustained on/off playing state).
// Compares a fast envelope (tracks sudden peaks) against a slow envelope
// (tracks the recent baseline level): a hit is a fast envelope that jumps
// well above the baseline. A refractory period after each detected onset
// stops a single transient's attack/ring from re-triggering multiple
// times. Used to drive Grain's Trigger mode, so a drum hit spawns a grain
// instead of relying on a free-running clock.
class TransientDetector
{
public:
    void prepare (double sampleRate);
    void reset();

    // Call once per block, before anything else touches the signal (a
    // pristine, unprocessed input reads onsets most reliably). Returns
    // true if an onset was detected anywhere in this block.
    bool updateAndDetectOnset (const juce::AudioBuffer<float>& input, int numSamples);

private:
    double sampleRate = 44100.0;

    float fastEnvelope = 0.0f;
    float slowEnvelope = 0.0f;

    float fastAttackCoeff = 0.0f, fastReleaseCoeff = 0.0f;
    float slowAttackCoeff = 0.0f, slowReleaseCoeff = 0.0f;

    int refractorySamplesRemaining = 0;

    static constexpr float onsetRatio = 1.6f;   // fast envelope must exceed slow x this to count as a hit
    static constexpr float onsetFloor = 0.015f; // ...and clear this absolute floor, so noise floor doesn't trigger
    static constexpr float refractoryMs = 40.0f;
};
