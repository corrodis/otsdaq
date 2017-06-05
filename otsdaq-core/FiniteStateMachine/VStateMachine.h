#ifndef _ots_VStateMachine_h_
#define _ots_VStateMachine_h_

#include <string>

namespace ots
{

class VStateMachine
{
public:

	VStateMachine (void){;}
	virtual ~VStateMachine(void){;}

	//Transitions
    virtual void configure (void) = 0;
    virtual void halt      (void) = 0;
    virtual void pause     (void) = 0;
    virtual void resume    (void) = 0;
    virtual void start     (std::string runNumber) = 0;
    virtual void stop      (void) = 0;

    //States
    virtual bool running   (void){return false;}
    virtual void paused    (void){;}
    virtual void halted    (void){;}
    virtual void configured(void){;}
    virtual void initial   (void){;}
    virtual void inError   (void){;}


};

}

#endif
