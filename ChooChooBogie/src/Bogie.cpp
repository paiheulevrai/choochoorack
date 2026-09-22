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
    configParam(LEVEL_PARAM, 0.0f, 1.0f, 0.8f, "Level");
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
    outputs[AUDIO_OUTPUT].setVoltage(5.0f * params[LEVEL_PARAM].getValue() * core.process());
  }
 private:
  float macroValue(ParamIds parameter, InputIds input, ParamIds attenuverter) {
    return limit(params[parameter].getValue() + inputs[input].getVoltage() * 0.1f * params[attenuverter].getValue(), 0.0f, 1.0f);
  }
};

struct BogieDisplay : TransparentWidget {
  Bogie* module;
  BogieDisplay(Bogie* module) : module(module) { box.size = mm2px(Vec(36.0f, 8.0f)); }
  void draw(const DrawArgs& args) override {
    nvgBeginPath(args.vg); nvgRect(args.vg, 0, 0, box.size.x, box.size.y); nvgFillColor(args.vg, nvgRGB(0x00, 0x1f, 0x00)); nvgFill(args.vg);
    nvgStrokeWidth(args.vg, 1.0f); nvgStrokeColor(args.vg, nvgRGB(0xa0, 0x60, 0x00)); nvgStroke(args.vg);
    if (APP->window->uiFont) {
      nvgFontFaceId(args.vg, APP->window->uiFont->handle); nvgFontSize(args.vg, 15.0f); nvgTextLetterSpacing(args.vg, 1.0f);
      nvgFillColor(args.vg, nvgRGB(0xef, 0xcf, 0x7f)); nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
      nvgText(args.vg, box.size.x * .5f, box.size.y * .55f, modelName(module ? modelIndex(module->params[Bogie::MODEL_PARAM].getValue()) : 0), nullptr);
    }
  }
};

struct BogieLabels : TransparentWidget {
  Bogie* module;
  BogieLabels(Bogie* module) : module(module) { box.size = mm2px(Vec(81.28f, 128.5f)); }
  void draw(const DrawArgs& args) override {
    if (!APP->window->uiFont) return;
    nvgFontFaceId(args.vg, APP->window->uiFont->handle); nvgTextLetterSpacing(args.vg, .25f); nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
    text(args, 4, 6, 16, "BOGIE", nvgRGB(0xef, 0xcf, 0x7f)); text(args, 29, 6, 7, "CHOOCHOO DRUM SYNTH", nvgRGB(0x9f, 0x9f, 0x50));
    text(args, 5.5f, 14.5f, 7, "1V/OCT", nvgRGB(0xa0, 0x60, 0x00)); text(args, 34.4f, 14.5f, 8, "PITCH", nvgRGB(0xef, 0xcf, 0x7f)); text(args, 66, 14.5f, 7, "TRIG", nvgRGB(0xa0, 0x60, 0x00));
    text(args, 7, 35, 9, "MODEL", nvgRGB(0x9f, 0x9f, 0x50));
    int model = module ? modelIndex(module->params[Bogie::MODEL_PARAM].getValue()) : 0;
    const float xs[] = {9, 49, 9, 49, 9, 49}; const float ys[] = {55, 55, 77, 77, 99, 99};
    for (int i = 0; i < 6; ++i) { text(args, xs[i], ys[i], 7.2f, macroName(model, i), nvgRGB(0xef, 0xcf, 0x7f)); text(args, xs[i], ys[i] + 4, 4.6f, "CV IN", nvgRGB(0x9f, 0x9f, 0x50)); text(args, xs[i] + 21, ys[i] + 4, 4.6f, "ATT", nvgRGB(0x9f, 0x9f, 0x50)); }
    text(args, 10, 116.5f, 8, "LEVEL", nvgRGB(0xef, 0xcf, 0x7f)); text(args, 52, 116.5f, 8, "AUDIO OUT", nvgRGB(0x9f, 0x9f, 0x50)); text(args, 4, 125, 10, "ChooChoo", nvgRGB(0xef, 0xcf, 0x7f)); text(args, 55, 125, 5.5f, "WOOD EDITION", nvgRGB(0x9f, 0x9f, 0x50));
  }
  void text(const DrawArgs& args, float x, float y, float size, const char* value, NVGcolor color) {
    nvgFontSize(args.vg, size); nvgFillColor(args.vg, color); Vec pos = mm2px(Vec(x, y)); nvgText(args.vg, pos.x, pos.y, value, nullptr);
  }
};

struct BogieWidget : ModuleWidget {
  BogieWidget(Bogie* module) {
    setModule(module); setPanel(createPanel(asset::plugin(pluginInstance, "res/Bogie.svg"))); addChild(new BogieLabels(module));
    BogieDisplay* display = new BogieDisplay(module); display->box.pos = mm2px(Vec(32, 35.5f)); addChild(display);
    addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(40.64f, 22)), module, Bogie::PITCH_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20, 40)), module, Bogie::MODEL_PARAM));
    const float x[] = {22, 62, 22, 62, 22, 62}; const float y[] = {62, 62, 84, 84, 106, 106};
    for (int i = 0; i < 6; ++i) { addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(x[i], y[i])), module, Bogie::DECAY_PARAM + i)); addParam(createParamCentered<Trimpot>(mm2px(Vec(x[i] + 11.5f, y[i])), module, Bogie::DECAY_ATTEN_PARAM + i)); }
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22, 120)), module, Bogie::LEVEL_PARAM));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 22)), module, Bogie::VOCT_INPUT)); addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71, 22)), module, Bogie::TRIG_INPUT));
    for (int i = 0; i < 6; ++i) addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x[i] - 12, y[i])), module, Bogie::DECAY_INPUT + i));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(62, 120)), module, Bogie::AUDIO_OUTPUT));
  }
};

Model* modelBogie = createModel<Bogie, BogieWidget>("Bogie");
