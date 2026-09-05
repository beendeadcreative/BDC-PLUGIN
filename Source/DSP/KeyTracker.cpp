#include "KeyTracker.h"

namespace
{
    // Same 5 scales as GenerativeEngine, as semitone intervals from the root.
    const int scaleTemplates[5][7] = {
        { 0, 2, 4, 5, 7, 9, 11 },  // Major
        { 0, 2, 3, 5, 7, 8, 10 },  // Natural Minor
        { 0, 2, 3, 5, 7, 9, 10 },  // Dorian
        { 0, 2, 4, 7, 9, -1, -1 }, // Major Pentatonic
        { 0, 3, 5, 7, 10, -1, -1 } // Minor Pentatonic
    };
    const int scaleTemplateSizes[5] { 7, 7, 7, 5, 5 };
}

void KeyTracker::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    // Histogram half-life of ~6 seconds: recent playing dominates, but a
    // brief gap between phrases doesn't reset the sense of key.
    decayAlpha = std::pow (0.5f, 1.0f / (float) (6.0 * sampleRate));
    reset();
}

void KeyTracker::reset()
{
    pitchClassWeight.fill (0.0f);
}

void KeyTracker::seedRootScale (int rootPitchClass, int scaleType) noexcept
{
    currentRoot = juce::jlimit (0, 11, rootPitchClass);
    currentScale = juce::jlimit (0, 4, scaleType);
}

float KeyTracker::scoreCombo (int root, int scale) const noexcept
{
    float score = 0.0f;

    for (int pc = 0; pc < 12; ++pc)
    {
        int interval = ((pc - root) % 12 + 12) % 12;
        bool inScale = false;
        for (int i = 0; i < scaleTemplateSizes[scale]; ++i)
        {
            if (scaleTemplates[scale][i] == interval)
            {
                inScale = true;
                break;
            }
        }

        // Reward weight that falls inside the scale, penalize (lightly)
        // weight that falls outside it.
        score += pitchClassWeight[(size_t) pc] * (inScale ? 1.0f : -0.4f);
    }

    return score;
}

void KeyTracker::update (bool pitchDetected, float frequencyHz, int numSamples)
{
    float blockDecay = std::pow (decayAlpha, (float) numSamples);
    for (auto& w : pitchClassWeight)
        w *= blockDecay;

    if (pitchDetected && frequencyHz > 0.0f)
    {
        float midiFloat = 69.0f + 12.0f * std::log2 (frequencyHz / 440.0f);
        int pitchClass = ((int) std::round (midiFloat)) % 12;
        if (pitchClass < 0)
            pitchClass += 12;

        // Weight by how long this block was, so sustained notes build up
        // more than fleeting ones.
        pitchClassWeight[(size_t) pitchClass] += (float) numSamples / (float) sampleRate;
    }

    float totalWeight = 0.0f;
    for (auto w : pitchClassWeight)
        totalWeight += w;

    // Not enough has been played yet to say anything meaningful - keep
    // whatever the seeded/current guess is.
    if (totalWeight < 0.5f)
        return;

    int bestRoot = currentRoot;
    int bestScale = currentScale;
    float bestScore = scoreCombo (currentRoot, currentScale);

    for (int root = 0; root < 12; ++root)
    {
        for (int scale = 0; scale < 5; ++scale)
        {
            float score = scoreCombo (root, scale);
            if (score > bestScore)
            {
                bestScore = score;
                bestRoot = root;
                bestScale = scale;
            }
        }
    }

    // Only switch when the new candidate is meaningfully better, not just
    // narrowly ahead - keeps the tracked key from jittering between two
    // near-identical fits.
    float currentComboScore = scoreCombo (currentRoot, currentScale);
    if (bestScore > currentComboScore + 0.3f)
    {
        currentRoot = bestRoot;
        currentScale = bestScale;
    }
}
