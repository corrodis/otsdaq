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
, DataProcessor (supervisorApplicationUID, bufferUID, processorUID)
, bufferSize_   (bufferSize)
{
	registerToBuffer();
}

//========================================================================================================================
DataProducer::~DataProducer(void)
{
}

//========================================================================================================================
void DataProducer::registerToBuffer(void)
{
	__COUT__ << "\tProducerID: " << processorUID_ << " is registering to DataManager pointer: " << DataManagerSingleton::getInstance(supervisorApplicationUID_) << std::endl;
	(DataManagerSingleton::getInstance(supervisorApplicationUID_))->registerProducer(bufferUID_,this);
	__COUT__ << "\tProducerID: " << processorUID_ << "...registered" << std::endl;
}

//========================================================================================================================
void DataProducer::startProcessingData(std::string runNumber)
{
	WorkLoop::startWorkLoop();
}

//========================================================================================================================
void DataProducer::stopProcessingData(void)
{
	WorkLoop::stopWorkLoop();
}


