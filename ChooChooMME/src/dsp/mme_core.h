#pragma once

#include <array>

namespace choochoomme {

enum class Model {
  Ring,
  Fold,
  Cross,
  Vpm,
  Sync,
  Logic,
  Vocode,
};

struct Parameters {
  Model model = Model::Ring;
  float waves = 0.0f;
  float interval = 0.5f;
  float amount = 0.5f;
  float flow = 0.5f;
  float feedback = 0.0f;
  float shaper = 0.0f;
};

class Core {
 public:
  void init(float sampleRate);
  void setFrequency(float frequency);
  void setParameters(const Parameters& parameters);
  void reset();
  float process();

 private:
  float oscillator(float phase, int shape) const;
  float diode(float value) const;
  float shape(float value) const;
  float vocode(float modulator, float carrier);

  Parameters parameters_{};
  float sampleRate_ = 48000.0f;
  float frequency_ = 440.0f;
  float phaseA_ = 0.0f;
  float phaseB_ = 0.0f;
  float feedback_ = 0.0f;
  float feedbackDC_ = 0.0f;
  std::array<float, 20> vocodeModLo_{};
  std::array<float, 20> vocodeModHi_{};
  std::array<float, 20> vocodeCarLo_{};
  std::array<float, 20> vocodeCarHi_{};
  std::array<float, 20> vocodeEnv_{};
};

}  // namespace choochoomme