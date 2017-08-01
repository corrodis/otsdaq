#ifndef _Supervisor_Supervisor_h
#define _Supervisor_Supervisor_h

#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"
#include "otsdaq-core/WebUsersUtilities/WebUsers.h"
#include "otsdaq-core/SystemMessenger/SystemMessenger.h"
#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/Supervisor/SupervisorsInfo.h"
#include "otsdaq-core/SupervisorDescriptorInfo/SupervisorDescriptorInfo.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop
//#include <toolbox/fsm/FiniteStateMachine.h>
#include <toolbox/task/WorkLoop.h>
#include <xgi/Method.h>
#include <xdata/String.h>

#include <string>
#include <set>

//defines used also by OtsConfigurationWizardSupervisor
#define FSM_LAST_CONFIGURED_GROUP_ALIAS_FILE	std::string("FSMLastConfiguredGroupAlias.hist")
#define FSM_LAST_STARTED_GROUP_ALIAS_FILE		std::string("FSMLastStartedGroupAlias.hist")


namespace ots
{

class ConfigurationManager;
class ConfigurationGroupKey;
class WorkLoopManager;

class Supervisor: public xdaq::Application, public SOAPMessenger, public RunControlStateMachine
{
	friend class OtsConfigurationWizardSupervisor;

public:

    XDAQ_INSTANTIATOR();

    Supervisor (xdaq::ApplicationStub * s) throw (xdaq::exception::Exception);
    virtual ~Supervisor(void);

    void 						init        				 	(void);
    static void                 URLDisplayThread			 	(Supervisor *);
    void 						Default     				 	(xgi::Input* in, xgi::Output* out ) 	throw (xgi::exception::Exception);
//    void 						TmpTest     				 	(xgi::Input* in, xgi::Output* out ) 	throw (xgi::exception::Exception);

    void 						loginRequest 				 	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    void 						request                      	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    void 						tooltipRequest              	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);

    //State Machine requests handlers
    void 						stateMachineXgiHandler       	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    void 						stateMachineResultXgiHandler 	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    xoap::MessageReference 		stateMachineXoapHandler      	(xoap::MessageReference msg )  	        throw (xoap::exception::Exception);
    xoap::MessageReference 		stateMachineResultXoapHandler	(xoap::MessageReference msg )  	        throw (xoap::exception::Exception);
    bool                        stateMachineThread           	(toolbox::task::WorkLoop* workLoop);

    //Status requests handlers
    void 						infoRequestHandler   		 	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    void 						infoRequestResultHandler	 	(xgi::Input* in, xgi::Output* out )  	throw (xgi::exception::Exception);
    bool                        infoRequestThread            	(toolbox::task::WorkLoop* workLoop);

    //External Supervisor XOAP handlers
    xoap::MessageReference 		supervisorCookieCheck 		 	(xoap::MessageReference msg) 			throw (xoap::exception::Exception);
    xoap::MessageReference 		supervisorGetActiveUsers	 	(xoap::MessageReference msg) 			throw (xoap::exception::Exception);
    xoap::MessageReference 		supervisorSystemMessage		 	(xoap::MessageReference msg) 			throw (xoap::exception::Exception);
    xoap::MessageReference 		supervisorGetUserInfo 		 	(xoap::MessageReference msg) 			throw (xoap::exception::Exception);
    xoap::MessageReference 		supervisorSystemLogbookEntry 	(xoap::MessageReference msg) 			throw (xoap::exception::Exception);
    xoap::MessageReference 		supervisorLastConfigGroupRequest(xoap::MessageReference msg) 			throw (xoap::exception::Exception);

    //Finite State Machine States
    void stateInitial    		(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void statePaused     		(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void stateRunning    		(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void stateHalted     		(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void stateConfigured 		(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    void inError         		(toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception);
    
    void transitionConfiguring 	(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionHalting     	(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionInitializing	(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionPausing     	(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionResuming    	(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionStarting    	(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void transitionStopping    	(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
    void enteringError         	(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);

    void getSupervisorsStatus  	(void) throw (toolbox::fsm::exception::Exception);

    void makeSystemLogbookEntry (std::string entryText);

   // void simpleFunction () { std::cout << __COUT_HDR_FL__ << "hi\n" << std::endl;}

private:
    unsigned int 														getNextRunNumber(const std::string &fsmName = "");
    bool         														setNextRunNumber(unsigned int runNumber, const std::string &fsmName = "");
    static std::pair<std::string /*group name*/, ConfigurationGroupKey>	loadGroupNameAndKey(const std::string &fileName, std::string &returnedTimeString);
    void																saveGroupNameAndKey(const std::pair<std::string /*group name*/,	ConfigurationGroupKey> &theGroup, const std::string &fileName);
    static xoap::MessageReference 										lastConfigGroupRequestHandler(const SOAPParameters &parameters);

    bool         														broadcastMessage(xoap::MessageReference msg);

    bool								supervisorGuiHasBeenLoaded_	; //use to indicate first access by user of ots since execution

    //Variables
    std::string                         outputDir_                  ;
    SupervisorDescriptorInfo            theSupervisorDescriptorInfo_;
    SupervisorsInfo                     theSupervisorsInfo_         ;
    ConfigurationManager*               theConfigurationManager_    ;
    WebUsers 						    theWebUsers_                ;
    SystemMessenger				        theSysMessenger_            ;
    
    WorkLoopManager                     stateMachineWorkLoopManager_;
    toolbox::BSem                      	stateMachineSemaphore_      ;
    WorkLoopManager                    	infoRequestWorkLoopManager_ ;
    toolbox::BSem                  		infoRequestSemaphore_       ;

    std::string 						supervisorContextUID_		;
    std::string 						supervisorApplicationUID_	;

    std::string							activeStateMachineName_; //when multiple state machines, this is the name of the state machine which executed the configure transition
    std::string							activeStateMachineWindowName_;
    std::pair<std::string /*group name*/, ConfigurationGroupKey> theConfigurationGroup_; //used to track the active configuration group at states after the configure state

    //Trash tests
    void wait(int milliseconds, std::string who="") const;
    unsigned long long 		counterTest_ ;
    std::vector<int>   		vectorTest_  ;
    std::string 	 		securityType_;

};

}

#endif

