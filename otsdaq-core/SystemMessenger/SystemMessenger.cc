#include "otsdaq-core/SystemMessenger/SystemMessenger.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

using namespace ots;



//========================================================================================================================
//addSystemMessage
void SystemMessenger::addSystemMessage(std::string targetUser, std::string msg)
{
	sysMsgCleanup();

	//reject if same message is already in vector set
	//for(uint64_t i=0;i<sysMsgTargetUser_.size();++i)
	//if(sysMsgTargetUser_[i] == targetUser && sysMsgMessage_[i] == msg) return;
	// reject only if last message
	if(sysMsgTargetUser_.size() && sysMsgTargetUser_[sysMsgTargetUser_.size()-1] == targetUser && sysMsgMessage_[sysMsgTargetUser_.size()-1] == msg) return;

	sysMsgSetLock(true);	//set lock
	sysMsgTargetUser_.push_back(targetUser);
	sysMsgMessage_.push_back(msg);
	sysMsgTime_.push_back(time(0));
	sysMsgDelivered_.push_back(false);
	sysMsgSetLock(false);	//unset lock

	std::cout << __COUT_HDR_FL__ << "Current System Messages: " << sysMsgTargetUser_.size() <<  std::endl << std::endl;
}

//========================================================================================================================
//getSystemMessage
//	Deliver | separated system messages (time | msg | time | msg...etc),
//		if there is any in vector set for user or for wildcard *
//	Empty std::string "" returned if no message for targetUser
//	Note: | is an illegal character and will cause GUI craziness
std::string SystemMessenger::getSystemMessage(std::string targetUser)
{
	//std::cout << __COUT_HDR_FL__ << "Current System Messages: " << targetUser <<  std::endl << std::endl;
	std::string retStr = "";
	int cnt = 0;
	char tmp[100];
	for(uint64_t i=0;i<sysMsgTargetUser_.size();++i)
		if( sysMsgTargetUser_[i] == targetUser || sysMsgTargetUser_[i] == "*")
		{
			//deliver system message
			if(cnt)
				retStr += "|";
			sprintf(tmp,"%lu",sysMsgTime_[i]);
			retStr += std::string(tmp) + "|" + sysMsgMessage_[i];

			if(sysMsgTargetUser_[i] != "*") //mark delivered
				sysMsgDelivered_[i] = true;
			++cnt;
		}

	sysMsgCleanup();
	return retStr;
}

//========================================================================================================================
//sysMsgSetLock
//	ALWAYS calling thread with true, must also call with false to release lock
void SystemMessenger::sysMsgSetLock(bool set)
{
	while(set && sysMsgLock_) usleep(1000); //wait for other thread to unlock
	sysMsgLock_ = set;
}

//========================================================================================================================
//sysMsgCleanup
//	Cleanup messages if delivered, and targetUser != wildcard *
//	For all remaining messages, wait some time before removing (e.g. 30 sec)
void SystemMessenger::sysMsgCleanup()
{
	//std::cout << __COUT_HDR_FL__ << "Current System Messages: " << sysMsgTargetUser_.size() <<  std::endl << std::endl;
	for(uint64_t i=0;i<sysMsgTargetUser_.size();++i)
		if((sysMsgDelivered_[i] && sysMsgTargetUser_[i] != "*") ||  //delivered and != *
				sysMsgTime_[i] + SYS_CLEANUP_WILDCARD_TIME < time(0)) //expired
		{
			//remove
			sysMsgSetLock(true);	//set lock
			sysMsgTargetUser_.erase (sysMsgTargetUser_.begin()+i);
			sysMsgMessage_.erase (sysMsgMessage_.begin()+i);
			sysMsgTime_.erase (sysMsgTime_.begin()+i);
			sysMsgDelivered_.erase (sysMsgDelivered_.begin()+i);
			sysMsgSetLock(false);	//unset lock
			--i; //rewind
		}
	//std::cout << __COUT_HDR_FL__ << "Remaining System Messages: " << sysMsgTargetUser_.size() <<  std::endl << std::endl;
}
