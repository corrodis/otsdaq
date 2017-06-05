#ifndef _ots_Configurable_h_
#define _ots_Configurable_h_

#include "otsdaq-core/ConfigurationInterface/ConfigurationTree.h"

namespace ots
{

class Configurable
{
public:
  Configurable(const ConfigurationTree& theXDAQContextConfigTree, const std::string& theConfigurationPath)
: theXDAQContextConfigTree_(theXDAQContextConfigTree)
, theConfigurationPath_    (theConfigurationPath)
{
  	std::cout << __PRETTY_FUNCTION__ << std::endl;
}
  virtual ~Configurable(){;}


protected:
	const ConfigurationTree theXDAQContextConfigTree_;
	const std::string       theConfigurationPath_;

};
}

#endif
