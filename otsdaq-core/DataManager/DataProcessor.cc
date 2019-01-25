#include "otsdaq-core/DataManager/DataProcessor.h"




using namespace ots;

#undef  __MF_SUBJECT__
#define __MF_SUBJECT__ (std::string("Processor-") + DataProcessor::processorUID_)

//========================================================================================================================
DataProcessor::DataProcessor(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID)
: supervisorApplicationUID_	(supervisorApplicationUID)
, bufferUID_               	(bufferUID)
, processorUID_            	(processorUID)
, theCircularBuffer_       	(nullptr)
{
	__COUT__ << "Constructor." << __E__;
	__COUTV__(supervisorApplicationUID_);
	__COUTV__(bufferUID_);
	__COUTV__(processorUID_);

}

//========================================================================================================================
DataProcessor::~DataProcessor(void)
{
	__COUT__ << "Destructor." << __E__;
}

//========================================================================================================================
void DataProcessor::setCircularBuffer(CircularBufferBase* circularBuffer)
{
	theCircularBuffer_ = circularBuffer;
}
