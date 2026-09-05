#include "PitchDetector.h"

void PitchDetector::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    ringBuffer.assign ((size_t) analysisSize, 0.0f);
    analysisScratch.assign ((size_t) analysisSize, 0.0f);
    yinBuffer.assign ((size_t) (analysisSize / 2), 0.0f);
    reset();
}

void PitchDetector::reset()
{
    std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
    ringWritePos = 0;
    samplesSinceAnalysis = 0;
    totalSamplesWritten = 0;
    frequencyHz.store (0.0f);
    detected.store (false);
}

void PitchDetector::process (const juce::AudioBuffer<float>& input, int numSamples)
{
    const int numChannels = input.getNumChannels();
    const int size = (int) ringBuffer.size();

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += input.getSample (ch, i);
        if (numChannels > 0)
            mono /= (float) numChannels;

        ringBuffer[(size_t) ringWritePos] = mono;
        ringWritePos = (ringWritePos + 1) % size;
        ++totalSamplesWritten;
        ++samplesSinceAnalysis;
    }

    if (totalSamplesWritten >= analysisSize && samplesSinceAnalysis >= analysisHop)
    {
        samplesSinceAnalysis = 0;
        analyze();
    }
}

void PitchDetector::analyze()
{
    const int size = analysisSize;

    // Unwrap the ring buffer into chronological order (oldest first).
    for (int i = 0; i < size; ++i)
    {
        int idx = (ringWritePos + i) % size;
        analysisScratch[(size_t) i] = ringBuffer[(size_t) idx];
    }

    // Skip analysis on near-silence so we don't report noise/hiss as a note.
    float rms = 0.0f;
    for (int i = 0; i < size; ++i)
        rms += analysisScratch[(size_t) i] * analysisScratch[(size_t) i];
    rms = std::sqrt (rms / (float) size);

    if (rms < 0.01f)
    {
        detected.store (false);
        return;
    }

    const int maxTau = juce::jmin ((int) (sampleRate / minFrequencyHz), (int) yinBuffer.size() - 1);
    const int minTau = juce::jmax (2, (int) (sampleRate / maxFrequencyHz));

    // YIN difference function: d(tau) = sum of squared differences between
    // the signal and itself shifted by tau samples.
    yinBuffer[0] = 1.0f;
    for (int tau = 1; tau <= maxTau; ++tau)
    {
        float sum = 0.0f;
        for (int i = 0; i < size - maxTau; ++i)
        {
            float d = analysisScratch[(size_t) i] - analysisScratch[(size_t) (i + tau)];
            sum += d * d;
        }
        yinBuffer[(size_t) tau] = sum;
    }

    // Cumulative mean normalized difference function.
    float runningSum = 0.0f;
    for (int tau = 1; tau <= maxTau; ++tau)
    {
        runningSum += yinBuffer[(size_t) tau];
        yinBuffer[(size_t) tau] *= (float) tau / runningSum;
    }

    // First dip below threshold within the musically relevant tau range,
    // walked forward to its local minimum (standard YIN absolute threshold
    // step - avoids picking spurious very-short-period octave errors).
    const float threshold = 0.15f;
    int tauEstimate = -1;
    for (int tau = minTau; tau < maxTau; ++tau)
    {
        if (yinBuffer[(size_t) tau] < threshold)
        {
            while (tau + 1 <= maxTau && yinBuffer[(size_t) (tau + 1)] < yinBuffer[(size_t) tau])
                ++tau;
            tauEstimate = tau;
            break;
        }
    }

    if (tauEstimate < 0)
    {
        detected.store (false);
        return;
    }

    // Parabolic interpolation around the minimum for sub-sample accuracy.
    float betterTau = (float) tauEstimate;
    if (tauEstimate > 0 && tauEstimate < maxTau)
    {
        float s0 = yinBuffer[(size_t) (tauEstimate - 1)];
        float s1 = yinBuffer[(size_t) tauEstimate];
        float s2 = yinBuffer[(size_t) (tauEstimate + 1)];
        float denom = 2.0f * (2.0f * s1 - s0 - s2);
        if (std::abs (denom) > 1.0e-9f)
            betterTau += (s2 - s0) / denom;
    }

    float freq = (float) sampleRate / betterTau;
    if (freq >= minFrequencyHz && freq <= maxFrequencyHz)
    {
        frequencyHz.store (freq);
        detected.store (true);
    }
    else
    {
        detected.store (false);
    }
}

PitchDetector::NoteResult PitchDetector::frequencyToNote (float freqHz) noexcept
{
    if (freqHz <= 0.0f)
        return { "--", 0 };

    static const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    float midiFloat = 69.0f + 12.0f * std::log2 (freqHz / 440.0f);
    int nearestMidi = (int) std::round (midiFloat);
    int cents = (int) std::round ((midiFloat - (float) nearestMidi) * 100.0f);

    int nameIndex = ((nearestMidi % 12) + 12) % 12;
    int octave = nearestMidi / 12 - 1;

    return { juce::String (names[nameIndex]) + juce::String (octave), cents };
}
