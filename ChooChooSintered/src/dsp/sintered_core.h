#pragma once

#include <array>
#include <cstdint>

namespace sintered {

enum class Model { Knot, Shard, Burst, Comb, Logic, Melt };

struct Parameters {
  Model model = Model::Knot;
  float decay = 82.0f / 255.0f;
  float mod = 112.0f / 255.0f;
  float a = 128.0f / 255.0f;
  float b = 96.0f / 255.0f;
  float motion = 128.0f / 255.0f;
  float c = 64.0f / 255.0f;
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
  float oscillator(float frequency, int slot);
  float random();

  Parameters parameters_{};
  float sampleRate_ = 48000.0f;
  float frequency_ = 440.0f;
  float age_ = 0.0f;
  float duration_ = 0.1f;
  float amplitude_ = 0.0f;
  std::array<float, 3> phase_{};
  float feedback_ = 0.0f;
  float dc_ = 0.0f;
  float noiseLow_ = 0.0f;
  std::array<float, 64> comb_{};
  std::uint32_t randomState_ = 0x53494e54u;
  int combIndex_ = 0;
  bool active_ = false;
};

}  // namespace sintered
