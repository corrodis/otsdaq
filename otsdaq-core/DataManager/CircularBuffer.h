#ifndef _ots_CircularBuffer_h_
#define _ots_CircularBuffer_h_

#include "otsdaq-core/DataManager/CircularBufferBase.h"
#include "otsdaq-core/DataManager/BufferImplementation.h"

#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <iostream>
#include <string>
#include <map>
#include <atomic>

namespace ots
{
template <class D, class H>
class CircularBuffer : public CircularBufferBase
{
public:
	CircularBuffer         (void);
	virtual ~CircularBuffer(void);

	void         reset             (void);//This DOES NOT reset the consumer list
	void         resetConsumerList (void);
    bool         isEmpty           (void);
    unsigned int getNumberOfBuffers(void);

	inline int read (D& buffer, const std::string& consumerID)
	{
		H dummyHeader;
		return read(buffer, dummyHeader, consumerID);
	}

	inline int read (D& buffer, H& header, const std::string& consumerID)
	{
		setNextProducerBuffer(consumerID);
		unsigned int readCounter = theBuffer_.size()-1;
		++megaCounter_;
		if(megaCounter_%10000000 == 0)
			std::cout << __COUT_HDR_FL__ << __COUT_HDR_FL__
			<< "Consumer: " << consumerID
			<< " Reading producer: " << lastReadBuffer_[consumerID]->first
			<< " Buffer empty? " << lastReadBuffer_[consumerID]->second.isEmpty()
			<< " written buffers: " << lastReadBuffer_[consumerID]->second.numberOfWrittenBuffers()
			<< std::endl;
  	    int readReturnVal;
		while((readReturnVal = lastReadBuffer_[consumerID]->second.read(buffer, header, consumerID)) < 0 && readCounter > 0)
		{
		  setNextProducerBuffer(consumerID);
		  --readCounter;
		}
		return readReturnVal;
	}

	int read(D*& buffer, H*& header, const std::string& consumerID)
	{
		setNextProducerBuffer(consumerID);
		unsigned int readCounter = theBuffer_.size()-1;
		++megaCounter_;
		if(megaCounter_%10000000 == 0)
			std::cout << __COUT_HDR_FL__ << __COUT_HDR_FL__
			<< "Consumer: " << consumerID
			<< " Reading producer: " << lastReadBuffer_[consumerID]->first
			<< " Buffer empty? " << lastReadBuffer_[consumerID]->second.isEmpty()
			<< " written buffers: " << lastReadBuffer_[consumerID]->second.numberOfWrittenBuffers()
			<< std::endl;
  	    int readReturnVal;
		while((readReturnVal = lastReadBuffer_[consumerID]->second.read(buffer, header, consumerID)) < 0 && readCounter > 0)
		{
		  setNextProducerBuffer(consumerID);
		  --readCounter;
		}
		return readReturnVal;
	}

	BufferImplementation<D,H>& getLastReadBuffer(std::string consumerID){return lastReadBuffer_[consumerID]->second;}
	BufferImplementation<D,H>& getBuffer        (std::string producerID){return theBuffer_[producerID];}

private:
	std::map<std::string, BufferImplementation<D,H>> theBuffer_;

	void registerProducer     (std::string producerID, unsigned int numberOfSubBuffers=100);
	void registerConsumer     (std::string consumerID, CircularBufferBase::ConsumerPriority priority);
	void unregisterConsumer   (std::string consumerID);
	void setNextProducerBuffer(const std::string& consumer);
	//The first element is a CONSUMER, then it is the producer buffer map!!!!! 
	std::map<std::string, typename std::map<std::string, BufferImplementation<D,H>>::iterator> lastReadBuffer_;

	unsigned long long megaCounter_;
};
#include "otsdaq-core/DataManager/CircularBuffer.icc"

}
#endif













