#ifndef _ots_ConfigurationManager_h_
#define _ots_ConfigurationManager_h_

#include <string>
#include <map>
#include <set>
#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationTree.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationVersion.h"

#include <memory>

#include <cxxabi.h> //for getting type name using typeid and GCC



namespace ots {

///////////////////////////////////////////////////
//class RegisterBase;
class DACStream;
//class DACsConfigurationBase;
class ProgressBar;
///////////////////////////////////////////////////

#define __GET_CONFIG__(X)  getConfiguration<X>(QUOTE(X))

class ConfigurationManager
{
	//ConfigurationManagerRW is a "Friend" class of ConfigurationManager so has access to private members.
	friend class ConfigurationManagerRW;

public:

	static const std::string READONLY_USER;
	static const std::string ACTIVE_GROUP_FILENAME;
	static const std::string ALIAS_VERSION_PREAMBLE;
	static const std::string SCRATCH_VERSION_ALIAS;
	static const std::string XDAQ_CONTEXT_CONFIG_NAME;

	static const std::string ACTIVE_GROUP_NAME_CONTEXT;
	static const std::string ACTIVE_GROUP_NAME_BACKBONE;
	static const std::string ACTIVE_GROUP_NAME_CONFIGURATION;

	enum {
		CONTEXT_TYPE,
		BACKBONE_TYPE,
		CONFIGURATION_TYPE
	};

	ConfigurationManager			();
	virtual ~ConfigurationManager	(void);

	void init                		(std::string *accumulatedErrors = 0);
	void destroy             		(void);
	void destroyConfigurationGroup	(const std::string &theGroup = "", bool onlyDeactivate = false);

	//==============================================================================
	//getConfiguration
	//	get configuration * with specific configuration type
	template<class T>
	const T* getConfiguration(std::string name)
	{return (T*) (getConfigurationByName(name));}
	//==============================================================================



	//FIXME.. don' use activateConfigurationGroupKey.. instead "Activate" a global config
	//void activateConfigurationGroupKey  	(std::shared_ptr<const ConfigurationGroupKey> ConfigurationGroupKey, unsigned int supervisorInstance);


	void																  dumpActiveConfiguration		  (const std::string &filePath, const std::string &dumpType) const;

	//   map<name       , ConfigurationVersion >
	std::map<std::string, ConfigurationVersion >                          loadConfigurationGroup		  (const std::string &configGroupName, ConfigurationGroupKey configGroupKey, bool doActivate=false, ProgressBar* progressBar=0, std::string *accumulateWarnings=0, std::string *groupComment=0, std::string	*groupAuthor=0,	std::string *groupCreateTime=0, bool doNotLoadMember=false);
	void										                          loadMemberMap					  (const std::map<std::string /*name*/, ConfigurationVersion /*version*/> &memberMap);
	ConfigurationGroupKey							                      loadConfigurationBackbone       (void);
	void											                      restoreActiveConfigurationGroups(bool throwErrors=false);
	int																	  getTypeOfGroup				  (const std::string &configGroupName, ConfigurationGroupKey configGroupKey, const std::map<std::string /*name*/, ConfigurationVersion /*version*/> &memberMap);
	const std::string&													  convertGroupTypeIdToName		  (int groupTypeId);

	//==============================================================================
	//Getters
	const ConfigurationBase*                      					      getConfigurationByName        (const std::string &configurationName) const;

	//   map<type,        pair     <groupName  , ConfigurationGroupKey>>
	std::map<std::string, std::pair<std::string, ConfigurationGroupKey>>  getActiveConfigurationGroups  (void) const;

	ConfigurationTree 							   	                      getNode					    (const std::string &nodeString) const;	//"root/parent/parent/"
	ConfigurationTree													  getSupervisorConfigurationNode(const std::string &contextUID, const std::string &applicationUID) const;
	//   map<name       , version
	std::vector<std::pair<std::string,ConfigurationTree> >                getChildren				    (std::map<std::string, ConfigurationVersion> *memberMap = 0, std::string *accumulatedTreeErrors = 0) const;
	std::string															  getFirstPathToNode			(const ConfigurationTree &node, const std::string &startPath = "/") const;


	//   map<alias      ,      pair<group name,  ConfigurationGroupKey> >
	std::map<std::string, std::pair<std::string, ConfigurationGroupKey> > getGroupAliasesConfiguration  (void);
	//   pair<group name , ConfigurationGroupKey>
	std::pair<std::string, ConfigurationGroupKey>                         getConfigurationGroupFromAlias(std::string runType, ProgressBar* progressBar=0);

	std::map<std::string, ConfigurationVersion> 		  			      getActiveVersions		  		(void) const;

	const std::set<std::string>&					                      getContextMemberNames		  	(void) const {return contextMemberNames_;}
	const std::set<std::string>&					                      getBackboneMemberNames		(void) const {return backboneMemberNames_;}

	static std::string													  encodeURIComponent(const std::string &sourceStr);
	//const DACStream&                               	getDACStream                   	(std::string fecName);
	//ConfigurationVersion                           	getConfigurationBackboneVersion	(void);

	//==============================================================================
	//Setters/Modifiers
	std::shared_ptr<ConfigurationGroupKey> 			makeTheConfigurationGroupKey(ConfigurationGroupKey key);

private:
	ConfigurationManager							(const std::string& userName); //private constructor called by ConfigurationManagerRW

	std::string								 		username_;	//user of the configuration is READONLY_USER unless using ConfigurationManagerRW
	ConfigurationInterface*                       	theInterface_;
	std::shared_ptr<ConfigurationGroupKey>          theConfigurationGroupKey_,	theContextGroupKey_, 	theBackboneGroupKey_;
	std::string										theConfigurationGroup_, 	theContextGroup_, 		theBackboneGroup_;

	std::map<std::string, ConfigurationBase* >     	nameToConfigurationMap_;

	const std::set<std::string>                   	contextMemberNames_;  //list of context members
	const std::set<std::string>	                   	backboneMemberNames_;  //list of backbone members

	ConfigurationBase								groupMetadataTable_; //special table - version saved each time a group is created

	//std::map<std::string, DACStream>              	theDACStreams_;
	//	std::map<std::string, DACsConfigurationBase*> 	theDACsConfigurations_;
	//	RegisterBase*                                 	registerBase_;

};
}

#endif
