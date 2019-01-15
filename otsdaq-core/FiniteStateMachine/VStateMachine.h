#ifndef _ots_VStateMachine_h_
#define _ots_VStateMachine_h_

#include <string>

namespace ots
{

class CoreSupervisorBase;

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


    void		 						setIterationIndex			(unsigned int i) { iterationIndex_ = i; }
    void		 						setSubIterationIndex		(unsigned int i) { subIterationIndex_ = i; }
    unsigned int 						getIterationIndex			(void) { return iterationIndex_; }
    unsigned int 						getSubIterationIndex		(void) { return subIterationIndex_; }
    void 		 						indicateIterationWork		(void) { iterationWorkFlag_ = true; }
    void 		 						clearIterationWork			(void) { iterationWorkFlag_ = false; }
    bool		 						getIterationWork			(void) { return iterationWorkFlag_; }
    void 								indicateSubIterationWork	(void) { subIterationWorkFlag_ = true; }
    void 								clearSubIterationWork		(void) { subIterationWorkFlag_ = false; }
    bool								getSubIterationWork			(void) { return subIterationWorkFlag_; }


    CoreSupervisorBase* parentSupervisor_; //e.g. to communicate error fault and start transition to error for entire system

private:
    unsigned int iterationIndex_, subIterationIndex_;
    bool		 iterationWorkFlag_, subIterationWorkFlag_;


};

}

#endif
