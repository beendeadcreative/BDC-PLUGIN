#include "SeedGenerator.h"

void SeedGenerator::prepare (double newSampleRate, int /*numChannels*/)
{
    sampleRate = newSampleRate;

    // Envelope follower time constants for the presence detector: fast-ish
    // attack (notice input quickly), slower release (don't yank the seed
    // pad back in the instant you stop playing).
    presenceAttackCoeff  = (float) std::exp (-1.0 / (0.05 * sampleRate));
    presenceReleaseCoeff = (float) std::exp (-1.0 / (1.5 * sampleRate));

    reset();
}

void SeedGenerator::reset()
{
    inputPresence = 0.0f;
    for (auto& p : partials)
    {
        p.phase = 0.0f;
        p.driftPhase = 0.0f;
    }
}

void SeedGenerator::process (juce::AudioBuffer<float>& outputBlock, int numSamples)
{
    outputBlock.clear();

    const float rootFreq = (float) juce::MidiMessage::getMidiNoteInHertz (rootMidiNote);
    const int numChannels = outputBlock.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;

        for (auto& p : partials)
        {
            // Slow drift keeps the detune moving so partials don't lock
            // into a static, obviously-synthetic beating pattern.
            float detuneCents = 6.0f * std::sin (p.driftPhase);
            float freq = rootFreq * p.ratio * std::pow (2.0f, detuneCents / 1200.0f);

            sample += std::sin (p.phase) * 0.18f;

            p.phase += juce::MathConstants<float>::twoPi * freq / (float) sampleRate;
            if (p.phase > juce::MathConstants<float>::twoPi)
                p.phase -= juce::MathConstants<float>::twoPi;

            p.driftPhase += juce::MathConstants<float>::twoPi * p.driftRate / (float) sampleRate;
            if (p.driftPhase > juce::MathConstants<float>::twoPi)
                p.driftPhase -= juce::MathConstants<float>::twoPi;
        }

        for (int ch = 0; ch < numChannels; ++ch)
            outputBlock.setSample (ch, i, sample);
    }
}

float SeedGenerator::updateInputPresence (const juce::AudioBuffer<float>& liveInput, int numSamples)
{
    const int numChannels = liveInput.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float rectified = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            rectified = juce::jmax (rectified, std::abs (liveInput.getSample (ch, i)));

        float coeff = rectified > inputPresence ? presenceAttackCoeff : presenceReleaseCoeff;
        inputPresence = coeff * inputPresence + (1.0f - coeff) * rectified;
    }

    // Map from raw amplitude to a 0-1 "presence" with a small noise floor
    // so room hiss doesn't count as "playing".
    const float noiseFloor = 0.01f;
    float presence01 = juce::jlimit (0.0f, 1.0f, (inputPresence - noiseFloor) / 0.2f);
    return presence01;
}
