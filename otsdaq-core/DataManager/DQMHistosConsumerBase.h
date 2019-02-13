#ifndef _ots_DQMHistosConsumerBase_h_
#define _ots_DQMHistosConsumerBase_h_

#include <string>
#include "otsdaq-core/DataManager/DataConsumer.h"
#include "otsdaq-core/RootUtilities/DQMHistosBase.h"

namespace ots {

class DQMHistosConsumerBase : public DQMHistosBase, public DataConsumer {
 public:
  DQMHistosConsumerBase(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID,
                        ConsumerPriority priority)
      : WorkLoop(processorUID), DataConsumer(supervisorApplicationUID, bufferUID, processorUID, LowConsumerPriority) {
    ;
  }
  virtual ~DQMHistosConsumerBase(void) { ; }
};
}  // namespace ots

#endif
