#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/Macros/TablePluginMacros.h"
#include "otsdaq-core/TablePlugins/ARTDAQBoardReaderTable.h"
#include "otsdaq-core/TablePlugins/XDAQContextTable.h"

#include <stdio.h>
#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>

using namespace ots;

#define ARTDAQ_FCL_PATH std::string(__ENV__("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE "boardReader"

// helpers
#define OUT out << tabStr << commentStr
#define PUSHTAB tabStr += "\t"
#define POPTAB tabStr.resize(tabStr.size() - 1)
#define PUSHCOMMENT commentStr += "# "
#define POPCOMMENT commentStr.resize(commentStr.size() - 2)

//========================================================================================================================
ARTDAQBoardReaderTable::ARTDAQBoardReaderTable(void) : TableBase("ARTDAQBoardReaderTable")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////
}

//========================================================================================================================
ARTDAQBoardReaderTable::~ARTDAQBoardReaderTable(void) {}

//========================================================================================================================
void ARTDAQBoardReaderTable::init(ConfigurationManager* configManager)
{
	// use isFirstAppInContext to only run once per context, for example to avoid
	//	generating files on local disk multiple times.
	bool isFirstAppInContext = configManager->isOwnerFirstAppInContext();

	//__COUTV__(isFirstAppInContext);
	if(!isFirstAppInContext)
		return;

	// make directory just in case
	mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);

	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << __E__;
	__COUT__ << configManager->__SELF_NODE__ << __E__;

	const XDAQContextTable* contextConfig =
	    configManager->__GET_CONFIG__(XDAQContextTable);

	std::vector<const XDAQContextTable::XDAQContext*> readerContexts =
	    contextConfig->getBoardReaderContexts();

	// for each reader context
	//	do NOT output associated fcl config file
	//  but do check the link is to a DataManager-esque thing
	for(auto& readerContext : readerContexts)
	{
		try
		{
			ConfigurationTree readerConfigNode = contextConfig->getSupervisorConfigNode(
			    configManager,
			    readerContext->contextUID_,
			    readerContext->applications_[0].applicationUID_);

			__COUT__ << "Path for this reader config is " << readerContext->contextUID_
			         << "/" << readerContext->applications_[0].applicationUID_ << "/"
			         << readerConfigNode.getValueAsString() << __E__;

			__COUT__ << "Checking that this reader supervisor node is DataManager-like."
			         << __E__;

			readerConfigNode.getNode("LinkToDataBufferTable").getChildren();
		}
		catch(const std::runtime_error& e)
		{
			__SS__ << "artdaq Board Readers must be instantiated as a Consumer within a "
			          "DataManager table. "
			       << "Error found while checking for LinkToDataBufferTable: " << e.what()
			       << __E__;
			__COUT_ERR__ << ss.str();
			__COUT__ << "Path for this reader config is " << readerContext->contextUID_
			         << "/" << readerContext->applications_[0].applicationUID_ << "/X"
			         << __E__;
			__COUT_ERR__ << "This board reader will likely not get instantiated "
			                "properly! Proceeding anyway with fcl generation."
			             << __E__;

			// proceed anyway, because it was really annoying to not be able to activate
			// the table group when the context is being developed also.
			//__SS_THROW__;
		}

		// artdaq Reader is not at Supervisor level like other apps
		//	it is at Consumer level!
		// outputFHICL(readerConfigNode,
		//		contextConfig);
	}

	// handle fcl file generation, wherever the level of this table

	auto        childrenMap = configManager->__SELF_NODE__.getChildren();
	std::string appUID, buffUID, consumerUID;
	for(auto& child : childrenMap)
	{
		const XDAQContextTable::XDAQContext* thisContext = nullptr;
		for(auto& readerContext : readerContexts)
		{
			ConfigurationTree readerConfigNode = contextConfig->getSupervisorConfigNode(
			    configManager,
			    readerContext->contextUID_,
			    readerContext->applications_[0].applicationUID_);
			auto dataManagerConfMap =
			    readerConfigNode.getNode("LinkToDataBufferTable").getChildren();
			for(auto dmc : dataManagerConfMap)
			{
				auto dataBufferConfMap =
				    dmc.second.getNode("LinkToDataProcessorTable").getChildren();
				for(auto dbc : dataBufferConfMap)
				{
					auto processorConfUID =
					    dbc.second.getNode("LinkToProcessorUID").getUIDAsString();
					if(processorConfUID == child.second.getValue())
					{
						__COUT__ << "Found match for context UID: "
						         << readerContext->contextUID_ << __E__;
						thisContext = readerContext;
					}
				}
			}
		}

		if(thisContext == nullptr)
		{
			__COUT_ERR__ << "Could not find matching context for this table!" << __E__;
		}
		else
		{
			if(child.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			       .getValue<bool>())
			{
				outputFHICL(configManager,
				            child.second,
				            contextConfig->getARTDAQAppRank(thisContext->contextUID_),
				            contextConfig->getContextAddress(thisContext->contextUID_),
				            contextConfig->getARTDAQDataPort(configManager,
				                                             thisContext->contextUID_),
				            contextConfig);
			}
		}
	}
}

////========================================================================================================================
// void ARTDAQBoardReaderTable::getBoardReaderParents(const ConfigurationTree
// &readerNode, 		const ConfigurationTree &contextNode, std::string &applicationUID,
//		std::string &bufferUID, std::string &consumerUID)
//{
//	//search through all board reader contexts
//	contextNode.getNode()
//}

//========================================================================================================================
std::string ARTDAQBoardReaderTable::getFHICLFilename(
    const ConfigurationTree& boardReaderNode)
{
	__COUT__ << "ARTDAQ BoardReader UID: " << boardReaderNode.getValue() << __E__;
	std::string filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
	std::string uid      = boardReaderNode.getValue();
	for(unsigned int i = 0; i < uid.size(); ++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') || (uid[i] >= 'A' && uid[i] <= 'Z') ||
		   (uid[i] >= '0' && uid[i] <= '9'))  // only allow alpha numeric in file name
			filename += uid[i];

	filename += ".fcl";

	__COUT__ << "fcl: " << filename << __E__;

	return filename;
}

//========================================================================================================================
void ARTDAQBoardReaderTable::outputFHICL(ConfigurationManager*    configManager,
                                         const ConfigurationTree& boardReaderNode,
                                         unsigned int             selfRank,
                                         std::string              selfHost,
                                         unsigned int             selfPort,
                                         const XDAQContextTable*  contextConfig)
{
	/*
	    the file will look something like this:

	      daq: {
	          fragment_receiver: {
	            mpi_sync_interval: 50

	            # CommandableFragmentGenerator Table:
	        fragment_ids: []
	        fragment_id: -99 # Please define only one of these

	        sleep_on_stop_us: 0

	        requests_enabled: false # Whether to set up the socket for listening for
	   trigger messages request_mode: "Ignored" # Possible values are: Ignored, Single,
	   Buffer, Window

	        data_buffer_depth_fragments: 1000
	        data_buffer_depth_mb: 1000

	        request_port: 3001
	        request_address: "227.128.12.26" # Multicast request address

	        request_window_offset: 0 # Request message contains tzero. Window will be from
	   tzero - offset to tzero + width request_window_width: 0 stale_request_timeout:
	   "0xFFFFFFFF" # How long to wait before discarding request messages that are outside
	   the available data request_windows_are_unique: true # If request windows are
	   unique, avoids a copy operation, but the same data point cannot be used for two
	   requests. If this is not anticipated, leave set to "true"

	        separate_data_thread: false # MUST be true for triggers to be applied! If
	   triggering is not desired, but a separate readout thread is, set this to true,
	   triggers_enabled to false and trigger_mode to ignored. separate_monitoring_thread:
	   false # Whether a thread should be started which periodically calls checkHWStatus_,
	   a user-defined function which should be used to check hardware status registers and
	   report to MetricMan. poll_hardware_status: false # Whether checkHWStatus_ will be
	   called, either through the thread or at the start of getNext
	        hardware_poll_interval_us: 1000000 # If hardware monitoring thread is enabled,
	   how often should it call checkHWStatus_


	            # Generated Parameters:
	            generator: ToySimulator
	            fragment_type: TOY1
	            fragment_id: 0
	            board_id: 0
	            starting_fragment_id: 0
	            random_seed: 5780
	            sleep_on_stop_us: 500000

	            # Generator-Specific Table:

	        nADCcounts: 40

	        throttle_usecs: 100000

	        distribution_type: 1

	        timestamp_scale_factor: 1


	            destinations: {
	              d2: { transferPluginType: MPI
	                      destination_rank: 2
	                       max_fragment_size_bytes: 2097152
	                       host_map: [
	           {
	              host: "mu2edaq01.fnal.gov"
	              rank: 0
	           },
	           {
	              host: "mu2edaq01.fnal.gov"
	              rank: 1
	           }]
	                       }
	               d3: { transferPluginType: MPI
	                       destination_rank: 3
	                       max_fragment_size_bytes: 2097152
	                       host_map: [
	           {
	              host: "mu2edaq01.fnal.gov"
	              rank: 0
	           },
	           {
	              host: "mu2edaq01.fnal.gov"
	              rank: 1
	           }]
	           }

	            }
	          }

	          metrics: {
	            brFile: {
	              metricPluginType: "file"
	              level: 3
	              fileName: "/tmp/boardreader/br_%UID%_metrics.log"
	              uniquify: true
	            }
	            # ganglia: {
	            #   metricPluginType: "ganglia"
	            #   level: %{ganglia_level}
	            #   reporting_interval: 15.0
	            #
	            #   configFile: "/etc/ganglia/gmond.conf"
	            #   group: "ARTDAQ"
	            # }
	            # msgfac: {
	            #    level: %{mf_level}
	            #    metricPluginType: "msgFacility"
	            #    output_message_application_name: "ARTDAQ Metric"
	            #    output_message_severity: 0
	            # }
	            # graphite: {
	            #   level: %{graphite_level}
	            #   metricPluginType: "graphite"
	            #   host: "localhost"
	            #   port: 20030
	            #   namespace: "artdaq."
	            # }
	          }
	        }

	 */

	std::string filename = getFHICLFilename(boardReaderNode);

	/////////////////////////
	// generate xdaq run parameter file
	std::fstream out;

	std::string tabStr     = "";
	std::string commentStr = "";

	out.open(filename, std::fstream::out | std::fstream::trunc);
	if(out.fail())
	{
		__SS__ << "Failed to open ARTDAQ Builder fcl file: " << filename << __E__;
		__SS_THROW__;
	}

	//--------------------------------------
	// header
	OUT << "###########################################################" << __E__;
	OUT << "#" << __E__;
	OUT << "# artdaq reader fcl configuration file produced by otsdaq." << __E__;
	OUT << "# 	Creation timestamp: " << StringMacros::getTimestampString() << __E__;
	OUT << "# 	Original filename: " << filename << __E__;
	OUT << "#	otsdaq-ARTDAQ Reader UID: " << boardReaderNode.getValue() << __E__;
	OUT << "#" << __E__;
	OUT << "###########################################################" << __E__;
	OUT << "\n\n";

	// no primary link to table tree for reader node!
	try
	{
		if(boardReaderNode.isDisconnected())
		{
			// create empty fcl
			OUT << "{}\n\n";
			out.close();
			return;
		}
	}
	catch(const std::runtime_error&)
	{
		__COUT__ << "Ignoring error, assume this a valid UID node." << __E__;
		// error is expected here for UIDs.. so just ignore
		// this check is valuable if source node is a unique-Link node, rather than UID
	}

	//--------------------------------------
	// handle daq
	OUT << "daq: {\n";

	// fragment_receiver
	PUSHTAB;
	OUT << "fragment_receiver: {\n";

	PUSHTAB;
	{
		// plugin type and fragment data-type
		OUT << "generator"
		    << ": " << boardReaderNode.getNode("daqGeneratorPluginType").getValue()
		    << ("\t #daq generator plug-in type") << "\n";
		OUT << "fragment_type"
		    << ": " << boardReaderNode.getNode("daqGeneratorFragmentType").getValue()
		    << ("\t #generator data fragment type") << "\n\n";

		// shared and unique parameters
		auto parametersLink = boardReaderNode.getNode("daqParametersLink");
		if(!parametersLink.isDisconnected())
		{
			auto parameters = parametersLink.getChildren();
			for(auto& parameter : parameters)
			{
				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					PUSHCOMMENT;

				//				__COUT__ <<
				// parameter.second.getNode("daqParameterKey").getValue() <<
				//						": " <<
				//						parameter.second.getNode("daqParameterValue").getValue()
				//						<<
				//						"\n";

				auto comment =
				    parameter.second.getNode(TableViewColumnInfo::COL_NAME_COMMENT);
				OUT << parameter.second.getNode("daqParameterKey").getValue() << ": "
				    << parameter.second.getNode("daqParameterValue").getValue()
				    << (comment.isDefaultValue() ? "" : ("\t # " + comment.getValue()))
				    << "\n";

				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					POPCOMMENT;
			}
		}
		OUT << "\n";  // end daq board reader parameters
	}
	//	{	//unique parameters
	//		auto parametersLink = boardReaderNode.getNode("daqUniqueParametersLink");
	//		if(!parametersLink.isDisconnected())
	//		{
	//
	//			auto parameters = parametersLink.getChildren();
	//			for(auto &parameter:parameters)
	//			{
	//				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
	//					PUSHCOMMENT;
	//
	//				auto comment =
	// parameter.second.getNode(TableViewColumnInfo::COL_NAME_COMMENT); 				OUT
	// <<  parameter.second.getNode("daqParameterKey").getValue() <<
	//						": " <<
	//						parameter.second.getNode("daqParameterValue").getValue()
	//						<<
	//						(comment.isDefaultValue()?"":("\t # " + comment.getValue()))
	//<<
	//						"\n";
	//
	//				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
	//					POPCOMMENT;
	//			}
	//		}
	//		OUT << "\n";	//end shared daq board reader parameters
	//	}

	OUT << "destinations: {\n";

	PUSHTAB;
	auto destinationsGroup = boardReaderNode.getNode("daqDestinationsLink");
	if(!destinationsGroup.isDisconnected())
	{
		try
		{
			auto destinations = destinationsGroup.getChildren();
			for(auto& destination : destinations)
			{
				auto destinationContextUID =
				    destination.second.getNode("destinationARTDAQContextLink")
				        .getValueAsString();

				unsigned int destinationRank = -1;
				try
				{
					destinationRank =
					    contextConfig->getARTDAQAppRank(destinationContextUID);
				}
				catch(const std::runtime_error& e)
				{
					__MCOUT_WARN__(
					    "Are the DAQ destinations valid? "
					    << "Perhaps a Context has been turned off? "
					    << "Skipping destination due to an error looking for Board "
					    << "Reader DAQ source context '" << destinationContextUID
					    << "' for UID '" << boardReaderNode.getValue()
					    << "': " << e.what() << __E__);
					continue;
				}

				std::string host =
				    contextConfig->getContextAddress(destinationContextUID);
				unsigned int port = contextConfig->getARTDAQDataPort(
				    configManager, destinationContextUID);

				// open destination object
				OUT << destination.second.getNode("destinationKey").getValue() << ": {\n";
				PUSHTAB;

				OUT << "transferPluginType: "
				    << destination.second.getNode("transferPluginType").getValue()
				    << __E__;

				OUT << "destination_rank: " << destinationRank << __E__;

				try
				{
					auto mfsb = destination.second
					                .getNode("ARTDAQGlobalTableLink/maxFragmentSizeBytes")
					                .getValue<unsigned long long>();
					OUT << "max_fragment_size_bytes: " << mfsb << __E__;
					OUT << "max_fragment_size_words: " << (mfsb / 8) << __E__;
				}
				catch(...)
				{
					__SS__ << "The field ARTDAQGlobalTableLink/maxFragmentSizeBytes "
					          "could not be accessed. Make sure the link is valid."
					       << __E__;
					__SS_THROW__;
				}

				OUT << "host_map: [\n";
				PUSHTAB;
				OUT << "{\n";
				PUSHTAB;
				OUT << "rank: " << destinationRank << __E__;
				OUT << "host: \"" << host << "\"" << __E__;
				OUT << "portOffset: " << std::to_string(port) << __E__;
				POPTAB;
				OUT << "},\n";
				OUT << "{\n";
				PUSHTAB;
				OUT << "rank: " << selfRank << __E__;
				OUT << "host: \"" << selfHost << "\"" << __E__;
				OUT << "portOffset: " << std::to_string(selfPort) << __E__;
				POPTAB;
				OUT << "}" << __E__;
				POPTAB;
				OUT << "]" << __E__;  // close host_map

				POPTAB;
				OUT << "}" << __E__;  // close destination object
			}
		}
		catch(const std::runtime_error& e)
		{
			__SS__ << "Are the DAQ destinations valid? Error occurred looking for Board "
			          "Reader DAQ sources for UID '"
			       << boardReaderNode.getValue() << "': " << e.what() << __E__;
			__SS_THROW__;
		}
	}
	POPTAB;
	OUT << "}\n\n";  // end destinations

	POPTAB;
	OUT << "}\n\n";  // end fragment_receiver

	OUT << "metrics: {\n";

	PUSHTAB;
	auto metricsGroup = boardReaderNode.getNode("daqMetricsLink");
	if(!metricsGroup.isDisconnected())
	{
		auto metrics = metricsGroup.getChildren();

		for(auto& metric : metrics)
		{
			if(!metric.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				PUSHCOMMENT;

			OUT << metric.second.getNode("metricKey").getValue() << ": {\n";
			PUSHTAB;

			OUT << "metricPluginType: "
			    << metric.second.getNode("metricPluginType").getValue() << "\n";
			OUT << "level: " << metric.second.getNode("metricLevel").getValue() << "\n";

			auto metricParametersGroup = metric.second.getNode("metricParametersLink");
			if(!metricParametersGroup.isDisconnected())
			{
				auto metricParameters = metricParametersGroup.getChildren();
				for(auto& metricParameter : metricParameters)
				{
					if(!metricParameter.second
					        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
					        .getValue<bool>())
						PUSHCOMMENT;

					OUT << metricParameter.second.getNode("metricParameterKey").getValue()
					    << ": "
					    << metricParameter.second.getNode("metricParameterValue")
					           .getValue()
					    << "\n";

					if(!metricParameter.second
					        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
					        .getValue<bool>())
						POPCOMMENT;
				}
			}
			POPTAB;
			OUT << "}\n\n";  // end metric

			if(!metric.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				POPCOMMENT;
		}
	}
	POPTAB;
	OUT << "}\n\n";  // end metrics

	POPTAB;
	OUT << "}\n\n";  // end daq

	out.close();
}

DEFINE_OTS_TABLE(ARTDAQBoardReaderTable)
