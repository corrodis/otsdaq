#include "otsdaq-core/PluginMakers/MakeInterfaceConfiguration.h"
#include <cetlib/BasicPluginFactory.h>

#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
TableBase* makeInterfaceConfiguration(std::string const& configurationPluginName)
{
	static cet::BasicPluginFactory basicPluginInterfaceFactory("configuration", "make");

	return basicPluginInterfaceFactory.makePlugin<TableBase*>(configurationPluginName);
}
}  // namespace ots
