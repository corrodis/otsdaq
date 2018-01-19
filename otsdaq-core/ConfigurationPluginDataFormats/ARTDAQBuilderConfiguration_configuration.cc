#include "otsdaq-core/ConfigurationPluginDataFormats/ARTDAQBuilderConfiguration.h"
#include "otsdaq-core/Macros/ConfigurationPluginMacros.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"

#include <iostream>
#include <fstream>      // std::fstream
#include <stdio.h>
#include <sys/stat.h> 	//for mkdir

using namespace ots;


#define ARTDAQ_FCL_PATH			std::string(getenv("USER_DATA")) + "/"+ "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE	"builder"

//helpers
#define OUT						out << tabStr << commentStr
#define PUSHTAB					tabStr += "\t"
#define POPTAB					tabStr.resize(tabStr.size()-1)
#define PUSHCOMMENT				commentStr += "# "
#define POPCOMMENT				commentStr.resize(commentStr.size()-2)


//========================================================================================================================
ARTDAQBuilderConfiguration::ARTDAQBuilderConfiguration(void)
	: ConfigurationBase("ARTDAQBuilderConfiguration")
	, configManager_(0)
{
	//////////////////////////////////////////////////////////////////////
	//WARNING: the names used in C++ MUST match the Configuration INFO  //
	//////////////////////////////////////////////////////////////////////

}

//========================================================================================================================
ARTDAQBuilderConfiguration::~ARTDAQBuilderConfiguration(void)
{}

//========================================================================================================================
void ARTDAQBuilderConfiguration::init(ConfigurationManager* configManager)
{
	configManager_ = configManager;
	//make directory just in case
	mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);

	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << std::endl;
	__COUT__ << configManager->__SELF_NODE__ << std::endl;

	const XDAQContextConfiguration *contextConfig = configManager_->__GET_CONFIG__(XDAQContextConfiguration);

	//	auto childrenMap = configManager->__SELF_NODE__.getChildren();
	//
	//	for(auto &child:childrenMap)
	//		if(child.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
	//			outputFHICL(child.second, contextConfig);

	std::vector<const XDAQContextConfiguration::XDAQContext *> builderContexts =
		contextConfig->getEventBuilderContexts();

	//for each aggregator context
	//	output associated fcl config file
	for (auto &builderContext : builderContexts)
	{
		ConfigurationTree builderConfigNode = contextConfig->getSupervisorConfigNode(configManager,
			builderContext->contextUID_, builderContext->applications_[0].applicationUID_);

		__COUT__ << "Path for this aggregator config is " <<
			builderContext->contextUID_ << "/" <<
			builderContext->applications_[0].applicationUID_ << "/" <<
			builderConfigNode.getValueAsString() <<
			std::endl;

		outputFHICL(builderConfigNode,
			contextConfig);
	}
}

//========================================================================================================================
std::string ARTDAQBuilderConfiguration::getFHICLFilename(const ConfigurationTree &builderNode)
{
	__COUT__ << "ARTDAQ Builder UID: " << builderNode.getValue() << std::endl;
	std::string filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
	std::string uid = builderNode.getValue();
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
void ARTDAQBuilderConfiguration::outputFHICL(const ConfigurationTree &builderNode,
	const XDAQContextConfiguration *contextConfig)
{
	/*
	the file will look something like this:

		services: {
		  scheduler: {
			fileMode: NOMERGE
			errorOnFailureToPut: false
		  }
		  NetMonTransportServiceInterface: {
			service_provider: NetMonTransportService
			#broadcast_sends: true
			destinations: {
			  d4: { transferPluginType: MPI destination_rank: 4 max_fragment_size_words: 2097152}

			}
		  }

		  #SimpleMemoryCheck: { }
		}

		daq: {
		  event_builder: {
			expected_fragments_per_event: 2
			use_art: true
			print_event_store_stats: true
			verbose: false
			send_requests: true
			request_port:
			request_address:

			sources: {
				s0: { transferPluginType: MPI source_rank: 0 max_fragment_size_words: 2097152}
				s1: { transferPluginType: MPI source_rank: 1 max_fragment_size_words: 2097152}

			}
		  }
		  metrics: {
			evbFile: {
			  metricPluginType: "file"
			  level: 3
			  fileName: "/tmp/eventbuilder/evb_%UID%_metrics.log"
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

		outputs: {
		  rootMPIOutput: {
			module_type: RootMPIOutput
			#SelectEvents: { SelectEvents: [ pmod2,pmod3 ] }
		  }
		  #normalOutput: {
		  #  module_type: RootOutput
		  #  fileName: "/tmp/artdaqdemo_eb00_r%06r_sr%02s_%to.root"
		  #  #SelectEvents: { SelectEvents: [ pmod2,pmod3 ] }
		  #}
		}

		physics: {
		  analyzers: {

		  }

		  producers: {
		  }

		  filters: {

			prescaleMod2: {
			   module_type: NthEvent
			   nth: 2
			}

			prescaleMod3: {
			   module_type: NthEvent
			   nth: 3
			}
		  }

		  pmod2: [ prescaleMod2 ]
		  pmod3: [ prescaleMod3 ]


		  #a1: [ app, wf ]

		  my_output_modules: [ rootMPIOutput ]
		  #my_output_modules: [ normalOutput ]
		}
		source: {
		  module_type: DemoInput
		  waiting_time: 2500000
		  resume_after_timeout: true
		}
		process_name: DAQ

	 */


	std::string filename = getFHICLFilename(builderNode);

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
	if (builderNode.isDisconnected())
	{
		//create empty fcl
		OUT << "{}\n\n";
		out.close();
		return;
	}


	//--------------------------------------
	//handle services
	auto services = builderNode.getNode("servicesLink");
	if (!services.isDisconnected())
	{
		OUT << "services: {\n";

		//scheduler
		PUSHTAB;
		OUT << "scheduler: {\n";

		PUSHTAB;
		OUT << "fileMode: " << services.getNode("schedulerFileMode").getValue() <<
			"\n";
		OUT << "errorOnFailureToPut: " <<
			(services.getNode("schedulerErrorOnFailtureToPut").getValue<bool>() ? "true" : "false") <<
			"\n";
		POPTAB;

		OUT << "}\n\n";


		//NetMonTransportServiceInterface
		OUT << "NetMonTransportServiceInterface: {\n";

		PUSHTAB;
		OUT << "service_provider: " <<
			//services.getNode("NetMonTrasportServiceInterfaceServiceProvider").getEscapedValue()
			services.getNode("NetMonTrasportServiceInterfaceServiceProvider").getValue()
			<< "\n";
		OUT << "destinations: {\n";

		PUSHTAB;
		auto destinationsGroup = services.getNode("NetMonTrasportServiceInterfaceDestinationsLink");
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
					unsigned int port = contextConfig->getARTDAQDataPort(destinationContextUID);
					OUT << destination.second.getNode("destinationKey").getValue() <<
						": {" <<
						" transferPluginType: " <<
						destination.second.getNode("transferPluginType").getValue() <<
						" destination_rank: " <<
						destinationRank <<
						" max_fragment_size_words: " <<
						destination.second.getNode("ARTDAQGlobalConfigurationLink/maxFragmentSizeWords").getValue<unsigned int>() <<
						" host_map: [{rank: " << destinationRank << " host: \"" << host << "\" portOffset: " << std::to_string(port) << "}] " <<
						"}\n";
				}
			}
			catch (const std::runtime_error& e)
			{
				__SS__ << "Are the Net Monitor Transport Service destinations valid? Error occurred looking for Event Builder transport service destinations for UID '" <<
					builderNode.getValue() << "': " << e.what() << std::endl;
				__COUT_ERR__ << ss.str() << std::endl;
				throw std::runtime_error(ss.str());
			}
		}
		POPTAB;
		OUT << "}\n\n";	//end destinations

		POPTAB;
		OUT << "}\n\n";	//end NetMonTransportServiceInterface

		POPTAB;
		OUT << "}\n\n"; //end services

	}

	//--------------------------------------
	//handle daq
	auto daq = builderNode.getNode("daqLink");
	if (!daq.isDisconnected())
	{
		///////////////////////
		OUT << "daq: {\n";

		//event_builder
		PUSHTAB;
		OUT << "event_builder: {\n";

		PUSHTAB;
		auto parametersLink = daq.getNode("daqEventBuilderParametersLink");
		if (!parametersLink.isDisconnected())
		{

			auto parameters = parametersLink.getChildren();
			for (auto &parameter : parameters)
			{
				if (!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					PUSHCOMMENT;

				OUT << parameter.second.getNode("daqParameterKey").getValue() <<
					": " <<
					parameter.second.getNode("daqParameterValue").getValue()
					<< "\n";

				if (!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					POPCOMMENT;
			}
		}
		OUT << "\n";	//end daq event builder parameters


		OUT << "sources: {\n";

		PUSHTAB;
		auto sourcesGroup = daq.getNode("daqEventBuilderSourcesLink");
		if (!sourcesGroup.isDisconnected())
		{
			try
			{
				auto sources = sourcesGroup.getChildren();
				for (auto &source : sources)
				{
					auto sourceContextUID = source.second.getNode("sourceARTDAQContextLink").getValueAsString();
					unsigned int sourceRank = contextConfig->getARTDAQAppRank(sourceContextUID);
					std::string host = contextConfig->getContextAddress(sourceContextUID);
					unsigned int port = contextConfig->getARTDAQDataPort(sourceContextUID);

					OUT << source.second.getNode("sourceKey").getValue() <<
						": {" <<
						" transferPluginType: " <<
						source.second.getNode("transferPluginType").getValue() <<
						" source_rank: " <<
						sourceRank <<
						" max_fragment_size_words: " <<
						source.second.getNode("ARTDAQGlobalConfigurationLink/maxFragmentSizeWords").getValue<unsigned int>() <<
						" host_map: [{rank: " << sourceRank << " host: \"" << host << "\" portOffset: " << std::to_string(port) << "}] " <<
						"}\n";
				}
			}
			catch (const std::runtime_error& e)
			{
				__SS__ << "Are the DAQ sources valid? Error occurred looking for Event Builder DAQ sources for UID '" <<
					builderNode.getValue() << "': " << e.what() << std::endl;
				__COUT_ERR__ << ss.str() << std::endl;
				throw std::runtime_error(ss.str());
			}
		}
		POPTAB;
		OUT << "}\n\n";	//end sources

		POPTAB;
		OUT << "}\n\n";	//end event builder


		OUT << "metrics: {\n";

		PUSHTAB;
		auto metricsGroup = daq.getNode("daqMetricsLink");
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
		OUT << "}\n\n"; //end daq
	}

	//--------------------------------------
	//handle outputs
	auto outputs = builderNode.getNode("outputsLink");
	if (!outputs.isDisconnected())
	{
		OUT << "outputs: {\n";

		PUSHTAB;

		auto outputPlugins = outputs.getChildren();
		for (auto &outputPlugin : outputPlugins)
		{
			if (!outputPlugin.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
				PUSHCOMMENT;

			OUT << outputPlugin.second.getNode("outputKey").getValue() <<
				": {\n";
			PUSHTAB;
			OUT << "module_type: " <<
				outputPlugin.second.getNode("outputModuleType").getValue() <<
				"\n";
			auto pluginParameterLink = outputPlugin.second.getNode("outputModuleParameterLink");
			if (!pluginParameterLink.isDisconnected())
			{
				auto pluginParameters = pluginParameterLink.getChildren();
				for (auto &pluginParameter : pluginParameters)
				{
					if (!pluginParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
						PUSHCOMMENT;

					OUT << pluginParameter.second.getNode("outputParameterKey").getValue() <<
						": " <<
						pluginParameter.second.getNode("outputParameterValue").getValue()
						<< "\n";

					if (!pluginParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
						POPCOMMENT;
				}
			}
			POPTAB;
			OUT << "}\n\n";	//end output module

			if (!outputPlugin.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
				POPCOMMENT;
		}

		POPTAB;
		OUT << "}\n\n";	//end outputs
	}


	//--------------------------------------
	//handle physics
	auto physics = builderNode.getNode("physicsLink");
	if (!physics.isDisconnected())
	{
		///////////////////////
		OUT << "physics: {\n";

		PUSHTAB;

		auto analyzers = physics.getNode("analyzersLink");
		if (!analyzers.isDisconnected())
		{
			///////////////////////
			OUT << "analyzers: {\n";

			PUSHTAB;
			auto modules = analyzers.getChildren();
			for (auto &module : modules)
			{
				if (!module.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					PUSHCOMMENT;

				OUT << module.second.getNode("analyzerKey").getValue() <<
					": {\n";
				PUSHTAB;
				OUT << "module_type: " <<
					module.second.getNode("analyzerModuleType").getValue() <<
					"\n";
				auto moduleParameterLink = module.second.getNode("analyzerModuleParameterLink");
				if (!moduleParameterLink.isDisconnected())
				{
					auto moduleParameters = moduleParameterLink.getChildren();
					for (auto &moduleParameter : moduleParameters)
					{
						if (!moduleParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
							PUSHCOMMENT;

						OUT << moduleParameter.second.getNode("analyzerParameterKey").getValue() <<
							": " <<
							moduleParameter.second.getNode("analyzerParameterValue").getValue()
							<< "\n";

						if (!moduleParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
							POPCOMMENT;
					}
				}
				POPTAB;
				OUT << "}\n\n";	//end analyzer module

				if (!module.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					POPCOMMENT;
			}
			POPTAB;
			OUT << "}\n\n";	//end analyzer
		}

		auto producers = physics.getNode("producersLink");
		if (!producers.isDisconnected())
		{
			///////////////////////
			OUT << "producers: {\n";

			PUSHTAB;
			auto modules = producers.getChildren();
			for (auto &module : modules)
			{
				if (!module.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					PUSHCOMMENT;

				OUT << module.second.getNode("producerKey").getValue() <<
					": {\n";
				PUSHTAB;
				OUT << "module_type: " <<
					module.second.getNode("producerModuleType").getValue() <<
					"\n";
				auto moduleParameterLink = module.second.getNode("producerModuleParameterLink");
				if (!moduleParameterLink.isDisconnected())
				{
					auto moduleParameters = moduleParameterLink.getChildren();
					for (auto &moduleParameter : moduleParameters)
					{
						if (!moduleParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
							PUSHCOMMENT;

						OUT << moduleParameter.second.getNode("producerParameterKey").getValue() <<
							":" <<
							moduleParameter.second.getNode("producerParameterValue").getValue()
							<< "\n";

						if (!moduleParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
							POPCOMMENT;
					}
				}
				POPTAB;
				OUT << "}\n\n";	//end producer module

				if (!module.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					POPCOMMENT;
			}
			POPTAB;
			OUT << "}\n\n";	//end producer
		}


		auto filters = physics.getNode("filtersLink");
		if (!filters.isDisconnected())
		{
			///////////////////////
			OUT << "filters: {\n";

			PUSHTAB;
			auto modules = filters.getChildren();
			for (auto &module : modules)
			{
				if (!module.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					PUSHCOMMENT;

				OUT << module.second.getNode("filterKey").getValue() <<
					": {\n";
				PUSHTAB;
				OUT << "module_type: " <<
					module.second.getNode("filterModuleType").getValue() <<
					"\n";
				auto moduleParameterLink = module.second.getNode("filterModuleParameterLink");
				if (!moduleParameterLink.isDisconnected())
				{
					auto moduleParameters = moduleParameterLink.getChildren();
					for (auto &moduleParameter : moduleParameters)
					{
						if (!moduleParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
							PUSHCOMMENT;

						OUT << moduleParameter.second.getNode("filterParameterKey").getValue()
							<< ": " <<
							moduleParameter.second.getNode("filterParameterValue").getValue()
							<< "\n";

						if (!moduleParameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
							POPCOMMENT;
					}
				}
				POPTAB;
				OUT << "}\n\n";	//end filter module

				if (!module.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					POPCOMMENT;
			}
			POPTAB;
			OUT << "}\n\n";	//end filter
		}


		auto otherParameterLink = physics.getNode("physicsOtherParametersLink");
		if (!otherParameterLink.isDisconnected())
		{
			///////////////////////
			auto physicsParameters = otherParameterLink.getChildren();
			for (auto &parameter : physicsParameters)
			{
				if (!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					PUSHCOMMENT;

				OUT << parameter.second.getNode("physicsParameterKey").getValue() <<
					": " <<
					parameter.second.getNode("physicsParameterValue").getValue()
					<< "\n";

				if (!parameter.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())
					POPCOMMENT;
			}
		}
		POPTAB;
		OUT << "}\n\n";	//end physics
	}


	//--------------------------------------
	//handle source
	auto source = builderNode.getNode("sourceLink");
	if (!source.isDisconnected())
	{
		OUT << "source: {\n";

		PUSHTAB;
		OUT << "module_type: " << source.getNode("sourceModuleType").getValue() <<
			"\n";
		OUT << "waiting_time: " << source.getNode("sourceWaitingTime").getValue() <<
			"\n";
		OUT << "resume_after_timeout: " <<
			(source.getNode("sourceResumeAfterTimeout").getValue<bool>() ? "true" : "false") <<
			"\n";
		POPTAB;
		OUT << "}\n\n";	//end source
	}

	//--------------------------------------
	//handle process_name
	OUT << "process_name: " <<
		builderNode.getNode("ARTDAQGlobalConfigurationForProcessNameLink/processNameForBuilders")
		<< "\n";




	out.close();
}


DEFINE_OTS_CONFIGURATION(ARTDAQBuilderConfiguration)
