#ifndef _ots_DataProducer_h_
#define _ots_DataProducer_h_

#include "otsdaq-core/WorkLoopManager/WorkLoop.h"
#include "otsdaq-core/DataManager/DataProcessor.h"
#include "otsdaq-core/DataManager/CircularBuffer.h"
#include <string>

namespace ots
{

class DataProducer : public DataProcessor, public virtual WorkLoop
{
public:
	DataProducer(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, unsigned int bufferSize=100);
	virtual ~DataProducer(void);

    void         registerToBuffer   (void);
    virtual void startProcessingData(std::string runNumber);
    virtual void stopProcessingData (void);

    template<class D, class H>
	int attachToEmptySubBuffer(D*& data, H*& header)
	{
		return static_cast<CircularBuffer<D,H>* >(theCircularBuffer_)->getBuffer(DataProcessor::processorUID_).attachToEmptySubBuffer(data, header);
	}

    template<class D, class H>
	int setWrittenSubBuffer(void)
	{
		return static_cast<CircularBuffer<D,H>* >(theCircularBuffer_)->getBuffer(DataProcessor::processorUID_).setWrittenSubBuffer();
	}

    template<class D, class H>
	int write(const D& buffer)
	{
		return static_cast<CircularBuffer<D,H>* >(theCircularBuffer_)->getBuffer(DataProcessor::processorUID_).write(buffer);
	}

	template<class D, class H>
	int write(const D& buffer, const H& header)
	{
		return static_cast<CircularBuffer<D,H>* >(theCircularBuffer_)->getBuffer(DataProcessor::processorUID_).write(buffer, header);
	}

	unsigned int getBufferSize(void){return bufferSize_;}
private:
	unsigned int bufferSize_;

};

}

#endif
