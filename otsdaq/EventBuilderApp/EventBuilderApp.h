#ifndef _ots_EventBuilderApp_h_
#define _ots_EventBuilderApp_h_

#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/Supervisor/SupervisorsInfo.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"
#include "otsdaq-core/SupervisorDescriptorInfo/SupervisorDescriptorInfo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop
#include <xgi/Method.h>

#include <string>
#include <map>

#include <memory>

#include "artdaq/Application/MPI2/MPISentry.hh"


namespace ots
{

class ConfigurationManager;
class ConfigurationGroupKey;
class EventBuilderInterface;

class EventBuilderApp: public xdaq::Application, public SOAPMessenger, public RunControlStateMachine
{

public:

    XDAQ_INSTANTIATOR();

    EventBuilderApp            (xdaq::ApplicationStub * s) throw (xdaq::exception::Exception);
    virtual ~EventBuilderApp   (void);
    void init                            (void);
    void destroy                         (void);
    void Default                         (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);


    //State Machine requests handlers
    void 			        stateMachineXgiHandler       	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    void 			        stateMachineResultXgiHandler 	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    xoap::MessageReference 	stateMachineXoapHandler      	(xoap::MessageReference message )  	throw (xoap::exception::Exception);
    xoap::MessageReference 	stateMachineResultXoapHandler	(xoap::MessageReference message )  	throw (xoap::exception::Exception);
    bool                    stateMachineThread           	(toolbox::task::WorkLoop* workLoop);

    xoap::MessageReference 	stateMachineStateRequest     	(xoap::MessageReference message )  	throw (xoap::exception::Exception);
    xoap::MessageReference 	stateMachineErrorMessageRequest	(xoap::MessageReference message )  	throw (xoap::exception::Exception);

    void stateInitial    (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void statePaused     (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void stateRunning    (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void stateHalted     (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void stateConfigured (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void inError         (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);

    void transitionConfiguring (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionHalting     (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionInitializing(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionPausing     (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionResuming    (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionStarting    (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionStopping    (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void enteringError         (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);

private:
    WorkLoopManager                            stateMachineWorkLoopManager_;
    toolbox::BSem                              stateMachineSemaphore_;

    SupervisorDescriptorInfo                   theSupervisorDescriptorInfo_;
    ConfigurationManager*                      theConfigurationManager_;
	std::string                                XDAQContextConfigurationName_;
	std::string                                supervisorConfigurationPath_;
	std::string                                supervisorContextUID_;
	std::string                                supervisorApplicationUID_;

    std::map<int, ots::EventBuilderInterface*> theARTDAQEventBuilderInterfaces_;
    std::unique_ptr<artdaq::MPISentry>         mpiSentry_;

    //    std::shared_ptr<const ConfigurationGroupKey>    theConfigurationGroupKey_;
    //    int                                        supervisorInstance_;

};

}

#endif
