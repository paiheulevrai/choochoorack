# Choo Choo Rack

Two VCV Rack 2 modules ported from ChooChooTracker/MobileGroove.

## Modules

### MME — Multi Modulation Engine

A mono two-oscillator VCO built around interactions between its internal
oscillators. Choose Ring, Fold, Cross, VPM, Sync, Logic or Vocode; `Interval`
sets their relationship. Waves selects paired oscillator shapes, Amount drives
the core interaction, Flow changes its direction or variant, Feedback ranges
from subtle to deliberately unruly, and Shaper goes from clean through
saturation to wavefolding. Each macro has CV input and a bipolar attenuverter.

### Bogie — Drum Synth

A mono, one-shot drum voice with Kick, Snare, Hat, Clap, Tom, Rim, FM, Noise,
Cowbell, Cymbal, Shaker and Clave models. Trigger retriggers the voice; V/Oct,
six macro CV inputs and their attenuverters support modular sequencing. Its
six macros are Decay, Tone, Sweep, Noise, FM and Drive; their displayed names
adapt per model, such as `Click` for a Kick or `Crossmod` for a Cowbell. The
lower half of their ranges targets conventional drum sounds, while higher
settings open into more synthetic timbres.

## Install

Download the Windows x64 `.vcvplugin` releases and install them with Rack's
Library menu, or copy them to `Documents/Rack2/plugins-win-x64`, then restart
Rack.

## Build

Both plugins target Rack 2 and require the matching Rack SDK plus a MinGW64
toolchain on Windows.

```sh
# MME
cd ChooChooMME
make RACK_DIR=/path/to/Rack-SDK dist

# Bogie
cd ChooChooBogie
make RACK_DIR=/path/to/Rack-SDK dist
```

The standalone DSP tests are `ChooChooMME/tests/mme_core_test.cpp` and
`ChooChooBogie/tests/bogie_core_test.cpp`.

## License and attribution

MIT. Bogie is an original Choo Choo implementation. MME incorporates tracker
adaptations of MIT-licensed Mutable Instruments Warps techniques; retained
source notices apply.
