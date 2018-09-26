#include "otsdaq-core/GatewaySupervisor/GatewaySupervisor.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"

#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManagerRW.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/DesktopIconConfiguration.h"
#include "otsdaq-core/GatewaySupervisor/ARTDAQCommandable.h"


#include "otsdaq-core/NetworkUtilities/TransceiverSocket.h" // for UDP state changer



#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPCookie.h>
#include <cgicc/HTMLDoctype.h>
#include <cgicc/HTTPHeader.h>
#include <xgi/Utils.h>

#include <xoap/Method.h>
#include <xdaq/NamespaceURI.h>
#include <toolbox/task/WorkLoopFactory.h>
#include <toolbox/fsm/FailedEvent.h>

#include <fstream>
#include <thread>         	// std::this_thread::sleep_for
#include <chrono>         	// std::chrono::seconds
#include <sys/stat.h> 		// for mkdir

using namespace ots;


#define ICON_FILE_NAME 							std::string(getenv("SERVICE_DATA_PATH")) + "/OtsWizardData/iconList.dat"
#define RUN_NUMBER_PATH		 					std::string(getenv("SERVICE_DATA_PATH")) + "/RunNumber/"
#define RUN_NUMBER_FILE_NAME 					"NextRunNumber.txt"
#define FSM_LAST_GROUP_ALIAS_PATH				std::string(getenv("SERVICE_DATA_PATH")) + "/RunControlData/"
#define FSM_LAST_GROUP_ALIAS_FILE_START			std::string("FSMLastGroupAlias-")
#define FSM_USERS_PREFERENCES_FILETYPE			"pref"

#define CORE_TABLE_INFO_FILENAME 				((getenv("SERVICE_DATA_PATH") == NULL)?(std::string(getenv("USER_DATA"))+"/ServiceData"):(std::string(getenv("SERVICE_DATA_PATH")))) + "/CoreTableInfoNames.dat"

#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "GatewaySupervisor"


XDAQ_INSTANTIATOR_IMPL(GatewaySupervisor)


//========================================================================================================================
GatewaySupervisor::GatewaySupervisor(xdaq::ApplicationStub * s)
	:xdaq::Application(s)
	, SOAPMessenger					(this)
	, RunControlStateMachine		("GatewaySupervisor")
	, CorePropertySupervisorBase	(this)
	//, CorePropertySupervisorBase::theConfigurationManager_(new ConfigurationManager)
	//,theConfigurationGroupKey_    (nullptr)
	, theArtdaqCommandable_			(this)
	, stateMachineWorkLoopManager_	(toolbox::task::bind(this, &GatewaySupervisor::stateMachineThread, "StateMachine"))
	, stateMachineSemaphore_		(toolbox::BSem::FULL)
	, infoRequestWorkLoopManager_	(toolbox::task::bind(this, &GatewaySupervisor::infoRequestThread, "InfoRequest"))
	, infoRequestSemaphore_			(toolbox::BSem::FULL)
	, activeStateMachineName_		("")
	, theIterator_					(this)
	, counterTest_					(0)
{
	INIT_MF("GatewaySupervisor");
	__SUP_COUT__ << __E__;

	//attempt to make directory structure (just in case)
	mkdir((FSM_LAST_GROUP_ALIAS_PATH).c_str(), 0755);
	mkdir((RUN_NUMBER_PATH).c_str(), 0755);

	securityType_ = theWebUsers_.getSecurity();

	__SUP_COUT__ << "Security: " << securityType_ << __E__;

	xgi::bind (this, &GatewaySupervisor::Default,                  "Default");
	xgi::bind (this, &GatewaySupervisor::loginRequest,             "LoginRequest");
	xgi::bind (this, &GatewaySupervisor::request,                  "Request");
	xgi::bind (this, &GatewaySupervisor::stateMachineXgiHandler,   "StateMachineXgiHandler");
	xgi::bind (this, &GatewaySupervisor::infoRequestHandler,       "InfoRequestHandler");
	xgi::bind (this, &GatewaySupervisor::infoRequestResultHandler, "InfoRequestResultHandler");
	xgi::bind (this, &GatewaySupervisor::tooltipRequest,           "TooltipRequest");

	xoap::bind(this, &GatewaySupervisor::supervisorCookieCheck,        		"SupervisorCookieCheck",        	XDAQ_NS_URI);
	xoap::bind(this, &GatewaySupervisor::supervisorGetActiveUsers,     		"SupervisorGetActiveUsers",     	XDAQ_NS_URI);
	xoap::bind(this, &GatewaySupervisor::supervisorSystemMessage,      		"SupervisorSystemMessage",      	XDAQ_NS_URI);
	//xoap::bind(this, &GatewaySupervisor::supervisorGetUserInfo,        		"SupervisorGetUserInfo",        	XDAQ_NS_URI);
	xoap::bind(this, &GatewaySupervisor::supervisorSystemLogbookEntry, 		"SupervisorSystemLogbookEntry", 	XDAQ_NS_URI);
	xoap::bind(this, &GatewaySupervisor::supervisorLastConfigGroupRequest, 	"SupervisorLastConfigGroupRequest", XDAQ_NS_URI);


	init();
	//exit(1);
}

//========================================================================================================================
//	TODO: Lore needs to detect program quit through killall or ctrl+c so that Logbook entry is made when ots is halted
GatewaySupervisor::~GatewaySupervisor(void)
{
	delete CorePropertySupervisorBase::theConfigurationManager_;
	makeSystemLogbookEntry("ots halted.");
}

//========================================================================================================================
void GatewaySupervisor::init(void)
{
	supervisorGuiHasBeenLoaded_ = false;

//	const XDAQContextConfiguration* contextConfiguration = CorePropertySupervisorBase::theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration);
//
//	CorePropertySupervisorBase::supervisorContextUID_ = contextConfiguration->getContextUID(
//		getApplicationContext()->getContextDescriptor()->getURL()
//	);
//	__SUP_COUT__ << "Context UID:" << supervisorContextUID_ << __E__;
//
//	CorePropertySupervisorBase::supervisorApplicationUID_ = contextConfiguration->getApplicationUID(
//		getApplicationContext()->getContextDescriptor()->getURL(),
//		getApplicationDescriptor()->getLocalId()
//	);
//
//	__SUP_COUT__ << "Application UID:" << supervisorApplicationUID_ << __E__;
//
//	ConfigurationTree configLinkNode = CorePropertySupervisorBase::getSupervisorTreeNode();
//
//	std::string supervisorUID;
//	if (!configLinkNode.isDisconnected())
//		supervisorUID = configLinkNode.getValue();
//	else
//		supervisorUID = ViewColumnInfo::DATATYPE_LINK_DEFAULT;
//
	//__SUP_COUT__ << "GatewaySupervisor UID:" << supervisorUID << __E__;



	//setting up thread for UDP thread to drive state machine
	bool enableStateChanges = false;
	try
	{
		enableStateChanges =
				CorePropertySupervisorBase::getSupervisorConfigurationNode().getNode("EnableStateChangesOverUDP").getValue<bool>();
	}
	catch (...)
	{
		;
	} //ignore errors

	try
	{
		auto artdaqStateChangeEnabled = CorePropertySupervisorBase::getSupervisorConfigurationNode().getNode("EnableARTDAQCommanderPlugin").getValue<bool>();
		if (artdaqStateChangeEnabled)
		{
			auto artdaqStateChangePort = CorePropertySupervisorBase::getSupervisorConfigurationNode().getNode("ARTDAQCommanderID").getValue<int>();
			auto artdaqStateChangePluginType = CorePropertySupervisorBase::getSupervisorConfigurationNode().getNode("ARTDAQCommanderType").getValue<std::string>();
			theArtdaqCommandable_.init(artdaqStateChangePort, artdaqStateChangePluginType);
		}
	}
	catch (...)
	{
		;
	} //ignore errors

	if (enableStateChanges)
	{
		__SUP_COUT__ << "Enabling state changes over UDP..." << __E__;
		//start state changer UDP listener thread
		std::thread([](GatewaySupervisor *s) { GatewaySupervisor::StateChangerWorkLoop(s); }, this).detach();
	}
	else
		__SUP_COUT__ << "State changes over UDP are disabled." << __E__;



	//=========================
	//dump names of core tables (so UpdateOTS.sh can copy core tables for user)
	// only if table does not exist
	{
		const std::set<std::string>& contextMemberNames = CorePropertySupervisorBase::theConfigurationManager_->getContextMemberNames();
		const std::set<std::string>& backboneMemberNames = CorePropertySupervisorBase::theConfigurationManager_->getBackboneMemberNames();
		const std::set<std::string>& iterateMemberNames = CorePropertySupervisorBase::theConfigurationManager_->getIterateMemberNames();

		FILE * fp = fopen((CORE_TABLE_INFO_FILENAME).c_str(), "r");

		__SUP_COUT__ << "Updating core tables table..." << __E__;

		if (fp) //check for all core table names in file, and force their presence
		{
			std::vector<unsigned int> foundVector;
			char line[100];
			for (const auto &name : contextMemberNames)
			{
				foundVector.push_back(false);
				rewind(fp);
				while (fgets(line, 100, fp))
				{
					if (strlen(line) < 1) continue;
					line[strlen(line) - 1] = '\0'; //remove endline
					if (strcmp(line, name.c_str()) == 0) //is match?
					{
						foundVector.back() = true;
						//__SUP_COUTV__(name);
						break;
					}
				}
			}

			for (const auto &name : backboneMemberNames)
			{
				foundVector.push_back(false);
				rewind(fp);
				while (fgets(line, 100, fp))
				{
					if (strlen(line) < 1) continue;
					line[strlen(line) - 1] = '\0'; //remove endline
					if (strcmp(line, name.c_str()) == 0) //is match?
					{
						foundVector.back() = true;
						//__SUP_COUTV__(name);
						break;
					}
				}
			}

			for (const auto &name : iterateMemberNames)
			{
				foundVector.push_back(false);
				rewind(fp);
				while (fgets(line, 100, fp))
				{
					if (strlen(line) < 1) continue;
					line[strlen(line) - 1] = '\0'; //remove endline
					if (strcmp(line, name.c_str()) == 0) //is match?
					{
						foundVector.back() = true;
						//__SUP_COUTV__(name);
						break;
					}
				}
			}

			fclose(fp);

			//for(const auto &found:foundVector)
			//	__SUP_COUTV__(found);


			//open file for appending the missing names
			fp = fopen((CORE_TABLE_INFO_FILENAME).c_str(), "a");
			if (fp)
			{
				unsigned int i = 0;
				for (const auto &name : contextMemberNames)
				{
					if (!foundVector[i])
						fprintf(fp, "%s\n", name.c_str());

					++i;
				}
				for (const auto &name : backboneMemberNames)
				{
					if (!foundVector[i])
						fprintf(fp, "%s\n", name.c_str());

					++i;
				}
				for (const auto &name : iterateMemberNames)
				{
					if (!foundVector[i])
						fprintf(fp, "%s\n", name.c_str());

					++i;
				}
				fclose(fp);
			}
			else
			{
				__SUP_SS__ << "Failed to open core table info file for appending: " << CORE_TABLE_INFO_FILENAME << __E__;
				__SS_THROW__;
			}

		}
		else
		{
			fp = fopen((CORE_TABLE_INFO_FILENAME).c_str(), "w");
			if (fp)
			{
				for (const auto &name : contextMemberNames)
					fprintf(fp, "%s\n", name.c_str());
				for (const auto &name : backboneMemberNames)
					fprintf(fp, "%s\n", name.c_str());
				for (const auto &name : iterateMemberNames)
					fprintf(fp, "%s\n", name.c_str());
				fclose(fp);
			}
			else
			{
				__SUP_SS__ << "Failed to open core table info file: " << CORE_TABLE_INFO_FILENAME << __E__;
				__SS_THROW__;
			}
		}
	}



}

//========================================================================================================================
//StateChangerWorkLoop
//	child thread
void GatewaySupervisor::StateChangerWorkLoop(GatewaySupervisor *theSupervisor)
{
	ConfigurationTree configLinkNode = theSupervisor->CorePropertySupervisorBase::getSupervisorConfigurationNode();

	std::string ipAddressForStateChangesOverUDP = configLinkNode.getNode("IPAddressForStateChangesOverUDP").getValue<std::string>();
	int         portForStateChangesOverUDP      = configLinkNode.getNode("PortForStateChangesOverUDP"     ).getValue<int>();
	bool        acknowledgementEnabled          = configLinkNode.getNode("EnableAckForStateChangesOverUDP").getValue<bool>();

	//__COUT__ << "IPAddressForStateChangesOverUDP = " << ipAddressForStateChangesOverUDP << __E__;
	//__COUT__ << "PortForStateChangesOverUDP      = " << portForStateChangesOverUDP << __E__;
	//__COUT__ << "acknowledgmentEnabled           = " << acknowledgmentEnabled << __E__;

	TransceiverSocket sock(ipAddressForStateChangesOverUDP, portForStateChangesOverUDP); //Take Port from Configuration
	try
	{
		sock.initialize();
	}
	catch (...)
	{

		//generate special message to indicate failed socket
		__SS__ << "FATAL Console error. Could not initialize socket at ip '" << ipAddressForStateChangesOverUDP <<
			"' and port " <<
			portForStateChangesOverUDP << ". Perhaps it is already in use? Exiting State Changer receive loop." << __E__;
		__COUT__ << ss.str();
		__SS_THROW__;
		return;
	}

	std::size_t  commaPosition;
	unsigned int commaCounter = 0;
	std::size_t  begin = 0;
	std::string  buffer;
	std::string  errorStr;
	std::string  fsmName;
	std::string  command;
	std::vector<std::string> parameters;
	while (1)
	{
		//workloop procedure
		//	if receive a UDP command
		//		execute command
		//	else
		//		sleep


		if (sock.receive(buffer, 0 /*timeoutSeconds*/, 1/*timeoutUSeconds*/,
						 false /*verbose*/) != -1)
		{
			__COUT__ << "UDP State Changer received size = " << buffer.size() << __E__;
			size_t nCommas = std::count(buffer.begin(), buffer.end(), ',');
			if(nCommas == 0)
			{
				__SS__ << "Unrecognized State Machine command :-" << buffer << "-. Format is FiniteStateMachineName,Command,Parameter(s). Where Parameter(s) is/are optional." << __E__;
				__COUT_INFO__ << ss.str();
				__MOUT_INFO__ << ss.str();
			}
			begin        = 0;
			commaCounter = 0;
			parameters.clear();
			while((commaPosition = buffer.find(',', begin)) != std::string::npos || commaCounter == nCommas)
			{
				if(commaCounter == nCommas) commaPosition = buffer.size();
				if(commaCounter == 0)
					fsmName = buffer.substr(begin, commaPosition-begin);
				else if(commaCounter == 1)
					command = buffer.substr(begin, commaPosition-begin);
				else
					parameters.push_back(buffer.substr(begin, commaPosition-begin));
				__COUT__ << "Word: " << buffer.substr(begin, commaPosition-begin) << __E__;

				begin = commaPosition + 1;
				++commaCounter;
			}

			//set scope of mutext
			{
				//should be mutually exclusive with GatewaySupervisor main thread state machine accesses
				//lockout the messages array for the remainder of the scope
				//this guarantees the reading thread can safely access the messages
				if (theSupervisor->VERBOSE_MUTEX) __COUT__ << "Waiting for FSM access" << __E__;
				std::lock_guard<std::mutex> lock(theSupervisor->stateMachineAccessMutex_);
				if (theSupervisor->VERBOSE_MUTEX) __COUT__ << "Have FSM access" << __E__;

				errorStr = theSupervisor->attemptStateMachineTransition(
					0, 0,
					command, fsmName,
					WebUsers::DEFAULT_STATECHANGER_USERNAME /*fsmWindowName*/,
					WebUsers::DEFAULT_STATECHANGER_USERNAME,
					parameters);
			}

			if (errorStr != "")
			{
				__SS__ << "UDP State Changer failed to execute command because of the following error: " << errorStr;
				__COUT_ERR__ << ss.str();
				__MOUT_ERR__ << ss.str();
				if (acknowledgementEnabled) sock.acknowledge(errorStr, true /*verbose*/);
			}
			else
			{
				__SS__ << "Successfully executed state change command '" << command << ".'" << __E__;
				__COUT_INFO__ << ss.str();
				__MOUT_INFO__ << ss.str();
				if (acknowledgementEnabled) sock.acknowledge("Done", true /*verbose*/);
			}
		}
		else
			sleep(1);
	}
} //end StateChangerWorkLoop

//========================================================================================================================
//makeSystemLogbookEntry
//	makes a logbook entry into all Logbook supervisors
//		and specifically the current active experiments within the logbook
//	escape entryText to make it html/xml safe!!
////      reserved: ", ', &, <, >, \n, double-space
void GatewaySupervisor::makeSystemLogbookEntry(std::string entryText)
{
	__SUP_COUT__ << "Making System Logbook Entry: " << entryText << __E__;

	SupervisorInfoMap logbookInfoMap = allSupervisorInfo_.getAllLogbookTypeSupervisorInfo();

	if (logbookInfoMap.size() == 0)
	{
		__SUP_COUT__ << "No logbooks found! Here is entry: " << entryText << __E__;
		__MOUT__ << "No logbooks found! Here is entry: " << entryText << __E__;
		return;
	}
	else
	{
		__SUP_COUT__ << "Making logbook entry: " << entryText << __E__;
		__MOUT__ << "Making logbook entry: " << entryText << __E__;
	}


	//__SUP_COUT__ << "before: " << entryText << __E__;
	{	//input entryText
		std::string replace[] =
		{ "\"", "'", "&", "<", ">", "\n", "  " };
		std::string with[] =
		{ "%22","%27", "%26", "%3C", "%3E", "%0A%0D", "%20%20" };

		int numOfKeys = 7;

		size_t f;
		for (int i = 0; i < numOfKeys; ++i)
		{
			while ((f = entryText.find(replace[i])) != std::string::npos)
			{
				entryText = entryText.substr(0, f) + with[i] +
					entryText.substr(f + replace[i].length());
				//__SUP_COUT__ << "found " << " " << entryText << __E__;
			}
		}
	}
	//__SUP_COUT__ << "after: " << entryText << __E__;

	SOAPParameters parameters("EntryText", entryText);
	//SOAPParametersV parameters(1);
	//parameters[0].setName("EntryText"); parameters[0].setValue(entryText);

	for (auto& logbookInfo : logbookInfoMap)
	{
		xoap::MessageReference retMsg = SOAPMessenger::sendWithSOAPReply(
			logbookInfo.second.getDescriptor(),
			"MakeSystemLogbookEntry", parameters);

		SOAPParameters retParameters("Status");
		//SOAPParametersV retParameters(1);
		//retParameters[0].setName("Status");
		receive(retMsg, retParameters);

		__SUP_COUT__ << "Returned Status: " << retParameters.getValue("Status") << __E__;//retParameters[0].getValue() << __E__ << __E__;
	}

}

//========================================================================================================================
void GatewaySupervisor::Default(xgi::Input* in, xgi::Output* out)

{
	//
	if (!supervisorGuiHasBeenLoaded_ && (supervisorGuiHasBeenLoaded_ = true)) //make system logbook entry that ots has been started
		makeSystemLogbookEntry("ots started.");

	*out <<
		"<!DOCTYPE HTML><html lang='en'><head><title>ots</title>" <<
		//show ots icon
		//	from http://www.favicon-generator.org/
		"<link rel='apple-touch-icon' sizes='57x57' href='/WebPath/images/otsdaqIcons/apple-icon-57x57.png'>\
		<link rel='apple-touch-icon' sizes='60x60' href='/WebPath/images/otsdaqIcons/apple-icon-60x60.png'>\
		<link rel='apple-touch-icon' sizes='72x72' href='/WebPath/images/otsdaqIcons/apple-icon-72x72.png'>\
		<link rel='apple-touch-icon' sizes='76x76' href='/WebPath/images/otsdaqIcons/apple-icon-76x76.png'>\
		<link rel='apple-touch-icon' sizes='114x114' href='/WebPath/images/otsdaqIcons/apple-icon-114x114.png'>\
		<link rel='apple-touch-icon' sizes='120x120' href='/WebPath/images/otsdaqIcons/apple-icon-120x120.png'>\
		<link rel='apple-touch-icon' sizes='144x144' href='/WebPath/images/otsdaqIcons/apple-icon-144x144.png'>\
		<link rel='apple-touch-icon' sizes='152x152' href='/WebPath/images/otsdaqIcons/apple-icon-152x152.png'>\
		<link rel='apple-touch-icon' sizes='180x180' href='/WebPath/images/otsdaqIcons/apple-icon-180x180.png'>\
		<link rel='icon' type='image/png' sizes='192x192'  href='/WebPath/images/otsdaqIcons/android-icon-192x192.png'>\
		<link rel='icon' type='image/png' sizes='32x32' href='/WebPath/images/otsdaqIcons/favicon-32x32.png'>\
		<link rel='icon' type='image/png' sizes='96x96' href='/WebPath/images/otsdaqIcons/favicon-96x96.png'>\
		<link rel='icon' type='image/png' sizes='16x16' href='/WebPath/images/otsdaqIcons/favicon-16x16.png'>\
		<link rel='manifest' href='/WebPath/images/otsdaqIcons/manifest.json'>\
		<meta name='msapplication-TileColor' content='#ffffff'>\
		<meta name='msapplication-TileImage' content='/ms-icon-144x144.png'>\
		<meta name='theme-color' content='#ffffff'>" <<
		//end show ots icon
		"</head>" <<
		"<frameset col='100%' row='100%'>" <<
		"<frame src='/WebPath/html/Desktop.html?urn=" <<
		this->getApplicationDescriptor()->getLocalId() << "=securityType=" <<
		securityType_ << "'></frameset></html>";
}

//========================================================================================================================
void GatewaySupervisor::stateMachineXgiHandler(xgi::Input* in, xgi::Output* out)
{
	//for simplicity assume all commands should be mutually exclusive with iterator thread state machine accesses (really should just be careful with RunControlStateMachine access)
	if (VERBOSE_MUTEX) __SUP_COUT__ << "Waiting for FSM access" << __E__;
	std::lock_guard<std::mutex> lock(stateMachineAccessMutex_);
	if (VERBOSE_MUTEX) __SUP_COUT__ << "Have FSM access" << __E__;

	cgicc::Cgicc cgiIn(in);

	std::string command = CgiDataUtilities::getData(cgiIn, "StateMachine");
	std::string requestType = "StateMachine" + command; //prepend StateMachine to request type

	HttpXmlDocument xmlOut;
	WebUsers::RequestUserInfo userInfo(requestType,
			CgiDataUtilities::postData(cgiIn, "CookieCode"));

	CorePropertySupervisorBase::getRequestUserInfo(userInfo);

	if(!theWebUsers_.xmlRequestOnGateway(
			cgiIn,
			out,
			&xmlOut,
			userInfo
			))
		return; //access failed

//	uint8_t userPermissions;
//	uint64_t uid;
//	std::string userWithLock;
//	std::string cookieCode = CgiDataUtilities::postData(cgiIn, "CookieCode");
//	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode, &userPermissions,
//			&uid, "0", 1, &userWithLock))
//	{
//		*out << cookieCode;
//		return;
//	}
//
//	std::string command = CgiDataUtilities::getData(cgiIn, "StateMachine");
//
//	//**** start LOCK GATEWAY CODE ***//
//	std::string username = "";
//	username = theWebUsers_.getUsersUsername(uid);
//	if (userWithLock != "" && userWithLock != username)
//	{
//		*out << WebUsers::REQ_USER_LOCKOUT_RESPONSE;
//		__SUP_COUT__ << "User " << username << " is locked out. " << userWithLock << " has lock." << __E__;
//		return;
//	}
//	//**** end LOCK GATEWAY CODE ***//
//
//	HttpXmlDocument xmlOut(cookieCode);

	std::string fsmName = CgiDataUtilities::getData(cgiIn, "fsmName");
	std::string fsmWindowName = CgiDataUtilities::getData(cgiIn, "fsmWindowName");
	fsmWindowName = CgiDataUtilities::decodeURIComponent(fsmWindowName);
	std::string currentState = theStateMachine_.getCurrentStateName();

	__SUP_COUT__ << "Check for Handled by theIterator_" << __E__;

	//check if Iterator should handle
	if((activeStateMachineWindowName_ == "" ||
			activeStateMachineWindowName_ == "iterator") &&
			theIterator_.handleCommandRequest(xmlOut,command,fsmWindowName))
	{
		__SUP_COUT__ << "Handled by theIterator_" << __E__;
		xmlOut.outputXmlDocument((std::ostringstream*) out, false);
		return;
	}

	//Do not allow transition while in transition
	if (theStateMachine_.isInTransition())
	{
		__SUP_SS__ << "Error - Can not accept request because the State Machine is already in transition!" << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();

		xmlOut.addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
		xmlOut.addTextElementToData("state_tranisition_attempted_err",
				ss.str()); //indicate to GUI transition NOT attempted
		xmlOut.outputXmlDocument((std::ostringstream*) out, false, true);
		return;
	}



	/////////////////
	//Validate FSM name
	//	if fsm name != active fsm name
	//		only allow, if current state is halted or init
	//		take active fsm name when configured
	//	else, allow
	if (activeStateMachineName_ != "" &&
		activeStateMachineName_ != fsmName)
	{
		__SUP_COUT__ << "currentState = " <<
			currentState << __E__;
		if (currentState != "Halted" &&
			currentState != "Initial")
		{
			//illegal for this FSM name to attempt transition

			__SUP_SS__ << "Error - Can not accept request because the State Machine " <<
				"with window name '" <<
				activeStateMachineWindowName_ << "' (UID: " <<
				activeStateMachineName_ << ") "
				"is currently " <<
				"in control of State Machine progress. ";
			ss << "\n\nIn order for this State Machine with window name '" <<
				fsmWindowName << "' (UID: " << fsmName << ") "
				"to control progress, please transition to Halted using the active " <<
				"State Machine '" << activeStateMachineWindowName_ << ".'" << __E__;
			__SUP_COUT_ERR__ << "\n" << ss.str();

			xmlOut.addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
			xmlOut.addTextElementToData("state_tranisition_attempted_err",
					ss.str()); //indicate to GUI transition NOT attempted
			xmlOut.outputXmlDocument((std::ostringstream*) out, false, true);
			return;
		}
		else	//clear active state machine
		{
			activeStateMachineName_ = "";
			activeStateMachineWindowName_ = "";
		}
	}




	//At this point, attempting transition!

	std::vector<std::string> parameters;
	if(command == "Configure")
		parameters.push_back(CgiDataUtilities::postData(cgiIn, "ConfigurationAlias"));
	attemptStateMachineTransition(&xmlOut,out,command,fsmName,fsmWindowName,
			userInfo.username_,parameters);
}

//========================================================================================================================
std::string GatewaySupervisor::attemptStateMachineTransition(
	HttpXmlDocument* xmldoc, std::ostringstream* out,
	const std::string& command,
	const std::string& fsmName, const std::string& fsmWindowName,
	const std::string& username,
	const std::vector<std::string>& commandParameters)
{
	std::string errorStr = "";

	std::string currentState = theStateMachine_.getCurrentStateName();
	__SUP_COUT__ << "State Machine command = " << command << __E__;
	__SUP_COUT__ << "fsmName = " << fsmName << __E__;
	__SUP_COUT__ << "fsmWindowName = " << fsmWindowName << __E__;
	__SUP_COUT__ << "activeStateMachineName_ = " << activeStateMachineName_ << __E__;
	__SUP_COUT__ << "command = " << command << __E__;
	__SUP_COUT__ << "commandParameters.size = " << commandParameters.size() << __E__;

	SOAPParameters parameters;
	if (command == "Configure")
	{
		if (currentState != "Halted") //check if out of sync command
		{
			__SUP_SS__ << "Error - Can only transition to Configured if the current " <<
				"state is Halted. Perhaps your state machine is out of sync." <<
				__E__;
			__SUP_COUT_ERR__ << "\n" << ss.str();
			errorStr = ss.str();

			if (xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
			if (xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted_err",
													 ss.str()); //indicate to GUI transition NOT attempted
			if (out) xmldoc->outputXmlDocument((std::ostringstream*) out, false /*dispStdOut*/, true /*allowWhiteSpace*/);

			return errorStr;
		}

		//NOTE Original name of the configuration key
		//parameters.addParameter("RUN_KEY",CgiDataUtilities::postData(cgi,"ConfigurationAlias"));
		if (commandParameters.size() == 0)
		{
			__SUP_SS__ << "Error - Can only transition to Configured if a Configuration Alias parameter is provided." <<
				__E__;
			__SUP_COUT_ERR__ << "\n" << ss.str();
			errorStr = ss.str();

			if (xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
			if (xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted_err",
													 ss.str()); //indicate to GUI transition NOT attempted
			if (out) xmldoc->outputXmlDocument((std::ostringstream*) out, false /*dispStdOut*/, true /*allowWhiteSpace*/);

			return errorStr;
		}

		parameters.addParameter("ConfigurationAlias",
								commandParameters[0]);

		std::string configurationAlias = parameters.getValue("ConfigurationAlias");
		__SUP_COUT__ << "Configure --> Name: ConfigurationAlias Value: " <<
			configurationAlias << __E__;

		//save last used config alias
		std::string fn = FSM_LAST_GROUP_ALIAS_PATH + FSM_LAST_GROUP_ALIAS_FILE_START +
			username + "." + FSM_USERS_PREFERENCES_FILETYPE;

		__SUP_COUT__ << "Save FSM preferences: " << fn << __E__;
		FILE *fp = fopen(fn.c_str(), "w");
		if (!fp)
		{
			__SUP_SS__ << ("Could not open file: " + fn) << __E__;
			__SUP_COUT_ERR__ << ss.str();
			__SS_THROW__;
		}
		fprintf(fp, "FSM_last_configuration_alias %s", configurationAlias.c_str());
		fclose(fp);

		activeStateMachineName_ = fsmName;
		activeStateMachineWindowName_ = fsmWindowName;
	}
	else if (command == "Start")
	{
		if (currentState != "Configured") //check if out of sync command
		{
			__SUP_SS__ << "Error - Can only transition to Configured if the current " <<
				"state is Halted. Perhaps your state machine is out of sync. " <<
				"(Likely the server was restarted or another user changed the state)" <<
				__E__;
			__SUP_COUT_ERR__ << "\n" << ss.str();
			errorStr = ss.str();

			if (xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
			if (xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted_err",
													 ss.str()); //indicate to GUI transition NOT attempted
			if (out) xmldoc->outputXmlDocument((std::ostringstream*) out, false /*dispStdOut*/, true /*allowWhiteSpace*/);

			return errorStr;
		}
		unsigned int runNumber;
		if(commandParameters.size() == 0)
		{
			runNumber = getNextRunNumber();
			setNextRunNumber(runNumber+1);
		}
		else
		{
			runNumber = std::atoi(commandParameters[0].c_str());
		}
		parameters.addParameter("RunNumber", runNumber);
	}

	xoap::MessageReference message = SOAPUtilities::makeSOAPMessageReference(
		command, parameters);
	//Maybe we return an acknowledgment that the message has been received and processed
	xoap::MessageReference reply = stateMachineXoapHandler(message);
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);

	if (xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted", "1"); //indicate to GUI transition attempted
	if (out) xmldoc->outputXmlDocument((std::ostringstream*) out, false);
	__SUP_COUT__ << "FSM state transition launched!" << __E__;

	stateMachineLastCommandInput_ = command;
	return errorStr;
}

////========================================================================================================================
////FIXME -- delete? is this ever used by anything? ever?
//void GatewaySupervisor::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out)
//throw (xgi::exception::Exception)
//{
//	cgicc::Cgicc cgiIn(in);
//	__SUP_COUT__ << "Xgi Request!" << __E__;
//
//	uint8_t userPermissions; // uint64_t uid;
//	std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
//	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode,
//			&userPermissions))
//	{
//		*out << cookieCode;
//		return;
//	}
//
//	HttpXmlDocument xmldoc(cookieCode);
//
//	std::string command = CgiDataUtilities::getData(cgi, "StateMachine");
//
//	SOAPParameters parameters;
//	/*
//	 //FIXME I don't think that there should be this if statement and I guess it has been copied from the stateMachineXgiHandler method
//	 if (command == "Configure")
//
//{
//	 parameters.addParameter("RUN_KEY", CgiDataUtilities::postData(cgi, "ConfigurationAlias"));
//	 __SUP_COUT__ << "Configure --> Name: RUN_KEY Value: " << parameters.getValue("RUN_KEY") << __E__;
//	 }
//	 else if (command == "Start")
//
//{
//	 unsigned int runNumber = getNextRunNumber();
//	 std::stringstream runNumberStream;
//	 runNumberStream << runNumber;
//	 parameters.addParameter("RUN_NUMBER", runNumberStream.str().c_str());
//	 setNextRunNumber(++runNumber);
//	 }
//	 */
//	xoap::MessageReference message = SOAPUtilities::makeSOAPMessageReference(
//			CgiDataUtilities::getData(cgi, "StateMachine"), parameters);
//	//Maybe we return an aknowledgment that the message has been received and processed
//	xoap::MessageReference reply = stateMachineResultXoapHandler(message);
//	//stateMachineWorkLoopManager_.removeProcessedRequests();
//	//stateMachineWorkLoopManager_.processRequest(message);
//	//xmldoc.outputXmlDocument((ostringstream*)out,false);
//	__SUP_COUT__ << "Done - Xgi Request!" << __E__;
//}

//========================================================================================================================
xoap::MessageReference GatewaySupervisor::stateMachineXoapHandler(xoap::MessageReference message)

{
	__SUP_COUT__ << "Soap Handler!" << __E__;
	stateMachineWorkLoopManager_.removeProcessedRequests();
	stateMachineWorkLoopManager_.processRequest(message);
	__SUP_COUT__ << "Done - Soap Handler!" << __E__;
	return message;
}

//========================================================================================================================
xoap::MessageReference GatewaySupervisor::stateMachineResultXoapHandler(
	xoap::MessageReference message)
	
{
	__SUP_COUT__ << "Soap Handler!" << __E__;
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);
	__SUP_COUT__ << "Done - Soap Handler!" << __E__;
	return message;
}

//========================================================================================================================
bool GatewaySupervisor::stateMachineThread(toolbox::task::WorkLoop* workLoop)
{
	stateMachineSemaphore_.take();
	std::string command = SOAPUtilities::translate(
			stateMachineWorkLoopManager_.getMessage(workLoop)).getCommand();

	__SUP_COUT__ << "Propagating command '" << command << "'..." << __E__;

	std::string reply = send(
		allSupervisorInfo_.getGatewayDescriptor(),
		stateMachineWorkLoopManager_.getMessage(workLoop));
	stateMachineWorkLoopManager_.report(workLoop, reply, 100, true);

	__SUP_COUT__ << "Done with command '" << command << ".' Reply = " << reply << __E__;
	stateMachineSemaphore_.give();

	if (reply == "Fault")
	{
		__SUP_SS__ << "Failure to send Workloop transition command '" << command <<
				"!' An error response '" << reply << "' was received." << __E__;
		__SUP_COUT_ERR__ << ss.str();
		__MOUT_ERR__ << ss.str();
	}
	return false; //execute once and automatically remove the workloop so in WorkLoopManager the try workLoop->remove(job_) could be commented out
	//return true;//go on and then you must do the workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
//infoRequestHandler ~~
//	Call from JS GUI
//		parameter:
//
void GatewaySupervisor::infoRequestHandler(xgi::Input* in, xgi::Output* out)

{
	__SUP_COUT__ << "Starting to Request!" << __E__;


	cgicc::Cgicc cgiIn(in);
	std::string requestType = "infoRequestHandler"; //force request type to infoRequestHandler

	HttpXmlDocument xmlOut;
	WebUsers::RequestUserInfo userInfo(requestType,
			CgiDataUtilities::postData(cgiIn, "CookieCode"));

	CorePropertySupervisorBase::getRequestUserInfo(userInfo);

	if(!theWebUsers_.xmlRequestOnGateway(
			cgiIn,
			out,
			&xmlOut,
			userInfo
	))
		return; //access failed

	//If the thread is canceled when you are still in this method that activated it, then everything will freeze.
	//The way the workloop manager now works is safe since it cancel the thread only when the infoRequestResultHandler is called
	//and that method can be called ONLY when I am already out of here!

	HttpXmlDocument tmpDoc = infoRequestWorkLoopManager_.processRequest(cgiIn);

	xmlOut.copyDataChildren(tmpDoc);

	xmlOut.outputXmlDocument((std::ostringstream*) out, false);
}

//========================================================================================================================
void GatewaySupervisor::infoRequestResultHandler(xgi::Input* in, xgi::Output* out)

{
	__SUP_COUT__ << "Starting ask!" << __E__;
	cgicc::Cgicc cgi(in);


	cgicc::Cgicc cgiIn(in);
	std::string requestType = "infoRequestResultHandler"; //force request type to infoRequestResultHandler

	HttpXmlDocument xmlOut;
	WebUsers::RequestUserInfo userInfo(requestType,
			CgiDataUtilities::postData(cgiIn, "CookieCode"));

	CorePropertySupervisorBase::getRequestUserInfo(userInfo);

	if(!theWebUsers_.xmlRequestOnGateway(
			cgiIn,
			out,
			&xmlOut,
			userInfo
	))
		return; //access failed

//	//**** start LOGIN GATEWAY CODE ***//
//	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions, uint64_t uid
//	//Else, error message is returned in cookieCode
//	std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
//	uint8_t userPermissions;// uint64_t uid;
//	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode, &userPermissions))
//	{
//		*out << cookieCode;
//		return;
//	}
//	//**** end LOGIN GATEWAY CODE ***//
//
//	HttpXmlDocument xmldoc(cookieCode);

	infoRequestWorkLoopManager_.getRequestResult(cgiIn, xmlOut);

	//return xml doc holding server response
	xmlOut.outputXmlDocument((std::ostringstream*) out, false);

	__SUP_COUT__ << "Done asking!" << __E__;
}

//========================================================================================================================
bool GatewaySupervisor::infoRequestThread(toolbox::task::WorkLoop* workLoop)
{
	//    std::string workLoopName = workLoop->getName();
	//    __SUP_COUT__ << " Starting WorkLoop: " << workLoopName << __E__;
	//    __SUP_COUT__ << " Ready to lock" << __E__;
	infoRequestSemaphore_.take();
	//    __SUP_COUT__ << " Locked" << __E__;
	vectorTest_.clear();

	for (unsigned long long i = 0; i < 100000000; i++)
	{
		counterTest_ += 2;
		vectorTest_.push_back(counterTest_);
	}

	infoRequestWorkLoopManager_.report(workLoop,
									   "RESULT: This is the best result ever", 50, false);
	std::string workLoopName = workLoop->getName();
	__SUP_COUT__ << workLoopName << " test: " << counterTest_
		<< " vector size: " << vectorTest_.size() << __E__;
	wait(400, "InfoRequestThread ----- locked");
	infoRequestSemaphore_.give();
	//    __SUP_COUT__ << " Lock released" << __E__;
	wait(200, "InfoRequestThread");
	//    __SUP_COUT__ << " Ready to lock again" << __E__;
	infoRequestSemaphore_.take();
	//    __SUP_COUT__ << " Locked again" << __E__;
	vectorTest_.clear();

	for (unsigned long long i = 0; i < 100000000; i++)
	{
		counterTest_ += 2;
		vectorTest_.push_back(counterTest_);
	}

	wait(400, "InfoRequestThread ----- locked");
	__SUP_COUT__ << workLoopName << " test: " << counterTest_ << " vector size: " << vectorTest_.size() << __E__;
	infoRequestSemaphore_.give();
	//    __SUP_COUT__ << " Lock released again" << __E__;
	//infoRequestWorkLoopManager_->report(workLoop,"RESULT: This is the best result ever");
	infoRequestWorkLoopManager_.report(workLoop,
									   theStateMachine_.getCurrentStateName(), 100, true);
	//    __SUP_COUT__ << " Done with WorkLoop: " << workLoopName << __E__;
	return false; //execute once and automatically remove the workloop so in WorkLoopManager the try workLoop->remove(job_) could be commented out
	//return true;//go on and then you must do the workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
void GatewaySupervisor::stateInitial(toolbox::fsm::FiniteStateMachine & fsm)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;
	//diagService_->reportError("--- GatewaySupervisor is in its Initial state ---",DIAGINFO);
	//diagService_->reportError("GatewaySupervisor::stateInitial: workloop active: "+stringF(calibWorkloop_->isActive())+", workloop type: "+calibWorkloop_->getType(),DIAGINFO);
}

//========================================================================================================================
void GatewaySupervisor::statePaused(toolbox::fsm::FiniteStateMachine & fsm)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;
	/*
	 try
{
	 rcmsStateNotifier_.stateChanged("Paused", "");
	 }
	 catch(xcept::Exception &e)
{
	 diagService_->reportError("Failed to notify state change : "+ xcept::stdformat_exception_history(e),DIAGERROR);
	 }

	 diagService_->reportError("GatewaySupervisor::statePaused: workloop active: "+stringF(calibWorkloop_->isActive())+", workloop type: "+calibWorkloop_->getType(),DIAGINFO);
	 */
}

//========================================================================================================================
void GatewaySupervisor::stateRunning(toolbox::fsm::FiniteStateMachine & fsm)

{

	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;
	/*
	 try
{
	 rcmsStateNotifier_.stateChanged("Running", "");
	 }
	 catch(xcept::Exception &e)
{
	 diagService_->reportError("Failed to notify state change : "+ xcept::stdformat_exception_history(e),DIAGERROR);
	 }

	 diagService_->reportError("GatewaySupervisor::stateRunning: workloop active: "+stringF(calibWorkloop_->isActive())+", workloop type: "+calibWorkloop_->getType(),DIAGINFO);
	 */
}

//========================================================================================================================
void GatewaySupervisor::stateHalted(toolbox::fsm::FiniteStateMachine& fsm)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;
	__SUP_COUT__ << "Fsm is in transition? " << (theStateMachine_.isInTransition() ? "yes" : "no") << __E__;


	/*
	 percentageConfigured_=0.0;
	 aliasesAndKeys_=PixelConfigInterface::getAliases();
	 diagService_->reportError("GatewaySupervisor::stateHalted: aliases and keys reloaded",DIAGINFO);
	 previousState_ = theStateMachine_.getCurrentStateName();
	 diagService_->reportError("GatewaySupervisor::stateHalted: workloop active: "+stringF(calibWorkloop_->isActive())+", workloop type: "+calibWorkloop_->getType(),DIAGINFO);

	 //(hopefully) make RCMS aware of successful Recovery
	 //need to test that: (a) this works and (b) this does not break anything (regular Halt transition)
	 //update -- condition (b) seems to be satisfied. not yet sure about condition (a)
	 try
{
	 rcmsStateNotifier_.stateChanged("Halted", "");
	 }
	 catch(xcept::Exception &e)

{
	 diagService_->reportError("Failed to notify state change : "+ xcept::stdformat_exception_history(e),DIAGERROR);
	 }
	 */
}

//========================================================================================================================
void GatewaySupervisor::stateConfigured(toolbox::fsm::FiniteStateMachine & fsm)

{
	/*
	 // Notify RCMS of having entered the Configured state
	 try

{
	 //rcmsStateNotifier_.stateChanged(previousState_.toString(), "");
	 rcmsStateNotifier_.stateChanged("Configured", std::stringF(theGlobalKey_->key()) );
	 }
	 catch(xcept::Exception &e)

{
	 diagService_->reportError("Failed to notify state change : "+ xcept::stdformat_exception_history(e),DIAGERROR);
	 }

	 PixelTimer debugTimer;
	 if (extratimers_)
{
	 debugTimer.setName("GatewaySupervisor::stateConfigured");
	 debugTimer.printTime("RCMS notified of Configured state");
	 }
	 if (configurationTimer_.started() )
{
	 configurationTimer_.stop();

	 std::string confsource(getenv("PIXELCONFIGURATIONBASE"));
	 if (confsource != "DB") confsource = "files";

	 diagService_->reportError("Total configuration time ["+confsource+"] = "+stringF(configurationTimer_.tottime()),DIAGUSERINFO);
	 configurationTimer_.reset();
	 }

	 diagService_->reportError( "GatewaySupervisor::stateConfigured: workloop active: "+stringF(calibWorkloop_->isActive())+", workloop type: "+calibWorkloop_->getType(),DIAGDEBUG);
	 */
}

//void GatewaySupervisor::stateTTSTestMode (toolbox::fsm::FiniteStateMachine & fsm) 
//{
//  previousState_ = theStateMachine_.getCurrentStateName();
//}

//void GatewaySupervisor::inError (toolbox::fsm::FiniteStateMachine & fsm) 
//{
//  previousState_ = theStateMachine_.getCurrentStateName();
//rcmsStateNotifier_.stateChanged("Error", "");
//}

/*
 xoap::MessageReference GatewaySupervisor::reset (xoap::MessageReference message) 

{
 //diagService_->reportError("New state before reset is: " + theStateMachine_.getCurrentStateName(),DIAGINFO);

 theStateMachine_.reset();

 xoap::MessageReference reply = xoap::createMessage();
 xoap::SOAPEnvelope envelope = reply->getSOAPPart().getEnvelope();
 xoap::SOAPName responseName = envelope.createName("ResetResponse", "xdaq", XDAQ_NS_URI);
 (void) envelope.getBody().addBodyElement ( responseName );

 diagService_->reportError("New state after reset is: " + theStateMachine_.getCurrentStateName(),DIAGINFO);

 return reply;
 }
 */

 //========================================================================================================================
void GatewaySupervisor::inError(toolbox::fsm::FiniteStateMachine & fsm)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;
	//rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void GatewaySupervisor::enteringError(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;

	//extract error message and save for user interface access
	toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&> (*e);
	__SUP_SS__ << "\nFailure performing transition from " << failedEvent.getFromState() << "-" <<
		theStateMachine_.getStateName(failedEvent.getFromState()) <<
		" to " << failedEvent.getToState() << "-" <<
		theStateMachine_.getStateName(failedEvent.getToState()) <<
		".\n\nException:\n" << failedEvent.getException().what() << __E__;
	__SUP_COUT_ERR__ << "\n" << ss.str();

	theStateMachine_.setErrorMessage(ss.str());

	//move everything else to Error!
	broadcastMessage(SOAPUtilities::makeSOAPMessageReference("Error"));
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// FSM State Transition Functions //////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


//========================================================================================================================
void GatewaySupervisor::transitionConfiguring(toolbox::Event::Reference e)

{
	RunControlStateMachine::theProgressBar_.step();

	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;

	std::string systemAlias = SOAPUtilities::translate(
		theStateMachine_.getCurrentMessage()).getParameters().getValue("ConfigurationAlias");

	__SUP_COUT__ << "Transition parameter: " << systemAlias << __E__;

	RunControlStateMachine::theProgressBar_.step();


	try
	{
		CorePropertySupervisorBase::theConfigurationManager_->init(); //completely reset to re-align with any changes
	}
	catch (...)
	{
		__SUP_SS__ << "\nTransition to Configuring interrupted! " <<
			"The Configuration Manager could not be initialized." << __E__;

		__SUP_COUT_ERR__ << "\n" << ss.str();
		XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
		return;
	}

	RunControlStateMachine::theProgressBar_.step();

	//Translate the system alias to a group name/key
	try
	{
		theConfigurationGroup_ = CorePropertySupervisorBase::theConfigurationManager_->getConfigurationGroupFromAlias(systemAlias);
	}
	catch (...)
	{
		__SUP_COUT_INFO__ << "Exception occurred" << __E__;
	}

	RunControlStateMachine::theProgressBar_.step();

	if (theConfigurationGroup_.second.isInvalid())
	{
		__SUP_SS__ << "\nTransition to Configuring interrupted! System Alias " <<
			systemAlias << " could not be translated to a group name and key." << __E__;

		__SUP_COUT_ERR__ << "\n" << ss.str();
		XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
		return;
	}

	RunControlStateMachine::theProgressBar_.step();

	__SUP_COUT__ << "Configuration group name: " << theConfigurationGroup_.first << " key: " <<
		theConfigurationGroup_.second << __E__;

	//make logbook entry
	{
		std::stringstream ss;
		ss << "Configuring '" << systemAlias << "' which translates to " <<
			theConfigurationGroup_.first << " (" << theConfigurationGroup_.second << ").";
		makeSystemLogbookEntry(ss.str());
	}

	RunControlStateMachine::theProgressBar_.step();

	//load and activate
	try
	{
		CorePropertySupervisorBase::theConfigurationManager_->loadConfigurationGroup(theConfigurationGroup_.first, theConfigurationGroup_.second, true);

		//When configured, set the translated System Alias to be persistently active
		ConfigurationManagerRW tmpCfgMgr("TheSupervisor");
		tmpCfgMgr.activateConfigurationGroup(theConfigurationGroup_.first, theConfigurationGroup_.second);
	}
	catch (...)
	{
		__SUP_SS__ << "\nTransition to Configuring interrupted! System Alias " <<
			systemAlias << " was translated to " << theConfigurationGroup_.first <<
			" (" << theConfigurationGroup_.second << ") but could not be loaded and initialized." << __E__;
		ss << "\n\nTo debug this problem, try activating this group in the Configuration GUI " <<
			" and detailed errors will be shown." << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
		return;
	}

	//check if configuration dump is enabled on configure transition
	{
		ConfigurationTree configLinkNode = CorePropertySupervisorBase::theConfigurationManager_->getSupervisorConfigurationNode(
			supervisorContextUID_, supervisorApplicationUID_);
		if (!configLinkNode.isDisconnected())
		{

			try //errors in dump are not tolerated
			{
				bool dumpConfiguration = true;
				std::string dumpFilePath, dumpFileRadix, dumpFormat;
				try //for backwards compatibility
				{
					ConfigurationTree fsmLinkNode = configLinkNode.getNode("LinkToStateMachineConfiguration").
						getNode(activeStateMachineName_);
					dumpConfiguration = fsmLinkNode.getNode("EnableConfigurationDumpOnConfigureTransition").
						getValue<bool>();
					dumpFilePath = fsmLinkNode.getNode("ConfigurationDumpOnConfigureFilePath").getValue<std::string>();
					dumpFileRadix = fsmLinkNode.getNode("ConfigurationDumpOnConfigureFileRadix").getValue<std::string>();
					dumpFormat = fsmLinkNode.getNode("ConfigurationDumpOnConfigureFormat").getValue<std::string>();

				}
				catch (std::runtime_error &e)
				{
					__SUP_COUT_INFO__ << "FSM configuration dump Link disconnected." << __E__;
					dumpConfiguration = false;
				}

				if (dumpConfiguration)
				{
					//dump configuration
					CorePropertySupervisorBase::theConfigurationManager_->dumpActiveConfiguration(
						dumpFilePath +
						"/" +
						dumpFileRadix +
						"_" +
						std::to_string(time(0)) +
						".dump",
						dumpFormat
					);
				}

			}
			catch (std::runtime_error &e)
			{
				__SUP_SS__ << "\nTransition to Configuring interrupted! There was an error identified " <<
					"during the configuration dump attempt:\n\n " <<
					e.what() << __E__;
				__SUP_COUT_ERR__ << "\n" << ss.str();
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
				return;
			}
			catch (...)
			{
				__SUP_SS__ << "\nTransition to Configuring interrupted! There was an error identified " <<
					"during the configuration dump attempt.\n\n " <<
					__E__;
				__SUP_COUT_ERR__ << "\n" << ss.str();
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
				return;
			}
		}
	}

	RunControlStateMachine::theProgressBar_.step();
	SOAPParameters parameters;
	parameters.addParameter("ConfigurationGroupName", theConfigurationGroup_.first);
	parameters.addParameter("ConfigurationGroupKey", theConfigurationGroup_.second.toString());

	//xoap::MessageReference message = SOAPUtilities::makeSOAPMessageReference(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getCommand(), parameters);
	xoap::MessageReference message = theStateMachine_.getCurrentMessage();
	SOAPUtilities::addParameters(message, parameters);
	broadcastMessage(message);
	RunControlStateMachine::theProgressBar_.step();
	// Advertise the exiting of this method
	//diagService_->reportError("GatewaySupervisor::stateConfiguring: Exiting",DIAGINFO);

	//save last configured group name/key
	saveGroupNameAndKey(theConfigurationGroup_, FSM_LAST_CONFIGURED_GROUP_ALIAS_FILE);

	__SUP_COUT__ << "Done" << __E__;
	RunControlStateMachine::theProgressBar_.complete();
}

//========================================================================================================================
void GatewaySupervisor::transitionHalting(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;

	makeSystemLogbookEntry("Run halting.");

	broadcastMessage(theStateMachine_.getCurrentMessage());
}

//========================================================================================================================
void GatewaySupervisor::transitionShuttingDown(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;

	RunControlStateMachine::theProgressBar_.step();
	makeSystemLogbookEntry("System shutting down.");
	RunControlStateMachine::theProgressBar_.step();

	//kill all non-gateway contexts
	launchStartOTSCommand("OTS_APP_SHUTDOWN");
	RunControlStateMachine::theProgressBar_.step();

	//important to give time for StartOTS script to recognize command (before user does Startup again)
	for (int i = 0; i < 5; ++i)
	{
		sleep(1);
		RunControlStateMachine::theProgressBar_.step();
	}
}

//========================================================================================================================
void GatewaySupervisor::transitionStartingUp(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;

	RunControlStateMachine::theProgressBar_.step();
	makeSystemLogbookEntry("System starting up.");
	RunControlStateMachine::theProgressBar_.step();

	//start all non-gateway contexts
	launchStartOTSCommand("OTS_APP_STARTUP");
	RunControlStateMachine::theProgressBar_.step();

	//important to give time for StartOTS script to recognize command and for apps to instantiate things (before user does Initialize)
	for (int i = 0; i < 10; ++i)
	{
		sleep(1);
		RunControlStateMachine::theProgressBar_.step();
	}
}

//========================================================================================================================
void GatewaySupervisor::transitionInitializing(toolbox::Event::Reference e)

{
	__SUP_COUT__ << theStateMachine_.getCurrentStateName() << __E__;

	if (!broadcastMessage(theStateMachine_.getCurrentMessage()))
	{
		__SUP_COUT_ERR__ << "I can't Initialize the supervisors!" << __E__;
	}

	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;
	__SUP_COUT__ << "Fsm current transition: " << theStateMachine_.getCurrentTransitionName(e->type()) << __E__;
	__SUP_COUT__ << "Fsm final state: " << theStateMachine_.getTransitionFinalStateName(e->type()) << __E__;
}

//========================================================================================================================
void GatewaySupervisor::transitionPausing(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;

	makeSystemLogbookEntry("Run pausing.");

	broadcastMessage(theStateMachine_.getCurrentMessage());
}

//========================================================================================================================
void GatewaySupervisor::transitionResuming(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;

	makeSystemLogbookEntry("Run resuming.");

	broadcastMessage(theStateMachine_.getCurrentMessage());
}

//========================================================================================================================
void GatewaySupervisor::transitionStarting(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;

	SOAPParameters parameters("RunNumber");
	receive(theStateMachine_.getCurrentMessage(), parameters);

	std::string runNumber = parameters.getValue("RunNumber");
	__SUP_COUT__ << runNumber << __E__;

	//check if configuration dump is enabled on configure transition
	{
		ConfigurationTree configLinkNode = CorePropertySupervisorBase::theConfigurationManager_->getSupervisorConfigurationNode(
			supervisorContextUID_, supervisorApplicationUID_);
		if (!configLinkNode.isDisconnected())
		{
			try //errors in dump are not tolerated
			{
				bool dumpConfiguration = true;
				std::string dumpFilePath, dumpFileRadix, dumpFormat;
				try //for backwards compatibility
				{
					ConfigurationTree fsmLinkNode = configLinkNode.getNode("LinkToStateMachineConfiguration").
						getNode(activeStateMachineName_);
					dumpConfiguration = fsmLinkNode.getNode("EnableConfigurationDumpOnRunTransition").
						getValue<bool>();
					dumpFilePath = fsmLinkNode.getNode("ConfigurationDumpOnRunFilePath").getValue<std::string>();
					dumpFileRadix = fsmLinkNode.getNode("ConfigurationDumpOnRunFileRadix").getValue<std::string>();
					dumpFormat = fsmLinkNode.getNode("ConfigurationDumpOnRunFormat").getValue<std::string>();
				}
				catch (std::runtime_error &e)
				{
					__SUP_COUT_INFO__ << "FSM configuration dump Link disconnected." << __E__;
					dumpConfiguration = false;
				}

				if (dumpConfiguration)
				{
					//dump configuration
					CorePropertySupervisorBase::theConfigurationManager_->dumpActiveConfiguration(
						dumpFilePath +
						"/" +
						dumpFileRadix +
						"_Run" +
						runNumber +
						"_" +
						std::to_string(time(0)) +
						".dump",
						dumpFormat
					);
				}

			}
			catch (std::runtime_error &e)
			{
				__SUP_SS__ << "\nTransition to Running interrupted! There was an error identified " <<
					"during the configuration dump attempt:\n\n " <<
					e.what() << __E__;
				__SUP_COUT_ERR__ << "\n" << ss.str();
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
				return;
			}
			catch (...)
			{
				__SUP_SS__ << "\nTransition to Running interrupted! There was an error identified " <<
					"during the configuration dump attempt.\n\n " <<
					__E__;
				__SUP_COUT_ERR__ << "\n" << ss.str();
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
				return;
			}
		}
	}


	makeSystemLogbookEntry("Run " + runNumber + " starting.");

	broadcastMessage(theStateMachine_.getCurrentMessage());

	//save last started group name/key
	saveGroupNameAndKey(theConfigurationGroup_, FSM_LAST_STARTED_GROUP_ALIAS_FILE);
}

//========================================================================================================================
void GatewaySupervisor::transitionStopping(toolbox::Event::Reference e)

{
	__SUP_COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << __E__;

	makeSystemLogbookEntry("Run stopping.");

	broadcastMessage(theStateMachine_.getCurrentMessage());
}

////////////////////////////////////////////////////////////////////////////////////////////
//////////////      MESSAGES                 ///////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

//========================================================================================================================
//broadcastMessage
//	Broadcast state transition to all xdaq Supervisors.
//		- Transition in order of priority as given by AllSupervisorInfo
//	Update Supervisor Info based on result of transition.
bool GatewaySupervisor::broadcastMessage(xoap::MessageReference message)

{
	RunControlStateMachine::theProgressBar_.step();

	//transition of Gateway Supervisor is assumed successful so update status
	allSupervisorInfo_.setSupervisorStatus(this,
										   theStateMachine_.getCurrentStateName());

	std::string command = SOAPUtilities::translate(message).getCommand();
	bool proceed = true;
	std::string reply;

	__SUP_COUT__ << "=========> Broadcasting state machine command = " << command << __E__;

	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
	// Send a SOAP message to every Supervisor in order by priority
	for (auto& it : allSupervisorInfo_.getOrderedSupervisorDescriptors(command))
	{
		const SupervisorInfo& appInfo = *it;

		RunControlStateMachine::theProgressBar_.step();

		__SUP_COUT__ << "Sending message to Supervisor " << appInfo.getName() << " [LID=" <<
			appInfo.getId() << "]: " << command << __E__;
		__SUP_COUT__ << "Sending message to Supervisor " << appInfo.getName() << " [LID=" <<
			appInfo.getId() << "]: " << command << __E__;
		__SUP_COUT__ << "Sending message to Supervisor " << appInfo.getName() << " [LID=" <<
			appInfo.getId() << "]: " << command << __E__;
		__SUP_COUT__ << "Sending message to Supervisor " << appInfo.getName() << " [LID=" <<
			appInfo.getId() << "]: " << command << __E__;

		try
		{
			reply = send(appInfo.getDescriptor(), message);
		}
		catch (const xdaq::exception::Exception &e) //due to xoap send failure
		{
			//do not kill whole system if xdaq xoap failure
			__SUP_SS__ << "Error! Gateway Supervisor can NOT " << command << " Supervisor instance = '" <<
				appInfo.getName() << "' [LID=" <<
				appInfo.getId() << "] in Context '" <<
				appInfo.getContextName() << "' [URL=" <<
				appInfo.getURL() <<
				"].\n\n" <<
				"Xoap message failure. Did the target Supervisor crash? Try re-initializing or restarting otsdaq." << __E__;
			__SUP_COUT_ERR__ << ss.str();
			__MOUT_ERR__ << ss.str();

			try
			{
				__SUP_COUT__ << "Try again.." << __E__;
				reply = send(appInfo.getDescriptor(), message);
			}
			catch (const xdaq::exception::Exception &e) //due to xoap send failure
			{
				__SUP_COUT__ << "Failed.." << __E__;
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
			}
			__SUP_COUT__ << "2nd passed.." << __E__;
			proceed = false;
		}

		if ((reply != command + "Done") && (reply != command + "Response") )
		{
			__SUP_SS__ << "Error! Gateway Supervisor can NOT " << command << " Supervisor instance = '" <<
				appInfo.getName() << "' [LID=" <<
				appInfo.getId() << "] in Context '" <<
				appInfo.getContextName() << "' [URL=" <<
				appInfo.getURL() <<
				"].\n\n" <<
				reply;
			__SUP_COUT_ERR__ << ss.str() << __E__;
			__MOUT_ERR__ << ss.str() << __E__;

			__SUP_COUT__ << "Getting error message..." << __E__;
			try
			{
				xoap::MessageReference errorMessage = sendWithSOAPReply(appInfo.getDescriptor(),
																		SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
				SOAPParameters parameters;
				parameters.addParameter("ErrorMessage");
				SOAPMessenger::receive(errorMessage, parameters);

				std::string error = parameters.getValue("ErrorMessage");
				if (error == "")
				{
					std::stringstream err;
					err << "Unknown error from Supervisor instance = '" <<
						appInfo.getName() << "' [LID=" <<
						appInfo.getId() << "] in Context '" <<
						appInfo.getContextName() << "' [URL=" <<
						appInfo.getURL() <<
						"]. If the problem persists or is repeatable, please notify admins.\n\n";
					error = err.str();
				}

				__SUP_SS__ << "Received error from Supervisor instance = '" <<
					appInfo.getName() << "' [LID=" <<
					appInfo.getId() << "] in Context '" <<
					appInfo.getContextName() << "' [URL=" <<
					appInfo.getURL() <<
					"].\n\n Error Message = " << error << __E__;

				__SUP_COUT_ERR__ << ss.str() << __E__;
				__MOUT_ERR__ << ss.str() << __E__;

				if (command == "Error") continue; //do not throw exception and exit loop if informing all apps about error
				//else throw exception and go into Error
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());

				proceed = false;
			}
			catch (const xdaq::exception::Exception &e) //due to xoap send failure
			{
				//do not kill whole system if xdaq xoap failure
				__SUP_SS__ << "Error! Gateway Supervisor failed to read error message from Supervisor instance = '" <<
					appInfo.getName() << "' [LID=" <<
					appInfo.getId() << "] in Context '" <<
					appInfo.getContextName() << "' [URL=" <<
					appInfo.getURL() <<
					"].\n\n" <<
					"Xoap message failure. Did the target Supervisor crash? Try re-initializing or restarting otsdaq." << __E__;
				__SUP_COUT_ERR__ << ss.str();
				__MOUT_ERR__ << ss.str();
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());

				proceed = false;
			}
		}
		else
		{
			__SUP_COUT__ << "Supervisor instance = '" <<
				appInfo.getName() << "' [LID=" <<
				appInfo.getId() << "] in Context '" <<
				appInfo.getContextName() << "' [URL=" <<
				appInfo.getURL() <<
				"] was " << command << "'d correctly!" << __E__;
		}

		if (!proceed)
		{
			__SUP_COUT__ << "Breaking out of loop." << __E__;
			break;
		}
	}

	RunControlStateMachine::theProgressBar_.step();

	return proceed;
}

//========================================================================================================================
void GatewaySupervisor::wait(int milliseconds, std::string who) const
{
	for (int s = 1; s <= milliseconds; s++)
	{
		usleep(1000);

		if (s % 100 == 0)
			__SUP_COUT__ << s << " msecs " << who << __E__;
	}
}

//========================================================================================================================
//LoginRequest
//  handles all users login/logout actions from web GUI.
//  NOTE: there are two ways for a user to be logged out: timeout or manual logout
//      System logbook messages are generated for login and logout
void GatewaySupervisor::loginRequest(xgi::Input * in, xgi::Output * out)

{
	cgicc::Cgicc cgi(in);
	std::string Command = CgiDataUtilities::getData(cgi, "RequestType");
	__SUP_COUT__ << "*** Login RequestType = " << Command << __E__;

	//RequestType Commands:
	//login
	//sessionId
	//checkCookie
	//logout

	//always cleanup expired entries and get a vector std::string of logged out users
	std::vector<std::string> loggedOutUsernames;
	theWebUsers_.cleanupExpiredEntries(&loggedOutUsernames);
	for (unsigned int i = 0; i < loggedOutUsernames.size(); ++i) //Log logout for logged out users
		makeSystemLogbookEntry(loggedOutUsernames[i] + " login timed out.");

	if (Command == "sessionId")
	{
		//	When client loads page, client submits unique user id and receives random sessionId from server
		//	Whenever client submits user name and password it is jumbled by sessionId when sent to server and sent along with UUID. Server uses sessionId to unjumble.
		//
		//	Server maintains list of active sessionId by UUID
		//	sessionId expires after set time if no login attempt (e.g. 5 minutes)
		std::string uuid = CgiDataUtilities::postData(cgi, "uuid");

		std::string sid = theWebUsers_.createNewLoginSession(uuid,
				cgi.getEnvironment().getRemoteAddr() /* ip */);

//		__SUP_COUT__ << "uuid = " << uuid << __E__;
//		__SUP_COUT__ << "SessionId = " << sid.substr(0, 10) << __E__;
		*out << sid;
	}
	else if (Command == "checkCookie")
	{
		uint64_t uid;
		std::string uuid;
		std::string jumbledUser;
		std::string cookieCode;

		//	If client has a cookie, client submits cookie and username, jumbled, to see if cookie and user are still active
		//	if active, valid cookie code is returned and name to display, in XML
		// 	if not, return 0
		// 	params:
		//		uuid 			- unique user id, to look up sessionId
		//		ju 				- jumbled user name
		//		CookieCode 		- cookie code to check

		uuid = CgiDataUtilities::postData(cgi, "uuid");
		jumbledUser = CgiDataUtilities::postData(cgi, "ju");
		cookieCode = CgiDataUtilities::postData(cgi, "cc");

//		__SUP_COUT__ << "uuid = " << uuid << __E__;
//		__SUP_COUT__ << "Cookie Code = " << cookieCode.substr(0, 10) << __E__;
//		__SUP_COUT__ << "jumbledUser = " << jumbledUser.substr(0, 10) << __E__;

		//If cookie code is good, then refresh and return with display name, else return 0 as CookieCode value
		uid = theWebUsers_.isCookieCodeActiveForLogin(uuid, cookieCode,
													  jumbledUser); //after call jumbledUser holds displayName on success

		if (uid == theWebUsers_.NOT_FOUND_IN_DATABASE)
		{
			__SUP_COUT__ << "cookieCode invalid" << __E__;
			jumbledUser = ""; //clear display name if failure
			cookieCode = "0";//clear cookie code if failure
		}
		else
			__SUP_COUT__ << "cookieCode is good." << __E__;

		//return xml holding cookie code and display name
		HttpXmlDocument xmldoc(cookieCode, jumbledUser);

		theWebUsers_.insertSettingsForUser(uid, &xmldoc); //insert settings

		xmldoc.outputXmlDocument((std::ostringstream*) out);

	}
	else if (Command == "login")
	{
		//	If login attempt or create account, jumbled user and pw are submitted
		//	if successful, valid cookie code and display name returned.
		// 	if not, return 0
		// 	params:
		//		uuid 			- unique user id, to look up sessionId
		//		nac				- new account code for first time logins
		//		ju 				- jumbled user name
		//		jp		 		- jumbled password

		std::string uuid = CgiDataUtilities::postData(cgi, "uuid");
		std::string newAccountCode = CgiDataUtilities::postData(cgi, "nac");
		std::string jumbledUser = CgiDataUtilities::postData(cgi, "ju");
		std::string jumbledPw = CgiDataUtilities::postData(cgi, "jp");

//		__SUP_COUT__ << "jumbledUser = " << jumbledUser.substr(0, 10) << __E__;
//		__SUP_COUT__ << "jumbledPw = " << jumbledPw.substr(0, 10) << __E__;
//		__SUP_COUT__ << "uuid = " << uuid << __E__;
//		__SUP_COUT__ << "nac =-" << newAccountCode << "-" << __E__;

		uint64_t uid = theWebUsers_.attemptActiveSession(uuid, jumbledUser,
				jumbledPw, newAccountCode, cgi.getEnvironment().getRemoteAddr()); //after call jumbledUser holds displayName on success


		if (uid == theWebUsers_.NOT_FOUND_IN_DATABASE)
		{
			__SUP_COUT__ << "Login invalid." << __E__;
			jumbledUser = ""; //clear display name if failure
			if (newAccountCode != "1")//indicates uuid not found
				newAccountCode = "0";//clear cookie code if failure
		}
		else	//Log login in logbook for active experiment
			makeSystemLogbookEntry(
				theWebUsers_.getUsersUsername(uid) + " logged in.");

		//__SUP_COUT__ << "new cookieCode = " << newAccountCode.substr(0, 10) << __E__;

		HttpXmlDocument xmldoc(newAccountCode, jumbledUser);

		theWebUsers_.insertSettingsForUser(uid, &xmldoc); //insert settings

		//insert active session count for user

		if (uid != theWebUsers_.NOT_FOUND_IN_DATABASE)
		{
			uint64_t asCnt = theWebUsers_.getActiveSessionCountForUser(uid) - 1; //subtract 1 to remove just started session from count
			char asStr[20];
			sprintf(asStr, "%lu", asCnt);
			xmldoc.addTextElementToData("user_active_session_count", asStr);
		}

		xmldoc.outputXmlDocument((std::ostringstream*) out);

	}
	else if (Command == "cert")
	{
		//	If login attempt or create account, jumbled user and pw are submitted
		//	if successful, valid cookie code and display name returned.
		// 	if not, return 0
		// 	params:
		//		uuid 			- unique user id, to look up sessionId
		//		nac				- new account code for first time logins
		//		ju 				- jumbled user name
		//		jp		 		- jumbled password

		std::string uuid = CgiDataUtilities::postData(cgi, "uuid");
		std::string jumbledEmail = cgicc::form_urldecode(CgiDataUtilities::getData(cgi, "httpsUser"));
		std::string username = "";
		std::string cookieCode = "";

//		__SUP_COUT__ << "CERTIFICATE LOGIN REUEST RECEVIED!!!" << __E__;
//		__SUP_COUT__ << "jumbledEmail = " << jumbledEmail << __E__;
//		__SUP_COUT__ << "uuid = " << uuid << __E__;

		uint64_t uid = theWebUsers_.attemptActiveSessionWithCert(uuid, jumbledEmail,
				cookieCode, username, cgi.getEnvironment().getRemoteAddr()); //after call jumbledUser holds displayName on success


		if (uid == theWebUsers_.NOT_FOUND_IN_DATABASE)
		{
			__SUP_COUT__ << "cookieCode invalid" << __E__;
			jumbledEmail = ""; //clear display name if failure
			if (cookieCode != "1")//indicates uuid not found
				cookieCode = "0";//clear cookie code if failure
		}
		else //Log login in logbook for active experiment
			makeSystemLogbookEntry(
				theWebUsers_.getUsersUsername(uid) + " logged in.");

		//__SUP_COUT__ << "new cookieCode = " << cookieCode.substr(0, 10) << __E__;

		HttpXmlDocument xmldoc(cookieCode, jumbledEmail);

		theWebUsers_.insertSettingsForUser(uid, &xmldoc); //insert settings

		//insert active session count for user

		if (uid != theWebUsers_.NOT_FOUND_IN_DATABASE)
		{
			uint64_t asCnt = theWebUsers_.getActiveSessionCountForUser(uid) - 1; //subtract 1 to remove just started session from count
			char asStr[20];
			sprintf(asStr, "%lu", asCnt);
			xmldoc.addTextElementToData("user_active_session_count", asStr);
		}

		xmldoc.outputXmlDocument((std::ostringstream*) out);
	}
	else if (Command == "logout")
	{
		std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
		std::string logoutOthers = CgiDataUtilities::postData(cgi,
															  "LogoutOthers");

//		__SUP_COUT__ << "Cookie Code = " << cookieCode.substr(0, 10) << __E__;
//		__SUP_COUT__ << "logoutOthers = " << logoutOthers << __E__;

		uint64_t uid; //get uid for possible system logbook message
		if (theWebUsers_.cookieCodeLogout(cookieCode, logoutOthers == "1",
				&uid,  cgi.getEnvironment().getRemoteAddr())
			!= theWebUsers_.NOT_FOUND_IN_DATABASE) //user logout
		{
			//if did some logging out, check if completely logged out
			//if so, system logbook message should be made.
			if (!theWebUsers_.isUserIdActive(uid))
				makeSystemLogbookEntry(
					theWebUsers_.getUsersUsername(uid) + " logged out.");
		}
	}
	else
	{
		__SUP_COUT__ << __LINE__ << "\tInvalid Command" << __E__;
		*out << "0";
	}
}

//========================================================================================================================
void GatewaySupervisor::tooltipRequest(xgi::Input * in, xgi::Output * out)

{
	cgicc::Cgicc cgi(in);

	std::string Command = CgiDataUtilities::getData(cgi, "RequestType");
	__SUP_COUT__ << "Tooltip RequestType = " << Command << __E__;

	//**** start LOGIN GATEWAY CODE ***//
	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions, uint64_t uid
	//Else, error message is returned in cookieCode
	//Notes: cookie code not refreshed if RequestType = getSystemMessages
	std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
	uint64_t uid;

	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode,
			0 /*userPermissions*/,
			&uid,
			"0" /*dummy ip*/,
			false /*refresh*/))
	{
		*out << cookieCode;
		return;
	}

	//**** end LOGIN GATEWAY CODE ***//

	HttpXmlDocument xmldoc(cookieCode);

	if (Command == "check")
	{
		WebUsers::tooltipCheckForUsername(
			theWebUsers_.getUsersUsername(uid),
			&xmldoc,
			CgiDataUtilities::getData(cgi, "srcFile"),
			CgiDataUtilities::getData(cgi, "srcFunc"),
			CgiDataUtilities::getData(cgi, "srcId"));
	}
	else if (Command == "setNeverShow")
	{
		WebUsers::tooltipSetNeverShowForUsername(
			theWebUsers_.getUsersUsername(uid),
			&xmldoc,
			CgiDataUtilities::getData(cgi, "srcFile"),
			CgiDataUtilities::getData(cgi, "srcFunc"),
			CgiDataUtilities::getData(cgi, "srcId"),
			CgiDataUtilities::getData(cgi, "doNeverShow") == "1" ? true : false,
			CgiDataUtilities::getData(cgi, "temporarySilence") == "1" ? true : false);

	}
	else
		__SUP_COUT__ << "Command Request, " << Command << ", not recognized." << __E__;

	xmldoc.outputXmlDocument((std::ostringstream*) out, false, true);
}

//========================================================================================================================
//setSupervisorPropertyDefaults
//		override to set defaults for supervisor property values (before user settings override)
void GatewaySupervisor::setSupervisorPropertyDefaults()
{
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.UserPermissionsThreshold, std::string() +
			"*=1 | gatewayLaunchOTS=-1 | gatewayLaunchWiz=-1 | codeEditor=-1");
}

//========================================================================================================================
//forceSupervisorPropertyValues
//		override to force supervisor property values (and ignore user settings)
void GatewaySupervisor::forceSupervisorPropertyValues()
{
	//note used by these handlers:
	//	request()
	//	stateMachineXgiHandler() -- prepend StateMachine to request type

	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.AutomatedRequestTypes,
			"getSystemMessages | getCurrentState | gatewayLaunchOTS | gatewayLaunchWiz");
	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.RequireUserLockRequestTypes,
			"gatewayLaunchOTS | gatewayLaunchWiz | codeEditor");
//	CorePropertySupervisorBase::setSupervisorProperty(CorePropertySupervisorBase::SUPERVISOR_PROPERTIES.NeedUsernameRequestTypes,
//			"StateMachine*"); //for all stateMachineXgiHandler requests

}

//========================================================================================================================
void GatewaySupervisor::request(xgi::Input * in, xgi::Output * out)

{
	//for simplicity assume all commands should be mutually exclusive with iterator thread state machine accesses (really should just be careful with RunControlStateMachine access)
	if (VERBOSE_MUTEX) __SUP_COUT__ << "Waiting for FSM access" << __E__;
	std::lock_guard<std::mutex> lock(stateMachineAccessMutex_);
	if (VERBOSE_MUTEX) __SUP_COUT__ << "Have FSM access" << __E__;

	cgicc::Cgicc cgiIn(in);

	std::string requestType = CgiDataUtilities::getData(cgiIn, "RequestType");


	HttpXmlDocument xmlOut;
	WebUsers::RequestUserInfo userInfo(requestType,
			CgiDataUtilities::postData(cgiIn, "CookieCode"));

	CorePropertySupervisorBase::getRequestUserInfo(userInfo);


	if(!theWebUsers_.xmlRequestOnGateway(
			cgiIn,
			out,
			&xmlOut,
			userInfo
			))
		return; //access failed



//	//**** start LOGIN GATEWAY CODE ***//
//	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions, uint64_t userInfo.uid_
//	//Else, error message is returned in cookieCode
//	//Notes: cookie code not refreshed if RequestType = getSystemMessages
//	std::string cookieCode = CgiDataUtilities::postData(cgiIn, "CookieCode");
//	uint8_t userPermissions;
//	uint64_t userInfo.uid_;
//	std::string userWithLock;
//	bool refreshCookie = requestType != "getSystemMessages" &&
//			requestType != "getCurrentState" &&
//			requestType != "gatewayLaunchOTS" &&
//			requestType != "gatewayLaunchWiz";
//
//	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode, &userPermissions,
//			&userInfo.uid_, "0",
//			refreshCookie,
//			&userWithLock))
//	{
//		*out << cookieCode;
//		return;
//	}

	//**** end LOGIN GATEWAY CODE ***//

	//RequestType Commands:
	//getSettings
	//setSettings
	//accountSettings
	//getAliasList
	//getFecList
	//getSystemMessages
	//setUserWithLock
	//getStateMachine
	//stateMatchinePreferences
	//getStateMachineNames
	//getCurrentState
	//getIterationPlanStatus
	//getErrorInStateMatchine
	//getDesktopIcons

	//resetUserTooltips

	//gatewayLaunchOTS
	//gatewayLaunchWiz

	//codeEditor


	//HttpXmlDocument xmlOut(cookieCode);

	if (requestType == "getSettings")
	{
		std::string accounts = CgiDataUtilities::getData(cgiIn, "accounts");

		__SUP_COUT__ << "Get Settings Request" << __E__;
		__SUP_COUT__ << "accounts = " << accounts << __E__;
		theWebUsers_.insertSettingsForUser(userInfo.uid_, &xmlOut, accounts == "1");
	}
	else if (requestType == "setSettings")
	{
		std::string bgcolor = 	CgiDataUtilities::postData(cgiIn, "bgcolor");
		std::string dbcolor = 	CgiDataUtilities::postData(cgiIn, "dbcolor");
		std::string wincolor = 	CgiDataUtilities::postData(cgiIn, "wincolor");
		std::string layout = 	CgiDataUtilities::postData(cgiIn, "layout");
		std::string syslayout = CgiDataUtilities::postData(cgiIn, "syslayout");

		__SUP_COUT__ << "Set Settings Request" << __E__;
		__SUP_COUT__ << "bgcolor = " << bgcolor << __E__;
		__SUP_COUT__ << "dbcolor = " << dbcolor << __E__;
		__SUP_COUT__ << "wincolor = " << wincolor << __E__;
		__SUP_COUT__ << "layout = " << layout << __E__;
		__SUP_COUT__ << "syslayout = " << syslayout << __E__;

		theWebUsers_.changeSettingsForUser(userInfo.uid_, bgcolor, dbcolor, wincolor,
				layout, syslayout);
		theWebUsers_.insertSettingsForUser(userInfo.uid_, &xmlOut, true); //include user accounts
	}
	else if (requestType == "accountSettings")
	{
		std::string type = CgiDataUtilities::postData(cgiIn, "type"); //updateAccount, createAccount, deleteAccount
		int type_int = -1;

		if (type == "updateAccount")
			type_int = 0;
		else if (type == "createAccount")
			type_int = 1;
		else if (type == "deleteAccount")
			type_int = 2;

		std::string username = CgiDataUtilities::postData(cgiIn, "username");
		std::string displayname = CgiDataUtilities::postData(cgiIn,
				"displayname");
		std::string email = CgiDataUtilities::postData(cgiIn, "useremail");
		std::string permissions = CgiDataUtilities::postData(cgiIn,
				"permissions");
		std::string accounts = CgiDataUtilities::getData(cgiIn, "accounts");

		__SUP_COUT__ << "accountSettings Request" << __E__;
		__SUP_COUT__ << "type = " << type << " - " << type_int << __E__;
		__SUP_COUT__ << "username = " << username << __E__;
		__SUP_COUT__ << "useremail = " << email << __E__;
		__SUP_COUT__ << "displayname = " << displayname << __E__;
		__SUP_COUT__ << "permissions = " << permissions << __E__;

		theWebUsers_.modifyAccountSettings(userInfo.uid_, type_int, username, displayname,email,
				permissions);

		__SUP_COUT__ << "accounts = " << accounts << __E__;

		theWebUsers_.insertSettingsForUser(userInfo.uid_, &xmlOut, accounts == "1");
	}
	else if(requestType == "stateMatchinePreferences")
	{
		std::string set = CgiDataUtilities::getData(cgiIn, "set");
		const std::string DEFAULT_FSM_VIEW = "Default_FSM_View";
		if(set == "1")
			theWebUsers_.setGenericPreference(userInfo.uid_, DEFAULT_FSM_VIEW,
					CgiDataUtilities::getData(cgiIn, DEFAULT_FSM_VIEW));
		else
			theWebUsers_.getGenericPreference(userInfo.uid_, DEFAULT_FSM_VIEW, &xmlOut);
	}
	else if(requestType == "getAliasList")
	{
		std::string username = theWebUsers_.getUsersUsername(userInfo.uid_);
		std::string fsmName = CgiDataUtilities::getData(cgiIn, "fsmName");
		__SUP_COUT__ << "fsmName = " << fsmName << __E__;

		std::string stateMachineAliasFilter = "*"; //default to all

		std::map<std::string /*alias*/,
			std::pair<std::string /*group name*/, ConfigurationGroupKey> > aliasMap =
			CorePropertySupervisorBase::theConfigurationManager_->getActiveGroupAliases();


		// get stateMachineAliasFilter if possible
		ConfigurationTree configLinkNode = CorePropertySupervisorBase::theConfigurationManager_->getSupervisorConfigurationNode(
			supervisorContextUID_, supervisorApplicationUID_);

		if (!configLinkNode.isDisconnected())
		{
			try //for backwards compatibility
			{
				ConfigurationTree fsmLinkNode = configLinkNode.getNode("LinkToStateMachineConfiguration");
				if (!fsmLinkNode.isDisconnected())
					stateMachineAliasFilter =
					fsmLinkNode.getNode(fsmName + "/SystemAliasFilter").getValue<std::string>();
				else
					__SUP_COUT_INFO__ << "FSM Link disconnected." << __E__;
			}
			catch (std::runtime_error &e) { __SUP_COUT_INFO__ << e.what() << __E__; }
			catch (...) { __SUP_COUT_ERR__ << "Unknown error. Should never happen." << __E__; }
		}
		else
			__SUP_COUT_INFO__ << "FSM Link disconnected." << __E__;

		__SUP_COUT__ << "stateMachineAliasFilter  = " << stateMachineAliasFilter << __E__;


		//filter list of aliases based on stateMachineAliasFilter
		//  ! as first character means choose those that do NOT match filter
		//	* can be used as wild card.
		{
			bool invertFilter = stateMachineAliasFilter.size() && stateMachineAliasFilter[0] == '!';
			std::vector<std::string> filterArr;

			size_t  i = 0;
			if (invertFilter) ++i;
			size_t  f;
			std::string tmp;
			while ((f = stateMachineAliasFilter.find('*', i)) != std::string::npos)
			{
				tmp = stateMachineAliasFilter.substr(i, f - i);
				i = f + 1;
				filterArr.push_back(tmp);
				//__SUP_COUT__ << filterArr[filterArr.size()-1] << " " << i <<
				//		" of " << stateMachineAliasFilter.size() << __E__;

			}
			if (i <= stateMachineAliasFilter.size())
			{
				tmp = stateMachineAliasFilter.substr(i);
				filterArr.push_back(tmp);
				//__SUP_COUT__ << filterArr[filterArr.size()-1] << " last." << __E__;
			}


			bool filterMatch;


			for (auto& aliasMapPair : aliasMap)
			{
				//__SUP_COUT__ << "aliasMapPair.first: " << aliasMapPair.first << __E__;

				filterMatch = true;

				if (filterArr.size() == 1)
				{
					if (filterArr[0] != "" &&
						filterArr[0] != "*" &&
						aliasMapPair.first != filterArr[0])
						filterMatch = false;
				}
				else
				{
					i = -1;
					for (f = 0; f < filterArr.size(); ++f)
					{
						if (!filterArr[f].size()) continue; //skip empty filters

						if (f == 0) //must start with this filter
						{
							if ((i = aliasMapPair.first.find(filterArr[f])) != 0)
							{
								filterMatch = false;
								break;
							}
						}
						else if (f == filterArr.size() - 1) //must end with this filter
						{
							if (aliasMapPair.first.rfind(filterArr[f]) !=
								aliasMapPair.first.size() - filterArr[f].size())
							{
								filterMatch = false;
								break;
							}
						}
						else if ((i = aliasMapPair.first.find(filterArr[f])) ==
								 std::string::npos)
						{
							filterMatch = false;
							break;
						}
					}
				}

				if (invertFilter) filterMatch = !filterMatch;

				//__SUP_COUT__ << "filterMatch=" << filterMatch  << __E__;

				if (!filterMatch) continue;

				xmlOut.addTextElementToData("config_alias", aliasMapPair.first);
				xmlOut.addTextElementToData("config_key",
						ConfigurationGroupKey::getFullGroupString(aliasMapPair.second.first,
								aliasMapPair.second.second).c_str());

				std::string groupComment, groupAuthor, groupCreationTime;
				try
				{
					CorePropertySupervisorBase::theConfigurationManager_->loadConfigurationGroup(
						aliasMapPair.second.first, aliasMapPair.second.second,
						false, 0, 0, 0,
						&groupComment, &groupAuthor, &groupCreationTime,
						false /*false to not load member map*/);

					xmlOut.addTextElementToData("config_comment", groupComment);
					xmlOut.addTextElementToData("config_author", groupAuthor);
					xmlOut.addTextElementToData("config_create_time", groupCreationTime);
				}
				catch (...)
				{
					__SUP_COUT_WARN__ << "Failed to load group metadata." << __E__;
				}
			}
		}

		//return last group alias
		std::string fn = FSM_LAST_GROUP_ALIAS_PATH + FSM_LAST_GROUP_ALIAS_FILE_START +
			username + "." + FSM_USERS_PREFERENCES_FILETYPE;
		__SUP_COUT__ << "Load preferences: " << fn << __E__;
		FILE *fp = fopen(fn.c_str(), "r");
		if (fp)
		{
			char tmpLastAlias[500];
			fscanf(fp, "%*s %s", tmpLastAlias);
			__SUP_COUT__ << "tmpLastAlias: " << tmpLastAlias << __E__;

			xmlOut.addTextElementToData("UserLastConfigAlias",tmpLastAlias);
			fclose(fp);
		}
	}
	else if (requestType == "getFecList")
	{
		xmlOut.addTextElementToData("fec_list", "");

		for (auto it : allSupervisorInfo_.getAllFETypeSupervisorInfo())
		{
			xmlOut.addTextElementToParent("fec_url",
					it.second.getURL(), "fec_list");
			xmlOut.addTextElementToParent(
					"fec_urn",
					std::to_string(it.second.getId()),	"fec_list");
		}
	}
	else if (requestType == "getSystemMessages")
	{
		xmlOut.addTextElementToData("systemMessages",
				theSystemMessenger_.getSystemMessage(
						theWebUsers_.getUsersDisplayName(userInfo.uid_)));

		xmlOut.addTextElementToData("username_with_lock",
				theWebUsers_.getUserWithLock()); //always give system lock update

							//__SUP_COUT__ << "userWithLock " << theWebUsers_.getUserWithLock() << __E__;
	}
	else if (requestType == "setUserWithLock")
	{
		std::string username = CgiDataUtilities::postData(cgiIn, "username");
		std::string lock = CgiDataUtilities::postData(cgiIn, "lock");
		std::string accounts = CgiDataUtilities::getData(cgiIn, "accounts");

		__SUP_COUT__ << requestType <<  __E__;
		__SUP_COUT__ << "username " << username <<  __E__;
		__SUP_COUT__ << "lock " << lock <<  __E__;
		__SUP_COUT__ << "accounts " << accounts <<  __E__;
		__SUP_COUT__ << "userInfo.uid_ " << userInfo.uid_ <<  __E__;

		std::string tmpUserWithLock = theWebUsers_.getUserWithLock();
		if(!theWebUsers_.setUserWithLock(userInfo.uid_, lock == "1", username))
			xmlOut.addTextElementToData("server_alert",
					std::string("Set user lock action failed. You must have valid permissions and ") +
					"locking user must be currently logged in.");

		theWebUsers_.insertSettingsForUser(userInfo.uid_, &xmlOut, accounts == "1");

		if (tmpUserWithLock != theWebUsers_.getUserWithLock()) //if there was a change, broadcast system message
			theSystemMessenger_.addSystemMessage("*", theWebUsers_.getUserWithLock()
												 == "" ? tmpUserWithLock + " has unlocked ots."
												 : theWebUsers_.getUserWithLock()
												 + " has locked ots.");
	}
	else if (requestType == "getStateMachine")
	{
		// __SUP_COUT__ << "Getting state machine" << __E__;
		std::vector<toolbox::fsm::State> states;
		states = theStateMachine_.getStates();
		char stateStr[2];
		stateStr[1] = '\0';
		std::string transName;
		std::string transParameter;

		//bool addRun, addCfg;
		for (unsigned int i = 0; i < states.size(); ++i)//get all states
		{
			stateStr[0] = states[i];
			DOMElement* stateParent = xmlOut.addTextElementToData("state", stateStr);

			xmlOut.addTextElementToParent("state_name", theStateMachine_.getStateName(states[i]), stateParent);

			//__SUP_COUT__ << "state: " << states[i] << " - " << theStateMachine_.getStateName(states[i]) << __E__;

			//get all transition final states, transitionNames and actionNames from state
			std::map<std::string, toolbox::fsm::State, std::less<std::string> >
				trans = theStateMachine_.getTransitions(states[i]);
			std::set<std::string> actionNames = theStateMachine_.getInputs(states[i]);

			std::map<std::string, toolbox::fsm::State, std::less<std::string> >::iterator it =
				trans.begin();
			std::set<std::string>::iterator ait = actionNames.begin();

			//			addRun = false;
			//			addCfg = false;

			//handle hacky way to keep "forward" moving states on right of FSM display
			//must be first!

			for (; it != trans.end() && ait != actionNames.end(); ++it, ++ait)
			{
				stateStr[0] = it->second;

				if (stateStr[0] == 'R')
				{
					//addRun = true;
					xmlOut.addTextElementToParent("state_transition", stateStr, stateParent);

					//__SUP_COUT__ << states[i] << " => " << *ait << __E__;

					xmlOut.addTextElementToParent("state_transition_action", *ait, stateParent);

					transName = theStateMachine_.getTransitionName(states[i], *ait);
					//__SUP_COUT__ << states[i] << " => " << transName << __E__;

					xmlOut.addTextElementToParent("state_transition_name",
							transName, stateParent);
					transParameter = theStateMachine_.getTransitionParameter(states[i], *ait);
					//__SUP_COUT__ << states[i] << " => " << transParameter<< __E__;

					xmlOut.addTextElementToParent("state_transition_parameter", transParameter, stateParent);
					break;
				}
				else if (stateStr[0] == 'C')
				{
					//addCfg = true;
					xmlOut.addTextElementToParent("state_transition", stateStr, stateParent);

					//__SUP_COUT__ << states[i] << " => " << *ait << __E__;

					xmlOut.addTextElementToParent("state_transition_action", *ait, stateParent);

					transName = theStateMachine_.getTransitionName(states[i], *ait);
					//__SUP_COUT__ << states[i] << " => " << transName << __E__;

					xmlOut.addTextElementToParent("state_transition_name",
							transName, stateParent);
					transParameter = theStateMachine_.getTransitionParameter(states[i], *ait);
					//__SUP_COUT__ << states[i] << " => " << transParameter<< __E__;

					xmlOut.addTextElementToParent("state_transition_parameter", transParameter, stateParent);
					break;
				}
			}

			//reset for 2nd pass
			it = trans.begin();
			ait = actionNames.begin();

			//other states
			for (; it != trans.end() && ait != actionNames.end(); ++it, ++ait)
			{
				//__SUP_COUT__ << states[i] << " => " << it->second << __E__;

				stateStr[0] = it->second;

				if (stateStr[0] == 'R')
					continue;
				else if (stateStr[0] == 'C')
					continue;

				xmlOut.addTextElementToParent("state_transition", stateStr, stateParent);

				//__SUP_COUT__ << states[i] << " => " << *ait << __E__;

				xmlOut.addTextElementToParent("state_transition_action", *ait, stateParent);

				transName = theStateMachine_.getTransitionName(states[i], *ait);
				//__SUP_COUT__ << states[i] << " => " << transName << __E__;

				xmlOut.addTextElementToParent("state_transition_name",
						transName, stateParent);
				transParameter = theStateMachine_.getTransitionParameter(states[i], *ait);
				//__SUP_COUT__ << states[i] << " => " << transParameter<< __E__;

				xmlOut.addTextElementToParent("state_transition_parameter", transParameter, stateParent);
			}
		}

	}
	else if (requestType == "getStateMachineNames")
	{
		//get stateMachineAliasFilter if possible
		ConfigurationTree configLinkNode = CorePropertySupervisorBase::theConfigurationManager_->getSupervisorConfigurationNode(
			supervisorContextUID_, supervisorApplicationUID_);

		try
		{
			auto fsmNodes = configLinkNode.getNode(
					"LinkToStateMachineConfiguration").getChildren();
			for(const auto& fsmNode:fsmNodes)
				xmlOut.addTextElementToData("stateMachineName", fsmNode.first);
		}
		catch (...) //else empty set of state machines.. can always choose ""
		{
			__SUP_COUT__ << "Caught exception, assuming no valid FSM names." << __E__;
			xmlOut.addTextElementToData("stateMachineName", "");
		}
	}
	else if (requestType == "getIterationPlanStatus")
	{
		//__SUP_COUT__ << "checking it status" << __E__;
		theIterator_.handleCommandRequest(xmlOut,requestType,"");
	}
	else if (requestType == "getCurrentState")
	{
		xmlOut.addTextElementToData("current_state", theStateMachine_.getCurrentStateName());
		xmlOut.addTextElementToData("in_transition", theStateMachine_.isInTransition() ? "1" : "0");
		if (theStateMachine_.isInTransition())
			xmlOut.addTextElementToData("transition_progress", RunControlStateMachine::theProgressBar_.readPercentageString());
		else
			xmlOut.addTextElementToData("transition_progress", "100");


		char tmp[20];
		sprintf(tmp,"%lu",theStateMachine_.getTimeInState());
		xmlOut.addTextElementToData("time_in_state", tmp);



		//__SUP_COUT__ << "current state: " << theStateMachine_.getCurrentStateName() << __E__;


		//// ======================== get run alias based on fsm name ====

		std::string fsmName = CgiDataUtilities::getData(cgiIn, "fsmName");
		//		__SUP_COUT__ << "fsmName = " << fsmName << __E__;
		//		__SUP_COUT__ << "activeStateMachineName_ = " << activeStateMachineName_ << __E__;
		//		__SUP_COUT__ << "theStateMachine_.getProvenanceStateName() = " <<
		//				theStateMachine_.getProvenanceStateName() << __E__;
		//		__SUP_COUT__ << "theStateMachine_.getCurrentStateName() = " <<
		//				theStateMachine_.getCurrentStateName() << __E__;

		if (!theStateMachine_.isInTransition())
		{
			std::string stateMachineRunAlias = "Run"; //default to "Run"

			// get stateMachineAliasFilter if possible
			ConfigurationTree configLinkNode = CorePropertySupervisorBase::theConfigurationManager_->getSupervisorConfigurationNode(
				supervisorContextUID_, supervisorApplicationUID_);

			if (!configLinkNode.isDisconnected())
			{
				try //for backwards compatibility
				{
					ConfigurationTree fsmLinkNode = configLinkNode.getNode("LinkToStateMachineConfiguration");
					if (!fsmLinkNode.isDisconnected())
						stateMachineRunAlias =
						fsmLinkNode.getNode(fsmName + "/RunDisplayAlias").getValue<std::string>();
					//else
					//	__SUP_COUT_INFO__ << "FSM Link disconnected." << __E__;
				}
				catch (std::runtime_error &e)
				{
					//__SUP_COUT_INFO__ << e.what() << __E__;
					//__SUP_COUT_INFO__ << "No state machine Run alias. Ignoring and assuming alias of '" <<
					//		stateMachineRunAlias << ".'" << __E__;

				}
				catch (...)
				{
					__SUP_COUT_ERR__ << "Unknown error. Should never happen." << __E__;

					__SUP_COUT_INFO__ << "No state machine Run alias. Ignoring and assuming alias of '" <<
						stateMachineRunAlias << ".'" << __E__;
				}
			}
			//else
			//	__SUP_COUT_INFO__ << "FSM Link disconnected." << __E__;

			//__SUP_COUT__ << "stateMachineRunAlias  = " << stateMachineRunAlias	<< __E__;

			xmlOut.addTextElementToData("stateMachineRunAlias", stateMachineRunAlias);


			//// ======================== get run number based on fsm name ====


			if (theStateMachine_.getCurrentStateName() == "Running" ||
				theStateMachine_.getCurrentStateName() == "Paused")
				sprintf(tmp, "Current %s Number: %u", stateMachineRunAlias.c_str(), getNextRunNumber(activeStateMachineName_) - 1);
			else
				sprintf(tmp,"Next %s Number: %u",stateMachineRunAlias.c_str(),getNextRunNumber(fsmName));
			xmlOut.addTextElementToData("run_number", tmp);
		}
	}
	else if(requestType == "getErrorInStateMatchine")
	{
		xmlOut.addTextElementToData("FSM_Error", theStateMachine_.getErrorMessage());
	}
	else if(requestType == "getDesktopIcons")
	{

		//get icons and create comma-separated string based on user permissions
		//	note: each icon has own permission threshold, so each user can have
		//		a unique desktop icon experience.

		const DesktopIconConfiguration* iconConfiguration =
				CorePropertySupervisorBase::theConfigurationManager_->__GET_CONFIG__(DesktopIconConfiguration);
		std::vector<DesktopIconConfiguration::DesktopIcon> icons =
				iconConfiguration->getAllDesktopIcons();

		std::string iconString = ""; //comma-separated icon string, 7 fields:
		//				0 - caption 		= text below icon
		//				1 - altText 		= text icon if no image given
		//				2 - uniqueWin 		= if true, only one window is allowed, else multiple instances of window
		//				3 - permissions 	= security level needed to see icon
		//				4 - picfn 			= icon image filename
		//				5 - linkurl 		= url of the window to open
		//				6 - folderPath 		= folder and subfolder location '/' separated
		//for example:
		//State Machine,FSM,1,200,icon-Physics.gif,/WebPath/html/StateMachine.html?fsm_name=OtherRuns0,,Chat,CHAT,1,1,icon-Chat.png,/urn:xdaq-application:lid=250,,Visualizer,VIS,0,10,icon-Visualizer.png,/WebPath/html/Visualization.html?urn=270,,Configure,CFG,0,10,icon-Configure.png,/urn:xdaq-application:lid=281,,Front-ends,CFG,0,15,icon-Configure.png,/WebPath/html/ConfigurationGUI_subset.html?urn=281&subsetBasePath=FEInterfaceConfiguration&groupingFieldList=Status%2CFEInterfacePluginName&recordAlias=Front%2Dends&editableFieldList=%21%2ACommentDescription%2C%21SlowControls%2A,Config Subsets


		//__COUTV__((unsigned int)userInfo.permissionLevel_);

		std::map<std::string,WebUsers::permissionLevel_t> userPermissionLevelsMap =
				theWebUsers_.getPermissionsForUser(userInfo.uid_);
		std::map<std::string,WebUsers::permissionLevel_t> iconPermissionThresholdsMap;


		bool firstIcon = true;
		for(const auto& icon: icons)
		{
			__COUTV__(icon.caption_);
			__COUTV__(icon.permissionThresholdString_);

			CorePropertySupervisorBase::extractPermissionsMapFromString(
					icon.permissionThresholdString_,
					iconPermissionThresholdsMap);

			if(!CorePropertySupervisorBase::doPermissionsGrantAccess(
					userPermissionLevelsMap,
					iconPermissionThresholdsMap)) continue; //skip icon if no access

			//__COUTV__(icon.caption_);

			//have icon access, so add to CSV string
			if(firstIcon) firstIcon = false;
			else iconString += ",";

			iconString += icon.caption_;
			iconString += "," + icon.alternateText_;
			iconString += "," + std::string(icon.enforceOneWindowInstance_?"1":"0");
			iconString += "," + std::string("1"); //set permission to 1 so the desktop shows every icon that the server allows (i.e., trust server security, ignore client security)
			iconString += "," + icon.imageURL_;
			iconString += "," + icon.windowContentURL_;
			iconString += "," + icon.folderPath_;
		}
		__COUTV__(iconString);

		xmlOut.addTextElementToData("iconList", iconString);
	}
	else if(requestType == "gatewayLaunchOTS" || requestType == "gatewayLaunchWiz")
	{
		//NOTE: similar to ConfigurationGUI version but DOES keep active sessions

		__SUP_COUT_WARN__ << requestType << " requestType received! " << __E__;
		__MOUT_WARN__ << requestType << " requestType received! " << __E__;


		//gateway launch is different, in that it saves user sessions
		theWebUsers_.saveActiveSessions();

		//now launch

		if(requestType == "gatewayLaunchOTS")
			launchStartOTSCommand("LAUNCH_OTS");
		else if(requestType == "gatewayLaunchWiz")
			launchStartOTSCommand("LAUNCH_WIZ");

		//			__SUP_COUT__ << "Extracting target context hostnames... " << __E__;
		//			std::vector<std::string> hostnames;
		//			try
		//			{
		//				CorePropertySupervisorBase::theConfigurationManager_->init(); //completely reset to re-align with any changes
		//
		//				const XDAQContextConfiguration* contextConfiguration = CorePropertySupervisorBase::theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration);
		//
		//				auto contexts = contextConfiguration->getContexts();
		//				unsigned int i,j;
		//				for(const auto& context: contexts)
		//				{
		//					if(!context.status_) continue;
		//
		//					//find last slash
		//					j=0; //default to whole string
		//					for(i=0;i<context.address_.size();++i)
		//						if(context.address_[i] == '/')
		//							j = i+1;
		//					hostnames.push_back(context.address_.substr(j));
		//					__SUP_COUT__ << "hostname = " << hostnames.back() << __E__;
		//				}
		//			}
		//			catch(...)
		//			{
		//				__SUP_SS__ << "\nRelaunch of otsdaq interrupted! " <<
		//						"The Configuration Manager could not be initialized." << __E__;
		//
		//				__SUP_COUT_ERR__ << "\n" << ss.str();
		//				return;
		//			}
		//
		//			for(const auto& hostname: hostnames)
		//			{
		//				std::string fn = (std::string(getenv("SERVICE_DATA_PATH")) +
		//						"/StartOTS_action_" + hostname + ".cmd");
		//				FILE* fp = fopen(fn.c_str(),"w");
		//				if(fp)
		//				{
		//					if(requestType == "gatewayLaunchOTS")
		//						fprintf(fp,"LAUNCH_OTS");
		//					else if(requestType == "gatewayLaunchWiz")
		//						fprintf(fp,"LAUNCH_WIZ");
		//
		//					fclose(fp);
		//				}
		//				else
		//					__SUP_COUT_ERR__ << "Unable to open requestType file: " << fn << __E__;
		//			}

	}
	else if(requestType == "resetUserTooltips")
	{
		WebUsers::resetAllUserTooltips(theWebUsers_.getUsersUsername(userInfo.uid_));
	}
	else if (requestType == "codeEditor")
	{

		__SUP_COUT__ << "Code Editor" << __E__;

		codeEditor_.xmlRequest(
				CgiDataUtilities::getData(cgiIn, "option"),
				cgiIn,
				&xmlOut);
	}
	else
		__SUP_COUT__ << "requestType Request, " << requestType << ", not recognized." << __E__;

	//__SUP_COUT__ << "Made it" << __E__;

	//return xml doc holding server response
	xmlOut.outputXmlDocument((std::ostringstream*) out, false /*dispStdOut*/,
			true /*allowWhiteSpace*/); //Note: allow white space need for error response

	//__SUP_COUT__ << "done " << requestType << __E__;
} // end request()

//========================================================================================================================
//launchStartOTSCommand
void GatewaySupervisor::launchStartOTSCommand(const std::string& command)
{
	__SUP_COUT__ << "launch StartOTS Command = " << command << __E__;
	__SUP_COUT__ << "Extracting target context hostnames... " << __E__;

	std::vector<std::string> hostnames;
	try
	{
		CorePropertySupervisorBase::theConfigurationManager_->init(); //completely reset to re-align with any changes

		const XDAQContextConfiguration* contextConfiguration = CorePropertySupervisorBase::theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration);

		auto contexts = contextConfiguration->getContexts();
		unsigned int i, j;
		for (const auto& context : contexts)
		{
			if (!context.status_) continue;

			//find last slash
			j = 0; //default to whole string
			for (i = 0; i < context.address_.size(); ++i)
				if (context.address_[i] == '/')
					j = i + 1;
			hostnames.push_back(context.address_.substr(j));
			__SUP_COUT__ << "hostname = " << hostnames.back() << __E__;
		}
	}
	catch (...)
	{
		__SUP_SS__ << "\nRelaunch of otsdaq interrupted! " <<
			"The Configuration Manager could not be initialized." << __E__;

		__SS_THROW__;
	}

	for (const auto& hostname : hostnames)
	{
		std::string fn = (std::string(getenv("SERVICE_DATA_PATH")) +
						  "/StartOTS_action_" + hostname + ".cmd");
		FILE* fp = fopen(fn.c_str(), "w");
		if (fp)
		{
			fprintf(fp, command.c_str());
			fclose(fp);
		}
		else
		{
			__SUP_SS__ << "Unable to open command file: " << fn << __E__;
			__SS_THROW__;
		}
	}
}

//FIXME -- delete -- now all cookie checks return all info
////========================================================================================================================
////xoap::supervisorGetUserInfo
////	get user info to external supervisors
//xoap::MessageReference GatewaySupervisor::supervisorGetUserInfo(
//		xoap::MessageReference message)
//throw (xoap::exception::Exception)
//{
//	SOAPParameters parameters;
//	parameters.addParameter("CookieCode");
//	receive(message, parameters);
//	std::string cookieCode = parameters.getValue("CookieCode");
//
//	std::string username, displayName;
//	uint64_t activeSessionIndex;
//
//	theWebUsers_.getUserInfoForCookie(cookieCode, &username, &displayName,
//			&activeSessionIndex);
//
//	//__SUP_COUT__ << "username " << username << __E__;
//	//__SUP_COUT__ << "displayName " << displayName << __E__;
//
//	//fill return parameters
//	SOAPParameters retParameters;
//	retParameters.addParameter("Username", username);
//	retParameters.addParameter("DisplayName", displayName);
//	char tmpStr[100];
//	sprintf(tmpStr, "%lu", activeSessionIndex);
//	retParameters.addParameter("ActiveSessionIndex", tmpStr);
//
//	return SOAPUtilities::makeSOAPMessageReference("UserInfoResponse",
//			retParameters);
//}

//========================================================================================================================
//xoap::supervisorCookieCheck
//	verify cookie
xoap::MessageReference GatewaySupervisor::supervisorCookieCheck(xoap::MessageReference message)

{
	//__SUP_COUT__ << __E__;

	//receive request parameters
	SOAPParameters parameters;
	parameters.addParameter("CookieCode");
	parameters.addParameter("RefreshOption");
	parameters.addParameter("IPAddress");
	receive(message, parameters);
	std::string cookieCode = parameters.getValue("CookieCode");
	std::string refreshOption = parameters.getValue("RefreshOption"); //give external supervisors option to refresh cookie or not, "1" to refresh
	std::string ipAddress = parameters.getValue("IPAddress"); //give external supervisors option to refresh cookie or not, "1" to refresh

	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions, uint64_t uid
	//Else, error message is returned in cookieCode
	std::map<std::string /*groupName*/,WebUsers::permissionLevel_t> userGroupPermissionsMap;
	std::string userWithLock = "";
	uint64_t activeSessionIndex, uid;
	theWebUsers_.cookieCodeIsActiveForRequest(
			cookieCode,
			&userGroupPermissionsMap,
			&uid /*uid is not given to remote users*/,
			ipAddress,
			refreshOption == "1",
			&userWithLock,
			&activeSessionIndex);

	//__SUP_COUT__ << "userWithLock " << userWithLock << __E__;

	//fill return parameters
	SOAPParameters retParameters;
	retParameters.addParameter("CookieCode", 	cookieCode);
	retParameters.addParameter("Permissions", 	StringMacros::mapToString(userGroupPermissionsMap).c_str());
	retParameters.addParameter("UserWithLock", 	userWithLock);
	retParameters.addParameter("Username", 		theWebUsers_.getUsersUsername(uid));
	retParameters.addParameter("DisplayName", 	theWebUsers_.getUsersDisplayName(uid));
	sprintf(tmpStringForConversions_, "%lu", 	activeSessionIndex);
	retParameters.addParameter("ActiveSessionIndex", tmpStringForConversions_);

	//__SUP_COUT__ << __E__;

	return SOAPUtilities::makeSOAPMessageReference("CookieResponse",
												   retParameters);
}

//========================================================================================================================
//xoap::supervisorGetActiveUsers
//	get display names for all active users
xoap::MessageReference GatewaySupervisor::supervisorGetActiveUsers(
	xoap::MessageReference message)
	
{
	__SUP_COUT__ << __E__;

	SOAPParameters
		parameters("UserList", theWebUsers_.getActiveUsersString());
	return SOAPUtilities::makeSOAPMessageReference("ActiveUserResponse",
												   parameters);
}

//========================================================================================================================
//xoap::supervisorSystemMessage
//	receive a new system Message from a supervisor
//	ToUser wild card * is to all users
xoap::MessageReference GatewaySupervisor::supervisorSystemMessage(
	xoap::MessageReference message)
	
{
	SOAPParameters parameters;
	parameters.addParameter("ToUser");
	parameters.addParameter("Message");
	receive(message, parameters);

	__SUP_COUT__ << "toUser: " << parameters.getValue("ToUser").substr(
		0, 10) << ", message: " << parameters.getValue("Message").substr(0,
																		 10) << __E__;

	theSystemMessenger_.addSystemMessage(parameters.getValue("ToUser"),
										 parameters.getValue("Message"));
	return SOAPUtilities::makeSOAPMessageReference("SystemMessageResponse");
}

//===================================================================================================================
//xoap::supervisorSystemLogbookEntry
//	receive a new system Message from a supervisor
//	ToUser wild card * is to all users
xoap::MessageReference GatewaySupervisor::supervisorSystemLogbookEntry(
	xoap::MessageReference message)
	
{
	SOAPParameters parameters;
	parameters.addParameter("EntryText");
	receive(message, parameters);

	__SUP_COUT__ << "EntryText: " << parameters.getValue("EntryText").substr(
		0, 10) << __E__;

	makeSystemLogbookEntry(parameters.getValue("EntryText"));

	return SOAPUtilities::makeSOAPMessageReference("SystemLogbookResponse");
}

//===================================================================================================================
//supervisorLastConfigGroupRequest
//	return the group name and key for the last state machine activity
//
//	Note: same as OtsConfigurationWizardSupervisor::supervisorLastConfigGroupRequest
xoap::MessageReference GatewaySupervisor::supervisorLastConfigGroupRequest(
	xoap::MessageReference message)
	
{
	SOAPParameters parameters;
	parameters.addParameter("ActionOfLastGroup");
	receive(message, parameters);

	return GatewaySupervisor::lastConfigGroupRequestHandler(parameters);
}

//===================================================================================================================
//xoap::lastConfigGroupRequestHandler
//	handles last config group request.
//	called by both:
//		GatewaySupervisor::supervisorLastConfigGroupRequest
//		OtsConfigurationWizardSupervisor::supervisorLastConfigGroupRequest
xoap::MessageReference GatewaySupervisor::lastConfigGroupRequestHandler(
	const SOAPParameters &parameters)
{
	std::string action = parameters.getValue("ActionOfLastGroup");
	__COUT__ << "ActionOfLastGroup: " << action.substr(
		0, 10) << __E__;

	std::string fileName = "";
	if (action == "Configured")
		fileName = FSM_LAST_CONFIGURED_GROUP_ALIAS_FILE;
	else if (action == "Started")
		fileName = FSM_LAST_STARTED_GROUP_ALIAS_FILE;
	else
	{
		__COUT_ERR__ << "Invalid last group action requested." << __E__;
		return SOAPUtilities::makeSOAPMessageReference("LastConfigGroupResponseFailure");
	}
	std::string timeString;
	std::pair<std::string /*group name*/, ConfigurationGroupKey> theGroup =
		loadGroupNameAndKey(fileName, timeString);

	//fill return parameters
	SOAPParameters retParameters;
	retParameters.addParameter("GroupName", theGroup.first);
	retParameters.addParameter("GroupKey", theGroup.second.toString());
	retParameters.addParameter("GroupAction", action);
	retParameters.addParameter("GroupActionTime", timeString);


	return SOAPUtilities::makeSOAPMessageReference("LastConfigGroupResponse",
												   retParameters);
}

//========================================================================================================================
//getNextRunNumber
//
//	If fsmName is passed, then get next run number for that FSM name
//	Else get next run number for the active FSM name, activeStateMachineName_
//
// 	Note: the FSM name is sanitized of special characters and used in the filename.
unsigned int GatewaySupervisor::getNextRunNumber(const std::string &fsmNameIn)
{
	std::string runNumberFileName = RUN_NUMBER_PATH + "/";
	std::string fsmName = fsmNameIn == "" ? activeStateMachineName_ : fsmNameIn;
	//prepend sanitized FSM name
	for (unsigned int i = 0; i < fsmName.size(); ++i)
		if ((fsmName[i] >= 'a' && fsmName[i] <= 'z') ||
			(fsmName[i] >= 'A' && fsmName[i] <= 'Z') ||
			(fsmName[i] >= '0' && fsmName[i] <= '9'))
			runNumberFileName += fsmName[i];
	runNumberFileName += RUN_NUMBER_FILE_NAME;
	//__SUP_COUT__ << "runNumberFileName: " << runNumberFileName << __E__;

	std::ifstream runNumberFile(runNumberFileName.c_str());
	if (!runNumberFile.is_open())
	{
		__SUP_COUT__ << "Can't open file: " << runNumberFileName << __E__;

		__SUP_COUT__ << "Creating file and setting Run Number to 1: " << runNumberFileName << __E__;
		FILE *fp = fopen(runNumberFileName.c_str(), "w");
		fprintf(fp, "1");
		fclose(fp);

		runNumberFile.open(runNumberFileName.c_str());
		if (!runNumberFile.is_open())
		{
			__SUP_SS__ << "Error. Can't create file: " << runNumberFileName << __E__;
			__SUP_COUT_ERR__ << ss.str();
			__SS_THROW__;
		}
	}
	std::string runNumberString;
	runNumberFile >> runNumberString;
	runNumberFile.close();
	return atoi(runNumberString.c_str());
}

//========================================================================================================================
bool GatewaySupervisor::setNextRunNumber(unsigned int runNumber, const std::string &fsmNameIn)
{
	std::string runNumberFileName = RUN_NUMBER_PATH + "/";
	std::string fsmName = fsmNameIn == "" ? activeStateMachineName_ : fsmNameIn;
	//prepend sanitized FSM name
	for (unsigned int i = 0; i < fsmName.size(); ++i)
		if ((fsmName[i] >= 'a' && fsmName[i] <= 'z') ||
			(fsmName[i] >= 'A' && fsmName[i] <= 'Z') ||
			(fsmName[i] >= '0' && fsmName[i] <= '9'))
			runNumberFileName += fsmName[i];
	runNumberFileName += RUN_NUMBER_FILE_NAME;
	__SUP_COUT__ << "runNumberFileName: " << runNumberFileName << __E__;

	std::ofstream runNumberFile(runNumberFileName.c_str());
	if (!runNumberFile.is_open())
	{
		__SUP_SS__ << "Can't open file: " << runNumberFileName << __E__;
		__SUP_COUT__ << ss.str();
		__SS_THROW__;
	}
	std::stringstream runNumberStream;
	runNumberStream << runNumber;
	runNumberFile << runNumberStream.str().c_str();
	runNumberFile.close();
	return true;
}

//========================================================================================================================
//loadGroupNameAndKey
//	loads group name and key (and time) from specified file
//	returns time string in returnedTimeString
//
//	Note: this is static so the OtsConfigurationWizardSupervisor can call it
std::pair<std::string /*group name*/,
	ConfigurationGroupKey> GatewaySupervisor::loadGroupNameAndKey(const std::string &fileName,
																  std::string &returnedTimeString)
{
	std::string fullPath = FSM_LAST_GROUP_ALIAS_PATH + "/" + fileName;

	FILE *groupFile = fopen(fullPath.c_str(), "r");
	if (!groupFile)
	{
		__COUT__ << "Can't open file: " << fullPath << __E__;

		__COUT__ << "Returning empty groupName and key -1" << __E__;

		return std::pair<std::string /*group name*/,
			ConfigurationGroupKey>("", ConfigurationGroupKey());
	}

	char line[500]; //assuming no group names longer than 500 chars
	//name and then key
	std::pair<std::string /*group name*/,
		ConfigurationGroupKey> theGroup;

	fgets(line, 500, groupFile); //name
	theGroup.first = line;

	fgets(line, 500, groupFile); //key
	int key;
	sscanf(line, "%d", &key);
	theGroup.second = key;

	fgets(line, 500, groupFile); //time
	time_t timestamp;
	sscanf(line, "%ld", &timestamp); //type long int
	struct tm tmstruct;
	::localtime_r(&timestamp, &tmstruct);
	::strftime(line, 30, "%c %Z", &tmstruct);
	returnedTimeString = line;
	fclose(groupFile);


	__COUT__ << "theGroup.first= " << theGroup.first <<
		" theGroup.second= " << theGroup.second << __E__;

	return theGroup;
}

//========================================================================================================================
void GatewaySupervisor::saveGroupNameAndKey(const std::pair<std::string /*group name*/,
	ConfigurationGroupKey> &theGroup,
											const std::string &fileName)
{
	std::string fullPath = FSM_LAST_GROUP_ALIAS_PATH + "/" + fileName;

	std::ofstream groupFile(fullPath.c_str());
	if (!groupFile.is_open())
	{
		__SUP_SS__ << "Error. Can't open file: " << fullPath << __E__;
		__SUP_COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
	}
	std::stringstream outss;
	outss << theGroup.first << "\n" << theGroup.second << "\n" << time(0);
	groupFile << outss.str().c_str();
	groupFile.close();
}



/////////////////////////////////////////////////////////////////////////////////////


////========================================================================================================================
////GatewaySupervisor::infoRequest ---
//void GatewaySupervisor::infoRequest(xgi::Input * in, xgi::Output * out )  
//{
//    __SUP_COUT__ << __LINE__ <<  __E__ << __E__;
//    cgicc::Cgicc cgi(in);
//    std::string Command=cgi.getElement("RequestType")->getValue();
//
//    if (Command=="FEWList")
//   {
//        *out << "<FEWList>" << __E__;
//        for (std::set<xdaq::ApplicationDescriptor*>::iterator pxFESupervisorsIt=pxFESupervisors_.begin(); pxFESupervisorsIt!=pxFESupervisors_.end(); pxFESupervisorsIt++)
//       {
//        	*out << "<FEWInterface id='1' alias='FPIX Disk 1' type='OtsUDPHardware'>" << __E__
//        	     << "<svg xmlns='http://www.w3.org/2000/svg' version='1.1'>" <<  __E__
//                 << "<circle cx='100' cy='50' r='40' stroke='black' stroke-width='2' fill='red' />" << __E__
//                 << "</svg>" << __E__
//                 << "</FEWInterface>" << __E__;
//         	*out << "<FEWInterface id='2' alias='FPIX Disk 2' type='OtsUDPHardware'>" << __E__
//        	     << "<svg xmlns='http://www.w3.org/2000/svg' version='1.1'>" <<  __E__
//                 << "<circle cx='100' cy='50' r='40' stroke='black' stroke-width='2' fill='red' />" << __E__
//                 << "</svg>" << __E__
//                 << "</FEWInterface>" << __E__;
//        }
//        *out << "</FEWList>" << __E__;
//    }
//}
//========================================================================================================================

