#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Placeholder UI: JUCE's generic parameter editor gives every APVTS
// parameter a slider/box automatically, which is enough to hear and tune
// everything while the DSP settles. Swap this out for a custom GUI later
// without touching PluginProcessor at all.
class BDCPluginAudioProcessorEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit BDCPluginAudioProcessorEditor (BDCPluginAudioProcessor& processor);
    ~BDCPluginAudioProcessorEditor() override = default;
};
