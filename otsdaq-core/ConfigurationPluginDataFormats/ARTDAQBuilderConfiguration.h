#ifndef _ots_ARTDAQBuilderConfiguration_h_
#define _ots_ARTDAQBuilderConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include <string>

namespace ots
{

class XDAQContextConfiguration;

class ARTDAQBuilderConfiguration : public ConfigurationBase
{

public:

	ARTDAQBuilderConfiguration(void);
	virtual ~ARTDAQBuilderConfiguration(void);

	//Methods
	void init						(ConfigurationManager *configManager);
	void outputFHICL				(const ConfigurationTree &builderNode, unsigned int selfRank, std::string selfHost, unsigned int selfPort, const XDAQContextConfiguration *contextConfig);
	std::string getFHICLFilename	(const ConfigurationTree &builderNode);

private:

	ConfigurationManager *configManager_;
};
}
#endif
