#include "otsdaq-core/PluginMakers/MakeControls.h"
#include "otsdaq-core/ControlsCore/ControlsVInterface.h"

#include <cetlib/BasicPluginFactory.h>

ots::ControlsVInterface* ots::makeControls(
    const std::string& controlsPluginName,
    const std::string&
                                  controlsUID,  // Key value for (eventual) ControlsDashboard Table in Configuration
    const ots::ConfigurationTree& configurationTree,  // Pass the big tree
    const std::string& pathToControlsConfiguration)   // Path to ControlsDashboard Table
{
	static cet::BasicPluginFactory basicPluginInterfaceFactory("controls", "make");

	return basicPluginInterfaceFactory.makePlugin<ots::ControlsVInterface*,
	                                              const std::string&,
	                                              const ots::ConfigurationTree&,
	                                              const std::string&>(
	    controlsPluginName, controlsUID, configurationTree, pathToControlsConfiguration);
}
