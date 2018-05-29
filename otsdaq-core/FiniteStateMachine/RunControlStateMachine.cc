#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"
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
	theStateMachine_.addStateTransition('F', 'H', "Halt"	  , "Halting"	   , this, &RunControlStateMachine::transitionHalting);
		//this attempt to get out of fail state makes things crash FIXME
	//end RAR added back in on 11/20/2016.. why was it removed..

	theStateMachine_.addStateTransition('H', 'C', "Configure" , "Configuring"  , "ConfigurationAlias", this, &RunControlStateMachine::transitionConfiguring);
	theStateMachine_.addStateTransition('H', 'X', "Shutdown"  , "Shutting Down", this, &RunControlStateMachine::transitionShuttingDown);
	theStateMachine_.addStateTransition('X', 'I', "Startup"   , "Starting Up"  , this, &RunControlStateMachine::transitionStartingUp);

	//Every state can transition to halted
	theStateMachine_.addStateTransition('I', 'H', "Initialize", "Initializing" , this, &RunControlStateMachine::transitionInitializing);
	theStateMachine_.addStateTransition('H', 'H', "Halt"      , "Halting"      , this, &RunControlStateMachine::transitionHalting);
	theStateMachine_.addStateTransition('C', 'H', "Halt"      , "Halting"      , this, &RunControlStateMachine::transitionHalting);
	theStateMachine_.addStateTransition('R', 'H', "Abort"     , "Aborting"     , this, &RunControlStateMachine::transitionHalting);
	theStateMachine_.addStateTransition('P', 'H', "Abort"     , "Aborting"     , this, &RunControlStateMachine::transitionHalting);

	theStateMachine_.addStateTransition('R', 'P', "Pause"     , "Pausing"      , this, &RunControlStateMachine::transitionPausing);
	theStateMachine_.addStateTransition('P', 'R', "Resume"    , "Resuming"     , this, &RunControlStateMachine::transitionResuming);
	theStateMachine_.addStateTransition('C', 'R', "Start"     , "Starting"     , this, &RunControlStateMachine::transitionStarting);
	theStateMachine_.addStateTransition('R', 'C', "Stop"      , "Stopping"     , this, &RunControlStateMachine::transitionStopping);
	theStateMachine_.addStateTransition('P', 'C', "Stop"      , "Stopping"     , this, &RunControlStateMachine::transitionStopping);


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

//========================================================================================================================
xoap::MessageReference RunControlStateMachine::runControlMessageHandler(
		xoap::MessageReference message)

{
	__COUT__ << "Starting state for " << stateMachineName_ << " is " <<
			theStateMachine_.getCurrentStateName() << std::endl;
	__COUT__ << SOAPUtilities::translate(message) << std::endl;

	theStateMachine_.setErrorMessage(""); //clear error message

	std::string command = SOAPUtilities::translate(message).getCommand();
	//__COUT__ << "Command:-" << command << "-" << std::endl;
	theProgressBar_.reset(command,stateMachineName_);
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
			if(theStateMachine_.getCurrentStateName() == "Configured")
				theStateMachine_.execTransition("Halt",message);
			else if(theStateMachine_.getCurrentStateName() == "Running" ||
					theStateMachine_.getCurrentStateName() == "Paused")
				theStateMachine_.execTransition("Abort",message);
		}
		catch(...)
		{
			__COUT_ERR__ << "Halting failed in reaction to Error... ignoring." << __E__;
		}
		return SOAPUtilities::makeSOAPMessageReference(result);
	}


	//if already Halted, respond to Initialize with "done"
	//	(this avoids race conditions involved with artdaq mpi reset)
	if(command == "Initialize" &&
			theStateMachine_.getCurrentStateName() == "Halted")
	{
		__COUT__ << "Already Initialized.. ignoring Initialize command." << std::endl;
		return SOAPUtilities::makeSOAPMessageReference(result);
	}

	//handle normal transitions here
	try
	{
		theStateMachine_.execTransition(command,message);
		//__COUT__ << "I don't know what is going on!" << std::endl;

		if(theStateMachine_.getCurrentStateName() == RunControlStateMachine::FAILED_STATE_NAME)
		{
			result = command + " " + RunControlStateMachine::FAILED_STATE_NAME + ": " + theStateMachine_.getErrorMessage();
			__COUT_ERR__ << "Unexpected Failure state for " << stateMachineName_ << " is " << theStateMachine_.getCurrentStateName() << std::endl;
			__COUT_ERR__ << "Error message was as follows: " << theStateMachine_.getErrorMessage() << std::endl;
		}
	}
	catch (toolbox::fsm::exception::Exception& e)
	{
		result = command + " " + RunControlStateMachine::FAILED_STATE_NAME + ": " + theStateMachine_.getErrorMessage();
		__SS__ << "Run Control Message Handling Failed: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		__COUT_ERR__ << "Error message was as follows: " << theStateMachine_.getErrorMessage() << std::endl;
	}

	RunControlStateMachine::theProgressBar_.step();
	theProgressBar_.complete();

	__COUT__ << "Ending state for " << stateMachineName_ << " is " << theStateMachine_.getCurrentStateName() << std::endl;
	__COUT__ << "result = " << result << std::endl;
	return SOAPUtilities::makeSOAPMessageReference(result);
}

