#ifndef _ots_ConfigurationManagerRW_h_
#define _ots_ConfigurationManagerRW_h_

//#include <string>
//#include <map>
//#include <set>
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

// Configuration Types

//#include <memory>

namespace ots
{
struct TableInfo
{
	TableInfo()
	    :  // constructor
	    tablePtr_(0)
	{
	}

	std::set<TableVersion> versions_;
	TableBase*             tablePtr_;
};

struct GroupInfo
{
	std::set<TableGroupKey> keys_;
	std::string             latestKeyGroupAuthor_, latestKeyGroupComment_,
	    latestKeyGroupCreationTime_, latestKeyGroupTypeString_;
	std::map<std::string /*name*/, TableVersion /*version*/> latestKeyMemberMap_;

	TableGroupKey getLatestKey() { return *(keys_.rbegin()); }
};

#define Q(X) #X
#define QUOTE(X) Q(X)
#define __GETCFG_RW__(X) getConfigurationRW<X>(QUOTE(X))

//==============================================================================
// ConfigurationManagerRW
//	This is the ConfigurationManger with write access
//	This class inherits all public function from ConfigurationManager
// and is a "Friend" class of ConfigurationManager so has access to private members.
class ConfigurationManagerRW : public ConfigurationManager
{
  public:
	ConfigurationManagerRW(std::string username);

	//==============================================================================
	// Getters
	const std::string&      getUsername(void) const { return username_; }
	ConfigurationInterface* getConfigurationInterface(void) const
	{
		return theInterface_;
	}

	const std::map<std::string, TableInfo>& getAllTableInfo(
	    bool               refresh           = false,
	    std::string*       accumulatedErrors = 0,
	    const std::string& errorFilterName   = "");
	std::map<std::string /*tableName*/,
	         std::map<std::string /*aliasName*/, TableVersion /*version*/> >
	getVersionAliases(void) const;

	template<class T>
	T* getConfigurationRW(std::string name)
	{
		return (T*)getTableByName(name);
	}
	TableBase*    getVersionedTableByName(const std::string& tableName,
	                                      TableVersion       version,
	                                      bool               looseColumnMatching = false);
	TableBase*    getTableByName(const std::string& tableName);
	TableGroupKey findTableGroup(
	    const std::string&                                           groupName,
	    const std::map<std::string, TableVersion>&                   groupMembers,
	    const std::map<std::string /*name*/, std::string /*alias*/>& groupAliases =
	        std::map<std::string /*name*/, std::string /*alias*/>());
	TableBase* getMetadataTable(void)
	{
		return &groupMetadataTable_; /* created for use in otsdaq_flatten_system_aliases,
		                                e.g. */
	}

	//==============================================================================
	// modifiers of generic TableBase

	TableVersion saveNewTable(
	    const std::string& tableName,
	    TableVersion       temporaryVersion = TableVersion(),
	    bool makeTemporary = false);  //, bool saveToScratchVersion = false);
	TableVersion copyViewToCurrentColumns(const std::string& tableName,
	                                      TableVersion       sourceVersion);
	void         eraseTemporaryVersion(const std::string& tableName,
	                                   TableVersion       targetVersion = TableVersion());
	void         clearCachedVersions(const std::string& tableName);
	void         clearAllCachedVersions();

	//==============================================================================
	// modifiers of table groups

	void activateConfigurationGroup(const std::string& configGroupName,
	                                TableGroupKey      configGroupKey,
	                                std::string*       accumulatedTreeErrors = 0);

	TableVersion createTemporaryBackboneView(
	    TableVersion sourceViewVersion =
	        TableVersion());  //-1, from MockUp, else from valid backbone view version
	TableVersion saveNewBackbone(TableVersion temporaryVersion = TableVersion());

	//==============================================================================
	// modifiers of a table group based on alias, e.g. "Physics"
	TableGroupKey saveNewTableGroup(
	    const std::string&                   groupName,
	    std::map<std::string, TableVersion>& groupMembers,
	    const std::string& groupComment = TableViewColumnInfo::DATATYPE_COMMENT_DEFAULT,
	    std::map<std::string /*table*/, std::string /*alias*/>* groupAliases = 0);

	//==============================================================================
	// public group cache handling
	const GroupInfo&                        getGroupInfo(const std::string& groupName);
	const std::map<std::string, GroupInfo>& getAllGroupInfo() { return allGroupInfo_; }

	void testXDAQContext();  // for debugging

  private:
	//==============================================================================
	// group cache handling
	void cacheGroupKey(const std::string& groupName, TableGroupKey key);

	//==============================================================================
	// private members
	std::map<std::string, TableInfo> allTableInfo_;
	std::map<std::string, GroupInfo> allGroupInfo_;
};

/////
struct TableEditStruct
{
	// everything needed for editing a table
	TableBase*   table_;
	TableView*   tableView_;
	TableVersion temporaryVersion_, originalVersion_;
	bool         createdTemporaryVersion_;  // indicates if temp version was created here
	bool         modified_;                 // indicates if temp version was modified
	std::string  tableName_;
	/////
	TableEditStruct()
	{
		__SS__ << "impossible!" << std::endl;
		__SS_THROW__;
	}
	TableEditStruct(const std::string& tableName, ConfigurationManagerRW* cfgMgr)
	    : createdTemporaryVersion_(false), modified_(false), tableName_(tableName)
	{
		__COUT__ << "Creating Table-Edit Struct for " << tableName_ << std::endl;
		table_ = cfgMgr->getTableByName(tableName_);

		if(!(originalVersion_ = table_->getView().getVersion()).isTemporaryVersion())
		{
			__COUT__ << "Start version " << originalVersion_ << std::endl;
			// create temporary version for editing
			temporaryVersion_ = table_->createTemporaryView(originalVersion_);
			cfgMgr->saveNewTable(
			    tableName_,
			    temporaryVersion_,
			    true);  // proper bookkeeping for temporary version with the new version

			__COUT__ << "Created temporary version " << temporaryVersion_ << std::endl;
			createdTemporaryVersion_ = true;
		}
		else  // else table is already temporary version
			__COUT__ << "Using temporary version " << temporaryVersion_ << std::endl;

		tableView_ = table_->getViewP();
	}
};  // end TableEditStruct declaration

}  // namespace ots

#endif
