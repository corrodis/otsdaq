#ifndef _ots_DatabaseConfigurationInterface_h_
#define _ots_DatabaseConfigurationInterface_h_

#include "otsdaq/ConfigurationInterface/ConfigurationInterface.h"

//#include "ConfigurationInterface.h"

#include <list>
#include <map>
#include <set>
#include <stdexcept>

namespace ots
{
class TableBase;

class DatabaseConfigurationInterface : public ConfigurationInterface
{
  public:
	using config_version_map_t = std::map<std::string /*name*/, TableVersion /*version*/>;

	DatabaseConfigurationInterface();
	~DatabaseConfigurationInterface() { ; }

	// read configuration from database
	void fill(TableBase* /*configuration*/, TableVersion /*version*/) const;

	// write configuration to database
	void saveActiveVersion(const TableBase* /*configuration*/, bool overwrite = false) const;

	// find the latest configuration version by configuration type
	TableVersion findLatestVersion(const TableBase* /*configuration*/) const noexcept;

	// returns a list of all configuration names
	std::set<std::string /*name*/> getAllTableNames() const;
	// find all configuration versions by configuration type
	std::set<TableVersion> getVersions(const TableBase* /*configuration*/) const noexcept;

	// find all configuration groups in database
	std::set<std::string /*name+version*/> getAllTableGroupNames(std::string const& filterString = "") const;
	std::set<TableGroupKey>                getKeys(const std::string& groupName) const;
	TableGroupKey                          findLatestGroupKey(const std::string& groupName) const noexcept;

	// return the contents of a configuration group
	config_version_map_t getTableGroupMembers(std::string const& /*configurationGroup*/, bool includeMetaDataTable = false) const;

	// create a new configuration group from the contents map
	void saveTableGroup(config_version_map_t const& /*configurationMap*/, std::string const& /*configurationGroup*/) const;

  private:
};
}  // namespace ots

#endif
