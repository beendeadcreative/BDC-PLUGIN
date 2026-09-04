# BDC Plugin

A custom audio effect combining a generative granulator, chorus, rotary
(Leslie-style) speaker, and delay — built with [JUCE](https://juce.com).

Feed it audio and it captures what you play into a rolling buffer, then
continuously spins up new melodic phrases from that material using a
scale-constrained generative engine driving the granulator. It can also run
with **no input at all**: an internal seed pad fills the buffer when
there's silence (or all the time, if you want a constantly self-generating
texture), so it works as a standalone generative instrument too.

Signal chain: `Input → Capture Buffer (real input + seed pad) → Generative
Granulator → Chorus → Rotary → Delay → Output`.

## Requirements

- macOS with **Xcode** installed (full app, not just Command Line Tools)
- **CMake** (`brew install cmake`)
- Git

No paid Apple Developer account is needed to build and run this locally.

## Build

```bash
git clone <this-repo-url>
cd BDC-PLUGIN
git checkout claude/custom-music-plugin-effects-hf0rua

cmake -B build -G Xcode
cmake --build build --config Debug
```

The first configure step downloads JUCE automatically (pinned to 8.0.15) —
this can take a few minutes. AU and VST3 builds are copied to your system
plugin folders automatically (`COPY_PLUGIN_AFTER_BUILD`).

## Try it

**Standalone app (no DAW needed, easiest first step):**
```
build/BDCPlugin_artefacts/Debug/Standalone/BDC Plugin.app
```
Open it, pick your audio input/output device from its settings, and play.
With nothing plugged in, set **Self-Generate** to `Auto` or `Always` and it
will generate on its own.

**In a DAW (e.g. Logic Pro):** restart the DAW or rescan plugins — "BDC
Plugin" will show up as an Audio Unit / VST3 effect.

## Parameters (current placeholder UI)

The editor is JUCE's generic parameter list for now — every control below
shows up as a slider/toggle so everything is testable before a custom GUI
is built:

- **Self-Generate**: `Off` (only real input feeds the engine) / `Auto`
  (fades in the internal seed pad when you stop playing) / `Always`
  (constantly layers the seed pad under your playing).
- **Root Note / Scale**: key and scale the generative engine stays inside.
- **Unpredictability**: how often the generated melody takes a big leap vs.
  moving stepwise.
- **Grain Density / Size / Spread**: granulator character — how many
  grains per second, how long each is, and how far back into the capture
  buffer they're allowed to be pulled from.
- **Generative Mix**: blend between your dry input and the generated
  material feeding into the effects chain.
- **Chorus Rate / Depth / Mix**, **Rotary Fast / Mix**, **Delay Time /
  Feedback / Mix**, **Output Gain**: as named.

## Project layout

```
CMakeLists.txt
Source/
  PluginProcessor.h/.cpp   - parameter layout + signal chain
  PluginEditor.h/.cpp      - UI (currently the generic APVTS editor)
  DSP/
    CircularBuffer.*       - rolling capture buffer the granulator reads from
    SeedGenerator.*        - self-generating pad + input presence detector
    GenerativeEngine.*     - scale-constrained random walk driving grains
    Granulator.*           - grain scheduling/windowing/playback
    ChorusModule.*
    RotaryModule.*
    DelayModule.*
```
