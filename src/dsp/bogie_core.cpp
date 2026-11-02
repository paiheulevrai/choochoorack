#include "bogie_core.h"

#include <algorithm>
#include <cmath>

namespace bogie {
namespace {
constexpr float kPi = 3.14159265358979323846f;
float clamp(float value, float low, float high) { return std::max(low, std::min(value, high)); }
}

void Core::init(float sampleRate) {
  sampleRate_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
  active_ = false; age_ = duration_ = envelope_ = noiseLow_ = 0.0f;
  phase_.fill(0.0f);
}
void Core::setFrequency(float frequency) { frequency_ = clamp(frequency, 10.0f, sampleRate_ * 0.45f); }
void Core::setParameters(const Parameters& parameters) {
  parameters_ = parameters;
  parameters_.decay = clamp(parameters_.decay, 0.0f, 1.0f);
  parameters_.tone = clamp(parameters_.tone, 0.0f, 1.0f);
  parameters_.sweep = clamp(parameters_.sweep, 0.0f, 1.0f);
  parameters_.noise = clamp(parameters_.noise, 0.0f, 1.0f);
  parameters_.fm = clamp(parameters_.fm, 0.0f, 1.0f);
  parameters_.drive = clamp(parameters_.drive, 0.0f, 1.0f);
}
void Core::trigger() {
  age_ = 0.0f; envelope_ = 1.0f; noiseLow_ = 0.0f; active_ = true;
  phase_.fill(0.0f);
  duration_ = 0.018f + parameters_.decay * parameters_.decay * (parameters_.model == Model::Hat ? 1.55f : 2.0f);
}
float Core::random() {
  randomState_ = randomState_ * 1664525u + 1013904223u;
  return ((randomState_ >> 8) * (1.0f / 8388608.0f)) - 1.0f;
}
float Core::oscillator(float frequency, int shape, int slot) {
  phase_[slot] += frequency / sampleRate_;
  phase_[slot] -= std::floor(phase_[slot]);
  float phase = phase_[slot];
  if (shape == 1) return 1.0f - 4.0f * std::fabs(phase - 0.5f);
  if (shape == 2) return phase < 0.5f ? 1.0f : -1.0f;
  return std::sin(2.0f * kPi * phase);
}
float Core::process() {
  if (!active_) return 0.0f;
  if (age_ >= duration_) { active_ = false; return 0.0f; }
  static constexpr float modelGain[] = {
    1.335575f, 1.253229f, 1.202260f, 1.352630f, 1.331551f, 1.335482f,
    1.335162f, 1.286303f, 1.332512f, 1.084859f, 1.337063f, 1.329288f
  };
  const float calibration = modelGain[static_cast<int>(parameters_.model)];
  float progress = age_ / duration_;
  float fast = std::exp(-age_ / (0.006f + 0.050f * (1.0f - parameters_.tone)));
  envelope_ = std::exp(-5.5f * progress);
  float n = random();
  noiseLow_ += (n - noiseLow_) * (0.015f + parameters_.tone * 0.25f);
  float brightNoise = n - noiseLow_;
  float sample = 0.0f;
  const float tone = parameters_.tone, sweep = parameters_.sweep, noise = parameters_.noise, fm = parameters_.fm;
  switch (parameters_.model) {
    case Model::Kick: {
      float frequency = frequency_ * (1.0f + sweep * 7.0f * fast);
      sample = oscillator(frequency, 0, 0) * .90f + oscillator(frequency * 2.0f, 1, 1) * fm * .30f;
      sample += brightNoise * noise * .45f * std::exp(-age_ / .006f); break;
    }
    case Model::Snare:
      sample = oscillator(frequency_ * (.65f + tone * 1.05f) * (1.0f + sweep * 1.1f * fast), 1, 0) * (1.0f - noise * .75f);
      sample += oscillator(frequency_ * (1.20f + fm * 1.50f), 0, 1) * (.12f + fm * .58f) + brightNoise * noise * .95f; break;
    case Model::Hat: {
      const float ratios[] = {1.18f, 1.56f, 2.17f, 2.87f, 3.73f, 4.61f}; float metal = 0.0f;
      float base = (180.0f + tone * 260.0f + frequency_ * .30f) * (1.0f + sweep * .75f * fast);
      for (int i = 0; i < 6; ++i) metal += oscillator(base * (ratios[i] + fm * (i + 1) * .20f), 2, i) * std::exp(-age_ / (.025f + i * .020f + tone * .070f)) * (.06f + fm * .06f);
      sample = metal + brightNoise * noise * 1.25f; break;
    }
    case Model::Clap: {
      float spacing = .010f + sweep * .018f;
      float burst = age_ < .010f || (age_ > spacing && age_ < spacing + .010f) || (age_ > spacing * 2.0f && age_ < spacing * 2.0f + .010f) ? 1.0f : .55f;
      sample = brightNoise * (.25f + noise * .75f) * burst + oscillator(frequency_ * (2.0f + tone * 8.0f), 2, 0) * fm * .55f * fast; break;
    }
    case Model::Tom:
      sample = oscillator(frequency_ * (.90f + tone * .35f) * (1.0f + sweep * 1.3f * fast), 0, 0) + oscillator(frequency_ * (1.6f + fm * 1.4f), 0, 1) * fm * .55f + brightNoise * noise * .55f * fast; break;
    case Model::Rim:
      sample = oscillator(frequency_ * (.85f + tone * .45f) * (1.0f + sweep * .25f), 1, 0) * .70f + oscillator(frequency_ * (1.7f + fm * 2.5f), 0, 1) * (.10f + fm * .55f) + brightNoise * noise * .50f * fast; break;
    case Model::Fm: {
      float ratio = .25f * std::pow(2.0f, tone * 2.5f); float mod = oscillator(frequency_ * ratio, 0, 1) * fm * 9.0f * fast;
      sample = oscillator(frequency_ * (1.0f + sweep * 2.0f * fast + mod), 0, 0) * .85f + brightNoise * noise * .45f; break;
    }
    case Model::Noise:
      sample = (brightNoise * (1.0f - sweep * .75f) + noiseLow_ * sweep * .75f) * (.2f + noise * .8f) + oscillator(frequency_ * (1.0f + tone * 5.0f), 0, 0) * fm * .22f; break;
    case Model::Cowbell: {
      float mod = oscillator(frequency_ * 2.37f, 0, 2) * fm * 1.5f; float pitch = 1.0f + sweep * .80f * fast;
      sample = oscillator(frequency_ * pitch * (1.0f + mod), 2, 0) * .55f + oscillator(frequency_ * pitch * (1.39f + tone * .18f + mod * .42f), 2, 1) * .45f + brightNoise * noise * .12f; break;
    }
    case Model::Cymbal: {
      const float ratios[] = {1.31f, 1.79f, 2.41f, 3.16f, 4.07f, 5.23f}; float metal = 0.0f;
      float base = (250.0f + tone * 360.0f + frequency_ * .45f) * (1.0f + sweep * .90f * fast);
      for (int i = 0; i < 6; ++i) metal += oscillator(base * (ratios[i] + fm * (i + 1) * .18f), 2, i) * std::exp(-age_ / (.10f + i * .075f + tone * .70f)) * (.05f + fm * .06f);
      sample = metal + brightNoise * (.10f + noise * .90f); break;
    }
    case Model::Shaker:
      sample = brightNoise * (.28f + noise * .72f) * (.45f + .55f * std::sin(age_ * (28.0f + tone * 75.0f + sweep * 180.0f))) + oscillator(frequency_ * (4.0f + tone * 6.0f), 2, 0) * fm * .16f * fast; break;
    case Model::Clave:
      sample = oscillator(frequency_ * (2.2f + tone * 1.8f) * (1.0f + sweep * 1.5f * fast), 1, 0) * .85f + oscillator(frequency_ * (3.7f + fm * 1.3f), 0, 1) * .25f + brightNoise * noise * .20f * fast; break;
  }
  float drive = 1.0f + parameters_.drive * 5.5f;
  age_ += 1.0f / sampleRate_;
  return std::tanh(sample * drive) / std::tanh(drive) * envelope_ * calibration;
}
}  // namespace bogie
