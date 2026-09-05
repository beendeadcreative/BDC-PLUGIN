#include "PluginEditor.h"

BDCPluginAudioProcessorEditor::BDCPluginAudioProcessorEditor (BDCPluginAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    logoLabel.setText ("BDC", juce::dontSendNotification);
    logoLabel.setFont (juce::Font (34.0f));
    logoLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (logoLabel);

    tunerLabel.setText ("--", juce::dontSendNotification);
    tunerLabel.setFont (juce::Font (22.0f));
    tunerLabel.setJustificationType (juce::Justification::centred);
    tunerLabel.setTooltip ("Live tuner: shows the nearest note to what you're playing and how many cents sharp (+) or flat (-) you are.");
    addAndMakeVisible (tunerLabel);

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

    setupSyncGroup (grainSync, "grainRateSync", "grainNoteDivision", "grainRateMultiplier",
        "Locks the generator's note rate to the song tempo instead of the free Density knob. Pick a note value and optionally halve/quarter or double/quadruple it.");
    setupSyncGroup (delaySync, "delaySync", "delayNoteDivision", "delayTimeMultiplier",
        "Locks the delay time to the song tempo instead of the free Time knob. Pick a note value and optionally halve/quarter or double/quadruple it.");

    setupKnob (manualBpmKnob, "BPM", "manualBpm",
        "Manual tempo used for Sync when no host tempo is available (e.g. running Standalone with nothing playing).");

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

    setupKnob (tapeKnob, "TAPE", "tapeAmount",
        "Runs the whole mix through emulated cassette 4-track character (Tascam Porta 02 MkII vibe): pitch wobble, dulled top end, saturation, and tape hiss. 0% is clean, 100% is fully lo-fi.");

    sustainButton.setClickingTogglesState (true);
    sustainButton.setTooltip ("When on, the generator keeps evolving off your last captured audio during silence instead of fading out. Generation never starts until you've actually played something in, either way.");
    addAndMakeVisible (sustainButton);
    sustainAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "sustainOnSilence", sustainButton);

    setSize (860, 620);
    startTimerHz (20);
}

BDCPluginAudioProcessorEditor::~BDCPluginAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BDCPluginAudioProcessorEditor::timerCallback()
{
    if (processorRef.isPitchDetected())
    {
        auto note = PitchDetector::frequencyToNote (processorRef.getDetectedFrequencyHz());
        juce::String centsText = (note.cents > 0 ? "+" : "") + juce::String (note.cents) + "c";
        tunerLabel.setText (note.name + "   " + centsText, juce::dontSendNotification);
        tunerLabel.setColour (juce::Label::textColourId,
                               std::abs (note.cents) <= 5 ? BDCLookAndFeel::ink : BDCLookAndFeel::text);
    }
    else
    {
        tunerLabel.setText ("--", juce::dontSendNotification);
        tunerLabel.setColour (juce::Label::textColourId, BDCLookAndFeel::text);
    }
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

    k.valueLabel.setJustificationType (juce::Justification::centred);
    k.valueLabel.setFont (juce::Font (9.0f));
    addAndMakeVisible (k.valueLabel);

    k.dial.setTooltip (tooltip);
    addAndMakeVisible (k.dial);
    k.attachment = std::make_unique<SliderAttachment> (processorRef.apvts, paramID, k.dial);

    // Keep the value readout in sync with drags AND host/automation-driven
    // changes (the APVTS attachment updates the slider with a notifying
    // setValue(), so onValueChange fires either way).
    if (auto* param = processorRef.apvts.getParameter (paramID))
    {
        juce::Label* label = &k.valueLabel;
        auto updateText = [label, param] { label->setText (param->getCurrentValueAsText(), juce::dontSendNotification); };
        k.dial.onValueChange = updateText;
        updateText();
    }
}

void BDCPluginAudioProcessorEditor::setupSyncGroup (SyncGroup& s, const juce::String& syncParamID,
                                                     const juce::String& divisionParamID,
                                                     const juce::String& multiplierParamID,
                                                     const juce::String& tooltip)
{
    s.syncButton.setClickingTogglesState (true);
    s.syncButton.setTooltip (tooltip);
    addAndMakeVisible (s.syncButton);
    s.syncAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, syncParamID, s.syncButton);

    s.divisionBox.addItemList ({ "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4.", "1/8.", "1/16.", "1/4T", "1/8T", "1/16T" }, 1);
    s.divisionBox.setTooltip ("Note value to sync to, when SYNC is on.");
    addAndMakeVisible (s.divisionBox);
    s.divisionAttachment = std::make_unique<ComboBoxAttachment> (processorRef.apvts, divisionParamID, s.divisionBox);

    s.multiplierBox.addItemList ({ "/4", "/2", "x1", "x2", "x4" }, 1);
    s.multiplierBox.setTooltip ("Divides or multiplies the synced note value - e.g. \"1/4\" + \"x4\" gives a four-bar-long time.");
    addAndMakeVisible (s.multiplierBox);
    s.multiplierAttachment = std::make_unique<ComboBoxAttachment> (processorRef.apvts, multiplierParamID, s.multiplierBox);
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

    tunerLabel.setBounds (header);

    area.removeFromTop (20);

    auto footer = area.removeFromBottom (78);
    sustainButton.setBounds (footer.removeFromRight (140).withSizeKeepingCentre (140, 32));
    footer.removeFromRight (16);

    auto tapeSlot = footer.removeFromRight (72);
    tapeKnob.valueLabel.setBounds (tapeSlot.removeFromBottom (13));
    tapeKnob.caption.setBounds (tapeSlot.removeFromBottom (16));
    tapeKnob.dial.setBounds (tapeSlot);
    footer.removeFromRight (16);

    outputGainCaption.setBounds (footer.removeFromLeft (90).withSizeKeepingCentre (90, 24));
    outputGainSlider.setBounds (footer.withSizeKeepingCentre (footer.getWidth(), 24));

    area.removeFromBottom (16);

    auto detail = area.removeFromBottom (120);
    area.removeFromBottom (12);

    auto syncStrip = area.removeFromBottom (58);
    area.removeFromBottom (16);

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
        k.valueLabel.setBounds (slot.removeFromBottom (13));
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

    // Sync strip: SYNC + note division + x/÷ multiplier for Grain and
    // Delay (matching their hero-bar columns); a manual BPM fallback knob
    // sits under Rotary's column, where there's otherwise nothing to sync.
    auto layoutSync = [] (SyncGroup& s, juce::Rectangle<int> slot)
    {
        auto row = slot.withSizeKeepingCentre (slot.getWidth(), 28);
        s.syncButton.setBounds (row.removeFromLeft (50));
        row.removeFromLeft (4);
        s.multiplierBox.setBounds (row.removeFromRight (50));
        row.removeFromRight (4);
        s.divisionBox.setBounds (row);
    };

    auto sy1 = syncStrip.removeFromLeft (colWidth); syncStrip.removeFromLeft (gap);
    auto sy2 = syncStrip.removeFromLeft (colWidth); syncStrip.removeFromLeft (gap);
    auto sy3 = syncStrip.removeFromLeft (colWidth); syncStrip.removeFromLeft (gap);
    auto sy4 = syncStrip;
    juce::ignoreUnused (sy3);

    layoutSync (grainSync, sy1);
    layoutSync (delaySync, sy2);

    manualBpmKnob.valueLabel.setBounds (sy4.removeFromBottom (13));
    manualBpmKnob.caption.setBounds (sy4.removeFromBottom (16));
    manualBpmKnob.dial.setBounds (sy4.withSizeKeepingCentre (juce::jmin (sy4.getWidth(), 44), sy4.getHeight()));
}
