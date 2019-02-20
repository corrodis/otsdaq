#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/Macros/TablePluginMacros.h"
#include "otsdaq-core/TablePluginDataFormats/XDAQContextTable.h"

#include <stdio.h>
#include <fstream>  // std::fstream
#include <iostream>

using namespace ots;

#define XDAQ_RUN_FILE                                           \
	std::string(getenv("XDAQ_CONFIGURATION_DATA_PATH")) + "/" + \
	    std::string(getenv("XDAQ_CONFIGURATION_XML")) + ".xml"
#define APP_PRIORITY_FILE                                       \
	std::string(getenv("XDAQ_CONFIGURATION_DATA_PATH")) + "/" + \
	    "xdaqAppStateMachinePriority"

//#define XDAQ_SCRIPT				std::string(getenv("XDAQ_CONFIGURATION_DATA_PATH")) +
//"/"+ "StartXDAQ_gen.sh" #define ARTDAQ_MPI_SCRIPT
// std::string(getenv("XDAQ_CONFIGURATION_DATA_PATH")) + "/"+ "StartMPI_gen.sh"

const std::string XDAQContextTable::DEPRECATED_SUPERVISOR_CLASS =
    "ots::Supervisor";  // still allowed for now, in StartOTS
const std::string XDAQContextTable::GATEWAY_SUPERVISOR_CLASS = "ots::GatewaySupervisor";
const std::string XDAQContextTable::WIZARD_SUPERVISOR_CLASS  = "ots::WizardSupervisor";
const std::set<std::string> XDAQContextTable::FETypeClassNames_ = {
    "ots::FESupervisor",
    "ots::FEDataManagerSupervisor",
    "ots::ARTDAQFEDataManagerSupervisor"};
const std::set<std::string> XDAQContextTable::DMTypeClassNames_ = {
    "ots::DataManagerSupervisor",
    "ots::FEDataManagerSupervisor",
    "ots::ARTDAQFEDataManagerSupervisor"};
const std::set<std::string> XDAQContextTable::LogbookTypeClassNames_ = {
    "ots::LogbookSupervisor"};
const std::set<std::string> XDAQContextTable::MacroMakerTypeClassNames_ = {
    "ots::MacroMakerSupervisor"};
const std::set<std::string> XDAQContextTable::ChatTypeClassNames_ = {
    "ots::ChatSupervisor"};
const std::set<std::string> XDAQContextTable::ConsoleTypeClassNames_ = {
    "ots::ConsoleSupervisor"};
const std::set<std::string> XDAQContextTable::ConfigurationGUITypeClassNames_ = {
    "ots::TableGUISupervisor"};

const std::string XDAQContextTable::ARTDAQ_OFFSET_PORT = "OffsetPort";

const uint8_t XDAQContextTable::XDAQApplication::DEFAULT_PRIORITY = 100;

//========================================================================================================================
XDAQContextTable::XDAQContextTable(void) : TableBase("XDAQContextTable")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////

	//	<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
	//	 <ROOT xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	// xsi:noNamespaceSchemaLocation="TableInfo.xsd"> 	   <TABLE
	// Name="XDAQContextTable"> 	     <VIEW Name="XDAQ_CONTEXT_TABLE"
	// Type="File,Database,DatabaseTest">
	//           <COLUMN Type="UID" 	           Name="ContextUID"
	//           StorageName="CONTEXT_UID" 		                 DataType="VARCHAR2"/>
	//           <COLUMN Type="ChildLink-0" 	   Name="LinkToApplicationTable"
	//           StorageName="LINK_TO_APPLICATION_TABLE" DataType="VARCHAR2"/>
	//           <COLUMN Type="ChildLinkGroupID-0" Name="ApplicationGroupID"
	//           StorageName="APPLICATION_GROUP_ID"              DataType="VARCHAR2"/>
	//           <COLUMN Type="OnOff" 	           Name="Status"
	//           StorageName="STATUS" 	   	                     DataType="VARCHAR2"/>
	//           <COLUMN Type="Data" 	           Name="Id"
	//           StorageName="ID" 		                         DataType="VARCHAR2"/>
	//           <COLUMN Type="Data" 	           Name="Address"
	//           StorageName="ADDRESS" 		                     DataType="VARCHAR2"/>
	//           <COLUMN Type="Data" 	           Name="Port"
	//           StorageName="PORT" 		                     DataType="VARCHAR2"/>
	//           <COLUMN Type="Data" 	           Name="ARTDAQDataPort"
	//           StorageName="ARTDAQ_DATA_PORT"  	             DataType="VARCHAR2"/>
	//           <COLUMN Type="Comment" 	       Name="CommentDescription"
	//           StorageName="COMMENT_DESCRIPTION" 	             DataType="VARCHAR2"/>
	//           <COLUMN Type="Author" 	           Name="Author"
	//           StorageName="AUTHOR" 		                     DataType="VARCHAR2"/>
	//           <COLUMN Type="Timestamp" 	       Name="RecordInsertionTime"
	//           StorageName="RECORD_INSERTION_TIME"             DataType="TIMESTAMP WITH
	//           TIMEZONE"/>
	//	     </VIEW>
	//	   </TABLE>
	//	 </ROOT>
}

//========================================================================================================================
XDAQContextTable::~XDAQContextTable(void) {}

//========================================================================================================================
void XDAQContextTable::init(ConfigurationManager* configManager)
{
	//__COUT__ << "init" << __E__;
	extractContexts(configManager);

	{
		/////////////////////////
		// generate xdaq run parameter file
		std::fstream fs;
		fs.open(XDAQ_RUN_FILE, std::fstream::out | std::fstream::trunc);
		if(fs.fail())
		{
			__SS__ << "Failed to open XDAQ run file: " << XDAQ_RUN_FILE << std::endl;
			__SS_THROW__;
		}
		outputXDAQXML((std::ostream&)fs);
		fs.close();
	}
}

//========================================================================================================================
// isARTDAQContext
bool XDAQContextTable::isARTDAQContext(const std::string& contextUID)
{
	return (contextUID.find("ART") == 0 || contextUID.find("ARTDAQ") == 0);
}

//========================================================================================================================
// isARTDAQContext
//	looks through all active artdaq contexts for UID
//	throws exception if not found
//
//	if contextUID == "X" (which happens automatically for broken link)
//		then highest possible rank plus 1 is returned
unsigned int XDAQContextTable::getARTDAQAppRank(const std::string& contextUID) const
{
	//	__COUT__ << "artdaqContexts_.size() = " <<
	//			artdaqContexts_.size() << std::endl;

	if(artdaqBoardReaders_.size() == 0 && artdaqEventBuilders_.size() == 0 &&
	   artdaqAggregators_.size() == 0)
	{
		__COUT_WARN__ << "Assuming since there are 0 active ARTDAQ context UID(s), we "
		                 "can ignore rank failure."
		              << std::endl;
		return -1;
	}

	// define local "lambda" getRank function
	auto localGetRank = [](const XDAQContext& context) -> unsigned int {
		if(context.applications_.size() != 1)
		{
			__SS__ << "Invalid number of ARTDAQ applications in context '"
			       << context.contextUID_ << ".' Must be 1. Currently is "
			       << context.applications_.size() << "." << __E__;
			__SS_THROW__;
		}

		return context.applications_[0].id_;
	};

	for(auto& i : artdaqBoardReaders_)
		if(contexts_[i].contextUID_ == contextUID)
			return localGetRank(contexts_[i]);

	for(auto& i : artdaqEventBuilders_)
		if(contexts_[i].contextUID_ == contextUID)
			return localGetRank(contexts_[i]);

	for(auto& i : artdaqAggregators_)
		if(contexts_[i].contextUID_ == contextUID)
			return localGetRank(contexts_[i]);

	// if (contextUID == "X")
	//	return -1; //assume disconnected link should not error?

	__SS__ << "ARTDAQ rank could not be found for context UID '" << contextUID << ".'"
	       << std::endl;
	__SS_THROW__;  // should never happen!
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
}

unsigned int XDAQContextTable::getARTDAQDataPort(
    const ConfigurationManager* configManager, const std::string& contextUID) const
{
	if(contextUID == "X")
		return 0;
	for(auto& context : contexts_)
	{
		if(context.contextUID_ == contextUID)
		{
			if(context.applications_.size() != 1)
			{
				__SS__ << "Invalid number of ARTDAQ applications in context '"
				       << contextUID << ".' Must be 1. Currently is "
				       << context.applications_.size() << "." << __E__;
				__SS_THROW__;
			}

			if(context.applications_[0].class_ == "ots::ARTDAQDataManagerSupervisor")
			{
				auto processors =
				    getSupervisorConfigNode(configManager,
				                            context.contextUID_,
				                            context.applications_[0].applicationUID_)
				        .getNode("LinkToDataBufferTable")
				        .getChildren()[0]
				        .second.getNode("LinkToDataProcessorTable")
				        .getChildren();

				std::string processorType;

				// take first board reader processor (could be Consumer or Producer)
				for(auto& processor : processors)
				{
					processorType = processor.second.getNode("ProcessorPluginName")
					                    .getValue<std::string>();
					__COUTV__(processorType);

					if(processorType == "ARTDAQConsumer" ||
					   processorType == "ARTDAQProducer")
						return processor.second.getNode("LinkToProcessorTable")
						    .getNode(XDAQContextTable::ARTDAQ_OFFSET_PORT)
						    .getValue<unsigned int>();
				}

				__SS__ << "No ARTDAQ processor was found while looking for data port."
				       << __E__;
				__SS_THROW__;
			}
			// else
			return getSupervisorConfigNode(configManager,
			                               context.contextUID_,
			                               context.applications_[0].applicationUID_)
			    .getNode(XDAQContextTable::ARTDAQ_OFFSET_PORT)
			    .getValue<unsigned int>();
		}
	}
	return 0;
}

//========================================================================================================================
std::vector<const XDAQContextTable::XDAQContext*>
XDAQContextTable::getBoardReaderContexts() const
{
	std::vector<const XDAQContext*> retVec;
	for(auto& i : artdaqBoardReaders_)
		retVec.push_back(&contexts_[i]);
	return retVec;
}
//========================================================================================================================
std::vector<const XDAQContextTable::XDAQContext*>
XDAQContextTable::getEventBuilderContexts() const
{
	std::vector<const XDAQContext*> retVec;
	for(auto& i : artdaqEventBuilders_)
		retVec.push_back(&contexts_[i]);
	return retVec;
}
//========================================================================================================================
std::vector<const XDAQContextTable::XDAQContext*>
XDAQContextTable::getAggregatorContexts() const
{
	std::vector<const XDAQContext*> retVec;
	for(auto& i : artdaqAggregators_)
		retVec.push_back(&contexts_[i]);
	return retVec;
}

//========================================================================================================================
ConfigurationTree XDAQContextTable::getContextNode(
    const ConfigurationManager* configManager, const std::string& contextUID) const
{
	return configManager->__SELF_NODE__.getNode(contextUID);
}

//========================================================================================================================
ConfigurationTree XDAQContextTable::getApplicationNode(
    const ConfigurationManager* configManager,
    const std::string&          contextUID,
    const std::string&          appUID) const
{
	return configManager->__SELF_NODE__.getNode(
	    contextUID + "/" + colContext_.colLinkToApplicationTable_ + "/" + appUID);
}

//========================================================================================================================
ConfigurationTree XDAQContextTable::getSupervisorConfigNode(
    const ConfigurationManager* configManager,
    const std::string&          contextUID,
    const std::string&          appUID) const
{
	return configManager->__SELF_NODE__.getNode(
	    contextUID + "/" + colContext_.colLinkToApplicationTable_ + "/" + appUID + "/" +
	    colApplication_.colLinkToSupervisorTable_);
}

//========================================================================================================================
// extractContexts
//	Could be called by other tables if they need to access the context.
//		This doesn't re-write config files, it just re-makes constructs in software.
void XDAQContextTable::extractContexts(ConfigurationManager* configManager)
{
	//__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	//__COUT__ << configManager->__SELF_NODE__ << std::endl;

	//	__COUT__ << configManager->getNode(this->getTableName()).getValueAsString()
	//		  											  << std::endl;

	auto children = configManager->__SELF_NODE__.getChildren();

	contexts_.clear();  // reset
	                    //	artdaqContexts_.clear();

	artdaqBoardReaders_.clear();
	artdaqEventBuilders_.clear();
	artdaqAggregators_.clear();

	// Enforce that app IDs do not repeat!
	//	Note: this is important because there are maps in MacroMaker and
	// SupervisorDescriptorInfoBase that rely on localId() as key
	std::set<unsigned int /*appId*/> appIdSet;

	for(auto& child : children)
	{
		contexts_.push_back(XDAQContext());
		//__COUT__ << child.first << std::endl;
		//		__COUT__ << child.second.getNode(colContextUID_) << std::endl;

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
		// child.second.getNode(colContext_.colARTDAQDataPort_).getValue(contexts_.back().artdaqDataPort_);

		//__COUT__ << contexts_.back().address_ << std::endl;
		auto appLink = child.second.getNode(colContext_.colLinkToApplicationTable_);
		if(appLink.isDisconnected())
		{
			__SS__ << "Application link is disconnected!" << std::endl;
			__SS_THROW__;
		}

		// add xdaq applications to this context
		auto appChildren = appLink.getChildren();
		for(auto appChild : appChildren)
		{
			//__COUT__ << "Loop: " << child.first << "/" << appChild.first << std::endl;

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
				       << contexts_.back().applications_.back().applicationUID_
				       << std::endl;
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
					       << std::endl;
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
				//<< std::endl;

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
					// std::endl;
				}
			}
			// else
			//	__COUT__ << "Disconnected." << std::endl;
		}

		// check artdaq type
		if(isARTDAQContext(contexts_.back().contextUID_))
		{
			// artdaqContexts_.push_back(contexts_.size() - 1);

			if(contexts_.back().applications_.size() != 1)
			{
				__SS__ << "ARTDAQ Context '" << contexts_.back().contextUID_
				       << "' must have one Application! "
				       << contexts_.back().applications_.size() << " were found. "
				       << std::endl;
				__SS_THROW__;
			}

			if(!contexts_.back().status_)
				continue;  // skip if disabled

			if(contexts_.back().applications_[0].class_ ==  // if board reader
			   "ots::ARTDAQDataManagerSupervisor")
				artdaqBoardReaders_.push_back(contexts_.size() - 1);
			else if(contexts_.back().applications_[0].class_ ==  // if event builder
			        "ots::EventBuilderApp")
				artdaqEventBuilders_.push_back(contexts_.size() - 1);
			else if(contexts_.back().applications_[0].class_ ==  // if aggregator
			            "ots::DataLoggerApp" ||
			        contexts_.back().applications_[0].class_ ==  // if aggregator
			            "ots::DispatcherApp")
				artdaqAggregators_.push_back(contexts_.size() - 1);
			else
			{
				__SS__ << "ARTDAQ Context must be have Application of an allowed class "
				          "type:\n "
				       << "\tots::ARTDAQDataManagerSupervisor\n"
				       << "\tots::EventBuilderApp\n"
				       << "\tots::DataLoggerApp\n"
				       << "\tots::DispatcherApp\n"
				       << "\nClass found was " << contexts_.back().applications_[0].class_
				       << std::endl;
				__SS_THROW__;
			}
		}
	}
}

//========================================================================================================================
// void XDAQContextTable::outputXDAQScript(std::ostream &out)
//{
/*
    the file will look something like this:
            #!/bin/sh

            echo
            echo

            echo "Launching XDAQ contexts..."

            #for each XDAQ context
            xdaq.exe -p ${CONTEXT_PORT} -e ${XDAQ_ARGS} &

    */

//	out << "#!/bin/sh\n\n";
//
//	out << "echo\necho\n\n";
//
//	out << "echo \"Launching XDAQ contexts...\"\n\n";
//	out << "#for each XDAQ context\n";
//
//
//	std::stringstream ss;
//	int count = 0;
//	//for each non-"ART" or "ARTDAQ" context make a xdaq entry
//	for(XDAQContext &context:contexts_)
//	{
//		if(isARTDAQContext(context.contextUID_))
//			continue;		//skip if UID does identify as artdaq
//
//		if(!context.status_)
//			continue;		//skip if disabled
//
//		//at this point we have a xdaq context.. so make an xdaq entry
//
//		++count;
//		ss << "echo \"xdaq.exe -p " <<
//				context.port_ << " -e ${XDAQ_ARGS} &\"\n";
//		ss << "xdaq.exe -p " <<
//				context.port_ << " -e ${XDAQ_ARGS} & " <<
//				"#" << context.contextUID_ << "\n";
//	}
//
//
//	if(count == 0) //if no artdaq contexts at all
//	{
//		out << "echo \"No XDAQ (non-artdaq) contexts found.\"\n\n";
//		out << "echo\necho\n";
//		return;
//	}
//
//	out << ss.str();
//
//	out << "\n\n";
//}

////========================================================================================================================
// void XDAQContextTable::outputARTDAQScript(std::ostream &out)
//{
/*
        the file will look something like this:
                #!/bin/sh


                echo
                echo
                while [ 1 ]; do

                    echo "Cleaning up old MPI instance..."
                    killall mpirun


                    echo "Starting mpi run..."
                    echo "$1"
                    echo
                    echo

                    echo mpirun $1 \
                           -np 1 xdaq.exe -p ${ARTDAQ_BOARDREADER_PORT1} -e ${XDAQ_ARGS} :
   \
                           -np 1 xdaq.exe -p ${ARTDAQ_BOARDREADER_PORT2} -e ${XDAQ_ARGS} :
   \
                           -np 1 xdaq.exe -p ${ARTDAQ_BUILDER_PORT}      -e ${XDAQ_ARGS} :
   \ -np 1 xdaq.exe -p ${ARTDAQ_AGGREGATOR_PORT}   -e ${XDAQ_ARGS} &

                    echo
                    echo

                    ret=mpirun $1 \
                        -np 1 xdaq.exe -p ${ARTDAQ_BOARDREADER_PORT1} -e ${XDAQ_ARGS} : \
                        -np 1 xdaq.exe -p ${ARTDAQ_BOARDREADER_PORT2} -e ${XDAQ_ARGS} : \
                        -np 1 xdaq.exe -p ${ARTDAQ_BUILDER_PORT}      -e ${XDAQ_ARGS} : \
                        -np 1 xdaq.exe -p ${ARTDAQ_AGGREGATOR_PORT}   -e ${XDAQ_ARGS}

                    if [ $ret -eq 0 ]; then
                        exit
                    fi

                done
         */

//	__COUT__ << artdaqContexts_.size() << " total artdaq context(s)." << std::endl;
//	__COUT__ << artdaqBoardReaders_.size() << " active artdaq board reader(s)." <<
// std::endl;
//	__COUT__ << artdaqEventBuilders_.size() << " active artdaq event builder(s)." <<
// std::endl;
//	__COUT__ << artdaqAggregators_.size() << " active artdaq aggregator(s)." << std::endl;
//
//	out << "#!/bin/sh\n\n";
//
//	out << "\techo\n\techo\n\n";
//	//out << "while [ 1 ]; do\n\n";
//
//	//out << "\techo \"Cleaning up old MPI instance...\"\n";
//
//	out << "\techo \"" <<
//			artdaqContexts_.size() << " artdaq Contexts." <<
//			"\"\n";
//	out << "\techo \"\t" <<
//			artdaqBoardReaders_.size() << " artdaq board readers." <<
//			"\"\n";
//	out << "\techo \"\t" <<
//			artdaqEventBuilders_.size() << " artdaq event builders." <<
//			"\"\n";
//	out << "\techo \"\t" <<
//			artdaqAggregators_.size() << " artdaq aggregators_." <<
//			"\"\n";
//
//	//out << "\tkillall mpirun\n";
//	out << "\techo\n\techo\n\n";
//
//	out << "\techo \"Starting mpi run...\"\n";
//	out << "\techo \"$1\"\n\techo\n\techo\n\n";
//
//
//
//	std::stringstream ss,ssUID;
//	int count = 0;
//
//	//ss << "\tret=`mpirun $1 \\\n"; `
//
//	ss << "\tmpirun $1 \\\n";
//
//	//for each "ART" or "ARTDAQ" context make an mpi entry
//	//make an mpi entry for board readers, then event builders, then aggregators
//
//	for(auto &i:artdaqBoardReaders_)
//	{
//		if(count++) //add line breaks if not first context
//			ss << ": \\\n";
//
//		ss << "     -np 1 xdaq.exe -p " <<
//				contexts_[i].port_ << " -e ${XDAQ_ARGS} ";
//
//		ssUID << "\n\t#board reader \t context.port_ \t " << contexts_[i].port_ <<
//				": \t" << contexts_[i].contextUID_;
//	}
//
//	for(auto &i:artdaqEventBuilders_)
//	{
//		if(count++) //add line breaks if not first context
//			ss << ": \\\n";
//
//		ss << "     -np 1 xdaq.exe -p " <<
//				contexts_[i].port_ << " -e ${XDAQ_ARGS} ";
//
//		ssUID << "\n\t#event builder \t context.port_ \t " << contexts_[i].port_ <<
//				": \t" << contexts_[i].contextUID_;
//	}
//
//	for(auto &i:artdaqAggregators_)
//	{
//		if(count++) //add line breaks if not first context
//			ss << ": \\\n";
//
//		ss << "     -np 1 xdaq.exe -p " <<
//				contexts_[i].port_ << " -e ${XDAQ_ARGS} ";
//
//		ssUID << "\n\t#aggregator \t context.port_ \t " << contexts_[i].port_ <<
//				": \t" << contexts_[i].contextUID_;
//	}
//
//
//	if(count == 0) //if no artdaq contexts at all
//	{
//		out << "\techo \"No ARTDAQ contexts found. So no mpirun necessary.\"\n\n";
//		out << "\techo\necho\n";
//		return;
//	}
//
//	out << "\techo \"" << ss.str() << "\""; //print mpirun
//	out << "\t\n\techo\n\techo\n\n";
//	out << ssUID.str() << "\n\n";
//	out << ss.str();			//run mpirun
//
//	out << "\n\n";
//	//out << ""\tif [ ${ret:-1} -eq 0 ]; then\n\t\texit\n\tfi\n";
//	//out << "\ndone\n";
//}
//
//
////========================================================================================================================
// void XDAQContextTable::outputAppPriority(std::ostream &out, const std::string&
// stateMachineCommand)
//{
//	//output app ID and priority order [1:255] pairs.. new line separated
//	//	0/undefined gets translated to 100
//
//	for (XDAQContext &context : contexts_)
//		for (XDAQApplication &app : context.applications_)
//		{
//			out << app.id_ << "\n";
//
//			if(app.stateMachineCommandPriority_.find(stateMachineCommand) ==
// app.stateMachineCommandPriority_.end()) 				out << 100; 			else
//				out << ((app.stateMachineCommandPriority_[stateMachineCommand])?
//					app.stateMachineCommandPriority_[stateMachineCommand]:100);
//
//			out << "\n";
//		}
//}

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
		//__COUT__ << context.contextUID_ << std::endl;

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
			//__COUT__ << app.id_ << std::endl;

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

			//__COUT__ << "app.properties_ " << app.properties_.size() << std::endl;
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
