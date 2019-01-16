#include "otsdaq-core/DataManager/DataProducer.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"


#include <iostream>
#include <memory>
using namespace ots;

//========================================================================================================================
DataProducer::DataProducer(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, unsigned int bufferSize)
: WorkLoop      (processorUID)
, DataProducerBase (supervisorApplicationUID, bufferUID, processorUID)
{
}

//========================================================================================================================
DataProducer::~DataProducer(void)
{
}

//========================================================================================================================
void DataProducer::startProcessingData(std::string runNumber)
{
	__COUT__ << "startWorkLoop..." << std::endl;
	WorkLoop::startWorkLoop();
}

//========================================================================================================================
void DataProducer::stopProcessingData(void)
{
	__COUT__ << "stopWorkLoop..." << std::endl;
	WorkLoop::stopWorkLoop();
}
