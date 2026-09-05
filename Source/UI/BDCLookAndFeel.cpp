#include "BDCLookAndFeel.h"

const juce::Colour BDCLookAndFeel::background { 0xffec84d6 }; // bright orchid pink
const juce::Colour BDCLookAndFeel::ink        { 0xff8a3568 }; // deep plum
const juce::Colour BDCLookAndFeel::track      { 0xffd699c6 }; // soft mid pink
const juce::Colour BDCLookAndFeel::text       { 0xfff8e6f4 }; // pale pink-lavender

juce::Font BDCLookAndFeel::trackedFont (float height, bool bold)
{
    juce::Font f (height, bold ? juce::Font::bold : juce::Font::plain);
    f.setExtraKerningFactor (0.07f);
    return f;
}

BDCLookAndFeel::BDCLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::DocumentWindow::backgroundColourId, background);

    setColour (juce::Slider::backgroundColourId, track);
    setColour (juce::Slider::trackColourId, ink);
    setColour (juce::Slider::thumbColourId, ink);
    setColour (juce::Slider::rotarySliderFillColourId, ink);
    setColour (juce::Slider::rotarySliderOutlineColourId, track);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, background);
    setColour (juce::Slider::textBoxOutlineColourId, track);

    setColour (juce::Label::textColourId, text);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::ComboBox::backgroundColourId, ink);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, ink);
    setColour (juce::ComboBox::arrowColourId, text);
    setColour (juce::PopupMenu::backgroundColourId, ink);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, text);
    setColour (juce::PopupMenu::highlightedTextColourId, ink);

    setColour (juce::TextButton::buttonColourId, track);
    setColour (juce::TextButton::buttonOnColourId, ink);
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, text);
}

void BDCLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                        const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::Rectangle<float> bounds ((float) x, (float) y, (float) width, (float) height);

    if (style == juce::Slider::LinearBarVertical)
    {
        g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.fillRect (bounds);

        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillRect (bounds.withTop (sliderPos));
        return;
    }

    if (style == juce::Slider::LinearBar)
    {
        g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.fillRect (bounds);

        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillRect (bounds.withRight (sliderPos));
        return;
    }

    LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
}

void BDCLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto centre = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto strokeWidth = juce::jmax (2.0f, radius * 0.22f);

    juce::Path trackArc;
    trackArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
    g.strokePath (trackArc, juce::PathStrokeType (strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
    g.strokePath (valueArc, juce::PathStrokeType (strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Point<float> tip (centre.x + (radius - strokeWidth) * std::sin (angle),
                             centre.y - (radius - strokeWidth) * std::cos (angle));
    g.drawLine ({ centre, tip }, 2.0f);
}

void BDCLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    bool isOn = button.getToggleState() || shouldDrawButtonAsDown;

    g.setColour (isOn ? ink : track);
    g.fillRect (bounds);

    if (shouldDrawButtonAsHighlighted && ! isOn)
    {
        g.setColour (ink.withAlpha (0.15f));
        g.fillRect (bounds);
    }
}

juce::Font BDCLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return trackedFont (juce::jmin (16.0f, (float) box.getHeight() * 0.6f));
}

juce::Font BDCLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return trackedFont (juce::jmin (15.0f, (float) buttonHeight * 0.55f));
}

juce::Font BDCLookAndFeel::getLabelFont (juce::Label& label)
{
    return trackedFont (label.getFont().getHeight());
}
