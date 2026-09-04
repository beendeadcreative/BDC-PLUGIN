#pragma once

#include <JuceHeader.h>
#include "CircularBuffer.h"
#include "GenerativeEngine.h"

// The core "new melody" engine. Continuously schedules short overlapping
// grains (windowed snippets read back from the CircularBuffer), each
// pitch-shifted and time-positioned by the GenerativeEngine, and sums them
// into an output buffer. This is what turns raw captured audio (real
// playing, or the SeedGenerator's pad when idle) into evolving melodic
// phrases rather than a straight-through granular blur.
class Granulator
{
public:
    void prepare (double sampleRate, int numChannels, int maxBlockSize);
    void reset();

    // grainsPerSecond: how often new grains are spawned.
    // grainSizeMs: length of each grain.
    // spreadSeconds: how far back into the buffer grains are allowed to be
    //   read from (the generative engine's position walk is scaled to this).
    void setParameters (float grainsPerSecond, float grainSizeMs, float spreadSeconds);

    void process (const CircularBuffer& source, GenerativeEngine& generative,
                   juce::AudioBuffer<float>& output, int numSamples);

private:
    struct Grain
    {
        bool active = false;
        double readPos = 0.0;      // absolute position on the buffer's global timeline
        double pitchRatio = 1.0;
        int age = 0;               // samples played so far
        int lengthSamples = 1;
        float pan = 0.5f;
    };

    void spawnGrain (const CircularBuffer& source, GenerativeEngine& generative);
    static float hannWindow (float phase01);

    double sampleRate = 44100.0;
    int numChannels = 2;

    static constexpr int maxGrains = 48;
    std::array<Grain, maxGrains> grains;

    float grainsPerSecond = 6.0f;
    float grainSizeMs = 120.0f;
    float spreadSeconds = 2.0f;

    double samplesUntilNextGrain = 0.0;
    juce::Random panRandom { 2 };
};
