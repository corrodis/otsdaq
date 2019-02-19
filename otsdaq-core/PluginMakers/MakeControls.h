#ifndef _ots_MakeControls_h_
#define _ots_MakeControls_h_
// Using LibraryManager, find the correct library and return an instance of the specified
// Slow Controls Interface.

#include <string>

namespace ots
{
class ControlsVInterface;
class ConfigurationTree;

ControlsVInterface* makeControls(
    const std::string& controlsPluginName,
    const std::string&
        controlsUID  // Key value for (eventual) ControlsDashboard Table in Configuration
    ,
    const ConfigurationTree& configurationTree  // Pass the big tree
    ,
    const std::string& pathToControlsConfiguration);  // Path to ControlsDashboard Table

}  // namespace ots

#endif /* _ots_MakeControls_h_ */
