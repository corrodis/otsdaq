#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/Macros/TablePluginMacros.h"
#include "otsdaq-core/TablePlugins/MessageFacilityTable.h"

#include <stdio.h>
#include <fstream>  // std::fstream
#include <iostream>
using namespace ots;

#define MF_CFG_FILE                     \
	std::string(__ENV__("USER_DATA")) + \
	    "/MessageFacilityConfigurations/MessageFacilityGen.fcl"
#define QT_CFG_FILE                     \
	std::string(__ENV__("USER_DATA")) + \
	    "/MessageFacilityConfigurations/QTMessageViewerGen.fcl"
#define QUIET_CFG_FILE                  \
	std::string(__ENV__("USER_DATA")) + \
	    "/MessageFacilityConfigurations/QuietForwarderGen.cfg"
#define USE_WEB_BOOL_FILE                 \
	std::string(__ENV__("USER_DATA")) +   \
	    "/MessageFacilityConfigurations/" \
	    "UseWebConsole.bool"
#define USE_QT_BOOL_FILE                  \
	std::string(__ENV__("USER_DATA")) +   \
	    "/MessageFacilityConfigurations/" \
	    "UseQTViewer.bool"

// MessageFacilityTable Column names
#define COL_NAME "UID"
#define COL_STATUS TableViewColumnInfo::COL_NAME_STATUS
#define COL_ENABLE_FWD "EnableUDPForwarding"

#define COL_USE_WEB "ForwardToWebConsoleGUI"
#define COL_WEB_IP "WebConsoleForwardingIPAddress"
#define COL_WEB_PORT0 "WebConsoleForwardingPort0"
#define COL_WEB_PORT1 "WebConsoleForwardingPort1"

#define COL_USE_QT "ForwardToQTViewerGUI"
#define COL_QT_IP "QTViewerForwardingIPAddress"
#define COL_QT_PORT "QTViewerForwardingPort"

MessageFacilityTable::MessageFacilityTable(void) : TableBase("MessageFacilityTable")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////
}

MessageFacilityTable::~MessageFacilityTable(void) {}

void MessageFacilityTable::init(ConfigurationManager* configManager)
{
	//use isFirstAppInContext to only run once per context, for example to avoid
	//	generating files on local disk multiple times.
	bool isFirstAppInContext = configManager->isOwnerFirstAppInContext();

	//__COUTV__(isFirstAppInContext);
	if(!isFirstAppInContext) return;

	//	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	//	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	bool        status, enableFwd, useWeb, useQT;
	int         fwdPort;
	std::string fwdIP;

	auto childrenMap = configManager->__SELF_NODE__.getChildren();

	// generate MF_CFG_FILE file
	std::fstream fs;
	fs.open(MF_CFG_FILE, std::fstream::out | std::fstream::trunc);
	if(fs.fail())
	{
		__SS__ << "Failed to open Message Facility table file: " << MF_CFG_FILE
		       << std::endl;
		__SS_THROW__;
	}
	else
		__COUT__ << "Opened.. " << MF_CFG_FILE << __E__;

	// loop through all children just to be same as other tables
	//	exit loop after first active one
	for(auto& child : childrenMap)
	{
		child.second.getNode(COL_STATUS).getValue(status);

		if(!status)
			continue;  // skip inactive rows

		child.second.getNode(COL_ENABLE_FWD).getValue(enableFwd);

		child.second.getNode(COL_USE_WEB).getValue(useWeb);
		child.second.getNode(COL_USE_QT).getValue(useQT);

		if(useWeb && useQT)
		{
			fs.close();
			__SS__ << "Illegal Message Facility table: "
			       << "Can only enable Web Console or QT Viewer, not both." << std::endl;
			__SS_THROW__;
		}

		std::fstream bfs;
		// output use web bool for StartOTS.sh
		bfs.open(USE_WEB_BOOL_FILE, std::fstream::out | std::fstream::trunc);
		if(bfs.fail())
		{
			fs.close();
			__SS__ << "Failed to open boolean Use of Web Console table file: "
			       << USE_WEB_BOOL_FILE << std::endl;
			__SS_THROW__;
		}
		bfs << (useWeb ? 1 : 0);
		bfs.close();

		// output use web bool for StartOTS.sh
		bfs.open(USE_QT_BOOL_FILE, std::fstream::out | std::fstream::trunc);
		if(bfs.fail())
		{
			fs.close();
			__SS__ << "Failed to open boolean Use of QT Viewer table file: "
			       << USE_QT_BOOL_FILE << std::endl;
			__SS_THROW__;
		}
		bfs << (useQT ? 1 : 0);
		bfs.close();

		if(enableFwd)  // write udp forward config
		{
			// handle using web gui
			if(useWeb)
			{
				__COUT__ << "Forwarding to Web GUI with UDP forward MesageFacility "
				            "table."
				         << std::endl;

				child.second.getNode(COL_WEB_PORT0).getValue(fwdPort);
				child.second.getNode(COL_WEB_IP).getValue(fwdIP);

				fs << "udp: {\n";
				fs << "\t"
				   << "type: UDP\n";
				fs << "\t"
				   << "threshold: DEBUG\n";
				fs << "\t"
				   << "port: " << fwdPort << "\n";
				fs << "\t"
				   << "host: \"" << fwdIP << "\"\n";
				fs << "}\n";

				// output quiet forwarder config file
				std::fstream qtfs;
				qtfs.open(QUIET_CFG_FILE, std::fstream::out | std::fstream::trunc);
				if(qtfs.fail())
				{
					fs.close();
					__SS__ << "Failed to open Web Console's 'Quiet Forwarder' "
					          "table file: "
					       << QUIET_CFG_FILE << std::endl;
					__SS_THROW__;
				}
				qtfs << "RECEIVE_PORT \t " << fwdPort << "\n";
				child.second.getNode(COL_WEB_PORT1).getValue(fwdPort);
				qtfs << "DESTINATION_PORT \t " << fwdPort << "\n";
				qtfs << "DESTINATION_IP \t " << fwdIP << "\n";
				qtfs.close();
			}

			// handle using qt viewer
			if(useQT)
			{
				__COUT__ << "Forwarding to Web GUI with UDP forward MesageFacility "
				            "table."
				         << std::endl;

				child.second.getNode(COL_QT_PORT).getValue(fwdPort);
				child.second.getNode(COL_QT_IP).getValue(fwdIP);

				fs << "udp: {\n";
				fs << "\t"
				   << "type: UDP\n";
				fs << "\t"
				   << "threshold: DEBUG\n";
				fs << "\t"
				   << "port: " << fwdPort << "\n";
				fs << "\t"
				   << "host: \"" << fwdIP << "\"\n";
				fs << "}\n";

				// output QT Viewer config file
				std::fstream qtfs;
				qtfs.open(QT_CFG_FILE, std::fstream::out | std::fstream::trunc);
				if(qtfs.fail())
				{
					fs.close();
					__SS__ << "Failed to open QT Message Viewer table file: "
					       << QT_CFG_FILE << std::endl;
					__SS_THROW__;
				}
				qtfs << "receivers: \n{\n";
				qtfs << "\t"
				     << "syslog: \n{\n";
				qtfs << "\t\t"
				     << "receiverType: "
				     << "\"UDP\""
				     << "\n";
				qtfs << "\t\t"
				     << "port: " << fwdPort << "\n";
				qtfs << "\t}\n";  // close syslog
				qtfs << "}\n";    // close receivers
				qtfs << "\n";
				qtfs << "threshold: "
				     << "DEBUG"
				     << "\n";
				qtfs.close();
			}
		}
		else  // write cout config (not forwarding to external process)
		{
			__COUT__ << "Using cout-only MesageFacility table." << std::endl;
			fs << "console: {\n";
			fs << "\t"
			   << "type: \"cout\"\n";
			fs << "\t"
			   << "threshold: \"DEBUG\"\n";
			fs << "\t"
			   << "noTimeStamps: true\n";
			fs << "\t"
			   << "noLineBreaks: true\n";
			fs << "}\n";
		}

		break;  // take first enable row only!
	}

	// close MF config file
	fs.close();
}

DEFINE_OTS_TABLE(MessageFacilityTable)
