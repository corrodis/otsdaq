#ifndef _ots_WorkLoop_h
#define _ots_WorkLoop_h

#include <toolbox/task/WorkLoop.h>
#include "toolbox/lang/Class.h"
#include <string>

namespace ots
{

class WorkLoop : public virtual toolbox::lang::Class
{
public:
             WorkLoop(std::string name);
    virtual ~WorkLoop(void);

    void startWorkLoop (void);
    bool stopWorkLoop  (void);

protected:
    volatile bool continueWorkLoop_;
    virtual bool workLoopThread(toolbox::task::WorkLoop* workLoop) = 0;

    //Getters
    const std::string& getWorkLoopName(void);

private:
    const std::string               cWorkLoopName_;
    const std::string               cWorkLoopType_;
    toolbox::task::WorkLoop*        workLoop_;
    toolbox::task::ActionSignature* job_;
};

}

#endif
