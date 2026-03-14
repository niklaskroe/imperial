#pragma once
#include <JuceHeader.h>

//==============================================================================
// Imperial - Cabinet Simulation
//
// Implements a 1x12 Celestion Alnico Blue cabinet using partitioned convolution.
//
// IR loading priority:
//   1. User-loaded WAV or AIFF file (via loadIR)
//   2. Built-in synthesised IR (Alnico Blue approximation) when no file is loaded
//
// The synthesised IR models:
//   - Open-back bass cancellation (HP @ 80 Hz + HP @ 55 Hz)
//   - Box/cone resonance peak (+3 dB @ 130 Hz)
//   - Alnico upper-mid bloom (+2 dB @ 2.5 kHz)
//   - Natural Celestion treble rolloff (LP @ 6.5 kHz)
//==============================================================================

class CabinetStage
{
public:
    CabinetStage();
    ~CabinetStage() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    // IR management — call from the message thread only
    bool loadIR  (const juce::File& irFile);   // returns false if load failed
    void clearIR ();                            // revert to synth IR

    bool          hasUserIR()           const { return userIRLoaded; }
    juce::String  getLoadedIRName()     const { return loadedIRName; }
    juce::String  getLoadedIRFilePath() const { return loadedIRPath; }

    // Returns the convolution latency in samples so the processor can report it to the host.
    // Must be called after prepare().
    int           getLatencySamples()   const { return convolution.getLatency(); }

    // Duration of the currently loaded IR in seconds (for getTailLengthSeconds reporting).
    double        getIRDurationSeconds() const { return irDurationSeconds; }

private:
    double sampleRate     = 44100.0;
    bool   userIRLoaded   = false;
    double irDurationSeconds = 0.023;   // synth IR default (1024 samples @ 44.1 kHz ≈ 23 ms)
    juce::String loadedIRName;
    juce::String loadedIRPath;

    juce::dsp::Convolution convolution { juce::dsp::Convolution::NonUniform { 512 } };

    int preparedSamplesPerBlock = 512;

    // Generates the built-in Alnico Blue approximation IR
    static juce::AudioBuffer<float> generateCabinetIR (double sr);

    void loadSynthIR();
};
