#pragma once

#include <JuceHeader.h>

// A Roland Juno-style chorus, not a generic modulated delay: real Juno-60/
// 106 chorus circuits get their character from three things a plain
// crossfaded chorus doesn't have -
//   1. A single BBD delay tap, triangle-LFO modulated (a linear, "swoopy"
//      sweep rather than a smooth sine curve).
//   2. Stereo width from phase inversion, not a second voice: the delayed
//      tap is summed *with* the dry signal (not crossfaded) as +wet on the
//      left and -wet on the right, so the two channels' comb-filter notches
//      land in different places and the effect feels wide without ever
//      being fed two independently delayed signals.
//   3. A BBD chip's inherent top-end rolloff on the delayed signal, which
//      is what keeps the comb-filtered swirl sounding warm rather than
//      harsh/metallic.
// Mix still blends the resulting (already-wide) chorus voice back with the
// pristine dry signal, same as every other effect module here.
class ChorusModule
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setParameters (float rateHz, float depth01, float mix01);
    void process (juce::AudioBuffer<float>& buffer);

private:
    static float triangleWave (float phase01) noexcept;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 4096 };
    juce::dsp::IIR::Filter<float> wetLowpass;

    float rateHz = 0.5f;
    float depthMs = 3.0f; // peak-to-peak modulation range
    float mix = 0.25f;
    double currentSampleRate = 44100.0;

    float lfoPhase = 0.0f; // 0..1

    static constexpr float baseDelayMs = 7.0f;
    static constexpr float wetGain = 0.55f; // keeps dry+wet summing from pushing well past unity
};
