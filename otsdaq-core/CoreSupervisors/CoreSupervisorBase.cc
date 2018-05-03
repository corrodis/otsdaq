#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

//#include "otsdaq-core/Singleton/Singleton.h"
//#include "otsdaq-core/FECore/FEVInterfacesManager.h"



#include <iostream>

using namespace ots;

//XDAQ_INSTANTIATOR_IMPL(CoreSupervisorBase)


const std::string								CoreSupervisorBase::WORK_LOOP_DONE 			= "Done";
const std::string								CoreSupervisorBase::WORK_LOOP_WORKING 		= "Working";

//========================================================================================================================
CoreSupervisorBase::CoreSupervisorBase(xdaq::ApplicationStub * s)
: xdaq::Application             (s)
, SOAPMessenger                 (this)
, stateMachineWorkLoopManager_  (toolbox::task::bind(this, &CoreSupervisorBase::stateMachineThread, "StateMachine"))
, stateMachineSemaphore_        (toolbox::BSem::FULL)
, theConfigurationManager_      (new ConfigurationManager)//(Singleton<ConfigurationManager>::getInstance()) //I always load the full config but if I want to load a partial configuration (new ConfigurationManager)
, XDAQContextConfigurationName_ (theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getConfigurationName())
, supervisorConfigurationPath_  ("INITIALIZED INSIDE THE CONTRUCTOR BECAUSE IT NEEDS supervisorContextUID_ and supervisorApplicationUID_")
, supervisorContextUID_         ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
, supervisorApplicationUID_     ("INITIALIZED INSIDE THE CONTRUCTOR TO LAUNCH AN EXCEPTION")
, supervisorClass_              (getApplicationDescriptor()->getClassName())
, supervisorClassNoNamespace_   (supervisorClass_.substr(supervisorClass_.find_last_of(":")+1, supervisorClass_.length()-supervisorClass_.find_last_of(":")))
, theRemoteWebUsers_ 			(this)
, propertiesAreSetup_			(false)
{
	INIT_MF("CoreSupervisorBase");

	__COUT__ << "Begin!" << std::endl;

	xgi::bind (this, &CoreSupervisorBase::defaultPageWrapper,     "Default" );
	xgi::bind (this, &CoreSupervisorBase::requestWrapper, 		  "Request");

	xgi::bind (this, &CoreSupervisorBase::stateMachineXgiHandler, "StateMachineXgiHandler");

	xoap::bind(this, &CoreSupervisorBase::stateMachineStateRequest,    		"StateMachineStateRequest",    		XDAQ_NS_URI );
	xoap::bind(this, &CoreSupervisorBase::stateMachineErrorMessageRequest,  "StateMachineErrorMessageRequest",  XDAQ_NS_URI );
	//xoap::bind(this, &CoreSupervisorBase::macroMakerSupervisorRequest, 		"MacroMakerSupervisorRequest", 		XDAQ_NS_URI ); //moved to only FESupervisor!
	xoap::bind(this, &CoreSupervisorBase::workLoopStatusRequestWrapper, 	"WorkLoopStatusRequest",    		XDAQ_NS_URI );

	try
	{
		supervisorContextUID_ = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getContextUID(
				getApplicationContext()->getContextDescriptor()->getURL());
	}
	catch(...)
	{
		__COUT_ERR__ << "XDAQ Supervisor could not access it's configuration through the theConfigurationManager_." <<
				" The XDAQContextConfigurationName = " << XDAQContextConfigurationName_ <<
				". The getApplicationContext()->getContextDescriptor()->getURL() = " << getApplicationContext()->getContextDescriptor()->getURL() << std::endl;
		throw;
	}

	try
	{
		supervisorApplicationUID_ = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getApplicationUID
				(
						supervisorContextUID_,
						getApplicationDescriptor()->getLocalId()
				);
	}
	catch(...)
	{
		__COUT_ERR__ << "XDAQ Supervisor could not access it's configuration through the theConfigurationManager_." <<
				" The supervisorContextUID_ = " << supervisorContextUID_ <<
				". The supervisorApplicationUID = " << supervisorApplicationUID_ << std::endl;
		throw;
	}

	try
	{
		supervisorNode_ = theConfigurationManager_->getSupervisorNode(
				supervisorContextUID_, supervisorApplicationUID_);
	}
	catch(...)
	{
		__COUT_ERR__ << "XDAQ Supervisor could not access it's configuration node through theConfigurationManager_." <<
				" The supervisorContextUID_ = " << supervisorContextUID_ <<
				". The supervisorApplicationUID = " << supervisorApplicationUID_ << std::endl;
		throw;
	}

	supervisorConfigurationPath_  = "/" + supervisorContextUID_ + "/LinkToApplicationConfiguration/" + supervisorApplicationUID_ + "/LinkToSupervisorConfiguration";

	setStateMachineName(supervisorApplicationUID_);

	allSupervisorInfo_.init(getApplicationContext());
	__COUT__ << "Name = " << allSupervisorInfo_.getSupervisorInfo(this).getName();

	CorePropertySupervisorBase::init(supervisorContextUID_,supervisorApplicationUID_,theConfigurationManager_);
//=======
//
//	__COUT__ << "Initializing..." << std::endl;
//
//	setSupervisorPropertyDefaults(); //calls virtual init (where default supervisor properties should be set)
//
//	//try to get security settings
//	{
//
//		__COUT__ << "Looking for " <<
//				supervisorContextUID_ << "/" << supervisorApplicationUID_ <<
//				" supervisor security settings..." << __E__;
//
//		try
//		{
//		ConfigurationTree appNode = theConfigurationManager_->getSupervisorNode(
//				supervisorContextUID_, supervisorApplicationUID_);
//			auto /*map<name,node>*/ children = appNode.getNode("LinkToPropertyConfiguration").getChildren();
//
//			for(auto& child:children)
//			{
//				if(child.second.getNode("Status").getValue<bool>() == false) continue; //skip OFF properties
//
//				auto propertyName = child.second.getNode("PropertyName");
//
//				if(propertyName.getValue() ==
//						supervisorProperties_.fieldRequireLock)
//				{
//					LOCK_REQUIRED_ = child.second.getNode("PropertyValue").getValue<bool>();
//					__COUTV__(LOCK_REQUIRED_);
//				}
//				else if(propertyName.getValue() ==
//						supervisorProperties_.fieldUserPermissionsThreshold)
//				{
//					USER_PERMISSIONS_THRESHOLD_ = child.second.getNode("PropertyValue").getValue<uint8_t>();
//					__COUTV__(USER_PERMISSIONS_THRESHOLD_);
//				}
//			}
//		}
//		catch(...)
//		{
//			__COUT__ << "No supervisor security settings found, going with defaults." << __E__;
//		}
//	}
//>>>>>>> 858be5cf91c8b11a8035fccacb669ee69f17820d
}

//========================================================================================================================
CoreSupervisorBase::~CoreSupervisorBase(void)
{
	destroy();
}

//========================================================================================================================
void CoreSupervisorBase::destroy(void)
{
	__COUT__ << "Destroying..." << std::endl;
	for(auto& it: theStateMachineImplementation_)
		delete it;
	theStateMachineImplementation_.clear();
}
//
////========================================================================================================================
////When overriding, setup default property values here
//// called by CoreSupervisorBase constructor before loading user defined property values
//void CorePropertySupervisorBase::setSupervisorPropertyDefaults(void)
//{
//	//This can be done in the constructor because when you start xdaq it loads the configuration that can't be changed while running!
//
//	__COUT__ << "Setting up Core Supervisor Base property defaults..." << std::endl;
//
//	//set core Supervisor base class defaults
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserPermissionsThreshold,		"*=1");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsAllowed,				"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsDisallowed,			"");
//
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.CheckUserLockRequestTypes,		"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.RequireUserLockRequestTypes,	"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.AutomatedRequestTypes,			"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.AllowNoLoginRequestTypes,		"");
//
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NeedUsernameRequestTypes,		"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NeedDisplayNameRequestTypes,	"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NeedGroupMembershipRequestTypes,"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NeedSessionIndexRequestTypes,	"");
//
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NoXmlWhiteSpaceRequestTypes,	"");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NonXMLRequestTypes,				"");
//}
//
////========================================================================================================================
//void CoreSupervisorBase::checkSupervisorPropertySetup()
//{
//	if(propertiesAreSetup_) return;
//
//
//	CorePropertySupervisorBase::setSupervisorPropertyDefaults(); 	//calls base class version defaults
//	setSupervisorPropertyDefaults();						//calls override version defaults
//	CoreSupervisorBase::loadUserSupervisorProperties();		//loads user settings from configuration
//	forceSupervisorPropertyValues();						//calls override forced values
//
//
//	propertyStruct_.UserPermissionsThreshold.clear();
//	StringMacros::getMapFromString(
//			getSupervisorProperty(
//					CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserPermissionsThreshold),
//					propertyStruct_.UserPermissionsThreshold);
//
//	propertyStruct_.UserGroupsAllowed.clear();
//	StringMacros::getMapFromString(
//			getSupervisorProperty(
//					CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsAllowed),
//					propertyStruct_.UserGroupsAllowed);
//
//	propertyStruct_.UserGroupsDisallowed.clear();
//	StringMacros::getMapFromString(
//			getSupervisorProperty(
//					CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserGroupsDisallowed),
//					propertyStruct_.UserGroupsDisallowed);
//
//	auto nameIt = SUPERVISOR_PROPERTIES.allSetNames_.begin();
//	auto setIt = propertyStruct_.allSets_.begin();
//	while(nameIt != SUPERVISOR_PROPERTIES.allSetNames_.end() &&
//			setIt != propertyStruct_.allSets_.end())
//	{
//		(*setIt)->clear();
//		StringMacros::getSetFromString(
//				getSupervisorProperty(
//						*(*nameIt)),
//						*(*setIt));
//
//		++nameIt; ++setIt;
//	}
//
//	//at this point supervisor property setup is complete
//	//	only redo if Context configuration group changes
//	propertiesAreSetup_ = true;
//
//	__COUT__ << "Final property settings:" << std::endl;
//	for(auto& property: propertyMap_)
//		__COUT__ << property.first << " = " << property.second << __E__;
//}
//
////========================================================================================================================
////loadUserSupervisorProperties ~
////	try to get user supervisor properties
//void CoreSupervisorBase::loadUserSupervisorProperties(void)
//{
//	__COUT__ << "Looking for " <<
//			supervisorContextUID_ << "/" << supervisorApplicationUID_ <<
//			" supervisor user properties..." << __E__;
//
//	//re-acquire the configuration supervisor node, in case the config has changed
//	try
//	{
//		supervisorNode_ = theConfigurationManager_->getSupervisorNode(
//				supervisorContextUID_, supervisorApplicationUID_);
//	}
//	catch(...)
//	{
//		__COUT_ERR__ << "XDAQ Supervisor could not access it's configuration node through theConfigurationManager_." <<
//				" The supervisorContextUID_ = " << supervisorContextUID_ <<
//				". The supervisorApplicationUID = " << supervisorApplicationUID_ << std::endl;
//		throw;
//	}
//
//	try
//	{
//		auto /*map<name,node>*/ children = supervisorNode_.getNode("LinkToPropertyConfiguration").getChildren();
//
//		for(auto& child:children)
//		{
//			if(child.second.getNode("Status").getValue<bool>() == false) continue; //skip OFF properties
//
//			auto propertyName = child.second.getNode("PropertyName").getValue();
//			setSupervisorProperty(propertyName, child.second.getNode("PropertyValue").getValue<std::string>());
//		}
//	}
//	catch(...)
//	{
//		__COUT__ << "No supervisor security settings found, going with defaults." << __E__;
//	}
//
//}
//
////========================================================================================================================
//void CorePropertySupervisorBase::setSupervisorProperty(const std::string& propertyName, const std::string& propertyValue)
//{
//	propertyMap_[propertyName] = propertyValue;
//	__COUT__ << "Set propertyMap_[" << propertyName <<
//			"] = " << propertyMap_[propertyName] << __E__;
//}
//
////========================================================================================================================
//void CoreSupervisorBase::addSupervisorProperty(const std::string& propertyName, const std::string& propertyValue)
//{
//	propertyMap_[propertyName] = propertyValue + " | " + getSupervisorProperty(propertyName);
//	__COUT__ << "Set propertyMap_[" << propertyName <<
//			"] = " << propertyMap_[propertyName] << __E__;
//}
//
//
////========================================================================================================================
////getSupervisorProperty
////		string version of template function
//std::string CoreSupervisorBase::getSupervisorProperty(const std::string& propertyName)
//{
//	//check if need to setup properties
//	checkSupervisorPropertySetup ();
//
//	auto it = propertyMap_.find(propertyName);
//	if(it == propertyMap_.end())
//	{
//		__SS__ << "Could not find property named " << propertyName << __E__;
//		__SS_THROW__;
//	}
//	return StringMacros::validateValueForDefaultStringDataType(it->second);
//}
//
////========================================================================================================================
//uint8_t CoreSupervisorBase::getSupervisorPropertyUserPermissionsThreshold(const std::string& requestType)
//{
//	//check if need to setup properties
//	checkSupervisorPropertySetup();
//
//	auto it = propertyStruct_.UserPermissionsThreshold.find(requestType);
//	if(it == propertyStruct_.UserPermissionsThreshold.end())
//	{
//		__SS__ << "Could not find requestType named " << requestType << " in UserPermissionsThreshold map." << __E__;
//		__SS_THROW__;
//	}
//	return it->second;
//}


//========================================================================================================================
//wrapper for inheritance call
void CoreSupervisorBase::defaultPageWrapper(xgi::Input * in, xgi::Output * out )
throw (xgi::exception::Exception)
{
	return defaultPage(in,out);
}

//========================================================================================================================
void CoreSupervisorBase::defaultPage(xgi::Input * in, xgi::Output * out )
{
	__COUT__<< "Supervisor class " << supervisorClass_ << std::endl;

	std::stringstream pagess;
	pagess << "/WebPath/html/" <<
			supervisorClassNoNamespace_ << ".html?urn=" <<
			this->getApplicationDescriptor()->getLocalId();

	__COUT__<< "Default page = " << pagess.str() << std::endl;

	*out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' row='100%'><frame src='" <<
			pagess.str() <<
			"'></frameset></html>";
}

//========================================================================================================================
//requestWrapper ~
//	wrapper for inheritance Supervisor request call
void CoreSupervisorBase::requestWrapper(xgi::Input * in, xgi::Output * out )

{
	//checkSupervisorPropertySetup();

	cgicc::Cgicc cgiIn(in);
	std::string requestType = CgiDataUtilities::getData(cgiIn,"RequestType");

	//__COUT__ << "requestType " << requestType << " files: " << cgiIn.getFiles().size() << std::endl;

	HttpXmlDocument xmlOut;
	WebUsers::RequestUserInfo userInfo(requestType,
			CgiDataUtilities::getOrPostData(cgiIn,"CookieCode"));

	CorePropertySupervisorBase::getRequestUserInfo(userInfo);
//
//	bool automatedCommand 	= StringMacros::inWildCardSet(requestType, propertyStruct_.AutomatedRequestTypes); //automatic commands should not refresh cookie code.. only user initiated commands should!
//	bool NonXMLRequestType 	= StringMacros::inWildCardSet(requestType, propertyStruct_.NonXMLRequestTypes); //non-xml request types just return the request return string to client
//
//	//**** start LOGIN GATEWAY CODE ***//
//	//check cookieCode, sequence, userWithLock, and permissions access all in one shot!
//	{
//		bool checkLock 			= StringMacros::inWildCardSet(requestType, propertyStruct_.CheckUserLockRequestTypes);
//		bool requireLock 		= StringMacros::inWildCardSet(requestType, propertyStruct_.RequireUserLockRequestTypes);
//		bool allowNoUser 		= StringMacros::inWildCardSet(requestType, propertyStruct_.AllowNoLoginRequestTypes);
//		bool needUserName 		= StringMacros::inWildCardSet(requestType, propertyStruct_.NeedUsernameRequestTypes);
//		bool needDisplayName 	= StringMacros::inWildCardSet(requestType, propertyStruct_.NeedDisplayNameRequestTypes);
//		bool needGroupMembership= StringMacros::inWildCardSet(requestType, propertyStruct_.NeedGroupMembershipRequestTypes);
//		bool needSessionIndex 	= StringMacros::inWildCardSet(requestType, propertyStruct_.NeedSessionIndexRequestTypes);
//		uint8_t permissionsThreshold = -1; //default to max
//		try
//		{
//			permissionsThreshold = CoreSupervisorBase::getSupervisorPropertyUserPermissionsThreshold(requestType);
//		}
//		catch(std::runtime_error& e)
//		{
//			 __COUT__ << "Error getting permissions threshold for requestType='" <<
//					 requestType << "!' Defaulting to max threshold = " << (unsigned int)permissionsThreshold << __E__;
//		}
//
//		std::set<std::string> groupsAllowed, groupsDisallowed;
//		try
//		{
//			StringMacros::getSetFromString(
//					StringMacros::getWildCardMatchFromMap(requestType,
//							propertyStruct_.UserGroupsAllowed),
//							groupsAllowed);
//		}
//		catch(std::runtime_error& e)
//		{
//			groupsAllowed.clear();
//			 __COUT__ << "Error getting groups allowed requestType='" <<
//					 requestType << "!' Defaulting to empty groups. " << e.what() << __E__;
//		}
//		try
//		{
//			StringMacros::getSetFromString(
//					StringMacros::getWildCardMatchFromMap(requestType,
//									propertyStruct_.UserGroupsDisallowed),
//									groupsDisallowed);
//		}
//		catch(std::runtime_error& e)
//		{
//			groupsDisallowed.clear();
//			 __COUT__ << "Error getting groups allowed requestType='" <<
//					 requestType << "!' Defaulting to empty groups. " << e.what() << __E__;
//		}


	if(!theRemoteWebUsers_.xmlRequestToGateway(
			cgiIn,
			out,
			&xmlOut,
			allSupervisorInfo_,
			userInfo))
		return; //access failed


	//done checking cookieCode, sequence, userWithLock, and permissions access all in one shot!
	//**** end LOGIN GATEWAY CODE ***//

	if(!userInfo.automatedCommand_)
		__COUT__ << "requestType: " << requestType << __E__;

	if(userInfo.NonXMLRequestType_)
	{
		nonXmlRequest(requestType,cgiIn,*out,userInfo);
		return;
	}
	//else xml request type

	request(requestType,cgiIn,xmlOut,userInfo);

	//report any errors encountered
	{
		unsigned int occurance = 0;
		std::string err = xmlOut.getMatchingValue("Error",occurance++);
		while(err != "")
		{
			__COUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err << std::endl;
			__MOUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err << std::endl;
			err = xmlOut.getMatchingValue("Error",occurance++);
		}
	}

	//return xml doc holding server response
	xmlOut.outputXmlDocument((std::ostringstream*) out, false /*print to cout*/,
			!userInfo.NoXmlWhiteSpace_/*allow whitespace*/);

//
//		std::string groupMemebershipString;
//		if(!theRemoteWebUsers_.xmlRequestToGateway(
//				cgiIn
//				,out
//				,&xmlOut
//				,allSupervisorInfo_
//				,&userInfo.permissionLevel_					//acquire user's access level (optionally null pointer)
//				,permissionsThreshold						//set access level requirement to pass gateway
//				,allowNoUser 								//allow no user access
//				,groupsAllowed
//				,groupsDisallowed
//				,!automatedCommand							//true/false refresh cookie code
//				,checkLock									//true/false enable check that system is unlocked or this user has the lock
//				,requireLock								//true/false requires this user has the lock to proceed
//				,((checkLock || requireLock)?&userInfo.usernameWithLock_:0)	//acquire username with lock (optionally null pointer)
//				,(needUserName?&userInfo.username_:0)			//acquire username of this user (optionally null pointer)
//				,(needDisplayName?&userInfo.displayName_:0)						//acquire user's Display Name
//				,((needGroupMembership || groupsAllowed.size() || groupsDisallowed.size())?
//						&groupMemebershipString:0)	//acquire user's group memberships
//				,(needSessionIndex?&userInfo.activeUserSessionIndex_:0)		//acquire user's session index associated with the cookieCode
//		))
//		{
//			//failure
//
//			//print out return string on failure
//			if(!automatedCommand)
//				__COUT__ << "Failed request (requestType = " << requestType <<
//					"): " << out->str() << __E__;
//
//			return;
//		}
//
//		//re-factor membership string to set
//		StringMacros::getSetFromString(
//				groupMemebershipString,
//				userInfo.groupMembership_);
//	}

	//done checking cookieCode, sequence, userWithLock, and permissions access all in one shot!
	//**** end LOGIN GATEWAY CODE ***//
//
//	if(!automatedCommand)
//		__COUT__ << "requestType: " << requestType << __E__;
//
//	if(NonXMLRequestType)
//	{
//		nonXmlRequest(requestType,cgiIn,*out,userInfo);
//		return;
//	}
//	//else xml request type
//
//	request(requestType,cgiIn,xmlOut,userInfo);
//
//	//report any errors encountered
//	{
//		unsigned int occurance = 0;
//		std::string err = xmlOut.getMatchingValue("Error",occurance++);
//		while(err != "")
//		{
//			__COUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err << std::endl;
//			__MOUT_ERR__ << "'" << requestType << "' ERROR encountered: " << err << std::endl;
//			err = xmlOut.getMatchingValue("Error",occurance++);
//		}
//	}
//
//	//return xml doc holding server response
//	xmlOut.outputXmlDocument((std::ostringstream*) out, false /*print to cout*/,
//			!(StringMacros::inWildCardSet(requestType,
//					propertyStruct_.NoXmlWhiteSpaceRequestTypes))/*allow whitespace*/);
}

//========================================================================================================================
//request
//		Supervisors should override this function. It will be called after user access has been verified
//		according to the Supervisor Property settings. The CoreSupervisorBase class provides consistent
//		access, responses, and error handling across all inheriting supervisors that use ::request.
void CoreSupervisorBase::request(const std::string& requestType, cgicc::Cgicc& cgiIn, HttpXmlDocument& xmlOut,
		const WebUsers::RequestUserInfo& userInfo)
{
	__COUT__ << "This is the empty Core Supervisor request. Supervisors should override this function." << __E__;

// KEEP:
//	here are some possibly interesting example lines of code for overriding supervisors
//
//
//  if(requestType == "savePlanCommandSequence")
//	{
//		std::string 	planName 		= CgiDataUtilities::getData(cgiIn,"planName"); //from GET
//		std::string 	commands 		= CgiDataUtilities::postData(cgiIn,"commands"); //from POST
//
//		cgiIn.getFiles()
//		__COUT__ << "planName: " << planName << __E__;
//		__COUTV__(commands);
//
//
//	}
//	else
//	{
//		__SS__ << "requestType '" << requestType << "' request not recognized." << std::endl;
//		__COUT__ << "\n" << ss.str();
//		xmlOut.addTextElementToData("Error", ss.str());
//	}
//	xmlOut.addTextElementToData("Error",
//			"request encountered an error!");
}

//========================================================================================================================
//nonXmlRequest
//		Supervisors should override this function. It will be called after user access has been verified
//		according to the Supervisor Property settings. The CoreSupervisorBase class provides consistent
//		access, responses, and error handling across all inheriting supervisors that use ::request.
void CoreSupervisorBase::nonXmlRequest(const std::string& requestType, cgicc::Cgicc& cgiIn, std::ostream& out,
		const WebUsers::RequestUserInfo& userInfo)
throw (xgi::exception::Exception)
{
	__COUT__ << "This is the empty Core Supervisor non-xml request. Supervisors should override this function." << __E__;
	out << "This is the empty Core Supervisor non-xml request. Supervisors should override this function." << __E__;
}

//========================================================================================================================
void CoreSupervisorBase::stateMachineXgiHandler(xgi::Input * in, xgi::Output * out )

{}

//========================================================================================================================
void CoreSupervisorBase::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out )

{}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineXoapHandler(xoap::MessageReference message )

{
	__COUT__<< "Soap Handler!" << std::endl;
	stateMachineWorkLoopManager_.removeProcessedRequests();
	stateMachineWorkLoopManager_.processRequest(message);
	__COUT__<< "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineResultXoapHandler(xoap::MessageReference message )

{
	__COUT__<< "Soap Handler!" << std::endl;
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);
	__COUT__<< "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
//indirection to allow for overriding handler
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequestWrapper(xoap::MessageReference message)

{
	//this should have an override for monitoring work loops being done
	return workLoopStatusRequest(message);
} //end workLoopStatusRequest()

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::workLoopStatusRequest(xoap::MessageReference message)

{
	//this should have an override for monitoring work loops being done
	return SOAPUtilities::makeSOAPMessageReference(CoreSupervisorBase::WORK_LOOP_DONE);
} //end workLoopStatusRequest()

//========================================================================================================================
bool CoreSupervisorBase::stateMachineThread(toolbox::task::WorkLoop* workLoop)
{
	stateMachineSemaphore_.take();
	__COUT__<< "Re-sending message..." << SOAPUtilities::translate(stateMachineWorkLoopManager_.getMessage(workLoop)).getCommand() << std::endl;
	std::string reply = send(this->getApplicationDescriptor(),stateMachineWorkLoopManager_.getMessage(workLoop));
	stateMachineWorkLoopManager_.report(workLoop, reply, 100, true);
	__COUT__<< "Done with message" << std::endl;
	stateMachineSemaphore_.give();
	return false;//execute once and automatically remove the workloop so in WorkLoopManager the try workLoop->remove(job_) could be commented out
	//return true;//go on and then you must do the workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineStateRequest(xoap::MessageReference message)

{
	__COUT__<< "theStateMachine_.getCurrentStateName() = " << theStateMachine_.getCurrentStateName() << std::endl;
	return SOAPUtilities::makeSOAPMessageReference(theStateMachine_.getCurrentStateName());
}

//========================================================================================================================
xoap::MessageReference CoreSupervisorBase::stateMachineErrorMessageRequest(xoap::MessageReference message)

{
	__COUT__<< "theStateMachine_.getErrorMessage() = " << theStateMachine_.getErrorMessage() << std::endl;

	SOAPParameters retParameters;
	retParameters.addParameter("ErrorMessage",theStateMachine_.getErrorMessage());
	return SOAPUtilities::makeSOAPMessageReference("stateMachineErrorMessageRequestReply",retParameters);
}

//========================================================================================================================
void CoreSupervisorBase::stateInitial(toolbox::fsm::FiniteStateMachine& fsm)

{
	__COUT__ << "CoreSupervisorBase::stateInitial" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateHalted(toolbox::fsm::FiniteStateMachine& fsm)

{
	__COUT__ << "CoreSupervisorBase::stateHalted" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateRunning(toolbox::fsm::FiniteStateMachine& fsm)

{
	__COUT__ << "CoreSupervisorBase::stateRunning" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::stateConfigured(toolbox::fsm::FiniteStateMachine& fsm)

{
	__COUT__ << "CoreSupervisorBase::stateConfigured" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::statePaused(toolbox::fsm::FiniteStateMachine& fsm)

{
	__COUT__ << "CoreSupervisorBase::statePaused" << std::endl;
}

//========================================================================================================================
void CoreSupervisorBase::inError (toolbox::fsm::FiniteStateMachine & fsm)

{
	__COUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()<< std::endl;
	//rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void CoreSupervisorBase::enteringError (toolbox::Event::Reference e)

{
	__COUT__<< "Fsm current state: " << theStateMachine_.getCurrentStateName()
			<< "\n\nError Message: " <<
			theStateMachine_.getErrorMessage() << std::endl;
	toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&>(*e);
	std::ostringstream error;
	error << "Failure performing transition from "
			<< failedEvent.getFromState()
			<<  " to "
			<< failedEvent.getToState()
			<< " exception: " << failedEvent.getException().what();
	__COUT_ERR__<< error.str() << std::endl;
	//diagService_->reportError(errstr.str(),DIAGERROR);

}

//========================================================================================================================
void CoreSupervisorBase::transitionConfiguring(toolbox::Event::Reference e)

{
	__COUT__ << "transitionConfiguring" << std::endl;

	std::pair<std::string /*group name*/, ConfigurationGroupKey> theGroup(
			SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).
			getParameters().getValue("ConfigurationGroupName"),
			ConfigurationGroupKey(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).
					getParameters().getValue("ConfigurationGroupKey")));

	__COUT__ << "Configuration group name: " << theGroup.first << " key: " <<
			theGroup.second << std::endl;

	theConfigurationManager_->loadConfigurationGroup(
			theGroup.first,
			theGroup.second, true);

	//Now that the configuration manager has all the necessary configurations, create all objects that depend on the configuration

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->configure();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while configuring: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionConfiguring" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
				);
	}

}

//========================================================================================================================
//transitionHalting
//	Ignore errors if coming from Failed state
void CoreSupervisorBase::transitionHalting(toolbox::Event::Reference e)

{
	__COUT__ << "transitionHalting" << std::endl;

	for(auto& it: theStateMachineImplementation_)
	{
		try
		{
			it->halt();
		}
		catch(const std::runtime_error& e)
		{
			//if halting from Failed state, then ignore errors
			if(theStateMachine_.getProvenanceStateName() ==
					RunControlStateMachine::FAILED_STATE_NAME)
			{
				__COUT_INFO__ << "Error was caught while halting (but ignoring because previous state was '" <<
						RunControlStateMachine::FAILED_STATE_NAME << "'): " << e.what() << std::endl;
			}
			else //if not previously in Failed state, then fail
			{
				__SS__ << "Error was caught while halting: " << e.what() << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				theStateMachine_.setErrorMessage(ss.str());
				throw toolbox::fsm::exception::Exception(
						"Transition Error" /*name*/,
						ss.str() /* message*/,
						"CoreSupervisorBase::transitionHalting" /*module*/,
						__LINE__ /*line*/,
						__FUNCTION__ /*function*/
						);
			}
		}
	}
}

//========================================================================================================================
//Inheriting supervisor classes should not override this function, or should at least also call it in the override
//	to maintain property functionality.
void CoreSupervisorBase::transitionInitializing(toolbox::Event::Reference e)

{
	__COUT__ << "transitionInitializing" << std::endl;

	propertiesAreSetup_ = false; //indicate need to re-load user properties


	//Note: Do not initialize the state machine implementations... do any initializing in configure
	//	This allows re-instantiation at each configure time.
	//for(auto& it: theStateMachineImplementation_)
	//it->initialize();
}

//========================================================================================================================
void CoreSupervisorBase::transitionPausing(toolbox::Event::Reference e)

{
	__COUT__ << "transitionPausing" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->pause();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while pausing: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionPausing" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
		);
	}
}

//========================================================================================================================
void CoreSupervisorBase::transitionResuming(toolbox::Event::Reference e)

{
	//NOTE: I want to first start the data manager first if this is a FEDataManagerSupervisor


	__COUT__ << "transitionResuming" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->resume();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while resuming: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionResuming" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
		);
	}
}

//========================================================================================================================
void CoreSupervisorBase::transitionStarting(toolbox::Event::Reference e)

{

	//NOTE: I want to first start the data manager first if this is a FEDataManagerSupervisor

	__COUT__ << "transitionStarting" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->start(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getParameters().getValue("RunNumber"));
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while starting: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionStarting" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
		);
	}
}

//========================================================================================================================
void CoreSupervisorBase::transitionStopping(toolbox::Event::Reference e)

{
	__COUT__ << "transitionStopping" << std::endl;

	try
	{
		for(auto& it: theStateMachineImplementation_)
			it->stop();
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "Error was caught while pausing: " << e.what() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		theStateMachine_.setErrorMessage(ss.str());
		throw toolbox::fsm::exception::Exception(
				"Transition Error" /*name*/,
				ss.str() /* message*/,
				"CoreSupervisorBase::transitionStopping" /*module*/,
				__LINE__ /*line*/,
				__FUNCTION__ /*function*/
		);
	}
}
