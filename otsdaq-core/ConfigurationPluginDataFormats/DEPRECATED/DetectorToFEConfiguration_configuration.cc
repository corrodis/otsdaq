#include "otsdaq-core/ConfigurationPluginDataFormats/DetectorToFEConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
DetectorToFEConfiguration::DetectorToFEConfiguration(void)
    : ConfigurationBase("DetectorToFEConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//DetectorToFEConfigurationInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//  <CONFIGURATION Name="DetectorToFEConfiguration">
	//    <VIEW Name="DETECTOR_TO_FE_CONFIGURATION" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="DetectorID"              StorageName="DETECTOR_ID"                DataType="VARCHAR2"/>
	//      <COLUMN Name="FEWriterID"              StorageName="FE_WRITER_ID"               DataType="VARCHAR2"/>
	//      <COLUMN Name="FEWriterChannel"         StorageName="FE_WRITER_CHANNEL"          DataType="NUMBER"  />
	//      <COLUMN Name="FEWriterDetectorAddress" StorageName="FE_WRITER_DETECTOR_ADDRESS" DataType="NUMBER"  />
	//      <COLUMN Name="FEReaderID"              StorageName="FE_READER_ID"               DataType="VARCHAR2"/>
	//      <COLUMN Name="FEReaderChannel"         StorageName="FE_READER_CHANNEL"          DataType="NUMBER"  />
	//      <COLUMN Name="FEReaderDetectorAddress" StorageName="FE_READER_DETECTOR_ADDRESS" DataType="NUMBER"  />
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>
}

//==============================================================================
DetectorToFEConfiguration::~DetectorToFEConfiguration(void)
{
}

//==============================================================================
void DetectorToFEConfiguration::init(ConfigurationManager* configManager)
{
	std::string tmpDetectorName;
	for (unsigned int row = 0; row < ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
	{
		ConfigurationBase::activeConfigurationView_->getValue(tmpDetectorName, row, DetectorID);
		nameToInfoMap_[tmpDetectorName] = DetectorInfo();
		DetectorInfo& aDetectorInfo     = nameToInfoMap_[tmpDetectorName];
		ConfigurationBase::activeConfigurationView_->getValue(aDetectorInfo.theFEWriterID_, row, FEWriterID);
		ConfigurationBase::activeConfigurationView_->getValue(aDetectorInfo.theFEWriterChannel_, row, FEWriterChannel);
		ConfigurationBase::activeConfigurationView_->getValue(aDetectorInfo.theFEWriterDetectorAddress_, row, FEWriterDetectorAddress);
		ConfigurationBase::activeConfigurationView_->getValue(aDetectorInfo.theFEReaderID_, row, FEReaderID);
		ConfigurationBase::activeConfigurationView_->getValue(aDetectorInfo.theFEReaderChannel_, row, FEReaderChannel);
		ConfigurationBase::activeConfigurationView_->getValue(aDetectorInfo.theFEReaderDetectorAddress_, row, FEReaderDetectorAddress);
	}
}

//==============================================================================
std::vector<std::string> DetectorToFEConfiguration::getFEWriterDetectorList(std::string interfaceID) const
{
	std::string		 tmpDetectorID;
	std::string		 tmpFEWriterID;
	std::vector<std::string> list;
	for (unsigned int row = 0; row < ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
	{
		ConfigurationBase::activeConfigurationView_->getValue(tmpFEWriterID, row, FEWriterID);
		if (tmpFEWriterID == interfaceID)
		{
			ConfigurationBase::activeConfigurationView_->getValue(tmpDetectorID, row, DetectorID);
			list.push_back(tmpDetectorID);
		}
	}
	return list;
}

//==============================================================================
std::vector<std::string> DetectorToFEConfiguration::getFEReaderDetectorList(std::string interfaceID) const
{
	std::string		 tmpDetectorID;
	std::string		 tmpFEReaderID;
	std::vector<std::string> list;
	for (unsigned int row = 0; row < ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
	{
		ConfigurationBase::activeConfigurationView_->getValue(tmpFEReaderID, row, FEReaderID);
		if (tmpFEReaderID == interfaceID)
		{
			ConfigurationBase::activeConfigurationView_->getValue(tmpDetectorID, row, DetectorID);
			list.push_back(tmpDetectorID);
		}
	}
	return list;
}

//==============================================================================
unsigned int DetectorToFEConfiguration::getFEWriterChannel(const std::string& detectorID) const
{
	return nameToInfoMap_.find(detectorID)->second.theFEWriterChannel_;
}

//==============================================================================
unsigned int DetectorToFEConfiguration::getFEWriterDetectorAddress(const std::string& detectorID) const
{
	return nameToInfoMap_.find(detectorID)->second.theFEWriterDetectorAddress_;
}

//==============================================================================
unsigned int DetectorToFEConfiguration::getFEReaderChannel(const std::string& detectorID) const
{
	return nameToInfoMap_.find(detectorID)->second.theFEReaderChannel_;
}

DEFINE_OTS_CONFIGURATION(DetectorToFEConfiguration)
