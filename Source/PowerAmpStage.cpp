// SPDX-License-Identifier: GPL-3.0-only
#include "PowerAmpStage.h"

PowerAmpStage::PowerAmpStage() {}

void PowerAmpStage::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 1 };

    outputHP.prepare (spec);
    oversampler.initProcessing ((size_t) samplesPerBlock);

    reset();
    updateCoefficients();
}

void PowerAmpStage::reset()
{
    outputHP.reset();
    oversampler.reset();
    sagState = 0.0f;
}

void PowerAmpStage::setDrive (float v) { currentDrive = v; }
void PowerAmpStage::setSag   (float v) { currentSag   = v; updateCoefficients(); }

void PowerAmpStage::updateCoefficients()
{
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    // Output transformer coupling cap — blocks DC / sub-bass below 30 Hz
    // The cabinet IR handles all frequency shaping above that.
    *outputHP.coefficients = *Coeffs::makeHighPass (sampleRate, 30.0f, 0.7f);

    // Sag envelope: 8 ms attack (PSU responds quickly to peaks), 300 ms release
    sagAttackCoeff  = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.008));
    sagReleaseCoeff = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.3));
}

void PowerAmpStage::process (juce::dsp::AudioBlock<float>& block)
{
    // Power amp is fed a mono block (1 channel) from the preamp
    float* data = block.getChannelPointer (0);
    const int numSamples = (int) block.getNumSamples();

    // drive 0..1 → internal k: at 0 it's a mild push, at 1 it clips hard
    const float drive     = 0.3f + currentDrive * 0.7f;
    const float sagAmount = currentSag * 0.35f;    // max sag: 35% gain reduction at full signal

    // 1. Power supply sag — envelope follower at base rate (no aliasing concern)
    for (int i = 0; i < numSamples; ++i)
    {
        const float absX   = std::abs (data[i]);
        const float eCoeff = absX > sagState ? sagAttackCoeff : sagReleaseCoeff;
        sagState += eCoeff * (absX - sagState);
        data[i] *= 1.0f / (1.0f + sagAmount * sagState * 4.0f);
    }

    // 2+3. EL84 + transformer saturation — upsampled to 2× to reduce aliasing
    {
        auto oversampledBlock = oversampler.processSamplesUp (block);
        float* os = oversampledBlock.getChannelPointer (0);
        const int osN = (int) oversampledBlock.getNumSamples();

        for (int i = 0; i < osN; ++i)
            os[i] = transformerSat (el84Clip (os[i], drive));

        oversampler.processSamplesDown (block);
    }

    // 4. Output transformer HP (DC block / coupling)
    for (int i = 0; i < numSamples; ++i)
        data[i] = outputHP.processSample (data[i]);
}
