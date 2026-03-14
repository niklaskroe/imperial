# Imperial

A guitar amp simulator JUCE plugin modelled after the Tone King Imperial MkII — an American clean/crunch combo known for its warm low-end, glassy highs, and touch-sensitive response.

Built with JUCE. Formats: **VST3**, **AU**, **Standalone**. macOS only.

## Controls

All knobs run **1–10**, like a real amp.

| Knob | Section | Description |
|---|---|---|
| Gain | Preamp | Input drive into the 12AX7 stages. Low = clean, high = crunch/break-up |
| Bass | Preamp | Low-shelf boost/cut centred at 100 Hz |
| Mid | Preamp | Passive mid scoop depth at 650 Hz. 10 = nearly flat, 1 = deepest scoop |
| Treble | Preamp | High-shelf boost/cut above 2 kHz |
| Presence | Preamp | Upper-mid air peak at 2.5 kHz, 0–8 dB boost |
| Power Drive | Power | EL84 push-pull saturation intensity |
| Sag | Power | Power supply sag (compression on loud transients) |
| IR Loader | Output | Load a custom cabinet WAV/AIFF, or use the built-in Alnico Blue IR |
| Master | Output | Output volume, ±12 dB around unity at position 5 |

## Installation

See [docs/installation.md](docs/installation.md) for full step-by-step instructions.

**Quick start (from pre-built binaries):**

- **VST3** — copy `Imperial.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
- **AU** — copy `Imperial.component` to `~/Library/Audio/Plug-Ins/Components/`, then run `killall -9 AudioComponentRegistrar`
- **Standalone** — copy `Imperial.app` to `/Applications/`

## Building from source

Requires **JUCE 8+** installed at `~/JUCE` and **CMake 3.22+**.

```sh
git clone https://github.com/niklaskroe/imperial.git
cd imperial
cmake -B build
cmake --build build --config Release
```

Artefacts land in `build/Imperial_artefacts/Release/`.

See [docs/building.md](docs/building.md) for prerequisites and options.

## Documentation

- [docs/architecture.md](docs/architecture.md) — signal chain and DSP design
- [docs/controls.md](docs/controls.md) — detailed control reference
- [docs/ir-loader.md](docs/ir-loader.md) — using the cabinet IR loader
- [docs/building.md](docs/building.md) — build instructions
- [docs/installation.md](docs/installation.md) — installation instructions

## License

This project is licensed under the **GNU General Public License v3.0**. See [LICENSE](LICENSE) for the full text.

This project uses [JUCE](https://juce.com), which is also licensed under the GPL-3.0 for open-source use.
