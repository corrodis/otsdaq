#ifndef _ots_FileConfigurationInterface_h_
#define _ots_FileConfigurationInterface_h_

#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"

#include <set>

namespace ots
{
class ConfigurationBase;

class FileConfigurationInterface : public ConfigurationInterface
{
  public:
	FileConfigurationInterface() { ; }
	virtual ~FileConfigurationInterface() { ; }

	// read configuration from database
	void fill(ConfigurationBase* /*configuration*/, ConfigurationVersion /*version*/) const;

	// write configuration to database
	void saveActiveVersion(const ConfigurationBase* /*configuration*/, bool overwrite = false) const;

	// find the latest configuration version by configuration type
	ConfigurationVersion findLatestVersion(const ConfigurationBase* /*configuration*/) const;

	// find all configuration versions by configuration type
	std::set<ConfigurationVersion> getVersions(const ConfigurationBase* /*configuration*/) const;

  private:
};
}  // namespace ots

#endif
