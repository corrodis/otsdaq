#ifndef _ots_RunControlStateMachine_h_
#define _ots_RunControlStateMachine_h_

#include "otsdaq-core/FiniteStateMachine/FiniteStateMachine.h"
#include "otsdaq-core/ProgressBar/ProgressBar.h"

#include "toolbox/lang/Class.h"
#include <string>


namespace ots
{

class RunControlStateMachine : public virtual toolbox::lang::Class
{

public:

    RunControlStateMachine(std::string name="Undefined Name");
    virtual ~RunControlStateMachine(void);

    void reset(void);
    void setStateMachineName(std::string name){stateMachineName_ = name;}

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
    xoap::MessageReference runControlMessageHandler(xoap::MessageReference message) ;

    static const std::string FAILED_STATE_NAME;

protected:
	FiniteStateMachine theStateMachine_;
    ProgressBar		   theProgressBar_;
    std::string        stateMachineName_;

};

}

#endif

