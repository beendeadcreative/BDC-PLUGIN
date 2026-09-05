#pragma once

#include <JuceHeader.h>

// Continuously estimates a "key center" (root + scale) from what's being
// played, instead of requiring it to be set by hand: a slowly-decaying
// 12-bin pitch-class histogram is fed by the tuner's pitch detector every
// block, then scored against all 12 roots x the same 5 scale templates
// GenerativeEngine already supports. Light hysteresis keeps it from
// flickering between near-tied candidates.
class KeyTracker
{
public:
    void prepare (double sampleRate);
    void reset();

    // Sets the starting root/scale (e.g. from the manual Root/Scale
    // controls) so there's a sensible guess before anything's been played.
    void seedRootScale (int rootPitchClass, int scaleType) noexcept;

    // Call once per block with this block's pitch-detection result.
    void update (bool pitchDetected, float frequencyHz, int numSamples);

    // Safe to call from another thread (e.g. the editor's UI timer) while
    // update() keeps running on the audio thread.
    int getRootPitchClass() const noexcept { return currentRoot.load(); } // 0-11, C..B
    int getScaleType() const noexcept { return currentScale.load(); }     // 0-4, same ordering as the Scale parameter

private:
    float scoreCombo (int root, int scale) const noexcept;

    double sampleRate = 44100.0;
    float decayAlpha = 0.0f; // per-sample histogram decay factor
    std::array<float, 12> pitchClassWeight {};

    std::atomic<int> currentRoot { 9 };
    std::atomic<int> currentScale { 4 };
};
