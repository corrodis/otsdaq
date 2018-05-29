#ifndef _ots_Utilities_SystemMessenger_h
#define _ots_Utilities_SystemMessenger_h

#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include <string>
#include <vector>
#include <iostream>

namespace ots
{
    
class SystemMessenger
{
public:

	SystemMessenger() : sysMsgLock_(false) {}; //constructor
	
    void 						addSystemMessage(std::string targetUser, std::string msg);
    std::string 				getSystemMessage(std::string targetUser);

private:

    //Members for system messages ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //	Set of vectors to delivers system messages to active users of the Web Gui
    //		When a message is generated, sysMsgLock is set,
    //			message is added the vector set sysMsgDelivered_[i] = false,
    //			and sysMsgLock is unset.
    //		When a message is delivered sysMsgDelivered_[i] = true,
    //		During sysMsgCleanup(), sysMsgLock is set, delivered messages are removed,
    //			and sysMsgLock is unset.
    std::vector<std::string>   	sysMsgTargetUser_;
    std::vector<std::string>   	sysMsgMessage_;
    std::vector<time_t>   		sysMsgTime_;
    std::vector<bool>   		sysMsgDelivered_;
    void 						sysMsgSetLock(bool set);
    void 						sysMsgCleanup();

    volatile bool				sysMsgLock_;

    enum {
       	SYS_CLEANUP_WILDCARD_TIME = 30, //30 seconds
    };
};

	
}

#endif
