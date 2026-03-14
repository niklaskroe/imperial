// SPDX-License-Identifier: GPL-3.0-only
#include "PluginEditor.h"

ImperialEditor::ImperialEditor (ImperialProcessor& p)
    : AudioProcessorEditor (&p), imperialProcessor (p),
      irLoader (p, &laf), inputMeter (p)
{
    setLookAndFeel (&laf);

    auto& apvts = imperialProcessor.apvts;

    knobGain     = std::make_unique<LabelledKnob> ("GAIN",     apvts, "gain",       &laf);
    knobBass     = std::make_unique<LabelledKnob> ("BASS",     apvts, "bass",       &laf);
    knobMid      = std::make_unique<LabelledKnob> ("MID",      apvts, "mid",        &laf);
    knobTreble   = std::make_unique<LabelledKnob> ("TREBLE",   apvts, "treble",     &laf);
    knobPresence = std::make_unique<LabelledKnob> ("PRESENCE", apvts, "presence",   &laf);
    knobPower    = std::make_unique<LabelledKnob> ("POWER",    apvts, "powerDrive", &laf);
    knobSag      = std::make_unique<LabelledKnob> ("SAG",      apvts, "sag",        &laf);
    knobMaster   = std::make_unique<LabelledKnob> ("MASTER",   apvts, "master",     &laf);

    addAndMakeVisible (*knobGain);
    addAndMakeVisible (*knobBass);
    addAndMakeVisible (*knobMid);
    addAndMakeVisible (*knobTreble);
    addAndMakeVisible (*knobPresence);
    addAndMakeVisible (*knobPower);
    addAndMakeVisible (*knobSag);
    addAndMakeVisible (irLoader);
    addAndMakeVisible (*knobMaster);
    addAndMakeVisible (inputMeter);

    // Window: 846px wide — knobs occupy x=8..814, meter at x=822 width=18
    setSize (846, 230);
}

ImperialEditor::~ImperialEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void ImperialEditor::drawSectionLabel (juce::Graphics& g,
                                               juce::Rectangle<int> area,
                                               const juce::String& text)
{
    g.setColour (juce::Colour (0x44ffffff));
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText (text, area, juce::Justification::centredTop);
}

//==============================================================================
void ImperialEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour (0xff1a1a1a));

    // Top banner
    g.setColour (juce::Colour (0xff111111));
    g.fillRect (0, 0, getWidth(), 28);

    // Brand name — centred over the knob area (x=0..820), not the full window
    g.setColour (juce::Colour (0xffC8860A));
    g.setFont (juce::FontOptions ("Arial", "Bold Italic", 18.0f));
    g.drawText ("Imperial", 0, 0, 820, 28,
                juce::Justification::centred);

    // Section dividers and labels — positions derived from resized() layout:
    // Preamp (5 knobs, x=8..430), divider at 448, Power (x=458..622), divider at 640, Output (x=650..814)
    g.setColour (juce::Colour (0xff333322));
    g.fillRect (448, 30, 1, getHeight() - 36);
    g.fillRect (640, 30, 1, getHeight() - 36);

    const int labelY = 32;
    drawSectionLabel (g, {   0, labelY, 448, 14 }, "PREAMP");
    drawSectionLabel (g, { 449, labelY, 191, 14 }, "POWER");
    drawSectionLabel (g, { 641, labelY, 179, 14 }, "OUTPUT");

    // "IN" label above the meter
    g.setColour (juce::Colour (0x88ffffff));
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("IN", 820, 30, 26, 12, juce::Justification::centred);
}

//==============================================================================
void ImperialEditor::resized()
{
    // 9 knobs × 78px wide with 8px margins
    const int knobW  = 78;
    const int knobH  = 95;
    const int startY = 48;
    const int gap    = 8;  // gap between knobs within a section
    const int sectionGap = 20; // extra gap between sections

    // Preamp: Gain, Bass, Mid, Treble, Presence
    int x = gap;
    knobGain    ->setBounds (x, startY, knobW, knobH); x += knobW + gap;
    knobBass    ->setBounds (x, startY, knobW, knobH); x += knobW + gap;
    knobMid     ->setBounds (x, startY, knobW, knobH); x += knobW + gap;
    knobTreble  ->setBounds (x, startY, knobW, knobH); x += knobW + gap;
    knobPresence->setBounds (x, startY, knobW, knobH); x += knobW + gap + sectionGap;

    // Power: Power Drive, Sag
    knobPower->setBounds (x, startY, knobW, knobH); x += knobW + gap;
    knobSag  ->setBounds (x, startY, knobW, knobH); x += knobW + gap + sectionGap;

    // Output: IR loader widget, Master
    // IR loader occupies the same slot as the old CAB MIX knob (78px wide, same height minus bottom label)
    irLoader.setBounds (x, startY, knobW, knobH - 18); x += knobW + gap;
    knobMaster->setBounds (x, startY, knobW, knobH);

    // Input meter: slim vertical bar to the right of all knobs
    // "IN" label is drawn in paint() at y=30..42; meter bar below banner
    inputMeter.setBounds (822, 42, 18, getHeight() - 50);
}
