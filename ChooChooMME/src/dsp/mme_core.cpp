#include "mme_core.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace choochoomme {

namespace {

constexpr float kPi = 3.14159265358979323846f;

float clamp(float value, float low, float high) {
  return std::max(low, std::min(value, high));
}

float wrap(float value) {
  return value - std::floor(value);
}

}  // namespace

void Core::init(float sampleRate) {
  sampleRate_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
  reset();
}

void Core::setFrequency(float frequency) {
  frequency_ = clamp(frequency, 0.0f, sampleRate_ * 0.45f);
}

void Core::setParameters(const Parameters& parameters) {
  parameters_ = parameters;
  parameters_.waves = clamp(parameters_.waves, 0.0f, 1.0f);
  parameters_.interval = clamp(parameters_.interval, 0.0f, 1.0f);
  parameters_.amount = clamp(parameters_.amount, 0.0f, 1.0f);
  parameters_.flow = clamp(parameters_.flow, 0.0f, 1.0f);
  parameters_.feedback = clamp(parameters_.feedback, 0.0f, 1.0f);
  parameters_.shaper = clamp(parameters_.shaper, 0.0f, 1.0f);
}

void Core::reset() {
  phaseA_ = 0.0f;
  phaseB_ = 0.0f;
  feedback_ = 0.0f;
  feedbackDC_ = 0.0f;
  vocodeModLo_.fill(0.0f);
  vocodeModHi_.fill(0.0f);
  vocodeCarLo_.fill(0.0f);
  vocodeCarHi_.fill(0.0f);
  vocodeEnv_.fill(0.0f);
}

float Core::oscillator(float phase, int shape) const {
  phase = wrap(phase);
  switch (shape) {
    case 1:
      return 1.0f - 4.0f * std::fabs(phase - 0.5f);
    case 2:
      return 2.0f * phase - 1.0f;
    case 3:
      return phase < 0.5f ? 1.0f : -1.0f;
    case 4:
      return std::sin(2.0f * kPi * phase) * std::sin(2.0f * kPi * phase);
    default:
      return std::sin(2.0f * kPi * phase);
  }
}

float Core::diode(float value) const {
  float sign = value > 0.0f ? 1.0f : -1.0f;
  float dead = std::fabs(value) - 0.667f;
  dead += std::fabs(dead);
  return 0.043247658f * dead * dead * sign;
}

float Core::shape(float value) const {
  float amount = parameters_.shaper;
  if (amount <= 0.0f)
    return value;
  float saturated = std::tanh(value * (1.0f + amount * 5.0f));
  float folded = std::fabs(std::fmod(saturated * (1.0f + amount * 3.0f) + 1.0f, 4.0f) - 2.0f) - 1.0f;
  float foldMix = clamp((amount - 0.55f) / 0.45f, 0.0f, 1.0f);
  return saturated + (folded - saturated) * foldMix;
}

float Core::vocode(float modulator, float carrier) {
  float result = 0.0f;
  float shift = parameters_.flow;
  float release = 0.003f + (1.0f - parameters_.amount) * 0.08f;
  for (int i = 0; i < 20; ++i) {
    float t = static_cast<float>(i) / 19.0f;
    float hz = 90.0f * std::pow(42.0f, clamp(t + (shift - 0.5f) * 0.26f, 0.0f, 1.0f));
    float coefficient = clamp(2.0f * kPi * hz / sampleRate_, 0.0001f, 0.45f);
    vocodeModLo_[i] += coefficient * (modulator - vocodeModLo_[i]);
    vocodeModHi_[i] += coefficient * (vocodeModLo_[i] - vocodeModHi_[i]);
    vocodeCarLo_[i] += coefficient * (carrier - vocodeCarLo_[i]);
    vocodeCarHi_[i] += coefficient * (vocodeCarLo_[i] - vocodeCarHi_[i]);
    float envelope = std::fabs(vocodeModLo_[i] - vocodeModHi_[i]);
    float rate = envelope > vocodeEnv_[i] ? 0.18f : release;
    vocodeEnv_[i] += rate * (envelope - vocodeEnv_[i]);
    result += (vocodeCarLo_[i] - vocodeCarHi_[i]) * clamp(vocodeEnv_[i] * 6.0f, 0.0f, 2.0f);
  }
  return result * 0.18f;
}

float Core::process() {
  float interval = (parameters_.interval - 0.5f) * 4.0f;
  float ratio = std::pow(2.0f, interval);
  int pair = static_cast<int>(parameters_.waves * 5.0f);
  int shapeA = pair == 0 ? 0 : pair == 1 ? 1 : pair == 2 ? 2 : pair == 3 ? 0 : 3;
  int shapeB = pair == 0 ? 0 : pair == 1 ? 0 : pair == 2 ? 1 : pair == 3 ? 2 : 3;
  float amount = parameters_.amount;
  float flow = parameters_.flow;
  float feedbackControl = parameters_.feedback;
  float feedbackGain = feedbackControl * feedbackControl * 2.5f;
  float stepA = frequency_ / sampleRate_;
  float stepB = frequency_ * ratio / sampleRate_;
  bool masterWraps = phaseA_ + stepA >= 1.0f;

  if (parameters_.model == Model::Sync && masterWraps)
    phaseB_ += (flow - phaseB_) * amount;

  float injected = feedback_ * feedbackGain;
  float a = std::tanh(oscillator(phaseA_, shapeA) + injected * (0.35f + 0.75f * flow));
  float b = std::tanh(oscillator(phaseB_, shapeB) - injected * (1.10f - 0.50f * flow));
  float sample = 0.0f;

  switch (parameters_.model) {
    case Model::Ring: {
      float analog = std::tanh((diode(a + b * amount * 2.0f) + diode(a - b * amount * 2.0f)) * 12.0f);
      float digital = 4.0f * a * b * amount;
      digital /= 1.0f + std::fabs(digital);
      sample = analog + (digital - analog) * flow;
      break;
    }
    case Model::Fold: {
      float sum = (a + b * (0.15f + amount) + a * b * 0.25f) * (0.02f + amount * 1.4f);
      sample = std::fabs(std::fmod(sum + 1.0f, 4.0f) - 2.0f) - 1.0f;
      break;
    }
    case Model::Cross: {
      float ab = oscillator(phaseA_ + (b + feedback_) * amount * 0.28f, shapeA);
      float ba = oscillator(phaseB_ + (a + feedback_) * amount * 0.28f, shapeB);
      sample = ab + (ba - ab) * flow;
      break;
    }
    case Model::Vpm: {
      float ab = oscillator(phaseA_ + (b + feedback_) * amount * 0.45f, shapeA);
      float ba = oscillator(phaseB_ + (a + feedback_) * amount * 0.45f, shapeB);
      sample = ab + (ba - ab) * flow;
      break;
    }
    case Model::Sync:
      sample = oscillator(phaseB_, shapeB) + a * (amount * 0.18f);
      break;
    case Model::Logic: {
      int16_t ia = static_cast<int16_t>(a * 32767.0f);
      int16_t ib = static_cast<int16_t>(b * 32767.0f);
      float xorValue = static_cast<float>(ia ^ ib) / 32768.0f;
      float compare = std::fabs(a) > std::fabs(b) ? a : b;
      sample = (a + b) * (1.0f - amount) * 0.5f + (xorValue + (compare - xorValue) * flow) * amount;
      break;
    }
    case Model::Vocode:
      sample = vocode(b * amount, a);
      break;
  }

  sample = std::tanh(sample + injected * 1.2f);
  sample = shape(sample);
  float rawFeedback = std::tanh(feedback_ * (0.10f + feedbackGain) + sample * (0.45f + feedbackControl * 1.9f));
  feedbackDC_ += 0.025f * (rawFeedback - feedbackDC_);
  feedback_ = (rawFeedback - feedbackDC_) * (0.72f + feedbackControl * 0.22f);
  phaseA_ = wrap(phaseA_ + stepA);
  phaseB_ = wrap(phaseB_ + stepB);
  return sample;
}

}  // namespace choochoomme