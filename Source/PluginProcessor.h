#pragma once

#include <JuceHeader.h>
#include "DSP/CircularBuffer.h"
#include "DSP/InputActivityDetector.h"
#include "DSP/PitchDetector.h"
#include "DSP/KeyTracker.h"
#include "DSP/GenerativeEngine.h"
#include "DSP/Granulator.h"
#include "DSP/ChorusModule.h"
#include "DSP/RotaryModule.h"
#include "DSP/DelayModule.h"
#include "DSP/TapeModule.h"

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

    // Factory presets, exposed as host "programs" so DAW preset menus can
    // browse them (see Presets.h/.cpp for the actual data).
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Live tuner readout, polled by the editor's Timer - safe to call from
    // the message thread while the audio thread keeps updating it.
    float getDetectedFrequencyHz() const noexcept { return pitchDetector.getDetectedFrequencyHz(); }
    bool isPitchDetected() const noexcept { return pitchDetector.isPitchDetected(); }

    // Key Follow's live tracked root/scale, for the editor to display while
    // AUTO is on (0-11 pitch class, 0-4 scale index - same ordering as the
    // Root/Scale parameters).
    int getTrackedRootPitchClass() const noexcept { return keyTracker.getRootPitchClass(); }
    int getTrackedScaleType() const noexcept { return keyTracker.getScaleType(); }

    // The manually-selected Root/Scale (i.e. what's used when Key Follow is
    // off), so the editor can resync its combo boxes when AUTO is switched
    // off after having shown the tracked key instead.
    int getManualRootIndex() const noexcept { return rootNoteParam->getIndex(); }
    int getManualScaleIndex() const noexcept { return scaleTypeParam->getIndex(); }

    // Grab: a manual, momentary override of the capture buffer, driven by
    // the editor's GRAB button rather than a saved parameter - freezing a
    // slice of audio history isn't something a saved session should recall.
    // While on, the capture buffer stops being overwritten no matter what
    // Sustain When Silent is set to or whether you keep playing, so you can
    // hold onto a specific passage on demand instead of only when it goes
    // quiet; turning it back off resumes normal capture immediately.
    void setGrabFrozen (bool frozen) noexcept { grabFrozen.store (frozen, std::memory_order_relaxed); }
    bool isGrabFrozen() const noexcept { return grabFrozen.load (std::memory_order_relaxed); }

    // Live input/output level meters, polled by the editor's Timer. Linear
    // peak amplitude with a ~400ms release, updated once per audio block.
    float getInputLevel() const noexcept { return inputLevel.load (std::memory_order_relaxed); }
    float getOutputLevel() const noexcept { return outputLevel.load (std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    int rootNoteMidiFor (int rootChoiceIndex) const noexcept { return 48 + rootChoiceIndex; }
    GenerativeEngine::Scale scaleFor (int scaleChoiceIndex) const noexcept;

    // Host tempo if the DAW is reporting one, else the manual BPM fallback
    // (used e.g. in Standalone with no transport).
    double getCurrentBpm() const;

    // Shared tempo-sync helpers used by both Delay and Grain sync.
    static float noteDivisionToBeats (int divisionChoiceIndex) noexcept;
    static float multiplierChoiceToValue (int multiplierChoiceIndex) noexcept;

    // Cached parameter pointers, set once in the constructor.
    juce::AudioParameterBool*   sustainOnSilenceParam = nullptr;
    juce::AudioParameterChoice* rootNoteParam = nullptr;
    juce::AudioParameterChoice* scaleTypeParam = nullptr;
    juce::AudioParameterBool*   rotaryFastParam = nullptr;
    juce::AudioParameterBool*   keyFollowParam = nullptr;

    juce::AudioParameterBool*   delaySyncParam = nullptr;
    juce::AudioParameterChoice* delayNoteDivisionParam = nullptr;
    juce::AudioParameterChoice* delayTimeMultiplierParam = nullptr;
    juce::AudioParameterBool*   grainRateSyncParam = nullptr;
    juce::AudioParameterChoice* grainNoteDivisionParam = nullptr;
    juce::AudioParameterChoice* grainRateMultiplierParam = nullptr;

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
    std::atomic<float>* delayTapsParam = nullptr;
    std::atomic<float>* delayTapSpreadParam = nullptr;
    std::atomic<float>* tapeAmountParam = nullptr;
    std::atomic<float>* outputGainDbParam = nullptr;
    std::atomic<float>* manualBpmParam = nullptr;
    std::atomic<float>* masterMixParam = nullptr;

    CircularBuffer captureBuffer;
    InputActivityDetector inputActivityDetector;
    PitchDetector pitchDetector;
    KeyTracker keyTracker;
    GenerativeEngine generativeEngine;
    Granulator granulator;
    ChorusModule chorusModule;
    RotaryModule rotaryModule;
    DelayModule delayModule;
    TapeModule tapeModule;

    // Latches true the first time real audio is played in; generation stays
    // silent until then, so the plugin never generates out of nothing.
    bool hasBeenPrimed = false;

    std::atomic<bool> grabFrozen { false };
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    int currentProgramIndex = 0;

    juce::AudioBuffer<float> generatedScratch;
    juce::AudioBuffer<float> masterDryScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BDCPluginAudioProcessor)
};
