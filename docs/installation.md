# Installation

## Requirements

- macOS 11 Big Sur or later
- An AU or VST3 host (Logic Pro, GarageBand, Ableton Live, Reaper, etc.) — or run as Standalone

---

## VST3

1. Locate `Imperial.vst3` in `build/Imperial_artefacts/Release/VST3/` (or download from releases).
2. Copy it to your VST3 plug-ins folder:

   ```sh
   cp -r Imperial.vst3 ~/Library/Audio/Plug-Ins/VST3/
   ```

3. Restart your DAW. Imperial should appear in the plug-in list under **OpenCode > Imperial**.

> **Note:** On first launch macOS may quarantine the bundle. If your DAW shows a "cannot be opened" error, run:
> ```sh
> xattr -rd com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Imperial.vst3
> ```

---

## AU (Audio Unit)

1. Copy `Imperial.component` to the Components folder:

   ```sh
   cp -r Imperial.component ~/Library/Audio/Plug-Ins/Components/
   ```

2. Force macOS to re-scan the AU cache:

   ```sh
   killall -9 AudioComponentRegistrar
   ```

3. Open your DAW. In Logic Pro the plug-in appears under **Audio Units > OpenCode > Imperial**.

> If the plug-in does not appear after a DAW restart, run `auval` to verify registration:
> ```sh
> auval -v aufx TkIm Opcd
> ```
> A passing result confirms the component is valid.

---

## Standalone

1. Copy `Imperial.app` to `/Applications/`:

   ```sh
   cp -r Imperial.app /Applications/
   ```

2. Launch it from Spotlight or Finder.
3. On first launch you will be prompted for **microphone access** — grant it so the app can receive your guitar input.
4. Open **Options > Audio/MIDI settings** (the gear icon in the title bar) to choose your input device and output device.

> Connect your guitar via an audio interface. Direct line-in without a DI box or interface is not recommended (impedance mismatch).

---

## Uninstalling

Remove the relevant bundle(s):

```sh
# VST3
rm -rf ~/Library/Audio/Plug-Ins/VST3/Imperial.vst3

# AU
rm -rf ~/Library/Audio/Plug-Ins/Components/Imperial.component
killall -9 AudioComponentRegistrar

# Standalone
rm -rf /Applications/Imperial.app
```

Preferences and saved presets are not stored separately — removing the bundle is sufficient.
