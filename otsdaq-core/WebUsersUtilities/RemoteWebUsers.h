#ifndef _ots_Utilities_RemoteWebUsers_h
#define _ots_Utilities_RemoteWebUsers_h

#include "otsdaq-core/ConfigurationDataFormats/ConfigurationGroupKey.h"  //for ConfigurationGroupKey
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"                     //for xdaq::ApplicationDescriptor
#include "otsdaq-core/WebUsersUtilities/WebUsers.h"

#include <iostream>
#include <string>

namespace ots {

class AllSupervisorInfo;
class HttpXmlDocument;

// RemoteWebUsers
//	This class provides the functionality for client supervisors to check with the Gateway Supervisor
//	to verify user access. It also provides the functionality for client supervisors to retreive user info.
class RemoteWebUsers : public SOAPMessenger {
 public:
  RemoteWebUsers(xdaq::Application* application);

  // for external supervisors to check with Supervisor for login
  // if false, user request handling code should just return.. out is handled on false; on true, out is untouched
  bool xmlRequestToGateway(cgicc::Cgicc& cgi, std::ostringstream* out, HttpXmlDocument* xmldoc,
                           const AllSupervisorInfo& allSupervisorInfo, WebUsers::RequestUserInfo& userInfo);

  //			uint8_t* 						userPermissions = 0,
  //			const uint8_t					permissionsThreshold = 1,
  //			const bool						allowNoUser = false,
  //			const std::set<std::string>&	groupsAllowed = {},
  //			const std::set<std::string>&	groupsDisallowed = {},
  //			const bool						refreshCookie = true,
  //			const bool						checkLock = false,
  //			const bool						lockRequired = false,
  //			std::string* 					userWithLock = 0,
  //			std::string* 					username = 0,
  //			std::string* 					displayName = 0,
  //			std::string* 					userGroups = 0,
  //			uint64_t* 						activeSessionIndex = 0);

  std::string getActiveUserList(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor);
  void sendSystemMessage(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor, const std::string& toUser,
                         const std::string& msg);
  void makeSystemLogbookEntry(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor,
                              const std::string& entryText);
  std::pair<std::string /*group name*/, ConfigurationGroupKey> getLastConfigGroup(
      XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor, const std::string& actionOfLastGroup,
      std::string& actionTimeString);  // actionOfLastGroup = "Configured" or "Started", for example

 private:
  bool cookieCodeIsActiveForRequest(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor,
                                    std::string& cookieCode, uint8_t* userPermissions = 0, std::string ip = "0",
                                    bool refreshCookie = true, std::string* userWithLock = 0);

  bool getUserInfoForCookie(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor, std::string& cookieCode,
                            std::string* userName, std::string* displayName = 0, uint64_t* activeSessionIndex = 0);

  //"Active User List" associations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  std::string ActiveUserList_;
  time_t ActiveUserLastUpdateTime_;
  enum {
    ACTIVE_USERS_UPDATE_THRESHOLD = 10,  // 10 seconds, min amount of time between Supervisor requests
  };

  std::string tmpUserWithLock_, tmpUserGroups_, tmpUsername_;
  // uint8_t 	tmpUserPermissions_;
};

}  // namespace ots

#endif
