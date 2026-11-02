#include "../src/dsp/bogie_core.h"

#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
  for (float rate : {44100.0f, 48000.0f, 96000.0f}) {
    for (int model = 0; model < 12; ++model) {
      bogie::Core core; bogie::Parameters parameters; parameters.model = static_cast<bogie::Model>(model); parameters.decay = .55f; parameters.tone = .63f; parameters.sweep = .44f; parameters.noise = .72f; parameters.fm = .68f; parameters.drive = .40f;
      core.init(rate); core.setFrequency(110.0f); core.setParameters(parameters); core.trigger(); float peak = 0.0f;
      for (int i = 0; i < static_cast<int>(rate * 2.2f); ++i) { float value = core.process(); assert(std::isfinite(value)); peak = std::max(peak, std::fabs(value)); }
      assert(peak > .001f); assert(!core.active());
    }
  }
  bogie::Core core; core.init(48000.0f); core.setParameters({}); core.trigger(); for (int i = 0; i < 100; ++i) core.process(); core.trigger(); assert(core.active());
  for (int model = 0; model < 12; ++model) {
    bogie::Parameters base; base.model = static_cast<bogie::Model>(model); base.decay = base.tone = base.sweep = base.noise = base.fm = base.drive = .35f;
    for (int macro = 0; macro < 6; ++macro) {
      bogie::Parameters changed = base;
      float* values[] = {&changed.decay, &changed.tone, &changed.sweep, &changed.noise, &changed.fm, &changed.drive};
      *values[macro] = .85f;
      bogie::Core a, b; a.init(48000.0f); b.init(48000.0f); a.setFrequency(180.0f); b.setFrequency(180.0f); a.setParameters(base); b.setParameters(changed); a.trigger(); b.trigger();
      float difference = 0.0f;
      for (int i = 0; i < 2048; ++i) difference += std::fabs(a.process() - b.process());
      assert(difference > .01f);
    }
  }
  std::cout << "bogie_core_test passed\n";
}
