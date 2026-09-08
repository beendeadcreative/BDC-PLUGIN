#pragma once

#include <JuceHeader.h>

// Decides what each new grain should sound like: a scale-constrained random
// walk picks the next pitch (so melodies stay in key rather than wandering
// chromatically), and a second, slower walk picks where in the capture
// buffer to read from (so it keeps revisiting different moments of what was
// played instead of always grabbing the newest audio). The pitch walk also
// leans toward whatever pitch classes are actually ringing right now (see
// updateChroma()), so it reads as harmonizing with the input in the moment
// rather than just staying diatonically in key the whole time.
class GenerativeEngine
{
public:
    enum class Scale
    {
        Major,
        NaturalMinor,
        Dorian,
        MajorPentatonic,
        MinorPentatonic
    };

    void prepare (double sampleRate) noexcept;

    void setRootNote (int midiNoteNumber) noexcept { rootMidiNote = midiNoteNumber; }
    void setScale (Scale newScale) noexcept { scale = newScale; }

    // 0 = very stepwise/predictable melodic motion, 1 = frequent large leaps.
    void setUnpredictability (float amount01) noexcept { unpredictability = juce::jlimit (0.0f, 1.0f, amount01); }

    // Call once per block with this block's pitch-detection result (the
    // same data KeyTracker uses) so the pitch walk can gravitate toward
    // pitch classes with live energy. Its own histogram decays much faster
    // than KeyTracker's (~1s vs ~6s): this is meant to track "what's
    // sounding right now" for the walk to chase, not settle on a stable key.
    void updateChroma (bool pitchDetected, float frequencyHz, int numSamples) noexcept;

    // Called once per spawned grain. Returns a pitch ratio to apply to
    // playback speed (1.0 = no shift) derived from the current scale degree.
    float nextPitchRatio();

    // Called once per spawned grain. Returns how far back (0-1, fraction of
    // the buffer/spray range) to read from.
    float nextPositionFraction();

    void reset();

private:
    int degreeToSemitone (int degree) const;
    int stepRandomWalk (int current, int minVal, int maxVal, float leapProbability, juce::Random& rng);

    // Nudges a freshly-proposed pitch degree toward a nearby neighbour with
    // more live chroma energy, rather than overriding the walk outright -
    // see updateChroma().
    int applyChromaBias (int proposedDegree);
    int pitchClassForDegree (int degree) const noexcept;

    int rootMidiNote = 57; // A3
    Scale scale = Scale::MinorPentatonic;
    float unpredictability = 0.3f;

    double sampleRate = 44100.0;
    float chromaDecayAlpha = 0.0f; // per-sample histogram decay factor
    std::array<float, 12> chromaWeight {};

    // The pitch walk is deliberately skewed above the root rather than
    // centred on it: a symmetric walk spends half its time doubling the
    // same or a lower register as what's being played, which reads as the
    // generator "building up underneath" the source. Biasing the range
    // (and starting there) makes it characteristically sit in a higher
    // register instead, more like a wash floating over the top.
    static constexpr int pitchDegreeMin = -3;
    static constexpr int pitchDegreeMax = 11; // trimmed down from 17: the top of the old range read as thin/icy rather than airy
    static constexpr int pitchDegreeCenter = 7;

    int pitchDegree = pitchDegreeCenter;
    int positionDegree = 0;

    juce::Random random { 1 };
};
