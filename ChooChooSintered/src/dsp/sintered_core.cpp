#include "sintered_core.h"

#include <algorithm>
#include <cmath>

namespace sintered {
namespace {
constexpr float kPi = 3.14159265358979323846f;
float clamp(float value) { return std::max(0.0f, std::min(value, 1.0f)); }
float fold(float value) { return std::fabs(std::fmod(value + 1.0f, 4.0f) - 2.0f) - 1.0f; }
int modelIndex(Model model) { return std::max(0, std::min(static_cast<int>(model), 5)); }
}

void Core::init(float sampleRate) {
  sampleRate_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
  active_ = false;
  age_ = amplitude_ = feedback_ = dc_ = noiseLow_ = 0.0f;
  phase_.fill(0.0f);
  comb_.fill(0.0f);
}

void Core::setFrequency(float frequency) {
  frequency_ = std::max(10.0f, std::min(frequency, sampleRate_ * 0.45f));
}

void Core::setParameters(const Parameters& parameters) {
  parameters_ = parameters;
  parameters_.decay = clamp(parameters_.decay);
  parameters_.mod = clamp(parameters_.mod);
  parameters_.a = clamp(parameters_.a);
  parameters_.b = clamp(parameters_.b);
  parameters_.motion = clamp(parameters_.motion);
  parameters_.c = clamp(parameters_.c);
}

void Core::trigger() {
  age_ = amplitude_ = feedback_ = dc_ = noiseLow_ = 0.0f;
  combIndex_ = 0;
  active_ = true;
  randomState_ = 0x53494e54u;
  phase_.fill(0.0f);
  comb_.fill(0.0f);
  static const float tails[] = {.85f, .70f, .48f, 1.10f, .42f, .80f};
  duration_ = .018f + parameters_.decay * parameters_.decay * tails[modelIndex(parameters_.model)];
}

float Core::random() {
  randomState_ = randomState_ * 1664525u + 1013904223u;
  return ((randomState_ >> 8) * (1.0f / 8388608.0f)) - 1.0f;
}

float Core::oscillator(float frequency, int slot) {
  phase_[slot] += frequency / sampleRate_;
  phase_[slot] -= std::floor(phase_[slot]);
  return std::sin(2.0f * kPi * phase_[slot]);
}

float Core::process() {
  if (!active_ || age_ >= duration_) {
    active_ = false;
    return 0.0f;
  }

  static constexpr float modelGain[] = {1.822647f, 1.995262f, 1.995262f, 1.995262f, 1.995262f, 1.995262f};
  const float calibration = modelGain[modelIndex(parameters_.model)];
  float mod = parameters_.mod, a = parameters_.a, b = parameters_.b, c = parameters_.c;
  float motion = (parameters_.motion * 255.0f - 128.0f) / 127.0f;
  if (std::fabs(motion) > 1.0e-5f) {
    float motionTime = .008f + (1.0f - std::min(std::fabs(motion), 1.0f)) * .35f;
    float x = age_ / motionTime;
    float movement = motion < 0.0f ? (x < 1.0f ? std::sin(kPi * x) : 0.0f) : std::exp(-6.0f * x);
    switch (parameters_.model) {
      case Model::Knot: mod = clamp(mod + movement * .65f); c = clamp(c + movement * .55f); break;
      case Model::Shard: b = clamp(b + movement * .65f); c = clamp(c + movement * .55f); break;
      case Model::Burst: a = clamp(a + movement * .70f); b = clamp(b + movement * .55f); c = clamp(c + movement * .35f); break;
      case Model::Comb: b = clamp(b + movement * .45f); c = clamp(c + movement * .55f); break;
      case Model::Logic: a = clamp(a + movement * .60f); c = clamp(c + movement * .70f); break;
      case Model::Melt: b = clamp(b + movement * .70f); c = clamp(c + movement * .60f); break;
    }
  }

  float noise = random();
  noiseLow_ += (noise - noiseLow_) * (.01f + b * .25f);
  float bright = noise - noiseLow_;
  float f2 = frequency_ * std::pow(2.0f, (a - .5f) * 5.0f);
  float x = oscillator(frequency_, 0);
  float y = oscillator(f2 + feedback_ * frequency_ * mod * 1.5f, 1);
  float sample = 0.0f;

  switch (parameters_.model) {
    case Model::Knot: {
      float z = oscillator(frequency_ * std::pow(2.0f, (b - .5f) * 7.0f), 2);
      sample = std::sin(2.0f * kPi * (phase_[0] + y * mod * .32f + z * mod * .18f));
      sample = sample * (1.0f - c) + fold(sample * (1.0f + c * 14.0f)) * c;
      break;
    }
    case Model::Shard: {
      float teeth = fold((x + y * (1.0f + mod * 6.0f) + feedback_ * b * 4.0f) * (1.0f + c * 12.0f));
      sample = std::tanh(teeth * (1.0f + c * 5.0f));
      break;
    }
    case Model::Burst: {
      float color = bright * (1.0f - b) + noiseLow_ * b;
      sample = color * (a * (1.2f + mod * .8f)) + y * (1.0f - a) + feedback_ * mod * 1.5f;
      sample = fold(sample * (1.0f + c * 8.0f));
      break;
    }
    case Model::Comb: {
      int delay = 1 + static_cast<int>(a * 62.0f);
      float delayed = comb_[(combIndex_ + 64 - delay) & 63];
      float excite = x + y * mod * .75f + bright * mod * .25f;
      comb_[combIndex_] = std::tanh((excite + delayed * c * 1.35f) * (.4f + mod * 1.6f));
      combIndex_ = (combIndex_ + 1) & 63;
      sample = delayed * (1.0f - b * .92f) + excite * .18f;
      break;
    }
    case Model::Logic: {
      int q = 2 + static_cast<int>(a * 126.0f), pattern = static_cast<int>(b * 3.99f);
      int ia = static_cast<int>((x + 1.0f) * q), ib = static_cast<int>((y + 1.0f) * q);
      int logic = pattern == 0 ? (ia ^ ib) : pattern == 1 ? (ia & ib) : pattern == 2 ? (ia | ib) : (ia > ib ? ia : ib);
      sample = ((logic % (q * 2)) / static_cast<float>(q) - 1.0f) * mod + x * (1.0f - mod);
      sample = fold(sample * (1.0f + c * 15.0f));
      break;
    }
    case Model::Melt: {
      float warped = oscillator(frequency_ * (1.0f + y * mod * (3.0f + b * 24.0f) + feedback_ * b * 6.0f), 2);
      sample = std::tanh(warped * (1.0f + c * 13.0f) + bright * mod * b);
      break;
    }
  }

  float impact = bright * std::exp(-age_ / (.0015f + .006f * (1.0f - b)));
  static const float impactMix[] = {.22f, .18f, .72f, .38f, .24f, .16f};
  sample += impact * impactMix[modelIndex(parameters_.model)] * (.25f + .75f * mod);
  float raw = std::tanh(sample + feedback_ * (.2f + mod * 1.4f));
  dc_ += .02f * (raw - dc_);
  float target = (raw - dc_) * (.12f + parameters_.mod * .82f);
  feedback_ += (target - feedback_) * (.025f + .10f * (1.0f - c));
  amplitude_ = std::exp(-5.5f * age_ / duration_);
  age_ += 1.0f / sampleRate_;
  return std::tanh(std::tanh(sample) * amplitude_ * calibration);
}

}  // namespace sintered
