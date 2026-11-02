#include "../src/dsp/mme_core.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

using choochoomme::Core;
using choochoomme::Model;
using choochoomme::Parameters;

int main() {
  const Model models[] = {Model::Ring, Model::Fold, Model::Cross, Model::Vpm,
                          Model::Sync, Model::Logic, Model::Vocode};
  for (float sampleRate : {44100.0f, 48000.0f, 96000.0f}) {
    for (Model model : models) {
      Core core;
      Parameters parameters;
      parameters.model = model;
      parameters.waves = 0.4f;
      parameters.interval = 0.5f;
      parameters.amount = 0.7f;
      parameters.flow = 0.6f;
      parameters.feedback = 1.0f;
      parameters.shaper = 0.8f;
      core.init(sampleRate);
      core.setFrequency(220.0f);
      core.setParameters(parameters);
      float peak = 0.0f;
      for (int i = 0; i < 4096; ++i) {
        float value = core.process();
        assert(std::isfinite(value));
        peak = std::max(peak, std::fabs(value));
      }
      assert(peak > 0.001f);
    }
  }

  for (Model model : models) {
    double previous = 0.0;
    for (int shaper = 0; shaper < 256; ++shaper) {
      Parameters parameters;
      parameters.model = model;
      parameters.shaper = shaper / 255.0f;
      Core core;
      core.init(48000.0f);
      core.setFrequency(60.0f);
      core.setParameters(parameters);
      double energy = 0.0;
      for (int i = 0; i < 1024; ++i) {
        float value = core.process();
        assert(std::isfinite(value));
        energy += value * value;
      }
      energy = std::sqrt(energy / 1024.0);
      if (previous > 1.0e-6 && energy > 1.0e-6)
        assert(std::fabs(20.0 * std::log10(energy / previous)) < 3.0);
      previous = energy;
    }
  }

  // Adjacent Waves values must morph, including across the old pair boundaries.
  for (Model model : {Model::Cross, Model::Vpm}) {
    for (float boundary : {.2f, .25f, .4f, .5f, .6f, .75f, .8f, 1.0f}) {
      Parameters parameters;
      parameters.model = model;
      parameters.amount = 0.0f;
      parameters.waves = boundary - .0001f;
      Core a, b;
      a.init(48000.0f); b.init(48000.0f);
      a.setParameters(parameters);
      parameters.waves = boundary;
      b.setParameters(parameters);
      float difference = 0.0f;
      for (int i = 0; i < 1024; ++i)
        difference += std::fabs(a.process() - b.process());
      assert(difference < 2.0f);
    }
  }

  Core morphA, morphB;
  Parameters morph;
  morph.model = Model::Cross;
  morph.amount = 0.0f;
  morph.waves = .1f;
  morphA.init(48000.0f); morphB.init(48000.0f);
  morphA.setParameters(morph);
  morph.waves = .15f;
  morphB.setParameters(morph);
  float morphDifference = 0.0f;
  for (int i = 0; i < 1024; ++i) morphDifference += std::fabs(morphA.process() - morphB.process());
  assert(morphDifference > .01f);

  Core resetCore;
  resetCore.init(48000.0f);
  resetCore.setFrequency(110.0f);
  resetCore.setParameters(Parameters{});
  for (int i = 0; i < 100; ++i)
    resetCore.process();
  resetCore.reset();
  float first = resetCore.process();
  Core fresh;
  fresh.init(48000.0f);
  fresh.setFrequency(110.0f);
  fresh.setParameters(Parameters{});
  assert(std::fabs(first - fresh.process()) < 1.0e-6f);
  std::cout << "mme_core_test passed\n";
}
