#pragma once

#include <JuceHeader.h>

// Rolling capture buffer per channel. The granulator reads out of this at
// arbitrary fractional positions (for pitch-shifted grains); the plugin
// continuously writes fresh audio (live input, or seed material, or a mix
// of both) into it every block.
class CircularBuffer
{
public:
    void prepare (double sampleRate, int numChannels, float lengthInSeconds);
    void reset();

    // Writes one block. inputBlock must have numChannels channels.
    void write (const juce::AudioBuffer<float>& inputBlock);

    // Reads one interpolated sample, `samplesAgo` behind the current write
    // head (0 = most recently written sample). Fractional values are
    // linearly interpolated, which is what makes pitch-shifted grain
    // playback possible.
    float readInterpolated (int channel, float samplesAgo) const;

    // Ever-increasing count of samples written so far (never wraps). Grains
    // store an absolute read position on this timeline so their playback
    // stays correct even as they run across multiple process blocks and the
    // write head keeps moving.
    int64_t getGlobalWritePosition() const noexcept { return globalWritePos; }
    float readAtGlobalIndex (int channel, double globalIndex) const;

    int getBufferLength() const noexcept { return bufferLength; }

private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
    int bufferLength = 0;
    int64_t globalWritePos = 0;
};
