#include "otsdaq-core/WizardSupervisor/WizardSupervisor.h"

#include "otsdaq-core/GatewaySupervisor/GatewaySupervisor.h"

#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include <xdaq/NamespaceURI.h>
#include "otsdaq-core/CgiDataUtilities/CgiDataUtilities.h"
#include "otsdaq-core/SOAPUtilities/SOAPCommand.h"
#include "otsdaq-core/SOAPUtilities/SOAPUtilities.h"
#include "otsdaq-core/WebUsersUtilities/WebUsers.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"

#include <dirent.h>    //for DIR
#include <sys/stat.h>  // mkdir
#include <chrono>      // std::chrono::seconds
#include <fstream>
#include <iostream>
#include <string>
#include <thread>  // std::this_thread::sleep_for

using namespace ots;

#define SECURITY_FILE_NAME \
	std::string(__ENV__("SERVICE_DATA_PATH")) + "/OtsWizardData/security.dat"
#define SEQUENCE_FILE_NAME \
	std::string(__ENV__("SERVICE_DATA_PATH")) + "/OtsWizardData/sequence.dat"
#define SEQUENCE_OUT_FILE_NAME \
	std::string(__ENV__("SERVICE_DATA_PATH")) + "/OtsWizardData/sequence.out"
#define USER_DATA_PATH std::string(__ENV__("SERVICE_DATA_PATH")) + std::string("/")
//#define LOGBOOK_PREVIEWS_PATH 			"uploads/"

#define XML_STATUS "editUserData_status"

#define XML_ADMIN_STATUS "logbook_admin_status"
#define XML_MOST_RECENT_DAY "most_recent_day"
#define XML_EXPERIMENTS_ROOT "experiments"
#define XML_EXPERIMENT "experiment"
#define XML_ACTIVE_EXPERIMENT "active_experiment"
#define XML_EXPERIMENT_CREATE "create_time"
#define XML_EXPERIMENT_CREATOR "creator"

#define XML_LOGBOOK_ENTRY "logbook_entry"
#define XML_LOGBOOK_ENTRY_SUBJECT "logbook_entry_subject"
#define XML_LOGBOOK_ENTRY_TEXT "logbook_entry_text"
#define XML_LOGBOOK_ENTRY_FILE "logbook_entry_file"
#define XML_LOGBOOK_ENTRY_TIME "logbook_entry_time"
#define XML_LOGBOOK_ENTRY_CREATOR "logbook_entry_creator"
#define XML_LOGBOOK_ENTRY_HIDDEN "logbook_entry_hidden"
#define XML_LOGBOOK_ENTRY_HIDER "logbook_entry_hider"
#define XML_LOGBOOK_ENTRY_HIDDEN_TIME "logbook_entry_hidden_time"

#define XML_PREVIEW_INDEX "preview_index"
#define LOGBOOK_PREVIEW_FILE "preview.xml"

XDAQ_INSTANTIATOR_IMPL(WizardSupervisor)

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "Wizard"

//========================================================================================================================
WizardSupervisor::WizardSupervisor(xdaq::ApplicationStub* s)
    : xdaq::Application(s)
    , SOAPMessenger(this)
    , supervisorClass_(getApplicationDescriptor()->getClassName())
    , supervisorClassNoNamespace_(supervisorClass_.substr(
          supervisorClass_.find_last_of(":") + 1,
          supervisorClass_.length() - supervisorClass_.find_last_of(":")))
{
	__COUT__ << "Constructor started." << __E__;

	INIT_MF("OtsConfigurationWizard");

	// attempt to make directory structure (just in case)
	mkdir((std::string(__ENV__("SERVICE_DATA_PATH"))).c_str(), 0755);
	mkdir((std::string(__ENV__("SERVICE_DATA_PATH")) + "/OtsWizardData").c_str(), 0755);

	GatewaySupervisor::indicateOtsAlive();

	generateURL();
	xgi::bind(this, &WizardSupervisor::Default, "Default");
	xgi::bind(this, &WizardSupervisor::verification, "Verify");
	xgi::bind(this, &WizardSupervisor::request, "Request");
	xgi::bind(this, &WizardSupervisor::requestIcons, "requestIcons");
	xgi::bind(this, &WizardSupervisor::editSecurity, "editSecurity");
	xgi::bind(this, &WizardSupervisor::UserSettings, "UserSettings");
	xgi::bind(this, &WizardSupervisor::tooltipRequest, "TooltipRequest");
	xgi::bind(this,
	          &WizardSupervisor::toggleSecurityCodeGeneration,
	          "ToggleSecurityCodeGeneration");
	xoap::bind(this,
	           &WizardSupervisor::supervisorSequenceCheck,
	           "SupervisorSequenceCheck",
	           XDAQ_NS_URI);
	xoap::bind(this,
	           &WizardSupervisor::supervisorLastConfigGroupRequest,
	           "SupervisorLastConfigGroupRequest",
	           XDAQ_NS_URI);
	init();

	__COUT__ << "Constructor complete." << __E__;
}

//========================================================================================================================
WizardSupervisor::~WizardSupervisor(void) { destroy(); }

//========================================================================================================================
void WizardSupervisor::init(void)
{
	// getApplicationContext();

	// init allowed file upload types
	allowedFileUploadTypes_.push_back("image/png");
	matchingFileUploadTypes_.push_back("png");
	allowedFileUploadTypes_.push_back("image/jpeg");
	matchingFileUploadTypes_.push_back("jpeg");
	allowedFileUploadTypes_.push_back("image/gif");
	matchingFileUploadTypes_.push_back("gif");
	allowedFileUploadTypes_.push_back("image/bmp");
	matchingFileUploadTypes_.push_back("bmp");
	allowedFileUploadTypes_.push_back("application/pdf");
	matchingFileUploadTypes_.push_back("pdf");
	allowedFileUploadTypes_.push_back("application/zip");
	matchingFileUploadTypes_.push_back("zip");
	allowedFileUploadTypes_.push_back("text/plain");
	matchingFileUploadTypes_.push_back("txt");
}

//========================================================================================================================
void WizardSupervisor::requestIcons(xgi::Input* in, xgi::Output* out)
{
	cgicc::Cgicc cgi(in);

	std::string submittedSequence = CgiDataUtilities::postData(cgi, "sequence");

	// SECURITY CHECK START ****
	if(securityCode_.compare(submittedSequence) != 0)
	{
		__COUT__ << "Unauthorized Request made, security sequence doesn't match! "
		         << time(0) << std::endl;
		return;
	}
	else
	{
		__COUT__ << "***Successfully authenticated security sequence. " << time(0)
		         << std::endl;
	}
	// SECURITY CHECK END ****

	// an icon is 7 fields.. give comma-separated
	// 0 - subtext = text below icon
	// 1 - altText = text for icon if image set to 0
	// 2 - uniqueWin = if true, only one window is allowed, else multiple instances of
	// window  3 - permissions = security level needed to see icon  4 - picfn = icon image
	// filename, 0 for no image  5 - linkurl = url of the window to open  6 - folderPath =
	// folder and subfolder location

	// clang-format off
	*out << "Configure,CFG,0,1,icon-Configure.png,/urn:xdaq-application:lid=280/,/"

	     << ",Table Editor,TBL,0,1,icon-ControlsDashboard.png,"
		 	"/urn:xdaq-application:lid=280/?configWindowName=tableEditor,/"

	     << ",Icon Editor,ICON,0,1,icon-IconEditor.png,"
		 	"/WebPath/html/ConfigurationGUI_subset.html?urn=280&subsetBasePath=DesktopIconTable&"
	        "recordAlias=Icons&groupingFieldList=Status%2CForceOnlyOneInstance%2CRequiredPermissionLevel,/"

	     << ",Security Settings,SEC,1,1,icon-SecuritySettings.png,"
		 	"/WebPath/html/SecuritySettings.html,/User Settings"

	     << ",Edit User Data,USER,1,1,icon-EditUserData.png,/WebPath/html/EditUserData.html,/User Settings"

	     << ",Console,C,1,1,icon-Console.png,/urn:xdaq-application:lid=260/,/" 
		 
		 //Configuration Wizards ------------------
		 << ",Front-end Wizard,CFG,0,1,icon-Configure.png,"
		 	"/WebPath/html/RecordWiz_ConfigurationGUI.html?urn=280&recordAlias=Front%2Dend,Config Wizards"

	     << ",Processor Wizard,CFG,0,1,icon-Configure.png,"
		 	"/WebPath/html/RecordWiz_ConfigurationGUI.html?urn=280&recordAlias=Processor,Config Wizards"

	     << ",Block Diagram,CFG,0,1,icon-Configure.png,"
		 	"/WebPath/html/ConfigurationSubsetBlockDiagram.html?urn=280,Config Wizards"
		 //end Configuration Wizards ------------------

	     << ",Code Editor,CODE,0,1,icon-CodeEditor.png,/urn:xdaq-application:lid=240/,/"

		 //Documentation ------------------
	     << ",State Machine Screenshot,FSM-SS,1,1,icon-StateMachine.png,"
		 	"/WebPath/images/windowContentImages/state_machine_screenshot.png,/Documentation"	
		 //end Documentation ------------------

	    //",Iterate,IT,0,1,icon-Iterate.png,/urn:xdaq-application:lid=280/?configWindowName=iterate,/"
	    //<<
	    //",Configure,CFG,0,1,icon-Configure.png,/urn:xdaq-application:lid=280/,myFolder"
	    //<<
	    //",Configure,CFG,0,1,icon-Configure.png,/urn:xdaq-application:lid=280/,/myFolder/mySub.folder"
	    //<<
	    //",Configure,CFG,0,1,icon-Configure.png,/urn:xdaq-application:lid=280/,myFolder/"
	    //<<

	    
	    //",Consumer
	    // Wizard,CFG,0,1,icon-Configure.png,/WebPath/html/RecordWiz_ConfigurationGUI.html?urn=280&subsetBasePath=FEInterfaceConfiguration&recordAlias=Consumer,Config
	    // Wizards" <<

	    //",DB Utilities,DB,1,1,0,http://127.0.0.1:8080/db/client.html" <<
        
	     //",Code Editor,CODE,0,1,icon-CodeEditor.png,/WebPath/html/CodeEditor.html,/"
	     << "";
	// clang-format on
	return;
}  // end requestIcons()

//========================================================================================================================
void WizardSupervisor::verification(xgi::Input* in, xgi::Output* out)
{
	cgicc::Cgicc cgi(in);
	std::string  submittedSequence = CgiDataUtilities::getData(cgi, "code");
	__COUT__ << "submittedSequence=" << submittedSequence << " " << time(0) << std::endl;

	std::string securityWarning = "";

	if(securityCode_.compare(submittedSequence) != 0)
	{
		__COUT__ << "Unauthorized Request made, security sequence doesn't match!"
		         << std::endl;
		*out << "Invalid code.";
		return;
	}
	else
	{
		// defaultSequence_ = false;
		__COUT__ << "*** Successfully authenticated security sequence "
		         << "@ " << time(0) << std::endl;

		if(defaultSequence_)
		{
			//__COUT__ << " UNSECURE!!!" << std::endl;
			securityWarning = "&secure=False";
		}
	}

	*out << "<!DOCTYPE HTML><html lang='en'><head><title>ots wiz</title>" <<
	    // show ots icon
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
		<meta name='theme-color' content='#ffffff'>"
	     <<
	    // end show ots icon
	    "</head>"
	     << "<frameset col='100%' row='100%'><frame src='/WebPath/html/Wizard.html?urn="
	     << this->getApplicationDescriptor()->getLocalId() << securityWarning
	     << "'></frameset></html>";
}  // end verification()

//========================================================================================================================
void WizardSupervisor::generateURL()
{
	defaultSequence_ = true;

	int   length = 4;
	FILE* fp     = fopen((SEQUENCE_FILE_NAME).c_str(), "r");
	if(fp)
	{
		__COUT_INFO__ << "Sequence length file found: " << SEQUENCE_FILE_NAME
		              << std::endl;
		char line[100];
		fgets(line, 100, fp);
		sscanf(line, "%d", &length);
		fclose(fp);
		if(length < 4)
			length = 4;  // don't allow shorter than 4
		else
			defaultSequence_ = false;
		srand(time(0));  // randomize differently each "time"
	}
	else
	{
		__COUT_INFO__
		    << "(Reverting to default wiz security) Sequence length file NOT found: "
		    << SEQUENCE_FILE_NAME << std::endl;
		srand(0);  // use same seed for convenience if file not found
	}

	__COUT__ << "Sequence length = " << length << std::endl;

	securityCode_ = "";

	const char alphanum[] =
	    "0123456789"
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz";

	for(int i = 0; i < length; ++i)
	{
		securityCode_ += alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	__COUT__ << __ENV__("OTS_CONFIGURATION_WIZARD_SUPERVISOR_SERVER") << ":"
	         << __ENV__("PORT") << "/urn:xdaq-application:lid="
	         << this->getApplicationDescriptor()->getLocalId()
	         << "/Verify?code=" << securityCode_ << std::endl;

	// Note: print out handled by StartOTS.sh now
	// std::thread([&](WizardSupervisor *ptr, std::string securityCode)
	//		{printURL(ptr,securityCode);},this,securityCode_).detach();

	fp = fopen((SEQUENCE_OUT_FILE_NAME).c_str(), "w");
	if(fp)
	{
		fprintf(fp, "%s", securityCode_.c_str());
		fclose(fp);
	}
	else
		__COUT_ERR__ << "Sequence output file NOT found: " << SEQUENCE_OUT_FILE_NAME
		             << std::endl;

	return;
}  // end generateURL()

void WizardSupervisor::printURL(WizardSupervisor* ptr, std::string securityCode)
{
	INIT_MF("ConfigurationWizard");
	// child process
	int i = 0;
	for(; i < 5; ++i)
	{
		std::this_thread::sleep_for(std::chrono::seconds(2));
		__COUT__ << __ENV__("OTS_CONFIGURATION_WIZARD_SUPERVISOR_SERVER") << ":"
		         << __ENV__("PORT") << "/urn:xdaq-application:lid="
		         << ptr->getApplicationDescriptor()->getLocalId()
		         << "/Verify?code=" << securityCode << std::endl;
	}
}

//========================================================================================================================
void WizardSupervisor::destroy(void)
{
	// called by destructor
}

//========================================================================================================================
void WizardSupervisor::tooltipRequest(xgi::Input* in, xgi::Output* out)
{
	cgicc::Cgicc cgi(in);

	std::string Command = CgiDataUtilities::getData(cgi, "RequestType");
	//__COUT__ << "Command = " << Command << std::endl;

	std::string submittedSequence = CgiDataUtilities::postData(cgi, "sequence");

	// SECURITY CHECK START ****
	if(securityCode_.compare(submittedSequence) != 0)
	{
		__COUT__ << "Unauthorized Request made, security sequence doesn't match!"
		         << std::endl;
		return;
	}
	//	else
	//	{
	//		__COUT__ << "***Successfully authenticated security sequence." << std::endl;
	//	}
	// SECURITY CHECK END ****

	HttpXmlDocument xmldoc;

	if(Command == "check")
	{
		WebUsers::tooltipCheckForUsername(WebUsers::DEFAULT_ADMIN_USERNAME,
		                                  &xmldoc,
		                                  CgiDataUtilities::getData(cgi, "srcFile"),
		                                  CgiDataUtilities::getData(cgi, "srcFunc"),
		                                  CgiDataUtilities::getData(cgi, "srcId"));
	}
	else if(Command == "setNeverShow")
	{
		WebUsers::tooltipSetNeverShowForUsername(
		    WebUsers::DEFAULT_ADMIN_USERNAME,
		    &xmldoc,
		    CgiDataUtilities::getData(cgi, "srcFile"),
		    CgiDataUtilities::getData(cgi, "srcFunc"),
		    CgiDataUtilities::getData(cgi, "srcId"),
		    CgiDataUtilities::getData(cgi, "doNeverShow") == "1" ? true : false,
		    CgiDataUtilities::getData(cgi, "temporarySilence") == "1" ? true : false);
	}
	else
		__COUT__ << "Command Request, " << Command << ", not recognized." << std::endl;

	xmldoc.outputXmlDocument((std::ostringstream*)out, false, true);
}  // end tooltipRequest()

//========================================================================================================================
void WizardSupervisor::toggleSecurityCodeGeneration(xgi::Input* in, xgi::Output* out)
{
	cgicc::Cgicc cgi(in);

	std::string Command = CgiDataUtilities::getData(cgi, "RequestType");
	__COUT__ << "Got to Command = " << Command << std::endl;

	std::string submittedSequence = CgiDataUtilities::postData(cgi, "sequence");

	// SECURITY CHECK START ****
	if(securityCode_.compare(submittedSequence) != 0)
	{
		__COUT__ << "Unauthorized Request made, security sequence doesn't match!"
		         << std::endl;
		return;
	}
	else
	{
		__COUT__ << "***Successfully authenticated security sequence." << std::endl;
	}
	// SECURITY CHECK END ****

	HttpXmlDocument xmldoc;

	if(Command == "TurnGenerationOn")
	{
		__COUT__ << "Turning automatic URL Generation on with a sequence depth of 16!"
		         << std::endl;
		std::ofstream outfile((SEQUENCE_FILE_NAME).c_str());
		outfile << "16" << std::endl;
		outfile.close();
		generateURL();

		// std::stringstream url;
		//	url << __ENV__("OTS_CONFIGURATION_WIZARD_SUPERVISOR_SERVER") << ":" <<
		// __ENV__("PORT")
		//	    << "/urn:xdaq-application:lid=" <<
		// this->getApplicationDescriptor()->getLocalId()
		//	    << "/Verify?code=" << securityCode_;
		//	printURL(this, securityCode_);
		std::thread([&](WizardSupervisor* ptr,
		                std::string       securityCode) { printURL(ptr, securityCode); },
		            this,
		            securityCode_)
		    .detach();

		xmldoc.addTextElementToData("Status", "Generation_Success");
	}
	else
		__COUT__ << "Command Request, " << Command << ", not recognized." << std::endl;

	xmldoc.outputXmlDocument((std::ostringstream*)out, false, true);
}

//========================================================================================================================
// xoap::supervisorSequenceCheck
//	verify cookie
xoap::MessageReference WizardSupervisor::supervisorSequenceCheck(
    xoap::MessageReference message)
{
	// SOAPUtilities::receive request parameters
	SOAPParameters parameters;
	parameters.addParameter("sequence");
	SOAPUtilities::receive(message, parameters);

	std::string submittedSequence = parameters.getValue("sequence");

	// If submittedSequence matches securityCode_ then return full permissions (255)
	//	else, return permissions 0
	std::map<std::string /*groupName*/, WebUsers::permissionLevel_t> permissionMap;

	if(securityCode_ == submittedSequence)
		permissionMap.emplace(
		    std::pair<std::string /*groupName*/, WebUsers::permissionLevel_t>(
		        WebUsers::DEFAULT_USER_GROUP, WebUsers::PERMISSION_LEVEL_ADMIN));
	else
	{
		__COUT__ << "Unauthorized Request made, security sequence doesn't match!"
		         << std::endl;

		permissionMap.emplace(
		    std::pair<std::string /*groupName*/, WebUsers::permissionLevel_t>(
		        WebUsers::DEFAULT_USER_GROUP, WebUsers::PERMISSION_LEVEL_INACTIVE));
	}

	// fill return parameters
	SOAPParameters retParameters;
	retParameters.addParameter("Permissions", StringMacros::mapToString(permissionMap));

	return SOAPUtilities::makeSOAPMessageReference("SequenceResponse", retParameters);
}

//===================================================================================================================
// xoap::supervisorLastConfigGroupRequest
//	return the group name and key for the last state machine activity
//
//	Note: same as Supervisor::supervisorLastConfigGroupRequest
xoap::MessageReference WizardSupervisor::supervisorLastConfigGroupRequest(
    xoap::MessageReference message)
{
	SOAPParameters parameters;
	parameters.addParameter("ActionOfLastGroup");
	SOAPUtilities::receive(message, parameters);

	return GatewaySupervisor::lastConfigGroupRequestHandler(parameters);
}

//========================================================================================================================
void WizardSupervisor::Default(xgi::Input* in, xgi::Output* out)
{
	__COUT__ << "Unauthorized Request made, security sequence doesn't match!"
	         << std::endl;
	*out << "Unauthorized Request.";
}

//========================================================================================================================
void WizardSupervisor::request(xgi::Input* in, xgi::Output* out)
{
	cgicc::Cgicc cgiIn(in);

	std::string submittedSequence = CgiDataUtilities::postData(cgiIn, "sequence");

	// SECURITY CHECK START ****
	if(securityCode_.compare(submittedSequence) != 0)
	{
		__COUT__ << "Unauthorized Request made, security sequence doesn't match! "
		         << time(0) << std::endl;
		return;
	}
	else
	{
		__COUT__ << "***Successfully authenticated security sequence. " << time(0)
		         << std::endl;
	}
	// SECURITY CHECK END ****

	std::string requestType = CgiDataUtilities::getData(cgiIn, "RequestType");
	__COUTV__(requestType);

	HttpXmlDocument xmlOut;

	try
	{
		if(requestType == "gatewayLaunchOTS" || requestType == "gatewayLaunchWiz")
		{
			// NOTE: similar to ConfigurationGUI version but DOES keep active sessions

			__COUT_WARN__ << requestType << " requestType received! " << __E__;
			__MOUT_WARN__ << requestType << " requestType received! " << __E__;

			// now launch
			ConfigurationManager cfgMgr;
			if(requestType == "gatewayLaunchOTS")
				GatewaySupervisor::launchStartOTSCommand("LAUNCH_OTS", &cfgMgr);
			else if(requestType == "gatewayLaunchWiz")
				GatewaySupervisor::launchStartOTSCommand("LAUNCH_WIZ", &cfgMgr);
		}
		else
		{
			__SS__ << "requestType Request, " << requestType << ", not recognized."
			       << __E__;
			__SS_THROW__;
		}
	}
	catch(const std::runtime_error& e)
	{
		__SS__ << "An error was encountered handling requestType '" << requestType
		       << "':" << e.what() << __E__;
		__COUT__ << "\n" << ss.str();
		xmlOut.addTextElementToData("Error", ss.str());
	}
	catch(...)
	{
		__SS__ << "An unknown error was encountered handling requestType '" << requestType
		       << ".' "
		       << "Please check the printouts to debug." << __E__;
		__COUT__ << "\n" << ss.str();
		xmlOut.addTextElementToData("Error", ss.str());
	}

	// return xml doc holding server response
	xmlOut.outputXmlDocument(
	    (std::ostringstream*)out,
	    false /*dispStdOut*/,
	    true /*allowWhiteSpace*/);  // Note: allow white space need for error response

}  // end request()

//========================================================================================================================
void WizardSupervisor::editSecurity(xgi::Input* in, xgi::Output* out)
{
	// if sequence doesn't match up -> return
	cgicc::Cgicc cgi(in);
	std::string  submittedSequence = CgiDataUtilities::postData(cgi, "sequence");
	std::string  submittedSecurity = CgiDataUtilities::postData(cgi, "selection");
	std::string  securityFileName  = SECURITY_FILE_NAME;

	// SECURITY CHECK START ****
	if(securityCode_.compare(submittedSequence) != 0)
	{
		__COUT__ << "Unauthorized Request made, security sequence doesn't match!"
		         << std::endl;
		return;
	}
	else
	{
		__COUT__ << "***Successfully authenticated security sequence." << std::endl;
	}
	// SECURITY CHECK END ****

	if(submittedSecurity != "")
	{
		__COUT__ << "Selection exists!" << std::endl;
		__COUT__ << submittedSecurity << std::endl;

		if(submittedSecurity == "ResetAllUserData")
		{
			WebUsers::deleteUserData();
			__COUT__ << "Turning URL Generation back to default!" << std::endl;
			// std::remove((SEQUENCE_FILE_NAME).c_str());
			// std::remove((SEQUENCE_OUT_FILE_NAME).c_str());
			std::ofstream newFile((SEQUENCE_FILE_NAME).c_str());
			newFile << "4" << std::endl;
			newFile.close();

			generateURL();
			std::thread([&](WizardSupervisor* ptr,
			                std::string securityCode) { printURL(ptr, securityCode); },
			            this,
			            securityCode_)
			    .detach();
			*out << "Default_URL_Generation";
		}
		else if(submittedSecurity == "ResetAllUserTooltips")
		{
			WebUsers::resetAllUserTooltips();
			*out << submittedSecurity;
			return;
		}
		else if(submittedSecurity == "DigestAccessAuthentication" ||
		        submittedSecurity == "NoSecurity")
		{
			std::ofstream writeSecurityFile;

			writeSecurityFile.open(securityFileName.c_str());
			if(writeSecurityFile.is_open())
				writeSecurityFile << submittedSecurity;
			else
				__COUT__ << "Error writing file!" << std::endl;

			writeSecurityFile.close();
		}
		else
		{
			__COUT_ERR__ << "Invalid submittedSecurity string: " << submittedSecurity
			             << std::endl;
			*out << "Error";
			return;
		}
	}

	// Always return the file
	std::ifstream securityFile;
	std::string   line;
	std::string   security   = "";
	int           lineNumber = 0;

	securityFile.open(securityFileName.c_str());

	if(!securityFile)
	{
		//__SS__ << "Error opening file: "<< securityFileName << std::endl;
		//__COUT_ERR__ << "\n" << ss.str();

		//__SS_THROW__;
		// return;
		security = "DigestAccessAuthentication";  // default security when no file exists
	}
	if(securityFile.is_open())
	{
		//__COUT__ << "Opened File: " << securityFileName << std::endl;
		while(std::getline(securityFile, line))
		{
			security += line;
			lineNumber++;
		}
		//__COUT__ << std::to_string(lineNumber) << ":" << iconList << std::endl;

		// Close file
		securityFile.close();
	}

	*out << security;
}
//========================================================================================================================
void WizardSupervisor::UserSettings(xgi::Input* in, xgi::Output* out)
{
	// if sequence doesn't match up -> return
	cgicc::Cgicc cgi(in);
	std::string  submittedSequence = CgiDataUtilities::postData(cgi, "sequence");
	std::string  securityFileName  = SECURITY_FILE_NAME;
	std::string  Command;
	if((Command = CgiDataUtilities::postData(cgi, "RequestType")) == "")
		Command = cgi("RequestType");  // get command from form, if PreviewEntry

	__COUT__ << Command << std::endl;
	__COUT__ << "We are vewing Users' Settings!" << std::endl;

	// SECURITY CHECK START ****
	if(securityCode_.compare(submittedSequence) != 0)
	{
		__COUT__ << "Unauthorized Request made, security sequence doesn't match!"
		         << std::endl;
		__COUT__ << submittedSequence << std::endl;
		// return;
	}
	else
	{
		__COUT__ << "***Successfully authenticated security sequence." << std::endl;
	}
	// SECURITY CHECK END ****

	HttpXmlDocument xmldoc;
	uint64_t        activeSessionIndex;
	std::string     user;
	uint8_t         userPermissions;

	if(Command != "")
	{
		__COUT__ << "Action exists!" << std::endl;
		__COUT__ << Command << std::endl;

		if(Command == "Import")
		{
			// cleanup temporary folder
			// NOTE: all input parameters for User Data will be attached to form
			//	so use cgi(xxx) to get values.
			// increment number for each temporary preview, previewPostTempIndex_
			// save entry and uploads to previewPath / previewPostTempIndex_ /.

			// cleanUpPreviews();
			//			std::string EntryText = cgi("EntryText");
			//			__COUT__ << "EntryText " << EntryText <<  std::endl << std::endl;
			//			std::string EntrySubject = cgi("EntrySubject");
			//			__COUT__ << "EntrySubject " << EntrySubject <<  std::endl <<
			// std::endl;

			// get creator name
			// std::string creator = user;
			// CgiDataUtilities::postData(cgi, "file"); CgiDataUtilities::postData(cgi,
			// "file");//
			__COUT__ << cgi("Entry") << std::endl;
			__COUT__ << cgi("Filename") << std::endl;
			__COUT__ << cgi("Imported_File") << std::endl;

			const std::vector<cgicc::FormFile> files = cgi.getFiles();
			__COUT__ << "FormFiles: " << sizeof(files) << std::endl;
			__COUT__ << "Number of files: " << files.size() << std::endl;

			for(unsigned int i = 0; i < files.size(); ++i)
			{
				std::string filename = USER_DATA_PATH + files[i].getFilename();
				__COUT__ << filename << std::endl;
				std::ofstream myFile;
				myFile.open(filename.c_str());
				files[0].writeToStream(myFile);
			}

			__COUT__ << files[0].getFilename() << std::endl;
			__COUT__ << "********************Files Begin********************"
			         << std::endl;
			for(unsigned int i = 0; i < files.size(); ++i)
			{
				__COUT__ << files[i].getDataType() << std::endl;
			}
			__COUT__ << "*********************Files End*********************"
			         << std::endl;

			//			savePostPreview(EntrySubject, EntryText, cgi.getFiles(), creator,
			//&xmldoc);  else xmldoc.addTextElementToData(XML_STATUS,"Failed - could not
			// get username info.");
		}
		else if(Command == "Export")
		{
			__SS__ << "This has been commented out due to problems compiling. Contact "
			          "system admins."
			       << __E__;
			__SS_THROW__;
			//
			//			__COUT__ << "We are exporting Users' Settings!!!" << std::endl;
			//			//system('pwd');
			//
			//			//Check for a TMP directory; if it doesn't exist, make it
			//			std::string command = "cd " + USER_DATA_PATH;
			//			std::string pathToTmp = USER_DATA_PATH + "/tmp/";
			//			std::filesystem::path tmp = pathToTmp;
			//
			//			exec(command.c_str());
			//			__COUT__ << exec("pwd") << std::endl;
			//
			//
			//			if(std::filesystem::status_known(tmpDir) ?
			// std::filesystem::exists(tmpDir) : std::filesystem::exists(tmpDir))
			//				__COUT__ << pathToTmp << " exists!" << std::endl;
			//			else
			//				__COUT__ << pathToTmp << "does not exist! Creating it now. "
			//<<  std::endl;
			//
			//			//Zip current files into TMP directory
			//			command = std::string("tar -cvf user_settings.tar") +
			//					std::string("ActiveTableGroups.cfg ") +
			//					std::string("ConsolePreferences ") +
			//					std::string("CoreTableInfoNames.dat ") +
			//					std::string("LoginData ") +
			//					std::string("OtsWizardData ") +
			//					std::string("ProgressBarData ");
			//
			//
			//			//return zip
			//			if(exec("pwd").find(USER_DATA_PATH) != std::string::npos)
			//			{
			//				__COUT__ << "Found USER_DATA directory " << std::endl;
			//				system(command.c_str());
			//				__COUT__ << system("ls") << std::endl;
			//			}
		}
		else
		{
			__COUT__ << "Command request not recognized: " << Command << std::endl;
			*out << "Error";
			return;
		}
	}

	*out << "test";
	return;
}
//========================================================================================================================
//	validateUploadFileType
//      returns "" if file type is invalid, else returns file extension to use
std::string WizardSupervisor::validateUploadFileType(const std::string fileType)
{
	for(unsigned int i = 0; i < allowedFileUploadTypes_.size(); ++i)
		if(allowedFileUploadTypes_[i] == fileType)
			return matchingFileUploadTypes_[i];  // found and done

	return "";  // not valid, return ""
}
//========================================================================================================================
//	cleanUpPreviews
//      cleanup logbook preview directory
//      all names have time_t creation time + "_" + incremented index
void WizardSupervisor::cleanUpPreviews()
{
	std::string userData = (std::string)USER_DATA_PATH;

	DIR* dir = opendir(userData.c_str());
	if(!dir)
	{
		__COUT__ << "Error - User Data directory missing: " << userData << std::endl;
		return;
	}

	struct dirent* entry;
	time_t         dirCreateTime;
	unsigned int   i;

	while(
	    (entry = readdir(
	         dir)))  // loop through all entries in directory and remove anything expired
	{
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 &&
		   strcmp(entry->d_name, ".svn") != 0)
		{
			// replace _ with space so sscanf works
			for(i = 0; i < strlen(entry->d_name); ++i)
				if(entry->d_name[i] == '_')
				{
					entry->d_name[i] = ' ';
					break;
				}
			sscanf(entry->d_name, "%li", &dirCreateTime);

			if((time(0) - dirCreateTime) > USER_DATA_EXPIRATION_TIME)
			{
				__COUT__ << "Expired" << std::endl;

				entry->d_name[i] = '_';  // put _ back

				__COUT__ << "rm -rf " << USER_DATA_PATH + (std::string)entry->d_name
				         << std::endl
				         << std::endl;
				system(((std::string)("rm -rf " + userData + (std::string)entry->d_name))
				           .c_str());
			}
		}
	}

	closedir(dir);
}

//========================================================================================================================
//	savePostPreview
//      save post to preview directory named with time and incremented index
void WizardSupervisor::savePostPreview(std::string&                        subject,
                                       std::string&                        text,
                                       const std::vector<cgicc::FormFile>& files,
                                       std::string                         creator,
                                       HttpXmlDocument*                    xmldoc)
{
	/*if(activeExperiment_ == "") //no active experiment!
	{
	    if(xmldoc) xmldoc->addTextElementToData(XML_STATUS,"Failed - no active experiment
	currently!"); return;
	}
*/
	char fileIndex[40];
	sprintf(fileIndex,
	        "%lu_%lu",
	        time(0),
	        clock());  // create unique time label for entry time(0)_clock()
	std::string userDataPath = (std::string)USER_DATA_PATH + (std::string)fileIndex;

	__COUT__ << "userDataPath " << userDataPath << std::endl;
	if(-1 == mkdir(userDataPath.c_str(), 0755))
	{
		if(xmldoc)
			xmldoc->addTextElementToData(XML_STATUS,
			                             "Failed - directory could not be generated.");
		return;
	}
	/*
	//new directory created successfully, save text and files
	//entry structure:
	//  <XML_LOGBOOK_ENTRY>
	//		<XML_LOGBOOK_ENTRY_TIME>
	//		<XML_LOGBOOK_ENTRY_CREATOR>
	//      <XML_LOGBOOK_ENTRY_SUBJECT>
	//      <XML_LOGBOOK_ENTRY_TEXT>
	//      <XML_LOGBOOK_ENTRY_FILE value=fileType0>
	//      <XML_LOGBOOK_ENTRY_FILE value=fileType1> ...
	//  </XML_LOGBOOK_ENTRY>

	escapeLogbookEntry(text);
	escapeLogbookEntry(subject);
	__COUT__ << "~~subject " << subject << std::endl << "~~text " << text << std::endl <<
	std::endl;

	HttpXmlDocument previewXml;

	previewXml.addTextElementToData(XML_LOGBOOK_ENTRY);
	previewXml.addTextElementToParent(XML_LOGBOOK_ENTRY_TIME, fileIndex,
	XML_LOGBOOK_ENTRY); if(xmldoc)
	xmldoc->addTextElementToData(XML_LOGBOOK_ENTRY_TIME,fileIndex); //return time
	previewXml.addTextElementToParent(XML_LOGBOOK_ENTRY_CREATOR, creator,
	XML_LOGBOOK_ENTRY); if(xmldoc)
	xmldoc->addTextElementToData(XML_LOGBOOK_ENTRY_CREATOR,creator); //return creator
	previewXml.addTextElementToParent(XML_LOGBOOK_ENTRY_TEXT, text, XML_LOGBOOK_ENTRY);
	if(xmldoc) xmldoc->addTextElementToData(XML_LOGBOOK_ENTRY_TEXT,text); //return text
	previewXml.addTextElementToParent(XML_LOGBOOK_ENTRY_SUBJECT, subject,
	XML_LOGBOOK_ENTRY); if(xmldoc)
	xmldoc->addTextElementToData(XML_LOGBOOK_ENTRY_SUBJECT,subject); //return subject

	__COUT__ << "file size " << files.size() << std::endl;

	std::string filename;
	std::ofstream myfile;
	for (unsigned int i=0; i<files.size(); ++i)
	{

	    previewXml.addTextElementToParent(XML_LOGBOOK_ENTRY_FILE, files[i].getDataType(),
	XML_LOGBOOK_ENTRY); if(xmldoc)
	xmldoc->addTextElementToData(XML_LOGBOOK_ENTRY_FILE,files[i].getDataType()); //return
	file type

	    if((filename = validateUploadFileType(files[i].getDataType())) == "") //invalid
	file type
	    {
	        if(xmldoc) xmldoc->addTextElementToData(XML_STATUS,"Failed - invalid file
	type, " + files[i].getDataType() + "."); return;
	    }*/

	/*//file validated, so save upload to temp directory
	    sprintf(fileIndex,"%d",i);
	    filename = previewPath + "/" + (std::string)LOGBOOK_PREVIEW_UPLOAD_PREFACE +
	            (std::string)fileIndex + "." + filename;

	    __COUT__ << "file " << i << " - " << filename << std::endl;
	    myfile.open(filename.c_str());
	    if (myfile.is_open())
	    {
	        files[i].writeToStream(myfile);
	        myfile.close();
	    }
	}*/
	/*
	//save xml doc for preview entry
	previewXml.saveXmlDocument(USER_DATA_PATH + "/" + (std::string)LOGBOOK_PREVIEW_FILE);

	if(xmldoc) xmldoc->addTextElementToData(XML_STATUS,"1"); //1 indicates success!
	if(xmldoc) xmldoc->addTextElementToData(XML_PREVIEW_INDEX,"1"); //1 indicates is a
	preview post*/
}
