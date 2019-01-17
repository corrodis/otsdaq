#include "otsdaq-core/DataManager/DataProcessor.h"




using namespace ots;


//========================================================================================================================
DataProcessor::DataProcessor(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID)
: supervisorApplicationUID_(supervisorApplicationUID)
, bufferUID_               (bufferUID)
, processorUID_            (processorUID)
, theCircularBuffer_       (nullptr)
{
	__COUT__ << "DataProcessor constructed." << __E__;
	__COUTV__(supervisorApplicationUID_);
	__COUTV__(bufferUID_);
	__COUTV__(processorUID_);
}

//========================================================================================================================
DataProcessor::~DataProcessor(void)
{
}

//========================================================================================================================
std::string DataProcessor::getProcessorID(void)
{
	return processorUID_;
}

//========================================================================================================================
void DataProcessor::setCircularBuffer(CircularBufferBase* circularBuffer)
{
	theCircularBuffer_ = circularBuffer;
}
