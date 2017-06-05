#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"

#include <toolbox/fsm/FailedEvent.h>
#include <xoap/Method.h>
#include <xdaq/NamespaceURI.h>

#include <iostream>

using namespace ots;


//========================================================================================================================
RunControlStateMachine::RunControlStateMachine(std::string name)
: stateMachineName_(name)
{
	theStateMachine_.addState('I', "Initial",     this, &RunControlStateMachine::stateInitial);
	theStateMachine_.addState('H', "Halted",      this, &RunControlStateMachine::stateHalted);
	theStateMachine_.addState('C', "Configured",  this, &RunControlStateMachine::stateConfigured);
	theStateMachine_.addState('R', "Running",     this, &RunControlStateMachine::stateRunning);
	theStateMachine_.addState('P', "Paused",      this, &RunControlStateMachine::statePaused);
	//theStateMachine_.addState('v', "Recovering",  this, &RunControlStateMachine::stateRecovering);
	//theStateMachine_.addState('T', "TTSTestMode", this, &RunControlStateMachine::stateTTSTestMode);

	//RAR added back in on 11/20/2016.. why was it removed..
	//exceptions like..
	//	XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());)
	//	take state machine to "failed" otherwise
	theStateMachine_.setStateName('F',"Failed");//x
	theStateMachine_.setFailedStateTransitionAction (this, &RunControlStateMachine::enteringError);
	theStateMachine_.setFailedStateTransitionChanged(this, &RunControlStateMachine::inError);

	//this line was added to get out of Failed state
	theStateMachine_.addStateTransition('F', 'H', "Halt"	  , "Halting"	  , this, &RunControlStateMachine::transitionHalting);
		//this attempt to get out of fail state makes things crash FIXME
	//end RAR added back in on 11/20/2016.. why was it removed..

	theStateMachine_.addStateTransition('H', 'C', "Configure" , "Configuring" , "ConfigurationAlias", this, &RunControlStateMachine::transitionConfiguring);

	//Every state can transition to halted
	theStateMachine_.addStateTransition('I', 'H', "Initialize", "Initializing", this, &RunControlStateMachine::transitionInitializing);
	theStateMachine_.addStateTransition('H', 'H', "Halt"      , "Halting"     , this, &RunControlStateMachine::transitionHalting);
	theStateMachine_.addStateTransition('C', 'H', "Halt"      , "Halting"     , this, &RunControlStateMachine::transitionHalting);
	theStateMachine_.addStateTransition('R', 'H', "Abort"     , "Aborting"    , this, &RunControlStateMachine::transitionHalting);
	theStateMachine_.addStateTransition('P', 'H', "Abort"     , "Aborting"    , this, &RunControlStateMachine::transitionHalting);


	theStateMachine_.addStateTransition('R', 'P', "Pause"     , "Pausing"     , this, &RunControlStateMachine::transitionPausing);
	theStateMachine_.addStateTransition('P', 'R', "Resume"    , "Resuming"    , this, &RunControlStateMachine::transitionResuming);
	theStateMachine_.addStateTransition('C', 'R', "Start"     , "Starting"    , this, &RunControlStateMachine::transitionStarting);
	theStateMachine_.addStateTransition('R', 'C', "Stop"      , "Stopping"    , this, &RunControlStateMachine::transitionStopping);
	theStateMachine_.addStateTransition('P', 'C', "Stop"      , "Stopping"    , this, &RunControlStateMachine::transitionStopping);


	// NOTE!! There must be a defined message handler for each transition name created above
	// Also Note: The definition of theStateMachine above will get
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Initialize", XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Configure" , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Start"     , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Stop"      , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Pause"     , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Resume"    , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Halt"      , XDAQ_NS_URI);
	xoap::bind(this, &RunControlStateMachine::runControlMessageHandler, "Abort"     , XDAQ_NS_URI); //added for "Abort" transition name


	reset();
}

//========================================================================================================================
RunControlStateMachine::~RunControlStateMachine(void)
{
}

//========================================================================================================================
void RunControlStateMachine::reset(void)
{
	__MOUT__ << stateMachineName_ << " is in transition?" << theStateMachine_.isInTransition() << std::endl;
	theStateMachine_.setInitialState('I');
	theStateMachine_.reset();
	__MOUT__ << stateMachineName_ << " is in transition?" << theStateMachine_.isInTransition() << std::endl;
}

//========================================================================================================================
xoap::MessageReference RunControlStateMachine::runControlMessageHandler(
		xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	__MOUT__ << "Starting state for " << stateMachineName_ << " is " << theStateMachine_.getCurrentStateName() << std::endl;
	__MOUT__ << SOAPUtilities::translate(message) << std::endl;
	std::string command = SOAPUtilities::translate(message).getCommand();
	//__MOUT__ << "Command:-" << command << "-" << std::endl;
	theProgressBar_.reset(command,stateMachineName_);
	std::string result = command + "Done";

	//if error is received, immediately go to fail state
	//	likely error was sent by central FSM or external xoap
	if(command == "Error" || command == "Fail")
	{
		__SS__ << command << " was received! Immediately throwing FSM exception." << std::endl;
		__MOUT_ERR__ << "\n" << ss.str();
		XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
		return SOAPUtilities::makeSOAPMessageReference(result);
	}

	//handle normal transitions here
	try
	{
		theStateMachine_.execTransition(command,message);
		//__MOUT__ << "I don't know what is going on!" << std::endl;
	}
	catch (toolbox::fsm::exception::Exception& e)
	{
		result = command + "Failed";
		__MOUT__ << e.what() << std::endl;
	}

	theProgressBar_.complete();
	__MOUT__ << "Ending state for " << stateMachineName_ << " is " << theStateMachine_.getCurrentStateName() << std::endl;
	return SOAPUtilities::makeSOAPMessageReference(result);
}

