#include "otsdaq-core/DataManager/DataProducerBase.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"

#include <iostream>
#include <memory>
using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ (std::string("Producer-") + DataProcessor::processorUID_)

//========================================================================================================================
DataProducerBase::DataProducerBase(const std::string& supervisorApplicationUID,
                                   const std::string& bufferUID,
                                   const std::string& processorUID,
                                   unsigned int       bufferSize)
    : DataProcessor(supervisorApplicationUID, bufferUID, processorUID)
    , bufferSize_(bufferSize)
{
	__COUT__ << "Constructor." << __E__;
	registerToBuffer();
	__COUT__ << "Constructed." << __E__;
}  // end constructor()

//========================================================================================================================
DataProducerBase::~DataProducerBase(void)
{
	__COUT__ << "Destructed." << __E__;
}  // end destructor()

//========================================================================================================================
// mirror DataConsumer::registerToBuffer
void DataProducerBase::registerToBuffer(void)
{
	__COUT__ << "Producer '" << DataProcessor::processorUID_
	         << "' is registering to DataManager Supervisor Buffer '"
	         << DataProcessor::supervisorApplicationUID_ << ":"
	         << DataProcessor::bufferUID_ << ".'" << std::endl;

	DataManager* dataManager =
	    (DataManagerSingleton::getInstance(supervisorApplicationUID_));

	dataManager->registerProducer(bufferUID_, this);

	{
		__SS__;
		dataManager->dumpStatus(&ss);
		std::cout << ss.str() << __E__;
	}

	__COUT__ << "Producer '" << DataProcessor::processorUID_ << "' Registered." << __E__;
}  // end registerToBuffer()
//
////========================================================================================================================
////mirror DataConsumer::unregisterFromBuffer
// void DataProducerBase::unregisterFromBuffer(void)
//{
//	__COUT__ << "Producer '" << DataProcessor::processorUID_ <<
//			"' is unregistering to DataManager Supervisor Buffer '" <<
//			DataProcessor::supervisorApplicationUID_ << ":" <<
//			DataProcessor::bufferUID_ << ".'" << std::endl;
//
//	DataManager* dataManager =
//		(DataManagerSingleton::getInstance(
//				supervisorApplicationUID_));
//
//	dataManager->unregisterProducer(
//						bufferUID_,DataProcessor::processorUID_);
//
//	{
//		__SS__;
//		dataManager->dumpStatus(&ss);
//		std::cout << ss.str() << __E__;
//	}
//
//	__COUT__ << "Producer '" << DataProcessor::processorUID_ <<
//			"' Unregistered." << __E__;
//} //end unregisterFromBuffer()
