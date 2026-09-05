#include "Presets.h"

namespace
{
    void setParamRaw (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float rawValue)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (rawValue));
    }
}

void applyFactoryPreset (juce::AudioProcessorValueTreeState& apvts, const FactoryPresetValues& v)
{
    setParamRaw (apvts, "sustainOnSilence", v.sustainOnSilence ? 1.0f : 0.0f);
    setParamRaw (apvts, "rootNote", (float) v.rootNote);
    setParamRaw (apvts, "scaleType", (float) v.scaleType);
    setParamRaw (apvts, "unpredictability", v.unpredictability);
    setParamRaw (apvts, "grainDensity", v.grainDensity);
    setParamRaw (apvts, "grainSizeMs", v.grainSizeMs);
    setParamRaw (apvts, "grainSpreadSec", v.grainSpreadSec);
    setParamRaw (apvts, "grainRateSync", v.grainRateSync ? 1.0f : 0.0f);
    setParamRaw (apvts, "grainNoteDivision", (float) v.grainNoteDivision);
    setParamRaw (apvts, "grainRateMultiplier", (float) v.grainRateMultiplier);
    setParamRaw (apvts, "generativeMix", v.generativeMix);
    setParamRaw (apvts, "chorusRate", v.chorusRate);
    setParamRaw (apvts, "chorusDepth", v.chorusDepth);
    setParamRaw (apvts, "chorusMix", v.chorusMix);
    setParamRaw (apvts, "rotaryFast", v.rotaryFast ? 1.0f : 0.0f);
    setParamRaw (apvts, "rotaryMix", v.rotaryMix);
    setParamRaw (apvts, "delayTimeMs", v.delayTimeMs);
    setParamRaw (apvts, "delaySync", v.delaySync ? 1.0f : 0.0f);
    setParamRaw (apvts, "delayNoteDivision", (float) v.delayNoteDivision);
    setParamRaw (apvts, "delayTimeMultiplier", (float) v.delayTimeMultiplier);
    setParamRaw (apvts, "delayFeedback", v.delayFeedback);
    setParamRaw (apvts, "delayMix", v.delayMix);
    setParamRaw (apvts, "tapeAmount", v.tapeAmount);
    setParamRaw (apvts, "outputGainDb", v.outputGainDb);
    setParamRaw (apvts, "manualBpm", v.manualBpm);
}

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        // name                sustain root scale unpred  density size  spread sync  div mult genMix chRate chDepth chMix rFast rMix  dTime  dSync dDiv dMult  dFb   dMix  tape  outGain bpm
        { "Init",              { true,  9,   4,    0.18f,  4.5f,  170.0f, 2.0f, false, 3,  2, 0.5f,  0.6f,  0.3f,  0.25f, false, 0.4f, 350.0f, false, 2, 2, 0.35f, 0.3f, 0.0f,  0.0f, 120.0f } },
        { "Gentle Echoes",     { true,  9,   0,    0.10f,  3.0f,  220.0f, 1.5f, false, 3,  2, 0.25f, 0.4f,  0.15f, 0.15f, false, 0.10f, 400.0f, true,  2, 2, 0.30f, 0.28f, 8.0f, 0.0f, 100.0f } },
        { "Ghost Choir",       { true,  0,   4,    0.12f,  3.5f,  320.0f, 3.2f, false, 2,  2, 0.75f, 0.3f,  0.6f,  0.4f,  false, 0.50f, 520.0f, false, 2, 2, 0.40f, 0.30f, 15.0f, -2.0f, 90.0f } },
        { "Leslie Dreams",     { true,  7,   1,    0.15f,  4.0f,  200.0f, 2.0f, false, 3,  2, 0.4f,  0.5f,  0.25f, 0.2f,  false, 0.80f, 300.0f, false, 2, 2, 0.25f, 0.20f, 5.0f,  0.0f, 110.0f } },
        { "Tape Memory",       { true,  2,   2,    0.20f,  3.0f,  250.0f, 2.5f, false, 3,  2, 0.35f, 0.5f,  0.2f,  0.15f, false, 0.15f, 400.0f, true,  7, 2, 0.45f, 0.35f, 55.0f, -1.0f, 95.0f } },
        { "Glitch Garden",     { true,  9,   4,    0.75f,  16.0f, 60.0f,  1.2f, false, 3,  2, 0.85f, 1.2f,  0.4f,  0.3f,  true,  0.30f, 180.0f, false, 2, 2, 0.50f, 0.35f, 20.0f, -3.0f, 128.0f } },
        { "Ambient Wash",      { true,  4,   3,    0.08f,  1.5f,  480.0f, 4.0f, false, 2,  2, 0.6f,  0.2f,  0.5f,  0.35f, false, 0.45f, 900.0f, false, 2, 2, 0.55f, 0.30f, 10.0f, -2.0f, 70.0f } },
        { "Space Echo Dub",    { true,  9,   4,    0.15f,  2.5f,  180.0f, 1.8f, false, 3,  2, 0.2f,  0.6f,  0.2f,  0.15f, false, 0.20f, 400.0f, true,  2, 2, 0.60f, 0.45f, 35.0f, -2.0f, 100.0f } },
        { "AM Radio Ghost",    { true,  9,   1,    0.20f,  2.0f,  260.0f, 2.2f, false, 3,  2, 0.5f,  0.4f,  0.15f, 0.1f,  false, 0.10f, 350.0f, false, 2, 2, 0.30f, 0.25f, 80.0f, 1.0f, 90.0f } },
        { "Rhythmic Pulse",    { true,  9,   4,    0.30f,  6.0f,  120.0f, 1.0f, true,  4,  2, 0.6f,  0.6f,  0.25f, 0.2f,  false, 0.20f, 300.0f, true,  2, 2, 0.35f, 0.30f, 10.0f, 0.0f, 124.0f } },
    };

    return presets;
}
