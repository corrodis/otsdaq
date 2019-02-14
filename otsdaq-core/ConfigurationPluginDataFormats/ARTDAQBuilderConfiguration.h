#ifndef _ots_ARTDAQBuilderConfiguration_h_
#define _ots_ARTDAQBuilderConfiguration_h_

#include <string>
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

namespace ots
{
class XDAQContextConfiguration;

class ARTDAQBuilderConfiguration : public ConfigurationBase
{
  public:
	ARTDAQBuilderConfiguration(void);
	virtual ~ARTDAQBuilderConfiguration(void);

	//Methods
	void        init(ConfigurationManager *configManager);
	void        outputFHICL(ConfigurationManager *configManager, const ConfigurationTree &builderNode, unsigned int selfRank, std::string selfHost, unsigned int selfPort, const XDAQContextConfiguration *contextConfig);
	std::string getFHICLFilename(const ConfigurationTree &builderNode);
};
}  // namespace ots
#endif
