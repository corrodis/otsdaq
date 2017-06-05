#ifndef _ots_ARTDAQAggregatorConfiguration_h_
#define _ots_ARTDAQAggregatorConfiguration_h_

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include <string>

namespace ots
{

class XDAQContextConfiguration;

class ARTDAQAggregatorConfiguration : public ConfigurationBase
{

public:

	ARTDAQAggregatorConfiguration(void);
	virtual ~ARTDAQAggregatorConfiguration(void);

	//Methods
	void init						(ConfigurationManager *configManager);
	void outputFHICL				(const ConfigurationTree &builderNode, unsigned int selfRank, const XDAQContextConfiguration *contextConfig);
	std::string getFHICLFilename	(const ConfigurationTree &builderNode);


};
}
#endif
