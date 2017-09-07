#include "otsdaq-core/FiniteStateMachine/FiniteStateMachine.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <sstream>
#include <map>

using namespace ots;

//========================================================================================================================
FiniteStateMachine::FiniteStateMachine(void)
: stateEntranceTime_(0)
, inTransition_   	(false)
, provenanceState_	('X')
, theErrorMessage_ ("")
{
	__MOUT__ << "Transition? "<< inTransition_ << std::endl;
}

//========================================================================================================================
FiniteStateMachine::~FiniteStateMachine(void)
{}

//========================================================================================================================
toolbox::fsm::State FiniteStateMachine::getProvenanceState(void)
{
	return provenanceState_;
}

//========================================================================================================================
toolbox::fsm::State FiniteStateMachine::getTransitionFinalState(const std::string& transition) throw (toolbox::fsm::exception::Exception)
{
	if(stateTransitionTable_[currentState_].find(transition) != stateTransitionTable_[currentState_].end())
		return stateTransitionTable_[currentState_][transition];
	else
	{
		std::ostringstream error;
		error << "Cannot find transition name with transition: " << transition << ", unknown!";
		XCEPT_RAISE (toolbox::fsm::exception::Exception, error.str());
	}
}

//========================================================================================================================
std::string FiniteStateMachine::getProvenanceStateName(void)
{
	return getStateName(getProvenanceState());
}

//========================================================================================================================
std::string FiniteStateMachine::getCurrentStateName(void)
{
	return getStateName(getCurrentState());
}

//========================================================================================================================
//getTimeInState
//	returns number of seconds elapsed while in current state
//	returns 0 if invalid (i.e. stateEntranceTime_ is not set - stateEntranceTime_ is initialized to 0)
time_t FiniteStateMachine::getTimeInState (void)
{
	return stateEntranceTime_?(time(0) - stateEntranceTime_):0;
}

//========================================================================================================================
std::string FiniteStateMachine::getCurrentTransitionName(const std::string& transition) throw (toolbox::fsm::exception::Exception)
{
	if(stateTransitionNameTable_[currentState_].find(transition) != stateTransitionNameTable_[currentState_].end())
	{
		return stateTransitionNameTable_[currentState_][transition];
	}
	else
	{
		std::ostringstream error;
		error << "Cannot find transition name with transition: " << transition << ", unknown!";
		XCEPT_RAISE (toolbox::fsm::exception::Exception, error.str());
	}
}

//========================================================================================================================
std::string FiniteStateMachine::getTransitionName(const toolbox::fsm::State from, const std::string& transition) throw (toolbox::fsm::exception::Exception)
{
	if(stateTransitionNameTable_[from].find(transition) != stateTransitionNameTable_[from].end())
	{
		return stateTransitionNameTable_[from][transition];
	}
	else
	{
		std::ostringstream error;
		error << "Cannot find transition name from " << from << " with transition: " << transition << ", unknown!";
		XCEPT_RAISE (toolbox::fsm::exception::Exception, error.str());
	}
}

//========================================================================================================================
std::string FiniteStateMachine::getTransitionParameter(const toolbox::fsm::State from, const std::string& transition) throw (toolbox::fsm::exception::Exception)
{
	if(stateTransitionParameterTable_[from].find(transition) != stateTransitionParameterTable_[from].end())
	{
		return stateTransitionParameterTable_[from][transition];
	}
	return "";
}

//========================================================================================================================
std::string FiniteStateMachine::getTransitionFinalStateName(const std::string& transition) throw (toolbox::fsm::exception::Exception)
{
	return getStateName(getTransitionFinalState(transition));
}

//========================================================================================================================
bool FiniteStateMachine::execTransition(const std::string& transition) throw (toolbox::fsm::exception::Exception)
{
	const xoap::MessageReference message;//FIXME I would like to have everything in 1 line but like this is not a big deal
	return execTransition(transition,message);
}

//========================================================================================================================
bool FiniteStateMachine::execTransition(const std::string& transition, const xoap::MessageReference& message) throw (toolbox::fsm::exception::Exception)
{
//	__MOUT__ << "Transition?" << inTransition_ << std::endl;
	if(inTransition_) return false;
	inTransition_ = true;
	bool transitionSuccessful = true;
	provenanceState_ = getCurrentState();

	std::map<std::string, toolbox::fsm::State> transitions = getTransitions(getCurrentState());
	if(transitions.find(transition) == transitions.end())
	{
		inTransition_ = false;
		std::ostringstream error;
		error << transition << " is not in the list of the transitions from current state " << getStateName (getCurrentState());
		__MOUT_ERR__ << error.str() << std::endl;
		XCEPT_RAISE (toolbox::fsm::exception::Exception, error.str());
		//__MOUT__ << error << std::endl;
//		__MOUT__ << "Transition?" << inTransition_ << std::endl;
		return false;
	}

	try
	{
		toolbox::Event::Reference e(new toolbox::Event(transition, this));
		theMessage_ = message;//Even if it is bad, there can only be 1 transition at a time so this parameter should not change during all transition
		this->fireEvent(e);
	}
	catch (toolbox::fsm::exception::Exception& e)
	{
		inTransition_ = false;
		transitionSuccessful = false;
		std::ostringstream error;
		error << "Transition " << transition << " cannot be executed from current state " << getStateName (getCurrentState());
		__MOUT_ERR__ << error.str() << std::endl;
		//diagService_->reportError(err.str(),DIAGERROR);
		XCEPT_RAISE (toolbox::fsm::exception::Exception, error.str());//This make everything crash so you know for sure there is something wrong
	}
//	__MOUT__ << "Transition?" << inTransition_ << std::endl;
	inTransition_ = false;
//	__MOUT__ << "Done with fsm transition" << std::endl;
//	__MOUT__ << "Transition?" << inTransition_ << std::endl;
	stateEntranceTime_ = time(0);
	return transitionSuccessful;
}

//========================================================================================================================
bool FiniteStateMachine::isInTransition(void)
{
	return inTransition_;
}

//========================================================================================================================
void FiniteStateMachine::setErrorMessage(const std::string &errMessage)
{
	theErrorMessage_ = errMessage;
}

//========================================================================================================================
const std::string& FiniteStateMachine::getErrorMessage() const
{
	return theErrorMessage_;
}

//========================================================================================================================
void FiniteStateMachine::setInitialState(toolbox::fsm::State state)
{
	toolbox::fsm::FiniteStateMachine::setInitialState(state);
	provenanceState_ = state;
	stateEntranceTime_ = time(0);
}

//========================================================================================================================
const xoap::MessageReference& FiniteStateMachine::getCurrentMessage(void)
{
	return theMessage_;
}
