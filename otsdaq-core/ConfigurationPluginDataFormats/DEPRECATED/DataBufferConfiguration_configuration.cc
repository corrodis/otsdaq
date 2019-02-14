#include "otsdaq-core/ConfigurationPluginDataFormats/DataBufferConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
DataBufferConfiguration::DataBufferConfiguration (void)
    : ConfigurationBase ("DataBufferConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names and the order MUST match the ones in the enum  //
	//////////////////////////////////////////////////////////////////////
	//  <ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd">
	//    <CONFIGURATION Name="DataBufferConfiguration">
	//      <VIEW Name="DATA_BUFFER_CONFIGURATION" Type="File,Database,DatabaseTest">
	//        <COLUMN Name="DataBufferID"    StorageName="DATA_BUFFER_ID"   DataType="VARCHAR2"/>
	//        <COLUMN Name="ProcessorID"     StorageName="PROCESSOR_ID"     DataType="VARCHAR2"/>
	//        <COLUMN Name="ProcessorType"   StorageName="PROCESSOR_TYPE"   DataType="VARCHAR2"/>
	//        <COLUMN Name="ProcessorClass"  StorageName="PROCESSOR_CLASS"  DataType="VARCHAR2"/>
	//        <COLUMN Name="ProcessorStatus" StorageName="PROCESSOR_STATUS" DataType="VARCHAR2"/>
	//      </VIEW>
	//    </CONFIGURATION>
	//  </ROOT>
}

//==============================================================================
DataBufferConfiguration::~DataBufferConfiguration (void)
{
}

//==============================================================================
void DataBufferConfiguration::init (ConfigurationManager* configManager)
{
	processorInfos_.clear ();

	//std::string uniqueID;//Right now ignored
	std::string dataBufferID;
	std::string processorUID;
	std::string processorType;
	Info        processorInfo;
	for (unsigned int row = 0; row < ConfigurationBase::activeConfigurationView_->getNumberOfRows (); row++)
	{
		ConfigurationBase::activeConfigurationView_->getValue (dataBufferID, row, DataBufferID);
		ConfigurationBase::activeConfigurationView_->getValue (processorUID, row, ProcessorID);
		ConfigurationBase::activeConfigurationView_->getValue (processorType, row, ProcessorType);
		ConfigurationBase::activeConfigurationView_->getValue (processorInfo.class_, row, ProcessorClass);
		ConfigurationBase::activeConfigurationView_->getValue (processorInfo.status_, row, ProcessorStatus);

		if (processorType == "Producer")
		{
			processorInfos_[dataBufferID].producers_[processorUID]  = processorInfo;
			processorInfos_[dataBufferID].processors_[processorUID] = processorInfo;
		}
		else if (processorType == "Consumer")
		{
			processorInfos_[dataBufferID].consumers_[processorUID]  = processorInfo;
			processorInfos_[dataBufferID].processors_[processorUID] = processorInfo;
		}
		else
		{
			std::cout << __COUT_HDR_FL__ << "Unrecognized Processor Type: " << processorType << std::endl;
			assert (0);
		}
	}
}

//==============================================================================
std::vector<std::string> DataBufferConfiguration::getProcessorIDList (std::string dataBufferID) const
{
	std::vector<std::string> list;
	for (auto const& it : processorInfos_.find (dataBufferID)->second.processors_)
		list.push_back (it.first);
	return list;
}

//==============================================================================
std::vector<std::string> DataBufferConfiguration::getProducerIDList (std::string dataBufferID) const
{
	std::vector<std::string> list;
	for (auto const& it : processorInfos_.find (dataBufferID)->second.producers_)
		list.push_back (it.first);
	return list;
}

//==============================================================================
bool DataBufferConfiguration::getProducerStatus (std::string dataBufferID, std::string producerID) const
{
	return processorInfos_.find (dataBufferID)->second.producers_.find (producerID)->second.status_;
}

//==============================================================================
std::string DataBufferConfiguration::getProducerClass (std::string dataBufferID, std::string producerID) const
{
	return processorInfos_.find (dataBufferID)->second.producers_.find (producerID)->second.class_;
}

//==============================================================================
std::vector<std::string> DataBufferConfiguration::getConsumerIDList (std::string dataBufferID) const
{
	std::vector<std::string> list;
	for (auto& it : processorInfos_.find (dataBufferID)->second.consumers_)
		list.push_back (it.first);
	return list;
}

//==============================================================================
bool DataBufferConfiguration::getConsumerStatus (std::string dataBufferID, std::string consumerID) const
{
	return processorInfos_.find (dataBufferID)->second.consumers_.find (consumerID)->second.status_;
}

//==============================================================================
std::string DataBufferConfiguration::getConsumerClass (std::string dataBufferID, std::string consumerID) const
{
	return processorInfos_.find (dataBufferID)->second.consumers_.find (consumerID)->second.class_;
}

DEFINE_OTS_CONFIGURATION (DataBufferConfiguration)
