#include "otsdaq-core/ConfigurationPluginDataFormats/ARTDAQBuilderConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
ARTDAQBuilderConfiguration::ARTDAQBuilderConfiguration(void)
: ConfigurationBase("ARTDAQBuilderConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//FSSRDACsConfigurationInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//  <CONFIGURATION Name="ARTDAQBuilderConfiguration">
	//    <VIEW Name="ARTDAQ_BUILDER_CONFIGURATION" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="SupervisorInstance"  StorageName="SUPERVISOR_INSTANCE"  DataType="NUMBER"  />
	//      <COLUMN Name="BuilderID"           StorageName="BUILDER_ID"           DataType="VARCHAR2"/>
	//      <COLUMN Name="Status"              StorageName="STATUS"               DataType="VARCHAR2"/>
	//      <COLUMN Name="ConfigurationString" StorageName="CONFIGURATION_STRING" DataType="VARCHAR2"/>
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>

}

//==============================================================================
ARTDAQBuilderConfiguration::~ARTDAQBuilderConfiguration(void)
{}

//==============================================================================
void ARTDAQBuilderConfiguration::init(ConfigurationManager *configManager)
{
}

//==============================================================================
std::string ARTDAQBuilderConfiguration::getAggregatorID(unsigned int supervisorInstance) const
{
	std::string tmpID;
	ConfigurationBase::activeConfigurationView_->getValue(tmpID, ConfigurationBase::activeConfigurationView_->findRow(SupervisorInstance,supervisorInstance), BuilderID);
	return tmpID;
}

//==============================================================================
bool  ARTDAQBuilderConfiguration::getStatus(unsigned int supervisorInstance) const
{
	bool tmpStatus;
	ConfigurationBase::activeConfigurationView_->getValue(tmpStatus, ConfigurationBase::activeConfigurationView_->findRow(SupervisorInstance,supervisorInstance), Status);
	return tmpStatus;
}

//==============================================================================
const std::string ARTDAQBuilderConfiguration::getConfigurationString(unsigned int supervisorInstance) const
{
	std::string tmpConfiguration;
	ConfigurationBase::activeConfigurationView_->getValue(tmpConfiguration, ConfigurationBase::activeConfigurationView_->findRow(SupervisorInstance,supervisorInstance), ConfigurationString);
	return tmpConfiguration;
}

DEFINE_OTS_CONFIGURATION(ARTDAQBuilderConfiguration)
