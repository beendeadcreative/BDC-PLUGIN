#include "PluginEditor.h"

namespace
{
    // Reference size the whole layout is designed at; resized() scales
    // every dimension (including fonts) by getWidth() / kDesignWidth, so
    // resizing the window scales the graphics rather than just reflowing.
    constexpr int kDesignWidth = 860;
    constexpr int kDesignHeight = 620;

    constexpr float kLogoFontSize = 34.0f;
    constexpr float kTunerFontSize = 22.0f;
    constexpr float kCaptionFontSize = 12.0f;
    constexpr float kHeroHeaderFontSize = 15.0f;
    constexpr float kHeroValueFontSize = 11.0f;
    constexpr float kKnobCaptionFontSize = 10.0f;
    constexpr float kKnobValueFontSize = 9.0f;
    constexpr float kRotaryLabelFontSize = 10.0f;

    // A continuous, multi-bend "fold" line spanning the canvas (a bit past
    // each edge so nothing looks abruptly cut off) - the backbone for one
    // liquid-chrome ripple.
    juce::Path buildFoldPath (juce::Random& rng, float w, float h, int segments)
    {
        juce::Path path;

        const float x0 = -0.12f * w;
        const float xEnd = 1.12f * w;
        const float segW = (xEnd - x0) / (float) segments;

        float prevX = x0;
        float prevY = rng.nextFloat() * h;
        path.startNewSubPath (prevX, prevY);

        for (int s = 0; s < segments; ++s)
        {
            float targetX = x0 + segW * (float) (s + 1);
            float targetY = rng.nextFloat() * h;
            float c1x = prevX + segW * 0.33f;
            float c1y = prevY + (rng.nextFloat() - 0.5f) * h * 0.6f;
            float c2x = targetX - segW * 0.33f;
            float c2y = targetY + (rng.nextFloat() - 0.5f) * h * 0.6f;

            path.cubicTo (c1x, c1y, c2x, c2y, targetX, targetY);

            prevX = targetX;
            prevY = targetY;
        }

        return path;
    }
}

BDCPluginAudioProcessorEditor::BDCPluginAudioProcessorEditor (BDCPluginAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    logoLabel.setText ("BDC", juce::dontSendNotification);
    logoLabel.setFont (juce::Font (34.0f));
    logoLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (logoLabel);

    presetCaption.setText ("PRESET:", juce::dontSendNotification);
    presetCaption.setFont (juce::Font (12.0f));
    addAndMakeVisible (presetCaption);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        presetBox.addItem (processorRef.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (processorRef.getCurrentProgram(), juce::dontSendNotification);
    presetBox.setTooltip ("Factory presets - a starting point to tweak from.");
    presetBox.onChange = [this]
    {
        auto index = presetBox.getSelectedItemIndex();
        if (index >= 0)
            processorRef.setCurrentProgram (index);
    };
    addAndMakeVisible (presetBox);

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

    keyFollowButton.setClickingTogglesState (true);
    keyFollowButton.setTooltip ("When on, the generative engine follows the key of what you're playing automatically instead of the Scale/Root pickers above (which stay put, ready for when you turn this off).");
    addAndMakeVisible (keyFollowButton);
    keyFollowAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "keyFollow", keyFollowButton);

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

    setupKnob (masterMixKnob, "MIX", "masterMix",
        "Overall dry/wet for everything combined - Grain, Chorus, Rotary, Delay, and Tape together. 0% is your untouched input, 100% is the fully processed signal (each effect's own mix still shapes how much of it there is within that 100%).");

    sustainButton.setClickingTogglesState (true);
    sustainButton.setTooltip ("When on, the generator keeps evolving off your last captured audio during silence instead of fading out. Generation never starts until you've actually played something in, either way.");
    addAndMakeVisible (sustainButton);
    sustainAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "sustainOnSilence", sustainButton);

    setSize (kDesignWidth, kDesignHeight);

    setResizable (true, true);
    setResizeLimits (560, 400, 1720, 1240);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio ((double) kDesignWidth / (double) kDesignHeight);

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

    // Keep the preset box in sync if the program changes from outside the
    // combo box itself (e.g. the host's own preset menu, or session reload).
    if (presetBox.getSelectedItemIndex() != processorRef.getCurrentProgram())
        presetBox.setSelectedItemIndex (processorRef.getCurrentProgram(), juce::dontSendNotification);

    // While Key Follow is on, the Scale/Root pickers aren't driving the
    // sound - disable them and show the live tracked key instead. When it
    // turns back off, hand control back and resync the display to the
    // actual manual parameter values.
    const bool keyFollowOn = keyFollowButton.getToggleState();
    if (keyFollowOn)
    {
        static const char* rootNames[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        static const char* scaleNames[] { "Major", "Natural Minor", "Dorian", "Major Pentatonic", "Minor Pentatonic" };

        rootBox.setEnabled (false);
        scaleBox.setEnabled (false);
        rootBox.setText (rootNames[processorRef.getTrackedRootPitchClass()], juce::dontSendNotification);
        scaleBox.setText (scaleNames[processorRef.getTrackedScaleType()], juce::dontSendNotification);
    }
    else if (wasKeyFollowOn)
    {
        // Hand control back and resync the display to the actual manual
        // parameter values (the attachment didn't repaint while we were
        // overriding the text, since the parameter itself never changed).
        rootBox.setEnabled (true);
        scaleBox.setEnabled (true);
        rootBox.setSelectedItemIndex (processorRef.getManualRootIndex(), juce::dontSendNotification);
        scaleBox.setSelectedItemIndex (processorRef.getManualScaleIndex(), juce::dontSendNotification);
    }
    wasKeyFollowOn = keyFollowOn;
}

void BDCPluginAudioProcessorEditor::setupHeroBar (HeroBar& hb, const juce::String& labelText,
                                                   const juce::String& paramID, const juce::String& tooltip)
{
    hb.header.setText (labelText, juce::dontSendNotification);
    hb.header.setJustificationType (juce::Justification::centred);
    hb.header.setFont (juce::Font (15.0f));
    addAndMakeVisible (hb.header);

    hb.valueLabel.setJustificationType (juce::Justification::centred);
    hb.valueLabel.setFont (juce::Font (11.0f));
    addAndMakeVisible (hb.valueLabel);

    hb.bar.setTooltip (tooltip);
    addAndMakeVisible (hb.bar);
    hb.attachment = std::make_unique<SliderAttachment> (processorRef.apvts, paramID, hb.bar);

    if (auto* param = processorRef.apvts.getParameter (paramID))
    {
        juce::Label* label = &hb.valueLabel;
        auto updateText = [label, param] { label->setText (param->getCurrentValueAsText(), juce::dontSendNotification); };
        hb.bar.onValueChange = updateText;
        updateText();
    }
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
    if (backgroundTexture.isValid())
        g.drawImageAt (backgroundTexture, 0, 0);
    else
        g.fillAll (BDCLookAndFeel::background);
}

void BDCPluginAudioProcessorEditor::renderBackgroundTexture()
{
    const int w = juce::jmax (1, getWidth());
    const int h = juce::jmax (1, getHeight());
    const float fw = (float) w, fh = (float) h;

    backgroundTexture = juce::Image (juce::Image::RGB, w, h, true);
    juce::Graphics g (backgroundTexture);

    // Deliberately more saturated/contrasty than the UI's own ink/text/
    // background colours - this is meant to read as a glossy poured-metal
    // surface, not a flat UI panel.
    const juce::Colour baseLight     { 0xfff6cfe8 };
    const juce::Colour baseDark      { 0xffd748ab };
    const juce::Colour foldShadow    { 0xff6e1a4c };
    const juce::Colour foldMid       { 0xffe154a6 };
    const juce::Colour foldHighlight { 0xfffef4fa };

    g.setGradientFill (juce::ColourGradient (baseLight, 0.0f, 0.0f, baseDark, fw, fh, false));
    g.fillRect (0, 0, w, h);

    // Fixed seed: the pattern looks the same (just rescaled) every time,
    // rather than reshuffling on every resize.
    juce::Random rng (42);

    // Each "fold" is one wavy line stroked three times at decreasing width
    // (dark shadow -> saturated mid-tone -> bright offset highlight), like
    // a lit, rounded ridge of poured chrome. Later folds are drawn on top,
    // so they occlude earlier ones the way real overlapping folds would.
    const int numFolds = 10;
    for (int i = 0; i < numFolds; ++i)
    {
        auto fold = buildFoldPath (rng, fw, fh, 2 + rng.nextInt (2));

        float bandScale = juce::jmap (rng.nextFloat(), 0.7f, 1.4f);
        float shadowWidth = fh * 0.10f * bandScale;
        float midWidth = fh * 0.052f * bandScale;
        float highlightWidth = fh * 0.014f * bandScale;

        g.setColour (foldShadow.withAlpha (0.6f));
        g.strokePath (fold, juce::PathStrokeType (shadowWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (foldMid.withAlpha (0.8f));
        g.strokePath (fold, juce::PathStrokeType (midWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // The highlight traces the same curve but offset toward the "lit"
        // side rather than sitting dead-centre in the fold.
        auto highlightPath = fold;
        highlightPath.applyTransform (juce::AffineTransform::translation (
            -shadowWidth * 0.22f, -shadowWidth * 0.22f));
        g.setColour (foldHighlight.withAlpha (0.7f));
        g.strokePath (highlightPath, juce::PathStrokeType (highlightWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void BDCPluginAudioProcessorEditor::resized()
{
    renderBackgroundTexture();

    // Everything below scales off this ratio, so resizing the window scales
    // the whole UI (fonts included) rather than just reflowing whitespace.
    const float scale = (float) getWidth() / (float) kDesignWidth;
    auto S  = [scale] (int v)   { return juce::roundToInt ((float) v * scale); };
    auto SF = [scale] (float v) { return juce::Font (v * scale); };

    logoLabel.setFont (SF (kLogoFontSize));
    presetCaption.setFont (SF (kCaptionFontSize));
    tunerLabel.setFont (SF (kTunerFontSize));
    scaleCaption.setFont (SF (kCaptionFontSize));
    rootCaption.setFont (SF (kCaptionFontSize));
    outputGainCaption.setFont (SF (kCaptionFontSize));
    rotaryFastLabel.setFont (SF (kRotaryLabelFontSize));

    for (auto* hb : { &grainBar, &delayBar, &chorusBar, &rotaryBar })
    {
        hb->header.setFont (SF (kHeroHeaderFontSize));
        hb->valueLabel.setFont (SF (kHeroValueFontSize));
    }

    for (auto* k : { &grainDensityKnob, &grainSizeKnob, &grainSpreadKnob, &unpredictabilityKnob,
                      &delayTimeKnob, &delayFeedbackKnob, &chorusRateKnob, &chorusDepthKnob,
                      &manualBpmKnob, &tapeKnob, &masterMixKnob })
    {
        k->caption.setFont (SF (kKnobCaptionFontSize));
        k->valueLabel.setFont (SF (kKnobValueFontSize));
    }

    auto area = getLocalBounds().reduced (S (24));

    auto header = area.removeFromTop (S (40));
    logoLabel.setBounds (header.removeFromLeft (S (180)));

    auto controls = header.removeFromRight (S (362));
    keyFollowButton.setBounds (controls.removeFromRight (S (64)).withSizeKeepingCentre (S (58), S (24)));
    controls.removeFromRight (S (6));
    auto scaleRow = controls.removeFromLeft (S (148));
    scaleCaption.setBounds (scaleRow.removeFromLeft (S (56)));
    scaleBox.setBounds (scaleRow);
    auto rootRow = controls;
    rootCaption.setBounds (rootRow.removeFromLeft (S (50)));
    rootBox.setBounds (rootRow);

    tunerLabel.setBounds (header);

    area.removeFromTop (S (10));

    auto presetRow = area.removeFromTop (S (26));
    presetCaption.setBounds (presetRow.removeFromLeft (S (60)));
    presetBox.setBounds (presetRow.removeFromLeft (S (240)));

    area.removeFromTop (S (14));

    auto footer = area.removeFromBottom (S (78));
    sustainButton.setBounds (footer.removeFromRight (S (140)).withSizeKeepingCentre (S (140), S (32)));
    footer.removeFromRight (S (16));

    auto tapeSlot = footer.removeFromRight (S (72));
    tapeKnob.valueLabel.setBounds (tapeSlot.removeFromBottom (S (13)));
    tapeKnob.caption.setBounds (tapeSlot.removeFromBottom (S (16)));
    tapeKnob.dial.setBounds (tapeSlot);
    footer.removeFromRight (S (16));

    auto mixSlot = footer.removeFromRight (S (72));
    masterMixKnob.valueLabel.setBounds (mixSlot.removeFromBottom (S (13)));
    masterMixKnob.caption.setBounds (mixSlot.removeFromBottom (S (16)));
    masterMixKnob.dial.setBounds (mixSlot);
    footer.removeFromRight (S (16));

    outputGainCaption.setBounds (footer.removeFromLeft (S (90)).withSizeKeepingCentre (S (90), S (24)));
    outputGainSlider.setBounds (footer.withSizeKeepingCentre (footer.getWidth(), S (24)));

    area.removeFromBottom (S (16));

    auto detail = area.removeFromBottom (S (120));
    area.removeFromBottom (S (12));

    auto syncStrip = area.removeFromBottom (S (58));
    area.removeFromBottom (S (16));

    auto heroArea = area;
    const int numCols = 4;
    const int gap = S (20);
    const int colWidth = (heroArea.getWidth() - gap * (numCols - 1)) / numCols;

    auto layoutHero = [S] (HeroBar& hb, juce::Rectangle<int> col)
    {
        hb.header.setBounds (col.removeFromTop (S (26)));
        hb.valueLabel.setBounds (col.removeFromTop (S (16)));
        col.removeFromTop (S (6));
        auto barArea = col.withSizeKeepingCentre (juce::jmin (col.getWidth(), S (90)), col.getHeight());
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

    auto layoutKnob = [S] (Knob& k, juce::Rectangle<int> slot)
    {
        k.valueLabel.setBounds (slot.removeFromBottom (S (13)));
        k.caption.setBounds (slot.removeFromBottom (S (16)));
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
        rotaryFastLabel.setBounds (d4.removeFromBottom (S (16)));
        rotaryFastButton.setBounds (d4.withSizeKeepingCentre (juce::jmin (d4.getWidth(), S (100)), S (32)));
    }

    // Sync strip: SYNC + note division + x/÷ multiplier for Grain and
    // Delay (matching their hero-bar columns); a manual BPM fallback knob
    // sits under Rotary's column, where there's otherwise nothing to sync.
    auto layoutSync = [S] (SyncGroup& s, juce::Rectangle<int> slot)
    {
        auto row = slot.withSizeKeepingCentre (slot.getWidth(), S (28));
        s.syncButton.setBounds (row.removeFromLeft (S (50)));
        row.removeFromLeft (S (4));
        s.multiplierBox.setBounds (row.removeFromRight (S (50)));
        row.removeFromRight (S (4));
        s.divisionBox.setBounds (row);
    };

    auto sy1 = syncStrip.removeFromLeft (colWidth); syncStrip.removeFromLeft (gap);
    auto sy2 = syncStrip.removeFromLeft (colWidth); syncStrip.removeFromLeft (gap);
    auto sy3 = syncStrip.removeFromLeft (colWidth); syncStrip.removeFromLeft (gap);
    auto sy4 = syncStrip;
    juce::ignoreUnused (sy3);

    layoutSync (grainSync, sy1);
    layoutSync (delaySync, sy2);

    manualBpmKnob.valueLabel.setBounds (sy4.removeFromBottom (S (13)));
    manualBpmKnob.caption.setBounds (sy4.removeFromBottom (S (16)));
    manualBpmKnob.dial.setBounds (sy4.withSizeKeepingCentre (juce::jmin (sy4.getWidth(), S (44)), sy4.getHeight()));
}
