#ifndef _ots_ConfigurationInterface_h_
#define _ots_ConfigurationInterface_h_

#include <memory>
#include <set>
#include <sstream>
#include "otsdaq-core/Macros/CoutMacros.h"

#include "otsdaq-core/PluginMakers/MakeTable.h"
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
	//	if accumulatedErrors, then invalid data is allowed
	//		(not the same as "warnings allowed", because could create invalid tree
	//situations)
	void get(TableBase*&                          table,
	         const std::string                    tableName,
	         std::shared_ptr<const TableGroupKey> groupKey            = 0,
	         const std::string*                   groupName           = 0,
	         bool                                 dontFill            = false,
	         TableVersion                         version             = TableVersion(),
	         bool                                 resetConfiguration  = true,
	         bool                                 looseColumnMatching = false,
	         std::string*                         accumulatedErrors   = 0)
	{
		if(table == 0)
		{
			// try making table table plugin, if fails use TableBase
			try
			{
				table = makeTable(tableName);
			}
			catch(...)
			{
			}

			if(table == 0)
			{
				//__COUT__ << "Using TableBase object with table name " <<
				// tableName << std::endl;

				// try making table base..
				//	if it fails, then probably something wrong with Info file
				try
				{
					table = new TableBase(tableName);
				}
				catch(...)  // failure so cleanup any halfway complete table work
				{
					__COUT_WARN__ << "Failed to even use TableBase!" << std::endl;
					if(table)
						delete table;
					table = 0;
					throw;
				}
			}
			//__COUT__ << "Table constructed!" << std::endl;
		}

		if(groupKey != 0 && groupName != 0)
		{  // FIXME -- new TableGroup and TableGroupKey should be used!
			// version = configurations->getConditionVersion(*groupKey,
			// table->getTableName());
			__SS__ << "FATAL ERROR: new TableGroup and TableGroupKey should be used!"
			       << std::endl;
			__SS_THROW__;
		}
		else if(!dontFill)
		{
			// check version choice
			if(version == TableVersion::INVALID &&
			   (version = findLatestVersion(table)) == TableVersion::INVALID)
			{
				__COUT__ << "FATAL ERROR: Can't ask to fill a table object with "
				            "a negative version! "
				         << tableName << std::endl;
				__SS__ << "FATAL ERROR: Invalid latest version." << std::endl
				       << std::endl
				       << std::endl
				       << "*******************" << std::endl
				       << "Suggestion: If you expect a version to exist for this "
				          "table, perhaps this is your first time running with "
				          "the artdaq database. (and your old configurations have not "
				          "been transferred?) "
				       << std::endl
				       << "Try running this once:\n\n\totsdaq_database_migrate"
				       << std::endl
				       << std::endl
				       << "This will migrate the old ots file system table to "
				          "the artdaq database approach."
				       << std::endl
				       << std::endl
				       << std::endl;
				__SS_THROW__;
			}
		}

		if(resetConfiguration)  // reset to empty table views and no active view
		{  // EXCEPT, keep temporary views! (call TableBase::reset to really reset all)
			table->deactivate();
			std::set<TableVersion> versions = table->getStoredVersions();
			for(auto& version : versions)
				if(!version.isTemporaryVersion())  // if not temporary
					table->eraseView(version);
		}

		if(dontFill)
			return;

		// Note: assume new view becomes active view

		// take advantage of version possibly being cached
		if(table->isStored(version))
		{
			//__COUT__ << "Using archived version: " << version << std::endl;

			// Make sure this version is not already active
			if(!table->isActive() || version != table->getViewVersion())
				table->setActiveView(version);

			table->getViewP()->setLastAccessTime();

			try
			{
				// sanitize for column info and dataTypes
				table->getViewP()->init();
			}
			catch(const std::runtime_error& e)
			{
				__SS__ << "Error occurred while getting and filling Table \"" << tableName
				       << "\" version:" << version << std::endl;
				ss << "\n" << e.what() << __E__;
				__COUT__ << StringMacros::stackTrace() << __E__;

				// if accumulating errors, allow invalid data
				if(accumulatedErrors)
					*accumulatedErrors += ss.str();
				else
					throw;
			}

			return;
		}

		try  // to fill
		{
			if(version.isTemporaryVersion())
			{
				__SS__ << "FATAL ERROR: Can not use interface to fill a "
				          "table object with a temporary version!"
				       << std::endl;
				ss << "FATAL ERROR: Invalid temporary version v" << version << std::endl;
				__SS_THROW__;
			}

			table->setupMockupView(version);
			table->setActiveView(version);

			// loose column matching can be used to ignore column names
			table->getViewP()->setLooseColumnMatching(looseColumnMatching);
			fill(table, version);
			if(looseColumnMatching)
				table->getViewP()->setLooseColumnMatching(false);
			table->getViewP()->setLastAccessTime();

			/////////////////////
			// verify the new view
			if(table->getViewP()->getVersion() != version)
			{
				__SS__ << "Version mismatch!! " << table->getViewP()->getVersion()
				       << " vs " << version << std::endl;
				__SS_THROW__;
			}

			// match key by ignoring '_'
			bool         nameIsMatch = true;
			unsigned int nameIsMatchIndex, nameIsMatchStorageIndex;
			for(nameIsMatchIndex = 0, nameIsMatchStorageIndex = 0;
			    nameIsMatchIndex < table->getViewP()->getTableName().size();
			    ++nameIsMatchIndex)
			{
				if(table->getMockupViewP()->getTableName()[nameIsMatchStorageIndex] ==
				   '_')
					++nameIsMatchStorageIndex;  // skip to next storage character
				if(table->getViewP()->getTableName()[nameIsMatchIndex] == '_')
					continue;  // skip to next character

				// match to storage name
				if(nameIsMatchStorageIndex >=
				       table->getMockupViewP()->getTableName().size() ||
				   table->getViewP()->getTableName()[nameIsMatchIndex] !=
				       table->getMockupViewP()->getTableName()[nameIsMatchStorageIndex])
				{
					// size mismatch or character mismatch
					nameIsMatch = false;
					break;
				}
				++nameIsMatchStorageIndex;
			}

			if(nameIsMatch)  // if name is considered match by above rule, then force
			                 // matchup
				table->getViewP()->setTableName(table->getMockupViewP()->getTableName());
			else  // table->getViewP()->getTableName() !=
			      // table->getMockupViewP()->getTableName())
			{
				__SS__ << "View Table Name mismatch!! "
				       << table->getViewP()->getTableName() << " vs "
				       << table->getMockupViewP()->getTableName() << std::endl;
				__SS_THROW__;
			}

			try
			{
				// sanitize for column info and dataTypes
				table->getViewP()->init();
			}
			catch(const std::runtime_error& e)
			{
				__SS__ << "Error occurred while getting and filling Table \"" << tableName
				       << "\" version:" << version << std::endl;
				ss << "\n" << e.what() << __E__;
				__COUT__ << StringMacros::stackTrace() << __E__;

				// if accumulating errors, allow invalid data
				if(accumulatedErrors)
					*accumulatedErrors += ss.str();
				else
					throw;
			}

			// at this point, view has been verified!
			/////////////////////
		}
		catch(const std::runtime_error& e)
		{
			__SS__ << "Error occurred while getting and filling Table \"" << tableName
			       << "\" version:" << version << std::endl;
			ss << "\n" << e.what() << __E__;
			__SS_THROW__;
		}
		catch(...)
		{
			__SS__ << "Unknown error occurred while getting and filling Table \""
			       << tableName << "\" version:" << version << std::endl;
			__SS_THROW__;
		}

	}  // end get()

	// table handling
	virtual std::set<std::string /*name*/> getAllTableNames() const
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "getAllTableNames in a mode with this functionality "
		          "implemented (e.g. DatabaseConfigurationInterface).");
	}
	virtual std::set<TableVersion> getVersions(const TableBase* configuration) const = 0;
	const bool&                    getMode() const { return theMode_; }
	TableVersion                   saveNewVersion(TableBase*   configuration,
	                                              TableVersion temporaryVersion,
	                                              TableVersion newVersion = TableVersion());

	// group handling
	virtual std::set<std::string /*name*/> getAllTableGroupNames(
	    const std::string& filterString = "") const
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "getAllTableGroupNames in a mode with this functionality "
		          "implemented (e.g. DatabaseConfigurationInterface).");
	}
	virtual std::set<TableGroupKey> getKeys(const std::string& groupName) const
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "getKeys in a mode with this functionality "
		          "implemented (e.g. DatabaseConfigurationInterface).");
	}

	// Caution: getTableGroupMembers must be carefully used.. the table versions
	// are as initially defined for table versions aliases, i.e. not converted according
	// to the metadata groupAliases!
	virtual std::map<std::string /*name*/, TableVersion /*version*/> getTableGroupMembers(
	    std::string const& /*groupName*/, bool includeMetaDataTable = false) const
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "getTableGroupMembers in a mode with this functionality "
		          "implemented (e.g. DatabaseConfigurationInterface).");
	}

	virtual void saveTableGroup(
	    std::map<std::string /*name*/,
	             TableVersion /*version*/> const& /*tableToVersionMap*/,
	    std::string const& /*groupName*/) const
	{
		__SS__;
		__THROW__(ss.str() +
		          "ConfigurationInterface::... Must only call "
		          "saveTableGroup in a mode with this functionality "
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
