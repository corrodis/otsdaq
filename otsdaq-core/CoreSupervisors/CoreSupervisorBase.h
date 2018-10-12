#ifndef _ots_CoreSupervisorBase_h_
#define _ots_CoreSupervisorBase_h_

#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"

#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationTree.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"

#include "otsdaq-core/CoreSupervisors/CorePropertySupervisorBase.h"
#include "otsdaq-core/WebUsersUtilities/RemoteWebUsers.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop
#include "otsdaq-core/Macros/XDAQApplicationMacros.h"
#include "xgi/Method.h"

#include <toolbox/fsm/FailedEvent.h>

#include <xdaq/NamespaceURI.h>
#include <xoap/Method.h>

#include <string> /*string and to_string*/
#include <vector>
#include <map>
#include <memory>



namespace ots
{

//CoreSupervisorBase
//	This class should be the base class for all client otsdaq, XDAQ-based, supervisors.
//	That is, all supervisors that need web requests through the ots desktop
//		with access verified by the Gateway Supervisor,
//		or that need a state machines driven by the Gateway Supervisor.
class CoreSupervisorBase: public xdaq::Application, public SOAPMessenger,
	public CorePropertySupervisorBase, public RunControlStateMachine
{

public:

    CoreSupervisorBase         								(xdaq::ApplicationStub * s);
    virtual ~CoreSupervisorBase								(void);

    void 					destroy               			(void);

    //Here are the common web request handlers:
    //	defaultPage returns the public html page
    //	request checks the login before proceeding to virtual request
    //		- All Supervisors should implement request for their actions (and they will get the security wrapper for free)
    //		- The security setting defaults can be setup or forced by overriding setSupervisorPropertyDefaults and forceSupervisorProperties
    virtual void 			defaultPage      				(xgi::Input* in, xgi::Output* out) 	;
    virtual void			request         	 			(const std::string& requestType, cgicc::Cgicc& cgiIn, HttpXmlDocument& xmlOut, 	const WebUsers::RequestUserInfo& userInfo) ;
    virtual void			nonXmlRequest          			(const std::string& requestType, cgicc::Cgicc& cgiIn, std::ostream& out, 		const WebUsers::RequestUserInfo& userInfo) ;

private:
    xoap::MessageReference 	workLoopStatusRequestWrapper  	(xoap::MessageReference message )  	;
    void 					defaultPageWrapper              (xgi::Input* in, xgi::Output* out)  ;
    void 					requestWrapper              	(xgi::Input* in, xgi::Output* out)  ;

public:

    //State Machine request handlers
    void 			        stateMachineXgiHandler       	(xgi::Input* in, xgi::Output* out ) ;
    void 			        stateMachineResultXgiHandler 	(xgi::Input* in, xgi::Output* out ) ;
    xoap::MessageReference 	stateMachineXoapHandler      	(xoap::MessageReference message )  	;
    xoap::MessageReference 	stateMachineResultXoapHandler	(xoap::MessageReference message )  	;

    xoap::MessageReference 	stateMachineStateRequest     	(xoap::MessageReference message )  	;
    xoap::MessageReference 	stateMachineErrorMessageRequest	(xoap::MessageReference message )  	;

    virtual xoap::MessageReference 	workLoopStatusRequest	(xoap::MessageReference message )  	;

    bool                    stateMachineThread           	(toolbox::task::WorkLoop* workLoop);

    virtual void 			stateInitial    				(toolbox::fsm::FiniteStateMachine& fsm) ;
    virtual void 			statePaused     				(toolbox::fsm::FiniteStateMachine& fsm) ;
    virtual void 			stateRunning    				(toolbox::fsm::FiniteStateMachine& fsm) ;
    virtual void 			stateHalted     				(toolbox::fsm::FiniteStateMachine& fsm) ;
    virtual void 			stateConfigured 				(toolbox::fsm::FiniteStateMachine& fsm) ;
    virtual void			inError         				(toolbox::fsm::FiniteStateMachine& fsm) ;

    virtual void 			transitionConfiguring 			(toolbox::Event::Reference e) ;
    virtual void 			transitionHalting     			(toolbox::Event::Reference e) ;
    virtual void 			transitionInitializing			(toolbox::Event::Reference e) ;
    virtual void 			transitionPausing     			(toolbox::Event::Reference e) ;
    virtual void 			transitionResuming    			(toolbox::Event::Reference e) ;
    virtual void 			transitionStarting    			(toolbox::Event::Reference e) ;
    virtual void 			transitionStopping    			(toolbox::Event::Reference e) ;
    virtual void 			enteringError         			(toolbox::Event::Reference e) ;


    static const std::string	   WORK_LOOP_DONE, WORK_LOOP_WORKING;

protected:

    WorkLoopManager                	stateMachineWorkLoopManager_;
    toolbox::BSem                  	stateMachineSemaphore_;
    std::vector<VStateMachine*>    	theStateMachineImplementation_;

    //for managing transition iterations
    std::vector<bool>				stateMachinesIterationDone_;
    unsigned int					stateMachinesIterationWorkCount_;
    unsigned int					subIterationWorkStateMachineIndex_;
    void							preStateMachineExecution(unsigned int i);
    void							postStateMachineExecution(unsigned int i);
    void							preStateMachineExecutionLoop(void);
    void							postStateMachineExecutionLoop(void);


    RemoteWebUsers             		theRemoteWebUsers_;
};

}

#endif
