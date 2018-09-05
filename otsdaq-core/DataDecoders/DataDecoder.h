#ifndef _ots_DataDecoder_h_
#define _ots_DataDecoder_h_

#include "otsdaq-core/DataDecoders/TriggerData.h"
#include "otsdaq-core/DataDecoders/BCOData.h"
#include "otsdaq-core/DataDecoders/DetectorDataBase.h"
#include "otsdaq-core/DataDecoders/FSSRData.h"
#include "otsdaq-core/DataDecoders/VIPICData.h"
#include "otsdaq-core/DataDecoders/PSI46DigData.h"
#include "otsdaq-core/DataDecoders/PSI46Data.h"
#include <string>
#include <queue>
#include <stdint.h>

namespace ots
{

class DataDecoder
{
public:
             DataDecoder(void);
    virtual ~DataDecoder(void);

    void     convertBuffer         (const std::string& buffer,std::queue<uint32_t>& convertedBuffer, bool invert);
    uint32_t convertBuffer         (const std::string& buffer,unsigned int bufferIndex, bool invert);
    bool     isBCOHigh             (uint32_t data);
    bool     isBCOLow              (uint32_t data);
    bool     isTrigger             (uint32_t data);
    bool     isFSSRData            (uint32_t data);
    bool     isVIPICData           (uint32_t data);
    bool     isPSI46DigData        (uint32_t data);
    bool     isPSI46Data           (uint32_t data);
    uint64_t mergeBCOHighAndLow    (uint32_t dataBCOHigh, uint32_t dataBCOLow);
    uint32_t decodeTrigger         (uint32_t data);
    void     insertBCOHigh         (uint64_t& bco, uint32_t dataBCOHigh);
    void     insertBCOLow          (uint64_t& bco, uint32_t dataBCOLow);
    void     decodeData            (uint32_t data, DetectorDataBase** decodedData);

protected:
    std::string   name_;
    TriggerData   triggerDataDecoder_;
    BCOData       bcoDataDecoder_;
    FSSRData      FSSRDataDecoder_;
    VIPICData     VIPICDataDecoder_;
    PSI46DigData  PSI46DigDataDecoder_;
    PSI46Data     PSI46DataDecoder_;

};

}

#endif
