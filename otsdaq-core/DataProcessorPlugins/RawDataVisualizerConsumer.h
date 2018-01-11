#ifndef _ots_RawDataVisualizerConsumer_h_
#define _ots_RawDataVisualizerConsumer_h_

#include "otsdaq-core/DataManager/DataConsumer.h"
#include "otsdaq-core/ConfigurationInterface/Configurable.h"

#include <string>

namespace ots
{

  class ConfigurationManager;

class RawDataVisualizerConsumer : public DataConsumer, public Configurable
{
public:
	RawDataVisualizerConsumer(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& configurationPath);
	virtual ~RawDataVisualizerConsumer(void);

	void startProcessingData(std::string runNumber) override;
	void stopProcessingData (void) override;

	const std::string& getLastRawDataBuffer(void) {return data_;}

private:
	bool workLoopThread(toolbox::task::WorkLoop* workLoop);
	void fastRead(void);
	void slowRead(void);
	
	
	//For fast read
	std::string*                       dataP_;
	std::map<std::string,std::string>* headerP_;
	//For slow read
	std::string                        data_;
	std::map<std::string,std::string>  header_;

};
}

#endif
