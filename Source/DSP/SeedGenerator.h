#pragma once

#include <JuceHeader.h>

// Supplies material for the granulator when there's nothing (or not enough)
// coming in from the real input, so the plugin can run and generate music
// entirely on its own. A handful of detuned, slowly-drifting sine partials
// around the current root note form a soft evolving pad. Also tracks input
// level so the processor knows how much of this seed signal to blend in.
class SeedGenerator
{
public:
    void prepare (double sampleRate, int numChannels);
    void reset();

    void setRootNote (int midiNoteNumber) noexcept { rootMidiNote = midiNoteNumber; }

    // Fills outputBlock with numSamples of pad audio.
    void process (juce::AudioBuffer<float>& outputBlock, int numSamples);

    // Feed it the real input block each callback; returns a smoothed 0-1
    // "how present is the live input" estimate for crossfading.
    float updateInputPresence (const juce::AudioBuffer<float>& liveInput, int numSamples);

private:
    struct Partial
    {
        float ratio;      // multiplier on the root frequency
        float phase = 0.0f;
        float driftPhase = 0.0f;
        float driftRate;  // Hz, very slow, for gentle detune wander
    };

    double sampleRate = 44100.0;
    int rootMidiNote = 57; // A3
    std::array<Partial, 4> partials { {
        { 1.0f, 0.0f, 0.0f, 0.037f },
        { 1.5f, 0.0f, 0.0f, 0.021f },
        { 2.0f, 0.0f, 0.0f, 0.053f },
        { 3.0f, 0.0f, 0.0f, 0.014f }
    } };

    float inputPresence = 0.0f;
    float presenceAttackCoeff = 0.0f;
    float presenceReleaseCoeff = 0.0f;
};
