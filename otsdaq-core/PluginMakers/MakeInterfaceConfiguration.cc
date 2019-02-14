#include "otsdaq-core/PluginMakers/MakeInterfaceConfiguration.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <cetlib/BasicPluginFactory.h>

namespace ots
{
ConfigurationBase* makeInterfaceConfiguration(std::string const& configurationPluginName)
{
	static cet::BasicPluginFactory basicPluginInterfaceFactory("configuration", "make");

	return basicPluginInterfaceFactory.makePlugin<ConfigurationBase*>(configurationPluginName);
}
}  // namespace ots
