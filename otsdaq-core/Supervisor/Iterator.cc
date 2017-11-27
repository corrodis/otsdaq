#include "otsdaq-core/Supervisor/Iterator.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/Supervisor/Supervisor.h"
#include "otsdaq-core/WebUsersUtilities/WebUsers.h"

#include "otsdaq-core/ConfigurationInterface/ConfigurationManagerRW.h"


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
	__MOUT__ << "Iterator constructed." << std::endl;
	__COUT__ << "Iterator constructed." << std::endl;

}

//========================================================================================================================
Iterator::~Iterator(void)
{
}

//========================================================================================================================
void Iterator::IteratorWorkLoop(Iterator *iterator)
try
{
	__MOUT__ << "Iterator work loop starting..." << std::endl;
	__COUT__ << "Iterator work loop starting..." << std::endl;

	//mutex init scope
	{
		std::lock_guard<std::mutex> lock(iterator->accessMutex_);
		iterator->errorMessage_ = ""; //clear error message
	}


	ConfigurationManagerRW theConfigurationManager(WebUsers::DEFAULT_ITERATOR_USERNAME); //this is a restricted username
	IteratorWorkLoopStruct theIteratorStruct(iterator,
			&theConfigurationManager);


	const IterateConfiguration* itConfig;

	std::vector<IterateConfiguration::Command> commands;
	unsigned int commandIndex = (unsigned int)-1;
	//std::string activePlan = "";
	//bool running = false;
	//bool commandBusy = false;

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
			std::lock_guard<std::mutex> lock(iterator->accessMutex_);

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
								" vs new=" << iterator->activePlanName_ << std::endl;
						theIteratorStruct.commandIndex_ = -1; //reset for new plan
					}

					theIteratorStruct.activePlan_ = iterator->activePlanName_;
					iterator->lastStartedPlanName_ = iterator->activePlanName_;

					if(commandIndex == (unsigned int)-1)
					{
						__COUT__ << "Starting plan '" << theIteratorStruct.activePlan_ << ".'" << std::endl;
						__MOUT__ << "Starting plan '" << theIteratorStruct.activePlan_ << ".'" << std::endl;
					}
					else
					{
						__COUT__ << "Continuing plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
								theIteratorStruct.commandIndex_ << ". " << std::endl;
						__MOUT__ << "Continuing plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
								theIteratorStruct.commandIndex_ << ". " << std::endl;
					}
				}
			}
			else if(iterator->commandPause_)
			{
				iterator->commandPause_ = false; //clear

				if(iterator->activePlanIsRunning_)
				{
					//valid PAUSE command!

					//safely pause plan!
					//	i.e. check that command is complete

					while(!iterator->checkCommand(&theIteratorStruct))
						__COUT__ << "Waiting to pause..." << std::endl;

					iterator->activePlanIsRunning_ = false;

					__COUT__ << "Paused plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
							theIteratorStruct.commandIndex_ << ". " << std::endl;
					__MOUT__ << "Paused plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
							theIteratorStruct.commandIndex_ << ". " << std::endl;
				}
			}
			else if(iterator->commandHalt_)
			{
				iterator->commandHalt_ = false; //clear

				//safely end plan!
				//	i.e. check that command is complete

				while(!iterator->checkCommand(&theIteratorStruct))
					__COUT__ << "Waiting to halt..." << std::endl;

				//valid HALT command!

				iterator->activePlanIsRunning_ = false;
				iterator->iteratorBusy_ = false;

				__COUT__ << "Halted plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
						theIteratorStruct.commandIndex_ << ". " << std::endl;
				__MOUT__ << "Halted plan '" << theIteratorStruct.activePlan_ << "' at command index " <<
						theIteratorStruct.commandIndex_ << ". " << std::endl;

				theIteratorStruct.activePlan_ = ""; //clear
				theIteratorStruct.commandIndex_ = -1; //clear
			}

			theIteratorStruct.running_ = iterator->activePlanIsRunning_;

			if(commandIndex != iterator->activeCommandIndex_)
			{
				iterator->activeCommandIndex_ = theIteratorStruct.commandIndex_;
				iterator->activeCommandStartTime_ = time(0); //reset on any change
			}

		} //end command handling and mutex


		////////////////
		////////////////
		//	handle running
		//__COUT__ << "thinking.." << running << " " << activePlan << " cmd=" <<
		//		commandIndex << std::endl;
		if(theIteratorStruct.running_ &&
				theIteratorStruct.activePlan_ != "") //important, because after errors, still "running" until halt
		{
			if(theIteratorStruct.commandIndex_ == (unsigned int)-1)
			{
				__COUT__ << "Get commands" << std::endl;

				theIteratorStruct.commandIndex_ = 0;

				theIteratorStruct.cfgMgr_->init(); //completely reset to re-align with any changes
				itConfig = theIteratorStruct.cfgMgr_->__GET_CONFIG__(IterateConfiguration);

				theIteratorStruct.commands_ = itConfig->getPlanCommands(theIteratorStruct.cfgMgr_,
						theIteratorStruct.activePlan_);

				for(auto& command:theIteratorStruct.commands_)
				{
					__COUT__ << "command " <<
							command.type_ << std::endl;
					__COUT__ << "table " <<
							IterateConfiguration::commandToTableMap_.at(command.type_) << std::endl;
					__COUT__ << "param count = " << command.params_.size() << std::endl;

					for(auto& param:command.params_)
					{
						__COUT__ << "\t param " <<
								param.first << " : " <<
								param.second << std::endl;
					}
				}
			}

			if(!theIteratorStruct.commandBusy_)
			{
				if(theIteratorStruct.commandIndex_ < theIteratorStruct.commands_.size())
				{
					//execute command
					theIteratorStruct.commandBusy_ = true;

					__COUT__ << "Iterator starting command " << theIteratorStruct.commandIndex_+1 << ": " <<
							theIteratorStruct.commands_[theIteratorStruct.commandIndex_].type_ << std::endl;
					__MOUT__ << "Iterator starting command " << theIteratorStruct.commandIndex_+1 << ": " <<
							theIteratorStruct.commands_[theIteratorStruct.commandIndex_].type_ << std::endl;
					iterator->startCommand(&theIteratorStruct);
				}
				else if(theIteratorStruct.commandIndex_ ==
						theIteratorStruct.commands_.size()) //Done!
				{
					__COUT__ << "Finished Iteration Plan '" << theIteratorStruct.activePlan_ << std::endl;
					__MOUT__ << "Finished Iteration Plan '" << theIteratorStruct.activePlan_ << std::endl;

					//lockout the messages array for the remainder of the scope
					//this guarantees the reading thread can safely access the messages
					std::lock_guard<std::mutex> lock(iterator->accessMutex_);

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
							theIteratorStruct.commands_.size() << std::endl;
					__MOUT__ << "Iterator ready for next command. Done with " << theIteratorStruct.commandIndex_ << " of " <<
							theIteratorStruct.commands_.size() << std::endl;
				}
			}



		} 	//end running
		else
			sleep(1); //when inactive sleep a lot

		////////////////
		////////////////


	}

	iterator->workloopRunning_ = false; //if we ever exit
}
catch(const std::runtime_error &e)
{
	__SS__ << "Encountered error in Iterator thread:\n" << e.what() << std::endl;
	__COUT_ERR__ << ss.str();

	std::lock_guard<std::mutex> lock(iterator->accessMutex_);
	iterator->workloopRunning_ = false; //if we ever exit
	iterator->errorMessage_ = ss.str();

}
catch(...)
{
	__SS__ << "Encountered unknown error in Iterator thread." << std::endl;
	__COUT_ERR__ << ss.str();

		std::lock_guard<std::mutex> lock(iterator->accessMutex_);
	iterator->workloopRunning_ = false; //if we ever exit
	iterator->errorMessage_ = ss.str();
}

//========================================================================================================================
void Iterator::startCommand(IteratorWorkLoopStruct *iteratorStruct)
{
	//for simplicity assume all commands should be mutually exclusive with Supervisor main thread state machine accesses (really should just be careful with RunControlStateMachine access)
	std::lock_guard<std::mutex> lock(iteratorStruct->theIterator_->theSupervisor_->stateMachineAccessMutex_);

	//for out of range, throw exception - should never happen
	if(iteratorStruct->commandIndex_ >= iteratorStruct->commands_.size())
	{
		__SS__ << "Out of range commandIndex = " << iteratorStruct->commandIndex_ <<
				" in size = " << iteratorStruct->commands_.size() << std::endl;
		throw std::runtime_error(ss.str());
	}


	std::string type = iteratorStruct->commands_[iteratorStruct->commandIndex_].type_;
	if(type == IterateConfiguration::COMMAND_BEGIN_LABEL)
	{
		//do nothing
		return;
	}
	else if(type == IterateConfiguration::COMMAND_CHOOSE_FSM)
	{
		startCommandChooseFSM(
				iteratorStruct,
				iteratorStruct->commands_
				[iteratorStruct->commandIndex_].params_
				[IterateConfiguration::commandChooseFSMParams_.NameOfFSM_]);
		return;
	}
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_ACTIVE_GROUP)
	{
		//TODO
		return;
	}
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_ALIAS)
	{
		startCommandConfigureAlias(
				iteratorStruct,
				iteratorStruct->commands_
				[iteratorStruct->commandIndex_].params_
				[IterateConfiguration::commandConfigureAliasParams_.SystemAlias_]);
		return;
	}
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_GROUP)
	{
		//TODO
		return;
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
		//TODO
		return;
	}
	else if(type == IterateConfiguration::COMMAND_REPEAT_LABEL)
	{
		//search for first matching label backward and set command to there

		int numOfRepetitions;
		sscanf(iteratorStruct->commands_[iteratorStruct->commandIndex_].params_[
			IterateConfiguration::commandRepeatLabelParams_.NumberOfRepetitions_].c_str(),
				"%d",&numOfRepetitions);
		__COUT__ << "numOfRepetitions remaining = " << numOfRepetitions << std::endl;

		if(numOfRepetitions <= 0) return; //no more repetitions

		--numOfRepetitions;

		unsigned int i;
		for(i=iteratorStruct->commandIndex_;i>0;--i) //assume 0 is always the fallback option
			if(iteratorStruct->commands_[i].type_ == IterateConfiguration::COMMAND_BEGIN_LABEL &&
					iteratorStruct->commands_[iteratorStruct->commandIndex_].params_[IterateConfiguration::commandRepeatLabelParams_.Label_] ==
							iteratorStruct->commands_[i].params_[IterateConfiguration::commandBeginLabelParams_.Label_]) break;

		char repStr[200];
		sprintf(repStr,"%d",numOfRepetitions);
		iteratorStruct->commands_[iteratorStruct->commandIndex_].params_[
			IterateConfiguration::commandRepeatLabelParams_.NumberOfRepetitions_] =
					repStr; //re-store as string

		iteratorStruct->commandIndex_ = i;
		__COUT__ << "Jumping back to commandIndex " << iteratorStruct->commandIndex_ << std::endl;

		return;
	}
	else if(type == IterateConfiguration::COMMAND_RUN)
	{
		//TODO
		return;
	}
	else
	{
		__SS__ << "Attempt to start unrecognized command type = " << type << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
}

//========================================================================================================================
//checkCommand
//	when busy for a while, start to sleep
//		use sleep() or nanosleep()
bool Iterator::checkCommand(IteratorWorkLoopStruct *iteratorStruct)
{
	//for simplicity assume all commands should be mutually exclusive with Supervisor main thread state machine accesses (really should just be careful with RunControlStateMachine access)
	std::lock_guard<std::mutex> lock(iteratorStruct->theIterator_->theSupervisor_->stateMachineAccessMutex_);

	//for out of range, return done
	if(iteratorStruct->commandIndex_ >= iteratorStruct->commands_.size())
	{
		__COUT__ << "Out of range commandIndex = " << iteratorStruct->commandIndex_ <<
				" in size = " << iteratorStruct->commands_.size() << std::endl;
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
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_ACTIVE_GROUP)
	{
		//do nothing
		return true;
	}
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_ALIAS)
	{
		//do nothing
		return true;
	}
	else if(type == IterateConfiguration::COMMAND_CONFIGURE_GROUP)
	{
		//do nothing
		return true;
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
		//do nothing
		return true;
	}
	else
	{
		__SS__ << "Attempt to check unrecognized command type = " << type << std::endl;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
	}
}

//========================================================================================================================
void Iterator::startCommandChooseFSM(IteratorWorkLoopStruct *iteratorStruct, std::string fsmName)
{
	__COUT__ << "fsmName " << fsmName << std::endl;


	iteratorStruct->fsmName_ = fsmName;

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
				__COUT_INFO__ << "FSM Link disconnected." << std::endl;
		}
		catch(std::runtime_error &e)
		{
			//__COUT_INFO__ << e.what() << std::endl;
			__COUT_INFO__ << "No state machine Run alias. Ignoring and assuming alias of '" <<
					iteratorStruct->fsmRunAlias_  << ".'" << std::endl;

		}
		catch(...) {
			__COUT_ERR__ << "Unknown error. Should never happen." << std::endl;

			__COUT_INFO__ << "No state machine Run alias. Ignoring and assuming alias of '" <<
					iteratorStruct->fsmRunAlias_  << ".'" << std::endl;
		}
	}
	else
		__COUT_INFO__ << "FSM Link disconnected." << std::endl;

	__COUT__ << "fsmRunAlias_  = " << iteratorStruct->fsmRunAlias_ 	<< std::endl;




	//// ======================== get run number based on fsm name ====

	iteratorStruct->fsmNextRunNumber_  = iteratorStruct->theIterator_->theSupervisor_->getNextRunNumber(
			iteratorStruct->fsmName_);

	if(iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getCurrentStateName() == "Running" ||
			iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.getCurrentStateName() == "Paused")
		--iteratorStruct->fsmNextRunNumber_; //current run number is one back

	__COUT__ << "fsmNextRunNumber_  = " << iteratorStruct->fsmNextRunNumber_ << std::endl;
}

//========================================================================================================================
void Iterator::startCommandConfigureAlias(IteratorWorkLoopStruct *iteratorStruct, std::string systemAlias)
{
	__COUT__ << "systemAlias " << systemAlias << std::endl;

	std::vector<std::string> commandParameters;
	commandParameters.push_back(systemAlias);

	std::string errorStr = iteratorStruct->theIterator_->theSupervisor_->attemptStateMachineTransition(0,0,
			"Configure",iteratorStruct->fsmName_,
			"" /*fsmWindowName*/,
			WebUsers::DEFAULT_ITERATOR_USERNAME,
			commandParameters);

	if(errorStr != "")
	{
		__SS__ << "Iterator failed to configure with system alias '" << systemAlias <<
				"' because of the following error: " << errorStr;
		throw std::runtime_error(ss.str());
	}

	//else successfully launched
	__COUT__ << "FSM in transition = " << iteratorStruct->theIterator_->theSupervisor_->theStateMachine_.isInTransition() << std::endl;
	__COUT__ << "startCommandConfigureAlias success." << std::endl;
}

//========================================================================================================================
//return true if done
bool Iterator::checkCommandConfigureAlias(IteratorWorkLoopStruct *iteratorStruct)
{
	return true;
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
		std::lock_guard<std::mutex> lock(accessMutex_);
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
	__MOUT__ << "Attempting to playing iteration plan '" << planName << ".'" << std::endl;
	__COUT__ << "Attempting to playing iteration plan '" << planName << ".'" << std::endl;

	if(!workloopRunning_)
	{
		//start thread with member variables initialized

		workloopRunning_ = true;

		//must start thread first
		std::thread([](Iterator *iterator){ Iterator::IteratorWorkLoop(iterator); },this).detach();
	}

	//setup "play" command

	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	std::lock_guard<std::mutex> lock(accessMutex_);
	if(workloopRunning_ && !activePlanIsRunning_ && !commandPlay_)
	{
		activePlanName_ = planName;
		commandPlay_ = true;
	}
	else
	{
		__SS__ << "Invalid play command attempted. Can only play when the Iterator is inactive or paused." << std::endl;
		__MOUT__ << ss.str();

		xmldoc.addTextElementToData("error_message", ss.str());

		__COUT__ << "Invalid play command attempted. " <<
				commandPlay_ << " " <<
				activePlanName_ << std::endl;
	}
}

//========================================================================================================================
void Iterator::pauseIterationPlan(HttpXmlDocument& xmldoc)
{
	__MOUT__ << "Attempting to pause iteration plan '" << activePlanName_ << ".'" << std::endl;
	__COUT__ << "Attempting to pause iteration plan '" << activePlanName_ << ".'" << std::endl;

	//setup "pause" command

	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	std::lock_guard<std::mutex> lock(accessMutex_);
	if(workloopRunning_ && activePlanIsRunning_ && !commandPause_)
	{
		commandPause_ = true;
	}
	else
	{
		__MOUT__ << "Invalid pause command attempted." << std::endl;
		__COUT__ << "Invalid pause command attempted. " <<
				workloopRunning_ << " " <<
				activePlanIsRunning_ << " " <<
				commandPause_ << " " <<
				activePlanName_ << std::endl;
	}
}

//========================================================================================================================
void Iterator::haltIterationPlan(HttpXmlDocument& xmldoc)
{
	__MOUT__ << "Attempting to halt iteration plan '" << activePlanName_ << ".'" << std::endl;
	__COUT__ << "Attempting to halt iteration plan '" << activePlanName_ << ".'" << std::endl;

	//setup "halt" command

	//lockout the messages array for the remainder of the scope
	//this guarantees the reading thread can safely access the messages
	std::lock_guard<std::mutex> lock(accessMutex_);
	if(workloopRunning_ && activePlanIsRunning_ && !commandHalt_)
	{
		commandHalt_ = true;
	}
	else
	{
		__MOUT__ << "Invalid halt command attempted." << std::endl;
		__COUT__ << "Invalid halt command attempted. " <<
				workloopRunning_ << " " <<
				activePlanIsRunning_ << " " <<
				commandHalt_ << " " <<
				activePlanName_ << std::endl;
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
	std::lock_guard<std::mutex> lock(accessMutex_);

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


