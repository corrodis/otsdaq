#include "otsdaq-core/ConfigurationPluginDataFormats/MaskConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
MaskConfiguration::MaskConfiguration(void) : ConfigurationBase("MaskConfiguration") {
  //////////////////////////////////////////////////////////////////////
  // WARNING: the names and the order MUST match the ones in the enum  //
  //////////////////////////////////////////////////////////////////////
  // MaskConfigurationInfo.xml
  //<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
  //<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
  //  <CONFIGURATION Name="MaskConfiguration">
  //    <VIEW Name="MASK_CONFIGURATION" Type="File,Database,DatabaseTest">
  //      <COLUMN Name="DetectorID"  StorageName="DETECTOR_ID"  DataType="VARCHAR2" />
  //      <COLUMN Name="KillMask" StorageName="KILL_MASK" DataType="VARCHAR2" />
  //    </VIEW>
  //  </CONFIGURATION>
  //</ROOT>
}

//==============================================================================
MaskConfiguration::~MaskConfiguration(void) {}

//==============================================================================
void MaskConfiguration::init(ConfigurationManager* configManager) {
  std::string tmpDetectorID;
  for (unsigned int row = 0; row < ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++) {
    ConfigurationBase::activeConfigurationView_->getValue(tmpDetectorID, row, DetectorID);
    nameToRow_[tmpDetectorID] = row;
  }
}

//==============================================================================
const std::string& MaskConfiguration::getROCMask(std::string rocName) const {
  // FIXME This check should be removed when you are sure you don't have inconsistencies between configurations
  if (nameToRow_.find(rocName) == nameToRow_.end()) {
    std::cout << __COUT_HDR_FL__ << "ROC named " << rocName << " doesn't exist in the mask configuration." << std::endl;
    assert(0);
  }
  return ConfigurationBase::getView().getDataView()[nameToRow_.find(rocName)->second][KillMask];
}

DEFINE_OTS_CONFIGURATION(MaskConfiguration)
