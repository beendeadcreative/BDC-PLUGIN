#pragma once

#include <JuceHeader.h>

// Feedback delay line styled after a tape echo (Roland Space Echo-ish)
// rather than a clean digital delay: repeats wobble slightly in pitch
// (tape wow/flutter), darken and band-limit each pass, and soft-saturate
// in the feedback path so heavy feedback compresses/glows instead of
// clipping or running away.
class DelayModule
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setParameters (float delayMs, float feedback01, float mix01);
    void process (juce::AudioBuffer<float>& buffer);

private:
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
