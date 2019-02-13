//#include <otsdaq_demo/otsdaq-demo/FEInterfaces/FEWROtsUDPFSSRInterface.h>
//#include <otsdaq_demo/otsdaq-demo/UserConfigurationDataFormats/FEWROtsUDPFSSRInterfaceConfiguration.h>
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"
//#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"

//#include "otsdaq-demo/FEInterfaces/FEWOtsGenericInterface.h"
#include "otsdaq-core/FECore/FEVInterface.h"

#include <iostream>
#include <memory>
#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq-core/PluginMakers/MakeInterface.h"
#include "otsdaq-core/PluginMakers/MakeInterfaceConfiguration.h"

//#include "otsdaq-components/FEInterfaces/FEWOtsUDPFSSRInterface.h"
//#include "otsdaq-components/FEInterfaces/FEWOtsUDPHCALInterface.h"

using namespace ots;

int main() {
  // Variables
  std::string supervisorContextUID_ = "MainContext";
  std::string supervisorApplicationUID_ = "FeSupervisor0";
  std::string ConfigurationAlias_ = "Physics";
  std::string theSupervisorConfigurationPath_ =
      supervisorContextUID_ + "/LinkToApplicationTable/" + supervisorApplicationUID_ + "/LinkToSupervisorTable";
  // const int ConfigurationGroupKeyValue_ = 0;
  // std::shared_ptr<ConfigurationGroupKey> theConfigurationGroupKey_(new
  // ConfigurationGroupKey(ConfigurationGroupKeyValue_));

  ////////////////////////////////////////////////////////////////
  // INSERTED GLOBALLY IN THE CODE
  ConfigurationManager* theConfigurationManager_ = new ConfigurationManager;
  FEVInterfacesManager theFEVInterfacesManager_(
      theConfigurationManager_->getNode(ConfigurationManager::XDAQ_CONTEXT_CONFIG_NAME),
      theSupervisorConfigurationPath_);

  std::pair<std::string /*group name*/, ConfigurationGroupKey> theGroup =
      theConfigurationManager_->getConfigurationGroupFromAlias(ConfigurationAlias_);

  theConfigurationManager_->loadConfigurationGroup(theGroup.first, theGroup.second, true);

  theFEVInterfacesManager_.configure();
  ////////////////////////////////////////////////////////////////
  exit(0);
  ////////////////////////////////////////////////////////////////
  // Getting just the informations about the FEWInterface

  //	const std::string feId_ = "0";
  //	ConfigurationInterface* theInterface_;
  //	theInterface_ = ConfigurationInterface::getInstance(true);//FIXME This will be variable because if false it
  //takes it from the database 	Configurations* configurations = 0;
  //	theInterface_->get((ConfigurationBase*&)configurations,"Configurations");
  //	ConfigurationBase* frontEndConfiguration = 0;
  //	theInterface_->get(frontEndConfiguration, "FEConfiguration", theConfigurationGroupKey_, configurations);
  //
  //	const std::string interfaceName     =  "FEOtsUDPFSSRInterface";
  //	const std::string configurationName =  interfaceName + "Configuration";
  //
  //	//FEWOtsUDPHardwareConfiguration* interfaceConfiguration_ = 0;
  //
  //	ConfigurationBase* interfaceConfiguration_ = 0;//makeConfigurationInterface(configurationName);
  //
  //	if(configurations->findKOC(*theConfigurationGroupKey_,configurationName))
  //	{
  //		theInterface_->get(interfaceConfiguration_, configurationName, theConfigurationGroupKey_,
  //configurations);
  //	}
  //
  //	std::unique_ptr<FEVInterface> theFEWInterface  = makeInterface(interfaceName, feId_,
  //(FEInterfaceConfigurationBase*)interfaceConfiguration_);

  ////////////////////////////////////////////////////////////////

  return 0;
}
