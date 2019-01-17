#include "otsdaq-core/DataManager/CircularBufferBase.h"
#include "otsdaq-core/DataManager/DataProducer.h"
#include "otsdaq-core/DataManager/DataConsumer.h"

using namespace ots;


//========================================================================================================================
CircularBufferBase::CircularBufferBase(void)
{
}

//========================================================================================================================
CircularBufferBase::~CircularBufferBase(void)
{
}

//========================================================================================================================
void CircularBufferBase::registerProducer(DataProcessor* producer, unsigned int numberOfSubBuffers)
{
	registerProducer(producer->getProcessorID(), numberOfSubBuffers);
	producer->setCircularBuffer(this);
}

//========================================================================================================================
void CircularBufferBase::registerConsumer(DataProcessor* consumer)
{
	registerConsumer(consumer->getProcessorID(), HighConsumerPriority);
	consumer->setCircularBuffer(this);
}

//========================================================================================================================
void CircularBufferBase::unregisterConsumer(DataProcessor* consumer)
{
	unregisterConsumer(consumer->getProcessorID());
	consumer->setCircularBuffer(0);
}
