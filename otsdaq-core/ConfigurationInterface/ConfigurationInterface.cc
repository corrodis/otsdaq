#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq-core/ConfigurationInterface/DatabaseConfigurationInterface.h"
#include "otsdaq-core/ConfigurationInterface/FileConfigurationInterface.h"

#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include <dirent.h>
#include <cassert>
#include <iostream>
#include <typeinfo>

using namespace ots;

#define DEBUG_CONFIGURATION true

//==============================================================================
ConfigurationInterface* ConfigurationInterface::theInstance_ = 0;
bool ConfigurationInterface::theMode_ = true;
bool ConfigurationInterface::theVersionTrackingEnabled_ = true;

const std::string ConfigurationInterface::GROUP_METADATA_TABLE_NAME = "ConfigurationGroupMetadata";

//==============================================================================
ConfigurationInterface::ConfigurationInterface() {}

//==============================================================================
ConfigurationInterface* ConfigurationInterface::getInstance(bool mode) {
  if (mode == true) {
    if (theInstance_ != 0 && dynamic_cast<FileConfigurationInterface*>(theInstance_) == 0) {
      delete theInstance_;
      theInstance_ = 0;
    }
    if (theInstance_ == 0)  // && typeid(theInstance_) != static_cast<DatabaseConfigurationInterface*> )
      theInstance_ = new FileConfigurationInterface();
  } else {
    if (theInstance_ != 0 && dynamic_cast<DatabaseConfigurationInterface*>(theInstance_) == 0) {
      delete theInstance_;
      theInstance_ = 0;
    }
    if (theInstance_ == 0)  // && typeid(theInstance_) != static_cast<DatabaseConfigurationInterface*> )
    {
      theInstance_ = new DatabaseConfigurationInterface();
    }
  }
  theMode_ = mode;
  return theInstance_;
}

//==============================================================================
bool ConfigurationInterface::isVersionTrackingEnabled() { return ConfigurationInterface::theVersionTrackingEnabled_; }

//==============================================================================
void ConfigurationInterface::setVersionTrackingEnabled(bool setValue) {
  ConfigurationInterface::theVersionTrackingEnabled_ = setValue;
}

//==============================================================================
// saveNewVersion
// 	If newVersion is 0, then save the temporaryVersion as the next positive version number,
//		save using the interface, and return the new version number
//	If newVersion is non 0, attempt to save as given newVersion number, else throw exception.
//	return ConfigurationVersion::INVALID on failure
ConfigurationVersion ConfigurationInterface::saveNewVersion(ConfigurationBase* configuration,
                                                            ConfigurationVersion temporaryVersion,
                                                            ConfigurationVersion newVersion) {
  if (!temporaryVersion.isTemporaryVersion() || !configuration->isStored(temporaryVersion)) {
    std::cout << __COUT_HDR_FL__ << "Invalid temporary version number: " << temporaryVersion << std::endl;
    return ConfigurationVersion();  // return INVALID
  }

  if (!ConfigurationInterface::isVersionTrackingEnabled())  // tracking is OFF, so always save to same version
    newVersion = ConfigurationVersion::SCRATCH;

  bool rewriteableExists = false;

  std::set<ConfigurationVersion> versions = getVersions(configuration);
  if (newVersion == ConfigurationVersion::INVALID) {
    if (versions.size() &&  // 1 more than last version, if any non-scratch versions exist
        *(versions.rbegin()) != ConfigurationVersion(ConfigurationVersion::SCRATCH))
      newVersion = ConfigurationVersion::getNextVersion(*(versions.rbegin()));
    else if (versions.size() > 1)  // if scratch exists, take 1 more than second to last version
      newVersion = ConfigurationVersion::getNextVersion(*(--(versions.rbegin())));
    else
      newVersion = ConfigurationVersion::DEFAULT;
    std::cout << __COUT_HDR_FL__ << "Next available version number is " << newVersion << std::endl;
    //
    //		//for sanity check, compare with config's idea of next version
    //		ConfigurationVersion baseNextVersion = configuration->getNextVersion();
    //		if(newVersion <= baseNextVersion)
    //			newVersion = ConfigurationVersion::getNextVersion(baseNextVersion);
    //
    //		std::cout << __COUT_HDR_FL__ << "After considering baseNextVersion, " << baseNextVersion <<
    //				", next available version number is " << newVersion << std::endl;
  } else if (versions.find(newVersion) != versions.end()) {
    std::cout << __COUT_HDR_FL__ << "newVersion(" << newVersion << ") already exists!" << std::endl;
    rewriteableExists = newVersion == ConfigurationVersion::SCRATCH;

    // throw error if version already exists and this is not the rewriteable version
    if (!rewriteableExists || ConfigurationInterface::isVersionTrackingEnabled()) {
      __SS__ << ("New version already exists!") << std::endl;
      std::cout << __COUT_HDR_FL__ << ss.str();
      __SS_THROW__;
    }
  }

  std::cout << __COUT_HDR_FL__ << "Version number to save is " << newVersion << std::endl;

  // copy to new version
  configuration->changeVersionAndActivateView(temporaryVersion, newVersion);

  // save to disk
  //	only allow overwrite if version tracking is disabled AND the rewriteable version
  //		already exists.
  saveActiveVersion(configuration, !ConfigurationInterface::isVersionTrackingEnabled() && rewriteableExists);

  return newVersion;
}
