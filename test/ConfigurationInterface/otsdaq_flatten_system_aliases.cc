#include <string>
#include <iostream>
#include <memory>
#include <cassert>
#include <dirent.h>
#include "otsdaq-core/ConfigurationInterface/ConfigurationManagerRW.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"
//#include "artdaq-database/StorageProviders/FileSystemDB/provider_filedb_index.h"
//#include "artdaq-database/JsonDocument/JSONDocument.h"

//usage:
// otsdaq_flatten_system_aliases <baseFlatVersion> <pathToSwapIn (optional)>
//
// if baseFlatVersion is invalid or temporary nothing is saved in the new db
//	(Note: this can be used to swap dbs using pathToSwapIn)

using namespace ots;


void FlattenActiveSystemAliasConfigurationGroups(int argc, char* argv[])
{
	std::cout << "=================================================\n";
	std::cout << "=================================================\n";
	std::cout << "=================================================\n";
	std::cout << __COUT_HDR_FL__ << "\nFlattening Active System Aliases!" << std::endl;

	std::cout << "\n\nusage: Two arguments:\n\t pathToSwapIn <baseFlatVersion> <pathToSwapIn (optional)> \n\n" <<
			"\t Default values: baseFlatVersion = 0, pathToSwapIn = \"\" \n\n" <<
			std::endl;

	std::cout << "\n\nNote: you can optionally just swap databases (and not modify their contents at all)" <<
			" by providing an invalid baseFlatVersion of -1.\n\n" <<
			std::endl;

	std::cout << "\n\nNote: This assumes artdaq db file type interface. " <<
			"The current database/ will be moved to database_<linuxtime>/ " <<
			"and if a pathToSwapIn is specified it will be copied to database/ " <<
			"before saving the currently active groups.\n\n" <<
			std::endl;

	std::cout << "argc = " << argc << std::endl;
	for(int i = 0; i < argc; i++)
		std::cout << "argv[" << i << "] = " << argv[i] << std::endl;

	if(argc < 2)
	{
		std::cout << "Must provide at least one parameter.";
		return;
	}

	//determine if "h"elp was first parameter
	std::string flatVersionStr = argv[1];
	if(flatVersionStr.find('h') != std::string::npos)
	{
		std::cout << "Recognized parameter 1. as a 'help' option. Usage was printed. Exiting." << std::endl;
		return;
	}

	int flatVersion = 0;
	std::string pathToSwapIn = "";
	if(argc >= 2)
		sscanf(argv[1],"%d",&flatVersion);
	if(argc >= 3)
		pathToSwapIn = argv[2];

	std::cout << __COUT_HDR_FL__ << "flatVersion = " << flatVersion << std::endl;
	std::cout << __COUT_HDR_FL__ << "pathToSwapIn = " << pathToSwapIn << std::endl;

	//return;
	//==============================================================================
	//Define environment variables
	//	Note: normally these environment variables are set by StartOTS.sh

	//These are needed by otsdaq/otsdaq-core/ConfigurationDataFormats/ConfigurationInfoReader.cc [207]
	setenv("CONFIGURATION_TYPE","File",1);	//Can be File, Database, DatabaseTest
	setenv("CONFIGURATION_DATA_PATH",(std::string(getenv("USER_DATA")) + "/ConfigurationDataExamples").c_str(),1);
	setenv("CONFIGURATION_INFO_PATH",(std::string(getenv("USER_DATA")) + "/ConfigurationInfo").c_str(),1);
	////////////////////////////////////////////////////

	//Some configuration plug-ins use getenv("SERVICE_DATA_PATH") in init() so define it
	setenv("SERVICE_DATA_PATH",(std::string(getenv("USER_DATA")) + "/ServiceData").c_str(),1);

	//Some configuration plug-ins use getenv("OTSDAQ_LIB") and getenv("OTSDAQ_UTILITIES_LIB") in init() so define it
	//	to a non-sense place is ok
	setenv("OTSDAQ_LIB",(std::string(getenv("USER_DATA")) + "/").c_str(),1);
	setenv("OTSDAQ_UTILITIES_LIB",(std::string(getenv("USER_DATA")) + "/").c_str(),1);

	//Some configuration plug-ins use getenv("OTS_MAIN_PORT") in init() so define it
	setenv("OTS_MAIN_PORT","2015",1);

	//also xdaq envs for XDAQContextConfiguration
	setenv("XDAQ_CONFIGURATION_DATA_PATH",(std::string(getenv("USER_DATA")) + "/XDAQConfigurations").c_str(),1);
	setenv("XDAQ_CONFIGURATION_XML","otsConfigurationNoRU_CMake",1);
	////////////////////////////////////////////////////

	//==============================================================================
	//get prepared with initial source db

	//ConfigurationManager instance immediately loads active groups
	std::cout << "\n\n\n" << __COUT_HDR_FL__ << "Loading active Aliases..." << std::endl;
	ConfigurationManagerRW cfgMgrInst("flatten_admin");
	ConfigurationManagerRW *cfgMgr = &cfgMgrInst;


	//create set of groups to persist
	//	include active context
	//	include active backbone
	//	include active config group
	//		(keep key translation separate activeGroupKeys)
	//	include all groups with system aliases

	//for group in set
	//	load/activate group and flatten tables to flatVersion to new DB
	//		save new version to modifiedTables
	//	save group with flatVersion key to new DB
	//		save new key to groupSet
	//	++flatVersion

	//reload the active backbone (using activeGroupKeys)
	//	modify group aliases and table aliases properly based on groupSet and modifiedTables
	//	save new backbone with flatVersion to new DB

	//backup the file ConfigurationManager::ACTIVE_GROUP_FILENAME with time
	// 	and change the ConfigurationManager::ACTIVE_GROUP_FILENAME
	//	to reflect new group names/keys


	/* map<<groupName, origKey>, newKey> */
	std::map<std::pair<std::string,ConfigurationGroupKey>,
		ConfigurationGroupKey> 					groupSet;
	/* <tableName, <origVersion, newVersion> >*/
	std::map<std::pair<std::string,ConfigurationVersion>,
		ConfigurationVersion>					modifiedTables;
	std::map<std::string, std::pair<ConfigurationGroupKey,
		ConfigurationGroupKey>>					activeGroupKeys;
	std::map<std::pair<std::string,ConfigurationGroupKey>,
		std::string> 							groupErrors;

	std::string									activeBackboneTableName = "";
	std::string									activeContextTableName = "";
	std::string									activeConfigTableName = "";

	std::string									nowTime = std::to_string(time(0));

	std::string									thenTime = "";
	if(pathToSwapIn != "") //get target then time
	{
		thenTime = pathToSwapIn.substr(pathToSwapIn.rfind('_')+1);
		std::cout << __COUT_HDR_FL__<< "thenTime = " << thenTime << std::endl;
		//return;
	}

	//add active groups to set
	std::map<std::string, std::pair<std::string, ConfigurationGroupKey>> activeGroupsMap =
			cfgMgr->getActiveConfigurationGroups();

	for(const auto &activeGroup: activeGroupsMap)
	{
		groupSet.insert(std::pair<
				std::pair<std::string,ConfigurationGroupKey>,
				ConfigurationGroupKey> (
						std::pair<std::string,ConfigurationGroupKey>(
								activeGroup.second.first,
								activeGroup.second.second),
								ConfigurationGroupKey())
		);
		activeGroupKeys.insert(std::pair<
				std::string,
				std::pair<ConfigurationGroupKey,ConfigurationGroupKey>> (
						activeGroup.second.first,
						std::pair<ConfigurationGroupKey,ConfigurationGroupKey>(
								activeGroup.second.second,
								ConfigurationGroupKey()))
		);

		if(activeGroup.first == ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE)
		{
			activeBackboneTableName = activeGroup.second.first;
			std::cout << __COUT_HDR_FL__<< "found activeBackboneTableName = " <<
					activeBackboneTableName << std::endl;
		}
		else if(activeGroup.first == ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT)
		{
			activeContextTableName = activeGroup.second.first;
			std::cout << __COUT_HDR_FL__<< "found activeContextTableName = " <<
					activeContextTableName << std::endl;
		}
		else if(activeGroup.first == ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION)
		{
			activeConfigTableName = activeGroup.second.first;
			std::cout << __COUT_HDR_FL__<< "found activeConfigTableName = " <<
					activeConfigTableName << std::endl;
		}
	}

	//add system alias groups to set
	const std::string groupAliasesTableName = "GroupAliasesConfiguration";
	std::map<std::string, ConfigurationVersion> activeVersions = cfgMgr->getActiveVersions();
	if(activeVersions.find(groupAliasesTableName) == activeVersions.end())
	{
		__SS__ << "\nActive version of GroupAliasesConfiguration missing! " <<
				"GroupAliasesConfiguration is a required member of the Backbone configuration group." <<
				"\n\nLikely you need to activate a valid Backbone group." <<
				std::endl;
		throw std::runtime_error(ss.str());
	}


	std::vector<std::pair<std::string,ConfigurationTree> > aliasNodePairs =
			cfgMgr->getNode(groupAliasesTableName).getChildren();
	for(auto& groupPair:aliasNodePairs)
		groupSet.insert(std::pair<
				std::pair<std::string,ConfigurationGroupKey>,
				ConfigurationGroupKey> (
						std::pair<std::string,ConfigurationGroupKey>(
								groupPair.second.getNode("GroupName").getValueAsString(),
								ConfigurationGroupKey(groupPair.second.getNode("GroupKey").getValueAsString())),
								ConfigurationGroupKey())
		);

	std::cout << __COUT_HDR_FL__<< "Identfied groups:" << std::endl;
	for(auto& group:groupSet)
		std::cout << __COUT_HDR_FL__<< group.first.first << " " << group.first.second << std::endl;
	std::cout << __COUT_HDR_FL__<< std::endl;
	std::cout << __COUT_HDR_FL__<< std::endl;

	//return;
	//==============================================================================
	//prepare to manipulate directories
	std::string currentDir = getenv("ARTDAQ_DATABASE_URI");

	if(currentDir.find("filesystemdb://") != 0)
	{
		__SS__ << "filesystemdb:// was not found in $ARTDAQ_DATABASE_URI!" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	currentDir = currentDir.substr(std::string("filesystemdb://").length());
	while(currentDir.length() && currentDir[currentDir.length()-1] == '/') //remove trailing '/'s
		currentDir = currentDir.substr(0,currentDir.length()-1);
	std::string moveToDir = currentDir + "_" + nowTime;
	if(argc < 2)
	{
		__SS__ << ("Aborting move! Must at least give version argument to flatten to!") << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	if(pathToSwapIn != "")
	{
		DIR *dp;
		if((dp = opendir(pathToSwapIn.c_str())) == 0)
		{
			std::cout << __COUT_HDR_FL__<< "ERROR:(" << errno << ").  Can't open directory: " << pathToSwapIn << std::endl;
			exit(0);
		}
		closedir(dp);
	}

	//handle directory swap
	std::cout << __COUT_HDR_FL__ << "Moving current directory: \t" << currentDir << std::endl;
	std::cout << __COUT_HDR_FL__ << "\t... to: \t\t" << moveToDir << std::endl;
	//return;
	rename(currentDir.c_str(),moveToDir.c_str());

	if(pathToSwapIn != "")
	{
		std::cout << __COUT_HDR_FL__ << "Swapping in directory: \t" << pathToSwapIn << std::endl;
		std::cout << __COUT_HDR_FL__ << "\t.. to: \t\t" << currentDir << std::endl;
		rename(pathToSwapIn.c_str(),currentDir.c_str());

		//also swap in active groups file
		//check if original active file exists
		std::string activeGroupsFile = ConfigurationManager::ACTIVE_GROUP_FILENAME + "." + thenTime;
		FILE *fp = fopen(activeGroupsFile.c_str(),"r");
		if(fp)
		{
			std::cout << __COUT_HDR_FL__  << "Swapping active groups file: \t" <<
					activeGroupsFile << std::endl;
			std::cout << __COUT_HDR_FL__ << "\t.. to: \t\t" <<
					ConfigurationManager::ACTIVE_GROUP_FILENAME << std::endl;
			rename(activeGroupsFile.c_str(),
					ConfigurationManager::ACTIVE_GROUP_FILENAME.c_str());
		}
	}





	ConfigurationInterface* theInterface_ = ConfigurationInterface::getInstance(false); //true for File interface, false for artdaq database;
	ConfigurationView* cfgView;
	ConfigurationBase* config;

	bool errDetected;
	std::string accumulateErrors = "";
	std::map<std::string, ConfigurationVersion> memberMap;
	int count = 0;

	//don't do anything more if flatVersion is not persistent
	if(ConfigurationVersion(flatVersion).isInvalid() ||
			ConfigurationVersion(flatVersion).isTemporaryVersion())
	{
		std::cout << __COUT_HDR_FL__ << "\n\nflatVersion " << ConfigurationVersion(flatVersion) <<
				" is an invalid or temporary version. Skipping to end!" << std::endl;
		goto CLEAN_UP;
	}


	for(auto& groupPair:groupSet)
	{
		errDetected = false;

		std::cout << __COUT_HDR_FL__ << "****************************" << std::endl;
		std::cout << __COUT_HDR_FL__ << "Loading members for " <<
				groupPair.first.first <<
				"(" << groupPair.first.second << ")" <<
				std::endl;
		std::cout << __COUT_HDR_FL__ << "flatVersion = " << flatVersion << std::endl;

		//handle directory swap BACK
		if(pathToSwapIn != "")
		{
			std::cout << __COUT_HDR_FL__ << "REVERT by Swapping back directory: \t" << currentDir << std::endl;
			std::cout << __COUT_HDR_FL__ << "\t.. to: \t\t" << pathToSwapIn << std::endl;
			if(rename(currentDir.c_str(),pathToSwapIn.c_str()) < 0)
			{
				__SS__ << "Problem!" << std::endl;
				throw std::runtime_error(ss.str());
			}

		}
		else if(count) //if not first time, move currentDir to temporarily holding area
		{
			std::cout << __COUT_HDR_FL__ << "REVERT by Moving directory: \t" << currentDir << std::endl;
			std::cout << __COUT_HDR_FL__ << "\t.. to temporary directory: \t\t" << (moveToDir+"_tmp") << std::endl;
			if(rename(currentDir.c_str(),(moveToDir+"_tmp").c_str()) < 0)
			{
				__SS__ << "Problem!" << std::endl;
				throw std::runtime_error(ss.str());
			}
		}


		std::cout << __COUT_HDR_FL__ << "REVERT by Moving directory: \t" << moveToDir << std::endl;
		std::cout << __COUT_HDR_FL__ << "\t... to: \t\t" << currentDir << std::endl;
		if(rename(moveToDir.c_str(),currentDir.c_str()) < 0)
		{
			__SS__ << "Problem!" << std::endl;
			throw std::runtime_error(ss.str());
		}


		//=========================
		//load group and tables from original DB
		try
		{
			memberMap =
				cfgMgr->loadConfigurationGroup(
					groupPair.first.first,
					groupPair.first.second,
					true,0,&accumulateErrors);
		}
		catch(std::runtime_error& e)
		{

			std::cout << __COUT_HDR_FL__ << "Error was caught loading members for " <<
					groupPair.first.first <<
					"(" << groupPair.first.second << ")" <<
					std::endl;
			std::cout << __COUT_HDR_FL__ << e.what() << std::endl;
			errDetected = true;
		}
		catch(...)
		{
			std::cout << __COUT_HDR_FL__ << "Error was caught loading members for " <<
					groupPair.first.first <<
					"(" << groupPair.first.second << ")" <<
					std::endl;
			errDetected = true;
		}

		//=========================

		//if(count == 2) break;


		//handle directory swap
		std::cout << __COUT_HDR_FL__ << "Moving current directory: \t" << currentDir << std::endl;
		std::cout << __COUT_HDR_FL__ << "\t... to: \t\t" << moveToDir << std::endl;
		if(rename(currentDir.c_str(),moveToDir.c_str()) < 0)
		{
			__SS__ << "Problem!" << std::endl;
			throw std::runtime_error(ss.str());
		}

		if(pathToSwapIn != "")
		{
			std::cout << __COUT_HDR_FL__ << "Swapping in directory: \t" << pathToSwapIn << std::endl;
			std::cout << __COUT_HDR_FL__ << "\t.. to: \t\t" << currentDir << std::endl;
			if(rename(pathToSwapIn.c_str(),currentDir.c_str()) < 0)
			{
				__SS__ << "Problem!" << std::endl;
				throw std::runtime_error(ss.str());
			}
		}
		else if(count) //if not first time, replace from temporarily holding area
		{
			std::cout << __COUT_HDR_FL__ << "Moving temporary directory: \t" << (moveToDir+"_tmp") << std::endl;
			std::cout << __COUT_HDR_FL__ << "\t.. to current directory: \t\t" << currentDir << std::endl;
			if(rename((moveToDir+"_tmp").c_str(),currentDir.c_str()) < 0)
			{
				__SS__ << "Problem!" << std::endl;
				throw std::runtime_error(ss.str());
			}
		}


		//exit loop if any (loading) failure
		if(errDetected)
		{
			//goto CLEAN_UP;

			//power on if group failed
			//	and record error

			groupErrors.insert(std::pair<
					std::pair<std::string,ConfigurationGroupKey>,
					std::string> (
							std::pair<std::string,ConfigurationGroupKey>(
									groupPair.first.first,
									groupPair.first.second),
									"Error caught loading the group."));
			continue;
		}




		//=========================
		//save group and its tables with new key and versions!
		try
		{
			//saving tables
			for(auto &memberPair:memberMap)
			{
				std::cout << __COUT_HDR_FL__ << memberPair.first << ":v" << memberPair.second << std::endl;

				//check if table has already been modified by a previous group
				//	(i.e. two groups using the same version of a table)
				if(modifiedTables.find(std::pair<std::string,ConfigurationVersion>(
						memberPair.first,
						memberPair.second
						)) != modifiedTables.end())
				{

					std::cout << __COUT_HDR_FL__ << "Table was already modified!" << std::endl;
					memberPair.second = modifiedTables[std::pair<std::string,ConfigurationVersion>(
							memberPair.first,
							memberPair.second
							)];
					std::cout << __COUT_HDR_FL__ << "\t to...\t" <<
							memberPair.first << ":v" << memberPair.second << std::endl;
					continue;
				}

				//change the version of the active view to flatVersion and save it
				config = cfgMgr->getConfigurationByName(memberPair.first);
				cfgView = config->getViewP();
				cfgView->setVersion(ConfigurationVersion(flatVersion));
				theInterface_->saveActiveVersion(config);

				//set it back for the table so that future groups can re-use cached version
				cfgView->setVersion(memberPair.second); //IMPORTANT

				memberPair.second = flatVersion; //change version in the member map

				std::cout << __COUT_HDR_FL__ << "\t to...\t" <<
						memberPair.first << ":v" << memberPair.second << std::endl;

				//save new version to modifiedTables
				modifiedTables.insert(std::pair<
						std::pair<std::string,ConfigurationVersion>,
						ConfigurationVersion> (
								std::pair<std::string,ConfigurationVersion>(
										memberPair.first,
										memberPair.second),
										ConfigurationVersion(flatVersion))
				);
			}

			//memberMap should now consist of members with new flat version, so save
			theInterface_->saveConfigurationGroup(memberMap,
					ConfigurationGroupKey::getFullGroupString(
							groupPair.first.first,
							ConfigurationGroupKey(flatVersion)));

			//and modify groupSet and activeGroupKeys keys
			groupPair.second = ConfigurationGroupKey(flatVersion);

			//if this is an active group, save key change
			if(activeGroupKeys.find(groupPair.first.first) != activeGroupKeys.end() &&
					activeGroupKeys[groupPair.first.first].first == groupPair.first.second)
				activeGroupKeys[groupPair.first.first].second =
						ConfigurationGroupKey(flatVersion);
		}
		catch(std::runtime_error& e)
		{
			std::cout << __COUT_HDR_FL__ << "Error was caught saving group " <<
					groupPair.first.first <<
					" (" << groupPair.first.second << ") " <<
					std::endl;
			std::cout << __COUT_HDR_FL__ << e.what() << std::endl;

			groupErrors.insert(std::pair<
					std::pair<std::string,ConfigurationGroupKey>,
					std::string> (
							std::pair<std::string,ConfigurationGroupKey>(
									groupPair.first.first,
									groupPair.first.second),
									"Error caught saving the group."));
		}
		catch(...)
		{
			std::cout << __COUT_HDR_FL__ << "Error was caught saving group " <<
					groupPair.first.first <<
					" (" << groupPair.first.second << ") " <<
					std::endl;

			groupErrors.insert(std::pair<
					std::pair<std::string,ConfigurationGroupKey>,
					std::string> (
							std::pair<std::string,ConfigurationGroupKey>(
									groupPair.first.first,
									groupPair.first.second),
									"Error caught saving the group."));
		}
		//=========================


		//increment flat version
		++flatVersion;
		++count;
	}

	//record in readme for moveto
	{
		FILE *fp = fopen((moveToDir + "/README_otsdaq_flatten.txt").c_str(),"a");
		if(!fp)
			std::cout << __COUT_HDR_FL__ << "\tError opening README file!" << std::endl;
		else
		{
			time_t rawtime;
			struct tm * timeinfo;
			char buffer [200];

			time (&rawtime);
			timeinfo = localtime (&rawtime);
			strftime (buffer,200,"%b %d, %Y %I:%M%p %Z",timeinfo);

			fprintf(fp,"This database was moved from...\n\t %s \nto...\n\t %s \nat this time \n\t %lu \t %s\n\n\n",
					currentDir.c_str(),moveToDir.c_str(),time(0),buffer);

			fclose(fp);
		}
	}


	//record in readme for swapin
	{
		FILE *fp = fopen((currentDir + "/README_otsdaq_flatten.txt").c_str(),"a");

		if(!fp)
			std::cout << __COUT_HDR_FL__ << "\tError opening README file!" << std::endl;
		else
		{
			time_t rawtime;
			struct tm * timeinfo;
			char buffer [200];

			time (&rawtime);
			timeinfo = localtime (&rawtime);
			strftime (buffer,200,"%b %d, %Y %I:%M:%S%p %Z",timeinfo);

			fprintf(fp,"This database was moved from...\t %s \t to...\t %s at this time \t %lu \t %s\n\n",
					pathToSwapIn.c_str(),currentDir.c_str(),time(0),buffer);
			fclose(fp);
		}
	}

//	//print resulting all groups
//
//	std::cout << "\n\n" << __COUT_HDR_FL__ << "Resulting Groups:" << std::endl;
//	for(const auto &group: groupSet)
//		std::cout << __COUT_HDR_FL__ << group.first.first << ": " <<
//			group.first.second << " => " << group.second << std::endl;
//	std::cout << "\n\n" << __COUT_HDR_FL__ << "Resulting Groups end." << std::endl;
//
//
//	//print resulting active groups
//
//	std::cout << "\n\n" << __COUT_HDR_FL__ << "Resulting Active Groups:" << std::endl;
//	for(const auto &activeGroup: activeGroupKeys)
//		std::cout << __COUT_HDR_FL__ << activeGroup.first << ": " <<
//			activeGroup.second.first << " => " << activeGroup.second.second << std::endl;
//
//	std::cout << __COUT_HDR_FL__ << activeBackboneTableName << " is the " <<
//			ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE << "." << std::endl;
//	std::cout << "\n\n" << __COUT_HDR_FL__ << "Resulting Active Groups end." << std::endl;





	//reload the active backbone (using activeGroupKeys)
	//	modify group aliases and table aliases properly based on groupSet and modifiedTables
	//	save new backbone with flatVersion to new DB

	if(activeBackboneTableName == "")
	{
		std::cout << __COUT_HDR_FL__ << "No active Backbone table identified." << std::endl;
		goto CLEAN_UP;
	}

	std::cout << "\n\n" << __COUT_HDR_FL__ <<
			"Modifying the active Backbone table to reflect new table versions and group keys." <<
			std::endl;

	{
		memberMap =
				cfgMgr->loadConfigurationGroup(
						activeBackboneTableName,
						activeGroupKeys[activeBackboneTableName].second,
						true,0,&accumulateErrors);

		//modify GroupAliasesConfiguration and VersionAliasesConfiguration to point
		//	at DEFAULT and flatVersion respectively

		const std::string groupAliasesName = "GroupAliasesConfiguration";
		const std::string versionAliasesName = "VersionAliasesConfiguration";


		std::map<std::string, ConfigurationVersion> activeMap = cfgMgr->getActiveVersions();

		//modify GroupAliasesConfiguration
		if(activeMap.find(groupAliasesName) != activeMap.end())
		{
			std::cout << __COUT_HDR_FL__ << "\n\nModifying " << groupAliasesName << std::endl;
			config = cfgMgr->getConfigurationByName(groupAliasesName);
			cfgView = config->getViewP();

			unsigned int col1 = cfgView->findCol("GroupName");
			unsigned int col2 = cfgView->findCol("GroupKey");

			//cfgView->print();

			//change all key entries found to the new key and delete rows for groups not found
			bool found;
			for(unsigned int row = 0; row<cfgView->getNumberOfRows(); ++row )
			{
				found = false;
				for(const auto &group: groupSet)
					if(group.second.isInvalid()) continue;
					else if(cfgView->getDataView()[row][col1] == group.first.first &&
							cfgView->getDataView()[row][col2] == group.first.second.toString())
					{
						//found a matching group/key pair
						std::cout << __COUT_HDR_FL__ <<
								"Changing row " << row << " for " <<
								cfgView->getDataView()[row][col1] << " key=" <<
								cfgView->getDataView()[row][col2] << " to NEW key=" <<
								group.second << std::endl;
						cfgView->setValue(
								group.second.toString(),
								row,col2);
						found = true;
						break;
					}

				if(!found) //delete row
					cfgView->deleteRow(row--);
			}
			//cfgView->print();
		}

		//modify VersionAliasesConfiguration
		if(activeMap.find(versionAliasesName) != activeMap.end())
		{
			std::cout << __COUT_HDR_FL__ << "\n\nModifying " << versionAliasesName << std::endl;
			config = cfgMgr->getConfigurationByName(versionAliasesName);
			cfgView = config->getViewP();
			unsigned int col1 = cfgView->findCol("ConfigurationName");
			unsigned int col2 = cfgView->findCol("Version");

			//change all version entries to the new version and delete rows with no match
			bool found;
			for(unsigned int row = 0; row<cfgView->getNumberOfRows(); ++row )
			{
				found = false;
				for(const auto &table:modifiedTables)
					if(cfgView->getDataView()[row][col1] == table.first.first &&
							cfgView->getDataView()[row][col2] == table.first.second.toString())
					{
						//found a matching group/key pair
						std::cout << __COUT_HDR_FL__ << "Changing row " << row << " for " <<
								cfgView->getDataView()[row][col1] << " version=" <<
								cfgView->getDataView()[row][col2] << " to NEW version=" <<
								table.second << std::endl;
						cfgView->setValue(table.second.toString(),row,col2);
						found = true;
						break;
					}

				if(!found) //delete row
					cfgView->deleteRow(row--);
			}
		}



		//save new "GroupAliasesConfiguration" and "VersionAliasesConfiguration"

		std::cout << __COUT_HDR_FL__ << groupAliasesName << ":v" <<
				memberMap[groupAliasesName] << std::endl;
		//change the version of the active view to flatVersion and save it
		config = cfgMgr->getConfigurationByName(groupAliasesName);
		cfgView = config->getViewP();
		cfgView->setVersion(ConfigurationVersion(flatVersion));
		theInterface_->saveActiveVersion(config);

		memberMap[groupAliasesName] = flatVersion; //change version in the member map

		std::cout << __COUT_HDR_FL__ << "\t to...\t" <<
				groupAliasesName << ":v" << memberMap[groupAliasesName] << std::endl;



		std::cout << __COUT_HDR_FL__ << versionAliasesName << ":v" <<
				memberMap[versionAliasesName] << std::endl;
		//change the version of the active view to flatVersion and save it
		config = cfgMgr->getConfigurationByName(versionAliasesName);
		cfgView = config->getViewP();
		cfgView->setVersion(ConfigurationVersion(flatVersion));
		theInterface_->saveActiveVersion(config);

		memberMap[versionAliasesName] = flatVersion; //change version in the member map

		std::cout << __COUT_HDR_FL__ << "\t to...\t" <<
				versionAliasesName << ":v" << memberMap[versionAliasesName] << std::endl;

		//memberMap should now consist of members with new flat version, so save
		theInterface_->saveConfigurationGroup(memberMap,
				ConfigurationGroupKey::getFullGroupString(
						activeBackboneTableName,
						ConfigurationGroupKey(flatVersion)));

		activeGroupKeys[activeBackboneTableName].second =
								ConfigurationGroupKey(flatVersion);

		std::cout << __COUT_HDR_FL__ << "New to-be-active backbone group " << activeBackboneTableName <<
				":v" << activeGroupKeys[activeBackboneTableName].second << std::endl;
	}



	//backup the file ConfigurationManager::ACTIVE_GROUP_FILENAME with time
	// 	and change the ConfigurationManager::ACTIVE_GROUP_FILENAME
	//	to reflect new group names/keys

	{
		std::cout << "\n\n" << __COUT_HDR_FL__ << "Manipulating the Active Groups file..." << std::endl;

		//check if original active file exists
		FILE *fp = fopen(ConfigurationManager::ACTIVE_GROUP_FILENAME.c_str(),"r");
		if(!fp)
		{
			__SS__ << "Original active groups file '" << ConfigurationManager::ACTIVE_GROUP_FILENAME
					<< "' not found." << std::endl;
			goto CLEAN_UP;
		}

		std::cout << __COUT_HDR_FL__ << "Backing up file: " << ConfigurationManager::ACTIVE_GROUP_FILENAME
				<< std::endl;

		fclose(fp);

		std::string renameFile = ConfigurationManager::ACTIVE_GROUP_FILENAME + "." + nowTime;
		rename(ConfigurationManager::ACTIVE_GROUP_FILENAME.c_str(),
				renameFile.c_str());

		std::cout << __COUT_HDR_FL__ << "Backup file name: " << renameFile
				<< std::endl;

		ConfigurationGroupKey         	*theConfigurationGroupKey_,	*theContextGroupKey_, 	*theBackboneGroupKey_;
		std::string						theConfigurationGroup_, 	theContextGroup_, 		theBackboneGroup_;

		theConfigurationGroup_ = activeConfigTableName;
		theConfigurationGroupKey_ = &(activeGroupKeys[activeConfigTableName].second);

		theContextGroup_ = activeContextTableName;
		theContextGroupKey_ = &(activeGroupKeys[activeContextTableName].second);

		theBackboneGroup_ = activeBackboneTableName;
		theBackboneGroupKey_ = &(activeGroupKeys[activeBackboneTableName].second);


		//the following is copied from ConfigurationManagerRW::activateConfigurationGroup
		{
			std::cout << __COUT_HDR_FL__ << "Updating persistent active groups to " <<
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
	}

	//print resulting all groups

	std::cout << "\n\n" << __COUT_HDR_FL__ << "Resulting Groups:" << std::endl;
	for(const auto &group: groupSet)
		std::cout << __COUT_HDR_FL__ << "\t" << group.first.first << ": " <<
			group.first.second << " => " << group.second << std::endl;
	std::cout << "\n\n" << __COUT_HDR_FL__ << "Resulting Groups end." << std::endl;


	//print resulting active groups

	std::cout << "\n\n" << __COUT_HDR_FL__ << "Resulting Active Groups:" << std::endl;
	for(const auto &activeGroup: activeGroupKeys)
		std::cout << __COUT_HDR_FL__ << "\t" << activeGroup.first << ": " <<
			activeGroup.second.first << " => " << activeGroup.second.second << std::endl;

	std::cout << __COUT_HDR_FL__ << activeBackboneTableName << " is the " <<
			ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE << "." << std::endl;
	std::cout << "\n\n" << __COUT_HDR_FL__ << "Resulting Active Groups end." << std::endl;


CLEAN_UP:
	//==============================================================================
	std::cout << "\n\n" << __COUT_HDR_FL__ << "End of Flattening Active Configuration Groups!\n\n\n" << std::endl;



	std::cout << __COUT_HDR_FL__ << "****************************" << std::endl;
	std::cout << __COUT_HDR_FL__ << "There were " << groupSet.size() <<
			" groups considered, and there were errors handling " << groupErrors.size() <<
			" of those groups." << std::endl;
	std::cout << __COUT_HDR_FL__ << "The following errors were found handling the groups:" << std::endl;
	for(auto& groupErr:groupErrors)
		std::cout << __COUT_HDR_FL__ << "\t" << groupErr.first.first << " " << groupErr.first.second <<
			": \t" << groupErr.second << std::endl;
	std::cout << __COUT_HDR_FL__ << "End of errors.\n\n" << std::endl;


	std::cout << __COUT_HDR_FL__ << "Run the following to return to your previous database structure:" <<
			std::endl;
	std::cout << __COUT_HDR_FL__ << "\t otsdaq_flatten_system_aliases -1 " << moveToDir <<
			"\n\n" << std::endl;


	return;
}

int main(int argc, char* argv[])
{
	FlattenActiveSystemAliasConfigurationGroups(argc,argv);
	return 0;
}
//BOOST_AUTO_TEST_SUITE_END()


