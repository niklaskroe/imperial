// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <JuceHeader.h>

//==============================================================================
// Imperial - Power Amp Stage
// Models EL84 push-pull Class A power section:
//   - Transformer input saturation (phase inverter)
//   - EL84 pentode soft clipping (2× oversampled to reduce aliasing)
//   - Output transformer saturation / magnetic hysteresis approximation
//   - Dynamic sag (power supply compression)
//==============================================================================

class PowerAmpStage
{
public:
    PowerAmpStage();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    void setDrive (float driveNorm);   // 0..1 — power amp drive/push
    void setSag   (float sagNorm);     // 0..1 — power supply sag amount

private:
    double sampleRate = 44100.0;
    float currentDrive = 0.5f;
    float currentSag   = 0.5f;

    // Sag envelope follower state
    float sagState = 0.0f;
    float sagAttackCoeff  = 0.0f;
    float sagReleaseCoeff = 0.0f;

    // Output transformer HPF (30 Hz, blocks DC from saturation)
    // No LP here — the cabinet IR handles all frequency shaping above 30 Hz
    juce::dsp::IIR::Filter<float> outputHP;

    // 2× oversampler around EL84 + transformer saturation nonlinearities
    juce::dsp::Oversampling<float> oversampler { 1, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };

    void updateCoefficients();

    // EL84 transfer function approximation:
    // Uses an asymmetric soft-clip with odd/even harmonics
    static inline float el84Clip (float x, float drive)
    {
        const float k = 1.0f + drive * 3.0f;
        // Asymmetry: negative half clips harder (class A bias shift)
        float pos = std::tanh (k * x);
        float neg = std::tanh (k * x * 1.15f);
        // Smooth crossfade between pos and neg (not a hard switch)
        float blend = 0.5f + 0.5f * std::tanh (x * 10.0f);
        return blend * pos + (1.0f - blend) * neg;
    }

    // Output transformer saturation (magnetic hysteresis approximation)
    static inline float transformerSat (float x)
    {
        // Soft-knee hard clip approximating transformer core saturation
        const float knee = 0.8f;
        if (std::abs (x) < knee)
            return x;
        float sign = x > 0.0f ? 1.0f : -1.0f;
        float over = std::abs (x) - knee;
        return sign * (knee + over / (1.0f + over * 2.0f));
    }
};
