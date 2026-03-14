#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Custom look and feel: dark amp-head aesthetic
class ImperialLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ImperialLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (0xffC8860A));
        setColour (juce::Slider::thumbColourId,               juce::Colour (0xffF0C040));
        setColour (juce::Label::textColourId,                 juce::Colour (0xffDDCCAA));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override
    {
        const float radius  = (float) std::min (width, height) * 0.5f - 4.0f;
        const float centreX = (float) x + (float) width  * 0.5f;
        const float centreY = (float) y + (float) height * 0.5f;
        const float angle   = rotaryStartAngle
                              + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        juce::Path outerRing;
        outerRing.addEllipse (centreX - radius, centreY - radius,
                              radius * 2.0f, radius * 2.0f);
        g.setColour (juce::Colour (0xff333333));
        g.fillPath (outerRing);
        g.setColour (juce::Colour (0xff555544));
        g.strokePath (outerRing, juce::PathStrokeType (1.5f));

        const float arcRadius = radius - 3.0f;
        juce::Path arc;
        arc.addArc (centreX - arcRadius, centreY - arcRadius,
                    arcRadius * 2.0f, arcRadius * 2.0f,
                    rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xffC8860A));
        g.strokePath (arc, juce::PathStrokeType (3.0f,
                      juce::PathStrokeType::curved,
                      juce::PathStrokeType::rounded));

        const float px = centreX + (arcRadius - 4.0f) * std::sin (angle);
        const float py = centreY - (arcRadius - 4.0f) * std::cos (angle);
        g.setColour (juce::Colour (0xffF0C040));
        g.drawLine (centreX, centreY, px, py, 2.5f);

        g.setColour (juce::Colour (0xff222222));
        g.fillEllipse (centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
    }

    // Custom button look: dark background, amber border, small bold text
    void drawButtonBackground (juce::Graphics& g, juce::Button& btn,
                                const juce::Colour&, bool isHighlighted, bool isDown) override
    {
        auto bounds = btn.getLocalBounds().toFloat().reduced (1.0f);
        const auto bg = isDown        ? juce::Colour (0xff3a2800)
                      : isHighlighted ? juce::Colour (0xff2a1e00)
                                      : juce::Colour (0xff1e1e1e);
        g.setColour (bg);
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (juce::Colour (isDown ? 0xffF0C040 : 0xffC8860A));
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& btn,
                         bool, bool) override
    {
        g.setColour (juce::Colour (0xffDDCCAA));
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        g.drawText (btn.getButtonText(), btn.getLocalBounds(),
                    juce::Justification::centred, true);
    }
};

//==============================================================================
struct LabelledKnob : public juce::Component
{
    juce::Slider slider;
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    LabelledKnob (const juce::String& labelText,
                  juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramId,
                  juce::LookAndFeel* laf)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        slider.setLookAndFeel (laf);
        addAndMakeVisible (slider);

        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
        addAndMakeVisible (label);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                     (apvts, paramId, slider);
    }

    ~LabelledKnob() override
    {
        slider.setLookAndFeel (nullptr);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds  (area.removeFromBottom (18));
        slider.setBounds (area);
    }
};

//==============================================================================
// Vertical input level meter, timer-driven at 30 Hz
class InputMeter : public juce::Component, private juce::Timer
{
public:
    explicit InputMeter (ImperialProcessor& p) : processor (p) { startTimerHz (30); }
    ~InputMeter() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff111111));
        g.fillRoundedRectangle (bounds, 2.0f);
        g.setColour (juce::Colour (0xff222222));
        g.drawRoundedRectangle (bounds, 2.0f, 1.0f);

        const float innerX      = bounds.getX() + 2.0f;
        const float innerW      = bounds.getWidth() - 4.0f;
        const float innerBottom = bounds.getBottom() - 2.0f;
        const float innerH      = bounds.getHeight() - 4.0f;

        constexpr float minDb = -60.0f, maxDb = 0.0f;
        const float peakDb = displayLevel > 1e-6f
                             ? juce::Decibels::gainToDecibels (displayLevel) : minDb;
        const float norm = juce::jlimit (0.0f, 1.0f, (peakDb - minDb) / (maxDb - minDb));
        const float barH = norm * innerH;

        if (barH > 0.0f)
        {
            juce::ColourGradient grad (juce::Colour (0xff22cc44), innerX, innerBottom,
                                       juce::Colour (0xffff2222), innerX, innerBottom - innerH, false);
            grad.addColour (0.75, juce::Colour (0xffC8860A));
            g.setGradientFill (grad);
            g.fillRoundedRectangle (innerX, innerBottom - barH, innerW, barH, 1.5f);
        }

        const float tickY = innerBottom - ((-18.0f - minDb) / (maxDb - minDb)) * innerH;
        g.setColour (juce::Colour (0x66ffffff));
        g.drawLine (innerX, tickY, innerX + innerW, tickY, 1.0f);
    }

private:
    ImperialProcessor& processor;
    float displayLevel = 0.0f;

    void timerCallback() override
    {
        const float newLevel = processor.inputPeakLevel.load (std::memory_order_relaxed);
        if (std::abs (newLevel - displayLevel) > 1e-6f)
        {
            displayLevel = newLevel;
            repaint();
        }
    }
};

//==============================================================================
// IR loader widget: load button + filename label, timer-polls processor for name changes
class IRLoaderWidget : public juce::Component, private juce::Timer
{
public:
    explicit IRLoaderWidget (ImperialProcessor& p, juce::LookAndFeel* laf)
        : processor (p)
    {
        loadButton.setButtonText ("LOAD IR");
        loadButton.setLookAndFeel (laf);
        loadButton.onClick = [this] { browseForIR(); };
        addAndMakeVisible (loadButton);

        clearButton.setButtonText ("X");
        clearButton.setLookAndFeel (laf);
        clearButton.onClick = [this]
        {
            processor.clearIR();
            updateLabel();
        };
        addAndMakeVisible (clearButton);

        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setFont (juce::FontOptions (9.5f));
        nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xffAA9966));
        addAndMakeVisible (nameLabel);

        updateLabel();
        startTimerHz (4);   // low-rate poll — just to pick up state-restore updates
    }

    ~IRLoaderWidget() override { stopTimer(); loadButton.setLookAndFeel (nullptr); clearButton.setLookAndFeel (nullptr); }

    void resized() override
    {
        auto area = getLocalBounds();
        // Bottom: name label
        nameLabel.setBounds (area.removeFromBottom (18));
        // Remaining: load button (most of width) + clear button (small square)
        clearButton.setBounds (area.removeFromRight (22));
        area.removeFromRight (2);  // gap
        loadButton.setBounds (area);
    }

private:
    ImperialProcessor& processor;
    juce::TextButton loadButton, clearButton;
    juce::Label      nameLabel;
    juce::String     lastKnownName;

    void updateLabel()
    {
        if (processor.hasUserIR())
            nameLabel.setText (processor.getLoadedIRName(), juce::dontSendNotification);
        else
            nameLabel.setText ("Built-in IR", juce::dontSendNotification);

        clearButton.setVisible (processor.hasUserIR());
    }

    void timerCallback() override
    {
        const auto name = processor.getLoadedIRName();
        if (name != lastKnownName)
        {
            lastKnownName = name;
            updateLabel();
        }
    }

    void browseForIR()
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            "Load Cabinet IR",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff");

        chooser->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser] (const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result.existsAsFile())
                {
                    if (! processor.loadIR (result))
                        juce::AlertWindow::showMessageBoxAsync (
                            juce::AlertWindow::WarningIcon, "IR Load Failed",
                            "Could not read " + result.getFileName() + ".\n"
                            "Make sure it is a valid WAV or AIFF file.");
                    updateLabel();
                }
            });
    }
};

//==============================================================================
class ImperialEditor : public juce::AudioProcessorEditor
{
public:
    explicit ImperialEditor (ImperialProcessor&);
    ~ImperialEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    ImperialProcessor& imperialProcessor;
    ImperialLookAndFeel laf;

    std::unique_ptr<LabelledKnob> knobGain, knobBass, knobMid, knobTreble, knobPresence;
    std::unique_ptr<LabelledKnob> knobPower, knobSag;
    std::unique_ptr<LabelledKnob> knobMaster;

    IRLoaderWidget irLoader;
    InputMeter     inputMeter;

    void drawSectionLabel (juce::Graphics& g, juce::Rectangle<int> area,
                           const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImperialEditor)
};
