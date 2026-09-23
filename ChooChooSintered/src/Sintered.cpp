#include "plugin.hpp"
#include "dsp/sintered_core.h"

#include <algorithm>
#include <cmath>

namespace {
using sintered::Core;
using sintered::Parameters;
using SinteredModel = sintered::Model;

float limit(float value, float low, float high) { return std::max(low, std::min(value, high)); }
int modelIndex(float value) { return static_cast<int>(std::round(limit(value, 0.0f, 5.0f))); }
SinteredModel modelFromValue(float value) { return static_cast<SinteredModel>(modelIndex(value)); }
const char* modelName(int model) {
  static const char* const names[] = {"KNOT", "SHARD", "BURST", "COMB", "LOGIC", "MELT"};
  return names[std::max(0, std::min(model, 5))];
}
const char* macroName(int model, int macro) {
  static const char* const names[6][6] = {
    {"DECAY", "MOD", "RATIO", "SPREAD", "MOTION", "FOLD"},
    {"DECAY", "MOD", "RATIO", "FEEDBACK", "MOTION", "BITE"},
    {"DECAY", "MOD", "NOISE", "COLOR", "MOTION", "FEEDBACK"},
    {"DECAY", "MOD", "TIME", "DAMPING", "MOTION", "REGEN"},
    {"DECAY", "MOD", "RATE", "PATTERN", "MOTION", "CRUSH"},
    {"DECAY", "MOD", "RATIO", "CHAOS", "MOTION", "DRIVE"}};
  return names[std::max(0, std::min(model, 5))][macro];
}
}

struct Sintered : Module {
  enum ParamIds {
    PITCH_PARAM, MODEL_PARAM, DECAY_PARAM, MOD_PARAM, A_PARAM, B_PARAM, MOTION_PARAM, C_PARAM, LEVEL_PARAM,
    DECAY_ATTEN_PARAM, MOD_ATTEN_PARAM, A_ATTEN_PARAM, B_ATTEN_PARAM, MOTION_ATTEN_PARAM, C_ATTEN_PARAM, NUM_PARAMS
  };
  enum InputIds { VOCT_INPUT, TRIG_INPUT, DECAY_INPUT, MOD_INPUT, A_INPUT, B_INPUT, MOTION_INPUT, C_INPUT, NUM_INPUTS };
  enum OutputIds { AUDIO_OUTPUT, NUM_OUTPUTS };

  Core core;
  dsp::SchmittTrigger trigger;

  Sintered() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
    configParam(PITCH_PARAM, -4.0f, 4.0f, 0.0f, "Pitch", " oct");
    configParam(MODEL_PARAM, 0.0f, 5.0f, 0.0f, "Model"); getParamQuantity(MODEL_PARAM)->snapEnabled = true;
    const float defaults[] = {82.0f / 255.0f, 112.0f / 255.0f, 128.0f / 255.0f, 96.0f / 255.0f, 128.0f / 255.0f, 64.0f / 255.0f};
    const char* names[] = {"Decay", "Mod", "A", "B", "Motion", "C"};
    for (int i = 0; i < 6; ++i) {
      configParam(DECAY_PARAM + i, 0.0f, 1.0f, defaults[i], names[i]);
      configParam(DECAY_ATTEN_PARAM + i, -1.0f, 1.0f, 0.0f, std::string(names[i]) + " CV attenuverter");
      configInput(DECAY_INPUT + i, std::string(names[i]) + " CV");
    }
    configParam(LEVEL_PARAM, 0.0f, 1.0f, 1.0f, "Legacy level");
    configInput(VOCT_INPUT, "V/Oct"); configInput(TRIG_INPUT, "Trigger"); configOutput(AUDIO_OUTPUT, "Audio");
    core.init(48000.0f);
  }

  void onSampleRateChange(const SampleRateChangeEvent& event) override { core.init(event.sampleRate); }

  void process(const ProcessArgs& args) override {
    Parameters parameters;
    parameters.model = modelFromValue(params[MODEL_PARAM].getValue());
    parameters.decay = macroValue(DECAY_PARAM, DECAY_INPUT, DECAY_ATTEN_PARAM);
    parameters.mod = macroValue(MOD_PARAM, MOD_INPUT, MOD_ATTEN_PARAM);
    parameters.a = macroValue(A_PARAM, A_INPUT, A_ATTEN_PARAM);
    parameters.b = macroValue(B_PARAM, B_INPUT, B_ATTEN_PARAM);
    parameters.motion = macroValue(MOTION_PARAM, MOTION_INPUT, MOTION_ATTEN_PARAM);
    parameters.c = macroValue(C_PARAM, C_INPUT, C_ATTEN_PARAM);
    core.setParameters(parameters);
    core.setFrequency(261.625565f * std::pow(2.0f, params[PITCH_PARAM].getValue() + inputs[VOCT_INPUT].getVoltage()));
    if (trigger.process(inputs[TRIG_INPUT].getVoltage())) core.trigger();
    outputs[AUDIO_OUTPUT].setVoltage(5.0f * core.process());
  }

 private:
  float macroValue(ParamIds parameter, InputIds input, ParamIds attenuverter) {
    return limit(params[parameter].getValue() + inputs[input].getVoltage() * 0.1f * params[attenuverter].getValue(), 0.0f, 1.0f);
  }
};

struct SinteredDisplay : TransparentWidget {
  Sintered* module;
  std::shared_ptr<Font> font;
  SinteredDisplay(Sintered* module) : module(module) { box.size = mm2px(Vec(36.0f, 8.0f)); font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/FragmentMono-Regular.ttf")); }
  void draw(const DrawArgs& args) override {
    nvgBeginPath(args.vg); nvgRect(args.vg, 0, 0, box.size.x, box.size.y); nvgFillColor(args.vg, nvgRGB(0x03, 0x06, 0x09)); nvgFill(args.vg);
    nvgStrokeWidth(args.vg, 1.0f); nvgStrokeColor(args.vg, nvgRGB(0x20, 0xd9, 0xcf)); nvgStroke(args.vg);
    for (float y = 3.0f; y < box.size.y; y += 4.0f) { nvgBeginPath(args.vg); nvgMoveTo(args.vg, 1, y); nvgLineTo(args.vg, box.size.x - 1, y); nvgStrokeWidth(args.vg, .5f); nvgStrokeColor(args.vg, nvgRGBA(0xc5, 0xff, 0xf8, 32)); nvgStroke(args.vg); }
    if (font) {
      nvgFontFaceId(args.vg, font->handle); nvgFontSize(args.vg, 15.0f); nvgTextLetterSpacing(args.vg, 2.0f);
      nvgFillColor(args.vg, nvgRGB(0xc5, 0xff, 0xf8)); nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
      nvgText(args.vg, box.size.x * .5f, box.size.y * .55f, modelName(module ? modelIndex(module->params[Sintered::MODEL_PARAM].getValue()) : 0), nullptr);
      nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE); nvgText(args.vg, 7, box.size.y * .55f, ">", nullptr);
    }
  }
};

struct SinteredLabels : TransparentWidget {
  Sintered* module;
  std::shared_ptr<Font> font;
  SinteredLabels(Sintered* module) : module(module) { box.size = mm2px(Vec(81.28f, 128.5f)); font = APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf")); }
  void draw(const DrawArgs& args) override {
    if (!font) return;
    nvgFontFaceId(args.vg, font->handle); nvgTextLetterSpacing(args.vg, .25f); nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
    text(args, 4, 6, 15, "SINTERED", nvgRGB(0xd8, 0xff, 0xfb)); text(args, 39, 6, 7.5f, "PARALLEL WORLD PERCUSSIONS", nvgRGB(0xb7, 0xa8, 0xff));
    centeredText(args, 9, 14.5f, 8, "1V/OCT", nvgRGB(0x8d, 0xff, 0xf7)); centeredText(args, 30, 14.5f, 9, "PITCH", nvgRGB(0xd8, 0xff, 0xfb)); centeredText(args, 52, 14.5f, 8, "TRIG", nvgRGB(0x8d, 0xff, 0xf7)); centeredText(args, 73, 14.5f, 8, "OUT", nvgRGB(0xb7, 0xa8, 0xff));
    text(args, 5, 34, 10, "MODEL", nvgRGB(0xb7, 0xa8, 0xff));
    int model = module ? modelIndex(module->params[Sintered::MODEL_PARAM].getValue()) : 0;
    const float centers[] = {20.5f, 60.5f, 20.5f, 60.5f, 20.5f, 60.5f}; const float ys[] = {55, 55, 77, 77, 99, 99};
    for (int i = 0; i < 6; ++i) {
      centeredText(args, centers[i], ys[i], 11.0f, macroName(model, i), nvgRGB(0xd8, 0xff, 0xfb));
      centeredText(args, centers[i], ys[i] + 4, 7.5f, "-  /  +", nvgRGB(0xb7, 0xa8, 0xff));
    }
    text(args, 4, 125, 9, "ChooChoo Labs", nvgRGB(0xd8, 0xff, 0xfb)); rightText(args, 78, 125, 7, "Pierre-Emmanuel Surga", nvgRGB(0xb7, 0xa8, 0xff));
  }
  void text(const DrawArgs& args, float x, float y, float size, const char* value, NVGcolor color) {
    nvgFontSize(args.vg, size); nvgFillColor(args.vg, color); Vec pos = mm2px(Vec(x, y)); nvgText(args.vg, pos.x, pos.y, value, nullptr);
  }
  void centeredText(const DrawArgs& args, float x, float y, float size, const char* value, NVGcolor color) {
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE); text(args, x, y, size, value, color); nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  }
  void rightText(const DrawArgs& args, float x, float y, float size, const char* value, NVGcolor color) {
    nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE); text(args, x, y, size, value, color); nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  }
};

struct SinteredWidget : ModuleWidget {
  SinteredWidget(Sintered* module) {
    setModule(module); setPanel(createPanel(asset::plugin(pluginInstance, "res/Sintered.svg"))); addChild(new SinteredLabels(module));
    SinteredDisplay* display = new SinteredDisplay(module); display->box.pos = mm2px(Vec(32, 35.5f)); addChild(display);
    addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(30, 22)), module, Sintered::PITCH_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 42)), module, Sintered::MODEL_PARAM));
    const float knobX[] = {31, 50, 31, 50, 31, 50}; const float attenX[] = {20.5f, 60.5f, 20.5f, 60.5f, 20.5f, 60.5f}; const float inputX[] = {10, 71, 10, 71, 10, 71}; const float y[] = {62, 62, 84, 84, 106, 106};
    for (int i = 0; i < 6; ++i) {
      addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(knobX[i], y[i])), module, Sintered::DECAY_PARAM + i));
      addParam(createParamCentered<Trimpot>(mm2px(Vec(attenX[i], y[i])), module, Sintered::DECAY_ATTEN_PARAM + i));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(inputX[i], y[i])), module, Sintered::DECAY_INPUT + i));
    }
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9, 22)), module, Sintered::VOCT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(52, 22)), module, Sintered::TRIG_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(73, 22)), module, Sintered::AUDIO_OUTPUT));
  }
};

Model* modelSintered = createModel<Sintered, SinteredWidget>("Sintered");
