#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "artdaq/Application/LoadParameterSet.hh"
#include "otsdaq/ConfigurationInterface/ConfigurationInterface.h"  //All configurable objects are included here
#include "otsdaq/ProgressBar/ProgressBar.h"

#include <fstream>  // std::ofstream

#include "otsdaq/TableCore/TableGroupKey.h"
#include "otsdaq/TablePlugins/DesktopIconTable.h"  //for dynamic desktop icon change

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "ConfigurationManager"


const std::string ConfigurationManager::LAST_TABLE_GROUP_SAVE_PATH 			= std::string(__ENV__("SERVICE_DATA_PATH")) + "/RunControlData/";
const std::string ConfigurationManager::LAST_ACTIVATED_CONFIG_GROUP_FILE 	= "CFGLastActivatedConfigGroup.hist";
const std::string ConfigurationManager::LAST_ACTIVATED_CONTEXT_GROUP_FILE 	= "CFGLastActivatedContextGroup.hist";
const std::string ConfigurationManager::LAST_ACTIVATED_BACKBONE_GROUP_FILE 	= "CFGLastActivatedBackboneGroup.hist";
const std::string ConfigurationManager::LAST_ACTIVATED_ITERATOR_GROUP_FILE 	= "CFGLastActivatedIteratorGroup.hist";


const std::string ConfigurationManager::READONLY_USER = "READONLY_USER";

const std::string ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME      = "XDAQContextTable";
const std::string ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME  = "XDAQApplicationTable";
const std::string ConfigurationManager::XDAQ_APP_PROPERTY_TABLE_NAME = "XDAQApplicationPropertyTable";
const std::string ConfigurationManager::GROUP_ALIASES_TABLE_NAME     = "GroupAliasesTable";
const std::string ConfigurationManager::VERSION_ALIASES_TABLE_NAME   = "VersionAliasesTable";
const std::string ConfigurationManager::ARTDAQ_TOP_TABLE_NAME        = "ARTDAQSupervisorTable";
const std::string ConfigurationManager::DESKTOP_ICON_TABLE_NAME      = "DesktopIconTable";

// added env check for otsdaq_flatten_active_to_version to function
const std::string ConfigurationManager::ACTIVE_GROUPS_FILENAME =
    ((getenv("SERVICE_DATA_PATH") == NULL) ? (std::string(__ENV__("USER_DATA")) + "/ServiceData") : (std::string(__ENV__("SERVICE_DATA_PATH")))) +
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

const std::set<std::string> ConfigurationManager::contextMemberNames_  = {ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME,
                                                                         ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME,
                                                                         "XDAQApplicationPropertyTable",
                                                                         ConfigurationManager::DESKTOP_ICON_TABLE_NAME,
                                                                         "MessageFacilityTable",
                                                                         "GatewaySupervisorTable",
                                                                         "StateMachineTable",
                                                                         "DesktopWindowParameterTable",
                                                                         "SlowControlsDashboardSupervisorTable"};
const std::set<std::string> ConfigurationManager::backboneMemberNames_ = {ConfigurationManager::GROUP_ALIASES_TABLE_NAME,
                                                                          ConfigurationManager::VERSION_ALIASES_TABLE_NAME};
const std::set<std::string> ConfigurationManager::iterateMemberNames_  = {"IterateTable",
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
ConfigurationManager::ConfigurationManager(bool initForWriteAccess /*=false*/, bool doInitializeFromFhicl /*=false*/)
    : username_(ConfigurationManager::READONLY_USER)
    , theInterface_(0)
    , theConfigurationTableGroupKey_(0)
    , theContextTableGroupKey_(0)
    , theBackboneTableGroupKey_(0)
    , theConfigurationTableGroup_("")
    , theContextTableGroup_("")
    , theBackboneTableGroup_("")
    , groupMetadataTable_(true /*special table*/,ConfigurationInterface::GROUP_METADATA_TABLE_NAME)
{
	theInterface_ = ConfigurationInterface::getInstance(false);  // false to use artdaq DB

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

		groupMetadataTable_.setTableName(ConfigurationInterface::GROUP_METADATA_TABLE_NAME);
		std::vector<TableViewColumnInfo>* colInfo = groupMetadataTable_.getMockupViewP()->getColumnsInfoP();

		colInfo->push_back(TableViewColumnInfo(TableViewColumnInfo::TYPE_UID,  // just to make init() happy
		                                       "UnusedUID",
		                                       "UNUSED_UID",
		                                       TableViewColumnInfo::DATATYPE_NUMBER,
											   0 /*Default*/,
		                                       "",
		                                       0));
		colInfo->push_back(TableViewColumnInfo(TableViewColumnInfo::TYPE_DATA, "GroupAliases", "GROUP_ALIASES", TableViewColumnInfo::DATATYPE_STRING,
				   0 /*Default*/, "", 0));
		colInfo->push_back(TableViewColumnInfo(TableViewColumnInfo::TYPE_COMMENT,  // just to make init() happy
		                                       TableViewColumnInfo::COL_NAME_COMMENT,
		                                       "COMMENT_DESCRIPTION",
		                                       TableViewColumnInfo::DATATYPE_STRING,
											   0 /*Default*/,
		                                       "",
		                                       0));
		colInfo->push_back(TableViewColumnInfo(TableViewColumnInfo::TYPE_AUTHOR,  // just to make init() happy
		                                       "GroupAuthor",
		                                       "AUTHOR",
		                                       TableViewColumnInfo::DATATYPE_STRING,
											   0 /*Default*/,
		                                       "",
		                                       0));
		colInfo->push_back(
		    TableViewColumnInfo(TableViewColumnInfo::TYPE_TIMESTAMP, "GroupCreationTime", "GROUP_CREATION_TIME", TableViewColumnInfo::DATATYPE_TIME,
					   0 /*Default*/, "", 0));
		auto tmpVersion = groupMetadataTable_.createTemporaryView();
		groupMetadataTable_.setActiveView(tmpVersion);
		// only need this one and only row for all time
		groupMetadataTable_.getViewP()->addRow();
	}

	if(doInitializeFromFhicl)
	{
		// create tables and fill based on fhicl
		initializeFromFhicl(__ENV__("CONFIGURATION_INIT_FCL"));
		return;
	}
	// else do normal init
	init(0 /*accumulatedErrors*/, initForWriteAccess);

}  // end constructor()

//==============================================================================
ConfigurationManager::ConfigurationManager(const std::string& username) : ConfigurationManager(true /*initForWriteAccess*/)
{
	__COUT_INFO__ << "Private constructor for write access called." << __E__;
	username_ = username;
}  // end constructor(username)

//==============================================================================
ConfigurationManager::~ConfigurationManager() { destroy(); }

//==============================================================================
// init
//	if accumulatedErrors is not null.. fill it with errors
//	else throw errors (but do not ask restoreActiveTableGroups to throw errors)
//	Notes: Errors are handled separately from Warnings. Errors are used to monitor
//		errors but do not allow, and warnings are used to allow warnings and monitor.
void ConfigurationManager::init(std::string* accumulatedErrors /*=0*/, bool initForWriteAccess /*= false*/, std::string* accumulatedWarnings /*=0*/)
{
	// if(accumulatedErrors)
	//	*accumulatedErrors = "";

	// destroy();

	// once Interface is false (using artdaq db) ..
	//	then can test (configurationInterface_->getMode() == false)
	{
		try
		{
			__COUTV__(username_);
			// clang-format off
			restoreActiveTableGroups(accumulatedErrors ? true : false /*throwErrors*/,
				 "" /*pathToActiveGroupsFile*/,

				 // if write access, then load all specified table groups (including configuration group),
				 //	otherwise skip configuration group.
				 (username_ == ConfigurationManager::READONLY_USER) ?
						 (!initForWriteAccess)  // important to consider initForWriteAccess
						 // because this may be called before
						 // username_ is properly initialized
							: false /*onlyLoadIfBackboneOrContext*/,

				 accumulatedWarnings
			);
			// clang-format on
		}
		catch(std::runtime_error& e)
		{
			__COUT_ERR__ << "Error caught in init(): " << e.what();
			if(accumulatedErrors)
				*accumulatedErrors += e.what();
			else
			{
				__SS__ << e.what();  // add line number of rethrow
				__SS_ONLY_THROW__;
			}
		}
	}
}  // end init()

//==============================================================================
// restoreActiveTableGroups
//	load the active groups from file
//	Note: this should be used by the Supervisor to maintain
//		the same configurationGroups surviving software system restarts
void ConfigurationManager::restoreActiveTableGroups(bool               throwErrors /*=false*/,
                                                    const std::string& pathToActiveGroupsFile /*=""*/,
                                                    bool               onlyLoadIfBackboneOrContext /*= false*/,
                                                    std::string*       accumulatedWarnings /*=0*/)
{
	destroyTableGroup("", true);  // deactivate all

	std::string fn = pathToActiveGroupsFile == "" ? ACTIVE_GROUPS_FILENAME : pathToActiveGroupsFile;
	FILE*       fp = fopen(fn.c_str(), "r");

	__COUT__ << "ACTIVE_GROUPS_FILENAME = " << fn << __E__;
	__COUT__ << "ARTDAQ_DATABASE_URI = " << std::string(__ENV__("ARTDAQ_DATABASE_URI")) << __E__;

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

	while(fgets(tmp, 500, fp))
	{
		// do check for out of sync.. i.e. name is a number
		{
			int numberCheck = 0;
			sscanf(tmp, "%d", &numberCheck);
			if(numberCheck)
			{
				__COUT__ << "Out of sync with active groups file lines, attempting to resync." << __E__;
				continue;
			}
		}

		skip = false;
		sscanf(tmp, "%s", strVal);  // sscanf to remove '\n'
		for(unsigned int j = 0; j < strlen(strVal); ++j)
			if(!((strVal[j] >= 'a' && strVal[j] <= 'z') || (strVal[j] >= 'A' && strVal[j] <= 'Z') || (strVal[j] >= '0' && strVal[j] <= '9')))
			{
				strVal[j] = '\0';
				__COUT_INFO__ << "Illegal character found in group name '" << strVal << "', so skipping! Check active groups file: " << fn << __E__;

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
					__COUT_INFO__ << "Skipping active group with illegal character in group key '" << strVal << ".' Check active groups file: " << fn << __E__;

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
			__COUT__ << "illegal group according to TableGroupKey::getFullGroupString... "
			            "Check active groups file: "
			         << fn << __E__;
			skip = true;
		}

		if(skip)
			continue;

		try
		{
			// load and doActivate
			std::string groupAccumulatedErrors = "";

			if(accumulatedWarnings)
				__COUT__ << "Ignoring warnings while loading and activating group '" << groupName << "(" << strVal << ")'" << __E__;

			loadTableGroup(groupName,
			               TableGroupKey(strVal),
			               true /*doActivate*/,
			               0 /*groupMembers*/,
			               0 /*progressBar*/,
			               (accumulatedWarnings ? &groupAccumulatedErrors : 0) /*accumulateWarnings = 0*/,
			               0 /*groupComment       = 0*/,
			               0 /*groupAuthor        = 0*/,
			               0 /*groupCreateTime    = 0*/,
			               0 /*doNotLoadMembers   = false*/,
			               0 /*groupTypeString    = 0*/,
			               0 /*groupAliases       = 0*/,
			               onlyLoadIfBackboneOrContext /*onlyLoadIfBackboneOrContext = false*/
			);

			if(accumulatedWarnings)
				*accumulatedWarnings += groupAccumulatedErrors;
		}
		catch(std::runtime_error& e)
		{
			ss << "Failed to load group in ConfigurationManager::init() with name '" << groupName << "(" << strVal
			   << ")' specified active by active groups file: " << fn << __E__;
			ss << e.what() << __E__;

			errorStr += ss.str();
		}
		catch(...)
		{
			ss << "Failed to load group in ConfigurationManager::init() with name '" << groupName << "(" << strVal
			   << ")' specified active by active groups file: " << fn << __E__;

			errorStr += ss.str();
		}
	}

	fclose(fp);

	if(throwErrors && errorStr != "")
	{
		__SS__ << "\n" << errorStr;
		__SS_ONLY_THROW__;
	}
	else if(errorStr != "")
		__COUT_INFO__ << "\n" << errorStr;

}  // end restoreActiveTableGroups()

//==============================================================================
// destroyTableGroup
//	destroy all if theGroup == ""
//	else destroy that group
// 	if onlyDeactivate, then don't delete, just deactivate view
void ConfigurationManager::destroyTableGroup(const std::string& theGroup, bool onlyDeactivate)
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
		if(theGroup == "" || ((isContext && contextFindIt != contextMemberNames_.end()) || (isBackbone && backboneFindIt != backboneMemberNames_.end()) ||
		                      (isIterate && iterateFindIt != iterateMemberNames_.end()) ||
		                      (!isContext && !isBackbone && contextFindIt == contextMemberNames_.end() && backboneFindIt == backboneMemberNames_.end() &&
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
			__COUT__ << "Destroying Configuration Key: " << *theConfigurationTableGroupKey_ << __E__;
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
const std::string& ConfigurationManager::convertGroupTypeToName(const ConfigurationManager::GroupType& groupTypeId)
{
	return groupTypeId == ConfigurationManager::GroupType::CONTEXT_TYPE
	           ? ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT
	           : (groupTypeId == ConfigurationManager::GroupType::BACKBONE_TYPE
	                  ? ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE
	                  : (groupTypeId == ConfigurationManager::GroupType::ITERATE_TYPE ? ConfigurationManager::ACTIVE_GROUP_NAME_ITERATE
	                                                                                  : ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION));
}  // end convertGroupTypeToName()

//==============================================================================
// getTypeOfGroup
//	return
//		CONTEXT_TYPE for context
//		BACKBONE_TYPE for backbone
//		ITERATE_TYPE for iterate
//		CONFIGURATION_TYPE for configuration (others)
ConfigurationManager::GroupType ConfigurationManager::getTypeOfGroup(const std::map<std::string /*name*/, TableVersion /*version*/>& memberMap)
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

	if((isContext || inContext) && matchCount != contextMemberNames_.size())
	{
		__SS__ << "This group is an incomplete match to a Context group: "
		       << " Size=" << matchCount << " but should be " << contextMemberNames_.size() << __E__;
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

	if((isBackbone || inBackbone) && matchCount != backboneMemberNames_.size())
	{
		__SS__ << "This group is an incomplete match to a Backbone group: "
		       << " Size=" << matchCount << " but should be " << backboneMemberNames_.size() << __E__;
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

	if((isIterate || inIterate) && matchCount != iterateMemberNames_.size())
	{
		__SS__ << "This group is an incomplete match to a Iterate group: "
		       << " Size=" << matchCount << " but should be " << iterateMemberNames_.size() << __E__;
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

	return isContext ? ConfigurationManager::GroupType::CONTEXT_TYPE
	                 : (isBackbone ? ConfigurationManager::GroupType::BACKBONE_TYPE
	                               : (isIterate ? ConfigurationManager::GroupType::ITERATE_TYPE : ConfigurationManager::GroupType::CONFIGURATION_TYPE));
}  // end getTypeOfGroup()

//==============================================================================
// getTypeNameOfGroup
//	return string for group type
const std::string& ConfigurationManager::getTypeNameOfGroup(const std::map<std::string /*name*/, TableVersion /*version*/>& memberMap)
{
	return convertGroupTypeToName(getTypeOfGroup(memberMap));
}  // end getTypeNameOfGroup()

//==============================================================================
// dumpMacroMakerModeFhicl
//	Generates functional configuration file for MacroMaker mode
//	based on current configuration.

// helpers
#define OUT out << tabStr << commentStr
#define PUSHTAB tabStr += "\t"
#define POPTAB tabStr.resize(tabStr.size() - 1)
#define PUSHCOMMENT commentStr += "# "
#define POPCOMMENT commentStr.resize(commentStr.size() - 2)

void ConfigurationManager::dumpMacroMakerModeFhicl()
{
	std::string filepath = __ENV__("USER_DATA") + std::string("/") + "MacroMakerModeConfigurations";
	mkdir(filepath.c_str(), 0755);
	filepath += "/MacroMakerModeFhiclDump.fcl";
	__COUT__ << "dumpMacroMakerModeFhicl: " << filepath << __E__;

	/////////////////////////
	// generate MacroMaker mode fhicl file
	std::fstream out;

	std::string tabStr     = "";
	std::string commentStr = "";

	out.open(filepath, std::fstream::out | std::fstream::trunc);
	if(out.fail())
	{
		__SS__ << "Failed to open MacroMaker mode fcl file for configuration dump: " << filepath << __E__;
		__SS_THROW__;
	}

	try
	{
		std::vector<std::pair<std::string, ConfigurationTree>> fes = getNode("FEInterfaceTable").getChildren();

		for(auto& fe : fes)
		{
			// skip status false
			if(!fe.second.getNode("Status").getValue<bool>())
				continue;

			__COUTV__(fe.first);

			OUT << fe.first << ": {" << __E__;
			PUSHTAB;

			// only do FEInterfacePluginName and LinkToFETypeTable at top level

			OUT << "FEInterfacePluginName"
			    << ": \t"
			    << "\"" << fe.second.getNode("FEInterfacePluginName").getValueAsString() << "\"" << __E__;

			recursiveTreeToFhicl(fe.second.getNode("LinkToFETypeTable"), out, tabStr, commentStr);

			POPTAB;
			OUT << "} //end " << fe.first << __E__ << __E__;

		}  // end fe handling
	}
	catch(...)
	{
		__COUT_ERR__ << "Failed to complete MacroMaker mode fcl "
		                "file configuration dump due to error."
		             << __E__;
	}

	out.close();
	// end fhicl output
}  // end dumpMacroMakerModeFhicl()

//==============================================================================
// recursiveTreeToFhicl
//		Output from treeRecord to specified depth
//		depth of -1 is translated to "a lot" (e.g. 10)
//		to avoid infinite loops.
//
//		The node must be a UID or link node
//
//	e.g., out = std::cout, tabStr = "", commentStr = ""
void ConfigurationManager::recursiveTreeToFhicl(ConfigurationTree node,
                                                std::ostream&     out /* = std::cout */,
                                                std::string&      tabStr /* = "" */,
                                                std::string&      commentStr /* = "" */,
                                                unsigned int      depth /* = -1 */)
{
	if(depth == 0)
	{
		__COUT__ << __COUT_HDR_P__ << "Depth limit reached. Ending recursion." << __E__;
		return;
	}

	//__COUT__ << __COUT_HDR_P__ << "Adding tree record '" << node.getValueAsString() << "' fields..." << __E__;

	if(depth == (unsigned int)-1)
		depth = 10;

	// decorate link node with link_table {} wrapper
	if(node.isLinkNode())
	{
		if(node.isDisconnected())
		{
			__COUT__ << node.getFieldName() << " field is a disconnected link." << __E__;
			return;
		}

		OUT << node.getFieldName() << "_" << node.getValueAsString(true /* returnLinkTableValue */) << " \t{" << __E__;
		PUSHTAB;
	}  // end link preamble decoration

	if(node.isGroupLinkNode())
	{
		// for group link, handle each as a UID record
		std::vector<std::pair<std::string, ConfigurationTree>> children = node.getChildren();
		for(auto& child : children)
			recursiveTreeToFhicl(child.second, out, tabStr, commentStr, depth - 1);

		return;
	}  // end group link handling

	// treat as UID record now
	//	give UID decoration, then contents

	// open UID decoration
	OUT << node.getValueAsString() << ": \t{" << __E__;
	PUSHTAB;
	{  // open UID content

		std::vector<std::pair<std::string, ConfigurationTree>> fields = node.getChildren();

		// skip last 3 fields that are always common
		for(unsigned int i = 0; i < fields.size() - 3; ++i)
		{
		  //__COUT__ << fields[i].first << __E__;

			if(fields[i].second.isLinkNode())
			{
				recursiveTreeToFhicl(fields[i].second, out, tabStr, commentStr, depth - 1);
				continue;
			}
			// else a normal field

			OUT << fields[i].second.getFieldName() << ": \t";
			if(fields[i].second.isValueNumberDataType())
				OUT << fields[i].second.getValueAsString() << __E__;
			else
				OUT << "\"" << fields[i].second.getValueAsString() << "\"" << __E__;

		}  // end fe fields

	}        // close UID content
	POPTAB;  // close UID decoration
	OUT << "} //end " << node.getValueAsString() << " record" << __E__;

	// handle link closing decoration
	if(node.isLinkNode())
	{
		POPTAB;
		OUT << "} //end " << node.getValueAsString(true /* returnLinkTableValue */) << " link record" << __E__;
	}  // end link closing decoration

}  // end recursiveTreeToFhicl

//==============================================================================
// dumpActiveConfiguration
//	if filePath == "", then output to cout
void ConfigurationManager::dumpActiveConfiguration(const std::string& filePath, const std::string& dumpType)
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
			__SS__ << "Invalid file path to dump active configuration. File " << filePath << " could not be opened!" << __E__;
			__COUT_ERR__ << ss.str();
			__SS_THROW__;
		}
		out = &(std::cout);
	}

	(*out) << "#################################" << __E__;
	(*out) << "This is an ots configuration dump.\n\n" << __E__;
	(*out) << "Source database is $ARTDAQ_DATABASE_URI = \t" << __ENV__("ARTDAQ_DATABASE_URI") << __E__;
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

	auto localDumpActiveGroups = [](const ConfigurationManager* cfgMgr, std::ostream* out) {
		std::map<std::string, std::pair<std::string, TableGroupKey>> activeGroups = cfgMgr->getActiveTableGroups();

		(*out) << "\n\n************************" << __E__;
		(*out) << "Active Groups:" << __E__;
		for(auto& group : activeGroups)
		{
			(*out) << "\t" << group.first << " := " << group.second.first << " (" << group.second.second << ")" << __E__;
		}
	};

	auto localDumpActiveTables = [](const ConfigurationManager* cfgMgr, std::ostream* out) {
		std::map<std::string, TableVersion> activeTables = cfgMgr->getActiveVersions();

		(*out) << "\n\n************************" << __E__;
		(*out) << "Active Tables:" << __E__;
		(*out) << "Active Tables count = " << activeTables.size() << __E__;

		unsigned int i = 0;
		for(auto& table : activeTables)
		{
			(*out) << "\t" << ++i << ". " << table.first << "-v" << table.second << __E__;
		}
	};

	auto localDumpActiveGroupMembers = [](ConfigurationManager* cfgMgr, std::ostream* out) {
		std::map<std::string, std::pair<std::string, TableGroupKey>> activeGroups = cfgMgr->getActiveTableGroups();
		(*out) << "\n\n************************" << __E__;
		(*out) << "Active Group Members:" << __E__;
		int tableCount = 0;
		for(auto& group : activeGroups)
		{
			(*out) << "\t" << group.first << " := " << group.second.first << " (" << group.second.second << ")" << __E__;

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
			                       true /*doNotLoadMembers*/,
			                       0 /*groupTypeString*/,
			                       &groupAliases);

			(*out) << "\t\tGroup Comment: \t" << groupComment << __E__;
			(*out) << "\t\tGroup Author: \t" << groupAuthor << __E__;

			sscanf(groupCreateTime.c_str(), "%ld", &groupCreateTime_t);
			(*out) << "\t\tGroup Create Time: \t" << ctime(&groupCreateTime_t) << __E__;
			(*out) << "\t\tGroup Aliases: \t" << StringMacros::mapToString(groupAliases) << __E__;

			(*out) << "\t\tMember table count = " << memberMap.size() << __E__;
			tableCount += memberMap.size();

			unsigned int i = 0;
			for(auto& member : memberMap)
			{
				(*out) << "\t\t\t" << ++i << ". " << member.first << "-v" << member.second << __E__;
			}
		}
		(*out) << "\nActive Group Members total table count = " << tableCount << __E__;
	};

	auto localDumpActiveTableContents = [](const ConfigurationManager* cfgMgr, std::ostream* out) {
		std::map<std::string, TableVersion> activeTables = cfgMgr->getActiveVersions();

		(*out) << "\n\n************************" << __E__;
		(*out) << "Active Table Contents (table count = " << activeTables.size() << "):" << __E__;
		unsigned int i = 0;
		for(auto& table : activeTables)
		{
			(*out) << "\n\n=============================================================="
			          "================"
			       << __E__;
			(*out) << "=================================================================="
			          "============"
			       << __E__;
			(*out) << "\t" << ++i << ". " << table.first << "-v" << table.second << __E__;

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
		__SS__ << "Invalid dump type '" << dumpType << "' given during dumpActiveConfiguration(). Valid types are as follows:\n"
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

		    "\n\nPlease change the State Machine configuration to a valid dump type." << __E__;
		__SS_THROW__;
	}

	if(fs.is_open())
		fs.close();
}

//==============================================================================
// loadMemberMap
//	loads tables given by name/version pairs in memberMap
//	Note: does not activate them.
//
// if accumulateWarnings, then put in string, do not throw
void ConfigurationManager::loadMemberMap(const std::map<std::string /*name*/, TableVersion /*version*/>& memberMap, std::string* accumulateWarnings /* =0 */)
{
	TableBase* tmpConfigBasePtr;
	//	for each member
	//		get()
	for(auto& memberPair : memberMap)
	{
		//		if(accumulateWarnings)
		//			__COUT__ << "\tMember config " << memberPair.first << ":" <<
		//				memberPair.second << __E__;

		// get the proper temporary pointer
		//	use 0 if doesn't exist yet.
		//	Note: do not want to give nameToTableMap_[memberPair.first]
		//		in case there is failure in get... (exceptions may be thrown)
		// Note: Default constructor is called by Map, i.e.
		// nameToTableMap_[memberPair.first] = 0; //create pointer and set to 0
		tmpConfigBasePtr = 0;
		if(nameToTableMap_.find(memberPair.first) != nameToTableMap_.end())
			tmpConfigBasePtr = nameToTableMap_[memberPair.first];

		std::string getError = "";
		try
		{
			theInterface_->get(tmpConfigBasePtr,   // configurationPtr
			                   memberPair.first,   // tableName
			                   0,                  // groupKey
			                   0,                  // groupName
			                   false,              // dontFill=false to fill
			                   memberPair.second,  // version
			                   false               // resetTable
			);
		}
		catch(const std::runtime_error& e)
		{
			__SS__ << "Failed to load member table '" << memberPair.first << "' - here is the error: \n\n" << e.what() << __E__;

			ss << "\nIf the table '" << memberPair.first
			   << "' should not exist, then please remove it from the group. If it "
			      "should exist, then it "
			   << "seems to have a problem; use the Table Editor to fix the table "
			      "definition, or "
			      "edit the table content to match the table definition."
			   << __E__;

			// if accumulating warnings and table view was created, then continue
			if(accumulateWarnings)
				getError = ss.str();
			else
				__SS_ONLY_THROW__;
		}
		catch(...)
		{
			__SS__ << "Failed to load member table '" << memberPair.first << "' due to unknown error!" << __E__;

			ss << "\nIf the table '" << memberPair.first
			   << "' should not exist, then please remove it from the group. If it "
			      "should exist, then it "
			   << "seems to have a problem; use the Table Editor to fix the table "
			      "definition, or "
			      "edit the table content to match the table definition."
			   << __E__;

			// if accumulating warnings and table view was created, then continue
			if(accumulateWarnings)
				getError = ss.str();
			else
				__SS_THROW__;
		}

		//__COUT__ << "Checking ptr.. " <<  (tmpConfigBasePtr?"GOOD":"BAD") << __E__;
		if(!tmpConfigBasePtr)
		{
			__SS__ << "Null pointer returned for table '" << memberPair.first << ".' Was the table info deleted?" << __E__;
			__COUT_ERR__ << ss.str();

			nameToTableMap_.erase(memberPair.first);
			if(accumulateWarnings)
			{
				*accumulateWarnings += ss.str();
				continue;
			}
			else
				__SS_ONLY_THROW__;
		}

		nameToTableMap_[memberPair.first] = tmpConfigBasePtr;
		if(nameToTableMap_[memberPair.first]->getViewP())
		{
			//__COUT__ << "Activated version: " <<
			// nameToTableMap_[memberPair.first]->getViewVersion() << __E__;

			if(accumulateWarnings && getError != "")
			{
				__SS__ << "Error caught during '" << memberPair.first << "' table retrieval: \n" << getError << __E__;
				__COUT_ERR__ << ss.str();
				*accumulateWarnings += ss.str();
			}
		}
		else
		{
			__SS__ << nameToTableMap_[memberPair.first]->getTableName() << ": View version not activated properly!";
			__SS_THROW__;
		}
	}  // end member map loop
}  // end loadMemberMap()

//==============================================================================
// loadTableGroup
//	load all members of configuration group
//	if doActivate
//		DOES set theConfigurationTableGroup_, theContextTableGroup_, or
// theBackboneTableGroup_ on success 			this also happens with
// ConfigurationManagerRW::activateTableGroup 		for each member
//			configBase->init()
//
//	if progressBar != 0, then do step handling, for finer granularity
//
// 	if(doNotLoadMembers) return memberMap; //this is useful if just getting group metadata
//	else NOTE: active views are changed! (when loading member map)
//
//	throws exception on failure.
// 	if accumulatedTreeErrors, then "ignore warnings"
void ConfigurationManager::loadTableGroup(const std::string&                                     groupName,
                                          TableGroupKey                                          groupKey,
                                          bool                                                   doActivate /*=false*/,
                                          std::map<std::string /*table name*/, TableVersion>*    groupMembers,
                                          ProgressBar*                                           progressBar,
                                          std::string*                                           accumulatedWarnings,
                                          std::string*                                           groupComment,
                                          std::string*                                           groupAuthor,
                                          std::string*                                           groupCreateTime,
                                          bool                                                   doNotLoadMembers /*=false*/,
                                          std::string*                                           groupTypeString,
                                          std::map<std::string /*name*/, std::string /*alias*/>* groupAliases,
                                          bool                                                   onlyLoadIfBackboneOrContext /*=false*/) try
{
	// clear to defaults
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
	//		set theConfigurationTableGroup_, theContextTableGroup_, or
	// theBackboneTableGroup_ on success

	//	__COUT_INFO__ << "Loading Table Group: " << groupName <<
	//			"(" << groupKey << ")" << __E__;

	std::map<std::string /*name*/, TableVersion /*version*/> memberMap =
	    theInterface_->getTableGroupMembers(TableGroupKey::getFullGroupString(groupName, groupKey), true /*include meta data table*/);
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

			if(groupTypeString)  // do before exit case
				*groupTypeString = convertGroupTypeToName(getTypeOfGroup(memberMap));

			return;  // memberMap;
		}

		// groupMetadataTable_.print();

		// extract fields
		StringMacros::getMapFromString(groupMetadataTable_.getView().getValueAsString(0, ConfigurationManager::METADATA_COL_ALIASES), aliasMap);
		if(groupAliases)
			*groupAliases = aliasMap;
		if(groupComment)
			*groupComment = groupMetadataTable_.getView().getValueAsString(0, ConfigurationManager::METADATA_COL_COMMENT);
		if(groupAuthor)
			*groupAuthor = groupMetadataTable_.getView().getValueAsString(0, ConfigurationManager::METADATA_COL_AUTHOR);
		if(groupCreateTime)
			*groupCreateTime = groupMetadataTable_.getView().getValueAsString(0, ConfigurationManager::METADATA_COL_TIMESTAMP);

		// modify members based on aliases
		{
			std::map<std::string /*table*/, std::map<std::string /*alias*/, TableVersion>> versionAliases;
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
					__COUT__ << "Group member '" << aliasPair.first << "' was found in group member map!" << __E__;
					__COUT__ << "Looking for alias '" << aliasPair.second << "' in active version aliases..." << __E__;

					if(versionAliases.find(aliasPair.first) == versionAliases.end() ||
					   versionAliases[aliasPair.first].find(aliasPair.second) == versionAliases[aliasPair.first].end())
					{
						__SS__ << "Group '" << groupName << "(" << groupKey << ")' requires table version alias '" << aliasPair.first << ":" << aliasPair.second
						       << ",' which was not found in the active Backbone!" << __E__;
						__SS_ONLY_THROW__;
					}

					memberMap[aliasPair.first] = versionAliases[aliasPair.first][aliasPair.second];
					__COUT__ << "Version alias translated to " << aliasPair.first << __E__;
				}
			}
		}
	}  // end metadata handling

	if(groupMembers)
		*groupMembers = memberMap;  // copy map for return

	if(progressBar)
		progressBar->step();

	//__COUT__ << "memberMap loaded size = " << memberMap.size() << __E__;

	ConfigurationManager::GroupType groupType = ConfigurationManager::GroupType::CONFIGURATION_TYPE;
	try
	{
		if(groupTypeString)  // do before exit case
		{
			groupType        = getTypeOfGroup(memberMap);
			*groupTypeString = convertGroupTypeToName(groupType);
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

		__COUTV__(doNotLoadMembers);
		__COUT__ << "group '" << groupName << "(" << groupKey << ")' := " << StringMacros::mapToString(memberMap) << __E__;
		if(doNotLoadMembers)
			return;  // memberMap; //this is useful if just getting group metadata

		// if not already done, determine the type configuration group
		if(!groupTypeString)
			groupType = getTypeOfGroup(memberMap);

		if(onlyLoadIfBackboneOrContext && groupType != ConfigurationManager::GroupType::CONTEXT_TYPE &&
		   groupType != ConfigurationManager::GroupType::BACKBONE_TYPE)
		{
			__COUT_WARN__ << "Not loading group because it is not of type Context or "
			                 "Backbone (it is type '"
			              << convertGroupTypeToName(groupType) << "')." << __E__;
			return;
		}

		if(doActivate)
			__COUT__ << "------------------------------------- init start    \t [for all "
			            "plug-ins in "
			         << convertGroupTypeToName(groupType) << " group '" << groupName << "(" << groupKey << ")"
			         << "']" << __E__;

		if(doActivate)
		{
			std::string groupToDeactivate =
			    groupType == ConfigurationManager::GroupType::CONTEXT_TYPE
			        ? theContextTableGroup_
			        : (groupType == ConfigurationManager::GroupType::BACKBONE_TYPE
			               ? theBackboneTableGroup_
			               : (groupType == ConfigurationManager::GroupType::ITERATE_TYPE ? theIterateTableGroup_ : theConfigurationTableGroup_));

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

		//__COUT__ << "Loading member map..." << __E__;

		loadMemberMap(memberMap, accumulatedWarnings);

		//__COUT__ << "Member map loaded..." << __E__;

		if(progressBar)
			progressBar->step();

		if(accumulatedWarnings)
		{
			//__COUT__ << "Checking chosen group for tree errors..." << __E__;

			getChildren(&memberMap, accumulatedWarnings);
			if(*accumulatedWarnings != "")
			{
				__COUT_ERR__ << "Errors detected while loading Table Group: " << groupName << "(" << groupKey << "). Ignoring the following errors: "
				             << "\n"
				             << *accumulatedWarnings << __E__;
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
				if(ConfigurationInterface::isVersionTrackingEnabled() && memberPair.second.isScratchVersion())
				{
					__SS__ << "Error while activating member Table '" << nameToTableMap_[memberPair.first]->getTableName() << "-v" << memberPair.second
					       << " for Table Group '" << groupName << "(" << groupKey << ")'. When version tracking is enabled, Scratch views"
					       << " are not allowed! Please only use unique, persistent "
					          "versions when version tracking is enabled."
					       << __E__;
					__SS_ONLY_THROW__;
				}

				// attempt to init using the configuration's specific init
				//	this could be risky user code, try and catch
				try
				{
					nameToTableMap_.at(memberPair.first)->init(this);
				}
				catch(std::runtime_error& e)
				{
					__SS__ << "Error detected calling " << memberPair.first << ".init()!\n\n " << e.what() << __E__;

					if(accumulatedWarnings)
					{
						*accumulatedWarnings += ss.str();
					}
					else
					{
						ss << StringMacros::stackTrace();
						__SS_ONLY_THROW__;
					}
				}
				catch(...)
				{
					__SS__ << "Unknown Error detected calling " << memberPair.first << ".init()!\n\n " << __E__;
					//__SS_THROW__;
					if(accumulatedWarnings)
					{
						*accumulatedWarnings += ss.str();
					}
					else  // ignore error
						__COUT_WARN__ << ss.str();
				}
			}

		if(progressBar)
			progressBar->step();

		//	if doActivate
		//		set theConfigurationTableGroup_, theContextTableGroup_, or
		// theBackboneTableGroup_ on
		// success

		if(doActivate)
		{
			if(groupType == ConfigurationManager::GroupType::CONTEXT_TYPE)  //
			{
				//			__COUT_INFO__ << "Type=Context, Group loaded: " <<
				// groupName
				//<<
				//					"(" << groupKey << ")" << __E__;
				theContextTableGroup_    = groupName;
				theContextTableGroupKey_ = std::shared_ptr<TableGroupKey>(new TableGroupKey(groupKey));
			}
			else if(groupType == ConfigurationManager::GroupType::BACKBONE_TYPE)
			{
				//			__COUT_INFO__ << "Type=Backbone, Group loaded: " <<
				// groupName <<
				//					"(" << groupKey << ")" << __E__;
				theBackboneTableGroup_    = groupName;
				theBackboneTableGroupKey_ = std::shared_ptr<TableGroupKey>(new TableGroupKey(groupKey));
			}
			else if(groupType == ConfigurationManager::GroupType::ITERATE_TYPE)
			{
				//			__COUT_INFO__ << "Type=Iterate, Group loaded: " <<
				// groupName
				//<<
				//					"(" << groupKey << ")" << __E__;
				theIterateTableGroup_    = groupName;
				theIterateTableGroupKey_ = std::shared_ptr<TableGroupKey>(new TableGroupKey(groupKey));
			}
			else  // is theConfigurationTableGroup_
			{
				//			__COUT_INFO__ << "Type=Configuration, Group loaded: " <<
				// groupName <<
				//					"(" << groupKey << ")" << __E__;
				theConfigurationTableGroup_    = groupName;
				theConfigurationTableGroupKey_ = std::shared_ptr<TableGroupKey>(new TableGroupKey(groupKey));
			}
		}

		if(progressBar)
			progressBar->step();

		if(doActivate)
			__COUT__ << "------------------------------------- init complete \t [for all "
			            "plug-ins in "
			         << convertGroupTypeToName(groupType) << " group '" << groupName << "(" << groupKey << ")"
			         << "']" << __E__;
	}  // end failed group load try
	catch(...)
	{
		// save group name and key of failed load attempt
		lastFailedGroupLoad_[convertGroupTypeToName(groupType)] = std::pair<std::string, TableGroupKey>(groupName, TableGroupKey(groupKey));

		try
		{
			throw;
		}
		catch(const std::runtime_error& e)
		{
			__SS__ << "Error occurred while loading table group '" << groupName << "(" << groupKey << ")': \n" << e.what() << __E__;

			if(accumulatedWarnings)
				*accumulatedWarnings += ss.str();
			else
				__SS_ONLY_THROW__;
		}
		catch(...)
		{
			__SS__ << "An unknown error occurred while loading table group '" << groupName << "(" << groupKey << ")." << __E__;

			if(accumulatedWarnings)
				*accumulatedWarnings += ss.str();
			else
				__SS_ONLY_THROW__;
		}
	}

	return;
}
catch(...)
{
	// save group name and key of failed load attempt
	lastFailedGroupLoad_[ConfigurationManager::ACTIVE_GROUP_NAME_UNKNOWN] = std::pair<std::string, TableGroupKey>(groupName, TableGroupKey(groupKey));

	try
	{
		throw;
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error occurred while loading table group: " << e.what() << __E__;

		if(accumulatedWarnings)
			*accumulatedWarnings += ss.str();
		else
			__SS_ONLY_THROW__;
	}
	catch(...)
	{
		__SS__ << "An unknown error occurred while loading table group." << __E__;

		if(accumulatedWarnings)
			*accumulatedWarnings += ss.str();
		else
			__SS_ONLY_THROW__;
	}
}  // end loadTableGroup()

//==============================================================================
// getActiveTableGroups
//	get the active table groups map
//   map<type,        pair     <groupName  , TableGroupKey> >
//
//	Note: invalid TableGroupKey means no active group currently
std::map<std::string, std::pair<std::string, TableGroupKey>> ConfigurationManager::getActiveTableGroups(void) const
{
	//   map<type,        pair     <groupName  , TableGroupKey> >
	std::map<std::string, std::pair<std::string, TableGroupKey>> retMap;

	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT] =
	    std::pair<std::string, TableGroupKey>(theContextTableGroup_, theContextTableGroupKey_ ? *theContextTableGroupKey_ : TableGroupKey());
	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE] =
	    std::pair<std::string, TableGroupKey>(theBackboneTableGroup_, theBackboneTableGroupKey_ ? *theBackboneTableGroupKey_ : TableGroupKey());
	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_ITERATE] =
	    std::pair<std::string, TableGroupKey>(theIterateTableGroup_, theIterateTableGroupKey_ ? *theIterateTableGroupKey_ : TableGroupKey());
	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION] =
	    std::pair<std::string, TableGroupKey>(theConfigurationTableGroup_, theConfigurationTableGroupKey_ ? *theConfigurationTableGroupKey_ : TableGroupKey());
	return retMap;
}  // end getActiveTableGroups()

//==============================================================================
const std::string& ConfigurationManager::getActiveGroupName(const ConfigurationManager::GroupType& type) const
{
	if(type == ConfigurationManager::GroupType::CONFIGURATION_TYPE)
		return theConfigurationTableGroup_;
	else if(type == ConfigurationManager::GroupType::CONTEXT_TYPE)
		return theContextTableGroup_;
	else if(type == ConfigurationManager::GroupType::BACKBONE_TYPE)
		return theBackboneTableGroup_;
	else if(type == ConfigurationManager::GroupType::ITERATE_TYPE)
		return theIterateTableGroup_;

	__SS__ << "IMPOSSIBLE! Invalid type requested '" << (int)type << "'" << __E__;
	__SS_THROW__;
}  // end getActiveGroupName()

//==============================================================================
TableGroupKey ConfigurationManager::getActiveGroupKey(const ConfigurationManager::GroupType& type) const
{
	if(type == ConfigurationManager::GroupType::CONFIGURATION_TYPE)
		return theConfigurationTableGroupKey_ ? *theConfigurationTableGroupKey_ : TableGroupKey();
	else if(type == ConfigurationManager::GroupType::CONTEXT_TYPE)
		return theContextTableGroupKey_ ? *theContextTableGroupKey_ : TableGroupKey();
	else if(type == ConfigurationManager::GroupType::BACKBONE_TYPE)
		return theBackboneTableGroupKey_ ? *theBackboneTableGroupKey_ : TableGroupKey();
	else if(type == ConfigurationManager::GroupType::ITERATE_TYPE)
		return theIterateTableGroupKey_ ? *theIterateTableGroupKey_ : TableGroupKey();

	__SS__ << "IMPOSSIBLE! Invalid type requested '" << (int)type << "'" << __E__;
	__SS_THROW__;
}  // end getActiveGroupKey()

//==============================================================================
ConfigurationTree ConfigurationManager::getContextNode(const std::string& contextUID, const std::string& /*applicationUID*/) const
{
	return getNode("/" + getTableByName(XDAQ_CONTEXT_TABLE_NAME)->getTableName() + "/" + contextUID);
}  // end getContextNode()

//==============================================================================
ConfigurationTree ConfigurationManager::getSupervisorNode(const std::string& contextUID, const std::string& applicationUID) const
{
	return getNode("/" + getTableByName(XDAQ_CONTEXT_TABLE_NAME)->getTableName() + "/" + contextUID + "/LinkToApplicationTable/" + applicationUID);
}

//==============================================================================
ConfigurationTree ConfigurationManager::getSupervisorTableNode(const std::string& contextUID, const std::string& applicationUID) const
{
	return getNode("/" + getTableByName(XDAQ_CONTEXT_TABLE_NAME)->getTableName() + "/" + contextUID + "/LinkToApplicationTable/" + applicationUID +
	               "/LinkToSupervisorTable");
}

//==============================================================================
ConfigurationTree ConfigurationManager::getNode(const std::string& nodeString, bool doNotThrowOnBrokenUIDLinks) const
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

	std::string nodeName = nodeString.substr(startingIndex, nodeString.find('/', startingIndex) - startingIndex);
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
std::string ConfigurationManager::getFirstPathToNode(const ConfigurationTree& /*node*/, const std::string& /*startPath*/) const
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
std::vector<std::pair<std::string, ConfigurationTree>> ConfigurationManager::getChildren(std::map<std::string, TableVersion>* memberMap,
                                                                                         std::string*                         accumulatedTreeErrors) const
{
	std::vector<std::pair<std::string, ConfigurationTree>> retMap;

	// if(accumulatedTreeErrors)
	//	*accumulatedTreeErrors = "";

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
						std::vector<std::pair<std::string, ConfigurationTree>> newNodeChildren = newNode.getChildren();
						for(auto& newNodeChild : newNodeChildren)
						{
							std::vector<std::pair<std::string, ConfigurationTree>> twoDeepChildren = newNodeChild.second.getChildren();

							for(auto& twoDeepChild : twoDeepChildren)
							{
								//__COUT__ << configPair.first << " " <<
								// newNodeChild.first << " " << 		twoDeepChild.first
								// <<  __E__;
								if(twoDeepChild.second.isLinkNode() && twoDeepChild.second.isDisconnected() &&
								   twoDeepChild.second.getDisconnectedTableName() != TableViewColumnInfo::DATATYPE_LINK_DEFAULT)
								{
									__SS__ << "At node '" + configPair.first + "' with entry UID '" + newNodeChild.first +
									              "' there is a disconnected child node at link "
									              "column '" +
									              twoDeepChild.first + "'" + " that points to table named '" + twoDeepChild.second.getDisconnectedTableName() +
									              "' ...";
									*accumulatedTreeErrors += ss.str();
								}
							}
						}
					}
					catch(std::runtime_error& e)
					{
						__SS__ << "At node '" + configPair.first + "' error detected descending through children:\n" + e.what();
						*accumulatedTreeErrors += ss.str();
					}
				}

				retMap.push_back(std::pair<std::string, ConfigurationTree>(configPair.first, newNode));
			}

			//__COUT__ << configPair.first <<  __E__;
		}
	}
	else  // return only members from the member map (they must be present and active!)
	{
		for(auto& memberPair : *memberMap)
		{
			auto mapIt = nameToTableMap_.find(memberPair.first);
			try
			{
				if(mapIt == nameToTableMap_.end())
				{
					__SS__ << "Get Children with member map requires a child '" << memberPair.first << "' that is not present!" << __E__;
					__SS_THROW__;
				}
				if(!(*mapIt).second->isActive())
				{
					__SS__ << "Get Children with member map requires a child '" << memberPair.first << "' that is not active!" << __E__;
					__SS_THROW__;
				}
			}
			catch(const std::runtime_error& e)
			{
				if(accumulatedTreeErrors)
				{
					*accumulatedTreeErrors += e.what();
					__COUT_ERR__ << "Skipping " << memberPair.first
					             << " since the table "
					                "is not active."
					             << __E__;
					continue;
				}
			}

			ConfigurationTree newNode(this, (*mapIt).second);

			if(accumulatedTreeErrors)  // check for disconnects
			{
				try
				{
					std::vector<std::pair<std::string, ConfigurationTree>> newNodeChildren = newNode.getChildren();
					for(auto& newNodeChild : newNodeChildren)
					{
						std::vector<std::pair<std::string, ConfigurationTree>> twoDeepChildren = newNodeChild.second.getChildren();

						for(auto& twoDeepChild : twoDeepChildren)
						{
							//__COUT__ << memberPair.first << " " << newNodeChild.first <<
							//" " << 		twoDeepChild.first << __E__;
							if(twoDeepChild.second.isLinkNode() && twoDeepChild.second.isDisconnected() &&
							   twoDeepChild.second.getDisconnectedTableName() != TableViewColumnInfo::DATATYPE_LINK_DEFAULT)
							{
								__SS__ << "At node '" + memberPair.first + "' with entry UID '" + newNodeChild.first +
								              "' there is a disconnected child node at link column "
								              "'" +
								              twoDeepChild.first + "'" + " that points to table named '" + twoDeepChild.second.getDisconnectedTableName() +
								              "' ...";
								*accumulatedTreeErrors += ss.str();

								// check if disconnected table is in group, if not
								// software error

								bool found = false;
								for(auto& searchMemberPair : *memberMap)
									if(searchMemberPair.first == twoDeepChild.second.getDisconnectedTableName())
									{
										found = true;
										break;
									}
								if(!found)
								{
									__SS__ << "Note: It may be safe to ignore this "
									       << "error since the link's target table " << twoDeepChild.second.getDisconnectedTableName()
									       << " is not a member of this group (and may not "
									          "be "
									       << "loaded yet)" << __E__;
									*accumulatedTreeErrors += ss.str();
								}
							}
						}
					}
				}
				catch(std::runtime_error& e)
				{
					__SS__ << "At node '" << memberPair.first << "' error detected descending through children:\n" << e.what() << __E__;
					*accumulatedTreeErrors += ss.str();
				}
			}

			retMap.push_back(std::pair<std::string, ConfigurationTree>(memberPair.first, newNode));
		}
	}

	return retMap;
}  // end getChildren()

//==============================================================================
// getTableByName
//	Get read-only pointer to configuration.
//	If Read/Write access is needed use ConfigurationManagerWithWriteAccess
//		(For general use, Write access should be avoided)
const TableBase* ConfigurationManager::getTableByName(const std::string& tableName) const
{
	std::map<std::string, TableBase*>::const_iterator it;
	if((it = nameToTableMap_.find(tableName)) == nameToTableMap_.end())
	{
		__SS__ << "Can not find table named '" << tableName << "' - you need to load the table before it can be used."
		       << " It probably is missing from the member list of the Table "
		          "Group that was loaded.\n"
		       << "\nYou may need to enter wiz mode to remedy the situation, use the "
		          "following:\n"
		       << "\n\t StartOTS.sh --wiz"
		       << "\n\n\n\n"
		       << __E__;

		ss << __E__ << StringMacros::stackTrace() << __E__;

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
		__COUT_WARN__ << "getTableGroupKey() Failed! No active backbone currently." << __E__;
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
std::pair<std::string, TableGroupKey> ConfigurationManager::getTableGroupFromAlias(std::string systemAlias, ProgressBar* progressBar)
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
			return std::pair<std::string, TableGroupKey>(systemAlias.substr(i, j - i), TableGroupKey(systemAlias.substr(j + 1)));
		else  // failure
			return std::pair<std::string, TableGroupKey>("", TableGroupKey());
	}

	loadConfigurationBackbone();

	if(progressBar)
		progressBar->step();

	try
	{
		//	find runType in Group Aliases table
		ConfigurationTree entry = getNode(ConfigurationManager::GROUP_ALIASES_TABLE_NAME).getNode(systemAlias);

		if(progressBar)
			progressBar->step();

		return std::pair<std::string, TableGroupKey>(entry.getNode("GroupName").getValueAsString(),
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
std::map<std::string /*groupAlias*/, std::pair<std::string /*groupName*/, TableGroupKey>> ConfigurationManager::getActiveGroupAliases(void)
{
	restoreActiveTableGroups();  // make sure the active configuration backbone is
	                             // loaded!
	// loadConfigurationBackbone();

	std::map<std::string /*groupAlias*/, std::pair<std::string /*groupName*/, TableGroupKey>> retMap;

	std::vector<std::pair<std::string, ConfigurationTree>> entries = getNode(ConfigurationManager::GROUP_ALIASES_TABLE_NAME).getChildren();
	for(auto& entryPair : entries)
	{
		retMap[entryPair.first] = std::pair<std::string, TableGroupKey>(entryPair.second.getNode("GroupName").getValueAsString(),
		                                                                TableGroupKey(entryPair.second.getNode("GroupKey").getValueAsString()));
	}
	return retMap;
}

//==============================================================================
// getVersionAliases()
//	get version aliases organized by table, for currently active backbone tables
std::map<std::string /*table name*/, std::map<std::string /*version alias*/, TableVersion /*aliased version*/>> ConfigurationManager::getVersionAliases(
    void) const
{
	//__COUT__ << "getVersionAliases()" << __E__;

	std::map<std::string /*table name*/, std::map<std::string /*version alias*/, TableVersion /*aliased version*/>> retMap;

	std::map<std::string, TableVersion> activeVersions          = getActiveVersions();
	std::string                         versionAliasesTableName = ConfigurationManager::VERSION_ALIASES_TABLE_NAME;
	if(activeVersions.find(versionAliasesTableName) == activeVersions.end())
	{
		__SS__ << "Active version of VersionAliases  missing!"
		       << "Make sure you have a valid active Backbone Group." << __E__;
		__COUT_WARN__ << "\n" << ss.str();
		return retMap;
	}

	//__COUT__ << "activeVersions[\"" << versionAliasesTableName << "\"]=" << activeVersions[versionAliasesTableName] << __E__;

	std::vector<std::pair<std::string, ConfigurationTree>> aliasNodePairs = getNode(versionAliasesTableName).getChildren();

	// create map
	//	add the first of each tableName, versionAlias pair encountered
	//	ignore any repeats (Note: this also prevents overwriting of Scratch alias)
	std::string tableName, versionAlias;
	for(auto& aliasNodePair : aliasNodePairs)
	{
		tableName    = aliasNodePair.second.getNode("TableName").getValueAsString();
		versionAlias = aliasNodePair.second.getNode("VersionAlias").getValueAsString();

		if(retMap.find(tableName) != retMap.end() && retMap[tableName].find(versionAlias) != retMap[tableName].end())
			continue;  // skip repeats (Note: this also prevents overwriting of Scratch
			           // alias)

		// else add version to map
		retMap[tableName][versionAlias] = TableVersion(aliasNodePair.second.getNode("Version").getValueAsString());
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
			retMap.insert(std::pair<std::string, TableVersion>(config.first, config.second->getViewVersion()));
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
std::shared_ptr<TableGroupKey> ConfigurationManager::makeTheTableGroupKey(TableGroupKey key)
{
	if(theConfigurationTableGroupKey_)
	{
		if(*theConfigurationTableGroupKey_ != key)
			destroyTableGroup();
		else
			return theConfigurationTableGroupKey_;
	}
	return std::shared_ptr<TableGroupKey>(new TableGroupKey(key));
}  // end makeTheTableGroupKey()

//==============================================================================
const std::set<std::string>& ConfigurationManager::getContextMemberNames() { return ConfigurationManager::contextMemberNames_; }  // end getContextMemberNames()

//==============================================================================
const std::set<std::string>& ConfigurationManager::getBackboneMemberNames()
{
	return ConfigurationManager::backboneMemberNames_;
}  // end getBackboneMemberNames()

//==============================================================================
const std::set<std::string>& ConfigurationManager::getIterateMemberNames() { return ConfigurationManager::iterateMemberNames_; }  // end getIterateMemberNames()

const std::set<std::string>& ConfigurationManager::getConfigurationMemberNames(void)
{
	configurationMemberNames_.clear();

	std::map<std::string, TableVersion> activeTables = getActiveVersions();

	for(auto& tablePair : activeTables)
		if(ConfigurationManager::contextMemberNames_.find(tablePair.first) == ConfigurationManager::contextMemberNames_.end() &&
		   ConfigurationManager::backboneMemberNames_.find(tablePair.first) == ConfigurationManager::backboneMemberNames_.end() &&
		   ConfigurationManager::iterateMemberNames_.find(tablePair.first) == ConfigurationManager::iterateMemberNames_.end())
			configurationMemberNames_.emplace(tablePair.first);

	return configurationMemberNames_;
}  // end getConfigurationMemberNames()

//==============================================================================
void ConfigurationManager::initializeFromFhicl(const std::string& fhiclPath)
{
	__COUT__ << "Initializing from fhicl: " << fhiclPath << __E__;

	// https://cdcvs.fnal.gov/redmine/projects/fhicl-cpp/wiki

	// LoadParameterSet() ... from  $ARTDAQ_INC/artdaq/Application/LoadParameterSet.hh
	fhicl::ParameterSet pset = LoadParameterSet(fhiclPath);

	//===========================
	// fcl should be FE record(s):
	//	interface0: {
	//		FEInterfacePluginName:	"FEOtsUDPTemplateInterface"
	//		LinkToFETypeTable_FEOtsUDPTemplateInterfaceTable:		{
	//			OtsInterface0:			{
	//				InterfaceIPAddress:		"127.0.0.1"
	//				InterfacePort:			4000
	//				HostIPAddress:			"127.0.0.1"
	//				HostPort:				4020
	//				StreamToIPAddress:		"127.0.0.1"
	//				StreamToPort:			4021
	//			}
	//		} //end FEOtsUDPTemplateInterfaceTable link record
	//	} //end interface0
	//===========================

	// Steps:
	//	Create one context with one FE supervisor
	//		and one/many FEs specified by fcl
	//

	TableBase* table;

	// create context and add context record
	{
		table = 0;
		theInterface_->get(table,                                          // configurationPtr
		                   ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME,  // tableName
		                   0,                                              // groupKey
		                   0,                                              // groupName
		                   true                                            // dontFill=false to fill
		);

		nameToTableMap_[ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME] = table;

		table->setupMockupView(TableVersion(TableVersion::DEFAULT));
		table->setActiveView(TableVersion(TableVersion::DEFAULT));

		TableView* view = table->getViewP();
		__COUT__ << "Activated version: " << view->getVersion() << __E__;
		// view->print();

		// add context record 		---------------------
		view->addRow();
		auto colMap = view->getColumnNamesMap();

		view->setValue("MacroMakerFEContext", 0, colMap["ContextUID"]);
		view->setValue("XDAQApplicationTable", 0, colMap["LinkToApplicationTable"]);
		view->setValue("MacroMakerFEContextApps", 0, colMap["ApplicationGroupID"]);
		view->setValue("1", 0, colMap["Status"]);

		__COUT__ << "Done adding context record..." << __E__;
		view->print();

	}  // done with context record

	// create app table and add application record
	{
		table = 0;
		theInterface_->get(table,                                              // configurationPtr
		                   ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME,  // tableName
		                   0,                                                  // groupKey
		                   0,                                                  // groupName
		                   true                                                // dontFill=false to fill
		);

		nameToTableMap_[ConfigurationManager::XDAQ_APPLICATION_TABLE_NAME] = table;

		table->setupMockupView(TableVersion(TableVersion::DEFAULT));
		table->setActiveView(TableVersion(TableVersion::DEFAULT));

		TableView* view = table->getViewP();
		__COUT__ << "Activated version: " << view->getVersion() << __E__;
		// view->print();

		// add application record 		---------------------
		view->addRow();
		auto colMap = view->getColumnNamesMap();

		view->setValue("MacroMakerFEContextApps", 0, colMap["ApplicationGroupID"]);
		view->setValue("MacroMakerFESupervisor", 0, colMap["ApplicationUID"]);
		view->setValue("FESupervisorTable", 0, colMap["LinkToSupervisorTable"]);
		view->setValue("MacroMakerFESupervisor", 0, colMap["LinkToSupervisorUID"]);
		view->setValue("1", 0, colMap["Status"]);

		__COUT__ << "Done adding application record..." << __E__;
		view->print();
	}  // done with app record

	// create FE Supervisor table and Supervisor record
	{
		table = 0;
		theInterface_->get(table,                // configurationPtr
		                   "FESupervisorTable",  // tableName
		                   0,                    // groupKey
		                   0,                    // groupName
		                   true                  // dontFill=false to fill
		);

		nameToTableMap_["FESupervisorTable"] = table;

		table->setupMockupView(TableVersion(TableVersion::DEFAULT));
		table->setActiveView(TableVersion(TableVersion::DEFAULT));

		TableView* view = table->getViewP();
		__COUT__ << "Activated version: " << view->getVersion() << __E__;
		// view->print();

		// add application record 		---------------------
		view->addRow();
		auto colMap = view->getColumnNamesMap();

		view->setValue("MacroMakerFESupervisor", 0, colMap["SupervisorUID"]);
		view->setValue("FEInterfaceTable", 0, colMap["LinkToFEInterfaceTable"]);
		view->setValue("MacroMakerFESupervisorInterfaces", 0, colMap["LinkToFEInterfaceGroupID"]);

		__COUT__ << "Done adding supervisor record..." << __E__;
		view->print();
	}  // done with app record

	// create FE Interface table and interface record(s)
	recursiveInitFromFhiclPSet(
	    "FEInterfaceTable" /*tableName*/, pset /*fhicl parameter set*/, "" /*uid*/, "MacroMakerFESupervisorInterfaces" /*groupID*/, "FE" /*childLinkIndex*/);

	// init every table after modifications
	for(auto& table : nameToTableMap_)
	{
		table.second->getViewP()->init();
	}

	// verify extraction
	if(0)
	{
		__COUT__ << "================================================" << __E__;
		nameToTableMap_["FESupervisorTable"]->getViewP()->print();
		nameToTableMap_["FEInterfaceTable"]->getViewP()->print();

		auto sups = getNode("FESupervisorTable").getChildrenNames();
		__COUT__ << "Supervisors extracted from fhicl: " << sups.size() << __E__;
		auto fes = getNode("FEInterfaceTable").getChildrenNames();
		__COUT__ << "Front-ends extracted from fhicl: " << fes.size() << __E__;
		{
			auto a = getNode(ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME);
			__COUTV__(a.getValueAsString());

			auto b = a.getNode("MacroMakerFEContext");
			__COUTV__(b.getValueAsString());

			auto c = b.getNode("LinkToApplicationTable");
			__COUTV__(c.getValueAsString());

			auto d = c.getNode("MacroMakerFESupervisor");
			__COUTV__(d.getValueAsString());

			auto e = d.getNode("LinkToSupervisorTable");
			__COUTV__(e.getValueAsString());

			auto f = e.getNode("LinkToFEInterfaceTable");
			__COUTV__(f.getValueAsString());

			auto z = f.getChildrenNames();
			__COUTV__(StringMacros::vectorToString(z));
			__COUTV__(z.size());
			auto y = f.getChildrenNames(false /*byPriority*/, true /*onlyStatusTrue*/);
			__COUTV__(StringMacros::vectorToString(y));
			__COUTV__(y.size());
			auto x = f.getChildrenNames(true /*byPriority*/, true /*onlyStatusTrue*/);
			__COUTV__(StringMacros::vectorToString(x));
			__COUTV__(x.size());

			auto g = f.getNode("dtc0");
			__COUTV__(g.getValueAsString());
			auto h = f.getNode("interface0");
			__COUTV__(h.getValueAsString());

			auto fes = getNode(ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME)
			               .getNode(
			                   "MacroMakerFEContext/LinkToApplicationTable/"
			                   "MacroMakerFESupervisor/LinkToSupervisorTable")
			               .getNode("LinkToFEInterfaceTable")
			               .getChildrenNames(true /*byPriority*/, true /*onlyStatusTrue*/);
			__COUTV__(fes.size());
			__COUTV__(StringMacros::vectorToString(fes));
		}
	}

}  // end initializeFromFhicl()

//==============================================================================
// recursiveInitFromFhiclPSet
//		Add records and all children parameters starting at table
//			recursively. If groupName given then loop through
//			records and add to table.
void ConfigurationManager::recursiveInitFromFhiclPSet(const std::string&         tableName,
                                                      const fhicl::ParameterSet& pset,
                                                      const std::string&         recordName,
                                                      const std::string&         groupName,
                                                      const std::string&         groupLinkIndex)
{
	__COUT__ << __COUT_HDR_P__ << "Adding table '" << tableName << "' record(s)..." << __E__;

	TableBase* table;
	// create context and add context record
	{
		table = 0;
		if(nameToTableMap_.find(tableName) == nameToTableMap_.end())
		{
			__COUT__ << "Table not found, so making '" << tableName << "'instance..." << __E__;
			theInterface_->get(table,      // configurationPtr
			                   tableName,  // tableName
			                   0,          // groupKey
			                   0,          // groupName
			                   true        // dontFill=false to fill
			);

			nameToTableMap_[tableName] = table;
			table->setupMockupView(TableVersion(TableVersion::DEFAULT));
		}
		else
		{
			__COUT__ << "Existing table found, so using '" << tableName << "'instance..." << __E__;
			table = nameToTableMap_[tableName];
		}

		table->setActiveView(TableVersion(TableVersion::DEFAULT));

		TableView* view = table->getViewP();
		__COUT__ << "Activated version: " << view->getVersion() << __E__;
		// view->print();

		if(recordName != "")  // then add this record
		{
			// Steps:
			//	- add row
			//	- set UID and enable (if possible)
			//	- set values for parameter columns
			//	- define links

			__COUTV__(recordName);

			// add row and get column map
			unsigned int r      = view->addRow();
			auto         colMap = view->getColumnNamesMap();

			// set UID and enable (if possible)
			view->setValue(recordName, r, view->getColUID());
			try
			{
				view->setValue("1", r, view->getColStatus());
			}
			catch(...)
			{
				__COUT__ << "No status column to set for '" << recordName << "'" << __E__;
			}

			if(groupName != "")  // then set groupID for this record
			{
				int groupIDCol = view->getLinkGroupIDColumn(groupLinkIndex);
				__COUT__ << "Setting group ID for group link ID '" << groupLinkIndex << "' at column " << groupIDCol << " to '" << groupName << ".'" << __E__;

				view->setValue(groupName, r, groupIDCol);
			}

			// then set parameters
			auto names = pset.get_names();
			for(const auto& colName : names)
			{
				if(!pset.is_key_to_atom(colName))
					continue;

				auto colIt = colMap.find(colName);
				if(colIt == colMap.end())
				{
					__SS__ << "Field '" << colName << "' of record '" << recordName << "' in table '" << tableName << "' was not found in columns."
					       << "\n\nHere are the existing column names:\n";
					unsigned int i = 0;
					for(const auto& col : colMap)
						ss << "\n" << ++i << ".\t" << col.first;
					ss << __E__;
					__SS_THROW__;
				}
				const std::string value = pset.get<std::string>(colName);
				__COUT__ << "Setting '" << recordName << "' parameter at column " << colIt->second << ", '" << colName << "'\t = " << value << __E__;
				view->setValueAsString(value, r, colIt->second);
			}  // end set parameters

			// then define links
			for(const auto& linkName : names)
			{
				if(pset.is_key_to_atom(linkName))
					continue;

				__COUTV__(linkName);

				// split into column name and table
				unsigned int c = linkName.size() - 1;
				for(; c >= 1; --c)
					if(linkName[c] == '_')  // find first underscore to split linkName
						break;

				if(c == 0)
				{
					__SS__ << "Illegal link name '" << linkName
					       << "' found. The format must be <Column name>_<Target table "
					          "name>,.. for example '"
					       << "LinkToFETypeTable_FEOtsUDPTemplateInterfaceTable'" << __E__;
					__SS_THROW__;
				}
				std::string colName = linkName.substr(0, c);
				__COUTV__(colName);

				auto colIt = colMap.find(colName);
				if(colIt == colMap.end())
				{
					__SS__ << "Link '" << colName << "' of record '" << recordName << "' in table '" << tableName << "' was not found in columns."
					       << "\n\nHere are the existing column names:\n";
					unsigned int i = 0;
					for(const auto& col : colMap)
						ss << "\n" << i << ".\t" << col.first << __E__;
					__SS_THROW__;
				}
				//__COUT__ << "Setting link at column " << colIt->second << __E__;

				std::pair<unsigned int /*link col*/, unsigned int /*link id col*/> linkPair;
				bool                                                               isGroupLink;
				view->getChildLink(colIt->second, isGroupLink, linkPair);

				//__COUTV__(isGroupLink);
				//__COUTV__(linkPair.first);
				//__COUTV__(linkPair.second);

				std::string linkTableName = linkName.substr(c + 1);
				__COUTV__(linkTableName);

				auto linkPset    = pset.get<fhicl::ParameterSet>(linkName);
				auto linkRecords = linkPset.get_pset_names();
				if(!isGroupLink && linkRecords.size() > 1)
				{
					__SS__ << "A Unique Link can only point to one record. "
					       << "The specified link '" << colName << "' of record '" << recordName << "' in table '" << tableName << "' has "
					       << linkRecords.size() << " children records specified. " << __E__;
					__SS_THROW__;
				}

				if(linkRecords.size() == 0)
				{
					__COUT__ << "No child records, so leaving link disconnected." << __E__;
					continue;
				}

				__COUT__ << "Setting Link at columns [" << linkPair.first << "," << linkPair.second << "]" << __E__;
				view->setValue(linkTableName, r, linkPair.first);

				if(!isGroupLink)
				{
					__COUT__ << "Setting up Unique link to " << linkRecords[0] << __E__;

					view->setValue(linkRecords[0], r, linkPair.second);

					recursiveInitFromFhiclPSet(linkTableName /*tableName*/,
					                           linkPset.get<fhicl::ParameterSet>(linkRecords[0]) /*fhicl parameter set*/,
					                           linkRecords[0] /*uid*/,
					                           "" /*groupID*/);

					view->print();
				}
				else
				{
					std::string childLinkIndex = view->getColumnInfo(linkPair.first).getChildLinkIndex();
					std::string groupName      = recordName + "Group";

					view->setValue(groupName, r, linkPair.second);

					for(const auto& groupRecord : linkRecords)
					{
						__COUT__ << "Setting '" << childLinkIndex << "' Group link to '" << groupName << "' record '" << groupRecord << "'" << __E__;

						recursiveInitFromFhiclPSet(linkTableName /*tableName*/,
						                           linkPset.get<fhicl::ParameterSet>(groupRecord) /*fhicl parameter set*/,
						                           groupRecord /*uid*/,
						                           groupName /*groupID*/,
						                           childLinkIndex /*groupLinkIndex*/);
					}
				}

			}  // end link handling
		}
		else if(groupName != "")  // then add group of records
		{
			// get_pset_names();
			// get_names
			__COUTV__(groupName);
			auto psets = pset.get_pset_names();
			for(const auto& ps : psets)
			{
				__COUTV__(ps);
				recursiveInitFromFhiclPSet(tableName /*tableName*/,
				                           pset.get<fhicl::ParameterSet>(ps) /*fhicl parameter set*/,
				                           ps /*uid*/,
				                           groupName /*groupID*/,
				                           groupLinkIndex /*groupLinkIndex*/);
			}

			view->print();
		}
		else
		{
			__SS__ << "Illegal recursive parameters!" << __E__;
			__SS_THROW__;
		}
	}

	__COUT__ << __COUT_HDR_P__ << "Done adding table '" << tableName << "' record(s)..." << __E__;

}  // end recursiveInitFromFhiclPSet()

//==============================================================================
bool ConfigurationManager::isOwnerFirstAppInContext()
{
	//__COUT__ << "Checking if owner is first App in Context." << __E__;
	if(ownerContextUID_ == "" || ownerAppUID_ == "")
		return true;  // default to 'yes'

	//__COUTV__(ownerContextUID_);
	//__COUTV__(ownerAppUID_);

	auto contextChildren = getNode(ConfigurationManager::XDAQ_CONTEXT_TABLE_NAME + "/" + ownerContextUID_).getChildrenNames();

	bool isFirstAppInContext = contextChildren.size() == 0 || contextChildren[0] == ownerAppUID_;

	//__COUTV__(isFirstAppInContext);

	return isFirstAppInContext;
}  // end isOwnerFirstAppInContext()

//==============================================================================
// allow for just the desktop icons of the Context to be changed during run-time
TableBase* ConfigurationManager::getDesktopIconTable(void)
{
	if(nameToTableMap_.find(DESKTOP_ICON_TABLE_NAME) == nameToTableMap_.end())
	{
		__SS__ << "Desktop icon table not found!" << __E__;
		ss << StringMacros::stackTrace() << __E__;
		__SS_THROW__;
	}

	return nameToTableMap_.at(DESKTOP_ICON_TABLE_NAME);
}  // end dynamicDesktopIconChange()

//==============================================================================
void ConfigurationManager::saveGroupNameAndKey(const std::pair<std::string /*group name*/, TableGroupKey>& theGroup, const std::string& fileName)
{
	std::string fullPath = ConfigurationManager::LAST_TABLE_GROUP_SAVE_PATH +
			"/" + fileName;

	std::ofstream groupFile(fullPath.c_str());
	if(!groupFile.is_open())
	{
		__SS__ << "Error. Can't open file to save group activity: " << fullPath << __E__;
		__SS_THROW__;
	}
	std::stringstream outss;
	outss << theGroup.first << "\n" << theGroup.second << "\n" << time(0);
	groupFile << outss.str().c_str();
	groupFile.close();
}  // end saveGroupNameAndKey()

//==============================================================================
// loadGroupNameAndKey
//	loads group name and key (and time) from specified file
//	returns time string in returnedTimeString
//
//	Note: this is static so the GatewaySupervisor and WizardSupervisor can call it
std::pair<std::string /*group name*/, TableGroupKey> ConfigurationManager::loadGroupNameAndKey(const std::string& fileName, std::string& returnedTimeString)
{
	std::string fullPath = ConfigurationManager::LAST_TABLE_GROUP_SAVE_PATH +
			"/" + fileName;

	FILE* groupFile = fopen(fullPath.c_str(), "r");
	if(!groupFile)
	{
		__COUT__ << "Can't open file: " << fullPath << __E__;

		__COUT__ << "Returning empty groupName and key -1" << __E__;

		return std::pair<std::string /*group name*/, TableGroupKey>("", TableGroupKey());
	}

	char line[500];  // assuming no group names longer than 500 chars
	// name and then key
	std::pair<std::string /*group name*/, TableGroupKey> theGroup;

	fgets(line, 500, groupFile);  // name
	if(strlen(line) && line[strlen(line)-1] == '\n')
		line[strlen(line)-1] = '\0'; //remove trailing newline
	theGroup.first = line;

	fgets(line, 500, groupFile);  // key
	int key;
	sscanf(line, "%d", &key);
	theGroup.second = key;

	fgets(line, 500, groupFile);  // time
	time_t timestamp;
	sscanf(line, "%ld", &timestamp);                                   // type long int
	                                                                   //	struct tm tmstruct;
	                                                                   //	::localtime_r(&timestamp, &tmstruct);
	                                                                   //	::strftime(line, 30, "%c %Z", &tmstruct);
	returnedTimeString = StringMacros::getTimestampString(timestamp);  // line;
	fclose(groupFile);

	__COUT__ << "theGroup.first= " << theGroup.first << " theGroup.second= " << theGroup.second << __E__;

	return theGroup;
}  // end loadGroupNameAndKey()




