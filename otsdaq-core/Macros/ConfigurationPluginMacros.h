#ifndef _ots_ConfigurationPluginMacro_h_
#define _ots_ConfigurationPluginMacro_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

namespace ots
{
  typedef ConfigurationBase*(cbmakeFunc_t) ();
}

#define DEFINE_OTS_CONFIGURATION(klass) \
  extern "C" \
  ConfigurationBase* \
  make() \
  {\
    return new klass(); \
  }
#endif /* _ots_ConfigurationPluginMacro_h_ */
