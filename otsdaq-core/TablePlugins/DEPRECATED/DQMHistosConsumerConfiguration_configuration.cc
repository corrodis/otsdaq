#include <iostream>
#include "otsdaq-core/ConfigurationPluginDataFormats/DQMHistosConsumerTable.h"
#include "otsdaq-coreMacros/TablePluginMacros.h"

using namespace ots;

//==============================================================================
DQMHistosConsumerConfiguration::DQMHistosConsumerConfiguration(void)
    : TableBase("DQMHistosConsumerConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//	<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//	<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	// xsi:noNamespaceSchemaLocation="TableInfo.xsd"> 	  <CONFIGURATION
	// Name="DQMHistosConsumerConfiguration"> 	    <VIEW
	// Name="DQM_HISTOS_CONSUMER_CONFIGURATION" Type="File,Database,DatabaseTest">
	//	      <COLUMN Name="ProcessorID"   StorageName="PROCESSOR_ID"
	// DataType="VARCHAR2"/> 	      <COLUMN Name="FilePath"      StorageName="FILE_PATH"
	// DataType="VARCHAR2"/> 	      <COLUMN Name="RadixFileName"
	// StorageName="RADIX_FILE_NAME" DataType="VARCHAR2"/>
	//        <COLUMN Name="SaveFile"      StorageName="SAVE_FILE" DataType="VARCHAR2"/>
	//	    </VIEW>
	//	  </CONFIGURATION>
	//	</ROOT>
}

//==============================================================================
DQMHistosConsumerConfiguration::~DQMHistosConsumerConfiguration(void) {}

//==============================================================================
void DQMHistosConsumerConfiguration::init(ConfigurationManager* configManager)
{
	std::string processorUID;
	for(unsigned int row = 0; row < TableBase::activeTableView_->getNumberOfRows(); row++)
	{
		TableBase::activeTableView_->getValue(processorUID, row, ProcessorID);
		processorIDToRowMap_[processorUID] = row;
	}
}

//==============================================================================
std::vector<std::string> DQMHistosConsumerConfiguration::getProcessorIDList(void) const
{
	std::vector<std::string> list;
	for(auto& it : processorIDToRowMap_)
		list.push_back(it.first);
	return list;
}

//==============================================================================
std::string DQMHistosConsumerConfiguration::getFilePath(std::string processorUID) const
{
	check(processorUID);
	std::string val;
	TableBase::activeTableView_->getValue(
	    val, processorIDToRowMap_.find(processorUID)->second, FilePath);
	return val;
}

//==============================================================================
std::string DQMHistosConsumerConfiguration::getRadixFileName(
    std::string processorUID) const
{
	check(processorUID);
	std::string val;
	TableBase::activeTableView_->getValue(
	    val, processorIDToRowMap_.find(processorUID)->second, RadixFileName);
	return val;
}

//==============================================================================
bool DQMHistosConsumerConfiguration::getSaveFile(std::string processorUID) const
{
	check(processorUID);
	bool val;
	TableBase::activeTableView_->getValue(
	    val, processorIDToRowMap_.find(processorUID)->second, SaveFile);
	return val;
}

//==============================================================================
void DQMHistosConsumerConfiguration::check(std::string processorUID) const
{
	if(processorIDToRowMap_.find(processorUID) == processorIDToRowMap_.end())
	{
		std::cout << __COUT_HDR_FL__ << "Couldn't find processor " << processorUID
		          << " in the UDPDataStreamerConsumerConfiguration!" << std::endl;
		assert(0);
	}
}

DEFINE_OTS_CONFIGURATION(DQMHistosConsumerConfiguration)
