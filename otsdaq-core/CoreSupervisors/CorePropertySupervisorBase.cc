#include "otsdaq-core/CoreSupervisors/CorePropertySupervisorBase.h"


//#include <iostream>

using namespace ots;

const CorePropertySupervisorBase::SupervisorProperties 	CorePropertySupervisorBase::SUPERVISOR_PROPERTIES = CorePropertySupervisorBase::SupervisorProperties();

////========================================================================================================================
//void CorePropertySupervisorBase::init(
//		const std::string& supervisorContextUID,
//		const std::string& supervisorApplicationUID,
//		ConfigurationManager *theConfigurationManager)
//{
//	__COUT__ << "Begin!" << std::endl;
//
//
//	__COUT__ << "Looking for " <<
//				supervisorContextUID << "/" << supervisorApplicationUID <<
//				" supervisor node..." << __E__;
//
//	//test the supervisor UIDs at init
//	auto supervisorNode = CorePropertySupervisorBase::getSupervisorTreeNode();
//
//	supervisorConfigurationPath_  = "/" + supervisorContextUID + "/LinkToApplicationConfiguration/" +
//			supervisorApplicationUID + "/LinkToSupervisorConfiguration";
//
//}

//========================================================================================================================
CorePropertySupervisorBase::CorePropertySupervisorBase(void)
: theConfigurationManager_      (new ConfigurationManager)
, XDAQContextConfigurationName_ (theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getConfigurationName())
, supervisorConfigurationPath_  ("MUST BE INITIALIZED INSIDE THE CONTRUCTOR OF CLASS WHICH INHERITS, BECAUSE IT NEEDS supervisorContextUID_ and supervisorApplicationUID_")
, supervisorContextUID_         ("INITIALIZED INSIDE THE CONTRUCTOR OF CLASS WHICH INHERITS, WHICH HAS ACCESS TO XDAQ APP MEMBER FUNCTIONS")
, supervisorApplicationUID_     ("INITIALIZED INSIDE THE CONTRUCTOR OF CLASS WHICH INHERITS, WHICH HAS ACCESS TO XDAQ APP MEMBER FUNCTIONS")
{
	INIT_MF("CorePropertySupervisorBase");
}


//========================================================================================================================
CorePropertySupervisorBase::~CorePropertySupervisorBase(void)
{
}


//========================================================================================================================
//When overriding, setup default property values here
// called by CorePropertySupervisorBase constructor before loading user defined property values
void CorePropertySupervisorBase::setSupervisorPropertyDefaults(void)
{
	//This can be done in the constructor because when you start xdaq it loads the configuration that can't be changed while running!

	__COUT__ << "Setting up Core Supervisor Base property defaults..." << std::endl;

	//set core Supervisor base class defaults
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserPermissionsThreshold,		"*=1");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsAllowed,				"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsDisallowed,			"");

	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.CheckUserLockRequestTypes,		"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.RequireUserLockRequestTypes,	"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.AutomatedRequestTypes,			"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.AllowNoLoginRequestTypes,		"");

//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NeedUsernameRequestTypes,		"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NeedDisplayNameRequestTypes,	"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NeedGroupMembershipRequestTypes,"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NeedSessionIndexRequestTypes,	"");

	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NoXmlWhiteSpaceRequestTypes,	"");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NonXMLRequestTypes,				"");
}

//========================================================================================================================
void CorePropertySupervisorBase::checkSupervisorPropertySetup()
{
	if(propertiesAreSetup_) return;


	CorePropertySupervisorBase::setSupervisorPropertyDefaults(); 	//calls base class version defaults
	setSupervisorPropertyDefaults();						//calls override version defaults
	CorePropertySupervisorBase::loadUserSupervisorProperties();		//loads user settings from configuration
	forceSupervisorPropertyValues();						//calls override forced values


	propertyStruct_.UserPermissionsThreshold.clear();
	StringMacros::getMapFromString(
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

	//at this point supervisor property setup is complete
	//	only redo if Context configuration group changes
	propertiesAreSetup_ = true;

	__COUT__ << "Final property settings:" << std::endl;
	for(auto& property: propertyMap_)
		__COUT__ << property.first << " = " << property.second << __E__;
}

//========================================================================================================================
//getSupervisorTreeNode ~
//	try to get this Supervisors configuration tree node
ConfigurationTree CorePropertySupervisorBase::getSupervisorTreeNode(void)
try
{
	return theConfigurationManager_->getSupervisorNode(
			supervisorContextUID_, supervisorApplicationUID_);
}
catch(...)
{
	__COUT_ERR__ << "XDAQ Supervisor could not access it's configuration node through theConfigurationManager_ " <<
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
	__COUT__ << "Looking for " <<
			supervisorContextUID_ << "/" << supervisorApplicationUID_ <<
			" supervisor user properties..." << __E__;

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
		__COUT__ << "No supervisor security settings found, going with defaults." << __E__;
	}

}

//========================================================================================================================
void CorePropertySupervisorBase::setSupervisorProperty(const std::string& propertyName, const std::string& propertyValue)
{
	propertyMap_[propertyName] = propertyValue;
	__COUT__ << "Set propertyMap_[" << propertyName <<
			"] = " << propertyMap_[propertyName] << __E__;
}

//========================================================================================================================
void CorePropertySupervisorBase::addSupervisorProperty(const std::string& propertyName, const std::string& propertyValue)
{
	propertyMap_[propertyName] = propertyValue + " | " + getSupervisorProperty(propertyName);
	__COUT__ << "Set propertyMap_[" << propertyName <<
			"] = " << propertyMap_[propertyName] << __E__;
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
		__SS__ << "Could not find property named " << propertyName << __E__;
		__SS_THROW__;
	}
	return StringMacros::validateValueForDefaultStringDataType(it->second);
}

//========================================================================================================================
uint8_t CorePropertySupervisorBase::getSupervisorPropertyUserPermissionsThreshold(const std::string& requestType)
{
	//check if need to setup properties
	checkSupervisorPropertySetup();

	auto it = propertyStruct_.UserPermissionsThreshold.find(requestType);
	if(it == propertyStruct_.UserPermissionsThreshold.end())
	{
		__SS__ << "Could not find requestType named " << requestType << " in UserPermissionsThreshold map." << __E__;
		__SS_THROW__;
	}
	return it->second;
}

//========================================================================================================================
//getRequestUserInfo ~
//	extract user info for request based on property configuration
void CorePropertySupervisorBase::getRequestUserInfo(WebUsers::RequestUserInfo& userInfo)
{
	checkSupervisorPropertySetup();

	//__COUT__ << "userInfo.requestType_ " << userInfo.requestType_ << " files: " << cgiIn.getFiles().size() << std::endl;

	userInfo.automatedCommand_ 			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.AutomatedRequestTypes); //automatic commands should not refresh cookie code.. only user initiated commands should!
	userInfo.NonXMLRequestType_			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.NonXMLRequestTypes); //non-xml request types just return the request return string to client
	userInfo.NoXmlWhiteSpace_ 			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.NoXmlWhiteSpaceRequestTypes);

	//**** start LOGIN GATEWAY CODE ***//
	//check cookieCode, sequence, userWithLock, and permissions access all in one shot!
	{
		userInfo.checkLock_				= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.CheckUserLockRequestTypes);
		userInfo.requireLock_ 			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.RequireUserLockRequestTypes);
		userInfo.allowNoUser_ 			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.AllowNoLoginRequestTypes);
//		userInfo.needUserName_ 			= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.NeedUsernameRequestTypes);
//		userInfo.needDisplayName_ 		= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.NeedDisplayNameRequestTypes);
//		userInfo.needGroupMembership_	= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.NeedGroupMembershipRequestTypes);
//		userInfo.needSessionIndex_ 		= StringMacros::inWildCardSet(userInfo.requestType_, propertyStruct_.NeedSessionIndexRequestTypes);
		userInfo.permissionsThreshold_ 	= -1; //default to max
		try
		{
			userInfo.permissionsThreshold_ = CorePropertySupervisorBase::getSupervisorPropertyUserPermissionsThreshold(userInfo.requestType_);
		}
		catch(std::runtime_error& e)
		{
			 __COUT__ << "Error getting permissions threshold for userInfo.requestType_='" <<
					 userInfo.requestType_ << "!' Defaulting to max threshold = " << (unsigned int)userInfo.permissionsThreshold_ << __E__;
		}

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
			 __COUT__ << "Error getting groups allowed userInfo.requestType_='" <<
					 userInfo.requestType_ << "!' Defaulting to empty groups. " << e.what() << __E__;
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
			 __COUT__ << "Error getting groups allowed userInfo.requestType_='" <<
					 userInfo.requestType_ << "!' Defaulting to empty groups. " << e.what() << __E__;
		}
	} //**** end LOGIN GATEWAY CODE ***//

	//completed user info, for the request type, is returned to caller
}

