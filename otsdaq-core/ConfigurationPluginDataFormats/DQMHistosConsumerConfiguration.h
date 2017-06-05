#ifndef _ots_DQMHistosConsumerConfiguration_h_
#define _ots_DQMHistosConsumerConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"

#include <string>


namespace ots
{

class DQMHistosConsumerConfiguration : public ConfigurationBase
{

public:

	DQMHistosConsumerConfiguration(void);
	virtual ~DQMHistosConsumerConfiguration(void);

	//Methods
	void init(ConfigurationManager *configManager);

	//Getter
	std::vector<std::string>  getProcessorIDList(void) const;
	std::string               getFilePath       (std::string processorUID) const;
	std::string               getRadixFileName  (std::string processorUID) const;
	bool                      getSaveFile       (std::string processorUID) const;

private:

	void check(std::string processorUID) const;
	enum{
		ProcessorID,
		FilePath,
		RadixFileName,
		SaveFile
	};

	std::map<std::string, unsigned int> processorIDToRowMap_;

};
}
#endif
