#ifndef _ots_TriggerData_h
#define _ots_TriggerData_h

#include <stdint.h>

namespace ots
{

class TriggerData
{
public:
             TriggerData(void);
    virtual ~TriggerData(void);

    bool     isTriggerHigh         (uint32_t data);
    bool     isTriggerLow          (uint32_t data);
    uint32_t decodeTriggerHigh     (uint32_t data);
    uint32_t decodeTriggerLow      (uint32_t data);
    uint64_t mergeTriggerHighAndLow(uint32_t  triggerHigh, uint32_t triggerLow);
    void     insertTriggerHigh     (uint64_t& trigger,     uint32_t dataTriggerHigh);
    void     insertTriggerLow      (uint64_t& trigger,     uint32_t dataTriggerLow);
};

}

#endif
