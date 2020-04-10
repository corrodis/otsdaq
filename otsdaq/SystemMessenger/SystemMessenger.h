#ifndef _ots_Utilities_SystemMessenger_h
#define _ots_Utilities_SystemMessenger_h

#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/MessageFacility/MessageFacility.h"

#include <iostream>
#include <string>
#include <vector>
#include <mutex>

namespace ots
{
	class SystemMessenger
	{
	public:
		SystemMessenger() : sysMessages_() {};  // constructor

		void        addSystemMessage(std::string targetUsersCSV, std::string msg, bool doEmail);
		void        addSystemMessage(std::vector<std::string> targetUsers, std::string msg, bool doEmail);
		std::string getSystemMessage(std::string targetUser);

	private:
		// Members for system messages ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//	Set of vectors to delivers system messages to active users of the Web Gui
		//		When a message is generated, sysMsgLock is set,
		//			message is added the vector set sysMsgDelivered_[i] = false,
		//			and sysMsgLock is unset.
		//		When a message is delivered sysMsgDelivered_[i] = true,
		//		During sysMsgCleanup(), sysMsgLock is set, delivered messages are removed,
		//			and sysMsgLock is unset.

		struct SysMessageData {
			std::string targetUser;
			std::string msg;
			time_t time;
			bool delivered;

			SysMessageData(std::string user, std::string m, time_t t) : targetUser(user), time(t), delivered(false) {
				while (m.find("|") != std::string::npos) {
					m.replace(m.find("|"), 1, ",");
				}
				msg = m;
			}
		};

		std::string format_time_(time_t time) {
			size_t constexpr SIZE{ 144 };
			struct tm timebuf;
			char ts[SIZE];
			strftime(ts, sizeof(ts), "%d-%b-%Y %H:%M:%S %Z", localtime_r(&time, &timebuf));
			return std::string(ts);
		}

		std::unordered_map<std::string /*targetUser*/, std::list<SysMessageData>> sysMessages_;

		void                     sysMsgCleanup();

		std::mutex sysMsgLock_;

		enum
		{
			SYS_CLEANUP_WILDCARD_TIME = 30,  // 30 seconds
		};
	};

}  // namespace ots

#endif
