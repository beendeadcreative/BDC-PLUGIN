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

        void setVisible (bool v)
        {
            caption.setVisible (v);
            dial.setVisible (v);
            valueLabel.setVisible (v);
        }
    };

    struct SyncGroup
    {
        juce::TextButton syncButton { "SYNC" };
        std::unique_ptr<ButtonAttachment> syncAttachment;
        juce::ComboBox divisionBox;
        std::unique_ptr<ComboBoxAttachment> divisionAttachment;
        juce::ComboBox multiplierBox;
        std::unique_ptr<ComboBoxAttachment> multiplierAttachment;

        void setVisible (bool v)
        {
            syncButton.setVisible (v);
            divisionBox.setVisible (v);
            multiplierBox.setVisible (v);
        }
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

    // Jumbles the generator + effect tone knobs within curated ranges (not
    // their full extremes) for a quick "happy accident" starting point.
    // Doesn't touch Scale/Root/Key Follow, sync settings, or level-critical
    // params (Mix, Output, BPM) - those are context, not tone, and
    // randomizing them would be more disruptive than fun.
    void randomizeSound();

    // Shows/hides the detail-strip and sync-strip knobs (the per-effect
    // fine-tuning controls) and resizes the window to match, so the default
    // view stays lean - the four hero mix bars and footer macros are always
    // visible either way; this only affects the deeper knobs underneath.
    void setAdvancedVisible (bool show);

    BDCPluginAudioProcessor& processorRef;
    BDCLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 400 };

    juce::Label logoLabel;

    juce::Label presetCaption;
    juce::ComboBox presetBox;

    juce::TextButton advancedToggleButton { "SHOW ADVANCED" };
    bool showAdvanced = false;

    juce::TextButton randomizeButton { "RANDOM" };

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
    juce::Label outputGainValueLabel; // live readout, shown below the slider like the other controls

    Knob tapeKnob;
    Knob masterMixKnob;
    Knob glueKnob;

    juce::TextButton sustainButton { "SUSTAIN" };
    std::unique_ptr<ButtonAttachment> sustainAttachment;

    // Grab isn't a saved parameter (see PluginProcessor::setGrabFrozen) so
    // it's wired by hand rather than through an APVTS ButtonAttachment.
    juce::TextButton grabButton { "GRAB" };

    // Live input/output level meters - plain non-interactive bar sliders
    // (same visual style as the hero bars) updated from timerCallback().
    juce::Label inputMeterLabel, outputMeterLabel;
    juce::Slider inputMeter { juce::Slider::LinearBarVertical, juce::Slider::NoTextBox };
    juce::Slider outputMeter { juce::Slider::LinearBarVertical, juce::Slider::NoTextBox };

    // Thin divider lines drawn in paint() to visually separate the four
    // effect columns (Grain/Delay/Chorus/Rotary) and the footer's control
    // clusters (Output / tone macros / Sustain), purely for scanability -
    // none of this affects layout, just where paint() strokes a line.
    // Recomputed every resized() alongside the actual component bounds.
    std::array<int, 3> columnDividerX { 0, 0, 0 };
    int columnDividerTop = 0, columnDividerBottom = 0;
    std::array<int, 2> footerDividerX { 0, 0 };
    int footerDividerTop = 0, footerDividerBottom = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BDCPluginAudioProcessorEditor)
};
