#include "otsdaq-core/Supervisor/Iterator.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/Supervisor/Supervisor.h"
#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"
#include "otsdaq-core/WebUsersUtilities/WebUsers.h"


#include <iostream>
#include <thread>       //for std::thread

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "SupervisorIterator"

using namespace ots;

//========================================================================================================================
Iterator::Iterator(Supervisor* supervisor)
: workloopRunning_				(false)
, activePlanIsRunning_			(false)
, iteratorBusy_					(false)
, commandPlay_(false), commandPause_(false), commandHalt_(false)
, activePlanName_				("")
, activeCommandIndex_			(-1)
, activeCommandStartTime_		(0)
, theSupervisor_				(supervisor)
{
	__MOUT__ << "Iterator constructed." << __E__;
	__COUT__ << "Iterator constructed." << __E__;

}

//========================================================================================================================
Iterator::~Iterator(void)
{
}

//========================================================================================================================
void Iterator::IteratorWorkLoop(Iterator *iterator)
try
{
	__MOUT__ << "Iterator work loop starting..." << __E__;
	__COUT__ << "Iterator work loop starting..." << __E__;

	//mutex init scope
	{
		//lockout the messages array for the remainder of the scope
		//this guarantees the reading thread can safely access the messages
		if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
		std::lock_guard<std::mutex> lock(iterator->accessMutex_);
		if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

		iterator->errorMessage_ = ""; //clear error message
	}


	ConfigurationManagerRW theConfigurationManager(WebUsers::DEFAULT_ITERATOR_USERNAME); //this is a restricted username
	theConfigurationManager.getAllConfigurationInfo(true); // to prep all info

	IteratorWorkLoopStruct theIteratorStruct(iterator,
			&theConfigurationManager);


	const IterateConfiguration* itConfig;

	std::vector<IterateConfiguration::Command> commands;

	while(1)
	{
		//Process:
		//	- always "listen" for commands
		//		- play: if no plan running, activePlanIsRunning_ = true,
		//			and start or continue plan based on name/commandIndex
		//		- pause: if plan playing, pause it, activePlanIsRunning_ = false
		//			and do not clear commandIndex or name, iteratorBusy_ = true
		//		- halt: if plan playing or not, activePlanIsRunning_ = false
		//			and clear commandIndex or name, iteratorBusy_ = false
		//	- when running
		//		- go through each command
		//			- start the command, commandBusy = true
		//			- check for complete, then commandBusy = false

		//start command handling
		//define mutex scope
		{
			//lockout the messages array for the remainder of the scope
			//this guarantees the reading thread can safely access the messages
			if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
			std::lock_guard<std::mutex> lock(iterator->accessMutex_);
			if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

			if(iterator->commandPlay_)
			{
				iterator->commandPlay_ = false; //clear

				if(!iterator->activePlanIsRunning_)
				{
					//valid PLAY command!

					iterator->activePlanIsRunning_ = true;
					iterator->iteratorBusy_ = true;

					if(theIteratorStruct.activePlan_ != iterator->activePlanName_)
					{
						__COUT__ << "New plan name encountered old=" << theIteratorStruct.activePlan_ <<
								" vs new=" << iterator->activePlanName_ << __E__;
						theIteratorStruct.commandIndex_ = -1; //reset for new plan
					}

					theIteratorStruct.activePlan_ = iterator->activePlanName_;
					iterator->lastStartedPlanName_ = iterator->activePlanName_;

					if(theIteratorStruct.commandIndex_ == (unsigned int)-1)
					{
						__COUT__ << "Starting plan '" << theIteratorStruct.activePlan_ << ".'" << __E__;
						__MOUT__ << "Starting plan '" << theIteratorStruct.activePlan_ << ".'" << __E__;
					}
					else
					{
						theIteratorStruct.doResumeAction_ = true;
						__COUT__ << "Continuing plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
								theIteratorStruct.commandIndex_ << ". " << __E__;
						__MOUT__ << "Continuing plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
								theIteratorStruct.commandIndex_ << ". " << __E__;
					}
				}
			}
			else if(iterator->commandPause_ && !theIteratorStruct.doPauseAction_)
			{
				theIteratorStruct.doPauseAction_ = true;
				iterator->commandPause_ = false; //clear
			}
			else if(iterator->commandHalt_ && !theIteratorStruct.doHaltAction_)
			{
				theIteratorStruct.doHaltAction_ = true;
				iterator->commandHalt_ = false; //clear
			}

			theIteratorStruct.running_ = iterator->activePlanIsRunning_;

			if(iterator->activeCommandIndex_ != //update active command status if changed
					theIteratorStruct.commandIndex_)
			{
				iterator->activeCommandIndex_ = theIteratorStruct.commandIndex_;
				iterator->activeCommandStartTime_ = time(0); //reset on any change
			}

		} //end command handling and iterator mutex


		////////////////
		////////////////
		//do halt or pause action outside of iterator mutex

		if(theIteratorStruct.doPauseAction_)
		{
			//valid PAUSE command!

			//safely pause plan!
			//	i.e. check that command is complete

			__COUT__ << "Waiting to pause..." << __E__;
			while(!iterator->checkCommand(&theIteratorStruct))
				__COUT__ << "Waiting to pause..." << __E__;

			__COUT__ << "Completeing pause..." << __E__;

			theIteratorStruct.doPauseAction_ = false; //clear

			//lockout the messages array for the remainder of the scope
			//this guarantees the reading thread can safely access the messages
			if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
			std::lock_guard<std::mutex> lock(iterator->accessMutex_);
			if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

			iterator->activePlanIsRunning_ = false;

			__COUT__ << "Paused plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
					theIteratorStruct.commandIndex_ << ". " << __E__;
			__MOUT__ << "Paused plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
					theIteratorStruct.commandIndex_ << ". " << __E__;

			continue; //resume workloop
		}
		else if(theIteratorStruct.doHaltAction_)
		{
			//valid HALT command!

			//safely end plan!
			//	i.e. check that command is complete

			__COUT__ << "Waiting to halt..." << __E__;
			while(!iterator->checkCommand(&theIteratorStruct))
				__COUT__ << "Waiting to halt..." << __E__;

			__COUT__ << "Completing halt..." << __E__;

			theIteratorStruct.doHaltAction_ = false; //clear

			//last ditch effort to make sure FSM is halted
			iterator->haltStateMachine(
					iterator->theSupervisor_,
					theIteratorStruct.fsmName_);

			//lockout the messages array for the remainder of the scope
			//this guarantees the reading thread can safely access the messages
			if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
			std::lock_guard<std::mutex> lock(iterator->accessMutex_);
			if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

			iterator->activePlanIsRunning_ = false;
			iterator->iteratorBusy_ = false;

			__COUT__ << "Halted plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
					theIteratorStruct.commandIndex_ << ". " << __E__;
			__MOUT__ << "Halted plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
					theIteratorStruct.commandIndex_ << ". " << __E__;

			theIteratorStruct.activePlan_ = ""; //clear
			theIteratorStruct.commandIndex_ = -1; //clear

			continue; //resume workloop
		}


		////////////////
		////////////////
		//	handle running
		//		__COUT__ << "thinking.." << theIteratorStruct.running_ << " " <<
		//				theIteratorStruct.activePlan_ << " cmd=" <<
		//				theIteratorStruct.commandIndex_ << __E__;
		if(theIteratorStruct.running_ &&
				theIteratorStruct.activePlan_ != "") //important, because after errors, still "running" until halt
		{
			if(theIteratorStruct.commandIndex_ == (unsigned int)-1)
			{
				//initialize the running plan

				__COUT__ << "Get commands" << __E__;

				theIteratorStruct.commandIndex_ = 0;

				theIteratorStruct.cfgMgr_->init(); //completely reset to re-align with any changes
				itConfig = theIteratorStruct.cfgMgr_->__GET_CONFIG__(IterateConfiguration);

				theIteratorStruct.commands_ = itConfig->getPlanCommands(
						theIteratorStruct.cfgMgr_,
						theIteratorStruct.activePlan_);

				for(auto& command:theIteratorStruct.commands_)
				{
					__COUT__ << "command " <<
							command.type_ << __E__;
					__COUT__ << "table " <<
							IterateConfiguration::commandToTableMap_.at(command.type_) << __E__;
					__COUT__ << "param count = " << command.params_.size() << __E__;

					for(auto& param:command.params_)
					{
						__COUT__ << "\t param " <<
								param.first << " : " <<
								param.second << __E__;
					}
				}


				theIteratorStruct.originalTrackChanges_ =
						ConfigurationInterface::isVersionTrackingEnabled();
				theIteratorStruct.originalConfigGroup_ =
						theIteratorStruct.cfgMgr_->getActiveGroupName();
				theIteratorStruct.originalConfigKey_ =
						theIteratorStruct.cfgMgr_->getActiveGroupKey();

				__COUT__ << "originalTrackChanges " <<
						theIteratorStruct.originalTrackChanges_ << __E__;
				__COUT__ << "originalConfigGroup " <<
						theIteratorStruct.originalConfigGroup_ << __E__;
				__COUT__ << "originalConfigKey " <<
						theIteratorStruct.originalConfigKey_ << __E__;

			} //end initial section


			if(!theIteratorStruct.commandBusy_)
			{
				if(theIteratorStruct.commandIndex_ < theIteratorStruct.commands_.size())
				{
					//execute command
					theIteratorStruct.commandBusy_ = true;

					__COUT__ << "Iterator starting command " << theIteratorStruct.commandIndex_+1 << ": " <<
							theIteratorStruct.commands_[theIteratorStruct.commandIndex_].type_ << __E__;
					__MOUT__ << "Iterator starting command " << theIteratorStruct.commandIndex_+1 << ": " <<
							theIteratorStruct.commands_[theIteratorStruct.commandIndex_].type_ << __E__;

					iterator->startCommand(&theIteratorStruct);
				}
				else if(theIteratorStruct.commandIndex_ ==
						theIteratorStruct.commands_.size()) //Done!
				{
					__COUT__ << "Finished Iteration Plan '" << theIteratorStruct.activePlan_ << __E__;
					__MOUT__ << "Finished Iteration Plan '" << theIteratorStruct.activePlan_ << __E__;

					__COUT__ << "Reverting track changes." << __E__;
			    	ConfigurationInterface::setVersionTrackingEnabled(theIteratorStruct.originalTrackChanges_);

			    	__COUT__ << "Activating original group..." << __E__;
			    	try
			    	{
			    		theIteratorStruct.cfgMgr_->activateConfigurationGroup(
								theIteratorStruct.originalConfigGroup_,theIteratorStruct.originalConfigKey_);
			    	}
			    	catch(...)
			    	{
			    		__COUT_WARN__ << "Original group could not be activated." << __E__;
			    	}

			    	__COUT__ << "Completing halt..." << __E__;
			    	//leave FSM halted
			    	iterator->haltStateMachine(
			    			iterator->theSupervisor_,
							theIteratorStruct.fsmName_);

					//lockout the messages array for the remainder of the scope
					//this guarantees the reading thread can safely access the messages
					if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
					std::lock_guard<std::mutex> lock(iterator->accessMutex_);
					if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

					//similar to halt
					iterator->activePlanIsRunning_ = false;
					iterator->iteratorBusy_ = false;

					iterator->lastStartedPlanName_ = theIteratorStruct.activePlan_;
					theIteratorStruct.activePlan_ = ""; //clear
					theIteratorStruct.commandIndex_ = -1; //clear
				}
			}
			else if(theIteratorStruct.commandBusy_)
			{
				//check for command completion
				if(iterator->checkCommand(&theIteratorStruct))
				{
					theIteratorStruct.commandBusy_ = false; //command complete

					++theIteratorStruct.commandIndex_;

					__COUT__ << "Ready for next command. Done with " << theIteratorStruct.commandIndex_ << " of " <<
							theIteratorStruct.commands_.size() << __E__;
					__MOUT__ << "Iterator ready for next command. Done with " << theIteratorStruct.commandIndex_ << " of " <<
							theIteratorStruct.commands_.size() << __E__;
				}

				//Note: check command gets one shot to resume
				if(theIteratorStruct.doResumeAction_) //end resume action
					theIteratorStruct.doResumeAction_ = false;
			}



		} 	//end running
		else
			sleep(1); //when inactive sleep a lot

		////////////////
		////////////////

		//		__COUT__ << "end loop.." << theIteratorStruct.running_ << " " <<
		//				theIteratorStruct.activePlan_ << " cmd=" <<
		//				theIteratorStruct.commandIndex_ << __E__;

	}

	iterator->workloopRunning_ = false; //if we ever exit
}
catch(const std::runtime_error &e)
{
	__SS__ << "Encountered error in Iterator thread:\n" << e.what() << __E__;
	__COUT_ERR__ << ss.str();

	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
	std::lock_guard<std::mutex> lock(iterator->accessMutex_);
	if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

	iterator->workloopRunning_ = false; //if we ever exit
	iterator->errorMessage_ = ss.str();

}
catch(...)
{
	__SS__ << "Encountered unknown error in Iterator thread." << __E__;
	__COUT_ERR__ << ss.str();

	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
	std::lock_guard<std::mutex> lock(iterator->accessMutex_);
	if(iterator->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

	iterator->workloopRunning_ = false; //if we ever exit
	iterator->errorMessage_ = ss.str();
}

//========================================================================================================================
void Iterator::startCommand(IteratorWorkLoopStruct *iteratorStruct)
try
{
	//should be mutually exclusive with Supervisor main thread state machine accesses
	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	if(iteratorStruct->theIterator_->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for FSM access" << __E__;
	std::lock_guard<std::mutex> lock(iteratorStruct->theIterator_->theSupervisor_->stateMachineAccessMutex_);
	if(iteratorStruct->theIterator_->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have FSM access" << __E__;


	//for out of range, throw exception - should never happen
	if(iteratorStruct->commandIndex_ >= iteratorStruct->commands_.size())
	{
		__SS__ << "Out of range commandIndex = " << iteratorStruct->commandIndex_ <<
				" in size = " << iteratorStruct->commands_.size() << __E__;
		throw std::runtime_error(ss.str());
	}


	std::string type = iteratorStruct->commands_[iteratorStruct->commandIndex_].type_;
	if(type == IterateConfiguration::COMMAND_BEGIN_LABEL)
	{
		return startCommandBeginLabel(iteratorStruct);
	}
	else if(type == IterateConfiguration::COMMAND_CHOOSE_FSM)
	{
		return startCommandChooseFSM(
				iteratorStruct,
				iteratorStruct->commands_
				[iteratorStruct->commandIndex_].params_
				[IterateConfiguration::commandChooseFSMParams_.NameOfFSM_]);
	}
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_ACTIVE_GROUP)
	{
		return startCommandConfigureActive(iteratorStruct);
	}
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_ALIAS)
	{
		return startCommandConfigureAlias(
				iteratorStruct,
				iteratorStruct->commands_
				[iteratorStruct->commandIndex_].params_
				[IterateConfiguration::commandConfigureAliasParams_.SystemAlias_]);
	}
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_GROUP)
	{
		return startCommandConfigureGroup(iteratorStruct);
	}
	else if(type == IterateConfiguration::COMMAND_EXECUTE_FE_MACRO)
	{
		//TODO
		return;
	}
	else if(type == IterateConfiguration::COMMAND_EXECUTE_MACRO)
	{
		//TODO
		return;
	}
	else if(type == IterateConfiguration::COMMAND_MODIFY_ACTIVE_GROUP)
	{
		return startCommandModifyActive(iteratorStruct);
	}
	else if(type == IterateConfiguration::COMMAND_REPEAT_LABEL)
	{
		return startCommandRepeatLabel(iteratorStruct);
	}
	else if(type == IterateConfiguration::COMMAND_RUN)
	{
		return startCommandRun(iteratorStruct);
	}
	else
	{
		__SS__ << "Attempt to start unrecognized command type = " << type << __E__;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
}
catch(...)
{
	__COUT__ << "Error caught. Reverting track changes." << __E__;
	ConfigurationInterface::setVersionTrackingEnabled(iteratorStruct->originalTrackChanges_);

	__COUT__ << "Activating original group..." << __E__;
	try
	{
		iteratorStruct->cfgMgr_->activateConfigurationGroup(
				iteratorStruct->originalConfigGroup_,iteratorStruct->originalConfigKey_);
	}
	catch(...)
	{
		__COUT_WARN__ << "Original group could not be activated." << __E__;
	}
	throw;
}


//========================================================================================================================
//checkCommand
//	when busy for a while, start to sleep
//		use sleep() or nanosleep()
bool Iterator::checkCommand(IteratorWorkLoopStruct *iteratorStruct)
try
{
	//for out of range, return done
	if(iteratorStruct->commandIndex_ >= iteratorStruct->commands_.size())
	{
		__COUT__ << "Out of range commandIndex = " << iteratorStruct->commandIndex_ <<
				" in size = " << iteratorStruct->commands_.size() << __E__;
		return true;
	}

	std::string type = iteratorStruct->commands_[iteratorStruct->commandIndex_].type_;
	if(type == IterateConfiguration::COMMAND_BEGIN_LABEL)
	{
		//do nothing
		return true;
	}
	else if(type == IterateConfiguration::COMMAND_CHOOSE_FSM)
	{
		//do nothing
		return true;
	}
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_ALIAS ||
			type == IterateConfiguration::COMMAND_CONFIGURE_ACTIVE_GROUP ||
			type == IterateConfiguration::COMMAND_CONFIGURE_GROUP)
	{
		return checkCommandConfigure(iteratorStruct);
	}
	else if(type == IterateConfiguration::COMMAND_EXECUTE_FE_MACRO)
	{
		//do nothing
		return true;
	}
	else if(type == IterateConfiguration::COMMAND_EXECUTE_MACRO)
	{
		//do nothing
		return true;
	}
	else if(type == IterateConfiguration::COMMAND_MODIFY_ACTIVE_GROUP)
	{
		//do nothing
		return true;
	}
	else if(type == IterateConfiguration::COMMAND_REPEAT_LABEL)
	{
		//do nothing
		return true;
	}
	else if(type == IterateConfiguration::COMMAND_RUN)
	{
		return checkCommandRun(iteratorStruct);
	}
	else
	{
		__SS__ << "Attempt to check unrecognized command type = " << type << __E__;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
}
catch(...)
{
	__COUT__ << "Error caught. Reverting track changes." << __E__;
	ConfigurationInterface::setVersionTrackingEnabled(iteratorStruct->originalTrackChanges_);

	__COUT__ << "Activating original group..." << __E__;
	try
	{
		iteratorStruct->cfgMgr_->activateConfigurationGroup(
				iteratorStruct->originalConfigGroup_,iteratorStruct->originalConfigKey_);
	}
	catch(...)
	{
		__COUT_WARN__ << "Original group could not be activated." << __E__;
	}

	throw;
}

//========================================================================================================================
void Iterator::startCommandChooseFSM(IteratorWorkLoopStruct *iteratorStruct,
		const std::string& fsmName)
{
	__COUT__ << "fsmName " << fsmName << __E__;


	iteratorStruct->fsmName_ = fsmName;
	iteratorStruct->theIterator_->lastFsmName_ = fsmName;

	//Translate fsmName
	//	to gives us run alias (fsmRunAlias_) and next run number (fsmNextRunNumber_)


	//CAREFUL?? Threads


	//// ======================== get run alias based on fsm name ====

	iteratorStruct->fsmRunAlias_ = "Run"; //default to "Run"

	// get stateMachineAliasFilter if possible
	ConfigurationTree configLinkNode = iteratorStruct->cfgMgr_->getSupervisorConfigurationNode(
			iteratorStruct->theIterator_->theSupervisor_->supervisorContextUID_,
			iteratorStruct->theIterator_->theSupervisor_->supervisorApplicationUID_);

	if(!configLinkNode.isDisconnected())
	{
		try //for backwards compatibility
		{
			ConfigurationTree fsmLinkNode = configLinkNode.getNode("LinkToStateMachineConfiguration");
			if(!fsmLinkNode.isDisconnected())
				iteratorStruct->fsmRunAlias_  =
						fsmLinkNode.getNode(fsmName + "/RunDisplayAlias").getValue<std::string>();
			else
				__COUT_INFO__ << "FSM Link disconnected." << __E__;
		}
		catch(std::runtime_error &e)
		{
			//__COUT_INFO__ << e.what() << __E__;
			__COUT_INFO__ << "No state machine Run alias. Ignoring and assuming alias of '" <<
					iteratorStruct->fsmRunAlias_  << ".'" << __E__;

		}
		catch(...) {
			__COUT_ERR__ << "Unknown error. Should never happen." << __E__;

			__COUT_INFO__ << "No state machine Run alias. Ignoring and assuming alias of '" <<
					iteratorStruct->fsmRunAlias_  << ".'" << __E__;
		}
	}
	else
		__COUT_INFO__ << "FSM Link disconnected." << __E__;

	__COUT__ << "fsmRunAlias_  = " << iteratorStruct->fsmRunAlias_ 	<< __E__;




	//// ======================== get run number based on fsm name ====

	iteratorStruct->fsmNextRunNumber_  = iteratorStruct->theIterator_->theSupervisor_->getNextRunNumber(
			iteratorStruct->fsmName_);

	if(iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getCurrentStateName() == "Running" ||
			iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getCurrentStateName() == "Paused")
		--iteratorStruct->fsmNextRunNumber_; //current run number is one back

	__COUT__ << "fsmNextRunNumber_  = " << iteratorStruct->fsmNextRunNumber_ << __E__;
}

//========================================================================================================================
// return true if an action was attempted
bool Iterator::haltStateMachine(Supervisor* theSupervisor, const std::string& fsmName)
{
	std::vector<std::string> fsmCommandParameters;
	std::string errorStr = "";
	std::string currentState = theSupervisor->theStateMachine_.getCurrentStateName();

	if(currentState == "Initialized" ||
			currentState == "Halted")
	{
		__COUT__ << "Do nothing. Already halted." << __E__;
		return false;
	}
	else if(currentState == "Running")
		errorStr = theSupervisor->attemptStateMachineTransition(
				0,0,
				"Abort",fsmName,
				WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
				WebUsers::DEFAULT_ITERATOR_USERNAME,
				fsmCommandParameters);
	else
		errorStr = theSupervisor->attemptStateMachineTransition(
				0,0,
				"Halt",fsmName,
				WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
				WebUsers::DEFAULT_ITERATOR_USERNAME,
				fsmCommandParameters);

	if(errorStr != "")
	{
		__SS__ << "Iterator failed to halt because of the following error: " << errorStr;
		throw std::runtime_error(ss.str());
	}

	//else successfully launched
	__COUT__ << "FSM in transition = " << theSupervisor->theStateMachine_.isInTransition() << __E__;
	__COUT__ << "haltStateMachine launched." << __E__;
	return true;
}

//========================================================================================================================
void Iterator::startCommandBeginLabel(IteratorWorkLoopStruct *iteratorStruct)
{
	__COUT__ << "Entering label '" <<
			iteratorStruct->commands_
			[iteratorStruct->commandIndex_].params_
			[IterateConfiguration::commandBeginLabelParams_.Label_]
			 << "'..." << std::endl;

	//add new step index to stack
	iteratorStruct->stepIndexStack_.push_back(0);
}


//========================================================================================================================
void Iterator::startCommandRepeatLabel(IteratorWorkLoopStruct *iteratorStruct)
{
	//search for first matching label backward and set command to there

	int numOfRepetitions;
	sscanf(iteratorStruct->commands_[iteratorStruct->commandIndex_].params_
			[IterateConfiguration::commandRepeatLabelParams_.NumberOfRepetitions_].c_str(),
			"%d",&numOfRepetitions);
	__COUT__ << "numOfRepetitions remaining = " << numOfRepetitions << __E__;

	if(numOfRepetitions <= 0)
	{
		//remove step index from stack
		iteratorStruct->stepIndexStack_.pop_back();

		return; //no more repetitions
	}

	--numOfRepetitions;

	//increment step index in stack
	++(iteratorStruct->stepIndexStack_.back());

	unsigned int i;
	for(i=iteratorStruct->commandIndex_;i>0;--i) //assume 0 is always the fallback option
		if(iteratorStruct->commands_[i].type_ == IterateConfiguration::COMMAND_BEGIN_LABEL &&
				iteratorStruct->commands_[iteratorStruct->commandIndex_].params_[IterateConfiguration::commandRepeatLabelParams_.Label_] ==
						iteratorStruct->commands_[i].params_[IterateConfiguration::commandBeginLabelParams_.Label_]) break;

	char repStr[200];
	sprintf(repStr,"%d",numOfRepetitions);
	iteratorStruct->commands_[iteratorStruct->commandIndex_].params_
		[IterateConfiguration::commandRepeatLabelParams_.NumberOfRepetitions_] =
				repStr; //re-store as string

	iteratorStruct->commandIndex_ = i;
	__COUT__ << "Jumping back to commandIndex " << iteratorStruct->commandIndex_ << __E__;
}

//========================================================================================================================
void Iterator::startCommandRun(IteratorWorkLoopStruct *iteratorStruct)
{
	iteratorStruct->runIsDone_ = false;
	iteratorStruct->fsmCommandParameters_.clear();

	std::string errorStr = "";
	std::string currentState = iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getCurrentStateName();

	//execute first transition (may need two)

	if(currentState == "Configured")
		errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
				0,0,
				"Start",iteratorStruct->fsmName_,
				WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
				WebUsers::DEFAULT_ITERATOR_USERNAME,
				iteratorStruct->fsmCommandParameters_);
	else
		errorStr = "Can only Run from the Configured state. The current state is " +
			currentState;


	if(errorStr != "")
	{
		__SS__ << "Iterator failed to run because of the following error: " << errorStr;
		throw std::runtime_error(ss.str());
	}

	//else successfully launched
	__COUT__ << "FSM in transition = " << iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.isInTransition() << __E__;
	__COUT__ << "startCommandRun success." << __E__;
}

//========================================================================================================================
void Iterator::startCommandConfigureActive(IteratorWorkLoopStruct *iteratorStruct)
{
	__COUT__ << "startCommandConfigureActive " << __E__;

	//steps:
	//	get active config group
	//	transition to configure with parameters describing group

	std::string group = iteratorStruct->cfgMgr_->getActiveGroupName();
	ConfigurationGroupKey key = iteratorStruct->cfgMgr_->getActiveGroupKey();

	__COUT__ << "group " << group << __E__;
	__COUT__ << "key " << key << __E__;

	//create special alias for this group using : separators

	std::stringstream systemAlias;
	systemAlias << "GROUP:" << group << ":" << key;
	startCommandConfigureAlias(iteratorStruct,systemAlias.str());
}

//========================================================================================================================
void Iterator::startCommandConfigureGroup(IteratorWorkLoopStruct *iteratorStruct)
{
	__COUT__ << "startCommandConfigureGroup " << __E__;

	//steps:
	//	transition to configure with parameters describing group

	std::string group =
			iteratorStruct->commands_
			[iteratorStruct->commandIndex_].params_
			[IterateConfiguration::commandConfigureGroupParams_.GroupName_];
	ConfigurationGroupKey key = ConfigurationGroupKey(
			iteratorStruct->commands_
			[iteratorStruct->commandIndex_].params_
			[IterateConfiguration::commandConfigureGroupParams_.GroupKey_])
					;

	__COUT__ << "group " << group << __E__;
	__COUT__ << "key " << key << __E__;

	//create special alias for this group using : separators

	std::stringstream systemAlias;
	systemAlias << "GROUP:" << group << ":" << key;
	startCommandConfigureAlias(iteratorStruct,systemAlias.str());
}

//========================================================================================================================
void Iterator::startCommandConfigureAlias(IteratorWorkLoopStruct *iteratorStruct,
		const std::string& systemAlias)
{
	__COUT__ << "systemAlias " << systemAlias << __E__;

	iteratorStruct->fsmCommandParameters_.clear();
	iteratorStruct->fsmCommandParameters_.push_back(systemAlias);

	std::string errorStr = "";
	std::string currentState = iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getCurrentStateName();

	//execute first transition (may need two in conjunction with checkCommandConfigure())

	if(currentState == "Initial")
		errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
				0,0,
				"Initialize",iteratorStruct->fsmName_,
				WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
				WebUsers::DEFAULT_ITERATOR_USERNAME,
				iteratorStruct->fsmCommandParameters_);
	else if(currentState == "Halted")
		errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
				0,0,
				"Configure",iteratorStruct->fsmName_,
				WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
				WebUsers::DEFAULT_ITERATOR_USERNAME,
				iteratorStruct->fsmCommandParameters_);
	else if(currentState == "Configured")
		errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
				0,0,
				"Halt",iteratorStruct->fsmName_,
				WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
				WebUsers::DEFAULT_ITERATOR_USERNAME,
				iteratorStruct->fsmCommandParameters_);
	else
		errorStr = "Can only Configure from the Initial or Halted state. The current state is " +
			currentState;



	if(errorStr != "")
	{
		__SS__ << "Iterator failed to configure with system alias '" <<
				(iteratorStruct->fsmCommandParameters_.size()?
						iteratorStruct->fsmCommandParameters_[0]:"UNKNOWN") <<
				"' because of the following error: " << errorStr;
		throw std::runtime_error(ss.str());
	}

	//else successfully launched
	__COUT__ << "FSM in transition = " << iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.isInTransition() << __E__;
	__COUT__ << "startCommandConfigureAlias success." << __E__;
}

//========================================================================================================================
void Iterator::startCommandModifyActive(IteratorWorkLoopStruct *iteratorStruct)
{
	//Steps:
	//	4 parameters commandModifyActiveParams_:
	//		const std::string DoTrackGroupChanges_  TrueFalse
	//		//targets
	//		const std::string RelativePathToField_ 		= "RelativePathToField";
	//		const std::string FieldStartValue_ 			= "FieldStartValue";
	//		const std::string FieldIterationStepSize_ 	= "FieldIterationStepSize";
	//
	//	if tracking changes,
	//		create a new group
	//		for every enabled FE
	//			set field = start value + stepSize * currentStepIndex_
	//		activate group
	//	else
	//		load scratch group
	//		for every enabled FE
	//			set field = start value + stepSize * stepIndex
	//		activate group

	bool doTrackGroupChanges = false;
	if("True" == iteratorStruct->commands_[iteratorStruct->commandIndex_].params_
			[IterateConfiguration::commandModifyActiveParams_.DoTrackGroupChanges_])
		doTrackGroupChanges = true;

	const std::string& startValueStr =
			iteratorStruct->commands_[iteratorStruct->commandIndex_].params_
			[IterateConfiguration::commandModifyActiveParams_.FieldStartValue_];
	const std::string& stepSizeStr =
			iteratorStruct->commands_[iteratorStruct->commandIndex_].params_
			[IterateConfiguration::commandModifyActiveParams_.FieldIterationStepSize_];

	const unsigned int stepIndex = iteratorStruct->stepIndexStack_.back();


	__COUT__ << "doTrackGroupChanges " << (doTrackGroupChanges?"yes":"no") << std::endl;
	__COUT__ << "stepIndex " << stepIndex << std::endl;

	ConfigurationInterface::setVersionTrackingEnabled(doTrackGroupChanges);

	//two approaches: double or long handling

	if(startValueStr.size() &&
			(startValueStr[startValueStr.size()-1] == 'f' ||
					startValueStr.find('.') != std::string::npos))
	{
		//handle as double
		double startValue = strtod(startValueStr.c_str(),0);
		double stepSize = strtod(stepSizeStr.c_str(),0);

		__COUT__ << "startValue " << startValue << std::endl;
		__COUT__ << "stepSize " << stepSize << std::endl;
		__COUT__ << "currentValue " << startValue + stepSize*stepIndex << std::endl;

		helpCommandModifyActive(
				iteratorStruct,
				startValue + stepSize*stepIndex,
				doTrackGroupChanges);
	}
	else //handle as long
	{
		long int startValue;

		if(startValueStr.size() > 2 && startValueStr[1] == 'x') //assume hex value
			startValue = strtol(startValueStr.c_str(),0,16);
		else if(startValueStr.size() > 1 && startValueStr[0] == 'b') //assume binary value
			startValue = strtol(startValueStr.substr(1).c_str(),0,2); //skip first 'b' character
		else
			startValue = strtol(startValueStr.c_str(),0,10);

		long int stepSize;

		if(stepSizeStr.size() > 2 && stepSizeStr[1] == 'x') //assume hex value
			stepSize = strtol(stepSizeStr.c_str(),0,16);
		else if(stepSizeStr.size() > 1 && stepSizeStr[0] == 'b') //assume binary value
			stepSize = strtol(stepSizeStr.substr(1).c_str(),0,2); //skip first 'b' character
		else
			stepSize = strtol(stepSizeStr.c_str(),0,10);

		__COUT__ << "startValue " << startValue << std::endl;
		__COUT__ << "stepSize " << stepSize << std::endl;
		__COUT__ << "currentValue " << startValue + stepSize*stepIndex << std::endl;

		helpCommandModifyActive(
				iteratorStruct,
				startValue + stepSize*stepIndex,
				doTrackGroupChanges);
	}

}


//========================================================================================================================
//checkCommandRun
//	return true if done
//
//	Either will be done on (priority 1) running threads (for Frontends) ending
//		or (priority 2 and ignored if <= 0) duration timeout
//
//	Note: use command structure strings to maintain duration left
//	Note: watch iterator->doPauseAction and iterator->doHaltAction and respond
bool Iterator::checkCommandRun(IteratorWorkLoopStruct *iteratorStruct)
{
	sleep(1); //sleep to give FSM time to transition

	//all RunControlStateMachine access commands should be mutually exclusive with Supervisor main thread state machine accesses
	//should be mutually exclusive with Supervisor main thread state machine accesses
	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	if(iteratorStruct->theIterator_->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for FSM access" << __E__;
	std::lock_guard<std::mutex> lock(iteratorStruct->theIterator_->theSupervisor_->stateMachineAccessMutex_);
	if(iteratorStruct->theIterator_->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have FSM access" << __E__;

	if(iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.isInTransition())
		return false;

	iteratorStruct->fsmCommandParameters_.clear();

	std::string errorStr = "";
	std::string currentState = iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getCurrentStateName();


	/////////////////////
	//check for imposed actions and forced exits
	if(iteratorStruct->doPauseAction_)
	{
		//transition to pause state
		__COUT__ << "Transitioning FSM to Paused..." << __E__;

		if(currentState == "Paused")
		{
			//done with early pause exit!
			__COUT__ << "Transition to Paused complete." << __E__;
			return true;
		}
		else if(currentState == "Running") //launch transition to pause
			errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
					0,0,
					"Pause",iteratorStruct->fsmName_,
					WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
					WebUsers::DEFAULT_ITERATOR_USERNAME,
					iteratorStruct->fsmCommandParameters_);
		else
			errorStr = "Expected to be in Paused. Unexpectedly, the current state is " +
				currentState + ". Last State Machine error message was as follows: " +
				iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getErrorMessage();

		if(errorStr != "")
		{
			__SS__ << "Iterator failed to pause because of the following error: " << errorStr;
			throw std::runtime_error(ss.str());
		}
		return false;
	}
	else if(iteratorStruct->doHaltAction_)
	{
		//transition to halted state
		__COUT__ << "Transitioning FSM to Halted..." << __E__;

		if(currentState == "Halted")
		{
			//done with early halt exit!
			__COUT__ << "Transition to Halted complete." << __E__;
			return true;
		}
		else if(currentState == "Running" ||  //launch transition to halt
				currentState == "Paused")
			errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
					0,0,
					"Abort",iteratorStruct->fsmName_,
					WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
					WebUsers::DEFAULT_ITERATOR_USERNAME,
					iteratorStruct->fsmCommandParameters_);
		else
			errorStr = "Expected to be in Halted. Unexpectedly, the current state is " +
				currentState + ". Last State Machine error message was as follows: " +
				iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getErrorMessage();

		if(errorStr != "")
		{
			__SS__ << "Iterator failed to halt because of the following error: " << errorStr;
			throw std::runtime_error(ss.str());
		}
		return false;
	}
	else if(iteratorStruct->doResumeAction_)
	{
		//Note: check command gets one shot to resume

		//transition to running state
		__COUT__ << "Transitioning FSM to Running..." << __E__;

		if(currentState == "Paused")  //launch transition to running
			errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
					0,0,
					"Resume",iteratorStruct->fsmName_,
					WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
					WebUsers::DEFAULT_ITERATOR_USERNAME,
					iteratorStruct->fsmCommandParameters_);

		if(errorStr != "")
		{
			__SS__ << "Iterator failed to run because of the following error: " << errorStr;
			throw std::runtime_error(ss.str());
		}
		return false;
	}



	/////////////////////
	//normal running

	if(currentState != "Running")
	{
		if(iteratorStruct->runIsDone_ &&
				currentState == "Configured")
		{
			//indication of done
			__COUT__ << "Reached end of run " <<
					iteratorStruct->fsmNextRunNumber_ << __E__;
			return true;
		}

		errorStr = "Expected to be in Running. Unexpectedly, the current state is " +
			currentState + ". Last State Machine error message was as follows: " +
			iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getErrorMessage();
	}
	else //else in Running state! Check for end of run
	{
		bool waitOnRunningThreads = false;
		if("True" == iteratorStruct->commands_[iteratorStruct->commandIndex_].params_
				[IterateConfiguration::commandRunParams_.WaitOnRunningThreads_])
			waitOnRunningThreads = true;

		time_t remainingDurationInSeconds; //parameter converted during start to the stop linux timestamp
		sscanf(iteratorStruct->commands_[iteratorStruct->commandIndex_].params_
				[IterateConfiguration::commandRunParams_.DurationInSeconds_].c_str(),
				"%ld",&remainingDurationInSeconds);


		__COUT__ << "waitOnRunningThreads " << waitOnRunningThreads << __E__;
		__COUT__ << "remainingDurationInSeconds " << remainingDurationInSeconds << __E__;

		///////////////////
		//priority 1 is waiting on running threads
		if(waitOnRunningThreads)
		{
			//	get status of all running FE workloops
			Supervisor* theSupervisor =
					iteratorStruct->theIterator_->theSupervisor_;

			bool allFrontEndsAreDone = true;
			for (auto& it :
					theSupervisor->theSupervisorDescriptorInfo_.getFEDescriptors())
			{
				try
				{
					std::string status = theSupervisor->send(it.second,
							"WorkLoopStatusRequest");

					__COUT__ << "FESupervisor instance " << it.first <<
							" has status = " << status << std::endl;

					if(status != CoreSupervisorBase::WORK_LOOP_DONE)
					{
						allFrontEndsAreDone = false;
						break;
					}
				}
				catch (xdaq::exception::Exception& e)
				{
					__SS__ << "Could not retrieve status from FESupervisor instance " <<
							it.first << "." << std::endl;
					__COUT_WARN__ << ss.str();
					errorStr = ss.str();

					if(errorStr != "")
					{
						__SS__ << "Iterator failed to run because of the following error: " << errorStr;
						throw std::runtime_error(ss.str());
					}
				}
			}

			if(allFrontEndsAreDone)
			{
				//need to end run!
				__COUT__ << "FE workloops all complete! Stopping run..." << __E__;

				errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
						0,0,
						"Stop",iteratorStruct->fsmName_,
						WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
						WebUsers::DEFAULT_ITERATOR_USERNAME,
						iteratorStruct->fsmCommandParameters_);

				if(errorStr != "")
				{
					__SS__ << "Iterator failed to stop run because of the following error: " << errorStr;
					throw std::runtime_error(ss.str());
				}

				//write indication of run done into duration
				iteratorStruct->runIsDone_ = true;

				return false;
			}

		}

		///////////////////
		//priority 2 is duration, if <= 0 it is ignored
		if(remainingDurationInSeconds > 1)
		{
			--remainingDurationInSeconds;

			//write back to string
			char str[200];
			sprintf(str,"%ld",remainingDurationInSeconds);
			iteratorStruct->commands_[iteratorStruct->commandIndex_].params_
				[IterateConfiguration::commandRunParams_.DurationInSeconds_] =
					str; //re-store as string
		}
		else if(remainingDurationInSeconds == 1)
		{
			//need to end run!
			__COUT__ << "Time duration reached! Stopping run..." << __E__;

			errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
					0,0,
					"Stop",iteratorStruct->fsmName_,
					WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
					WebUsers::DEFAULT_ITERATOR_USERNAME,
					iteratorStruct->fsmCommandParameters_);

			if(errorStr != "")
			{
				__SS__ << "Iterator failed to stop run because of the following error: " << errorStr;
				throw std::runtime_error(ss.str());
			}

			//write indication of run done into duration
			iteratorStruct->runIsDone_ = true;

			return false;
		}
	}

	if(errorStr != "")
	{
		__SS__ << "Iterator failed to run because of the following error: " << errorStr;
		throw std::runtime_error(ss.str());
	}
	return false;
}

//========================================================================================================================
//return true if done
bool Iterator::checkCommandConfigure(IteratorWorkLoopStruct *iteratorStruct)
{
	sleep(1); //sleep to give FSM time to transition

	//all RunControlStateMachine access commands should be mutually exclusive with Supervisor main thread state machine accesses
	//should be mutually exclusive with Supervisor main thread state machine accesses
	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	if(iteratorStruct->theIterator_->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for FSM access" << __E__;
	std::lock_guard<std::mutex> lock(iteratorStruct->theIterator_->theSupervisor_->stateMachineAccessMutex_);
	if(iteratorStruct->theIterator_->theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have FSM access" << __E__;

	if(iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.isInTransition())
		return false;

	std::string errorStr = "";
	std::string currentState = iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getCurrentStateName();

	if(currentState == "Halted")
		errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(
				0,0,
				"Configure",iteratorStruct->fsmName_,
				WebUsers::DEFAULT_ITERATOR_USERNAME /*fsmWindowName*/,
				WebUsers::DEFAULT_ITERATOR_USERNAME,
				iteratorStruct->fsmCommandParameters_);
	else if(currentState != "Configured")
		errorStr = "Expected to be in Configure. Unexpectedly, the current state is " +
			currentState + "."  + ". Last State Machine error message was as follows: " +
			iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getErrorMessage();
	else //else successfully done (in Configured state!)
	{
		__COUT__ << "checkCommandConfigureAlias complete." << __E__;
		return true;
	}

	if(errorStr != "")
	{
		__SS__ << "Iterator failed to configure with system alias '" <<
				(iteratorStruct->fsmCommandParameters_.size()?
						iteratorStruct->fsmCommandParameters_[0]:"UNKNOWN") <<
				"' because of the following error: " << errorStr;
		throw std::runtime_error(ss.str());
	}
	return false;
}


//========================================================================================================================
bool Iterator::handleCommandRequest(HttpXmlDocument& xmldoc,
		const std::string& command, const std::string& parameter)
{
	if(command == "iteratePlay")
	{
		playIterationPlan(xmldoc,parameter);
		return true;
	}
	else if(command == "iteratePause")
	{
		pauseIterationPlan(xmldoc);
		return true;
	}
	else if(command == "iterateHalt")
	{
		haltIterationPlan(xmldoc);
		return true;
	}
	else if(command == "getIterationPlanStatus")
	{
		getIterationPlanStatus(xmldoc);
		return true;
	}
	else //return true if iterator has control of state machine
	{
		//lockout the messages array for the remainder of the scope
		//this guarantees the reading thread can safely access the messages
		if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
		std::lock_guard<std::mutex> lock(accessMutex_);
		if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

		if(iteratorBusy_)
		{
			__SS__ << "Error - Can not accept request because the Iterator " <<
					"is currently " <<
					"in control of State Machine progress. ";
			__COUT_ERR__ << "\n" << ss.str();
			__MOUT_ERR__ << "\n" << ss.str();

			xmldoc.addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
			xmldoc.addTextElementToData("state_tranisition_attempted_err",
					ss.str()); //indicate to GUI transition NOT attempted

			return true; //to block other commands
		}
	}
	return false;
}

//========================================================================================================================
void Iterator::playIterationPlan(HttpXmlDocument& xmldoc, const std::string& planName)
{
	__MOUT__ << "Attempting to play iteration plan '" << planName << ".'" << __E__;
	__COUT__ << "Attempting to play iteration plan '" << planName << ".'" << __E__;



	//setup "play" command

	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
	std::lock_guard<std::mutex> lock(accessMutex_);
	if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

	if(!activePlanIsRunning_ && !commandPlay_)
	{
		if(!workloopRunning_)
		{
			//start thread with member variables initialized

			workloopRunning_ = true;

			//must start thread first
			std::thread([](Iterator *iterator){ Iterator::IteratorWorkLoop(iterator); },this).detach();
		}

		activePlanName_ = planName;
		commandPlay_ = true;
	}
	else
	{
		__SS__ << "Invalid play command attempted. Can only play when the Iterator is inactive or paused." <<
				" If you would like to restart an iteration plan, first try halting." << __E__;
		__MOUT__ << ss.str();

		xmldoc.addTextElementToData("error_message", ss.str());

		__COUT__ << "Invalid play command attempted. " <<
				commandPlay_ << " " <<
				activePlanName_ << __E__;
	}
}

//========================================================================================================================
void Iterator::pauseIterationPlan(HttpXmlDocument& xmldoc)
{
	__MOUT__ << "Attempting to pause iteration plan '" << activePlanName_ << ".'" << __E__;
	__COUT__ << "Attempting to pause iteration plan '" << activePlanName_ << ".'" << __E__;

	//setup "pause" command

	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
	std::lock_guard<std::mutex> lock(accessMutex_);
	if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

	if(workloopRunning_ && activePlanIsRunning_ && !commandPause_)
	{
		commandPause_ = true;
	}
	else
	{
		__SS__ << "Invalid pause command attempted. Can only pause when running." << __E__;
		__MOUT__ << ss.str();

		xmldoc.addTextElementToData("error_message", ss.str());

		__COUT__ << "Invalid pause command attempted. " <<
				workloopRunning_ << " " <<
				activePlanIsRunning_ << " " <<
				commandPause_ << " " <<
				activePlanName_ << __E__;
	}
}

//========================================================================================================================
void Iterator::haltIterationPlan(HttpXmlDocument& xmldoc)
{
	__MOUT__ << "Attempting to halt iteration plan '" << activePlanName_ << ".'" << __E__;
	__COUT__ << "Attempting to halt iteration plan '" << activePlanName_ << ".'" << __E__;

	//setup "halt" command

	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
	std::lock_guard<std::mutex> lock(accessMutex_);
	if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

	if(activePlanIsRunning_ && !commandHalt_)
	{
		if(workloopRunning_)
		{
			__COUT__ << "Passing halt command to iterator thread." << __E__;
			commandHalt_ = true;
		}
		else //no thread, so reset 'Error' without command to thread
		{
			__COUT__ << "No thread, so conducting halt." << __E__;
			activePlanIsRunning_ = false;
			iteratorBusy_ = false;

			try
			{
				Iterator::haltStateMachine(theSupervisor_, lastFsmName_);
			}
			catch(const std::runtime_error& e)
			{
				xmldoc.addTextElementToData("error_message", e.what());
			}
		}
	}
	else
	{
		__COUT__ << "No thread, so conducting halt." << __E__;

		bool haltAttempted = false;
		try
		{
			haltAttempted = Iterator::haltStateMachine(theSupervisor_, lastFsmName_);
		}
		catch(const std::runtime_error& e)
		{
			haltAttempted = false;
		}

		if(!haltAttempted) //then show error
		{
			__SS__ << "Invalid halt command attempted. Can only halt when there is an active iteration plan." << __E__;
			__MOUT__ << ss.str();

			xmldoc.addTextElementToData("error_message", ss.str());

			__COUT__ << "Invalid halt command attempted. " <<
					workloopRunning_ << " " <<
					activePlanIsRunning_ << " " <<
					commandHalt_ << " " <<
					activePlanName_ << __E__;
		}
		else
			__COUT__ << "Halt was attempted." << __E__;
	}
}

//========================================================================================================================
//	return state machine and iterator status
void Iterator::getIterationPlanStatus(HttpXmlDocument& xmldoc)
{
	xmldoc.addTextElementToData("current_state", theSupervisor_->theStateMachine_.getCurrentStateName());
	xmldoc.addTextElementToData("in_transition", theSupervisor_->theStateMachine_.isInTransition() ? "1" : "0");
	if(theSupervisor_->theStateMachine_.isInTransition())
		xmldoc.addTextElementToData("transition_progress",
				theSupervisor_->theProgressBar_.readPercentageString());
	else
		xmldoc.addTextElementToData("transition_progress", "100");

	char tmp[20];
	sprintf(tmp,"%lu",theSupervisor_->theStateMachine_.getTimeInState());
	xmldoc.addTextElementToData("time_in_state", tmp);



	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Waiting for iterator access" << __E__;
	std::lock_guard<std::mutex> lock(accessMutex_);
	if(theSupervisor_->VERBOSE_MUTEX) __COUT__ << "Have iterator access" << __E__;

	xmldoc.addTextElementToData("active_plan", activePlanName_);
	xmldoc.addTextElementToData("last_started_plan", lastStartedPlanName_);
	xmldoc.addTextElementToData("last_finished_plan", lastFinishedPlanName_);

	sprintf(tmp,"%u",activeCommandIndex_);
	xmldoc.addTextElementToData("current_command_index", tmp);
	sprintf(tmp,"%ld",time(0) - activeCommandStartTime_);
	xmldoc.addTextElementToData("current_command_duration", tmp);

	if(activePlanIsRunning_ && iteratorBusy_)
	{
		if(workloopRunning_)
			xmldoc.addTextElementToData("active_plan_status", "Running");
		else
			xmldoc.addTextElementToData("active_plan_status", "Error");
	}
	else if(!activePlanIsRunning_ && iteratorBusy_)
		xmldoc.addTextElementToData("active_plan_status", "Paused");
	else
		xmldoc.addTextElementToData("active_plan_status", "Inactive");

	xmldoc.addTextElementToData("error_message", errorMessage_);
}


