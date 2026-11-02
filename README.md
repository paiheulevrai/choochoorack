# Choo Choo Rack

One VCV Rack 2 plugin containing three modules, ported from ChooChooTracker:
MME, Bogie and Sintered. Plugin slug: `ChooChooRack`, version `2.0.1`.

![Choo Choo Rack — MME, Bogie and Sintered](capture.png)

## Modules

### MME - Multi Modulation VCO engine

A mono two-oscillator VCO built around interactions between its internal
oscillators. Inspired by Noise Engineering and Mutable Warps.

Good for dirty gritty sounds, unstable modulations, techno, noise and industrial.

Parameters:
- Models: Ring, Fold, Cross, VPM, Sync, Logic or Vocode. Different types of modulations
- Intervals sets the oscillators relationship
- Waves continuously morphs between five paired oscillator shapes
- Amount set the strength of the oscillators interaction
- Flow changes the direction/variant of the interatction
- Feedback : from subtle to deliberately unruly
- Shaper : from clean through saturation to wavefolding

### Bogie - Drum Synth

A kit of engines that sound like digital drum machines. Inspired by the Machinedrum. General purpose synthetic drums.

- Kick: Pitch-swept sine body with a modulated harmonic and a short noise transient.
- Snare: Two pitched resonators blended with bright, filtered noise.
- Hat: Six inharmonic square oscillators with staggered decays and a noise layer.
- Clap: Filtered noise shaped into three rapid bursts, with an optional tonal click.
- Tom: Dual sine resonator with pitch sweep, FM overtones and a brief noise attack.
- Rim: Short triangle-and-sine resonator sharpened by a fast noise transient.
- FM: Sine carrier driven by decaying audio-rate frequency modulation.
- Noise: Variable blend of bright and dark filtered noise with a tonal trace.
- Cowbell: Two inharmonic square oscillators with cross-modulation and metallic detuning.
- Cymbal: Six detuned square partials with staggered envelopes and broadband noise.
- Shaker: Bright noise animated by low-frequency amplitude modulation and a pitched tick.
- Clave: High-ratio triangle and sine resonators shaped into a short wooden impulse.

The lower half of the parameter ranges targets conventional drum sounds, while higher settings open into more synthetic timbres.

### Sintered - Experimental Digital Percussion

The percussive version of MME. Weird percussions from another world, good for IDM, noise, industrial, techno and experimentation. Very wild timbre variations. 
Inspired by "what would happen if you make drums with unconventional algos?"

- Knot: Three coupled sine oscillators tangled through phase modulation and wavefolding.
- Shard: Feedback-driven oscillator pair crushed through aggressive folding and saturation.
- Burst: Colored noise and a pitched resonator injected into a regenerative wavefolder.
- Comb: Oscillator-and-noise excitation fed into a short, unstable comb resonator.
- Logic: Quantized oscillators combined with XOR, AND, OR or comparator logic, then folded.
- Melt: Feedback-warped frequency modulation pushed through soft saturation and noisy instability.
- Squall: Three to five detuned, band-limited saw oscillators with noise FM and clipping.

Motion controls the depth and timing of model-specific modulation, with a neutral
center, a short swell on the negative side and an exponential fall on the positive side.

## Install

Download `ChooChooRack-2.0.1-win-x64.vcvplugin` from the GitHub releases,
copy it to Rack's user folder under `plugins-win-x64`, then restart Rack.
On Windows the default location is `%LOCALAPPDATA%/Rack2/plugins-win-x64`.
All three modules are included in this single package.

This combined plugin replaces the earlier separate ChooChooMME, ChooChooBogie
and ChooChooSintered plugins. Existing patches using those plugin slugs need
their modules replaced and reconnected; save a copy before migrating.

## Build

The plugin targets Rack 2 and requires the matching Rack SDK plus a MinGW64
toolchain on Windows.

```sh
# From the repository root
make RACK_DIR=/path/to/Rack-SDK dist
make RACK_DIR=/path/to/Rack-SDK test
```

The standalone DSP tests are [MME](tests/mme_core_test.cpp),
[Bogie](tests/bogie_core_test.cpp) and [Sintered](tests/sintered_core_test.cpp).
They cover all models at 44.1, 48 and 96 kHz, percussion tails and macro response,
MME waveform morphing, Sintered Motion continuity and retriggering.

The DSP port follows `chipnomad_lib/synth/mme_voice.cpp`, `drum_synth_voice.cpp`
and `sintered_voice.cpp` in ChooChooTracker. Tracker-specific envelopes, filters
and gain stages are omitted in Rack, where those functions can be patched externally.

## License and attribution

MIT. Bogie and Sintered are original Choo Choo implementations. MME incorporates tracker
adaptations of MIT-licensed Mutable Instruments Warps techniques; retained
source notices apply.
