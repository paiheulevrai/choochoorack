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
  std::shared_ptr<Font> font;

  ModelDisplay(ChooChooMME* module) : module(module) {
    box.size = mm2px(Vec(36.0f, 8.0f));
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/FragmentMono-Regular.ttf"));
  }

  void draw(const DrawArgs& args) override {
    static const char* const names[] = {"RING", "FOLD", "CROSS", "VPM", "SYNC", "LOGIC", "VOCODE"};
    int model = module ? static_cast<int>(std::round(limit(module->params[ChooChooMME::MODEL_PARAM].getValue(), 0.0f, 6.0f))) : 0;

    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0.0f, 0.0f, box.size.x, box.size.y);
    nvgFillColor(args.vg, nvgRGB(0x03, 0x05, 0x05));
    nvgFill(args.vg);
    nvgStrokeWidth(args.vg, 1.0f);
    nvgStrokeColor(args.vg, nvgRGB(0xe8, 0x1d, 0x32));
    nvgStroke(args.vg);
    for (float y = 3.0f; y < box.size.y; y += 4.0f) {
      nvgBeginPath(args.vg); nvgMoveTo(args.vg, 1.0f, y); nvgLineTo(args.vg, box.size.x - 1.0f, y);
      nvgStrokeWidth(args.vg, .5f); nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xd5, 0x24, 32)); nvgStroke(args.vg);
    }

    if (font) {
      nvgFontFaceId(args.vg, font->handle);
      nvgFontSize(args.vg, 15.0f);
      nvgTextLetterSpacing(args.vg, 2.0f);
      nvgFillColor(args.vg, nvgRGB(0xff, 0xd5, 0x24));
      nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
      nvgText(args.vg, box.size.x * 0.5f, box.size.y * 0.55f, names[model], nullptr);
      nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
      nvgText(args.vg, 7.0f, box.size.y * 0.55f, ">", nullptr);
    }
    Widget::draw(args);
  }
};

struct LabelOverlay : TransparentWidget {
  std::shared_ptr<Font> font;

  LabelOverlay() {
    box.size = mm2px(Vec(81.28f, 128.5f));
    font = APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf"));
  }

  void draw(const DrawArgs& args) override {
    if (!font)
      return;
    nvgFontFaceId(args.vg, font->handle);
    nvgTextLetterSpacing(args.vg, 0.35f);
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

    drawText(args, 4.0f, 6.0f, 16.0f, "MME", nvgRGB(0xf2, 0xf2, 0xed));
    drawText(args, 27.0f, 6.0f, 8.0f, "MULTI MODULATION ENGINE", nvgRGB(0xf0, 0xc9, 0x29));
    drawCenteredText(args, 9.0f, 14.5f, 9.0f, "1V/OCT", nvgRGB(0xf2, 0xf2, 0xed));
    drawCenteredText(args, 30.0f, 14.5f, 9.0f, "PITCH", nvgRGB(0xf2, 0xf2, 0xed));
    drawCenteredText(args, 52.0f, 14.5f, 8.0f, "RESET", nvgRGB(0xf2, 0xf2, 0xed));
    drawCenteredText(args, 73.0f, 14.5f, 8.0f, "OUT", nvgRGB(0xff, 0xdf, 0x42));
    drawText(args, 5.0f, 34.0f, 10.0f, "MODEL", nvgRGB(0xff, 0xdf, 0x42));

    drawControlLabel(args, 20.5f, 55.0f, "WAVES");
    drawControlLabel(args, 60.5f, 55.0f, "INTERVAL");
    drawControlLabel(args, 20.5f, 77.0f, "AMOUNT");
    drawControlLabel(args, 60.5f, 77.0f, "FLOW");
    drawControlLabel(args, 20.5f, 99.0f, "FEEDBACK");
    drawControlLabel(args, 60.5f, 99.0f, "SHAPER");
    drawText(args, 4.0f, 125.0f, 9.0f, "ChooChoo Labs", nvgRGB(0xf2, 0xf2, 0xed));
    drawRightText(args, 78.0f, 125.0f, 7.0f, "Pierre-Emmanuel Surga", nvgRGB(0xd0, 0xd4, 0xd8));
    Widget::draw(args);
  }

 private:
  void drawText(const DrawArgs& args, float x, float y, float size, const char* text, NVGcolor color) {
    nvgFontSize(args.vg, size);
    nvgFillColor(args.vg, color);
    Vec pos = mm2px(Vec(x, y));
    nvgText(args.vg, pos.x, pos.y, text, nullptr);
  }

  void drawControlLabel(const DrawArgs& args, float center, float y, const char* text) {
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
    drawText(args, center, y, 11.0f, text, nvgRGB(0xf8, 0xf8, 0xf3));
    drawText(args, center, y + 4.0f, 7.5f, "-  /  +", nvgRGB(0xd0, 0xd4, 0xd8));
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  }
  void drawCenteredText(const DrawArgs& args, float x, float y, float size, const char* text, NVGcolor color) {
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE); drawText(args, x, y, size, text, color); nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  }
  void drawRightText(const DrawArgs& args, float x, float y, float size, const char* text, NVGcolor color) {
    nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE); drawText(args, x, y, size, text, color); nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
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

    addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(30.0f, 22.0f)), module, ChooChooMME::PITCH_PARAM));

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.0f, 42.0f)), module, ChooChooMME::MODEL_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.0f, 62.0f)), module, ChooChooMME::WAVES_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.0f, 62.0f)), module, ChooChooMME::INTERVAL_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.0f, 84.0f)), module, ChooChooMME::AMOUNT_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.0f, 84.0f)), module, ChooChooMME::FLOW_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.0f, 106.0f)), module, ChooChooMME::FEEDBACK_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.0f, 106.0f)), module, ChooChooMME::SHAPER_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(20.5f, 62.0f)), module, ChooChooMME::WAVES_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(60.5f, 62.0f)), module, ChooChooMME::INTERVAL_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(20.5f, 84.0f)), module, ChooChooMME::AMOUNT_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(60.5f, 84.0f)), module, ChooChooMME::FLOW_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(20.5f, 106.0f)), module, ChooChooMME::FEEDBACK_ATTEN_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(60.5f, 106.0f)), module, ChooChooMME::SHAPER_ATTEN_PARAM));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.0f, 22.0f)), module, ChooChooMME::VOCT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(52.0f, 22.0f)), module, ChooChooMME::TRIG_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.0f, 62.0f)), module, ChooChooMME::WAVES_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71.0f, 62.0f)), module, ChooChooMME::INTERVAL_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.0f, 84.0f)), module, ChooChooMME::AMOUNT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71.0f, 84.0f)), module, ChooChooMME::FLOW_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.0f, 106.0f)), module, ChooChooMME::FEEDBACK_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71.0f, 106.0f)), module, ChooChooMME::SHAPER_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(73.0f, 22.0f)), module, ChooChooMME::VCO_OUTPUT));
  }
};

Model* modelChooChooMME = createModel<ChooChooMME, ChooChooMMEWidget>("ChooChooMME");
