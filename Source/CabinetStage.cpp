#include "CabinetStage.h"
#include <cmath>

CabinetStage::CabinetStage() {}

//==============================================================================
juce::AudioBuffer<float> CabinetStage::generateCabinetIR (double sr)
{
    // Synthesise a 1x12 Celestion Alnico Blue IR as a minimum-phase FIR.
    // The IR is created by passing a unit impulse through a cascade of IIR filters
    // that model the cab's known EQ characteristic, then windowing.
    //
    // Key targets from published Alnico Blue measurements:
    //   - Open-back cancellation: -24 dB/oct below ~80 Hz
    //   - Cone breakup resonance: +3 dB @ 130 Hz, Q=3
    //   - Smooth midrange — NO peaks between 400-1500 Hz
    //   - Alnico magnet warmth / upper-mid presence: +2 dB @ 2.5 kHz
    //   - Natural treble rolloff: -12 dB/oct above 6.5 kHz

    const int irLength = 1024;      // shorter IR = less latency + less low-freq smear
    juce::AudioBuffer<float> ir (1, irLength);
    ir.clear();
    ir.setSample (0, 0, 1.0f);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) irLength, 1 };

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    // Open-back high-pass: two poles model rear-wave cancellation
    Filter hp1; hp1.prepare (spec);
    *hp1.coefficients = *Coeffs::makeHighPass (sr, 80.0f, 0.9f);

    Filter hp2; hp2.prepare (spec);
    *hp2.coefficients = *Coeffs::makeHighPass (sr, 55.0f, 0.6f);

    // Cone / box resonance: narrow peak at 130 Hz
    Filter res; res.prepare (spec);
    *res.coefficients = *Coeffs::makePeakFilter (sr, 130.0f, 3.0f,
                                                  juce::Decibels::decibelsToGain (3.0f));

    // Alnico upper-mid bloom
    Filter bloom; bloom.prepare (spec);
    *bloom.coefficients = *Coeffs::makePeakFilter (sr, 2500.0f, 1.6f,
                                                    juce::Decibels::decibelsToGain (2.0f));

    // Natural Celestion treble rolloff — one LP pole at 6.5 kHz is enough
    Filter lp; lp.prepare (spec);
    *lp.coefficients = *Coeffs::makeLowPass (sr, 6500.0f, 0.7f);

    float* data = ir.getWritePointer (0);
    for (int i = 0; i < irLength; ++i)
    {
        float s = data[i];
        s = hp1.processSample   (s);
        s = hp2.processSample   (s);
        s = res.processSample   (s);
        s = bloom.processSample (s);
        s = lp.processSample    (s);
        data[i] = s;
    }

    // Tukey window — flat top (α=0.5) preserves the direct-sound attack,
    // cosine taper only on the final 25% of each end to smooth the tail.
    // This avoids the Hanning w[0]=0 problem that destroys transient snap.
    const float alpha = 0.5f;   // fraction of window occupied by cosine tapers (total)
    const float halfAlpha = alpha * 0.5f;
    for (int i = 0; i < irLength; ++i)
    {
        const float t = (float) i / (float) (irLength - 1);   // 0..1
        float w = 1.0f;
        if (t < halfAlpha)
            w = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::pi * t / halfAlpha));
        else if (t > 1.0f - halfAlpha)
            w = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::pi * (1.0f - t) / halfAlpha));
        data[i] *= w;
    }

    // Normalise to peak = 0.5
    float peak = 0.0f;
    for (int i = 0; i < irLength; ++i)
        peak = std::max (peak, std::abs (data[i]));
    if (peak > 1e-6f)
        for (int i = 0; i < irLength; ++i)
            data[i] *= 0.5f / peak;

    return ir;
}

//==============================================================================
void CabinetStage::loadSynthIR()
{
    auto ir = generateCabinetIR (sampleRate);
    convolution.loadImpulseResponse (std::move (ir),
                                     sampleRate,
                                     juce::dsp::Convolution::Stereo::no,
                                     juce::dsp::Convolution::Trim::no,
                                     juce::dsp::Convolution::Normalise::yes);
    userIRLoaded = false;
    loadedIRName = {};
    irDurationSeconds = 1024.0 / sampleRate;   // built-in IR is always 1024 samples
}

//==============================================================================
void CabinetStage::prepare (double sr, int samplesPerBlock)
{
    sampleRate             = sr;
    preparedSamplesPerBlock = samplesPerBlock;

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 2 };
    convolution.prepare (spec);

    loadSynthIR();
}

void CabinetStage::reset()
{
    convolution.reset();
}

//==============================================================================
bool CabinetStage::loadIR (const juce::File& irFile)
{
    if (! irFile.existsAsFile())
        return false;

    const auto ext = irFile.getFileExtension().toLowerCase();
    if (ext != ".wav" && ext != ".aif" && ext != ".aiff")
        return false;

    // Read the file via JUCE's AudioFormatManager
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();   // registers WAV and AIFF

    std::unique_ptr<juce::AudioFormatReader> reader (
        formatManager.createReaderFor (irFile));

    if (reader == nullptr)
        return false;

    // Limit IR to 4 seconds to avoid excessive memory / latency
    const int maxSamples = (int) std::min (
        reader->lengthInSamples,
        (juce::int64) (reader->sampleRate * 4.0));

    if (maxSamples <= 0)
        return false;

    juce::AudioBuffer<float> irBuffer ((int) reader->numChannels, maxSamples);
    reader->read (&irBuffer, 0, maxSamples, 0, true, true);

    convolution.loadImpulseResponse (std::move (irBuffer),
                                     reader->sampleRate,
                                     juce::dsp::Convolution::Stereo::yes,
                                     juce::dsp::Convolution::Trim::yes,
                                     juce::dsp::Convolution::Normalise::yes);

    userIRLoaded  = true;
    loadedIRName  = irFile.getFileNameWithoutExtension();
    loadedIRPath  = irFile.getFullPathName();
    irDurationSeconds = maxSamples / reader->sampleRate;
    return true;
}

void CabinetStage::clearIR()
{
    loadSynthIR();
}

//==============================================================================
void CabinetStage::process (juce::dsp::AudioBlock<float>& block)
{
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    convolution.process (ctx);
}
