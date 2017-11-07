#ifndef _ots_Utilities_WebUsers_h_
#define _ots_Utilities_WebUsers_h_

#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"

#include <string>
#include <vector>
#include <iostream>

#define WEB_LOGIN_DB_PATH 			    std::string(getenv("SERVICE_DATA_PATH")) + "/LoginData/"
#define HASHES_DB_PATH 					"HashesData/"
#define USERS_DB_PATH 					"UsersData/"
#define USERS_LOGIN_HISTORY_PATH 		USERS_DB_PATH + "UserLoginHistoryData/"
#define USERS_PREFERENCES_PATH 			USERS_DB_PATH + "UserPreferencesData/"
#define TOOLTIP_DB_PATH 				USERS_DB_PATH + "/TooltipData/"



//TODO -- include IP address handling!? Note from RAR -- I think IP addresses are being recorded now upon login.

namespace ots
{

class HttpXmlDocument;
    
class WebUsers
{
public:
	WebUsers(); 
	
	enum {
    	SESSION_ID_LENGTH = 512,
    	COOKIE_CODE_LENGTH = 512,
    	NOT_FOUND_IN_DATABASE =  uint64_t(-1),
    	USERNAME_LENGTH = 4,
    	DISPLAY_NAME_LENGTH = 4,
    };
    
    enum {
    	DB_SAVE_OPEN_AND_CLOSE,
    	DB_SAVE_OPEN,
    	DB_SAVE_CLOSE
    };
    
    enum {
    	DB_USERS,
		DB_HASHES
    };
    
    enum {
    	MOD_TYPE_UPDATE,
    	MOD_TYPE_ADD,
    	MOD_TYPE_DELETE
    };
    
    static const std::string DEFAULT_ADMIN_USERNAME;
    static const std::string DEFAULT_ADMIN_DISPLAY_NAME;

    static const std::string REQ_NO_LOGIN_RESPONSE;
    static const std::string REQ_NO_PERMISSION_RESPONSE;
    static const std::string REQ_USER_LOCKOUT_RESPONSE;

    static const std::string SECURITY_TYPE_NONE;
    static const std::string SECURITY_TYPE_DIGEST_ACCESS;
    static const std::string SECURITY_TYPE_KERBEROS;

	bool 			createNewAccount			    (std::string username, std::string displayName);
	void			cleanupExpiredEntries		    (std::vector<std::string> *loggedOutUsernames = 0);
	std::string 	createNewLoginSession		    (std::string uuid, std::string ip = "0");
	
	uint64_t 		attemptActiveSession		    (std::string uuid, std::string &jumbledUser, std::string jumbledPw, std::string &newAccountCode);
	uint64_t	 	isCookieCodeActiveForLogin	    (std::string uuid, std::string &cookieCode,std::string &username);
	bool	        cookieCodeIsActiveForRequest    (std::string &cookieCode, uint8_t *userPermissions = 0, uint64_t *uid = 0, std::string ip = "0", bool refresh = true,  std::string *userWithLock = 0);
	uint64_t        cookieCodeLogout			    (std::string cookieCode, bool logoutOtherUserSessions, uint64_t *uid = 0, std::string ip = "0");

    std::string     getUsersDisplayName			    (uint64_t uid);
    std::string     getUsersUsername			    (uint64_t uid);
    uint64_t        getActiveSessionCountForUser    (uint64_t uid);
    uint8_t         getPermissionsForUser		    (uint64_t uid);
    void            insertSettingsForUser		    (uint64_t uid, HttpXmlDocument *xmldoc,bool includeAccounts=false);
    std::string		getGenericPreference			(uint64_t uid, const std::string &preferenceName, HttpXmlDocument *xmldoc = 0) const;
    
    void            changeSettingsForUser		    (uint64_t uid, const std::string &bgcolor, const std::string &dbcolor, const std::string &wincolor, const std::string &layout, const std::string &syslayout);
    void			setGenericPreference			(uint64_t uid, const std::string &preferenceName, const std::string &preferenceValue);
    static void 	tooltipCheckForUsername			(const std::string& username, HttpXmlDocument *xmldoc, const std::string &srcFile, const std::string &srcFunc, const std::string &srcId);
    static void 	tooltipSetNeverShowForUsername	(const std::string& username, HttpXmlDocument *xmldoc, const std::string &srcFile, const std::string &srcFunc, const std::string &srcId, bool doNeverShow, bool temporarySilence);
    
    void            modifyAccountSettings		    (uint64_t uid_master, uint8_t cmd_type, std::string username, std::string displayname, std::string permissions);
    bool            setUserWithLock				    (uint64_t uid_master, bool lock, std::string username);
    std::string     getUserWithLock				    () { return usersUsernameWithLock_; }

	std::string 	getActiveUsersString	     	();
	
	bool	        getUserInfoForCookie		    (std::string &cookieCode, std::string *userName, std::string *displayName = 0, uint64_t *activeSessionIndex = 0);
    
	bool			isUsernameActive			    (std::string username) const;
	bool			isUserIdActive                  (uint64_t uid) const;
	uint64_t		getAdminUserID					();
	std::string		getSecurity						();

	static void 	deleteUserData					();
	static void 	resetAllUserTooltips			(const std::string &userNeedle = "*");

	static void 	NACDisplayThread				(std::string nac, std::string user);

	void			saveActiveSessions			();

private:
    void			loadSecuritySelection       ();
    void			loadUserWithLock			();
	unsigned int 	hexByteStrToInt				(const char *h);
	void 			intToHexStr					(uint8_t i, char *h);
	std::string		sha512						(std::string user, std::string password, std::string &salt);
	std::string 	dejumble					(std::string jumbledUser, std::string sessionId);
	std::string 	createNewActiveSession		(uint64_t uid,std::string ip = "0", uint64_t asIndex = 0);
	bool			addToHashesDatabase			(std::string hash);
	std::string 	genCookieCode				();
	std::string 	refreshCookieCode			(unsigned int i, bool enableRefresh = true);
	void			removeActiveSessionEntry	(unsigned int i);
	void			removeLoginSessionEntry		(unsigned int i);	
	bool 			deleteAccount				(std::string username, std::string displayName);
		
	void			saveToDatabase				(FILE * fp, std::string field, std::string value, uint8_t type = DB_SAVE_OPEN_AND_CLOSE, bool addNewLine = true);
	bool			saveDatabaseToFile			(uint8_t db); 
	bool			loadDatabases				(); 	

public:
	void			loadActiveSessions			();

	uint64_t	    searchUsersDatabaseForUsername	        (std::string username) const;
	uint64_t		searchUsersDatabaseForUserId			(uint64_t uid) const;
	uint64_t		searchLoginSessionDatabaseForUUID		(std::string uuid) const;
	uint64_t		searchHashesDatabaseForHash				(std::string hash);
	uint64_t		searchActiveSessionDatabaseForCookie	(std::string cookieCode) const;
	
	static std::string 	getTooltipFilename					(const std::string& username, const std::string &srcFile, const std::string &srcFunc, const std::string &srcId);


	
	std::vector<std::string> 	UsersDatabaseEntryFields,HashesDatabaseEntryFields;
	bool     					CareAboutCookieCodes_;
    std::string                 securityType_;

	 //"Login Session" database associations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    	//Generate random sessionId when receive a unique user ID (UUID)
			//reject UUID that have been used recently (e.g. last 5 minutes)
		//Maintain list of active sessionIds and associated UUID    	
    		//remove from list if been idle after some time or login attempts (e.g. 5 minutes or 3 login attempts)
			//maybe track IP address, to block multiple failed login attempts from same IP.    	
		//Use sessionId to un-jumble login attempts, lookup using UUID
    std::vector<std::string> 	LoginSessionIdVector, LoginSessionUUIDVector, LoginSessionIpVector;
    std::vector<time_t> 		LoginSessionStartTimeVector;
    std::vector<uint8_t> 		LoginSessionAttemptsVector;
    enum {
    	LOGIN_SESSION_EXPIRATION_TIME = 5*60, //5 minutes	
    	LOGIN_SESSION_ATTEMPTS_MAX = 5, //5 attempts on same session, forces new session
    };
    
    //"Active Session" database associations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    	//Maintain list of valid cookieCodes and associated user
			//all request must come with a valid cookieCode, else server fails request
		//On logout request, invalidate cookieCode
		//cookieCode expires after some idle time (e.g. 5 minutes) and 
			//is renewed and possibly changed each request
		//"single user - multiple locations" issue resolved using ActiveSessionIndex
			//where each independent login starts a new thread of cookieCodes tagged with ActiveSessionIndex
    	//if cookieCode not refreshed, then return most recent cookie code
    std::vector<std::string> 	ActiveSessionCookieCodeVector, ActiveSessionIpVector;
    std::vector<uint64_t> 		ActiveSessionUserIdVector, ActiveSessionIndex; 
    std::vector<time_t> 		ActiveSessionStartTimeVector;
    enum {
    	ACTIVE_SESSION_EXPIRATION_TIME = 120*60, //120 minutes, cookie is changed every half period of ACTIVE_SESSION_EXPIRATION_TIME
    	ACTIVE_SESSION_COOKIE_OVERLAP_TIME = 10*60, //10 minutes of overlap when new cookie is generated
    	ACTIVE_SESSION_STALE_COOKIE_LIMIT = 10, //10 stale cookies allowed for each active user
    };

    //"Users" database associations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    	//Maintain list of acceptable Usernames and associate:
			//permissions index (e.g. users, experts, masters) 0 to 255
				//0 := account inactive, not allowed to login (could be due to too many failed login attempts)
			//Last Login attempt time, and last USERS_LOGIN_HISTORY_SIZE successful logins
			//Name to display
			//random salt, before first login salt is empty string ""
			//Keep count of login attempt failures. Limit failures per unit time (e.g. 5 per hour)
			//Preferences (e.g. color scheme, etc)
			//Username appends to preferences file, and login history file
			//UsersLastModifierUsernameVector - is username of last master user to modify something about account
			//UsersLastModifierTimeVector - is time of last modify by a master user
    std::vector<std::string> 	UsersUsernameVector, UsersDisplayNameVector, UsersSaltVector, UsersLastModifierUsernameVector;
    std::vector<uint8_t> 		UsersPermissionsVector;
   	std::vector<uint64_t> 		UsersUserIdVector;
   	std::vector<time_t>			UsersLastLoginAttemptVector, UsersAccountCreatedTimeVector, UsersLastModifiedTimeVector; 
    std::vector<uint8_t> 		UsersLoginFailureCountVector;
  	uint64_t					usersNextUserId_;
    enum {
    	USERS_LOGIN_HISTORY_SIZE = 20,
    	USERS_GLOBAL_HISTORY_SIZE = 1000,
    	USERS_MAX_LOGIN_FAILURES = 20,
    };	
    std::string 				usersUsernameWithLock_;

    std::vector<std::string> 	UsersLoggedOutUsernames_;
			
    //"Hashes" database associations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//Maintain list of acceptable encoded (SHA-512) salt+user+pw's
    std::vector<std::string> 	HashesVector;
    std::vector<time_t> 		HashesAccessTimeVector;
};

}

#endif
