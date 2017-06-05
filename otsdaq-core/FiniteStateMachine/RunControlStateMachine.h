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
    virtual void stateInitial (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception){;}

    //1. XDAQ applications are running.
    //2. Hardware is running.
    //3. Triggers are accepted.
    //4. Triggers are not sent.
    //5. Data is sent / read out.
    virtual void statePaused (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception){;}

    //1. XDAQ applications are running.
    //2. Hardware is running.
    //3. Triggers are accepted.
    //4. Triggers are sent.
    //5. Data is sent / read out.
    virtual void stateRunning (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception){;}

    //1. Control hierarchy is instantiated.
    //2. XDAQ executives are running and configured.
    //3. XDAQ applications are loaded and instantiated.
    //4. DCS nodes are allocated.
    virtual void stateHalted (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception){;}

    //1. XDAQ applications are configured.
    //2. Run parameters have been distributed.
    //3. Hardware is configured.
    //4. I2O connections are established, no data is sent or read out.
    //5. Triggers are not sent.
    virtual void stateConfigured  (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception){;}

    virtual void inError          (toolbox::fsm::FiniteStateMachine& fsm) throw (toolbox::fsm::exception::Exception){;}

    virtual void transitionConfiguring (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception){;}
    virtual void transitionHalting     (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception){;}
    virtual void transitionInitializing(toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception){;}
    virtual void transitionPausing     (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception){;}
    virtual void transitionResuming    (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception){;}
    virtual void transitionStarting    (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception){;}
    virtual void transitionStopping    (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception){;}
    virtual void enteringError         (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception){;}

    //Run Control Messages
    xoap::MessageReference runControlMessageHandler(xoap::MessageReference message) throw (xoap::exception::Exception);

protected:
    FiniteStateMachine theStateMachine_;
    ProgressBar		   theProgressBar_;
    std::string        stateMachineName_;
};

}

#endif

