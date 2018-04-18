#ifndef _ots_Utilities_RemoteWebUsers_h
#define _ots_Utilities_RemoteWebUsers_h

#include "xgi/Method.h" 								//for cgicc::Cgicc
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h" 	//for xdaq::ApplicationDescriptor
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h" //for ConfigurationGroupKey

#include <string>
#include <iostream>

//TODO -- include IP address handling!?

namespace ots
{

class AllSupervisorInfo;
class HttpXmlDocument;

class RemoteWebUsers : public SOAPMessenger
{
public:
	RemoteWebUsers(xdaq::Application *application);

	enum
	{
		COOKIE_CODE_LENGTH = 512,
	};

	static const std::string REQ_NO_LOGIN_RESPONSE;
	static const std::string REQ_NO_PERMISSION_RESPONSE;
	static const std::string REQ_USER_LOCKOUT_RESPONSE;
	static const std::string REQ_LOCK_REQUIRED_RESPONSE;
	static const std::string REQ_ALLOW_NO_USER;


	//for external supervisors to check with Supervisor for login
	//if false, user code should just return.. out is handled on false; on true, out is untouched
	bool			xmlLoginGateway(
			cgicc::Cgicc& 					cgi,
			std::ostringstream* 			out,
			HttpXmlDocument* 				xmldoc,
			const AllSupervisorInfo& 		allSupervisorInfo,
			uint8_t* 						userPermissions = 0,
			const bool						refresh = true,
			const uint8_t					permissionsThreshold = 1,
			const bool						checkLock = false,
			const bool						lockRequired = false,
			std::string* 					userWithLock = 0,
			std::string* 					username = 0,
			std::string* 					displayName = 0,
			uint64_t* 						activeSessionIndex = 0,
			const bool						allowNoUser = false);


	std::string															getActiveUserList					(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor);
	void																sendSystemMessage					(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor, const std::string &toUser, const std::string& msg);
	void																makeSystemLogbookEntry			   	(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor, const std::string &entryText);
	std::pair<std::string /*group name*/, ConfigurationGroupKey>		getLastConfigGroup					(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor, const std::string &actionOfLastGroup, std::string &actionTimeString); //actionOfLastGroup = "Configured" or "Started", for example

private:
	bool			cookieCodeIsActiveForRequest(
			XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor,
			std::string&                 cookieCode,
			uint8_t*                     userPermissions = 0,
			std::string                  ip = "0",
			bool                         refresh = true,
			std::string*                 userWithLock = 0);

	bool			getUserInfoForCookie(
			XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor,
			std::string&                 cookieCode,
			std::string*                 userName,
			std::string*                 displayName = 0,
			uint64_t*                    activeSessionIndex = 0);


	//"Active User List" associations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::string 				ActiveUserList_;
	time_t						ActiveUserLastUpdateTime_;
	enum
	{
		ACTIVE_USERS_UPDATE_THRESHOLD = 10, 		//10 seconds, min amount of time between Supervisor requests
	};

	std::string tmpUserWithLock_;
	uint8_t 	tmpUserPermissions_;
};


}

#endif
