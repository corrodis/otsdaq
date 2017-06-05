#ifndef _ots_DataManager_h_
#define _ots_DataManager_h_

#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"
#include "otsdaq-core/DataManager/CircularBuffer.h"
#include "otsdaq-core/ConfigurationInterface/Configurable.h"

#include <map>
#include <string>
#include <vector>
#include <memory>

namespace ots
{
class DataProcessor;
class DataProducer;
class DataConsumer;
class CircularBufferBase;

class DataManager: public VStateMachine, public Configurable
{
public:
             DataManager(const ConfigurationTree& theXDAQContextConfigTree, const std::string& supervisorConfigurationPath);
    virtual ~DataManager(void);

    //State Machine Methods
    virtual void configure(void);
    virtual void halt     (void);
    virtual void pause    (void);
    virtual void resume   (void);
    virtual void start    (std::string runNumber);
    virtual void stop     (void);



    template<class D, class H>
    void configureBuffer(std::string bufferUID)
    {
    	buffers_[bufferUID].buffer_ = new CircularBuffer<D,H>();
    	buffers_[bufferUID].status_ = Initialized;
    }
    void registerProducer(std::string bufferUID, DataProducer* producer);//The data manager becomes the owner of the producer object!
    void registerConsumer(std::string bufferUID, DataConsumer* consumer, bool registerToBuffer=true);//The data manager becomes the owner of the consumer object!

protected:
    void eraseAllBuffers (void);//!!!!!Delete all Buffers and all the pointers of the producers and consumers
    void eraseBuffer     (std::string bufferUID);//!!!!!Delete all the pointers of the producers and consumers

    void startAllBuffers (std::string runNumber);
    void stopAllBuffers  (void);
    void resumeAllBuffers(void);
    void pauseAllBuffers (void);

    void startBuffer     (std::string bufferUID, std::string runNumber);
    void stopBuffer      (std::string bufferUID);
    void resumeBuffer    (std::string bufferUID);
    void pauseBuffer     (std::string bufferUID);

//    void startProducer   (std::string bufferUID, std::string producerID);
//    void stopProducer    (std::string bufferUID, std::string producerID);
//    void startConsumer   (std::string bufferUID, std::string consumerID);
//    void stopConsumer    (std::string bufferUID, std::string consumerID);

//#include "otsdaq-core/DataTypes/DataStructs.h"
//    void setConsumerParameters(std::string name);

    enum BufferStatus {Initialized, Running};

    bool unregisterConsumer(std::string consumerID);
    bool deleteBuffer      (std::string bufferUID);
    struct Buffer
    {
    	CircularBufferBase*        buffer_;
    	std::vector<std::shared_ptr<DataProducer>> producers_;
    	//std::vector<DataProducer*> producers_;
    	std::vector<std::shared_ptr<DataConsumer>> consumers_;
    	BufferStatus               status_;
    };
    std::map<std::string, Buffer> buffers_;
};

}

#endif
