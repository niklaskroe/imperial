#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
ImperialProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // All "knob" controls use 1-10 range like a real amp
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "gain", "Gain", juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 4.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "bass", "Bass", juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 5.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mid", "Mid", juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 5.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "treble", "Treble", juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 5.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "presence", "Presence", juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 4.0f));

    // Power amp controls
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "powerDrive", "Power Drive", juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 5.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sag", "Sag", juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 4.0f));

    // Master volume — stays as a dB trim, 1-10 maps to -20..+6 dB
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "master", "Master", juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 5.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
ImperialProcessor::ImperialProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

ImperialProcessor::~ImperialProcessor() {}

//==============================================================================
void ImperialProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    preamp.prepare   (sampleRate, samplesPerBlock);
    powerAmp.prepare (sampleRate, samplesPerBlock);
    cabinet.prepare  (sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec stereoSpec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    masterGain.prepare (stereoSpec);
    masterGain.setRampDurationSeconds (0.01);

    // Peak decay: ~1.5 s half-life, computed per block
    const double blocksPerSecond = sampleRate / std::max (samplesPerBlock, 1);
    peakDecayCoeff = (float) std::pow (0.5, 1.5 / blocksPerSecond);

    // Pre-allocate processing buffers (avoids per-block heap allocation on the audio thread)
    monoBuffer.setSize   (1, samplesPerBlock, false, true, false);
    stereoBuffer.setSize (2, samplesPerBlock, false, true, false);

    // Report convolution latency so the host can apply PDC
    setLatencySamples (cabinet.getLatencySamples());
}

void ImperialProcessor::releaseResources()
{
    preamp.reset();
    powerAmp.reset();
    cabinet.reset();
}

//==============================================================================
void ImperialProcessor::updateParametersFromAPVTS()
{
    // All knob params are 1-10; normalise to 0..1 for DSP setters
    auto norm = [] (float v) { return (v - 1.0f) / 9.0f; };

    preamp.setGain     (*apvts.getRawParameterValue ("gain"));       // passed as 1-10, remapped inside
    preamp.setBass     (norm (*apvts.getRawParameterValue ("bass")));
    preamp.setMid      (norm (*apvts.getRawParameterValue ("mid")));
    preamp.setTreble   (norm (*apvts.getRawParameterValue ("treble")));
    preamp.setPresence (norm (*apvts.getRawParameterValue ("presence")));

    powerAmp.setDrive  (norm (*apvts.getRawParameterValue ("powerDrive")));
    powerAmp.setSag    (norm (*apvts.getRawParameterValue ("sag")));

    // Master: 1-10 → -12..+12 dB. True unity (0 dB) is at knob ≈ 5.5.
    // Position 5 gives ≈ -1.3 dB (close enough to unity for practical use).
    const float masterNorm = norm (*apvts.getRawParameterValue ("master"));
    masterGain.setGainDecibels (-12.0f + masterNorm * 24.0f);
}

//==============================================================================
bool ImperialProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Must have at least one output channel
    if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;

    // Accept mono or stereo only
    const auto& outSet = layouts.getMainOutputChannelSet();
    if (outSet != juce::AudioChannelSet::mono() &&
        outSet != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output, or be disabled (for output-only use)
    const auto& inSet = layouts.getMainInputChannelSet();
    if (inSet != juce::AudioChannelSet::disabled() && inSet != outSet)
        return false;

    return true;
}

//==============================================================================
void ImperialProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    updateParametersFromAPVTS();

    const int totalCh    = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear unused output channels
    for (int ch = buffer.getNumChannels(); ch < totalCh; ++ch)
        buffer.clear (ch, 0, numSamples);

    // --- Measure input peak before DSP (thread-safe atomic, decayed per block) ---
    {
        float blockPeak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            blockPeak = std::max (blockPeak, buffer.getMagnitude (ch, 0, numSamples));
        // Decay the stored peak and blend in the new block peak
        float current = inputPeakLevel.load (std::memory_order_relaxed);
        current *= peakDecayCoeff;
        if (blockPeak > current) current = blockPeak;
        inputPeakLevel.store (current, std::memory_order_relaxed);
    }

    // --- Mono DSP path ---
    // Mix stereo input to mono, process, then copy mono back to both channels.
    // This matches a real amp (one signal path).
    monoBuffer.setSize (1, numSamples, false, false, true);  // resize without re-alloc if smaller
    monoBuffer.clear();

    const int inputCh = buffer.getNumChannels();
    for (int ch = 0; ch < inputCh; ++ch)
        monoBuffer.addFrom (0, 0, buffer, ch, 0, numSamples,
                            1.0f / (float) std::max (inputCh, 1));

    // Process through amp stages
    {
        juce::dsp::AudioBlock<float> monoBlock (monoBuffer);

        preamp.process   (monoBlock);
        powerAmp.process (monoBlock);
    }

    // Cabinet processes internally in stereo (copies mono to both channels)
    stereoBuffer.setSize (2, numSamples, false, false, true);
    for (int ch = 0; ch < 2; ++ch)
        stereoBuffer.copyFrom (ch, 0, monoBuffer, 0, 0, numSamples);

    {
        juce::dsp::AudioBlock<float> stereoBlock (stereoBuffer);
        cabinet.process (stereoBlock);
    }

    // Copy processed stereo to output buffer
    const int outCh = std::min (buffer.getNumChannels(), 2);
    for (int ch = 0; ch < outCh; ++ch)
        buffer.copyFrom (ch, 0, stereoBuffer, ch, 0, numSamples);

    // Master gain
    juce::dsp::AudioBlock<float> outBlock (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (outBlock);
    masterGain.process (ctx);
}

//==============================================================================
void ImperialProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Persist the loaded IR file path (if any) as an attribute on the state root
    if (cabinet.hasUserIR())
        state.setProperty ("irFilePath", cabinet.getLoadedIRFilePath(), nullptr);
    else
        state.removeProperty ("irFilePath", nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ImperialProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    apvts.replaceState (state);

    // Restore the IR file path if it was saved.
    // loadIR does disk I/O so we defer it to the message thread in case
    // setStateInformation is called on the audio thread (e.g. in Logic Pro).
    if (state.hasProperty ("irFilePath"))
    {
        juce::String savedPath = state["irFilePath"].toString();
        juce::MessageManager::callAsync ([this, savedPath]
        {
            juce::File irFile (savedPath);
            if (irFile.existsAsFile())
                cabinet.loadIR (irFile);
        });
    }
}

//==============================================================================
juce::AudioProcessorEditor* ImperialProcessor::createEditor()
{
    return new ImperialEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImperialProcessor();
}
