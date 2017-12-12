#ifndef _ots_ConfigurationManagerRW_h_
#define _ots_ConfigurationManagerRW_h_

//#include <string>
//#include <map>
//#include <set>
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

//Configuration Types

//#include <memory>

namespace ots {


struct ConfigurationInfo {
	ConfigurationInfo() :  //constructor
		configurationPtr_(0)
	{}

	std::set<ConfigurationVersion>      		versions_;
	ConfigurationBase* 					configurationPtr_;
};


#define Q(X) #X
#define QUOTE(X) Q(X)
#define __GETCFG_RW__(X)  getConfigurationRW<X>(QUOTE(X))

//==============================================================================
//ConfigurationManagerRW
//	This is the ConfigurationManger with write access
//	This class inherits all public function from ConfigurationManager
// and is a "Friend" class of ConfigurationManager so has access to private members.
class ConfigurationManagerRW : public ConfigurationManager
{
public:

	ConfigurationManagerRW	(std::string username);

	//==============================================================================
	//Getters
	const std::string&									getUsername								(void) const { return username_; }
	ConfigurationInterface* 							getConfigurationInterface				(void) const { return theInterface_; }

	const std::map<std::string, ConfigurationInfo>& 	getAllConfigurationInfo					(bool refresh=false, std::string *accumulatedErrors=0, const std::string &errorFilterName="");
	/* map < configName, map < aliasName, version > > */
	std::map<std::string,std::map<std::string,ConfigurationVersion> > getActiveVersionAliases	(void) const;

	template<class T>
	T* getConfigurationRW(std::string name)
	{
		return (T*)getConfigurationByName(name);
	}
	ConfigurationBase*                          		getVersionedConfigurationByName			(const std::string &configurationName, ConfigurationVersion version, bool looseColumnMatching=false);
	ConfigurationBase*                          		getConfigurationByName		  			(const std::string &configurationName);
	ConfigurationGroupKey 								findConfigurationGroup					(const std::string &groupName, const std::map<std::string, ConfigurationVersion> &groupMembers);

	//==============================================================================
	//modifiers of generic ConfigurationBase

	ConfigurationVersion								saveNewConfiguration					(const std::string &configurationName, ConfigurationVersion temporaryVersion = ConfigurationVersion(), bool makeTemporary = false);//, bool saveToScratchVersion = false);
	ConfigurationVersion								copyViewToCurrentColumns				(const std::string &configurationName, ConfigurationVersion sourceVersion);
	void												eraseTemporaryVersion					(const std::string &configurationName, ConfigurationVersion targetVersion = ConfigurationVersion());
	void												clearCachedVersions						(const std::string &configurationName);
	void												clearAllCachedVersions					();

	//==============================================================================
	//modifiers of configuration groups

	void 												activateConfigurationGroup				(const std::string &configGroupName, ConfigurationGroupKey configGroupKey, std::string *accumulatedTreeErrors=0);

	ConfigurationVersion								createTemporaryBackboneView				(ConfigurationVersion sourceViewVersion = ConfigurationVersion()); //-1, from MockUp, else from valid backbone view version
	ConfigurationVersion								saveNewBackbone							(ConfigurationVersion temporaryVersion = ConfigurationVersion());



	//==============================================================================
	//modifiers of a configuration group based on alias, e.g. "Physics"
	ConfigurationGroupKey								saveNewConfigurationGroup				(const std::string &groupName, std::map<std::string, ConfigurationVersion> &groupMembers, const std::string &groupComment = ViewColumnInfo::DATATYPE_COMMENT_DEFAULT);

	void testXDAQContext(); //for debugging

private:
	//==============================================================================
	//private members
	std::map<std::string, ConfigurationInfo> 	allConfigurationInfo_;

};


/////
struct TableEditStruct {
	//everything needed for editing a table
	ConfigurationBase* config_;
	ConfigurationView* cfgView_;
	ConfigurationVersion temporaryVersion_, originalVersion_;
	bool createdTemporaryVersion_; //indicates if temp version was created here
	bool modified_; //indicates if temp version was modified
	std::string configName_;
	/////
	TableEditStruct(){ __SS__ << "impossible!" << std::endl; throw std::runtime_error(ss.str());}
	TableEditStruct(const std::string& configName, ConfigurationManagerRW* cfgMgr)
	:createdTemporaryVersion_(false)
	,modified_(false)
	,configName_(configName)
	{
		__COUT__ << "Creating Table-Edit Struct for " << configName_ << std::endl;
		config_ = cfgMgr->getConfigurationByName(configName_);

		if(!(originalVersion_ =
				config_->getView().getVersion()).isTemporaryVersion())
		{
			__COUT__ << "Start version " << originalVersion_ << std::endl;
			//create temporary version for editing
			temporaryVersion_ = config_->createTemporaryView(originalVersion_);
			cfgMgr->saveNewConfiguration(
					configName_,
					temporaryVersion_, true); //proper bookkeeping for temporary version with the new version

			__COUT__ << "Created temporary version " << temporaryVersion_ << std::endl;
			createdTemporaryVersion_ = true;
		}
		else //else table is already temporary version
			__COUT__ << "Using temporary version " << temporaryVersion_ << std::endl;

		cfgView_ = config_->getViewP();
	}
}; //end TableEditStruct declaration

} //end namespace

#endif
