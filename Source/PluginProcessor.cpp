#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ParamIDs
{
    static const juce::String sustainOnSilence  { "sustainOnSilence" };
    static const juce::String rootNote          { "rootNote" };
    static const juce::String scaleType         { "scaleType" };
    static const juce::String unpredictability  { "unpredictability" };
    static const juce::String grainDensity      { "grainDensity" };
    static const juce::String grainSizeMs       { "grainSizeMs" };
    static const juce::String grainSpreadSec    { "grainSpreadSec" };
    static const juce::String generativeMix     { "generativeMix" };
    static const juce::String chorusRate        { "chorusRate" };
    static const juce::String chorusDepth       { "chorusDepth" };
    static const juce::String chorusMix         { "chorusMix" };
    static const juce::String rotaryFast        { "rotaryFast" };
    static const juce::String rotaryMix         { "rotaryMix" };
    static const juce::String delayTimeMs       { "delayTimeMs" };
    static const juce::String delayFeedback     { "delayFeedback" };
    static const juce::String delayMix          { "delayMix" };
    static const juce::String outputGainDb      { "outputGainDb" };
}

BDCPluginAudioProcessor::BDCPluginAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    sustainOnSilenceParam = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter (ParamIDs::sustainOnSilence));
    rootNoteParam         = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (ParamIDs::rootNote));
    scaleTypeParam        = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (ParamIDs::scaleType));
    rotaryFastParam       = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter (ParamIDs::rotaryFast));

    unpredictabilityParam = apvts.getRawParameterValue (ParamIDs::unpredictability);
    grainDensityParam     = apvts.getRawParameterValue (ParamIDs::grainDensity);
    grainSizeMsParam      = apvts.getRawParameterValue (ParamIDs::grainSizeMs);
    grainSpreadSecParam   = apvts.getRawParameterValue (ParamIDs::grainSpreadSec);
    generativeMixParam    = apvts.getRawParameterValue (ParamIDs::generativeMix);
    chorusRateParam       = apvts.getRawParameterValue (ParamIDs::chorusRate);
    chorusDepthParam      = apvts.getRawParameterValue (ParamIDs::chorusDepth);
    chorusMixParam        = apvts.getRawParameterValue (ParamIDs::chorusMix);
    rotaryMixParam        = apvts.getRawParameterValue (ParamIDs::rotaryMix);
    delayTimeMsParam      = apvts.getRawParameterValue (ParamIDs::delayTimeMs);
    delayFeedbackParam    = apvts.getRawParameterValue (ParamIDs::delayFeedback);
    delayMixParam         = apvts.getRawParameterValue (ParamIDs::delayMix);
    outputGainDbParam     = apvts.getRawParameterValue (ParamIDs::outputGainDb);
}

BDCPluginAudioProcessor::~BDCPluginAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout BDCPluginAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamIDs::sustainOnSilence, 1 }, "Sustain When Silent", true));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamIDs::rootNote, 1 }, "Root Note",
        StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 9));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamIDs::scaleType, 1 }, "Scale",
        StringArray { "Major", "Natural Minor", "Dorian", "Major Pentatonic", "Minor Pentatonic" }, 4));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::unpredictability, 1 }, "Unpredictability", 0.0f, 1.0f, 0.18f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::grainDensity, 1 }, "Grain Density", 0.5f, 30.0f, 4.5f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::grainSizeMs, 1 }, "Grain Size (ms)", 20.0f, 500.0f, 170.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::grainSpreadSec, 1 }, "Grain Spread (s)", 0.1f, 4.0f, 2.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::generativeMix, 1 }, "Generative Mix", 0.0f, 1.0f, 0.5f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::chorusRate, 1 }, "Chorus Rate", 0.05f, 5.0f, 0.9f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::chorusDepth, 1 }, "Chorus Depth", 0.0f, 1.0f, 0.3f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::chorusMix, 1 }, "Chorus Mix", 0.0f, 1.0f, 0.25f));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamIDs::rotaryFast, 1 }, "Rotary Fast", false));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::rotaryMix, 1 }, "Rotary Mix", 0.0f, 1.0f, 0.4f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::delayTimeMs, 1 }, "Delay Time (ms)", 1.0f, 2000.0f, 350.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::delayFeedback, 1 }, "Delay Feedback", 0.0f, 0.95f, 0.35f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::delayMix, 1 }, "Delay Mix", 0.0f, 1.0f, 0.3f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::outputGainDb, 1 }, "Output Gain (dB)", -24.0f, 12.0f, 0.0f));

    return layout;
}

GenerativeEngine::Scale BDCPluginAudioProcessor::scaleFor (int scaleChoiceIndex) const noexcept
{
    switch (scaleChoiceIndex)
    {
        case 0: return GenerativeEngine::Scale::Major;
        case 1: return GenerativeEngine::Scale::NaturalMinor;
        case 2: return GenerativeEngine::Scale::Dorian;
        case 3: return GenerativeEngine::Scale::MajorPentatonic;
        case 4:
        default: return GenerativeEngine::Scale::MinorPentatonic;
    }
}

void BDCPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int numChannels = getTotalNumOutputChannels();

    captureBuffer.prepare (sampleRate, numChannels, 6.0f); // 6s of history available for spread/reads
    inputActivityDetector.prepare (sampleRate);
    hasBeenPrimed = false;
    generativeEngine.reset();
    granulator.prepare (sampleRate, numChannels, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) numChannels;

    chorusModule.prepare (spec);
    rotaryModule.prepare (spec);
    delayModule.prepare (spec);

    generatedScratch.setSize (numChannels, samplesPerBlock);
}

void BDCPluginAudioProcessor::releaseResources() {}

bool BDCPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void BDCPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);

    generatedScratch.setSize (numChannels, numSamples, false, false, true);

    const int rootMidiNote = rootNoteMidiFor (rootNoteParam->getIndex());
    const auto scale = scaleFor (scaleTypeParam->getIndex());

    // --- 1. Is anything actually being played right now? -------------------
    const bool inputActive = inputActivityDetector.updateAndIsActive (buffer, numSamples);
    if (inputActive)
        hasBeenPrimed = true; // never generate until real audio has been played at least once

    // --- 2. Feed the capture buffer -----------------------------------------
    // While input is active, always capture the real signal. Once it goes
    // silent: if "Sustain When Silent" is on, freeze the buffer (stop
    // overwriting it) so generation keeps drawing on what was actually
    // played; if off, silence flows in like normal and generation fades out.
    if (inputActive || ! sustainOnSilenceParam->get())
        captureBuffer.write (buffer);

    // --- 3. Generative granulator: turns captured material into new phrases
    generativeEngine.setRootNote (rootMidiNote);
    generativeEngine.setScale (scale);
    generativeEngine.setUnpredictability (unpredictabilityParam->load());

    granulator.setParameters (grainDensityParam->load(), grainSizeMsParam->load(), grainSpreadSecParam->load());
    granulator.process (captureBuffer, generativeEngine, generatedScratch, numSamples);

    const float genMix = hasBeenPrimed ? generativeMixParam->load() : 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* dst = buffer.getWritePointer (ch);
        auto* generated = generatedScratch.getReadPointer (ch);

        for (int i = 0; i < numSamples; ++i)
            dst[i] = dst[i] * (1.0f - genMix) + generated[i] * genMix;
    }

    // --- 4. Chorus -> Rotary -> Delay --------------------------------------
    chorusModule.setParameters (chorusRateParam->load(), chorusDepthParam->load(), chorusMixParam->load());
    chorusModule.process (buffer);

    rotaryModule.setParameters (rotaryFastParam->get() ? 1.0f : 0.0f, rotaryMixParam->load());
    rotaryModule.process (buffer);

    delayModule.setParameters (delayTimeMsParam->load(), delayFeedbackParam->load(), delayMixParam->load());
    delayModule.process (buffer);

    // --- 5. Output trim ------------------------------------------------
    buffer.applyGain (juce::Decibels::decibelsToGain (outputGainDbParam->load()));
}

juce::AudioProcessorEditor* BDCPluginAudioProcessor::createEditor()
{
    return new BDCPluginAudioProcessorEditor (*this);
}

void BDCPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void BDCPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BDCPluginAudioProcessor();
}
