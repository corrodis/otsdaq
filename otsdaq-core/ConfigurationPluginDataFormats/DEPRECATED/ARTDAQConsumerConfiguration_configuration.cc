#include "otsdaq-core/ConfigurationPluginDataFormats/ARTDAQConsumerConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
ARTDAQConsumerConfiguration::ARTDAQConsumerConfiguration (void)
    : ConfigurationBase ("ARTDAQConsumerConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//FSSRDACsConfigurationInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//  <CONFIGURATION Name="ARTDAQConsumerConfiguration">
	//    <VIEW Name="ARTDAQ_CONSUMER_CONFIGURATION" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="ProcessorID"         StorageName="PROCESSOR_ID"                 DataType="VARCHAR2" />    //      <COLUMN Name="Status"              StorageName="STATUS"               DataType="VARCHAR2" />
	//      <COLUMN Name="ConfigurationString" StorageName="CONFIGURATION_STRING" DataType="VARCHAR2"/>
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>
}

//==============================================================================
ARTDAQConsumerConfiguration::~ARTDAQConsumerConfiguration (void)
{
}

//==============================================================================
void ARTDAQConsumerConfiguration::init (ConfigurationManager *configManager)
{
}

//==============================================================================
const std::string ARTDAQConsumerConfiguration::getConfigurationString (std::string processorUID) const
{
	std::string tmpConfiguration;
	ConfigurationBase::activeConfigurationView_->getValue (tmpConfiguration, ConfigurationBase::activeConfigurationView_->findRow (ProcessorID, processorUID), ConfigurationString);
	return tmpConfiguration;
}

DEFINE_OTS_CONFIGURATION (ARTDAQConsumerConfiguration)
