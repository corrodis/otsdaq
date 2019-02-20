#include <iostream>
#include "../../Macros/TablePluginMacros.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/ARTDAQAggregatorTable.h"

using namespace ots;

//==============================================================================
ARTDAQAggregatorConfiguration::ARTDAQAggregatorConfiguration(void)
    : TableBase("ARTDAQAggregatorConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	// FSSRDACsConfigurationInfo.xml
	//<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	// xsi:noNamespaceSchemaLocation="TableInfo.xsd">
	//  <CONFIGURATION Name="ARTDAQAggregatorConfiguration">
	//    <VIEW Name="ARTDAQ_AGGREGATOR_CONFIGURATION" Type="File,Database,DatabaseTest">
	//      <COLUMN Name="SupervisorInstance"  StorageName="SUPERVISOR_INSTANCE"
	//      DataType="NUMBER"  /> <COLUMN Name="AggregatorID"
	//      StorageName="AGGREGATOR_ID"        DataType="VARCHAR2"/> <COLUMN Name="Status"
	//      StorageName="STATUS"               DataType="VARCHAR2"/> <COLUMN
	//      Name="ConfigurationString" StorageName="CONFIGURATION_STRING"
	//      DataType="VARCHAR2"/>
	//    </VIEW>
	//  </CONFIGURATION>
	//</ROOT>
}

//==============================================================================
ARTDAQAggregatorConfiguration::~ARTDAQAggregatorConfiguration(void) {}

//==============================================================================
void ARTDAQAggregatorConfiguration::init(ConfigurationManager* configManager) {}

//==============================================================================
std::string ARTDAQAggregatorConfiguration::getAggregatorID(
    unsigned int supervisorInstance) const
{
	std::string tmpID;
	TableBase::activeTableView_->getValue(
	    tmpID,
	    TableBase::activeTableView_->findRow(SupervisorInstance, supervisorInstance),
	    AggregatorID);
	return tmpID;
}

//==============================================================================
bool ARTDAQAggregatorConfiguration::getStatus(unsigned int supervisorInstance) const
{
	bool tmpStatus;
	TableBase::activeTableView_->getValue(
	    tmpStatus,
	    TableBase::activeTableView_->findRow(SupervisorInstance, supervisorInstance),
	    Status);
	return tmpStatus;
}

//==============================================================================
const std::string ARTDAQAggregatorConfiguration::getConfigurationString(
    unsigned int supervisorInstance) const
{
	std::string tmpConfiguration;
	TableBase::activeTableView_->getValue(
	    tmpConfiguration,
	    TableBase::activeTableView_->findRow(SupervisorInstance, supervisorInstance),
	    ConfigurationString);
	return tmpConfiguration;
}

DEFINE_OTS_CONFIGURATION(ARTDAQAggregatorConfiguration)
