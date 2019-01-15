#ifndef _ots_CircularBufferBase_h_
#define _ots_CircularBufferBase_h_

#include <string>

namespace ots
{

class DataProcessor;

//CircularBufferBase
//	This class is the base class for the otsdaq Buffer
class CircularBufferBase
{
public:
	enum ConsumerPriority
	{
	    LowConsumerPriority, //If the buffers are full because a low priority consumer didn't emptied them then overwrite
	    HighConsumerPriority //Can't overwrite but need to wait for sometime before writing a buffer
	};

    CircularBufferBase();
    virtual ~CircularBufferBase();

    virtual void reset (void) = 0;
    void registerProducer  (DataProcessor*  producer, unsigned int numberOfSubBuffers=100);
    void registerConsumer  (DataProcessor*  consumer);
    void unregisterConsumer(DataProcessor*  consumer);
    //void registerConsumer(std::string name, ConsumerPriority priority);

    virtual bool         isEmpty           (void) = 0;
    virtual unsigned int getNumberOfBuffers(void) = 0;

protected:
    virtual void registerProducer  (std::string producerID, unsigned int numberOfSubBuffers=100) = 0;
    virtual void registerConsumer  (std::string consumerID, ConsumerPriority priority)           = 0;
    virtual void unregisterConsumer(std::string consumerID)                                      = 0;
};
}
#endif
