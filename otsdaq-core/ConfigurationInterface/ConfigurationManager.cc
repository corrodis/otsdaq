#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"  //All configurable objects are included here
#include "otsdaq-core/ProgressBar/ProgressBar.h"

#include <fstream>  // std::ofstream

#include "otsdaq-core/TableCore/TableGroupKey.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "ConfigurationManager"

const std::string ConfigurationManager::READONLY_USER = "READONLY_USER";

const std::string ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME = "XDAQContextTable";
const std::string ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME =
    "XDAQApplicationTable";
const std::string ConfigurationManager::GROUP_ALIASES_TABLE_NAME = "GroupAliasesTable";
const std::string ConfigurationManager::VERSION_ALIASES_TABLE_NAME =
    "VersionAliasesTable";

// added env check for otsdaq_flatten_active_to_version to function
const std::string ConfigurationManager::ACTIVE_GROUPS_FILENAME =
    ((getenv("SERVICE_DATA_PATH") == NULL)
         ? (std::string(getenv("USER_DATA")) + "/ServiceData")
         : (std::string(getenv("SERVICE_DATA_PATH")))) +
    "/ActiveTableGroups.cfg";
const std::string ConfigurationManager::ALIAS_VERSION_PREAMBLE = "ALIAS:";
const std::string ConfigurationManager::SCRATCH_VERSION_ALIAS  = "Scratch";

const std::string ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT       = "Context";
const std::string ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE      = "Backbone";
const std::string ConfigurationManager::ACTIVE_GROUP_NAME_ITERATE       = "Iterate";
const std::string ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION = "Configuration";
const std::string ConfigurationManager::ACTIVE_GROUP_NAME_UNKNOWN       = "Unknown";

const uint8_t ConfigurationManager::METADATA_COL_ALIASES   = 1;
const uint8_t ConfigurationManager::METADATA_COL_COMMENT   = 2;
const uint8_t ConfigurationManager::METADATA_COL_AUTHOR    = 3;
const uint8_t ConfigurationManager::METADATA_COL_TIMESTAMP = 4;

const std::set<std::string> ConfigurationManager::contextMemberNames_ = {
    ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME,
    ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME,
    "XDAQApplicationPropertyTable",
    "DesktopIconTable",
    "MessageFacilityTable",
    "GatewaySupervisorTable",
    "StateMachineTable",
    "DesktopWindowParameterTable"};
const std::set<std::string> ConfigurationManager::backboneMemberNames_ = {
    ConfigurationManager::GROUP_ALIASES_TABLE_NAME,
    ConfigurationManager::VERSION_ALIASES_TABLE_NAME};
const std::set<std::string> ConfigurationManager::iterateMemberNames_ = {
    "IterateTable",
    "IterationPlanTable",
    "IterationTargetTable",
    /*command specific tables*/ "IterationCommandBeginLabelTable",
    "IterationCommandChooseFSMTable",
    "IterationCommandConfigureAliasTable",
    "IterationCommandConfigureGroupTable",
    "IterationCommandExecuteFEMacroTable",
    "IterationCommandExecuteMacroTable",
    "IterationCommandMacroDimensionalLoopTable",
    "IterationCommandMacroDimensionalLoopParameterTable",
    "IterationCommandModifyGroupTable",
    "IterationCommandRepeatLabelTable",
    "IterationCommandRunTable"};

//==============================================================================
ConfigurationManager::ConfigurationManager()
    : username_(ConfigurationManager::READONLY_USER)
    , theInterface_(0)
    , theConfigurationTableGroupKey_(0)
    , theContextTableGroupKey_(0)
    , theBackboneTableGroupKey_(0)
    , theConfigurationTableGroup_("")
    , theContextTableGroup_("")
    , theBackboneTableGroup_("")
{
	theInterface_ = ConfigurationInterface::getInstance(false);  // false to use artdaq DB
	// NOTE: in ConfigurationManagerRW using false currently.. think about consistency!
	// FIXME

	// initialize special group metadata table
	{
		// Note: "TableGroupMetadata" should never be in conflict
		//	because all other tables end in "...Table"

		// This is a table called TableGroupMetadata
		//	with 4 fields:
		//		- GroupAliases
		//		- GroupAuthor
		//		- GroupCreationTime
		//		- CommentDescription

		groupMetadataTable_.setTableName(
		    ConfigurationInterface::GROUP_METADATA_TABLE_NAME);
		std::vector<TableViewColumnInfo>* colInfo =
		    groupMetadataTable_.getMockupViewP()->getColumnsInfoP();

		colInfo->push_back(TableViewColumnInfo(
		    TableViewColumnInfo::TYPE_UID,  // just to make init() happy
		    "UnusedUID",
		    "UNUSED_UID",
		    TableViewColumnInfo::DATATYPE_NUMBER,
		    "",
		    0));
		colInfo->push_back(TableViewColumnInfo(TableViewColumnInfo::TYPE_DATA,
		                                       "GroupAliases",
		                                       "GROUP_ALIASES",
		                                       TableViewColumnInfo::DATATYPE_STRING,
		                                       "",
		                                       0));
		colInfo->push_back(TableViewColumnInfo(
		    TableViewColumnInfo::TYPE_COMMENT,  // just to make init() happy
		    "CommentDescription",
		    "COMMENT_DESCRIPTION",
		    TableViewColumnInfo::DATATYPE_STRING,
		    "",
		    0));
		colInfo->push_back(TableViewColumnInfo(
		    TableViewColumnInfo::TYPE_AUTHOR,  // just to make init() happy
		    "GroupAuthor",
		    "AUTHOR",
		    TableViewColumnInfo::DATATYPE_STRING,
		    "",
		    0));
		colInfo->push_back(TableViewColumnInfo(TableViewColumnInfo::TYPE_TIMESTAMP,
		                                       "GroupCreationTime",
		                                       "GROUP_CREATION_TIME",
		                                       TableViewColumnInfo::DATATYPE_TIME,
		                                       "",
		                                       0));
		auto tmpVersion = groupMetadataTable_.createTemporaryView();
		groupMetadataTable_.setActiveView(tmpVersion);
		// only need this one and only row for all time
		groupMetadataTable_.getViewP()->addRow();
	}

	init();
} //end constructor()

//==============================================================================
ConfigurationManager::ConfigurationManager(const std::string& username)
    : ConfigurationManager()
{
	username_ = username;
} //end constructor(username)

//==============================================================================
ConfigurationManager::~ConfigurationManager() { destroy(); }

//==============================================================================
// init
//	if accumulatedErrors is not null.. fill it with errors
//	else throw errors (but do not ask restoreActiveTableGroups to throw errors)
void ConfigurationManager::init(std::string* accumulatedErrors)
{
	if(accumulatedErrors)
		*accumulatedErrors = "";

	// destroy();

	// once Interface is false (using artdaq db) .. then can cTableGroupce_->getMode() == false)
	{
		try
		{
			restoreActiveTableGroups(accumulatedErrors ? true : false);
		}
		catch(std::runtime_error& e)
		{
			if(accumulatedErrors)
				*accumulatedErrors = e.what();
			else
				throw;
		}
	}
} //end init()

//==============================================================================
// restoreActiveTableGroups
//	load the active groups from file
//	Note: this should be used by the Supervisor to maintain
//		the same configurationGroups surviving software system restarts
void ConfigurationManager::restoreActiveTableGroups(
    bool throwErrors, const std::string& pathToActiveGroupsFile)
{
	destroyTableGroup("", true);  // deactivate all

	std::string fn =
	    pathToActiveGroupsFile == "" ? ACTIVE_GROUPS_FILENAME : pathToActiveGroupsFile;
	FILE* fp = fopen(fn.c_str(), "r");

	__COUT__ << "ACTIVE_GROUPS_FILENAME = " << fn << __E__;
	__COUT__ << "ARTDAQ_DATABASE_URI = " << std::string(getenv("ARTDAQ_DATABASE_URI"))
	         << __E__;

	if(!fp)
	{
		__COUT_WARN__ << "No active groups file found at " << fn << __E__;
		return;
	}

	//__COUT__ << "throwErrors: " << throwErrors << __E__;

	char tmp[500];
	char strVal[500];

	std::string groupName;
	std::string errorStr = "";
	bool        skip;

	__SS__;

	while(fgets(tmp, 500, fp))  // for(int i=0;i<4;++i)
	{
		// fgets(tmp,500,fp);

		skip = false;
		sscanf(tmp, "%s", strVal);  // sscanf to remove '\n'
		for(unsigned int j = 0; j < strlen(strVal); ++j)
			if(!((strVal[j] >= 'a' && strVal[j] <= 'z') ||
			     (strVal[j] >= 'A' && strVal[j] <= 'Z') ||
			     (strVal[j] >= '0' && strVal[j] <= '9')))
			{
				strVal[j] = '\0';
				__COUT_INFO__ << "Illegal character found, so skipping!" << __E__;

				skip = true;
				break;
			}

		if(skip)
			continue;

		groupName = strVal;
		fgets(tmp, 500, fp);
		sscanf(tmp, "%s", strVal);  // sscanf to remove '\n'

		for(unsigned int j = 0; j < strlen(strVal); ++j)
			if(!((strVal[j] >= '0' && strVal[j] <= '9')))
			{
				strVal[j] = '\0';

				if(groupName.size() > 3)  // notify if seems like a real group name
					__COUT_INFO__
					    << "Skipping active group with illegal character in name."
					    << __E__;

				skip = true;
				break;
			}

		if(skip)
			continue;

		try
		{
			TableGroupKey::getFullGroupString(groupName, TableGroupKey(strVal));
		}
		catch(...)
		{
			__COUT__ << "illegal group according to TableGroupKey::getFullGroupString..."
			         << __E__;
			skip = true;
		}

		if(skip)
			continue;

		try
		{
			// load and doActivate
			loadTableGroup(groupName, TableGroupKey(strVal), true);
		}
		catch(std::runtime_error& e)
		{
			ss << "Failed to load group in ConfigurationManager::init() with name '"
			   << groupName << "(" << strVal << ")'" << __E__;
			ss << e.what() << __E__;

			errorStr += ss.str();
		}
		catch(...)
		{
			ss << "Failed to load group in ConfigurationManager::init() with name '"
			   << groupName << "(" << strVal << ")'" << __E__;

			errorStr += ss.str();
		}
	}

	fclose(fp);

	if(throwErrors && errorStr != "")
	{
		__COUT_INFO__ << "\n" << ss.str();
		__THROW__(errorStr);
	}
	else if(errorStr != "")
		__COUT_INFO__ << "\n" << ss.str();

}  // end restoreActiveTableGroups()

//==============================================================================
// destroyTableGroup
//	destroy all if theGroup == ""
//	else destroy that group
// 	if onlyDeactivate, then don't delete, just deactivate view
void ConfigurationManager::destroyTableGroup(const std::string& theGroup,
                                                     bool               onlyDeactivate)
{
	// delete
	bool isContext       = theGroup == "" || theGroup == theContextTableGroup_;
	bool isBackbone      = theGroup == "" || theGroup == theBackboneTableGroup_;
	bool isIterate       = theGroup == "" || theGroup == theIterateTableGroup_;
	bool isConfiguration = theGroup == "" || theGroup == theConfigurationTableGroup_;

	if(!isContext && !isBackbone && !isIterate && !isConfiguration)
	{
		__SS__ << "Invalid configuration group to destroy: " << theGroup << __E__;
		__COUT_ERR__ << ss.str();
		__SS_THROW__;
	}

	std::string dbgHeader = onlyDeactivate ? "Deactivating" : "Destroying";
	if(theGroup != "")
	{
		if(isContext)
			__COUT__ << dbgHeader << " Context group: " << theGroup << __E__;
		if(isBackbone)
			__COUT__ << dbgHeader << " Backbone group: " << theGroup << __E__;
		if(isIterate)
			__COUT__ << dbgHeader << " Iterate group: " << theGroup << __E__;
		if(isConfiguration)
			__COUT__ << dbgHeader << " Configuration group: " << theGroup << __E__;
	}

	std::set<std::string>::const_iterator contextFindIt, backboneFindIt, iterateFindIt;
	for(auto it = nameToTableMap_.begin(); it != nameToTableMap_.end();
	    /*no increment*/)
	{
		contextFindIt  = contextMemberNames_.find(it->first);
		backboneFindIt = backboneMemberNames_.find(it->first);
		iterateFindIt  = iterateMemberNames_.find(it->first);
		if(theGroup == "" ||
		   ((isContext && contextFindIt != contextMemberNames_.end()) ||
		    (isBackbone && backboneFindIt != backboneMemberNames_.end()) ||
		    (isIterate && iterateFindIt != iterateMemberNames_.end()) ||
		    (!isContext && !isBackbone && contextFindIt == contextMemberNames_.end() &&
		     backboneFindIt == backboneMemberNames_.end() &&
		     iterateFindIt == iterateMemberNames_.end())))
		{
			//__COUT__ << "\t" << it->first << __E__;
			// if(it->second->isActive())
			//	__COUT__ << "\t\t..._v" << it->second->getViewVersion() << __E__;

			if(onlyDeactivate)  // only deactivate
			{
				it->second->deactivate();
				++it;
			}
			else  // else, delete/erase
			{
				delete it->second;
				nameToTableMap_.erase(it++);
			}
		}
		else
			++it;
	}

	if(isConfiguration)
	{
		theConfigurationTableGroup_ = "";
		if(theConfigurationTableGroupKey_ != 0)
		{
			__COUT__ << "Destroying Configuration Key: " << *theConfigurationTableGroupKey_
			         << __E__;
			theConfigurationTableGroupKey_.reset();
		}

		//		theDACStreams_.clear();
	}
	if(isBackbone)
	{
		theBackboneTableGroup_ = "";
		if(theBackboneTableGroupKey_ != 0)
		{
			__COUT__ << "Destroying Backbone Key: " << *theBackboneTableGroupKey_ << __E__;
			theBackboneTableGroupKey_.reset();
		}
	}
	if(isIterate)
	{
		theIterateTableGroup_ = "";
		if(theIterateTableGroupKey_ != 0)
		{
			__COUT__ << "Destroying Iterate Key: " << *theIterateTableGroupKey_ << __E__;
			theIterateTableGroupKey_.reset();
		}
	}
	if(isContext)
	{
		theContextTableGroup_ = "";
		if(theContextTableGroupKey_ != 0)
		{
			__COUT__ << "Destroying Context Key: " << *theContextTableGroupKey_ << __E__;
			theContextTableGroupKey_.reset();
		}
	}
}

//==============================================================================
void ConfigurationManager::destroy(void)
{
	// NOTE: Moved to ConfigurationGUISupervisor [FIXME is this correct?? should we use
	// shared_ptr??]
	//    if( ConfigurationInterface::getInstance(true) != 0 )
	//        delete theInterface_;
	destroyTableGroup();
}

//==============================================================================
// convertGroupTypeIdToName
//	return translation:
//		0 for context
//		1 for backbone
//		2 for configuration (others)
const std::string& ConfigurationManager::convertGroupTypeIdToName(int groupTypeId)
{
	return groupTypeId == CONTEXT_TYPE
	           ? ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT
	           : (groupTypeId == BACKBONE_TYPE
	                  ? ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE
	                  : (groupTypeId == ITERATE_TYPE
	                         ? ConfigurationManager::ACTIVE_GROUP_NAME_ITERATE
	                         : ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION));
}

//==============================================================================
// getTypeOfGroup
//	return
//		CONTEXT_TYPE for context
//		BACKBONE_TYPE for backbone
//		ITERATE_TYPE for iterate
//		CONFIGURATION_TYPE for configuration (others)
int ConfigurationManager::getTypeOfGroup(
    const std::map<std::string /*name*/, TableVersion /*version*/>& memberMap)
{
	bool         isContext  = true;
	bool         isBackbone = true;
	bool         isIterate  = true;
	bool         inGroup;
	bool         inContext  = false;
	bool         inBackbone = false;
	bool         inIterate  = false;
	unsigned int matchCount = 0;

	for(auto& memberPair : memberMap)
	{
		//__COUT__ << "Member name: = "<< memberPair.first << __E__;
		////////////////////////////////////////
		inGroup = false;  // check context
		for(auto& contextMemberString : contextMemberNames_)
			if(memberPair.first == contextMemberString)
			{
				inGroup   = true;
				inContext = true;
				++matchCount;
				break;
			}
		if(!inGroup)
		{
			isContext = false;
			if(inContext)  // there was a member in context!
			{
				__SS__ << "This group is an incomplete match to a Context group.\n";
				__COUT_ERR__ << "\n" << ss.str();
				ss << "\nTo be a Context group, the members must exactly match "
				   << "the following members:\n";
				int i = 0;
				for(const auto& memberName : contextMemberNames_)
					ss << ++i << ". " << memberName << "\n";
				ss << "\nThe members are as follows::\n";
				i = 0;
				for(const auto& memberPairTmp : memberMap)
					ss << ++i << ". " << memberPairTmp.first << "\n";
				__SS_THROW__;
			}
		}

		////////////////////////////////////////
		inGroup = false;  // check backbone
		for(auto& backboneMemberString : backboneMemberNames_)
			if(memberPair.first == backboneMemberString)
			{
				inGroup    = true;
				inBackbone = true;
				++matchCount;
				break;
			}
		if(!inGroup)
		{
			isBackbone = false;
			if(inBackbone)  // there was a member in backbone!
			{
				__SS__ << "This group is an incomplete match to a Backbone group.\n";
				__COUT_ERR__ << "\n" << ss.str();
				ss << "\nTo be a Backbone group, the members must exactly match "
				   << "the following members:\n";
				int i = 0;
				for(auto& memberName : backboneMemberNames_)
					ss << ++i << ". " << memberName << "\n";
				ss << "\nThe members are as follows::\n";
				i = 0;
				for(const auto& memberPairTmp : memberMap)
					ss << ++i << ". " << memberPairTmp.first << "\n";
				//__COUT_ERR__ << "\n" << ss.str();
				__SS_THROW__;
			}
		}

		////////////////////////////////////////
		inGroup = false;  // check iterate
		for(auto& iterateMemberString : iterateMemberNames_)
			if(memberPair.first == iterateMemberString)
			{
				inGroup   = true;
				inIterate = true;
				++matchCount;
				break;
			}
		if(!inGroup)
		{
			isIterate = false;
			if(inIterate)  // there was a member in iterate!
			{
				__SS__ << "This group is an incomplete match to a Iterate group.\n";
				__COUT_ERR__ << "\n" << ss.str();
				ss << "\nTo be a Iterate group, the members must exactly match "
				   << "the following members:\n";
				int i = 0;
				for(auto& memberName : iterateMemberNames_)
					ss << ++i << ". " << memberName << "\n";
				ss << "\nThe members are as follows::\n";
				i = 0;
				for(const auto& memberPairTmp : memberMap)
					ss << ++i << ". " << memberPairTmp.first << "\n";
				//__COUT_ERR__ << "\n" << ss.str();
				__SS_THROW__;
			}
		}
	}

	if(isContext && matchCount != contextMemberNames_.size())
	{
		__SS__ << "This group is an incomplete match to a Context group: "
		       << " Size=" << matchCount << " but should be "
		       << contextMemberNames_.size() << __E__;
		__COUT_ERR__ << "\n" << ss.str();
		ss << "\nThe members currently are...\n";
		int i = 0;
		for(auto& memberPair : memberMap)
			ss << ++i << ". " << memberPair.first << "\n";
		ss << "\nThe expected Context members are...\n";
		i = 0;
		for(auto& memberName : contextMemberNames_)
			ss << ++i << ". " << memberName << "\n";
		//__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}

	if(isBackbone && matchCount != backboneMemberNames_.size())
	{
		__SS__ << "This group is an incomplete match to a Backbone group: "
		       << " Size=" << matchCount << " but should be "
		       << backboneMemberNames_.size() << __E__;
		__COUT_ERR__ << "\n" << ss.str();
		ss << "\nThe members currently are...\n";
		int i = 0;
		for(auto& memberPair : memberMap)
			ss << ++i << ". " << memberPair.first << "\n";
		ss << "\nThe expected Backbone members are...\n";
		i = 0;
		for(auto& memberName : backboneMemberNames_)
			ss << ++i << ". " << memberName << "\n";
		//__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}

	if(isIterate && matchCount != iterateMemberNames_.size())
	{
		__SS__ << "This group is an incomplete match to a Iterate group: "
		       << " Size=" << matchCount << " but should be "
		       << backboneMemberNames_.size() << __E__;
		__COUT_ERR__ << "\n" << ss.str();
		ss << "\nThe members currently are...\n";
		int i = 0;
		for(auto& memberPair : memberMap)
			ss << ++i << ". " << memberPair.first << "\n";
		ss << "\nThe expected Iterate members are...\n";
		i = 0;
		for(auto& memberName : iterateMemberNames_)
			ss << ++i << ". " << memberName << "\n";
		//__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}

	return isContext ? CONTEXT_TYPE
	                 : (isBackbone ? BACKBONE_TYPE
	                               : (isIterate ? ITERATE_TYPE : CONFIGURATION_TYPE));
}

//==============================================================================
// getTypeNameOfGroup
//	return string for group type
const std::string& ConfigurationManager::getTypeNameOfGroup(
    const std::map<std::string /*name*/, TableVersion /*version*/>& memberMap)
{
	return convertGroupTypeIdToName(getTypeOfGroup(memberMap));
}

//==============================================================================
// loadMemberMap
//	loads tables given by name/version pairs in memberMap
//	Note: does not activate them.
//
//	if filePath == "", then output to cout
void ConfigurationManager::dumpActiveConfiguration(const std::string& filePath,
                                                   const std::string& dumpType)
{
	time_t rawtime = time(0);
	__COUT__ << "filePath = " << filePath << __E__;
	__COUT__ << "dumpType = " << dumpType << __E__;

	std::ofstream fs;
	fs.open(filePath, std::fstream::out | std::fstream::trunc);

	std::ostream* out;

	// if file was valid use it, else default to cout
	if(fs.is_open())
		out = &fs;
	else
	{
		if(filePath != "")
		{
			__SS__ << "Invalid file path to dump active configuration. File " << filePath
			       << " could not be opened!" << __E__;
			__COUT_ERR__ << ss.str();
			__SS_THROW__;
		}
		out = &(std::cout);
	}

	(*out) << "#################################" << __E__;
	(*out) << "This is an ots configuration dump.\n\n" << __E__;
	(*out) << "Source database is $ARTDAQ_DATABASE_URI = \t"
	       << getenv("ARTDAQ_DATABASE_URI") << __E__;
	(*out) << "\nOriginal location of dump: \t" << filePath << __E__;
	(*out) << "Type of dump: \t" << dumpType << __E__;
	(*out) << "Linux time for dump: \t" << rawtime << __E__;

	{
		struct tm* timeinfo = localtime(&rawtime);
		char       buffer[100];
		strftime(buffer, 100, "%c %Z", timeinfo);
		(*out) << "Display time for dump: \t" << buffer << __E__;
	}

	// define local "lambda" functions
	//	active groups
	//	active tables
	//	active group members
	//	active table contents

	auto localDumpActiveGroups = [](const ConfigurationManager* cfgMgr,
	                                std::ostream*               out) {
		std::map<std::string, std::pair<std::string, TableGroupKey>> activeGroups =
		    cfgMgr->getActiveTableGroups();

		(*out) << "\n\n************************" << __E__;
		(*out) << "Active Groups:" << __E__;
		for(auto& group : activeGroups)
		{
			(*out) << "\t" << group.first << " := " << group.second.first << " ("
			       << group.second.second << ")" << __E__;
		}
	};

	auto localDumpActiveTables = [](const ConfigurationManager* cfgMgr,
	                                std::ostream*               out) {
		std::map<std::string, TableVersion> activeTables = cfgMgr->getActiveVersions();

		(*out) << "\n\n************************" << __E__;
		(*out) << "Active Tables:" << __E__;
		(*out) << "Active Tables count = " << activeTables.size() << __E__;

		unsigned int i = 0;
		for(auto& table : activeTables)
		{
			(*out) << "\t" << ++i << ". " << table.first << "-v" << table.second
			       << __E__;
		}
	};

	auto localDumpActiveGroupMembers = [](ConfigurationManager* cfgMgr,
	                                      std::ostream*         out) {
		std::map<std::string, std::pair<std::string, TableGroupKey>> activeGroups =
		    cfgMgr->getActiveTableGroups();
		(*out) << "\n\n************************" << __E__;
		(*out) << "Active Group Members:" << __E__;
		int tableCount = 0;
		for(auto& group : activeGroups)
		{
			(*out) << "\t" << group.first << " := " << group.second.first << " ("
			       << group.second.second << ")" << __E__;

			if(group.second.first == "")
			{
				(*out) << "\t"
				       << "Empty group name. Assuming no active group." << __E__;
				continue;
			}

			std::map<std::string /*name*/, TableVersion /*version*/> memberMap;
			std::map<std::string /*name*/, std::string /*alias*/>    groupAliases;
			std::string                                              groupComment;
			std::string                                              groupAuthor;
			std::string                                              groupCreateTime;
			time_t                                                   groupCreateTime_t;

			cfgMgr->loadTableGroup(group.second.first,
			                               group.second.second,
			                               false /*doActivate*/,
			                               &memberMap /*memberMap*/,
			                               0 /*progressBar*/,
			                               0 /*accumulateErrors*/,
			                               &groupComment,
			                               &groupAuthor,
			                               &groupCreateTime,
			                               true /*doNotLoadMember*/,
			                               0 /*groupTypeString*/,
			                               &groupAliases);

			(*out) << "\t\tGroup Comment: \t" << groupComment << __E__;
			(*out) << "\t\tGroup Author: \t" << groupAuthor << __E__;

			sscanf(groupCreateTime.c_str(), "%ld", &groupCreateTime_t);
			(*out) << "\t\tGroup Create Time: \t" << ctime(&groupCreateTime_t) << __E__;
			(*out) << "\t\tGroup Aliases: \t" << StringMacros::mapToString(groupAliases)
			       << __E__;

			(*out) << "\t\tMember table count = " << memberMap.size() << __E__;
			tableCount += memberMap.size();

			unsigned int i = 0;
			for(auto& member : memberMap)
			{
				(*out) << "\t\t\t" << ++i << ". " << member.first << "-v" << member.second
				       << __E__;
			}
		}
		(*out) << "\nActive Group Members total table count = " << tableCount
		       << __E__;
	};

	auto localDumpActiveTableContents = [](const ConfigurationManager* cfgMgr,
	                                       std::ostream*               out) {
		std::map<std::string, TableVersion> activeTables = cfgMgr->getActiveVersions();

		(*out) << "\n\n************************" << __E__;
		(*out) << "Active Table Contents (table count = " << activeTables.size()
		       << "):" << __E__;
		unsigned int i = 0;
		for(auto& table : activeTables)
		{
			(*out) << "\n\n=============================================================="
			          "================"
			       << __E__;
			(*out) << "=================================================================="
			          "============"
			       << __E__;
			(*out) << "\t" << ++i << ". " << table.first << "-v" << table.second
			       << __E__;

			cfgMgr->nameToTableMap_.find(table.first)->second->print(*out);
		}
	};

	if(dumpType == "GroupKeys")
	{
		localDumpActiveGroups(this, out);
	}
	else if(dumpType == "TableVersions")
	{
		localDumpActiveTables(this, out);
	}
	else if(dumpType == "GroupKeysAndTableVersions")
	{
		localDumpActiveGroups(this, out);
		localDumpActiveTables(this, out);
	}
	else if(dumpType == "All")
	{
		localDumpActiveGroups(this, out);
		localDumpActiveGroupMembers(this, out);
		localDumpActiveTables(this, out);
		localDumpActiveTableContents(this, out);
	}
	else
	{
		__SS__
		    << "Invalid dump type '" << dumpType
		    << "' given during dumpActiveConfiguration(). Valid types are as follows:\n"
		    <<

		    // List all choices
		    "GroupKeys"
		    << ", "
		    << "TableVersions"
		    << ", "
		    << "GroupsKeysAndTableVersions"
		    << ", "
		    << "All"
		    <<

		    "\n\nPlease change the State Machine configuration to a valid dump type."
		    << __E__;
		__SS_THROW__;
	}

	if(fs.is_open())
		fs.close();
}

//==============================================================================
// loadMemberMap
//	loads tables given by name/version pairs in memberMap
//	Note: does not activate them.
void ConfigurationManager::loadMemberMap(
    const std::map<std::string /*name*/, TableVersion /*version*/>& memberMap)
{
	TableBase* tmpConfigBasePtr;
	//	for each member
	//		get()
	for(auto& memberPair : memberMap)
	{
		//__COUT__ << "\tMember config " << memberPair.first << ":" <<
		//		memberPair.second << __E__;

		// get the proper temporary pointer
		//	use 0 if doesn't exist yet.
		//	Note: do not want to give nameToTableMap_[memberPair.first]
		//		in case there is failure in get... (exceptions may be thrown)
		// Note: Default constructor is called by Map, i.e.
		// nameToTableMap_[memberPair.first] = 0; //create pointer and set to 0
		tmpConfigBasePtr = 0;
		if(nameToTableMap_.find(memberPair.first) !=
		   nameToTableMap_.end())
			tmpConfigBasePtr = nameToTableMap_[memberPair.first];

		theInterface_->get(tmpConfigBasePtr,   // configurationPtr
		                   memberPair.first,   // tableName
		                   0,                  // groupKey
		                   0,                  // groupName
		                   false,              // dontFill=false to fill
		                   memberPair.second,  // version
		                   false               // resetTable
		);

		nameToTableMap_[memberPair.first] = tmpConfigBasePtr;
		if(nameToTableMap_[memberPair.first]->getViewP())
		{
			//__COUT__ << "\t\tActivated version: " <<
			// nameToTableMap_[memberPair.first]->getViewVersion() << __E__;
		}
		else
		{
			__SS__ << nameToTableMap_[memberPair.first]->getTableName()
			       << ": View version not activated properly!";
			__SS_THROW__;
		}
	}
} //end loadMemberMap()

//==============================================================================
// loadTableGroup
//	load all members of configuration group
//	if doActivate
//		DOES set theConfigurationTableGroup_, theContextTableGroup_, or theBackboneTableGroup_ on success
//			this also happens with ConfigurationManagerRW::activateTableGroup
//		for each member
//			configBase->init()
//
//	if progressBar != 0, then do step handling, for finer granularity
//
// 	if(doNotLoadMember) return memberMap; //this is useful if just getting group metadata
//	else NOTE: active views are changed! (when loading member map)
//
//	throws exception on failure.
//   map<name       , TableVersion >
void ConfigurationManager::loadTableGroup(
    const std::string&                                     groupName,
    TableGroupKey                                          groupKey,
    bool                                                   doActivate /*=false*/,
    std::map<std::string, TableVersion>*                   groupMembers,
    ProgressBar*                                           progressBar,
    std::string*                                           accumulatedTreeErrors,
    std::string*                                           groupComment,
    std::string*                                           groupAuthor,
    std::string*                                           groupCreateTime,
    bool                                                   doNotLoadMember /*=false*/,
    std::string*                                           groupTypeString,
    std::map<std::string /*name*/, std::string /*alias*/>* groupAliases) try
{
	// clear to defaults
	if(accumulatedTreeErrors)
		*accumulatedTreeErrors = "";
	if(groupComment)
		*groupComment = "NO COMMENT FOUND";
	if(groupAuthor)
		*groupAuthor = "NO AUTHOR FOUND";
	if(groupCreateTime)
		*groupCreateTime = "0";
	if(groupTypeString)
		*groupTypeString = "UNKNOWN";

	//	if(groupName == "defaultConfig")
	//	{ //debug active versions
	//		std::map<std::string, TableVersion> allActivePairs = getActiveVersions();
	//		for(auto& activePair: allActivePairs)
	//		{
	//			__COUT__ << "Active table = " <<
	//					activePair.first << "-v" <<
	//					getTableByName(activePair.first)->getView().getVersion() <<
	// __E__;
	//		}
	//	}

	//	load all members of configuration group
	//	if doActivate
	//		determine the type configuration
	//		deactivate all of that type (invalidate active view)
	//
	//	for each member
	//		get()
	//		if doActivate, configBase->init()
	//
	//	if doActivate
	//		set theConfigurationTableGroup_, theContextTableGroup_, or theBackboneTableGroup_ on success

	//	__COUT_INFO__ << "Loading Table Group: " << groupName <<
	//			"(" << groupKey << ")" << __E__;

	std::map<std::string /*name*/, TableVersion /*version*/> memberMap =
	    theInterface_->getTableGroupMembers(
	        TableGroupKey::getFullGroupString(groupName, groupKey),
	        true /*include meta data table*/);
	std::map<std::string /*name*/, std::string /*alias*/> aliasMap;

	if(progressBar)
		progressBar->step();

	// remove meta data table and extract info
	auto metaTablePair = memberMap.find(groupMetadataTable_.getTableName());
	if(metaTablePair != memberMap.end())
	{
		//__COUT__ << "Found group meta data. v" << metaTablePair->second << __E__;

		memberMap.erase(metaTablePair);  // remove from member map that is returned

		// clear table
		while(groupMetadataTable_.getView().getNumberOfRows())
			groupMetadataTable_.getViewP()->deleteRow(0);

		// retrieve metadata from database
		try
		{
			theInterface_->fill(&groupMetadataTable_, metaTablePair->second);
		}
		catch(const std::runtime_error& e)
		{
			__COUT_WARN__ << "Ignoring metadata error: " << e.what() << __E__;
		}
		catch(...)
		{
			__COUT_WARN__ << "Ignoring unknown metadata error. " << __E__;
		}

		// check that there is only 1 row
		if(groupMetadataTable_.getView().getNumberOfRows() != 1)
		{
			if(groupMembers)
				*groupMembers = memberMap;  // copy for return

			groupMetadataTable_.print();
			__SS__ << "Ignoring that groupMetadataTable_ has wrong number of rows! Must "
			          "be 1. Going with anonymous defaults."
			       << __E__;
			__COUT_ERR__ << "\n" << ss.str();

			// fix metadata table
			while(groupMetadataTable_.getViewP()->getNumberOfRows() > 1)
				groupMetadataTable_.getViewP()->deleteRow(0);
			if(groupMetadataTable_.getViewP()->getNumberOfRows() == 0)
				groupMetadataTable_.getViewP()->addRow();

			if(groupComment)
				*groupComment = "NO COMMENT FOUND";
			if(groupAuthor)
				*groupAuthor = "NO AUTHOR FOUND";
			if(groupCreateTime)
				*groupCreateTime = "0";

			int groupType = -1;
			if(groupTypeString)  // do before exit case
			{
				groupType        = getTypeOfGroup(memberMap);
				*groupTypeString = convertGroupTypeIdToName(groupType);
			}
			return;  // memberMap;
		}

		// groupMetadataTable_.print();

		// extract fields
		StringMacros::getMapFromString(groupMetadataTable_.getView().getValueAsString(
		                                   0, ConfigurationManager::METADATA_COL_ALIASES),
		                               aliasMap);
		if(groupAliases)
			*groupAliases = aliasMap;
		if(groupComment)
			*groupComment = groupMetadataTable_.getView().getValueAsString(
			    0, ConfigurationManager::METADATA_COL_COMMENT);
		if(groupAuthor)
			*groupAuthor = groupMetadataTable_.getView().getValueAsString(
			    0, ConfigurationManager::METADATA_COL_AUTHOR);
		if(groupCreateTime)
			*groupCreateTime = groupMetadataTable_.getView().getValueAsString(
			    0, ConfigurationManager::METADATA_COL_TIMESTAMP);

		// modify members based on aliases
		{
			std::map<std::string /*table*/, std::map<std::string /*alias*/, TableVersion>>
			    versionAliases;
			if(aliasMap.size())  // load version aliases
			{
				__COUTV__(StringMacros::mapToString(aliasMap));
				versionAliases = ConfigurationManager::getVersionAliases();
				__COUTV__(StringMacros::mapToString(versionAliases));
			}

			// convert alias to version
			for(auto& aliasPair : aliasMap)
			{
				// check for alias table in member names
				if(memberMap.find(aliasPair.first) != memberMap.end())
				{
					__COUT__ << "Group member '" << aliasPair.first
					         << "' was found in group member map!" << __E__;
					__COUT__ << "Looking for alias '" << aliasPair.second
					         << "' in active version aliases..." << __E__;

					if(versionAliases.find(aliasPair.first) == versionAliases.end() ||
					   versionAliases[aliasPair.first].find(aliasPair.second) ==
					       versionAliases[aliasPair.first].end())
					{
						__SS__ << "Group '" << groupName << "(" << groupKey
						       << ")' requires table version alias '" << aliasPair.first
						       << ":" << aliasPair.second
						       << ",' which was not found in the active Backbone!"
						       << __E__;
						__SS_THROW__;
					}

					memberMap[aliasPair.first] =
					    versionAliases[aliasPair.first][aliasPair.second];
					__COUT__ << "Version alias translated to " << aliasPair.first
					         << __E__;
				}
			}
		}
	}

	if(groupMembers)
		*groupMembers = memberMap;  // copy map for return

	if(progressBar)
		progressBar->step();

	//__COUT__ << "memberMap loaded size = " << memberMap.size() << __E__;

	int groupType = -1;
	try
	{
		if(groupTypeString)  // do before exit case
		{
			groupType        = getTypeOfGroup(memberMap);
			*groupTypeString = convertGroupTypeIdToName(groupType);
		}

		//	if(groupName == "defaultConfig")
		//		{ //debug active versions
		//			std::map<std::string, TableVersion> allActivePairs =
		// getActiveVersions(); 			for(auto& activePair: allActivePairs)
		//			{
		//				__COUT__ << "Active table = " <<
		//						activePair.first << "-v" <<
		//						getTableByName(activePair.first)->getView().getVersion()
		//<<  __E__;
		//			}
		//		}

		if(doNotLoadMember)
			return;  // memberMap; //this is useful if just getting group metadata

		// if not already done, determine the type configuration group
		if(!groupTypeString)
			groupType = getTypeOfGroup(memberMap);

		if(doActivate)
			__COUT__ << "------------------------------------- init start    \t [for all "
			            "plug-ins in "
			         << convertGroupTypeIdToName(groupType) << " group '"
			         << groupName << "(" << groupKey << ")"
			         << "']" << __E__;

		if(doActivate)
		{
			std::string groupToDeactivate =
			    groupType == ConfigurationManager::CONTEXT_TYPE
			        ? theContextTableGroup_
			        : (groupType == ConfigurationManager::BACKBONE_TYPE
			               ? theBackboneTableGroup_
			               : (groupType == ConfigurationManager::ITERATE_TYPE
			                      ? theIterateTableGroup_
			                      : theConfigurationTableGroup_));

			//		deactivate all of that type (invalidate active view)
			if(groupToDeactivate != "")  // deactivate only if pre-existing group
			{
				//__COUT__ << "groupToDeactivate '" << groupToDeactivate << "'" <<
				// __E__;
				destroyTableGroup(groupToDeactivate, true);
			}
			//		else
			//		{
			//			//Getting here, is kind of strange:
			//			//	- this group may have only been partially loaded before?
			//		}
		}
		//	if(groupName == "defaultConfig")
		//		{ //debug active versions
		//			std::map<std::string, TableVersion> allActivePairs =
		// getActiveVersions(); 			for(auto& activePair: allActivePairs)
		//			{
		//				__COUT__ << "Active table = " <<
		//						activePair.first << "-v" <<
		//						getTableByName(activePair.first)->getView().getVersion()
		//<<  __E__;
		//			}
		//		}

		if(progressBar)
			progressBar->step();

		//__COUT__ << "Activating chosen group:" << __E__;

		loadMemberMap(memberMap);

		if(progressBar)
			progressBar->step();

		if(accumulatedTreeErrors)
		{
			//__COUT__ << "Checking chosen group for tree errors..." << __E__;

			getChildren(&memberMap, accumulatedTreeErrors);
			if(*accumulatedTreeErrors != "")
			{
				__COUT_ERR__ << "Errors detected while loading Table Group: "
				             << groupName << "(" << groupKey << "). Aborting."
				             << __E__;
				return;  // memberMap; //return member name map to version
			}
		}

		if(progressBar)
			progressBar->step();

		//	for each member
		//		if doActivate, configBase->init()
		if(doActivate)
			for(auto& memberPair : memberMap)
			{
				// do NOT allow activating Scratch versions if tracking is ON!
				if(ConfigurationInterface::isVersionTrackingEnabled() &&
				   memberPair.second.isScratchVersion())
				{
					__SS__ << "Error while activating member Table '"
					       << nameToTableMap_[memberPair.first]->getTableName()
					       << "-v" << memberPair.second << " for Table Group '"
					       << groupName << "(" << groupKey
					       << ")'. When version tracking is enabled, Scratch views"
					       << " are not allowed! Please only use unique, persistent "
					          "versions when version tracking is enabled."
					       << __E__;
					__COUT_ERR__ << "\n" << ss.str();
					__SS_THROW__;
				}

				// attempt to init using the configuration's specific init
				//	this could be risky user code, try and catch
				try
				{
					nameToTableMap_[memberPair.first]->init(this);
				}
				catch(std::runtime_error& e)
				{
					__SS__ << "Error detected calling "
					       << nameToTableMap_[memberPair.first]->getTableName()
					       << ".init()!\n\n " << e.what() << __E__;
					__SS_THROW__;
				}
				catch(...)
				{
					__SS__ << "Unknown Error detected calling "
					       << nameToTableMap_[memberPair.first]->getTableName()
					       << ".init()!\n\n " << __E__;
					__SS_THROW__;
				}
			}

		if(progressBar)
			progressBar->step();

		//	if doActivate
		//		set theConfigurationTableGroup_, theContextTableGroup_, or theBackboneTableGroup_ on
		// success

		if(doActivate)
		{
			if(groupType == ConfigurationManager::CONTEXT_TYPE)  //
			{
				//			__COUT_INFO__ << "Type=Context, Group loaded: " <<
				// groupName
				//<<
				//					"(" << groupKey << ")" << __E__;
				theContextTableGroup_ = groupName;
				theContextTableGroupKey_ =
				    std::shared_ptr<TableGroupKey>(new TableGroupKey(groupKey));
			}
			else if(groupType == ConfigurationManager::BACKBONE_TYPE)
			{
				//			__COUT_INFO__ << "Type=Backbone, Group loaded: " <<
				// groupName <<
				//					"(" << groupKey << ")" << __E__;
				theBackboneTableGroup_ = groupName;
				theBackboneTableGroupKey_ =
				    std::shared_ptr<TableGroupKey>(new TableGroupKey(groupKey));
			}
			else if(groupType == ConfigurationManager::ITERATE_TYPE)
			{
				//			__COUT_INFO__ << "Type=Iterate, Group loaded: " <<
				// groupName
				//<<
				//					"(" << groupKey << ")" << __E__;
				theIterateTableGroup_ = groupName;
				theIterateTableGroupKey_ =
				    std::shared_ptr<TableGroupKey>(new TableGroupKey(groupKey));
			}
			else  // is theConfigurationTableGroup_
			{
				//			__COUT_INFO__ << "Type=Configuration, Group loaded: " <<
				// groupName <<
				//					"(" << groupKey << ")" << __E__;
				theConfigurationTableGroup_ = groupName;
				theConfigurationTableGroupKey_ =
				    std::shared_ptr<TableGroupKey>(new TableGroupKey(groupKey));
			}
		}

		if(progressBar)
			progressBar->step();

		if(doActivate)
			__COUT__ << "------------------------------------- init complete \t [for all "
			            "plug-ins in "
			         << convertGroupTypeIdToName(groupType) << " group '"
			         << groupName << "(" << groupKey << ")"
			         << "']" << __E__;
	}  // end failed group load try
	catch(...)
	{
		// save group name and key of failed load attempt
		lastFailedGroupLoad_[convertGroupTypeIdToName(groupType)] =
		    std::pair<std::string, TableGroupKey>(groupName,
		                                          TableGroupKey(groupKey));

		try
		{
			throw;
		}
		catch(const std::runtime_error& e)
		{
			__SS__ << "Error occurred while loading table group '"
			       << groupName << "(" << groupKey << ")': \n"
			       << e.what() << __E__;
			__COUT_WARN__ << ss.str();
			if(accumulatedTreeErrors)
				*accumulatedTreeErrors = ss.str();
		}
		catch(...)
		{
			__SS__ << "An unknown error occurred while loading table group '"
			       << groupName << "(" << groupKey << ")." << __E__;
			__COUT_WARN__ << ss.str();
			if(accumulatedTreeErrors)
				*accumulatedTreeErrors = ss.str();
		}
	}

	return;
}
catch(...)
{
	// save group name and key of failed load attempt
	lastFailedGroupLoad_[ConfigurationManager::ACTIVE_GROUP_NAME_UNKNOWN] =
	    std::pair<std::string, TableGroupKey>(groupName,
	                                          TableGroupKey(groupKey));

	try
	{
		throw;
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error occurred while loading table group: " << e.what()
		       << __E__;
		__COUT_WARN__ << ss.str();
		if(accumulatedTreeErrors)
			*accumulatedTreeErrors = ss.str();
	}
	catch(...)
	{
		__SS__ << "An unknown error occurred while loading table group." << __E__;
		__COUT_WARN__ << ss.str();
		if(accumulatedTreeErrors)
			*accumulatedTreeErrors = ss.str();
	}
}  // end loadTableGroup()

//==============================================================================
// getActiveTableGroups
//	get the active table groups map
//   map<type,        pair     <groupName  , TableGroupKey> >
//
//	Note: invalid TableGroupKey means no active group currently
std::map<std::string, std::pair<std::string, TableGroupKey>>
ConfigurationManager::getActiveTableGroups(void) const
{
	//   map<type,        pair     <groupName  , TableGroupKey> >
	std::map<std::string, std::pair<std::string, TableGroupKey>> retMap;

	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT] =
	    std::pair<std::string, TableGroupKey>(
	        theContextTableGroup_,
	        theContextTableGroupKey_ ? *theContextTableGroupKey_ : TableGroupKey());
	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE] =
	    std::pair<std::string, TableGroupKey>(
	        theBackboneTableGroup_,
	        theBackboneTableGroupKey_ ? *theBackboneTableGroupKey_ : TableGroupKey());
	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_ITERATE] =
	    std::pair<std::string, TableGroupKey>(
	        theIterateTableGroup_,
	        theIterateTableGroupKey_ ? *theIterateTableGroupKey_ : TableGroupKey());
	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION] =
	    std::pair<std::string, TableGroupKey>(
	        theConfigurationTableGroup_,
	        theConfigurationTableGroupKey_ ? *theConfigurationTableGroupKey_ : TableGroupKey());
	return retMap;
} // end getActiveTableGroups()

//==============================================================================
const std::string& ConfigurationManager::getActiveGroupName(const std::string& type) const
{
	if(type == "" || type == ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION)
		return theConfigurationTableGroup_;
	if(type == ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT)
		return theContextTableGroup_;
	if(type == ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE)
		return theBackboneTableGroup_;
	if(type == ConfigurationManager::ACTIVE_GROUP_NAME_ITERATE)
		return theIterateTableGroup_;

	__SS__ << "Invalid type requested '" << type << "'" << __E__;
	__COUT_ERR__ << ss.str();
	__SS_THROW__;
}

//==============================================================================
TableGroupKey ConfigurationManager::getActiveGroupKey(const std::string& type) const
{
	if(type == "" || type == ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION)
		return theConfigurationTableGroupKey_ ? *theConfigurationTableGroupKey_ : TableGroupKey();
	if(type == ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT)
		return theContextTableGroupKey_ ? *theContextTableGroupKey_ : TableGroupKey();
	if(type == ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE)
		return theBackboneTableGroupKey_ ? *theBackboneTableGroupKey_ : TableGroupKey();
	if(type == ConfigurationManager::ACTIVE_GROUP_NAME_ITERATE)
		return theIterateTableGroupKey_ ? *theIterateTableGroupKey_ : TableGroupKey();

	__SS__ << "Invalid type requested '" << type << "'" << __E__;
	__COUT_ERR__ << ss.str();
	__SS_THROW__;
}

//==============================================================================
ConfigurationTree ConfigurationManager::getContextNode(
    const std::string& contextUID, const std::string& applicationUID) const
{
	return getNode("/" + getTableByName(XDAQ_CONTEXT_TABLE_NAME)->getTableName() + "/" +
	               contextUID);
}

//==============================================================================
ConfigurationTree ConfigurationManager::getSupervisorNode(
    const std::string& contextUID, const std::string& applicationUID) const
{
	return getNode("/" + getTableByName(XDAQ_CONTEXT_TABLE_NAME)->getTableName() + "/" +
	               contextUID + "/LinkToApplicationTable/" + applicationUID);
}

//==============================================================================
ConfigurationTree ConfigurationManager::getSupervisorTableNode(
    const std::string& contextUID, const std::string& applicationUID) const
{
	return getNode("/" + getTableByName(XDAQ_CONTEXT_TABLE_NAME)->getTableName() + "/" +
	               contextUID + "/LinkToApplicationTable/" + applicationUID +
	               "/LinkToSupervisorTable");
}

//==============================================================================
ConfigurationTree ConfigurationManager::getNode(const std::string& nodeString,
                                                bool doNotThrowOnBrokenUIDLinks) const
{
	//__COUT__ << "nodeString=" << nodeString << " " << nodeString.length() << __E__;

	// get nodeName (in case of / syntax)
	if(nodeString.length() < 1)
	{
		__SS__ << ("Invalid empty node name") << __E__;
		__COUT_ERR__ << ss.str();
		__SS_THROW__;
	}

	// ignore multiple starting slashes
	unsigned int startingIndex = 0;
	while(startingIndex < nodeString.length() && nodeString[startingIndex] == '/')
		++startingIndex;

	std::string nodeName = nodeString.substr(
	    startingIndex, nodeString.find('/', startingIndex) - startingIndex);
	//__COUT__ << "nodeName=" << nodeName << " " << nodeName.length() << __E__;
	if(nodeName.length() < 1)
	{
		// return root node
		return ConfigurationTree(this, 0);

		//		__SS__ << "Invalid node name: " << nodeName << __E__;
		//		__COUT_ERR__ << ss.str();
		//		__SS_THROW__;
	}

	std::string childPath = nodeString.substr(nodeName.length() + startingIndex);

	//__COUT__ << "childPath=" << childPath << " " << childPath.length() << __E__;

	ConfigurationTree configTree(this, getTableByName(nodeName));

	if(childPath.length() > 1)
		return configTree.getNode(childPath, doNotThrowOnBrokenUIDLinks);
	else
		return configTree;
}

//==============================================================================
// getFirstPathToNode
std::string ConfigurationManager::getFirstPathToNode(const ConfigurationTree& node,
                                                     const std::string& startPath) const
// void ConfigurationManager::getFirstPathToNode(const ConfigurationTree &node, const
// ConfigurationTree &startNode) const
{
	std::string path = "/";
	return path;
}

//==============================================================================
// getChildren
//	if memberMap is passed then only consider children in the map
//
//	if accumulatedTreeErrors is non null, check for disconnects occurs.
//		check is 2 levels deep which should get to the links starting at tables.
std::vector<std::pair<std::string, ConfigurationTree>> ConfigurationManager::getChildren(
    std::map<std::string, TableVersion>* memberMap,
    std::string*                         accumulatedTreeErrors) const
{
	std::vector<std::pair<std::string, ConfigurationTree>> retMap;
	if(accumulatedTreeErrors)
		*accumulatedTreeErrors = "";

	if(!memberMap || memberMap->empty())  // return all present active members
	{
		for(auto& configPair : nameToTableMap_)
		{
			//__COUT__ << configPair.first <<  " " << (int)(configPair.second?1:0) <<
			// __E__;

			if(configPair.second->isActive())  // only consider if active
			{
				ConfigurationTree newNode(this, configPair.second);

				if(accumulatedTreeErrors)  // check for disconnects
				{
					try
					{
						std::vector<std::pair<std::string, ConfigurationTree>>
						    newNodeChildren = newNode.getChildren();
						for(auto& newNodeChild : newNodeChildren)
						{
							std::vector<std::pair<std::string, ConfigurationTree>>
							    twoDeepChildren = newNodeChild.second.getChildren();

							for(auto& twoDeepChild : twoDeepChildren)
							{
								//__COUT__ << configPair.first << " " <<
								// newNodeChild.first << " " << 		twoDeepChild.first
								// <<  __E__;
								if(twoDeepChild.second.isLinkNode() &&
								   twoDeepChild.second.isDisconnected() &&
								   twoDeepChild.second.getDisconnectedTableName() !=
								       TableViewColumnInfo::DATATYPE_LINK_DEFAULT)
									*accumulatedTreeErrors +=
									    "\n\nAt node '" + configPair.first +
									    "' with entry UID '" + newNodeChild.first +
									    "' there is a disconnected child node at link "
									    "column '" +
									    twoDeepChild.first + "'" +
									    " that points to table named '" +
									    twoDeepChild.second.getDisconnectedTableName() +
									    "' ...";
							}
						}
					}
					catch(std::runtime_error& e)
					{
						*accumulatedTreeErrors +=
						    "\n\nAt node '" + configPair.first +
						    "' error detected descending through children:\n" + e.what();
					}
				}

				retMap.push_back(
				    std::pair<std::string, ConfigurationTree>(configPair.first, newNode));
			}

			//__COUT__ << configPair.first <<  __E__;
		}
	}
	else  // return only members from the member map (they must be present and active!)
	{
		for(auto& memberPair : *memberMap)
		{
			auto mapIt = nameToTableMap_.find(memberPair.first);
			if(mapIt == nameToTableMap_.end())
			{
				__SS__ << "Get Children with member map requires a child '"
				       << memberPair.first << "' that is not present!" << __E__;
				__SS_THROW__;
			}
			if(!(*mapIt).second->isActive())
			{
				__SS__ << "Get Children with member map requires a child '"
				       << memberPair.first << "' that is not active!" << __E__;
				__SS_THROW__;
			}

			ConfigurationTree newNode(this, (*mapIt).second);

			if(accumulatedTreeErrors)  // check for disconnects
			{
				try
				{
					std::vector<std::pair<std::string, ConfigurationTree>>
					    newNodeChildren = newNode.getChildren();
					for(auto& newNodeChild : newNodeChildren)
					{
						std::vector<std::pair<std::string, ConfigurationTree>>
						    twoDeepChildren = newNodeChild.second.getChildren();

						for(auto& twoDeepChild : twoDeepChildren)
						{
							//__COUT__ << memberPair.first << " " << newNodeChild.first <<
							//" " << 		twoDeepChild.first << __E__;
							if(twoDeepChild.second.isLinkNode() &&
							   twoDeepChild.second.isDisconnected() &&
							   twoDeepChild.second.getDisconnectedTableName() !=
							       TableViewColumnInfo::DATATYPE_LINK_DEFAULT)
							{
								*accumulatedTreeErrors +=
								    "\n\nAt node '" + memberPair.first +
								    "' with entry UID '" + newNodeChild.first +
								    "' there is a disconnected child node at link column "
								    "'" +
								    twoDeepChild.first + "'" +
								    " that points to table named '" +
								    twoDeepChild.second.getDisconnectedTableName() +
								    "' ...";

								// check if disconnected table is in group, if not
								// software error

								bool found = false;
								for(auto& searchMemberPair : *memberMap)
									if(searchMemberPair.first ==
									   twoDeepChild.second.getDisconnectedTableName())
									{
										found = true;
										break;
									}
								if(!found)
									*accumulatedTreeErrors +=
									    std::string(
									        "\nNote: It may be safe to ignore this "
									        "error ") +
									    "since the link's target table " +
									    twoDeepChild.second.getDisconnectedTableName() +
									    " is not a member of this group (and may not be "
									    "loaded yet).";
							}
						}
					}
				}
				catch(std::runtime_error& e)
				{
					*accumulatedTreeErrors +=
					    "\n\nAt node '" + memberPair.first +
					    "' error detected descending through children:\n" + e.what();
				}
			}

			retMap.push_back(
			    std::pair<std::string, ConfigurationTree>(memberPair.first, newNode));
		}
	}

	return retMap;
}

//==============================================================================
// getTableByName
//	Get read-only pointer to configuration.
//	If Read/Write access is needed use ConfigurationManagerWithWriteAccess
//		(For general use, Write access should be avoided)
const TableBase* ConfigurationManager::getTableByName(
    const std::string& tableName) const
{
	std::map<std::string, TableBase*>::const_iterator it;
	if((it = nameToTableMap_.find(tableName)) ==
	   nameToTableMap_.end())
	{
		__SS__ << "\n\nCan not find configuration named '" << tableName
		       << "'\n\n\n\nYou need to load the configuration before it can be used."
		       << " It probably is missing from the member list of the Table "
		          "Group that was loaded.\n"
		       << "\nYou may need to enter wiz mode to remedy the situation, use the "
		          "following:\n"
		       << "\n\t StartOTS.sh --wiz"
		       << "\n\n\n\n"
		       << __E__;

		// prints out too often, so only throw
		// if(tableName != TableViewColumnInfo::DATATYPE_LINK_DEFAULT)
		//	__COUT_WARN__ << "\n" << ss.str();
		__SS_ONLY_THROW__;
	}
	return it->second;
}

//==============================================================================
// loadConfigurationBackbone
//	loads the active backbone configuration group
//	returns the active group key that was loaded
TableGroupKey ConfigurationManager::loadConfigurationBackbone()
{
	if(!theBackboneTableGroupKey_)  // no active backbone
	{
		__COUT_WARN__ << "getTableGroupKey() Failed! No active backbone currently."
		              << __E__;
		return TableGroupKey();
	}

	// may already be loaded, but that's ok, load anyway to be sure
	loadTableGroup(theBackboneTableGroup_, *theBackboneTableGroupKey_);

	return *theBackboneTableGroupKey_;
}

// Getters
//==============================================================================
// getTableGroupKey
//	use backbone to determine default key for systemAlias.
//		- runType translates to group key alias,
//			which maps to a group name and key pair
//
//	NOTE: temporary special aliases are also allowed
//		with the following format:
//		GROUP:<name>:<key>
//
//	return INVALID on failure
//   else, pair<group name , TableGroupKey>
std::pair<std::string, TableGroupKey>
ConfigurationManager::getTableGroupFromAlias(std::string  systemAlias,
                                                     ProgressBar* progressBar)
{
	// steps
	//	check if special alias
	//	if so, parse and return name/key
	//	else, load active backbone
	//	find runType in Group Aliases table
	//	return key

	if(progressBar)
		progressBar->step();

	if(systemAlias.find("GROUP:") == 0)
	{
		if(progressBar)
			progressBar->step();

		unsigned int i = strlen("GROUP:");
		unsigned int j = systemAlias.find(':', i);

		if(progressBar)
			progressBar->step();
		if(j > i)  // success
			return std::pair<std::string, TableGroupKey>(
			    systemAlias.substr(i, j - i), TableGroupKey(systemAlias.substr(j + 1)));
		else  // failure
			return std::pair<std::string, TableGroupKey>("", TableGroupKey());
	}

	loadConfigurationBackbone();

	if(progressBar)
		progressBar->step();

	try
	{
		//	find runType in Group Aliases table
		ConfigurationTree entry =
		    getNode(ConfigurationManager::GROUP_ALIASES_TABLE_NAME).getNode(systemAlias);

		if(progressBar)
			progressBar->step();

		return std::pair<std::string, TableGroupKey>(
		    entry.getNode("GroupName").getValueAsString(),
		    TableGroupKey(entry.getNode("GroupKey").getValueAsString()));
	}
	catch(...)
	{
	}

	// on failure, here

	if(progressBar)
		progressBar->step();

	return std::pair<std::string, TableGroupKey>("", TableGroupKey());
}

//==============================================================================
std::map<std::string /*groupAlias*/, std::pair<std::string /*groupName*/, TableGroupKey>>
ConfigurationManager::getActiveGroupAliases(void)
{
	restoreActiveTableGroups();  // make sure the active configuration backbone is
	                                     // loaded!
	// loadConfigurationBackbone();

	std::map<std::string /*groupAlias*/,
	         std::pair<std::string /*groupName*/, TableGroupKey>>
	    retMap;

	std::vector<std::pair<std::string, ConfigurationTree>> entries =
	    getNode(ConfigurationManager::GROUP_ALIASES_TABLE_NAME).getChildren();
	for(auto& entryPair : entries)
	{
		retMap[entryPair.first] = std::pair<std::string, TableGroupKey>(
		    entryPair.second.getNode("GroupName").getValueAsString(),
		    TableGroupKey(entryPair.second.getNode("GroupKey").getValueAsString()));
	}
	return retMap;
}

//==============================================================================
// getVersionAliases()
//	get version aliases organized by table, for currently active backbone tables
std::map<std::string /*table name*/,
         std::map<std::string /*version alias*/, TableVersion /*aliased version*/>>
ConfigurationManager::getVersionAliases(void) const
{
	//__COUT__ << "getVersionAliases()" << __E__;

	std::map<std::string /*table name*/,
	         std::map<std::string /*version alias*/, TableVersion /*aliased version*/>>
	    retMap;

	std::map<std::string, TableVersion> activeVersions = getActiveVersions();
	std::string                         versionAliasesTableName =
	    ConfigurationManager::VERSION_ALIASES_TABLE_NAME;
	if(activeVersions.find(versionAliasesTableName) == activeVersions.end())
	{
		__SS__ << "Active version of VersionAliases  missing!"
		       << "Make sure you have a valid active Backbone Group." << __E__;
		__COUT_WARN__ << "\n" << ss.str();
		return retMap;
	}

	__COUT__ << "activeVersions[\"" << versionAliasesTableName
	         << "\"]=" << activeVersions[versionAliasesTableName] << __E__;

	std::vector<std::pair<std::string, ConfigurationTree>> aliasNodePairs =
	    getNode(versionAliasesTableName).getChildren();

	// create map
	//	add the first of each tableName, versionAlias pair encountered
	//	ignore any repeats (Note: this also prevents overwriting of Scratch alias)
	std::string tableName, versionAlias;
	for(auto& aliasNodePair : aliasNodePairs)
	{
		tableName = aliasNodePair.second.getNode("TableName").getValueAsString();
		versionAlias = aliasNodePair.second.getNode("VersionAlias").getValueAsString();

		if(retMap.find(tableName) != retMap.end() &&
		   retMap[tableName].find(versionAlias) != retMap[tableName].end())
			continue;  // skip repeats (Note: this also prevents overwriting of Scratch
			           // alias)

		// else add version to map
		retMap[tableName][versionAlias] =
		    TableVersion(aliasNodePair.second.getNode("Version").getValueAsString());
	}

	return retMap;
}  // end getVersionAliases()

//==============================================================================
// getActiveVersions
std::map<std::string, TableVersion> ConfigurationManager::getActiveVersions(void) const
{
	std::map<std::string, TableVersion> retMap;
	for(auto& config : nameToTableMap_)
	{
		//__COUT__ << config.first << __E__;

		// check configuration pointer is not null and that there is an active view
		if(config.second && config.second->isActive())
		{
			//__COUT__ << config.first << "_v" << config.second->getViewVersion() <<
			// __E__;
			retMap.insert(std::pair<std::string, TableVersion>(
			    config.first, config.second->getViewVersion()));
		}
	}
	return retMap;
}

////==============================================================================
// const DACStream& ConfigurationManager::getDACStream(std::string fecName)
//{
//
//	//fixme/todo this is called before setupAll so it breaks!
//	//====================================================
//	const DetectorConfiguration* detectorConfiguration =
//__GET_CONFIG__(DetectorConfiguration); 	for(auto& type :
// detectorConfiguration->getDetectorTypes()) 		theDACsConfigurations_[type] =
//(DACsTableBase*)(getTableByName(type + "DACsConfiguration"));
//	//====================================================
//
//	theDACStreams_[fecName].makeStream(fecName,
//			__GET_CONFIG__(DetectorConfiguration),
//			__GET_CONFIG__(DetectorToFEConfiguration),
//			theDACsConfigurations_,
//			__GET_CONFIG__(MaskConfiguration));//, theTrimConfiguration_);
//
//	__COUT__ << "Done with DAC stream!" << __E__;
//	return theDACStreams_[fecName];
//}

//==============================================================================
std::shared_ptr<TableGroupKey> ConfigurationManager::makeTheTableGroupKey(
    TableGroupKey key)
{
	if(theConfigurationTableGroupKey_)
	{
		if(*theConfigurationTableGroupKey_ != key)
			destroyTableGroup();
		else
			return theConfigurationTableGroupKey_;
	}
	return std::shared_ptr<TableGroupKey>(new TableGroupKey(key));
}

//==============================================================================
std::string ConfigurationManager::encodeURIComponent(const std::string& sourceStr)
{
	std::string retStr = "";
	char        encodeStr[4];
	for(const auto& c : sourceStr)
		if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
			retStr += c;
		else
		{
			sprintf(encodeStr, "%%%2.2X", c);
			retStr += encodeStr;
		}
	return retStr;
}

//==============================================================================
const std::set<std::string>& ConfigurationManager::getContextMemberNames()
{
	return ConfigurationManager::contextMemberNames_;
}
//==============================================================================
const std::set<std::string>& ConfigurationManager::getBackboneMemberNames()
{
	return ConfigurationManager::backboneMemberNames_;
}
//==============================================================================
const std::set<std::string>& ConfigurationManager::getIterateMemberNames()
{
	return ConfigurationManager::iterateMemberNames_;
}
