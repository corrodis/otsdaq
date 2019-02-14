#ifndef _ots_RawDataSaverConsumerConfiguration_h_
#define _ots_RawDataSaverConsumerConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <string>


namespace ots
{

class RawDataSaverConsumerConfiguration : public ConfigurationBase
{

public:

	RawDataSaverConsumerConfiguration(void);
	virtual ~RawDataSaverConsumerConfiguration(void);

	//Methods
	void init(ConfigurationManager *configManager);

	//Getter
	std::vector<std::string>  getProcessorIDList(void) const;
	std::string               getFilePath       (std::string processorUID) const;
	std::string               getRadixFileName  (std::string processorUID) const;
private:

	void check(std::string processorUID) const;
	enum{
		ProcessorID,
		FilePath,
		RadixFileName
	};

	std::map<std::string, unsigned int> processorIDToRowMap_;

};
}
#endif
