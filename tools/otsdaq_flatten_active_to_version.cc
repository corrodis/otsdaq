#include <dirent.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManagerRW.h"
//#include "artdaq-database/StorageProviders/FileSystemDB/provider_filedb_index.h"
//#include "artdaq-database/JsonDocument/JSONDocument.h"

// usage:
// otsdaq_flatten_active_to_version <flatVersion> <pathToSwapIn (optional)>
//
// if flatVersion is invalid or temporary nothing is saved in the new db
//	(Note: this can be used to swap dbs using pathToSwapIn)

using namespace ots;

void FlattenActiveTableGroups(int argc, char* argv[])
{
	std::cout << "=================================================\n";
	std::cout << "=================================================\n";
	std::cout << "=================================================\n";
	__COUT__ << "\nFlattening Active Table Groups!"
	          << std::endl;

	std::cout << "\n\nusage: Two arguments:\n\t pathToSwapIn <flatVersion> <pathToSwapIn "
	             "(optional)> \n\n"
	          << "\t Default values: flatVersion = 0, pathToSwapIn = \"\" \n\n"
	          << std::endl;

	std::cout << "\n\nNote: you can optionally just swap databases (and not modify their "
	             "contents at all)"
	          << " by providing an invalid flatVersion of -1.\n\n"
	          << std::endl;

	std::cout << "\n\nNote: This assumes artdaq db file type interface. "
	          << "The current database/ will be moved to database_<linuxtime>/ "
	          << "and if a pathToSwapIn is specified it will be copied to database/ "
	          << "before saving the currently active groups.\n\n"
	          << std::endl;

	std::cout << "argc = " << argc << std::endl;
	for(int i = 0; i < argc; i++)
		std::cout << "argv[" << i << "] = " << argv[i] << std::endl;

	if(argc < 2)
	{
		std::cout << "Must provide at least one parameter.";
		return;
	}

	// determine if "h"elp was first parameter
	std::string flatVersionStr = argv[1];
	if(flatVersionStr.find('h') != std::string::npos)
	{
		std::cout
		    << "Recognized parameter 1. as a 'help' option. Usage was printed. Exiting."
		    << std::endl;
		return;
	}

	int         flatVersion  = 0;
	std::string pathToSwapIn = "";
	if(argc >= 2)
		sscanf(argv[1], "%d", &flatVersion);
	if(argc >= 3)
		pathToSwapIn = argv[2];

	__COUT__ << "flatVersion = " << flatVersion << std::endl;
	__COUT__ << "pathToSwapIn = " << pathToSwapIn << std::endl;

	//==============================================================================
	// Define environment variables
	//	Note: normally these environment variables are set by StartOTS.sh

	// These are needed by
	// otsdaq/otsdaq-core/ConfigurationDataFormats/ConfigurationInfoReader.cc [207]
	setenv("CONFIGURATION_TYPE", "File", 1);  // Can be File, Database, DatabaseTest
	setenv("CONFIGURATION_DATA_PATH",
	       (std::string(getenv("USER_DATA")) + "/ConfigurationDataExamples").c_str(),
	       1);
	setenv(
	    "TABLE_INFO_PATH", (std::string(getenv("USER_DATA")) + "/TableInfo").c_str(), 1);
	////////////////////////////////////////////////////

	// Some configuration plug-ins use getenv("SERVICE_DATA_PATH") in init() so define it
	setenv("SERVICE_DATA_PATH",
	       (std::string(getenv("USER_DATA")) + "/ServiceData").c_str(),
	       1);

	// also xdaq envs for XDAQContextTable
	setenv("XDAQ_CONFIGURATION_DATA_PATH",
	       (std::string(getenv("USER_DATA")) + "/XDAQConfigurations").c_str(),
	       1);
	setenv("XDAQ_CONFIGURATION_XML", "otsConfigurationNoRU_CMake", 1);
	////////////////////////////////////////////////////

	//==============================================================================
	// get prepared with initial source db

	// ConfigurationManager instance immediately loads active groups
	ConfigurationManagerRW  cfgMgrInst("flatten_admin");
	ConfigurationManagerRW* cfgMgr = &cfgMgrInst;

	__COUT__ << "\n\n\nLoading activeGroupsMap..." << std::endl;

	// save group members to reform groups
	std::map<std::string, std::pair<std::string, TableGroupKey> > activeGroupsMap =
	    cfgMgr->getActiveTableGroups();

	std::map<std::string, std::map<std::string, TableVersion> > activeGroupMembersMap;
	std::map<std::string, std::map<std::string /*name*/, std::string /*alias*/> >
	                                   activeGroupAliasesMap;
	std::map<std::string, std::string> activeGroupCommentMap;
	std::map<std::string, std::string> activeGroupAuthorMap;
	std::string                        groupCreateTime;
	std::map<std::string, time_t>      activeGroupCreateTimeMap;
	TableBase*                         groupMetadataTable = cfgMgr->getMetadataTable();

	for(auto& activeGroupPair : activeGroupsMap)
	{
		if(activeGroupPair.second.second.isInvalid())
		{
			__COUT__ << "Skipping invalid " << activeGroupPair.first
			          << std::endl;
			continue;
		}

		__COUT__ << "Loading members for " << activeGroupPair.first
		          << "\t" << activeGroupPair.second.first << "("
		          << activeGroupPair.second.second << ")" << std::endl;

		//		//old way before metadata
		//		activeGroupMembersMap[activeGroupPair.second.first] =
		//				cfgMgr->getConfigurationInterface()->getTableGroupMembers(
		//						TableGroupKey::getFullGroupString(
		//								activeGroupPair.second.first,activeGroupPair.second.second));

		//=========================
		// load group, group metadata, and tables from original DB
		try
		{
			cfgMgr->loadTableGroup(
			    activeGroupPair.second.first,
			    activeGroupPair.second.second,
			    true /*doActivate*/,
			    &activeGroupMembersMap[activeGroupPair.second.first] /*memberMap*/,
			    0 /*progressBar*/,
			    &accumulateErrors,
			    &activeGroupCommentMap[activeGroupPair.second.first],
			    &activeGroupAuthorMap[activeGroupPair.second.first],
			    &groupCreateTime,
			    false /*doNotLoadMember*/,
			    0 /*groupTypeString*/,
			    &activeGroupAliasesMap[activeGroupPair.second.first]);
			sscanf(groupCreateTime.c_str(),
			       "%ld",
			       &activeGroupCreateTimeMap[activeGroupPair.second.first]);
		}
		catch(std::runtime_error& e)
		{
			__COUT__ << "Error was caught loading members for " << groupPair.first.first
			         << "(" << groupPair.first.second << ")" << std::endl;
			__COUT__ << e.what() << std::endl;
			errDetected = true;
		}
		catch(...)
		{
			__COUT__ << "Error was caught loading members for " << groupPair.first.first
			         << "(" << groupPair.first.second << ")" << std::endl;
			errDetected = true;
		}

		//=========================
	}

	//==============================================================================
	// manipulate directories
	std::string currentDir = getenv("ARTDAQ_DATABASE_URI");

	if(currentDir.find("filesystemdb://") != 0)
	{
		__SS__ << "filesystemdb:// was not found in $ARTDAQ_DATABASE_URI!" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}

	currentDir = currentDir.substr(std::string("filesystemdb://").length());
	while(currentDir.length() &&
	      currentDir[currentDir.length() - 1] == '/')  // remove trailing '/'s
		currentDir = currentDir.substr(0, currentDir.length() - 1);
	std::string moveToDir = currentDir + "_" + std::to_string(time(0));

	__COUT__ << "Moving current directory: \t" << currentDir
	          << std::endl;
	__COUT__ << "\t... to: \t\t" << moveToDir << std::endl;

	if(argc < 2)
	{
		__SS__ << ("Aborting move! Must at least give version argument to flatten to!")
		       << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}

	rename(currentDir.c_str(), moveToDir.c_str());
	FILE* fp = fopen((moveToDir + "/README_otsdaq_flatten.txt").c_str(), "a");
	if(!fp)
		__COUT__ << "\tError opening README file!" << std::endl;
	else
	{
		time_t     rawtime;
		struct tm* timeinfo;
		char       buffer[200];

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(buffer, 200, "%b %d, %Y %I:%M%p %Z", timeinfo);

		fprintf(fp,
		        "This database was moved from...\n\t %s \nto...\n\t %s \nat this time "
		        "\n\t %lu \t %s\n\n\n",
		        currentDir.c_str(),
		        moveToDir.c_str(),
		        time(0),
		        buffer);

		fclose(fp);
	}

	if(pathToSwapIn != "")
	{
		DIR* dp;
		if((dp = opendir(pathToSwapIn.c_str())) == 0)
		{
			__COUT__ << "ERROR:(" << errno
			          << ").  Can't open directory: " << pathToSwapIn << std::endl;
			exit(0);
		}
		closedir(dp);

		__COUT__ << "Swapping in directory: \t" << pathToSwapIn
		          << std::endl;
		__COUT__ << "\t.. to: \t\t" << currentDir << std::endl;

		rename(pathToSwapIn.c_str(), currentDir.c_str());
		FILE* fp = fopen((currentDir + "/README_otsdaq_flatten.txt").c_str(), "a");
		if(!fp)
			__COUT__ << "\tError opening README file!" << std::endl;
		else
		{
			time_t     rawtime;
			struct tm* timeinfo;
			char       buffer[200];

			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(buffer, 200, "%b %d, %Y %I:%M:%S%p %Z", timeinfo);

			fprintf(fp,
			        "This database was moved from...\t %s \t to...\t %s at this time \t "
			        "%lu \t %s\n\n",
			        pathToSwapIn.c_str(),
			        currentDir.c_str(),
			        time(0),
			        buffer);
			fclose(fp);
		}
	}

	//==============================================================================
	// save current active versions with flatVersion
	// Note: do not try this at home kids.
	ConfigurationInterface* theInterface_ = ConfigurationInterface::getInstance(
	    false);  // true for File interface, false for artdaq database;
	TableView* cfgView;
	TableBase* config;

	std::map<std::string, TableVersion> activeMap = cfgMgr->getActiveVersions();

	// modify Group Aliases Table and Version Aliases Table to point
	//	at DEFAULT and flatVersion respectively

	const std::string groupAliasesName = ConfigurationManager::GROUP_ALIASES_TABLE_NAME;
	const std::string versionAliasesName =
	    ConfigurationManager::VERSION_ALIASES_TABLE_NAME;

	// don't do anything more if flatVersion is not persistent
	if(TableVersion(flatVersion).isInvalid() ||
	   TableVersion(flatVersion).isTemporaryVersion())
	{
		__COUT__ << "\n\nflatVersion " << TableVersion(flatVersion)
		          << " is an invalid or temporary version. Skipping to end!" << std::endl;
		goto CLEAN_UP;
	}

	// modify Group Aliases Table
	if(activeMap.find(groupAliasesName) != activeMap.end())
	{
		__COUT__ << "\n\nModifying " << groupAliasesName << std::endl;
		config  = cfgMgr->getTableByName(groupAliasesName);
		cfgView = config->getViewP();

		unsigned int col1 = cfgView->findCol("GroupName");
		unsigned int col2 = cfgView->findCol("GroupKey");

		// change all key entries to active groups to DEFAULT
		for(unsigned int row = 0; row < cfgView->getNumberOfRows(); ++row)
			for(auto& activeGroupPair : activeGroupsMap)
				if(activeGroupPair.second.second.isInvalid())
					continue;
				else if(cfgView->getDataView()[row][col1] ==
				            activeGroupPair.second.first &&
				        cfgView->getDataView()[row][col2] ==
				            activeGroupPair.second.second.toString())
				{
					// found a matching group/key pair
					__COUT__ << "Changing row " << row << " for "
					          << cfgView->getDataView()[row][col1]
					          << " key=" << cfgView->getDataView()[row][col2]
					          << " to DEFAULT="
					          << TableGroupKey(TableGroupKey::getDefaultKey())
					          << std::endl;
					cfgView->setValue(
					    TableGroupKey(TableGroupKey::getDefaultKey()).toString(),
					    row,
					    col2);
					break;
				}
	}

	// modify Version Aliases Table
	if(activeMap.find(versionAliasesName) != activeMap.end())
	{
		__COUT__ << "\n\nModifying " << versionAliasesName
		          << std::endl;
		config            = cfgMgr->getTableByName(versionAliasesName);
		cfgView           = config->getViewP();
		unsigned int col1 = cfgView->findCol("TableName");
		unsigned int col2 = cfgView->findCol("Version");

		// change all version entries to active tables to flatVersion
		for(unsigned int row = 0; row < cfgView->getNumberOfRows(); ++row)
			for(auto& activePair : activeMap)
				if(cfgView->getDataView()[row][col1] == activePair.first &&
				   cfgView->getDataView()[row][col2] == activePair.second.toString())
				{
					// found a matching group/key pair
					__COUT__ << "Changing row " << row << " for "
					          << cfgView->getDataView()[row][col1]
					          << " version=" << cfgView->getDataView()[row][col2]
					          << " to flatVersion=" << TableVersion(flatVersion)
					          << std::endl;
					cfgView->setValue(TableVersion(flatVersion).toString(), row, col2);
					break;
				}
	}

	__COUT__ << "\n\nChanging versions... " << std::endl;

	for(auto& activePair : activeMap)
	{
		__COUT__ << activePair.first << ":v" << activePair.second
		          << std::endl;
		// change the version of the active view to flatVersion and save it
		config  = cfgMgr->getTableByName(activePair.first);
		cfgView = config->getViewP();
		cfgView->setVersion(TableVersion(flatVersion));
		theInterface_->saveActiveVersion(config);
	}

	//==============================================================================
	// save the active groups with the new member flatVersions
	// Note: do not try this at home kids.
	for(auto& activeGroupMembersPair : activeGroupMembersMap)
	{
		__COUT__ << "Group " << activeGroupMembersPair.first
		          << std::endl;

		for(auto& groupMemberPair : activeGroupMembersPair.second)
		{
			__COUT__ << "\t from...\t" << groupMemberPair.first
			          << ":v" << groupMemberPair.second << std::endl;
			groupMemberPair.second = flatVersion;
		}

		for(auto& groupMemberPair : activeGroupMembersPair.second)
		{
			__COUT__ << "\t to...\t" << groupMemberPair.first << ":v"
			          << groupMemberPair.second << std::endl;
		}

		// Note: this code copies actions in ConfigurationManagerRW::saveNewTableGroup

		// add meta data
		__COUTV__(StringMacros::mapToString(
		    activeGroupAliasesMap[activeGroupMembersPair.first]));
		__COUTV__(activeGroupCommentMap[activeGroupMembersPair.first]);
		__COUTV__(activeGroupAuthorMap[activeGroupMembersPair.first]);
		__COUTV__(activeGroupCreateTimeMap[activeGroupMembersPair.first]);

		// to compensate for unusual errors upstream, make sure the metadata table has one
		// row
		while(groupMetadataTable->getViewP()->getNumberOfRows() > 1)
			groupMetadataTable->getViewP()->deleteRow(0);
		if(groupMetadataTable->getViewP()->getNumberOfRows() == 0)
			groupMetadataTable->getViewP()->addRow();

		// columns are uid,comment,author,time
		// ConfigurationManager::METADATA_COL_ALIASES TODO
		groupMetadataTable->getViewP()->setValue(
		    StringMacros::mapToString(activeGroupAliasesMap[activeGroupMembersPair.first],
		                              "," /*primary delimiter*/,
		                              ":" /*secondary delimeter*/),
		    0,
		    ConfigurationManager::METADATA_COL_ALIASES);
		groupMetadataTable->getViewP()->setValue(
		    activeGroupCommentMap[activeGroupMembersPair.first],
		    0,
		    ConfigurationManager::METADATA_COL_COMMENT);
		groupMetadataTable->getViewP()->setValue(
		    activeGroupAuthorMap[activeGroupMembersPair.first],
		    0,
		    ConfigurationManager::METADATA_COL_AUTHOR);
		groupMetadataTable->getViewP()->setValue(
		    activeGroupCreateTimeMap[activeGroupMembersPair.first],
		    0,
		    ConfigurationManager::METADATA_COL_TIMESTAMP);

		// set version of metadata table
		groupMetadataTable->getViewP()->setVersion(TableVersion(flatVersion));
		theInterface_->saveActiveVersion(groupMetadataTable);

		// force groupMetadataTable_ to be a member for the group
		activeGroupMembersPair.second[groupMetadataTable->getTableName()] =
		    groupMetadataTable->getViewVersion();

		// memberMap should now consist of members with new flat version, so save group
		theInterface_->saveTableGroup(
		    activeGroupMembersPair.second,
		    TableGroupKey::getFullGroupString(
		        activeGroupMembersPair.first,
		        TableGroupKey(TableGroupKey::getDefaultKey())));
	}

CLEAN_UP:
	//==============================================================================
	__COUT__
	          << "\n\nEnd of Flattening Active Table Groups!\n\n\n"
	          << std::endl;

	__COUT__
	          << "Run the following to return to your previous database structure:"
	          << std::endl;
	__COUT__ << "\t otsdaq_flatten_active_to_version -1 " << moveToDir
	          << "\n\n"
	          << std::endl;

	return;
}

int main(int argc, char* argv[])
{
	FlattenActiveTableGroups(argc, argv);
	return 0;
}
// BOOST_AUTO_TEST_SUITE_END()
