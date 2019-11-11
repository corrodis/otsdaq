#include "otsdaq/DataManager/DataProducer.h"
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/DataManager/DataManager.h"
#include "otsdaq/DataManager/DataManagerSingleton.h"

#include <iostream>
#include <memory>
using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ (std::string("producer-") + bufferUID_ + "-" + processorUID_)

//========================================================================================================================
DataProducer::DataProducer(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, unsigned int bufferSize)
    : WorkLoop(processorUID), DataProducerBase(supervisorApplicationUID, bufferUID, processorUID, bufferSize)
{
	__COUT__ << "Constructed." << __E__;
}

//========================================================================================================================
DataProducer::~DataProducer(void) { __COUT__ << "Destructed." << __E__; }

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
