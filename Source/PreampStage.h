// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <JuceHeader.h>

//==============================================================================
// Imperial - Preamp Stage
// Models a 12AX7-based preamp with a passive Fender-style tone stack.
//
// Signal path:
//   input → HP (80 Hz) → 2× oversample → gain + tube sat → downsample
//         → Fender tone stack → DC block
//         → 2× oversample → 2nd tube sat → downsample
//         → presence → output trim
//
// Tone stack is a passive RC network approximation (James/Fender topology):
//   - Bass: low-shelf boost (5 Hz–300 Hz), centred at 100 Hz
//   - Mid:  parametric cut at ~650 Hz (mid pot controls depth of scoop)
//   - Treble: high-shelf boost above 2 kHz
//   All three interact via the passive network characteristic.
//==============================================================================

class PreampStage
{
public:
    PreampStage();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    // All setters receive normalised 0..1 (already remapped from 1-10 by the processor)
    // except setGain which receives the raw 1-10 value
    void setGain     (float knob1to10);
    void setBass     (float norm);   // 0..1
    void setMid      (float norm);   // 0..1
    void setTreble   (float norm);   // 0..1
    void setPresence (float norm);   // 0..1

private:
    // Fender-style passive tone stack (cascaded biquads)
    juce::dsp::IIR::Filter<float> bassShelf;
    juce::dsp::IIR::Filter<float> trebleShelf;
    juce::dsp::IIR::Filter<float> midCut;      // parametric cut — mid is a scoop

    // Presence peak at phase inverter
    juce::dsp::IIR::Filter<float> presencePeak;

    // Input HP (anti-rumble / coupling cap sim)
    juce::dsp::IIR::Filter<float> inputHP;

    // DC block between stages (very low HP, just removes DC offset from saturation)
    juce::dsp::IIR::Filter<float> dcBlock;

    // 2× oversamplers — one per saturation stage to avoid shared state corruption.
    // Using polyphase IIR filters for low latency.
    juce::dsp::Oversampling<float> oversampler1 { 1, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::dsp::Oversampling<float> oversampler2 { 1, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };

    double sampleRate    = 44100.0;
    float currentBass    = 0.5f;
    float currentMid     = 0.5f;
    float currentTreble  = 0.5f;
    float currentPresence= 0.5f;
    float currentGainDb  = 18.0f;   // internal dB value (remapped from 1-10)
    bool  filtersDirty   = true;

    void updateFilters();

    // 12AX7 soft saturation — input is pre-scaled, drive controls harmonic content
    // Uses asymmetric tanh: negative rail clips slightly harder (grid conduction asymmetry)
    static inline float tubeSaturate (float x, float drive) noexcept
    {
        const float k = 1.0f + drive * 3.5f;
        // Positive half: normal tanh
        // Negative half: slightly harder clip (1.1× k)
        if (x >= 0.0f)
            return std::tanh (k * x) / k;   // normalise so unity-gain at low drive
        else
            return std::tanh (k * 1.1f * x) / (k * 1.1f);
    }
};
