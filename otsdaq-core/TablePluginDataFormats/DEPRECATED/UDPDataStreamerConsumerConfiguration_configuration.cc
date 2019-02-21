#include <iostream>
#include "otsdaq-coreMacros/TablePluginMacros.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/UDPDataStreamerConsumerTable.h"

using namespace ots;

//==============================================================================
UDPDataStreamerConsumerConfiguration::UDPDataStreamerConsumerConfiguration(void)
    : TableBase("UDPDataStreamerConsumerConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//	<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//	<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	// xsi:noNamespaceSchemaLocation="TableInfo.xsd"> 	  <CONFIGURATION
	// Name="UDPDataStreamerConsumerConfiguration"> 	    <VIEW
	// Name="UDP_DATA_LISTENER_PRODUCER_CONFIGURATION" Type="File,Database,DatabaseTest">
	//	      <COLUMN Name="ProcessorID"       StorageName="PROCESSOR_ID"
	// DataType="VARCHAR2"/> 	      <COLUMN Name="IPAddress"
	// StorageName="IP_ADDRESS"           DataType="VARCHAR2"/> 	      <COLUMN
	// Name="Port"  StorageName="PORT"                 DataType="NUMBER"  /> <COLUMN
	// Name="StreamToIPAddress" StorageName="STREAM_TO_IP_ADDRESS" DataType="VARCHAR2"/>
	//	      <COLUMN Name="StreamToPort"      StorageName="STREAM_TO_PORT"
	// DataType="NUMBER"  />
	//	    </VIEW>
	//	  </CONFIGURATION>
	//	</ROOT>
}

//==============================================================================
UDPDataStreamerConsumerConfiguration::~UDPDataStreamerConsumerConfiguration(void) {}

//==============================================================================
void UDPDataStreamerConsumerConfiguration::init(ConfigurationManager* configManager)
{
	std::string processorUID;
	for(unsigned int row = 0; row < TableBase::activeTableView_->getNumberOfRows(); row++)
	{
		TableBase::activeTableView_->getValue(processorUID, row, ProcessorID);
		processorIDToRowMap_[processorUID] = row;
	}
}

//==============================================================================
std::string UDPDataStreamerConsumerConfiguration::getIPAddress(
    std::string processorUID) const
{
	check(processorUID);
	std::string val;
	TableBase::activeTableView_->getValue(
	    val, processorIDToRowMap_.find(processorUID)->second, IPAddress);
	return val;
}

//==============================================================================
unsigned int UDPDataStreamerConsumerConfiguration::getPort(std::string processorUID) const
{
	check(processorUID);
	unsigned int val;
	TableBase::activeTableView_->getValue(
	    val, processorIDToRowMap_.find(processorUID)->second, Port);
	return val;
}

//==============================================================================
std::string UDPDataStreamerConsumerConfiguration::getStreamToIPAddress(
    std::string processorUID) const
{
	check(processorUID);
	std::string val;
	TableBase::activeTableView_->getValue(
	    val, processorIDToRowMap_.find(processorUID)->second, StreamToIPAddress);
	return val;
}

//==============================================================================
unsigned int UDPDataStreamerConsumerConfiguration::getStreamToPort(
    std::string processorUID) const
{
	check(processorUID);
	unsigned int val;
	TableBase::activeTableView_->getValue(
	    val, processorIDToRowMap_.find(processorUID)->second, StreamToPort);
	return val;
}

//==============================================================================
void UDPDataStreamerConsumerConfiguration::check(std::string processorUID) const
{
	if(processorIDToRowMap_.find(processorUID) == processorIDToRowMap_.end())
	{
		std::cout << __COUT_HDR_FL__ << "Couldn't find processor " << processorUID
		          << " in the UDPDataStreamerConsumerConfiguration!" << std::endl;
		assert(0);
	}
}

DEFINE_OTS_CONFIGURATION(UDPDataStreamerConsumerConfiguration)
