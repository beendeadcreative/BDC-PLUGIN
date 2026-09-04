#pragma once

#include <JuceHeader.h>
#include "DSP/CircularBuffer.h"
#include "DSP/SeedGenerator.h"
#include "DSP/GenerativeEngine.h"
#include "DSP/Granulator.h"
#include "DSP/ChorusModule.h"
#include "DSP/RotaryModule.h"
#include "DSP/DelayModule.h"

class BDCPluginAudioProcessor : public juce::AudioProcessor
{
public:
    BDCPluginAudioProcessor();
    ~BDCPluginAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    int rootNoteMidiFor (int rootChoiceIndex) const noexcept { return 48 + rootChoiceIndex; }
    GenerativeEngine::Scale scaleFor (int scaleChoiceIndex) const noexcept;

    // Cached parameter pointers, set once in the constructor.
    juce::AudioParameterChoice* selfGenerateModeParam = nullptr;
    juce::AudioParameterChoice* rootNoteParam = nullptr;
    juce::AudioParameterChoice* scaleTypeParam = nullptr;
    juce::AudioParameterBool*   rotaryFastParam = nullptr;

    std::atomic<float>* unpredictabilityParam = nullptr;
    std::atomic<float>* grainDensityParam = nullptr;
    std::atomic<float>* grainSizeMsParam = nullptr;
    std::atomic<float>* grainSpreadSecParam = nullptr;
    std::atomic<float>* generativeMixParam = nullptr;
    std::atomic<float>* chorusRateParam = nullptr;
    std::atomic<float>* chorusDepthParam = nullptr;
    std::atomic<float>* chorusMixParam = nullptr;
    std::atomic<float>* rotaryMixParam = nullptr;
    std::atomic<float>* delayTimeMsParam = nullptr;
    std::atomic<float>* delayFeedbackParam = nullptr;
    std::atomic<float>* delayMixParam = nullptr;
    std::atomic<float>* outputGainDbParam = nullptr;

    CircularBuffer captureBuffer;
    SeedGenerator seedGenerator;
    GenerativeEngine generativeEngine;
    Granulator granulator;
    ChorusModule chorusModule;
    RotaryModule rotaryModule;
    DelayModule delayModule;

    juce::AudioBuffer<float> seedScratch;
    juce::AudioBuffer<float> captureWriteScratch;
    juce::AudioBuffer<float> generatedScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BDCPluginAudioProcessor)
};
