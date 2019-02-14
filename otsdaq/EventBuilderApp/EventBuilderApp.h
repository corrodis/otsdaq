#ifndef _ots_EventBuilderApp_h_
#define _ots_EventBuilderApp_h_

#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"

#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"
#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"

#include "artdaq/Application/EventBuilderApp.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop
#include <xgi/Method.h>
#include "otsdaq-core/Macros/XDAQApplicationMacros.h"

#include <map>
#include <string>

#include <memory>

namespace ots
{
class ConfigurationManager;
class ConfigurationGroupKey;

/**
 * \brief
 */
//EventBuilderApp
//	This class provides the otsdaq interface to a single artdaq Event Builder,
class EventBuilderApp : public xdaq::Application, public SOAPMessenger, public RunControlStateMachine
{
  public:
	XDAQ_INSTANTIATOR ();

	EventBuilderApp (xdaq::ApplicationStub* s);
	virtual ~EventBuilderApp (void);
	void init (void);
	void destroy (void);
	void Default (xgi::Input* in, xgi::Output* out);

	//State Machine requests handlers
	void                   stateMachineXgiHandler (xgi::Input* in, xgi::Output* out);
	void                   stateMachineResultXgiHandler (xgi::Input* in, xgi::Output* out);
	xoap::MessageReference stateMachineXoapHandler (xoap::MessageReference message);
	xoap::MessageReference stateMachineResultXoapHandler (xoap::MessageReference message);
	bool                   stateMachineThread (toolbox::task::WorkLoop* workLoop);

	xoap::MessageReference stateMachineStateRequest (xoap::MessageReference message);
	xoap::MessageReference stateMachineErrorMessageRequest (xoap::MessageReference message);

	void stateInitial (toolbox::fsm::FiniteStateMachine& fsm);
	void statePaused (toolbox::fsm::FiniteStateMachine& fsm);
	void stateRunning (toolbox::fsm::FiniteStateMachine& fsm);
	void stateHalted (toolbox::fsm::FiniteStateMachine& fsm);
	void stateConfigured (toolbox::fsm::FiniteStateMachine& fsm);
	void inError (toolbox::fsm::FiniteStateMachine& fsm);

	void transitionConfiguring (toolbox::Event::Reference e);
	void transitionHalting (toolbox::Event::Reference e);
	void transitionInitializing (toolbox::Event::Reference e);
	void transitionPausing (toolbox::Event::Reference e);
	void transitionResuming (toolbox::Event::Reference e);
	void transitionStarting (toolbox::Event::Reference e);
	void transitionStopping (toolbox::Event::Reference e);
	void enteringError (toolbox::Event::Reference e);

  private:
	WorkLoopManager stateMachineWorkLoopManager_;
	toolbox::BSem   stateMachineSemaphore_;

	AllSupervisorInfo     allSupervisorInfo_;
	ConfigurationManager* theConfigurationManager_;
	std::string           XDAQContextConfigurationName_;
	std::string           supervisorConfigurationPath_;
	std::string           supervisorContextUID_;
	std::string           supervisorApplicationUID_;

	std::unique_ptr<artdaq::EventBuilderApp> theARTDAQEventBuilderInterface_;
};

}  // namespace ots

#endif
