#include "plugin.hpp"
#include "dsp/mme_core.h"

#include <algorithm>
#include <cmath>

namespace {

using choochoomme::Core;
using MMEModel = choochoomme::Model;
using choochoomme::Parameters;

float limit(float value, float low, float high) {
  return std::max(low, std::min(value, high));
}

MMEModel modelFromValue(float value) {
  switch (static_cast<int>(std::round(limit(value, 0.0f, 6.0f)))) {
    case 1: return MMEModel::Fold;
    case 2: return MMEModel::Cross;
    case 3: return MMEModel::Vpm;
    case 4: return MMEModel::Sync;
    case 5: return MMEModel::Logic;
    case 6: return MMEModel::Vocode;
    default: return MMEModel::Ring;
  }
}

}  // namespace

struct ChooChooMME : Module {
  enum ParamIds {
    PITCH_PARAM,
    MODEL_PARAM,
    WAVES_PARAM,
    INTERVAL_PARAM,
    AMOUNT_PARAM,
    FLOW_PARAM,
    FEEDBACK_PARAM,
    SHAPER_PARAM,
    WAVES_ATTEN_PARAM,
    INTERVAL_ATTEN_PARAM,
    AMOUNT_ATTEN_PARAM,
    FLOW_ATTEN_PARAM,
    FEEDBACK_ATTEN_PARAM,
    SHAPER_ATTEN_PARAM,
    NUM_PARAMS
  };
  enum InputIds {
    VOCT_INPUT,
    TRIG_INPUT,
    WAVES_INPUT,
    INTERVAL_INPUT,
    AMOUNT_INPUT,
    FLOW_INPUT,
    FEEDBACK_INPUT,
    SHAPER_INPUT,
    NUM_INPUTS
  };
  enum OutputIds {
    VCO_OUTPUT,
    NUM_OUTPUTS
  };

  Core core;
  dsp::SchmittTrigger trigger;

  ChooChooMME() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
    configParam(PITCH_PARAM, -4.0f, 4.0f, 0.0f, "Pitch", " oct");
    configParam(MODEL_PARAM, 0.0f, 6.0f, 0.0f, "Model");
    getParamQuantity(MODEL_PARAM)->snapEnabled = true;
    configParam(WAVES_PARAM, 0.0f, 1.0f, 0.0f, "Waves");
    configParam(INTERVAL_PARAM, 0.0f, 1.0f, 0.5f, "Interval");
    configParam(AMOUNT_PARAM, 0.0f, 1.0f, 0.5f, "Amount");
    configParam(FLOW_PARAM, 0.0f, 1.0f, 0.5f, "Flow");
    configParam(FEEDBACK_PARAM, 0.0f, 1.0f, 0.0f, "Feedback");
    configParam(SHAPER_PARAM, 0.0f, 1.0f, 0.0f, "Shaper");
    configParam(WAVES_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Waves CV attenuverter");
    configParam(INTERVAL_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Interval CV attenuverter");
    configParam(AMOUNT_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Amount CV attenuverter");
    configParam(FLOW_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Flow CV attenuverter");
    configParam(FEEDBACK_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Feedback CV attenuverter");
    configParam(SHAPER_ATTEN_PARAM, -1.0f, 1.0f, 0.0f, "Shaper CV attenuverter");
    configInput(VOCT_INPUT, "V/Oct");
    configInput(TRIG_INPUT, "Trigger");
    configInput(WAVES_INPUT, "Waves CV");
    configInput(INTERVAL_INPUT, "Interval CV");
    configInput(AMOUNT_INPUT, "Amount CV");
    configInput(FLOW_INPUT, "Flow CV");
    configInput(FEEDBACK_INPUT, "Feedback CV");
    configInput(SHAPER_INPUT, "Shaper CV");
    configOutput(VCO_OUTPUT, "VCO");
    core.init(48000.0f);
  }

  void onSampleRateChange(const SampleRateChangeEvent& event) override {
    core.init(event.sampleRate);
  }

  void process(const ProcessArgs& args) override {
    if (inputs[TRIG_INPUT].isConnected() && trigger.process(inputs[TRIG_INPUT].getVoltage()))
      core.reset();

    Parameters parameters;
    parameters.model = modelFromValue(params[MODEL_PARAM].getValue());
    parameters.waves = cvValue(WAVES_PARAM, WAVES_INPUT, WAVES_ATTEN_PARAM);
    parameters.interval = cvValue(INTERVAL_PARAM, INTERVAL_INPUT, INTERVAL_ATTEN_PARAM);
    parameters.amount = cvValue(AMOUNT_PARAM, AMOUNT_INPUT, AMOUNT_ATTEN_PARAM);
    parameters.flow = cvValue(FLOW_PARAM, FLOW_INPUT, FLOW_ATTEN_PARAM);
    parameters.feedback = cvValue(FEEDBACK_PARAM, FEEDBACK_INPUT, FEEDBACK_ATTEN_PARAM);
    parameters.shaper = cvValue(SHAPER_PARAM, SHAPER_INPUT, SHAPER_ATTEN_PARAM);
    core.setFrequency(261.625565f * std::pow(2.0f, params[PITCH_PARAM].getValue() +
        (inputs[VOCT_INPUT].isConnected() ? inputs[VOCT_INPUT].getVoltage() : 0.0f)));
    core.setParameters(parameters);
    outputs[VCO_OUTPUT].setVoltage(5.0f * core.process());
  }

 private:
  float cvValue(ParamIds parameter, InputIds input, ParamIds attenuverter) {
    float value = params[parameter].getValue();
    if (inputs[input].isConnected())
      value += inputs[input].getVoltage() / 10.0f * params[attenuverter].getValue();
    return limit(value, 0.0f, 1.0f);
  }
};

struct ModelDisplay : TransparentWidget {
  ChooChooMME* module;

  ModelDisplay(ChooChooMME* module) : module(module) {
    box.size = mm2px(Vec(36.0f, 8.0f));
  }

  void draw(const DrawArgs& args) override {
    static const char* const names[] = {"RING", "FOLD", "CROSS", "VPM", "SYNC", "LOGIC", "VOCODE"};
    int model = module ? static_cast<int>(std::round(limit(module->params[ChooChooMME::MODEL_PARAM].getValue(), 0.0f, 6.0f))) : 0;

    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0.0f, 0.0f, box.size.x, box.size.y);
    nvgFillColor(args.vg, nvgRGB(0x18, 0x08, 0x0a));
    nvgFill(args.vg);
    nvgStrokeWidth(args.vg, 1.0f);
    nvgStrokeColor(args.vg, nvgRGB(0xe8, 0x1d, 0x32));
    nvgStroke(args.vg);

    if (APP->window->uiFont) {
      nvgFontFaceId(args.vg, APP->window->uiFont->handle);
      nvgFontSize(args.vg, 16.0f);
      nvgTextLetterSpacing(args.vg, 1.2f);
      nvgFillColor(args.vg, nvgRGB(0xff, 0xd5, 0x24));
      nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
      nvgText(args.vg, box.size.x * 0.5f, box.size.y * 0.55f, names[model], nullptr);
    }
    Widget::draw(args);
  }
};

struct LabelOverlay : TransparentWidget {
  LabelOverlay() {
    box.size = mm2px(Vec(81.28f, 128.5f));
  }

  void draw(const DrawArgs& args) override {
    if (!APP->window->uiFont)
      return;
    nvgFontFaceId(args.vg, APP->window->uiFont->handle);
    nvgTextLetterSpacing(args.vg, 0.35f);
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

    drawText(args, 4.0f, 6.0f, 16.0f, "MME", nvgRGB(0xf2, 0xf2, 0xed));
    drawText(args, 27.0f, 6.0f, 7.0f, "MULTI MODULATION ENGINE", nvgRGB(0xf0, 0xc9, 0x29));
    drawText(args, 5.5f, 14.5f, 7.0f, "1V/OCT", nvgRGB(0xf2, 0xf2, 0xed));
    drawText(args, 34.4f, 14.5f, 8.0f, "PITCH", nvgRGB(0xf2, 0xf2, 0xed));
    drawText(args, 65.5f, 14.5f, 7.0f, "RESET", nvgRGB(0xf2, 0xf2, 0xed));
    drawText(args, 7.0f, 35.0f, 9.0f, "MODEL", nvgRGB(0xf0, 0xc9, 0x29));

    drawControlLabel(args, 9.0f, 55.0f, "WAVES");
    drawControlLabel(args, 49.0f, 55.0f, "INTERVAL");
    drawControlLabel(args, 9.0f, 77.0f, "AMOUNT");
    drawControlLabel(args, 49.0f, 77.0f, "FLOW");
    drawControlLabel(args, 9.0f, 99.0f, "FEEDBACK");
    drawControlLabel(args, 49.0f, 99.0f, "SHAPER");
    drawText(args, 37.0f, 116.5f, 8.0f, "AUDIO OUT", nvgRGB(0xf0, 0xc9, 0x29));
    drawText(args, 4.0f, 125.0f, 10.0f, "ChooChoo", nvgRGB(0xf2, 0xf2, 0xed));
    drawText(args, 55.0f, 125.0f, 5.5f, "BERLIN / SIGNAL DIV.", nvgRGB(0x9c, 0xa3, 0xaa));
    Widget::draw(args);
  }

 private:
  void drawText(const DrawArgs& args, float x, float y, float size, const char* text, NVGcolor color) {
    nvgFontSize(args.vg, size);
    nvgFillColor(args.vg, color);
    Vec pos = mm2px(Vec(x, y));
    nvgText(args.vg, pos.x, pos.y, text, nullptr);
  }

  void drawControlLabel(const DrawArgs& args, float x, float y, const char* text) {
    drawText(args, x, y, 8.0f, text, nvgRGB(0xf2, 0xf2, 0xed));
    drawText(args, x, y + 4.0f, 5.0f, "CV IN", nvgRGB(0x9c, 0xa3, 0xaa));
    drawText(args, x + 21.0f, y + 4.0f, 5.0f, "ATT", nvgRGB(0x9c, 0xa3, 0xaa));
  }
};

struct ChooChooMMEWidget : ModuleWidget {
  ChooChooMMEWidget(ChooChooMME* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/ChooChooMME.svg")));
    addChild(new LabelOverlay);
    ModelDisplay* modelDisplay = new ModelDisplay(module);
    modelDisplay->box.pos = mm2px(Vec(32.0f, 35.5f));
    addChild(modelDisplay);

    addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(40.64f, 22.0f)), module, ChooChooMME::PITCH_PARAM));

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.0f, 40.0f)), module, ChooChooMME::MODEL_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.0f, 62.0f)), module, ChooChooMME::WAVES_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.0f, 62.0f)), module, ChooChooMME::INTERVAL_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.0f, 84.0f)), module, ChooChooMME::AMOUNT_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.0f, 84.0f)), module, ChooChooMME::FLOW_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.0f, 106.0f)), module, ChooChooMME::FEEDBACK_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.0f, 106.0f)), module, ChooChooMME::SHAPER_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(33.5f, 62.0f)), module, ChooChooMME::WAVES_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(73.5f, 62.0f)), module, ChooChooMME::INTERVAL_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(33.5f, 84.0f)), module, ChooChooMME::AMOUNT_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(73.5f, 84.0f)), module, ChooChooMME::FLOW_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(33.5f, 106.0f)), module, ChooChooMME::FEEDBACK_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(73.5f, 106.0f)), module, ChooChooMME::SHAPER_ATTEN_PARAM));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.0f, 22.0f)), module, ChooChooMME::VOCT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71.0f, 22.0f)), module, ChooChooMME::TRIG_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.0f, 62.0f)), module, ChooChooMME::WAVES_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.0f, 62.0f)), module, ChooChooMME::INTERVAL_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.0f, 84.0f)), module, ChooChooMME::AMOUNT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.0f, 84.0f)), module, ChooChooMME::FLOW_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.0f, 106.0f)), module, ChooChooMME::FEEDBACK_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.0f, 106.0f)), module, ChooChooMME::SHAPER_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(40.64f, 120.0f)), module, ChooChooMME::VCO_OUTPUT));
  }
};

Model* modelChooChooMME = createModel<ChooChooMME, ChooChooMMEWidget>("ChooChooMME");
