#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* plugin) {
  pluginInstance = plugin;
  plugin->addModel(modelSintered);
}
