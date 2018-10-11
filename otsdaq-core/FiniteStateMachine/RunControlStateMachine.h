#ifndef _ots_RunControlStateMachine_h_
#define _ots_RunControlStateMachine_h_

#include "otsdaq-core/FiniteStateMachine/FiniteStateMachine.h"
#include "otsdaq-core/ProgressBar/ProgressBar.h"

#include "toolbox/lang/Class.h"
#include <string>


namespace ots
{

//CoreSupervisorBase
//This class provides finite state machine functionality for otsdaq supervisors.
class RunControlStateMachine : public virtual toolbox::lang::Class
{

public:

    RunControlStateMachine(std::string name="Undefined Name");
    virtual ~RunControlStateMachine(void);

    void reset(void);
    void setStateMachineName(std::string name){stateMachineName_ = name;}


    template <class OBJECT>
    void addStateTransition(
    		toolbox::fsm::State from,
			toolbox::fsm::State to,
			const std::string& input,
			const std::string& transitionName,
			OBJECT* obj,
			void (OBJECT::* func)(toolbox::Event::Reference) )

    {
    	stateTransitionFunctionTable_[from][input] = func;
    	theStateMachine_.addStateTransition(from, to, input, transitionName, obj, func);
    }

    template <class OBJECT>
    void addStateTransition(
    		toolbox::fsm::State from,
			toolbox::fsm::State to,
			const std::string& input,
			const std::string& transitionName,
			const std::string& transitionParameter,
			OBJECT* obj,
			void (OBJECT::* func)(toolbox::Event::Reference) )

    {
    	stateTransitionFunctionTable_[from][input] = func;
    	theStateMachine_.addStateTransition(from, to, input, transitionName, transitionParameter, obj, func);
    }

    //using	stateMachineFunction_t = void (ots::RunControlStateMachine::* )(toolbox::Event::Reference);
    //stateMachineFunction_t getTransitionFunction	(const toolbox::fsm::State from, const std::string &transition);





    //Finite State Machine States
    //1. Control Configuration and Function Manager are loaded.
    virtual void stateInitial (toolbox::fsm::FiniteStateMachine& fsm) {;}

    //1. XDAQ applications are running.
    //2. Hardware is running.
    //3. Triggers are accepted.
    //4. Triggers are not sent.
    //5. Data is sent / read out.
    virtual void statePaused (toolbox::fsm::FiniteStateMachine& fsm) {;}

    //1. XDAQ applications are running.
    //2. Hardware is running.
    //3. Triggers are accepted.
    //4. Triggers are sent.
    //5. Data is sent / read out.
    virtual void stateRunning (toolbox::fsm::FiniteStateMachine& fsm) {;}

    //1. Control hierarchy is instantiated.
    //2. XDAQ executives are running and configured.
    //3. XDAQ applications are loaded and instantiated.
    //4. DCS nodes are allocated.
    virtual void stateHalted (toolbox::fsm::FiniteStateMachine& fsm) {;}

    //1. Power supplies are turned off.
    virtual void stateShutdown (toolbox::fsm::FiniteStateMachine& fsm) {;}

    //1. XDAQ applications are configured.
    //2. Run parameters have been distributed.
    //3. Hardware is configured.
    //4. I2O connections are established, no data is sent or read out.
    //5. Triggers are not sent.
    virtual void stateConfigured  (toolbox::fsm::FiniteStateMachine& fsm) {;}

    virtual void inError          (toolbox::fsm::FiniteStateMachine& fsm) {;}

    virtual void transitionConfiguring (toolbox::Event::Reference e) {;}
    virtual void transitionHalting     (toolbox::Event::Reference e) {;}
    virtual void transitionShuttingDown(toolbox::Event::Reference e) {;}
    virtual void transitionStartingUp  (toolbox::Event::Reference e) {;}
    virtual void transitionInitializing(toolbox::Event::Reference e) {;}
    virtual void transitionPausing     (toolbox::Event::Reference e) {;}
    virtual void transitionResuming    (toolbox::Event::Reference e) {;}
    virtual void transitionStarting    (toolbox::Event::Reference e) {;}
    virtual void transitionStopping    (toolbox::Event::Reference e) {;}
    virtual void enteringError         (toolbox::Event::Reference e) {;}

    //Run Control Messages
    xoap::MessageReference 				runControlMessageHandler(xoap::MessageReference message) ;

    static const std::string FAILED_STATE_NAME;


    unsigned int						getIterationIndex			(void) { return iterationIndex_;}
    void 								indicateStillWorking		(void) { stillWorking_ = true; }
    void 								clearStillWorking			(void) { stillWorking_ = false; }
    bool								getStillWorking				(void) { return stillWorking_; }


protected:
	FiniteStateMachine 		theStateMachine_;
    ProgressBar		   		theProgressBar_;
    std::string        		stateMachineName_;


	unsigned int			iterationIndex_;
	bool					stillWorking_;

	toolbox::fsm::State		lastIterationState_;
	std::string				lastIterationCommand_;

	std::map<toolbox::fsm::State, std::map<std::string,
			void (RunControlStateMachine::*)(toolbox::Event::Reference), std::less<std::string> > >	stateTransitionFunctionTable_;

};

}

#endif

