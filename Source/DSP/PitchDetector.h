#pragma once

#include <JuceHeader.h>

// Monophonic pitch detector (YIN algorithm) for a live tuner readout: feed
// it the raw input each block, and it periodically re-analyzes a rolling
// window and updates an atomically-readable "current detected frequency"
// that the UI can poll from a Timer without touching audio-thread state.
class PitchDetector
{
public:
    void prepare (double sampleRate);
    void reset();

    // Feed live input each block (mixed down to mono internally).
    void process (const juce::AudioBuffer<float>& input, int numSamples);

    float getDetectedFrequencyHz() const noexcept { return frequencyHz.load(); }
    bool isPitchDetected() const noexcept { return detected.load(); }

    struct NoteResult
    {
        juce::String name;
        int cents;
    };

    // Converts a frequency to the nearest equal-tempered note name (A440)
    // and how many cents sharp (+) or flat (-) it is from that note.
    static NoteResult frequencyToNote (float freqHz) noexcept;

private:
    void analyze();

    double sampleRate = 44100.0;

    static constexpr int analysisSize = 2048;
    static constexpr int analysisHop = 512;
    static constexpr float minFrequencyHz = 55.0f;   // ~A1
    static constexpr float maxFrequencyHz = 1200.0f; // covers most melodic playing

    std::vector<float> ringBuffer;
    int ringWritePos = 0;
    int samplesSinceAnalysis = 0;
    int64_t totalSamplesWritten = 0;

    std::vector<float> analysisScratch;
    std::vector<float> yinBuffer;

    std::atomic<float> frequencyHz { 0.0f };
    std::atomic<bool> detected { false };
};
