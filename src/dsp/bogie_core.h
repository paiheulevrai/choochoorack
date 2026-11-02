#pragma once

#include <array>

namespace bogie {

enum class Model { Kick, Snare, Hat, Clap, Tom, Rim, Fm, Noise, Cowbell, Cymbal, Shaker, Clave };

struct Parameters {
  Model model = Model::Kick;
  float decay = 72.0f / 255.0f;
  float tone = 128.0f / 255.0f;
  float sweep = 0.5f;
  float noise = 0.5f;
  float fm = 0.5f;
  float drive = 0.0f;
};

class Core {
 public:
  void init(float sampleRate);
  void setFrequency(float frequency);
  void setParameters(const Parameters& parameters);
  void trigger();
  float process();
  bool active() const { return active_; }

 private:
  float random();
  float oscillator(float frequency, int shape, int slot);

  Parameters parameters_{};
  float sampleRate_ = 48000.0f;
  float frequency_ = 110.0f;
  float age_ = 0.0f;
  float duration_ = 0.0f;
  float envelope_ = 0.0f;
  float noiseLow_ = 0.0f;
  std::array<float, 8> phase_{};
  unsigned int randomState_ = 0x43525452u;
  bool active_ = false;
};

}  // namespace bogie
