#pragma once

#include <JuceHeader.h>

// Factory presets: a full snapshot of every parameter, so picking one gives
// a fully deterministic sound rather than layering on top of whatever was
// dialed in before. Note-division/multiplier indices refer to the same
// choice lists defined in PluginProcessor.cpp (TempoSync::divisionChoices /
// multiplierChoices): division 0-11 = 1/1,1/2,1/4,1/8,1/16,1/32,1/4.,1/8.,
// 1/16.,1/4T,1/8T,1/16T; multiplier 0-4 = /4,/2,x1,x2,x4.
struct FactoryPresetValues
{
    bool sustainOnSilence;
    int rootNote;             // 0-11, C..B
    int scaleType;            // 0-4, Major/NaturalMinor/Dorian/MajorPentatonic/MinorPentatonic
    float unpredictability;   // 0-1
    float grainDensity;       // 0.5-30 Hz
    float grainSizeMs;        // 20-500
    float grainSpreadSec;     // 0.1-4
    bool grainRateSync;
    int grainNoteDivision;
    int grainRateMultiplier;
    float generativeMix;      // 0-1
    float chorusRate;         // 0.05-2.5 Hz
    float chorusDepth;        // 0-1
    float chorusMix;          // 0-1
    bool rotaryFast;
    float rotaryMix;          // 0-1
    float delayTimeMs;        // 1-2000
    bool delaySync;
    int delayNoteDivision;
    int delayTimeMultiplier;
    float delayFeedback;      // 0-0.95
    float delayMix;           // 0-1
    float tapeAmount;         // 0-100
    float outputGainDb;       // -24..12
    float manualBpm;          // 40-300
};

struct FactoryPreset
{
    const char* name;
    FactoryPresetValues values;
};

const std::vector<FactoryPreset>& getFactoryPresets();
void applyFactoryPreset (juce::AudioProcessorValueTreeState& apvts, const FactoryPresetValues& values);
