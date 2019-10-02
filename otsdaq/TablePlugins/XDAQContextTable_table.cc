#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/Macros/TablePluginMacros.h"
#include "otsdaq/TablePlugins/XDAQContextTable.h"

#include <stdio.h>
#include <fstream>  // std::fstream
#include <iostream>

using namespace ots;

// clang-format off

#define XDAQ_RUN_FILE         std::string(__ENV__("XDAQ_CONFIGURATION_DATA_PATH")) + "/" + std::string(__ENV__("XDAQ_CONFIGURATION_XML")) + ".xml"
#define APP_PRIORITY_FILE     std::string(__ENV__("XDAQ_CONFIGURATION_DATA_PATH")) + "/" + "xdaqAppStateMachinePriority"

const std::string XDAQContextTable::DEPRECATED_SUPERVISOR_CLASS = "ots::Supervisor";  // still allowed for now, in StartOTS
const std::string XDAQContextTable::GATEWAY_SUPERVISOR_CLASS 	= "ots::GatewaySupervisor";
const std::string XDAQContextTable::WIZARD_SUPERVISOR_CLASS  	= "ots::WizardSupervisor";
const std::set<std::string> XDAQContextTable::FETypeClassNames_ 		= { "ots::FESupervisor", "ots::FEDataManagerSupervisor", "ots::ARTDAQFEDataManagerSupervisor"};
const std::set<std::string> XDAQContextTable::DMTypeClassNames_ 		= { "ots::DataManagerSupervisor", "ots::FEDataManagerSupervisor", "ots::ARTDAQFEDataManagerSupervisor"};
const std::set<std::string> XDAQContextTable::LogbookTypeClassNames_ 	= { "ots::LogbookSupervisor"};
const std::set<std::string> XDAQContextTable::MacroMakerTypeClassNames_ = { "ots::MacroMakerSupervisor"};
const std::set<std::string> XDAQContextTable::ChatTypeClassNames_ 		= { "ots::ChatSupervisor"};
const std::set<std::string> XDAQContextTable::ConsoleTypeClassNames_ 	= { "ots::ConsoleSupervisor"};
const std::set<std::string> XDAQContextTable::ConfigurationGUITypeClassNames_ = { "ots::TableGUISupervisor"};


const uint8_t XDAQContextTable::XDAQApplication::DEFAULT_PRIORITY = 100;

XDAQContextTable::ColContext 				XDAQContextTable::colContext_ 		= XDAQContextTable::ColContext();  // initialize static member
XDAQContextTable::ColApplication 			XDAQContextTable::colApplication_ 	= XDAQContextTable::ColApplication();  // initialize static member
XDAQContextTable::ColApplicationProperty 	XDAQContextTable::colAppProperty_ 	= XDAQContextTable::ColApplicationProperty();  // initialize static member

const std::string XDAQContextTable::XDAQ_CONTEXT_TABLE = "XDAQContextTable";


// clang-format on

//========================================================================================================================
XDAQContextTable::XDAQContextTable(void) : TableBase(XDAQContextTable::XDAQ_CONTEXT_TABLE)
, artdaqSupervisorContext_((unsigned int)-1)
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////
}

//========================================================================================================================
XDAQContextTable::~XDAQContextTable(void) {}

//========================================================================================================================
void XDAQContextTable::init(ConfigurationManager* configManager)
{
	//__COUT__ << "init" << __E__;
	extractContexts(configManager);

	bool isFirstAppInContext = configManager->isOwnerFirstAppInContext();

	//__COUTV__(isFirstAppInContext);
	if(!isFirstAppInContext)
		return;


	{
		/////////////////////////
		// generate xdaq run parameter file
		std::fstream fs;
		fs.open(XDAQ_RUN_FILE, std::fstream::out | std::fstream::trunc);
		if(fs.fail())
		{
			__SS__ << "Failed to open XDAQ run file: " << XDAQ_RUN_FILE << __E__;
			__SS_THROW__;
		}
		outputXDAQXML((std::ostream&)fs);
		fs.close();
	}
}

//========================================================================================================================
std::string XDAQContextTable::getContextAddress(const std::string& contextUID,
                                                bool               wantHttp) const
{
	if(contextUID == "X")
		return "";
	for(auto& context : contexts_)
	{
		if(context.contextUID_ == contextUID)
		{
			if(wantHttp)
				return context.address_;
			auto address = context.address_;
			if(address.find("http://") == 0)
			{
				address = address.replace(0, 7, "");
			}
			if(address.find("https://") == 0)
			{
				address = address.replace(0, 8, "");
			}
			return address;
		}
	}
	return "";
}  // end getContextAddress()

//========================================================================================================================
const XDAQContextTable::XDAQContext* XDAQContextTable::getTheARTDAQSupervisorContext() const
{
	if(artdaqSupervisorContext_ >= contexts_.size())
		return nullptr;
	return &contexts_[artdaqSupervisorContext_];
} //end getTheARTDAQSupervisorContext()

//========================================================================================================================
ConfigurationTree XDAQContextTable::getContextNode(
    const ConfigurationManager* configManager, const std::string& contextUID)
{
	return configManager->getNode(XDAQContextTable::XDAQ_CONTEXT_TABLE).getNode(contextUID);
} //end getContextNode()

//========================================================================================================================
ConfigurationTree XDAQContextTable::getApplicationNode(
    const ConfigurationManager* configManager,
    const std::string&          contextUID,
    const std::string&          appUID)
{
	return configManager->getNode(XDAQContextTable::XDAQ_CONTEXT_TABLE).getNode(
	    contextUID + "/" + colContext_.colLinkToApplicationTable_ + "/" + appUID);
} //end getApplicationNode()

//========================================================================================================================
ConfigurationTree XDAQContextTable::getSupervisorConfigNode(
    const ConfigurationManager* configManager,
    const std::string&          contextUID,
    const std::string&          appUID)
{
	return configManager->getNode(XDAQContextTable::XDAQ_CONTEXT_TABLE).getNode(
	    contextUID + "/" + XDAQContextTable::colContext_.colLinkToApplicationTable_ +
		"/" + appUID + "/" +
		XDAQContextTable::colApplication_.colLinkToSupervisorTable_);
} //end getSupervisorConfigNode()

//========================================================================================================================
// extractContexts
//	Could be called by other tables if they need to access the context.
//		This doesn't re-write config files, it just re-makes constructs in software.
void XDAQContextTable::extractContexts(ConfigurationManager* configManager)
{
	//__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << __E__;
	//__COUT__ << configManager->__SELF_NODE__ << __E__;

	//	__COUT__ << configManager->getNode(this->getTableName()).getValueAsString()
	//		  											  << __E__;

	auto children = configManager->__SELF_NODE__.getChildren();

	contexts_.clear();  // reset
	artdaqSupervisorContext_ = (unsigned int)-1; //reset

	// Enforce that app IDs do not repeat!
	//	Note: this is important because there are maps in MacroMaker and
	// SupervisorDescriptorInfoBase that rely on localId() as key
	std::set<unsigned int /*appId*/> appIdSet;

	for(auto& child : children)
	{
		contexts_.push_back(XDAQContext());
		//__COUT__ << child.first << __E__;
		//		__COUT__ << child.second.getNode(colContextUID_) << __E__;

		contexts_.back().contextUID_ = child.first;

		contexts_.back().sourceConfig_ =
		    child.second.getTableName() + "_v" +
		    child.second.getTableVersion().toString() + " @ " +
		    std::to_string(child.second.getTableCreationTime());
		child.second.getNode(colContext_.colContextUID_)
		    .getValue(contexts_.back().contextUID_);
		child.second.getNode(colContext_.colStatus_).getValue(contexts_.back().status_);
		child.second.getNode(colContext_.colId_).getValue(contexts_.back().id_);
		child.second.getNode(colContext_.colAddress_).getValue(contexts_.back().address_);
		child.second.getNode(colContext_.colPort_).getValue(contexts_.back().port_);
		// conversion to default happens at TableView.icc
		//		if(contexts_.back().port_ == 0) //convert 0 to ${OTS_MAIN_PORT}
		//			contexts_.back().port_ = atoi(__ENV__("OTS_MAIN_PORT"));
		if(contexts_.back().port_ < 1024 || contexts_.back().port_ > 49151)
		{
			__SS__ << "Illegal xdaq Context port: " << contexts_.back().port_
			       << ". Port must be between 1024 and 49151." << __E__;
		}
		// child.second.getNode(colContext_.colARTDAQDataPort_).getValue(contexts_.back().artdaqDataPort_);

		//__COUT__ << contexts_.back().address_ << __E__;
		auto appLink = child.second.getNode(colContext_.colLinkToApplicationTable_);
		if(appLink.isDisconnected())
		{
			__SS__ << "Application link is disconnected!" << __E__;
			__SS_THROW__;
		}

		// add xdaq applications to this context
		auto appChildren = appLink.getChildren();
		for(auto appChild : appChildren)
		{
			//__COUT__ << "Loop: " << child.first << "/" << appChild.first << __E__;

			contexts_.back().applications_.push_back(XDAQApplication());

			contexts_.back().applications_.back().applicationGroupID_ = child.first;
			contexts_.back().applications_.back().sourceConfig_ =
			    appChild.second.getTableName() + "_v" +
			    appChild.second.getTableVersion().toString() + " @ " +
			    std::to_string(appChild.second.getTableCreationTime());

			appChild.second.getNode(colApplication_.colApplicationUID_)
			    .getValue(contexts_.back().applications_.back().applicationUID_);
			appChild.second.getNode(colApplication_.colStatus_)
			    .getValue(contexts_.back().applications_.back().status_);
			appChild.second.getNode(colApplication_.colClass_)
			    .getValue(contexts_.back().applications_.back().class_);
			appChild.second.getNode(colApplication_.colId_)
			    .getValue(contexts_.back().applications_.back().id_);

			// assert Gateway is 200
			if((contexts_.back().applications_.back().id_ == 200 &&
			    contexts_.back().applications_.back().class_ !=
			        XDAQContextTable::GATEWAY_SUPERVISOR_CLASS &&
			    contexts_.back().applications_.back().class_ !=
			        XDAQContextTable::DEPRECATED_SUPERVISOR_CLASS) ||
			   (contexts_.back().applications_.back().id_ != 200 &&
			    (contexts_.back().applications_.back().class_ ==
			         XDAQContextTable::GATEWAY_SUPERVISOR_CLASS ||
			     contexts_.back().applications_.back().class_ ==
			         XDAQContextTable::DEPRECATED_SUPERVISOR_CLASS)))
			{
				__SS__ << "XDAQ Application ID of 200 is reserved for the Gateway "
				          "Supervisor "
				       << XDAQContextTable::GATEWAY_SUPERVISOR_CLASS
				       << ". Conflict specifically at id="
				       << contexts_.back().applications_.back().id_ << " appName="
				       << contexts_.back().applications_.back().applicationUID_ << __E__;
				__SS_THROW__;
			}

			// assert NO app id repeats if context/app enabled
			if(contexts_.back().status_ && contexts_.back().applications_.back().status_)
			{
				// assert NO app id repeats
				if(appIdSet.find(contexts_.back().applications_.back().id_) !=
				   appIdSet.end())
				{
					__SS__ << "XDAQ Application IDs are not unique. Specifically at id="
					       << contexts_.back().applications_.back().id_ << " appName="
					       << contexts_.back().applications_.back().applicationUID_
					       << __E__;
					__COUT_ERR__ << "\n" << ss.str();
					__SS_THROW__;
				}
				appIdSet.insert(contexts_.back().applications_.back().id_);
			}

			// convert defaults to values
			if(appChild.second.getNode(colApplication_.colInstance_).isDefaultValue())
				contexts_.back().applications_.back().instance_ = 1;
			else
				appChild.second.getNode(colApplication_.colInstance_)
				    .getValue(contexts_.back().applications_.back().instance_);

			if(appChild.second.getNode(colApplication_.colNetwork_).isDefaultValue())
				contexts_.back().applications_.back().network_ = "local";
			else
				appChild.second.getNode(colApplication_.colNetwork_)
				    .getValue(contexts_.back().applications_.back().network_);

			if(appChild.second.getNode(colApplication_.colGroup_).isDefaultValue())
				contexts_.back().applications_.back().group_ = "daq";
			else
				appChild.second.getNode(colApplication_.colGroup_)
				    .getValue(contexts_.back().applications_.back().group_);

			appChild.second.getNode(colApplication_.colModule_)
			    .getValue(contexts_.back().applications_.back().module_);

			// force deprecated Supervisor to GatewaySupervisor class
			if(contexts_.back().applications_.back().class_ ==
			   XDAQContextTable::DEPRECATED_SUPERVISOR_CLASS)
			{
				contexts_.back().applications_.back().class_ =
				    XDAQContextTable::GATEWAY_SUPERVISOR_CLASS;
				__COUT__ << "Fixing deprecated Supervisor class from "
				         << XDAQContextTable::DEPRECATED_SUPERVISOR_CLASS << " to "
				         << (contexts_.back().applications_.back().class_);
			}
			if(contexts_.back().applications_.back().module_.find("libSupervisor.so") !=
			   std::string::npos)
			{
				__COUT__ << "Fixing deprecated Supervisor class from "
				         << contexts_.back().applications_.back().module_ << " to ";
				contexts_.back().applications_.back().module_ =
				    contexts_.back().applications_.back().module_.substr(
				        0,
				        contexts_.back().applications_.back().module_.size() -
				            std::string("Supervisor.so").size()) +
				    "GatewaySupervisor.so";
				std::cout << contexts_.back().applications_.back().module_ << __E__;
			}

			try
			{
				appChild.second.getNode(colApplication_.colConfigurePriority_)
				    .getValue(contexts_.back()
				                  .applications_.back()
				                  .stateMachineCommandPriority_["Configure"]);
				appChild.second.getNode(colApplication_.colStartPriority_)
				    .getValue(contexts_.back()
				                  .applications_.back()
				                  .stateMachineCommandPriority_["Start"]);
				appChild.second.getNode(colApplication_.colStopPriority_)
				    .getValue(contexts_.back()
				                  .applications_.back()
				                  .stateMachineCommandPriority_["Stop"]);
			}
			catch(...)
			{
				__COUT__ << "Ignoring missing state machine priorities..." << __E__;
			}

			auto appPropertyLink =
			    appChild.second.getNode(colApplication_.colLinkToPropertyTable_);
			if(!appPropertyLink.isDisconnected())
			{
				// add xdaq application properties to this context

				auto appPropertyChildren = appPropertyLink.getChildren();

				//__COUT__ << "appPropertyChildren.size() " << appPropertyChildren.size()
				//<< __E__;

				for(auto appPropertyChild : appPropertyChildren)
				{
					contexts_.back().applications_.back().properties_.push_back(
					    XDAQApplicationProperty());
					contexts_.back().applications_.back().properties_.back().status_ =
					    appPropertyChild.second.getNode(colAppProperty_.colStatus_)
					        .getValue<bool>();
					contexts_.back().applications_.back().properties_.back().name_ =
					    appPropertyChild.second.getNode(colAppProperty_.colPropertyName_)
					        .getValue<std::string>();
					contexts_.back().applications_.back().properties_.back().type_ =
					    appPropertyChild.second.getNode(colAppProperty_.colPropertyType_)
					        .getValue<std::string>();
					contexts_.back().applications_.back().properties_.back().value_ =
					    appPropertyChild.second.getNode(colAppProperty_.colPropertyValue_)
					        .getValue<std::string>();

					//__COUT__ <<
					// contexts_.back().applications_.back().properties_.back().name_ <<
					// __E__;
				}
			}
			// else
			//	__COUT__ << "Disconnected." << __E__;
		}

		// check for artdaq Supervisor in context
		{
			if(!contexts_.back().status_)
				continue;  // skip if disabled

			for(auto& app : contexts_.back().applications_)
			{
				if(app.class_ ==  // if artdaq interface supervisor
						"ots::ARTDAQSupervisor" &&
						app.status_)
				{
					__COUT__ << "Found " << app.class_ << " in context '" <<
							contexts_.back().id_ << "'" << __E__;

					if(artdaqSupervisorContext_ < contexts_.size())
					{
						__SS__ << "Error! Only one artdaq Supervisor is allowed to be active - " <<
								"two encountered in context '" <<
								contexts_[artdaqSupervisorContext_].id_ << "' and '" <<
								contexts_.back().id_ << "'..." << __E__;
						__SS_THROW__;
					}

					artdaqSupervisorContext_ = contexts_.size() - 1;
					//break; //continue to look for invalid configuration
				}
			} //end artdaq app loop

		} //end artdaq context handling

	} //end primary context loop
} //end extractContexts()

//========================================================================================================================
void XDAQContextTable::outputXDAQXML(std::ostream& out)
{
	// each generated context will look something like this:
	//<xc:Context id="0" url="http://${SUPERVISOR_SERVER}:${PORT}">
	////<xc:Application class="ots::FESupervisor" id="${FEW_SUPERVISOR_ID}" instance="1"
	/// network="local" group="daq"/>
	////<xc:Module>${OTSDAQ_LIB}/libCoreSupervisors.so</xc:Module>
	//</xc:Context>

	// print xml header information and declare xc partition
	out << "<?xml version='1.0'?>\n"
	    << "<xc:Partition \txmlns:xsi\t= \"http://www.w3.org/2001/XMLSchema-instance\"\n"
	    << "\t\txmlns:soapenc\t= \"http://schemas.xmlsoap.org/soap/encoding/\"\n"
	    << "\t\txmlns:xc\t= "
	       "\"http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30\">\n\n";

	// print partition open
	// for each context
	//	open context
	//	for each app in context
	//		print application
	//		print module
	//	close context
	// close partition

	char tmp[200];
	for(XDAQContext& context : contexts_)
	{
		//__COUT__ << context.contextUID_ << __E__;

		sprintf(tmp,
		        "\t<!-- ContextUID='%s' sourceConfig='%s' -->",
		        context.contextUID_.c_str(),
		        context.sourceConfig_.c_str());
		out << tmp << "\n";

		if(!context.status_)  // comment out if disabled
			out << "\t<!--\n";

		sprintf(tmp,
		        "\t<xc:Context id=\"%u\" url=\"%s:%u\">",
		        context.id_,
		        context.address_.c_str(),
		        context.port_);
		out << tmp << "\n\n";

		for(XDAQApplication& app : context.applications_)
		{
			//__COUT__ << app.id_ << __E__;

			if(context.status_)
			{
				sprintf(
				    tmp,
				    "\t\t<!-- Application GroupID = '%s' UID='%s' sourceConfig='%s' -->",
				    app.applicationGroupID_.c_str(),
				    app.applicationUID_.c_str(),
				    app.sourceConfig_.c_str());
				out << tmp << "\n";

				if(!app.status_)  // comment out if disabled
					out << "\t\t<!--\n";
			}

			sprintf(tmp,
			        "\t\t<xc:Application class=\"%s\" id=\"%u\" instance=\"%u\" "
			        "network=\"%s\" group=\"%s\">\n",
			        app.class_.c_str(),
			        app.id_,
			        app.instance_,
			        app.network_.c_str(),
			        app.group_.c_str());
			out << tmp;

			////////////////////// properties
			int foundColon = app.class_.rfind(':');
			if(foundColon >= 0)
				++foundColon;
			else
			{
				__SS__ << "Illegal XDAQApplication class name value of '" << app.class_
				       << "' - please check the entry for app ID = " << app.id_ << __E__;
				__SS_THROW__;
			}
			out << "\t\t\t<properties xmlns=\"urn:xdaq-application:"
			    << app.class_.substr(foundColon) << "\" xsi:type=\"soapenc:Struct\">\n";

			//__COUT__ << "app.properties_ " << app.properties_.size() << __E__;
			for(XDAQApplicationProperty& appProperty : app.properties_)
			{
				if(appProperty.type_ == "ots::SupervisorProperty")
					continue;  // skip ots Property values

				if(!appProperty.status_)
					out << "\t\t\t\t<!--\n";

				sprintf(tmp,
				        "\t\t\t\t<%s xsi:type=\"%s\">%s</%s>\n",
				        appProperty.name_.c_str(),
				        appProperty.type_.c_str(),
				        appProperty.value_.c_str(),
				        appProperty.name_.c_str());
				out << tmp;

				if(!appProperty.status_)
					out << "\t\t\t\t-->\n";
			}
			out << "\t\t\t</properties>\n";
			////////////////////// end properties

			out << "\t\t</xc:Application>\n";

			sprintf(tmp, "\t\t<xc:Module>%s</xc:Module>\n", app.module_.c_str());
			out << tmp;

			if(context.status_ && !app.status_)
				out << "\t\t-->\n";
			out << "\n";

			//	__COUT__ << "DONE" << __E__;
		}

		out << "\t</xc:Context>\n";
		if(!context.status_)
			out << "\t-->\n";
		out << "\n";
	}

	//__COUT__ << "DONE" << __E__;
	out << "</xc:Partition>\n\n\n";
}

//========================================================================================================================
std::string XDAQContextTable::getContextUID(const std::string& url) const
{
	for(auto context : contexts_)
	{
		if(!context.status_)
			continue;

		if(url == context.address_ + ":" + std::to_string(context.port_))
			return context.contextUID_;
	}
	return "";
}

//========================================================================================================================
std::string XDAQContextTable::getApplicationUID(const std::string& url,
                                                unsigned int       id) const
{
	//__COUTV__(url); __COUTV__(id);
	for(auto context : contexts_)
	{
		//__COUT__ << "Checking " << (context.address_ + ":" +
		// std::to_string(context.port_)) << __E__;
		//__COUTV__(context.status_);

		if(!context.status_)
			continue;

		//__COUT__ << "Checking " << (context.address_ + ":" +
		// std::to_string(context.port_)) << __E__;
		if(url == context.address_ + ":" + std::to_string(context.port_))
			for(auto application : context.applications_)
			{
				//__COUTV__(application.status_);	__COUTV__(application.id_);
				if(!application.status_)
					continue;

				if(application.id_ == id)
				{
					return application.applicationUID_;
				}
			}
	}
	return "";
}

DEFINE_OTS_TABLE(XDAQContextTable)
