#ifndef _ots_MakeInterfaceConfiguration_h_
#define _ots_MakeInterfaceConfiguration_h_
// Using LibraryManager, find the correct library and return an instance of the specified
// configuration interface.

#include <string>

namespace ots
{
class TableBase;

TableBase* makeInterfaceConfiguration(std::string const& configurationPluginName);

}  // namespace ots

#endif /* _ots_MakeInterfaceConfiguration_h_ */
