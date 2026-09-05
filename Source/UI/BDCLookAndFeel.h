#pragma once

#include <JuceHeader.h>

// Flat, poster-style look - bright pink background, deep plum bars, pale
// pink-lavender type - matching the BDC reference mockup's colorway, set in
// the brand's uploaded TAYLennon typeface. No gradients, no bevels, no
// skeuomorphism. Vertical "bar" sliders render as solid blocks; rotary
// knobs render as a flat stroked arc.
class BDCLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BDCLookAndFeel();

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                            float sliderPos, float minSliderPos, float maxSliderPos,
                            const juce::Slider::SliderStyle style, juce::Slider&) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                            float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getPopupMenuFont() override;

    static juce::Font trackedFont (float height, bool bold = true);

    static const juce::Colour background;
    static const juce::Colour ink;
    static const juce::Colour track;
    static const juce::Colour text;
};
