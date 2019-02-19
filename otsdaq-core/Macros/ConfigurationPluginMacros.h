#ifndef _ots_ConfigurationPluginMacro_h_
#define _ots_ConfigurationPluginMacro_h_

#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
typedef TableBase*(cbmakeFunc_t)();
}

#define DEFINE_OTS_CONFIGURATION(klass) \
	extern "C" TableBase* make() { return new klass(); }
#endif /* _ots_ConfigurationPluginMacro_h_ */
