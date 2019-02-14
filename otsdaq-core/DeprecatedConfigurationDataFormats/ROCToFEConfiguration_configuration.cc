#include "otsdaq-core/ConfigurationPluginDataFormats/ROCToFEConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

const std::string ROCToFEConfiguration::staticConfigurationName_ = "ROCToFEConfiguration";
//==============================================================================
ROCToFEConfiguration::ROCToFEConfiguration(void) :
        ConfigurationBase(ROCToFEConfiguration::staticConfigurationName_)
{
    //////////////////////////////////////////////////////////////////////
    //WARNING: the names and the order MUST match the ones in the enum  //
    //////////////////////////////////////////////////////////////////////
    //ROCToFEConfigurationInfo.xml
    //<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
    //<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
    //  <CONFIGURATION Name="ROCToFEConfiguration">
    //    <VIEW Name="ROC_TO_FE_CONFIGURATION" Type="File,Database,DatabaseTest">
    //      <COLUMN Name="DetectorID"       StorageName="DETECTOR_ID"        DataType="VARCHAR2" />
    //      <COLUMN Name="FEWName"       StorageName="FEW_NAME"        DataType="NUMBER"   />
    //      <COLUMN Name="FEWChannel"    StorageName="FEW_CHANNEL"     DataType="NUMBER"   />
    //      <COLUMN Name="FEWROCAddress" StorageName="FEW_ROC_ADDRESS" DataType="NUMBER"   />
    //      <COLUMN Name="FERName"       StorageName="FER_NAME"        DataType="NUMBER"   />
    //      <COLUMN Name="FERChannel"    StorageName="FER_CHANNEL"     DataType="NUMBER"   />
    //      <COLUMN Name="FERROCAddress" StorageName="FER_ROC_ADDRESS" DataType="NUMBER"   />
    //    </VIEW>
    //  </CONFIGURATION>
    //</ROOT>

}

//==============================================================================
ROCToFEConfiguration::~ROCToFEConfiguration(void)
{}

//==============================================================================
void ROCToFEConfiguration::init(ConfigurationManager *configManager)
{
    std::string       tmpDetectorID;
    for(unsigned int row=0; row<ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
    {
        ConfigurationBase::activeConfigurationView_->getValue(tmpDetectorID                ,row,DetectorID);
        nameToInfoMap_[tmpDetectorID] = ROCInfo();
        ROCInfo& aROCInfo = nameToInfoMap_[tmpDetectorID];
        ConfigurationBase::activeConfigurationView_->getValue(aROCInfo.theFEWName_      ,row,FEWName);
        ConfigurationBase::activeConfigurationView_->getValue(aROCInfo.theFEWChannel_   ,row,FEWChannel);
        ConfigurationBase::activeConfigurationView_->getValue(aROCInfo.theFEWROCAddress_,row,FEWROCAddress);
        ConfigurationBase::activeConfigurationView_->getValue(aROCInfo.theFERName_      ,row,FERName);
        ConfigurationBase::activeConfigurationView_->getValue(aROCInfo.theFERChannel_   ,row,FEWChannel);
        ConfigurationBase::activeConfigurationView_->getValue(aROCInfo.theFERROCAddress_,row,FERROCAddress);
    }
}

//==============================================================================
std::vector<std::string> ROCToFEConfiguration::getFEWROCsList(std::string fECNumber) const
{
    std::string       tmpDetectorID;
    std::string       tmpFEWName;
    std::vector<std::string> list;
    for(unsigned int row=0; row<ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
    {
        ConfigurationBase::activeConfigurationView_->getValue(tmpFEWName,row,FEWName);
        if(tmpFEWName == fECNumber)
        {
            ConfigurationBase::activeConfigurationView_->getValue(tmpDetectorID,row,DetectorID);
            list.push_back(tmpDetectorID);
        }
    }
    return list;
}

//==============================================================================
std::vector<std::string> ROCToFEConfiguration::getFERROCsList(std::string fEDNumber) const
{
    std::string       tmpDetectorID;
    std::string       tmpFERName;
    std::vector<std::string> list;
    for(unsigned int row=0; row<ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
    {
        ConfigurationBase::activeConfigurationView_->getValue(tmpFERName,row,FERName);
        if(tmpFERName == fEDNumber)
        {
            ConfigurationBase::activeConfigurationView_->getValue(tmpDetectorID,row,DetectorID);
            list.push_back(tmpDetectorID);
        }
    }
    return list;
}

//==============================================================================
unsigned int ROCToFEConfiguration::getFEWChannel(const std::string& rOCName) const
{
    return nameToInfoMap_.find(rOCName)->second.theFEWChannel_;
}

//==============================================================================
unsigned int ROCToFEConfiguration::getFEWROCAddress(const std::string& rOCName) const
{
    return nameToInfoMap_.find(rOCName)->second.theFEWROCAddress_;
}

//==============================================================================
unsigned int ROCToFEConfiguration::getFERChannel(const std::string& rOCName) const
{
    return nameToInfoMap_.find(rOCName)->second.theFERChannel_;
}

DEFINE_OTS_CONFIGURATION(ROCToFEConfiguration)
