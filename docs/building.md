# Building from Source

## Prerequisites

| Requirement | Version | Notes |
|---|---|---|
| macOS | 11+ | Apple Silicon and Intel both supported |
| Xcode Command Line Tools | 14+ | `xcode-select --install` |
| CMake | 3.22+ | `brew install cmake` |
| JUCE | 8.0+ | Must be at `~/JUCE` |

### Installing JUCE

Clone or download JUCE to your home directory:

```sh
git clone https://github.com/juce-framework/JUCE.git ~/JUCE
```

The `CMakeLists.txt` references it as:

```cmake
add_subdirectory($ENV{HOME}/JUCE juce_build)
```

If you install JUCE elsewhere, update that line.

---

## Configure

```sh
git clone <this-repo> imperial
cd imperial
cmake -B build
```

CMake will configure JUCE, generate build files, and print a summary. The default generator on macOS is Unix Makefiles. To use Xcode instead:

```sh
cmake -B build -G Xcode
```

---

## Build

**Debug** (fast compile, no optimisations, includes symbols):

```sh
cmake --build build --config Debug
```

**Release** (optimised, suitable for distribution):

```sh
cmake --build build --config Release
```

Artefacts land in:

```
build/Imperial_artefacts/
└── Release/
    ├── Standalone/Imperial.app
    ├── VST3/Imperial.vst3
    └── AU/Imperial.component
```

---

## Build options

| CMake variable | Default | Description |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `Debug` | `Debug` or `Release` |
| `COPY_PLUGIN_AFTER_BUILD` | `FALSE` | Set to `TRUE` to auto-install after build |

Example — build Release and auto-install:

```sh
cmake -B build -DCOPY_PLUGIN_AFTER_BUILD=TRUE
cmake --build build --config Release
```

---

## Clean build

```sh
rm -rf build
cmake -B build
cmake --build build --config Release
```

---

## Troubleshooting

**"JUCE not found"** — ensure `~/JUCE/CMakeLists.txt` exists. JUCE must be the repository root, not a subdirectory.

**"No audio output in Standalone"** — if you built Debug, macOS may block the microphone. Grant access in System Settings > Privacy & Security > Microphone.

**AU not appearing in Logic** — run `auval -v aufx TkIm Opcd`. If it fails with "not found", re-copy the `.component` and run `killall -9 AudioComponentRegistrar`.
