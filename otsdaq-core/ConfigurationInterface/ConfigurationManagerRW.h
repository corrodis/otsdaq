#ifndef _ots_ConfigurationManagerRW_h_
#define _ots_ConfigurationManagerRW_h_

//#include <string>
//#include <map>
//#include <set>
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

//Configuration Types

//#include <memory>

namespace ots
{
struct ConfigurationInfo
{
	ConfigurationInfo()
	    :  //constructor
	    configurationPtr_(0)
	{
	}

	std::set<ConfigurationVersion> versions_;
	ConfigurationBase*             configurationPtr_;
};

struct GroupInfo
{
	std::set<ConfigurationGroupKey> keys_;
	std::string                     latestKeyGroupAuthor_, latestKeyGroupComment_,
	    latestKeyGroupCreationTime_, latestKeyGroupTypeString_;
	std::map<std::string /*name*/, ConfigurationVersion /*version*/> latestKeyMemberMap_;

	ConfigurationGroupKey getLatestKey() { return *(keys_.rbegin()); }
};

#define Q(X) #X
#define QUOTE(X) Q(X)
#define __GETCFG_RW__(X) getConfigurationRW<X>(QUOTE(X))

//==============================================================================
//ConfigurationManagerRW
//	This is the ConfigurationManger with write access
//	This class inherits all public function from ConfigurationManager
// and is a "Friend" class of ConfigurationManager so has access to private members.
class ConfigurationManagerRW : public ConfigurationManager
{
  public:
	ConfigurationManagerRW(std::string username);

	//==============================================================================
	//Getters
	const std::string&      getUsername(void) const { return username_; }
	ConfigurationInterface* getConfigurationInterface(void) const { return theInterface_; }

	const std::map<std::string, ConfigurationInfo>&                                                             getAllConfigurationInfo(bool refresh = false, std::string* accumulatedErrors = 0, const std::string& errorFilterName = "");
	std::map<std::string /*tableName*/, std::map<std::string /*aliasName*/, ConfigurationVersion /*version*/> > getVersionAliases(void) const;

	template<class T>
	T* getConfigurationRW(std::string name)
	{
		return (T*)getConfigurationByName(name);
	}
	ConfigurationBase*    getVersionedConfigurationByName(const std::string& configurationName, ConfigurationVersion version, bool looseColumnMatching = false);
	ConfigurationBase*    getConfigurationByName(const std::string& configurationName);
	ConfigurationGroupKey findConfigurationGroup(const std::string& groupName, const std::map<std::string, ConfigurationVersion>& groupMembers, const std::map<std::string /*name*/, std::string /*alias*/>& groupAliases = std::map<std::string /*name*/, std::string /*alias*/>());
	ConfigurationBase*    getMetadataTable(void) { return &groupMetadataTable_; /* created for use in otsdaq_flatten_system_aliases, e.g. */ }

	//==============================================================================
	//modifiers of generic ConfigurationBase

	ConfigurationVersion saveNewConfiguration(const std::string& configurationName, ConfigurationVersion temporaryVersion = ConfigurationVersion(), bool makeTemporary = false);  //, bool saveToScratchVersion = false);
	ConfigurationVersion copyViewToCurrentColumns(const std::string& configurationName, ConfigurationVersion sourceVersion);
	void                 eraseTemporaryVersion(const std::string& configurationName, ConfigurationVersion targetVersion = ConfigurationVersion());
	void                 clearCachedVersions(const std::string& configurationName);
	void                 clearAllCachedVersions();

	//==============================================================================
	//modifiers of configuration groups

	void activateConfigurationGroup(const std::string& configGroupName, ConfigurationGroupKey configGroupKey, std::string* accumulatedTreeErrors = 0);

	ConfigurationVersion createTemporaryBackboneView(ConfigurationVersion sourceViewVersion = ConfigurationVersion());  //-1, from MockUp, else from valid backbone view version
	ConfigurationVersion saveNewBackbone(ConfigurationVersion temporaryVersion = ConfigurationVersion());

	//==============================================================================
	//modifiers of a configuration group based on alias, e.g. "Physics"
	ConfigurationGroupKey saveNewConfigurationGroup(const std::string& groupName, std::map<std::string, ConfigurationVersion>& groupMembers, const std::string& groupComment = ViewColumnInfo::DATATYPE_COMMENT_DEFAULT, std::map<std::string /*table*/, std::string /*alias*/>* groupAliases = 0);

	//==============================================================================
	//public group cache handling
	const GroupInfo&                        getGroupInfo(const std::string& groupName);
	const std::map<std::string, GroupInfo>& getAllGroupInfo() { return allGroupInfo_; }

	void testXDAQContext();  //for debugging

  private:
	//==============================================================================
	//group cache handling
	void cacheGroupKey(const std::string& groupName, ConfigurationGroupKey key);

	//==============================================================================
	//private members
	std::map<std::string, ConfigurationInfo> allConfigurationInfo_;
	std::map<std::string, GroupInfo>         allGroupInfo_;
};

/////
struct TableEditStruct
{
	//everything needed for editing a table
	ConfigurationBase*   config_;
	ConfigurationView*   cfgView_;
	ConfigurationVersion temporaryVersion_, originalVersion_;
	bool                 createdTemporaryVersion_;  //indicates if temp version was created here
	bool                 modified_;                 //indicates if temp version was modified
	std::string          configName_;
	/////
	TableEditStruct()
	{
		__SS__ << "impossible!" << std::endl;
		__SS_THROW__;
	}
	TableEditStruct(const std::string& configName, ConfigurationManagerRW* cfgMgr)
	    : createdTemporaryVersion_(false)
	    , modified_(false)
	    , configName_(configName)
	{
		__COUT__ << "Creating Table-Edit Struct for " << configName_ << std::endl;
		config_ = cfgMgr->getConfigurationByName(configName_);

		if (!(originalVersion_ =
		          config_->getView().getVersion())
		         .isTemporaryVersion())
		{
			__COUT__ << "Start version " << originalVersion_ << std::endl;
			//create temporary version for editing
			temporaryVersion_ = config_->createTemporaryView(originalVersion_);
			cfgMgr->saveNewConfiguration(
			    configName_,
			    temporaryVersion_,
			    true);  //proper bookkeeping for temporary version with the new version

			__COUT__ << "Created temporary version " << temporaryVersion_ << std::endl;
			createdTemporaryVersion_ = true;
		}
		else  //else table is already temporary version
			__COUT__ << "Using temporary version " << temporaryVersion_ << std::endl;

		cfgView_ = config_->getViewP();
	}
};  //end TableEditStruct declaration

}  // namespace ots

#endif
