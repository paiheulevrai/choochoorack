# Choo Choo Rack

Two VCV Rack 2 modules ported from ChooChooTracker.

## Modules

### MME — Multi Modulation Engine

A mono two-oscillator VCO built around interactions between its internal
oscillators. Inspired by Noise Engineering and Mutable Warps.

Good for dirty gritty sounds, unstable modulations, techno, noise and industrial.

Parameters:
- Models: Ring, Fold, Cross, VPM, Sync, Logic or Vocode. Different types of modulations
- Intervals sets the oscillators relationship
- Waves selects paired oscillator shapes
- Amount set the strength of the oscillators interaction
- Flow changes the direction/variant of the interatction
- Feedback : from subtle to deliberately unruly
- Shaper : from clean through saturation to wavefolding

### Bogie — Drum Synth

A mono, one-shot drum voice with Kick, Snare, Hat, Clap, Tom, Rim, FM, Noise,
Cowbell, Cymbal, Shaker and Clave models. Sounds like digital drum machines.

Parameters depend of the model. The lower half of their ranges targets conventional drum sounds, while higher
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
