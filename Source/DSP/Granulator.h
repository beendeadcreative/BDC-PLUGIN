#pragma once

#include <JuceHeader.h>
#include "CircularBuffer.h"
#include "GenerativeEngine.h"

// The core "new melody" engine. Continuously schedules short overlapping
// grains (windowed snippets read back from the CircularBuffer), each
// pitch-shifted and time-positioned by the GenerativeEngine, and sums them
// into an output buffer. This is what turns captured audio into evolving
// melodic phrases rather than a straight-through granular blur. The summed
// output is gently lowpassed and soft-clipped to keep dense/pitched grain
// overlap sounding smooth instead of harsh.
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

    // triggerMode: when true, ignores the free-running/synced density
    // clock entirely and instead spawns exactly one grain per call where
    // onsetDetectedThisBlock is true (see TransientDetector) - each input
    // hit gets its own grain instead of grains drifting independently of
    // what's actually being played.
    void process (const CircularBuffer& source, GenerativeEngine& generative,
                   juce::AudioBuffer<float>& output, int numSamples,
                   bool triggerMode, bool onsetDetectedThisBlock);

private:
    struct Grain
    {
        bool active = false;
        double readPos = 0.0;      // absolute position on the buffer's global timeline
        double pitchRatio = 1.0;
        int age = 0;               // samples played so far
        int lengthSamples = 1;
        float pan = 0.5f;

        // Per-grain darkening lowpass - grains pitched well above unity get
        // a lower cutoff here (set once at spawn, see spawnGrain) so the
        // upper end of the pitch-walk range reads as warm rather than icy.
        float lowpassAlpha = 1.0f;
        std::array<float, 2> filterState { { 0.0f, 0.0f } };
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

    // Grains overlap heavily at normal density, so re-rolling the pitch walk
    // on every single grain used to mean simultaneous grains would often
    // disagree on pitch - a smeared, dissonant cluster rather than a clean
    // note. Instead, a pitch is drawn once and held across a short run of
    // grains (a "note"), so overlapping grains reinforce each other.
    static constexpr float noteHoldMinSeconds = 0.15f;
    static constexpr float noteHoldMaxSeconds = 0.4f;
    double samplesUntilNewNote = 0.0;
    double heldPitchRatio = 1.0;
    juce::Random noteHoldRandom { 3 };

    // Gentle one-pole lowpass (tames pitch-shift aliasing/grit) + soft clip
    // (rounds off peaks from dense grain overlap instead of hard-clipping).
    static constexpr float smoothingCutoffHz = 9000.0f;
    float lowpassAlpha = 1.0f;
    std::array<float, 2> lowpassState { { 0.0f, 0.0f } };
};
