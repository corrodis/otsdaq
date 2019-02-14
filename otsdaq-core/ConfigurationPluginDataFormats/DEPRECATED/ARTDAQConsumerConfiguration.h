#ifndef _ots_ARTDAQConsumerConfiguration_h_
#define _ots_ARTDAQConsumerConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include <string>

namespace ots
{

class ARTDAQConsumerConfiguration : public ConfigurationBase
{

public:

	ARTDAQConsumerConfiguration(void);
	virtual ~ARTDAQConsumerConfiguration(void);

	//Methods
	void init(ConfigurationManager *configManager);

	//Getters
	const std::string getConfigurationString(std::string processorUID) const;

private:
	enum{ProcessorID,
		ConfigurationString
	};

};
}
#endif
