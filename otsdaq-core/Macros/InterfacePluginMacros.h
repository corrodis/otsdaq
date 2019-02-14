#ifndef _ots_InterfacePluginMacro_h_
#define _ots_InterfacePluginMacro_h_

#include <memory>
#include "otsdaq-core/FECore/FEVInterface.h"

#define DEFINE_OTS_INTERFACE(klass)                                                                                                         \
	extern "C" std::unique_ptr<ots::FEVInterface>                                                                                           \
	make(const std::string& interfaceUID, const ots::ConfigurationTree& configurationTree, const std::string& pathToInterfaceConfiguration) \
	{                                                                                                                                       \
		return std::unique_ptr<ots::FEVInterface>(new klass(interfaceUID, configurationTree, pathToInterfaceConfiguration));                \
	}

#endif /* _ots_InterfacePluginMacro_h_ */
