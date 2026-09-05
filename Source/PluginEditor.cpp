#include "PluginEditor.h"

BDCPluginAudioProcessorEditor::BDCPluginAudioProcessorEditor (BDCPluginAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    logoLabel.setText ("BDC", juce::dontSendNotification);
    logoLabel.setFont (juce::Font (34.0f));
    logoLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (logoLabel);

    scaleCaption.setText ("SCALE:", juce::dontSendNotification);
    scaleCaption.setFont (juce::Font (12.0f));
    addAndMakeVisible (scaleCaption);
    scaleBox.addItemList ({ "Major", "Natural Minor", "Dorian", "Major Pentatonic", "Minor Pentatonic" }, 1);
    scaleBox.setTooltip ("The scale the generative melody stays inside.");
    addAndMakeVisible (scaleBox);
    scaleAttachment = std::make_unique<ComboBoxAttachment> (processorRef.apvts, "scaleType", scaleBox);

    rootCaption.setText ("ROOT:", juce::dontSendNotification);
    rootCaption.setFont (juce::Font (12.0f));
    addAndMakeVisible (rootCaption);
    rootBox.addItemList ({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    rootBox.setTooltip ("The key the generative melody stays inside.");
    addAndMakeVisible (rootBox);
    rootAttachment = std::make_unique<ComboBoxAttachment> (processorRef.apvts, "rootNote", rootBox);

    setupHeroBar (grainBar,  "GRAIN",  "generativeMix",
        "How much of the self-generated melody is mixed in with your dry signal.");
    setupHeroBar (delayBar,  "DELAY",  "delayMix",
        "How much of the delay's repeats is mixed in.");
    setupHeroBar (chorusBar, "CHORUS", "chorusMix",
        "How much of the chorus effect is mixed in.");
    setupHeroBar (rotaryBar, "ROTARY", "rotaryMix",
        "How much of the rotary (Leslie speaker) effect is mixed in.");

    setupKnob (grainDensityKnob, "DENSITY", "grainDensity",
        "How many new notes the generator plays per second. Higher = busier and more granular, lower = sparser and more spacious.");
    setupKnob (grainSizeKnob, "SIZE", "grainSizeMs",
        "How long each generated note lasts. Shorter = choppier/glitchier, longer = smoother and more sustained.");
    setupKnob (grainSpreadKnob, "SPREAD", "grainSpreadSec",
        "How far back in time the generator is allowed to pull material from. Higher = it draws on a longer memory of what you played.");
    setupKnob (unpredictabilityKnob, "CHAOS", "unpredictability",
        "How often the generated melody takes a big jump instead of moving stepwise. Higher = more unpredictable and adventurous.");

    setupKnob (delayTimeKnob, "TIME", "delayTimeMs",
        "Time between echoes, in milliseconds.");
    setupKnob (delayFeedbackKnob, "FEEDBACK", "delayFeedback",
        "How much of each echo feeds back into the next one. Higher = repeats for longer.");

    setupKnob (chorusRateKnob, "RATE", "chorusRate",
        "Speed of the chorus effect's modulation.");
    setupKnob (chorusDepthKnob, "DEPTH", "chorusDepth",
        "Intensity of the chorus effect's modulation.");

    rotaryFastLabel.setText ("SPEED", juce::dontSendNotification);
    rotaryFastLabel.setJustificationType (juce::Justification::centred);
    rotaryFastLabel.setFont (juce::Font (10.0f));
    addAndMakeVisible (rotaryFastLabel);

    rotaryFastButton.setClickingTogglesState (true);
    rotaryFastButton.setTooltip ("Toggles the rotary speaker between slow (chorale) and fast (tremolo) speed, ramping between them like a real Leslie motor.");
    addAndMakeVisible (rotaryFastButton);
    rotaryFastAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "rotaryFast", rotaryFastButton);

    outputGainCaption.setText ("OUTPUT", juce::dontSendNotification);
    outputGainCaption.setFont (juce::Font (12.0f));
    addAndMakeVisible (outputGainCaption);
    outputGainSlider.setTooltip ("Overall output level trim, in dB.");
    addAndMakeVisible (outputGainSlider);
    outputGainAttachment = std::make_unique<SliderAttachment> (processorRef.apvts, "outputGainDb", outputGainSlider);

    sustainButton.setClickingTogglesState (true);
    sustainButton.setTooltip ("When on, the generator keeps evolving off your last captured audio during silence instead of fading out. Generation never starts until you've actually played something in, either way.");
    addAndMakeVisible (sustainButton);
    sustainAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "sustainOnSilence", sustainButton);

    setSize (860, 620);
}

BDCPluginAudioProcessorEditor::~BDCPluginAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void BDCPluginAudioProcessorEditor::setupHeroBar (HeroBar& hb, const juce::String& labelText,
                                                   const juce::String& paramID, const juce::String& tooltip)
{
    hb.header.setText (labelText, juce::dontSendNotification);
    hb.header.setJustificationType (juce::Justification::centred);
    hb.header.setFont (juce::Font (15.0f));
    addAndMakeVisible (hb.header);

    hb.bar.setTooltip (tooltip);
    addAndMakeVisible (hb.bar);
    hb.attachment = std::make_unique<SliderAttachment> (processorRef.apvts, paramID, hb.bar);
}

void BDCPluginAudioProcessorEditor::setupKnob (Knob& k, const juce::String& labelText,
                                                const juce::String& paramID, const juce::String& tooltip)
{
    k.caption.setText (labelText, juce::dontSendNotification);
    k.caption.setJustificationType (juce::Justification::centred);
    k.caption.setFont (juce::Font (10.0f));
    addAndMakeVisible (k.caption);

    k.dial.setTooltip (tooltip);
    addAndMakeVisible (k.dial);
    k.attachment = std::make_unique<SliderAttachment> (processorRef.apvts, paramID, k.dial);
}

void BDCPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BDCLookAndFeel::background);
}

void BDCPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (24);

    auto header = area.removeFromTop (56);
    logoLabel.setBounds (header.removeFromLeft (180));

    auto controls = header.removeFromRight (300);
    auto scaleRow = controls.removeFromLeft (150);
    scaleCaption.setBounds (scaleRow.removeFromLeft (56));
    scaleBox.setBounds (scaleRow);
    auto rootRow = controls;
    rootCaption.setBounds (rootRow.removeFromLeft (50));
    rootBox.setBounds (rootRow);

    area.removeFromTop (20);

    auto footer = area.removeFromBottom (44);
    sustainButton.setBounds (footer.removeFromRight (140));
    footer.removeFromRight (12);
    outputGainCaption.setBounds (footer.removeFromLeft (90));
    outputGainSlider.setBounds (footer);

    area.removeFromBottom (16);

    auto detail = area.removeFromBottom (120);
    area.removeFromBottom (20);

    auto heroArea = area;
    const int numCols = 4;
    const int gap = 20;
    const int colWidth = (heroArea.getWidth() - gap * (numCols - 1)) / numCols;

    auto layoutHero = [] (HeroBar& hb, juce::Rectangle<int> col)
    {
        hb.header.setBounds (col.removeFromTop (26));
        col.removeFromTop (8);
        auto barArea = col.withSizeKeepingCentre (juce::jmin (col.getWidth(), 90), col.getHeight());
        hb.bar.setBounds (barArea);
    };

    auto col1 = heroArea.removeFromLeft (colWidth); heroArea.removeFromLeft (gap);
    auto col2 = heroArea.removeFromLeft (colWidth); heroArea.removeFromLeft (gap);
    auto col3 = heroArea.removeFromLeft (colWidth); heroArea.removeFromLeft (gap);
    auto col4 = heroArea;

    layoutHero (grainBar, col1);
    layoutHero (delayBar, col2);
    layoutHero (chorusBar, col3);
    layoutHero (rotaryBar, col4);

    auto d1 = detail.removeFromLeft (colWidth); detail.removeFromLeft (gap);
    auto d2 = detail.removeFromLeft (colWidth); detail.removeFromLeft (gap);
    auto d3 = detail.removeFromLeft (colWidth); detail.removeFromLeft (gap);
    auto d4 = detail;

    auto layoutKnob = [] (Knob& k, juce::Rectangle<int> slot)
    {
        k.caption.setBounds (slot.removeFromBottom (16));
        k.dial.setBounds (slot);
    };

    {
        const int n = 4;
        const int w = d1.getWidth() / n;
        Knob* knobs[] { &grainDensityKnob, &grainSizeKnob, &grainSpreadKnob, &unpredictabilityKnob };
        for (int i = 0; i < n; ++i)
            layoutKnob (*knobs[i], d1.removeFromLeft (w));
    }
    {
        const int w = d2.getWidth() / 2;
        layoutKnob (delayTimeKnob, d2.removeFromLeft (w));
        layoutKnob (delayFeedbackKnob, d2);
    }
    {
        const int w = d3.getWidth() / 2;
        layoutKnob (chorusRateKnob, d3.removeFromLeft (w));
        layoutKnob (chorusDepthKnob, d3);
    }
    {
        rotaryFastLabel.setBounds (d4.removeFromBottom (16));
        rotaryFastButton.setBounds (d4.withSizeKeepingCentre (juce::jmin (d4.getWidth(), 100), 32));
    }
}
