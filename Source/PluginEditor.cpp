#include "PluginEditor.h"
#include <BinaryData.h>

BDCPluginAudioProcessorEditor::LogoCoverOverlay::LogoCoverOverlay()
    : image (juce::ImageCache::getFromMemory (BinaryData::BDCLogo_png, BinaryData::BDCLogo_pngSize))
{
}

void BDCPluginAudioProcessorEditor::LogoCoverOverlay::paint (juce::Graphics& g)
{
    g.fillAll (BDCLookAndFeel::background);
    g.drawImage (image, getLocalBounds().toFloat(), juce::RectanglePlacement::centred);
}

void BDCPluginAudioProcessorEditor::LogoCoverOverlay::mouseUp (const juce::MouseEvent&)
{
    setVisible (false);
}

namespace
{
    // Reference size the whole layout is designed at; resized() scales
    // every dimension (including fonts) by getWidth() / kDesignWidth, so
    // resizing the window scales the graphics rather than just reflowing.
    constexpr int kDesignWidth = 860;
    constexpr int kDesignHeightExpanded = 620;
    constexpr int kDesignHeightCompact = 420; // hero bars + footer only, no detail/sync knobs

    constexpr float kTunerFontSize = 22.0f;
    constexpr float kCaptionFontSize = 12.0f;
    constexpr float kHeroHeaderFontSize = 15.0f;
    constexpr float kHeroValueFontSize = 11.0f;
    constexpr float kKnobCaptionFontSize = 10.0f;
    constexpr float kKnobValueFontSize = 9.0f;
    constexpr float kRotaryLabelFontSize = 10.0f;

    // The Character macro's sparse<->busy curve. Eased with t*t so most of
    // the knob's travel stays gentle and the chaotic end only shows up in
    // the last stretch, rather than getting noisy right away.
    struct CharacterTargets { float density, sizeMs, spreadSec, unpredictability; };

    CharacterTargets characterCurve (float t)
    {
        const float shaped = t * t;
        return {
            juce::jmap (shaped, 0.0f, 1.0f, 1.2f, 20.0f),
            juce::jmap (shaped, 0.0f, 1.0f, 420.0f, 45.0f),
            juce::jmap (shaped, 0.0f, 1.0f, 3.2f, 0.25f),
            juce::jmap (shaped, 0.0f, 1.0f, 0.06f, 0.85f)
        };
    }
}

BDCPluginAudioProcessorEditor::BDCPluginAudioProcessorEditor (BDCPluginAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    {
        auto logoImage = juce::ImageCache::getFromMemory (BinaryData::BDCLogo_png, BinaryData::BDCLogo_pngSize);
        logoButton.setImages (false, true, true,
                               logoImage, 1.0f, {},
                               logoImage, 1.0f, juce::Colours::white.withAlpha (0.12f),
                               logoImage, 1.0f, juce::Colours::white.withAlpha (0.22f));
    }
    logoButton.setTooltip ("BDC\n\nClick to see the full logo.");
    logoButton.onClick = [this]
    {
        logoOverlay.setVisible (true);
        logoOverlay.toFront (true);
    };
    addAndMakeVisible (logoButton);

    addChildComponent (logoOverlay); // starts hidden; shown full-window when the logo is clicked

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

    randomizeButton.setTooltip (
        "Jumbles the generator and effect tone knobs within musical ranges for a quick new starting point - "
        "Scale/Root/Key Follow, sync settings, and level knobs (Mix, Output, BPM) are left alone.");
    randomizeButton.onClick = [this] { randomizeSound(); };
    addAndMakeVisible (randomizeButton);

    advancedToggleButton.setTooltip (
        "Shows the per-effect fine-tuning knobs (grain density/size/spread/chaos, delay time/feedback/taps/"
        "spread, chorus rate/depth, rotary speed) and tempo sync controls. The four mix bars above and the "
        "footer macros work either way - this just reveals the deeper knobs underneath.");
    advancedToggleButton.onClick = [this] { setAdvancedVisible (! showAdvanced); };
    addAndMakeVisible (advancedToggleButton);

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
        "How many new notes the generator plays per second. Higher = busier and more granular, lower = sparser and more spacious. "
        "Ignored while TRIGGER is on - each input hit spawns a note instead.");
    setupKnob (grainSizeKnob, "SIZE", "grainSizeMs",
        "How long each generated note lasts. Shorter = choppier/glitchier, longer = smoother and more sustained.");
    setupKnob (grainSpreadKnob, "SPREAD", "grainSpreadSec",
        "How far back in time the generator is allowed to pull material from. Higher = it draws on a longer memory of what you played.");
    setupKnob (unpredictabilityKnob, "CHAOS", "unpredictability",
        "How often the generated melody takes a big jump instead of moving stepwise. Higher = more unpredictable and adventurous.");

    grainTriggerLabel.setText ("TRIGGER", juce::dontSendNotification);
    grainTriggerLabel.setJustificationType (juce::Justification::centred);
    grainTriggerLabel.setFont (juce::Font (10.0f));
    addAndMakeVisible (grainTriggerLabel);

    grainTriggerButton.setClickingTogglesState (true);
    grainTriggerButton.setTooltip (
        "TRIGGER\n\n"
        "Spawns a note the instant an input hit is detected, instead of the free-running Density clock - great "
        "for drums/percussion, where you want an answer-back on every hit rather than notes drifting "
        "independently of what's being played.");
    addAndMakeVisible (grainTriggerButton);
    grainTriggerAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "grainTrigger", grainTriggerButton);

    characterKnob.caption.setText ("CHARACTER", juce::dontSendNotification);
    characterKnob.caption.setJustificationType (juce::Justification::centred);
    characterKnob.caption.setFont (juce::Font (10.0f));
    addAndMakeVisible (characterKnob.caption);
    characterKnob.valueLabel.setJustificationType (juce::Justification::centred);
    characterKnob.valueLabel.setFont (juce::Font (9.0f));
    addAndMakeVisible (characterKnob.valueLabel);
    characterKnob.dial.setRange (0.0, 1.0);
    characterKnob.dial.setTooltip (
        "Macro control for the generator's personality: sparse and ambient at low settings, dense and chaotic "
        "at high settings. Moves Density, Size, Spread, and Chaos together in one gesture - each stays free "
        "for hand-tuning afterwards, and won't snap this knob back to match.");
    addAndMakeVisible (characterKnob.dial);
    characterKnob.dial.onValueChange = [this] { applyCharacterMacro ((float) characterKnob.dial.getValue()); };
    characterKnob.dial.setValue (0.3, juce::dontSendNotification);
    applyCharacterMacro (0.3f);

    setupKnob (delayTimeKnob, "TIME", "delayTimeMs",
        "Time between echoes, in milliseconds.");
    setupKnob (delayFeedbackKnob, "FEEDBACK", "delayFeedback",
        "How much of each echo feeds back into the next one. Higher = repeats for longer.");
    setupKnob (delayTapsKnob, "TAPS", "delayTaps",
        "Number of echoes per repeat, read off the same delay line like the multiple heads on a real tape echo. "
        "1 is a plain single-tap delay; higher counts add quieter pre-echoes ahead of the main repeat. "
        "Ignored while REVERSE is on.");
    setupKnob (delayTapSpreadKnob, "SPREAD", "delayTapSpread",
        "Spaces and pans the extra echo taps (when Taps is above 1) alternately left/right for a wider, more "
        "rhythmic texture. Has no effect with only 1 tap, or while REVERSE is on.");

    delayReverseLabel.setText ("REVERSE", juce::dontSendNotification);
    delayReverseLabel.setJustificationType (juce::Justification::centred);
    delayReverseLabel.setFont (juce::Font (10.0f));
    addAndMakeVisible (delayReverseLabel);

    delayReverseButton.setClickingTogglesState (true);
    delayReverseButton.setTooltip (
        "REVERSE\n\n"
        "Each repeat plays its chunk of history backwards instead of forwards - a classic \"reverse echo\" "
        "swell, timed to the Time knob. Overrides Taps while on (they work together again once it's off).");
    addAndMakeVisible (delayReverseButton);
    delayReverseAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "delayReverse", delayReverseButton);

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
    rotaryFastButton.setTooltip (
        "SPEED\n\n"
        "Toggles the rotary speaker between slow (chorale) and fast (tremolo) speed, ramping between them like a real Leslie motor.");
    addAndMakeVisible (rotaryFastButton);
    rotaryFastAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "rotaryFast", rotaryFastButton);

    setupKnob (glueKnob, "GLUE", "outputGlue",
        "A subtle saturation stage after everything else, like a mixing console's output stage rounding off "
        "whatever passes through it - gives the whole mix a bit of cohesive character even on patches that "
        "don't use Tape. Stays gentle even at 100%.");

    outputGainCaption.setText ("OUTPUT", juce::dontSendNotification);
    outputGainCaption.setFont (juce::Font (12.0f));
    addAndMakeVisible (outputGainCaption);
    outputGainSlider.setTooltip ("Overall output level trim, in dB.");
    addAndMakeVisible (outputGainSlider);
    outputGainAttachment = std::make_unique<SliderAttachment> (processorRef.apvts, "outputGainDb", outputGainSlider);

    outputGainValueLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputGainValueLabel);
    if (auto* param = processorRef.apvts.getParameter ("outputGainDb"))
    {
        juce::Label* label = &outputGainValueLabel;
        auto updateText = [label, param] { label->setText (param->getCurrentValueAsText(), juce::dontSendNotification); };
        outputGainSlider.onValueChange = updateText;
        updateText();
    }

    setupKnob (tapeKnob, "TAPE", "tapeAmount",
        "Runs the whole mix through emulated cassette 4-track character (Tascam Porta 02 MkII vibe): pitch wobble, dulled top end, saturation, and tape hiss. 0% is clean, 100% is fully lo-fi.");

    setupKnob (masterMixKnob, "MIX", "masterMix",
        "Overall dry/wet for everything combined - Grain, Chorus, Rotary, Delay, and Tape together. 0% is your untouched input, 100% is the fully processed signal (each effect's own mix still shapes how much of it there is within that 100%).");

    sustainButton.setClickingTogglesState (true);
    sustainButton.setTooltip ("When on, the generator keeps evolving off your last captured audio during silence instead of fading out. Generation never starts until you've actually played something in, either way.");
    addAndMakeVisible (sustainButton);
    sustainAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "sustainOnSilence", sustainButton);

    grabButton.setClickingTogglesState (true);
    grabButton.setTooltip (
        "Freezes the generator's memory right now, holding onto whatever's been captured so far - independent "
        "of Sustain, and regardless of whether you keep playing. Click again to let it start listening live again.");
    grabButton.onClick = [this]
    {
        const bool frozen = grabButton.getToggleState();
        processorRef.setGrabFrozen (frozen);
        grabButton.setButtonText (frozen ? "GRABBED" : "GRAB");
    };
    addAndMakeVisible (grabButton);

    inputMeterLabel.setText ("IN", juce::dontSendNotification);
    inputMeterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (inputMeterLabel);
    inputMeter.setRange (0.0, 1.0);
    inputMeter.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (inputMeter);

    outputMeterLabel.setText ("OUT", juce::dontSendNotification);
    outputMeterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputMeterLabel);
    outputMeter.setRange (0.0, 1.0);
    outputMeter.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (outputMeter);

    setResizable (true, true);
    setSize (kDesignWidth, kDesignHeightCompact);
    setAdvancedVisible (false); // starts collapsed to a lean default view; sets aspect ratio + resize limits too

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

    // Level meters: map linear peak amplitude to a 0-1 display value over a
    // -48dB..0dB window, so the bars actually move across their useful
    // range at typical playing levels instead of sitting near the bottom.
    auto levelToDisplay = [] (float linearPeak)
    {
        const float db = juce::Decibels::gainToDecibels (linearPeak, -48.0f);
        return juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
    };
    inputMeter.setValue (levelToDisplay (processorRef.getInputLevel()), juce::dontSendNotification);
    outputMeter.setValue (levelToDisplay (processorRef.getOutputLevel()), juce::dontSendNotification);
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

    // The knob/bar caption text below is often abbreviated or gets clipped
    // at small window sizes, so the tooltip leads with the control's full
    // name on its own line before the description of what it does.
    hb.bar.setTooltip (labelText + "\n\n" + tooltip);
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

    // The caption below the dial is often abbreviated (or, at small window
    // sizes, visually clipped), so the tooltip leads with the control's
    // full name on its own line before the description of what it does.
    k.dial.setTooltip (labelText + "\n\n" + tooltip);
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

void BDCPluginAudioProcessorEditor::applyCharacterMacro (float t01)
{
    const auto targets = characterCurve (t01);

    auto setRaw = [this] (const juce::String& id, float rawValue)
    {
        if (auto* p = processorRef.apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (rawValue));
    };

    setRaw ("grainDensity", targets.density);
    setRaw ("grainSizeMs", targets.sizeMs);
    setRaw ("grainSpreadSec", targets.spreadSec);
    setRaw ("unpredictability", targets.unpredictability);

    characterKnob.valueLabel.setText (juce::String ((int) std::round (t01 * 100.0f)) + "%", juce::dontSendNotification);
}

void BDCPluginAudioProcessorEditor::randomizeSound()
{
    juce::Random random;

    auto setRandom = [this, &random] (const juce::String& id, float lo, float hi)
    {
        if (auto* p = processorRef.apvts.getParameter (id))
        {
            const float value = lo + random.nextFloat() * (hi - lo);
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        }
    };

    // Grain gets the heaviest hand - it's the generator's actual voice.
    // Ranges stay well inside the knobs' full extremes (e.g. Density can go
    // to 30Hz, Unpredictability to 1.0) so a random click reads as "a
    // different idea" rather than "broken."
    setRandom ("grainDensity", 1.0f, 14.0f);
    setRandom ("grainSizeMs", 60.0f, 350.0f);
    setRandom ("grainSpreadSec", 0.3f, 3.0f);
    setRandom ("unpredictability", 0.05f, 0.6f);
    setRandom ("generativeMix", 0.25f, 0.75f);

    // Chorus/Rotary/Delay/Tape get a lighter touch, so the result still
    // reads as "the same instrument, new take" rather than a random preset.
    setRandom ("chorusRate", 0.1f, 1.5f);
    setRandom ("chorusDepth", 0.1f, 0.5f);
    setRandom ("chorusMix", 0.1f, 0.4f);
    setRandom ("rotaryMix", 0.1f, 0.6f);
    setRandom ("delayTimeMs", 150.0f, 900.0f);
    setRandom ("delayFeedback", 0.15f, 0.5f);
    setRandom ("delayMix", 0.15f, 0.45f);
    setRandom ("delayTaps", 1.0f, 3.0f);
    setRandom ("delayTapSpread", 0.0f, 0.6f);
    setRandom ("tapeAmount", 0.0f, 35.0f);
}

void BDCPluginAudioProcessorEditor::setAdvancedVisible (bool show)
{
    showAdvanced = show;
    advancedToggleButton.setButtonText (show ? "HIDE ADVANCED" : "SHOW ADVANCED");

    for (auto* k : { &grainDensityKnob, &grainSizeKnob, &grainSpreadKnob, &unpredictabilityKnob,
                      &delayTimeKnob, &delayFeedbackKnob, &delayTapsKnob, &delayTapSpreadKnob,
                      &chorusRateKnob, &chorusDepthKnob, &manualBpmKnob, &glueKnob })
        k->setVisible (show);

    rotaryFastLabel.setVisible (show);
    rotaryFastButton.setVisible (show);
    grainTriggerLabel.setVisible (show);
    grainTriggerButton.setVisible (show);
    delayReverseLabel.setVisible (show);
    delayReverseButton.setVisible (show);
    grainSync.setVisible (show);
    delaySync.setVisible (show);

    // The two modes are two different fixed aspect ratios (the detail/sync
    // knobs take a fixed chunk of height that either exists or doesn't), so
    // switching modes means re-pinning the constrainer and resize limits,
    // not just resizing once.
    const double aspect = (double) kDesignWidth / (double) (show ? kDesignHeightExpanded : kDesignHeightCompact);

    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (aspect);

    setResizeLimits (560, juce::roundToInt (560.0 / aspect), 1720, juce::roundToInt (1720.0 / aspect));
    setSize (getWidth(), juce::roundToInt ((double) getWidth() / aspect));
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

    // Faint dividers reinforcing groupings that already exist in the layout
    // (one column per effect; Output / tone macros / Sustain in the footer)
    // - purely a scanability aid, nothing here changes what's clickable.
    g.setColour (BDCLookAndFeel::ink.withAlpha (0.16f));

    for (auto x : columnDividerX)
        g.drawVerticalLine (x, (float) columnDividerTop, (float) columnDividerBottom);

    for (auto x : footerDividerX)
        g.drawVerticalLine (x, (float) footerDividerTop, (float) footerDividerBottom);
}

void BDCPluginAudioProcessorEditor::resized()
{
    // Everything below scales off this ratio, so resizing the window scales
    // the whole UI (fonts included) rather than just reflowing whitespace.
    const float scale = (float) getWidth() / (float) kDesignWidth;
    auto S  = [scale] (int v)   { return juce::roundToInt ((float) v * scale); };
    auto SF = [scale] (float v) { return juce::Font (v * scale); };

    presetCaption.setFont (SF (kCaptionFontSize));
    tunerLabel.setFont (SF (kTunerFontSize));
    scaleCaption.setFont (SF (kCaptionFontSize));
    rootCaption.setFont (SF (kCaptionFontSize));
    outputGainCaption.setFont (SF (kCaptionFontSize));
    outputGainValueLabel.setFont (SF (kKnobValueFontSize));
    rotaryFastLabel.setFont (SF (kRotaryLabelFontSize));
    grainTriggerLabel.setFont (SF (kRotaryLabelFontSize));
    delayReverseLabel.setFont (SF (kRotaryLabelFontSize));

    for (auto* hb : { &grainBar, &delayBar, &chorusBar, &rotaryBar })
    {
        hb->header.setFont (SF (kHeroHeaderFontSize));
        hb->valueLabel.setFont (SF (kHeroValueFontSize));
    }

    for (auto* k : { &grainDensityKnob, &grainSizeKnob, &grainSpreadKnob, &unpredictabilityKnob, &characterKnob,
                      &delayTimeKnob, &delayFeedbackKnob, &delayTapsKnob, &delayTapSpreadKnob,
                      &chorusRateKnob, &chorusDepthKnob,
                      &manualBpmKnob, &tapeKnob, &masterMixKnob, &glueKnob })
    {
        k->caption.setFont (SF (kKnobCaptionFontSize));
        k->valueLabel.setFont (SF (kKnobValueFontSize));
    }

    inputMeterLabel.setFont (SF (kKnobCaptionFontSize));
    outputMeterLabel.setFont (SF (kKnobCaptionFontSize));

    logoOverlay.setBounds (getLocalBounds());

    // Input/output meters live in slim columns right at the window edges,
    // outside the margin the rest of the UI is laid out within - carved off
    // first so `area` below is unaffected by their presence either mode.
    auto windowArea = getLocalBounds();
    auto inputMeterCol = windowArea.removeFromLeft (S (28)).reduced (0, S (24));
    auto outputMeterCol = windowArea.removeFromRight (S (28)).reduced (0, S (24));

    inputMeterLabel.setBounds (inputMeterCol.removeFromTop (S (16)));
    inputMeterCol.removeFromTop (S (6));
    inputMeter.setBounds (inputMeterCol.withSizeKeepingCentre (juce::jmin (inputMeterCol.getWidth(), S (14)), inputMeterCol.getHeight()));

    outputMeterLabel.setBounds (outputMeterCol.removeFromTop (S (16)));
    outputMeterCol.removeFromTop (S (6));
    outputMeter.setBounds (outputMeterCol.withSizeKeepingCentre (juce::jmin (outputMeterCol.getWidth(), S (14)), outputMeterCol.getHeight()));

    auto area = windowArea.reduced (S (24));

    auto header = area.removeFromTop (S (40));
    logoButton.setBounds (header.removeFromLeft (S (40)));

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
    presetRow.removeFromLeft (S (12));
    randomizeButton.setBounds (presetRow.removeFromLeft (S (90)).withSizeKeepingCentre (S (90), S (24)));
    advancedToggleButton.setBounds (presetRow.removeFromRight (S (150)).withSizeKeepingCentre (S (150), S (24)));

    area.removeFromTop (S (14));

    auto footer = area.removeFromBottom (S (78));
    footerDividerTop = footer.getY() + S (4);
    footerDividerBottom = footer.getBottom() - S (4);

    auto performanceRow = footer.removeFromRight (S (238));
    grabButton.setBounds (performanceRow.removeFromLeft (S (90)).withSizeKeepingCentre (S (90), S (32)));
    performanceRow.removeFromLeft (S (8));
    sustainButton.setBounds (performanceRow.withSizeKeepingCentre (S (140), S (32)));
    footer.removeFromRight (S (16) / 2);
    footerDividerX[1] = footer.getRight(); // Grab/Sustain | tone macros
    footer.removeFromRight (S (16) / 2);

    auto tapeSlot = footer.removeFromRight (S (72));
    tapeKnob.valueLabel.setBounds (tapeSlot.removeFromBottom (S (13)));
    tapeKnob.caption.setBounds (tapeSlot.removeFromBottom (S (16)));
    tapeSlot.removeFromTop (S (8)); // a little breathing room above the dial, so it doesn't sit flush at the top
    tapeKnob.dial.setBounds (tapeSlot);
    footer.removeFromRight (S (16));

    auto mixSlot = footer.removeFromRight (S (72));
    masterMixKnob.valueLabel.setBounds (mixSlot.removeFromBottom (S (13)));
    masterMixKnob.caption.setBounds (mixSlot.removeFromBottom (S (16)));
    mixSlot.removeFromTop (S (8));
    masterMixKnob.dial.setBounds (mixSlot);
    footer.removeFromRight (S (16));

    auto characterSlot = footer.removeFromRight (S (72));
    characterKnob.valueLabel.setBounds (characterSlot.removeFromBottom (S (13)));
    characterKnob.caption.setBounds (characterSlot.removeFromBottom (S (16)));
    characterSlot.removeFromTop (S (8));
    characterKnob.dial.setBounds (characterSlot);
    footer.removeFromRight (S (16) / 2);
    footerDividerX[0] = footer.getRight(); // tone macros | Output
    footer.removeFromRight (S (16) / 2);

    auto outputBlock = footer.withSizeKeepingCentre (footer.getWidth(), S (24 + 4 + 13));
    auto outputRow = outputBlock.removeFromTop (S (24));
    outputGainCaption.setBounds (outputRow.removeFromLeft (S (90)));
    outputGainSlider.setBounds (outputRow);
    const int outputSliderX = outputRow.getX();
    const int outputSliderWidth = outputRow.getWidth();
    outputBlock.removeFromTop (S (4));
    outputGainValueLabel.setBounds (outputSliderX, outputBlock.getY(), outputSliderWidth, outputBlock.getHeight());

    // The detail (per-effect fine-tuning knobs) and sync (tempo-sync)
    // strips only take up layout space when Advanced is shown - when it's
    // hidden, those knobs are invisible and the hero bars simply get the
    // reclaimed space instead (see setAdvancedVisible(), which resizes the
    // window between two fixed-aspect-ratio modes to match).
    juce::Rectangle<int> detail, syncStrip;
    if (showAdvanced)
    {
        area.removeFromBottom (S (16));
        detail = area.removeFromBottom (S (120));
        area.removeFromBottom (S (12));
        syncStrip = area.removeFromBottom (S (58));
        area.removeFromBottom (S (16));
    }

    auto heroArea = area;
    const int numCols = 4;
    const int gap = S (20);
    const int colWidth = (heroArea.getWidth() - gap * (numCols - 1)) / numCols;

    columnDividerTop = heroArea.getY();
    columnDividerBottom = showAdvanced ? syncStrip.getBottom() : heroArea.getBottom();

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

    columnDividerX[0] = col1.getRight() + gap / 2;
    columnDividerX[1] = col2.getRight() + gap / 2;
    columnDividerX[2] = col3.getRight() + gap / 2;

    layoutHero (grainBar, col1);
    layoutHero (delayBar, col2);
    layoutHero (chorusBar, col3);
    layoutHero (rotaryBar, col4);

    if (showAdvanced)
    {
        auto d1 = detail.removeFromLeft (colWidth); detail.removeFromLeft (gap);
        auto d2 = detail.removeFromLeft (colWidth); detail.removeFromLeft (gap);
        auto d3 = detail.removeFromLeft (colWidth); detail.removeFromLeft (gap);
        auto d4 = detail;

        auto layoutKnob = [S] (Knob& k, juce::Rectangle<int> slot)
        {
            k.valueLabel.setBounds (slot.removeFromBottom (S (13)));
            k.caption.setBounds (slot.removeFromBottom (S (16)));
            slot.removeFromTop (S (8)); // breathing room so the dial's arc doesn't sit flush against the top edge
            k.dial.setBounds (slot);
        };

        {
            const int n = 5;
            const int w = d1.getWidth() / n;
            Knob* knobs[] { &grainDensityKnob, &grainSizeKnob, &grainSpreadKnob, &unpredictabilityKnob };
            for (int i = 0; i < 4; ++i)
                layoutKnob (*knobs[i], d1.removeFromLeft (w));

            auto triggerSlot = d1;
            triggerSlot.removeFromLeft (S (6)); // gap so the button doesn't crowd the CHAOS knob beside it
            grainTriggerLabel.setBounds (triggerSlot.removeFromBottom (S (16)));
            grainTriggerButton.setBounds (triggerSlot.withSizeKeepingCentre (juce::jmin (triggerSlot.getWidth(), S (44)), S (32)));
        }
        {
            const int n = 5;
            const int w = d2.getWidth() / n;
            Knob* knobs[] { &delayTimeKnob, &delayFeedbackKnob, &delayTapsKnob, &delayTapSpreadKnob };
            for (int i = 0; i < 4; ++i)
                layoutKnob (*knobs[i], d2.removeFromLeft (w));

            auto reverseSlot = d2;
            reverseSlot.removeFromLeft (S (6)); // gap so the button doesn't crowd the SPREAD knob beside it
            delayReverseLabel.setBounds (reverseSlot.removeFromBottom (S (16)));
            delayReverseButton.setBounds (reverseSlot.withSizeKeepingCentre (juce::jmin (reverseSlot.getWidth(), S (44)), S (32)));
        }
        {
            const int w = d3.getWidth() / 2;
            layoutKnob (chorusRateKnob, d3.removeFromLeft (w));
            layoutKnob (chorusDepthKnob, d3);
        }
        {
            const int w = d4.getWidth() / 2;
            auto fastHalf = d4.removeFromLeft (w);
            d4.removeFromLeft (S (10)); // gap so FAST and the GLUE dial don't crowd each other
            rotaryFastLabel.setBounds (fastHalf.removeFromBottom (S (16)));
            rotaryFastButton.setBounds (fastHalf.withSizeKeepingCentre (juce::jmin (fastHalf.getWidth() - S (10), S (76)), S (32)));
            layoutKnob (glueKnob, d4);
        }

        // Sync strip: SYNC + note division + x/÷ multiplier for Grain and
        // Delay (matching their hero-bar columns); a manual BPM fallback
        // knob sits under Rotary's column, where there's otherwise nothing
        // to sync.
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
        sy4.removeFromTop (S (4)); // breathing room so the dial's arc doesn't sit flush against the top edge
        manualBpmKnob.dial.setBounds (sy4.withSizeKeepingCentre (juce::jmin (sy4.getWidth(), S (44)), sy4.getHeight()));
    }
}
