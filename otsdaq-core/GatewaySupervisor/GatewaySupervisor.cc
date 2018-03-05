#include "otsdaq-core/GatewaySupervisor/GatewaySupervisor.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"

#include "otsdaq-core/WorkLoopManager/WorkLoopManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManagerRW.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"


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


#undef 	__MF_SUBJECT__
#define __MF_SUBJECT__ "GatewaySupervisor"


XDAQ_INSTANTIATOR_IMPL(GatewaySupervisor)


//========================================================================================================================
GatewaySupervisor::GatewaySupervisor(xdaq::ApplicationStub * s) throw (xdaq::exception::Exception)
:xdaq::Application           	(s)
,SOAPMessenger               	(this)
,RunControlStateMachine      	("GatewaySupervisor")
,theConfigurationManager_   	(new ConfigurationManager)
//,theConfigurationGroupKey_    (nullptr)
,stateMachineWorkLoopManager_	(toolbox::task::bind(this, &GatewaySupervisor::stateMachineThread,"StateMachine"))
,stateMachineSemaphore_      	(toolbox::BSem::FULL)
,infoRequestWorkLoopManager_ 	(toolbox::task::bind(this, &GatewaySupervisor::infoRequestThread, "InfoRequest"))
,infoRequestSemaphore_       	(toolbox::BSem::FULL)
,activeStateMachineName_ 	 	("")
,theIterator_					(this)
,counterTest_                	(0)
{
	INIT_MF("GatewaySupervisor");
	__COUT__ << std::endl;

	//attempt to make directory structure (just in case)
	mkdir((FSM_LAST_GROUP_ALIAS_PATH).c_str(), 0755);
	mkdir((RUN_NUMBER_PATH).c_str(), 0755);

	securityType_ = theWebUsers_.getSecurity();

	__COUT__ << "Security: " << securityType_ << std::endl;

	xgi::bind(this, &GatewaySupervisor::Default,                  "Default");
	xgi::bind(this, &GatewaySupervisor::loginRequest,             "LoginRequest");
	xgi::bind(this, &GatewaySupervisor::request,                  "Request");
	xgi::bind(this, &GatewaySupervisor::stateMachineXgiHandler,   "StateMachineXgiHandler");
	xgi::bind(this, &GatewaySupervisor::infoRequestHandler,       "InfoRequestHandler");
	xgi::bind(this, &GatewaySupervisor::infoRequestResultHandler, "InfoRequestResultHandler");
	xgi::bind(this, &GatewaySupervisor::tooltipRequest,           "TooltipRequest");

	xoap::bind(this, &GatewaySupervisor::supervisorCookieCheck,        	"SupervisorCookieCheck",        	XDAQ_NS_URI);
	xoap::bind(this, &GatewaySupervisor::supervisorGetActiveUsers,     	"SupervisorGetActiveUsers",     	XDAQ_NS_URI);
	xoap::bind(this, &GatewaySupervisor::supervisorSystemMessage,      	"SupervisorSystemMessage",      	XDAQ_NS_URI);
	xoap::bind(this, &GatewaySupervisor::supervisorGetUserInfo,        	"SupervisorGetUserInfo",        	XDAQ_NS_URI);
	xoap::bind(this, &GatewaySupervisor::supervisorSystemLogbookEntry, 	"SupervisorSystemLogbookEntry", 	XDAQ_NS_URI);
	xoap::bind(this, &GatewaySupervisor::supervisorLastConfigGroupRequest, "SupervisorLastConfigGroupRequest", XDAQ_NS_URI);


	//old:
	//I always load all configuration even the one that is not related to the particular FEW/FER,
	//so I can load it once and is good for all applications I am scared about threads so I comment it out!
	//theConfigurationManager_ = Singleton<ConfigurationManager>::getInstance();

	init();
}

//========================================================================================================================
//	TODO: Lore needs to detect program quit through killall or ctrl+c so that Logbook entry is made when ots is halted
GatewaySupervisor::~GatewaySupervisor(void)
{
	delete theConfigurationManager_;
	makeSystemLogbookEntry("ots halted.");
}

//========================================================================================================================
void GatewaySupervisor::init(void)
{
	//This can be done in the constructor because when you start xdaq it loads the configuration that can't be changed while running!
	allSupervisorInfo_.init(getApplicationContext());

	supervisorGuiHasBeenLoaded_ = false;

	const XDAQContextConfiguration* contextConfiguration = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration);

	supervisorContextUID_ = contextConfiguration->getContextUID(
			getApplicationContext()->getContextDescriptor()->getURL()
	);
	__COUT__ << "Context UID:" << supervisorContextUID_	<< std::endl;

	supervisorApplicationUID_ = contextConfiguration->getApplicationUID(
			getApplicationContext()->getContextDescriptor()->getURL(),
			getApplicationDescriptor()->getLocalId()
	);

	__COUT__ << "Application UID:" << supervisorApplicationUID_	<< std::endl;

	ConfigurationTree configLinkNode = theConfigurationManager_->getSupervisorConfigurationNode(
			supervisorContextUID_, supervisorApplicationUID_);

	std::string supervisorUID;
	if(!configLinkNode.isDisconnected())
		supervisorUID = configLinkNode.getValue();
	else
		supervisorUID = ViewColumnInfo::DATATYPE_LINK_DEFAULT;

	__COUT__ << "GatewaySupervisor UID:" << supervisorUID	<< std::endl;



	//setting up thread for UDP thread to drive state machine
	bool enableStateChanges = false;
	try
	{
		enableStateChanges =
				configLinkNode.getNode("EnableStateChangesOverUDP").getValue<bool>();
	}
	catch(...)
	{;} //ignore errors

	if(enableStateChanges)
	{
		__COUT__ << "Enabling state changes over UDP..." << __E__;
		//start state changer UDP listener thread
		std::thread([](GatewaySupervisor *s){ GatewaySupervisor::StateChangerWorkLoop(s); },this).detach();
	}
	else
		__COUT__ << "State changes over UDP are disabled." << __E__;

}

//========================================================================================================================
//StateChangerWorkLoop
//	child thread
void GatewaySupervisor::StateChangerWorkLoop(GatewaySupervisor *theSupervisor)
{
	ConfigurationTree configLinkNode = theSupervisor->theConfigurationManager_->getSupervisorConfigurationNode(
			theSupervisor->supervisorContextUID_, theSupervisor->supervisorApplicationUID_);


	std::string myip = configLinkNode.getNode("IPAddressForStateChangesOverUDP").getValue<std::string>();
	int myport = configLinkNode.getNode("PortForStateChangesOverUDP").getValue<int>();
	bool ackEnabled = configLinkNode.getNode("EnableAckForStateChangesOverUDP").getValue<bool>();

	__COUT__ << "ip = " << myip << __E__;
	__COUT__ << "port = " << myport << __E__;
	__COUT__ << "ackEnabled = " << ackEnabled << __E__;

	TransceiverSocket sock(myip,myport); //Take Port from Configuration
	try
	{
		sock.initialize();
	}
	catch(...)
	{

		//generate special message to indicate failed socket
		__SS__ << "FATAL Console error. Could not initialize socket at ip '" << myip <<
				"' and port " <<
				myport << ". Perhaps it is already in use? Exiting State Changer receive loop." << std::endl;
		__COUT__ << ss.str();
		throw std::runtime_error(ss.str());
		return;
	}

	unsigned int i,firsti;
	std::string buffer;
	std::string errorStr;
	std::string command, fsmName;
	std::vector<std::string> parameters;
	while(1)
	{
		//workloop procedure
		//	if receive a UDP command
		//		execute command
		//	else
		//		sleep


		if(sock.receive(buffer,0 /*timeoutSeconds*/,1/*timeoutUSeconds*/,
				false /*verbose*/) != -1)
		{
			__COUT__ << "UDP State Changer received size = " << buffer.size() << __E__;

			command = "", fsmName = "";
			parameters.clear();

			//extract comma separated values: command,fsmname,parameter(s)
			for(firsti=0,i=0;i<buffer.size();++i)
			{
				if(buffer[i] == ',')
				{
					if(command == "")
					{
						command = buffer.substr(firsti,i-firsti);
						__COUT__ << "command = " << command << __E__;
					}
					else if(fsmName == "")
					{
						fsmName = buffer.substr(firsti,i-firsti);
						__COUT__ << "fsmName = " << fsmName << __E__;
					}
					else //parameter
					{
						parameters.push_back(buffer.substr(firsti,i-firsti));

						__COUT__ << "parameter[" << parameters.size()-1 <<
								"] = " << command << __E__;
					}
					firsti = i+1;
				}
			}

			//set scope of mutext
			{
				//should be mutually exclusive with GatewaySupervisor main thread state machine accesses
				//lockout the messages array for the remainder of the scope
				//this guarantees the reading thread can safely access the messages
				if(theSupervisor->VERBOSE_MUTEX) __COUT__ << "Waiting for FSM access" << __E__;
				std::lock_guard<std::mutex> lock(theSupervisor->stateMachineAccessMutex_);
				if(theSupervisor->VERBOSE_MUTEX) __COUT__ << "Have FSM access" << __E__;

				errorStr = theSupervisor->attemptStateMachineTransition(
						0,0,
						command,fsmName,
						WebUsers::DEFAULT_STATECHANGER_USERNAME /*fsmWindowName*/,
						WebUsers::DEFAULT_STATECHANGER_USERNAME,
						parameters);
			}

			if(errorStr != "")
			{
				__SS__ << "UDP State Changer failed to execute command because of the following error: " << errorStr;
				__COUT_ERR__ << ss.str();
				__MOUT_ERR__ << ss.str();
				if(ackEnabled) sock.acknowledge(errorStr,true /*verbose*/);
			}
			else
			{
				__SS__ << "Successfully executed state change command '" << command << ".'" << __E__;
				__COUT_INFO__ << ss.str();
				__MOUT_INFO__ << ss.str();
				if(ackEnabled) sock.acknowledge("Done",true /*verbose*/);
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
	__COUT__ << "Making System Logbook Entry: " << entryText << std::endl;

	SupervisorInfoMap logbookInfoMap = allSupervisorInfo_.getAllLogbookTypeSupervisorInfo();

	if(logbookInfoMap.size() == 0)
	{
		__COUT__ << "No logbooks found! Here is entry: " << entryText << std::endl;
		__MOUT__ << "No logbooks found! Here is entry: " << entryText << std::endl;
		return;
	}
	else
	{
		__COUT__ << "Making logbook entry: " << entryText << std::endl;
		__MOUT__ << "Making logbook entry: " << entryText << std::endl;
	}


	//__COUT__ << "before: " << entryText << std::endl;
	{	//input entryText
		std::string replace[] =
		{"\"", "'", "&", "<", ">", "\n", "  "};
		std::string with[] =
		{"%22","%27", "%26", "%3C", "%3E", "%0A%0D", "%20%20"};

		int numOfKeys = 7;

		size_t f;
		for(int i=0;i<numOfKeys;++i)
		{
			while((f=entryText.find(replace[i])) != std::string::npos)
			{
				entryText = entryText.substr(0,f) + with[i] +
						entryText.substr(f+replace[i].length());
				//__COUT__ << "found " << " " << entryText << std::endl;
			}
		}
	}
	//__COUT__ << "after: " << entryText << std::endl;

	SOAPParameters parameters("EntryText",entryText);
	//SOAPParametersV parameters(1);
	//parameters[0].setName("EntryText"); parameters[0].setValue(entryText);

	for(auto& logbookInfo: logbookInfoMap)
	{
		xoap::MessageReference retMsg = SOAPMessenger::sendWithSOAPReply(
				logbookInfo.second.getDescriptor(),
				"MakeSystemLogbookEntry",parameters);

		SOAPParameters retParameters("Status");
		//SOAPParametersV retParameters(1);
		//retParameters[0].setName("Status");
		receive(retMsg, retParameters);

		__COUT__ << "Returned Status: " << retParameters.getValue("Status") << std::endl;//retParameters[0].getValue() << std::endl << std::endl;
	}

}

//========================================================================================================================
void GatewaySupervisor::Default(xgi::Input* in, xgi::Output* out)
throw (xgi::exception::Exception)
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
throw (xgi::exception::Exception)
{
	//for simplicity assume all commands should be mutually exclusive with iterator thread state machine accesses (really should just be careful with RunControlStateMachine access)
	if(VERBOSE_MUTEX) __COUT__ << "Waiting for FSM access" << std::endl;
	std::lock_guard<std::mutex> lock(stateMachineAccessMutex_);
	if(VERBOSE_MUTEX) __COUT__ << "Have FSM access" << std::endl;

	cgicc::Cgicc cgi(in);

	uint8_t userPermissions;
	uint64_t uid;
	std::string userWithLock;
	std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode, &userPermissions,
			&uid, "0", 1, &userWithLock))
	{
		*out << cookieCode;
		return;
	}

	std::string command = CgiDataUtilities::getData(cgi, "StateMachine");

	//**** start LOCK GATEWAY CODE ***//
	std::string username = "";
	username = theWebUsers_.getUsersUsername(uid);
	if (userWithLock != "" && userWithLock != username)
	{
		*out << WebUsers::REQ_USER_LOCKOUT_RESPONSE;
		__COUT__ << "User " << username << " is locked out. " << userWithLock << " has lock." << std::endl;
		return;
	}
	//**** end LOCK GATEWAY CODE ***//

	HttpXmlDocument xmldoc(cookieCode);
	std::string fsmName = CgiDataUtilities::getData(cgi, "fsmName");
	std::string fsmWindowName = CgiDataUtilities::getData(cgi, "fsmWindowName");
	fsmWindowName = CgiDataUtilities::decodeURIComponent(fsmWindowName);
	std::string currentState = theStateMachine_.getCurrentStateName();

	__COUT__ << "Check for Handled by theIterator_" << std::endl;

	//check if Iterator should handle
	if((activeStateMachineWindowName_ == "" ||
			activeStateMachineWindowName_ == "iterator") &&
			theIterator_.handleCommandRequest(xmldoc,command,fsmWindowName))
	{
		__COUT__ << "Handled by theIterator_" << std::endl;
		xmldoc.outputXmlDocument((std::ostringstream*) out, false);
		return;
	}

	//Do not allow transition while in transition
	if (theStateMachine_.isInTransition())
	{
		__SS__ << "Error - Can not accept request because the State Machine is already in transition!" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();

		xmldoc.addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
		xmldoc.addTextElementToData("state_tranisition_attempted_err",
				ss.str()); //indicate to GUI transition NOT attempted
		xmldoc.outputXmlDocument((std::ostringstream*) out, false, true);
		return;
	}



	/////////////////
	//Validate FSM name
	//	if fsm name != active fsm name
	//		only allow, if current state is halted or init
	//		take active fsm name when configured
	//	else, allow
	if(activeStateMachineName_ != "" &&
			activeStateMachineName_ != fsmName)
	{
		__COUT__ << "currentState = " <<
				currentState << std::endl;
		if(currentState != "Halted" &&
				currentState != "Initial")
		{
			//illegal for this FSM name to attempt transition

			__SS__ << "Error - Can not accept request because the State Machine " <<
					"with window name '" <<
					activeStateMachineWindowName_ << "' (UID: " <<
					activeStateMachineName_ << ") "
					"is currently " <<
					"in control of State Machine progress. ";
			ss << "\n\nIn order for this State Machine with window name '" <<
					fsmWindowName << "' (UID: " << fsmName << ") "
					"to control progress, please transition to Halted using the active " <<
					"State Machine '" << activeStateMachineWindowName_ << ".'" << std::endl;
			__COUT_ERR__ << "\n" << ss.str();

			xmldoc.addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
			xmldoc.addTextElementToData("state_tranisition_attempted_err",
					ss.str()); //indicate to GUI transition NOT attempted
			xmldoc.outputXmlDocument((std::ostringstream*) out, false, true);
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
		parameters.push_back(CgiDataUtilities::postData(cgi, "ConfigurationAlias"));
	attemptStateMachineTransition(&xmldoc,out,command,fsmName,fsmWindowName,
			username,parameters);
}

std::string GatewaySupervisor::attemptStateMachineTransition(
		HttpXmlDocument* xmldoc, std::ostringstream* out,
		const std::string& command,
		const std::string& fsmName, const std::string& fsmWindowName,
		const std::string& username,
		const std::vector<std::string>& commandParameters)
{
	std::string errorStr = "";

	std::string currentState = theStateMachine_.getCurrentStateName();
	__COUT__ << "State Machine command = " << command << std::endl;
	__COUT__ << "fsmName = " << fsmName << std::endl;
	__COUT__ << "fsmWindowName = " << fsmWindowName << std::endl;
	__COUT__ << "activeStateMachineName_ = " << activeStateMachineName_ << std::endl;

	SOAPParameters parameters;
	if (command == "Configure")
	{
		if(currentState != "Halted") //check if out of sync command
		{
			__SS__ << "Error - Can only transition to Configured if the current " <<
					"state is Halted. Perhaps your state machine is out of sync." <<
					std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			errorStr = ss.str();

			if(xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
			if(xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted_err",
					ss.str()); //indicate to GUI transition NOT attempted
			if(out) xmldoc->outputXmlDocument((std::ostringstream*) out, false, true);

			return errorStr;
		}

		//NOTE Original name of the configuration key
		//parameters.addParameter("RUN_KEY",CgiDataUtilities::postData(cgi,"ConfigurationAlias"));
		if(!commandParameters.size())
		{
			__SS__ << "Error - Can only transition to Configured if a Configuration Alias parameter is provided." <<
					std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			errorStr = ss.str();

			if(xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
			if(xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted_err",
					ss.str()); //indicate to GUI transition NOT attempted
			if(out) xmldoc->outputXmlDocument((std::ostringstream*) out, false, true);

			return errorStr;
		}

		parameters.addParameter("ConfigurationAlias",
				commandParameters[0]);

		std::string configurationAlias = parameters.getValue("ConfigurationAlias");
		__COUT__ << "Configure --> Name: ConfigurationAlias Value: " <<
				configurationAlias << std::endl;

		//save last used config alias
		std::string fn = FSM_LAST_GROUP_ALIAS_PATH + FSM_LAST_GROUP_ALIAS_FILE_START +
				username + "." + FSM_USERS_PREFERENCES_FILETYPE;

		__COUT__ << "Save FSM preferences: " << fn << std::endl;
		FILE *fp = fopen(fn.c_str(),"w");
		if(!fp)
		{
			__SS__ << ("Could not open file: " + fn) << std::endl;
			__COUT_ERR__ << ss.str();
			throw std::runtime_error(ss.str());
		}
		fprintf(fp,"FSM_last_configuration_alias %s",configurationAlias.c_str());
		fclose(fp);

		activeStateMachineName_ = fsmName;
		activeStateMachineWindowName_ = fsmWindowName;
	}
	else if (command == "Start")
	{
		if(currentState != "Configured") //check if out of sync command
		{
			__SS__ << "Error - Can only transition to Configured if the current " <<
					"state is Halted. Perhaps your state machine is out of sync. " <<
					"(Likely the server was restarted or another user changed the state)" <<
					std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			errorStr = ss.str();

			if(xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted", "0"); //indicate to GUI transition NOT attempted
			if(xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted_err",
					ss.str()); //indicate to GUI transition NOT attempted
			if(out) xmldoc->outputXmlDocument((std::ostringstream*) out, false, true);

			return errorStr;
		}

		unsigned int runNumber = getNextRunNumber();
		parameters.addParameter("RunNumber", runNumber);
		setNextRunNumber(++runNumber);
	}

	xoap::MessageReference message = SOAPUtilities::makeSOAPMessageReference(
			command, parameters);
	//Maybe we return an acknowledgment that the message has been received and processed
	xoap::MessageReference reply = stateMachineXoapHandler(message);
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);

	if(xmldoc) xmldoc->addTextElementToData("state_tranisition_attempted", "1"); //indicate to GUI transition attempted
	if(out) xmldoc->outputXmlDocument((std::ostringstream*) out, false);
	__COUT__ << "FSM state transition launched!" << std::endl;

	stateMachineLastCommandInput_ = command;
	return errorStr;
}

//========================================================================================================================
void GatewaySupervisor::stateMachineResultXgiHandler(xgi::Input* in, xgi::Output* out)
throw (xgi::exception::Exception)
{
	cgicc::Cgicc cgi(in);
	__COUT__ << "Xgi Request!" << std::endl;

	uint8_t userPermissions; // uint64_t uid;
	std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode,
			&userPermissions))
	{
		*out << cookieCode;
		return;
	}

	HttpXmlDocument xmldoc(cookieCode);

	std::string command = CgiDataUtilities::getData(cgi, "StateMachine");

	SOAPParameters parameters;
	/*
	 //FIXME I don't think that there should be this if statement and I guess it has been copied from the stateMachineXgiHandler method
	 if (command == "Configure")

{
	 parameters.addParameter("RUN_KEY", CgiDataUtilities::postData(cgi, "ConfigurationAlias"));
	 __COUT__ << "Configure --> Name: RUN_KEY Value: " << parameters.getValue("RUN_KEY") << std::endl;
	 }
	 else if (command == "Start")

{
	 unsigned int runNumber = getNextRunNumber();
	 std::stringstream runNumberStream;
	 runNumberStream << runNumber;
	 parameters.addParameter("RUN_NUMBER", runNumberStream.str().c_str());
	 setNextRunNumber(++runNumber);
	 }
	 */
	xoap::MessageReference message = SOAPUtilities::makeSOAPMessageReference(
			CgiDataUtilities::getData(cgi, "StateMachine"), parameters);
	//Maybe we return an aknowledgment that the message has been received and processed
	xoap::MessageReference reply = stateMachineResultXoapHandler(message);
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);
	//xmldoc.outputXmlDocument((ostringstream*)out,false);
	__COUT__ << "Done - Xgi Request!" << std::endl;
}

//========================================================================================================================
xoap::MessageReference GatewaySupervisor::stateMachineXoapHandler(xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	__COUT__ << "Soap Handler!" << std::endl;
	stateMachineWorkLoopManager_.removeProcessedRequests();
	stateMachineWorkLoopManager_.processRequest(message);
	__COUT__ << "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
xoap::MessageReference GatewaySupervisor::stateMachineResultXoapHandler(
		xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	__COUT__ << "Soap Handler!" << std::endl;
	//stateMachineWorkLoopManager_.removeProcessedRequests();
	//stateMachineWorkLoopManager_.processRequest(message);
	__COUT__ << "Done - Soap Handler!" << std::endl;
	return message;
}

//========================================================================================================================
bool GatewaySupervisor::stateMachineThread(toolbox::task::WorkLoop* workLoop)
{
	stateMachineSemaphore_.take();
	__COUT__ << "Re-sending message..." << SOAPUtilities::translate( stateMachineWorkLoopManager_.getMessage(workLoop)).getCommand() << std::endl;
	std::string reply = send(
			allSupervisorInfo_.getGatewayDescriptor(),
			stateMachineWorkLoopManager_.getMessage(workLoop));
	stateMachineWorkLoopManager_.report(workLoop, reply, 100, true);

	__COUT__ << "Done with message. Reply = " << reply << std::endl;
	stateMachineSemaphore_.give();

	if(reply == "Fault")
	{
		__SS__ << "Failure to send Workloop transition command! Unrecognized transition name." << std::endl;
		__COUT_ERR__ << ss.str();
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
throw (xgi::exception::Exception)
{
	__COUT__ << "Starting to Request!" << std::endl;
	cgicc::Cgicc cgi(in);

	//**** start LOGIN GATEWAY CODE ***//
	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions, uint64_t uid
	//Else, error message is returned in cookieCode
	std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
	uint8_t userPermissions;// uint64_t uid;
	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode, &userPermissions))
	{
		*out << cookieCode;
		return;
	}
	//**** end LOGIN GATEWAY CODE ***//

	HttpXmlDocument xmldoc(cookieCode);

	//If the thread is canceled when you are still in this method that activated it, then everything will freeze.
	//The way the workloop manager now works is safe since it cancel the thread only when the infoRequestResultHandler is called
	//and that methid can be called ONLY when I am already out of here!

	HttpXmlDocument tmpDoc = infoRequestWorkLoopManager_.processRequest(cgi);

	xmldoc.copyDataChildren(tmpDoc);

	xmldoc.outputXmlDocument((std::ostringstream*) out, false);
}

//========================================================================================================================
void GatewaySupervisor::infoRequestResultHandler(xgi::Input* in, xgi::Output* out)
throw (xgi::exception::Exception)
{
	__COUT__ << "Starting ask!" << std::endl;
	cgicc::Cgicc cgi(in);

	//**** start LOGIN GATEWAY CODE ***//
	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions, uint64_t uid
	//Else, error message is returned in cookieCode
	std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
	uint8_t userPermissions;// uint64_t uid;
	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode, &userPermissions))
	{
		*out << cookieCode;
		return;
	}
	//**** end LOGIN GATEWAY CODE ***//

	HttpXmlDocument xmldoc(cookieCode);

	infoRequestWorkLoopManager_.getRequestResult(cgi, xmldoc);

	//return xml doc holding server response
	xmldoc.outputXmlDocument((std::ostringstream*) out, false);

	__COUT__ << "Done asking!" << std::endl;
}

//========================================================================================================================
bool GatewaySupervisor::infoRequestThread(toolbox::task::WorkLoop* workLoop)
{
	//    std::string workLoopName = workLoop->getName();
	//    __COUT__ << " Starting WorkLoop: " << workLoopName << std::endl;
	//    __COUT__ << " Ready to lock" << std::endl;
	infoRequestSemaphore_.take();
	//    __COUT__ << " Locked" << std::endl;
	vectorTest_.clear();

	for (unsigned long long i = 0; i < 100000000; i++)
	{
		counterTest_ += 2;
		vectorTest_.push_back(counterTest_);
	}

	infoRequestWorkLoopManager_.report(workLoop,
			"RESULT: This is the best result ever", 50, false);
	std::string workLoopName = workLoop->getName();
	__COUT__ << workLoopName << " test: " << counterTest_
			<< " vector size: " << vectorTest_.size() << std::endl;
	wait(400, "InfoRequestThread ----- locked");
	infoRequestSemaphore_.give();
	//    __COUT__ << " Lock released" << std::endl;
	wait(200, "InfoRequestThread");
	//    __COUT__ << " Ready to lock again" << std::endl;
	infoRequestSemaphore_.take();
	//    __COUT__ << " Locked again" << std::endl;
	vectorTest_.clear();

	for (unsigned long long i = 0; i < 100000000; i++)
	{
		counterTest_ += 2;
		vectorTest_.push_back(counterTest_);
	}

	wait(400, "InfoRequestThread ----- locked");
	__COUT__ << workLoopName << " test: " << counterTest_ << " vector size: " << vectorTest_.size() << std::endl;
	infoRequestSemaphore_.give();
	//    __COUT__ << " Lock released again" << std::endl;
	//infoRequestWorkLoopManager_->report(workLoop,"RESULT: This is the best result ever");
	infoRequestWorkLoopManager_.report(workLoop,
			theStateMachine_.getCurrentStateName(), 100, true);
	//    __COUT__ << " Done with WorkLoop: " << workLoopName << std::endl;
	return false; //execute once and automatically remove the workloop so in WorkLoopManager the try workLoop->remove(job_) could be commented out
	//return true;//go on and then you must do the workLoop->remove(job_) in WorkLoopManager
}

//========================================================================================================================
void GatewaySupervisor::stateInitial(toolbox::fsm::FiniteStateMachine & fsm)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;
	//diagService_->reportError("--- GatewaySupervisor is in its Initial state ---",DIAGINFO);
	//diagService_->reportError("GatewaySupervisor::stateInitial: workloop active: "+stringF(calibWorkloop_->isActive())+", workloop type: "+calibWorkloop_->getType(),DIAGINFO);
}

//========================================================================================================================
void GatewaySupervisor::statePaused(toolbox::fsm::FiniteStateMachine & fsm)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;
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
throw (toolbox::fsm::exception::Exception)
{

	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;
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
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;
	__COUT__ << "Fsm is in transition? " << (theStateMachine_.isInTransition()?"yes":"no") << std::endl;


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
throw (toolbox::fsm::exception::Exception)
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

//void GatewaySupervisor::stateTTSTestMode (toolbox::fsm::FiniteStateMachine & fsm) throw (toolbox::fsm::exception::Exception)
//{
//  previousState_ = theStateMachine_.getCurrentStateName();
//}

//void GatewaySupervisor::inError (toolbox::fsm::FiniteStateMachine & fsm) throw (toolbox::fsm::exception::Exception)
//{
//  previousState_ = theStateMachine_.getCurrentStateName();
//rcmsStateNotifier_.stateChanged("Error", "");
//}

/*
 xoap::MessageReference GatewaySupervisor::reset (xoap::MessageReference message) throw (xoap::exception::Exception)

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
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;
	//rcmsStateNotifier_.stateChanged("Error", "");
}

//========================================================================================================================
void GatewaySupervisor::enteringError(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;

	//extract error message and save for user interface access
	toolbox::fsm::FailedEvent& failedEvent = dynamic_cast<toolbox::fsm::FailedEvent&> (*e);
	__SS__ << "\nFailure performing transition from " << failedEvent.getFromState() << "-" <<
			theStateMachine_.getStateName(failedEvent.getFromState()) <<
			" to " << failedEvent.getToState() << "-" <<
			theStateMachine_.getStateName(failedEvent.getToState()) <<
			".\n\nException:\n" << failedEvent.getException().what() << std::endl;
	__COUT_ERR__ << "\n" << ss.str();

	theStateMachine_.setErrorMessage(ss.str());

	//move everything else to Error!
	broadcastMessage(SOAPUtilities::makeSOAPMessageReference("Error"));
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// FSM State Transition Functions //////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


//========================================================================================================================
void GatewaySupervisor::transitionConfiguring(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	//theProgressBar_.resetProgressBar(0);

	theProgressBar_.step();

	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;

	std::string systemAlias = SOAPUtilities::translate(
			theStateMachine_.getCurrentMessage()).getParameters().getValue("ConfigurationAlias");

	__COUT__ << "Transition parameter: " << systemAlias << std::endl;

	theProgressBar_.step();


	try
	{
		theConfigurationManager_->init(); //completely reset to re-align with any changes
	}
	catch(...)
	{
		__SS__ << "\nTransition to Configuring interrupted! " <<
				"The Configuration Manager could not be initialized." << std::endl;

		__COUT_ERR__ << "\n" << ss.str();
		XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
		return;
	}

	theProgressBar_.step();

	//Translate the system alias to a group name/key
	try
	{
		theConfigurationGroup_ = theConfigurationManager_->getConfigurationGroupFromAlias(systemAlias);
	}
	catch(...)
	{
		__COUT_INFO__ << "Exception occurred" << std::endl;
	}

	theProgressBar_.step();

	if(theConfigurationGroup_.second.isInvalid())
	{
		__SS__ << "\nTransition to Configuring interrupted! System Alias " <<
				systemAlias << " could not be translated to a group name and key." << std::endl;

		__COUT_ERR__ << "\n" << ss.str();
		XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
		return;
	}

	theProgressBar_.step();

	__COUT__ << "Configuration group name: " << theConfigurationGroup_.first << " key: " <<
			theConfigurationGroup_.second << std::endl;

	//make logbook entry
	{
		std::stringstream ss;
		ss << "Configuring '" << systemAlias << "' which translates to " <<
				theConfigurationGroup_.first << " (" << theConfigurationGroup_.second << ").";
		makeSystemLogbookEntry(ss.str());
	}

	theProgressBar_.step();

	//load and activate
	try
	{
		theConfigurationManager_->loadConfigurationGroup(theConfigurationGroup_.first, theConfigurationGroup_.second, true);

		//When configured, set the translated System Alias to be persistently active
		ConfigurationManagerRW tmpCfgMgr("TheSupervisor");
		tmpCfgMgr.activateConfigurationGroup(theConfigurationGroup_.first, theConfigurationGroup_.second);
	}
	catch(...)
	{
		__SS__ << "\nTransition to Configuring interrupted! System Alias " <<
				systemAlias << " was translated to " << theConfigurationGroup_.first <<
				" (" << theConfigurationGroup_.second << ") but could not be loaded and initialized." << std::endl;
		ss << "\n\nTo debug this problem, try activating this group in the Configuration GUI " <<
				" and detailed errors will be shown." << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
		return;
	}

	//check if configuration dump is enabled on configure transition
	{
		ConfigurationTree configLinkNode = theConfigurationManager_->getSupervisorConfigurationNode(
				supervisorContextUID_, supervisorApplicationUID_);
		if(!configLinkNode.isDisconnected())
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
				catch(std::runtime_error &e)
				{
					__COUT_INFO__ << "FSM configuration dump Link disconnected." << std::endl;
					dumpConfiguration = false;
				}

				if(dumpConfiguration)
				{
					//dump configuration
					theConfigurationManager_->dumpActiveConfiguration(
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
			catch(std::runtime_error &e) {
				__SS__ << "\nTransition to Configuring interrupted! There was an error identified " <<
						"during the configuration dump attempt:\n\n " <<
						e.what() << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
				return;
			}
			catch(...) {
				__SS__ << "\nTransition to Configuring interrupted! There was an error identified " <<
						"during the configuration dump attempt.\n\n " <<
						std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
				return;
			}
		}
	}

	theProgressBar_.step();
	SOAPParameters parameters;
	parameters.addParameter("ConfigurationGroupName", theConfigurationGroup_.first);
	parameters.addParameter("ConfigurationGroupKey", theConfigurationGroup_.second.toString());

	//xoap::MessageReference message = SOAPUtilities::makeSOAPMessageReference(SOAPUtilities::translate(theStateMachine_.getCurrentMessage()).getCommand(), parameters);
	xoap::MessageReference message = theStateMachine_.getCurrentMessage();
	SOAPUtilities::addParameters(message,parameters);
	broadcastMessage(message);
	theProgressBar_.step();
	// Advertise the exiting of this method
	//diagService_->reportError("GatewaySupervisor::stateConfiguring: Exiting",DIAGINFO);

	//save last configured group name/key
	saveGroupNameAndKey(theConfigurationGroup_,FSM_LAST_CONFIGURED_GROUP_ALIAS_FILE);

	__COUT__ << "Done" << std::endl;
	theProgressBar_.complete();
}

//========================================================================================================================
void GatewaySupervisor::transitionHalting(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;

	makeSystemLogbookEntry("Run halting.");

	broadcastMessage(theStateMachine_.getCurrentMessage());
}

//========================================================================================================================
void GatewaySupervisor::transitionShuttingDown(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;

	makeSystemLogbookEntry("System shutting down.");

	//kill all non-gateway contexts
	launchStartOTSCommand("OTS_APP_SHUTDOWN");

	//broadcastMessage(theStateMachine_.getCurrentMessage());
}

//========================================================================================================================
void GatewaySupervisor::transitionStartingUp(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;

	makeSystemLogbookEntry("System starting up.");

	//start all non-gateway contexts
	launchStartOTSCommand("OTS_APP_STARTUP");
	sleep(5);
	//transitionInitializing(e);
	//broadcastMessage(theStateMachine_.getCurrentMessage());
}

//========================================================================================================================
void GatewaySupervisor::transitionInitializing(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << theStateMachine_.getCurrentStateName() << std::endl;

	launchStartOTSCommand("OTS_APP_STARTUP");
	sleep(5);

	if(!broadcastMessage(theStateMachine_.getCurrentMessage()))
	{
		__COUT__ << "I can't Initialize the supervisors!" << std::endl;
	}

	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;
	__COUT__ << "Fsm current transition: " << theStateMachine_.getCurrentTransitionName(e->type()) << std::endl;
	__COUT__ << "Fsm final state: " << theStateMachine_.getTransitionFinalStateName(e->type()) << std::endl;
}

//========================================================================================================================
void GatewaySupervisor::transitionPausing(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;

	makeSystemLogbookEntry("Run pausing.");

	broadcastMessage(theStateMachine_.getCurrentMessage());
}

//========================================================================================================================
void GatewaySupervisor::transitionResuming(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;

	makeSystemLogbookEntry("Run resuming.");

	broadcastMessage(theStateMachine_.getCurrentMessage());
}

//========================================================================================================================
void GatewaySupervisor::transitionStarting(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;

	SOAPParameters parameters("RunNumber");
	receive(theStateMachine_.getCurrentMessage(), parameters);

	std::string runNumber = parameters.getValue("RunNumber");
	__COUT__ << runNumber << std::endl;

	//check if configuration dump is enabled on configure transition
	{
		ConfigurationTree configLinkNode = theConfigurationManager_->getSupervisorConfigurationNode(
				supervisorContextUID_, supervisorApplicationUID_);
		if(!configLinkNode.isDisconnected())
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
				catch(std::runtime_error &e)
				{
					__COUT_INFO__ << "FSM configuration dump Link disconnected." << std::endl;
					dumpConfiguration = false;
				}

				if(dumpConfiguration)
				{
					//dump configuration
					theConfigurationManager_->dumpActiveConfiguration(
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
			catch(std::runtime_error &e) {
				__SS__ << "\nTransition to Running interrupted! There was an error identified " <<
						"during the configuration dump attempt:\n\n " <<
						e.what() << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
				return;
			}
			catch(...) {
				__SS__ << "\nTransition to Running interrupted! There was an error identified " <<
						"during the configuration dump attempt.\n\n " <<
						std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
				return;
			}
		}
	}


	makeSystemLogbookEntry("Run " + runNumber + " starting.");

	broadcastMessage(theStateMachine_.getCurrentMessage());

	//save last started group name/key
	saveGroupNameAndKey(theConfigurationGroup_,FSM_LAST_STARTED_GROUP_ALIAS_FILE);
}

//========================================================================================================================
void GatewaySupervisor::transitionStopping(toolbox::Event::Reference e)
throw (toolbox::fsm::exception::Exception)
{
	__COUT__ << "Fsm current state: " << theStateMachine_.getCurrentStateName() << std::endl;

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
throw (toolbox::fsm::exception::Exception)
{
	//transition of Gateway Supervisor is assumed successful so update status
	allSupervisorInfo_.setSupervisorStatus(this,
			theStateMachine_.getCurrentStateName());

	std::string command = SOAPUtilities::translate(message).getCommand();
	bool proceed = true;
	std::string reply;

	__COUT__ << "=========> Broadcasting state machine command = " << command << __E__;

	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
	// Send a SOAP message to every Supervisor in order by priority
	for(auto& it: allSupervisorInfo_.getOrderedSupervisorDescriptors(command))
	{
		const SupervisorInfo& appInfo = *it;

		RunControlStateMachine::theProgressBar_.step();

		__COUT__ << "Sending message to Supervisor " << appInfo.getName() << " [LID=" <<
				appInfo.getId() << "]: " << command << std::endl;
		__COUT__ << "Sending message to Supervisor " << appInfo.getName() << " [LID=" <<
				appInfo.getId() << "]: " << command << std::endl;
		__COUT__ << "Sending message to Supervisor " << appInfo.getName() << " [LID=" <<
				appInfo.getId() << "]: " << command << std::endl;
		__COUT__ << "Sending message to Supervisor " << appInfo.getName() << " [LID=" <<
				appInfo.getId() << "]: " << command << std::endl;

		try
		{
			reply = send(appInfo.getDescriptor(), message);
		}
		catch(const xdaq::exception::Exception &e) //due to xoap send failure
		{
			//do not kill whole system if xdaq xoap failure
			__SS__ << "Error! Gateway Supervisor can NOT " << command << " Supervisor instance = '" <<
					appInfo.getName() << "' [LID=" <<
					appInfo.getId() << "] in Context '" <<
					appInfo.getContextName() << "' [URL=" <<
					appInfo.getURL() <<
					"].\n\n" <<
					"Xoap message failure. Did the target Supervisor crash? Try re-initializing or restarting otsdaq." << std::endl;
			__COUT_ERR__ << ss.str();
			__MOUT_ERR__ << ss.str();

			try
			{
				__COUT__ << "Try again.." << __E__;
				reply = send(appInfo.getDescriptor(), message);
			}
			catch(const xdaq::exception::Exception &e) //due to xoap send failure
			{
				__COUT__ << "Failed.." << __E__;
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
			}
			__COUT__ << "2nd passed.." << __E__;
			proceed = false;
		}

		if (reply != command + "Done")
		{
			__SS__ << "Error! Gateway Supervisor can NOT " << command << " Supervisor instance = '" <<
					appInfo.getName() << "' [LID=" <<
					appInfo.getId() << "] in Context '" <<
					appInfo.getContextName() <<  "' [URL=" <<
					appInfo.getURL() <<
					"].\n\n" <<
					reply;
			__COUT_ERR__ << ss.str() << std::endl;
			__MOUT_ERR__ << ss.str() << std::endl;

			__COUT__ << "Getting error message..." << std::endl;
			try
			{
				xoap::MessageReference errorMessage = sendWithSOAPReply(appInfo.getDescriptor(),
						SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
				SOAPParameters parameters;
				parameters.addParameter("ErrorMessage");
				SOAPMessenger::receive(errorMessage, parameters);

				std::string error = parameters.getValue("ErrorMessage");
				if(error == "")
				{
					std::stringstream err;
					err << "Unknown error from Supervisor instance = '" <<
						appInfo.getName() << "' [LID=" <<
						appInfo.getId() << "] in Context '" <<
						appInfo.getContextName() <<  "' [URL=" <<
						appInfo.getURL() <<
						"]. If the problem persists or is repeatable, please notify admins.\n\n";
					error = err.str();
				}

				__SS__ << "Received error from Supervisor instance = '" <<
						appInfo.getName() << "' [LID=" <<
						appInfo.getId() << "] in Context '" <<
						appInfo.getContextName() << "' [URL=" <<
						appInfo.getURL() <<
						"].\n\n Error Message = " << error << std::endl;

				__COUT_ERR__ << ss.str() << std::endl;
				__MOUT_ERR__ << ss.str() << std::endl;

				if(command == "Error") continue; //do not throw exception and exit loop if informing all apps about error
				//else throw exception and go into Error
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());

				proceed = false;
			}
			catch(const xdaq::exception::Exception &e) //due to xoap send failure
			{
				//do not kill whole system if xdaq xoap failure
				__SS__ << "Error! Gateway Supervisor failed to read error message from Supervisor instance = '" <<
						appInfo.getName() << "' [LID=" <<
						appInfo.getId() << "] in Context '" <<
						appInfo.getContextName() <<  "' [URL=" <<
						appInfo.getURL() <<
						"].\n\n" <<
						"Xoap message failure. Did the target Supervisor crash? Try re-initializing or restarting otsdaq." << std::endl;
				__COUT_ERR__ << ss.str();
				__MOUT_ERR__ << ss.str();
				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());

				proceed = false;
			}
		}
		else
		{
			__COUT__ << "Supervisor instance = '" <<
					appInfo.getName() << "' [LID=" <<
					appInfo.getId() << "] in Context '" <<
					appInfo.getContextName() <<  "' [URL=" <<
					appInfo.getURL() <<
					"] was " << command << "'d correctly!" << std::endl;
		}

		if(!proceed)
		{
			__COUT__ << "Breaking out of loop." << __E__;
			break;
		}
	}

	return proceed;
//
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	// Send a SOAP message to FESupervisor
//	for(auto& it: allSupervisorInfo_.getFEDescriptors())
//	{
//		RunControlStateMachine::theProgressBar_.step();
//		__COUT__ << "Sending message to FESupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to FESupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to FESupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to FESupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to FESupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//
//		try
//		{
//			reply = send(it.second, message);
//		}
//		catch(const xdaq::exception::Exception &e) //due to xoap send failure
//		{
//			//do not kill whole system if xdaq xoap failure
//			__SS__ << "Can NOT " << command << " FESupervisors, instance = " << it.first << ".\n\n" <<
//					"Xoap failure. Did the target GatewaySupervisor crash? Try restarting re-initializing or restarting otsdaq." << std::endl;
//			__COUT_ERR__ << ss.str();
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//		}
//
//		if (reply != command + "Done")
//		{
//			//diagService_->reportError("FESupervisor supervisor "+stringF(it->first) + " could not be initialized!",DIAGFATAL);
//
//			std::string appUID;
//			appUID = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration)->getApplicationUID
//							(
//									it.second->getContextDescriptor()->getURL(),
//									it.second->getLocalId()
//							);
//
//
//			__SS__ << "Can NOT " << command << " FESupervisor '" <<
//					appUID << "'" <<
//					", instance = " << it.first << ".\n\n" <<
//					reply;
//			__COUT_ERR__ << ss.str() << std::endl;
//
//			__COUT__ << "Getting error message..." << std::endl;
//			xoap::MessageReference errorMessage = sendWithSOAPReply(it.second, SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
//			SOAPParameters parameters;
//			parameters.addParameter("ErrorMessage");
//			SOAPMessenger::receive(errorMessage, parameters);
//			__COUT_ERR__ << "errorMessage = " << parameters.getValue("ErrorMessage") << std::endl;
//			ss << "\n\nError Message: " << parameters.getValue("ErrorMessage") << std::endl;
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//			//}
//		}
//		else
//		{
//			__COUT__ << "FESupervisor supervisor " << (it.first) << " was " << command << "'d correctly!" << std::endl;
//		}
//	}
//
//	for(auto& it: allSupervisorInfo_.getDTCDescriptors())
//	{
//		RunControlStateMachine::theProgressBar_.step();
//		__COUT__ << "Sending message to DTCSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to DTCSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to DTCSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to DTCSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to DTCSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//
//		try
//		{
//			reply = send(it.second, message);
//		}
//		catch(const xdaq::exception::Exception &e) //due to xoap send failure
//		{
//			//do not kill whole system if xdaq xoap failure
//			__SS__ << "Can NOT " << command << " DTCSupervisors, instance = " << it.first << ".\n\n" <<
//					"Xoap failure. Did the target GatewaySupervisor crash? Try restarting re-initializing or restarting otsdaq." << std::endl;
//			__COUT_ERR__ << ss.str();
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//		}
//
//		if (reply != command + "Response")
//		{
//			//diagService_->reportError("DTCSupervisor supervisor "+stringF(it->first) + " could not be initialized!",DIAGFATAL);
//
//			__SS__ << "Can NOT " << command << " DTCSupervisor, instance = " << it.first << ".\n\n" <<
//					reply;
//			__COUT_ERR__ << ss.str() << std::endl;
//
//			//xoap::MessageReference errorMessage = sendWithSOAPReply(it.second, SOAPUtilities::makeSOAPMessageReference("Special" + command));
//
//
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//			//}
//		}
//		else
//		{
//			__COUT__ << "DTCSupervisor supervisor " << (it.first) << " was " << command << "'d correctly!" << std::endl;
//		}
//	}
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	for(auto& it: allSupervisorInfo_.getDataManagerDescriptors())
//	{
//		RunControlStateMachine::theProgressBar_.step();
//		__COUT__ << "Sending message to DataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to DataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to DataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to DataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to DataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//
//		try
//		{
//			reply = send(it.second, message);
//		}
//		catch(const xdaq::exception::Exception &e) //due to xoap send failure
//		{
//			//do not kill whole system if xdaq xoap failure
//			__SS__ << "Can NOT " << command << " DataManagerSupervisors, instance = " << it.first << ".\n\n" <<
//					"Xoap failure. Did the target GatewaySupervisor crash? Try restarting re-initializing or restarting otsdaq." << std::endl;
//			__COUT_ERR__ << ss.str();
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//		}
//
//		if (reply != command + "Done")
//		{
//			__SS__ << "Can NOT " << command << " DataManagerSupervisor, instance = " << it.first << ".\n\n" <<
//					reply;
//			__COUT_ERR__ << ss.str() << std::endl;
//			__COUT__ << "Getting error message..." << std::endl;
//			xoap::MessageReference errorMessage = sendWithSOAPReply(it.second, SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
//			SOAPParameters parameters;
//			parameters.addParameter("ErrorMessage");
//			SOAPMessenger::receive(errorMessage, parameters);
//			__COUT_ERR__ << "errorMessage = " << parameters.getValue("ErrorMessage") << std::endl;
//			ss << "\n\nError Message: " << parameters.getValue("ErrorMessage") << std::endl;
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//		}
//		else
//		{
//			__COUT__ << "DataManagerSupervisor " << (it.first) << " was " << command << "'d correctly!" << std::endl;
//		}
//	}
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	for (auto& it: allSupervisorInfo_.getFEDataManagerDescriptors())
//	{
//		RunControlStateMachine::theProgressBar_.step();
//		__COUT__ << "Sending message to FEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to FEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to FEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to FEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to FEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//
//		try
//		{
//			reply = send(it.second, message);
//		}
//		catch(const xdaq::exception::Exception &e) //due to xoap send failure
//		{
//			//do not kill whole system if xdaq xoap failure
//			__SS__ << "Can NOT " << command << " FEDataManagerSupervisors, instance = " << it.first << ".\n\n" <<
//					"Xoap failure. Did the target GatewaySupervisor crash? Try restarting re-initializing or restarting otsdaq." << std::endl;
//			__COUT_ERR__ << ss.str();
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//		}
//
//		if (reply != command + "Done")
//		{
//			__SS__ << "Can NOT " << command << " FEDataManagerSupervisor, instance = " << it.first << ".\n\n" <<
//					reply;
//			__COUT_ERR__ << ss.str() << std::endl;
//			__COUT__ << "Getting error message..." << std::endl;
//			xoap::MessageReference errorMessage = sendWithSOAPReply(it.second, SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
//			SOAPParameters parameters;
//			parameters.addParameter("ErrorMessage");
//			SOAPMessenger::receive(errorMessage, parameters);
//			__COUT_ERR__ << "errorMessage = " << parameters.getValue("ErrorMessage") << std::endl;
//			ss << "\n\nError Message: " << parameters.getValue("ErrorMessage") << std::endl;
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//		}
//		else
//		{
//			__COUT__ << "FEDataManagerSupervisor " << (it.first) << " was " << command << "'d correctly!" << std::endl;
//		}
//	}
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	for(auto& it: allSupervisorInfo_.getVisualDescriptors())
//	{
//		RunControlStateMachine::theProgressBar_.step();
//		__COUT__ << "Sending message to VisualSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to VisualSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to VisualSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to VisualSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to VisualSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//
//
//		try
//		{
//			reply = send(it.second, message);
//		}
//		catch(const xdaq::exception::Exception &e) //due to xoap send failure
//		{
//			//do not kill whole system if xdaq xoap failure
//			__SS__ << "Can NOT " << command << " VisualSupervisor, instance = " << it.first << ".\n\n" <<
//					"Xoap failure. Did the target GatewaySupervisor crash? Try restarting re-initializing or restarting otsdaq." << std::endl;
//			__COUT_ERR__ << ss.str();
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//		}
//
//
//		if (reply != command + "Done")
//		{
//			__SS__ << "Can NOT " << command << " VisualSupervisor, instance = " << it.first << ".\n\n" <<
//					reply;
//			__COUT_ERR__ << ss.str() << std::endl;
//			__COUT__ << "Getting error message..." << std::endl;
//			xoap::MessageReference errorMessage = sendWithSOAPReply(it.second, SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
//			SOAPParameters parameters;
//			parameters.addParameter("ErrorMessage");
//			SOAPMessenger::receive(errorMessage, parameters);
//			__COUT_ERR__ << "errorMessage = " << parameters.getValue("ErrorMessage") << std::endl;
//			ss << "\n\nError Message: " << parameters.getValue("ErrorMessage") << std::endl;
//			XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//			proceed = false;
//		}
//		else
//		{
//			__COUT__ << "VisualSupervisor supervisor " << (it.first) << " was " << command << "'d correctly!" << std::endl;
//		}
//	}
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//
//	bool artdaqRestarted = false;
//	bool artdaqWasRestarted = false;
//
//	RunControlStateMachine::theProgressBar_.step();
//	if(command == "Halt" || command == "Initialize")
//	{
//		//FIXME -- temporary solution for keeping artdaq mpi alive through reconfiguring
//		//	Steps: (if Halt or Initialize
//		//		- Restart
//		//		- Send Initialize
//		//	this will place artdaq supervisors in Halt state, same as others
//
//		{ //MPI relaunch in StartOTS.sh
//
//			__COUT__ << "Extracting target context hostnames... " << std::endl;
//			std::vector<std::string> hostnames;
//			try
//			{
//				theConfigurationManager_->init(); //completely reset to re-align with any changes
//
//				const XDAQContextConfiguration* contextConfiguration = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration);
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
//					__COUT__ << "hostname = " << hostnames.back() << std::endl;
//				}
//			}
//			catch(...)
//			{
//				__SS__ << "\nTransition to Configuring interrupted! " <<
//						"The Configuration Manager could not be initialized." << std::endl;
//
//				__COUT_ERR__ << "\n" << ss.str();
//
//				XCEPT_RAISE (toolbox::fsm::exception::Exception, ss.str());
//			}
//
//			for(const auto& hostname: hostnames)
//			{
//				std::string fn = (std::string(getenv("SERVICE_DATA_PATH")) +
//						"/StartOTS_action_" + hostname + ".cmd");
//				FILE* fp = fopen(fn.c_str(),"w");
//				if(fp)
//				{
//					fprintf(fp,"RESET_MPI");
//					fclose(fp);
//				}
//				else
//					__COUT_ERR__ << "Unable to open command file: " << fn << std::endl;
//			}
//
//			//
//			//			FILE *fp = fopen((std::string(getenv("SERVICE_DATA_PATH")) +
//			//					"/StartOTS_action.cmd").c_str(),"w");
//			//			if(fp)
//			//			{
//			//				fprintf(fp,"RESET_MPI");
//			//				fclose(fp);
//			//
//			//			}
//		} //end mpi reset
//
//		artdaqRestarted = true;
//		//change message command to Initialize
//		SOAPParameters parameters;
//		message = SOAPUtilities::makeSOAPMessageReference(
//				"Initialize", parameters);
//		command = SOAPUtilities::translate(message).getCommand();
//		__COUT__ << "command now is " << command << std::endl;
//	}
//	RunControlStateMachine::theProgressBar_.step();
//
//
//	int MAX_ARTDAQ_RESTARTS = 10;
//	int ARTDAQ_RESTART_DELAY = 2;
//	int artdaqRestartCount = 0;
//	bool preArtdaqProceed = proceed;
//
//	ARTDAQ_RETRY: //label to jump back for artdaq retry
//	if(artdaqWasRestarted) //wait longer
//	{
//		++artdaqRestartCount;
//		proceed = preArtdaqProceed; //reset for each try
//		if(artdaqRestartCount < MAX_ARTDAQ_RESTARTS)
//			artdaqWasRestarted = false; //allow another restart
//		for(int i=0;i<ARTDAQ_RESTART_DELAY;++i)
//		{
//			sleep(1);
//			__COUT_INFO__ << "Waiting on artdaq reboot... " << i << " for " << artdaqRestartCount << "x" << std::endl;
//		}
//	}
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	for(auto& it: allSupervisorInfo_.getARTDAQFEDataManagerDescriptors())
//	{
//		RunControlStateMachine::theProgressBar_.step();
//		__COUT__ << "Sending message to ARTDAQFEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQFEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQFEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQFEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQFEDataManagerSupervisors: " << it.second->getLocalId() << " : " << command << std::endl;
//
//		try
//		{
//			reply = send(it.second, message);
//			if (reply != command + "Done")
//			{
//				__SS__ << "Can NOT " << command << " ARTDAQFEDataManagerSupervisor, instance = " << it.first << ".\n\n" <<
//						reply;
//				__COUT_ERR__ << ss.str() << std::endl;
//				__COUT__ << "Getting error message..." << std::endl;
//				xoap::MessageReference errorMessage = sendWithSOAPReply(it.second, SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
//				SOAPParameters parameters;
//				parameters.addParameter("ErrorMessage");
//				SOAPMessenger::receive(errorMessage, parameters);
//				__COUT_ERR__ << "errorMessage = " << parameters.getValue("ErrorMessage") << std::endl;
//				ss << "\n\nError Message: " << parameters.getValue("ErrorMessage") << std::endl;
//				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//				proceed = false;
//			}
//			else
//			{
//				__COUT__ << "ARTDAQFEDataManagerSupervisors supervisor " << (it.first) << " was " << command << "'d correctly!" << std::endl;
//			}
//		}
//		catch(const xdaq::exception::Exception &e) //due to xoap send failure
//		{
//			if(artdaqRestarted && !artdaqWasRestarted)
//			{
//				artdaqWasRestarted = true;
//				goto ARTDAQ_RETRY;
//			}
//			else
//			{
//				//do not kill whole system if xdaq xoap failure
//				__SS__ << "Can NOT " << command << " ARTDAQFEDataManagerSupervisor, instance = " << it.first << ".\n\n" <<
//						"Xoap failure. Did the target GatewaySupervisor crash? Try restarting re-initializing or restarting otsdaq." << std::endl;
//				__COUT_ERR__ << ss.str();
//				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//				proceed = false;
//			}
//		}
//		catch(const toolbox::fsm::exception::Exception &e) //due to state machine failure
//		{
//			if(artdaqRestarted && !artdaqWasRestarted)
//			{
//				artdaqWasRestarted = true;
//				goto ARTDAQ_RETRY;
//			}
//			else
//				throw;
//		}
//	}
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	// Whe artdaq FEs exist
//	//	for(auto& it: allSupervisorInfo_.getARTDAQFEDescriptors())
//	//	{
//	// ...
//	//}
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	for(auto& it: allSupervisorInfo_.getARTDAQDataManagerDescriptors())
//	{
//		RunControlStateMachine::theProgressBar_.step();
//		__COUT__ << "Sending message to ARTDAQDataManagerSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQDataManagerSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQDataManagerSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQDataManagerSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQDataManagerSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		try
//		{
//			reply = send(it.second, message);
//			if (reply != command + "Done")
//			{
//				__SS__ << "Can NOT " << command << " ARTDAQDataManagerSupervisor, instance = " << it.first << ".\n\n" <<
//						reply;
//				__COUT_ERR__ << ss.str() << std::endl;
//				__COUT__ << "Getting error message..." << std::endl;
//				xoap::MessageReference errorMessage = sendWithSOAPReply(it.second, SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
//				SOAPParameters parameters;
//				parameters.addParameter("ErrorMessage");
//				SOAPMessenger::receive(errorMessage, parameters);
//				__COUT_ERR__ << "errorMessage = " << parameters.getValue("ErrorMessage") << std::endl;
//				ss << "\n\nError Message: " << parameters.getValue("ErrorMessage") << std::endl;
//				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//				proceed = false;
//			}
//			else
//			{
//				__COUT__ << "ARTDAQDataManagerSupervisor supervisor " << (it.first) << " was " << command << "'d correctly!" << std::endl;
//			}
//		}
//		catch(const xdaq::exception::Exception &e) //due to xoap send failure
//		{
//			if(artdaqRestarted && !artdaqWasRestarted)
//			{
//				artdaqWasRestarted = true;
//				goto ARTDAQ_RETRY;
//			}
//			else
//			{
//				//do not kill whole system if xdaq xoap failure
//				__SS__ << "Can NOT " << command << " ARTDAQDataManagerSupervisor, instance = " << it.first << ".\n\n" <<
//						"Xoap failure. Did the target GatewaySupervisor crash? Try restarting re-initializing or restarting otsdaq." << std::endl;
//				__COUT_ERR__ << ss.str();
//				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//				proceed = false;
//			}
//		}
//		catch(const toolbox::fsm::exception::Exception &e) //due to state machine failure
//		{
//			if(artdaqRestarted && !artdaqWasRestarted)
//			{
//				artdaqWasRestarted = true;
//				goto ARTDAQ_RETRY;
//			}
//			else
//				throw;
//		}
//	}
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	for(auto& it: allSupervisorInfo_.getARTDAQBuilderDescriptors())
//	{
//		RunControlStateMachine::theProgressBar_.step();
//		__COUT__ << "Sending message to ARTDAQBuilderSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQBuilderSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQBuilderSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQBuilderSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQBuilderSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//
//		try
//		{
//			reply = send(it.second, message);
//			if (reply != command + "Done")
//			{
//				__SS__ << "Can NOT " << command << " ARTDAQBuilderSupervisor, instance = " << it.first << ".\n\n" <<
//						reply;
//				__COUT_ERR__ << ss.str() << std::endl;
//				__COUT__ << "Getting error message..." << std::endl;
//				xoap::MessageReference errorMessage = sendWithSOAPReply(it.second, SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
//				SOAPParameters parameters;
//				parameters.addParameter("ErrorMessage");
//				SOAPMessenger::receive(errorMessage, parameters);
//				__COUT_ERR__ << "errorMessage = " << parameters.getValue("ErrorMessage") << std::endl;
//				ss << "\n\nError Message: " << parameters.getValue("ErrorMessage") << std::endl;
//				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//				proceed = false;
//			}
//			else
//			{
//				__COUT__ << "ARTDAQBuilderSupervisor supervisor " << (it.first) << " was " << command << "'d correctly!" << std::endl;
//			}
//		}
//		catch(const xdaq::exception::Exception &e) //due to xoap send failure
//		{
//			if(artdaqRestarted && !artdaqWasRestarted)
//			{
//				artdaqWasRestarted = true;
//				goto ARTDAQ_RETRY;
//			}
//			else
//			{
//				//do not kill whole system if xdaq xoap failure
//				__SS__ << "Can NOT " << command << " ARTDAQBuilderSupervisor, instance = " << it.first << ".\n\n" <<
//						"Xoap failure. Did the target GatewaySupervisor crash? Try restarting re-initializing or restarting otsdaq." << std::endl;
//				__COUT_ERR__ << ss.str();
//				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//				proceed = false;
//			}
//		}
//		catch(const toolbox::fsm::exception::Exception &e) //due to state machine failure
//		{
//			if(artdaqRestarted && !artdaqWasRestarted)
//			{
//				artdaqWasRestarted = true;
//				goto ARTDAQ_RETRY;
//			}
//			else
//				throw;
//		}
//	}
//
//	//:::::::::::::::::::::::::::::::::::::::::::::::::::::
//	for(auto& it: allSupervisorInfo_.getARTDAQAggregatorDescriptors())
//	{
//		RunControlStateMachine::theProgressBar_.step();
//		__COUT__ << "Sending message to ARTDAQAggregatorSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQAggregatorSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQAggregatorSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQAggregatorSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//		__COUT__ << "Sending message to ARTDAQAggregatorSupervisor: " << it.second->getLocalId() << " : " << command << std::endl;
//
//		try
//		{
//			reply = send(it.second, message);
//			if (reply != command + "Done")
//			{
//				__SS__ << "Can NOT " << command << " ARTDAQAggregatorSupervisor, instance = " << it.first << ".\n\n" <<
//						reply;
//				__COUT_ERR__ << ss.str() << std::endl;
//				__COUT__ << "Getting error message..." << std::endl;
//				xoap::MessageReference errorMessage = sendWithSOAPReply(it.second, SOAPUtilities::makeSOAPMessageReference("StateMachineErrorMessageRequest"));
//				SOAPParameters parameters;
//				parameters.addParameter("ErrorMessage");
//				SOAPMessenger::receive(errorMessage, parameters);
//				__COUT_ERR__ << "errorMessage = " << parameters.getValue("ErrorMessage") << std::endl;
//				ss << "\n\nError Message: " << parameters.getValue("ErrorMessage") << std::endl;
//				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//				proceed = false;
//			}
//			else
//			{
//				__COUT__ << "ARTDAQAggregatorSupervisor supervisor " << (it.first) << " was " << command << "'d correctly!" << std::endl;
//			}
//		}
//		catch(const xdaq::exception::Exception &e) //due to xoap send failure
//		{
//			if(artdaqRestarted && !artdaqWasRestarted)
//			{
//				artdaqWasRestarted = true;
//				goto ARTDAQ_RETRY;
//			}
//			else
//			{
//				//do not kill whole system if xdaq xoap failure
//				__SS__ << "Can NOT " << command << " ARTDAQAggregatorSupervisor, instance = " << it.first << ".\n\n" <<
//						"Xoap failure. Did the target GatewaySupervisor crash? Try restarting re-initializing or restarting otsdaq." << std::endl;
//				__COUT_ERR__ << ss.str();
//				XCEPT_RAISE(toolbox::fsm::exception::Exception, ss.str());
//				proceed = false;
//			}
//		}
//		catch(const toolbox::fsm::exception::Exception &e) //due to state machine failure
//		{
//			if(artdaqRestarted && !artdaqWasRestarted)
//			{
//				artdaqWasRestarted = true;
//				goto ARTDAQ_RETRY;
//			}
//			else
//				throw;
//		}
//	}
//
//	return proceed;
}

//========================================================================================================================
void GatewaySupervisor::wait(int milliseconds, std::string who) const
{
	for (int s = 1; s <= milliseconds; s++)
	{
		usleep(1000);

		if (s % 100 == 0)
			__COUT__ << s << " msecs " << who << std::endl;
	}
}

//========================================================================================================================
//LoginRequest
//  handles all users login/logout actions from web GUI.
//  NOTE: there are two ways for a user to be logged out: timeout or manual logout
//      System logbook messages are generated for login and logout
void GatewaySupervisor::loginRequest(xgi::Input * in, xgi::Output * out)
throw (xgi::exception::Exception)
{
	cgicc::Cgicc cgi(in);
	std::string Command = CgiDataUtilities::getData(cgi, "RequestType");
	__COUT__ << "*** Login RequestType = " << Command << std::endl;

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

		std::string sid = theWebUsers_.createNewLoginSession(uuid);

		__COUT__ << "uuid = " << uuid << std::endl;
		__COUT__ << "SessionId = " << sid.substr(0, 10) << std::endl;
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

		__COUT__ << "uuid = " << uuid << std::endl;
		__COUT__ << "Cookie Code = " << cookieCode.substr(0, 10) << std::endl;
		__COUT__ << "jumbledUser = " << jumbledUser.substr(0, 10) << std::endl;

		//If cookie code is good, then refresh and return with display name, else return 0 as CookieCode value
		uid = theWebUsers_.isCookieCodeActiveForLogin(uuid, cookieCode,
				jumbledUser); //after call jumbledUser holds displayName on success

		if (uid == theWebUsers_.NOT_FOUND_IN_DATABASE)
		{
			__COUT__ << "cookieCode invalid" << std::endl;
			jumbledUser = ""; //clear display name if failure
			cookieCode = "0";//clear cookie code if failure
		}
		else
			__COUT__ << "cookieCode is good." << std::endl;

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

		__COUT__ << "jumbledUser = " << jumbledUser.substr(0, 10) << std::endl;
		__COUT__ << "jumbledPw = " << jumbledPw.substr(0, 10) << std::endl;
		__COUT__ << "uuid = " << uuid << std::endl;
		__COUT__ << "nac =-" << newAccountCode << "-" << std::endl;

		uint64_t uid = theWebUsers_.attemptActiveSession(uuid, jumbledUser,
				jumbledPw, newAccountCode); //after call jumbledUser holds displayName on success


		if (uid == theWebUsers_.NOT_FOUND_IN_DATABASE)
		{
			__COUT__ << "cookieCode invalid" << std::endl;
			jumbledUser = ""; //clear display name if failure
			if (newAccountCode != "1")//indicates uuid not found
				newAccountCode = "0";//clear cookie code if failure
		}

		__COUT__ << "new cookieCode = " << newAccountCode.substr(0, 10) << std::endl;

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

		//Log login in logbook for active experiment
		makeSystemLogbookEntry(
				theWebUsers_.getUsersUsername(uid) + " logged in.");
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

		__COUT__ << "CERTIFICATE LOGIN REUEST RECEVIED!!!" << std::endl;
		__COUT__ << "jumbledEmail = " << jumbledEmail << std::endl;
		__COUT__ << "uuid = " << uuid << std::endl;

		uint64_t uid = theWebUsers_.attemptActiveSessionWithCert(uuid, jumbledEmail,
				cookieCode, username); //after call jumbledUser holds displayName on success


		if (uid == theWebUsers_.NOT_FOUND_IN_DATABASE)
		{
			__COUT__ << "cookieCode invalid" << std::endl;
			jumbledEmail = ""; //clear display name if failure
			if (cookieCode != "1")//indicates uuid not found
				cookieCode = "0";//clear cookie code if failure
		}

		__COUT__ << "new cookieCode = " << cookieCode.substr(0, 10) << std::endl;

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

		//Log login in logbook for active experiment
		makeSystemLogbookEntry(
				theWebUsers_.getUsersUsername(uid) + " logged in.");
	}
	else if (Command == "logout")
	{
		std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
		std::string logoutOthers = CgiDataUtilities::postData(cgi,
				"LogoutOthers");

		__COUT__ << "Cookie Code = " << cookieCode.substr(0, 10) << std::endl;
		__COUT__ << "logoutOthers = " << logoutOthers << std::endl;

		uint64_t uid; //get uid for possible system logbook message
		if (theWebUsers_.cookieCodeLogout(cookieCode, logoutOthers == "1", &uid)
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
		__COUT__ << __LINE__ << "\tInvalid Command" << std::endl;
		*out << "0";
	}
}

//========================================================================================================================
void GatewaySupervisor::tooltipRequest(xgi::Input * in, xgi::Output * out)
throw (xgi::exception::Exception)
{
	cgicc::Cgicc cgi(in);

	std::string Command = CgiDataUtilities::getData(cgi, "RequestType");
	__COUT__ << "Tooltip RequestType = " << Command <<  std::endl;

	//**** start LOGIN GATEWAY CODE ***//
	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions, uint64_t uid
	//Else, error message is returned in cookieCode
	//Notes: cookie code not refreshed if RequestType = getSystemMessages
	std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
	uint8_t userPermissions;
	uint64_t uid;

	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode, &userPermissions,
			&uid, "0", false))
	{
		*out << cookieCode;
		return;
	}

	//**** end LOGIN GATEWAY CODE ***//

	HttpXmlDocument xmldoc(cookieCode);

	if(Command == "check")
	{
		WebUsers::tooltipCheckForUsername(
				theWebUsers_.getUsersUsername(uid),
				&xmldoc,
				CgiDataUtilities::getData(cgi, "srcFile"),
				CgiDataUtilities::getData(cgi, "srcFunc"),
				CgiDataUtilities::getData(cgi, "srcId"));
	}
	else if(Command == "setNeverShow")
	{
		WebUsers::tooltipSetNeverShowForUsername(
				theWebUsers_.getUsersUsername(uid),
				&xmldoc,
				CgiDataUtilities::getData(cgi, "srcFile"),
				CgiDataUtilities::getData(cgi, "srcFunc"),
				CgiDataUtilities::getData(cgi, "srcId"),
				CgiDataUtilities::getData(cgi, "doNeverShow") == "1"?true:false,
						CgiDataUtilities::getData(cgi, "temporarySilence") == "1"?true:false);

	}
	else
		__COUT__ << "Command Request, " << Command << ", not recognized." << std::endl;

	xmldoc.outputXmlDocument((std::ostringstream*) out, false, true);
}

//========================================================================================================================
void GatewaySupervisor::request(xgi::Input * in, xgi::Output * out)
throw (xgi::exception::Exception)
{
	//for simplicity assume all commands should be mutually exclusive with iterator thread state machine accesses (really should just be careful with RunControlStateMachine access)
	if(VERBOSE_MUTEX) __COUT__ << "Waiting for FSM access" <<  std::endl;
	std::lock_guard<std::mutex> lock(stateMachineAccessMutex_);
	if(VERBOSE_MUTEX) __COUT__ << "Have FSM access" <<  std::endl;

	cgicc::Cgicc cgi(in);

	std::string Command = CgiDataUtilities::getData(cgi, "RequestType");


	//**** start LOGIN GATEWAY CODE ***//
	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions, uint64_t uid
	//Else, error message is returned in cookieCode
	//Notes: cookie code not refreshed if RequestType = getSystemMessages
	std::string cookieCode = CgiDataUtilities::postData(cgi, "CookieCode");
	uint8_t userPermissions;
	uint64_t uid;
	std::string userWithLock;
	bool refreshCookie = Command != "getSystemMessages" &&
			Command != "getCurrentState" &&
			Command != "gatewayLaunchOTS" &&
			Command != "gatewayLaunchWiz";

	if (!theWebUsers_.cookieCodeIsActiveForRequest(cookieCode, &userPermissions,
			&uid, "0",
			refreshCookie,
			&userWithLock))
	{
		*out << cookieCode;
		return;
	}

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


	HttpXmlDocument xmldoc(cookieCode);

	if (Command == "getSettings")
	{
		std::string accounts = CgiDataUtilities::getData(cgi, "accounts");

		__COUT__ << "Get Settings Request" << std::endl;
		__COUT__ << "accounts = " << accounts << std::endl;
		theWebUsers_.insertSettingsForUser(uid, &xmldoc, accounts == "1");
	}
	else if (Command == "setSettings")
	{
		std::string bgcolor = 	CgiDataUtilities::postData(cgi, "bgcolor");
		std::string dbcolor = 	CgiDataUtilities::postData(cgi, "dbcolor");
		std::string wincolor = 	CgiDataUtilities::postData(cgi, "wincolor");
		std::string layout = 	CgiDataUtilities::postData(cgi, "layout");
		std::string syslayout = CgiDataUtilities::postData(cgi, "syslayout");

		__COUT__ << "Set Settings Request" << std::endl;
		__COUT__ << "bgcolor = " << bgcolor << std::endl;
		__COUT__ << "dbcolor = " << dbcolor << std::endl;
		__COUT__ << "wincolor = " << wincolor << std::endl;
		__COUT__ << "layout = " << layout << std::endl;
		__COUT__ << "syslayout = " << syslayout << std::endl;
		theWebUsers_.changeSettingsForUser(uid, bgcolor, dbcolor, wincolor,
				layout, syslayout);
		theWebUsers_.insertSettingsForUser(uid, &xmldoc, true); //include user accounts
	}
	else if (Command == "accountSettings")
	{
		std::string type = CgiDataUtilities::postData(cgi, "type"); //updateAccount, createAccount, deleteAccount
		int type_int = -1;

		if (type == "updateAccount")
			type_int = 0;
		else if (type == "createAccount")
			type_int = 1;
		else if (type == "deleteAccount")
			type_int = 2;

		std::string username = CgiDataUtilities::postData(cgi, "username");
		std::string displayname = CgiDataUtilities::postData(cgi,
				"displayname");
		std::string email = CgiDataUtilities::postData(cgi, "useremail");
		std::string permissions = CgiDataUtilities::postData(cgi,
				"permissions");
		std::string accounts = CgiDataUtilities::getData(cgi, "accounts");

		__COUT__ << "accountSettings Request" << std::endl;
		__COUT__ << "type = " << type << " - " << type_int << std::endl;
		__COUT__ << "username = " << username << std::endl;
		__COUT__ << "useremail = " << email << std::endl;
		__COUT__ << "displayname = " << displayname << std::endl;
		__COUT__ << "permissions = " << permissions << std::endl;

		theWebUsers_.modifyAccountSettings(uid, type_int, username, displayname,email,
				permissions);

		__COUT__ << "accounts = " << accounts << std::endl;

		theWebUsers_.insertSettingsForUser(uid, &xmldoc, accounts == "1");
	}
	else if(Command == "stateMatchinePreferences")
	{
		std::string set = CgiDataUtilities::getData(cgi, "set");
		const std::string DEFAULT_FSM_VIEW = "Default_FSM_View";
		if(set == "1")
			theWebUsers_.setGenericPreference(uid, DEFAULT_FSM_VIEW,
					CgiDataUtilities::getData(cgi, DEFAULT_FSM_VIEW));
		else
			theWebUsers_.getGenericPreference(uid, DEFAULT_FSM_VIEW, &xmldoc);
	}
	else if(Command == "getAliasList")
	{
		std::string username = theWebUsers_.getUsersUsername(uid);
		std::string fsmName = CgiDataUtilities::getData(cgi, "fsmName");
		__COUT__ << "fsmName = " << fsmName << std::endl;

		std::string stateMachineAliasFilter = "*"; //default to all

		std::map<std::string /*alias*/,
		std::pair<std::string /*group name*/, ConfigurationGroupKey> > aliasMap =
				theConfigurationManager_->getGroupAliasesConfiguration();


		// get stateMachineAliasFilter if possible
		ConfigurationTree configLinkNode = theConfigurationManager_->getSupervisorConfigurationNode(
				supervisorContextUID_, supervisorApplicationUID_);

		if(!configLinkNode.isDisconnected())
		{
			try //for backwards compatibility
			{
				ConfigurationTree fsmLinkNode = configLinkNode.getNode("LinkToStateMachineConfiguration");
				if(!fsmLinkNode.isDisconnected())
					stateMachineAliasFilter =
							fsmLinkNode.getNode(fsmName + "/SystemAliasFilter").getValue<std::string>();
				else
					__COUT_INFO__ << "FSM Link disconnected." << std::endl;
			}
			catch(std::runtime_error &e) { __COUT_INFO__ << e.what() << std::endl; }
			catch(...) { __COUT_ERR__ << "Unknown error. Should never happen." << std::endl; }
		}
		else
			__COUT_INFO__ << "FSM Link disconnected." << std::endl;

		__COUT__ << "stateMachineAliasFilter  = " << stateMachineAliasFilter	<< std::endl;


		//filter list of aliases based on stateMachineAliasFilter
		//  ! as first character means choose those that do NOT match filter
		//	* can be used as wild card.
		{
			bool invertFilter = stateMachineAliasFilter.size() && stateMachineAliasFilter[0] == '!';
			std::vector<std::string> filterArr;

			size_t  i = 0;
			if(invertFilter) ++i;
			size_t  f;
			std::string tmp;
			while((f = stateMachineAliasFilter.find('*',i)) != std::string::npos)
			{
				tmp = stateMachineAliasFilter.substr(i,f-i);
				i = f+1;
				filterArr.push_back(tmp);
				//__COUT__ << filterArr[filterArr.size()-1] << " " << i <<
				//		" of " << stateMachineAliasFilter.size() << std::endl;

			}
			if(i <= stateMachineAliasFilter.size())
			{
				tmp = stateMachineAliasFilter.substr(i);
				filterArr.push_back(tmp);
				//__COUT__ << filterArr[filterArr.size()-1] << " last." << std::endl;
			}


			bool filterMatch;


			for(auto& aliasMapPair : aliasMap)
			{
				//__COUT__ << "aliasMapPair.first: " << aliasMapPair.first << std::endl;

				filterMatch = true;

				if(filterArr.size() == 1)
				{
					if(filterArr[0] != "" &&
							filterArr[0] != "*" &&
							aliasMapPair.first != filterArr[0])
						filterMatch = false;
				}
				else
				{
					i = -1;
					for(f=0;f<filterArr.size();++f)
					{
						if(!filterArr[f].size()) continue; //skip empty filters

						if(f == 0) //must start with this filter
						{
							if((i = aliasMapPair.first.find(filterArr[f])) != 0)
							{
								filterMatch = false;
								break;
							}
						}
						else if(f == filterArr.size()-1) //must end with this filter
						{
							if(aliasMapPair.first.rfind(filterArr[f]) !=
									aliasMapPair.first.size() - filterArr[f].size())
							{
								filterMatch = false;
								break;
							}
						}
						else if((i = aliasMapPair.first.find(filterArr[f])) ==
								std::string::npos)
						{
							filterMatch = false;
							break;
						}
					}
				}

				if(invertFilter) filterMatch = !filterMatch;

				//__COUT__ << "filterMatch=" << filterMatch  << std::endl;

				if(!filterMatch) continue;

				xmldoc.addTextElementToData("config_alias", aliasMapPair.first);
				xmldoc.addTextElementToData("config_key",
						ConfigurationGroupKey::getFullGroupString(aliasMapPair.second.first,
								aliasMapPair.second.second).c_str());

				std::string groupComment, groupAuthor, groupCreationTime;
				try
				{
					theConfigurationManager_->loadConfigurationGroup(
							aliasMapPair.second.first,aliasMapPair.second.second,
							false,0,0,0,
							&groupComment, &groupAuthor, &groupCreationTime, false /*false to not load member map*/);

					xmldoc.addTextElementToData("config_comment", groupComment);
					xmldoc.addTextElementToData("config_author", groupAuthor);
					xmldoc.addTextElementToData("config_create_time", groupCreationTime);
				}
				catch(...)
				{
					__COUT_WARN__ << "Failed to load group metadata." << std::endl;
				}
			}
		}

		//return last group alias
		std::string fn = FSM_LAST_GROUP_ALIAS_PATH + FSM_LAST_GROUP_ALIAS_FILE_START +
				username + "." + FSM_USERS_PREFERENCES_FILETYPE;
		__COUT__ << "Load preferences: " << fn << std::endl;
		FILE *fp = fopen(fn.c_str(),"r");
		if(fp)
		{
			char tmpLastAlias[500];
			fscanf(fp,"%*s %s",tmpLastAlias);
			__COUT__ << "tmpLastAlias: " << tmpLastAlias << std::endl;

			xmldoc.addTextElementToData("UserLastConfigAlias",tmpLastAlias);
			fclose(fp);
		}
	}
	else if (Command == "getFecList")
	{
		xmldoc.addTextElementToData("fec_list", "");

		for (auto it: allSupervisorInfo_.getAllFETypeSupervisorInfo())
		{
			xmldoc.addTextElementToParent("fec_url",
					it.second.getURL(), "fec_list");
			xmldoc.addTextElementToParent(
					"fec_urn",
					std::to_string(it.second.getId()),	"fec_list");
		}
	}
	else if (Command == "getSystemMessages")
	{
		xmldoc.addTextElementToData("systemMessages",
				theSystemMessenger_.getSystemMessage(
						theWebUsers_.getUsersDisplayName(uid)));

		xmldoc.addTextElementToData("username_with_lock",
				theWebUsers_.getUserWithLock()); //always give system lock update

		//__COUT__ << "userWithLock " << theWebUsers_.getUserWithLock() << std::endl;
	}
	else if (Command == "setUserWithLock")
	{
		std::string username = CgiDataUtilities::postData(cgi, "username");
		std::string lock = CgiDataUtilities::postData(cgi, "lock");
		std::string accounts = CgiDataUtilities::getData(cgi, "accounts");

		__COUT__ << Command <<  std::endl;
		__COUT__ << "username " << username <<  std::endl;
		__COUT__ << "lock " << lock <<  std::endl;
		__COUT__ << "accounts " << accounts <<  std::endl;
		__COUT__ << "uid " << uid <<  std::endl;

		std::string tmpUserWithLock = theWebUsers_.getUserWithLock();
		if(!theWebUsers_.setUserWithLock(uid, lock == "1", username))
			xmldoc.addTextElementToData("server_alert",
					std::string("Set user lock action failed. You must have valid permissions and ") +
					"locking user must be currently logged in.");

		theWebUsers_.insertSettingsForUser(uid, &xmldoc, accounts == "1");

		if (tmpUserWithLock != theWebUsers_.getUserWithLock()) //if there was a change, broadcast system message
			theSystemMessenger_.addSystemMessage("*", theWebUsers_.getUserWithLock()
					== "" ? tmpUserWithLock + " has unlocked ots."
							: theWebUsers_.getUserWithLock()
							  + " has locked ots.");
	}
	else if (Command == "getStateMachine")
	{
		// __COUT__ << "Getting state machine" << std::endl;
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
			DOMElement* stateParent = xmldoc.addTextElementToData("state", stateStr);

			xmldoc.addTextElementToParent("state_name", theStateMachine_.getStateName(states[i]), stateParent);

			//__COUT__ << "state: " << states[i] << " - " << theStateMachine_.getStateName(states[i]) << std::endl;

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

				if(stateStr[0] == 'R')
				{
					//addRun = true;
					xmldoc.addTextElementToParent("state_transition", stateStr, stateParent);

					//__COUT__ << states[i] << " => " << *ait << std::endl;

					xmldoc.addTextElementToParent("state_transition_action", *ait, stateParent);

					transName = theStateMachine_.getTransitionName(states[i], *ait);
					//__COUT__ << states[i] << " => " << transName << std::endl;

					xmldoc.addTextElementToParent("state_transition_name",
							transName, stateParent);
					transParameter = theStateMachine_.getTransitionParameter(states[i], *ait);
					//__COUT__ << states[i] << " => " << transParameter<< std::endl;

					xmldoc.addTextElementToParent("state_transition_parameter", transParameter, stateParent);
					break;
				}
				else if(stateStr[0] == 'C')
				{
					//addCfg = true;
					xmldoc.addTextElementToParent("state_transition", stateStr, stateParent);

					//__COUT__ << states[i] << " => " << *ait << std::endl;

					xmldoc.addTextElementToParent("state_transition_action", *ait, stateParent);

					transName = theStateMachine_.getTransitionName(states[i], *ait);
					//__COUT__ << states[i] << " => " << transName << std::endl;

					xmldoc.addTextElementToParent("state_transition_name",
							transName, stateParent);
					transParameter = theStateMachine_.getTransitionParameter(states[i], *ait);
					//__COUT__ << states[i] << " => " << transParameter<< std::endl;

					xmldoc.addTextElementToParent("state_transition_parameter", transParameter, stateParent);
					break;
				}
			}

			//reset for 2nd pass
			it = trans.begin();
			ait = actionNames.begin();

			//other states
			for (; it != trans.end() && ait != actionNames.end(); ++it, ++ait)
			{
				//__COUT__ << states[i] << " => " << it->second << std::endl;

				stateStr[0] = it->second;

				if(stateStr[0] == 'R')
					continue;
				else if(stateStr[0] == 'C')
					continue;

				xmldoc.addTextElementToParent("state_transition", stateStr, stateParent);

				//__COUT__ << states[i] << " => " << *ait << std::endl;

				xmldoc.addTextElementToParent("state_transition_action", *ait, stateParent);

				transName = theStateMachine_.getTransitionName(states[i], *ait);
				//__COUT__ << states[i] << " => " << transName << std::endl;

				xmldoc.addTextElementToParent("state_transition_name",
						transName, stateParent);
				transParameter = theStateMachine_.getTransitionParameter(states[i], *ait);
				//__COUT__ << states[i] << " => " << transParameter<< std::endl;

				xmldoc.addTextElementToParent("state_transition_parameter", transParameter, stateParent);
			}
		}

	}
	else if (Command == "getStateMachineNames")
	{
		//get stateMachineAliasFilter if possible
		ConfigurationTree configLinkNode = theConfigurationManager_->getSupervisorConfigurationNode(
				supervisorContextUID_, supervisorApplicationUID_);

		try
		{
			auto fsmNodes = configLinkNode.getNode(
					"LinkToStateMachineConfiguration").getChildren();
			for(const auto& fsmNode:fsmNodes)
				xmldoc.addTextElementToData("stateMachineName", fsmNode.first);
		}
		catch(...) //else empty set of state machines.. can always choose ""
		{
			__COUT__ << "Caught exception, assuming no valid FSM names." << std::endl;
			xmldoc.addTextElementToData("stateMachineName", "");
		}
	}
	else if (Command == "getIterationPlanStatus")
	{
		//__COUT__ << "checking it status" << std::endl;
		theIterator_.handleCommandRequest(xmldoc,Command,"");
	}
	else if (Command == "getCurrentState")
	{
		xmldoc.addTextElementToData("current_state", theStateMachine_.getCurrentStateName());
		xmldoc.addTextElementToData("in_transition", theStateMachine_.isInTransition() ? "1" : "0");
		if (theStateMachine_.isInTransition())
			xmldoc.addTextElementToData("transition_progress", theProgressBar_.readPercentageString());
		else
			xmldoc.addTextElementToData("transition_progress", "100");


		char tmp[20];
		sprintf(tmp,"%lu",theStateMachine_.getTimeInState());
		xmldoc.addTextElementToData("time_in_state", tmp);



		//__COUT__ << "current state: " << theStateMachine_.getCurrentStateName() << std::endl;


		//// ======================== get run alias based on fsm name ====

		std::string fsmName = CgiDataUtilities::getData(cgi, "fsmName");
		//		__COUT__ << "fsmName = " << fsmName << std::endl;
		//		__COUT__ << "activeStateMachineName_ = " << activeStateMachineName_ << std::endl;
		//		__COUT__ << "theStateMachine_.getProvenanceStateName() = " <<
		//				theStateMachine_.getProvenanceStateName() << std::endl;
		//		__COUT__ << "theStateMachine_.getCurrentStateName() = " <<
		//				theStateMachine_.getCurrentStateName() << std::endl;

		if(!theStateMachine_.isInTransition())
		{
			std::string stateMachineRunAlias = "Run"; //default to "Run"

			// get stateMachineAliasFilter if possible
			ConfigurationTree configLinkNode = theConfigurationManager_->getSupervisorConfigurationNode(
					supervisorContextUID_, supervisorApplicationUID_);

			if(!configLinkNode.isDisconnected())
			{
				try //for backwards compatibility
				{
					ConfigurationTree fsmLinkNode = configLinkNode.getNode("LinkToStateMachineConfiguration");
					if(!fsmLinkNode.isDisconnected())
						stateMachineRunAlias =
								fsmLinkNode.getNode(fsmName + "/RunDisplayAlias").getValue<std::string>();
					//else
					//	__COUT_INFO__ << "FSM Link disconnected." << std::endl;
				}
				catch(std::runtime_error &e)
				{
					//__COUT_INFO__ << e.what() << std::endl;
					//__COUT_INFO__ << "No state machine Run alias. Ignoring and assuming alias of '" <<
					//		stateMachineRunAlias << ".'" << std::endl;

				}
				catch(...) {
					__COUT_ERR__ << "Unknown error. Should never happen." << std::endl;

					__COUT_INFO__ << "No state machine Run alias. Ignoring and assuming alias of '" <<
							stateMachineRunAlias << ".'" << std::endl;
				}
			}
			//else
			//	__COUT_INFO__ << "FSM Link disconnected." << std::endl;

			//__COUT__ << "stateMachineRunAlias  = " << stateMachineRunAlias	<< std::endl;

			xmldoc.addTextElementToData("stateMachineRunAlias", stateMachineRunAlias);


			//// ======================== get run number based on fsm name ====


			if(theStateMachine_.getCurrentStateName() == "Running" ||
					theStateMachine_.getCurrentStateName() == "Paused")
				sprintf(tmp,"Current %s Number: %u",stateMachineRunAlias.c_str(),getNextRunNumber(activeStateMachineName_)-1);
			else
				sprintf(tmp,"Next %s Number: %u",stateMachineRunAlias.c_str(),getNextRunNumber(fsmName));
			xmldoc.addTextElementToData("run_number", tmp);
		}
	}
	else if(Command == "getErrorInStateMatchine")
	{
		xmldoc.addTextElementToData("FSM_Error", theStateMachine_.getErrorMessage());
	}
	else if(Command == "getDesktopIcons")
	{

		std::string iconFileName = ICON_FILE_NAME;
		std::ifstream iconFile;
		std::string iconList = "";
		std::string line;
		iconFile.open(iconFileName.c_str());

		if(!iconFile)
		{
			__COUT__ << "Error opening file: "<< iconFileName << std::endl;
			system("pause");
			return;
		}
		if(iconFile.is_open())
		{
			__COUT__ << "Getting Desktop Icons - opened file: " << iconFileName << std::endl;
			while(std::getline(iconFile, line))
			{
				iconList = line;
			}
			//__COUT__ << iconList << std::endl;

			//Close file
			iconFile.close();
		}
		xmldoc.addTextElementToData("iconList", iconList);

	}
	else if(Command == "gatewayLaunchOTS" || Command == "gatewayLaunchWiz")
	{
		//NOTE: similar to ConfigurationGUI version but DOES keep active sessions

		if(userPermissions != 255)
		{
			__COUT__ << "Insufficient Permissions" << std::endl;
		}
		else
		{
			__COUT_WARN__ << Command << " command received! " << std::endl;
			__MOUT_WARN__ << Command << " command received! " << std::endl;


			//gateway launch is different, in that it saves user sessions
			theWebUsers_.saveActiveSessions();

			//now launch

			if(Command == "gatewayLaunchOTS")
				launchStartOTSCommand("LAUNCH_OTS");
			else if(Command == "gatewayLaunchWiz")
				launchStartOTSCommand("LAUNCH_WIZ");

//			__COUT__ << "Extracting target context hostnames... " << std::endl;
//			std::vector<std::string> hostnames;
//			try
//			{
//				theConfigurationManager_->init(); //completely reset to re-align with any changes
//
//				const XDAQContextConfiguration* contextConfiguration = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration);
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
//					__COUT__ << "hostname = " << hostnames.back() << std::endl;
//				}
//			}
//			catch(...)
//			{
//				__SS__ << "\nRelaunch of otsdaq interrupted! " <<
//						"The Configuration Manager could not be initialized." << std::endl;
//
//				__COUT_ERR__ << "\n" << ss.str();
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
//					if(Command == "gatewayLaunchOTS")
//						fprintf(fp,"LAUNCH_OTS");
//					else if(Command == "gatewayLaunchWiz")
//						fprintf(fp,"LAUNCH_WIZ");
//
//					fclose(fp);
//				}
//				else
//					__COUT_ERR__ << "Unable to open command file: " << fn << std::endl;
//			}
		}
	}
	else if(Command == "resetUserTooltips")
	{
		WebUsers::resetAllUserTooltips(theWebUsers_.getUsersUsername(uid));
	}
	else
		__COUT__ << "Command Request, " << Command << ", not recognized." << std::endl;

	//__COUT__ << "Made it" << std::endl;

	//return xml doc holding server response
	xmldoc.outputXmlDocument((std::ostringstream*) out, false, true); //Note: allow white space need for error response

	//__COUT__ << "done " << Command << std::endl;
}

//========================================================================================================================
//xoap::supervisorGetUserInfo
//	get user info to external supervisors
void GatewaySupervisor::launchStartOTSCommand(const std::string& command)
{
	__COUT__ << "launch StartOTS Command = " << command << __E__;
	__COUT__ << "Extracting target context hostnames... " << std::endl;

	std::vector<std::string> hostnames;
	try
	{
		theConfigurationManager_->init(); //completely reset to re-align with any changes

		const XDAQContextConfiguration* contextConfiguration = theConfigurationManager_->__GET_CONFIG__(XDAQContextConfiguration);

		auto contexts = contextConfiguration->getContexts();
		unsigned int i,j;
		for(const auto& context: contexts)
		{
			if(!context.status_) continue;

			//find last slash
			j=0; //default to whole string
			for(i=0;i<context.address_.size();++i)
				if(context.address_[i] == '/')
					j = i+1;
			hostnames.push_back(context.address_.substr(j));
			__COUT__ << "hostname = " << hostnames.back() << std::endl;
		}
	}
	catch(...)
	{
		__SS__ << "\nRelaunch of otsdaq interrupted! " <<
				"The Configuration Manager could not be initialized." << std::endl;

		__SS_THROW__;
	}

	for(const auto& hostname: hostnames)
	{
		std::string fn = (std::string(getenv("SERVICE_DATA_PATH")) +
				"/StartOTS_action_" + hostname + ".cmd");
		FILE* fp = fopen(fn.c_str(),"w");
		if(fp)
		{
			fprintf(fp,command.c_str());
			fclose(fp);
		}
		else
		{
			__SS__ << "Unable to open command file: " << fn << std::endl;
			__SS_THROW__;
		}
	}
}

//========================================================================================================================
//xoap::supervisorGetUserInfo
//	get user info to external supervisors
xoap::MessageReference GatewaySupervisor::supervisorGetUserInfo(
		xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	SOAPParameters parameters;
	parameters.addParameter("CookieCode");
	receive(message, parameters);
	std::string cookieCode = parameters.getValue("CookieCode");

	std::string username, displayName;
	uint64_t activeSessionIndex;

	theWebUsers_.getUserInfoForCookie(cookieCode, &username, &displayName,
			&activeSessionIndex);

	//__COUT__ << "username " << username << std::endl;
	//__COUT__ << "displayName " << displayName << std::endl;

	//fill return parameters
	SOAPParameters retParameters;
	retParameters.addParameter("Username", username);
	retParameters.addParameter("DisplayName", displayName);
	char tmpStr[100];
	sprintf(tmpStr, "%lu", activeSessionIndex);
	retParameters.addParameter("ActiveSessionIndex", tmpStr);

	return SOAPUtilities::makeSOAPMessageReference("UserInfoResponse",
			retParameters);
}

//========================================================================================================================
//xoap::supervisorCookieCheck
//	verify cookie
xoap::MessageReference GatewaySupervisor::supervisorCookieCheck(xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	//__COUT__ << std::endl;

	//receive request parameters
	SOAPParameters parameters;
	parameters.addParameter("CookieCode");
	parameters.addParameter("RefreshOption");
	receive(message, parameters);
	std::string cookieCode = parameters.getValue("CookieCode");
	std::string refreshOption = parameters.getValue("RefreshOption"); //give external supervisors option to refresh cookie or not, "1" to refresh

	//If TRUE, cookie code is good, and refreshed code is in cookieCode, also pointers optionally for uint8_t userPermissions, uint64_t uid
	//Else, error message is returned in cookieCode
	uint8_t userPermissions = 0;
	std::string userWithLock = "";
	theWebUsers_.cookieCodeIsActiveForRequest(cookieCode, &userPermissions, 0,
			"0", refreshOption == "1", &userWithLock);

	//__COUT__ << "userWithLock " << userWithLock << std::endl;

	//fill return parameters
	SOAPParameters retParameters;
	retParameters.addParameter("CookieCode", cookieCode);
	char tmp[5];
	sprintf(tmp, "%d", userPermissions);
	retParameters.addParameter("Permissions", tmp);
	retParameters.addParameter("UserWithLock", userWithLock);

	//__COUT__ << std::endl;

	return SOAPUtilities::makeSOAPMessageReference("CookieResponse",
			retParameters);
}

//========================================================================================================================
//xoap::supervisorGetActiveUsers
//	get display names for all active users
xoap::MessageReference GatewaySupervisor::supervisorGetActiveUsers(
		xoap::MessageReference message)
throw (xoap::exception::Exception)
{
	__COUT__ << std::endl;

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
throw (xoap::exception::Exception)
{
	SOAPParameters parameters;
	parameters.addParameter("ToUser");
	parameters.addParameter("Message");
	receive(message, parameters);

	__COUT__ << "toUser: " << parameters.getValue("ToUser").substr(
			0, 10) << ", message: " << parameters.getValue("Message").substr(0,
					10) << std::endl;

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
throw (xoap::exception::Exception)
{
	SOAPParameters parameters;
	parameters.addParameter("EntryText");
	receive(message, parameters);

	__COUT__ << "EntryText: " << parameters.getValue("EntryText").substr(
			0, 10) << std::endl;

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
throw (xoap::exception::Exception)
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
			0, 10) << std::endl;

	std::string fileName = "";
	if(action == "Configured")
		fileName = FSM_LAST_CONFIGURED_GROUP_ALIAS_FILE;
	else if(action == "Started")
		fileName = FSM_LAST_STARTED_GROUP_ALIAS_FILE;
	else
	{
		__COUT_ERR__ << "Invalid last group action requested." << std::endl;
		return SOAPUtilities::makeSOAPMessageReference("LastConfigGroupResponseFailure");
	}
	std::string timeString;
	std::pair<std::string /*group name*/, ConfigurationGroupKey> theGroup =
			loadGroupNameAndKey(fileName,timeString);

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
	std::string fsmName = fsmNameIn == ""?activeStateMachineName_:fsmNameIn;
	//prepend sanitized FSM name
	for(unsigned int i=0;i<fsmName.size();++i)
		if(		(fsmName[i] >= 'a' && fsmName[i] <= 'z') ||
				(fsmName[i] >= 'A' && fsmName[i] <= 'Z') ||
				(fsmName[i] >= '0' && fsmName[i] <= '9'))
			runNumberFileName += fsmName[i];
	runNumberFileName += RUN_NUMBER_FILE_NAME;
	//__COUT__ << "runNumberFileName: " << runNumberFileName << std::endl;

	std::ifstream runNumberFile(runNumberFileName.c_str());
	if (!runNumberFile.is_open())
	{
		__COUT__ << "Can't open file: " << runNumberFileName << std::endl;

		__COUT__ << "Creating file and setting Run Number to 1: " << runNumberFileName << std::endl;
		FILE *fp = fopen(runNumberFileName.c_str(),"w");
		fprintf(fp,"1");
		fclose(fp);

		runNumberFile.open(runNumberFileName.c_str());
		if(!runNumberFile.is_open())
		{
			__SS__ << "Error. Can't create file: " << runNumberFileName << std::endl;
			__COUT_ERR__ << ss.str();
			throw std::runtime_error(ss.str());
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
	std::string fsmName = fsmNameIn == ""?activeStateMachineName_:fsmNameIn;
	//prepend sanitized FSM name
	for(unsigned int i=0;i<fsmName.size();++i)
		if(		(fsmName[i] >= 'a' && fsmName[i] <= 'z') ||
				(fsmName[i] >= 'A' && fsmName[i] <= 'Z') ||
				(fsmName[i] >= '0' && fsmName[i] <= '9'))
			runNumberFileName += fsmName[i];
	runNumberFileName += RUN_NUMBER_FILE_NAME;
	__COUT__ << "runNumberFileName: " << runNumberFileName << std::endl;

	std::ofstream runNumberFile(runNumberFileName.c_str());
	if (!runNumberFile.is_open())
	{
		__SS__ << "Can't open file: " << runNumberFileName << std::endl;
		__COUT__ << ss.str();
		throw std::runtime_error(ss.str());
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

	FILE *groupFile = fopen(fullPath.c_str(),"r");
	if (!groupFile)
	{
		__COUT__ << "Can't open file: " << fullPath << std::endl;

		__COUT__ << "Returning empty groupName and key -1" << std::endl;

		return std::pair<std::string /*group name*/,
				ConfigurationGroupKey>("",ConfigurationGroupKey());
	}

	char line[500]; //assuming no group names longer than 500 chars
	//name and then key
	std::pair<std::string /*group name*/,
	ConfigurationGroupKey> theGroup;

	fgets(line,500,groupFile); //name
	theGroup.first = line;

	fgets(line,500,groupFile); //key
	int key;
	sscanf(line,"%d",&key);
	theGroup.second = key;

	fgets(line,500,groupFile); //time
	time_t timestamp;
	sscanf(line,"%ld",&timestamp); //type long int
	struct tm tmstruct;
	::localtime_r(&timestamp, &tmstruct);
	::strftime(line, 30, "%c %Z", &tmstruct);
	returnedTimeString = line;
	fclose(groupFile);


	__COUT__ << "theGroup.first= " << theGroup.first <<
			" theGroup.second= " << theGroup.second << std::endl;

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
		__SS__ << "Error. Can't open file: " << fullPath << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}
	std::stringstream outss;
	outss << theGroup.first << "\n" << theGroup.second << "\n" << time(0);
	groupFile << outss.str().c_str();
	groupFile.close();
}



/////////////////////////////////////////////////////////////////////////////////////


////========================================================================================================================
////GatewaySupervisor::infoRequest ---
//void GatewaySupervisor::infoRequest(xgi::Input * in, xgi::Output * out )  throw (xgi::exception::Exception)
//{
//    __COUT__ << __LINE__ <<  std::endl << std::endl;
//    cgicc::Cgicc cgi(in);
//    std::string Command=cgi.getElement("RequestType")->getValue();
//
//    if (Command=="FEWList")
//   {
//        *out << "<FEWList>" << std::endl;
//        for (std::set<xdaq::ApplicationDescriptor*>::iterator pxFESupervisorsIt=pxFESupervisors_.begin(); pxFESupervisorsIt!=pxFESupervisors_.end(); pxFESupervisorsIt++)
//       {
//        	*out << "<FEWInterface id='1' alias='FPIX Disk 1' type='OtsUDPHardware'>" << std::endl
//        	     << "<svg xmlns='http://www.w3.org/2000/svg' version='1.1'>" <<  std::endl
//                 << "<circle cx='100' cy='50' r='40' stroke='black' stroke-width='2' fill='red' />" << std::endl
//                 << "</svg>" << std::endl
//                 << "</FEWInterface>" << std::endl;
//         	*out << "<FEWInterface id='2' alias='FPIX Disk 2' type='OtsUDPHardware'>" << std::endl
//        	     << "<svg xmlns='http://www.w3.org/2000/svg' version='1.1'>" <<  std::endl
//                 << "<circle cx='100' cy='50' r='40' stroke='black' stroke-width='2' fill='red' />" << std::endl
//                 << "</svg>" << std::endl
//                 << "</FEWInterface>" << std::endl;
//        }
//        *out << "</FEWList>" << std::endl;
//    }
//}
//========================================================================================================================

