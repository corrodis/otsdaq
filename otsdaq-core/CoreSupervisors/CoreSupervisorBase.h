#ifndef _ots_CoreSupervisorBase_h_
#define _ots_CoreSupervisorBase_h_

#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
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

class CoreSupervisorBase: public xdaq::Application, public SOAPMessenger,
	public RunControlStateMachine, public CorePropertySupervisorBase
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

    //virtual void			setSupervisorPropertyDefaults	(void); //override to control supervisor specific defaults
    //virtual void			forceSupervisorPropertyValues	(void) {;} //override to force supervisor property values (and ignore user settings)


private:
    xoap::MessageReference 	workLoopStatusRequestWrapper  	(xoap::MessageReference message )  	;
    void 					defaultPageWrapper              (xgi::Input* in, xgi::Output* out)  ;
    void 					requestWrapper              	(xgi::Input* in, xgi::Output* out)  ;

public:

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


    AllSupervisorInfo 				allSupervisorInfo_;
    RemoteWebUsers             		theRemoteWebUsers_;
    std::vector<VStateMachine*>    	theStateMachineImplementation_;
//
//
//    //Supervisor Property names
//    //	to access, use CoreSupervisorBase::getSupervisorProperty and CorePropertySupervisorBase::setSupervisorProperty
//	static const struct SupervisorProperties
//	{
//		SupervisorProperties()
//		: allSetNames_({&CheckUserLockRequestTypes,&RequireUserLockRequestTypes,
//			&AutomatedRequestTypes,&AllowNoLoginRequestTypes,&NeedUsernameRequestTypes,
//			&NeedDisplayNameRequestTypes,&NeedGroupMembershipRequestTypes,&NeedSessionIndexRequestTypes,
//			&NoXmlWhiteSpaceRequestTypes,&NonXMLRequestTypes})
//		{}
//
//		const std::string UserPermissionsThreshold			= "UserPermissionsThreshold";
//		const std::string UserGroupsAllowed					= "UserGroupsAllowed";
//		const std::string UserGroupsDisallowed				= "UserGroupsDisallowed";
//
//		const std::string CheckUserLockRequestTypes			= "CheckUserLockRequestTypes";
//		const std::string RequireUserLockRequestTypes		= "RequireUserLockRequestTypes";
//		const std::string AutomatedRequestTypes				= "AutomatedRequestTypes";
//		const std::string AllowNoLoginRequestTypes			= "AllowNoLoginRequestTypes";
//
//		const std::string NeedUsernameRequestTypes			= "NeedUsernameRequestTypes";
//		const std::string NeedDisplayNameRequestTypes		= "NeedDisplayNameRequestTypes";
//		const std::string NeedGroupMembershipRequestTypes	= "NeedGroupMembershipRequestTypes";
//		const std::string NeedSessionIndexRequestTypes		= "NeedSessionIndexRequestTypes";
//
//		const std::string NoXmlWhiteSpaceRequestTypes		= "NoXmlWhiteSpaceRequestTypes";
//		const std::string NonXMLRequestTypes				= "NonXMLRequestTypes";
//
//		const std::set<const std::string*> allSetNames_;
//	} SUPERVISOR_PROPERTIES;

private:
	//property private members
//	void					checkSupervisorPropertySetup		(void);
//	volatile bool									propertiesAreSetup_;
//
//	//for public access to property map,..
//	//	use CoreSupervisorBase::getSupervisorProperty and CorePropertySupervisorBase::setSupervisorProperty
//	std::map<std::string, std::string> 				propertyMap_;
//	struct CoreSupervisorPropertyStruct
//	{
//		CoreSupervisorPropertyStruct()
//		: allSets_ ({&CheckUserLockRequestTypes,&RequireUserLockRequestTypes,
//			&AutomatedRequestTypes,&AllowNoLoginRequestTypes,&NeedUsernameRequestTypes,
//			&NeedDisplayNameRequestTypes,&NeedGroupMembershipRequestTypes,&NeedSessionIndexRequestTypes,
//			&NoXmlWhiteSpaceRequestTypes,&NonXMLRequestTypes})
//		{}
//
//		std::map<std::string,uint8_t> 				UserPermissionsThreshold;
//		std::map<std::string,std::string> 			UserGroupsAllowed;
//		std::map<std::string,std::string>  			UserGroupsDisallowed;
//
//		std::set<std::string> 						CheckUserLockRequestTypes;
//		std::set<std::string> 						RequireUserLockRequestTypes;
//		std::set<std::string> 						AutomatedRequestTypes;
//		std::set<std::string> 						AllowNoLoginRequestTypes;
//
//		std::set<std::string> 						NeedUsernameRequestTypes;
//		std::set<std::string> 						NeedDisplayNameRequestTypes;
//		std::set<std::string> 						NeedGroupMembershipRequestTypes;
//		std::set<std::string> 						NeedSessionIndexRequestTypes;
//
//		std::set<std::string> 						NoXmlWhiteSpaceRequestTypes;
//		std::set<std::string> 						NonXMLRequestTypes;
//
//		std::set< std::set<std::string>* > 			allSets_;
//	} propertyStruct_;

public:
//	void								loadUserSupervisorProperties		(void);
//	template<class T>
//	void 								setSupervisorProperty				(const std::string& propertyName, const T& propertyValue)
//	{
//		std::stringstream ss;
//		ss << propertyValue;
//		setSupervisorProperty(propertyName,ss.str());
//	}
//	void 								setSupervisorProperty				(const std::string& propertyName, const std::string& propertyValue);
//	template<class T>
//	void 								addSupervisorProperty				(const std::string& propertyName, const T& propertyValue)
//	{
//		//prepend new values.. since map/set extraction takes the first value encountered
//		std::stringstream ss;
//		ss << propertyValue << " | " << getSupervisorProperty(propertyName);
//		setSupervisorProperty(propertyName,ss.str());
//	}
//	void 								addSupervisorProperty				(const std::string& propertyName, const std::string& propertyValue);
//	template<class T>
//	T 									getSupervisorProperty				(const std::string& propertyName)
//	{
//		//check if need to setup properties
//		checkSupervisorPropertySetup();
//
//		auto it = propertyMap_.find(propertyName);
//		if(it == propertyMap_.end())
//		{
//			__SS__ << "Could not find property named " << propertyName << __E__;
//			__SS_THROW__;
//		}
//		return StringMacros::validateValueForDefaultStringDataType<T>(it->second);
//	}
//	std::string							getSupervisorProperty(const std::string& propertyName);
//	//void								setSupervisorPropertyUserPermissionsThreshold(uint8_t userPermissionThreshold);
//	WebUsers::permissionLevel_t			getSupervisorPropertyUserPermissionsThreshold(const std::string& requestType="*");


};

}

#endif
