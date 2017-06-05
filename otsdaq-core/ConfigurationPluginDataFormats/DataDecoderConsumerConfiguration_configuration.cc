#include "otsdaq-core/ConfigurationPluginDataFormats/DataDecoderConsumerConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
DataDecoderConsumerConfiguration::DataDecoderConsumerConfiguration(void)
: ConfigurationBase("DataDecoderConsumerConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//	<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//	<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//	  <CONFIGURATION Name="DataDecoderConsumerConfiguration">
	//	    <VIEW Name="DATA_DECODER_CONSUMER_CONFIGURATION" Type="File,Database,DatabaseTest">
	//	      <COLUMN Name="ProcessorID"   StorageName="PROCESSOR_ID"    DataType="VARCHAR2"/>
	//	    </VIEW>
	//	  </CONFIGURATION>
	//	</ROOT>


}

//==============================================================================
DataDecoderConsumerConfiguration::~DataDecoderConsumerConfiguration(void)
{}

//==============================================================================
void DataDecoderConsumerConfiguration::init(ConfigurationManager *configManager)
{
	std::string  processorUID;
	for(unsigned int row = 0; row < ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++)
	{
		ConfigurationBase::activeConfigurationView_->getValue(processorUID, row, ProcessorID);
		processorIDToRowMap_[processorUID] = row;
	}
}

//==============================================================================
std::vector<std::string> DataDecoderConsumerConfiguration::getProcessorIDList(void) const
{
	std::vector<std::string> list;
	for(auto& it: processorIDToRowMap_)
		list.push_back(it.first);
	return list;
}


//==============================================================================
void DataDecoderConsumerConfiguration::check(std::string processorUID) const
{
	if(processorIDToRowMap_.find(processorUID) == processorIDToRowMap_.end())
	{
		mf::LogError(__FILE__) << "Couldn't find processor " << processorUID << " in the UDPDataStreamerConsumerConfiguration!" << std::endl;
		assert(0);
	}
}

DEFINE_OTS_CONFIGURATION(DataDecoderConsumerConfiguration)
