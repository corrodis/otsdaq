#ifndef _ots_ARTDAQAggregatorConfiguration_h_
#define _ots_ARTDAQAggregatorConfiguration_h_

#include <string>
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

namespace ots
{
class XDAQContextConfiguration;

class ARTDAQAggregatorConfiguration : public ConfigurationBase
{
  public:
	ARTDAQAggregatorConfiguration (void);
	virtual ~ARTDAQAggregatorConfiguration (void);

	//Methods
	void        init (ConfigurationManager *configManager);
	void        outputFHICL (ConfigurationManager *configManager, const ConfigurationTree &builderNode, unsigned int selfRank, std::string selfHost, unsigned int selfPort, const XDAQContextConfiguration *contextConfig);
	std::string getFHICLFilename (const ConfigurationTree &builderNode);
};
}  // namespace ots
#endif
