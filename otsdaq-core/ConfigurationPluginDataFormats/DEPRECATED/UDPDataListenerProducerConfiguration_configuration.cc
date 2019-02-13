#include "otsdaq-core/ConfigurationPluginDataFormats/UDPDataListenerProducerConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"

#include <iostream>

using namespace ots;

//==============================================================================
UDPDataListenerProducerConfiguration::UDPDataListenerProducerConfiguration(void)
    : ConfigurationBase("UDPDataListenerProducerConfiguration") {
  //////////////////////////////////////////////////////////////////////
  // WARNING: the names and the order MUST match the ones in the enum  //
  //////////////////////////////////////////////////////////////////////
  //	<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
  //	<ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  //xsi:noNamespaceSchemaLocation="ConfigurationInfo.xsd"> 	  <CONFIGURATION
  //Name="UDPDataListenerProducerConfiguration"> 	    <VIEW Name="UDP_DATA_LISTENER_PRODUCER_CONFIGURATION"
  //Type="File,Database,DatabaseTest"> 	      <COLUMN Name="ProcessorID" StorageName="PROCESSOR_ID"
  //DataType="VARCHAR2"/> 	      <COLUMN Name="BufferSize"  StorageName="BUFFER_SIZE"  DataType="NUMBER"  />
  //	      <COLUMN Name="IPAddress"   StorageName="IP_ADDRESS"   DataType="VARCHAR2"/>
  //	      <COLUMN Name="Port"        StorageName="PORT"         DataType="NUMBER"  />
  //	    </VIEW>
  //	  </CONFIGURATION>
  //	</ROOT>
}

//==============================================================================
UDPDataListenerProducerConfiguration::~UDPDataListenerProducerConfiguration(void) {}

//==============================================================================
void UDPDataListenerProducerConfiguration::init(ConfigurationManager *configManager) {
  std::string processorUID;
  for (unsigned int row = 0; row < ConfigurationBase::activeConfigurationView_->getNumberOfRows(); row++) {
    ConfigurationBase::activeConfigurationView_->getValue(processorUID, row, ProcessorID);
    processorIDToRowMap_[processorUID] = row;
  }
}

//==============================================================================
unsigned int UDPDataListenerProducerConfiguration::getBufferSize(std::string processorUID) const {
  check(processorUID);
  unsigned int val;
  ConfigurationBase::activeConfigurationView_->getValue(val, processorIDToRowMap_.find(processorUID)->second,
                                                        BufferSize);
  return val;
}

//==============================================================================
std::string UDPDataListenerProducerConfiguration::getIPAddress(std::string processorUID) const {
  check(processorUID);
  std::string val;
  ConfigurationBase::activeConfigurationView_->getValue(val, processorIDToRowMap_.find(processorUID)->second,
                                                        IPAddress);
  return val;
}

//==============================================================================
unsigned int UDPDataListenerProducerConfiguration::getPort(std::string processorUID) const {
  check(processorUID);
  unsigned int val;
  ConfigurationBase::activeConfigurationView_->getValue(val, processorIDToRowMap_.find(processorUID)->second, Port);
  return val;
}

//==============================================================================
void UDPDataListenerProducerConfiguration::check(std::string processorUID) const {
  if (processorIDToRowMap_.find(processorUID) == processorIDToRowMap_.end()) {
    std::cout << __COUT_HDR_FL__ << "Couldn't find processor " << processorUID
              << " in the UDPDataStreamerConsumerConfiguration!" << std::endl;
    assert(0);
  }
}

DEFINE_OTS_CONFIGURATION(UDPDataListenerProducerConfiguration)
