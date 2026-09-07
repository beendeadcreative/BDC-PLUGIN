#include "Granulator.h"

void Granulator::prepare (double newSampleRate, int newNumChannels, int /*maxBlockSize*/)
{
    sampleRate = newSampleRate;
    numChannels = newNumChannels;

    lowpassAlpha = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * smoothingCutoffHz / (float) sampleRate);

    reset();
}

void Granulator::reset()
{
    for (auto& g : grains)
        g.active = false;

    samplesUntilNextGrain = 0.0;
    samplesUntilNewNote = 0.0;
    lowpassState.fill (0.0f);
}

void Granulator::setParameters (float newGrainsPerSecond, float newGrainSizeMs, float newSpreadSeconds)
{
    grainsPerSecond = juce::jmax (0.1f, newGrainsPerSecond);
    grainSizeMs = juce::jmax (5.0f, newGrainSizeMs);
    spreadSeconds = juce::jmax (0.05f, newSpreadSeconds);
}

float Granulator::hannWindow (float phase01)
{
    return 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase01);
}

void Granulator::spawnGrain (const CircularBuffer& source, GenerativeEngine& generative)
{
    Grain* freeGrain = nullptr;
    for (auto& g : grains)
    {
        if (! g.active)
        {
            freeGrain = &g;
            break;
        }
    }

    if (freeGrain == nullptr)
        return; // grain pool exhausted, drop this one

    freeGrain->lengthSamples = juce::jmax (16, (int) (grainSizeMs * 0.001f * (float) sampleRate));

    if (samplesUntilNewNote <= 0.0)
    {
        heldPitchRatio = generative.nextPitchRatio();
        const float holdSeconds = noteHoldMinSeconds + noteHoldRandom.nextFloat() * (noteHoldMaxSeconds - noteHoldMinSeconds);
        samplesUntilNewNote = (double) holdSeconds * sampleRate;
    }
    freeGrain->pitchRatio = heldPitchRatio;

    // Grains transposed well above unity read as thin/icy; darken those
    // specifically (grains at or below unity keep the full-bright cutoff)
    // so the top of the pitch-walk range sounds warm instead of glassy.
    const float octavesUp = juce::jmax (0.0f, std::log2 ((float) freeGrain->pitchRatio));
    const float darkenAmount = juce::jlimit (0.0f, 1.0f, octavesUp / 2.5f);
    const float grainCutoffHz = juce::jmap (darkenAmount, 0.0f, 1.0f, 9000.0f, 3200.0f);
    freeGrain->lowpassAlpha = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * grainCutoffHz / (float) sampleRate);
    freeGrain->filterState.fill (0.0f);

    float posFraction = generative.nextPositionFraction();
    float maxSamplesAgo = spreadSeconds * (float) sampleRate;
    float startSamplesAgo = posFraction * maxSamplesAgo;

    // Make sure the grain never has to read audio that hasn't been written
    // yet (relevant when pitchRatio > 1, i.e. reading forward faster than
    // real time).
    float minSamplesAgo = (float) freeGrain->lengthSamples * (float) freeGrain->pitchRatio + 4.0f;
    startSamplesAgo = juce::jmax (startSamplesAgo, minSamplesAgo);

    freeGrain->readPos = (double) source.getGlobalWritePosition() - (double) startSamplesAgo;
    freeGrain->age = 0;
    freeGrain->pan = 0.15f + 0.7f * panRandom.nextFloat();
    freeGrain->active = true;
}

void Granulator::process (const CircularBuffer& source, GenerativeEngine& generative,
                            juce::AudioBuffer<float>& output, int numSamples,
                            bool triggerMode, bool onsetDetectedThisBlock)
{
    output.clear();

    const double grainIntervalSamples = sampleRate / (double) grainsPerSecond;

    // Rough loudness compensation so density/size changes don't wildly
    // change overall output level: more overlapping grains means each one
    // should contribute less. Deliberately conservative (extra headroom)
    // so dense overlap doesn't push the soft clip below into audibly
    // squashing things. In trigger mode there's no "grains per second" to
    // reason about, so just use a flat, ungained-down level.
    float avgOverlap = juce::jmax (1.0f, grainsPerSecond * (grainSizeMs * 0.001f));
    float outputGain = triggerMode ? 0.6f : 0.6f / std::sqrt (avgOverlap + 0.5f);

    if (triggerMode && onsetDetectedThisBlock)
        spawnGrain (source, generative);

    for (int i = 0; i < numSamples; ++i)
    {
        samplesUntilNewNote -= 1.0;

        if (! triggerMode)
        {
            samplesUntilNextGrain -= 1.0;
            if (samplesUntilNextGrain <= 0.0)
            {
                spawnGrain (source, generative);
                samplesUntilNextGrain += grainIntervalSamples;
            }
        }

        for (auto& g : grains)
        {
            if (! g.active)
                continue;

            float windowGain = hannWindow ((float) g.age / (float) g.lengthSamples) * outputGain;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float sample = source.readAtGlobalIndex (juce::jmin (ch, 1), g.readPos);

                float& grainFilterState = g.filterState[(size_t) juce::jmin (ch, 1)];
                grainFilterState += g.lowpassAlpha * (sample - grainFilterState);
                sample = grainFilterState;

                float channelPan = (numChannels <= 1) ? 1.0f
                                  : (ch == 0 ? (1.0f - g.pan) : g.pan);

                output.addSample (ch, i, sample * windowGain * channelPan);
            }

            g.readPos += g.pitchRatio;
            g.age += 1;

            if (g.age >= g.lengthSamples)
                g.active = false;
        }

        // Gentle top-end smoothing (tames the aliasing/grit that comes from
        // resampled, pitch-shifted grains), then a soft clip so dense
        // overlaps round off gracefully instead of clipping harshly.
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float raw = output.getSample (ch, i);
            float& state = lowpassState[(size_t) juce::jmin (ch, 1)];
            state += lowpassAlpha * (raw - state);
            output.setSample (ch, i, std::tanh (state));
        }
    }
}
