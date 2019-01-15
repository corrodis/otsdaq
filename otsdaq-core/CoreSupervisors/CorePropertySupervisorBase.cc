#include "otsdaq-core/CoreSupervisors/CorePropertySupervisorBase.h"

using namespace ots;

const CorePropertySupervisorBase::SupervisorProperties 	CorePropertySupervisorBase::SUPERVISOR_PROPERTIES = CorePropertySupervisorBase::SupervisorProperties();

//========================================================================================================================
CorePropertySupervisorBase::CorePropertySupervisorBase(xdaq::Application* application)
: theConfigurationManager_      (new ConfigurationManager)
, supervisorClass_              (application->getApplicationDescriptor()->getClassName())
, supervisorClassNoNamespace_   (supervisorClass_.substr(supervisorClass_.find_last_of(":")+1, supervisorClass_.length()-supervisorClass_.find_last_of(":")))
, supervisorContextUID_         ("MUST BE INITIALIZED INSIDE THE CONTRUCTOR TO THROW EXCEPTIONS")
, supervisorApplicationUID_     ("MUST BE INITIALIZED INSIDE THE CONTRUCTOR TO THROW EXCEPTIONS")
, supervisorConfigurationPath_  ("MUST BE INITIALIZED INSIDE THE CONTRUCTOR TO THROW EXCEPTIONS")
, propertiesAreSetup_			(false)
{
	INIT_MF("CorePropertySupervisorBase");

	__SUP_COUTV__(application->getApplicationContext()->getContextDescriptor()->getURL());
	__SUP_COUTV__(application->getApplicationDescriptor()->getLocalId());
	__SUP_COUTV__(supervisorClass_);
	__SUP_COUTV__(supervisorClassNoNamespace_);

	//get all supervisor info, and wiz mode or not
	allSupervisorInfo_.init(application->getApplicationContext());

	if(allSupervisorInfo_.isWizardMode())
	{
		__SUP_COUT__ << "Wiz mode detected. So skipping configuration location work for supervisor of class '" <<
				supervisorClass_ << "'" << __E__;

		return;
	}

	__SUP_COUT__ << "Getting configuration specific info for supervisor '" <<
			(allSupervisorInfo_.getSupervisorInfo(application).getName()) <<
			"' of class " << supervisorClass_ << "." << __E__;

	//get configuration specific info for the application supervisor

	try
	{
		CorePropertySupervisorBase::supervisorContextUID_ =
				theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getContextUID(
						application->getApplicationContext()->getContextDescriptor()->getURL());
	}
	catch(...)
	{
		__SUP_COUT_ERR__ << "XDAQ Supervisor could not access it's configuration through the theConfigurationManager_." <<
				//" The XDAQContextConfigurationName = " << XDAQContextConfigurationName_ <<
				". The getApplicationContext()->getContextDescriptor()->getURL() = " <<
				application->getApplicationContext()->getContextDescriptor()->getURL() << std::endl;
		throw;
	}

	try
	{
		CorePropertySupervisorBase::supervisorApplicationUID_ =
				theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getApplicationUID
				(
						application->getApplicationContext()->getContextDescriptor()->getURL(),
						application->getApplicationDescriptor()->getLocalId()
				);
	}
	catch(...)
	{
		__SUP_COUT_ERR__ << "XDAQ Supervisor could not access it's configuration through the theConfigurationManager_." <<
				" The supervisorContextUID_ = " << supervisorContextUID_ <<
				". The supervisorApplicationUID = " << supervisorApplicationUID_ << std::endl;
		throw;
	}

	CorePropertySupervisorBase::supervisorConfigurationPath_  = "/" +
			CorePropertySupervisorBase::supervisorContextUID_ + "/LinkToApplicationConfiguration/" +
			CorePropertySupervisorBase::supervisorApplicationUID_ + "/LinkToSupervisorConfiguration";

	__SUP_COUTV__(CorePropertySupervisorBase::supervisorContextUID_);
	__SUP_COUTV__(CorePropertySupervisorBase::supervisorApplicationUID_);
	__SUP_COUTV__(CorePropertySupervisorBase::supervisorConfigurationPath_);
}


//========================================================================================================================
CorePropertySupervisorBase::~CorePropertySupervisorBase(void)
{
	if(theConfigurationManager_) delete theConfigurationManager_;
}


//========================================================================================================================
//When overriding, setup default property values here
// called by CorePropertySupervisorBase constructor before loading user defined property values
void CorePropertySupervisorBase::setSupervisorPropertyDefaults(void)
{
	//This can be done in the constructor because when you start xdaq it loads the configuration that can't be changed while running!

	//__SUP_COUT__ << "Setting up Core Supervisor Base property defaults for supervisor" <<
	//		"..." << __E__;

	//set core Supervisor base class defaults
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserPermissionsThreshold,		"*=1");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsAllowed,				"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsDisallowed,			"");

	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.CheckUserLockRequestTypes,		"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.RequireUserLockRequestTypes,	"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.AutomatedRequestTypes,			"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.AllowNoLoginRequestTypes,		"");

	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NoXmlWhiteSpaceRequestTypes,	"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NonXMLRequestTypes,				"");


//	__SUP_COUT__ << "Done setting up Core Supervisor Base property defaults for supervisor" <<
//			"..." << __E__;
}

//========================================================================================================================
//extractPermissionsMapFromString
//	Static function that extract map function to standardize approach
//		in case needed by supervisors for special permissions handling.
//	For example, used to serve Desktop Icons.
//
//	permissionsString format is as follows:
//		<groupName>:<permissionsThreshold> pairs separated by ',' '&' or '|'
//		for example, to give access admins and pixel team but not calorimeter team:
//			allUsers:255 | pixelTeam:1 | calorimeterTeam:0
//
//	Use with CorePropertySupervisorBase::doPermissionsGrantAccess to determine
//		if access is allowed.
void CorePropertySupervisorBase::extractPermissionsMapFromString(const std::string& permissionsString,
		std::map<std::string,WebUsers::permissionLevel_t>& permissionsMap)
{
	permissionsMap.clear();
	StringMacros::getMapFromString(
			permissionsString,
			permissionsMap);
}

//========================================================================================================================
//doPermissionsGrantAccess
//	Static function that checks permissionLevelsMap against permissionThresholdsMap and returns true if
//	access requirements are met.
//
//	This is useful in standardizing approach for supervisors in case of
//		of special permissions handling.
//	For example, used to serve Desktop Icons.
//
//	permissionLevelsString format is as follows:
//		<groupName>:<permissionsLevel> pairs separated by ',' '&' or '|'
//		for example, to be a standard user and an admin on the pixel team and no access to calorimeter team:
//			allUsers:1 | pixelTeam:255 | calorimeterTeam:0
//
//	permissionThresoldsString format is as follows:
//		<groupName>:<permissionsThreshold> pairs separated by ',' '&' or '|'
//		for example, to give access admins and pixel team but not calorimeter team:
//			allUsers:255 | pixelTeam:1 | calorimeterTeam:0
bool CorePropertySupervisorBase::doPermissionsGrantAccess(
		std::map<std::string,WebUsers::permissionLevel_t>& permissionLevelsMap,
		std::map<std::string,WebUsers::permissionLevel_t>& permissionThresholdsMap)
{
	//return true if a permission level group name is found with a permission level
	//	greater than or equal to the permission level at a matching group name entry in the thresholds map.

	//__COUTV__(StringMacros::mapToString(permissionLevelsMap));
	//__COUTV__(StringMacros::mapToString(permissionThresholdsMap));

	for(const auto& permissionLevelGroupPair: permissionLevelsMap)
	{
		//__COUTV__(permissionLevelGroupPair.first); __COUTV__(permissionLevelGroupPair.second);

		for(const auto& permissionThresholdGroupPair: permissionThresholdsMap)
		{
			//__COUTV__(permissionThresholdGroupPair.first); __COUTV__(permissionThresholdGroupPair.second);
			if(permissionLevelGroupPair.first == permissionThresholdGroupPair.first &&
					permissionThresholdGroupPair.second && //not explicitly disallowed
					permissionLevelGroupPair.second >= permissionThresholdGroupPair.second)
				return true; //access granted!
		}
	}
	//__COUT__ << "Denied." << __E__;

	//if here, no access group match found
	//so denied
	return false;
} //end doPermissionsGrantAccess


//========================================================================================================================
void CorePropertySupervisorBase::checkSupervisorPropertySetup()
{
	if(propertiesAreSetup_) return;

	//Immediately mark properties as setup, (prevent infinite loops due to
	//	other launches from within this function, e.g. from getSupervisorProperty)
	//	only redo if Context configuration group changes
	propertiesAreSetup_ = true;


	CorePropertySupervisorBase::setSupervisorPropertyDefaults(); 	//calls base class version defaults

	//__SUP_COUT__ << "Setting up supervisor specific property DEFAULTS for supervisor..." << __E__;
	setSupervisorPropertyDefaults();						//calls override version defaults
//	__SUP_COUT__ << "Done setting up supervisor specific property DEFAULTS for supervisor" <<
//			"." << __E__;

	if(allSupervisorInfo_.isWizardMode())
		__SUP_COUT__ << "Wiz mode detected. Skipping setup of supervisor properties for supervisor of class '" <<
						supervisorClass_ <<
					"'" << __E__;
	else
		CorePropertySupervisorBase::loadUserSupervisorProperties();		//loads user settings from configuration


	//__SUP_COUT__ << "Setting up supervisor specific FORCED properties for supervisor..." << __E__;
	forceSupervisorPropertyValues();						//calls override forced values
//	__SUP_COUT__ << "Done setting up supervisor specific FORCED properties for supervisor" <<
//			"." << __E__;

	CorePropertySupervisorBase::extractPermissionsMapFromString(
			getSupervisorProperty(
					CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserPermissionsThreshold),
					propertyStruct_.UserPermissionsThreshold);

	propertyStruct_.UserGroupsAllowed.clear();
	StringMacros::getMapFromString(
			getSupervisorProperty(
					CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsAllowed),
					propertyStruct_.UserGroupsAllowed);

	propertyStruct_.UserGroupsDisallowed.clear();
	StringMacros::getMapFromString(
			getSupervisorProperty(
					CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsDisallowed),
					propertyStruct_.UserGroupsDisallowed);

	auto nameIt = SUPERVISOR_PROPERTIES.allSetNames_.begin();
	auto setIt = propertyStruct_.allSets_.begin();
	while(nameIt != SUPERVISOR_PROPERTIES.allSetNames_.end() &&
			setIt != propertyStruct_.allSets_.end())
	{
		(*setIt)->clear();
		StringMacros::getSetFromString(
				getSupervisorProperty(
						*(*nameIt)),
						*(*setIt));

		++nameIt; ++setIt;
	}


	__SUP_COUT__ << "Final supervisor property settings:" << std::endl;
	for(auto& property: propertyMap_)
		__SUP_COUT__ << "\t" << property.first << " = " << property.second << __E__;
}

//========================================================================================================================
//getSupervisorTreeNode ~
//	try to get this Supervisors configuration tree node
ConfigurationTree CorePropertySupervisorBase::getSupervisorTreeNode(void)
try
{
	if(supervisorContextUID_ == "" || supervisorApplicationUID_ == "")
	{
		__SUP_SS__ << "Empty supervisorContextUID_ or supervisorApplicationUID_." << __E__;
		__SUP_SS_THROW__;
	}
	return theConfigurationManager_->getSupervisorNode(
			supervisorContextUID_, supervisorApplicationUID_);
}
catch(...)
{
	__SUP_COUT_ERR__ << "XDAQ Supervisor could not access it's configuration node through theConfigurationManager_ " <<
			"(Did you remember to initialize using CorePropertySupervisorBase::init()?)." <<
			" The supervisorContextUID_ = " << supervisorContextUID_ <<
			". The supervisorApplicationUID = " << supervisorApplicationUID_ << std::endl;
	throw;
}

//========================================================================================================================
//loadUserSupervisorProperties ~
//	try to get user supervisor properties
void CorePropertySupervisorBase::loadUserSupervisorProperties(void)
{
//	__SUP_COUT__ << "Loading user properties for supervisor '" <<
//			supervisorContextUID_ << "/" << supervisorApplicationUID_ <<
//			"'..." << __E__;

	//re-acquire the configuration supervisor node, in case the config has changed
	auto supervisorNode = CorePropertySupervisorBase::getSupervisorTreeNode();

	try
	{
		auto /*map<name,node>*/ children = supervisorNode.getNode("LinkToPropertyConfiguration").getChildren();

		for(auto& child:children)
		{
			if(child.second.getNode("Status").getValue<bool>() == false) continue; //skip OFF properties

			auto propertyName = child.second.getNode("PropertyName").getValue();
			setSupervisorProperty(propertyName, child.second.getNode("PropertyValue").getValue<std::string>());
		}
	}
	catch(...)
	{
		__SUP_COUT__ << "No user supervisor property settings found in the configuration tree, going with the defaults." << __E__;
	}

	//	__SUP_COUT__ << "Done loading user properties for supervisor '" <<
	//			supervisorContextUID_ << "/" << supervisorApplicationUID_ <<
	//			"'" << __E__;
}

//========================================================================================================================
void CorePropertySupervisorBase::setSupervisorProperty(const std::string& propertyName, const std::string& propertyValue)
{
	propertyMap_[propertyName] = propertyValue;
//	__SUP_COUT__ << "Set propertyMap_[" << propertyName <<
//			"] = " << propertyMap_[propertyName] << __E__;
}

//========================================================================================================================
void CorePropertySupervisorBase::addSupervisorProperty(const std::string& propertyName, const std::string& propertyValue)
{
	propertyMap_[propertyName] = propertyValue + " | " + getSupervisorProperty(propertyName);
//	__SUP_COUT__ << "Set propertyMap_[" << propertyName <<
//			"] = " << propertyMap_[propertyName] << __E__;
}


//========================================================================================================================
//getSupervisorProperty
//		string version of template function
std::string CorePropertySupervisorBase::getSupervisorProperty(const std::string& propertyName)
{
	//check if need to setup properties
	checkSupervisorPropertySetup ();

	auto it = propertyMap_.find(propertyName);
	if(it == propertyMap_.end())
	{
		__SUP_SS__ << "Could not find property named " << propertyName << __E__;
		__SS_THROW__;//__SUP_SS_THROW__;
	}
	return StringMacros::validateValueForDefaultStringDataType(it->second);
}

//========================================================================================================================
//getSupervisorPropertyUserPermissionsThreshold
//	returns the threshold based on the requestType
WebUsers::permissionLevel_t CorePropertySupervisorBase::getSupervisorPropertyUserPermissionsThreshold(
		const std::string& requestType)
{
	//check if need to setup properties
	checkSupervisorPropertySetup();

	return StringMacros::getWildCardMatchFromMap(requestType,
			propertyStruct_.UserPermissionsThreshold);

//	auto it = propertyStruct_.UserPermissionsThreshold.find(requestType);
//	if(it == propertyStruct_.UserPermissionsThreshold.end())
//	{
//		__SUP_SS__ << "Could not find requestType named " << requestType << " in UserPermissionsThreshold map." << __E__;
//		__SS_THROW__; //__SUP_SS_THROW__;
//	}
//	return it->second;
}

//========================================================================================================================
//getRequestUserInfo ~
//	extract user info for request based on property configuration
void CorePropertySupervisorBase::getRequestUserInfo(WebUsers::RequestUserInfo& userInfo)
{
	checkSupervisorPropertySetup();

	//__SUP_COUT__ << "userInfo.requestType_ " << userInfo.requestType_ << " files: " << cgiIn.getFiles().size() << std::endl;

	userInfo.automatedCommand_ 			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.AutomatedRequestTypes); //automatic commands should not refresh cookie code.. only user initiated commands should!
	userInfo.NonXMLRequestType_			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.NonXMLRequestTypes); //non-xml request types just return the request return string to client
	userInfo.NoXmlWhiteSpace_ 			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.NoXmlWhiteSpaceRequestTypes);

	//**** start LOGIN GATEWAY CODE ***//
	//check cookieCode, sequence, userWithLock, and permissions access all in one shot!
	{
		userInfo.checkLock_				= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.CheckUserLockRequestTypes);
		userInfo.requireLock_ 			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.RequireUserLockRequestTypes);
		userInfo.allowNoUser_ 			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.AllowNoLoginRequestTypes);



		userInfo.permissionsThreshold_ 	= -1; //default to max
		try
		{
			userInfo.permissionsThreshold_ = CorePropertySupervisorBase::getSupervisorPropertyUserPermissionsThreshold(
					userInfo.requestType_);
		}
		catch(std::runtime_error& e)
		{
			if(!userInfo.automatedCommand_)
				 __SUP_COUT__ << "No explicit permissions threshold for request '" <<
						 userInfo.requestType_ << "'... Defaulting to max threshold = " <<
						 (unsigned int)userInfo.permissionsThreshold_ << __E__;
		}

		//		__COUTV__(userInfo.requestType_);
		//		__COUTV__(userInfo.checkLock_);
		//		__COUTV__(userInfo.requireLock_);
		//		__COUTV__(userInfo.allowNoUser_);
		//		__COUTV__((unsigned int)userInfo.permissionsThreshold_);

		try
		{
			StringMacros::getSetFromString(
					StringMacros::getWildCardMatchFromMap(userInfo.requestType_,
							propertyStruct_.UserGroupsAllowed),
							userInfo.groupsAllowed_);
		}
		catch(std::runtime_error& e)
		{
			userInfo.groupsAllowed_.clear();

//			if(!userInfo.automatedCommand_)
//				__SUP_COUT__ << "No explicit groups allowed for request '" <<
//					 userInfo.requestType_ << "'... Defaulting to empty groups allowed. " << __E__;
		}
		try
		{
			StringMacros::getSetFromString(
					StringMacros::getWildCardMatchFromMap(userInfo.requestType_,
									propertyStruct_.UserGroupsDisallowed),
									userInfo.groupsDisallowed_);
		}
		catch(std::runtime_error& e)
		{
			userInfo.groupsDisallowed_.clear();

//			if(!userInfo.automatedCommand_)
//				__SUP_COUT__ << "No explicit groups disallowed for request '" <<
//					 userInfo.requestType_ << "'... Defaulting to empty groups disallowed. " << __E__;
		}
	} //**** end LOGIN GATEWAY CODE ***//

	//completed user info, for the request type, is returned to caller
}

