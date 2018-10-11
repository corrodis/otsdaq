#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/Macros/StringMacros.h"

#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"

#include <toolbox/fsm/FailedEvent.h>
#include <xoap/Method.h>
#include <xdaq/NamespaceURI.h>

#include <iostream>

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ std::string("FSM-") + stateMachineName_


using namespace ots;

const std::string RunControlStateMachine::FAILED_STATE_NAME = "Failed";

//========================================================================================================================
RunControlStateMachine::RunControlStateMachine(std::string name)
: stateMachineName_(name)
{
	INIT_MF("RunControlStateMachine");

	theStateMachine_.addState('I', "Initial",     this, &RunControlStateMachine::stateInitial);
	theStateMachine_.addState('H', "Halted",      this, &RunControlStateMachine::stateHalted);
	theStateMachine_.addState('C', "Configured",  this, &RunControlStateMachine::stateConfigured);
	theStateMachine_.addState('R', "Running",     this, &RunControlStateMachine::stateRunning);
	theStateMachine_.addState('P', "Paused",      this, &RunControlStateMachine::statePaused);
	theStateMachine_.addState('X', "Shutdown",    this, &RunControlStateMachine::stateShutdown);
	//theStateMachine_.addState('v', "Recovering",  this, &RunControlStateMachine::stateRecovering);
	//theStateMachine_.addState('T', "TTSTestMode", this, &RunControlStateMachine::stateTTSTestMode);

	//RAR added back in on 11/20/2016.. why was it removed..
	//exceptions like..
	//	XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());)
	//	take state machine to "failed" otherwise
	theStateMachine_.setStateName('F',RunControlStateMachine::FAILED_STATE_NAME);//x
	theStateMachine_.setFailedStateTransitionAction (this, &RunControlStateMachine::enteringError);
	theStateMachine_.setFailedStateTransitionChanged(this, &RunControlStateMachine::inError);

	//this line was added to get out of Failed state
	RunControlStateMachine::addStateTransition('F', 'H', "Halt"	  , "Halting"	   , this, &RunControlStateMachine::transitionHalting);
	RunControlStateMachine::addStateTransition('F', 'X', "Shutdown"  , "Shutting Down", this, &RunControlStateMachine::transitionShuttingDown);


	RunControlStateMachine::addStateTransition('H', 'C', "Configure" , "Configuring"  , "ConfigurationAlias", this, &RunControlStateMachine::transitionConfiguring);
	RunControlStateMachine::addStateTransition('H', 'X', "Shutdown"  , "Shutting Down", this, &RunControlStateMachine::transitionShuttingDown);
	RunControlStateMachine::addStateTransition('X', 'I', "Startup"   , "Starting Up"  , this, &RunControlStateMachine::transitionStartingUp);

	//Every state can transition to halted
	RunControlStateMachine::addStateTransition('I', 'H', "Initialize", "Initializing" , this, &RunControlStateMachine::transitionInitializing);
	RunControlStateMachine::addStateTransition('H', 'H', "Halt"      , "Halting"      , this, &RunControlStateMachine::transitionHalting);
	RunControlStateMachine::addStateTransition('C', 'H', "Halt"      , "Halting"      , this, &RunControlStateMachine::transitionHalting);
	RunControlStateMachine::addStateTransition('R', 'H', "Abort"     , "Aborting"     , this, &RunControlStateMachine::transitionHalting);
	RunControlStateMachine::addStateTransition('P', 'H', "Abort"     , "Aborting"     , this, &RunControlStateMachine::transitionHalting);

	RunControlStateMachine::addStateTransition('R', 'P', "Pause"     , "Pausing"      , this, &RunControlStateMachine::transitionPausing);
	RunControlStateMachine::addStateTransition('P', 'R', "Resume"    , "Resuming"     , this, &RunControlStateMachine::transitionResuming);
	RunControlStateMachine::addStateTransition('C', 'R', "Start"     , "Starting"     , this, &RunControlStateMachine::transitionStarting);
	RunControlStateMachine::addStateTransition('R', 'C', "Stop"      , "Stopping"     , this, &RunControlStateMachine::transitionStopping);
	RunControlStateMachine::addStateTransition('P', 'C', "Stop"      , "Stopping"     , this, &RunControlStateMachine::transitionStopping);


	// NOTE!! There must be a defined message handler for each transition name created above
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Initialize", XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Configure" , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Start"     , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Stop"      , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Pause"     , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Resume"    , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Halt"      , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Abort"     , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Shutdown"  , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Startup"   , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Fail"      , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Error"     , XDAQ_NS_URI);


	reset();
}

//========================================================================================================================
RunControlStateMachine::~RunControlStateMachine(void)
{
}

//========================================================================================================================
void RunControlStateMachine::reset(void)
{
	__COUT__ << "Resetting RunControlStateMachine with name '" << stateMachineName_ << "'..." << __E__;
	theStateMachine_.setInitialState('I');
	theStateMachine_.reset();
}

////========================================================================================================================
//(RunControlStateMachine::stateMachineFunction_t) RunControlStateMachine::getTransitionName(
//		const toolbox::fsm::State from,
//		const std::string& transition)
//{
//	auto itFrom = stateTransitionFunctionTable_.find(from);
//	if(itFrom == stateTransitionFunctionTable_.end())
//	{
//		__SS__ <<	"Cannot find transition function from '" << from <<
//				"' with transition '" << transition << "!'" << __E__;
//		XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
//	}
//
//	auto itTrans = itFrom->second.find(transition);
//	if(itTrans == itFrom->second.end())
//	{
//		__SS__ <<	"Cannot find transition function from '" << from <<
//				"' with transition '" << transition << "!'" << __E__;
//		XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
//	}
//
//	return itTrans->second;
//}

//========================================================================================================================
//runControlMessageHandler
//	Handles the command broadcast message from the Gateway Supervisor
//	and maps the command to a transition function, allowing for multiple iteration
//	passes through the transition function.
xoap::MessageReference RunControlStateMachine::runControlMessageHandler(
		xoap::MessageReference message)

{
	__COUT__ << SOAPUtilities::translate(message) << std::endl;

	theStateMachine_.setErrorMessage(""); //clear error message

	std::string command = SOAPUtilities::translate(message).getCommand();

	//get iteration index
	try
	{
		 StringMacros::getNumber(SOAPUtilities::translate(message).getParameters().getValue(
					"iterationIndex"), iterationIndex_);
	}
	catch(...) //ignore errors and set iteration index to 0
	{
		__COUT__ << "Defaulting iteration index to 0." << __E__;
		iterationIndex_ = 0;
	}

	std::string currentState;
	if(iterationIndex_ == 0)
	{
		//this is the first iteration attempt for this transition
		theProgressBar_.reset(command,stateMachineName_);
		currentState = theStateMachine_.getCurrentStateName();
		__COUT__ << "Starting state for " << stateMachineName_ << " is " <<
				currentState << " and attempting to " << command << std::endl;
	}
	else
	{
		currentState = theStateMachine_.getStateName(lastIterationState_);

		__COUT__ << "Iteration index " << iterationIndex_ << " for " << stateMachineName_ << " from " <<
				currentState << " attempting to " << command << std::endl;
	}

	RunControlStateMachine::theProgressBar_.step();

	std::string result = command + "Done";

	//if error is received, immediately go to fail state
	//	likely error was sent by central FSM or external xoap
	if(command == "Error" || command == "Fail")
	{
		__SS__ << command << " was received! Halting immediately." << std::endl;
		__COUT_ERR__ << "\n" << ss.str();

		try
		{
			if(currentState == "Configured")
				theStateMachine_.execTransition("Halt",message);
			else if(currentState == "Running" ||
					currentState == "Paused")
				theStateMachine_.execTransition("Abort",message);
		}
		catch(...)
		{
			__COUT_ERR__ << "Halting failed in reaction to " << command <<
					"... ignoring." << __E__;
		}
		return SOAPUtilities::makeSOAPMessageReference(result);
	}


	//if already Halted, respond to Initialize with "done"
	//	(this avoids race conditions involved with artdaq mpi reset)
	if(command == "Initialize" &&
			currentState == "Halted")
	{
		__COUT__ << "Already Initialized.. ignoring Initialize command." << std::endl;
		return SOAPUtilities::makeSOAPMessageReference(result);
	}




	//handle normal transitions here
	try
	{
		stillWorking_ = false;
		if(iterationIndex_)
		{
			__COUT__ << command << " iteration " <<  iterationIndex_ << __E__;
			toolbox::Event::Reference event(new toolbox::Event(command, this));


			//call inheriting transition function based on last state and command
			{
				//e.g. transitionConfiguring(event);
				__COUT__ << "Iterating on the transition function from " << currentState <<
						" through " << lastIterationCommand_ << __E__;

				auto itFrom = stateTransitionFunctionTable_.find(lastIterationState_);
				if(itFrom == stateTransitionFunctionTable_.end())
				{
					__SS__ <<	"Cannot find transition function from '" << currentState <<
							"' with transition '" << lastIterationCommand_ << "!'" << __E__;
					__COUT_ERR__ << ss.str();
					XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
				}

				auto itTransition = itFrom->second.find(lastIterationCommand_);
				if(itTransition == itFrom->second.end())
				{
					__SS__ <<	"Cannot find transition function from '" << currentState <<
							"' with transition '" << lastIterationCommand_ << "!'" << __E__;
					__COUT_ERR__ << ss.str();
					XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
				}

				(this->*(itTransition->second))(event); //call the transition function
			}
		}
		else
		{
			//save the lookup parameters for the last function to be called for the case of additional iterations
			lastIterationState_ = theStateMachine_.getCurrentState();
			lastIterationCommand_ = command;

			theStateMachine_.execTransition(command,message);

		}

		if(stillWorking_)
		{
			__COUTV__(stillWorking_);
			result = command + "Working"; //indicate still working back to Gateway
		}
	}
	catch(toolbox::fsm::exception::Exception& e)
	{
		__SS__ << "Run Control Message Handling Failed: " << e.what() <<
				" " << theStateMachine_.getErrorMessage() << __E__;
		__COUT_ERR__ << ss.str();
		theStateMachine_.setErrorMessage(ss.str());

		result = command + " " + RunControlStateMachine::FAILED_STATE_NAME + ": " +
				theStateMachine_.getErrorMessage();
	}
	catch(...)
	{
		__SS__ << "Run Control Message Handling encountered an unknown error." <<
				theStateMachine_.getErrorMessage() << __E__;
		__COUT_ERR__ << ss.str();
		theStateMachine_.setErrorMessage(ss.str());

		result = command + " " + RunControlStateMachine::FAILED_STATE_NAME + ": " +
				theStateMachine_.getErrorMessage();
	}

	RunControlStateMachine::theProgressBar_.step();

	currentState = theStateMachine_.getCurrentStateName();

	if(currentState == RunControlStateMachine::FAILED_STATE_NAME)
	{
		result = command + " " + RunControlStateMachine::FAILED_STATE_NAME + ": " +
				theStateMachine_.getErrorMessage();
		__COUT_ERR__ << "Unexpected Failure state for " << stateMachineName_ << " is " << currentState << std::endl;
		__COUT_ERR__ << "Error message was as follows: " << theStateMachine_.getErrorMessage() << std::endl;
	}

	RunControlStateMachine::theProgressBar_.step();

	if(!stillWorking_)
		theProgressBar_.complete();

	__COUT__ << "Ending state for " << stateMachineName_ << " is " << currentState << std::endl;
	__COUT__ << "result = " << result << std::endl;
	return SOAPUtilities::makeSOAPMessageReference(result);
}

