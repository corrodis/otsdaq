#include "otsdaq-core/ConfigurationPluginDataFormats/ARTDAQBoardReaderConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"

#include <iostream>
#include <fstream>      // std::fstream
#include <stdio.h>
#include <sys/stat.h> 	//for mkdir

using namespace ots;


#define ARTDAQ_FCL_PATH			std::string(getenv("USER_DATA")) + "/"+ "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE	"boardReader"

//helpers
#define OUT						out << tabStr << commentStr
#define PUSHTAB					tabStr += "\t"
#define POPTAB					tabStr.resize(tabStr.size()-1)
#define PUSHCOMMENT				commentStr += "# "
#define POPCOMMENT				commentStr.resize(commentStr.size()-2)


//========================================================================================================================
ARTDAQBoardReaderConfiguration::ARTDAQBoardReaderConfiguration(void)
	: ConfigurationBase("ARTDAQBoardReaderConfiguration")
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names used in C++ MUST match the Configuration INFO  //
	//////////////////////////////////////////////////////////////////////

}

//========================================================================================================================
ARTDAQBoardReaderConfiguration::~ARTDAQBoardReaderConfiguration(void)
{}

//========================================================================================================================
void ARTDAQBoardReaderConfiguration::init(ConfigurationManager* configManager)
{
	//make directory just in case
	mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);

	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	const XDAQContextConfiguration *contextConfig = configManager->__GET_CONFIG__(XDAQContextConfiguration);
	
	std::vector<const XDAQContextConfiguration::XDAQContext *> readerContexts =
		contextConfig->getBoardReaderContexts();


	//for each reader context
	//	do NOT output associated fcl config file
	//  but do check the link is to a DataManager-esque thing
	for (auto &readerContext : readerContexts)
	{
		try
		{
			ConfigurationTree readerConfigNode = contextConfig->getSupervisorConfigNode(configManager,
				readerContext->contextUID_, readerContext->applications_[0].applicationUID_);

			__COUT__ << "Path for this reader config is " <<
				readerContext->contextUID_ << "/" <<
				readerContext->applications_[0].applicationUID_ << "/" <<
				readerConfigNode.getValueAsString() <<
				std::endl;

			__COUT__ << "Checking that this reader supervisor node is DataManager-like." << std::endl;

			readerConfigNode.getNode("LinkToDataManagerConfiguration").getChildren();

		}
		catch (const std::runtime_error& e)
		{
			__SS__ << "artdaq Board Readers must be instantiated as a Consumer within a DataManager configuration. Error found while checking for LinkToDataManagerConfiguration: " <<
				e.what() << std::endl;
			__COUT_ERR__ << ss.str();
			__COUT__ << "Path for this reader config is " <<
				readerContext->contextUID_ << "/" <<
				readerContext->applications_[0].applicationUID_ << "/X" << std::endl;
			__COUT_ERR__ << "This board reader will likely not get instantiated properly! Proceeding anyway with fcl generation." << std::endl;

			//proceed anyway, because it was really annoying to not be able to activate the configuration group when the context is being developed also.
			//throw std::runtime_error(ss.str());
		}

		//artdaq Reader is not at Supervisor level like other apps
		//	it is at Consumer level!
		//outputFHICL(readerConfigNode,
		//		contextConfig);
	}

	//handle fcl file generation, wherever the level of this configuration

	auto childrenMap = configManager->__SELF_NODE__.getChildren();
	std::string appUID, buffUID, consumerUID;
	for (auto &child : childrenMap)
	{
		const XDAQContextConfiguration::XDAQContext * thisContext = nullptr;
		for (auto& readerContext : readerContexts) {
			ConfigurationTree readerConfigNode = contextConfig->getSupervisorConfigNode(configManager,
				readerContext->contextUID_, readerContext->applications_[0].applicationUID_);
			auto dataManagerConfMap = readerConfigNode.getNode("LinkToDataManagerConfiguration").getChildren();
			for (auto dmc : dataManagerConfMap) {
				auto dataBufferConfMap = dmc.second.getNode("LinkToDataBufferConfiguration").getChildren();
				for (auto dbc : dataBufferConfMap) {
					auto processorConfUID = dbc.second.getNode("LinkToProcessorUID").getUIDAsString();
					if (processorConfUID == child.second.getValue()) {
						__COUT__ << "Found match for context UID: " << readerContext->contextUID_ << std::endl;
						thisContext = readerContext;
					}
				}
			}
		}

		if (thisContext == nullptr) {
			__COUT_ERR__ << "Could not find matching context for this configuration!" << std::endl;
		}
		else {
			if (child.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
			{
//				outputFHICL(child.second,
//					contextConfig->getARTDAQAppRank(thisContext->contextUID_),
//					contextConfig->getContextAddress(thisContext->contextUID_),
//					contextConfig->getARTDAQDataPort(thisContext->contextUID_),
//					contextConfig);

				outputFHICL(child.second,
						thisContext->applications_[0].getNode("Id").getValue<unsigned int>(),
						contextConfig->getContextAddress(thisContext->contextUID_),
						thisContext->applications_[0].getNode("Id").getValue<unsigned int>()
						contextConfig);
			}
		}
	}
}

////========================================================================================================================
//void ARTDAQBoardReaderConfiguration::getBoardReaderParents(const ConfigurationTree &readerNode,
//		const ConfigurationTree &contextNode, std::string &applicationUID,
//		std::string &bufferUID, std::string &consumerUID)
//{
//	//search through all board reader contexts
//	contextNode.getNode()
//}

//========================================================================================================================
std::string ARTDAQBoardReaderConfiguration::getFHICLFilename(const ConfigurationTree &boardReaderNode)
{
	__COUT__ << "ARTDAQ BoardReader UID: " << boardReaderNode.getValue() << std::endl;
	std::string filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
	std::string uid = boardReaderNode.getValue();
	for (unsigned int i = 0; i < uid.size(); ++i)
		if ((uid[i] >= 'a' && uid[i] <= 'z') ||
			(uid[i] >= 'A' && uid[i] <= 'Z') ||
			(uid[i] >= '0' && uid[i] <= '9')) //only allow alpha numeric in file name
			filename += uid[i];

	filename += ".fcl";

	__COUT__ << "fcl: " << filename << std::endl;

	return filename;
}

//========================================================================================================================
void ARTDAQBoardReaderConfiguration::outputFHICL(const ConfigurationTree &boardReaderNode, unsigned int selfRank, std::string selfHost, unsigned int selfPort,
	const XDAQContextConfiguration *contextConfig)
{
	/*
		the file will look something like this:

		  daq: {
			  fragment_receiver: {
				mpi_sync_interval: 50

				# CommandableFragmentGenerator Configuration:
			fragment_ids: []
			fragment_id: -99 # Please define only one of these

			sleep_on_stop_us: 0

			requests_enabled: false # Whether to set up the socket for listening for trigger messages
			request_mode: "Ignored" # Possible values are: Ignored, Single, Buffer, Window

			data_buffer_depth_fragments: 1000
			data_buffer_depth_mb: 1000

			request_port: 3001
			request_address: "227.128.12.26" # Multicast request address

			request_window_offset: 0 # Request message contains tzero. Window will be from tzero - offset to tzero + width
			request_window_width: 0
			stale_request_timeout: "0xFFFFFFFF" # How long to wait before discarding request messages that are outside the available data
			request_windows_are_unique: true # If request windows are unique, avoids a copy operation, but the same data point cannot be used for two requests. If this is not anticipated, leave set to "true"

			separate_data_thread: false # MUST be true for triggers to be applied! If triggering is not desired, but a separate readout thread is, set this to true, triggers_enabled to false and trigger_mode to ignored.
			separate_monitoring_thread: false # Whether a thread should be started which periodically calls checkHWStatus_, a user-defined function which should be used to check hardware status registers and report to MetricMan.
			poll_hardware_status: false # Whether checkHWStatus_ will be called, either through the thread or at the start of getNext
			hardware_poll_interval_us: 1000000 # If hardware monitoring thread is enabled, how often should it call checkHWStatus_


				# Generated Parameters:
				generator: ToySimulator
				fragment_type: TOY1
				fragment_id: 0
				board_id: 0
				starting_fragment_id: 0
				random_seed: 5780
				sleep_on_stop_us: 500000

				# Generator-Specific Configuration:

			nADCcounts: 40

			throttle_usecs: 100000

			distribution_type: 1

			timestamp_scale_factor: 1


				destinations: {
				  d2: { transferPluginType: MPI destination_rank: 2 max_fragment_size_words: 2097152}
			d3: { transferPluginType: MPI destination_rank: 3 max_fragment_size_words: 2097152}

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
	//generate xdaq run parameter file
	std::fstream out;

	std::string tabStr = "";
	std::string commentStr = "";

	out.open(filename, std::fstream::out | std::fstream::trunc);
	if (out.fail())
	{
		__SS__ << "Failed to open ARTDAQ Builder fcl file: " << filename << std::endl;
		throw std::runtime_error(ss.str());
	}

	//no primary link to configuration tree for reader node!
	try
	{
		if (boardReaderNode.isDisconnected())
		{
			//create empty fcl
			OUT << "{}\n\n";
			out.close();
			return;
		}
	}
	catch (const std::runtime_error)
	{
		__COUT__ << "Ignoring error, assume this a valid UID node." << std::endl;
		//error is expected here for UIDs.. so just ignore
		// this check is valuable if source node is a unique-Link node, rather than UID
	}

	//--------------------------------------
	//handle daq
	OUT << "daq: {\n";

	//fragment_receiver
	PUSHTAB;
	OUT << "fragment_receiver: {\n";

	PUSHTAB;
	{
		//plugin type and fragment data-type
		OUT << "generator" <<
			": " <<
			boardReaderNode.getNode("daqGeneratorPluginType").getValue() <<
			("\t #daq generator plug-in type") <<
			"\n";
		OUT << "fragment_type" <<
			": " <<
			boardReaderNode.getNode("daqGeneratorFragmentType").getValue() <<
			("\t #generator data fragment type") <<
			"\n\n";

		//shared and unique parameters
		auto parametersLink = boardReaderNode.getNode("daqParametersLink");
		if (!parametersLink.isDisconnected())
		{

			auto parameters = parametersLink.getChildren();
			for (auto &parameter : parameters)
			{
				if (!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					PUSHCOMMENT;

				//				__COUT__ << parameter.second.getNode("daqParameterKey").getValue() <<
				//						": " <<
				//						parameter.second.getNode("daqParameterValue").getValue()
				//						<<
				//						"\n";

				auto comment = parameter.second.getNode("CommentDescription");
				OUT << parameter.second.getNode("daqParameterKey").getValue() <<
					": " <<
					parameter.second.getNode("daqParameterValue").getValue()
					<<
					(comment.isDefaultValue() ? "" : ("\t # " + comment.getValue())) <<
					"\n";

				if (!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					POPCOMMENT;
			}
		}
		OUT << "\n";	//end daq board reader parameters
	}
	//	{	//unique parameters
	//		auto parametersLink = boardReaderNode.getNode("daqUniqueParametersLink");
	//		if(!parametersLink.isDisconnected())
	//		{
	//
	//			auto parameters = parametersLink.getChildren();
	//			for(auto &parameter:parameters)
	//			{
	//				if(!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
	//					PUSHCOMMENT;
	//
	//				auto comment = parameter.second.getNode("CommentDescription");
	//				OUT << parameter.second.getNode("daqParameterKey").getValue() <<
	//						": " <<
	//						parameter.second.getNode("daqParameterValue").getValue()
	//						<<
	//						(comment.isDefaultValue()?"":("\t # " + comment.getValue())) <<
	//						"\n";
	//
	//				if(!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
	//					POPCOMMENT;
	//			}
	//		}
	//		OUT << "\n";	//end shared daq board reader parameters
	//	}

	OUT << "destinations: {\n";

	PUSHTAB;
	auto destinationsGroup = boardReaderNode.getNode("daqDestinationsLink");
	if (!destinationsGroup.isDisconnected())
	{
		try
		{
			auto destinations = destinationsGroup.getChildren();
			for (auto &destination : destinations)
			{
				auto destinationContextUID = destination.second.getNode("destinationARTDAQContextLink").getValueAsString();
				unsigned int destinationRank = contextConfig->getARTDAQAppRank(destinationContextUID);
				std::string host = contextConfig->getContextAddress(destinationContextUID);
				unsigned int port = contextConfig->getARTDAQDataPort(
						boardReaderNode.getConfigurationManager(),
						destinationContextUID);
				
				OUT << destination.second.getNode("destinationKey").getValue() <<
					": {" <<
					" transferPluginType: " <<
					destination.second.getNode("transferPluginType").getValue() <<
					" destination_rank: " <<
					destinationRank <<
					" max_fragment_size_words: " <<
					destination.second.getNode("ARTDAQGlobalConfigurationLink/maxFragmentSizeWords").getValue<unsigned int>() <<
					" host_map: [{rank: " << destinationRank << " host: \"" << host << "\" portOffset: " << std::to_string(port) << "}, " <<
					"{rank: " << selfRank << " host: \"" << selfHost << "\" portOffset: " << std::to_string(selfPort) << "}]" <<
					"}\n";
			}
		}
		catch (const std::runtime_error& e)
		{
			__SS__ << "Are the DAQ destinations valid? Error occurred looking for Board Reader DAQ sources for UID '" <<
				boardReaderNode.getValue() << "': " << e.what() << std::endl;
			__COUT_ERR__ << ss.str() << std::endl;
			throw std::runtime_error(ss.str());
		}
	}
	POPTAB;
	OUT << "}\n\n";	//end destinations

	POPTAB;
	OUT << "}\n\n";	//end fragment_receiver


	OUT << "metrics: {\n";

	PUSHTAB;
	auto metricsGroup = boardReaderNode.getNode("daqMetricsLink");
	if (!metricsGroup.isDisconnected())
	{
		auto metrics = metricsGroup.getChildren();

		for (auto &metric : metrics)
		{
			if (!metric.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
				PUSHCOMMENT;

			OUT << metric.second.getNode("metricKey").getValue() <<
				": {\n";
			PUSHTAB;

			OUT << "metricPluginType: " <<
				metric.second.getNode("metricPluginType").getValue()
				<< "\n";
			OUT << "level: " <<
				metric.second.getNode("metricLevel").getValue()
				<< "\n";

			auto metricParametersGroup = metric.second.getNode("metricParametersLink");
			if (!metricParametersGroup.isDisconnected())
			{
				auto metricParameters = metricParametersGroup.getChildren();
				for (auto &metricParameter : metricParameters)
				{
					if (!metricParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
						PUSHCOMMENT;

					OUT << metricParameter.second.getNode("metricParameterKey").getValue() <<
						": " <<
						metricParameter.second.getNode("metricParameterValue").getValue()
						<< "\n";

					if (!metricParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
						POPCOMMENT;

				}
			}
			POPTAB;
			OUT << "}\n\n";	//end metric

			if (!metric.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
				POPCOMMENT;
		}
	}
	POPTAB;
	OUT << "}\n\n";	//end metrics

	POPTAB;
	OUT << "}\n\n";	//end daq


	out.close();
}

DEFINE_OTS_CONFIGURATION(ARTDAQBoardReaderConfiguration)
