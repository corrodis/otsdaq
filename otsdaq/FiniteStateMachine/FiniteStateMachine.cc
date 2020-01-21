#include "otsdaq/FiniteStateMachine/FiniteStateMachine.h"
#include "otsdaq/MessageFacility/MessageFacility.h"

#include "otsdaq/Macros/CoutMacros.h"

#include <map>
#include <sstream>

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "FSM"
#define mfSubject_ std::string("FSM-") + getStateMachineName()

//==============================================================================
FiniteStateMachine::FiniteStateMachine(const std::string& stateMachineName)
    : stateEntranceTime_(0), inTransition_(false), provenanceState_('X'), theErrorMessage_(""), stateMachineName_(stateMachineName)
{
	__GEN_COUT__ << "Constructing FiniteStateMachine" << std::endl;
}

//==============================================================================
FiniteStateMachine::~FiniteStateMachine(void) {}

//==============================================================================
toolbox::fsm::State FiniteStateMachine::getProvenanceState(void) { return provenanceState_; }

//==============================================================================
toolbox::fsm::State FiniteStateMachine::getTransitionFinalState(const std::string& transition)
{
	if(stateTransitionTable_[currentState_].find(transition) != stateTransitionTable_[currentState_].end())
		return stateTransitionTable_[currentState_][transition];
	else
	{
		std::ostringstream error;
		error << "Cannot find transition name with transition: " << transition << ", unknown!";
		XCEPT_RAISE(toolbox::fsm::exception::Exception, error.str());
	}
}

//==============================================================================
std::string FiniteStateMachine::getProvenanceStateName(void) { return getStateName(getProvenanceState()); }

//==============================================================================
std::string FiniteStateMachine::getCurrentStateName(void) { return getStateName(getCurrentState()); }

//==============================================================================
// getTimeInState
//	returns number of seconds elapsed while in current state
//	returns 0 if invalid (i.e. stateEntranceTime_ is not set - stateEntranceTime_ is
// initialized to 0)
time_t FiniteStateMachine::getTimeInState(void) { return stateEntranceTime_ ? (time(0) - stateEntranceTime_) : 0; }

//==============================================================================
std::string FiniteStateMachine::getCurrentTransitionName(const std::string& transition)
{
	if(stateTransitionNameTable_[currentState_].find(transition) != stateTransitionNameTable_[currentState_].end())
	{
		return stateTransitionNameTable_[currentState_][transition];
	}
	else
	{
		std::ostringstream error;
		error << "Cannot find transition name with transition: " << transition << ", unknown!";
		XCEPT_RAISE(toolbox::fsm::exception::Exception, error.str());
	}
}

//==============================================================================
std::string FiniteStateMachine::getTransitionName(const toolbox::fsm::State from, const std::string& transition)
{
	if(stateTransitionNameTable_[from].find(transition) != stateTransitionNameTable_[from].end())
	{
		return stateTransitionNameTable_[from][transition];
	}
	else
	{
		std::ostringstream error;
		error << "Cannot find transition name from " << from << " with transition: " << transition << ", unknown!";
		XCEPT_RAISE(toolbox::fsm::exception::Exception, error.str());
	}
}

//==============================================================================
std::string FiniteStateMachine::getTransitionParameter(const toolbox::fsm::State from, const std::string& transition)
{
	if(stateTransitionParameterTable_[from].find(transition) != stateTransitionParameterTable_[from].end())
	{
		return stateTransitionParameterTable_[from][transition];
	}
	return "";
}

//==============================================================================
std::string FiniteStateMachine::getTransitionFinalStateName(const std::string& transition) { return getStateName(getTransitionFinalState(transition)); }

//==============================================================================
bool FiniteStateMachine::execTransition(const std::string& transition)
{
	const xoap::MessageReference message;
	return execTransition(transition, message);
}

//==============================================================================
// execTransition
//
//	Returns true if transition is successfully executed
//		else false if this exec did not complete a transition.
//
//	Note: For iteration handling, there is iterationIndex_ and iterationWorkFlag_.
//		These are different (higher level) than the members of VStateMachine.
bool FiniteStateMachine::execTransition(const std::string& transition, const xoap::MessageReference& message)
{
	__GEN_COUTV__(transition);

	if(transition == "fail")
	{
		__GEN_COUT_INFO__ << "Failing now!!" << __E__;

		while(inTransition_)
		{
			__GEN_COUT__ << "Currently in a transition executed from current state " << getProvenanceStateName()
			             << ". Attempting to wait for the transition to complete." << __E__;
			sleep(1);
		}
		sleep(1);

		// find any valid transition and take it..
		//	all transition functions must check for a failure
		//	flag, and throw an exception to go to Fail state

		std::map<std::string, toolbox::fsm::State> transitions = getTransitions(getCurrentState());
		for(const auto& transitionPair : transitions)
		{
			__GEN_COUT__ << "Taking transition to indirect failure: " << transitionPair.first << __E__;
			toolbox::Event::Reference event(new toolbox::Event(transitionPair.first, this));

			try
			{
				this->fireEvent(event);
			}
			catch(toolbox::fsm::exception::Exception& e)
			{
				std::ostringstream error;
				error << "Transition " << transition << " was not executed from current state " << getStateName(getCurrentState())
				      << ". There was an error: " << e.what();
				__GEN_COUT_ERR__ << error.str() << std::endl;
			}
			inTransition_      = false;
			stateEntranceTime_ = time(0);
			return true;
		}
		//		//XCEPT_RAISE (toolbox::fsm::exception::Exception, transition);
		//		theMessage_ = message;
		//		toolbox::Event::Reference event(new toolbox::Event(, this));
		//
	}

	if(inTransition_)
	{
		__GEN_COUT_WARN__ << "In transition, and received another transition: " << transition << ". Ignoring..." << __E__;

		return false;
	}
	inTransition_             = true;
	bool transitionSuccessful = true;
	provenanceState_          = getCurrentState();

	std::map<std::string, toolbox::fsm::State> transitions = getTransitions(getCurrentState());
	if(transitions.find(transition) == transitions.end())
	{
		inTransition_ = false;
		std::ostringstream error;
		error << transition << " is not in the list of the transitions from current state " << getStateName(getCurrentState());
		__GEN_COUT_ERR__ << error.str() << std::endl;
		XCEPT_RAISE(toolbox::fsm::exception::Exception, error.str());
		//__GEN_COUT__ << error << std::endl;
		//		__GEN_COUT__ << "Transition?" << inTransition_ << std::endl;
		return false;
	}

	// fire FSM event by calling mapped function
	//	(e.g. mapped by RunControlStateMachine and function implemented by
	//		CoreSupervisorBase class inheriting from RunControlStateMachine)
	try
	{
		toolbox::Event::Reference event(new toolbox::Event(transition, this));
		theMessage_ = message;  // Even if it is bad, there can only be 1 transition at a time
		                        // so this parameter should not change during all transition

		this->fireEvent(event);
	}
	catch(toolbox::fsm::exception::Exception& e)
	{
		inTransition_        = false;
		transitionSuccessful = false;
		std::ostringstream error;
		__GEN_SS__ << "Transition " << transition << " was not executed from current state " << getStateName(getCurrentState())
		           << ". There was an error: " << e.what();
		__GEN_COUT_ERR__ << ss.str() << std::endl;
		// diagService_->reportError(err.str(),DIAGERROR);

		// send state machine to error
		XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
	}
	catch(...)
	{
		inTransition_        = false;
		transitionSuccessful = false;
		__GEN_SS__ << "Transition " << transition << " was not executed from current state " << getStateName(getCurrentState())
		           << ". There was an unknown error.";
		__GEN_COUT_ERR__ << ss.str() << std::endl;
		// diagService_->reportError(err.str(),DIAGERROR);

		// send state machine to error
		XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
	}

	inTransition_      = false;
	stateEntranceTime_ = time(0);
	return transitionSuccessful;
}

//==============================================================================
bool FiniteStateMachine::isInTransition(void) { return inTransition_; }

//==============================================================================
void FiniteStateMachine::setErrorMessage(const std::string& errMessage, bool append)
{
	if(append)
		theErrorMessage_ += errMessage;
	else
		theErrorMessage_ = errMessage;
}

//==============================================================================
const std::string& FiniteStateMachine::getErrorMessage() const { return theErrorMessage_; }

//==============================================================================
void FiniteStateMachine::setInitialState(toolbox::fsm::State state)
{
	toolbox::fsm::FiniteStateMachine::setInitialState(state);
	provenanceState_   = state;
	stateEntranceTime_ = time(0);
}

//==============================================================================
const xoap::MessageReference& FiniteStateMachine::getCurrentMessage(void) { return theMessage_; }
