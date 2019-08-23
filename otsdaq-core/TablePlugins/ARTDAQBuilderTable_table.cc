#include <fhiclcpp/ParameterSet.h>
#include <fhiclcpp/detail/print_mode.h>
#include <fhiclcpp/intermediate_table.h>
#include <fhiclcpp/make_ParameterSet.h>
#include <fhiclcpp/parse.h>

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/Macros/TablePluginMacros.h"
#include "otsdaq-core/TablePlugins/ARTDAQBuilderTable.h"
#include "otsdaq-core/TablePlugins/XDAQContextTable.h"

#include <stdio.h>
#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>

using namespace ots;

#define ARTDAQ_FCL_PATH std::string(__ENV__("USER_DATA")) + "/" + "ARTDAQConfigurations/"
#define ARTDAQ_FILE_PREAMBLE "builder"

// helpers
#define OUT out << tabStr << commentStr
#define PUSHTAB tabStr += "\t"
#define POPTAB tabStr.resize(tabStr.size() - 1)
#define PUSHCOMMENT commentStr += "# "
#define POPCOMMENT commentStr.resize(commentStr.size() - 2)

//========================================================================================================================
ARTDAQBuilderTable::ARTDAQBuilderTable(void) : TableBase("ARTDAQBuilderTable")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO  //
	//////////////////////////////////////////////////////////////////////
}

//========================================================================================================================
ARTDAQBuilderTable::~ARTDAQBuilderTable(void) {}

//========================================================================================================================
void ARTDAQBuilderTable::init(ConfigurationManager* configManager)
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

	std::vector<const XDAQContextTable::XDAQContext*> builderContexts =
	    contextConfig->getEventBuilderContexts();

	// for each aggregator context
	//	output associated fcl config file
	for(auto& builderContext : builderContexts)
	{
		ConfigurationTree builderAppNode = contextConfig->getApplicationNode(
		    configManager,
		    builderContext->contextUID_,
		    builderContext->applications_[0].applicationUID_);
		ConfigurationTree builderConfigNode = contextConfig->getSupervisorConfigNode(
		    configManager,
		    builderContext->contextUID_,
		    builderContext->applications_[0].applicationUID_);

		__COUT__ << "Path for this aggregator config is " << builderContext->contextUID_
		         << "/" << builderContext->applications_[0].applicationUID_ << "/"
		         << builderConfigNode.getValueAsString() << __E__;

		outputFHICL(
		    configManager,
		    builderConfigNode,
		    contextConfig->getARTDAQAppRank(builderContext->contextUID_),
		    contextConfig->getContextAddress(builderContext->contextUID_),
		    contextConfig->getARTDAQDataPort(configManager, builderContext->contextUID_),
		    contextConfig);

		flattenFHICL(builderConfigNode);

	}
}

//========================================================================================================================
std::string ARTDAQBuilderTable::getFHICLFilename(const ConfigurationTree& builderNode)
{
	__COUT__ << "ARTDAQ Builder UID: " << builderNode.getValue() << __E__;
	std::string filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
	std::string uid      = builderNode.getValue();
	for(unsigned int i = 0; i < uid.size(); ++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') || (uid[i] >= 'A' && uid[i] <= 'Z') ||
		   (uid[i] >= '0' && uid[i] <= '9'))  // only allow alpha numeric in file name
			filename += uid[i];

	filename += ".fcl";

	__COUT__ << "fcl: " << filename << __E__;

	return filename;
}  // end getFHICLFilename()

//========================================================================================================================
std::string ARTDAQBuilderTable::getFlatFHICLFilename(const ConfigurationTree& builderNode)
{
	__COUT__ << "ARTDAQ Builder UID: " << builderNode.getValue() << __E__;
	std::string filename = ARTDAQ_FCL_PATH + ARTDAQ_FILE_PREAMBLE + "-";
	std::string uid      = builderNode.getValue();
	for(unsigned int i = 0; i < uid.size(); ++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') || (uid[i] >= 'A' && uid[i] <= 'Z') ||
		   (uid[i] >= '0' && uid[i] <= '9'))  // only allow alpha numeric in file name
			filename += uid[i];

	filename += "_flattened.fcl";

	__COUT__ << "fcl: " << filename << __E__;

	return filename;
}  // end getFlatFHICLFilename()

void ARTDAQBuilderTable::flattenFHICL(const ConfigurationTree& builderNode)
{
	__COUT__ << "flattenFHICL()" << __E__;
	auto inFile  = getFHICLFilename(builderNode);
	auto outFile = getFlatFHICLFilename(builderNode);
	
	__COUTV__(__ENV__("FHICL_FILE_PATH"));

	cet::filepath_lookup_nonabsolute policy("FHICL_FILE_PATH");

	fhicl::ParameterSet pset;

	try
	{
		fhicl::intermediate_table tbl;
		fhicl::parse_document(inFile, policy, tbl);
		fhicl::ParameterSet pset;
		fhicl::make_ParameterSet(tbl, pset);

		std::ofstream ofs{outFile};
		ofs << pset.to_indented_string(0, fhicl::detail::print_mode::annotated);
	}
	catch(cet::exception const& e)
	{
		__SS__ << "Failed to parse fhicl: " << e.what() << __E__;
		__SS_THROW__;
	}
}

//========================================================================================================================
void ARTDAQBuilderTable::outputFHICL(ConfigurationManager*    configManager,
                                     const ConfigurationTree& builderNode,
                                     unsigned int             selfRank,
                                     std::string              selfHost,
                                     unsigned int             selfPort,
                                     const XDAQContextTable*  contextConfig)
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
	          d4: { transferPluginType: MPI destination_rank: 4 max_fragment_size_bytes:
	2097152}

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
	            s0: { transferPluginType: MPI source_rank: 0 max_fragment_size_bytes:
	2097152} s1: { transferPluginType: MPI source_rank: 1 max_fragment_size_bytes:
	2097152}

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
	OUT << "# artdaq builder fcl configuration file produced by otsdaq." << __E__;
	OUT << "# 	Creation timestamp: " << StringMacros::getTimestampString() << __E__;
	OUT << "# 	Original filename: " << filename << __E__;
	OUT << "#	otsdaq-ARTDAQ Builder UID: " << builderNode.getValue() << __E__;
	OUT << "#" << __E__;
	OUT << "###########################################################" << __E__;
	OUT << "\n\n";

	// no primary link to table tree for reader node!
	if(builderNode.isDisconnected())
	{
		// create empty fcl
		OUT << "{}\n\n";
		out.close();
		return;
	}

	//--------------------------------------
	// handle preamble parameters
	auto preambleParameterLink = builderNode.getNode("preambleParametersLink");
	if(!preambleParameterLink.isDisconnected())
	{
		///////////////////////
		auto otherParameters = preambleParameterLink.getChildren();

		std::string key;
		//__COUTV__(otherParameters.size());
		for(auto& parameter : otherParameters)
		{
			if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				PUSHCOMMENT;
			key = parameter.second.getNode("daqParameterKey").getValue();

			OUT << key;
			if(key.find("#include") == std::string::npos)
				OUT << ":";
			OUT << parameter.second.getNode("daqParameterValue").getValue() << "\n";

			if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				POPCOMMENT;
		}
	}
	// else
	//	__COUT__ << "No preamble parameters found" << __E__;

	//--------------------------------------
	// handle services
	auto services = builderNode.getNode("servicesLink");
	if(!services.isDisconnected())
	{
		OUT << "services: {\n";

		// scheduler
		PUSHTAB;
		OUT << "scheduler: {\n";

	#if ART_HEX_VERSION < 0x30200
		PUSHTAB;
		//		OUT << "fileMode: " << services.getNode("schedulerFileMode").getValue() <<
		//"\n";
		OUT << "errorOnFailureToPut: "
		    << (services.getNode("schedulerErrorOnFailtureToPut").getValue<bool>()
		            ? "true"
		            : "false")
		    << "\n";
		POPTAB;
	#endif
		OUT << "}\n\n";

		// NetMonTransportServiceInterface
		OUT << "NetMonTransportServiceInterface: {\n";

		PUSHTAB;
		OUT << "service_provider: " <<
		    // services.getNode("NetMonTrasportServiceInterfaceServiceProvider").getEscapedValue()
		    services.getNode("NetMonTrasportServiceInterfaceServiceProvider").getValue()
		    << "\n";
		OUT << "destinations: {\n";

		PUSHTAB;
		auto destinationsGroup =
		    services.getNode("NetMonTrasportServiceInterfaceDestinationsLink");
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
						    "Are the Transport Service destinations valid? "
						    << "Perhaps a Context has been turned off? "
						    << "Skipping destination due to an error looking for Event "
						    << "Builder DAQ source context '" << destinationContextUID
						    << "' for UID '" << builderNode.getValue()
						    << "': " << e.what() << __E__);
						continue;
					}

					std::string host =
					    contextConfig->getContextAddress(destinationContextUID);
					unsigned int port = contextConfig->getARTDAQDataPort(
					    configManager, destinationContextUID);

					// open destination object
					OUT << destination.second.getNode("destinationKey").getValue()
					    << ": {\n";
					PUSHTAB;

					OUT << "transferPluginType: "
					    << destination.second.getNode("transferPluginType").getValue()
					    << __E__;

					OUT << "destination_rank: " << destinationRank << __E__;

					try
					{
						auto mfsb =
						    destination.second
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
				__SS__ << "Are the Net Monitor Transport Service destinations valid? "
				          "Error occurred looking for Event Builder transport service "
				          "destinations for UID '"
				       << builderNode.getValue() << "': " << e.what() << __E__;
				__SS_THROW__;
			}
		}
		POPTAB;
		OUT << "}\n\n";  // end destinations

		POPTAB;
		OUT << "}\n\n";  // end NetMonTransportServiceInterface

		auto otherParameterLink = services.getNode("ServicesParametersLink");
		if(!otherParameterLink.isDisconnected())
		{
			///////////////////////
			auto servicesParameters = otherParameterLink.getChildren();

			//__COUTV__(servicesParameters.size());
			for(auto& parameter : servicesParameters)
			{
				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					PUSHCOMMENT;

				OUT << parameter.second.getNode("daqParameterKey").getValue() << ": "
				    << parameter.second.getNode("daqParameterValue").getValue() << "\n";

				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					POPCOMMENT;
			}
		}
		// else
		//	__COUT__ << "No services parameters found" << __E__;

		POPTAB;
		OUT << "}\n\n";  // end services
	}

	//--------------------------------------
	// handle daq
	auto daq = builderNode.getNode("daqLink");
	if(!daq.isDisconnected())
	{
		///////////////////////
		OUT << "daq: {\n";

		// event_builder
		PUSHTAB;
		OUT << "event_builder: {\n";

		PUSHTAB;

		auto mfsb = daq.getNode("daqEventBuilderMaxFragmentSizeBytes")
		                .getValue<unsigned long long>();
		OUT << "max_fragment_size_bytes: " << mfsb << __E__;
		OUT << "max_fragment_size_words: " << (mfsb / 8) << __E__;
		OUT << "buffer_count: "
		    << daq.getNode("daqEventBuilderBufferCount").getValue<unsigned long long>()
		    << __E__;

		auto parametersLink = daq.getNode("daqEventBuilderParametersLink");
		if(!parametersLink.isDisconnected())
		{
			auto parameters = parametersLink.getChildren();
			for(auto& parameter : parameters)
			{
				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					PUSHCOMMENT;

				OUT << parameter.second.getNode("daqParameterKey").getValue() << ": "
				    << parameter.second.getNode("daqParameterValue").getValue() << "\n";

				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					POPCOMMENT;
			}
		}
		OUT << "\n";  // end daq event builder parameters

		OUT << "sources: {\n";

		PUSHTAB;
		auto sourcesGroup = daq.getNode("daqEventBuilderSourcesLink");
		if(!sourcesGroup.isDisconnected())
		{
			try
			{
				auto sources = sourcesGroup.getChildren();
				for(auto& source : sources)
				{
					auto sourceContextUID =
					    source.second.getNode("sourceARTDAQContextLink")
					        .getValueAsString();

					unsigned int sourceRank = -1;
					try
					{
						sourceRank = contextConfig->getARTDAQAppRank(sourceContextUID);
					}
					catch(const std::runtime_error& e)
					{
						__MCOUT_WARN__(
						    "Are the DAQ sources valid? "
						    << "Perhaps a Context has been turned off? "
						    << "Skipping source due to an error looking for Event "
						    << "Builder DAQ source context '" << sourceContextUID
						    << "' for UID '" << builderNode.getValue()
						    << "': " << e.what() << __E__);
						continue;
					}

					std::string host = contextConfig->getContextAddress(sourceContextUID);
					unsigned int port =
					    contextConfig->getARTDAQDataPort(configManager, sourceContextUID);

					// open source object
					OUT << source.second.getNode("sourceKey").getValue() << ": {\n";
					PUSHTAB;

					OUT << "transferPluginType: "
					    << source.second.getNode("transferPluginType").getValue()
					    << __E__;

					OUT << "source_rank: " << sourceRank << __E__;

					try
					{
						auto mfsb =
						    source.second
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
					OUT << "rank: " << sourceRank << __E__;
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
					OUT << "}" << __E__;  // close source object
				}
			}
			catch(const std::runtime_error& e)
			{
				__SS__ << "Are the DAQ sources valid? Error occurred looking for Event "
				          "Builder DAQ sources for UID '"
				       << builderNode.getValue() << "': " << e.what() << __E__;
				__SS_THROW__;
			}
		}
		POPTAB;
		OUT << "}\n\n";  // end sources

		POPTAB;
		OUT << "}\n\n";  // end event builder

		OUT << "metrics: {\n";

		PUSHTAB;
		auto metricsGroup = daq.getNode("daqMetricsLink");
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
				OUT << "level: " << metric.second.getNode("metricLevel").getValue()
				    << "\n";

				auto metricParametersGroup =
				    metric.second.getNode("metricParametersLink");
				if(!metricParametersGroup.isDisconnected())
				{
					auto metricParameters = metricParametersGroup.getChildren();
					for(auto& metricParameter : metricParameters)
					{
						if(!metricParameter.second
						        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
						        .getValue<bool>())
							PUSHCOMMENT;

						OUT << metricParameter.second.getNode("metricParameterKey")
						           .getValue()
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
	}

	//--------------------------------------
	// handle outputs
	auto outputs = builderNode.getNode("outputsLink");
	if(!outputs.isDisconnected())
	{
		OUT << "outputs: {\n";

		PUSHTAB;

		auto outputPlugins = outputs.getChildren();
		for(auto& outputPlugin : outputPlugins)
		{
			if(!outputPlugin.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				PUSHCOMMENT;

			std::string moduleType =
			    outputPlugin.second.getNode("outputModuleType").getValue();

			OUT << outputPlugin.second.getNode("outputKey").getValue() << ": {\n";
			PUSHTAB;

			OUT << "module_type: " << moduleType << "\n";

			auto pluginParameterLink =
			    outputPlugin.second.getNode("outputModuleParameterLink");
			if(!pluginParameterLink.isDisconnected())
			{
				auto pluginParameters = pluginParameterLink.getChildren();
				for(auto& pluginParameter : pluginParameters)
				{
					if(!pluginParameter.second
					        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
					        .getValue<bool>())
						PUSHCOMMENT;

					OUT << pluginParameter.second.getNode("outputParameterKey").getValue()
					    << ": "
					    << pluginParameter.second.getNode("outputParameterValue")
					           .getValue()
					    << "\n";

					if(!pluginParameter.second
					        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
					        .getValue<bool>())
						POPCOMMENT;
				}
			}

			// ONLY For output module type 'BinaryNetOuput,' allow destinations
			auto destinationsGroup =
			    outputPlugin.second.getNode("outputModuleDestinationLink");
			if(!destinationsGroup.isDisconnected())
			{
				try
				{
					if(moduleType.find("BinaryNetOuput") == std::string::npos)
					{
						__SS__ << "Illegal output module type '" << moduleType
						       << "' to include destinations. Only modules of type "
						          "'BinaryNetOuput' "
						       << "are allowed to have destinations." << __E__;
						__SS_THROW__;
					}

					OUT << "destinations: {\n";
					PUSHTAB;

					auto destinations = destinationsGroup.getChildren();
					for(auto& destination : destinations)
						try
						{
							auto destinationContextUID =
							    destination.second.getNode("destinationARTDAQContextLink")
							        .getValueAsString();

							unsigned int destinationRank = -1;
							try
							{
								destinationRank = contextConfig->getARTDAQAppRank(
								    destinationContextUID);
							}
							catch(const std::runtime_error& e)
							{
								__MCOUT_WARN__(
								    "Are the DAQ destinations valid? "
								    << "Perhaps a Context has been turned off? "
								    << "Skipping destination due to an error looking for "
								       "Event "
								    << "Builder DAQ source context '"
								    << destinationContextUID << "' for UID '"
								    << builderNode.getValue() << "': " << e.what()
								    << __E__);
								continue;
							}

							std::string host =
							    contextConfig->getContextAddress(destinationContextUID);
							unsigned int port = contextConfig->getARTDAQDataPort(
							    configManager, destinationContextUID);

							// open destination object
							OUT << destination.second.getNode("destinationKey").getValue()
							    << ": {\n";
							PUSHTAB;

							OUT << "transferPluginType: "
							    << destination.second.getNode("transferPluginType")
							           .getValue()
							    << __E__;

							OUT << "destination_rank: " << destinationRank << __E__;

							try
							{
								auto mfsb =
								    destination.second
								        .getNode(
								            "ARTDAQGlobalTableLink/maxFragmentSizeBytes")
								        .getValue<unsigned long long>();
								OUT << "max_fragment_size_bytes: " << mfsb << __E__;
								OUT << "max_fragment_size_words: " << (mfsb / 8) << __E__;
							}
							catch(...)
							{
								__SS__
								    << "The field "
								       "ARTDAQGlobalTableLink/maxFragmentSizeBytes could "
								       "not be accessed. Make sure the link is valid."
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
						catch(const std::runtime_error& e)
						{
							__SS__ << "Error encountered populating parameters for "
							          "output module destination '"
							       << destination.first
							       << "'... Please verify fields. Here was the error: "
							       << e.what() << __E__;
							__SS_ONLY_THROW__;
						}

					POPTAB;
					OUT << "}\n\n";  // end destinations
				}
				catch(const std::runtime_error& e)
				{
					__SS__ << "Are the Output module destinations valid? "
					          "Error occurred looking for Event Builder output module "
					          "destinations for UID '"
					       << outputPlugin.second.getValue() << "': " << e.what()
					       << __E__;
					__SS_THROW__;
				}
			}

			POPTAB;
			OUT << "}\n\n";  // end output module

			if(!outputPlugin.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				POPCOMMENT;
		}

		POPTAB;
		OUT << "}\n\n";  // end outputs
	}

	//--------------------------------------
	// handle physics
	auto physics = builderNode.getNode("physicsLink");
	if(!physics.isDisconnected())
	{
		///////////////////////
		OUT << "physics: {\n";

		PUSHTAB;

		auto analyzers = physics.getNode("analyzersLink");
		if(!analyzers.isDisconnected())
		{
			///////////////////////
			OUT << "analyzers: {\n";

			PUSHTAB;
			auto modules = analyzers.getChildren();
			for(auto& module : modules)
			{
				if(!module.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					PUSHCOMMENT;

				OUT << module.second.getNode("analyzerKey").getValue() << ": {\n";
				PUSHTAB;
				OUT << "module_type: "
				    << module.second.getNode("analyzerModuleType").getValue() << "\n";
				auto moduleParameterLink =
				    module.second.getNode("analyzerModuleParameterLink");
				if(!moduleParameterLink.isDisconnected())
				{
					auto moduleParameters = moduleParameterLink.getChildren();
					for(auto& moduleParameter : moduleParameters)
					{
						if(!moduleParameter.second
						        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
						        .getValue<bool>())
							PUSHCOMMENT;

						OUT << moduleParameter.second.getNode("analyzerParameterKey")
						           .getValue()
						    << ": "
						    << moduleParameter.second.getNode("analyzerParameterValue")
						           .getValue()
						    << "\n";

						if(!moduleParameter.second
						        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
						        .getValue<bool>())
							POPCOMMENT;
					}
				}
				POPTAB;
				OUT << "}\n\n";  // end analyzer module

				if(!module.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					POPCOMMENT;
			}
			POPTAB;
			OUT << "}\n\n";  // end analyzer
		}

		auto producers = physics.getNode("producersLink");
		if(!producers.isDisconnected())
		{
			///////////////////////
			OUT << "producers: {\n";

			PUSHTAB;
			auto modules = producers.getChildren();
			for(auto& module : modules)
			{
				if(!module.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					PUSHCOMMENT;

				OUT << module.second.getNode("producerKey").getValue() << ": {\n";
				PUSHTAB;
				OUT << "module_type: "
				    << module.second.getNode("producerModuleType").getValue() << "\n";
				auto moduleParameterLink =
				    module.second.getNode("producerModuleParameterLink");
				if(!moduleParameterLink.isDisconnected())
				{
					auto moduleParameters = moduleParameterLink.getChildren();
					for(auto& moduleParameter : moduleParameters)
					{
						if(!moduleParameter.second
						        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
						        .getValue<bool>())
							PUSHCOMMENT;

						OUT << moduleParameter.second.getNode("producerParameterKey")
						           .getValue()
						    << ":"
						    << moduleParameter.second.getNode("producerParameterValue")
						           .getValue()
						    << "\n";

						if(!moduleParameter.second
						        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
						        .getValue<bool>())
							POPCOMMENT;
					}
				}
				POPTAB;
				OUT << "}\n\n";  // end producer module

				if(!module.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					POPCOMMENT;
			}
			POPTAB;
			OUT << "}\n\n";  // end producer
		}

		auto filters = physics.getNode("filtersLink");
		if(!filters.isDisconnected())
		{
			///////////////////////
			OUT << "filters: {\n";

			PUSHTAB;
			auto modules = filters.getChildren();
			for(auto& module : modules)
			{
				if(!module.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					PUSHCOMMENT;

				OUT << module.second.getNode("filterKey").getValue() << ": {\n";
				PUSHTAB;
				OUT << "module_type: "
				    << module.second.getNode("filterModuleType").getValue() << "\n";
				auto moduleParameterLink =
				    module.second.getNode("filterModuleParameterLink");
				if(!moduleParameterLink.isDisconnected())
				{
					auto moduleParameters = moduleParameterLink.getChildren();
					for(auto& moduleParameter : moduleParameters)
					{
						if(!moduleParameter.second
						        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
						        .getValue<bool>())
							PUSHCOMMENT;

						OUT << moduleParameter.second.getNode("filterParameterKey")
						           .getValue()
						    << ": "
						    << moduleParameter.second.getNode("filterParameterValue")
						           .getValue()
						    << "\n";

						if(!moduleParameter.second
						        .getNode(TableViewColumnInfo::COL_NAME_STATUS)
						        .getValue<bool>())
							POPCOMMENT;
					}
				}
				POPTAB;
				OUT << "}\n\n";  // end filter module

				if(!module.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					POPCOMMENT;
			}
			POPTAB;
			OUT << "}\n\n";  // end filter
		}

		auto otherParameterLink = physics.getNode("physicsOtherParametersLink");
		if(!otherParameterLink.isDisconnected())
		{
			///////////////////////
			auto physicsParameters = otherParameterLink.getChildren();
			for(auto& parameter : physicsParameters)
			{
				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					PUSHCOMMENT;

				OUT << parameter.second.getNode("physicsParameterKey").getValue() << ": "
				    << parameter.second.getNode("physicsParameterValue").getValue()
				    << "\n";

				if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					POPCOMMENT;
			}
		}
		POPTAB;
		OUT << "}\n\n";  // end physics
	}

	//--------------------------------------
	// handle source
	auto source = builderNode.getNode("sourceLink");
	if(!source.isDisconnected())
	{
		OUT << "source: {\n";

		PUSHTAB;
		OUT << "module_type: " << source.getNode("sourceModuleType").getValue() << "\n";
		OUT << "waiting_time: " << source.getNode("sourceWaitingTime").getValue() << "\n";
		OUT << "resume_after_timeout: "
		    << (source.getNode("sourceResumeAfterTimeout").getValue<bool>() ? "true"
		                                                                    : "false")
		    << "\n";
		POPTAB;
		OUT << "}\n\n";  // end source
	}

	//--------------------------------------
	// handle process_name
	OUT << "process_name: "
	    << builderNode.getNode(
	           "ARTDAQGlobalTableForProcessNameLink/processNameForBuilders")
	    << "\n";

	auto otherParameterLink = builderNode.getNode("addOnParametersLink");
	if(!otherParameterLink.isDisconnected())
	{
		///////////////////////
		auto otherParameters = otherParameterLink.getChildren();

		std::string key;
		//__COUTV__(otherParameters.size());
		for(auto& parameter : otherParameters)
		{
			if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				PUSHCOMMENT;
			key = parameter.second.getNode("daqParameterKey").getValue();

			OUT << key;
			if(key.find("#include") == std::string::npos)
				OUT << ":";
			OUT << parameter.second.getNode("daqParameterValue").getValue() << "\n";

			if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				POPCOMMENT;
		}
	}
	// else
	//	__COUT__ << "No add-on parameters found" << __E__;

	out.close();
}  // end outputFHICL()

DEFINE_OTS_TABLE(ARTDAQBuilderTable)
