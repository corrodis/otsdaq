#include "otsdaq-core/ConfigurationInterface/ConfigurationManagerRW.h"

// backbone includes
//#include "otsdaq-core/ConfigurationPluginDataFormats/ConfigurationAliases.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/Configurations.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/DefaultConfigurations.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/VersionAliases.h"

//#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"//All configurable
// objects are included here

//
//#include "otsdaq-core/ConfigurationPluginDataFormats/DetectorConfiguration.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/MaskConfiguration.h"
//#include "otsdaq-core/ConfigurationPluginDataFormats/DetectorToFEConfiguration.h"
//

//#include "otsdaq-core/ConfigurationInterface/DACStream.h"
//#include "otsdaq-core/ConfigurationDataFormats/TableGroupKey.h"
//
//#include "otsdaq-core/ConfigurationInterface/FileConfigurationInterface.h"
//
//#include <cassert>

#include <dirent.h>

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "ConfigurationManagerRW"

#define TABLE_INFO_PATH std::string(getenv("TABLE_INFO_PATH")) + "/"
#define TABLE_INFO_EXT "Info.xml"

//==============================================================================
// ConfigurationManagerRW
ConfigurationManagerRW::ConfigurationManagerRW(std::string username)
    : ConfigurationManager(username)  // for use as author of new views
{
	__COUT__ << "Using Config Mgr with Write Access! (for " << username << ")"
	         << std::endl;

	// FIXME only necessarily temporarily while Lore is still using fileSystem xml
	theInterface_ =
	    ConfigurationInterface::getInstance(false);  // false to use artdaq DB
	                                                 // FIXME -- can delete this change of
	                                                 // interface once RW and regular use
	                                                 // same interface instance
}

//==============================================================================
// getAllTableInfo()
//	Used by ConfigurationGUISupervisor to get all the info for the existing configurations
//
// if(accumulatedErrors)
//	this implies allowing column errors and accumulating such errors in given string
const std::map<std::string, TableInfo>& ConfigurationManagerRW::getAllTableInfo(
    bool refresh, std::string* accumulatedErrors, const std::string& errorFilterName)
{
	// allTableInfo_ is container to be returned

	if(accumulatedErrors)
		*accumulatedErrors = "";

	if(!refresh)
		return allTableInfo_;

	// else refresh!
	allTableInfo_.clear();
	allGroupInfo_.clear();

	TableBase* table;

	// existing configurations are defined by which infos are in TABLE_INFO_PATH
	// can test that the class exists based on this
	// and then which versions
	__COUT__ << "======================================================== "
	            "getAllTableInfo start"
	         << std::endl;
	__COUT__ << "Refreshing all! Extracting list of tables..." << std::endl;
	DIR*                pDIR;
	struct dirent*      entry;
	std::string         path               = TABLE_INFO_PATH;
	char                fileExt[]          = TABLE_INFO_EXT;
	const unsigned char MIN_TABLE_NAME_SZ = 3;
	if((pDIR = opendir(path.c_str())) != 0)
	{
		while((entry = readdir(pDIR)) != 0)
		{
			// enforce table name length
			if(strlen(entry->d_name) < strlen(fileExt) + MIN_TABLE_NAME_SZ)
				continue;

			// find file names with correct file extenstion
			if(strcmp(&(entry->d_name[strlen(entry->d_name) - strlen(fileExt)]),
			          fileExt) != 0)
				continue;  // skip different extentions

			entry->d_name[strlen(entry->d_name) - strlen(fileExt)] =
			    '\0';  // remove file extension to get table name

			//__COUT__ << entry->d_name << std::endl;

			// 0 will force the creation of new instance (and reload from Info)
			table = 0;

			try  // only add valid table instances to maps
			{
				theInterface_->get(table, entry->d_name, 0, 0,
				                   true);  // dont fill
			}
			catch(cet::exception)
			{
				if(table)
					delete table;
				table = 0;

				__COUT__ << "Skipping! No valid class found for... " << entry->d_name
				         << "\n";
				continue;
			}
			catch(std::runtime_error& e)
			{
				if(table)
					delete table;
				table = 0;

				__COUT__ << "Skipping! No valid class found for... " << entry->d_name
				         << "\n";
				__COUT__ << "Error: " << e.what() << std::endl;

				// for a runtime_error, it is likely that columns are the problem
				//	the Table Editor needs to still fix these.. so attempt to
				// 	proceed.
				if(accumulatedErrors)
				{
					if(errorFilterName == "" || errorFilterName == entry->d_name)
					{
						*accumulatedErrors += std::string("\nIn table '") +
						                      entry->d_name + "'..." +
						                      e.what();  // global accumulate

						__SS__ << "Attempting to allow illegal columns!" << std::endl;
						*accumulatedErrors += ss.str();
					}

					// attempt to recover and build a mock-up
					__COUT__ << "Attempting to allow illegal columns!" << std::endl;

					std::string returnedAccumulatedErrors;
					try
					{
						// table = new TableBase(entry->d_name,
						// &returnedAccumulatedErrors);
						table = new TableBase(entry->d_name, &returnedAccumulatedErrors);
					}
					catch(...)
					{
						__COUT__
						    << "Skipping! Allowing illegal columns didn't work either... "
						    << entry->d_name << "\n";
						continue;
					}
					__COUT__ << "Error (but allowed): " << returnedAccumulatedErrors
					         << std::endl;

					if(errorFilterName == "" || errorFilterName == entry->d_name)
						*accumulatedErrors +=
						    std::string("\nIn table '") + entry->d_name + "'..." +
						    returnedAccumulatedErrors;  // global accumulate
				}
				else
					continue;
			}

			//__COUT__ << "Instance created: " << entry->d_name << "\n"; //found!

			if(nameToConfigurationMap_[entry->d_name])  // handle if instance existed
			{
				// copy the temporary versions! (or else all is lost)
				std::set<TableVersion> versions =
				    nameToConfigurationMap_[entry->d_name]->getStoredVersions();
				for(auto& version : versions)
					if(version.isTemporaryVersion())
					{
						//__COUT__ << "copying tmp = " << version << std::endl;

						try  // do NOT let TableView::init() throw here
						{
							nameToConfigurationMap_[entry->d_name]->setActiveView(
							    version);
							table->copyView(  // this calls TableView::init()
							    nameToConfigurationMap_[entry->d_name]->getView(),
							    version,
							    username_);
						}
						catch(...)  // do NOT let invalid temporary version throw at this
						            // point
						{
						}  // just trust configurationBase throws out the failed version
					}
				//__COUT__ << "deleting: " << entry->d_name << "\n"; //found!
				delete nameToConfigurationMap_[entry->d_name];
				nameToConfigurationMap_[entry->d_name] = 0;
			}

			nameToConfigurationMap_[entry->d_name] = table;

			allTableInfo_[entry->d_name].tablePtr_ = table;
			allTableInfo_[entry->d_name].versions_ = theInterface_->getVersions(table);

			// also add any existing temporary versions to all table info
			//	because the interface wont find those versions
			std::set<TableVersion> versions =
			    nameToConfigurationMap_[entry->d_name]->getStoredVersions();
			for(auto& version : versions)
				if(version.isTemporaryVersion())
				{
					//__COUT__ << "surviving tmp = " << version << std::endl;
					allTableInfo_[entry->d_name].versions_.emplace(version);
				}
		}
		closedir(pDIR);
	}
	__COUT__ << "Extracting list of Configuration tables complete" << std::endl;

	// call init to load active versions by default
	init(accumulatedErrors);

	__COUT__
	    << "======================================================== getAllTableInfo end"
	    << std::endl;

	// get Group Info too!
	try
	{
		// build allGroupInfo_ for the ConfigurationManagerRW

		std::set<std::string /*name*/> configGroups =
		    theInterface_->getAllConfigurationGroupNames();
		__COUT__ << "Number of Groups: " << configGroups.size() << std::endl;

		TableGroupKey key;
		std::string   name;
		for(const auto& fullName : configGroups)
		{
			TableGroupKey::getGroupNameAndKey(fullName, name, key);
			cacheGroupKey(name, key);
		}

		// for each group get member map & comment, author, time, and type for latest key
		for(auto& groupInfo : allGroupInfo_)
		{
			try
			{
				loadConfigurationGroup(
				    groupInfo.first /*groupName*/,
				    groupInfo.second.getLatestKey(),
				    false /*doActivate*/,
				    &groupInfo.second.latestKeyMemberMap_ /*groupMembers*/,
				    0 /*progressBar*/,
				    0 /*accumulateErrors*/,
				    &groupInfo.second.latestKeyGroupComment_,
				    &groupInfo.second.latestKeyGroupAuthor_,
				    &groupInfo.second.latestKeyGroupCreationTime_,
				    true /*doNotLoadMember*/,
				    &groupInfo.second.latestKeyGroupTypeString_);
			}
			catch(...)
			{
				__COUT_WARN__
				    << "Error occurred loading latest group info into cache for '"
				    << groupInfo.first << "'..." << __E__;
				groupInfo.second.latestKeyGroupComment_      = "UNKNOWN";
				groupInfo.second.latestKeyGroupAuthor_       = "UNKNOWN";
				groupInfo.second.latestKeyGroupCreationTime_ = "0";
				groupInfo.second.latestKeyGroupTypeString_   = "UNKNOWN";
			}
		}  // end group info loop
	}      // end get group info
	catch(const std::runtime_error& e)
	{
		__SS__ << "A fatal error occurred reading the info for all table groups. Error: "
		       << e.what() << __E__;
		__COUT_ERR__ << "\n" << ss.str();
		if(accumulatedErrors)
			*accumulatedErrors += ss.str();
		else
			throw;
	}
	catch(...)
	{
		__SS__ << "An unknown fatal error occurred reading the info for all table groups."
		       << __E__;
		__COUT_ERR__ << "\n" << ss.str();
		if(accumulatedErrors)
			*accumulatedErrors += ss.str();
		else
			throw;
	}

	return allTableInfo_;
}  // end getAllTableInfo

//==============================================================================
// getVersionAliases()
//	get version aliases organized by table, for currently active backbone tables
//	add scratch versions to the alias map returned by ConfigurationManager
std::map<std::string /*table name*/,
         std::map<std::string /*version alias*/, TableVersion /*aliased version*/> >
ConfigurationManagerRW::getVersionAliases(void) const
{
	//__COUT__ << "getVersionAliases()" << std::endl;
	std::map<std::string /*table name*/,
	         std::map<std::string /*version alias*/, TableVersion /*aliased version*/> >
	    retMap = ConfigurationManager::getVersionAliases();

	// always have scratch alias for each table that has a scratch version
	//	overwrite map entry if necessary
	if(!ConfigurationInterface::isVersionTrackingEnabled())
		for(const auto& tableInfo : allTableInfo_)
			for(const auto& version : tableInfo.second.versions_)
				if(version.isScratchVersion())
					retMap[tableInfo.first][ConfigurationManager::SCRATCH_VERSION_ALIAS] =
					    TableVersion(TableVersion::SCRATCH);

	return retMap;
}  // end getVersionAliases()

//==============================================================================
// setActiveGlobalConfiguration
//	load table group and activate
//	deactivates previous table group of same type if necessary
void ConfigurationManagerRW::activateConfigurationGroup(
    const std::string& configGroupName,
    TableGroupKey      configGroupKey,
    std::string*       accumulatedTreeErrors)
{
	loadConfigurationGroup(configGroupName,
	                       configGroupKey,
	                       true,                    // loads and activates
	                       0,                       // no members needed
	                       0,                       // no progress bar
	                       accumulatedTreeErrors);  // accumulate warnings or not

	if(accumulatedTreeErrors && *accumulatedTreeErrors != "")
	{
		__COUT_ERR__ << "Errors were accumulated so de-activating group: "
		             << configGroupName << " (" << configGroupKey << ")" << std::endl;
		try  // just in case any lingering pieces, lets deactivate
		{
			destroyConfigurationGroup(configGroupName, true);
		}
		catch(...)
		{
		}
	}

	__COUT_INFO__ << "Updating persistent active groups to "
	              << ConfigurationManager::ACTIVE_GROUPS_FILENAME << " ..." << std::endl;
	__MOUT_INFO__ << "Updating persistent active groups to "
	              << ConfigurationManager::ACTIVE_GROUPS_FILENAME << " ..." << std::endl;

	std::string fn = ConfigurationManager::ACTIVE_GROUPS_FILENAME;
	FILE*       fp = fopen(fn.c_str(), "w");
	if(!fp)
	{
		__SS__ << "Fatal Error! Unable to open the file "
		       << ConfigurationManager::ACTIVE_GROUPS_FILENAME
		       << " for editing! Is there a permissions problem?" << std::endl;
		__COUT_ERR__ << ss.str();
		__SS_THROW__;
		return;
	}

	__MCOUT_INFO__("Active Context: " << theContextGroup_ << "("
	                                  << (theContextGroupKey_
	                                          ? theContextGroupKey_->toString().c_str()
	                                          : "-1")
	                                  << ")" << std::endl);
	__MCOUT_INFO__("Active Backbone: " << theBackboneGroup_ << "("
	                                   << (theBackboneGroupKey_
	                                           ? theBackboneGroupKey_->toString().c_str()
	                                           : "-1")
	                                   << ")" << std::endl);
	__MCOUT_INFO__("Active Iterate: " << theIterateGroup_ << "("
	                                  << (theIterateGroupKey_
	                                          ? theIterateGroupKey_->toString().c_str()
	                                          : "-1")
	                                  << ")" << std::endl);
	__MCOUT_INFO__("Active Configuration: "
	               << theConfigurationGroup_ << "("
	               << (theTableGroupKey_ ? theTableGroupKey_->toString().c_str() : "-1")
	               << ")" << std::endl);

	fprintf(fp, "%s\n", theContextGroup_.c_str());
	fprintf(
	    fp, "%s\n", theContextGroupKey_ ? theContextGroupKey_->toString().c_str() : "-1");
	fprintf(fp, "%s\n", theBackboneGroup_.c_str());
	fprintf(fp,
	        "%s\n",
	        theBackboneGroupKey_ ? theBackboneGroupKey_->toString().c_str() : "-1");
	fprintf(fp, "%s\n", theIterateGroup_.c_str());
	fprintf(
	    fp, "%s\n", theIterateGroupKey_ ? theIterateGroupKey_->toString().c_str() : "-1");
	fprintf(fp, "%s\n", theConfigurationGroup_.c_str());
	fprintf(fp, "%s\n", theTableGroupKey_ ? theTableGroupKey_->toString().c_str() : "-1");
	fclose(fp);
}

//==============================================================================
// createTemporaryBackboneView
//	sourceViewVersion of INVALID is from MockUp, else from valid view version
// 	returns temporary version number (which is always negative)
TableVersion ConfigurationManagerRW::createTemporaryBackboneView(
    TableVersion sourceViewVersion)
{
	__COUT_INFO__ << "Creating temporary backbone view from version " << sourceViewVersion
	              << std::endl;

	// find common available temporary version among backbone members
	TableVersion tmpVersion =
	    TableVersion::getNextTemporaryVersion();  // get the default temporary version
	TableVersion retTmpVersion;
	auto         backboneMemberNames = ConfigurationManager::getBackboneMemberNames();
	for(auto& name : backboneMemberNames)
	{
		retTmpVersion =
		    ConfigurationManager::getTableByName(name)->getNextTemporaryVersion();
		if(retTmpVersion < tmpVersion)
			tmpVersion = retTmpVersion;
	}

	__COUT__ << "Common temporary backbone version found as " << tmpVersion << std::endl;

	// create temporary views from source version to destination temporary version
	for(auto& name : backboneMemberNames)
	{
		retTmpVersion =
		    getTableByName(name)->createTemporaryView(sourceViewVersion, tmpVersion);
		if(retTmpVersion != tmpVersion)
		{
			__SS__ << "Failure! Temporary view requested was " << tmpVersion
			       << ". Mismatched temporary view created: " << retTmpVersion
			       << std::endl;
			__COUT_ERR__ << ss.str();
			__SS_THROW__;
		}
	}

	return tmpVersion;
}

//==============================================================================
TableBase* ConfigurationManagerRW::getTableByName(const std::string& tableName)
{
	if(nameToConfigurationMap_.find(tableName) == nameToConfigurationMap_.end())
	{
		__SS__ << "Configuration not found with name: " << tableName << std::endl;
		size_t f;
		if((f = tableName.find(' ')) != std::string::npos)
			ss << "There was a space character found in the table name needle at "
			      "position "
			   << f << " in the string (was this intended?). " << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}
	return nameToConfigurationMap_[tableName];
}

//==============================================================================
// getVersionedTableByName
//	Used by table GUI to load a particular table-version pair as the active version.
// 	This table instance must already exist and be owned by ConfigurationManager.
//	return null pointer on failure, on success return table pointer.
TableBase* ConfigurationManagerRW::getVersionedTableByName(const std::string& tableName,
                                                           TableVersion       version,
                                                           bool looseColumnMatching)
{
	auto it = nameToConfigurationMap_.find(tableName);
	if(it == nameToConfigurationMap_.end())
	{
		__SS__ << "\nCan not find table named '" << tableName
		       << "'\n\n\n\nYou need to load the table before it can be used."
		       << "It probably is missing from the member list of the Configuration "
		          "Group that was loaded?\n\n\n\n\n"
		       << std::endl;
		__SS_THROW__;
	}
	TableBase* table = it->second;
	theInterface_->get(table,
	                   tableName,
	                   0,
	                   0,
	                   false,  // fill w/version
	                   version,
	                   false,  // do not reset
	                   looseColumnMatching);
	return table;
}

//==============================================================================
// saveNewTable
//	saves version, makes the new version the active version, and returns new version
TableVersion ConfigurationManagerRW::saveNewTable(const std::string& tableName,
                                                  TableVersion       temporaryVersion,
                                                  bool               makeTemporary)  //,
// bool saveToScratchVersion)
{
	TableVersion newVersion(temporaryVersion);

	// set author of version
	TableBase* table = getTableByName(tableName);
	table->getTemporaryView(temporaryVersion)->setAuthor(username_);
	// NOTE: somehow? author is assigned to permanent versions when saved to DBI?

	if(!makeTemporary)  // saveNewVersion makes the new version the active version
		newVersion = theInterface_->saveNewVersion(table, temporaryVersion);
	else  // make the temporary version active
		table->setActiveView(newVersion);

	// if there is a problem, try to recover
	while(!makeTemporary && !newVersion.isScratchVersion() &&
	      allTableInfo_[tableName].versions_.find(newVersion) !=
	          allTableInfo_[tableName].versions_.end())
	{
		__COUT_ERR__ << "What happenened!?? ERROR::: new persistent version v"
		             << newVersion
		             << " already exists!? How is it possible? Retrace your steps and "
		                "tell an admin."
		             << std::endl;

		// create a new temporary version of the target view
		temporaryVersion = table->createTemporaryView(newVersion);

		if(newVersion.isTemporaryVersion())
			newVersion = temporaryVersion;
		else
			newVersion = TableVersion::getNextVersion(newVersion);

		__COUT_WARN__ << "Attempting to recover and use v" << newVersion << std::endl;

		if(!makeTemporary)  // saveNewVersion makes the new version the active version
			newVersion =
			    theInterface_->saveNewVersion(table, temporaryVersion, newVersion);
		else  // make the temporary version active
			table->setActiveView(newVersion);
	}

	if(newVersion.isInvalid())
	{
		__SS__ << "Something went wrong saving the new version v" << newVersion
		       << ". What happened?! (duplicates? database error?)" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}

	// update allTableInfo_ with the new version
	allTableInfo_[tableName].versions_.insert(newVersion);

	__COUT__ << "New version added to info " << newVersion << std::endl;

	return newVersion;
}

//==============================================================================
// eraseTemporaryVersion
//	if version is invalid then erases ALL temporary versions
//
//	maintains allTableInfo_ also while erasing
void ConfigurationManagerRW::eraseTemporaryVersion(const std::string& tableName,
                                                   TableVersion       targetVersion)
{
	TableBase* table = getTableByName(tableName);

	table->trimTemporary(targetVersion);

	// if allTableInfo_ is not setup, then done
	if(allTableInfo_.find(tableName) == allTableInfo_.end())
		return;
	// else cleanup table info

	if(targetVersion.isInvalid())
	{
		// erase all temporary versions!
		for(auto it = allTableInfo_[tableName].versions_.begin();
		    it != allTableInfo_[tableName].versions_.end();
		    /*no increment*/)
		{
			if(it->isTemporaryVersion())
			{
				__COUT__ << "Removing version info: " << *it << std::endl;
				allTableInfo_[tableName].versions_.erase(it++);
			}
			else
				++it;
		}
	}
	else  // erase target version only
	{
		__COUT__ << "Removing version info: " << targetVersion << std::endl;
		auto it = allTableInfo_[tableName].versions_.find(targetVersion);
		if(it == allTableInfo_[tableName].versions_.end())
		{
			__COUT__ << "Target version was not found in info versions..." << std::endl;
			return;
		}
		allTableInfo_[tableName].versions_.erase(
		    allTableInfo_[tableName].versions_.find(targetVersion));
		__COUT__ << "Target version was erased from info." << std::endl;
	}
}

//==============================================================================
// clearCachedVersions
//	clear ALL cached persistent versions (does not erase temporary versions)
//
//	maintains allTableInfo_ also while erasing (trivial, do nothing)
void ConfigurationManagerRW::clearCachedVersions(const std::string& tableName)
{
	TableBase* table = getTableByName(tableName);

	table->trimCache(0);
}

//==============================================================================
// clearAllCachedVersions
//	clear ALL cached persistent versions (does not erase temporary versions)
//
//	maintains allTableInfo_ also while erasing (trivial, do nothing)
void ConfigurationManagerRW::clearAllCachedVersions()
{
	for(auto configInfo : allTableInfo_)
		configInfo.second.tablePtr_->trimCache(0);
}

//==============================================================================
// copyViewToCurrentColumns
TableVersion ConfigurationManagerRW::copyViewToCurrentColumns(
    const std::string& tableName, TableVersion sourceVersion)
{
	getTableByName(tableName)->reset();

	// make sure source version is loaded
	// need to load with loose column rules!
	TableBase* table =
	    getVersionedTableByName(tableName, TableVersion(sourceVersion), true);

	// copy from source version to a new temporary version
	TableVersion newTemporaryVersion =
	    table->copyView(table->getView(), TableVersion(), username_);

	// update allTableInfo_ with the new version
	allTableInfo_[tableName].versions_.insert(newTemporaryVersion);

	return newTemporaryVersion;
}

//==============================================================================
// cacheGroupKey
void ConfigurationManagerRW::cacheGroupKey(const std::string& groupName,
                                           TableGroupKey      key)
{
	allGroupInfo_[groupName].keys_.emplace(key);
}

//==============================================================================
// getGroupInfo
//	the interface is slow when there are a lot of groups..
//	so plan is to maintain local cache of recent group info
const GroupInfo& ConfigurationManagerRW::getGroupInfo(const std::string& groupName)
{
	//	//NOTE: seems like this filter is taking the long amount of time
	//	std::set<std::string /*name*/> fullGroupNames =
	//			theInterface_->getAllConfigurationGroupNames(groupName); //db filter by
	// group  name

	// so instead caching ourselves...
	auto it = allGroupInfo_.find(groupName);
	if(it == allGroupInfo_.end())
	{
		__SS__ << "Group name '" << groupName
		       << "' not found in group info! (creating empty info)" << __E__;
		__COUT_WARN__ << ss.str();
		//__SS_THROW__;
		return allGroupInfo_[groupName];
	}
	return it->second;
}

//==============================================================================
// findTableGroup
//	return group with same name and same members and same aliases
//	else return invalid key
//
// Note: if aliases, then member alias is matched (not member
//
// Note: this is taking too long when there are a ton of groups.
//	Change to going back only a limited number.. (but the order also comes in alpha order
// from 	theInterface_->getAllConfigurationGroupNames which is a problem for choosing
// the 	most recent to check. )
TableGroupKey ConfigurationManagerRW::findTableGroup(
    const std::string&                                           groupName,
    const std::map<std::string, TableVersion>&                   groupMemberMap,
    const std::map<std::string /*name*/, std::string /*alias*/>& groupAliases)
{
	//	//NOTE: seems like this filter is taking the long amount of time
	//	std::set<std::string /*name*/> fullGroupNames =
	//			theInterface_->getAllConfigurationGroupNames(groupName); //db filter by
	// group  name
	const GroupInfo& groupInfo = getGroupInfo(groupName);

	// std::string name;
	// TableGroupKey key;
	std::map<std::string /*name*/, TableVersion /*version*/> compareToMemberMap;
	std::map<std::string /*name*/, std::string /*alias*/>    compareToGroupAliases;
	bool                                                     isDifferent;

	const unsigned int MAX_DEPTH_TO_CHECK = 20;
	unsigned int       keyMinToCheck      = 0;

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

	// have min key to check, now loop through and check groups
	// std::string fullName;
	for(const auto& key : groupInfo.keys_)
	{
		// TableGroupKey::getGroupNameAndKey(fullName,name,key);

		if(key.key() < keyMinToCheck)
			continue;  // skip keys that are too old

		//		fullName = TableGroupKey::getFullGroupString(groupName,key);
		//
		//		__COUT__ << "checking group... " << fullName << std::endl;
		//
		//		compareToMemberMap =
		// theInterface_->getConfigurationGroupMembers(fullName);

		loadConfigurationGroup(groupName,
		                       key,
		                       false /*doActivate*/,
		                       &compareToMemberMap /*memberMap*/,
		                       0,
		                       0,
		                       0,
		                       0,
		                       0, /*null pointers*/
		                       true /*doNotLoadMember*/,
		                       0 /*groupTypeString*/,
		                       &compareToGroupAliases);

		isDifferent = false;
		for(auto& memberPair : groupMemberMap)
		{
			//__COUT__ << memberPair.first << " - " << memberPair.second << std::endl;

			if(groupAliases.find(memberPair.first) != groupAliases.end())
			{
				// handle this table as alias, not version
				if(compareToGroupAliases.find(memberPair.first) ==
				       compareToGroupAliases.end() ||  // alias is missing
				   groupAliases.at(memberPair.first) !=
				       compareToGroupAliases.at(memberPair.first))
				{  // then different
					//__COUT__ << "alias mismatch found!" << std::endl;
					isDifferent = true;
					break;
				}
				else
					continue;
			}  // else check if compareTo group is using an alias for table
			else if(compareToGroupAliases.find(memberPair.first) !=
			        compareToGroupAliases.end())
			{
				// then different
				//__COUT__ << "alias mismatch found!" << std::endl;
				isDifferent = true;
				break;

			}  // else handle as table version comparison
			else if(compareToMemberMap.find(memberPair.first) ==
			            compareToMemberMap.end() ||  // name is missing
			        memberPair.second !=
			            compareToMemberMap.at(memberPair.first))  // or version mismatch
			{                                                     // then different
				//__COUT__ << "mismatch found!" << std::endl;
				isDifferent = true;
				break;
			}
		}
		if(isDifferent)
			continue;

		// check member size for exact match
		if(groupMemberMap.size() != compareToMemberMap.size())
			continue;  // different size, so not same (groupMemberMap is a subset of
			           // memberPairs)

		__COUT__ << "Found exact match with key: " << key << std::endl;
		// else found an exact match!
		return key;
	}
	__COUT__ << "No match found - this group is new!" << std::endl;
	// if here, then no match found
	return TableGroupKey();  // return invalid key
}

//==============================================================================
// saveNewTableGroup
//	saves new group and returns the new group key
//	if previousVersion is provided, attempts to just bump that version
//	else, bumps latest version found in db
//
//	Note: groupMembers map will get modified with group metadata table version
TableGroupKey ConfigurationManagerRW::saveNewTableGroup(
    const std::string&                                      groupName,
    std::map<std::string, TableVersion>&                    groupMembers,
    const std::string&                                      groupComment,
    std::map<std::string /*table*/, std::string /*alias*/>* groupAliases)
{
	// steps:
	//	determine new group key
	//	verify group members
	//	verify groupNameWithKey
	//	verify store

	if(groupMembers.size() == 0)  // do not allow empty groups
	{
		__SS__ << "Empty group member list. Can not create a group without members!"
		       << std::endl;
		__SS_THROW__;
	}

	// determine new group key
	TableGroupKey newKey =
	    TableGroupKey::getNextKey(theInterface_->findLatestGroupKey(groupName));

	__COUT__ << "New Key for group: " << groupName << " found as " << newKey << std::endl;

	//	verify group members
	//		- use all table info
	std::map<std::string, TableInfo> allCfgInfo = getAllTableInfo();
	for(auto& memberPair : groupMembers)
	{
		// check member name
		if(allCfgInfo.find(memberPair.first) == allCfgInfo.end())
		{
			__COUT_ERR__ << "Group member \"" << memberPair.first
			             << "\" not found in database!";

			if(groupMetadataTable_.getTableName() == memberPair.first)
			{
				__COUT_WARN__
				    << "Looks like this is the groupMetadataTable_ '"
				    << ConfigurationInterface::GROUP_METADATA_TABLE_NAME
				    << ".' Note that this table is added to the member map when groups "
				       "are saved."
				    << "It should not be part of member map when calling this function."
				    << std::endl;
				__COUT__ << "Attempting to recover." << std::endl;
				groupMembers.erase(groupMembers.find(memberPair.first));
			}
			else
			{
				__SS__ << ("Group member not found!") << std::endl;
				__SS_THROW__;
			}
		}
		// check member version
		if(allCfgInfo[memberPair.first].versions_.find(memberPair.second) ==
		   allCfgInfo[memberPair.first].versions_.end())
		{
			__SS__ << "Group member  \"" << memberPair.first << "\" version \""
			       << memberPair.second << "\" not found in database!";
			__SS_THROW__;
		}
	}  // end verify members

	// verify group aliases
	if(groupAliases)
	{
		for(auto& aliasPair : *groupAliases)
		{
			// check for alias table in member names
			if(groupMembers.find(aliasPair.first) == groupMembers.end())
			{
				__COUT_ERR__ << "Group member \"" << aliasPair.first
				             << "\" not found in group member map!";

				__SS__ << ("Alias table not found in member list!") << std::endl;
				__SS_THROW__;
			}
		}
	}  // end verify group aliases

	// verify groupNameWithKey and attempt to store
	try
	{
		// save meta data for group; reuse groupMetadataTable_
		std::string groupAliasesString = "";
		if(groupAliases)
			groupAliasesString = StringMacros::mapToString(
			    *groupAliases, "," /*primary delimeter*/, ":" /*secondary delimeter*/);
		__COUT__ << "Metadata: " << username_ << " " << time(0) << " " << groupComment
		         << " " << groupAliasesString << std::endl;

		// to compensate for unusual errors upstream, make sure the metadata table has one
		// row
		while(groupMetadataTable_.getViewP()->getNumberOfRows() > 1)
			groupMetadataTable_.getViewP()->deleteRow(0);
		if(groupMetadataTable_.getViewP()->getNumberOfRows() == 0)
			groupMetadataTable_.getViewP()->addRow();

		// columns are uid,comment,author,time
		groupMetadataTable_.getViewP()->setValue(
		    groupAliasesString, 0, ConfigurationManager::METADATA_COL_ALIASES);
		groupMetadataTable_.getViewP()->setValue(
		    groupComment, 0, ConfigurationManager::METADATA_COL_COMMENT);
		groupMetadataTable_.getViewP()->setValue(
		    username_, 0, ConfigurationManager::METADATA_COL_AUTHOR);
		groupMetadataTable_.getViewP()->setValue(
		    time(0), 0, ConfigurationManager::METADATA_COL_TIMESTAMP);

		// set version to first available persistent version
		groupMetadataTable_.getViewP()->setVersion(TableVersion::getNextVersion(
		    theInterface_->findLatestVersion(&groupMetadataTable_)));

		// groupMetadataTable_.print();

		theInterface_->saveActiveVersion(&groupMetadataTable_);

		// force groupMetadataTable_ to be a member for the group
		groupMembers[groupMetadataTable_.getTableName()] =
		    groupMetadataTable_.getViewVersion();

		theInterface_->saveConfigurationGroup(
		    groupMembers, TableGroupKey::getFullGroupString(groupName, newKey));
		__COUT__ << "Created table group: " << groupName << ":" << newKey << std::endl;
	}
	catch(std::runtime_error& e)
	{
		__COUT_ERR__ << "Failed to create table group: " << groupName << ":" << newKey
		             << std::endl;
		__COUT_ERR__ << "\n\n" << e.what() << std::endl;
		throw;
	}
	catch(...)
	{
		__COUT_ERR__ << "Failed to create table group: " << groupName << ":" << newKey
		             << std::endl;
		throw;
	}

	// store cache of recent groups
	cacheGroupKey(groupName, newKey);

	// at this point succeeded!
	return newKey;
}

//==============================================================================
// saveNewBackbone
//	makes the new version the active version and returns new version number
//	INVALID will give a new backbone from mockup
TableVersion ConfigurationManagerRW::saveNewBackbone(TableVersion temporaryVersion)
{
	__COUT_INFO__ << "Creating new backbone from temporary version " << temporaryVersion
	              << std::endl;

	// find common available temporary version among backbone members
	TableVersion newVersion(TableVersion::DEFAULT);
	TableVersion retNewVersion;
	auto         backboneMemberNames = ConfigurationManager::getBackboneMemberNames();
	for(auto& name : backboneMemberNames)
	{
		retNewVersion = ConfigurationManager::getTableByName(name)->getNextVersion();
		__COUT__ << "New version for backbone member (" << name << "): " << retNewVersion
		         << std::endl;
		if(retNewVersion > newVersion)
			newVersion = retNewVersion;
	}

	__COUT__ << "Common new backbone version found as " << newVersion << std::endl;

	// create new views from source temporary version
	for(auto& name : backboneMemberNames)
	{
		// saveNewVersion makes the new version the active version
		retNewVersion = getConfigurationInterface()->saveNewVersion(
		    getTableByName(name), temporaryVersion, newVersion);
		if(retNewVersion != newVersion)
		{
			__SS__ << "Failure! New view requested was " << newVersion
			       << ". Mismatched new view created: " << retNewVersion << std::endl;
			__COUT_ERR__ << ss.str();
			__SS_THROW__;
		}
	}

	return newVersion;
}

//==============================================================================
void ConfigurationManagerRW::testXDAQContext()
{
	//	//test creating table group with author, create time, and comment
	//	{
	//		__COUT__ << std::endl;
	//
	//		std::string groupName = "testGroup";
	//		TableGroupKey newKey;
	//		//ConfigurationGroupMetadata
	//
	//		try
	//		{
	//			{
	//				std::map<std::string, TableVersion> members;
	//				members["ARTDAQAggregatorConfiguration"] = TableVersion(1);
	//
	//				//TableGroupKey
	//				//	saveNewTableGroup
	//				//	(const std::string &groupName, const std::map<std::string,
	//				//	TableVersion> &groupMembers, TableGroupKey
	// previousVersion=TableGroupKey()); 				newKey =
	//						saveNewTableGroup(groupName,members);
	//			}
	//
	//			//std::map<std::string, TableVersion >
	//			//	loadConfigurationGroup
	//			//	(const std::string &configGroupName,
	//			//	TableGroupKey configGroupKey, bool doActivate=false, ProgressBar*
	// progressBar=0, std::string *accumulateWarnings=0);
	//			{
	//				std::map<std::string, TableVersion > memberMap =
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
	//				std::map<std::string, TableVersion> members;
	//				members["ARTDAQAggregatorConfiguration"] = TableVersion(1);
	//
	//				//TableGroupKey
	//				//	saveNewTableGroup
	//				//	(const std::string &groupName, const std::map<std::string,
	//				//	TableVersion> &groupMembers, TableGroupKey
	// previousVersion=TableGroupKey()); 				newKey =
	//						saveNewTableGroup(groupName,members);
	//			}
	//
	//			//std::map<std::string, TableVersion >
	//			//	loadConfigurationGroup
	//			//	(const std::string &configGroupName,
	//			//	TableGroupKey configGroupKey, bool doActivate=false, ProgressBar*
	// progressBar=0, std::string *accumulateWarnings=0);
	//			{
	//				std::map<std::string, TableVersion > memberMap =
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
	//			__COUT_ERR__ << "Failed to create table group: " << groupName << ":" <<
	// newKey
	//<< std::endl;
	//			__COUT_ERR__ << "\n\n" << e.what() << std::endl;
	//		}
	//		catch(...)
	//		{
	//			__COUT_ERR__ << "Failed to create table group: " << groupName << ":" <<
	// newKey
	//<< std::endl;
	//		}
	//
	//		return;
	//	}

	//	//test creating table group and reading
	//	{
	//		__COUT__ << std::endl;
	//
	//		auto gcfgs = theInterface_->getAllConfigurationGroupNames();
	//		__COUT__ << "Global table size: " << gcfgs.size() << std::endl;
	//		for(auto &g:gcfgs)
	//		{
	//			__COUT__ << "Global table " << g << std::endl;
	//			auto gcMap = theInterface_->getConfigurationGroupMembers(g, true
	///*getMetaTable*/);
	//
	//			for(auto &cv:gcMap)
	//				__COUT__ << "\tMember table " << cv.first << ":" << cv.second <<
	// std::endl;
	//		}
	//
	//		auto cfgs = theInterface_->getAllConfigurationNames();
	//		__COUT__ << "Sub-table size: " << cfgs.size() << std::endl;
	//		for(auto &c:cfgs)
	//		{
	//			__COUT__ << "table " << c << std::endl;
	//			auto vs = theInterface_->getVersions(getTableByName(c));
	//			for(auto &v:vs)
	//				__COUT__ << "\tversion " << v << std::endl;
	//		}
	//
	//		if(0) //create a global table group (storeGlobalConfiguration)
	//		{
	//			//storeGlobalConfiguration with latest of each
	//			std::map<std::string /*name*/, TableVersion /*version*/> gcMap;
	//
	//			for(auto &c:cfgs) //for each sub-table, take latest version
	//			{
	//				auto vs = theInterface_->getVersions(getTableByName(c));
	//				if(1 && vs.rbegin() != vs.rend()) //create latest!
	//				{
	//					gcMap[c] = *(vs.rbegin());
	//					__COUT__ << "Adding table " << c << ":" << gcMap[c] << std::endl;
	//				}
	//				else if(vs.begin() != vs.end())	//create oldest!
	//				{
	//					gcMap[c] = *(vs.begin());
	//					__COUT__ << "Adding table " << c << ":" << gcMap[c] << std::endl;
	//				}
	//			}
	//
	//			int i = 0;
	//			bool done = false;
	//			char gcname[100];
	//			bool found;
	//			while(!done && i<1)
	//			{
	//				do	//avoid conflicting global table names
	//				{
	//					found = false;
	//					sprintf(gcname,"GConfig_v%d",i++);
	//					for(auto &g:gcfgs)
	//						if(g == gcname) {found = true; break;}
	//				}
	//				while(found);
	//				__COUT__ << "Trying Global table: " << gcname<< std::endl;
	//
	//				try
	//				{
	//					theInterface_->saveConfigurationGroup(gcMap,gcname);
	//					done = true;
	//					__COUT__ << "Created Global table: " << gcname<< std::endl;
	//				}
	//				catch(...) {++i;} //repeat names are not allowed, so increment name
	//			}
	//
	//		}
	//
	//		//return;
	//	}
	//
	// this is to test table tree
	try
	{
		__COUT__ << "Loading table..." << std::endl;
		loadConfigurationGroup("FETest", TableGroupKey(2));  // Context_1
		ConfigurationTree t = getNode("/FEConfiguration/DEFAULT/FrontEndType");

		std::string v;

		__COUT__ << std::endl;
		t.getValue(v);
		__COUT__ << "Value: " << v << std::endl;
		__COUT__ << "Value index: " << t.getValue<int>() << std::endl;

		return;

		// ConfigurationTree t = getNode("XDAQContextConfiguration");
		// ConfigurationTree t = getNode("/FEConfiguration/OtsUDPFSSR3/FrontEndType");
		// ConfigurationTree t = getNode("/FEConfiguration").getNode("OtsUDPFSSR3");
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
		__COUT__ << "Failed to load table..." << std::endl;
	}
}
