#include "otsdaq/SystemMessenger/SystemMessenger.h"

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace ots;

//==============================================================================
// addSystemMessage
//	targetUser can be "*" for all users
void SystemMessenger::addSystemMessage(std::string targetUsersCSV, std::string msg, bool doEmail)
{
	std::vector<std::string> targetUsers;

	size_t pos = 0;

	while (pos != std::string::npos && pos < targetUsersCSV.size())
	{
		auto newpos = targetUsersCSV.find(',', pos);
		if (newpos != std::string::npos)
		{
			targetUsers.emplace_back(targetUsersCSV, pos, newpos - pos);
			//TLOG(TLVL_TRACE) << "tokenize_: " << targetUsers.back();
			pos = newpos + 1;
		}
		else
		{
			targetUsers.emplace_back(targetUsersCSV, pos);
			//TLOG(TLVL_TRACE) << "tokenize_: " << targetUsers.back();
			pos = newpos;
		}
	}

	addSystemMessage(targetUsers, msg, doEmail);
} //end addSystemMessage()
//==============================================================================
// addSystemMessage
//	targetUser can be "*" for all users
void SystemMessenger::addSystemMessage(std::vector<std::string> targetUsers, std::string msg, bool /*doEmail*/)
{
	sysMsgCleanup();

	std::lock_guard<std::mutex> lk(sysMsgLock_);
	auto now = time(0);

	// Reject messages already received
	if (sysMessages_.size() > 0) {
		for (auto& receivedMsg : (*sysMessages_.begin()).second) {
			if (receivedMsg.msg == msg) return;
		}
	}


	for (auto& targetUser : targetUsers)
	{
		sysMessages_[targetUser].emplace_back(targetUser, msg, now);
	}

	__COUT__ << "Current System Messages count = " << sysMessages_.size() << __E__;
} //end addSystemMessage()

//==============================================================================
// getSystemMessage
//	Deliver | separated system messages (time | msg | time | msg...etc),
//		if there is any in vector set for user or for wildcard *
//	Empty std::string "" returned if no message for targetUser
//	Note: | is an illegal character and will cause GUI craziness
std::string SystemMessenger::getSystemMessage(std::string targetUser)
{

	std::ostringstream strbuf;
	{
		std::lock_guard<std::mutex> lk(sysMsgLock_);
		// __COUT__ << "Current System Messages: " << targetUser <<
		// std::endl << std::endl;

		bool first = true;
		for (auto& msg : sysMessages_[targetUser]) {
			if (!first) strbuf << "|";
			strbuf << format_time_(msg.time) << "|" << msg.msg;
			first = false;
			msg.delivered = true;
		}
		for (auto& msg : sysMessages_["*"]) {
			if (!first) strbuf << "|";
			strbuf << format_time_(msg.time) << "|" << msg.msg;
			first = false;
		}
	}
	sysMsgCleanup();
	return strbuf.str();
}

//==============================================================================
// sysMsgCleanup
//	Cleanup messages if delivered, and targetUser != wildcard *
//	For all remaining messages, wait some time before removing (e.g. 30 sec)
void SystemMessenger::sysMsgCleanup()
{
	std::lock_guard<std::mutex> lk(sysMsgLock_);
	// __COUT__ << "Current System Messages: " <<
	// sysMsgTargetUser_.size() <<  std::endl << std::endl;
	time_t now = time(0);

	for (auto& user : sysMessages_) {
		for (auto it = user.second.begin(); it != user.second.end(); ++it) {
			if (it->delivered) it = user.second.erase(it);
			else if (it->time < now - SYS_CLEANUP_WILDCARD_TIME) it = user.second.erase(it);
		}
	}

	// __COUT__ << "Remaining System Messages: " <<
	// sysMsgTargetUser_.size() <<  std::endl << std::endl;
}
