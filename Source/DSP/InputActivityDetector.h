#pragma once

#include <JuceHeader.h>

// Tracks whether real audio is currently being played into the plugin.
// Used to (a) gate generation so it never starts from nothing, only after
// real input has actually been played, and (b) decide whether the capture
// buffer should keep recording silence or freeze on the last real material.
// Uses hysteresis (separate on/off thresholds) so it doesn't flicker at the
// edge of the noise floor.
class InputActivityDetector
{
public:
    void prepare (double sampleRate);
    void reset();

    // Call once per block. Returns true if input is currently considered
    // "active" (i.e. you're playing something into it right now).
    bool updateAndIsActive (const juce::AudioBuffer<float>& input, int numSamples);

private:
    float envelope = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    bool active = false;

    static constexpr float onThreshold = 0.02f;
    static constexpr float offThreshold = 0.01f;
};
