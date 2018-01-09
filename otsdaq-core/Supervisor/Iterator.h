#ifndef _ots_Iterator_h
#define _ots_Iterator_h


#include "otsdaq-core/ConfigurationPluginDataFormats/IterateConfiguration.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include <mutex>        //for std::mutex
#include <string>

#include "otsdaq-core/ConfigurationInterface/ConfigurationManagerRW.h"

namespace ots
{

class Supervisor;
class ConfigurationManagerRW;

class Iterator
{
	friend class Supervisor;
	
public:
    Iterator (Supervisor* supervisor);
    ~Iterator(void);

    void									playIterationPlan			(HttpXmlDocument& xmldoc, const std::string& planName);
    void									pauseIterationPlan			(HttpXmlDocument& xmldoc);
    void									haltIterationPlan			(HttpXmlDocument& xmldoc);
    void									getIterationPlanStatus		(HttpXmlDocument& xmldoc);

    bool									handleCommandRequest		(HttpXmlDocument& xmldoc, const std::string& command, const std::string& parameter);

private:

    //begin declaration of iterator workloop members
    struct IteratorWorkLoopStruct
	{
    	IteratorWorkLoopStruct(Iterator* iterator, ConfigurationManagerRW* cfgMgr)
    	:theIterator_			(iterator)
    	,cfgMgr_				(cfgMgr)
    	,originalTrackChanges_	(false)
    	,running_				(false)
    	,commandBusy_			(false)
    	,doPauseAction_			(false)
    	,doHaltAction_			(false)
    	,doResumeAction_		(false)
    	,commandIndex_			((unsigned int)-1)
    	{}

    	Iterator *									theIterator_;
    	ConfigurationManagerRW* 					cfgMgr_;
    	bool										originalTrackChanges_;
    	std::string									originalConfigGroup_;
    	ConfigurationGroupKey						originalConfigKey_;

    	bool 										running_, commandBusy_;
    	bool										doPauseAction_, doHaltAction_, doResumeAction_;

    	std::string 								activePlan_;
    	std::vector<IterateConfiguration::Command> 	commands_;
    	std::vector<unsigned int>				 	commandIterations_;
    	unsigned int 								commandIndex_;
    	std::vector<unsigned int>					stepIndexStack_;

    	//associated with FSM
    	std::string 								fsmName_, fsmRunAlias_;
    	unsigned int 								fsmNextRunNumber_;
    	bool										runIsDone_;

    	std::vector<std::string> 					fsmCommandParameters_;


	}; //end declaration of iterator workloop members

    static void								IteratorWorkLoop			(Iterator *iterator);
    static void								startCommand				(IteratorWorkLoopStruct *iteratorStruct);
    static bool								checkCommand				(IteratorWorkLoopStruct *iteratorStruct);

    static void								startCommandChooseFSM		(IteratorWorkLoopStruct *iteratorStruct, const std::string& fsmName);

    static void								startCommandConfigureActive	(IteratorWorkLoopStruct *iteratorStruct);
    static void								startCommandConfigureAlias	(IteratorWorkLoopStruct *iteratorStruct, const std::string& systemAlias);
    static void								startCommandConfigureGroup	(IteratorWorkLoopStruct *iteratorStruct);
    static bool								checkCommandConfigure		(IteratorWorkLoopStruct *iteratorStruct);

    static void								startCommandModifyActive	(IteratorWorkLoopStruct *iteratorStruct);


    static void								startCommandBeginLabel		(IteratorWorkLoopStruct *iteratorStruct);
    static void								startCommandRepeatLabel		(IteratorWorkLoopStruct *iteratorStruct);

    static void								startCommandRun				(IteratorWorkLoopStruct *iteratorStruct);
    static bool								checkCommandRun				(IteratorWorkLoopStruct *iteratorStruct);

    static bool								haltStateMachine			(Supervisor* theSupervisor, const std::string& fsmName);

    std::mutex								accessMutex_;
    volatile bool							workloopRunning_;
    volatile bool							activePlanIsRunning_;
    volatile bool							iteratorBusy_;
    volatile bool							commandPlay_, commandPause_, commandHalt_; //commands are set by supervisor thread, and cleared by iterator thread
    std::string								activePlanName_, lastStartedPlanName_, lastFinishedPlanName_;
    volatile unsigned int					activeCommandIndex_, activeCommandIteration_;
	std::vector<unsigned int>				depthIterationStack_;
    volatile time_t							activeCommandStartTime_;
    std::string 							lastFsmName_;
    std::string 							errorMessage_;

    Supervisor* 							theSupervisor_;

    //========================================================================================================================
    //helpCommandModifyActive
    //	set the value to all targets
    template<class T>
    static void								helpCommandModifyActive(
    		IteratorWorkLoopStruct *iteratorStruct,
			const T& setValue,
			bool doTrackGroupChanges)
    try
    {
    	__COUT__ << "Set value " << setValue << __E__;
    	__COUT__ << "doTrackGroupChanges " << doTrackGroupChanges << __E__;


    	const std::string& pathToField =
    			iteratorStruct->commands_[iteratorStruct->commandIndex_].params_
				[IterateConfiguration::commandModifyActiveParams_.RelativePathToField_];

    	__COUT__ << "pathToField " << pathToField << std::endl;

    	ConfigurationManagerRW* cfgMgr = iteratorStruct->cfgMgr_;

    	std::stringstream valSS;
    	valSS << setValue;

    	std::string groupName = cfgMgr->getActiveGroupName();
    	ConfigurationGroupKey originalKey = cfgMgr->getActiveGroupKey();

    	__COUT__ << "Active group is '" << groupName << " (" <<
    			originalKey << ")'" << __E__;

    	//track member names and versions of active config group
    	//	modify versions as tables are modified..
    	//	then save new group at end
    	std::map<std::string /*name*/, ConfigurationVersion /*version*/> memberMap;
    	try
    	{
    		memberMap = cfgMgr->loadConfigurationGroup(
    				groupName,
					originalKey,
					0,0,0,0,0,0, //defaults
					true); //doNotLoadMember
    	}
    	catch(const std::runtime_error& e)
    	{
    		__SS__ << "Failed to load the active configuration group. Is there a valid one activated? " <<
    				e.what() << __E__;
    		throw std::runtime_error(ss.str());
    	}

    	for(const auto& member: memberMap)
    		__COUT__ << "newGroup " << member.first << "-v" << member.second << __E__;

    	for(const auto& target:
    			iteratorStruct->commands_[iteratorStruct->commandIndex_].targets_)
    	{
    		__COUT__ << "target " << target.table_ << ":" << target.UID_ << __E__;
    		ConfigurationTree node = cfgMgr->getNode(
    				target.table_).getNode(
    						target.UID_).getNode(
    								pathToField);

    		if(!node.isValueNode())
    		{
    			__SS__ << "Invalid target node. The target path '" << pathToField <<
    					"' must be a value node." << __E__;
    			throw std::runtime_error(ss.str());
    		}

    		__COUT__ << "value node table: " << node.getConfigurationName() << __E__;
    		__COUT__ << "value node UID: " << node.getUIDAsString() << __E__;

    		__COUT__ << "value node row: " << node.getFieldRow() << __E__;
    		__COUT__ << "value node column: " << node.getFieldColumn() << __E__;



    		TableEditStruct valueTable(node.getConfigurationName(),cfgMgr);	// Table ready for editing!

    		try
    		{
    			valueTable.cfgView_->setValueAsString(
    					valSS.str(),
						node.getFieldRow(),
						node.getFieldColumn()
    			);


    			valueTable.cfgView_->print();
    			valueTable.cfgView_->init(); //verify new table (throws runtime_errors)
    		}
    		catch(...)
    		{
    			__COUT__ << "Handling command table errors while saving. Erasing all newly created versions." << std::endl;

    			if(valueTable.createdTemporaryVersion_) //if temporary version created here
    			{
    				__COUT__ << "Erasing temporary version " << valueTable.configName_ <<
    						"-v" << valueTable.temporaryVersion_ << std::endl;
    				//erase with proper version management
    				cfgMgr->eraseTemporaryVersion(valueTable.configName_,valueTable.temporaryVersion_);
    			}

    			throw; //re-throw
    		}

    		//at this point valid edited table.. so save

    		ConfigurationVersion newAssignedVersion;

    		//start save table code
    		//	do not check for duplicates if not tracking changes
    		{
    			const std::string& configName 					= valueTable.configName_;
    			ConfigurationBase* config 						= valueTable.config_;
    			ConfigurationVersion& temporaryModifiedVersion	= valueTable.temporaryVersion_;


    			//check for duplicate tables already in cache
    			if(doTrackGroupChanges) //check for duplicates if tracking changes
    			{
    				__COUT__ << "Checking for duplicate tables..." << std::endl;

    				{
    					//"DEEP" checking
    					//	load into cache 'recent' versions for this table
    					//		'recent' := those already in cache, plus highest version numbers not in cache
    					const std::map<std::string, ConfigurationInfo>& allCfgInfo =
    							cfgMgr->getAllConfigurationInfo(); //do not refresh

    					auto versionReverseIterator = allCfgInfo.at(configName).versions_.rbegin(); //get reverse iterator
    					__COUT__ << "Filling up cached from " <<
    							config->getNumberOfStoredViews() <<
								" to max count of " << config->MAX_VIEWS_IN_CACHE << std::endl;
    					for(;config->getNumberOfStoredViews() < config->MAX_VIEWS_IN_CACHE &&
    					versionReverseIterator != allCfgInfo.at(configName).versions_.rend();++versionReverseIterator)
    					{
    						__COUT__ << "Versions in reverse order " << *versionReverseIterator << std::endl;
    						cfgMgr->getVersionedConfigurationByName(configName,*versionReverseIterator); //load to cache
    					}
    				}

    				//check for duplicates (including original version)
    				newAssignedVersion = config->checkForDuplicate(temporaryModifiedVersion);

    				__COUT__ << "Check for duplicate tables complete." << std::endl;
    			}


    			if(!newAssignedVersion.isInvalid())
    			{
    				//found an equivalent!
    				__COUT__ << "Equivalent table found in version v" << newAssignedVersion << std::endl;

    				//erase modified temporary version
    				cfgMgr->eraseTemporaryVersion(configName,temporaryModifiedVersion);
    			}
    			else
    			{
    				//save as new table version

    				__COUT__ << "\t\t**************************** Save v" <<
    						temporaryModifiedVersion << " as new table version" << std::endl;

    				newAssignedVersion  =
    						cfgMgr->saveNewConfiguration(configName,temporaryModifiedVersion,
    								false /*makeTemporary*/);

    			}

    			__COUT__ << "\t\t newAssignedVersion: " << newAssignedVersion << std::endl;

    		} //end save table code

    		//now have newAssignedVersion

    		__COUT__ << "Final plan version is " << valueTable.configName_ << "-v" <<
    				newAssignedVersion << std::endl;

    		//record new version in modified tables map
    		memberMap[valueTable.configName_] = newAssignedVersion;

    	} //end target loop

    	//member map should now be modified
    	for(const auto& member: memberMap)
    		__COUT__ << "newGroup " << member.first << "-v" << member.second << __E__;

    	__COUT__ << "Checking for duplicate groups..." << std::endl;
    	ConfigurationGroupKey theKey =
    			cfgMgr->findConfigurationGroup(groupName,memberMap);

    	if(!theKey.isInvalid())
    	{
			__COUT__ << "Found equivalent group key (" << theKey << ") for " <<
					groupName << "." << std::endl;
    	}
    	else
    	{
			__COUT__ << "Saving new group..." << __E__;

			//save new group
			theKey = cfgMgr->saveNewConfigurationGroup(
					groupName,
					memberMap,
					"Created by Iterator modify active configuration command.");
    	}

    	__COUT__ << "Final group key is " << groupName << "(" <<
    			theKey << ")" << std::endl;

    	//now active new group
    	__COUT__ << "Activating new group..." << __E__;
    	cfgMgr->activateConfigurationGroup(groupName,theKey);

    } //end helpCommandModifyActive
    catch(const std::runtime_error& e)
    {
    	__SS__ << "Modify command failed! " << e.what() << __E__;


    	throw std::runtime_error(ss.str());
    }
};

}

#endif
