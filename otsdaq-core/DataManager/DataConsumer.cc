#include "otsdaq-core/DataManager/DataConsumer.h"

#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

using namespace ots;

//========================================================================================================================
DataConsumer::DataConsumer(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, ConsumerPriority priority)
: WorkLoop          (processorUID)
, DataProcessor     (supervisorApplicationUID, bufferUID, processorUID)
, priority_         (priority)

{
	registerToBuffer();
}

//========================================================================================================================
DataConsumer::~DataConsumer(void)
{
}

//========================================================================================================================
DataConsumer::ConsumerPriority DataConsumer::getPriority(void)
{
	return priority_;
}

//========================================================================================================================
void DataConsumer::registerToBuffer(void)
{
	std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << "\tConsumerID: " << processorUID_ << " is registering to DataManager pointer: " << DataManagerSingleton::getInstance(supervisorApplicationUID_) << std::endl;
	(DataManagerSingleton::getInstance(supervisorApplicationUID_))->registerConsumer(bufferUID_,this);
}

//========================================================================================================================
void DataConsumer::startProcessingData(std::string runNumber)
{
	WorkLoop::startWorkLoop();
}

//========================================================================================================================
void DataConsumer::stopProcessingData(void)
{
	WorkLoop::stopWorkLoop();
}


