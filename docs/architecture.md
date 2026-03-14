# Signal Chain and DSP Architecture

Imperial processes audio in a strictly mono signal path through three stages, then expands to stereo for the cabinet convolution. All internal processing is 32-bit float.

```
Input (stereo)
    │
    ▼
[sum to mono]
    │
    ▼
┌─────────────────────────────────────────────────────┐
│  PreampStage                                        │
│                                                     │
│  1. Input HP @ 80 Hz, Q=0.7  (coupling cap sim)    │
│  2. 12AX7 gain + tube saturation (stage 1)         │
│  3. Fender passive tone stack                       │
│     · Bass shelf  @  100 Hz  ±10 dB                │
│     · Mid scoop   @  650 Hz  -1 to -12 dB          │
│     · Treble shelf@ 2000 Hz  ±10 dB                │
│  4. DC block @ 20 Hz (removes sat offset)          │
│  5. 12AX7 tube saturation (stage 2, lighter)       │
│  6. Presence peak @ 2500 Hz  0..+8 dB              │
│  7. Output trim -6 dB                              │
└─────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────┐
│  PowerAmpStage                                      │
│                                                     │
│  1. Power supply sag  (envelope follower,           │
│     8 ms attack / 300 ms release)                   │
│  2. EL84 push-pull saturation                       │
│  3. Output transformer soft saturation             │
│  4. Transformer coupling HP @ 30 Hz                │
└─────────────────────────────────────────────────────┘
    │
    ▼ mono → copy to stereo
┌─────────────────────────────────────────────────────┐
│  CabinetStage                                       │
│                                                     │
│  Convolution with either:                          │
│  · User-loaded IR (WAV or AIFF, up to 4 s)         │
│  · Built-in synthesised Celestion Alnico Blue IR   │
│    (1024-sample min-phase FIR, Hanning windowed)   │
└─────────────────────────────────────────────────────┘
    │
    ▼
[Master gain  ±12 dB]
    │
    ▼
Output (stereo)
```

---

## Preamp Stage

**Source:** `Source/PreampStage.h`, `Source/PreampStage.cpp`

### Gain mapping

Knob 1–10 maps to 0–36 dB via a quadratic curve, giving finer control at low values:

```
gainDb = ((knob - 1) / 9)²  ×  36
```

At knob 5 ≈ 16 dB of gain. At knob 10 = 36 dB.

### Tube saturation

Models asymmetric grid conduction in a 12AX7:

```cpp
// Positive half: normal tanh
return tanh(k * x) / k;

// Negative half: slightly harder clip (grid conduction asymmetry)
return tanh(k * 1.1 * x) / (k * 1.1);
```

Dividing by `k` normalises gain so the function is approximately unity at low drive — the drive parameter only controls harmonic content, not level. `k = 1 + drive * 3.5`, where `drive` tracks the gain knob from 0 to 0.7.

### Tone stack

Implements a Fender-topology passive tone stack using three cascaded biquad IIR filters:

- **Bass shelf** (100 Hz, Q=0.7): the bass pot sweeps ±10 dB.
- **Mid parametric cut** (650 Hz, Q=1.8): always a cut, never a boost. Mid=10 → −1 dB; Mid=1 → −12 dB. This is correct behaviour — the real network's mid pot only controls scoop depth.
- **Treble shelf** (2 kHz, Q=0.7): the treble pot sweeps ±10 dB.

---

## Power Amp Stage

**Source:** `Source/PowerAmpStage.h`, `Source/PowerAmpStage.cpp`

### Sag

An envelope follower on the rectified signal models power-supply sag. When loud transients hit, the supply voltage sags and the amp compresses:

```
attack  = 8 ms   (PSU responds quickly to peaks)
release = 300 ms (slow recovery — bloom effect)
max sag = 35% gain reduction at full signal, full Sag knob
```

### EL84 push-pull saturation

`el84Clip` models the crossover and clipping characteristic of a class-AB push-pull pair. `transformerSat` applies a mild tanh-based soft saturation representing output transformer core saturation.

---

## Cabinet Stage

**Source:** `Source/CabinetStage.h`, `Source/CabinetStage.cpp`

Uses JUCE's `juce::dsp::Convolution` for zero-latency partitioned convolution.

### Built-in IR

A 1024-sample minimum-phase FIR is synthesised at `prepare()` time by passing a unit impulse through cascaded IIR filters that model the Celestion Alnico Blue's known response:

| Filter | Type | Frequency | Parameters |
|---|---|---|---|
| Open-back HP | High-pass | 80 Hz | Q=0.9 (rear-wave cancellation) |
| Box resonance HP | High-pass | 55 Hz | Q=0.6 |
| Cone resonance | Peak | 130 Hz | Q=3, +3 dB |
| Alnico bloom | Peak | 2500 Hz | Q=1.6, +2 dB |
| Treble rolloff | Low-pass | 6500 Hz | Q=0.7 |

The result is Hanning-windowed and normalised to peak = 0.5.

### User IR loading

WAV and AIFF files are accepted. The file is read via `juce::AudioFormatManager`, capped at 4 seconds, and passed directly to `juce::dsp::Convolution::loadImpulseResponse` with `Normalise::yes`. The loaded file path is persisted in the APVTS state so it survives DAW session saves.

---

## Parameter mapping

All knobs are 1–10 in the APVTS. The processor normalises them to 0..1 before passing to DSP setters:

```cpp
auto norm = [](float v) { return (v - 1.0f) / 9.0f; };
```

Master maps to ±12 dB: `masterDb = -12 + norm * 24`.

---

## Threading

- Audio processing runs on the **audio thread** (`processBlock`).
- `inputPeakLevel` is a `std::atomic<float>` — written on the audio thread, read by the UI timer at 30 Hz.
- IR loading (`loadIR`, `clearIR`) must be called from the **message thread**. JUCE's `Convolution` handles the cross-thread handoff internally.
