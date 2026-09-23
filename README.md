# Choo Choo Rack

Three VCV Rack 2 modules ported from ChooChooTracker.

![Choo Choo Rack — MME, Bogie and Sintered](capture.png)

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

### Sintered — Experimental Digital Percussion

A mono one-shot voice whose six models explore coupled oscillators, noise,
short combs, bitwise logic and unstable feedback. Knot, Shard, Burst, Comb,
Logic and Melt share Decay, Mod and Motion controls plus three model-specific
macros. Every macro has CV input and a bipolar attenuverter.

## Install

Download the Windows x64 `.vcvplugin` releases and install them with Rack's
Library menu, or copy them to `Documents/Rack2/plugins-win-x64`, then restart
Rack.

## Build

All three plugins target Rack 2 and require the matching Rack SDK plus a MinGW64
toolchain on Windows.

```sh
# MME
cd ChooChooMME
make RACK_DIR=/path/to/Rack-SDK dist

# Bogie
cd ChooChooBogie
make RACK_DIR=/path/to/Rack-SDK dist

# Sintered
cd ChooChooSintered
make RACK_DIR=/path/to/Rack-SDK dist
```

The standalone DSP tests are `ChooChooMME/tests/mme_core_test.cpp`,
`ChooChooBogie/tests/bogie_core_test.cpp` and
`ChooChooSintered/tests/sintered_core_test.cpp`.

## License and attribution

MIT. Bogie and Sintered are original Choo Choo implementations. MME incorporates tracker
adaptations of MIT-licensed Mutable Instruments Warps techniques; retained
source notices apply.
