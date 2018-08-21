#ifndef _ots_DatabaseConfigurationInterface_h_
#define _ots_DatabaseConfigurationInterface_h_

#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"

//#include "ConfigurationInterface.h"

#include <list>
#include <map>
#include <set>
#include <stdexcept>

namespace ots {

class ConfigurationBase;

class DatabaseConfigurationInterface : public ConfigurationInterface {
 public:
  using config_version_map_t = std::map<std::string /*name*/, ConfigurationVersion /*version*/>;

  DatabaseConfigurationInterface();
  ~DatabaseConfigurationInterface() { ; }

  // read configuration from database
  void 										fill(ConfigurationBase* /*configuration*/, ConfigurationVersion /*version*/) const throw(std::runtime_error);

  // write configuration to database
  void 										saveActiveVersion(const ConfigurationBase* /*configuration*/, bool overwrite = false) const throw(std::runtime_error);

  // find the latest configuration version by configuration type
  ConfigurationVersion 						findLatestVersion(const ConfigurationBase* /*configuration*/) const noexcept;


  // returns a list of all configuration names
  std::set<std::string /*name*/> 			getAllConfigurationNames() const throw(std::runtime_error);
  // find all configuration versions by configuration type
  std::set<ConfigurationVersion> 			getVersions(const ConfigurationBase* /*configuration*/) const noexcept;

  // find all configuration groups in database
  std::set<std::string /*name+version*/> 	getAllConfigurationGroupNames(const std::string &filterString = "") const throw(std::runtime_error);
  std::set<ConfigurationGroupKey> 			getKeys(const std::string &groupName) const;
  ConfigurationGroupKey  					findLatestGroupKey(const std::string& groupName) const noexcept;

  // return the contents of a configuration group
  config_version_map_t 			 			getConfigurationGroupMembers(std::string const& /*configurationGroup*/, bool includeMetaDataTable = false) const throw(std::runtime_error);

  // create a new configuration group from the contents map
  void 										saveConfigurationGroup(config_version_map_t const& /*configurationMap*/, std::string const& /*configurationGroup*/) const throw(std::runtime_error);

 private:
};
}

#endif

