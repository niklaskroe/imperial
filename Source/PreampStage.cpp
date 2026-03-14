#include "PreampStage.h"

PreampStage::PreampStage() {}

void PreampStage::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 1 };

    inputHP.prepare      (spec);
    bassShelf.prepare    (spec);
    trebleShelf.prepare  (spec);
    midCut.prepare       (spec);
    presencePeak.prepare (spec);
    dcBlock.prepare      (spec);

    // Oversampler: prepare at base rate/block size — internally works at 2× rate
    oversampler1.initProcessing ((size_t) samplesPerBlock);
    oversampler2.initProcessing ((size_t) samplesPerBlock);

    reset();
    updateFilters();
}

void PreampStage::reset()
{
    inputHP.reset();
    bassShelf.reset();
    trebleShelf.reset();
    midCut.reset();
    presencePeak.reset();
    dcBlock.reset();
    oversampler1.reset();
    oversampler2.reset();
}

// Gain knob 1-10 → 0–36 dB (exponential feel)
void PreampStage::setGain (float knob)
{
    // Map 1-10 to 0-36 dB with slight exponential curve so lower values feel usable
    const float norm = (knob - 1.0f) / 9.0f;           // 0..1
    currentGainDb = norm * norm * 36.0f;                 // quadratic: 0 dB at 1, 36 dB at 10
}

// Suppress -Wfloat-equal: these comparisons are intentional — we only dirty
// the filter when a value from the processor's APVTS actually changes.
// The incoming float is normalized by the same formula on every call, so
// bitwise equality is reliable for "has this param moved since last block?"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
void PreampStage::setBass     (float v) { if (v != currentBass)     { currentBass     = v; filtersDirty = true; } }
void PreampStage::setMid      (float v) { if (v != currentMid)      { currentMid      = v; filtersDirty = true; } }
void PreampStage::setTreble   (float v) { if (v != currentTreble)   { currentTreble   = v; filtersDirty = true; } }
void PreampStage::setPresence (float v) { if (v != currentPresence) { currentPresence = v; filtersDirty = true; } }
#pragma clang diagnostic pop

void PreampStage::updateFilters()
{
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    // Input high-pass: 80 Hz — removes sub-bass / rumble before the gain stage
    *inputHP.coefficients = *Coeffs::makeHighPass (sampleRate, 80.0f, 0.7f);

    // DC block between stages: 20 Hz, very low Q — just kills DC offset from saturation
    *dcBlock.coefficients = *Coeffs::makeHighPass (sampleRate, 20.0f, 0.5f);

    // --- Fender-style passive tone stack approximation ---
    //
    // Real network behaviour:
    //   Bass pot: low-shelf centred ~100 Hz. At max (+) it boosts lows;
    //             at min it cuts. Range ±10 dB.
    //   Mid pot:  controls depth of a passive mid scoop (~650 Hz).
    //             Mid=0 → deepest cut (-12 dB). Mid=1 → nearly flat (~-2 dB).
    //             It is NEVER a boost — it only controls the scoop depth.
    //   Treble:   high-shelf from ~2 kHz. At max boosts; at min cuts. Range ±10 dB.
    //
    // Using separate biquads to approximate the interaction:

    // Bass shelf: 100 Hz, Q=0.7, range -10 to +10 dB
    const float bassDb = (currentBass - 0.5f) * 20.0f;
    *bassShelf.coefficients = *Coeffs::makeLowShelf (
        sampleRate, 100.0f, 0.7f,
        juce::Decibels::decibelsToGain (bassDb));

    // Treble shelf: 2000 Hz, Q=0.7, range -10 to +10 dB
    const float trebleDb = (currentTreble - 0.5f) * 20.0f;
    *trebleShelf.coefficients = *Coeffs::makeHighShelf (
        sampleRate, 2000.0f, 0.7f,
        juce::Decibels::decibelsToGain (trebleDb));

    // Mid scoop: 650 Hz, Q=1.8, always a cut
    // At mid=0 → -12 dB (deepest scoop); at mid=1 → -1 dB (nearly flat)
    const float midCutDb = -1.0f - (1.0f - currentMid) * 11.0f;   // -1 to -12 dB
    *midCut.coefficients = *Coeffs::makePeakFilter (
        sampleRate, 650.0f, 1.8f,
        juce::Decibels::decibelsToGain (midCutDb));

    // Presence: upper-mid peak at 2.5 kHz, Q=1.5, 0..+8 dB
    const float presenceDb = currentPresence * 8.0f;
    *presencePeak.coefficients = *Coeffs::makePeakFilter (
        sampleRate, 2500.0f, 1.5f,
        juce::Decibels::decibelsToGain (presenceDb));

    filtersDirty = false;
}

void PreampStage::process (juce::dsp::AudioBlock<float>& block)
{
    if (filtersDirty)
        updateFilters();

    float* data = block.getChannelPointer (0);
    const int numSamples = (int) block.getNumSamples();

    // 1. Input HP — kills rumble before the drive stage
    for (int i = 0; i < numSamples; ++i)
        data[i] = inputHP.processSample (data[i]);

    // 2. First 12AX7 gain stage — upsampled to 2× to reduce aliasing from tanh
    const float gainLinear = juce::Decibels::decibelsToGain (currentGainDb);
    const float drive1     = (currentGainDb / 36.0f) * 0.7f;    // 0..0.7

    {
        auto oversampledBlock = oversampler1.processSamplesUp (block);
        float* os = oversampledBlock.getChannelPointer (0);
        const int osN = (int) oversampledBlock.getNumSamples();

        for (int i = 0; i < osN; ++i)
        {
            float s = os[i] * gainLinear;
            os[i] = tubeSaturate (s, drive1);
        }

        oversampler1.processSamplesDown (block);
    }

    // 3. Passive tone stack (bass shelf + mid scoop + treble shelf)
    for (int i = 0; i < numSamples; ++i)
    {
        float s = data[i];
        s = bassShelf.processSample   (s);
        s = midCut.processSample      (s);
        s = trebleShelf.processSample (s);
        data[i] = s;
    }

    // 4. DC block — removes any DC offset introduced by asymmetric saturation
    for (int i = 0; i < numSamples; ++i)
        data[i] = dcBlock.processSample (data[i]);

    // 5. Second 12AX7 stage — upsampled to 2×, lighter drive, adds upper harmonics
    const float drive2 = drive1 * 0.55f;

    {
        auto oversampledBlock = oversampler2.processSamplesUp (block);
        float* os = oversampledBlock.getChannelPointer (0);
        const int osN = (int) oversampledBlock.getNumSamples();

        for (int i = 0; i < osN; ++i)
            os[i] = tubeSaturate (os[i], drive2);

        oversampler2.processSamplesDown (block);
    }

    // 6. Presence peak at the phase inverter node
    for (int i = 0; i < numSamples; ++i)
        data[i] = presencePeak.processSample (data[i]);

    // 7. Output trim: compensate for the gain stage boost.
    //    tubeSaturate already normalises by 1/k, so headroom is good.
    //    -6 dB gives the power amp a hot but unclipped feed.
    const float outTrim = juce::Decibels::decibelsToGain (-6.0f);
    for (int i = 0; i < numSamples; ++i)
        data[i] *= outTrim;
}
