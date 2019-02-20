#include <cetlib/BasicPluginFactory.h>

#include "MakeTable.h"
#include "otsdaq-core/TableCore/TableBase.h"

namespace ots
{
TableBase* makeTable(std::string const& tablePluginName)
{
	static cet::BasicPluginFactory basicPluginInterfaceFactory("table", "make");

	return basicPluginInterfaceFactory.makePlugin<TableBase*>(tablePluginName);
}
}  // namespace ots
