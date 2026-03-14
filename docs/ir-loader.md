# IR Loader

The IR Loader in the OUTPUT section replaces the built-in cabinet simulation with any impulse response file you choose. This lets you use commercial or freely available cabinet IRs to change the speaker and room character while keeping Imperial's preamp and power amp stages.

---

## Loading an IR

1. Click **LOAD IR** in the OUTPUT section of the plugin.
2. A file browser opens. Navigate to your IR file.
3. Select the file and click Open.
4. The filename appears below the buttons confirming the load.

If the file cannot be read (unsupported format, corrupt file, wrong sample rate), an alert is shown and the previous IR remains active.

---

## Supported formats

- **WAV** (`.wav`) — any bit depth, any sample rate
- **AIFF** (`.aif`, `.aiff`) — any bit depth, any sample rate

Stereo IRs are accepted and used as-is. Mono IRs are loaded and applied identically to both channels.

IRs longer than **4 seconds** are truncated to 4 seconds. Most guitar cabinet IRs are well under 200 ms; the limit exists only as a safeguard against accidentally loading a reverb or room IR that would introduce excessive latency and memory use.

---

## Clearing an IR

Click **X** next to the loaded filename. The built-in Alnico Blue IR is restored immediately. The label returns to **Built-in IR**.

---

## Session persistence

The full path to the loaded IR file is saved with the DAW session (inside the plugin's state). When you reopen the session, Imperial reloads the file automatically — provided it still exists at the same path. If the file has moved or been deleted, the plugin falls back to the built-in IR silently.

---

## Built-in IR

When no user IR is loaded, Imperial uses a synthesised Celestion Alnico Blue 1×12 open-back response. It is generated at startup from a cascade of IIR filters and is not a sampled IR — it approximates the overall frequency character (open-back roll-off below 80 Hz, presence peak at 130 Hz, gentle Alnico bloom at 2.5 kHz, -12 dB/oct above 6.5 kHz) rather than the phase and room response of a real recording.

For more realistic cabinet character, load a real IR.

---

## Where to find IR files

Many cabinet IR packs are available free or commercially. Some starting points:

- **Celestion** — offers official IRs of their own speakers, including the Alnico Blue
- **OwnHammer** — large library of vintage and modern cabinets
- **Cab IR** — free and paid packs from various builders
- **Catharsis** — free community-sourced packs
- **Your own recordings** — capture your physical cabinet with a measurement microphone and deconvolution software (Room EQ Wizard, Impulse Record, etc.)

A standard IR file for a guitar cabinet is typically:
- 48 kHz or 44.1 kHz sample rate
- Mono, 16 or 24-bit
- 200–500 ms long (most of the energy in the first 50 ms)

---

## Tips

- Start with a clean IR recorded at close range (e.g. SM57 on-axis) for the most neutral result.
- For more room character, blend a room-position IR — or chain Imperial into a room reverb after the cabinet.
- If the IR sounds too bright or too dark, adjust Treble and Presence to compensate without swapping IRs.
- The Gain, Power Drive and Sag controls interact with the cabinet response — a bright IR + high Presence can become harsh; a dark IR + low Treble can become muddy.
