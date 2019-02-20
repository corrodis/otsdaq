#ifndef _ots_ConfigurationInterface_h_
#define _ots_ConfigurationInterface_h_

#include "otsdaq-core/Macros/CoutMacros.h"
//#include "otsdaq-core/TablePluginDataFormats/Configurations.h"
#include <memory>
#include <set>
#include <sstream>

#include "../PluginMakers/MakeTable.h"
#include "otsdaq-core/TableCore/TableBase.h"
#include "otsdaq-core/TableCore/TableGroupKey.h"
#include "otsdaq-core/TableCore/TableVersion.h"

namespace ots
{
class ConfigurationHandlerBase;

class ConfigurationInterface
{
	friend class ConfigurationManagerRW;  // because need access to latestVersion() call
	                                      // for group metadata
	friend class ConfigurationManager;    // because need access to fill() call for group
	                                      // metadata

  public:
	virtual ~ConfigurationInterface() { ; }

	static ConfigurationInterface* getInstance(bool mode);
	static bool                    isVersionTrackingEnabled();
	static void                    setVersionTrackingEnabled(bool setValue);

	static const std::string GROUP_METADATA_TABLE_NAME;
	//==============================================================================
	// get
	//	Note: If filling, assume, new view becomes active view.
	//
	//	Loose column matching can be used to ignore column names when filling.
	//
	void get(TableBase*&                          configuration,
	         const std::string                    configurationName,
	         std::shared_ptr<const TableGroupKey> groupKey            = 0,
	         const std::string*                   groupName           = 0,
	         bool                                 dontFill            = false,
	         TableVersion                         version             = TableVersion(),
	         bool                                 resetConfiguration  = true,
	         bool                                 looseColumnMatching = false)
	{
		if(configuration == 0)
		{
			// try making configuration table plugin, if fails use TableBase
			try
			{
				configuration = makeTable(configurationName);
			}
			catch(...)
			{
			}

			if(configuration == 0)
			{
				//__COUT__ << "Using TableBase object with configuration name " <<
				// configurationName << std::endl;

				// try making configuration base..
				//	if it fails, then probably something wrong with Info file
				try
				{
					configuration = new TableBase(configurationName);
				}
				catch(...)  // failure so cleanup any halfway complete configuration work
				{
					__COUT_WARN__ << "Failed to even use TableBase!" << std::endl;
					if(configuration)
						delete configuration;
					configuration = 0;
					throw;
				}
			}
			//__COUT__ << "Configuration constructed!" << std::endl;
		}

		if(groupKey != 0 && groupName != 0)
		{  // FIXME -- new TableGroup and TableGroupKey should be used!
			// version = configurations->getConditionVersion(*groupKey,
			// configuration->getTableName());
			__SS__
			    << "FATAL ERROR: new TableGroup and TableGroupKey should be used!"
			    << std::endl;
			__SS_THROW__;
		}
		else if(!dontFill)
		{
			// check version choice
			if(version == TableVersion::INVALID &&
			   (version = findLatestVersion(configuration)) == TableVersion::INVALID)
			{
				__COUT__ << "FATAL ERROR: Can't ask to fill a configuration object with "
				            "a negative version! "
				         << configurationName << std::endl;
				__SS__ << "FATAL ERROR: Invalid latest version." << std::endl
				       << std::endl
				       << std::endl
				       << "*******************" << std::endl
				       << "Suggestion: If you expect a version to exist for this "
				          "configuration, perhaps this is your first time running with "
				          "the artdaq database. (and your old configurations have not "
				          "been transferred?) "
				       << std::endl
				       << "Try running this once:\n\n\totsdaq_database_migrate"
				       << std::endl
				       << std::endl
				       << "This will migrate the old ots file system configuration to "
				          "the artdaq database approach."
				       << std::endl
				       << std::endl
				       << std::endl;
				__SS_THROW__;
			}
		}

		if(resetConfiguration)  // reset to empty configuration views and no active view
		{  // EXCEPT, keep temporary views! (call TableBase::reset to really reset all)
			configuration->deactivate();
			std::set<TableVersion> versions = configuration->getStoredVersions();
			for(auto& version : versions)
				if(!version.isTemporaryVersion())  // if not temporary
					configuration->eraseView(version);
		}

		if(dontFill)
			return;

		// Note: assume new view becomes active view

		// take advantage of version possibly being cached
		if(configuration->isStored(version))
		{
			//__COUT__ << "Using archived version: " << version << std::endl;

			// Make sure this version is not already active
			if(!configuration->isActive() || version != configuration->getViewVersion())
				configuration->setActiveView(version);

			configuration->getViewP()->setLastAccessTime();
			return;
		}

		try  // to fill
		{
			if(version.isTemporaryVersion())
			{
				__COUT_ERR__ << "FATAL ERROR: Can not use interface to fill a "
				                "configuration object with a temporary version!"
				             << std::endl;
				__SS__ << "FATAL ERROR: Invalid temporary version v" << version
				       << std::endl;
				__SS_THROW__;
			}

			configuration->setupMockupView(version);
			configuration->setActiveView(version);

			// loose column matching can be used to ignore column names
			configuration->getViewP()->setLooseColumnMatching(looseColumnMatching);
			fill(configuration, version);
			if(looseColumnMatching)
				configuration->getViewP()->setLooseColumnMatching(false);
			configuration->getViewP()->setLastAccessTime();

			/////////////////////
			// verify the new view
			if(configuration->getViewP()->getVersion() != version)
			{
				__COUT__ << "Version mismatch!! "
				         << configuration->getViewP()->getVersion() << " vs " << version
				         << std::endl;
				throw;
			}

			// match key by ignoring '_'
			bool         nameIsMatch = true;
			unsigned int nameIsMatchIndex, nameIsMatchStorageIndex;
			for(nameIsMatchIndex = 0, nameIsMatchStorageIndex = 0;
			    nameIsMatchIndex < configuration->getViewP()->getTableName().size();
			    ++nameIsMatchIndex)
			{
				if(configuration->getMockupViewP()
				       ->getTableName()[nameIsMatchStorageIndex] == '_')
					++nameIsMatchStorageIndex;  // skip to next storage character
				if(configuration->getViewP()->getTableName()[nameIsMatchIndex] == '_')
					continue;  // skip to next character

				// match to storage name
				if(nameIsMatchStorageIndex >=
				       configuration->getMockupViewP()->getTableName().size() ||
				   configuration->getViewP()->getTableName()[nameIsMatchIndex] !=
				       configuration->getMockupViewP()
				           ->getTableName()[nameIsMatchStorageIndex])
				{
					// size mismatch or character mismatch
					nameIsMatch = false;
					break;
				}
				++nameIsMatchStorageIndex;
			}

			if(nameIsMatch)  // if name is considered match by above rule, then force
			                 // matchup
				configuration->getViewP()->setTableName(
				    configuration->getMockupViewP()->getTableName());
			else  // configuration->getViewP()->getTableName() !=
			      // configuration->getMockupViewP()->getTableName())
			{
				__COUT__ << "View Table Name mismatch!! "
				         << configuration->getViewP()->getTableName() << " vs "
				         << configuration->getMockupViewP()->getTableName() << std::endl;
				throw;
			}
			configuration->getViewP()
			    ->init();  // sanitize for column info (and possibly on dataType soon?)

			// at this point, view has been verified!
			/////////////////////
		}
		catch(...)
		{
			__COUT__ << "Error occurred while getting and filling Configuration \""
			         << configurationName << "\" version:" << version << std::endl;
			__COUT__ << "\t-Configuration interface mode=" << theMode_ << std::endl;
			throw;
		}

	}  // end get()

	// table handling
	virtual std::set<std::string /*name*/> getAllTableNames() const
	    throw(std::runtime_error)
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "findAllGlobalConfigurations in a mode with this functionality "
		          "implemented (e.g. DatabaseConfigurationInterface).");
	}
	virtual std::set<TableVersion> getVersions(const TableBase* configuration) const = 0;
	const bool&                    getMode() const { return theMode_; }
	TableVersion                   saveNewVersion(TableBase*   configuration,
	                                              TableVersion temporaryVersion,
	                                              TableVersion newVersion = TableVersion());

	// group handling
	virtual std::set<std::string /*name*/> getAllTableGroupNames(
	    const std::string& filterString = "") const throw(std::runtime_error)
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "findAllGlobalConfigurations in a mode with this functionality "
		          "implemented (e.g. DatabaseConfigurationInterface).");
	}
	virtual std::set<TableGroupKey> getKeys(const std::string& groupName) const
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "findAllGlobalConfigurations in a mode with this functionality "
		          "implemented (e.g. DatabaseConfigurationInterface).");
	}

	// Caution: getTableGroupMembers must be carefully used.. the table versions
	// are as initially defined for table versions aliases, i.e. not converted according
	// to the metadata groupAliases!
	virtual std::map<std::string /*name*/, TableVersion /*version*/>
	getTableGroupMembers(std::string const& /*globalConfiguration*/,
	                             bool includeMetaDataTable = false) const
	    throw(std::runtime_error)
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "findAllGlobalConfigurations in a mode with this functionality "
		          "implemented (e.g. DatabaseConfigurationInterface).");
	}

	virtual void saveTableGroup(
	    std::map<std::string /*name*/,
	             TableVersion /*version*/> const& /*configurationMap*/,
	    std::string const& /*globalConfiguration*/) const throw(std::runtime_error)
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "findAllGlobalConfigurations in a mode with this functionality "
		          "implemented (e.g. DatabaseConfigurationInterface).");
	};

  protected:
	ConfigurationInterface(void);  // Protected constructor

	virtual void fill(TableBase* configuration, TableVersion version) const = 0;

  public:  // was protected,.. unfortunately, must be public to allow
	       // otsdaq_database_migrate and otsdaq_import_system_aliases to compile
	virtual TableGroupKey findLatestGroupKey(
	    const std::string& groupName) const /* return INVALID if no existing versions */
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call findLatestGroupKey in a "
		          "mode with this functionality implemented (e.g. "
		          "DatabaseConfigurationInterface).");
	}
	virtual TableVersion findLatestVersion(const TableBase* configuration)
	    const = 0;  // return INVALID if no existing versions
	virtual void saveActiveVersion(const TableBase* configuration,
	                               bool             overwrite = false) const = 0;

  protected:
	ConfigurationHandlerBase* theConfigurationHandler_;

  private:
	static ConfigurationInterface* theInstance_;
	static bool                    theMode_;  // 1 is FILE, 0 is artdaq-DB
	static bool
	    theVersionTrackingEnabled_;  // tracking versions 1 is enabled, 0 is disabled
};

}  // namespace ots
#endif
