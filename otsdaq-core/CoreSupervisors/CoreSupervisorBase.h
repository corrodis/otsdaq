#ifndef _ots_CoreSupervisorBase_h_
#define _ots_CoreSupervisorBase_h_

#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/Supervisor/SupervisorsInfo.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"
#include "otsdaq-core/SupervisorDescriptorInfo/SupervisorDescriptorInfo.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop
#include "xgi/Method.h"

#include <string>
#include <memory>
#include <vector>


namespace ots
{

class ConfigurationManager;
class VStateMachine;
class FEVInterfacesManager;

class CoreSupervisorBase: public xdaq::Application, public SOAPMessenger, public RunControlStateMachine
{

public:

    CoreSupervisorBase         (xdaq::ApplicationStub * s) throw (xdaq::exception::Exception);
    virtual ~CoreSupervisorBase(void);
    void init                  (void);
    void destroy               (void);
    void Default               (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
    void request               (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);

    //State Machine requests handlers
    void 			        stateMachineXgiHandler       	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    void 			        stateMachineResultXgiHandler 	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    xoap::MessageReference 	stateMachineXoapHandler      	(xoap::MessageReference message )  	throw (xoap::exception::Exception);
    xoap::MessageReference 	stateMachineResultXoapHandler	(xoap::MessageReference message )  	throw (xoap::exception::Exception);

    xoap::MessageReference 	stateMachineStateRequest     	(xoap::MessageReference message )  	throw (xoap::exception::Exception);
    xoap::MessageReference 	stateMachineErrorMessageRequest	(xoap::MessageReference message )  	throw (xoap::exception::Exception);
    xoap::MessageReference 	macroMakerSupervisorRequest  	(xoap::MessageReference message )  	throw (xoap::exception::Exception);

    bool                    stateMachineThread           	(toolbox::task::WorkLoop* workLoop);

    virtual void stateInitial    (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    virtual void statePaused     (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    virtual void stateRunning    (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    virtual void stateHalted     (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    virtual void stateConfigured (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    virtual void inError         (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);

    virtual void transitionConfiguring (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    virtual void transitionHalting     (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    virtual void transitionInitializing(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    virtual void transitionPausing     (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    virtual void transitionResuming    (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    virtual void transitionStarting    (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    virtual void transitionStopping    (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    virtual void enteringError         (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);

protected:
    WorkLoopManager                stateMachineWorkLoopManager_;
    toolbox::BSem                  stateMachineSemaphore_;

    ConfigurationManager*          theConfigurationManager_;

	std::string                    XDAQContextConfigurationName_;
	std::string                    supervisorConfigurationPath_;

	std::string                    supervisorContextUID_;
	std::string                    supervisorApplicationUID_;
	std::string                    supervisorClass_;
	std::string                    supervisorClassNoNamespace_;

    SupervisorDescriptorInfo       supervisorDescriptorInfo_;
    std::vector<VStateMachine*>    theStateMachineImplementation_;
};

}

#endif
