#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/BDCLookAndFeel.h"

// Custom editor: a flat, neutral, editorial layout - wordmark + scale/root
// readouts up top, four tall "bar" sliders (Grain/Delay/Chorus/Rotary) as
// the hero controls, and a compact strip of secondary knobs below so every
// parameter stays reachable without cluttering the main view.
class BDCPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit BDCPluginAudioProcessorEditor (BDCPluginAudioProcessor&);
    ~BDCPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct HeroBar
    {
        juce::Label header;
        juce::Label valueLabel; // live numeric readout, shown below the name
        juce::Slider bar { juce::Slider::LinearBarVertical, juce::Slider::NoTextBox };
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Knob
    {
        juce::Label caption;
        juce::Slider dial { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
        std::unique_ptr<SliderAttachment> attachment;
        juce::Label valueLabel; // live numeric readout, shown below the caption
    };

    struct SyncGroup
    {
        juce::TextButton syncButton { "SYNC" };
        std::unique_ptr<ButtonAttachment> syncAttachment;
        juce::ComboBox divisionBox;
        std::unique_ptr<ComboBoxAttachment> divisionAttachment;
        juce::ComboBox multiplierBox;
        std::unique_ptr<ComboBoxAttachment> multiplierAttachment;
    };

    void setupHeroBar (HeroBar&, const juce::String& labelText, const juce::String& paramID, const juce::String& tooltip);
    void setupKnob (Knob&, const juce::String& labelText, const juce::String& paramID, const juce::String& tooltip);
    void setupSyncGroup (SyncGroup&, const juce::String& syncParamID, const juce::String& divisionParamID,
                          const juce::String& multiplierParamID, const juce::String& tooltip);

    // The Character macro knob doesn't own a parameter itself - it's a
    // one-shot gesture that pushes a curated combination of values into the
    // four grain params below it, which stay independently automatable and
    // hand-tunable afterwards (moving one won't snap Character back to
    // whatever position would "match" it).
    void applyCharacterMacro (float t01);

    BDCPluginAudioProcessor& processorRef;
    BDCLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 400 };

    juce::Label logoLabel;

    juce::Label presetCaption;
    juce::ComboBox presetBox;

    juce::Label tunerLabel;

    juce::Label scaleCaption, rootCaption;
    juce::ComboBox scaleBox, rootBox;
    std::unique_ptr<ComboBoxAttachment> scaleAttachment, rootAttachment;

    juce::TextButton keyFollowButton { "AUTO" };
    std::unique_ptr<ButtonAttachment> keyFollowAttachment;
    bool wasKeyFollowOn = false;

    HeroBar grainBar, delayBar, chorusBar, rotaryBar;

    Knob grainDensityKnob, grainSizeKnob, grainSpreadKnob, unpredictabilityKnob;
    Knob characterKnob;
    Knob delayTimeKnob, delayFeedbackKnob, delayTapsKnob, delayTapSpreadKnob;
    Knob chorusRateKnob, chorusDepthKnob;

    juce::Label rotaryFastLabel;
    juce::TextButton rotaryFastButton { "FAST" };
    std::unique_ptr<ButtonAttachment> rotaryFastAttachment;

    SyncGroup grainSync, delaySync;
    Knob manualBpmKnob;

    juce::Label outputGainCaption;
    juce::Slider outputGainSlider { juce::Slider::LinearBar, juce::Slider::NoTextBox };
    std::unique_ptr<SliderAttachment> outputGainAttachment;

    Knob tapeKnob;
    Knob masterMixKnob;

    juce::TextButton sustainButton { "SUSTAIN" };
    std::unique_ptr<ButtonAttachment> sustainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BDCPluginAudioProcessorEditor)
};
