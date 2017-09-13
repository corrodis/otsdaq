#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationInterface.h"//All configurable objects are included here
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
#include "otsdaq-core/ProgressBar/ProgressBar.h"

#include <fstream>      // std::ofstream


using namespace ots;


#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "ConfigurationManager"

const std::string ConfigurationManager::READONLY_USER = "READONLY_USER";

const std::string ConfigurationManager::XDAQ_CONTEXT_CONFIG_NAME = "XDAQContextConfiguration";

//added env check for otsdaq_flatten_active_to_version to function
const std::string ConfigurationManager::ACTIVE_GROUP_FILENAME = ((getenv("SERVICE_DATA_PATH") == NULL)?(std::string(getenv("USER_DATA"))+"/ServiceData"):(std::string(getenv("SERVICE_DATA_PATH")))) + "/ActiveConfigurationGroups.cfg";
const std::string ConfigurationManager::ALIAS_VERSION_PREAMBLE = "ALIAS:";
const std::string ConfigurationManager::SCRATCH_VERSION_ALIAS = "Scratch";

#define CORE_TABLE_INFO_FILENAME ((getenv("SERVICE_DATA_PATH") == NULL)?(std::string(getenv("USER_DATA"))+"/ServiceData"):(std::string(getenv("SERVICE_DATA_PATH")))) + "/CoreTableInfoNames.dat"


const std::string ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT       = "Context";
const std::string ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE      = "Backbone";
const std::string ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION = "Configuration";


//==============================================================================
ConfigurationManager::ConfigurationManager()
: username_ 				(ConfigurationManager::READONLY_USER)
, theInterface_       		(0)
, theConfigurationGroupKey_	(0)
, theContextGroupKey_ 		(0)
, theBackboneGroupKey_		(0)
, theConfigurationGroup_	("")
, theContextGroup_			("")
, theBackboneGroup_			("")
, contextMemberNames_		({XDAQ_CONTEXT_CONFIG_NAME,"XDAQApplicationConfiguration","DesktopIconConfiguration","MessageFacilityConfiguration","TheSupervisorConfiguration","StateMachineConfiguration","DesktopWindowParameterConfiguration"})
, backboneMemberNames_		({"GroupAliasesConfiguration","VersionAliasesConfiguration"})
{
	theInterface_ = ConfigurationInterface::getInstance(false);  //false to use artdaq DB
	//NOTE: in ConfigurationManagerRW using false currently.. think about consistency! FIXME

	//initialize special group metadata table
	{
		//Note: "ConfigurationGroupMetadata" should never be in conflict
		//	because all other tables end in "...Configuration"

		//This is a table called ConfigurationGroupMetadata
		//	with 3 fields:
		//		- GroupAuthor
		//		- GroupCreationTime
		//		- CommentDescription

		groupMetadataTable_.setConfigurationName(ConfigurationInterface::GROUP_METADATA_TABLE_NAME);
		//ConfigurationView* mockup = configurationGroupMetadataTable_.getMockupViewP();
		std::vector<ViewColumnInfo>* colInfo =
				groupMetadataTable_.getMockupViewP()->getColumnsInfoP();


		colInfo->push_back(ViewColumnInfo(
				ViewColumnInfo::TYPE_UID, //just to make init() happy
				"UnusedUID",
				"UNUSED_UID",
				ViewColumnInfo::DATATYPE_NUMBER,
				"",0
		));
		colInfo->push_back(ViewColumnInfo(
				ViewColumnInfo::TYPE_COMMENT, //just to make init() happy
				"CommentDescription",
				"COMMENT_DESCRIPTION",
				ViewColumnInfo::DATATYPE_STRING,
				"",0
		));
		colInfo->push_back(ViewColumnInfo(
				ViewColumnInfo::TYPE_AUTHOR, //just to make init() happy
				"GroupAuthor",
				"AUTHOR",
				ViewColumnInfo::DATATYPE_STRING,
				"",0
		));
		colInfo->push_back(ViewColumnInfo(
				ViewColumnInfo::TYPE_TIMESTAMP,
				"GroupCreationTime",
				"GROUP_CREATION_TIME",
				ViewColumnInfo::DATATYPE_TIME,
				"",0
		));
		auto tmpVersion = groupMetadataTable_.createTemporaryView();
		groupMetadataTable_.setActiveView(tmpVersion);
		//only need this one and only row for all time
		groupMetadataTable_.getViewP()->addRow();
	}

	//dump names of core tables (so UpdateOTS.sh can copy core tables for user)
	{
		FILE * fp = fopen((CORE_TABLE_INFO_FILENAME).c_str(),"w");
		if(fp)
		{
			for(const auto &name:contextMemberNames_)
				fprintf(fp,"%s\n",name.c_str());
			for(const auto &name:backboneMemberNames_)
				fprintf(fp,"%s\n",name.c_str());
			fclose(fp);
		}
		else
			__MOUT__ << "Failed to open core table info file: " << CORE_TABLE_INFO_FILENAME << std::endl;
	}

	init();
}

//==============================================================================
ConfigurationManager::ConfigurationManager(const std::string& username)
: ConfigurationManager	()
{
	username_ = username;
}

//==============================================================================
ConfigurationManager::~ConfigurationManager()
{
	destroy();
}

//==============================================================================
//init
//	if accumulatedErrors is not null.. fill it with errors
//	else throw errors (but do not ask restoreActiveConfigurationGroups to throw errors)
void ConfigurationManager::init(std::string *accumulatedErrors)
{
	if(accumulatedErrors) *accumulatedErrors = "";

	//destroy();

	// once Interface is false (using artdaq db) .. then can call
	if(theInterface_->getMode() == false)
	{
		try
		{
			restoreActiveConfigurationGroups(accumulatedErrors?true:false);
		}
		catch(std::runtime_error &e)
		{
			if(accumulatedErrors) *accumulatedErrors = e.what();
			else throw;
		}
	}
}

//==============================================================================
//restoreActiveConfigurationGroups
//	load the active groups from file
//	Note: this should be used by the Supervisor to maintain
//		the same configurationGroups surviving software system restarts
void ConfigurationManager::restoreActiveConfigurationGroups(bool throwErrors)
{
	destroyConfigurationGroup("",true); //deactivate all

	std::string fn = ACTIVE_GROUP_FILENAME;
	FILE *fp = fopen(fn.c_str(),"r");

	__MOUT__ << "ACTIVE_GROUP_FILENAME = " << ACTIVE_GROUP_FILENAME << std::endl;
	__MOUT__ << "ARTDAQ_DATABASE_URI = " << std::string(getenv("ARTDAQ_DATABASE_URI")) << std::endl;

	if(!fp) return;

	__MOUT__ << "throwErrors: " << throwErrors << std::endl;

	char tmp[500];
	char strVal[500];

	std::string groupName;
	std::string errorStr = "";
	bool skip;

	__SS__;

	for(int i=0;i<3;++i)
	{
		skip = false;
		fgets(tmp,500,fp);
		sscanf(tmp,"%s",strVal); //sscanf to remove '\n'
		for(unsigned int j=0;j<strlen(strVal);++j)
			if(!(
					(strVal[j] >= 'a' && strVal[j] <= 'z') ||
					(strVal[j] >= 'A' && strVal[j] <= 'Z') ||
					(strVal[j] >= '0' && strVal[j] <= '9')))
				{
					strVal[j] = '\0';
					__MOUT_INFO__ << "Illegal character found, so skipping!" << std::endl;

					skip = true;
					break;
				}

		if(skip) continue;

		groupName = strVal;
		fgets(tmp,500,fp);
		sscanf(tmp,"%s",strVal);  //sscanf to remove '\n'

		for(unsigned int j=0;j<strlen(strVal);++j)
			if(!(
					(strVal[j] >= '0' && strVal[j] <= '9')))
				{
					strVal[j] = '\0';
					__MOUT_INFO__ << "Illegal character found, so skipping!" << std::endl;

					skip = true;
					break;
				}

		if(skip) continue;

		try
		{
			//load and doActivate
			loadConfigurationGroup(groupName,ConfigurationGroupKey(strVal),true);
		}
		catch(std::runtime_error &e)
		{
			ss << "Failed to load config group in ConfigurationManager::init() with name '" <<
					groupName << "_v" << strVal << "'" << std::endl;
			ss << e.what() << std::endl;

			errorStr += ss.str();
		}
		catch(...)
		{
			ss << "Failed to load config group in ConfigurationManager::init() with name '" <<
					groupName << "_v" << strVal << "'" << std::endl;

			errorStr += ss.str();
		}
	}

	fclose(fp);

	if(throwErrors && errorStr != "")
	{
		__MOUT_INFO__ << "\n" << ss.str();
		throw std::runtime_error(errorStr);
	}
}

//==============================================================================
//destroyConfigurationGroup
//	destroy all if theGroup == ""
//	else destroy that group
// 	if onlyDeactivate, then don't delete, just deactivate view
void ConfigurationManager::destroyConfigurationGroup(const std::string& theGroup, bool onlyDeactivate)
{
	//delete
	bool isContext       = theGroup == "" || theGroup == theContextGroup_;
	bool isBackbone      = theGroup == "" || theGroup == theBackboneGroup_;
	bool isConfiguration = theGroup == "" || theGroup == theConfigurationGroup_;

	if(!isContext && !isBackbone && !isConfiguration)
	{
		__MOUT__ << "Invalid configuration group to destroy: " << theGroup << std::endl;
		throw std::runtime_error("Invalid configuration group to destroy!");
	}

	std::string dbgHeader = onlyDeactivate?"Deactivating":"Destroying";
	if(isContext)
		__MOUT__ << dbgHeader << " Context group: " << theGroup << std::endl;
	if(isBackbone)
		__MOUT__ << dbgHeader << " Backbone group: " << theGroup << std::endl;
	if(isConfiguration)
		__MOUT__ << dbgHeader << " Configuration group: " << theGroup << std::endl;


	std::set<std::string>::const_iterator contextFindIt, backboneFindIt;
	for(auto it = nameToConfigurationMap_.begin(); it != nameToConfigurationMap_.end(); /*no increment*/)
	{
		contextFindIt = contextMemberNames_.find(it->first);
		backboneFindIt = backboneMemberNames_.find(it->first);
		if(theGroup == "" || ( 	(isContext  && contextFindIt != contextMemberNames_.end()) ||
				(isBackbone && backboneFindIt != backboneMemberNames_.end()) ||
				(!isContext && !isBackbone &&
						contextFindIt == contextMemberNames_.end() &&
						backboneFindIt == backboneMemberNames_.end())))
		{
			//__MOUT__ << "\t" << it->first << std::endl;
			//if(it->second->isActive())
			//	__MOUT__ << "\t\t..._v" << it->second->getViewVersion() << std::endl;


			if(onlyDeactivate) //only deactivate
			{
				it->second->deactivate();
				++it;
			}
			else	//else, delete/erase
			{
				delete it->second;
				nameToConfigurationMap_.erase(it++);
			}
		}
		else
			++it;
	}

	if(isConfiguration)
	{
		theConfigurationGroup_ = "";
		if(theConfigurationGroupKey_ != 0)
		{
			__MOUT__ << "Destroying Configuration Key: " << *theConfigurationGroupKey_ << std::endl;
			theConfigurationGroupKey_.reset();
		}

		//		theDACStreams_.clear();
	}
	if(isBackbone)
	{
		theBackboneGroup_ = "";
		if(theBackboneGroupKey_ != 0)
		{
			__MOUT__ << "Destroying Backbone Key: " << *theBackboneGroupKey_ << std::endl;
			theBackboneGroupKey_.reset();
		}
	}
	if(isContext)
	{
		theContextGroup_ = "";
		if(theContextGroupKey_ != 0)
		{
			__MOUT__ << "Destroying Context Key: " << *theContextGroupKey_ << std::endl;
			theContextGroupKey_.reset();
		}
	}
}

//==============================================================================
void ConfigurationManager::destroy(void)
{
	//NOTE: Moved to ConfigurationGUISupervisor [FIXME is this correct?? should we use shared_ptr??]
	//    if( ConfigurationInterface::getInstance(true) != 0 )
	//        delete theInterface_;
	destroyConfigurationGroup();
}


//==============================================================================
//convertGroupTypeIdToName
//	return translation:
//		0 for context
//		1 for backbone
//		2 for configuration (others)
const std::string& ConfigurationManager::convertGroupTypeIdToName(int groupTypeId)
{
	return groupTypeId==CONTEXT_TYPE?
			ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT:
			(groupTypeId==BACKBONE_TYPE?
					ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE:
					ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION);
}

//==============================================================================
//getTypeOfGroup
//	return
//		0 for context
//		1 for backbone
//		2 for configuration (others)
int ConfigurationManager::getTypeOfGroup(
		const std::map<std::string /*name*/, ConfigurationVersion /*version*/> &memberMap)
{

	bool isContext = true;
	bool isBackbone = true;
	bool inGroup;
	bool inContext = false;
	bool inBackbone = false;
	unsigned int matchCount = 0;

	for(auto &memberPair:memberMap)
	{
		//__MOUT__ << "Member name: = "<< memberPair.first << std::endl;
		inGroup = false; //check context
		for(auto &contextMemberString:contextMemberNames_)
			if(memberPair.first == contextMemberString)
			{
				inGroup = true;
				inContext = true;
				++matchCount;
				break;
			}
		if(!inGroup)
		{
			isContext = false;
			if(inContext) //there was a member in context!
			{
				__SS__ << "This group is an incomplete match to a Context group.\n";
				ss << "\nTo be a Context group, the members must exactly match" <<
						"the following members:\n";
				int i = 0;
				for(auto &memberName:contextMemberNames_)
					ss << ++i << ". " << memberName << "\n";
				__MOUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}
		}

		inGroup = false; //check backbone
		for(auto &backboneMemberString:backboneMemberNames_)
			if(memberPair.first == backboneMemberString)
			{
				inGroup = true;
				inBackbone = true;
				++matchCount;
				break;
			}
		if(!inGroup)
		{
			isBackbone = false;
			if(inBackbone) //there was a member in backbone!
			{
				__SS__ << "This group is an incomplete match to a Backbone group.\n";
				ss << "\nTo be a Backbone group, the members must exactly match" <<
						"the following members:\n";
				int i = 0;
				for(auto &memberName:backboneMemberNames_)
					ss << ++i << ". " << memberName << "\n";
				__MOUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}
		}

	}

	if(isContext && matchCount != contextMemberNames_.size())
	{
		__SS__ << "This group is an incomplete match to a Context group: " <<
				" Size=" << matchCount << " but should be " << contextMemberNames_.size() <<
				std::endl;
		ss << "\nThe members currently are...\n";
		int i = 0;
		for(auto &memberPair:memberMap)
			ss << ++i << ". " << memberPair.first << "\n";
		ss << "\nThe expected Context members are...\n";
		i = 0;
		for(auto &memberName:contextMemberNames_)
			ss << ++i << ". " << memberName << "\n";
		__MOUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	if(isBackbone && matchCount != backboneMemberNames_.size())
	{
		__SS__ << "This group is an incomplete match to a Backbone group: " <<
				" Size=" << matchCount << " but should be " << backboneMemberNames_.size() <<
				std::endl;
		ss << "\nThe members currently are...\n";
		int i = 0;
		for(auto &memberPair:memberMap)
			ss << ++i << ". " << memberPair.first << "\n";
		ss << "\nThe expected Backbone members are...\n";
		i = 0;
		for(auto &memberName:backboneMemberNames_)
			ss << ++i << ". " << memberName << "\n";
		__MOUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	return isContext?CONTEXT_TYPE:(isBackbone?BACKBONE_TYPE:CONFIGURATION_TYPE);
}

//==============================================================================
//getTypeNameOfGroup
//	return string for
//		0 for context
//		1 for backbone
//		2 for configuration (others)
const std::string& ConfigurationManager::getTypeNameOfGroup(
		const std::map<std::string /*name*/, ConfigurationVersion /*version*/> &memberMap)
{
	return convertGroupTypeIdToName(getTypeOfGroup(memberMap));
}
//==============================================================================
//loadMemberMap
//	loads tables given by name/version pairs in memberMap
//	Note: does not activate them.
void ConfigurationManager::dumpActiveConfiguration(
		const std::string &filePath, const std::string &dumpType) const
{
	time_t rawtime =  time(0);
	__MOUT__ << "filePath = " << filePath << std::endl;
	__MOUT__ << "dumpType = " << dumpType << std::endl;


	std::ofstream fs;
	fs.open(filePath, std::fstream::out | std::fstream::trunc);

	std::ostream *out;

	//if file was valid use it, else default to cout
	if(fs.is_open())
		out = &fs;
	else
		out = &(std::cout);


	(*out) << "#################################" << std::endl;
	(*out) << "This is an ots configuration dump.\n\n" << std::endl;
	(*out) << "Source database is $ARTDAQ_DATABASE_URI = \t" << getenv("ARTDAQ_DATABASE_URI") << std::endl;
	(*out) << "\nOriginal location of dump: \t" << filePath << std::endl;
	(*out) << "Type of dump: \t" << dumpType << std::endl;
	(*out) << "Linux time for dump: \t" << rawtime << std::endl;

	{
		struct tm * timeinfo = localtime (&rawtime);
		char buffer [100];
		strftime(buffer,100,"%c %Z",timeinfo);
		(*out) << "Display time for dump: \t" << buffer << std::endl;
	}


	//define local "lambda" functions
	//	active groups
	//	active tables
	//	active group members
	//	active table contents

	auto localDumpActiveGroups = [](const ConfigurationManager *cfgMgr, std::ostream *out) {
		std::map<std::string, std::pair<std::string, ConfigurationGroupKey>>  activeGroups =
				cfgMgr->getActiveConfigurationGroups();

		(*out) << "\n\n************************" << std::endl;
		(*out) << "Active Groups:" << std::endl;
		for(auto &group:activeGroups)
		{
			(*out) << "\t" << group.first << " := " <<
					group.second.first << " (" <<
					group.second.second << ")" << std::endl;
		}
	};

	auto localDumpActiveTables = [](const ConfigurationManager *cfgMgr, std::ostream *out) {
		std::map<std::string, ConfigurationVersion> activeTables =
						cfgMgr->getActiveVersions();

		(*out) << "\n\n************************" << std::endl;
		(*out) << "Active Tables:" << std::endl;
		(*out) << "Active Tables count = " << activeTables.size() << std::endl;
		for(auto &table:activeTables)
		{
			(*out) << "\t" << table.first << "-v" <<
					table.second << std::endl;
		}
	};

	auto localDumpActiveGroupMembers = [](const ConfigurationManager *cfgMgr, std::ostream *out) {
		std::map<std::string, std::pair<std::string, ConfigurationGroupKey>>  activeGroups =
				cfgMgr->getActiveConfigurationGroups();
		(*out) << "\n\n************************" << std::endl;
		(*out) << "Active Group Members:" << std::endl;
		int tableCount = 0;
		for(auto &group:activeGroups)
		{
			(*out) << "\t" << group.first << " := " <<
					group.second.first << " (" <<
					group.second.second << ")" << std::endl;

			std::map<std::string /*name*/, ConfigurationVersion /*version*/> memberMap =
					cfgMgr->theInterface_->getConfigurationGroupMembers(
							ConfigurationGroupKey::getFullGroupString(
									group.second.first,
									group.second.second));

			(*out) << "\tMember count = " << memberMap.size() << std::endl;
			tableCount += memberMap.size();

			for(auto &member:memberMap)
			{
				(*out) << "\t\t" << member.first << "-v" <<
						member.second << std::endl;
			}
		}
		(*out) << "Active Group Members total count = " << tableCount << std::endl;
	};

	auto localDumpActiveTableContents = [](const ConfigurationManager *cfgMgr, std::ostream *out) {
		std::map<std::string, ConfigurationVersion> activeTables =
						cfgMgr->getActiveVersions();

		(*out) << "\n\n************************" << std::endl;
		(*out) << "Active Table Contents:" << std::endl;
		for(auto &table:activeTables)
		{
			(*out) << "\t" << table.first << "-v" <<
					table.second << std::endl;

			cfgMgr->nameToConfigurationMap_.find(table.first)->second->print(*out);
		}
	};



	if(dumpType == "GroupKeys")
	{
		localDumpActiveGroups(this,out);
	}
	else if(dumpType == "TableVersions")
	{
		localDumpActiveTables(this,out);
	}
	else if(dumpType == "GroupKeysAndTableVersions")
	{
		localDumpActiveGroups(this,out);
		localDumpActiveTables(this,out);
	}
	else if(dumpType == "All")
	{
		localDumpActiveGroups(this,out);
		localDumpActiveGroupMembers(this,out);
		localDumpActiveTables(this,out);
		localDumpActiveTableContents(this,out);
	}
	else
	{
		__SS__ << "Invalid dump type '" << dumpType <<
				"' given during dumpActiveConfiguration(). Valid types are as follows:\n" <<

				//List all choices
				"GroupKeys" << ", " <<
				"TableVersions" << ", " <<
				"GroupsKeysAndTableVersions" << ", " <<
				"All" <<

				"\n\nPlease change the State Machine configuration to a valid dump type." <<
				std::endl;
		throw std::runtime_error(ss.str());
	}

	if(fs.is_open())
		fs.close();
}

//==============================================================================
//loadMemberMap
//	loads tables given by name/version pairs in memberMap
//	Note: does not activate them.
void ConfigurationManager::loadMemberMap(
		const std::map<std::string /*name*/, ConfigurationVersion /*version*/> &memberMap)
{
	ConfigurationBase *tmpConfigBasePtr;
	//	for each member
	//		get()
	for(auto &memberPair:memberMap)
	{
		//__MOUT__ << "\tMember config " << memberPair.first << ":" <<
		//		memberPair.second << std::endl;

		//get the proper temporary pointer
		//	use 0 if doesn't exist yet.
		//	Note: do not want to give nameToConfigurationMap_[memberPair.first]
		//		in case there is failure in get... (exceptions may be thrown)
		//Note: Default constructor is called by Map, i.e. nameToConfigurationMap_[memberPair.first] = 0; //create pointer and set to 0
		tmpConfigBasePtr = 0;
		if(nameToConfigurationMap_.find(memberPair.first) != nameToConfigurationMap_.end())
			tmpConfigBasePtr = nameToConfigurationMap_[memberPair.first];

		theInterface_->get(tmpConfigBasePtr, //configurationPtr
				memberPair.first,	//configurationName
				0,		//ConfigurationGroupKey
				0,		//configurations
				false,	//dontFill=false to fill
				memberPair.second, 	//version
				false	//resetConfiguration
		);

		nameToConfigurationMap_[memberPair.first] = tmpConfigBasePtr;
		if(nameToConfigurationMap_[memberPair.first]->getViewP())
		{
			//__MOUT__ << "\t\tActivated version: " << nameToConfigurationMap_[memberPair.first]->getViewVersion() << std::endl;
		}
		else
		{
			__SS__ << nameToConfigurationMap_[memberPair.first]->getConfigurationName() <<
					": View version not activated properly!";
			throw std::runtime_error(ss.str());
		}
	}
}

//==============================================================================
//loadConfigurationGroup
//	load all members of configuration group
//	if doActivate
//		DOES NOT theConfigurationGroup_, theContextGroup_, or theBackboneGroup_ on success
//			this happens with ConfigurationManagerRW::activateConfigurationGroup
//		for each member
//			configBase->init()
//
//	if progressBar != 0, then do step handling, for finer granularity
//
// 	if(doNotLoadMember) return memberMap; //this is useful if just getting group metadata
//
//	throws exception on failure.
//   map<name       , ConfigurationVersion >
std::map<std::string, ConfigurationVersion> ConfigurationManager::loadConfigurationGroup(
		const std::string     	&configGroupName,
		ConfigurationGroupKey 	configGroupKey,
		bool                  	doActivate,
		ProgressBar			  	*progressBar,
		std::string 			*accumulatedTreeErrors,
		std::string 			*groupComment,
		std::string 			*groupAuthor,
		std::string	 			*groupCreateTime,
		bool					doNotLoadMember,
		std::string				*groupTypeString)
{
	if(accumulatedTreeErrors) 	*accumulatedTreeErrors	= "";
	if(groupComment) 			*groupComment 			= "";
	if(groupAuthor) 			*groupAuthor 			= "";
	if(groupCreateTime) 		*groupCreateTime 		= "";
	if(groupTypeString)			*groupTypeString 				= "";

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
	//		set theConfigurationGroup_, theContextGroup_, or theBackboneGroup_ on success

	__MOUT_INFO__ << "Loading Configuration Group: " << configGroupName <<
			"(" << configGroupKey << ")" << std::endl;

	std::map<std::string /*name*/, ConfigurationVersion /*version*/> memberMap =
			theInterface_->getConfigurationGroupMembers(
					ConfigurationGroupKey::getFullGroupString(configGroupName,configGroupKey),
					true); //include meta data table


	if(progressBar) progressBar->step();

	//remove meta data table and extract info
	auto metaTablePair = memberMap.find(groupMetadataTable_.getConfigurationName());
	if(metaTablePair !=
			memberMap.end())
	{
		__MOUT__ << "Found group meta data. v" <<
				metaTablePair->second << std::endl;
		//clear table
		while(groupMetadataTable_.getView().getNumberOfRows())
			groupMetadataTable_.getViewP()->deleteRow(0);
		theInterface_->fill(&groupMetadataTable_,metaTablePair->second);
		//check that there is only 1 row
		if(groupMetadataTable_.getView().getNumberOfRows() != 1)
		{
			groupMetadataTable_.print();
			__SS__ << "groupMetadataTable_ has wrong number of rows! Must be 1." << std::endl;
			__MOUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}
		//groupMetadataTable_.print();

		memberMap.erase(metaTablePair);

		//extract fields
		if(groupComment) *groupComment 			= groupMetadataTable_.getView().getValueAsString(0,1);
		if(groupAuthor) *groupAuthor 			= groupMetadataTable_.getView().getValueAsString(0,2);
		if(groupCreateTime) *groupCreateTime 	= groupMetadataTable_.getView().getValueAsString(0,3);
	}

	if(progressBar) progressBar->step();

	__MOUT__ << "memberMap loaded size = " << memberMap.size() << std::endl;

	if(doNotLoadMember) return memberMap; //this is useful if just getting group metadata

	int groupType = -1;

	//determine the type configuration group
	groupType = getTypeOfGroup(memberMap);
	if(groupTypeString) *groupTypeString = convertGroupTypeIdToName(groupType);

	if(doActivate)
	{
		std::string groupToDeactivate =
				groupType==CONTEXT_TYPE?theContextGroup_:
						(groupType==BACKBONE_TYPE?theBackboneGroup_:theConfigurationGroup_);

		//		deactivate all of that type (invalidate active view)
		if(groupToDeactivate != "") //deactivate only if pre-existing group
		{
			__MOUT__ << "groupToDeactivate '" << groupToDeactivate << "'" << std::endl;
			destroyConfigurationGroup(groupToDeactivate,true);
		}
//		else
//		{
//			//Getting here, is kind of strange:
//			//	- this group may have only been partially loaded before?
//		}
	}

	if(progressBar) progressBar->step();

	__MOUT__ << "Activating chosen group:" << std::endl;


	loadMemberMap(memberMap);

	if(progressBar) progressBar->step();

	if(accumulatedTreeErrors)
	{
		__MOUT__ << "Checking chosen group for tree errors..." << std::endl;

		getChildren(&memberMap, accumulatedTreeErrors);
		if(*accumulatedTreeErrors != "")
		{
			__MOUT_ERR__ << "Errors detected while loading Configuration Group: " << configGroupName <<
					"(" << configGroupKey << "). Aborting." << std::endl;
			return memberMap; //return member name map to version
		}
	}

	if(progressBar) progressBar->step();

	//	for each member
	//		if doActivate, configBase->init()
	if(doActivate)
		for(auto &memberPair:memberMap)
		{
			//do NOT allow activating Scratch versions if tracking is ON!
			if(ConfigurationInterface::isVersionTrackingEnabled() &&
					memberPair.second.isScratchVersion())
			{
				__SS__ << "Error while activating member Table '" <<
						nameToConfigurationMap_[memberPair.first]->getConfigurationName() <<
						"-v" << memberPair.second <<
						" for Configuration Group '" << configGroupName <<
						"(" << configGroupKey << ")'. When version tracking is enabled, Scratch views" <<
						" are not allowed! Please only use unique, persistent versions when version tracking is enabled."
						<< std::endl;
				__MOUT_ERR__ << "\n" << ss.str();
				throw std::runtime_error(ss.str());
			}


			//attempt to init using the configuration's specific init
			//	this could be risky user code, try and catch
			try
			{
				nameToConfigurationMap_[memberPair.first]->init(this);
			}
			catch(std::runtime_error& e)
			{
				__SS__ << "Error detected calling " <<
						nameToConfigurationMap_[memberPair.first]->getConfigurationName() <<
						".init()!\n\n " << e.what() << std::endl;
				throw std::runtime_error(ss.str());
			}
			catch(...)
			{
				__SS__ << "Error detected calling " <<
						nameToConfigurationMap_[memberPair.first]->getConfigurationName() <<
						".init()!\n\n " << std::endl;
				throw std::runtime_error(ss.str());
			}

		}

	if(progressBar) progressBar->step();


	//	if doActivate
	//		set theConfigurationGroup_, theContextGroup_, or theBackboneGroup_ on success

	if(doActivate)
	{
		if(groupType == CONTEXT_TYPE) //
		{
			__MOUT_INFO__ << "Context Configuration Group loaded: " << configGroupName <<
					"(" << configGroupKey << ")" << std::endl;
			theContextGroup_ = configGroupName;
			theContextGroupKey_ = std::shared_ptr<ConfigurationGroupKey>(new ConfigurationGroupKey(configGroupKey));
		}
		else if(groupType == BACKBONE_TYPE)
		{
			__MOUT_INFO__ << "Backbone Configuration Group loaded: " << configGroupName <<
					"(" << configGroupKey << ")" << std::endl;
			theBackboneGroup_ = configGroupName;
			theBackboneGroupKey_ = std::shared_ptr<ConfigurationGroupKey>(new ConfigurationGroupKey(configGroupKey));
		}
		else //is theConfigurationGroup_
		{
			__MOUT_INFO__ << "The Configuration Group loaded: " << configGroupName <<
					"(" << configGroupKey << ")" << std::endl;
			theConfigurationGroup_ = configGroupName;
			theConfigurationGroupKey_ = std::shared_ptr<ConfigurationGroupKey>(new ConfigurationGroupKey(configGroupKey));
		}
	}

	if(progressBar) progressBar->step();
	__MOUT__ << "Load complete." << std::endl;
	return memberMap;
}


//==============================================================================
//setActiveGlobalConfiguration
//	get theActiveGlobalConfig_
//   map<type,        pair     <groupName  , ConfigurationGroupKey> >
//
//	Note: invalid ConfigurationGroupKey means no active group currently
std::map<std::string, std::pair<std::string, ConfigurationGroupKey> > ConfigurationManager::getActiveConfigurationGroups(void) const
{
	//   map<type,        pair     <groupName  , ConfigurationGroupKey> >
	std::map<std::string, std::pair<std::string, ConfigurationGroupKey> > retMap;

	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_CONTEXT]       =
			std::pair<std::string,ConfigurationGroupKey>(theContextGroup_      ,theContextGroupKey_      ?*theContextGroupKey_      : ConfigurationGroupKey());
	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_BACKBONE]      =
			std::pair<std::string,ConfigurationGroupKey>(theBackboneGroup_     ,theBackboneGroupKey_     ?*theBackboneGroupKey_     : ConfigurationGroupKey());
	retMap[ConfigurationManager::ACTIVE_GROUP_NAME_CONFIGURATION] =
			std::pair<std::string,ConfigurationGroupKey>(theConfigurationGroup_,theConfigurationGroupKey_?*theConfigurationGroupKey_: ConfigurationGroupKey());
	return retMap;
}

//==============================================================================
ConfigurationTree ConfigurationManager::getSupervisorConfigurationNode(
		const std::string &contextUID, const std::string &applicationUID) const
{
	return getNode(
			"/" + getConfigurationByName(XDAQ_CONTEXT_CONFIG_NAME)->getConfigurationName() +
			"/" + contextUID +
			"/LinkToApplicationConfiguration/" + applicationUID +
			"/LinkToSupervisorConfiguration");
}

//==============================================================================
ConfigurationTree ConfigurationManager::getNode(const std::string& nodeString) const
{
	__MOUT__ << "nodeString=" << nodeString << " " << nodeString.length() << std::endl;

	//get nodeName (in case of / syntax)
	if(nodeString.length() < 1) throw std::runtime_error("Invalid empty node name");

	bool startingSlash = (nodeString[0] == '/');

	std::string nodeName = nodeString.substr(startingSlash?1:0, nodeString.find('/',1)-(startingSlash?1:0));
	__MOUT__ << "nodeName=" << nodeName << " " << nodeName.length() << std::endl;
	if(nodeName.length() < 1) throw std::runtime_error("Invalid node name: " + nodeName);

	std::string childPath = nodeString.substr(nodeName.length() + (startingSlash?1:0));

	__MOUT__ << "childPath=" << childPath << " " << childPath.length() << std::endl;

	ConfigurationTree configTree(this, getConfigurationByName(nodeName));

	if(childPath.length() > 1)
		return configTree.getNode(childPath);
	else
		return configTree;
}

//==============================================================================
//getFirstPathToNode
std::string	ConfigurationManager::getFirstPathToNode(const ConfigurationTree &node, const std::string &startPath) const
//void ConfigurationManager::getFirstPathToNode(const ConfigurationTree &node, const ConfigurationTree &startNode) const
{
	std::string path = "/";
	return path;
}

//==============================================================================
//getChildren
//	if memberMap is passed then only consider children in the map
//
//	if accumulatedTreeErrors is non null, check for disconnects occurs.
//		check is 2 levels deep which should get to the links starting at tables.
std::vector<std::pair<std::string,ConfigurationTree> >	ConfigurationManager::getChildren(
		std::map<std::string, ConfigurationVersion> *memberMap,
		std::string *accumulatedTreeErrors) const
{
	std::vector<std::pair<std::string,ConfigurationTree> > retMap;
	if(accumulatedTreeErrors) *accumulatedTreeErrors = "";

	if(!memberMap || memberMap->empty())	//return all present active members
	{
		for(auto &configPair:nameToConfigurationMap_)
		{
			//__MOUT__ << configPair.first <<  " " << (int)(configPair.second?1:0) << std::endl;

			if(configPair.second->isActive()) //only consider if active
			{
				ConfigurationTree newNode(this, configPair.second);


				if(accumulatedTreeErrors) //check for disconnects
				{
					try
					{
						std::vector<std::pair<std::string,ConfigurationTree> > newNodeChildren =
								newNode.getChildren();
						for(auto &newNodeChild: newNodeChildren)
						{
							std::vector<std::pair<std::string,ConfigurationTree> > twoDeepChildren =
									newNodeChild.second.getChildren();

							for(auto &twoDeepChild: twoDeepChildren)
							{
								//__MOUT__ << configPair.first << " " << newNodeChild.first << " " <<
								//		twoDeepChild.first << std::endl;
								if(twoDeepChild.second.isLinkNode() &&
										twoDeepChild.second.isDisconnected() &&
										twoDeepChild.second.getDisconnectedTableName() !=
												ViewColumnInfo::DATATYPE_LINK_DEFAULT)
									*accumulatedTreeErrors += "\n\nAt node '" +
									configPair.first + "' with entry UID '" +
									newNodeChild.first + "' there is a disconnected child node at link column '" +
									twoDeepChild.first + "'" +
									" that points to table named '" +
									twoDeepChild.second.getDisconnectedTableName() +
									"' ...";
							}
						}
					}
					catch(std::runtime_error &e)
					{
						*accumulatedTreeErrors += "\n\nAt node '" +
								configPair.first + "' error detected descending through children:\n" +
								e.what();
					}
				}

				retMap.push_back(std::pair<std::string,ConfigurationTree>(configPair.first,
						newNode));
			}

			//__MOUT__ << configPair.first <<  std::endl;
		}
	}
	else //return only members from the member map (they must be present and active!)
	{

		for(auto &memberPair: *memberMap)
		{
			auto mapIt = nameToConfigurationMap_.find(memberPair.first);
			if(mapIt == nameToConfigurationMap_.end())
			{
				__SS__ << "Get Children with member map requires a child '" <<
						memberPair.first << "' that is not present!" << std::endl;
				throw std::runtime_error(ss.str());
			}
			if(!(*mapIt).second->isActive())
			{
				__SS__ << "Get Children with member map requires a child '" <<
						memberPair.first << "' that is not active!" << std::endl;
				throw std::runtime_error(ss.str());
			}

			ConfigurationTree newNode(this, (*mapIt).second);

			if(accumulatedTreeErrors) //check for disconnects
			{
				try
				{
					std::vector<std::pair<std::string,ConfigurationTree> > newNodeChildren =
							newNode.getChildren();
					for(auto &newNodeChild: newNodeChildren)
					{
						std::vector<std::pair<std::string,ConfigurationTree> > twoDeepChildren =
								newNodeChild.second.getChildren();

						for(auto &twoDeepChild: twoDeepChildren)
						{
							//__MOUT__ << memberPair.first << " " << newNodeChild.first << " " <<
							//		twoDeepChild.first << std::endl;
							if(twoDeepChild.second.isLinkNode() &&
									twoDeepChild.second.isDisconnected() &&
									twoDeepChild.second.getDisconnectedTableName() !=
											ViewColumnInfo::DATATYPE_LINK_DEFAULT)
							{
								*accumulatedTreeErrors += "\n\nAt node '" +
								memberPair.first + "' with entry UID '" +
								newNodeChild.first + "' there is a disconnected child node at link column '" +
								twoDeepChild.first + "'" +
								" that points to table named '" +
								twoDeepChild.second.getDisconnectedTableName() +
								"' ...";

								//check if disconnected table is in group, if not software error

								bool found = false;
								for(auto &searchMemberPair: *memberMap)
									if(searchMemberPair.first ==
											twoDeepChild.second.getDisconnectedTableName())
									{ found = true; break;}
								if(!found)
									*accumulatedTreeErrors +=
											std::string("\nNote: It may be safe to ignore this error ") +
									"since the link's target table " +
									twoDeepChild.second.getDisconnectedTableName() +
									" is not a member of this group (and may not be loaded yet).";
							}
						}
					}
				}
				catch(std::runtime_error &e)
				{
					*accumulatedTreeErrors += "\n\nAt node '" +
							memberPair.first + "' error detected descending through children:\n" +
							e.what();
				}
			}

			retMap.push_back(std::pair<std::string,ConfigurationTree>(memberPair.first,
					newNode));
		}
	}

	return retMap;
}

//==============================================================================
//getConfigurationByName
//	Get read-only pointer to configuration.
//	If Read/Write access is needed use ConfigurationManagerWithWriteAccess
//		(For general use, Write access should be avoided)
const ConfigurationBase* ConfigurationManager::getConfigurationByName(const std::string &configurationName) const
{
	std::map<std::string, ConfigurationBase*>::const_iterator it;
	if((it = nameToConfigurationMap_.find(configurationName)) == nameToConfigurationMap_.end())
	{
		__SS__ << "\n\nCan not find configuration named '" <<
				configurationName <<
				"'\n\n\n\nYou need to load the configuration before it can be used." <<
				" It probably is missing from the member list of the Configuration Group that was loaded.\n" <<
				"\nYou may need to enter wiz mode to remedy the situation, use the following:\n" <<
				"\n\t StartOTS.sh --wiz" <<
				"\n\n\n\n"
				<< std::endl;
		//prints out too often
		//if(configurationName != ViewColumnInfo::DATATYPE_LINK_DEFAULT)
		//	__MOUT_WARN__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}
	return it->second;
}

//==============================================================================
//loadConfigurationBackbone
//	loads the active backbone configuration group
//	returns the active group key that was loaded
ConfigurationGroupKey ConfigurationManager::loadConfigurationBackbone()
{
	if(!theBackboneGroupKey_) //no active backbone
	{
		__MOUT_WARN__ << "getConfigurationGroupKey() Failed! No active backbone currently." << std::endl;
		return ConfigurationGroupKey();
	}

	//may already be loaded, but that's ok, load anyway to be sure
	loadConfigurationGroup(theBackboneGroup_,*theBackboneGroupKey_);

	return *theBackboneGroupKey_;
}

//=============================================================ConfigurationManager=================
// verifies that all backbone members are loaded and have the same version
////	and returns the version
//ConfigurationVersion ConfigurationManager::getConfigurationBackboneVersion(void)
//{
//	//check versions match
//	//check all except version aliases since it is always latest
//	ConfigurationVersion v = getConfigurationByName(*(backboneMemberNames_.begin()))->getViewVersion();
//	for(const auto& name : backboneMemberNames_)
//	{
//		if(name == "VersionAliases" )
//			break;
//		if(v != getConfigurationByName(name)->getViewVersion())
//		{
//			__MOUT__ << "Backbone Versions do no match! " << v << " vs " <<
//					name << ":" <<
//					getConfigurationByName(name)->getViewVersion() << std::endl;
//			throw std::runtime_error("Backbone Versions do no match!");
//		}
//	}
//	return v;
//}




//Getters
//==============================================================================
//getConfigurationGroupKey
//	use backbone to determine default key for runType.
//		- runType translates to group key alias,
//			which maps to a group name and key pair
//	set
//
//	return INVALID on failure
//   pair<group name , ConfigurationGroupKey>
std::pair<std::string, ConfigurationGroupKey> ConfigurationManager::getConfigurationGroupFromAlias(std::string runType,	ProgressBar* progressBar)
{
	//steps
	//	load active backbone
	//	find runType in GroupAliasesConfiguration
	//	return key

	if(progressBar) progressBar->step();

	loadConfigurationBackbone();

	if(progressBar) progressBar->step();

	try
	{
		//	find runType in GroupAliasesConfiguration
		ConfigurationTree entry = getNode("GroupAliasesConfiguration").getNode(runType);
		return std::pair<std::string, ConfigurationGroupKey>(entry.getNode("GroupName").getValueAsString(),	ConfigurationGroupKey(entry.getNode("GroupKey").getValueAsString()));
	}
	catch(...)
	{}

	if(progressBar) progressBar->step();

	return std::pair<std::string, ConfigurationGroupKey>("",ConfigurationGroupKey());
}

//==============================================================================
//   map<alias      ,      pair<group name,  ConfigurationGroupKey> >
std::map<std::string, std::pair<std::string, ConfigurationGroupKey> > ConfigurationManager::getGroupAliasesConfiguration(void)
{
	restoreActiveConfigurationGroups(); //make sure the active configuration backbone is loaded!
	//loadConfigurationBackbone();

	std::map<std::string /*alias*/,
	std::pair<std::string /*group name*/, ConfigurationGroupKey> > retMap;

	std::vector<std::pair<std::string,ConfigurationTree> > entries = getNode("GroupAliasesConfiguration").getChildren();
	for(auto &entryPair: entries)
	{
		retMap[entryPair.first] = std::pair<std::string, ConfigurationGroupKey>(
				entryPair.second.getNode("GroupName").getValueAsString(),
				ConfigurationGroupKey(entryPair.second.getNode("GroupKey").getValueAsString()));

	}
	return retMap;
}

//==============================================================================
//getActiveVersions
std::map<std::string, ConfigurationVersion> ConfigurationManager::getActiveVersions(void) const
{
	std::map<std::string, ConfigurationVersion> retMap;
	for(auto &config:nameToConfigurationMap_)
	{
		//__MOUT__ << config.first << std::endl;

		//check configuration pointer is not null and that there is an active view
		if(config.second && config.second->isActive())
		{
			//__MOUT__ << config.first << "_v" << config.second->getViewVersion() << std::endl;
			retMap.insert(std::pair<std::string, ConfigurationVersion>(config.first, config.second->getViewVersion()));
		}
	}
	return retMap;
}

////==============================================================================
//const DACStream& ConfigurationManager::getDACStream(std::string fecName)
//{
//
//	//fixme/todo this is called before setupAll so it breaks!
//	//====================================================
//	const DetectorConfiguration* detectorConfiguration = __GET_CONFIG__(DetectorConfiguration);
//	for(auto& type : detectorConfiguration->getDetectorTypes())
//		theDACsConfigurations_[type] = (DACsConfigurationBase*)(getConfigurationByName(type + "DACsConfiguration"));
//	//====================================================
//
//	theDACStreams_[fecName].makeStream(fecName,
//			__GET_CONFIG__(DetectorConfiguration),
//			__GET_CONFIG__(DetectorToFEConfiguration),
//			theDACsConfigurations_,
//			__GET_CONFIG__(MaskConfiguration));//, theTrimConfiguration_);
//
//	__MOUT__ << "Done with DAC stream!" << std::endl;
//	return theDACStreams_[fecName];
//}

//==============================================================================
std::shared_ptr<ConfigurationGroupKey> ConfigurationManager::makeTheConfigurationGroupKey(ConfigurationGroupKey key)
{
	if(theConfigurationGroupKey_)
	{
		if(*theConfigurationGroupKey_ != key)
			destroyConfigurationGroup();
		else
			return theConfigurationGroupKey_;
	}
	return  std::shared_ptr<ConfigurationGroupKey>(new ConfigurationGroupKey(key));
}

//==============================================================================
std::string ConfigurationManager::encodeURIComponent(const std::string &sourceStr)
{
	std::string retStr = "";
	char encodeStr[4];
	for(const auto &c:sourceStr)
		if(
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') )
			retStr += c;
		else
		{
			sprintf(encodeStr,"%%%2.2X",c);
			retStr += encodeStr;
		}
	return retStr;
}










