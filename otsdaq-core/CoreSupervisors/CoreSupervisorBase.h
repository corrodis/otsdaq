#ifndef _ots_CoreSupervisorBase_h_
#define _ots_CoreSupervisorBase_h_

#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"
#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"

#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationTree.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"

#include "otsdaq-core/WebUsersUtilities/RemoteWebUsers.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop
#include "xgi/Method.h"

#include <toolbox/fsm/FailedEvent.h>

#include <xdaq/NamespaceURI.h>
#include <xoap/Method.h>

#include <string>
#include <vector>
#include <map>
#include <memory>



namespace ots
{

class CoreSupervisorBase: public xdaq::Application, public SOAPMessenger, public RunControlStateMachine
{

public:

    CoreSupervisorBase         (xdaq::ApplicationStub * s);
    virtual ~CoreSupervisorBase(void);

    virtual void			setSupervisorPropertyDefaults	(void);
    void 					destroy               			(void);

private:
    xoap::MessageReference 	workLoopStatusRequestWrapper  	(xoap::MessageReference message )  	;
    void 					DefaultWrapper               	(xgi::Input* in, xgi::Output* out)  ;
    void 					requestWrapper              	(xgi::Input* in, xgi::Output* out)  ;

public:
    //common web request handlers
    virtual void 			Default               			(xgi::Input* in, xgi::Output* out) ;
    virtual void			request               			(xgi::Input* in, xgi::Output* out) ;

    //bool					loginGateway					(xgi::Input* in, xgi::Output* out);

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

    ConfigurationManager*          	theConfigurationManager_;

	std::string                    	XDAQContextConfigurationName_;
	std::string                    	supervisorConfigurationPath_;

	std::string                    	supervisorContextUID_;
	std::string                    	supervisorApplicationUID_;
	std::string                    	supervisorClass_;
	std::string                    	supervisorClassNoNamespace_;

    AllSupervisorInfo 				allSupervisorInfo_;
    RemoteWebUsers             		theRemoteWebUsers_;
    std::vector<VStateMachine*>    	theStateMachineImplementation_;

    bool 							LOCK_REQUIRED_;
    uint8_t							USER_PERMISSIONS_THRESHOLD_;
    std::string						USER_GROUPS_ALLOWED_;
    std::string						USER_GROUPS_DISALLOWED_;

    //Supervisor Property names
	struct SupervisorProperties
	{
		std::string const fieldRequireLock						= "RequireUserHasLock";
		std::string const fieldUserPermissionsThreshold			= "UserPermissionsThreshold";
		std::string const fieldUserGroupsAllowed				= "UserGroupsAllowed";
		std::string const fieldUserGroupsDisallowed				= "UserGroupsDisallowed";
	} supervisorProperties_;
};

}

#endif
