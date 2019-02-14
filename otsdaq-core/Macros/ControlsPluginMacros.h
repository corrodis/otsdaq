#ifndef _ots_ControlsPluginMacro_h_
#define _ots_ControlsPluginMacro_h_

#include "otsdaq-core/ControlsCore/ControlsVInterface.h"
#include <memory>

#define DEFINE_OTS_CONTROLS(klass) \
  extern "C" \
  ots::ControlsVInterface* \
  make(const std::string& controlsUID, const ConfigurationTree& configurationTree, const std::string& pathToControlsConfiguration) \
  {\
    return new klass(controlsUID, configurationTree, pathToControlsConfiguration); \
  }

#endif /* _ots_ControlsPluginMacro_h_ */


















