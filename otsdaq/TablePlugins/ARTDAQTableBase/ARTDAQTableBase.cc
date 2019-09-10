#include <fhiclcpp/ParameterSet.h>
#include <fhiclcpp/detail/print_mode.h>
#include <fhiclcpp/intermediate_table.h>
#include <fhiclcpp/make_ParameterSet.h>
#include <fhiclcpp/parse.h>

#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

#include <fstream>   // for std::ofstream
#include <iostream>  // std::cout
#include <typeinfo>

#include "otsdaq/TableCore/TableInfoReader.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "ARTDAQTableBase-" + getTableName()

#define ARTDAQ_FCL_PATH std::string(__ENV__("USER_DATA")) + "/" + "ARTDAQConfigurations/"

//==============================================================================
// TableBase
//	If a valid string pointer is passed in accumulatedExceptions
//	then allowIllegalColumns is set for InfoReader
//	If accumulatedExceptions pointer = 0, then illegal columns throw std::runtime_error
// exception
ARTDAQTableBase::ARTDAQTableBase(std::string  tableName,
                                 std::string* accumulatedExceptions /* =0 */)
    : TableBase(tableName, accumulatedExceptions)
{
	// make directory just in case
	mkdir((ARTDAQ_FCL_PATH).c_str(), 0755);
}  // end constuctor()

//==============================================================================
// ARTDAQTableBase
//	Default constructor should never be used because table type is lost
ARTDAQTableBase::ARTDAQTableBase(void) : TableBase("ARTDAQTableBase")
{
	__SS__ << "Should not call void constructor, table type is lost!" << __E__;
	__SS_THROW__;
}  // end illegal default constructor()

//==============================================================================
ARTDAQTableBase::~ARTDAQTableBase(void) {}  // end destructor()

//========================================================================================================================
std::string ARTDAQTableBase::getFHICLFilename(const std::string& type,
                                              const std::string& name)
{
	__COUT__ << "Type: " << type << " Name: " << name << __E__;
	std::string filename = ARTDAQ_FCL_PATH + type + "-";
	std::string uid      = name;
	for(unsigned int i = 0; i < uid.size(); ++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') || (uid[i] >= 'A' && uid[i] <= 'Z') ||
		   (uid[i] >= '0' && uid[i] <= '9'))  // only allow alpha numeric in file name
			filename += uid[i];

	filename += ".fcl";

	__COUT__ << "fcl: " << filename << __E__;

	return filename;
}  // end getFHICLFilename()

//========================================================================================================================
std::string ARTDAQTableBase::getFlatFHICLFilename(const std::string& type,
                                                  const std::string& name)
{
	__COUT__ << "Type: " << type << " Name: " << name << __E__;
	std::string filename = ARTDAQ_FCL_PATH + type + "-";
	std::string uid      = name;
	for(unsigned int i = 0; i < uid.size(); ++i)
		if((uid[i] >= 'a' && uid[i] <= 'z') || (uid[i] >= 'A' && uid[i] <= 'Z') ||
		   (uid[i] >= '0' && uid[i] <= '9'))  // only allow alpha numeric in file name
			filename += uid[i];

	filename += "_flattened.fcl";

	__COUT__ << "fcl: " << filename << __E__;

	return filename;
}  // end getFlatFHICLFilename()

//========================================================================================================================
void ARTDAQTableBase::flattenFHICL(const std::string& type, const std::string& name)
{
	__COUT__ << "flattenFHICL()" << __E__;
	std::string inFile  = getFHICLFilename(type, name);
	std::string outFile = getFlatFHICLFilename(type, name);

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
}  // end flattenFHICL()

//========================================================================================================================
// insertParameters
//	Inserts parameters in FHiCL outputs stream.
//
// 	onlyInsertAtTableParameters allows @table:: parameters only,
//	so that calling code can do two passes (i.e. top of fcl block, @table:: only,
//	and bottom of fcl block, ignore/skip @table:: as default behavior).
void ARTDAQTableBase::insertParameters(std::ostream&      out,
                                       std::string&       tabStr,
                                       std::string&       commentStr,
                                       ConfigurationTree  parameterGroupLink,
                                       const std::string& parameterPreamble,
                                       bool onlyInsertAtTableParameters /*=false*/,
                                       bool includeAtTableParameters /*=false*/)
{
	// skip if link is disconnected
	if(!parameterGroupLink.isDisconnected())
	{
		///////////////////////
		auto otherParameters = parameterGroupLink.getChildren();

		std::string key;
		//__COUTV__(otherParameters.size());
		for(auto& parameter : otherParameters)
		{
			key = parameter.second.getNode(parameterPreamble + "Key").getValue();

			// handle special keyword @table:: (which imports full tables, usually as
			// defaults)
			if(key.find("@table::") != std::string::npos)
			{
				// include @table::
				if(onlyInsertAtTableParameters || includeAtTableParameters)
				{
					if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
					        .getValue<bool>())
						PUSHCOMMENT;

					OUT << key;

					// skip connecting : if special keywords found
					OUT << parameter.second.getNode(parameterPreamble + "Value")
					           .getValue()
					    << "\n";

					if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
					        .getValue<bool>())
						POPCOMMENT;
				}
				// else skip it

				continue;
			}
			// else NOT @table:: keyword parameter

			if(onlyInsertAtTableParameters)
				continue;  // skip all other types

			if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				PUSHCOMMENT;

			OUT << key;

			// skip connecting : if special keywords found
			if(key.find("#include") == std::string::npos)
				OUT << ":";
			OUT << parameter.second.getNode(parameterPreamble + "Value").getValue()
			    << "\n";

			if(!parameter.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
			        .getValue<bool>())
				POPCOMMENT;
		}
	}
	// else
	//	__COUT__ << "No parameters found" << __E__;

}  // end insertParameters

//========================================================================================================================
// insertModuleType
//	Inserts module type field, with consideration for @table::
std::string ARTDAQTableBase::insertModuleType(std::ostream&     out,
                                              std::string&      tabStr,
                                              std::string&      commentStr,
                                              ConfigurationTree moduleTypeNode)
{
	std::string value = moduleTypeNode.getValue();

	OUT;
	if(value.find("@table::") == std::string::npos)
		out << "module_type: ";
	out << value << "\n";

	return value;
}  // end

//========================================================================================================================
void ARTDAQTableBase::outputDataReceiverFHICL(
    ConfigurationManager*                configManager,
    const ConfigurationTree&             appNode,
    unsigned int                         selfRank,
    std::string                          selfHost,
    unsigned int                         selfPort,
    ARTDAQTableBase::DataReceiverAppType appType)
{
	std::string filename = getFHICLFilename(getPreamble(appType), appNode.getValue());

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
	OUT << "# artdaq " << getPreamble(appType)
	    << " fcl configuration file produced by otsdaq." << __E__;
	OUT << "# 	Creation timestamp: " << StringMacros::getTimestampString() << __E__;
	OUT << "# 	Original filename: " << filename << __E__;
	OUT << "#	otsdaq-ARTDAQ " << getPreamble(appType) << " UID: " << appNode.getValue()
	    << __E__;
	OUT << "#" << __E__;
	OUT << "###########################################################" << __E__;
	OUT << "\n\n";

	// no primary link to table tree for reader node!
	try
	{
		if(appNode.isDisconnected())
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
	// handle preamble parameters
	__COUT__ << "Inserting preamble parameters..." << __E__;
	ARTDAQTableBase::insertParameters(out,
	                                  tabStr,
	                                  commentStr,
	                                  appNode.getNode("preambleParametersLink"),
	                                  "daqParameter" /*parameterType*/,
	                                  false /*onlyInsertAtTableParameters*/,
	                                  true /*includeAtTableParameters*/);

	//--------------------------------------
	// handle daq
	__COUT__ << "Generating daq block..." << __E__;
	auto daq = appNode.getNode("daqLink");
	if(!daq.isDisconnected())
	{
		///////////////////////
		OUT << "daq: {\n";

		PUSHTAB;
		if(appType == DataReceiverAppType::EventBuilder)
		{
			// event_builder
			OUT << "event_builder: {\n";
		}
		else
		{
			// both datalogger and dispatcher use aggregator for now
			OUT << "aggregator: {\n";
			if(appType == DataReceiverAppType::DataLogger)
			{
				OUT << "is_datalogger: true\n";
			}
			else if(appType == DataReceiverAppType::Dispatcher)
			{
				OUT << "is_dispatcher: true\n";
			}
		}

		PUSHTAB;

		__COUT__ << "Getting max_fragment_size_bytes from Global table..." << __E__;
		OUT << "max_fragment_size_bytes: "
		    << appNode.getNode("ARTDAQGlobalTableForProcessNameLink/maxFragmentSizeBytes")
		    << "\n";

		//--------------------------------------
		// handle ALL daq parameters
		__COUT__ << "Inserting DAQ Parameters..." << __E__;
		ARTDAQTableBase::insertParameters(out,
		                                  tabStr,
		                                  commentStr,
		                                  daq.getNode("daqParametersLink"),
		                                  "daqParameter" /*parameterType*/,
		                                  false /*onlyInsertAtTableParameters*/,
		                                  true /*includeAtTableParameters*/);

		__COUT__ << "Adding sources placeholder" << __E__;
		OUT << "sources: {\n"
		    << "}\n\n";  // end sources

		POPTAB;
		OUT << "}\n\n";  // end event builder

		__COUT__ << "Filling in metrics" << __E__;
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

				//--------------------------------------
				// handle ALL metric parameters
				ARTDAQTableBase::insertParameters(
				    out,
				    tabStr,
				    commentStr,
				    metric.second.getNode("metricParametersLink"),
				    "metricParameter" /*parameterType*/,
				    false /*onlyInsertAtTableParameters*/,
				    true /*includeAtTableParameters*/);

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
	// handle art
	__COUT__ << "Filling art block..." << __E__;
	OUT << "art: {\n";

	PUSHTAB;

	//--------------------------------------
	// handle services
	__COUT__ << "Filling art.services" << __E__;
	auto services = appNode.getNode("servicesLink");
	if(!services.isDisconnected())
	{
		OUT << "services: {\n";

		PUSHTAB;

		//--------------------------------------
		// handle services @table:: parameters
		ARTDAQTableBase::insertParameters(out,
		                                  tabStr,
		                                  commentStr,
		                                  services.getNode("ServicesParametersLink"),
		                                  "daqParameter" /*parameterType*/,
		                                  true /*onlyInsertAtTableParameters*/,
		                                  false /*includeAtTableParameters*/);

		// scheduler
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

		POPTAB;
		OUT << "}\n\n";  // end NetMonTransportServiceInterface

		//--------------------------------------
		// handle services NOT @table:: parameters
		ARTDAQTableBase::insertParameters(out,
		                                  tabStr,
		                                  commentStr,
		                                  services.getNode("ServicesParametersLink"),
		                                  "daqParameter" /*parameterType*/,
		                                  false /*onlyInsertAtTableParameters*/,
		                                  false /*includeAtTableParameters*/);

		POPTAB;
		OUT << "}\n\n";  // end services
	}

	//--------------------------------------
	// handle outputs
	__COUT__ << "Filling art.outputs" << __E__;
	auto outputs = appNode.getNode("outputsLink");
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

			OUT << outputPlugin.second.getNode("outputKey").getValue() << ": {\n";
			PUSHTAB;

			std::string moduleType = ARTDAQTableBase::insertModuleType(
			    out, tabStr, commentStr, outputPlugin.second.getNode("outputModuleType"));

			//--------------------------------------
			// handle ALL output parameters
			ARTDAQTableBase::insertParameters(
			    out,
			    tabStr,
			    commentStr,
			    outputPlugin.second.getNode("outputModuleParameterLink"),
			    "outputParameter" /*parameterType*/,
			    false /*onlyInsertAtTableParameters*/,
			    true /*includeAtTableParameters*/);

			if(moduleType.find("BinaryNetOuput") != std::string::npos ||
			   moduleType.find("RootNetOuput") != std::string::npos)
			{
				OUT << "destinations: {\n"
				    << "}\n\n";  // end destinations
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
	__COUT__ << "Filling art.physics" << __E__;
	auto physics = appNode.getNode("physicsLink");
	if(!physics.isDisconnected())
	{
		///////////////////////
		OUT << "physics: {\n";

		PUSHTAB;

		//--------------------------------------
		// handle only @table:: physics parameters
		ARTDAQTableBase::insertParameters(out,
		                                  tabStr,
		                                  commentStr,
		                                  physics.getNode("physicsOtherParametersLink"),
		                                  "physicsParameter" /*parameterType*/,
		                                  true /*onlyInsertAtTableParameters*/,
		                                  false /*includeAtTableParameters*/);

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

				//--------------------------------------
				// handle only @table:: analyzer parameters
				ARTDAQTableBase::insertParameters(
				    out,
				    tabStr,
				    commentStr,
				    module.second.getNode("analyzerModuleParameterLink"),
				    "analyzerParameter" /*parameterType*/,
				    true /*onlyInsertAtTableParameters*/,
				    false /*includeAtTableParameters*/);

				OUT << module.second.getNode("analyzerKey").getValue() << ": {\n";
				PUSHTAB;
				ARTDAQTableBase::insertModuleType(
				    out, tabStr, commentStr, module.second.getNode("analyzerModuleType"));

				//--------------------------------------
				// handle NOT @table:: producer parameters
				ARTDAQTableBase::insertParameters(
				    out,
				    tabStr,
				    commentStr,
				    module.second.getNode("analyzerModuleParameterLink"),
				    "analyzerParameter" /*parameterType*/,
				    false /*onlyInsertAtTableParameters*/,
				    false /*includeAtTableParameters*/);

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

				//--------------------------------------
				// handle only @table:: producer parameters
				ARTDAQTableBase::insertParameters(
				    out,
				    tabStr,
				    commentStr,
				    module.second.getNode("producerModuleParameterLink"),
				    "producerParameter" /*parameterType*/,
				    true /*onlyInsertAtTableParameters*/,
				    false /*includeAtTableParameters*/);

				OUT << module.second.getNode("producerKey").getValue() << ": {\n";
				PUSHTAB;

				ARTDAQTableBase::insertModuleType(
				    out, tabStr, commentStr, module.second.getNode("producerModuleType"));

				//--------------------------------------
				// handle NOT @table:: producer parameters
				ARTDAQTableBase::insertParameters(
				    out,
				    tabStr,
				    commentStr,
				    module.second.getNode("producerModuleParameterLink"),
				    "producerParameter" /*parameterType*/,
				    false /*onlyInsertAtTableParameters*/,
				    false /*includeAtTableParameters*/);

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

				//--------------------------------------
				// handle only @table:: filter parameters
				ARTDAQTableBase::insertParameters(
				    out,
				    tabStr,
				    commentStr,
				    module.second.getNode("filterModuleParameterLink"),
				    "filterParameter" /*parameterType*/,
				    true /*onlyInsertAtTableParameters*/,
				    false /*includeAtTableParameters*/);

				OUT << module.second.getNode("filterKey").getValue() << ": {\n";
				PUSHTAB;

				ARTDAQTableBase::insertModuleType(
				    out, tabStr, commentStr, module.second.getNode("filterModuleType"));

				//--------------------------------------
				// handle NOT @table:: filter parameters
				ARTDAQTableBase::insertParameters(
				    out,
				    tabStr,
				    commentStr,
				    module.second.getNode("filterModuleParameterLink"),
				    "filterParameter" /*parameterType*/,
				    false /*onlyInsertAtTableParameters*/,
				    false /*includeAtTableParameters*/);

				POPTAB;
				OUT << "}\n\n";  // end filter module

				if(!module.second.getNode(TableViewColumnInfo::COL_NAME_STATUS)
				        .getValue<bool>())
					POPCOMMENT;
			}
			POPTAB;
			OUT << "}\n\n";  // end filter
		}

		//--------------------------------------
		// handle NOT @table:: physics parameters
		ARTDAQTableBase::insertParameters(out,
		                                  tabStr,
		                                  commentStr,
		                                  physics.getNode("physicsOtherParametersLink"),
		                                  "physicsParameter" /*parameterType*/,
		                                  false /*onlyInsertAtTableParameters*/,
		                                  false /*includeAtTableParameters*/);

		POPTAB;
		OUT << "}\n\n";  // end physics
	}

	//--------------------------------------
	// handle source
	__COUT__ << "Filling art.source" << __E__;
	auto source = appNode.getNode("sourceLink");
	if(!source.isDisconnected())
	{
		OUT << "source: {\n";

		PUSHTAB;

		ARTDAQTableBase::insertModuleType(
		    out, tabStr, commentStr, source.getNode("sourceModuleType"));

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
	__COUT__ << "Writing art.process_name" << __E__;
	switch(appType)
	{
	case DataReceiverAppType::EventBuilder:
		OUT << "process_name: "
		    << appNode.getNode(
		           "ARTDAQGlobalTableForProcessNameLink/processNameForBuilders")
		    << "\n";
		break;
	case DataReceiverAppType::DataLogger:
		OUT << "process_name: "
		    << appNode.getNode(
		           "ARTDAQGlobalTableForProcessNameLink/processNameForDataloggers")
		    << "\n";
		break;

	case DataReceiverAppType::Dispatcher:
		OUT << "process_name: "
		    << appNode.getNode(
		           "ARTDAQGlobalTableForProcessNameLink/processNameForDispatchers")
		    << "\n";
		break;
	}

	POPTAB;
	OUT << "}\n\n";  // end art

	//--------------------------------------
	// handle ALL metric parameters
	__COUT__ << "Inserting add-on parameters" << __E__;
	ARTDAQTableBase::insertParameters(out,
	                                  tabStr,
	                                  commentStr,
	                                  appNode.getNode("addOnParametersLink"),
	                                  "daqParameter" /*parameterType*/,
	                                  false /*onlyInsertAtTableParameters*/,
	                                  true /*includeAtTableParameters*/);

	__COUT__ << "outputDataReceiverFHICL DONE" << __E__;
	out.close();
}  // end outputFHICL()