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
	__MOUT__ << "Using Config Mgr with Write Access! (for " << username << ")" << std::endl;


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

	ConfigurationBase* configuration;

	//existing configurations are defined by which infos are in CONFIGURATION_INFO_PATH
	//can test that the class exists based on this
	//and then which versions
	__MOUT__ << "Extracting list of Configuration tables" << std::endl;
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

			//__MOUT__ << entry->d_name << std::endl;

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

				__MOUT__ << "Skipping! No valid class found for... " << entry->d_name << "\n";
				continue;
			}
			catch(std::runtime_error &e)
			{
				if(configuration) delete configuration;
				configuration = 0;

				__MOUT__ << "Skipping! No valid class found for... " << entry->d_name << "\n";
				__MOUT__ << "Error: " << e.what() << std::endl;


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
					__MOUT__ << "Attempting to allow illegal columns!" << std::endl;


					std::string returnedAccumulatedErrors;
					try
					{
						//configuration = new ConfigurationBase(entry->d_name, &returnedAccumulatedErrors);
						configuration = new ConfigurationBase(entry->d_name, &returnedAccumulatedErrors);
					}
					catch(...)
					{
						__MOUT__ << "Skipping! Allowing illegal columns didn't work either... " <<
								entry->d_name << "\n";
						continue;
					}
					__MOUT__ << "Error (but allowed): " << returnedAccumulatedErrors << std::endl;

					if(errorFilterName == "" ||
							errorFilterName == entry->d_name)
						*accumulatedErrors += std::string("\nIn table '") + entry->d_name +
						"'..." + returnedAccumulatedErrors; //global accumulate
				}
				else
					continue;
			}

			//__MOUT__ << "Instance created: " << entry->d_name << "\n"; //found!

			if(nameToConfigurationMap_[entry->d_name]) //handle if instance existed
			{
				//copy the temporary versions! (or else all is lost)
				std::set<ConfigurationVersion> versions =
						nameToConfigurationMap_[entry->d_name]->getStoredVersions();
				for(auto &version:versions)
					if(version.isTemporaryVersion())
					{
						__MOUT__ << "copying tmp = " << version << std::endl;

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
				//__MOUT__ << "deleting: " << entry->d_name << "\n"; //found!
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
					__MOUT__ << "surviving tmp = " << version << std::endl;
					allConfigurationInfo_[entry->d_name].versions_.emplace(version);
				}
		}
		closedir(pDIR);
	}
	__MOUT__ << "Extracting list of Configuration tables complete" << std::endl;

	//call init to load active versions by default
	init(accumulatedErrors);

	return allConfigurationInfo_;
}

//==============================================================================
//getActiveAliases()
//	get active version aliases organized by table
std::map<std::string,std::map<std::string,ConfigurationVersion> >
ConfigurationManagerRW::getActiveVersionAliases(void) const
{
	__MOUT__ << "getActiveVersionAliases()" << std::endl;
	std::map<std::string,std::map<std::string,ConfigurationVersion> > retMap;

	std::map<std::string, ConfigurationVersion> activeVersions = getActiveVersions();
	std::string versionAliasesTableName = "VersionAliasesConfiguration";
	if(activeVersions.find(versionAliasesTableName) == activeVersions.end())
	{
		__SS__ << "Active version of VersionAliases  missing!" <<
				"Make sure you have a valid active Backbone Group." << std::endl;
		__MOUT_WARN__ << "\n" << ss.str();
		return retMap;
	}

	__MOUT__ << "activeVersions[\"VersionAliasesConfiguration\"]=" <<
			activeVersions[versionAliasesTableName] << std::endl;

	//always have scratch alias for each table that has a scratch version
	if(!ConfigurationInterface::isVersionTrackingEnabled())
		for(const auto &tableInfo:allConfigurationInfo_)
			for(const auto &version:tableInfo.second.versions_)
				if(version.isScratchVersion())
					retMap[tableInfo.first][ConfigurationManager::SCRATCH_VERSION_ALIAS] =
							ConfigurationVersion(ConfigurationVersion::SCRATCH);

	std::vector<std::pair<std::string,ConfigurationTree> > aliasNodePairs =
			getNode(versionAliasesTableName).getChildren();

	//create map
	//	add the first of each configName, versionAlias pair encountered
	//	ignore any repeats (Note: this also prevents overwriting of Scratch alias)
	std::string configName, versionAlias;
	for(auto& aliasNodePair:aliasNodePairs)
	{
		configName = aliasNodePair.second.getNode(
				"ConfigurationName").getValueAsString();
		versionAlias = aliasNodePair.second.getNode(
				"VersionAlias").getValueAsString();

		if(retMap.find(configName) != retMap.end() &&
				retMap[configName].find(versionAlias) != retMap[configName].end())
			continue; //skip repeats (Note: this also prevents overwriting of Scratch alias)

		//else add version to map
		retMap[configName][versionAlias] = ConfigurationVersion(
								aliasNodePair.second.getNode("Version").getValueAsString());
	}

	return retMap;
}

//==============================================================================
//setActiveGlobalConfiguration
//	load config group and activate
//	deactivates previous config group of same type if necessary
void ConfigurationManagerRW::activateConfigurationGroup(const std::string &configGroupName,
		ConfigurationGroupKey configGroupKey, std::string *accumulatedTreeErrors)
{
	loadConfigurationGroup(configGroupName,configGroupKey,
			true, 	//loads and activates
			0,		//no progress bar
			accumulatedTreeErrors); //accumulate warnings or not

	if(accumulatedTreeErrors &&
			*accumulatedTreeErrors != "")
	{
		__MOUT_ERR__ << "Errors were accumulated so de-activating group: " <<
				configGroupName << " (" << configGroupKey << ")" << std::endl;
		try //just in case any lingering pieces, lets deactivate
		{ destroyConfigurationGroup(configGroupName,true); }
		catch(...){}
	}

	__MOUT_INFO__ << "Updating persistent active groups to " <<
			ConfigurationManager::ACTIVE_GROUP_FILENAME << " ..."  << std::endl;

	std::string fn = ConfigurationManager::ACTIVE_GROUP_FILENAME;
	FILE *fp = fopen(fn.c_str(),"w");
	if(!fp) return;

	fprintf(fp,"%s\n",theContextGroup_.c_str());
	fprintf(fp,"%s\n",theContextGroupKey_?theContextGroupKey_->toString().c_str():"-1");
	fprintf(fp,"%s\n",theBackboneGroup_.c_str());
	fprintf(fp,"%s\n",theBackboneGroupKey_?theBackboneGroupKey_->toString().c_str():"-1");
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
	__MOUT_INFO__ << "Creating temporary backbone view from version " <<
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

	__MOUT__ << "Common temporary backbone version found as " <<
			tmpVersion << std::endl;

	//create temporary views from source version to destination temporary version
	for (auto& name : backboneMemberNames)
	{
		retTmpVersion = getConfigurationByName(name)->createTemporaryView(sourceViewVersion, tmpVersion);
		if(retTmpVersion != tmpVersion)
		{
			__MOUT_ERR__ << "Failure! Temporary view requested was " <<
					tmpVersion << ". Mismatched temporary view created: " << retTmpVersion << std::endl;
			throw std::runtime_error("Mismatched temporary view created!");
		}
	}

	return tmpVersion;
}



//==============================================================================
ConfigurationBase* ConfigurationManagerRW::getConfigurationByName(const std::string &configurationName)
{
	if(nameToConfigurationMap_.find(configurationName) == nameToConfigurationMap_.end())
	{
		__SS__ << "\nConfiguration not found with name: " << configurationName << std::endl;
		__MOUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
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
		throw std::runtime_error(ss.str());
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
ConfigurationVersion ConfigurationManagerRW::saveNewConfiguration(const std::string &configurationName,
		ConfigurationVersion temporaryVersion, bool makeTemporary)
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
	while(!newVersion.isScratchVersion() && allConfigurationInfo_[configurationName].versions_.find(newVersion) !=
			allConfigurationInfo_[configurationName].versions_.end())
	{
		__MOUT_ERR__ << "What happenened!?? ERROR::: newVersion v" << newVersion <<
				" already exists!? How is it possible? Retrace your steps and tell an admin." << std::endl;

		//create a new temporary version of the target view
		temporaryVersion = config->createTemporaryView(newVersion);

		if(newVersion.isTemporaryVersion())
			newVersion = temporaryVersion;
		else
			newVersion = ConfigurationVersion::getNextVersion(newVersion);

		__MOUT_WARN__ << "Attempting to recover and use v" << newVersion << std::endl;


		if(!makeTemporary) //saveNewVersion makes the new version the active version
			newVersion = theInterface_->saveNewVersion(config, temporaryVersion, newVersion);
		else	//make the temporary version active
			config->setActiveView(newVersion);
	}

	if(newVersion.isInvalid())
	{
		__SS__ << "Something went wrong saving the new version v" << newVersion <<
				". What happened?! (duplicates? database error?)" << std::endl;
		__MOUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	//update allConfigurationInfo_ with the new version
	allConfigurationInfo_[configurationName].versions_.insert(newVersion);

	__MOUT__ << "New version added to info " << newVersion << std::endl;

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
				__MOUT__ << "Removing version info: " << *it << std::endl;
				allConfigurationInfo_[configurationName].versions_.erase(it++);
			}
			else
				++it;
		}
	}
	else //erase target version only
	{
		__MOUT__ << "Removing version info: " << targetVersion << std::endl;
		auto it = allConfigurationInfo_[configurationName].versions_.find(targetVersion);
		if(it == allConfigurationInfo_[configurationName].versions_.end())
		{
			__MOUT__ << "Target version was not found in info versions..." << std::endl;
			return;
		}
		allConfigurationInfo_[configurationName].versions_.erase(
				allConfigurationInfo_[configurationName].versions_.find(targetVersion));
		__MOUT__ << "Target version was erased from info." << std::endl;
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
//findConfigurationGroup
//	return group with same name and same members
//	else return invalid key
//
// FIXME -- this is taking too long when there are a ton of groups.
//	Change to going back only 20 or so.. (but the order also comes in alpha order from
//	theInterface_->getAllConfigurationGroupNames which is a problem for choosing the
//	most recent to check.
ConfigurationGroupKey ConfigurationManagerRW::findConfigurationGroup(const std::string &groupName,
		const std::map<std::string, ConfigurationVersion> &groupMemberMap)
{
	std::set<std::string /*name*/> groupNames =
			theInterface_->getAllConfigurationGroupNames(groupName);
	std::string name;
	ConfigurationGroupKey key;
	std::map<std::string /*name*/, ConfigurationVersion /*version*/> compareToMemberMap;
	bool isDifferent;
	for(auto &fullName: groupNames)
	{
		ConfigurationGroupKey::getGroupNameAndKey(fullName,name,key);

		__MOUT__ << fullName << " has name " << name << " ==? " << groupName << std::endl;
		if( name != groupName) continue;

		__MOUT__ << name << " == " << groupName << std::endl;
		compareToMemberMap = theInterface_->getConfigurationGroupMembers(fullName);

		isDifferent = false;
		for(auto &memberPair: groupMemberMap)
		{
			__MOUT__ << memberPair.first << " - " << memberPair.second << std::endl;
			if(compareToMemberMap.find(memberPair.first) == compareToMemberMap.end() ||	//name is missing
					memberPair.second != compareToMemberMap[memberPair.first]) //or version mismatch
			{	//then different
				__MOUT__ << "mismatch found!" << std::endl;
				isDifferent = true;
				break;
			}
		}
		if(isDifferent) continue;

		//check member size for exact match
		if(groupMemberMap.size() != compareToMemberMap.size()) continue; //different size, so not same (groupMemberMap is a subset of memberPairs)

		__MOUT__ << "Found exact match with key: " << key << std::endl;
		//else found an exact match!
		return key;
	}
	__MOUT__ << "no match found!" << std::endl;
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
ConfigurationGroupKey ConfigurationManagerRW::saveNewConfigurationGroup(const std::string &groupName,
		std::map<std::string, ConfigurationVersion> &groupMembers,
		ConfigurationGroupKey previousVersion, const std::string &groupComment)
{
	//steps:
	//	determine new group key
	//	verify group members
	//	verify groupNameWithKey
	//	verify store

	__MOUT__ << std::endl;
	//determine new group key
	ConfigurationGroupKey newKey;
	if(!previousVersion.isInvalid())	//if previous provided, bump that
		newKey = ConfigurationGroupKey::getNextKey(previousVersion);
	else							//get latest key from db, and bump
	{
		std::set<ConfigurationGroupKey> keys = theInterface_->getKeys(groupName);
		if(keys.size()) //if keys exist, bump the last
			newKey = ConfigurationGroupKey::getNextKey(*(keys.crbegin()));
		else			//else, take default
			newKey = ConfigurationGroupKey::getDefaultKey();
	}

	__MOUT__ << "New Key for group: " << groupName << " found as " << newKey << std::endl;

	//	verify group members
	//		- use all config info
	std::map<std::string, ConfigurationInfo> allCfgInfo = getAllConfigurationInfo();
	for(auto &memberPair : groupMembers )
	{
		//check member name
		if(allCfgInfo.find(memberPair.first) == allCfgInfo.end())
		{
			__MOUT_ERR__ << "Group member \"" << memberPair.first << "\" not found in configuration!";

			if(groupMetadataTable_.getConfigurationName() ==
					memberPair.first)
			{
				__MOUT_WARN__ << "Looks like this is the groupMetadataTable_. " <<
						"Note that this table is added to the member map when groups are saved." <<
						"It should not be part of member map when calling this function." << std::endl;
				__MOUT__ << "Attempting to recover." << std::endl;
				groupMembers.erase(groupMembers.find(memberPair.first));
			}
			else
				throw std::runtime_error("Group member not found!");
		}
		//check member version
		if(allCfgInfo[memberPair.first].versions_.find(memberPair.second) ==
				allCfgInfo[memberPair.first].versions_.end())
		{
			__MOUT_ERR__ << "Group member  \"" << memberPair.first << "\" version \"" <<
					memberPair.second << "\" not found in configuration!";
			throw std::runtime_error("Group member version not found!");
		}
	}

	// verify groupNameWithKey and attempt to store
	try
	{
		//save meta data for group; reuse groupMetadataTable_

		//__MOUT__ << username_ << " " << time(0) << " " << groupComment << std::endl;
		//columns are uid,comment,author,time
		groupMetadataTable_.getViewP()->setValue(groupComment,0,1);
		groupMetadataTable_.getViewP()->setValue(username_,0,2);
		groupMetadataTable_.getViewP()->setValue(time(0),0,3);

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
		__MOUT__ << "Created config group: " << groupName << ":" << newKey << std::endl;
	}
	catch(std::runtime_error &e)
	{
		__MOUT_ERR__ << "Failed to create config group: " << groupName << ":" << newKey << std::endl;
		__MOUT_ERR__ << "\n\n" << e.what() << std::endl;
		throw;
	}
	catch(...)
	{
		__MOUT_ERR__ << "Failed to create config group: " << groupName << ":" << newKey << std::endl;
		throw;
	}

	// at this point succeeded!
	return newKey;
}

//==============================================================================
//saveNewBackbone
//	makes the new version the active version and returns new version number
//	INVALID will give a new backbone from mockup
ConfigurationVersion ConfigurationManagerRW::saveNewBackbone(ConfigurationVersion temporaryVersion)
{
	__MOUT_INFO__ << "Creating new backbone from temporary version " <<
			temporaryVersion << std::endl;

	//find common available temporary version among backbone members
	ConfigurationVersion newVersion(ConfigurationVersion::DEFAULT);
	ConfigurationVersion retNewVersion;
	auto backboneMemberNames = ConfigurationManager::getBackboneMemberNames();
	for (auto& name : backboneMemberNames)
	{
		retNewVersion = ConfigurationManager::getConfigurationByName(name)->getNextVersion();
		__MOUT__ << "New version for backbone member (" << name << "): " <<
				retNewVersion << std::endl;
		if(retNewVersion > newVersion)
			newVersion = retNewVersion;
	}

	__MOUT__ << "Common new backbone version found as " <<
			newVersion << std::endl;

	//create new views from source temporary version
	for (auto& name : backboneMemberNames)
	{
		//saveNewVersion makes the new version the active version
		retNewVersion = getConfigurationInterface()->saveNewVersion(
				getConfigurationByName(name), temporaryVersion, newVersion);
		if(retNewVersion != newVersion)
		{
			__MOUT_ERR__ << "Failure! New view requested was " <<
					newVersion << ". Mismatched new view created: " << retNewVersion << std::endl;
			throw std::runtime_error("Mismatched temporary view created!");
		}
	}

	return newVersion;
}

//==============================================================================
void ConfigurationManagerRW::testXDAQContext()
{
//	//test creating config group with author, create time, and comment
//	{
//		__MOUT__ << std::endl;
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
//				__MOUT__ << "Group members:" << std::endl;
//				for(const auto &member: memberMap)
//					__MOUT__ << member.first << " " << member.second << std::endl;
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
//				__MOUT__ << "Group members:" << std::endl;
//				for(const auto &member: memberMap)
//					__MOUT__ << member.first << " " << member.second << std::endl;
//			}
//
//
//		}
//		catch(std::runtime_error &e)
//		{
//			__MOUT_ERR__ << "Failed to create config group: " << groupName << ":" << newKey << std::endl;
//			__MOUT_ERR__ << "\n\n" << e.what() << std::endl;
//		}
//		catch(...)
//		{
//			__MOUT_ERR__ << "Failed to create config group: " << groupName << ":" << newKey << std::endl;
//		}
//
//		return;
//	}

//	//test creating config group and reading
//	{
//		__MOUT__ << std::endl;
//
//		auto gcfgs = theInterface_->getAllConfigurationGroupNames();
//		__MOUT__ << "Global config size: " << gcfgs.size() << std::endl;
//		for(auto &g:gcfgs)
//		{
//			__MOUT__ << "Global config " << g << std::endl;
//			auto gcMap = theInterface_->getConfigurationGroupMembers(g);
//
//			for(auto &cv:gcMap)
//				__MOUT__ << "\tMember config " << cv.first << ":" << cv.second << std::endl;
//		}
//
//		auto cfgs = theInterface_->getAllConfigurationNames();
//		__MOUT__ << "Sub-config size: " << cfgs.size() << std::endl;
//		for(auto &c:cfgs)
//		{
//			__MOUT__ << "config " << c << std::endl;
//			auto vs = theInterface_->getVersions(getConfigurationByName(c));
//			for(auto &v:vs)
//				__MOUT__ << "\tversion " << v << std::endl;
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
//					__MOUT__ << "Adding config " << c << ":" << gcMap[c] << std::endl;
//				}
//				else if(vs.begin() != vs.end())	//create oldest!
//				{
//					gcMap[c] = *(vs.begin());
//					__MOUT__ << "Adding config " << c << ":" << gcMap[c] << std::endl;
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
//				__MOUT__ << "Trying Global config: " << gcname<< std::endl;
//
//				try
//				{
//					theInterface_->saveConfigurationGroup(gcMap,gcname);
//					done = true;
//					__MOUT__ << "Created Global config: " << gcname<< std::endl;
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
		__MOUT__ << "Loading config..." << std::endl;
		loadConfigurationGroup("FETest",ConfigurationGroupKey(2)); // Context_1
		ConfigurationTree t = getNode("/FEConfiguration/DEFAULT/FrontEndType");

		std::string v;

		__MOUT__ << std::endl;
		t.getValue(v);
		__MOUT__ << "Value: " << v << std::endl;
		__MOUT__ << "Value index: " << t.getValue<int>() << std::endl;

		return;

		//ConfigurationTree t = getNode("XDAQContextConfiguration");
		//ConfigurationTree t = getNode("/FEConfiguration/OtsUDPFSSR3/FrontEndType");
		//ConfigurationTree t = getNode("/FEConfiguration").getNode("OtsUDPFSSR3");
//		ConfigurationTree t = getNode("/XDAQContextConfiguration/testContext/");
//
//		__MOUT__ << std::endl;
//		t.getValue(v);
//		__MOUT__ << "Value: " << v << std::endl;
//
//		if(!t.isValueNode())
//		{
//			auto C = t.getChildrenNames();
//			for(auto &c: C)
//				__MOUT__ << "\t+ " << c << std::endl;
//
//			std::stringstream ss;
//			t.print(-1,ss);
//			__MOUT__ << "\n" << ss.str() << std::endl;
//
//			try
//			{
//				ConfigurationTree tt = t.getNode("OtsUDPFSSR3");
//				tt.getValue(v);
//				__MOUT__ << "Value: " << v << std::endl;
//
//				C = tt.getChildrenNames();
//				for(auto &c: C)
//					__MOUT__ << "\t+ " << c << std::endl;
//			}
//			catch(...)
//			{
//				__MOUT__ << "Failed to find extra node." << std::endl;
//			}
//		}

	}
	catch(...)
	{
		__MOUT__ << "Failed to load config..." << std::endl;
	}


}













