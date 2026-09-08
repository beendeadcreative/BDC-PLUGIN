#include "GenerativeEngine.h"

namespace
{
    const int majorIntervals[]           { 0, 2, 4, 5, 7, 9, 11 };
    const int naturalMinorIntervals[]    { 0, 2, 3, 5, 7, 8, 10 };
    const int dorianIntervals[]          { 0, 2, 3, 5, 7, 9, 10 };
    const int majorPentatonicIntervals[] { 0, 2, 4, 7, 9 };
    const int minorPentatonicIntervals[] { 0, 3, 5, 7, 10 };

    juce::Array<int> getIntervals (GenerativeEngine::Scale scale)
    {
        switch (scale)
        {
            case GenerativeEngine::Scale::Major:           return { majorIntervals, 7 };
            case GenerativeEngine::Scale::NaturalMinor:    return { naturalMinorIntervals, 7 };
            case GenerativeEngine::Scale::Dorian:          return { dorianIntervals, 7 };
            case GenerativeEngine::Scale::MajorPentatonic: return { majorPentatonicIntervals, 5 };
            case GenerativeEngine::Scale::MinorPentatonic:
            default:                                       return { minorPentatonicIntervals, 5 };
        }
    }
}

void GenerativeEngine::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    // Much faster than KeyTracker's ~6s key-sensing half-life - this is
    // meant to track "what's ringing right now", not settle on a stable key.
    chromaDecayAlpha = std::pow (0.5f, 1.0f / (float) (1.0 * newSampleRate));
    chromaWeight.fill (0.0f);
}

void GenerativeEngine::reset()
{
    pitchDegree = pitchDegreeCenter;
    positionDegree = 0;
    chromaWeight.fill (0.0f);
}

void GenerativeEngine::updateChroma (bool pitchDetected, float frequencyHz, int numSamples) noexcept
{
    float blockDecay = std::pow (chromaDecayAlpha, (float) numSamples);
    for (auto& w : chromaWeight)
        w *= blockDecay;

    if (pitchDetected && frequencyHz > 0.0f)
    {
        float midiFloat = 69.0f + 12.0f * std::log2 (frequencyHz / 440.0f);
        int pitchClass = ((int) std::round (midiFloat)) % 12;
        if (pitchClass < 0)
            pitchClass += 12;

        // Weight by how long this block was, so sustained notes build up
        // more chroma "gravity" than fleeting ones.
        chromaWeight[(size_t) pitchClass] += (float) numSamples / (float) sampleRate;
    }
}

int GenerativeEngine::degreeToSemitone (int degree) const
{
    auto intervals = getIntervals (scale);
    const int scaleSize = intervals.size();

    int octave = (int) std::floor ((float) degree / (float) scaleSize);
    int idx = degree - octave * scaleSize;

    return octave * 12 + intervals[idx];
}

int GenerativeEngine::stepRandomWalk (int current, int minVal, int maxVal, float leapProbability, juce::Random& rng)
{
    int step;

    if (rng.nextFloat() < leapProbability)
    {
        // A leap: jump further, in either direction, for melodic variety.
        // Kept modest (max ~a fourth in most scales) so it reads as an
        // interesting interval rather than a jarring register jump.
        step = rng.nextInt ({ 2, 4 }); // 2 or 3 scale steps
        if (rng.nextBool())
            step = -step;
    }
    else
    {
        // Ordinary stepwise motion, biased away from standing still so the
        // melody keeps moving.
        const int choices[] { -1, -1, 1, 1, 0 };
        step = choices[rng.nextInt (5)];
    }

    int next = current + step;

    // Reflect off the range boundaries rather than clamping, so motion
    // near the edges still feels alive instead of getting stuck.
    if (next < minVal) next = minVal + (minVal - next);
    if (next > maxVal) next = maxVal - (next - maxVal);

    return juce::jlimit (minVal, maxVal, next);
}

int GenerativeEngine::pitchClassForDegree (int degree) const noexcept
{
    int pc = (rootMidiNote + degreeToSemitone (degree)) % 12;
    if (pc < 0)
        pc += 12;
    return pc;
}

int GenerativeEngine::applyChromaBias (int proposedDegree)
{
    // Not enough live signal to say anything meaningful yet (mirrors
    // KeyTracker's silence-guard) - take the walk's own proposal untouched.
    float totalChroma = 0.0f;
    for (auto w : chromaWeight)
        totalChroma += w;
    if (totalChroma < 0.05f)
        return proposedDegree;

    // Weigh the proposed degree against its immediate neighbours rather
    // than picking freely across the whole range, so this nudges the
    // walk's own contour toward consonance instead of replacing it.
    const int candidates[3] { proposedDegree - 1, proposedDegree, proposedDegree + 1 };
    float weights[3];
    float weightSum = 0.0f;

    for (int i = 0; i < 3; ++i)
    {
        int clamped = juce::jlimit (pitchDegreeMin, pitchDegreeMax, candidates[i]);
        // A baseline keeps every candidate reachable even where there's no
        // live chroma energy, so this is a lean rather than a hard lock.
        weights[i] = 0.15f + chromaWeight[(size_t) pitchClassForDegree (clamped)];
        weightSum += weights[i];
    }

    float pick = random.nextFloat() * weightSum;
    for (int i = 0; i < 3; ++i)
    {
        pick -= weights[i];
        if (pick <= 0.0f)
            return juce::jlimit (pitchDegreeMin, pitchDegreeMax, candidates[i]);
    }

    return proposedDegree;
}

float GenerativeEngine::nextPitchRatio()
{
    int proposedDegree = stepRandomWalk (pitchDegree, pitchDegreeMin, pitchDegreeMax, unpredictability, random);
    pitchDegree = applyChromaBias (proposedDegree);
    int semitoneOffset = degreeToSemitone (pitchDegree);
    return std::pow (2.0f, (float) semitoneOffset / 12.0f);
}

float GenerativeEngine::nextPositionFraction()
{
    positionDegree = stepRandomWalk (positionDegree, 0, 100, unpredictability * 0.5f, random);
    return (float) positionDegree / 100.0f;
}
