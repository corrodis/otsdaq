#include "otsdaq-core/WebUsersUtilities/RemoteWebUsers.h"

#include "otsdaq-core/SOAPUtilities/SOAPParameters.h"	//must include in .h for static function
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"

#include <cstdlib>
#include <cstdio>
#include <vector>

#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"


using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "RemoteWebUsers"

//========================================================================================================================
//User Notes:
//	- use xmlRequestGateway to check security from outside the Supervisor and Wizard
//
//	Example usage:
//
//
//
//			void exampleClass::exampleRequestHandler(xgi::Input * in, xgi::Output * out)
//			throw (xgi::exception::Exception)
//			{
//				cgicc::Cgicc cgi(in);
//
//				//...
//
//				HttpXmlDocument xmldoc;
//				std::string userWithLock, userName, displayName;
//				uint64_t activeSessionIndex;
//				uint8_t userPermissions;
//
//				//**** start LOGIN GATEWAY CODE ***//
//				//check cookieCode, sequence, userWithLock, and permissions access all in one shot!
//				{
//					bool automaticCommand = 0; //automatic commands should not refresh cookie code.. only user initiated commands should!
//					bool checkLock = true;
//					bool lockRequired = true;
//
//					if(!theRemoteWebUsers_.xmlRequestToGateway(
//							cgi,out,&xmldoc,theSupervisorsConfiguration_
//							,&userPermissions  			//acquire user's access level (optionally null pointer)//
//							,!automaticCommand			//true/false refresh cookie code
//							,USER_PERMISSIONS_THRESHOLD //set access level requirement to pass gateway
//							,checkLock					//true/false enable check that system is unlocked or this user has the lock
//							,lockRequired				//true/false requires this user has the lock to proceed
//							,&userWithLock				//acquire username with lock (optionally null pointer)
//							,&userName					//acquire username of this user (optionally null pointer)
//							,0//,&displayName			//acquire user's Display Name
//							,0//,&activeSessionIndex	//acquire user's session index associated with the cookieCode
//							))
//					{	//failure
//						//std::cout << out->str() << std::endl; //could print out return string on failure
//						return;
//					}
//				}
//				//done checking cookieCode, sequence, userWithLock, and permissions access all in one shot!
//				//**** end LOGIN GATEWAY CODE ***//
//
//				//Success! if here.
//				//
//				//... use acquired values below
//				//...
//
//				//add to xml document, for example:
//				//DOMElement* parentEl;
//				//parentEl = xmldoc.addTextElementToData("ExampleTag", "parent-data");
//				//xmldoc.addTextElementToParent("ExampleChild", "child-data", parentEl);
//
//				//return xml doc holding server response
//				//xmldoc.outputXmlDocument((std::ostringstream*) out, true); //true to also print to std::cout
//			}
//
//
//========================================================================================================================




RemoteWebUsers::RemoteWebUsers(xdaq::Application* application)
: SOAPMessenger  (application)
{
	ActiveUserLastUpdateTime_ = 0; //init to never
	ActiveUserList_ = ""; //init to empty
}

//========================================================================================================================
//xmlRequestGateway
//	if false, user code should just return.. out is handled on false; on true, out is untouched
bool RemoteWebUsers::xmlRequestToGateway(
		cgicc::Cgicc& 					cgi,
		std::ostringstream* 			out,
		HttpXmlDocument* 				xmldoc,
		const AllSupervisorInfo& 		allSupervisorInfo,
		WebUsers::RequestUserInfo&		userInfo	)
//		uint8_t* 						userPermissions,
//		const uint8_t 					permissionsThreshold,
//		const bool						allowNoUser,
//		const std::set<std::string>&	groupsAllowed,
//		const std::set<std::string>& 	groupsDisallowed,
//		const bool						refreshCookie,
//		const bool						checkLock,
//		const bool						lockRequired,
//		std::string* 					userWithLock,
//		std::string* 					userName,
//		std::string* 					displayName,
//		std::string* 					userGroups,
//		uint64_t* 						activeSessionIndex
//		)
{
	//initialize user info parameters to failed results
	WebUsers::initializeRequestUserInfo(cgi,userInfo);
//
//	userInfo.permissionLevel_ = 0; //init to inactive
//	if(userInfo.checkLock_ || userInfo.requireLock_)	userInfo.usernameWithLock_       = "";
//	if(userInfo.userName)			*userName           = "";
//	if(userInfo.displayName)			*displayName        = "";
//	if(userInfo.userGroups)			*userGroups         = "";
//	if(userInfo.activeSessionIndex)	*activeSessionIndex = -1;
//
//	const std::string& ip = cgi.getEnvironment().getRemoteAddr();


	//const_cast away the const
	//	so that this line is compatible with slf6 and slf7 versions of xdaq
	//	where they changed to XDAQ_CONST_CALL xdaq::ApplicationDescriptor* in slf7
	//
	// XDAQ_CONST_CALL is defined in "otsdaq-core/Macros/CoutMacros.h"
	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* gatewaySupervisor;

	SOAPParameters               parameters;
	xoap::MessageReference       retMsg;

	//**** start LOGIN GATEWAY CODE ***//
	//If TRUE, cookie code is good, and refreshed code is in cookieCode
	//Else, error message is returned in cookieCode
	//tmpCookieCode_ = CgiDataUtilities::getOrPostData(cgi,"CookieCode"); //from GET or POST

	//	__COUT__ << cookieCode.length() << std::endl;
	//	__COUT__ << "cookieCode=" << cookieCode << std::endl;
	//	__COUT__ << std::endl;

	/////////////////////////////////////////////////////
	//have CookieCode, try it out
	if(allSupervisorInfo.isWizardMode())
	{
		//if missing CookieCode... check if in Wizard mode and using sequence
		std::string sequence = CgiDataUtilities::getOrPostData(cgi,"sequence"); //from GET or POST
		//__COUT__ << "sequence=" << sequence << std::endl;
		if(!sequence.length())
		{
			__COUT__ << "Invalid attempt (@" << userInfo.ip_ << ")." << std::endl;
			*out << WebUsers::REQ_NO_LOGIN_RESPONSE;
			//invalid cookie and also invalid sequence
			goto HANDLE_ACCESS_FAILURE; //return false, access failed
		}

		//have sequence, try it out

		gatewaySupervisor =	allSupervisorInfo.getWizardInfo().getDescriptor();
		if(!gatewaySupervisor)
		{
			__COUT_ERR__ << "Missing wizard supervisor." << std::endl;
			*out << WebUsers::REQ_NO_LOGIN_RESPONSE;
			//sequence code present, but no wizard supervisor
			goto HANDLE_ACCESS_FAILURE; //return false, access failed
		}

		parameters.addParameter("sequence",sequence);
		parameters.addParameter("IPAddress",userInfo.ip_);
		retMsg = SOAPMessenger::sendWithSOAPReply(gatewaySupervisor,
				"SupervisorSequenceCheck", parameters);
		parameters.clear();
		parameters.addParameter("Permissions");
		receive(retMsg, parameters);

		//sscanf(parameters.getValue("Permissions").c_str(),"%hhu",&userInfo.permissionLevel_); //hhu := unsigned char


		if(WebUsers::checkRequestAccess(cgi,out,xmldoc,userInfo,true /*isWizardMode*/))
			return true;
		else
			goto HANDLE_ACCESS_FAILURE; //return false, access failed

//		if(userInfo.permissionLevel_ < userInfo.permissionsThreshold_)
//		{
//			*out << WebUsers::REQ_NO_LOGIN_RESPONSE;
//			__COUT__ << "User (@" << userInfo.ip_ << ") has insufficient permissions: " << userInfo.permissionLevel_ << "<" <<
//					userInfo.permissionsThreshold_ << std::endl;
//			return false;	//invalid cookie and present sequence, but not correct sequence
//		}
//
//		userInfo.setUsername("admin");
//		userInfo.setDisplayName("Admin");
//		userInfo.setUsernameWithLock("admin");
//		userInfo.setActiveUserSessionIndex(0);
//		userInfo.setGroupMemebership("admin");
//
//		return true; //successful sequence login!
	}

	//else proceed with inquiry to Gateway Supervisor

	gatewaySupervisor = allSupervisorInfo.getGatewayInfo().getDescriptor();

	if(!gatewaySupervisor)
	{
		__COUT_ERR__ << "Missing gateway supervisor." << std::endl;
		*out << WebUsers::REQ_NO_LOGIN_RESPONSE;
		goto HANDLE_ACCESS_FAILURE; //return false, access failed
	}

	//__COUT__ << std::endl;

	parameters.clear();
	parameters.addParameter("CookieCode",userInfo.cookieCode_);
	parameters.addParameter("RefreshOption",userInfo.automatedCommand_?"0":"1");
	parameters.addParameter("IPAddress",userInfo.ip_);

	retMsg = SOAPMessenger::sendWithSOAPReply(gatewaySupervisor,
			"SupervisorCookieCheck", parameters);

	parameters.clear();
	parameters.addParameter("CookieCode");
	parameters.addParameter("Permissions");
	parameters.addParameter("UserGroups");
	parameters.addParameter("UserWithLock");
	parameters.addParameter("Username");
	parameters.addParameter("DisplayName");
	parameters.addParameter("ActiveSessionIndex");
	receive(retMsg, parameters);

	//first extract a few things always from parameters
	//	like permissionLevel for this request... must consider allowed groups!!
	userInfo.setGroupPermissionLevels(parameters.getValue("Permissions"));
	userInfo.cookieCode_ = parameters.getValue("CookieCode");
	userInfo.username_ = parameters.getValue("Username");
	userInfo.displayName_ = parameters.getValue("DisplayName");
	userInfo.usernameWithLock_ = parameters.getValue("UserWithLock");
	userInfo.activeUserSessionIndex_ = strtoul(parameters.getValue("ActiveSessionIndex").c_str(),0,0);

	if(!WebUsers::checkRequestAccess(cgi,out,xmldoc,userInfo))
		goto HANDLE_ACCESS_FAILURE; //return false, access failed

//	if(!userInfo.checkLock_ && !userInfo.requireLock_)
//		return true; //done, no need to get user info for this cookie code
//

	/////////////////////////////////////////////////////
	//get user info for cookie code and check lock (now that username is available)

//	parameters.clear();
//	parameters.addParameter("CookieCode",userInfo.cookieCode_);
//	retMsg = SOAPMessenger::sendWithSOAPReply(gatewaySupervisor,
//			"SupervisorGetUserInfo", parameters);
//
//	parameters.clear();
//	parameters.addParameter("Username");
//	parameters.addParameter("DisplayName");
//	parameters.addParameter("ActiveSessionIndex");
//	receive(retMsg, parameters);

//	if(WebUsers::finalizeRequestAccess(out,xmldoc,userInfo,
//			parameters.getValue("Username"),
//			parameters.getValue("DisplayName"),
//			strtoul(parameters.getValue("ActiveSessionIndex").c_str(),0,0)))
//		return true;
//	else
//		goto HANDLE_ACCESS_FAILURE; //return false, access failed

HANDLE_ACCESS_FAILURE:

	//print out return string on failure
	if(!userInfo.automatedCommand_)
		__COUT_ERR__ << "Failed request (requestType = " << userInfo.requestType_ <<
		"): " << out->str() << __E__;
	return false; //access failed

//	tmpUserWithLock_ = parameters.getValue("UserWithLock");
//	tmpUserGroups_ = parameters.getValue("UserGroups");
//	sscanf(parameters.getValue("Permissions").c_str(),"%hhu",&userInfo.permissionLevel_); //unsigned char
//	userInfo.setUsernameWithLock(parameters.getValue("UserWithLock"));
//	userInfo.setGroupMemebership(parameters.getValue("UserGroups"));
//	tmpCookieCode_ = parameters.getValue("CookieCode");



	//else access granted

	//__COUT__ << "cookieCode=" << cookieCode << std::endl;
//
//	if(!allowNoUser && cookieCode.length() != WebUsers::COOKIE_CODE_LENGTH)
//	{
//		__COUT__ << "User (@" << ip << ") has invalid cookie code: " << cookieCode << std::endl;
//		*out << WebUsers::REQ_NO_LOGIN_RESPONSE;
//		return false;	//invalid cookie and present sequence, but not correct sequence
//	}
//
//	if(!allowNoUser && tmpUserPermissions_ < permissionsThreshold)
//	{
//		*out << WebUsers::REQ_NO_PERMISSION_RESPONSE;
//		__COUT__ << "User (@" << ip << ") has insufficient permissions: " << tmpUserPermissions_ << "<" <<
//				permissionsThreshold << std::endl;
//		return false;
//	}

	//check group membership
//	if(!allowNoUser)
//	{
//		//check groups allowed
//		//	i.e. if user is a member of one of the groups allowed
//		//			then grant access
//
//		std::set<std::string> userMembership;
//		StringMacros::getSetFromString(
//				tmpUserGroups_,
//				userMembership);
//
//		bool accept = false;
//		for(const auto& userGroup:userMembership)
//			if(StringMacros::inWildCardSet(
//					userGroup,
//					groupsAllowed))
//			{
//				accept = true;
//				break;
//			}
//
//		if(!accept && userMembership.size())
//		{
//			*out << WebUsers::REQ_NO_PERMISSION_RESPONSE;
//			std::stringstream ss;
//			bool first = true;
//			for(const auto& group:groupsAllowed)
//				if(first && (first=false))
//					ss << group;
//				else
//					ss << " | " << group;
//
//			__COUT__ << "User (@" << ip << ") has insufficient group permissions: " << tmpUserGroups_ << " != " <<
//					ss.str() << std::endl;
//			return false;
//		}
//
//		//if no access groups specified, then check groups disallowed
//		if(!userMembership.size())
//		{
//			for(const auto& userGroup:userMembership)
//				if(StringMacros::inWildCardSet(
//						userGroup,
//						groupsDisallowed))
//				{
//					*out << WebUsers::REQ_NO_PERMISSION_RESPONSE;
//					std::stringstream ss;
//					bool first = true;
//					for(const auto& group:groupsDisallowed)
//						if(first && (first=false))
//							ss << group;
//						else
//							ss << " | " << group;
//
//					__COUT__ << "User (@" << ip << ") is in in a disallowed group permissions: " << tmpUserGroups_ << " == " <<
//							ss.str() << std::endl;
//					return false;
//				}
//		}
//	} //end group membership check
//
//	if(xmldoc) //fill with cookie code tag
//	{
//		if(!allowNoUser)
//			xmldoc->setHeader(cookieCode);
//		else
//			xmldoc->setHeader(WebUsers::REQ_ALLOW_NO_USER);
//	}
//
//
//
//	if(!userName && !displayName && !activeSessionIndex && !checkLock && !lockRequired)
//		return true; //done, no need to get user info for cookie
//
//	//__COUT__ << "User with Lock: " << tmpUserWithLock_ << std::endl;
//
//
//	/////////////////////////////////////////////////////
//	//get user info
//	parameters.clear();
//	parameters.addParameter("CookieCode",cookieCode);
//	retMsg = SOAPMessenger::sendWithSOAPReply(gatewaySupervisor,
//			"SupervisorGetUserInfo", parameters);
//
//	parameters.clear();
//	parameters.addParameter("Username");
//	parameters.addParameter("DisplayName");
//	parameters.addParameter("ActiveSessionIndex");
//	receive(retMsg, parameters);
//
//
//
//
//	userInfo.setActiveUserSessionIndex(0);
//
//
//	tmpUsername_ = parameters.getValue("Username");
//	userInfo.setUsername(tmpUsername_);
//	userInfo.setDisplayName(parameters.getValue("DisplayName"));
//	userInfo.setActiveUserSessionIndex(
//			strtoul(parameters.getValue("ActiveSessionIndex").c_str(),0,0));
//
//	if(userInfo.checkLock_ && tmpUserWithLock_ != "" && tmpUserWithLock_ != tmpUsername_)
//	{
//		*out << WebUsers::REQ_USER_LOCKOUT_RESPONSE;
//		__COUT__ << "User " << tmpUsername_ << " is locked out. " << tmpUserWithLock_ << " has lock." << std::endl;
//		return false;
//	}
//
//	if(userInfo.lockRequired_ && tmpUserWithLock_ != tmpUsername_)
//	{
//		*out << WebUsers::REQ_LOCK_REQUIRED_RESPONSE;
//		__COUT__ << "User " << tmpUsername_ << " must have lock to proceed. (" << tmpUserWithLock_ << " has lock.)" << std::endl;
//		return false;
//	}
//
//	return true;
}

//========================================================================================================================
//getActiveUserList
//	if lastUpdateTime is not too recent as spec'd by ACTIVE_USERS_UPDATE_THRESHOLD
//	if server responds with
std::string RemoteWebUsers::getActiveUserList(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor)
{

	if(1 || time(0) - ActiveUserLastUpdateTime_ > ACTIVE_USERS_UPDATE_THRESHOLD) //need to update
	{

		__COUT__ << "Need to update " << std::endl;

		xoap::MessageReference retMsg = ots::SOAPMessenger::sendWithSOAPReply(supervisorDescriptor,"SupervisorGetActiveUsers");


		SOAPParameters retParameters("UserList");
		receive(retMsg, retParameters);

		ActiveUserLastUpdateTime_ = time(0);
		return (ActiveUserList_ = retParameters.getValue("UserList"));
	}
	else
		return ActiveUserList_;
}

//========================================================================================================================
//getLastConfigGroup
//	request last "Configured" or "Started" group, for example
//	returns empty "" for actionTimeString on failure
//	returns "Wed Dec 31 18:00:01 1969 CST" for actionTimeString (in CST) if action never has occurred
std::pair<std::string /*group name*/, ConfigurationGroupKey> RemoteWebUsers::getLastConfigGroup(
		XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor,
		const std::string &actionOfLastGroup,
		std::string &actionTimeString)
{
	actionTimeString = "";
	xoap::MessageReference retMsg = ots::SOAPMessenger::sendWithSOAPReply(
			supervisorDescriptor,"SupervisorLastConfigGroupRequest",
			SOAPParameters("ActionOfLastGroup",actionOfLastGroup));


	SOAPParameters retParameters;
	retParameters.addParameter("GroupName");
	retParameters.addParameter("GroupKey");
	retParameters.addParameter("GroupAction");
	retParameters.addParameter("GroupActionTime");
	receive(retMsg, retParameters);

	std::pair<std::string /*group name*/, ConfigurationGroupKey> theGroup;
	if(retParameters.getValue("GroupAction") != actionOfLastGroup) //if action doesn't match.. weird
	{
		__COUT_WARN__ << "Returned group action '" << retParameters.getValue("GroupAction") <<
				"' does not match requested group action '" << actionOfLastGroup << ".'" << std::endl;
		return theGroup; //return empty and invalid
	}
	//else we have an action match

	theGroup.first = retParameters.getValue("GroupName");
	theGroup.second = strtol(retParameters.getValue("GroupKey").c_str(),0,0);
	actionTimeString = retParameters.getValue("GroupActionTime");
	return theGroup;
}

//========================================================================================================================
//getUserInfoForCookie
//	get username and display name for user based on cookie code
//	return true, if user info gotten successfully
//	else false
bool RemoteWebUsers::getUserInfoForCookie(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor,
		std::string &cookieCode, std::string *userName, std::string *displayName, uint64_t *activeSessionIndex)
{
	__COUT__ << std::endl;
	if(cookieCode.length() != WebUsers::COOKIE_CODE_LENGTH) return false; //return if invalid cookie code

	//	SOAPParametersV parameters(1);
	//	parameters[0].setName("CookieCode"); parameters[0].setValue(cookieCode);
	xoap::MessageReference retMsg = SOAPMessenger::sendWithSOAPReply(supervisorDescriptor, "SupervisorGetUserInfo", SOAPParameters("CookieCode",cookieCode));

	SOAPParameters retParameters;
	retParameters.addParameter("Username");
	retParameters.addParameter("DisplayName");
	retParameters.addParameter("ActiveSessionIndex");
	receive(retMsg, retParameters);
	if(userName)	*userName = retParameters.getValue("Username");
	if(displayName)	*displayName = retParameters.getValue("DisplayName");
	if(activeSessionIndex) *activeSessionIndex = strtoul(retParameters.getValue("ActiveSessionIndex").c_str(),0,0);

	__COUT__ << "userName " << *userName << std::endl;

	return true;
}

//========================================================================================================================
//cookieCodeIsActiveForRequest
//	for external supervisors to check with Supervisor for login
bool RemoteWebUsers::cookieCodeIsActiveForRequest(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor,
		std::string &cookieCode, uint8_t *userPermissions, std::string ip, bool refreshCookie, std::string *userWithLock)
{
	//__COUT__ << "CookieCode: " << cookieCode << " " << cookieCode.length() << std::endl;
	if(cookieCode.length() != WebUsers::COOKIE_CODE_LENGTH) return false; //return if invalid cookie code

	//

	SOAPParameters parameters;
	parameters.addParameter("CookieCode",cookieCode);
	parameters.addParameter("RefreshOption",refreshCookie?"1":"0");

	//__COUT__ << "CookieCode: " << cookieCode << std::endl;
	xoap::MessageReference retMsg = SOAPMessenger::sendWithSOAPReply(supervisorDescriptor, "SupervisorCookieCheck", parameters);


	SOAPParameters retParameters;
	retParameters.addParameter("CookieCode");
	retParameters.addParameter("Permissions");
	retParameters.addParameter("UserWithLock");
	receive(retMsg, retParameters);



	if(userWithLock)	*userWithLock = retParameters.getValue("UserWithLock");
	if(userPermissions) sscanf(retParameters.getValue("Permissions").c_str(),"%hhu",userPermissions); //unsigned char

	cookieCode = retParameters.getValue("CookieCode");

	return cookieCode.length() == WebUsers::COOKIE_CODE_LENGTH; //proper cookieCode has length WebUsers::COOKIE_CODE_LENGTH
}
//========================================================================================================================
//sendSystemMessage
//	send system message to toUser through Supervisor
//	toUser wild card * is to all users
void RemoteWebUsers::sendSystemMessage(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor, const std::string& toUser, const std::string& msg)
{
	SOAPParameters parameters;
	parameters.addParameter("ToUser" , toUser);
	parameters.addParameter("Message", msg);

	xoap::MessageReference retMsg = SOAPMessenger::sendWithSOAPReply(supervisorDescriptor, "SupervisorSystemMessage",parameters);
}


//========================================================================================================================
//makeSystemLogbookEntry
//	make system logbook through Supervisor
void RemoteWebUsers::makeSystemLogbookEntry(XDAQ_CONST_CALL xdaq::ApplicationDescriptor* supervisorDescriptor, const std::string& entryText)
{
	SOAPParameters parameters;
	parameters.addParameter("EntryText", entryText);

	xoap::MessageReference retMsg = SOAPMessenger::sendWithSOAPReply(supervisorDescriptor, "SupervisorSystemLogbookEntry",parameters);
}

