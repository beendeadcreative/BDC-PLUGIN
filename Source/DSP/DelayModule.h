#pragma once

#include <JuceHeader.h>

// Feedback delay line styled after a tape echo (Roland Space Echo-ish)
// rather than a clean digital delay: repeats wobble slightly in pitch
// (tape wow/flutter), darken and band-limit each pass, and soft-saturate
// in the feedback path so heavy feedback compresses/glows instead of
// clipping or running away.
//
// Multi-tap: with numTaps > 1, extra echoes are read from the same delay
// line at even fractions of the main delay time (e.g. 3 taps read at
// 1/3, 2/3, and 1x the delay time), like the multiple playback heads on
// a real tape echo. Only the last (full-time) tap feeds the feedback/tone
// path, so numTaps == 1 reproduces the original single-tap behaviour
// exactly. Tap Spread pans the earlier taps alternately left/right for a
// wider, more rhythmic texture; the main tap always stays centred.
//
// Reverse: instead of reading at a fixed (wobbling) lag, sweeps the read
// point's lag from 1 sample up to roughly 2x the delay time over a cycle
// exactly one delay-time long - which plays back that same chunk of
// history backwards, at normal speed, before jumping back to the start
// of the next chunk. Overrides Taps entirely (mutually exclusive) to
// keep the read logic simple; still feeds the same feedback/tone/
// saturation path as the normal single-tap case.
class DelayModule
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setParameters (float delayMs, float feedback01, float mix01, int numTaps, float tapSpread01, bool reverse);
    void process (juce::AudioBuffer<float>& buffer);

private:
    static constexpr int maxTaps = 4;

    int numTaps = 1;
    float tapSpread = 0.0f;
    bool reverseMode = false;
    int reverseWindowSamples = 1; // length of the chunk currently being played backwards
    int reverseCyclePos = 0;      // how far into that chunk's backwards playback we are

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 192000 };
    std::array<juce::dsp::IIR::Filter<float>, 2> feedbackLowpass;
    std::array<juce::dsp::IIR::Filter<float>, 2> feedbackHighpass;

    float delayInSamples = 0.0f;
    float feedback = 0.35f;
    float mix = 0.3f;
    double currentSampleRate = 44100.0;

    // Tape speed wobble: a slow "wow" and a faster "flutter" LFO, summed.
    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
    static constexpr float wowRateHz = 0.7f;
    static constexpr float wowDepthMs = 2.2f;
    static constexpr float flutterRateHz = 6.3f;
    static constexpr float flutterDepthMs = 0.5f;
};
