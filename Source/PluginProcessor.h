// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <JuceHeader.h>
#include "PreampStage.h"
#include "PowerAmpStage.h"
#include "CabinetStage.h"

//==============================================================================
class ImperialProcessor : public juce::AudioProcessor
{
public:
    ImperialProcessor();
    ~ImperialProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "Imperial"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return cabinet.getIRDurationSeconds(); }

    //==========================================================================
    int getNumPrograms()    override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    // Thread-safe input peak level for the UI meter (written audio thread, read UI thread)
    std::atomic<float> inputPeakLevel { 0.0f };

    // IR loader forwarding — call from the message thread
    bool         loadIR  (const juce::File& f) { return cabinet.loadIR (f); }
    void         clearIR ()                     { cabinet.clearIR(); }
    bool         hasUserIR()       const        { return cabinet.hasUserIR(); }
    juce::String getLoadedIRName() const        { return cabinet.getLoadedIRName(); }
    juce::String getLoadedIRFilePath() const    { return cabinet.getLoadedIRFilePath(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    PreampStage  preamp;
    PowerAmpStage powerAmp;
    CabinetStage  cabinet;

    // Output volume (master)
    juce::dsp::Gain<float> masterGain;

    // Peak hold decay coefficient (per-block, set in prepareToPlay)
    float peakDecayCoeff = 0.0f;

    // Pre-allocated processing buffers (avoids heap allocation on the audio thread)
    juce::AudioBuffer<float> monoBuffer;
    juce::AudioBuffer<float> stereoBuffer;

    // Input buffer resampled to mono for DSP, then mirrored back to stereo
    void updateParametersFromAPVTS();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImperialProcessor)
};
