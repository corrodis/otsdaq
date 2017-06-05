#ifndef _ots_DataProcessor_h_
#define _ots_DataProcessor_h_

#include "otsdaq-core/WorkLoopManager/WorkLoop.h"
#include "otsdaq-core/DataManager/CircularBuffer.h"
#include "otsdaq-core/DataManager/CircularBufferBase.h"
#include <string>

namespace ots
{


class DataProcessor
{
public:
    DataProcessor(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID);
    virtual ~DataProcessor(void);

    virtual void registerToBuffer    (void) = 0;
    virtual void startProcessingData (std::string runNumber) = 0;
    virtual void stopProcessingData  (void) = 0;
    virtual void pauseProcessingData (void){stopProcessingData();}
    virtual void resumeProcessingData(void){startProcessingData("");}

    //Getters
    std::string getProcessorID(void);

    void setCircularBuffer(CircularBufferBase* circularBuffer);

protected:
    const std::string       supervisorApplicationUID_;
    const std::string       bufferUID_;
    const std::string       processorUID_;
    CircularBufferBase*     theCircularBuffer_;
};

}

#endif
