#include "otsdaq-core/DataManager/DataProcessor.h"




using namespace ots;


//========================================================================================================================
DataProcessor::DataProcessor(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID)
: supervisorApplicationUID_(supervisorApplicationUID)
, bufferUID_               (bufferUID)
, processorUID_            (processorUID)
, theCircularBuffer_       (nullptr)
{
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
