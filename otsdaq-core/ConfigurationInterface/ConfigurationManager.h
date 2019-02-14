#ifndef _ots_ConfigurationManager_h_
#define _ots_ConfigurationManager_h_

#include <map>
#include <set>
#include <string>
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationVersion.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationTree.h"

namespace ots
{
class ProgressBar;

#define __GET_CONFIG__(X) getConfiguration<X>(QUOTE(X))

class ConfigurationManager
{
	//ConfigurationManagerRW is a "Friend" class of ConfigurationManager so has access to private members.
	friend class ConfigurationManagerRW;

  public:
	//==============================================================================
	//Static members
	static const std::string READONLY_USER;
	static const std::string ACTIVE_GROUP_FILENAME;
	static const std::string ALIAS_VERSION_PREAMBLE;
	static const std::string SCRATCH_VERSION_ALIAS;

	static const std::string XDAQ_CONTEXT_CONFIG_NAME;
	static const std::string XDAQ_APPLICATION_CONFIG_NAME;
	static const std::string GROUP_ALIASES_CONFIG_NAME;
	static const std::string VERSION_ALIASES_CONFIG_NAME;

	static const std::string ACTIVE_GROUP_NAME_CONTEXT;
	static const std::string ACTIVE_GROUP_NAME_BACKBONE;
	static const std::string ACTIVE_GROUP_NAME_ITERATE;
	static const std::string ACTIVE_GROUP_NAME_CONFIGURATION;
	static const std::string ACTIVE_GROUP_NAME_UNKNOWN;

	static const uint8_t METADATA_COL_ALIASES;
	static const uint8_t METADATA_COL_COMMENT;
	static const uint8_t METADATA_COL_AUTHOR;
	static const uint8_t METADATA_COL_TIMESTAMP;

	static const std::set<std::string> contextMemberNames_;   //list of context members
	static const std::set<std::string> backboneMemberNames_;  //list of backbone members
	static const std::set<std::string> iterateMemberNames_;   //list of iterate members

	static const std::set<std::string> &getContextMemberNames(void);
	static const std::set<std::string> &getBackboneMemberNames(void);
	static const std::set<std::string> &getIterateMemberNames(void);

	static std::string        encodeURIComponent(const std::string &sourceStr);
	static const std::string &convertGroupTypeIdToName(int groupTypeId);
	static int                getTypeOfGroup(const std::map<std::string /*name*/, ConfigurationVersion /*version*/> &memberMap);
	static const std::string &getTypeNameOfGroup(const std::map<std::string /*name*/, ConfigurationVersion /*version*/> &memberMap);

	//==============================================================================
	//Construct/Destruct

	enum
	{
		CONTEXT_TYPE,
		BACKBONE_TYPE,
		ITERATE_TYPE,
		CONFIGURATION_TYPE
	};

	ConfigurationManager();
	virtual ~ConfigurationManager(void);

	void init(std::string *accumulatedErrors = 0);
	void destroy(void);
	void destroyConfigurationGroup(const std::string &theGroup = "", bool onlyDeactivate = false);

	//==============================================================================
	//Getters

	void                  loadConfigurationGroup(const std::string &configGroupName, ConfigurationGroupKey configGroupKey, bool doActivate = false, std::map<std::string, ConfigurationVersion> *groupMembers = 0, ProgressBar *progressBar = 0, std::string *accumulateWarnings = 0, std::string *groupComment = 0, std::string *groupAuthor = 0, std::string *groupCreateTime = 0, bool doNotLoadMember = false, std::string *groupTypeString = 0, std::map<std::string /*name*/, std::string /*alias*/> *groupAliases = 0);
	void                  loadMemberMap(const std::map<std::string /*name*/, ConfigurationVersion /*version*/> &memberMap);
	ConfigurationGroupKey loadConfigurationBackbone(void);

	//================
	//getConfiguration
	//	get configuration * with specific configuration type
	template<class T>
	const T *getConfiguration(std::string name) const
	{
		return (T *)(getConfigurationByName(name));
	}

	const ConfigurationBase *getConfigurationByName(const std::string &configurationName) const;

	void dumpActiveConfiguration(const std::string &filePath, const std::string &dumpType);

	std::map<std::string /*groupAlias*/, std::pair<std::string /*groupName*/, ConfigurationGroupKey>> getActiveGroupAliases(void);
	//Note: this ConfigurationManager::getVersionAliases is called internally and by ConfigurationManagerRW::getVersionAliases
	std::map<std::string /*tableName*/, std::map<std::string /*aliasName*/, ConfigurationVersion>> getVersionAliases(void) const;

	std::pair<std::string /*groupName*/, ConfigurationGroupKey>                                             getConfigurationGroupFromAlias(std::string systemAlias, ProgressBar *progressBar = 0);
	std::map<std::string /*groupType*/, std::pair<std::string /*groupName*/, ConfigurationGroupKey>>        getActiveConfigurationGroups(void) const;
	const std::map<std::string /*groupType*/, std::pair<std::string /*groupName*/, ConfigurationGroupKey>> &getFailedConfigurationGroups(void) const { return lastFailedGroupLoad_; }
	const std::string &                                                                                     getActiveGroupName(const std::string &type = "") const;
	ConfigurationGroupKey                                                                                   getActiveGroupKey(const std::string &type = "") const;

	ConfigurationTree getNode(const std::string &nodeString, bool doNotThrowOnBrokenUIDLinks = false) const;  //"root/parent/parent/"
	ConfigurationTree getContextNode(const std::string &contextUID, const std::string &applicationUID) const;
	ConfigurationTree getSupervisorNode(const std::string &contextUID, const std::string &applicationUID) const;
	ConfigurationTree getSupervisorConfigurationNode(const std::string &contextUID, const std::string &applicationUID) const;

	std::vector<std::pair<std::string /*childName*/, ConfigurationTree>> getChildren(std::map<std::string, ConfigurationVersion> *memberMap = 0, std::string *accumulatedTreeErrors = 0) const;
	std::string                                                          getFirstPathToNode(const ConfigurationTree &node, const std::string &startPath = "/") const;

	std::map<std::string, ConfigurationVersion> getActiveVersions(void) const;

	//==============================================================================
	//Setters/Modifiers
	std::shared_ptr<ConfigurationGroupKey> makeTheConfigurationGroupKey(ConfigurationGroupKey key);
	void                                   restoreActiveConfigurationGroups(bool throwErrors = false, const std::string &pathToActiveGroupsFile = "");

  private:
	ConfigurationManager(const std::string &userName);  //private constructor called by ConfigurationManagerRW

	std::string                            username_;  //user of the configuration is READONLY_USER unless using ConfigurationManagerRW
	ConfigurationInterface *               theInterface_;
	std::shared_ptr<ConfigurationGroupKey> theConfigurationGroupKey_, theContextGroupKey_, theBackboneGroupKey_, theIterateGroupKey_;
	std::string                            theConfigurationGroup_, theContextGroup_, theBackboneGroup_, theIterateGroup_;

	std::map<std::string, std::pair<std::string, ConfigurationGroupKey>> lastFailedGroupLoad_;

	std::map<std::string, ConfigurationBase *> nameToConfigurationMap_;

	ConfigurationBase groupMetadataTable_;  //special table - version saved each time a group is created
};
}  // namespace ots

#endif
