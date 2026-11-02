#include "../src/plugin.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

using rack::engine::Module;

// Simulate the engine connecting a cable; setChannels() cannot connect a port.
static void connect(rack::engine::Port& port, int channels) {
  if (!port.isConnected()) port.channels = 1;
  port.setChannels(channels);
  if (channels == 0) port.channels = 0;
}

static std::unique_ptr<Module> create(rack::plugin::Model* model, int algorithm, float rate) {
  std::unique_ptr<Module> module(model->createModule());
  module->params[1].setValue(algorithm);
  for (int param = 0; param < module->getNumParams(); ++param) {
    if (module->getParamQuantity(param)->name.find("CV attenuverter") != std::string::npos)
      module->params[param].setValue(.6f);
  }
  module->onSampleRateChange({rate, 1.0f / rate});
  connect(module->outputs[0], 1);
  return module;
}

int main() {
  struct Synth { rack::plugin::Model* model; int algorithms; };
  const Synth synths[] = {{modelChooChooMME, 7}, {modelBogie, 12}, {modelSintered, 7}};
  for (const auto& synth : synths) {
    for (float rate : {44100.0f, 48000.0f, 96000.0f}) {
      for (int algorithm = 0; algorithm < synth.algorithms; ++algorithm) {
        // Poly pitch/trigger, mono broadcast, and CV-driven voice count.
        for (int scenario = 0; scenario < 3; ++scenario) {
          auto poly = create(synth.model, algorithm, rate);
          std::array<std::unique_ptr<Module>, PORT_MAX_CHANNELS> mono;
          for (auto& voice : mono) voice = create(synth.model, algorithm, rate);
          for (int input = 0; input < poly->getNumInputs(); ++input) {
            int count = input == 0 ? (scenario == 2 ? 1 : 16)
                : input == 1 ? (scenario == 0 ? 16 : 1)
                : input == 2 ? (scenario == 2 ? 16 : 4)
                : input == 3 ? 1 : input == 4 ? 0 : 4;
            connect(poly->inputs[input], count);
            for (auto& voice : mono) connect(voice->inputs[input], 1);
          }
          std::array<float, PORT_MAX_CHANNELS> peaks{};
          for (int frame = 0; frame < 256; ++frame) {
            Module::ProcessArgs args{rate, 1.0f / rate, frame};
            for (int input = 0; input < poly->getNumInputs(); ++input) {
              for (int channel = 0; channel < poly->inputs[input].getChannels(); ++channel) {
                float voltage = input == 0 ? (channel % 8 - 4) * .16f
                    : input == 1 ? (frame % 128 == 4 + channel ? 10.0f : 0.0f)
                    : ((channel + input) % 5 - 2) * .3f;
                poly->inputs[input].setVoltage(voltage, channel);
              }
              for (int channel = 0; channel < PORT_MAX_CHANNELS; ++channel)
                mono[channel]->inputs[input].setVoltage(poly->inputs[input].getPolyVoltage(channel));
            }
            poly->process(args);
            assert(poly->outputs[0].getChannels() == PORT_MAX_CHANNELS);
            for (int channel = 0; channel < PORT_MAX_CHANNELS; ++channel) {
              mono[channel]->process(args);
              float actual = poly->outputs[0].getVoltage(channel);
              float expected = mono[channel]->outputs[0].getVoltage();
              assert(std::isfinite(actual));
              assert(std::fabs(actual - expected) < 1.0e-5f);
              peaks[channel] = std::max(peaks[channel], std::fabs(actual));
            }
          }
          for (float peak : peaks) assert(peak > .001f);
        }
      }
    }

    // Removing then restoring voices must not resume stale oscillator/tail state.
    auto poly = create(synth.model, 0, 48000.0f);
    auto survivor = create(synth.model, 0, 48000.0f);
    connect(poly->inputs[0], 16);
    connect(poly->inputs[1], 1);
    connect(survivor->inputs[1], 1);
    Module::ProcessArgs args{48000.0f, 1.0f / 48000.0f, 0};
    for (int frame = 0; frame < 64; ++frame) {
      float gate = frame < 4 ? 0.0f : 10.0f;
      poly->inputs[1].setVoltage(gate);
      survivor->inputs[1].setVoltage(gate);
      poly->process(args); survivor->process(args);
    }
    connect(poly->inputs[0], 1);
    poly->process(args); survivor->process(args);
    assert(poly->outputs[0].getChannels() == 1);
    for (int channel = 1; channel < 16; ++channel)
      assert(poly->outputs[0].getVoltage(channel) == 0.0f);
    auto fresh = create(synth.model, 0, 48000.0f);
    connect(fresh->inputs[1], 1);
    fresh->inputs[1].setVoltage(10.0f);
    connect(poly->inputs[0], 16);
    for (int frame = 0; frame < 64; ++frame) {
      poly->process(args); survivor->process(args); fresh->process(args);
      assert(poly->outputs[0].getChannels() == 16);
      assert(poly->outputs[0].getVoltage() == survivor->outputs[0].getVoltage());
      for (int channel = 1; channel < 16; ++channel)
        assert(poly->outputs[0].getVoltage(channel) == fresh->outputs[0].getVoltage());
    }
  }
  std::cout << "polyphony_test passed: all 26 algorithms, 16 independent channels, mono broadcast, CV and voice removal\n";
}
