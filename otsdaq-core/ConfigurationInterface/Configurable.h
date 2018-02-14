#ifndef _ots_Configurable_h_
#define _ots_Configurable_h_

#include "otsdaq-core/ConfigurationInterface/ConfigurationTree.h"

namespace ots
{

class Configurable
{
public:
  Configurable(const ConfigurationTree& theXDAQContextConfigTree, const std::string& theConfigurationPath)
: theXDAQContextConfigTree_		(theXDAQContextConfigTree)
, theConfigurationPath_    		(theConfigurationPath)
, theConfigurationRecordName_	(theXDAQContextConfigTree_.getNode(theConfigurationPath_).getValueAsString())
{
  	__COUT__ << __E__;
}
  virtual ~Configurable(){;}


protected:
	const ConfigurationTree theXDAQContextConfigTree_;
	const std::string       theConfigurationPath_;
	const std::string       theConfigurationRecordName_;

};
}

#endif
