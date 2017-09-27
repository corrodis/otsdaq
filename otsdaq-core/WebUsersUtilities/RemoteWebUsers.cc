#include "otsdaq-core/WebUsersUtilities/RemoteWebUsers.h"

#include "otsdaq-core/SOAPUtilities/SOAPParameters.h"	//must include in .h for static function
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/SupervisorDescriptorInfo/SupervisorDescriptorInfo.h"

#include <cstdlib>
#include <cstdio>
#include <vector>


using namespace ots;

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "RemoteWebUsers"

//========================================================================================================================
//User Notes:
//	- use xmlLoginGateway to check security from outside the Supervisor and Wizard
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
//					if(!theRemoteWebUsers_.xmlLoginGateway(
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
//xmlLoginGateway
//	if false, user code should just return.. out is handled on false; on true, out is untouched
bool RemoteWebUsers::xmlLoginGateway(
		cgicc::Cgicc 					&cgi,
		std::ostringstream 				*out,
		HttpXmlDocument 				*xmldoc,
		const SupervisorDescriptorInfo 	&theSupervisorsDescriptorInfo,
		uint8_t 						*userPermissions,
		const bool						refresh,
		const uint8_t 					permissionsThreshold,
		const bool						checkLock,
		const bool						lockRequired,
		std::string 					*userWithLock,
		std::string 					*userName,
		std::string 					*displayName,
		uint64_t 						*activeSessionIndex )
{
	//initialized optional acquisition parameters to failed results
	if(userPermissions) 	*userPermissions    = 0;
	if(userWithLock)		*userWithLock       = "";
	if(userName)			*userName           = "";
	if(displayName)			*displayName        = "";
	if(activeSessionIndex)	*activeSessionIndex = -1;

	const std::string ip = cgi.getEnvironment().getRemoteAddr();


	//const_cast away the const
	//	so that this line is compatible with slf6 and slf7 versions of xdaq
	//	where they changed to const xdaq::ApplicationDescriptor* in slf7
	//
	// XDAQ_CONST_CALL is defined in "otsdaq-core/Macros/CoutHeaderMacros.h"
	const xdaq::ApplicationDescriptor* gatewaySupervisor;

	SOAPParameters               parameters;
	xoap::MessageReference       retMsg;

	//**** start LOGIN GATEWAY CODE ***//
	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions
	//Else, error message is returned in cookieCode
	std::string cookieCode = CgiDataUtilities::getOrPostData(cgi,"CookieCode"); //from GET or POST

	//	__COUT__ << cookieCode.length() << std::endl;
	//	__COUT__ << "cookieCode=" << cookieCode << std::endl;
	//	__COUT__ << std::endl;

	/////////////////////////////////////////////////////
	//have CookieCode, try it out
	gatewaySupervisor = theSupervisorsDescriptorInfo.getSupervisorDescriptor();
	if(!gatewaySupervisor) //assume using wizard mode
	{
		//if missing CookieCode... check if in Wizard mode and using sequence
		std::string sequence = CgiDataUtilities::getOrPostData(cgi,"sequence"); //from GET or POST
		//__COUT__ << "sequence=" << sequence << std::endl;
		if(!sequence.length())
		{
			__COUT__ << "Invalid attempt." << std::endl;
			*out << RemoteWebUsers::REQ_NO_LOGIN_RESPONSE;
			return false;	//invalid cookie and also invalid sequence
		}

		//have sequence, try it out

		gatewaySupervisor =	theSupervisorsDescriptorInfo.getWizardDescriptor();
		if(!gatewaySupervisor)
		{
			*out << RemoteWebUsers::REQ_NO_LOGIN_RESPONSE;
			return false;	//invalid cookie and present sequence, but no wizard supervisor
		}

		parameters.addParameter("sequence",sequence);
		retMsg = SOAPMessenger::sendWithSOAPReply(gatewaySupervisor,
				"SupervisorSequenceCheck", parameters);
		parameters.clear();
		parameters.addParameter("Permissions");
		receive(retMsg, parameters);

		uint8_t tmpUserPermissions;
		sscanf(parameters.getValue("Permissions").c_str(),"%hhu",&tmpUserPermissions); //unsigned char

		if(userPermissions) 	*userPermissions = tmpUserPermissions;

		if(tmpUserPermissions < permissionsThreshold)
		{
			*out << RemoteWebUsers::REQ_NO_LOGIN_RESPONSE;
			__COUT__ << "User has insufficient permissions: " << tmpUserPermissions << "<" <<
					permissionsThreshold << std::endl;
			return false;	//invalid cookie and present sequence, but not correct sequence
		}

		if(userWithLock)		*userWithLock = "admin";
		if(userName)			*userName = "admin";
		if(displayName)			*displayName = "Admin";
		if(activeSessionIndex) 	*activeSessionIndex = 0;

		return true; //successful sequence login!
	}

	//__COUT__ << std::endl;

	parameters.clear();
	parameters.addParameter("CookieCode",cookieCode);
	parameters.addParameter("RefreshOption",refresh?"1":"0");

	retMsg = SOAPMessenger::sendWithSOAPReply(gatewaySupervisor,
			"SupervisorCookieCheck", parameters);

	parameters.clear();
	parameters.addParameter("CookieCode");
	parameters.addParameter("Permissions");
	parameters.addParameter("UserWithLock");
	receive(retMsg, parameters);
	std::string tmpUserWithLock = parameters.getValue("UserWithLock");
	uint8_t tmpUserPermissions;
	sscanf(parameters.getValue("Permissions").c_str(),"%hhu",&tmpUserPermissions); //unsigned char
	if(userWithLock)	*userWithLock = tmpUserWithLock;
	if(userPermissions) *userPermissions = tmpUserPermissions;

	cookieCode = parameters.getValue("CookieCode");

	//__COUT__ << "cookieCode=" << cookieCode << std::endl;

	if(cookieCode.length() != COOKIE_CODE_LENGTH)
	{
		*out << RemoteWebUsers::REQ_NO_LOGIN_RESPONSE;
		return false;	//invalid cookie and present sequence, but not correct sequence
	}

	if(tmpUserPermissions < permissionsThreshold)
	{
		*out << RemoteWebUsers::REQ_NO_PERMISSION_RESPONSE;
		__COUT__ << "User has insufficient permissions: " << tmpUserPermissions << "<" <<
				permissionsThreshold << std::endl;
		return false;
	}

	if(xmldoc) //fill with cookie code tag
		xmldoc->setHeader(cookieCode);

	if(!userName && !displayName && !activeSessionIndex && !checkLock && !lockRequired)
		return true; //done, no need to get user info for cookie

	//__COUT__ << "User with Lock: " << tmpUserWithLock << std::endl;


	/////////////////////////////////////////////////////
	//get user info
	parameters.clear();
	parameters.addParameter("CookieCode",cookieCode);
	retMsg = SOAPMessenger::sendWithSOAPReply(gatewaySupervisor,
			"SupervisorGetUserInfo", parameters);

	parameters.clear();
	parameters.addParameter("Username");
	parameters.addParameter("DisplayName");
	parameters.addParameter("ActiveSessionIndex");
	receive(retMsg, parameters);
	std::string tmpUserName = parameters.getValue("Username");
	if(userName)	*userName = tmpUserName;
	if(displayName)	*displayName = parameters.getValue("DisplayName");
	if(activeSessionIndex) *activeSessionIndex = strtoul(parameters.getValue("ActiveSessionIndex").c_str(),0,0);

	if(checkLock && tmpUserWithLock != "" && tmpUserWithLock != tmpUserName)
	{
		*out << RemoteWebUsers::REQ_USER_LOCKOUT_RESPONSE;
		__COUT__ << "User " << tmpUserName << " is locked out. " << tmpUserWithLock << " has lock." << std::endl;
		return false;
	}

	if(lockRequired && tmpUserWithLock != tmpUserName)
	{
		*out << RemoteWebUsers::REQ_LOCK_REQUIRED_RESPONSE;
		__COUT__ << "User " << tmpUserName << " must have lock to proceed. (" << tmpUserWithLock << " has lock.)" << std::endl;
		return false;
	}

	return true;
}

//========================================================================================================================
//isWizardMode
//	return true if in wizard configuration mode
bool RemoteWebUsers::isWizardMode(const SupervisorDescriptorInfo& theSupervisorsDescriptorInfo)
{
	return theSupervisorsDescriptorInfo.getWizardDescriptor()?true:false;
}

//========================================================================================================================
//getActiveUserList
//	if lastUpdateTime is not too recent as spec'd by ACTIVE_USERS_UPDATE_THRESHOLD
//	if server responds with
std::string RemoteWebUsers::getActiveUserList(const xdaq::ApplicationDescriptor* supervisorDescriptor)
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
		const xdaq::ApplicationDescriptor* supervisorDescriptor,
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
bool RemoteWebUsers::getUserInfoForCookie(const xdaq::ApplicationDescriptor* supervisorDescriptor,
		std::string &cookieCode, std::string *userName, std::string *displayName, uint64_t *activeSessionIndex)
{
	__COUT__ << std::endl;
	if(cookieCode.length() != COOKIE_CODE_LENGTH) return false; //return if invalid cookie code

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
bool RemoteWebUsers::cookieCodeIsActiveForRequest(const xdaq::ApplicationDescriptor* supervisorDescriptor,
		std::string &cookieCode, uint8_t *userPermissions, std::string ip, bool refresh, std::string *userWithLock)
{
	//__COUT__ << "CookieCode: " << cookieCode << " " << cookieCode.length() << std::endl;
	if(cookieCode.length() != COOKIE_CODE_LENGTH) return false; //return if invalid cookie code

	//

	SOAPParameters parameters;
	parameters.addParameter("CookieCode",cookieCode);
	parameters.addParameter("RefreshOption",refresh?"1":"0");

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

	return cookieCode.length() == COOKIE_CODE_LENGTH; //proper cookieCode has length COOKIE_CODE_LENGTH
}
//========================================================================================================================
//sendSystemMessage
//	send system message to toUser through Supervisor
//	toUser wild card * is to all users
void RemoteWebUsers::sendSystemMessage(const xdaq::ApplicationDescriptor* supervisorDescriptor, const std::string& toUser, const std::string& msg)
{
	SOAPParameters parameters;
	parameters.addParameter("ToUser" , toUser);
	parameters.addParameter("Message", msg);

	xoap::MessageReference retMsg = SOAPMessenger::sendWithSOAPReply(supervisorDescriptor, "SupervisorSystemMessage",parameters);
}


//========================================================================================================================
//makeSystemLogbookEntry
//	make system logbook through Supervisor
void RemoteWebUsers::makeSystemLogbookEntry(const xdaq::ApplicationDescriptor* supervisorDescriptor, const std::string& entryText)
{
	SOAPParameters parameters;
	parameters.addParameter("EntryText", entryText);

	xoap::MessageReference retMsg = SOAPMessenger::sendWithSOAPReply(supervisorDescriptor, "SupervisorSystemLogbookEntry",parameters);
}

