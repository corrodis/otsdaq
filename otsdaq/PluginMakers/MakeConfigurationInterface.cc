#include <cetlib/BasicPluginFactory.h>

#include "MakeConfigurationInterface.h"
#include "otsdaq/ConfigurationInterface/ConfigurationInterface.h"

namespace ots
{
ConfigurationInterface* makeConfigurationInterface(std::string const& interfacePluginName)
{
	static cet::BasicPluginFactory basicPluginInterfaceFactory("configInterface", "make");

	return basicPluginInterfaceFactory.makePlugin<ConfigurationInterface*>(interfacePluginName);
}
}  // namespace ots
