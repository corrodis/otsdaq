#include "otsdaq-core/ConfigurationInterface/ConfigurationManagerRW.h"

//backbone includes
//#include "otsdaq-core/ConfigurationPluginDataFormats/ConfigurationAliases.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/Configurations.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/DefaultConfigurations.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/VersionAliases.h"

//#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"//All configurable objects are included here


//
//#include "otsdaq-core/ConfigurationPluginDataFormats/DetectorConfiguration.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/MaskConfiguration.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/DetectorToFEConfiguration.h"
//

//#include "otsdaq-core/ConfigurationInterface/DACStream.h"
//#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
//
//#include "otsdaq-core/ConfigurationInterface/FileConfigurationInterface.h"
//
//#include <cassert>

#include<dirent.h>

using namespace ots;


#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "ConfigurationManagerRW"

#define CONFIGURATION_INFO_PATH		std::string(getenv("CONFIGURATION_INFO_PATH")) + "/"
#define CONFIGURATION_INFO_EXT		"Info.xml"

//==============================================================================
//ConfigurationManagerRW
ConfigurationManagerRW::ConfigurationManagerRW(std::string username)
: ConfigurationManager(username) //for use as author of new views
{
	__COUT__ << "Using Config Mgr with Write Access! (for " << username << ")" << std::endl;


	//FIXME only necessarily temporarily while Lore is still using fileSystem xml
	theInterface_ = ConfigurationInterface::getInstance(false); //false to use artdaq DB
	// FIXME -- can delete this change of interface once RW and regular use same interface instance
}

//==============================================================================
//getAllConfigurationInfo()
//	Used by ConfigurationGUISupervisor to get all the info for the existing configurations
//
//if(accumulatedErrors)
//	this implies allowing column errors and accumulating such errors in given string
const std::map<std::string, ConfigurationInfo>& ConfigurationManagerRW::getAllConfigurationInfo(
		bool refresh, std::string *accumulatedErrors, const std::string &errorFilterName)
{
	//allConfigurationInfo_ is container to be returned

	if(accumulatedErrors) *accumulatedErrors = "";

	if(!refresh) return allConfigurationInfo_;

	//else refresh!
	allConfigurationInfo_.clear();
	allGroupInfo_.clear();

	ConfigurationBase* configuration;

	//existing configurations are defined by which infos are in CONFIGURATION_INFO_PATH
	//can test that the class exists based on this
	//and then which versions
	__COUT__ << "======================================================== getAllConfigurationInfo start" << std::endl;
	__COUT__ << "Refreshing all! Extracting list of Configuration tables..." << std::endl;
	DIR *pDIR;
	struct dirent *entry;
	std::string path = CONFIGURATION_INFO_PATH;
	char fileExt[] = CONFIGURATION_INFO_EXT;
	const unsigned char MIN_CONFIG_NAME_SZ = 3;
	if( (pDIR=opendir( path.c_str() )) != 0 )
	{
		while((entry = readdir(pDIR)) != 0)
		{
			//enforce configuration name length
			if(strlen(entry->d_name) < strlen(fileExt)+MIN_CONFIG_NAME_SZ)
				continue;

			//find file names with correct file extenstion
			if( strcmp(
					&(entry->d_name[strlen(entry->d_name) - strlen(fileExt)]),
					fileExt) != 0)
				continue; //skip different extentions


			entry->d_name[strlen(entry->d_name) - strlen(fileExt)] = '\0'; //remove file extension to get config name

			//__COUT__ << entry->d_name << std::endl;

			//0 will force the creation of new instance (and reload from Info)
			configuration = 0;

			try //only add valid configuration instances to maps
			{
				theInterface_->get(configuration,
						entry->d_name,
						0, 0,
						true); //dont fill
			}
			catch(cet::exception)
			{
				if(configuration) delete configuration;
				configuration = 0;

				__COUT__ << "Skipping! No valid class found for... " << entry->d_name << "\n";
				continue;
			}
			catch(std::runtime_error &e)
			{
				if(configuration) delete configuration;
				configuration = 0;

				__COUT__ << "Skipping! No valid class found for... " << entry->d_name << "\n";
				__COUT__ << "Error: " << e.what() << std::endl;


				//for a runtime_error, it is likely that columns are the problem
				//	the Table Editor needs to still fix these.. so attempt to
				// 	proceed.
				if(accumulatedErrors)
				{

					if(errorFilterName == "" ||
							errorFilterName == entry->d_name)
					{
						*accumulatedErrors += std::string("\nIn table '") + entry->d_name +
							"'..." + e.what(); //global accumulate

						__SS__ << "Attempting to allow illegal columns!" << std::endl;
						*accumulatedErrors += ss.str();
					}

					//attempt to recover and build a mock-up
					__COUT__ << "Attempting to allow illegal columns!" << std::endl;


					std::string returnedAccumulatedErrors;
					try
					{
						//configuration = new ConfigurationBase(entry->d_name, &returnedAccumulatedErrors);
						configuration = new ConfigurationBase(entry->d_name, &returnedAccumulatedErrors);
					}
					catch(...)
					{
						__COUT__ << "Skipping! Allowing illegal columns didn't work either... " <<
								entry->d_name << "\n";
						continue;
					}
					__COUT__ << "Error (but allowed): " << returnedAccumulatedErrors << std::endl;

					if(errorFilterName == "" ||
							errorFilterName == entry->d_name)
						*accumulatedErrors += std::string("\nIn table '") + entry->d_name +
						"'..." + returnedAccumulatedErrors; //global accumulate
				}
				else
					continue;
			}

			//__COUT__ << "Instance created: " << entry->d_name << "\n"; //found!

			if(nameToConfigurationMap_[entry->d_name]) //handle if instance existed
			{
				//copy the temporary versions! (or else all is lost)
				std::set<ConfigurationVersion> versions =
						nameToConfigurationMap_[entry->d_name]->getStoredVersions();
				for(auto &version:versions)
					if(version.isTemporaryVersion())
					{
						//__COUT__ << "copying tmp = " << version << std::endl;

						try //do NOT let ConfigurationView::init() throw here
						{
							nameToConfigurationMap_[entry->d_name]->setActiveView(version);
							configuration->copyView(	//this calls ConfigurationView::init()
									nameToConfigurationMap_[entry->d_name]->getView(),
									version,
									username_);
						}
						catch(...) //do NOT let invalid temporary version throw at this point
						{}	//just trust configurationBase throws out the failed version
					}
				//__COUT__ << "deleting: " << entry->d_name << "\n"; //found!
				delete nameToConfigurationMap_[entry->d_name];
				nameToConfigurationMap_[entry->d_name] = 0;
			}

			nameToConfigurationMap_[entry->d_name] = configuration;

			allConfigurationInfo_[entry->d_name].configurationPtr_ 	= configuration;
			allConfigurationInfo_[entry->d_name].versions_ 			= theInterface_->getVersions(configuration);

			//also add any existing temporary versions to all config info
			//	because the interface wont find those versions
			std::set<ConfigurationVersion> versions =
					nameToConfigurationMap_[entry->d_name]->getStoredVersions();
			for(auto &version:versions)
				if(version.isTemporaryVersion())
				{
					//__COUT__ << "surviving tmp = " << version << std::endl;
					allConfigurationInfo_[entry->d_name].versions_.emplace(version);
				}
		}
		closedir(pDIR);
	}
	__COUT__ << "Extracting list of Configuration tables complete" << std::endl;

	//call init to load active versions by default
	init(accumulatedErrors);

	__COUT__ << "======================================================== getAllConfigurationInfo end" << std::endl;


	//get Group Info too!
	try
	{
		//build allGroupInfo_ for the ConfigurationManagerRW

		std::set<std::string /*name*/>  configGroups = theInterface_->getAllConfigurationGroupNames();
		__COUT__ << "Number of Groups: " << configGroups.size() << std::endl;

		ConfigurationGroupKey key;
		std::string name;
		for(const auto& fullName:configGroups)
		{
			ConfigurationGroupKey::getGroupNameAndKey(fullName,name,key);
			cacheGroupKey(name,key);
		}

		//for each group get member map & comment, author, time, and type for latest key
		for(auto& groupInfo:allGroupInfo_)
		{
			try
			{
				loadConfigurationGroup(
						groupInfo.first /*groupName*/,
						groupInfo.second.getLatestKey(),
						false /*doActivate*/,
						&groupInfo.second.latestKeyMemberMap_ /*groupMembers*/,
						0 /*progressBar*/,0 /*accumulateErrors*/,
						&groupInfo.second.latestKeyGroupComment_,
						&groupInfo.second.latestKeyGroupAuthor_,
						&groupInfo.second.latestKeyGroupCreationTime_,
						true /*doNotLoadMember*/,
						&groupInfo.second.latestKeyGroupTypeString_);
			}
			catch(...)
			{
				__COUT_WARN__ << "Error occurred loading latest group info into cache for '" <<
						groupInfo.first << "'..." << __E__;
				groupInfo.second.latestKeyGroupComment_ = "UNKNOWN";
				groupInfo.second.latestKeyGroupAuthor_ = "UNKNOWN";
				groupInfo.second.latestKeyGroupCreationTime_ = "0";
				groupInfo.second.latestKeyGroupTypeString_ = "UNKNOWN";
			}
		} //end group info loop
	} //end get group info
	catch(const std::runtime_error& e)
	{
		__SS__ << "A fatal error occurred reading the info for all configuration groups. Error: " <<
				e.what() << __E__;
		__COUT_ERR__ << "\n" << ss.str();
		if(accumulatedErrors) *accumulatedErrors += ss.str();
		else throw;
	}
	catch(...)
	{
		__SS__ << "An unknown fatal error occurred reading the info for all configuration groups." << __E__;
		__COUT_ERR__ << "\n" << ss.str();
		if(accumulatedErrors) *accumulatedErrors += ss.str();
		else throw;
	}

	return allConfigurationInfo_;
} //end getAllConfigurationInfo

//==============================================================================
//getVersionAliases()
//	get version aliases organized by table, for currently active backbone tables
//	add scratch versions to the alias map returned by ConfigurationManager
std::map<std::string /*table name*/,std::map<
std::string /*version alias*/,ConfigurationVersion /*aliased version*/> >
ConfigurationManagerRW::getVersionAliases(void) const
{
	//__COUT__ << "getVersionAliases()" << std::endl;
	std::map<std::string /*table name*/,
	std::map<std::string /*version alias*/,ConfigurationVersion /*aliased version*/> > retMap =
			ConfigurationManager::getVersionAliases();

	//always have scratch alias for each table that has a scratch version
	//	overwrite map entry if necessary
	if(!ConfigurationInterface::isVersionTrackingEnabled())
		for(const auto &tableInfo:allConfigurationInfo_)
			for(const auto &version:tableInfo.second.versions_)
				if(version.isScratchVersion())
					retMap[tableInfo.first][ConfigurationManager::SCRATCH_VERSION_ALIAS] =
							ConfigurationVersion(ConfigurationVersion::SCRATCH);

	return retMap;
} //end getVersionAliases()

//==============================================================================
//setActiveGlobalConfiguration
//	load config group and activate
//	deactivates previous config group of same type if necessary
void ConfigurationManagerRW::activateConfigurationGroup(const std::string &configGroupName,
		ConfigurationGroupKey configGroupKey, std::string *accumulatedTreeErrors)
{
	loadConfigurationGroup(configGroupName,configGroupKey,
			true, 	//loads and activates
			0,		//no members needed
			0,		//no progress bar
			accumulatedTreeErrors); //accumulate warnings or not

	if(accumulatedTreeErrors &&
			*accumulatedTreeErrors != "")
	{
		__COUT_ERR__ << "Errors were accumulated so de-activating group: " <<
				configGroupName << " (" << configGroupKey << ")" << std::endl;
		try //just in case any lingering pieces, lets deactivate
		{ destroyConfigurationGroup(configGroupName,true); }
		catch(...){}
	}

	__COUT_INFO__ << "Updating persistent active groups to " <<
			ConfigurationManager::ACTIVE_GROUP_FILENAME << " ..."  << std::endl;
	__MOUT_INFO__ << "Updating persistent active groups to " <<
			ConfigurationManager::ACTIVE_GROUP_FILENAME << " ..."  << std::endl;

	std::string fn = ConfigurationManager::ACTIVE_GROUP_FILENAME;
	FILE *fp = fopen(fn.c_str(),"w");
	if(!fp)
	{
		__SS__ << "Fatal Error! Unable to open the file " <<
				ConfigurationManager::ACTIVE_GROUP_FILENAME << " for editing! Is there a permissions problem?" << std::endl;
		__COUT_ERR__ << ss.str();
		__SS_THROW__;
		return;
	}

	__COUT_INFO__ << "Active Context: " << theContextGroup_ << "(" <<
			(theContextGroupKey_?theContextGroupKey_->toString().c_str():"-1") << ")" << std::endl;
	__COUT_INFO__ << "Active Backbone: " << theBackboneGroup_ << "(" <<
			(theBackboneGroupKey_?theBackboneGroupKey_->toString().c_str():"-1") << ")" << std::endl;
	__COUT_INFO__ << "Active Iterate: " << theIterateGroup_ << "(" <<
			(theIterateGroupKey_?theIterateGroupKey_->toString().c_str():"-1") << ")" << std::endl;
	__COUT_INFO__ << "Active Configuration: " << theConfigurationGroup_ << "(" <<
			(theConfigurationGroupKey_?theConfigurationGroupKey_->toString().c_str():"-1") << ")" << std::endl;

	__MOUT_INFO__ << "Active Context: " << theContextGroup_ << "(" <<
			(theContextGroupKey_?theContextGroupKey_->toString().c_str():"-1") << ")" << std::endl;
	__MOUT_INFO__ << "Active Backbone: " << theBackboneGroup_ << "(" <<
			(theBackboneGroupKey_?theBackboneGroupKey_->toString().c_str():"-1") << ")" << std::endl;
	__MOUT_INFO__ << "Active Iterate: " << theIterateGroup_ << "(" <<
			(theIterateGroupKey_?theIterateGroupKey_->toString().c_str():"-1") << ")" << std::endl;
	__MOUT_INFO__ << "Active Configuration: " << theConfigurationGroup_ << "(" <<
			(theConfigurationGroupKey_?theConfigurationGroupKey_->toString().c_str():"-1") << ")" << std::endl;

	fprintf(fp,"%s\n",theContextGroup_.c_str());
	fprintf(fp,"%s\n",theContextGroupKey_?theContextGroupKey_->toString().c_str():"-1");
	fprintf(fp,"%s\n",theBackboneGroup_.c_str());
	fprintf(fp,"%s\n",theBackboneGroupKey_?theBackboneGroupKey_->toString().c_str():"-1");
	fprintf(fp,"%s\n",theIterateGroup_.c_str());
	fprintf(fp,"%s\n",theIterateGroupKey_?theIterateGroupKey_->toString().c_str():"-1");
	fprintf(fp,"%s\n",theConfigurationGroup_.c_str());
	fprintf(fp,"%s\n",theConfigurationGroupKey_?theConfigurationGroupKey_->toString().c_str():"-1");
	fclose(fp);
}

//==============================================================================
//createTemporaryBackboneView
//	sourceViewVersion of INVALID is from MockUp, else from valid view version
// 	returns temporary version number (which is always negative)
ConfigurationVersion ConfigurationManagerRW::createTemporaryBackboneView(ConfigurationVersion sourceViewVersion)
{
	__COUT_INFO__ << "Creating temporary backbone view from version " <<
			sourceViewVersion << std::endl;

	//find common available temporary version among backbone members
	ConfigurationVersion tmpVersion = ConfigurationVersion::getNextTemporaryVersion(); //get the default temporary version
	ConfigurationVersion retTmpVersion;
	auto backboneMemberNames = ConfigurationManager::getBackboneMemberNames();
	for (auto& name : backboneMemberNames)
	{
		retTmpVersion = ConfigurationManager::getConfigurationByName(name)->getNextTemporaryVersion();
		if(retTmpVersion < tmpVersion)
			tmpVersion = retTmpVersion;
	}

	__COUT__ << "Common temporary backbone version found as " <<
			tmpVersion << std::endl;

	//create temporary views from source version to destination temporary version
	for (auto& name : backboneMemberNames)
	{
		retTmpVersion = getConfigurationByName(name)->createTemporaryView(sourceViewVersion, tmpVersion);
		if(retTmpVersion != tmpVersion)
		{
			__SS__ << "Failure! Temporary view requested was " <<
					tmpVersion << ". Mismatched temporary view created: " << retTmpVersion << std::endl;
			__COUT_ERR__ << ss.str();
			__SS_THROW__;
		}
	}

	return tmpVersion;
}



//==============================================================================
ConfigurationBase* ConfigurationManagerRW::getConfigurationByName(const std::string &configurationName)
{
	if(nameToConfigurationMap_.find(configurationName) == nameToConfigurationMap_.end())
	{
		__SS__ << "Configuration not found with name: " << configurationName << std::endl;
		size_t f;
		if((f=configurationName.find(' ')) != std::string::npos)
			ss << "There was a space character found in the configuration name needle at position " <<
				f << " in the string (was this intended?). " << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}
	return nameToConfigurationMap_[configurationName];
}

//==============================================================================
//getVersionedConfigurationByName
//	Used by configuration GUI to load a particular configuration-version pair as the active version.
// 	This configuration instance must already exist and be owned by ConfigurationManager.
//	return null pointer on failure, on success return configuration pointer.
ConfigurationBase* ConfigurationManagerRW::getVersionedConfigurationByName(const std::string &configurationName,
		ConfigurationVersion version, bool looseColumnMatching)
{
	auto it = nameToConfigurationMap_.find(configurationName);
	if(it == nameToConfigurationMap_.end())
	{
		__SS__ << "\nCan not find configuration named '" <<
				configurationName <<
				"'\n\n\n\nYou need to load the configuration before it can be used." <<
				"It probably is missing from the member list of the Configuration Group that was loaded?\n\n\n\n\n"
				<< std::endl;
		__SS_THROW__;
	}
	ConfigurationBase* configuration = it->second;
	theInterface_->get(configuration, configurationName, 0 , 0,
			false, //fill w/version
			version,
			false, //do not reset
			looseColumnMatching);
	return configuration;
}

//==============================================================================
//saveNewConfiguration
//	saves version, makes the new version the active version, and returns new version
ConfigurationVersion ConfigurationManagerRW::saveNewConfiguration(
		const std::string &configurationName,
		ConfigurationVersion temporaryVersion,
		bool makeTemporary)//,
		//bool saveToScratchVersion)
{
	ConfigurationVersion newVersion(temporaryVersion);

	//set author of version
	ConfigurationBase *config = getConfigurationByName(configurationName);
	config->getTemporaryView(temporaryVersion)->setAuthor(username_);
	//NOTE: somehow? author is assigned to permanent versions when saved to DBI?

	if(!makeTemporary) //saveNewVersion makes the new version the active version
		newVersion = theInterface_->saveNewVersion(config, temporaryVersion);
	else	//make the temporary version active
		config->setActiveView(newVersion);

	//if there is a problem, try to recover
	while(!makeTemporary && !newVersion.isScratchVersion() &&
			allConfigurationInfo_[configurationName].versions_.find(newVersion) !=
			allConfigurationInfo_[configurationName].versions_.end())
	{
		__COUT_ERR__ << "What happenened!?? ERROR::: new persistent version v" << newVersion <<
				" already exists!? How is it possible? Retrace your steps and tell an admin." << std::endl;

		//create a new temporary version of the target view
		temporaryVersion = config->createTemporaryView(newVersion);

		if(newVersion.isTemporaryVersion())
			newVersion = temporaryVersion;
		else
			newVersion = ConfigurationVersion::getNextVersion(newVersion);

		__COUT_WARN__ << "Attempting to recover and use v" << newVersion << std::endl;


		if(!makeTemporary) //saveNewVersion makes the new version the active version
			newVersion = theInterface_->saveNewVersion(config, temporaryVersion, newVersion);
		else	//make the temporary version active
			config->setActiveView(newVersion);
	}

	if(newVersion.isInvalid())
	{
		__SS__ << "Something went wrong saving the new version v" << newVersion <<
				". What happened?! (duplicates? database error?)" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}

	//update allConfigurationInfo_ with the new version
	allConfigurationInfo_[configurationName].versions_.insert(newVersion);

	__COUT__ << "New version added to info " << newVersion << std::endl;

	return newVersion;
}

//==============================================================================
//eraseTemporaryVersion
//	if version is invalid then erases ALL temporary versions
//
//	maintains allConfigurationInfo_ also while erasing
void ConfigurationManagerRW::eraseTemporaryVersion(const std::string &configurationName,
		ConfigurationVersion targetVersion)
{
	ConfigurationBase* config = getConfigurationByName(configurationName);

	config->trimTemporary(targetVersion);

	//if allConfigurationInfo_ is not setup, then done
	if(allConfigurationInfo_.find(configurationName) ==
					allConfigurationInfo_.end()) return;
	//else cleanup config info

	if(targetVersion.isInvalid())
	{
		//erase all temporary versions!
		for(auto it = allConfigurationInfo_[configurationName].versions_.begin();
				it != allConfigurationInfo_[configurationName].versions_.end(); /*no increment*/)
		{
			if(it->isTemporaryVersion())
			{
				__COUT__ << "Removing version info: " << *it << std::endl;
				allConfigurationInfo_[configurationName].versions_.erase(it++);
			}
			else
				++it;
		}
	}
	else //erase target version only
	{
		__COUT__ << "Removing version info: " << targetVersion << std::endl;
		auto it = allConfigurationInfo_[configurationName].versions_.find(targetVersion);
		if(it == allConfigurationInfo_[configurationName].versions_.end())
		{
			__COUT__ << "Target version was not found in info versions..." << std::endl;
			return;
		}
		allConfigurationInfo_[configurationName].versions_.erase(
				allConfigurationInfo_[configurationName].versions_.find(targetVersion));
		__COUT__ << "Target version was erased from info." << std::endl;
	}
}

//==============================================================================
//clearCachedVersions
//	clear ALL cached persistent versions (does not erase temporary versions)
//
//	maintains allConfigurationInfo_ also while erasing (trivial, do nothing)
void ConfigurationManagerRW::clearCachedVersions(const std::string &configurationName)
{
	ConfigurationBase* config = getConfigurationByName(configurationName);

	config->trimCache(0);
}

//==============================================================================
//clearAllCachedVersions
//	clear ALL cached persistent versions (does not erase temporary versions)
//
//	maintains allConfigurationInfo_ also while erasing (trivial, do nothing)
void ConfigurationManagerRW::clearAllCachedVersions()
{
	for(auto configInfo: allConfigurationInfo_)
		configInfo.second.configurationPtr_->trimCache(0);
}

//==============================================================================
//copyViewToCurrentColumns
ConfigurationVersion ConfigurationManagerRW::copyViewToCurrentColumns(const std::string &configurationName,
		ConfigurationVersion sourceVersion)
{
	getConfigurationByName(configurationName)->reset();

	//make sure source version is loaded
	//need to load with loose column rules!
	ConfigurationBase *config = getVersionedConfigurationByName(configurationName,
			ConfigurationVersion(sourceVersion), true);

	//copy from source version to a new temporary version
	ConfigurationVersion newTemporaryVersion = config->copyView(config->getView(),
			ConfigurationVersion(),username_);

	//update allConfigurationInfo_ with the new version
	allConfigurationInfo_[configurationName].versions_.insert(newTemporaryVersion);

	return newTemporaryVersion;
}

//==============================================================================
//cacheGroupKey
void ConfigurationManagerRW::cacheGroupKey(const std::string &groupName,
		ConfigurationGroupKey key)
{
	allGroupInfo_[groupName].keys_.emplace(key);
}

//==============================================================================
//getGroupInfo
//	the interface is slow when there are a lot of groups..
//	so plan is to maintain local cache of recent group info
const GroupInfo& ConfigurationManagerRW::getGroupInfo(const std::string &groupName)
{

	//	//NOTE: seems like this filter is taking the long amount of time
	//	std::set<std::string /*name*/> fullGroupNames =
	//			theInterface_->getAllConfigurationGroupNames(groupName); //db filter by group name

	//so instead caching ourselves...
	auto it = allGroupInfo_.find(groupName);
	if(it == allGroupInfo_.end())
	{
		__SS__ << "Group name '" << groupName << "' not found in group info! (creating empty info)" << __E__;
		__COUT_WARN__ << ss.str();
		//__SS_THROW__;
		return allGroupInfo_[groupName];
	}
	return it->second;
}

//==============================================================================
//findConfigurationGroup
//	return group with same name and same members and same aliases
//	else return invalid key
//
// Note: if aliases, then member alias is matched (not member
//
// Note: this is taking too long when there are a ton of groups.
//	Change to going back only a limited number.. (but the order also comes in alpha order from
//	theInterface_->getAllConfigurationGroupNames which is a problem for choosing the
//	most recent to check. )
ConfigurationGroupKey ConfigurationManagerRW::findConfigurationGroup(const std::string &groupName,
		const std::map<std::string, ConfigurationVersion> &groupMemberMap,
		const std::map<std::string /*name*/, std::string /*alias*/> &groupAliases)
{

	//	//NOTE: seems like this filter is taking the long amount of time
	//	std::set<std::string /*name*/> fullGroupNames =
	//			theInterface_->getAllConfigurationGroupNames(groupName); //db filter by group name
	const GroupInfo& groupInfo = getGroupInfo(groupName);

	//std::string name;
	//ConfigurationGroupKey key;
	std::map<std::string /*name*/, ConfigurationVersion /*version*/> compareToMemberMap;
	std::map<std::string /*name*/, std::string /*alias*/> 			 compareToGroupAliases;
	bool isDifferent;

	const unsigned int MAX_DEPTH_TO_CHECK = 20;
	unsigned int keyMinToCheck = 0;

	if(groupInfo.keys_.size())
		keyMinToCheck = groupInfo.keys_.rbegin()->key();
	if(keyMinToCheck > MAX_DEPTH_TO_CHECK)
	{
		keyMinToCheck -= MAX_DEPTH_TO_CHECK;
		__COUT__ << "Checking groups back to key... " << keyMinToCheck << std::endl;
	}
	else
	{
		keyMinToCheck = 0;
		__COUT__ << "Checking all groups." << std::endl;
	}



	//have min key to check, now loop through and check groups
	//std::string fullName;
	for(const auto& key: groupInfo.keys_)
	{
		//ConfigurationGroupKey::getGroupNameAndKey(fullName,name,key);

		if(key.key() < keyMinToCheck) continue; //skip keys that are too old

//		fullName = ConfigurationGroupKey::getFullGroupString(groupName,key);
//
//		__COUT__ << "checking group... " << fullName << std::endl;
//
//		compareToMemberMap = theInterface_->getConfigurationGroupMembers(fullName);


		loadConfigurationGroup(
				groupName,key,
				false /*doActivate*/,
				&compareToMemberMap/*memberMap*/,
				0,0,0,0,0, /*null pointers*/
				true /*doNotLoadMember*/,
				0 /*groupTypeString*/,
				&compareToGroupAliases
		);


		isDifferent = false;
		for(auto &memberPair: groupMemberMap)
		{
			//__COUT__ << memberPair.first << " - " << memberPair.second << std::endl;

			if(groupAliases.find(memberPair.first) != groupAliases.end())
			{
				//handle this table as alias, not version
				if(compareToGroupAliases.find(memberPair.first) == compareToGroupAliases.end() || //alias is missing
						groupAliases.at(memberPair.first) != compareToGroupAliases.at(memberPair.first))
				{ //then different
					//__COUT__ << "alias mismatch found!" << std::endl;
					isDifferent = true;
					break;
				}
				else
					continue;
			} //else check if compareTo group is using an alias for table
			else if(compareToGroupAliases.find(memberPair.first) != compareToGroupAliases.end())
			{
				//then different
				//__COUT__ << "alias mismatch found!" << std::endl;
				isDifferent = true;
				break;

			} //else handle as table version comparison
			else if(compareToMemberMap.find(memberPair.first) == compareToMemberMap.end() ||	//name is missing
					memberPair.second != compareToMemberMap.at(memberPair.first)) //or version mismatch
			{	//then different
				//__COUT__ << "mismatch found!" << std::endl;
				isDifferent = true;
				break;
			}
		}
		if(isDifferent) continue;

		//check member size for exact match
		if(groupMemberMap.size() != compareToMemberMap.size()) continue; //different size, so not same (groupMemberMap is a subset of memberPairs)

		__COUT__ << "Found exact match with key: " << key << std::endl;
		//else found an exact match!
		return key;
	}
	__COUT__ << "No match found - this group is new!" << std::endl;
	//if here, then no match found
	return ConfigurationGroupKey(); //return invalid key
}


//==============================================================================
//saveNewConfigurationGroup
//	saves new group and returns the new group key
//	if previousVersion is provided, attempts to just bump that version
//	else, bumps latest version found in db
//
//	Note: groupMembers map will get modified with group metadata table version
ConfigurationGroupKey ConfigurationManagerRW::saveNewConfigurationGroup(
		const std::string &groupName,
		std::map<std::string, ConfigurationVersion> &groupMembers,
		const std::string &groupComment,
		std::map<std::string /*table*/, std::string /*alias*/> *groupAliases)
{
	//steps:
	//	determine new group key
	//	verify group members
	//	verify groupNameWithKey
	//	verify store


	if(groupMembers.size() == 0) //do not allow empty groups
	{
		__SS__ << "Empty group member list. Can not create a group without members!" << std::endl;
		__SS_THROW__;
	}

	//determine new group key
	ConfigurationGroupKey newKey = ConfigurationGroupKey::getNextKey(
			theInterface_->findLatestGroupKey(groupName));

	__COUT__ << "New Key for group: " << groupName << " found as " << newKey << std::endl;

	//	verify group members
	//		- use all config info
	std::map<std::string, ConfigurationInfo> allCfgInfo = getAllConfigurationInfo();
	for(auto &memberPair : groupMembers )
	{
		//check member name
		if(allCfgInfo.find(memberPair.first) == allCfgInfo.end())
		{
			__COUT_ERR__ << "Group member \"" << memberPair.first << "\" not found in database!";

			if(groupMetadataTable_.getConfigurationName() ==
					memberPair.first)
			{
				__COUT_WARN__ << "Looks like this is the groupMetadataTable_ '" <<
						ConfigurationInterface::GROUP_METADATA_TABLE_NAME <<
						".' Note that this table is added to the member map when groups are saved." <<
						"It should not be part of member map when calling this function." << std::endl;
				__COUT__ << "Attempting to recover." << std::endl;
				groupMembers.erase(groupMembers.find(memberPair.first));
			}
			else
			{
				__SS__ << ("Group member not found!") << std::endl;
				__SS_THROW__;
			}
		}
		//check member version
		if(allCfgInfo[memberPair.first].versions_.find(memberPair.second) ==
				allCfgInfo[memberPair.first].versions_.end())
		{
			__SS__ << "Group member  \"" << memberPair.first << "\" version \"" <<
					memberPair.second << "\" not found in database!";
			__SS_THROW__;
		}
	} //end verify members

	//verify group aliases
	if(groupAliases)
	{
		for(auto &aliasPair : *groupAliases )
		{
			//check for alias table in member names
			if(groupMembers.find(aliasPair.first) == groupMembers.end())
			{
				__COUT_ERR__ << "Group member \"" << aliasPair.first << "\" not found in group member map!";

				__SS__ << ("Alias table not found in member list!") << std::endl;
				__SS_THROW__;
			}
		}
	} //end verify group aliases

	// verify groupNameWithKey and attempt to store
	try
	{
		//save meta data for group; reuse groupMetadataTable_
		std::string groupAliasesString = "";
		if(groupAliases)
			groupAliasesString = StringMacros::mapToString(*groupAliases,
					"," /*primary delimeter*/,":" /*secondary delimeter*/);
		__COUT__ << "Metadata: " << username_ << " " << time(0) << " " << groupComment <<
				" " << groupAliasesString << std::endl;

		//to compensate for unusual errors upstream, make sure the metadata table has one row
		while(groupMetadataTable_.getViewP()->getNumberOfRows() > 1)
			groupMetadataTable_.getViewP()->deleteRow(0);
		if(groupMetadataTable_.getViewP()->getNumberOfRows() == 0)
			groupMetadataTable_.getViewP()->addRow();

		//columns are uid,comment,author,time
		groupMetadataTable_.getViewP()->setValue(groupAliasesString,0,ConfigurationManager::METADATA_COL_ALIASES);
		groupMetadataTable_.getViewP()->setValue(groupComment,0,ConfigurationManager::METADATA_COL_COMMENT);
		groupMetadataTable_.getViewP()->setValue(username_,0,ConfigurationManager::METADATA_COL_AUTHOR);
		groupMetadataTable_.getViewP()->setValue(time(0),0,ConfigurationManager::METADATA_COL_TIMESTAMP);

		//set version to first available persistent version
		groupMetadataTable_.getViewP()->setVersion(
				ConfigurationVersion::getNextVersion(theInterface_->findLatestVersion(&groupMetadataTable_))
		);

		//groupMetadataTable_.print();

		theInterface_->saveActiveVersion(&groupMetadataTable_);

		//force groupMetadataTable_ to be a member for the group
		groupMembers[groupMetadataTable_.getConfigurationName()] =
				groupMetadataTable_.getViewVersion();

		theInterface_->saveConfigurationGroup(groupMembers,
				ConfigurationGroupKey::getFullGroupString(groupName,newKey));
		__COUT__ << "Created config group: " << groupName << ":" << newKey << std::endl;
	}
	catch(std::runtime_error &e)
	{
		__COUT_ERR__ << "Failed to create config group: " << groupName << ":" << newKey << std::endl;
		__COUT_ERR__ << "\n\n" << e.what() << std::endl;
		throw;
	}
	catch(...)
	{
		__COUT_ERR__ << "Failed to create config group: " << groupName << ":" << newKey << std::endl;
		throw;
	}


	//store cache of recent groups
	cacheGroupKey(groupName,newKey);

	// at this point succeeded!
	return newKey;
}

//==============================================================================
//saveNewBackbone
//	makes the new version the active version and returns new version number
//	INVALID will give a new backbone from mockup
ConfigurationVersion ConfigurationManagerRW::saveNewBackbone(ConfigurationVersion temporaryVersion)
{
	__COUT_INFO__ << "Creating new backbone from temporary version " <<
			temporaryVersion << std::endl;

	//find common available temporary version among backbone members
	ConfigurationVersion newVersion(ConfigurationVersion::DEFAULT);
	ConfigurationVersion retNewVersion;
	auto backboneMemberNames = ConfigurationManager::getBackboneMemberNames();
	for (auto& name : backboneMemberNames)
	{
		retNewVersion = ConfigurationManager::getConfigurationByName(name)->getNextVersion();
		__COUT__ << "New version for backbone member (" << name << "): " <<
				retNewVersion << std::endl;
		if(retNewVersion > newVersion)
			newVersion = retNewVersion;
	}

	__COUT__ << "Common new backbone version found as " <<
			newVersion << std::endl;

	//create new views from source temporary version
	for (auto& name : backboneMemberNames)
	{
		//saveNewVersion makes the new version the active version
		retNewVersion = getConfigurationInterface()->saveNewVersion(
				getConfigurationByName(name), temporaryVersion, newVersion);
		if(retNewVersion != newVersion)
		{
			__SS__ << "Failure! New view requested was " <<
					newVersion << ". Mismatched new view created: " << retNewVersion << std::endl;
			__COUT_ERR__ << ss.str();
			__SS_THROW__;
		}
	}

	return newVersion;
}

//==============================================================================
void ConfigurationManagerRW::testXDAQContext()
{

//	//test creating config group with author, create time, and comment
//	{
//		__COUT__ << std::endl;
//
//		std::string groupName = "testGroup";
//		ConfigurationGroupKey newKey;
//		//ConfigurationGroupMetadata
//
//		try
//		{
//			{
//				std::map<std::string, ConfigurationVersion> members;
//				members["ARTDAQAggregatorConfiguration"] = ConfigurationVersion(1);
//
//				//ConfigurationGroupKey
//				//	saveNewConfigurationGroup
//				//	(const std::string &groupName, const std::map<std::string,
//				//	ConfigurationVersion> &groupMembers, ConfigurationGroupKey previousVersion=ConfigurationGroupKey());
//				newKey =
//						saveNewConfigurationGroup(groupName,members);
//			}
//
//			//std::map<std::string, ConfigurationVersion >
//			//	loadConfigurationGroup
//			//	(const std::string &configGroupName,
//			//	ConfigurationGroupKey configGroupKey, bool doActivate=false, ProgressBar* progressBar=0, std::string *accumulateWarnings=0);
//			{
//				std::map<std::string, ConfigurationVersion > memberMap =
//						loadConfigurationGroup(groupName,newKey);
//				__COUT__ << "Group members:" << std::endl;
//				for(const auto &member: memberMap)
//					__COUT__ << member.first << " " << member.second << std::endl;
//			}
//
//
//
//			//do it again
//			{
//				std::map<std::string, ConfigurationVersion> members;
//				members["ARTDAQAggregatorConfiguration"] = ConfigurationVersion(1);
//
//				//ConfigurationGroupKey
//				//	saveNewConfigurationGroup
//				//	(const std::string &groupName, const std::map<std::string,
//				//	ConfigurationVersion> &groupMembers, ConfigurationGroupKey previousVersion=ConfigurationGroupKey());
//				newKey =
//						saveNewConfigurationGroup(groupName,members);
//			}
//
//			//std::map<std::string, ConfigurationVersion >
//			//	loadConfigurationGroup
//			//	(const std::string &configGroupName,
//			//	ConfigurationGroupKey configGroupKey, bool doActivate=false, ProgressBar* progressBar=0, std::string *accumulateWarnings=0);
//			{
//				std::map<std::string, ConfigurationVersion > memberMap =
//						loadConfigurationGroup(groupName,newKey);
//				__COUT__ << "Group members:" << std::endl;
//				for(const auto &member: memberMap)
//					__COUT__ << member.first << " " << member.second << std::endl;
//			}
//
//
//		}
//		catch(std::runtime_error &e)
//		{
//			__COUT_ERR__ << "Failed to create config group: " << groupName << ":" << newKey << std::endl;
//			__COUT_ERR__ << "\n\n" << e.what() << std::endl;
//		}
//		catch(...)
//		{
//			__COUT_ERR__ << "Failed to create config group: " << groupName << ":" << newKey << std::endl;
//		}
//
//		return;
//	}

//	//test creating config group and reading
//	{
//		__COUT__ << std::endl;
//
//		auto gcfgs = theInterface_->getAllConfigurationGroupNames();
//		__COUT__ << "Global config size: " << gcfgs.size() << std::endl;
//		for(auto &g:gcfgs)
//		{
//			__COUT__ << "Global config " << g << std::endl;
//			auto gcMap = theInterface_->getConfigurationGroupMembers(g, true /*getMetaTable*/);
//
//			for(auto &cv:gcMap)
//				__COUT__ << "\tMember config " << cv.first << ":" << cv.second << std::endl;
//		}
//
//		auto cfgs = theInterface_->getAllConfigurationNames();
//		__COUT__ << "Sub-config size: " << cfgs.size() << std::endl;
//		for(auto &c:cfgs)
//		{
//			__COUT__ << "config " << c << std::endl;
//			auto vs = theInterface_->getVersions(getConfigurationByName(c));
//			for(auto &v:vs)
//				__COUT__ << "\tversion " << v << std::endl;
//		}
//
//		if(0) //create a global config group (storeGlobalConfiguration)
//		{
//			//storeGlobalConfiguration with latest of each
//			std::map<std::string /*name*/, ConfigurationVersion /*version*/> gcMap;
//
//			for(auto &c:cfgs) //for each sub-config, take latest version
//			{
//				auto vs = theInterface_->getVersions(getConfigurationByName(c));
//				if(1 && vs.rbegin() != vs.rend()) //create latest!
//				{
//					gcMap[c] = *(vs.rbegin());
//					__COUT__ << "Adding config " << c << ":" << gcMap[c] << std::endl;
//				}
//				else if(vs.begin() != vs.end())	//create oldest!
//				{
//					gcMap[c] = *(vs.begin());
//					__COUT__ << "Adding config " << c << ":" << gcMap[c] << std::endl;
//				}
//			}
//
//			int i = 0;
//			bool done = false;
//			char gcname[100];
//			bool found;
//			while(!done && i<1)
//			{
//				do	//avoid conflicting global config names
//				{
//					found = false;
//					sprintf(gcname,"GConfig_v%d",i++);
//					for(auto &g:gcfgs)
//						if(g == gcname) {found = true; break;}
//				}
//				while(found);
//				__COUT__ << "Trying Global config: " << gcname<< std::endl;
//
//				try
//				{
//					theInterface_->saveConfigurationGroup(gcMap,gcname);
//					done = true;
//					__COUT__ << "Created Global config: " << gcname<< std::endl;
//				}
//				catch(...) {++i;} //repeat names are not allowed, so increment name
//			}
//
//		}
//
//		//return;
//	}
//
	//this is to test config tree
	try
	{
		__COUT__ << "Loading config..." << std::endl;
		loadConfigurationGroup("FETest",ConfigurationGroupKey(2)); // Context_1
		ConfigurationTree t = getNode("/FEConfiguration/DEFAULT/FrontEndType");

		std::string v;

		__COUT__ << std::endl;
		t.getValue(v);
		__COUT__ << "Value: " << v << std::endl;
		__COUT__ << "Value index: " << t.getValue<int>() << std::endl;

		return;

		//ConfigurationTree t = getNode("XDAQContextConfiguration");
		//ConfigurationTree t = getNode("/FEConfiguration/OtsUDPFSSR3/FrontEndType");
		//ConfigurationTree t = getNode("/FEConfiguration").getNode("OtsUDPFSSR3");
//		ConfigurationTree t = getNode("/XDAQContextConfiguration/testContext/");
//
//		__COUT__ << std::endl;
//		t.getValue(v);
//		__COUT__ << "Value: " << v << std::endl;
//
//		if(!t.isValueNode())
//		{
//			auto C = t.getChildrenNames();
//			for(auto &c: C)
//				__COUT__ << "\t+ " << c << std::endl;
//
//			std::stringstream ss;
//			t.print(-1,ss);
//			__COUT__ << "\n" << ss.str() << std::endl;
//
//			try
//			{
//				ConfigurationTree tt = t.getNode("OtsUDPFSSR3");
//				tt.getValue(v);
//				__COUT__ << "Value: " << v << std::endl;
//
//				C = tt.getChildrenNames();
//				for(auto &c: C)
//					__COUT__ << "\t+ " << c << std::endl;
//			}
//			catch(...)
//			{
//				__COUT__ << "Failed to find extra node." << std::endl;
//			}
//		}

	}
	catch(...)
	{
		__COUT__ << "Failed to load config..." << std::endl;
	}


}













