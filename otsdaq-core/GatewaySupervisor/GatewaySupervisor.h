#ifndef _ots_GatewaySupervisor_h
#define _ots_GatewaySupervisor_h

#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"
#include "otsdaq-core/WebUsersUtilities/WebUsers.h"
#include "otsdaq-core/SystemMessenger/SystemMessenger.h"
#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/FiniteStateMachine/RunControlStateMachine.h"
#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"
#include "otsdaq-core/GatewaySupervisor/Iterator.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"
#include "otsdaq-core/CoreSupervisors/CorePropertySupervisorBase.h"
#include "otsdaq-core/GatewaySupervisor/ARTDAQCommandable.h"

#include "otsdaq-core/CodeEditor/CodeEditor.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <xdaq/Application.h>
#pragma GCC diagnostic pop
#include "otsdaq-core/Macros/XDAQApplicationMacros.h"
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

//GatewaySupervisor
//	This class is the gateway server for all otsdaq requests in "Normal Mode." It validates user access
//	for every request. It synchronizes
//	the state machines of all other supervisors.
class GatewaySupervisor: public xdaq::Application, public SOAPMessenger,
	public RunControlStateMachine, public CorePropertySupervisorBase
{
	friend class WizardSupervisor;
	friend class Iterator;
	friend class ARTDAQCommandable;

public:

    XDAQ_INSTANTIATOR();

    GatewaySupervisor (xdaq::ApplicationStub * s);
    virtual ~GatewaySupervisor(void);

    void 						init        				 	(void);

    void 						Default     				 	(xgi::Input* in, xgi::Output* out ) 	;
//    void 						TmpTest     				 	(xgi::Input* in, xgi::Output* out ) 	;

    void 						loginRequest 				 	(xgi::Input* in, xgi::Output* out )  	;
    void 						request                      	(xgi::Input* in, xgi::Output* out )  	;
    void 						tooltipRequest              	(xgi::Input* in, xgi::Output* out )  	;

    //State Machine requests handlers

    void 						stateMachineXgiHandler       	(xgi::Input* in, xgi::Output* out )		;
    //void 						stateMachineResultXgiHandler 	(xgi::Input* in, xgi::Output* out )  	;

    xoap::MessageReference 		stateMachineXoapHandler      	(xoap::MessageReference msg )  	        ;
    xoap::MessageReference 		stateMachineResultXoapHandler	(xoap::MessageReference msg )  	        ;

    bool                        stateMachineThread           	(toolbox::task::WorkLoop* workLoop)		;

    //Status requests handlers
    void 						infoRequestHandler   		 	(xgi::Input* in, xgi::Output* out )  	;
    void 						infoRequestResultHandler	 	(xgi::Input* in, xgi::Output* out )  	;
    bool                        infoRequestThread            	(toolbox::task::WorkLoop* workLoop)		;

    //External GatewaySupervisor XOAP handlers
    xoap::MessageReference 		supervisorCookieCheck 		 	(xoap::MessageReference msg) 			;
    xoap::MessageReference 		supervisorGetActiveUsers	 	(xoap::MessageReference msg) 			;
    xoap::MessageReference 		supervisorSystemMessage		 	(xoap::MessageReference msg) 			;
    xoap::MessageReference 		supervisorGetUserInfo 		 	(xoap::MessageReference msg) 			;
    xoap::MessageReference 		supervisorSystemLogbookEntry 	(xoap::MessageReference msg) 			;
    xoap::MessageReference 		supervisorLastConfigGroupRequest(xoap::MessageReference msg) 			;

    //Finite State Machine States
    void stateInitial    		(toolbox::fsm::FiniteStateMachine& fsm) ;
    void statePaused     		(toolbox::fsm::FiniteStateMachine& fsm) ;
    void stateRunning    		(toolbox::fsm::FiniteStateMachine& fsm) ;
    void stateHalted     		(toolbox::fsm::FiniteStateMachine& fsm) ;
    void stateConfigured 		(toolbox::fsm::FiniteStateMachine& fsm) ;
    void inError         		(toolbox::fsm::FiniteStateMachine& fsm) ;

    void transitionConfiguring 	(toolbox::Event::Reference e) ;
    void transitionHalting     	(toolbox::Event::Reference e) ;
    void transitionInitializing	(toolbox::Event::Reference e) ;
    void transitionPausing     	(toolbox::Event::Reference e) ;
    void transitionResuming    	(toolbox::Event::Reference e) ;
    void transitionStarting    	(toolbox::Event::Reference e) ;
    void transitionStopping    	(toolbox::Event::Reference e) ;
    void transitionShuttingDown (toolbox::Event::Reference e) ;
    void transitionStartingUp   (toolbox::Event::Reference e) ;
    void enteringError         	(toolbox::Event::Reference e) ;

    void makeSystemLogbookEntry (std::string entryText);

    void checkForAsyncError		(void);

    //CorePropertySupervisorBase override functions
    virtual void			setSupervisorPropertyDefaults	(void) override; //override to control supervisor specific defaults
    virtual void			forceSupervisorPropertyValues	(void) override; //override to force supervisor property values (and ignore user settings)


private:
    unsigned int 														getNextRunNumber					(const std::string &fsmName = "");
    bool         														setNextRunNumber					(unsigned int runNumber, const std::string &fsmName = "");
    static std::pair<std::string /*group name*/, ConfigurationGroupKey>	loadGroupNameAndKey					(const std::string &fileName, std::string &returnedTimeString);
    void																saveGroupNameAndKey					(const std::pair<std::string /*group name*/,	ConfigurationGroupKey> &theGroup, const std::string &fileName);
    static xoap::MessageReference 										lastConfigGroupRequestHandler		(const SOAPParameters &parameters);
    void																launchStartOTSCommand				(const std::string& command);

    static void															StateChangerWorkLoop				(GatewaySupervisor *supervisorPtr);
    std::string															attemptStateMachineTransition		(HttpXmlDocument* xmldoc, std::ostringstream* out, const std::string& command, const std::string& fsmName, const std::string& fsmWindowName, const std::string& username, const std::vector<std::string>& parameters);
    bool         														broadcastMessage					(xoap::MessageReference msg);




    bool								supervisorGuiHasBeenLoaded_	; //use to indicate first access by user of ots since execution

    //Member Variables

    //AllSupervisorInfo                   allSupervisorInfo_         	;
   // ConfigurationManager*               theConfigurationManager_    ;
    WebUsers 						    theWebUsers_                ;
    SystemMessenger				        theSystemMessenger_         ;
	ARTDAQCommandable					theArtdaqCommandable_;

    WorkLoopManager                     stateMachineWorkLoopManager_;
    toolbox::BSem                      	stateMachineSemaphore_      ;
    WorkLoopManager                    	infoRequestWorkLoopManager_ ;
    toolbox::BSem                  		infoRequestSemaphore_       ;
//
//    std::string 						supervisorContextUID_		; //now comes from CorePropertySup
//    std::string 						supervisorApplicationUID_	;

    std::string							activeStateMachineName_			; //when multiple state machines, this is the name of the state machine which executed the configure transition
    std::string							activeStateMachineWindowName_	;
    std::pair<std::string /*group name*/, ConfigurationGroupKey> theConfigurationGroup_; //used to track the active configuration group at states after the configure state

    Iterator							theIterator_					;
    std::mutex							stateMachineAccessMutex_		; //for sharing state machine access with iterator thread
    std::string							stateMachineLastCommandInput_	;

    enum {
    	VERBOSE_MUTEX = 0
    };

    CodeEditor 							codeEditor_;

    //temporary member variable to avoid redeclaration in repetitive functions
    char								tmpStringForConversions_[100];

    //Trash tests
    void wait(int milliseconds, std::string who="") const;
    unsigned long long 		counterTest_ ;
    std::vector<int>   		vectorTest_  ;
    std::string 	 		securityType_;

};

}

#endif

