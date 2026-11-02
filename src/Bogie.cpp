#include "plugin.hpp"
#include "dsp/bogie_core.h"

#include <algorithm>
#include <cmath>

namespace {
using bogie::Core;
using BogieModel = bogie::Model;
using bogie::Parameters;

float limit(float value, float low, float high) { return std::max(low, std::min(value, high)); }
BogieModel modelFromValue(float value) { return static_cast<BogieModel>(static_cast<int>(std::round(limit(value, 0.0f, 11.0f)))); }
int modelIndex(float value) { return static_cast<int>(std::round(limit(value, 0.0f, 11.0f))); }
const char* modelName(int model) {
  static const char* const names[] = {"KICK", "SNARE", "HAT", "CLAP", "TOM", "RIM", "FM", "NOISE", "COWBELL", "CYMBAL", "SHAKER", "CLAVE"};
  return names[std::max(0, std::min(model, 11))];
}
const char* macroName(int model, int macro) {
  static const char* const names[12][6] = {
    {"DECAY", "TONE", "SWEEP", "CLICK", "HARM", "DRIVE"}, {"DECAY", "TONE", "SNAP", "WIRE", "BODY", "DRIVE"},
    {"DECAY", "TONE", "PITCH ENV", "NOISE", "METAL", "DRIVE"}, {"DECAY", "TONE", "SPACING", "NOISE", "TONE MIX", "DRIVE"},
    {"DECAY", "TONE", "SWEEP", "NOISE", "HARM", "DRIVE"}, {"DECAY", "TONE", "SWEEP", "NOISE", "RATIO", "DRIVE"},
    {"DECAY", "RATIO", "PITCH ENV", "NOISE", "INDEX", "DRIVE"}, {"DECAY", "OSC PITCH", "COLOR", "LEVEL", "TONE MIX", "DRIVE"},
    {"DECAY", "DETUNE", "PITCH ENV", "NOISE", "CROSSMOD", "DRIVE"}, {"DECAY", "TONE", "PITCH ENV", "NOISE", "METAL", "DRIVE"},
    {"DECAY", "RATE", "MOTION", "NOISE", "TONE MIX", "DRIVE"}, {"DECAY", "TONE", "PITCH ENV", "NOISE", "RATIO", "DRIVE"}};
  return names[std::max(0, std::min(model, 11))][macro];
}
}

struct Bogie : Module {
  enum ParamIds {
    PITCH_PARAM, MODEL_PARAM, DECAY_PARAM, TONE_PARAM, SWEEP_PARAM, NOISE_PARAM, FM_PARAM, DRIVE_PARAM, LEVEL_PARAM,
    DECAY_ATTEN_PARAM, TONE_ATTEN_PARAM, SWEEP_ATTEN_PARAM, NOISE_ATTEN_PARAM, FM_ATTEN_PARAM, DRIVE_ATTEN_PARAM, NUM_PARAMS
  };
  enum InputIds { VOCT_INPUT, TRIG_INPUT, DECAY_INPUT, TONE_INPUT, SWEEP_INPUT, NOISE_INPUT, FM_INPUT, DRIVE_INPUT, NUM_INPUTS };
  enum OutputIds { AUDIO_OUTPUT, NUM_OUTPUTS };

  Core core;
  dsp::SchmittTrigger trigger;

  Bogie() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
    configParam(PITCH_PARAM, -4.0f, 4.0f, 0.0f, "Pitch", " oct");
    configParam(MODEL_PARAM, 0.0f, 11.0f, 0.0f, "Model"); getParamQuantity(MODEL_PARAM)->snapEnabled = true;
    configParam(DECAY_PARAM, 0.0f, 1.0f, 72.0f / 255.0f, "Decay");
    configParam(TONE_PARAM, 0.0f, 1.0f, 128.0f / 255.0f, "Tone");
    configParam(SWEEP_PARAM, 0.0f, 1.0f, 0.5f, "Sweep"); configParam(NOISE_PARAM, 0.0f, 1.0f, 0.5f, "Noise");
    configParam(FM_PARAM, 0.0f, 1.0f, 0.5f, "FM"); configParam(DRIVE_PARAM, 0.0f, 1.0f, 0.0f, "Drive");
    configParam(LEVEL_PARAM, 0.0f, 1.0f, 1.0f, "Legacy level");
    const char* names[] = {"Decay", "Tone", "Sweep", "Noise", "FM", "Drive"};
    for (int i = 0; i < 6; ++i) configParam(DECAY_ATTEN_PARAM + i, -1.0f, 1.0f, 0.0f, std::string(names[i]) + " CV attenuverter");
    configInput(VOCT_INPUT, "V/Oct"); configInput(TRIG_INPUT, "Trigger");
    for (int i = 0; i < 6; ++i) configInput(DECAY_INPUT + i, std::string(names[i]) + " CV");
    configOutput(AUDIO_OUTPUT, "Audio");
    core.init(48000.0f);
  }
  void onSampleRateChange(const SampleRateChangeEvent& event) override { core.init(event.sampleRate); }
  void process(const ProcessArgs& args) override {
    Parameters parameters;
    parameters.model = modelFromValue(params[MODEL_PARAM].getValue());
    parameters.decay = macroValue(DECAY_PARAM, DECAY_INPUT, DECAY_ATTEN_PARAM);
    parameters.tone = macroValue(TONE_PARAM, TONE_INPUT, TONE_ATTEN_PARAM);
    parameters.sweep = macroValue(SWEEP_PARAM, SWEEP_INPUT, SWEEP_ATTEN_PARAM);
    parameters.noise = macroValue(NOISE_PARAM, NOISE_INPUT, NOISE_ATTEN_PARAM);
    parameters.fm = macroValue(FM_PARAM, FM_INPUT, FM_ATTEN_PARAM);
    parameters.drive = macroValue(DRIVE_PARAM, DRIVE_INPUT, DRIVE_ATTEN_PARAM);
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

struct BogieDisplay : TransparentWidget {
  Bogie* module;
  std::shared_ptr<Font> font;
  BogieDisplay(Bogie* module) : module(module) { box.size = mm2px(Vec(36.0f, 8.0f)); font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/FragmentMono-Regular.ttf")); }
  void draw(const DrawArgs& args) override {
    nvgBeginPath(args.vg); nvgRect(args.vg, 0, 0, box.size.x, box.size.y); nvgFillColor(args.vg, nvgRGB(0x03, 0x07, 0x03)); nvgFill(args.vg);
    nvgStrokeWidth(args.vg, 1.0f); nvgStrokeColor(args.vg, nvgRGB(0xa0, 0x60, 0x00)); nvgStroke(args.vg);
    for (float y = 3.0f; y < box.size.y; y += 4.0f) { nvgBeginPath(args.vg); nvgMoveTo(args.vg, 1, y); nvgLineTo(args.vg, box.size.x - 1, y); nvgStrokeWidth(args.vg, .5f); nvgStrokeColor(args.vg, nvgRGBA(0xef, 0xcf, 0x7f, 32)); nvgStroke(args.vg); }
    if (font) {
      nvgFontFaceId(args.vg, font->handle); nvgFontSize(args.vg, 15.0f); nvgTextLetterSpacing(args.vg, 2.0f);
      nvgFillColor(args.vg, nvgRGB(0xef, 0xcf, 0x7f)); nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
      nvgText(args.vg, box.size.x * .5f, box.size.y * .55f, modelName(module ? modelIndex(module->params[Bogie::MODEL_PARAM].getValue()) : 0), nullptr);
      nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE); nvgText(args.vg, 7, box.size.y * .55f, ">", nullptr);
    }
  }
};

struct BogieLabels : TransparentWidget {
  Bogie* module;
  std::shared_ptr<Font> font;
  BogieLabels(Bogie* module) : module(module) { box.size = mm2px(Vec(81.28f, 128.5f)); font = APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf")); }
  void draw(const DrawArgs& args) override {
    if (!font) return;
    nvgFontFaceId(args.vg, font->handle); nvgTextLetterSpacing(args.vg, .25f); nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
    text(args, 4, 6, 16, "BOGIE", nvgRGB(0xff, 0xe8, 0xa0)); text(args, 29, 6, 8, "CHOOCHOO DRUM SYNTH", nvgRGB(0xd9, 0xcf, 0x87));
    centeredText(args, 9, 14.5f, 8, "1V/OCT", nvgRGB(0xff, 0xe8, 0xa0)); centeredText(args, 30, 14.5f, 9, "PITCH", nvgRGB(0xff, 0xe8, 0xa0)); centeredText(args, 52, 14.5f, 8, "TRIG", nvgRGB(0xff, 0xe8, 0xa0)); centeredText(args, 73, 14.5f, 8, "OUT", nvgRGB(0xff, 0xe8, 0xa0));
    text(args, 5, 34, 10, "MODEL", nvgRGB(0xd9, 0xcf, 0x87));
    int model = module ? modelIndex(module->params[Bogie::MODEL_PARAM].getValue()) : 0;
    const float centers[] = {20.5f, 60.5f, 20.5f, 60.5f, 20.5f, 60.5f}; const float ys[] = {55, 55, 77, 77, 99, 99};
    for (int i = 0; i < 6; ++i) {
      centeredText(args, centers[i], ys[i], 11.0f, macroName(model, i), nvgRGB(0xff, 0xe8, 0xa0));
      centeredText(args, centers[i], ys[i] + 4, 7.5f, "-  /  +", nvgRGB(0xd9, 0xcf, 0x87));
    }
    text(args, 4, 125, 9, "ChooChoo Labs", nvgRGB(0xff, 0xe8, 0xa0)); rightText(args, 78, 125, 7, "Pierre-Emmanuel Surga", nvgRGB(0xd9, 0xcf, 0x87));
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

struct BogieWidget : ModuleWidget {
  BogieWidget(Bogie* module) {
    setModule(module); setPanel(createPanel(asset::plugin(pluginInstance, "res/Bogie.svg"))); addChild(new BogieLabels(module));
    BogieDisplay* display = new BogieDisplay(module); display->box.pos = mm2px(Vec(32, 35.5f)); addChild(display);
    addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(30, 22)), module, Bogie::PITCH_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 42)), module, Bogie::MODEL_PARAM));
    const float knobX[] = {31, 50, 31, 50, 31, 50}; const float attenX[] = {20.5f, 60.5f, 20.5f, 60.5f, 20.5f, 60.5f}; const float inputX[] = {10, 71, 10, 71, 10, 71}; const float y[] = {62, 62, 84, 84, 106, 106};
    for (int i = 0; i < 6; ++i) { addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(knobX[i], y[i])), module, Bogie::DECAY_PARAM + i)); addParam(createParamCentered<Trimpot>(mm2px(Vec(attenX[i], y[i])), module, Bogie::DECAY_ATTEN_PARAM + i)); }
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9, 22)), module, Bogie::VOCT_INPUT)); addInput(createInputCentered<PJ301MPort>(mm2px(Vec(52, 22)), module, Bogie::TRIG_INPUT));
    for (int i = 0; i < 6; ++i) addInput(createInputCentered<PJ301MPort>(mm2px(Vec(inputX[i], y[i])), module, Bogie::DECAY_INPUT + i));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(73, 22)), module, Bogie::AUDIO_OUTPUT));
  }
};

Model* modelBogie = createModel<Bogie, BogieWidget>("Bogie");
