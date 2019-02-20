#include "otsdaq-core/ConfigurationPluginDataFormats/RawDataSaverConsumerTable.h"
#include <iostream>
#include "../../Macros/TablePluginMacros.h"

using namespace ots;

//==============================================================================
RawDataSaverConsumerConfiguration::RawDataSaverConsumerConfiguration(void)
    : TableBase("RawDataSaverConsumerConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//	<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//	<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	// xsi:noNamespaceSchemaLocation="TableInfo.xsd"> 	  <CONFIGURATION
	// Name="RawDataSaverConsumerConfiguration"> 	    <VIEW
	// Name="RAW_DATA_SAVER_CONSUMER_CONFIGURATION" Type="File,Database,DatabaseTest">
	//	      <COLUMN Name="ProcessorID"   StorageName="PROCESSOR_ID"
	// DataType="VARCHAR2"/> 	      <COLUMN Name="FilePath"      StorageName="FILE_PATH"
	// DataType="VARCHAR2"/> 	      <COLUMN Name="RadixFileName"
	// StorageName="RADIX_FILE_NAME" DataType="VARCHAR2"/>
	//	    </VIEW>
	//	  </CONFIGURATION>
	//	</ROOT>
}

//==============================================================================
RawDataSaverConsumerConfiguration::~RawDataSaverConsumerConfiguration(void) {}

//==============================================================================
void RawDataSaverConsumerConfiguration::init(ConfigurationManager* configManager)
{
	std::string processorUID;
	for(unsigned int row = 0; row < TableBase::activeTableView_->getNumberOfRows(); row++)
	{
		TableBase::activeTableView_->getValue(processorUID, row, ProcessorID);
		processorIDToRowMap_[processorUID] = row;
	}
}

//==============================================================================
std::vector<std::string> RawDataSaverConsumerConfiguration::getProcessorIDList(void) const
{
	std::vector<std::string> list;
	for(auto& it : processorIDToRowMap_)
		list.push_back(it.first);
	return list;
}

//==============================================================================
std::string RawDataSaverConsumerConfiguration::getFilePath(std::string processorUID) const
{
	check(processorUID);
	std::string val;
	TableBase::activeTableView_->getValue(
	    val, processorIDToRowMap_.find(processorUID)->second, FilePath);
	return val;
}

//==============================================================================
std::string RawDataSaverConsumerConfiguration::getRadixFileName(
    std::string processorUID) const
{
	check(processorUID);
	std::string val;
	TableBase::activeTableView_->getValue(
	    val, processorIDToRowMap_.find(processorUID)->second, RadixFileName);
	return val;
}

//==============================================================================
void RawDataSaverConsumerConfiguration::check(std::string processorUID) const
{
	if(processorIDToRowMap_.find(processorUID) == processorIDToRowMap_.end())
	{
		std::cout << __COUT_HDR_FL__ << "Couldn't find processor " << processorUID
		          << " in the UDPDataStreamerConsumerConfiguration!" << std::endl;
		assert(0);
	}
}

DEFINE_OTS_CONFIGURATION(RawDataSaverConsumerConfiguration)
