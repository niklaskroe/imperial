# Controls Reference

All knobs run **1–10**, like a real amp. There are no percentage values or dB readouts by design — use your ears.

---

## PREAMP section

### Gain

Controls input drive into the two 12AX7 preamp stages.

- **1–3**: Clean headroom. The amp remains clean even with a hot input signal.
- **4–6**: Edge of breakup. Pick dynamics produce light harmonic crunch on the attack.
- **7–10**: Crunch/overdrive. Sustain increases, picking attack softens, upper harmonics bloom.

The gain curve is quadratic — lower positions are more spread out, giving you fine control in the clean range. The internal range is 0–36 dB.

### Bass

Low-shelf EQ centred at 100 Hz, ±10 dB.

- **1**: Cuts low-end significantly — tight and thin, useful for cutting through a mix.
- **5**: Approximately flat.
- **10**: Full low-shelf boost — thicker, rounder. Can get muddy with high Gain settings.

### Mid

Controls the depth of the passive mid scoop at 650 Hz. This is a **cut-only** control, matching the real amp's passive RC network behaviour.

- **10**: Shallowest scoop (~−1 dB) — nearly full mids present.
- **5**: Medium scoop (~−6 dB) — the classic "scooped" American tone.
- **1**: Deepest scoop (~−12 dB) — very hollow, high contrast between bass and treble.

> Mid does not boost. Turning it up reduces the scoop depth; it cannot add mid energy above the input level.

### Treble

High-shelf EQ above 2 kHz, ±10 dB.

- **1**: Dull, muffled top-end. Useful with a bright or harsh guitar.
- **5**: Approximately flat.
- **10**: Bright, glassy. Works well with single-coil pickups.

### Presence

A narrower presence peak centred at 2.5 kHz, boosting 0–8 dB (Q=1.5). Applied at the phase inverter node, after the tone stack.

- **1**: No presence boost — the tone stack controls the full treble shape.
- **10**: +8 dB at 2.5 kHz — adds attack definition and pick clarity.

Presence and Treble interact: high Treble + high Presence can produce an overly bright result. Start with Presence around 4–5.

---

## POWER section

### Power Drive

Controls the EL84 push-pull power amp saturation intensity.

- **1**: Clean, uncoloured — the power stage is essentially transparent.
- **5**: Mild power amp compression and even-order harmonics. Adds warmth without obvious distortion.
- **10**: Hard saturation — adds asymmetric clipping and prominent even/odd harmonics. Sounds louder and more aggressive than the preamp drive alone.

### Sag

Controls how much the simulated power supply "sags" under load.

- **1**: No sag — tight and punchy, every transient hits equally hard.
- **5**: Moderate sag — loud chords compress slightly, giving a blooming quality.
- **10**: Heavy sag — similar to a worn-out amp running a low-quality PSU. Chord attacks soften noticeably; single notes sustain longer.

Sag interacts strongly with Power Drive. High Sag + high Power Drive = the loose, compressed feel of a cranked vintage combo.

---

## OUTPUT section

### IR Loader

Replaces the built-in cabinet simulation with a custom impulse response file.

- Click **LOAD IR** to open a file browser. WAV and AIFF formats are accepted (up to 4 seconds).
- The loaded filename is shown below the buttons.
- Click **X** to remove the user IR and revert to the built-in Alnico Blue cabinet.
- The loaded IR path is saved with the DAW session and will reload automatically on next open.

When no user IR is loaded, the label reads **Built-in IR** — a synthesised Celestion Alnico Blue 1×12 open-back response.

See [ir-loader.md](ir-loader.md) for tips on sourcing and using IR files.

### Master

Output volume control. ±12 dB around unity.

- **5**: 0 dB (unity gain) — output level matches what the amp stages produce.
- **1**: −12 dB — significantly quieter.
- **10**: +12 dB — significantly louder.

The Master knob does not affect the sound character — it is a clean gain stage after the cabinet convolution. Use it to match levels with other tracks or to compensate for a very quiet or very hot guitar signal.
