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

float CircularBuffer::readInterpolated (int channel, float samplesAgo) const
{
    if (channel >= buffer.getNumChannels())
        return 0.0f;

    samplesAgo = juce::jlimit (0.0f, (float) (bufferLength - 2), samplesAgo);

    // Position samplesAgo behind the write head, wrapped into the buffer.
    float exactPos = (float) writePos - samplesAgo;
    while (exactPos < 0.0f)
        exactPos += (float) bufferLength;

    int pos0 = (int) exactPos;
    int pos1 = (pos0 + 1) % bufferLength;
    float frac = exactPos - (float) pos0;

    const float* data = buffer.getReadPointer (channel);
    return data[pos0] + frac * (data[pos1] - data[pos0]);
}

float CircularBuffer::readAtGlobalIndex (int channel, double globalIndex) const
{
    double samplesAgo = (double) globalWritePos - globalIndex;
    return readInterpolated (channel, (float) samplesAgo);
}
