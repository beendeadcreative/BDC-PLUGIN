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

void GenerativeEngine::reset()
{
    pitchDegree = 0;
    positionDegree = 0;
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

float GenerativeEngine::nextPitchRatio()
{
    pitchDegree = stepRandomWalk (pitchDegree, -14, 14, unpredictability, random);
    int semitoneOffset = degreeToSemitone (pitchDegree);
    return std::pow (2.0f, (float) semitoneOffset / 12.0f);
}

float GenerativeEngine::nextPositionFraction()
{
    positionDegree = stepRandomWalk (positionDegree, 0, 100, unpredictability * 0.5f, random);
    return (float) positionDegree / 100.0f;
}
