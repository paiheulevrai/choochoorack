#include "../src/dsp/sintered_core.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

using sintered::Core;
using sintered::Model;
using sintered::Parameters;

int main() {
  for (float rate : {44100.0f, 48000.0f, 96000.0f}) {
    for (int model = 0; model < 6; ++model) {
      Parameters parameters; parameters.model = static_cast<Model>(model);
      parameters.decay = .62f; parameters.mod = .58f; parameters.a = .47f;
      parameters.b = .39f; parameters.motion = .72f; parameters.c = .55f;
      Core core; core.init(rate); core.setFrequency(261.625565f); core.setParameters(parameters); core.trigger();
      float peak = 0.0f;
      for (int i = 0; i < static_cast<int>(rate * 1.2f); ++i) {
        float value = core.process(); assert(std::isfinite(value)); assert(std::fabs(value) <= 1.001f);
        peak = std::max(peak, std::fabs(value));
      }
      assert(peak > .001f); assert(!core.active());
    }
  }

  for (int model = 0; model < 6; ++model) {
    Parameters base; base.model = static_cast<Model>(model);
    base.decay = .30f; base.mod = .40f; base.a = .35f; base.b = .30f; base.motion = 128.0f / 255.0f; base.c = .30f;
    for (int macro = 0; macro < 6; ++macro) {
      Parameters changed = base;
      float* values[] = {&changed.decay, &changed.mod, &changed.a, &changed.b, &changed.motion, &changed.c};
      *values[macro] = macro == 4 ? .85f : .80f;
      Core a, b; a.init(48000.0f); b.init(48000.0f); a.setFrequency(180.0f); b.setFrequency(180.0f);
      a.setParameters(base); b.setParameters(changed); a.trigger(); b.trigger();
      float difference = 0.0f;
      for (int i = 0; i < 48000; ++i) difference += std::fabs(a.process() - b.process());
      assert(difference > .01f);
    }
  }

  Core retriggered, fresh; Parameters parameters; parameters.model = Model::Burst;
  retriggered.init(48000.0f); fresh.init(48000.0f); retriggered.setParameters(parameters); fresh.setParameters(parameters);
  retriggered.trigger(); for (int i = 0; i < 100; ++i) retriggered.process(); retriggered.trigger(); fresh.trigger();
  for (int i = 0; i < 512; ++i) assert(retriggered.process() == fresh.process());
  std::cout << "sintered_core_test passed\n";
}
