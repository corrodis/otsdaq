#ifndef _ots_DataDecoderConsumer_h_
#define _ots_DataDecoderConsumer_h_

#include "otsdaq-core/Configurable/Configurable.h"
#include "otsdaq-core/DataDecoders/DataDecoder.h"
#include "otsdaq-core/DataManager/DataConsumer.h"

namespace ots
{
class DataDecoderConsumer : public DataDecoder, public DataConsumer, public Configurable
{
  public:
	DataDecoderConsumer(std::string              supervisorApplicationUID,
	                    std::string              bufferUID,
	                    std::string              processorUID,
	                    const ConfigurationTree& theXDAQContextConfigTree,
	                    const std::string&       configurationPath);
	virtual ~DataDecoderConsumer(void);

  protected:
	bool workLoopThread(toolbox::task::WorkLoop* workLoop);
};

}  // namespace ots

#endif
