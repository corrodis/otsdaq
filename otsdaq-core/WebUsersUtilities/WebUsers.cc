#include "otsdaq-core/WebUsersUtilities/WebUsers.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <openssl/sha.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

using namespace ots;



#define WEB_LOGIN_BKUP_DB_PATH 			"bkup/"

#define SECURITY_FILE_NAME 				std::string(getenv("SERVICE_DATA_PATH")) + "/OtsWizardData/security.dat"

#define USERS_ACTIVE_SESSIONS_FILE 		USERS_DB_PATH + "/activeSessions.sv"

#define HASHES_DB_FILE 					HASHES_DB_PATH + "/hashes.xml"
#define USERS_DB_FILE 					USERS_DB_PATH + "/users.xml"
#define USERS_GLOBAL_HISTORY_FILE		"__global"
#define USERS_LOGIN_HISTORY_FILETYPE	"hist"
#define USERS_PREFERENCES_FILETYPE 		"pref"
#define SYSTEM_PREFERENCES_PREFIX 		"system.preset"
#define USER_WITH_LOCK_FILE				WEB_LOGIN_DB_PATH + "/user_with_lock.dat"
#define IP_BLACKLIST_FILE				WEB_LOGIN_DB_PATH + "/ip_generated_blacklist.dat"
#define IP_REJECT_FILE					WEB_LOGIN_DB_PATH + "/ip_reject.dat"
#define IP_ACCEPT_FILE					WEB_LOGIN_DB_PATH + "/ip_accept.dat"

#define HASHES_DB_GLOBAL_STRING 		"hashData"
#define HASHES_DB_ENTRY_STRING 			"hashEntry"
#define USERS_DB_GLOBAL_STRING 			"userData"
#define USERS_DB_ENTRY_STRING 			"userEntry"
#define USERS_DB_NEXT_UID_STRING 		"nextUserId"

//defines for user preferences
#define PREF_XML_BGCOLOR_FIELD	 		"pref_bgcolor"				// -background color
#define PREF_XML_DBCOLOR_FIELD	 		"pref_dbcolor"				// -dashboard color
#define PREF_XML_WINCOLOR_FIELD	 		"pref_wincolor"				// -window color
#define PREF_XML_LAYOUT_FIELD	 		"pref_layout"				// -3 defaults window layouts(and current)
#define PREF_XML_SYSLAYOUT_FIELD	 	"pref_syslayout"			// -2 defaults window layouts
#define PREF_XML_PERMISSIONS_FIELD	 	"desktop_user_permissions"	// 0-255 permissions value (255 is admin super user)
#define PREF_XML_USERLOCK_FIELD			"username_with_lock"		// user with lock (to lockout others)
#define PREF_XML_USERNAME_FIELD			"pref_username"				// user with lock (to lockout others)

#define PREF_XML_BGCOLOR_DEFAULT		"rgb(0,76,151)"			// -background color
#define PREF_XML_DBCOLOR_DEFAULT		"rgb(0,40,85)"			// -dashboard color
#define PREF_XML_WINCOLOR_DEFAULT		"rgba(196,229,255,0.9)"	// -window color
#define PREF_XML_LAYOUT_DEFAULT	 		"0;0;0;0"				// 3 default window layouts(and current)
#define PREF_XML_SYSLAYOUT_DEFAULT	 	"0;0"					// 2 system default window layouts

#define PREF_XML_ACCOUNTS_FIELD	 		"users_accounts"	// user accounts field for super users
#define PREF_XML_LOGIN_HISTORY_FIELD	"login_entry"		// login history field for user login history data 

const std::string WebUsers::DEFAULT_ADMIN_USERNAME 			= "admin";
const std::string WebUsers::DEFAULT_ADMIN_DISPLAY_NAME 		= "Administrator";
const std::string WebUsers::DEFAULT_ADMIN_EMAIL 			= "root@otsdaq.fnal.gov";
const std::string WebUsers::DEFAULT_ITERATOR_USERNAME 		= "iterator";
const std::string WebUsers::DEFAULT_STATECHANGER_USERNAME 	= "statechanger";
const std::string WebUsers::DEFAULT_USER_GROUP 				= "allUsers";

const std::string WebUsers::REQ_NO_LOGIN_RESPONSE 			= "NoLogin";
const std::string WebUsers::REQ_NO_PERMISSION_RESPONSE 		= "NoPermission";
const std::string WebUsers::REQ_USER_LOCKOUT_RESPONSE 		= "UserLockout";
const std::string WebUsers::REQ_LOCK_REQUIRED_RESPONSE 		= "LockRequired";
const std::string WebUsers::REQ_ALLOW_NO_USER 				= "AllowNoUser";

const std::string WebUsers::SECURITY_TYPE_NONE 				= "NoSecurity";
const std::string WebUsers::SECURITY_TYPE_DIGEST_ACCESS 	= "DigestAccessAuthentication";




#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "WebUsers"


WebUsers::WebUsers()
{
	//deleteUserData(); //leave for debugging to reset user data

	usersNextUserId_ = 0;			//first UID, default to 0 but get from database
	usersUsernameWithLock_ = "";	//init to no user with lock

	//define fields
	HashesDatabaseEntryFields.push_back("hash");
	HashesDatabaseEntryFields.push_back("lastAccessTime");  //last login month resolution, blurred by 1/2 month

	UsersDatabaseEntryFields.push_back("username");
	UsersDatabaseEntryFields.push_back("displayName");
	UsersDatabaseEntryFields.push_back("salt");
	UsersDatabaseEntryFields.push_back("uid");
	UsersDatabaseEntryFields.push_back("permissions");
	UsersDatabaseEntryFields.push_back("lastLoginAttemptTime");
	UsersDatabaseEntryFields.push_back("accountCreatedTime");
	UsersDatabaseEntryFields.push_back("loginFailureCount");
	UsersDatabaseEntryFields.push_back("lastModifiedTime");
	UsersDatabaseEntryFields.push_back("lastModifierUsername");
	UsersDatabaseEntryFields.push_back("useremail");

	//attempt to make directory structure (just in case)
	mkdir(((std::string)WEB_LOGIN_DB_PATH).c_str(), 0755);
	mkdir(((std::string)WEB_LOGIN_DB_PATH + "bkup/" + USERS_DB_PATH).c_str(), 0755);
	mkdir(((std::string)WEB_LOGIN_DB_PATH + HASHES_DB_PATH).c_str(), 0755);
	mkdir(((std::string)WEB_LOGIN_DB_PATH + USERS_DB_PATH).c_str(), 0755);
	mkdir(((std::string)WEB_LOGIN_DB_PATH + USERS_LOGIN_HISTORY_PATH).c_str(), 0755);
	mkdir(((std::string)WEB_LOGIN_DB_PATH + USERS_PREFERENCES_PATH).c_str(), 0755);


	if (!loadDatabases())
		__COUT__ << "FATAL USER DATABASE ERROR - failed to load!!!" << __E__;

	loadSecuritySelection();


	//print out admin new user code for ease of use
	uint64_t i;
	std::string user = DEFAULT_ADMIN_USERNAME;
	if ((i = searchUsersDatabaseForUsername(user)) == NOT_FOUND_IN_DATABASE)
	{
		__SS__ << "user: " << user << " is not found" << __E__;
		__COUT_ERR__ << ss.str();
		throw std::runtime_error(ss.str());
		exit(0); //THIS CAN NOT HAPPEN?! There must be an admin user
	}
	else if (UsersSaltVector[i] == "" && //admin password not setup, so print out NAC to help out
			securityType_ == SECURITY_TYPE_DIGEST_ACCESS)
	{
		char charTimeStr[10];
		sprintf(charTimeStr, "%d", int(UsersAccountCreatedTimeVector[i] & 0xffff));
		std::string tmpTimeStr = charTimeStr;

		//////////////////////////////////////////////////////////////////////
		//start thread for notifying the user about the admin new account code
		// notify for 10 seconds (e.g.)
		std::thread([](const std::string& nac, const std::string& user) { WebUsers::NACDisplayThread(nac, user); },
				tmpTimeStr, user).detach();

	}


	//attempt to load persistent user sessions
	loadActiveSessions();

	//default user with lock to admin and/or try to load last user with lock
	//Note: this must happen after getting persistent active sessions
	loadUserWithLock();

	srand(time(0)); //seed random for hash salt generation

	__COUT__ << "Done with Web Users initialization!" << __E__;

	// FIXME -- can delete this commented section eventually
	// this is for debugging the registering and login functionality
	//
	//  // deleteUserData();

	//std::string uuid = "0";
	//std::string sid = createNewLoginSession(uuid);
	//exit(0);

	// 	std::string newAccountCode = "60546";
	// 	std::string pw = "testbeam";
	//
	//	__COUT__ << "user: " << user <<  __E__ << __E__;
	//
	//
	//	if(1) //test salt functionality
	//	{
	//		std::string salt = "";  //don't want to modify saved salt
	//		std::string hash1 = sha512(user,pw,salt);
	//		std::string hash2 = sha512(user,pw,salt);
	//		__COUT__ << hash1 << __E__;
	//		__COUT__ << hash2 << __E__;
	//
	//		__COUT__ << "String comparison result: " << strcmp(hash1.c_str(),hash2.c_str()) << __E__;
	//	}
	//
	//	if(0) //login attempt
	//	{
	//		//search users for username
	//		if((i = searchUsersDatabaseForUsername(user)) == NOT_FOUND_IN_DATABASE)
	//		{
	//			__COUT__ << "user: " << user << " is not found" << __E__;
	//			return;// NOT_FOUND_IN_DATABASE;
	//		}
	//
	//		__COUT__ << "user: " << user <<  __E__ << __E__;
	//
	//		std::string salt = UsersSaltVector[i];  //don't want to modify saved salt
	//		__COUT__ << salt<< " " << i << __E__;
	//		if(searchHashesDatabaseForHash(sha512(user,pw,salt)) == NOT_FOUND_IN_DATABASE)
	//		{
	//			__COUT__ << "not found?" << __E__;
	//			++UsersLoginFailureCountVector[i];
	//			if(UsersLoginFailureCountVector[i] >= USERS_MAX_LOGIN_FAILURES)
	//				UsersPermissionsVector[i] = 0; //Lock account
	//
	//			__COUT__ << "\tUser/pw for user: " << user << " was not correct Failed Attempt #" << (int)(UsersLoginFailureCountVector[i]) << __E__;
	//			if(!UsersPermissionsVector[i])
	//				__COUT__ << "Account is locked!" << __E__;
	//
	//			saveDatabaseToFile(DB_USERS); //users db modified, so save
	//			return;//NOT_FOUND_IN_DATABASE;
	//		}
	//	}
	//
	//
	//	if(0) //first login
	//	{
	//		//search users for username
	//		if((i = searchUsersDatabaseForUsername(user)) == NOT_FOUND_IN_DATABASE)
	//		{
	//			__COUT__ << "user: " << user << " is not found" << __E__;
	//			return;// NOT_FOUND_IN_DATABASE;
	//		}
	//		__COUT__ << "user: " << user <<  __E__ << __E__;
	//
	//		UsersLastLoginAttemptVector[i] = time(0);
	//		if(!UsersPermissionsVector[i])
	//		{
	//			__COUT__ << "user: " << user << " account INACTIVE (could be due to failed logins)" << __E__;
	//			return;// NOT_FOUND_IN_DATABASE;
	//		}
	//		__COUT__ << "user: " << user <<  __E__ << __E__;
	//
	//		if(UsersSaltVector[i] == "") //first login
	//		{
	//			__COUT__ << "First login attempt for user: " << user << __E__;
	//
	//			char charTimeStr[10];
	//			sprintf(charTimeStr,"%d",int(UsersAccountCreatedTimeVector[i] & 0xffff));
	//			std::string tmpTimeStr = charTimeStr;
	//			if(newAccountCode != tmpTimeStr)
	//			{
	//				__COUT__ << "New account code did not match: " << tmpTimeStr << " != " << newAccountCode << __E__;
	//				saveDatabaseToFile(DB_USERS); //users db modified, so save
	//				return;// NOT_FOUND_IN_DATABASE;
	//			}
	//
	//			__COUT__ << "First login attempt for user: " << user << __E__;
	//			//initial user account setup
	//
	//			//add until no collision (should 'never' be a collision)
	//			while(!addToHashesDatabase(sha512(user,pw,UsersSaltVector[i]))) //sha256 modifies UsersSaltVector[i]
	//			{
	//				//this should never happen, it would mean the user+pw+saltcontext was the same
	//				// but if it were to happen, try again...
	//				UsersSaltVector[i] = "";
	//			}
	//
	//
	//			__COUT__ << "\tHash added: " << HashesVector[0] << __E__;
	//		}
	//
	//		__COUT__ << "Login successful for: " << user << __E__;
	//
	//		UsersLoginFailureCountVector[i] = 0;
	//
	//		saveDatabaseToFile(DB_USERS); //users db modified, so save
	//	}


	//    //starting
	//
	//	SHA512_CTX sha512_context;
	//	char hexStr[3];
	//	SHA512_Init(&sha512_context);
	//
	//
	//	std::string password = "testbeam";
	//	std::string salt = "";
	//
	//	{
	//		for(unsigned int i=0;i<sizeof(SHA512_CTX);++i)
	//		{
	//			intToHexStr((((uint8_t *)(&sha512_context))[i] + (i<32)?rand():0),hexStr);
	//			salt.append(hexStr);
	//		}
	//		__COUT__ << salt << __E__;
	//
	//		std::string strToHash = salt + user + password;
	//
	//		__COUT__ << salt << __E__;
	//		unsigned char hash[SHA512_DIGEST_LENGTH];
	//		__COUT__ << salt << __E__;
	//		char retHash[SHA512_DIGEST_LENGTH*2+1];
	//		__COUT__ << strToHash.length() << " " << strToHash << __E__;
	//		SHA512_Update(&sha512_context, strToHash.c_str(), strToHash.length());
	//		__COUT__ << salt << __E__;
	//		SHA512_Final(hash, &sha512_context);
	//
	//		__COUT__ << salt << __E__;
	//		int i = 0;
	//		for(i = 0; i < SHA512_DIGEST_LENGTH; i++)
	//			sprintf(retHash + (i * 2), "%02x", hash[i]);
	//
	//		__COUT__ << salt << __E__;
	//		retHash[SHA512_DIGEST_LENGTH*2] = '\0';
	//
	//		__COUT__ << "retHash: " <<  retHash << __E__;
	//	}
	//
	//	//check it
	//
	//	{
	//		__COUT__ << salt << __E__;
	//
	//		for(unsigned int i=0;i<sizeof(SHA512_CTX);++i)
	//			((uint8_t *)(&sha512_context))[i] = hexByteStrToInt(&(salt.c_str()[i*2]));
	//
	//
	//		std::string strToHash = salt + user + password;
	//
	//		__COUT__ << salt << __E__;
	//		unsigned char hash[SHA512_DIGEST_LENGTH];
	//		__COUT__ << salt << __E__;
	//		char retHash[SHA512_DIGEST_LENGTH*2+1];
	//		__COUT__ << strToHash.length() << " " << strToHash << __E__;
	//		SHA512_Update(&sha512_context, strToHash.c_str(), strToHash.length());
	//		__COUT__ << salt << __E__;
	//		SHA512_Final(hash, &sha512_context);
	//
	//		__COUT__ << salt << __E__;
	//		int i = 0;
	//		for(i = 0; i < SHA512_DIGEST_LENGTH; i++)
	//			sprintf(retHash + (i * 2), "%02x", hash[i]);
	//
	//		__COUT__ << salt << __E__;
	//		retHash[SHA512_DIGEST_LENGTH*2] = '\0';
	//
	//		__COUT__ << "retHash: " <<  retHash << __E__;
	//	}
}


//========================================================================================================================
//xmlRequestOnGateway
//	check the validity of an xml request at the server side, i.e. at the Gateway supervisor, which is the owner
//		of the web users instance.
//	if false, gateway request code should just return.. out is handled on false; on true, out is untouched
bool WebUsers::xmlRequestOnGateway(
		cgicc::Cgicc& 					cgi,
		std::ostringstream* 			out,
		HttpXmlDocument* 				xmldoc,
		WebUsers::RequestUserInfo&		userInfo
		)
{

	//initialize user info parameters to failed results
	WebUsers::initializeRequestUserInfo(cgi,userInfo);

	//tmpUserWithLock_ = "";


	if (!cookieCodeIsActiveForRequest(
			userInfo.cookieCode_,
			&userInfo.groupPermissionLevelMap_,
			&userInfo.uid_,
			userInfo.ip_,
			!userInfo.automatedCommand_ /*refresh cookie*/,
			&userInfo.usernameWithLock_,
			&userInfo.activeUserSessionIndex_))
	{
		*out << userInfo.cookieCode_;
		goto HANDLE_ACCESS_FAILURE; //return false, access failed
	}

	//setup userInfo.permissionLevel_ based on userInfo.groupPermissionLevelMap_
	userInfo.getGroupPermissionLevel();
	userInfo.username_ = UsersUsernameVector[userInfo.uid_];
	userInfo.displayName_ = UsersDisplayNameVector[userInfo.uid_];

	if(!WebUsers::checkRequestAccess(cgi,out,xmldoc,userInfo))
		goto HANDLE_ACCESS_FAILURE; //return false, access failed

	return true; //access success!


HANDLE_ACCESS_FAILURE:
	//print out return string on failure
	if(!userInfo.automatedCommand_)
		__COUT_ERR__ << "Failed request (requestType = " << userInfo.requestType_ <<
		"): " << out->str() << __E__;
	return false; //access failed

} //end xmlRequestOnGateway()

//========================================================================================================================
//initializeRequestUserInfo
//	initialize user info parameters to failed results
void WebUsers::initializeRequestUserInfo(
		cgicc::Cgicc& cgi,
		WebUsers::RequestUserInfo&	userInfo)
{
	userInfo.permissionLevel_ = 0; //always init to inactive
	userInfo.ip_ = cgi.getEnvironment().getRemoteAddr();

	//note if related bools are false, members below may not be set
	userInfo.username_ = "";
	userInfo.displayName_ = "";
	userInfo.usernameWithLock_ = "";
	userInfo.activeUserSessionIndex_ = -1;
	userInfo.setGroupPermissionLevels("");
}

//========================================================================================================================
//checkRequestAccess
//	check user permission parameters based on cookie code, user permission level (extracted previous from group membership)
//	Note: assumes userInfo.groupPermissionLevelMap_ and userInfo.permissionLevel_ are properly setup
//		by either calling userInfo.setGroupPermissionLevels() or userInfo.getGroupPermissionLevel()
bool WebUsers::checkRequestAccess(
		cgicc::Cgicc& 					cgi,
		std::ostringstream* 			out,
		HttpXmlDocument* 				xmldoc,
		WebUsers::RequestUserInfo&		userInfo,
		bool							isWizardMode)
{
	//steps:
	// - check access based on cookieCode and permission level
	// - check user lock flags and status


	if(!userInfo.automatedCommand_)
	{
		__COUT__ << "requestType ==========>>> " << userInfo.requestType_ << __E__;
		__COUTV__((unsigned int)userInfo.permissionLevel_);
		__COUTV__((unsigned int)userInfo.permissionsThreshold_);
	}

	//second, start check access -------
	if(!isWizardMode && !userInfo.allowNoUser_ &&
			userInfo.cookieCode_.length() != WebUsers::COOKIE_CODE_LENGTH)
	{
		__COUT__ << "User (@" << userInfo.ip_ << ") has invalid cookie code: " <<
				userInfo.cookieCode_ << std::endl;
		*out << WebUsers::REQ_NO_LOGIN_RESPONSE;
		return false;	//invalid cookie and present sequence, but not correct sequence
	}

	if(!userInfo.allowNoUser_ &&
			(userInfo.permissionLevel_ == 0 || //reject inactive permission level
			userInfo.permissionLevel_ < userInfo.permissionsThreshold_))
	{
		*out << WebUsers::REQ_NO_PERMISSION_RESPONSE;
		__COUT__ << "User (@" << userInfo.ip_ << ") has insufficient permissions for requestType '" <<
				userInfo.requestType_ <<
				"' : " <<
				(unsigned int)userInfo.permissionLevel_ << "<" <<
				(unsigned int)userInfo.permissionsThreshold_ << std::endl;
		return false;	//invalid cookie and present sequence, but not correct sequence
	}
	//end check access -------

	if(isWizardMode)
	{
		userInfo.username_ = "admin";
		userInfo.displayName_ = "Admin";
		userInfo.usernameWithLock_ = "admin";
		userInfo.activeUserSessionIndex_ = 0;
		return true; //done, wizard mode access granted
	}
	//else, normal gateway verify mode

	if(xmldoc) //fill with cookie code tag
	{
		if(userInfo.allowNoUser_)
			xmldoc->setHeader(WebUsers::REQ_ALLOW_NO_USER);
		else
			xmldoc->setHeader(userInfo.cookieCode_);
	}

	if(userInfo.allowNoUser_) return true; //ignore lock for allow-no-user case


//	if(!userInfo.automatedCommand_)
//	{
//		__COUTV__(userInfo.username_);
//		__COUTV__(userInfo.usernameWithLock_);
//	}

	if((userInfo.checkLock_ || userInfo.requireLock_) &&
			userInfo.usernameWithLock_ != "" &&
			userInfo.usernameWithLock_ != userInfo.username_)
	{
		*out << WebUsers::REQ_USER_LOCKOUT_RESPONSE;
		__COUT__ << "User '" << userInfo.username_ << "' is locked out. '" <<
				userInfo.usernameWithLock_ << "' has lock." << std::endl;
		return false; //failed due to another user having lock
	}

	if(userInfo.requireLock_ &&
			userInfo.usernameWithLock_ != userInfo.username_)
	{
		*out << WebUsers::REQ_LOCK_REQUIRED_RESPONSE;
		__COUT__ << "User '" << userInfo.username_ << "' must have lock to proceed. ('" <<
				userInfo.usernameWithLock_ << "' has lock.)" << std::endl;
		return false; //failed due to lock being required, and this user does not have it
	}

	return true; //access success!

} //end checkRequestAccess()

//========================================================================================================================
//saveActiveSessions
//	save active sessions structure so that they can survive restart
void WebUsers::saveActiveSessions()
{
	std::string fn;

	fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_ACTIVE_SESSIONS_FILE;
	__COUT__ << fn << __E__;

	FILE *fp = fopen(fn.c_str(), "w");
	if (!fp)
	{
		__COUT_ERR__ << "Error! Persistent active sessions could not be saved to file: " <<
				fn << __E__;
		return;
	}

	int version = 0;
	fprintf(fp, "%d\n", version);
	for (unsigned int i = 0; i < ActiveSessionCookieCodeVector.size(); ++i)
	{
		//		__COUT__ << "SAVE " << ActiveSessionCookieCodeVector[i] << __E__;
		//		__COUT__ << "SAVE " << ActiveSessionIpVector[i] << __E__;
		//		__COUT__ << "SAVE " << ActiveSessionUserIdVector[i] << __E__;
		//		__COUT__ << "SAVE " << ActiveSessionIndex[i] << __E__;
		//		__COUT__ << "SAVE " << ActiveSessionStartTimeVector[i] << __E__;

		fprintf(fp, "%s\n", ActiveSessionCookieCodeVector[i].c_str());
		fprintf(fp, "%s\n", ActiveSessionIpVector[i].c_str());
		fprintf(fp, "%lu\n", ActiveSessionUserIdVector[i]);
		fprintf(fp, "%lu\n", ActiveSessionIndex[i]);
		fprintf(fp, "%ld\n", ActiveSessionStartTimeVector[i]);
	}

	__COUT__ << "ActiveSessionCookieCodeVector saved with size " <<
			ActiveSessionCookieCodeVector.size() << __E__;

	fclose(fp);
}

//====================================================================================================================
//loadActiveSessions
//	load active sessions structure so that they can survive restart
void WebUsers::loadActiveSessions()
{
	std::string fn;

	fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_ACTIVE_SESSIONS_FILE;
	__COUT__ << fn << __E__;
	FILE *fp = fopen(fn.c_str(), "r");
	if (!fp)
	{
		__COUT_INFO__ << "Persistent active sessions were not found to be loaded at file: " <<
				fn << __E__;
		return;
	}

	int version;

	const int LINELEN = 1000;
	char line[LINELEN];
	fgets(line, LINELEN, fp);
	sscanf(line, "%d", &version);
	if (version == 0)
	{
		__COUT__ << "Extracting active sessions..." << __E__;

	}
	unsigned int i = 0;
	while (fgets(line, LINELEN, fp))
	{
		if (strlen(line)) line[strlen(line) - 1] = '\0'; //remove new line
		if (strlen(line) != COOKIE_CODE_LENGTH)
		{
			__COUT__ << "Illegal cookie code found: " << line << __E__;

			fclose(fp);
			return;
		}
		ActiveSessionCookieCodeVector.push_back(line);

		fgets(line, LINELEN, fp);
		if (strlen(line)) line[strlen(line) - 1] = '\0'; //remove new line
		ActiveSessionIpVector.push_back(line);

		fgets(line, LINELEN, fp);
		ActiveSessionUserIdVector.push_back(uint64_t());
		sscanf(line, "%lu", &(ActiveSessionUserIdVector[ActiveSessionUserIdVector.size() - 1]));

		fgets(line, LINELEN, fp);
		ActiveSessionIndex.push_back(uint64_t());
		sscanf(line, "%lu", &(ActiveSessionIndex[ActiveSessionIndex.size() - 1]));

		fgets(line, LINELEN, fp);
		ActiveSessionStartTimeVector.push_back(time_t());
		sscanf(line, "%ld", &(ActiveSessionStartTimeVector[ActiveSessionStartTimeVector.size() - 1]));


		//		__COUT__ << "LOAD " << ActiveSessionCookieCodeVector[i] << __E__;
		//		__COUT__ << "LOAD " << ActiveSessionIpVector[i] << __E__;
		//		__COUT__ << "LOAD " << ActiveSessionUserIdVector[i] << __E__;
		//		__COUT__ << "LOAD " << ActiveSessionIndex[i] << __E__;
		//		__COUT__ << "LOAD " << ActiveSessionStartTimeVector[i] << __E__;
		++i;
	}

	__COUT__ << "ActiveSessionCookieCodeVector loaded with size " <<
			ActiveSessionCookieCodeVector.size() << __E__;


	fclose(fp);
	//clear file after loading
	fp = fopen(fn.c_str(), "w");
	if (fp)fclose(fp);

}

//========================================================================================================================
//loadDatabaseFromFile
//	load Hashes and Users from file
//	create database if non-existent
bool WebUsers::loadDatabases()
{
	std::string fn;

	FILE *fp;
	const unsigned int LINE_LEN = 1000;
	char line[LINE_LEN];
	unsigned int i, si, c, len, f;
	uint64_t tmpInt64;

	//hashes
	//	File Organization:
	//		<hashData>
	//			<hashEntry><hash>hash0</hash><lastAccessTime>lastAccessTime0</lastAccessTime></hashEntry>
	//			<hashEntry><hash>hash1</hash><lastAccessTime>lastAccessTime1</lastAccessTime></hashEntry>
	//			..
	//		</hashData>

	fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)HASHES_DB_FILE;
	__COUT__ << fn << __E__;
	fp = fopen(fn.c_str(), "r");
	if (!fp) //need to create file
	{
		mkdir(((std::string)WEB_LOGIN_DB_PATH + (std::string)HASHES_DB_PATH).c_str(), 0755);
		__COUT__ << ((std::string)WEB_LOGIN_DB_PATH + (std::string)HASHES_DB_PATH).c_str() << __E__;
		fp = fopen(fn.c_str(), "w");
		if (!fp) return false;
		__COUT__ << "Hashes database created: " << fn << __E__;

		saveToDatabase(fp, HASHES_DB_GLOBAL_STRING, "", DB_SAVE_OPEN);
		saveToDatabase(fp, HASHES_DB_GLOBAL_STRING, "", DB_SAVE_CLOSE);
		fclose(fp);
	}
	else //load structures if hashes exists
	{
		//for every HASHES_DB_ENTRY_STRING, extract to local vector
		//trusting file construction, assuming fields based >'s and <'s
		while (fgets(line, LINE_LEN, fp))
		{
			if (strlen(line) < SHA512_DIGEST_LENGTH) continue;

			c = 0;
			len = strlen(line); //save len, strlen will change because of \0 manipulations
			for (i = 0; i < len; ++i)
				if (line[i] == '>')
				{
					++c; //count >'s
					if (c != 2 && c != 4) continue; //only proceed for field data

					si = ++i; //save start index
					while (i < len && line[i] != '<') ++i;
					if (i == len)
						break;
					line[i] = '\0'; //close std::string

					//__COUT__ << "Found Hashes field " << c/2 << " " << &line[si] << __E__;

					f = c / 2 - 1;
					if (f == 0)	//hash
						HashesVector.push_back(&line[si]);
					else if (f == 1)	//lastAccessTime
					{
						sscanf(&line[si], "%lu", &tmpInt64);
						HashesAccessTimeVector.push_back(tmpInt64);
					}
				}
		}
		__COUT__ << HashesAccessTimeVector.size() << " Hashes found." << __E__;

		fclose(fp);
	}

	//users
	//	File Organization:
	//		<userData>
	//			<nextUserId>...</nextUserId>
	//			<userEntry>...</userEntry>
	//			<userEntry>...</userEntry>
	//			..
	//		</userData>

	fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_DB_FILE;
	fp = fopen(fn.c_str(), "r");
	if (!fp) //need to create file
	{
		mkdir(((std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_DB_PATH).c_str(), 0755);
		__COUT__ << ((std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_DB_PATH).c_str() << __E__;
		fp = fopen(fn.c_str(), "w");
		if (!fp) return false;
		__COUT__ << "Users database created: " << fn << __E__;

		saveToDatabase(fp, USERS_DB_GLOBAL_STRING, "", DB_SAVE_OPEN);
		char nidStr[100];
		sprintf(nidStr, "%lu", usersNextUserId_);
		saveToDatabase(fp, USERS_DB_NEXT_UID_STRING, nidStr, DB_SAVE_OPEN_AND_CLOSE);
		saveToDatabase(fp, USERS_DB_GLOBAL_STRING, "", DB_SAVE_CLOSE);
		fclose(fp);

		createNewAccount(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_DISPLAY_NAME, DEFAULT_ADMIN_EMAIL); //account 0 is always admin
	}
	else //extract next user id and user entries if users exists
	{
		//for every USERS_DB_ENTRY_STRING, extract to local vector
		//trusting file construction, assuming fields based >'s and <'s

		char salt[] = "nextUserId";
		while (fgets(line, LINE_LEN, fp))
		{
			if (strlen(line) < strlen(salt) * 2) continue; //line size	should indicate xml tags on same line

			for (i = 0; i < strlen(salt); ++i)		//check for opening tag
				if (line[i + 1] != salt[i]) break;

			if (i == strlen(salt)) //all salt matched, so found correct line! increment to get line index
			{
				i += 2;
				si = i;
				while (i < LINE_LEN && line[i] != '\0' && line[i] != '<') ++i; //find '<'
				line[i] = '\0'; //close std::string
				sscanf(&line[si], "%lu", &usersNextUserId_);
				break; //done with next uid
			}
		}

		__COUT__ << "Found Users database next user Id: " << usersNextUserId_ << __E__;

		//trusting file construction, assuming fields based >'s and <'s and each entry on one line
		while (fgets(line, LINE_LEN, fp))
		{
			if (strlen(line) < 30) continue; //rule out header tags

			c = 0;
			len = strlen(line); //save len, strlen will change because of \0 manipulations
			if (len >= LINE_LEN)
			{
				__COUT__ << "Line buffer too small: " << len << __E__;
				break;
			}

			//get fields from line
			f = 0;
			for (i = 0; i < len; ++i)
				if (line[i] == '>')
				{
					++c; //count >'s
					if (c == 0 || c % 2 == 1) continue; //only proceed for field data (even

					si = ++i; //save start index
					while (i < len && line[i] != '<') ++i;
					if (i == len)
						break;
					line[i] = '\0'; //close std::string
					f = c / 2 - 1;

					//__COUT__ << "Found Users field " << f << " " << &line[si] << __E__;

					if (f == 0)	//username
						UsersUsernameVector.push_back(&line[si]);
					else if (f == 1)	//displayName
						UsersDisplayNameVector.push_back(&line[si]);
					else if (f == 2)	//salt
						UsersSaltVector.push_back(&line[si]);
					else if (f == 3)	//uid
					{
						sscanf(&line[si], "%lu", &tmpInt64);
						UsersUserIdVector.push_back(tmpInt64);
					}
					else if (f == 4)	//permissions
					{
						UsersPermissionsVector.push_back(std::map<std::string,uint8_t>());
						std::map<std::string,uint8_t>& lastPermissionsMap = UsersPermissionsVector.back();
						StringMacros::getMapFromString<uint8_t>(&line[si],
								lastPermissionsMap);

						//__COUT__ << "User permission levels:" << StringMacros::mapToString(lastPermissionsMap) << __E__;

						//verify 'allUsers' is there
						//	if not, add it as a diabled user (i.e. WebUsers::PERMISSION_LEVEL_INACTIVE)
						if(lastPermissionsMap.find(WebUsers::DEFAULT_USER_GROUP) ==
								lastPermissionsMap.end())
						{
							//try to accomplish backwards compatibility to
							//	allow for the time before group permissions
							sscanf(&line[si], "%lu", &tmpInt64);
							tmpInt64 &= 0xFF;
							if(tmpInt64) //if not 0
							{
								lastPermissionsMap.clear();
								__COUT_INFO__ << "User '" <<
									UsersUsernameVector.back() << "' is not a member of the default user group '" <<
									WebUsers::DEFAULT_USER_GROUP << ".' For backward compatibility, permission level assumed for default group (permission level := " <<
									tmpInt64 << ")." << __E__;
								lastPermissionsMap[WebUsers::DEFAULT_USER_GROUP] = WebUsers::permissionLevel_t(tmpInt64);
							}
							else
							{
								__MCOUT_INFO__( "User '" <<
										UsersUsernameVector.back() << "' is not a member of the default user group '" <<
										WebUsers::DEFAULT_USER_GROUP << ".' Assuming user account is inactive (permission level := " <<
										WebUsers::PERMISSION_LEVEL_INACTIVE << ")." << __E__);
								lastPermissionsMap[WebUsers::DEFAULT_USER_GROUP] = WebUsers::PERMISSION_LEVEL_INACTIVE; //mark inactive
							}
						}
					}
					else if (f == 5)	//lastLoginAttemptTime
					{
						sscanf(&line[si], "%lu", &tmpInt64);
						UsersLastLoginAttemptVector.push_back(tmpInt64);
					}
					else if (f == 6)	//accountCreatedTime
					{
						sscanf(&line[si], "%lu", &tmpInt64);
						UsersAccountCreatedTimeVector.push_back(tmpInt64);
					}
					else if (f == 7)	//loginFailureCount
					{
						sscanf(&line[si], "%lu", &tmpInt64);
						UsersLoginFailureCountVector.push_back(tmpInt64);
					}
					else if (f == 8)	//lastModifierTime
					{
						sscanf(&line[si], "%lu", &tmpInt64);
						UsersLastModifiedTimeVector.push_back(tmpInt64);
					}
					else if (f == 9)	//lastModifierUsername
						UsersLastModifierUsernameVector.push_back(&line[si]);
					else if (f == 10) // user email
						UsersUserEmailVector.push_back(&line[si]);
				}

			//If user found in line, check if all fields found, else auto fill
			//update in DB fields could cause inconsistencies!
			if (f && f != UsersDatabaseEntryFields.size() - 1)
			{
				if (f != 7 && f != 9) //original database was size 8, so is ok to not match
				{
					__SS__ << "FATAL ERROR - invalid user database found with field number " << f << __E__;
					fclose(fp);
					__SS_THROW__;
					return false;
				}

				if (f == 7)
				{
					//fix here if database size was 8
					__COUT__ << "Update database to current version - adding fields: " <<
							(UsersDatabaseEntryFields.size() - 1 - f) << __E__;
					//add db updates -- THIS IS FOR VERSION WITH UsersDatabaseEntryFields.size() == 10 !!
					UsersLastModifiedTimeVector.push_back(0);
					UsersLastModifierUsernameVector.push_back("");
				}
				else
				{
					UsersUserEmailVector.push_back("");
				}
			}
		}
		fclose(fp);
	}

	__COUT__ << UsersLastModifiedTimeVector.size() << " Users found." << __E__;
	for (size_t ii = 0; ii < UsersLastModifiedTimeVector.size(); ++ii)
	{
		__COUT__ << "User " << UsersUserIdVector[ii] << ": Name: " << UsersUsernameVector[ii] <<
				"\t\tDisplay Name: " << UsersDisplayNameVector[ii] << "\t\tEmail: " <<
				UsersUserEmailVector[ii] << "\t\tPermissions: " <<
				StringMacros::mapToString(UsersPermissionsVector[ii]) << __E__;
	}
	return true;
}

//========================================================================================================================
//saveToDatabase
void WebUsers::saveToDatabase(FILE * fp, const std::string& field, const std::string& value, uint8_t type, bool addNewLine)
{
	if (!fp) return;

	std::string newLine = addNewLine ? "\n" : "";

	if (type == DB_SAVE_OPEN_AND_CLOSE)
		fprintf(fp, "<%s>%s</%s>%s", field.c_str(), value.c_str(), field.c_str(), newLine.c_str());
	else if (type == DB_SAVE_OPEN)
		fprintf(fp, "<%s>%s%s", field.c_str(), value.c_str(), newLine.c_str());
	else if (type == DB_SAVE_CLOSE)
		fprintf(fp, "</%s>%s", field.c_str(), newLine.c_str());
}

//========================================================================================================================
//saveDatabaseToFile
//	returns true if saved database successfully
//		db: DB_USERS or DB_HASHES
//	else false



bool WebUsers::saveDatabaseToFile(uint8_t db)
{
	__COUT__ << "Save Database: " << (int)db << __E__;

	std::string fn = (std::string)WEB_LOGIN_DB_PATH +
			((db == DB_USERS) ? (std::string)USERS_DB_FILE : (std::string)HASHES_DB_FILE);

	__COUT__ << "Save Database Filename: " << fn << __E__;

	//backup file organized by day
	if (0)
	{
		char dayAppend[20];
		sprintf(dayAppend, ".%lu.bkup", time(0) / (3600 * 24));
		std::string bkup_fn = (std::string)WEB_LOGIN_DB_PATH +
				(std::string)WEB_LOGIN_BKUP_DB_PATH +
				((db == DB_USERS) ? (std::string)USERS_DB_FILE : (std::string)HASHES_DB_FILE) +
				(std::string)dayAppend;

		__COUT__ << "Backup file: " << bkup_fn << __E__;

		std::string shell_command = "mv " + fn + " " + bkup_fn;
		system(shell_command.c_str());
	}

	FILE *fp = fopen(fn.c_str(), "wb"); //write in binary mode
	if (!fp) return false;

	char fldStr[100];

	if (db == DB_USERS) //USERS
	{
		saveToDatabase(fp, USERS_DB_GLOBAL_STRING, "", DB_SAVE_OPEN);

		sprintf(fldStr, "%lu", usersNextUserId_);
		saveToDatabase(fp, USERS_DB_NEXT_UID_STRING, fldStr, DB_SAVE_OPEN_AND_CLOSE);

		__COUT__ << "Saving " << UsersUsernameVector.size() << " Users." << __E__;

		for (uint64_t i = 0; i < UsersUsernameVector.size(); ++i)
		{
			//__COUT__ << "Saving User: " << UsersUsernameVector[i] << __E__;

			saveToDatabase(fp, USERS_DB_ENTRY_STRING, "", DB_SAVE_OPEN, false);

			for (unsigned int f = 0; f < UsersDatabaseEntryFields.size(); ++f)
			{
				//__COUT__ << "Saving Field: " << f << __E__;
				if (f == 0)	//username
					saveToDatabase(fp, UsersDatabaseEntryFields[f], UsersUsernameVector[i], DB_SAVE_OPEN_AND_CLOSE, false);
				else if (f == 1)	//displayName
					saveToDatabase(fp, UsersDatabaseEntryFields[f], UsersDisplayNameVector[i], DB_SAVE_OPEN_AND_CLOSE, false);
				else if (f == 2)	//salt
					saveToDatabase(fp, UsersDatabaseEntryFields[f], UsersSaltVector[i], DB_SAVE_OPEN_AND_CLOSE, false);
				else if (f == 3)	//uid
				{
					sprintf(fldStr, "%lu", UsersUserIdVector[i]);
					saveToDatabase(fp, UsersDatabaseEntryFields[f], fldStr, DB_SAVE_OPEN_AND_CLOSE, false);
				}
				else if (f == 4)	//permissions
					saveToDatabase(fp, UsersDatabaseEntryFields[f],
							StringMacros::mapToString(UsersPermissionsVector[i],","/*primary delimeter*/,":"/*secondary delimeter*/), DB_SAVE_OPEN_AND_CLOSE, false);
				else if (f == 5)	//lastLoginAttemptTime
				{
					sprintf(fldStr, "%lu", UsersLastLoginAttemptVector[i]);
					saveToDatabase(fp, UsersDatabaseEntryFields[f], fldStr, DB_SAVE_OPEN_AND_CLOSE, false);
				}
				else if (f == 6)	//accountCreatedTime
				{
					sprintf(fldStr, "%lu", UsersAccountCreatedTimeVector[i]);
					saveToDatabase(fp, UsersDatabaseEntryFields[f], fldStr, DB_SAVE_OPEN_AND_CLOSE, false);
				}
				else if (f == 7)	//loginFailureCount
				{
					sprintf(fldStr, "%d", UsersLoginFailureCountVector[i]);
					saveToDatabase(fp, UsersDatabaseEntryFields[f], fldStr, DB_SAVE_OPEN_AND_CLOSE, false);
				}
				else if (f == 8)	//lastModifierTime
				{
					sprintf(fldStr, "%lu", UsersLastModifiedTimeVector[i]);
					saveToDatabase(fp, UsersDatabaseEntryFields[f], fldStr, DB_SAVE_OPEN_AND_CLOSE, false);
				}
				else if (f == 9)	//lastModifierUsername
					saveToDatabase(fp, UsersDatabaseEntryFields[f], UsersLastModifierUsernameVector[i], DB_SAVE_OPEN_AND_CLOSE, false);
				else if (f == 10) // useremail
					saveToDatabase(fp, UsersDatabaseEntryFields[f], UsersUserEmailVector[i], DB_SAVE_OPEN_AND_CLOSE, false);
			}

			saveToDatabase(fp, USERS_DB_ENTRY_STRING, "", DB_SAVE_CLOSE);
		}

		saveToDatabase(fp, USERS_DB_GLOBAL_STRING, "", DB_SAVE_CLOSE);

	}
	else		//HASHES
	{
		saveToDatabase(fp, HASHES_DB_GLOBAL_STRING, "", DB_SAVE_OPEN);

		__COUT__ << "Saving " << HashesVector.size() << " Hashes." << __E__;
		for (uint64_t i = 0; i < HashesVector.size(); ++i)
		{
			__COUT__ << "Saving " << HashesVector[i] << " Hashes." << __E__;
			saveToDatabase(fp, HASHES_DB_ENTRY_STRING, "", DB_SAVE_OPEN, false);
			for (unsigned int f = 0; f < HashesDatabaseEntryFields.size(); ++f)
			{
				if (f == 0)	//hash
					saveToDatabase(fp, HashesDatabaseEntryFields[f], HashesVector[i], DB_SAVE_OPEN_AND_CLOSE, false);
				else if (f == 1)	//lastAccessTime
				{
					sprintf(fldStr, "%lu", HashesAccessTimeVector[i]);
					saveToDatabase(fp, HashesDatabaseEntryFields[f], fldStr, DB_SAVE_OPEN_AND_CLOSE, false);
				}
			}
			saveToDatabase(fp, HASHES_DB_ENTRY_STRING, "", DB_SAVE_CLOSE);
		}

		saveToDatabase(fp, HASHES_DB_GLOBAL_STRING, "", DB_SAVE_CLOSE);
	}


	fclose(fp);
	return true;
}

//========================================================================================================================
//createNewAccount
//	adds a new valid user to database 
//		inputs: username and name to display
//		initializes database entry with minimal permissions
//			and salt starts as "" until password is set
//		Special case if first user name!! max permissions given (super user made)
bool WebUsers::createNewAccount(const std::string& username, const std::string& displayName,
		const std::string& email)
{
	__COUT__ << "Creating account: " << username << __E__;
	//check if username already exists
	uint64_t i;
	if ((i = searchUsersDatabaseForUsername(username)) != NOT_FOUND_IN_DATABASE ||
			username == WebUsers::DEFAULT_ITERATOR_USERNAME ||
			username == WebUsers::DEFAULT_STATECHANGER_USERNAME) //prevent reserved usernames from being created!
	{
		__COUT_ERR__ << "Username '" << username << "' already exists" << __E__;
		return false;
	}

	//create Users database entry
	UsersUsernameVector.push_back(username);
	UsersDisplayNameVector.push_back(displayName);
	UsersUserEmailVector.push_back(email);
	UsersSaltVector.push_back("");
	std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> initPermissions = {{
			WebUsers::DEFAULT_USER_GROUP,
			(UsersPermissionsVector.size() ?
					WebUsers::PERMISSION_LEVEL_NOVICE:
					WebUsers::PERMISSION_LEVEL_ADMIN)}};
	UsersPermissionsVector.push_back(initPermissions); //max permissions if first user

	UsersUserIdVector.push_back(usersNextUserId_++);
	if (usersNextUserId_ == (uint64_t)-1) //error wrap around case
	{
		__COUT__ << "usersNextUserId_ wrap around!! Too many users??? Notify Admins." << __E__;
		usersNextUserId_ = 1; //for safety to avoid wierd issues at -1 and 0 (if used for error indication)
	}
	UsersLastLoginAttemptVector.push_back(0);
	UsersLoginFailureCountVector.push_back(0);
	UsersAccountCreatedTimeVector.push_back(time(0));
	UsersLastModifiedTimeVector.push_back(0);
	UsersLastModifierUsernameVector.push_back("");

	return saveDatabaseToFile(DB_USERS);
}

//========================================================================================================================
//deleteAccount
//	private function, deletes user account
//		inputs: username and name to display
//		if username and display name match account found, then account is deleted and true returned
//		else false
bool WebUsers::deleteAccount(const std::string& username, const std::string& displayName)
{
	uint64_t i = searchUsersDatabaseForUsername(username);
	if (i == NOT_FOUND_IN_DATABASE) return false;
	if (UsersDisplayNameVector[i] != displayName) return false; //display name does not match

	//delete entry from all user database vectors

	UsersUsernameVector.erase(UsersUsernameVector.begin() + i);
	UsersUserEmailVector.erase(UsersUserEmailVector.begin() + i);
	UsersDisplayNameVector.erase(UsersDisplayNameVector.begin() + i);
	UsersSaltVector.erase(UsersSaltVector.begin() + i);
	UsersPermissionsVector.erase(UsersPermissionsVector.begin() + i);
	UsersUserIdVector.erase(UsersUserIdVector.begin() + i);
	UsersLastLoginAttemptVector.erase(UsersLastLoginAttemptVector.begin() + i);
	UsersAccountCreatedTimeVector.erase(UsersAccountCreatedTimeVector.begin() + i);
	UsersLoginFailureCountVector.erase(UsersLoginFailureCountVector.begin() + i);
	UsersLastModifierUsernameVector.erase(UsersLastModifierUsernameVector.begin() + i);
	UsersLastModifiedTimeVector.erase(UsersLastModifiedTimeVector.begin() + i);

	//save database
	return saveDatabaseToFile(DB_USERS);
}

//========================================================================================================================
unsigned int WebUsers::hexByteStrToInt(const char *h)
{
	unsigned int rv;
	char hs[3] = { h[0],h[1],'\0' };
	sscanf(hs, "%X", &rv);
	return rv;
}

//========================================================================================================================
void WebUsers::intToHexStr(unsigned char i, char *h)
{
	sprintf(h, "%2.2X", i);
}



//========================================================================================================================
//WebUsers::attemptActiveSession ---
//	Attempts login.
//
//	If new login, then new account code must match account creation time and account is made with pw
//    
//	if old login, password is checked
//	returns User Id, cookieCode in newAccountCode, and displayName in jumbledUser on success
//	else returns -1 and cookieCode "0" 
uint64_t WebUsers::attemptActiveSession(const std::string& uuid, std::string& jumbledUser,
		const std::string& jumbledPw, std::string& newAccountCode, const std::string& ip)
{
	//__COUTV__(ip);
	if(!checkIpAccess(ip))
	{
		__COUT_ERR__ << "rejected ip: " << ip << __E__;
		return NOT_FOUND_IN_DATABASE;
	}

	cleanupExpiredEntries(); //remove expired active and login sessions

	if (!CareAboutCookieCodes_) //NO SECURITY
	{
		uint64_t uid = getAdminUserID();
		jumbledUser = getUsersDisplayName(uid);
		newAccountCode = genCookieCode(); //return "dummy" cookie code by reference
		return uid;
	}

	uint64_t i;

	//search login sessions for uuid
	if ((i = searchLoginSessionDatabaseForUUID(uuid)) == NOT_FOUND_IN_DATABASE)
	{
		__COUT_ERR__ << "uuid: " << uuid << " is not found" << __E__;
		newAccountCode = "1"; //to indicate uuid was not found

		incrementIpBlacklistCount(ip); //increment ip blacklist counter

		return NOT_FOUND_IN_DATABASE;
	}
	++LoginSessionAttemptsVector[i];

	std::string user = dejumble(jumbledUser, LoginSessionIdVector[i]);
	__COUTV__(user);
	std::string pw = dejumble(jumbledPw, LoginSessionIdVector[i]);

	//search users for username
	if ((i = searchUsersDatabaseForUsername(user)) == NOT_FOUND_IN_DATABASE)
	{
		__COUT_ERR__ << "user: " << user << " is not found" << __E__;

		incrementIpBlacklistCount(ip); //increment ip blacklist counter

		return NOT_FOUND_IN_DATABASE;
	}
	else
		ipBlacklistCounts_[ip] = 0; //clear blacklist count

	UsersLastLoginAttemptVector[i] = time(0);

	if (isInactiveForGroup(UsersPermissionsVector[i]))
	{
		__MCOUT_ERR__("User '" << user << "' account INACTIVE (could be due to failed logins)" << __E__);
		return NOT_FOUND_IN_DATABASE;
	}

	if (UsersSaltVector[i] == "") //first login
	{
		__MCOUT__("First login attempt for user: " << user << __E__);

		char charTimeStr[10];
		sprintf(charTimeStr, "%d", int(UsersAccountCreatedTimeVector[i] & 0xffff));
		std::string tmpTimeStr = charTimeStr;
		if (newAccountCode != tmpTimeStr)
		{
			__COUT__ << "New account code did not match: " << tmpTimeStr << " != " << newAccountCode << __E__;
			saveDatabaseToFile(DB_USERS); //users db modified, so save
			return NOT_FOUND_IN_DATABASE;
		}

		//initial user account setup 

		//add until no collision (should 'never' be a collision)
		while (!addToHashesDatabase(sha512(user, pw, UsersSaltVector[i]))) //sha256 modifies UsersSaltVector[i]
		{
			//this should never happen, it would mean the user+pw+saltcontext was the same
			// but if it were to happen, try again...
			UsersSaltVector[i] = "";
		}

		__COUT__ << "\tHash added: " << HashesVector[HashesVector.size() - 1] << __E__;
	}
	else
	{
		std::string salt = UsersSaltVector[i];  //don't want to modify saved salt
		//__COUT__ << salt << " " << i << __E__;
		if (searchHashesDatabaseForHash(sha512(user, pw, salt)) == NOT_FOUND_IN_DATABASE)
		{
			__COUT__ << "Failed login for " << user << " with permissions " <<
					StringMacros::mapToString(UsersPermissionsVector[i]) <<	__E__;

			++UsersLoginFailureCountVector[i];
			if (UsersLoginFailureCountVector[i] >= USERS_MAX_LOGIN_FAILURES)
				UsersPermissionsVector[i][WebUsers::DEFAULT_USER_GROUP] = WebUsers::PERMISSION_LEVEL_INACTIVE; //Lock account

			__COUT_INFO__ << "User/pw for user '" << user << "' was not correct (Failed Attempt #" <<
					(int)UsersLoginFailureCountVector[i] << " of " <<
					(int)USERS_MAX_LOGIN_FAILURES << ")." << __E__;

			__COUTV__(isInactiveForGroup(UsersPermissionsVector[i]));
			if (isInactiveForGroup(UsersPermissionsVector[i]))
				__MCOUT_INFO__("Account '" << user << "' has been marked inactive due to too many failed login attempts (Failed Attempt #" <<
						(int)UsersLoginFailureCountVector[i] <<
						")! Note only admins can reactivate accounts." << __E__);


			saveDatabaseToFile(DB_USERS); //users db modified, so save
			return NOT_FOUND_IN_DATABASE;
		}
	}

	__MCOUT_INFO__("Login successful for: " << user << __E__);

	UsersLoginFailureCountVector[i] = 0;

	//record to login history for user (h==0) and on global server level (h==1)
	for (int h = 0; h < 2; ++h)
	{
		std::string fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_LOGIN_HISTORY_PATH + (h ? USERS_GLOBAL_HISTORY_FILE : UsersUsernameVector[i])
					+ "." + (std::string)USERS_LOGIN_HISTORY_FILETYPE;

		HttpXmlDocument histXml;

		if (histXml.loadXmlDocument(fn)) //not found
		{
			while (histXml.getChildrenCount() + 1 > (h ? USERS_GLOBAL_HISTORY_SIZE : USERS_LOGIN_HISTORY_SIZE))
				histXml.removeDataElement();
		}
		else
			__COUT__ << "No previous login history found." << __E__;

		//add new entry to history
		char entryStr[500];
		if (h)
			sprintf(entryStr, "Time=%lu Username=%s Permissions=%s UID=%lu",
					time(0), UsersUsernameVector[i].c_str(),
					StringMacros::mapToString(UsersPermissionsVector[i]).c_str(), UsersUserIdVector[i]);
		else
			sprintf(entryStr, "Time=%lu displayName=%s Permissions=%s UID=%lu",
					time(0), UsersDisplayNameVector[i].c_str(),
					StringMacros::mapToString(UsersPermissionsVector[i]).c_str(), UsersUserIdVector[i]);
		histXml.addTextElementToData(PREF_XML_LOGIN_HISTORY_FIELD, entryStr);

		//save file
		histXml.saveXmlDocument(fn);
	}

	//SUCCESS!!
	saveDatabaseToFile(DB_USERS); //users db modified, so save
	jumbledUser = UsersDisplayNameVector[i]; //pass by reference displayName
	newAccountCode = createNewActiveSession(UsersUserIdVector[i],ip); //return cookie code by reference
	return UsersUserIdVector[i]; //return user Id
}


//========================================================================================================================
//WebUsers::attemptActiveSessionWithCert ---
//	Attempts login using certificate.
//
//	returns User Id, cookieCode, and displayName in jumbledEmail on success
//	else returns -1 and cookieCode "0" 
uint64_t WebUsers::attemptActiveSessionWithCert(const std::string& uuid, std::string& email,
		std::string& cookieCode, std::string& user, const std::string& ip)
{
	if (!checkIpAccess(ip))
	{
		__COUT_ERR__ << "rejected ip: " << ip << __E__;
		return NOT_FOUND_IN_DATABASE;
	}

	cleanupExpiredEntries(); //remove expired active and login sessions

	if (!CareAboutCookieCodes_) //NO SECURITY
	{
		uint64_t uid = getAdminUserID();
		email = getUsersDisplayName(uid);
		cookieCode = genCookieCode(); //return "dummy" cookie code by reference
		return uid;
	}

	if (email == "")
	{
		__COUT__ << "Rejecting logon with blank fingerprint" << __E__;

		incrementIpBlacklistCount(ip); //increment ip blacklist counter

		return NOT_FOUND_IN_DATABASE;
	}

	uint64_t i;

	//search login sessions for uuid
	if ((i = searchLoginSessionDatabaseForUUID(uuid)) == NOT_FOUND_IN_DATABASE)
	{
		__COUT__ << "uuid: " << uuid << " is not found" << __E__;
		cookieCode = "1"; //to indicate uuid was not found

		incrementIpBlacklistCount(ip); //increment ip blacklist counter

		return NOT_FOUND_IN_DATABASE;
	}
	++LoginSessionAttemptsVector[i];

	email = getUserEmailFromFingerprint(email);
	__COUT__ << "DejumbledEmail = " << email << __E__;
	if (email == "")
	{
		__COUT__ << "Rejecting logon with unknown fingerprint" << __E__;

		incrementIpBlacklistCount(ip); //increment ip blacklist counter

		return NOT_FOUND_IN_DATABASE;
	}

	//search users for username
	if ((i = searchUsersDatabaseForUserEmail(email)) == NOT_FOUND_IN_DATABASE)
	{
		__COUT__ << "email: " << email << " is not found" << __E__;

		incrementIpBlacklistCount(ip); //increment ip blacklist counter

		return NOT_FOUND_IN_DATABASE;
	}
	else
		ipBlacklistCounts_[ip] = 0; //clear blacklist count

	user = getUsersUsername(i);

	UsersLastLoginAttemptVector[i] = time(0);
	if (isInactiveForGroup(UsersPermissionsVector[i]))
	{
		__MCOUT__ ("User '" << user << "' account INACTIVE (could be due to failed logins)." << __E__);
		return NOT_FOUND_IN_DATABASE;
	}

	if (UsersSaltVector[i] == "") //Can't be first login
	{
		return NOT_FOUND_IN_DATABASE;
	}

	__MCOUT__("Login successful for: " << user << __E__);

	UsersLoginFailureCountVector[i] = 0;

	//record to login history for user (h==0) and on global server level (h==1)
	for (int h = 0; h < 2; ++h)
	{
		std::string fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_LOGIN_HISTORY_PATH + (h ? USERS_GLOBAL_HISTORY_FILE : UsersUsernameVector[i])
					+ "." + (std::string)USERS_LOGIN_HISTORY_FILETYPE;

		HttpXmlDocument histXml;

		if (histXml.loadXmlDocument(fn)) //not found
		{
			while (histXml.getChildrenCount() + 1 > (h ? USERS_GLOBAL_HISTORY_SIZE : USERS_LOGIN_HISTORY_SIZE))
				histXml.removeDataElement();
		}
		else
			__COUT__ << "No previous login history found." << __E__;

		//add new entry to history
		char entryStr[500];
		if (h)
			sprintf(entryStr, "Time=%lu Username=%s Permissions=%s UID=%lu",
					time(0), UsersUsernameVector[i].c_str(),
					StringMacros::mapToString(UsersPermissionsVector[i]).c_str(), UsersUserIdVector[i]);
		else
			sprintf(entryStr, "Time=%lu displayName=%s Permissions=%s UID=%lu",
					time(0), UsersDisplayNameVector[i].c_str(),
					StringMacros::mapToString(UsersPermissionsVector[i]).c_str(), UsersUserIdVector[i]);
		histXml.addTextElementToData(PREF_XML_LOGIN_HISTORY_FIELD, entryStr);

		//save file
		histXml.saveXmlDocument(fn);
	}

	//SUCCESS!!
	saveDatabaseToFile(DB_USERS); //users db modified, so save
	email = UsersDisplayNameVector[i]; //pass by reference displayName
	cookieCode = createNewActiveSession(UsersUserIdVector[i],ip); //return cookie code by reference
	return UsersUserIdVector[i]; //return user Id
}

//========================================================================================================================
//WebUsers::searchActiveSessionDatabaseForUID ---
//	returns index if found, else -1
uint64_t WebUsers::searchActiveSessionDatabaseForCookie(const std::string& cookieCode) const
{
	uint64_t i = 0;
	for (; i < ActiveSessionCookieCodeVector.size(); ++i)
		if (ActiveSessionCookieCodeVector[i] == cookieCode) break;
	return (i == ActiveSessionCookieCodeVector.size()) ? NOT_FOUND_IN_DATABASE : i;
}


//========================================================================================================================
//WebUsers::isUsernameActive ---
//	returns true if found, else false
bool WebUsers::isUsernameActive(const std::string& username) const
{
	uint64_t u;
	if ((u = searchUsersDatabaseForUsername(username)) == NOT_FOUND_IN_DATABASE) return false;
	return isUserIdActive(UsersUserIdVector[u]);
}


//========================================================================================================================
//WebUsers::isUserIdActive ---
//	returns true if found, else false
bool WebUsers::isUserIdActive(uint64_t uid) const
{
	uint64_t i = 0;
	for (; i < ActiveSessionUserIdVector.size(); ++i)
		if (ActiveSessionUserIdVector[i] == uid) return true;
	return false;
}

//========================================================================================================================
//WebUsers::searchUsersDatabaseForUsername ---
//	returns index if found, else -1
uint64_t WebUsers::searchUsersDatabaseForUsername(const std::string& username) const
{
	uint64_t i = 0;
	for (; i < UsersUsernameVector.size(); ++i)
		if (UsersUsernameVector[i] == username) break;
	return (i == UsersUsernameVector.size()) ? NOT_FOUND_IN_DATABASE : i;
}

//========================================================================================================================
//WebUsers::searchUsersDatabaseForUserEmail ---
//	returns index if found, else -1
uint64_t WebUsers::searchUsersDatabaseForUserEmail(const std::string& useremail) const
{
	uint64_t i = 0;
	for (; i < UsersUserEmailVector.size(); ++i)
		if (UsersUserEmailVector[i] == useremail) break;
	return (i == UsersUserEmailVector.size()) ? NOT_FOUND_IN_DATABASE : i;
}

//========================================================================================================================
//WebUsers::searchUsersDatabaseForUserId ---
//	returns index if found, else -1
uint64_t WebUsers::searchUsersDatabaseForUserId(uint64_t uid) const
{
	uint64_t i = 0;
	for (; i < UsersUserIdVector.size(); ++i)
		if (UsersUserIdVector[i] == uid) break;
	return (i == UsersUserIdVector.size()) ? NOT_FOUND_IN_DATABASE : i;
}

//========================================================================================================================
//WebUsers::searchLoginSessionDatabaseForUUID ---
//	returns index if found, else -1
uint64_t WebUsers::searchLoginSessionDatabaseForUUID(const std::string& uuid) const
{
	uint64_t i = 0;
	for (; i < LoginSessionUUIDVector.size(); ++i)
		if (LoginSessionUUIDVector[i] == uuid) break;
	return (i == LoginSessionUUIDVector.size()) ? NOT_FOUND_IN_DATABASE : i;
}

//========================================================================================================================
//WebUsers::searchHashesDatabaseForHash ---
//	returns index if found, else -1
uint64_t WebUsers::searchHashesDatabaseForHash(const std::string& hash)
{
	uint64_t i = 0;
	//__COUT__ << i << " " << HashesVector.size() << " " << HashesAccessTimeVector.size() <<
	//		hash << __E__;
	for (; i < HashesVector.size(); ++i)
		if (HashesVector[i] == hash) break;
		//else
		//	__COUT__ << HashesVector[i] << " ?????? " << __E__;
	//__COUT__ << i << __E__;
	if (i < HashesAccessTimeVector.size()) //if found, means login successful, so update access time
		HashesAccessTimeVector.push_back((time(0) + (rand() % 2 ? 1 : -1)*(rand() % 30 * 24 * 60 * 60)) & 0x0FFFFFFFFFE000000);

	//__COUT__ << i << __E__;
	return (i == HashesVector.size()) ? NOT_FOUND_IN_DATABASE : i;
}

//========================================================================================================================
//WebUsers::addToHashesDatabase ---
//	returns false if hash already exists
//	else true for success
bool WebUsers::addToHashesDatabase(const std::string& hash)
{
	if (searchHashesDatabaseForHash(hash) != NOT_FOUND_IN_DATABASE)
	{
		__COUT__ << "Hash collision: " << hash << __E__;
		return false;
	}
	HashesVector.push_back(hash);
	HashesAccessTimeVector.push_back((time(0) + (rand() % 2 ? 1 : -1)*(rand() % 30 * 24 * 60 * 60)) & 0x0FFFFFFFFFE000000);
	//in seconds, blur by month and mask out changes on year time frame: 0xFFFFFFFF FE000000
	return saveDatabaseToFile(DB_HASHES);
}

//========================================================================================================================
//WebUsers::genCookieCode ---
std::string WebUsers::genCookieCode()
{
	char hexStr[3];
	std::string cc = "";
	for (uint32_t i = 0; i < COOKIE_CODE_LENGTH / 2; ++i)
	{
		intToHexStr(rand(), hexStr);
		cc.append(hexStr);
	}
	return cc;
}

//========================================================================================================================
//WebUsers::removeLoginSessionEntry ---
void WebUsers::removeLoginSessionEntry(unsigned int i)
{
	LoginSessionIdVector.erase(LoginSessionIdVector.begin() + i);
	LoginSessionUUIDVector.erase(LoginSessionUUIDVector.begin() + i);
	LoginSessionIpVector.erase(LoginSessionIpVector.begin() + i);
	LoginSessionStartTimeVector.erase(LoginSessionStartTimeVector.begin() + i);
	LoginSessionAttemptsVector.erase(LoginSessionAttemptsVector.begin() + i);
}

//========================================================================================================================
//WebUsers::createNewActiveSession ---
//	if asIndex is not specified (0), new session receives max(ActiveSessionIndex) for user +1.. always skipping 0.
//	In this ActiveSessionIndex should link a thread of cookieCodes
std::string WebUsers::createNewActiveSession(uint64_t uid, const std::string& ip, uint64_t asIndex)
{
	//__COUTV__(ip);
	ActiveSessionCookieCodeVector.push_back(genCookieCode());
	ActiveSessionIpVector.push_back(ip);
	ActiveSessionUserIdVector.push_back(uid);
	ActiveSessionStartTimeVector.push_back(time(0));

	if (asIndex) //this is a refresh of current active session
		ActiveSessionIndex.push_back(asIndex);
	else
	{
		//find max(ActiveSessionIndex)
		uint64_t max = 0;
		for (uint64_t j = 0; j < ActiveSessionIndex.size(); ++j)
			if (ActiveSessionUserIdVector[j] == uid && max < ActiveSessionIndex[j]) //new max
				max = ActiveSessionIndex[j];

		ActiveSessionIndex.push_back(max ? max + 1 : 1); //0 is illegal
	}

	return ActiveSessionCookieCodeVector[ActiveSessionCookieCodeVector.size() - 1];
}

//========================================================================================================================
//WebUsers::removeActiveSession ---
void WebUsers::removeActiveSessionEntry(unsigned int i)
{
	ActiveSessionCookieCodeVector.erase(ActiveSessionCookieCodeVector.begin() + i);
	ActiveSessionIpVector.erase(ActiveSessionIpVector.begin() + i);
	ActiveSessionUserIdVector.erase(ActiveSessionUserIdVector.begin() + i);
	ActiveSessionStartTimeVector.erase(ActiveSessionStartTimeVector.begin() + i);
	ActiveSessionIndex.erase(ActiveSessionIndex.begin() + i);
}

//========================================================================================================================
//WebUsers::refreshCookieCode ---
// Basic idea is to return valid cookieCode to user for future commands
// There are two issues that arise due to "same user - multiple location":
//		1. Multiple Tabs Scenario (same browser cookie)
//		2. Multiple Browser Scenario (separate login chain)
// We want to allow both modes of operation.
//
// Solution to 1. : long expiration and overlap times
//	return most recent cookie for ActiveSessionIndex (should be deepest in vector always)
//		- If half of expiration time is up, a new cookie is generated as most recent
//		but previous is maintained and start time is changed to accommodate overlap time.
//		- Overlap time should be enough to allow other tabs to take an action and 
//		receive the new cookie code.
//	
// Solution to 2. : ActiveSessionIndex
//	return most recent cookie for ActiveSessionIndex (should be deepest in vector always)
//		- Independent browsers will have independent cookie chains for same user
//		based on ActiveSessionIndex.
//		- Can use ActiveSessionIndex to detect old logins and log them out.	
// 
//	enableRefresh added for automatic actions that take place, that should still get
//		the most recent code, but should not generate new codes (set enableRefresh = false).
std::string WebUsers::refreshCookieCode(unsigned int i, bool enableRefresh)
{
	//find most recent cookie for ActiveSessionIndex (should be deepest in vector always)
	for (uint64_t j = ActiveSessionUserIdVector.size() - 1; j != (uint64_t)-1; --j) //reverse iterate vector
		if (ActiveSessionUserIdVector[j] == ActiveSessionUserIdVector[i] &&
				ActiveSessionIndex[j] == ActiveSessionIndex[i])  //if uid and asIndex match, found match
		{
			//found!

			//If half of expiration time is up, a new cookie is generated as most recent
			if (enableRefresh && (time(0) - ActiveSessionStartTimeVector[j] > ACTIVE_SESSION_EXPIRATION_TIME / 2))
			{
				//but previous is maintained and start time is changed to accommodate overlap time.
				ActiveSessionStartTimeVector[j] = time(0) - ACTIVE_SESSION_EXPIRATION_TIME +
						ACTIVE_SESSION_COOKIE_OVERLAP_TIME; //give time window for stale cookie commands before expiring

				//create new active cookieCode with same ActiveSessionIndex, will now be found as most recent
				return createNewActiveSession(ActiveSessionUserIdVector[i], ActiveSessionIpVector[i], ActiveSessionIndex[i]);
			}

			return ActiveSessionCookieCodeVector[j]; //cookieCode is unchanged
		}

	return "0"; //failure, should be impossible since i is already validated
}

//========================================================================================================================
//WebUsers::IsCookieActive ---
//	returns User Id on success, returns by reference refreshed cookieCode and displayName if cookieCode/user combo is still active
//	displayName is returned in username std::string
//	else returns -1
uint64_t WebUsers::isCookieCodeActiveForLogin(const std::string& uuid, std::string& cookieCode,
		std::string& username)
{
	if (!CareAboutCookieCodes_)
		return getAdminUserID(); //always successful

	//else
	//	__COUT__ << "I care about cookies?!?!?!*************************************************" << __E__;

	if (!ActiveSessionStartTimeVector.size()) return NOT_FOUND_IN_DATABASE; //no active sessions, so do nothing

	uint64_t  i, j; //used to iterate and search

	//find uuid in login session database else return "0"	
	if ((i = searchLoginSessionDatabaseForUUID(uuid)) == NOT_FOUND_IN_DATABASE)
	{
		__COUT__ << "uuid not found: " << uuid << __E__;
		return NOT_FOUND_IN_DATABASE;
	}

	username = dejumble(username, LoginSessionIdVector[i]); //dejumble user for cookie check

	//search active users for cookie code	
	if ((i = searchActiveSessionDatabaseForCookie(cookieCode)) == NOT_FOUND_IN_DATABASE)
	{
		__COUT__ << "Cookie code not found" << __E__;
		return NOT_FOUND_IN_DATABASE;
	}

	//search users for user id	
	if ((j = searchUsersDatabaseForUserId(ActiveSessionUserIdVector[i])) == NOT_FOUND_IN_DATABASE)
	{
		__COUT__ << "User ID not found" << __E__;
		return NOT_FOUND_IN_DATABASE;
	}

	//match username, with one found
	if (UsersUsernameVector[j] != username)
	{

		__COUT__ << "cookieCode: " << cookieCode << " was.." << __E__;
		__COUT__ << "username: " << username << " is not found" << __E__;
		return NOT_FOUND_IN_DATABASE;
	}

	username = UsersDisplayNameVector[j]; //return display name by reference
	cookieCode = refreshCookieCode(i); //refresh cookie by reference
	return UsersUserIdVector[j]; //return user ID
}

//========================================================================================================================
//WebUsers::getActiveSessionCountForUser ---
//	Returns count of unique ActiveSessionIndex entries for user's uid
uint64_t WebUsers::getActiveSessionCountForUser(uint64_t uid)
{
	bool unique;
	std::vector<uint64_t> uniqueAsi; //maintain unique as indices for reference

	uint64_t i, j;
	for (i = 0; i < ActiveSessionUserIdVector.size(); ++i)
		if (ActiveSessionUserIdVector[i] == uid)  //found active session for user
		{
			//check if ActiveSessionIndex is unique
			unique = true;

			for (j = 0; j < uniqueAsi.size(); ++j)
				if (uniqueAsi[j] == ActiveSessionIndex[i])
				{
					unique = false; break;
				}

			if (unique) //unique! so count and save
				uniqueAsi.push_back(ActiveSessionIndex[i]);
		}

	__COUT__ << "Found " << uniqueAsi.size() << " active sessions for uid " << uid << __E__;

	return uniqueAsi.size();
}

//========================================================================================================================
//WebUsers::checkIpAccess ---
//	checks user defined accept,
//	then checks reject IP file
//	then checks blacklist file
//	return true if ip is accepted, and false if rejected
bool WebUsers::checkIpAccess(const std::string& ip)
{
	if(ip == "0") return true; //always accept dummy IP

	FILE *fp = fopen((IP_ACCEPT_FILE).c_str(),"r");
	char line[300];
	size_t len;

	if(fp)
	{
		while(fgets(line,300,fp))
		{
			len = strlen(line);
			//remove new line
			if(len > 2 && line[len-1] == '\n')
				line[len-1] = '\0';
			if(StringMacros::wildCardMatch(ip,line))
				return true; //found in accept file, so accept
		}

		fclose(fp);
	}

	fp = fopen((IP_REJECT_FILE).c_str(),"r");
	if(fp)
	{
		while(fgets(line,300,fp))
		{
			len = strlen(line);
			//remove new line
			if(len > 2 && line[len-1] == '\n')
				line[len-1] = '\0';
			if(StringMacros::wildCardMatch(ip,line))
				return false; //found in reject file, so reject
		}

		fclose(fp);
	}

	fp = fopen((IP_BLACKLIST_FILE).c_str(),"r");
	if(fp)
	{
		while(fgets(line,300,fp))
		{
			len = strlen(line);
			//remove new line
			if(len > 2 && line[len-1] == '\n')
				line[len-1] = '\0';
			if(StringMacros::wildCardMatch(ip,line))
				return false; //found in blacklist file, so reject
		}

		fclose(fp);
	}

	//default to accept if nothing triggered above
	return true;
}

//========================================================================================================================
//WebUsers::incrementIpBlacklistCount ---
void WebUsers::incrementIpBlacklistCount(const std::string& ip)
{
	//increment ip blacklist counter
	auto it = ipBlacklistCounts_.find(ip);
	if(it == ipBlacklistCounts_.end())
	{
		__COUT__ << "First error for ip '" << ip << "'" << __E__;
		ipBlacklistCounts_[ip] = 1;
	}
	else
	{
		++(it->second);

		if(it->second >= IP_BLACKLIST_COUNT_THRESHOLD)
		{
			__MCOUT__("Adding IP '" << ip << "' to blacklist!" << __E__);

			//append to blacklisted IP to generated IP reject file
			FILE *fp = fopen((IP_BLACKLIST_FILE).c_str(),"a");
			if(!fp)
			{
				__SS__ << "IP black list file '" << IP_BLACKLIST_FILE << "' could not be opened." << __E__;
				__MCOUT_ERR__(ss.str());
				return;
			}
			fprintf(fp,"%s\n",ip.c_str());
			fclose(fp);
		}
	}
}

//========================================================================================================================
//WebUsers::getUsersDisplayName ---
std::string WebUsers::getUsersDisplayName(uint64_t uid)
{
	uint64_t i;
	if ((i = searchUsersDatabaseForUserId(uid)) == NOT_FOUND_IN_DATABASE) return "";
	return UsersDisplayNameVector[i];
}

//========================================================================================================================
//WebUsers::getUsersUsername ---
std::string WebUsers::getUsersUsername(uint64_t uid)
{
	uint64_t i;
	if ((i = searchUsersDatabaseForUserId(uid)) == NOT_FOUND_IN_DATABASE) return "";
	return UsersUsernameVector[i];
}

//========================================================================================================================
//WebUsers::cookieCodeLogout ---
//	Used to logout user based on cookieCode and ActiveSessionIndex
//		logoutOtherUserSessions true logs out all of user's other sessions by uid
//		Note: when true, user will remain logged in to current active session
//		logoutOtherUserSessions false logs out only this cookieCode/ActiveSessionIndex
//		Note: when false, user will remain logged in other locations based different ActiveSessionIndex
//
//  on failure, returns -1
// 	on success returns number of active sessions that were removed
uint64_t WebUsers::cookieCodeLogout(const std::string& cookieCode, bool logoutOtherUserSessions,
		uint64_t *userId, const std::string& ip)
{
	uint64_t  i;

	//search active users for cookie code
	if ((i = searchActiveSessionDatabaseForCookie(cookieCode)) == NOT_FOUND_IN_DATABASE)
	{
		__COUT__ << "Cookie code not found" << __E__;

		incrementIpBlacklistCount(ip); //increment ip blacklist counter

		return NOT_FOUND_IN_DATABASE;
	}
	else
		ipBlacklistCounts_[ip] = 0; //clear blacklist count


	//check ip
	if (ActiveSessionIpVector[i] != ip)
	{
		__COUT__ << "IP does not match active session" << __E__;
		return NOT_FOUND_IN_DATABASE;
	}

	//found valid active session i
	//if logoutOtherUserSessions
	//	remove active sessions that match ActiveSessionUserIdVector[i] and ActiveSessionIndex[i]
	//else
	//	remove active sessions that match ActiveSessionUserIdVector[i] but not ActiveSessionIndex[i]

	uint64_t asi = ActiveSessionIndex[i];
	uint64_t uid = ActiveSessionUserIdVector[i];
	if (userId) *userId = uid; //return uid if requested
	uint64_t logoutCount = 0;

	i = 0;
	while (i < ActiveSessionIndex.size())
	{
		if ((logoutOtherUserSessions && ActiveSessionUserIdVector[i] == uid &&
				ActiveSessionIndex[i] != asi) ||
				(!logoutOtherUserSessions && ActiveSessionUserIdVector[i] == uid &&
						ActiveSessionIndex[i] == asi))
		{
			__COUT__ << "Logging out of active session " << ActiveSessionUserIdVector[i]
																					  << "-" << ActiveSessionIndex[i] << __E__;
			removeActiveSessionEntry(i);
			++logoutCount;
		}
		else  //only increment if no delete
			++i;
	}

	__COUT__ << "Found and removed active session count = " << logoutCount << __E__;

	return logoutCount;
}

//========================================================================================================================
//WebUsers::getUserInfoForCookie ---
bool WebUsers::getUserInfoForCookie(std::string& cookieCode,
		std::string *userName, std::string *displayName,
		uint64_t *activeSessionIndex)
{
	if (userName) *userName = "";
	if (displayName) *displayName = "";

	if (!CareAboutCookieCodes_) //NO SECURITY, return admin
	{
		uint64_t uid = getAdminUserID();
		if (userName) *userName = getUsersUsername(uid);
		if (displayName) *displayName = getUsersDisplayName(uid);
		if (activeSessionIndex) *activeSessionIndex = -1;
		return true;
	}

	uint64_t  i, j;

	//search active users for cookie code
	if ((i = searchActiveSessionDatabaseForCookie(cookieCode)) == NOT_FOUND_IN_DATABASE)
	{
		__COUT__ << "cookieCode NOT_FOUND_IN_DATABASE" << __E__;
		return false;
	}

	//get Users record
	if ((j = searchUsersDatabaseForUserId(ActiveSessionUserIdVector[i])) == NOT_FOUND_IN_DATABASE)
	{
		__COUT__ << "ActiveSessionUserIdVector NOT_FOUND_IN_DATABASE" << __E__;
		return false;
	}

	if (userName) *userName = UsersUsernameVector[j];
	if (displayName) *displayName = UsersDisplayNameVector[j];
	if (activeSessionIndex) *activeSessionIndex = ActiveSessionIndex[i];
	return true;
}

//========================================================================================================================
//WebUsers::isCookieCodeActiveForRequest ---
//	Used to verify cookie code for all general user requests
//  cookieCode/ip must be active to pass
//
//  cookieCode is passed by reference. It is refreshed, if refresh=true on success and may be modified.
//		on success, if userPermissions and/or uid are not null, the permissions and uid are returned
//  on failure, cookieCode contains error message to return to client
//
//  If do NOT care about cookie code, then returns uid 0 (admin)
//		and grants full permissions
bool WebUsers::cookieCodeIsActiveForRequest(std::string& cookieCode,
		std::map<std::string /*groupName*/,WebUsers::permissionLevel_t>* userPermissions,
		uint64_t *uid, const std::string& ip,
		bool refresh, std::string *userWithLock,
		uint64_t* activeUserSessionIndex)
{
	//__COUTV__(ip);

	//check ip black list and increment counter if cookie code not found
	if(!checkIpAccess(ip))
	{
		__COUT_ERR__ << "User IP rejected." << __E__;
		cookieCode = REQ_NO_LOGIN_RESPONSE;
		return false;
	}

	cleanupExpiredEntries(); //remove expired cookies

	uint64_t  i, j;

	//__COUT__ << "I care about cookie codes: " << CareAboutCookieCodes_ << __E__;
	//__COUT__ << "refresh cookie " << refresh << __E__;

	if (!CareAboutCookieCodes_) //No Security, so grant admin
	{
		if (userPermissions) 	*userPermissions =
				std::map<std::string /*groupName*/,WebUsers::permissionLevel_t>({{
					WebUsers::DEFAULT_USER_GROUP,
					WebUsers::PERMISSION_LEVEL_ADMIN
				}});
		if (uid) 				*uid = getAdminUserID();
		if (userWithLock)		*userWithLock = usersUsernameWithLock_;
		if (activeUserSessionIndex) *activeUserSessionIndex = -1;

		if(cookieCode.size() != COOKIE_CODE_LENGTH)
			cookieCode = genCookieCode(); //return "dummy" cookie code

		return true;
	}
	//else using security!

	//search active users for cookie code
	if ((i = searchActiveSessionDatabaseForCookie(cookieCode)) == NOT_FOUND_IN_DATABASE)
	{
		__COUT_ERR__ << "Cookie code not found" << __E__;
		cookieCode = REQ_NO_LOGIN_RESPONSE;

		incrementIpBlacklistCount(ip); //increment ip blacklist counter

		return false;
	}
	else
		ipBlacklistCounts_[ip] = 0; //clear blacklist count

	//check ip
	if (ip != "0" && ActiveSessionIpVector[i] != ip)
	{
		__COUTV__(ActiveSessionIpVector[i]);
		//__COUTV__(ip);
		__COUT_ERR__ << "IP does not match active session." << __E__;
		cookieCode = REQ_NO_LOGIN_RESPONSE;
		return false;
	}

	//get Users record
	if ((j = searchUsersDatabaseForUserId(ActiveSessionUserIdVector[i])) == NOT_FOUND_IN_DATABASE)
	{
		__COUT_ERR__ << "User ID not found" << __E__;
		cookieCode = REQ_NO_LOGIN_RESPONSE;
		return false;
	}

	std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> tmpPerm =
			getPermissionsForUser(UsersUserIdVector[j]);

	if (isInactiveForGroup(tmpPerm)) //Check for inactive for all requests!
	{
		cookieCode = REQ_NO_PERMISSION_RESPONSE;
		return false;
	}

	//success!
	if (userPermissions)	*userPermissions = tmpPerm;
	if (uid)			  	*uid = UsersUserIdVector[j];
	if (userWithLock)		*userWithLock = usersUsernameWithLock_;
	if (activeUserSessionIndex) *activeUserSessionIndex = ActiveSessionIndex[i];

	cookieCode = refreshCookieCode(i, refresh); //refresh cookie by reference

	return true;
} // end cookieCodeIsActiveForRequest()

//========================================================================================================================
//WebUsers::cleanupExpiredEntries ---
//	cleanup expired entries form Login Session and Active Session databases
//	check if usersUsernameWithLock_ is still active
//  return the vector of logged out user names if a parameter
//		if not a parameter, store logged out user names for next time called with parameter
void WebUsers::cleanupExpiredEntries(std::vector<std::string> *loggedOutUsernames)
{
	uint64_t  i; //used to iterate and search
	uint64_t  tmpUid;

	if (loggedOutUsernames) //return logged out users this time and clear storage vector
	{
		for (i = 0; i < UsersLoggedOutUsernames_.size(); ++i)
			loggedOutUsernames->push_back(UsersLoggedOutUsernames_[i]);
		UsersLoggedOutUsernames_.clear();
	}



	//remove expired entries from Login Session
	for (i = 0; i < LoginSessionStartTimeVector.size(); ++i)
		if (LoginSessionStartTimeVector[i] + LOGIN_SESSION_EXPIRATION_TIME < time(0) || //expired
				LoginSessionAttemptsVector[i] > LOGIN_SESSION_ATTEMPTS_MAX)
		{
			//__COUT__ << "Found expired userId: " << LoginSessionUUIDVector[i] <<
			//	" at time " << LoginSessionStartTimeVector[i] << " with attempts " << LoginSessionAttemptsVector[i] << __E__;

			removeLoginSessionEntry(i);
			--i; //rewind loop
		}

	//declare structures for ascii time
	//	struct tm * timeinfo;
	//	time_t tmpt;
	//	char tstr[200];
	//	timeinfo = localtime ( &(tmpt=time(0)) );
	//	sprintf(tstr,"\"%s\"",asctime (timeinfo)); tstr[strlen(tstr)-2] = '\"';
	//__COUT__ << "Current time is: " << time(0) << " " << tstr << __E__;

	//remove expired entries from Active Session
	for (i = 0; i < ActiveSessionStartTimeVector.size(); ++i)
		if (ActiveSessionStartTimeVector[i] + ACTIVE_SESSION_EXPIRATION_TIME <= time(0)) //expired
		{
			//timeinfo = localtime (&(tmpt=ActiveSessionStartTimeVector[i]));
			//sprintf(tstr,"\"%s\"",asctime (timeinfo)); tstr[strlen(tstr)-2] = '\"';
			//__COUT__ << "Found expired user: " << ActiveSessionUserIdVector[i] <<
			//	" start time " << tstr << " i: " << i << " size: " << ActiveSessionStartTimeVector.size()
			//	<< __E__;
			tmpUid = ActiveSessionUserIdVector[i];
			removeActiveSessionEntry(i);



			if (!isUserIdActive(tmpUid)) //if uid no longer active, then user was completely logged out
			{
				if (loggedOutUsernames) //return logged out users this time
					loggedOutUsernames->push_back(UsersUsernameVector[searchUsersDatabaseForUserId(tmpUid)]);
				else  //store for next time requested as parameter
					UsersLoggedOutUsernames_.push_back(UsersUsernameVector[searchUsersDatabaseForUserId(tmpUid)]);
			}

			--i; //rewind loop
		}
	//		else
	//		{
	//			timeinfo = localtime (&(tmpt=ActiveSessionStartTimeVector[i] + ACTIVE_SESSION_EXPIRATION_TIME));
	//			sprintf(tstr,"\"%s\"",asctime (timeinfo)); tstr[strlen(tstr)-2] = '\"';
	//
	//			//__COUT__ << "Found user: " << ActiveSessionUserIdVector[i] << "-" << ActiveSessionIndex[i] <<
	//			//	" expires " << tstr <<
	//    		//	" sec left " << ActiveSessionStartTimeVector[i] + ACTIVE_SESSION_EXPIRATION_TIME - time(0) << __E__;
	//
	//		}

	//__COUT__ << "Found usersUsernameWithLock_: " << usersUsernameWithLock_ << " - " << userWithLockVerified << __E__;
	if (CareAboutCookieCodes_ && !isUsernameActive(usersUsernameWithLock_)) //unlock if user no longer logged in
		usersUsernameWithLock_ = "";
}

//========================================================================================================================
//createNewLoginSession
//	adds a new login session id to database 
//		inputs: UUID
//		checks that UUID is unique
//		initializes database entry and returns sessionId std::string
//		return "" on failure
std::string WebUsers::createNewLoginSession(const std::string& UUID, const std::string& ip)
{
	__COUTV__(UUID);
	//__COUTV__(ip);

	uint64_t i = 0;
	for (; i < LoginSessionUUIDVector.size(); ++i)
		if (LoginSessionUUIDVector[i] == UUID) break;

	if (i != LoginSessionUUIDVector.size())
	{
		__COUT_ERR__ << "UUID: " << UUID << " is not unique" << __E__;
		return "";
	}
	//else UUID is unique

	LoginSessionUUIDVector.push_back(UUID);

	//generate sessionId
	char hexStr[3];
	std::string sid = "";
	for (i = 0; i < SESSION_ID_LENGTH / 2; ++i)
	{
		intToHexStr(rand(), hexStr);
		sid.append(hexStr);
	}
	LoginSessionIdVector.push_back(sid);
	LoginSessionIpVector.push_back(ip);
	LoginSessionStartTimeVector.push_back(time(0));
	LoginSessionAttemptsVector.push_back(0);

	return sid;
}



//========================================================================================================================
//WebUsers::sha512
//	performs SHA-512 encoding using openssl linux library crypto on context+user+password
//	if context is empty std::string "", context is generated and returned by reference
//	hashed result is returned
std::string	WebUsers::sha512(const std::string& user, const std::string& password, std::string& salt)
{
	SHA512_CTX sha512_context;
	char hexStr[3];

	if (salt == "") //generate context
	{
		SHA512_Init(&sha512_context);

		for (unsigned int i = 0; i < 8; ++i)
			sha512_context.h[i] += rand();

		for (unsigned int i = 0; i < sizeof(SHA512_CTX); ++i)
		{
			intToHexStr((uint8_t)(((uint8_t *)(&sha512_context))[i]), hexStr);

			salt.append(hexStr);
		}
		//__COUT__ << salt << __E__;
	}
	else //use existing context
	{

		//__COUT__ << salt << __E__;

		for (unsigned int i = 0; i < sizeof(SHA512_CTX); ++i)
			((uint8_t *)(&sha512_context))[i] = hexByteStrToInt(&(salt.c_str()[i * 2]));
	}

	std::string strToHash = salt + user + password;

	//__COUT__ << salt << __E__;
	unsigned char hash[SHA512_DIGEST_LENGTH];
	//__COUT__ << salt << __E__;
	char retHash[SHA512_DIGEST_LENGTH * 2 + 1];
	//__COUT__ << strToHash.length() << " " << strToHash << __E__;


	//__COUT__ << "If crashing occurs here, may be an illegal salt context." << __E__;
	SHA512_Update(&sha512_context, strToHash.c_str(), strToHash.length());

	SHA512_Final(hash, &sha512_context);

	//__COUT__ << salt << __E__;
	int i = 0;
	for (i = 0; i < SHA512_DIGEST_LENGTH; i++)
		sprintf(retHash + (i * 2), "%02x", hash[i]);

	//__COUT__ << salt << __E__;
	retHash[SHA512_DIGEST_LENGTH * 2] = '\0';


	//__COUT__ << salt << __E__;


	return retHash;
}


//========================================================================================================================
//WebUsers::dejumble
//	the client sends username and pw jumbled for http transmission
//	this function dejumbles
std::string WebUsers::dejumble(const std::string& u, const std::string& s)
{

	if (s.length() != SESSION_ID_LENGTH) return ""; //session std::string must be even

	const int ss = s.length() / 2;
	int p = hexByteStrToInt(&(s.c_str()[0])) % ss;
	int n = hexByteStrToInt(&(s.c_str()[p * 2])) % ss;
	int len = (hexByteStrToInt(&(u.c_str()[p * 2])) - p - n + ss * 3) % ss;

	std::vector<bool> x(ss);
	for (int i = 0; i < ss; ++i) x[i] = 0;
	x[p] = 1;

	int c = hexByteStrToInt(&(u.c_str()[p * 2]));

	std::string user = "";

	for (int l = 0; l < len; ++l)
	{
		p = (p + hexByteStrToInt(&(s.c_str()[p * 2]))) % ss;
		while (x[p]) p = (p + 1) % ss;
		x[p] = 1;
		n = hexByteStrToInt(&(s.c_str()[p * 2]));
		user.append(1, (hexByteStrToInt(&(u.c_str()[p * 2])) - c - n + ss * 4) % ss);
		c = hexByteStrToInt(&(u.c_str()[p * 2]));
	}

	return user;
}

//========================================================================================================================
//WebUsers::getPermissionForUser
// return WebUsers::PERMISSION_LEVEL_INACTIVE if invalid index
std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> WebUsers::getPermissionsForUser(
		uint64_t uid)
{
	//__COUTV__(uid);
	uint64_t userIndex = searchUsersDatabaseForUserId(uid);
	//__COUTV__(userIndex); __COUTV__(UsersPermissionsVector.size());
	if (userIndex < UsersPermissionsVector.size())
		return UsersPermissionsVector[userIndex];

	//else return all user inactive map
	std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> retErrorMap;
	retErrorMap[WebUsers::DEFAULT_USER_GROUP] =
			WebUsers::PERMISSION_LEVEL_INACTIVE;
	return retErrorMap;
}

//========================================================================================================================
WebUsers::permissionLevel_t WebUsers::getPermissionLevelForGroup(
		std::map<std::string /*groupName*/,WebUsers::permissionLevel_t>& permissionMap,
		const std::string& groupName)
{
	auto it = permissionMap.find(groupName);
	if(it == permissionMap.end())
	{
		__COUT__ << "Group name '" << groupName << "' not found - assuming inactive user in this group." << __E__;
		return WebUsers::PERMISSION_LEVEL_INACTIVE;
	}
	return it->second;
}

//========================================================================================================================
bool WebUsers::isInactiveForGroup(
		std::map<std::string /*groupName*/,WebUsers::permissionLevel_t>& permissionMap,
		const std::string& groupName)
{
	return getPermissionLevelForGroup(permissionMap,groupName) ==
			WebUsers::PERMISSION_LEVEL_INACTIVE;
}

//========================================================================================================================
bool WebUsers::isAdminForGroup(
		std::map<std::string /*groupName*/,WebUsers::permissionLevel_t>& permissionMap,
		const std::string& groupName)
{
	return getPermissionLevelForGroup(permissionMap,groupName) ==
			WebUsers::PERMISSION_LEVEL_ADMIN;
}

//========================================================================================================================
//WebUsers::getPermissionForUser
// return 0 if invalid index
std::string WebUsers::getTooltipFilename(
		const std::string& username, const std::string& srcFile,
		const std::string& srcFunc, const std::string& srcId)
{
	std::string filename = (std::string)WEB_LOGIN_DB_PATH + TOOLTIP_DB_PATH + "/";

	//make tooltip directory if not there
	//	note: this is static so WebUsers constructor has not necessarily been called
	mkdir(((std::string)WEB_LOGIN_DB_PATH).c_str(), 0755);
	mkdir(((std::string)WEB_LOGIN_DB_PATH + USERS_DB_PATH).c_str(), 0755);
	mkdir(filename.c_str(), 0755);

	for (const char& c : username)
		if (	//only keep alpha numeric
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9'))
			filename += c;
	filename += "/";

	//make username tooltip directory if not there
	mkdir(filename.c_str(), 0755);

	for (const char& c : srcFile)
		if (	//only keep alpha numeric
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9'))
			filename += c;
	filename += "_";
	for (const char& c : srcFunc)
		if (	//only keep alpha numeric
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9'))
			filename += c;
	filename += "_";
	for (const char& c : srcId)
		if (	//only keep alpha numeric
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9'))
			filename += c;
	filename += ".tip";
	//__COUT__ << "filename " << filename << __E__;
	return filename;
}

std::string ots::WebUsers::getUserEmailFromFingerprint(const std::string& fingerprint)
{
	std::ifstream f(WEB_LOGIN_CERTDATA_PATH);
	if (f.is_open())
	{
		std::string email;
		std::string fp;
		getline(f, email);
		getline(f, fp);
		certFingerprints_[email] = fp;
		f.close();
		remove(WEB_LOGIN_CERTDATA_PATH.c_str());
	}

	for (auto fp : certFingerprints_)
	{
		if (fp.second == fingerprint) return fp.first;
	}
	return "";
}

//========================================================================================================================
//WebUsers::tooltipSetNeverShowForUsername
//	temporarySilence has priority over the neverShow setting
void WebUsers::tooltipSetNeverShowForUsername(const std::string& username,
		HttpXmlDocument *xmldoc,
		const std::string& srcFile, const std::string& srcFunc,
		const std::string& srcId, bool doNeverShow, bool temporarySilence)
{

	__COUT__ << "Setting tooltip never show for user: " << username <<
			" to " << doNeverShow << " (temporarySilence=" <<
			temporarySilence << ")" << __E__;


	std::string filename = getTooltipFilename(username, srcFile, srcFunc, srcId);
	FILE *fp = fopen(filename.c_str(), "w");
	if (fp)
	{	//file exists, so do NOT show tooltip
		if (temporarySilence)
			fprintf(fp, "%ld", time(0) + 60 * 60); //mute for an hour
		else
			fputc(doNeverShow ? '1' : '0', fp);
		fclose(fp);
	}
	else //default to show tool tip
		__COUT_ERR__ << "Big problme with tooltips! File not accessible: " << filename << __E__;
}

//========================================================================================================================
//WebUsers::tooltipCheckForUsername
//	read file for tooltip
//		if not 1 then never show
//		if 0 then "always show"
//		if other then treat as temporary mute..
//			i.e. if time(0) > val show
void WebUsers::tooltipCheckForUsername(const std::string& username,
		HttpXmlDocument *xmldoc,
		const std::string& srcFile, const std::string& srcFunc,
		const std::string& srcId)
{
	if (srcId == "ALWAYS")
	{
		//ALWAYS shows tool tip
		xmldoc->addTextElementToData("ShowTooltip", "1");
		return;
	}

	//	__COUT__ << "username " << username << __E__;
	//	__COUT__ << "srcFile " << srcFile << __E__;
	//	__COUT__ << "srcFunc " << srcFunc << __E__;
	//	__COUT__ << "srcId " << srcId << __E__;
	//__COUT__ << "Checking tooltip for user: " << username << __E__;



	std::string filename = getTooltipFilename(username, srcFile, srcFunc, srcId);
	FILE *fp = fopen(filename.c_str(), "r");
	if (fp)
	{	//file exists, so do NOT show tooltip
		time_t val;
		char line[100];
		fgets(line, 100, fp);
		//int val = fgetc(fp);
		sscanf(line, "%ld", &val);
		__COUT__ << "tooltip value read = " << val << __E__;
		fclose(fp);

		//if first line in file is a 1 then do not show
		//	else show if current time is greater than value
		xmldoc->addTextElementToData("ShowTooltip", val == 1 ? "0" :
				(time(0) > val ? "1" : "0"));
	}
	else //default to show tool tip
		xmldoc->addTextElementToData("ShowTooltip", "1");
}

//========================================================================================================================
//WebUsers::resetAllUserTooltips
void WebUsers::resetAllUserTooltips(const std::string& userNeedle)
{
	std::system(("rm -rf " + (std::string)WEB_LOGIN_DB_PATH + TOOLTIP_DB_PATH +
			"/" + userNeedle).c_str());
	__COUT__ << "Successfully reset Tooltips for user " << userNeedle << __E__;
}

//========================================================================================================================
//WebUsers::insertGetSettingsResponse
//  add settings to xml document
//  all active users have permissions of at least 1 so have web preferences:
//      -background color
//      -dashboard color
//      -window color
//      -3 user defaults for window layouts(and current), can set current as one of defaults
//  super users have account controls:
//      -list of user accounts to edit permissions, display name, or delete account
//      -add new account
//	...and super users have system default window layout
//		-2 system defaults for window layouts
//
//	layout settings explanation
//		0 = no windows, never set, empty desktop
//		example 2 layouts set, 2 not, 
//			[<win name>, <win subname>, <win url>, <x>, <y>, <w>, <h>]; [<win name>, <win subname>, <win url>, <x>, <y>, <w>, <h>]...];0;0
void WebUsers::insertSettingsForUser(uint64_t uid, HttpXmlDocument *xmldoc, bool includeAccounts)
{
	std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> permissionMap =
			getPermissionsForUser(uid);
	__COUTV__(StringMacros::mapToString(permissionMap));
	if (isInactiveForGroup(permissionMap)) return; //not an active user

	uint64_t	userIndex = searchUsersDatabaseForUserId(uid);
	__COUT__ << "Gettings settings for user: " << UsersUsernameVector[userIndex] << __E__;

	std::string fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_PREFERENCES_PATH + UsersUsernameVector[userIndex] + "." + (std::string)USERS_PREFERENCES_FILETYPE;

	HttpXmlDocument prefXml;

	__COUT__ << "Preferences file: " << fn << __E__;

	if (!prefXml.loadXmlDocument(fn))
	{
		__COUT__ << "Preferences are defaults." << __E__;
		//insert defaults, no pref document found
		xmldoc->addTextElementToData(PREF_XML_BGCOLOR_FIELD, PREF_XML_BGCOLOR_DEFAULT);
		xmldoc->addTextElementToData(PREF_XML_DBCOLOR_FIELD, PREF_XML_DBCOLOR_DEFAULT);
		xmldoc->addTextElementToData(PREF_XML_WINCOLOR_FIELD, PREF_XML_WINCOLOR_DEFAULT);
		xmldoc->addTextElementToData(PREF_XML_LAYOUT_FIELD, PREF_XML_LAYOUT_DEFAULT);
	}
	else
	{
		__COUT__ << "Saved Preferences found." << __E__;
		xmldoc->copyDataChildren(prefXml);
	}

	char permStr[10];

	//add settings if super user
	if (includeAccounts &&
			isAdminForGroup(permissionMap))
	{

		__COUT__ << "Admin on our hands" << __E__;

		xmldoc->addTextElementToData(PREF_XML_ACCOUNTS_FIELD, "");


		//get all accounts
		for (uint64_t i = 0; i < UsersUsernameVector.size(); ++i)
		{
			xmldoc->addTextElementToParent("username", UsersUsernameVector[i], PREF_XML_ACCOUNTS_FIELD);
			xmldoc->addTextElementToParent("display_name", UsersDisplayNameVector[i], PREF_XML_ACCOUNTS_FIELD);

			if (UsersUserEmailVector.size() > i)
			{
				xmldoc->addTextElementToParent("useremail", UsersUserEmailVector[i], PREF_XML_ACCOUNTS_FIELD);
			}
			else
			{
				xmldoc->addTextElementToParent("useremail", "", PREF_XML_ACCOUNTS_FIELD);
			}

			sprintf(permStr, "%s", StringMacros::mapToString(
					UsersPermissionsVector[i]).c_str());
			xmldoc->addTextElementToParent("permissions", permStr, PREF_XML_ACCOUNTS_FIELD);
			if (UsersSaltVector[i] == "") //only give nac if account has not been activated yet with password
				sprintf(permStr, "%d", int(UsersAccountCreatedTimeVector[i] & 0xffff));
			else
				permStr[0] = '\0';
			xmldoc->addTextElementToParent("nac", permStr, PREF_XML_ACCOUNTS_FIELD);
		}
	}

	//get system layout defaults
	fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_PREFERENCES_PATH +
			(std::string)SYSTEM_PREFERENCES_PREFIX + "." + (std::string)USERS_PREFERENCES_FILETYPE;
	if (!prefXml.loadXmlDocument(fn))
	{
		__COUT__ << "System Preferences are defaults." << __E__;
		//insert defaults, no pref document found
		xmldoc->addTextElementToData(PREF_XML_SYSLAYOUT_FIELD, PREF_XML_SYSLAYOUT_DEFAULT);
	}
	else
	{
		__COUT__ << "Saved System Preferences found." << __E__;
		xmldoc->copyDataChildren(prefXml);
	}

	//add permissions value
	sprintf(permStr, "%s", StringMacros::mapToString(permissionMap).c_str());
	xmldoc->addTextElementToData(PREF_XML_PERMISSIONS_FIELD, permStr);

	//add user with lock
	xmldoc->addTextElementToData(PREF_XML_USERLOCK_FIELD, usersUsernameWithLock_);
	//add user name
	xmldoc->addTextElementToData(PREF_XML_USERNAME_FIELD, getUsersUsername(uid));
}

//========================================================================================================================
//WebUsers::setGenericPreference
//	each generic preference has its own directory, and each user has their own file
void WebUsers::setGenericPreference(uint64_t uid, const std::string& preferenceName,
		const std::string& preferenceValue)
{
	uint64_t	userIndex = searchUsersDatabaseForUserId(uid);
	//__COUT__ << "setGenericPreference for user: " << UsersUsernameVector[userIndex] << __E__;

	//force alpha-numeric with dash/underscore
	std::string safePreferenceName = "";
	for (const auto &c : preferenceName)
		if ((c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				(c >= '-' || c <= '_'))
			safePreferenceName += c;

	std::string dir = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_PREFERENCES_PATH +
			"generic_" + safePreferenceName + "/";

	//attempt to make directory (just in case)
	mkdir(dir.c_str(), 0755);

	std::string fn = UsersUsernameVector[userIndex] + "_" + safePreferenceName +
			"." + (std::string)USERS_PREFERENCES_FILETYPE;

	__COUT__ << "Preferences file: " << (dir + fn) << __E__;

	FILE *fp = fopen((dir + fn).c_str(), "w");
	if (fp)
	{
		fprintf(fp, "%s", preferenceValue.c_str());
		fclose(fp);
	}
	else
		__COUT_ERR__ << "Preferences file could not be opened for writing!" << __E__;
}

//========================================================================================================================
//WebUsers::getGenericPreference
//	each generic preference has its own directory, and each user has their own file
//	default preference is empty string.
std::string WebUsers::getGenericPreference(uint64_t uid, const std::string& preferenceName,
		HttpXmlDocument *xmldoc) const
{
	uint64_t	userIndex = searchUsersDatabaseForUserId(uid);
	//__COUT__ << "getGenericPreference for user: " << UsersUsernameVector[userIndex] << __E__;

	//force alpha-numeric with dash/underscore
	std::string safePreferenceName = "";
	for (const auto &c : preferenceName)
		if ((c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				(c >= '-' || c <= '_'))
			safePreferenceName += c;

	std::string dir = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_PREFERENCES_PATH +
			"generic_" + safePreferenceName + "/";

	std::string fn = UsersUsernameVector[userIndex] + "_" + safePreferenceName +
			"." + (std::string)USERS_PREFERENCES_FILETYPE;

	__COUT__ << "Preferences file: " << (dir + fn) << __E__;

	//read from preferences file
	FILE *fp = fopen((dir + fn).c_str(), "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		long size = ftell(fp);
		std::string line;
		line.reserve(size + 1);
		rewind(fp);
		fgets(&line[0], size + 1, fp);
		fclose(fp);

		__COUT__ << "Read value " << line << __E__;
		if (xmldoc) xmldoc->addTextElementToData(safePreferenceName, line);
		return line;
	}
	else
		__COUT__ << "Using default value." << __E__;

	//default preference is empty string
	if (xmldoc) xmldoc->addTextElementToData(safePreferenceName, "");
	return "";
}

//========================================================================================================================
//WebUsers::changeSettingsForUser
void WebUsers::changeSettingsForUser(uint64_t uid, const std::string& bgcolor, const std::string& dbcolor,
		const std::string& wincolor, const std::string& layout, const std::string& syslayout)
{
	std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> permissionMap =
			getPermissionsForUser(uid);
	if (isInactiveForGroup(permissionMap)) return; //not an active user

	uint64_t	userIndex = searchUsersDatabaseForUserId(uid);
	__COUT__ << "Changing settings for user: " << UsersUsernameVector[userIndex] << __E__;

	std::string fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_PREFERENCES_PATH + UsersUsernameVector[userIndex] + "." + (std::string)USERS_PREFERENCES_FILETYPE;

	__COUT__ << "Preferences file: " << fn << __E__;

	HttpXmlDocument prefXml;
	prefXml.addTextElementToData(PREF_XML_BGCOLOR_FIELD, bgcolor);
	prefXml.addTextElementToData(PREF_XML_DBCOLOR_FIELD, dbcolor);
	prefXml.addTextElementToData(PREF_XML_WINCOLOR_FIELD, wincolor);
	prefXml.addTextElementToData(PREF_XML_LAYOUT_FIELD, layout);

	prefXml.saveXmlDocument(fn);

	//if admin privilieges set system default layouts
	if (!isAdminForGroup(permissionMap)) return; //not admin

	//set system layout defaults
	fn = (std::string)WEB_LOGIN_DB_PATH + (std::string)USERS_PREFERENCES_PATH +
			(std::string)SYSTEM_PREFERENCES_PREFIX + "." + (std::string)USERS_PREFERENCES_FILETYPE;

	HttpXmlDocument sysPrefXml;
	sysPrefXml.addTextElementToData(PREF_XML_SYSLAYOUT_FIELD, syslayout);

	sysPrefXml.saveXmlDocument(fn);

}

//========================================================================================================================
//WebUsers::setUserWithLock
// if lock is true, set lock user specified
// if lock is false, attempt to unlock user specified
//	return true on success
bool WebUsers::setUserWithLock(uint64_t actingUid, bool lock,
		const std::string& username)
{
	std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> permissionMap =
			getPermissionsForUser(actingUid);

	std::string actingUser = getUsersUsername(actingUid);

	__COUTV__(actingUser);
	__COUT__ << "Permissions: " << StringMacros::mapToString(permissionMap) << __E__;
	__COUTV__(usersUsernameWithLock_);
	__COUTV__(lock);
	__COUTV__(username);
	__COUTV__(isUsernameActive(username));


	if (lock && (isUsernameActive(username) || !CareAboutCookieCodes_)) //lock and currently active
	{
		if (!CareAboutCookieCodes_ && username != DEFAULT_ADMIN_USERNAME) //enforce wiz mode only use admin account
		{
			__MCOUT_ERR__("User '" << actingUser << "' tried to lock for a user other than admin in wiz mode. Not allowed." << __E__);
			return false;
		}
		else if (!isAdminForGroup(permissionMap) && actingUser != username) //enforce normal mode admin privleges
		{
			__MCOUT_ERR__("A non-admin user '" << actingUser <<
					"' tried to lock for a user other than self. Not allowed." << __E__);
			return false;
		}
		usersUsernameWithLock_ = username;
	}
	else if (!lock && usersUsernameWithLock_ == username) //unlock
		usersUsernameWithLock_ = "";
	else
	{
		if (!isUsernameActive(username))
			__MCOUT_ERR__("User '" << username << "' is inactive." << __E__);
		__MCOUT_ERR__("Failed to lock for user '" << username << ".'" << __E__);
		return false;
	}

	__MCOUT_INFO__("User '" << username << "' has locked out the system!" << __E__);

	//save username with lock
	{
		std::string securityFileName = USER_WITH_LOCK_FILE;
		FILE *fp = fopen(securityFileName.c_str(), "w");
		if (!fp)
		{
			__COUT_INFO__ << "USER_WITH_LOCK_FILE " << USER_WITH_LOCK_FILE <<
					" not found. Ignoring." << __E__;
		}
		else
		{
			fprintf(fp, "%s", usersUsernameWithLock_.c_str());
			fclose(fp);
		}
	}
	return true;
}

//========================================================================================================================
//WebUsers::modifyAccountSettings
void WebUsers::modifyAccountSettings(uint64_t actingUid, uint8_t cmd_type,
		const std::string& username, const std::string& displayname,
		const std::string& email, const std::string& permissions)
{

	std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> permissionMap =
			getPermissionsForUser(actingUid);
	if (!isAdminForGroup(permissionMap))
	{
		__MCOUT_ERR__("Only admins can modify user settings." << __E__);
		return; //not an admin
	}

	uint64_t modi = searchUsersDatabaseForUsername(username);
	if (modi == 0)
	{
		__MCOUT_ERR__("Cannot modify first user" << __E__);
		return;
	}

	if (username.length() < USERNAME_LENGTH || displayname.length() < DISPLAY_NAME_LENGTH)
	{
		__MCOUT_ERR__("Invalid Username or Display Name must be length " << USERNAME_LENGTH <<
				" or " << DISPLAY_NAME_LENGTH << __E__);
		return;
	}

	__COUT__ << "Input Permissions: " << permissions << __E__;
	 std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> newPermissionsMap;

	switch (cmd_type)
	{
	case MOD_TYPE_UPDATE:

		__COUT__ << "MOD_TYPE_UPDATE " << username << " := " << permissions << __E__;

		if (modi == NOT_FOUND_IN_DATABASE)
		{
			__COUT__ << "User not found!? Should not happen." << __E__;
			return;
		}

		UsersDisplayNameVector[modi] = displayname;
		UsersUserEmailVector[modi] = email;

		StringMacros::getMapFromString(permissions,newPermissionsMap);

		//If account is currently inactive and re-activating, then reset fail count and password.
		//	Note: this is the account unlock mechanism.
		if (isInactiveForGroup(UsersPermissionsVector[modi]) && //curently inactive
				!isInactiveForGroup(newPermissionsMap))	//and re-activating
		{
			UsersLoginFailureCountVector[modi] = 0;
			UsersSaltVector[modi] = "";
		}
		UsersPermissionsVector[modi] = newPermissionsMap;

		//save information about modifier
		{
			uint64_t i = searchUsersDatabaseForUserId(actingUid);
			if (i == NOT_FOUND_IN_DATABASE)
			{
				__COUT__ << "Master User not found!? Should not happen." << __E__;
				return;
			}
			UsersLastModifierUsernameVector[modi] = UsersUsernameVector[i];
			UsersLastModifiedTimeVector[modi] = time(0);
		}
		break;
	case MOD_TYPE_ADD:
		__COUT__ << "MOD_TYPE_ADD " << username << " - " << displayname << __E__;
		createNewAccount(username, displayname, email);
		break;
	case MOD_TYPE_DELETE:
		__COUT__ << "MOD_TYPE_DELETE " << username << " - " << displayname << __E__;
		deleteAccount(username, displayname);
		break;
	default:
		__COUT__ << "Undefined command - do nothing " << username << __E__;
	}

	saveDatabaseToFile(DB_USERS);
}
//========================================================================================================================
//WebUsers::getActiveUsersString
//	return comma separated list of active Display Names
std::string WebUsers::getActiveUsersString()
{
	std::string ret = "";
	uint64_t u;
	bool repeat;
	for (uint64_t i = 0; i < ActiveSessionUserIdVector.size(); ++i)
	{
		repeat = false;
		//check for no repeat
		for (uint64_t j = 0; j < i; ++j)
			if (ActiveSessionUserIdVector[i] == ActiveSessionUserIdVector[j])
			{
				repeat = true; break;
			} //found repeat!

		if (!repeat && (u = searchUsersDatabaseForUserId(ActiveSessionUserIdVector[i])) !=
				NOT_FOUND_IN_DATABASE) //if found, add displayName
			ret += UsersDisplayNameVector[u] + ",";
	}
	if (ret.length() > 1) ret.erase(ret.length() - 1); //get rid of last comma
	return ret;
}
//========================================================================================================================
//WebUsers::getAdminUserID
//
uint64_t WebUsers::getAdminUserID()
{
	uint64_t uid = searchUsersDatabaseForUsername(DEFAULT_ADMIN_USERNAME);
	return uid;
}


//========================================================================================================================
//WebUsers::loadUserWithLock
//	//load username with lock from file
void WebUsers::loadUserWithLock()
{
	char username[300] = ""; //assume username is less than 300 chars

	std::string securityFileName = USER_WITH_LOCK_FILE;
	FILE *fp = fopen(securityFileName.c_str(), "r");
	if (!fp)
	{
		__COUT_INFO__ << "USER_WITH_LOCK_FILE " << USER_WITH_LOCK_FILE <<
				" not found. Defaulting to admin lock." << __E__;

		//default to admin lock if no file exists
		sprintf(username, "%s", DEFAULT_ADMIN_USERNAME.c_str());
	}
	else
	{
		fgets(username, 300, fp);
		username[299] = '\0'; //likely does nothing, but make sure there is closure on string
		fclose(fp);
	}

	//attempt to set lock
	__COUT__ << "Attempting to load username with lock: " << username << __E__;

	if (strlen(username) == 0)
	{
		__COUT_INFO__ << "Loaded state for user-with-lock is unlocked." << __E__;
		return;
	}

	uint64_t i = searchUsersDatabaseForUsername(username);
	if (i == NOT_FOUND_IN_DATABASE)
	{
		__COUT_INFO__ << "username " << username <<
				" not found in database. Ignoring." << __E__;
		return;
	}
	__COUT__ << "Setting lock" << __E__;
	setUserWithLock(UsersUserIdVector[i], true, username);
}

//========================================================================================================================
//WebUsers::getSecurity
//
std::string WebUsers::getSecurity()
{
	return securityType_;
}
//========================================================================================================================
//WebUsers::loadSecuritySelection
//
void WebUsers::loadSecuritySelection()
{
	std::string securityFileName = SECURITY_FILE_NAME;
	FILE *fp = fopen(securityFileName.c_str(), "r");
	char line[100] = "";
	if (fp) fgets(line, 100, fp);
	unsigned int i = 0;

	//find first character that is not alphabetic
	while (i < strlen(line) && line[i] >= 'A'  && line[i] <= 'z') ++i;
	line[i] = '\0'; //end string at first illegal character


	if (strcmp(line, SECURITY_TYPE_NONE.c_str()) == 0 ||
			strcmp(line, SECURITY_TYPE_DIGEST_ACCESS.c_str()) == 0)
		securityType_ = line;
	else
		securityType_ = SECURITY_TYPE_DIGEST_ACCESS; // default, if illegal

	__COUT__ << "The current security type is " << securityType_ << __E__;

	if (fp) fclose(fp);


	if (securityType_ == SECURITY_TYPE_NONE)
		CareAboutCookieCodes_ = false;
	else
		CareAboutCookieCodes_ = true;

	__COUT__ << "CareAboutCookieCodes_: " <<
			CareAboutCookieCodes_ << __E__;

}



//========================================================================================================================
void WebUsers::NACDisplayThread(const std::string& nac, const std::string& user)
{
	INIT_MF("WebUsers_NAC");
	//////////////////////////////////////////////////////////////////////
	//thread notifying the user about the admin new account code
	// notify for 10 seconds (e.g.)

	// child thread
	int i = 0;
	for (; i < 5; ++i)
	{
		std::this_thread::sleep_for(std::chrono::seconds(2));
		__COUT__ << "\n******************************************************************** " << __E__;
		__COUT__ << "\n******************************************************************** " << __E__;
		__COUT__ << "\n\nNew account code = " << nac << " for user: " << user << "\n" << __E__;
		__COUT__ << "\n******************************************************************** " << __E__;
		__COUT__ << "\n******************************************************************** " << __E__;
	}
}

//========================================================================================================================
void WebUsers::deleteUserData()
{
	//delete Login data
	std::system(("rm -rf " + (std::string)WEB_LOGIN_DB_PATH + HASHES_DB_PATH + "/*").c_str());
	std::system(("rm -rf " + (std::string)WEB_LOGIN_DB_PATH + USERS_DB_PATH + "/*").c_str());
	std::system(("rm -rf " + (std::string)WEB_LOGIN_DB_PATH + USERS_LOGIN_HISTORY_PATH + "/*").c_str());
	std::system(("rm -rf " + (std::string)WEB_LOGIN_DB_PATH + USERS_PREFERENCES_PATH + "/*").c_str());
	std::system(("rm -rf " + (std::string)WEB_LOGIN_DB_PATH + TOOLTIP_DB_PATH).c_str());

	std::string serviceDataPath = getenv("SERVICE_DATA_PATH");
	//delete macro maker folders
	std::system(("rm -rf " + std::string(serviceDataPath) + "/MacroData/").c_str());
	std::system(("rm -rf " + std::string(serviceDataPath) + "/MacroHistory/").c_str());
	std::system(("rm -rf " + std::string(serviceDataPath) + "/MacroExport/").c_str());

	//delete console folders
	std::system(("rm -rf " + std::string(serviceDataPath) + "/ConsolePreferences/").c_str());

	//delete wizard folders
	std::system(("rm -rf " + std::string(serviceDataPath) + "/OtsWizardData/").c_str());

	//delete progress bar folders
	std::system(("rm -rf " + std::string(serviceDataPath) + "/ProgressBarData/").c_str());

	//delete The Supervisor run folders
	std::system(("rm -rf " + std::string(serviceDataPath) + "/RunNumber/").c_str());
	std::system(("rm -rf " + std::string(serviceDataPath) + "/RunControlData/").c_str());

	//delete Visualizer folders
	std::system(("rm -rf " + std::string(serviceDataPath) + "/VisualizerData/").c_str());

	//DO NOT delete active groups file (this messes with people's configuration world, which is not expected when "resetting user info")
	//std::system(("rm -rf " + std::string(serviceDataPath) + "/ActiveConfigurationGroups.cfg").c_str());

	//delete Logbook folders
	std::system(("rm -rf " + std::string(getenv("LOGBOOK_DATA_PATH")) + "/").c_str());

	std::cout << __COUT_HDR_FL__ << "$$$$$$$$$$$$$$ Successfully deleted ALL service user data $$$$$$$$$$$$" << __E__;
}


