#include "CircularBuffer.h"

void CircularBuffer::prepare (double sampleRate, int numChannels, float lengthInSeconds)
{
    bufferLength = juce::jmax (1, (int) (sampleRate * lengthInSeconds));
    buffer.setSize (numChannels, bufferLength);
    reset();
}

void CircularBuffer::reset()
{
    buffer.clear();
    writePos = 0;
    globalWritePos = 0;
}

void CircularBuffer::write (const juce::AudioBuffer<float>& inputBlock)
{
    const int numChannels = juce::jmin (buffer.getNumChannels(), inputBlock.getNumChannels());
    const int numSamples = inputBlock.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = inputBlock.getReadPointer (ch);
        float* dst = buffer.getWritePointer (ch);

        int pos = writePos;
        for (int i = 0; i < numSamples; ++i)
        {
            dst[pos] = src[i];
            pos = (pos + 1) % bufferLength;
        }
    }

    writePos = (writePos + numSamples) % bufferLength;
    globalWritePos += numSamples;
}

int CircularBuffer::wrapIndex (int index) const noexcept
{
    index %= bufferLength;
    if (index < 0)
        index += bufferLength;
    return index;
}

float CircularBuffer::readInterpolated (int channel, float samplesAgo) const
{
    if (channel >= buffer.getNumChannels())
        return 0.0f;

    samplesAgo = juce::jlimit (1.0f, (float) (bufferLength - 3), samplesAgo);

    // Position samplesAgo behind the write head, wrapped into the buffer.
    float exactPos = (float) writePos - samplesAgo;
    while (exactPos < 0.0f)
        exactPos += (float) bufferLength;

    int pos1 = (int) exactPos;
    float frac = exactPos - (float) pos1;

    int pos0 = wrapIndex (pos1 - 1);
    int pos2 = wrapIndex (pos1 + 1);
    int pos3 = wrapIndex (pos1 + 2);
    pos1 = wrapIndex (pos1);

    const float* data = buffer.getReadPointer (channel);
    float y0 = data[pos0], y1 = data[pos1], y2 = data[pos2], y3 = data[pos3];

    // Catmull-Rom cubic interpolation instead of a straight lerp: this is
    // what keeps pitch-shifted grain playback sounding smooth rather than
    // aliased/gritty, especially at larger pitch ratios.
    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    float a3 = y1;

    return ((a0 * frac + a1) * frac + a2) * frac + a3;
}

float CircularBuffer::readAtGlobalIndex (int channel, double globalIndex) const
{
    double samplesAgo = (double) globalWritePos - globalIndex;
    return readInterpolated (channel, (float) samplesAgo);
}
