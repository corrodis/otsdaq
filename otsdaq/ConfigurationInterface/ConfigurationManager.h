#ifndef _ots_ConfigurationManager_h_
#define _ots_ConfigurationManager_h_

#include <map>
#include <set>
#include <string>

#include "otsdaq/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq/ConfigurationInterface/ConfigurationTree.h"
#include "otsdaq/TableCore/TableVersion.h"

namespace ots
{
class ProgressBar;

#define __GET_CONFIG__(X) getTable<X>(QUOTE(X))

class ConfigurationManager
{
	// ConfigurationManagerRW is a "Friend" class of ConfigurationManager so has access to
	// private members.
	friend class ConfigurationManagerRW;

  public:
	//==============================================================================
	// Static members
	static const std::string READONLY_USER;
	static const std::string ACTIVE_GROUPS_FILENAME;
	static const std::string ALIAS_VERSION_PREAMBLE;
	static const std::string SCRATCH_VERSION_ALIAS;

	static const std::string XDAQ_CONTEXT_TABLE_NAME;
	static const std::string XDAQ_APPLICATION_TABLE_NAME;
	static const std::string XDAQ_APP_PROPERTY_TABLE_NAME;
	static const std::string GROUP_ALIASES_TABLE_NAME;
	static const std::string VERSION_ALIASES_TABLE_NAME;

	static const std::string ACTIVE_GROUP_NAME_CONTEXT;
	static const std::string ACTIVE_GROUP_NAME_BACKBONE;
	static const std::string ACTIVE_GROUP_NAME_ITERATE;
	static const std::string ACTIVE_GROUP_NAME_CONFIGURATION;
	static const std::string ACTIVE_GROUP_NAME_UNKNOWN;

	static const uint8_t METADATA_COL_ALIASES;
	static const uint8_t METADATA_COL_COMMENT;
	static const uint8_t METADATA_COL_AUTHOR;
	static const uint8_t METADATA_COL_TIMESTAMP;

	static const std::set<std::string> contextMemberNames_;   // list of context members
	static const std::set<std::string> backboneMemberNames_;  // list of backbone members
	static const std::set<std::string> iterateMemberNames_;   // list of iterate members
	std::set<std::string> 			   configurationMemberNames_;   // list of 'active' configuration members

	enum class GroupType
	{
		CONTEXT_TYPE,
		BACKBONE_TYPE,
		ITERATE_TYPE,
		CONFIGURATION_TYPE
	};

	// clang-format off

	static const std::set<std::string>& getContextMemberNames		(void);
	static const std::set<std::string>& getBackboneMemberNames		(void);
	static const std::set<std::string>& getIterateMemberNames		(void);
	const std::set<std::string>& 		getConfigurationMemberNames	(void);

	static const std::string& 			convertGroupTypeToName		(const ConfigurationManager::GroupType& groupTypeId);
	static ConfigurationManager::GroupType getTypeOfGroup			(const std::map<std::string /*name*/, TableVersion /*version*/>& memberMap);
	static const std::string& 			getTypeNameOfGroup			(const std::map<std::string /*name*/, TableVersion /*version*/>& memberMap);

	//==============================================================================
	// Construct/Destruct

	ConfigurationManager(bool initForWriteAccess  = false,
	                     bool initializeFromFhicl = false);
	virtual ~ConfigurationManager(void);
	
	

	void 								init						(std::string* accumulatedErrors = 0, bool initForWriteAccess = false);
	void 								destroy						(void);
	void 								destroyTableGroup			(const std::string& theGroup = "", bool onlyDeactivate = false);

	//==============================================================================
	// Getters

	void 								loadTableGroup				(
	    const std::string&                                     configGroupName,
	    TableGroupKey                                          tableGroupKey,
	    bool                                                   doActivate         = false,
	    std::map<std::string, TableVersion>*                   groupMembers       = 0,
	    ProgressBar*                                           progressBar        = 0,
	    std::string*                                           accumulateWarnings = 0,
	    std::string*                                           groupComment       = 0,
	    std::string*                                           groupAuthor        = 0,
	    std::string*                                           groupCreateTime    = 0,
	    bool                                                   doNotLoadMember    = false,
	    std::string*                                           groupTypeString    = 0,
	    std::map<std::string /*name*/, std::string /*alias*/>* groupAliases       = 0,
	    bool 												   onlyLoadIfBackboneOrContext = false);
	void 								loadMemberMap				(const std::map<std::string /*name*/, TableVersion /*version*/>& memberMap, std::string* accumulateWarnings = 0);
	TableGroupKey 						loadConfigurationBackbone	(void);

	//================
	// getTable
	//	get configuration * with specific configuration type
	template<class T>
	const T* 							getTable					(std::string name) const { return (T*)(getTableByName(name)); }
	const TableBase* 					getTableByName				(const std::string& configurationName) const;

	void 								dumpActiveConfiguration		(const std::string& filePath, const std::string& dumpType);
	void								dumpMacroMakerModeFhicl		(void);	
	
	std::map<std::string /*groupAlias*/,
		 std::pair<std::string /*groupName*/,
		 TableGroupKey>> 				getActiveGroupAliases		(void);
	// Note: this ConfigurationManager::getVersionAliases is called internally and by
	// ConfigurationManagerRW::getVersionAliases
	std::map<std::string /*tableName*/,
		std::map<std::string /*aliasName*/,
		TableVersion>>					getVersionAliases			(void) const;
	std::pair<std::string /*groupName*/,
		TableGroupKey> 					getTableGroupFromAlias		(std::string systemAlias, ProgressBar* progressBar = 0);
	std::map<std::string /*groupType*/,
		 std::pair<std::string /*groupName*/,
		 TableGroupKey>>				getActiveTableGroups		(void) const;
	const std::map<std::string /*groupType*/,
		std::pair<std::string /*groupName*/,
		TableGroupKey>>& 				getFailedTableGroups		(void) const {return lastFailedGroupLoad_;}
	const std::string& 					getActiveGroupName			(const ConfigurationManager::GroupType& type = ConfigurationManager::GroupType::CONFIGURATION_TYPE) const;
	TableGroupKey      					getActiveGroupKey			(const ConfigurationManager::GroupType& type = ConfigurationManager::GroupType::CONFIGURATION_TYPE) const;

	ConfigurationTree 					getNode						(const std::string& nodeString, bool doNotThrowOnBrokenUIDLinks = false) const;  //"root/parent/parent/"
	ConfigurationTree 					getContextNode				(const std::string& contextUID, const std::string& applicationUID) const;
	ConfigurationTree 					getSupervisorNode			(const std::string& contextUID, const std::string& applicationUID) const;
	ConfigurationTree 					getSupervisorTableNode		(const std::string& contextUID, const std::string& applicationUID) const;

	std::vector<std::pair<std::string /*childName*/,
		ConfigurationTree>> 			getChildren					(std::map<std::string, TableVersion>* memberMap = 0, std::string* accumulatedTreeErrors = 0) const;
	std::string 						getFirstPathToNode			(const ConfigurationTree& node, const std::string& startPath = "/") const;

	std::map<std::string, TableVersion> getActiveVersions			(void) const;

	const std::string& 					getOwnerContext				(void) { return ownerContextUID_; }
	const std::string& 					getOwnerApp					(void) { return ownerAppUID_; }
	bool               					isOwnerFirstAppInContext	(void);

	//==============================================================================
	// Setters/Modifiers
	std::shared_ptr<TableGroupKey> 		makeTheTableGroupKey		(TableGroupKey key);
	void                           		restoreActiveTableGroups	(bool throwErrors = false, const std::string& pathToActiveGroupsFile = "", bool onlyLoadIfBackboneOrContext = false);

	void 								setOwnerContext				(const std::string& contextUID) { ownerContextUID_ = contextUID; }
	void 								setOwnerApp					(const std::string& appUID) { ownerAppUID_ = appUID; }

  private:
	ConfigurationManager(const std::string& userName);  // private constructor called by ConfigurationManagerRW

	void 								initializeFromFhicl			(const std::string& fhiclPath);
	void 								recursiveInitFromFhiclPSet	(const std::string& tableName, const fhicl::ParameterSet& pset, const std::string& recordName = "", const std::string& groupName = "", const std::string& groupLinkIndex = "");
	void 								recursiveTreeToFhicl		(ConfigurationTree node, std::ostream& out, std::string& tabStr, std::string& commentStr, unsigned int depth = -1);

	std::string 										username_;  // user of the configuration is READONLY_USER unless using ConfigurationManagerRW
	ConfigurationInterface*        						theInterface_;
	std::shared_ptr<TableGroupKey> 						theConfigurationTableGroupKey_, theContextTableGroupKey_, theBackboneTableGroupKey_, theIterateTableGroupKey_;
	std::string 										theConfigurationTableGroup_, theContextTableGroup_, theBackboneTableGroup_, theIterateTableGroup_;

	std::map<std::string, 
		std::pair<std::string, TableGroupKey>> 			lastFailedGroupLoad_;

	std::map<std::string, TableBase*> 					nameToTableMap_;

	TableBase 											groupMetadataTable_;  	// special table - version saved each time a group is created

	std::string 										ownerContextUID_;  // optional, often there is a context that owns this configuration manager
	std::string 										ownerAppUID_;  // optional, often there is a supervisor that owns this configuration manager

	// clang-format on
};
}  // namespace ots

#endif
