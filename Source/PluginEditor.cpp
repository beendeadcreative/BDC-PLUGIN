#include "PluginEditor.h"

BDCPluginAudioProcessorEditor::BDCPluginAudioProcessorEditor (BDCPluginAudioProcessor& processor)
    : juce::GenericAudioProcessorEditor (processor)
{
    setSize (420, 640);
}
