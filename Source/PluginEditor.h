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
        juce::Slider bar { juce::Slider::LinearBarVertical, juce::Slider::NoTextBox };
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Knob
    {
        juce::Label caption;
        juce::Slider dial { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setupHeroBar (HeroBar&, const juce::String& labelText, const juce::String& paramID, const juce::String& tooltip);
    void setupKnob (Knob&, const juce::String& labelText, const juce::String& paramID, const juce::String& tooltip);

    BDCPluginAudioProcessor& processorRef;
    BDCLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 400 };

    juce::Label logoLabel;

    juce::Label tunerLabel;

    juce::Label scaleCaption, rootCaption;
    juce::ComboBox scaleBox, rootBox;
    std::unique_ptr<ComboBoxAttachment> scaleAttachment, rootAttachment;

    HeroBar grainBar, delayBar, chorusBar, rotaryBar;

    Knob grainDensityKnob, grainSizeKnob, grainSpreadKnob, unpredictabilityKnob;
    Knob delayTimeKnob, delayFeedbackKnob;
    Knob chorusRateKnob, chorusDepthKnob;

    juce::Label rotaryFastLabel;
    juce::TextButton rotaryFastButton { "FAST" };
    std::unique_ptr<ButtonAttachment> rotaryFastAttachment;

    juce::Label outputGainCaption;
    juce::Slider outputGainSlider { juce::Slider::LinearBar, juce::Slider::NoTextBox };
    std::unique_ptr<SliderAttachment> outputGainAttachment;

    juce::TextButton sustainButton { "SUSTAIN" };
    std::unique_ptr<ButtonAttachment> sustainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BDCPluginAudioProcessorEditor)
};
