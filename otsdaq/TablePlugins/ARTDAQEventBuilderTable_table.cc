#include "otsdaq/Macros/TablePluginMacros.h"
#include "otsdaq/TablePlugins/ARTDAQEventBuilderTable.h"
#include "otsdaq/TablePlugins/XDAQContextTable.h"

#include <stdio.h>
#include <sys/stat.h>  //for mkdir
#include <fstream>     // std::fstream
#include <iostream>

using namespace ots;

// clang-format off

#define EPICS_PV_FILE_PATH \
		std::string( \
			getenv("OTSDAQ_EPICS_DATA")? \
				(std::string(getenv("OTSDAQ_EPICS_DATA")) + "/" + __ENV__("MU2E_OWNER") + "_otsdaq_artdaqEventBuilder-ai.dbg"): \
				(EPICS_CONFIG_PATH + "/_otsdaq_artdaqEventBuilder-ai.dbg")  )

// clang-format on

//==============================================================================
ARTDAQEventBuilderTable::ARTDAQEventBuilderTable(void)
    : TableBase("ARTDAQEventBuilderTable")
    , ARTDAQTableBase("ARTDAQEventBuilderTable")
    , SlowControlsTableBase("ARTDAQEventBuilderTable")
    // , isFirstAppInContext_(false)
    // , channelListHasChanged_(false)
    // , lastConfigManager_(nullptr)
{
	__COUT__ << "Constructed." << __E__;
} //end constructor()

//==============================================================================
ARTDAQEventBuilderTable::~ARTDAQEventBuilderTable(void) {}

//==============================================================================
void ARTDAQEventBuilderTable::init(ConfigurationManager* configManager)
{
	lastConfigManager_ = configManager;

	// use isFirstAppInContext to only run once per context, for example to avoid
	//	generating files on local disk multiple times.
	isFirstAppInContext_ = configManager->isOwnerFirstAppInContext();

	//__COUTV__(isFirstAppInContext);
	if(!isFirstAppInContext_)
		return;

	// make directory just in case
	mkdir((ARTDAQTableBase::ARTDAQ_FCL_PATH).c_str(), 0755);

	//	__COUT__ << "*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*" << __E__;
	//	__COUT__ << configManager->__SELF_NODE__ << __E__;

	// handle fcl file generation, wherever the level of this table

	auto buiders = lastConfigManager_->getNode(ARTDAQTableBase::getTableName()).getChildren(
	    /*default filterMap*/ std::map<std::string /*relative-path*/, std::string /*value*/>(),
	    /*default byPriority*/ false,
	    /*TRUE! onlyStatusTrue*/ true);

	for(auto& builder : buiders)
	{
		ARTDAQTableBase::outputDataReceiverFHICL(builder.second, ARTDAQTableBase::ARTDAQAppType::EventBuilder);
		ARTDAQTableBase::flattenFHICL(ARTDAQAppType::EventBuilder, builder.second.getValue());
	}
}  // end init()



//==============================================================================
//return channel list if pointer passed
bool ARTDAQEventBuilderTable::outputEpicsPVFile(
    ConfigurationManager* configManager,
    std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>*
        channelList /*= 0*/) const
{
	std::string filename = EPICS_PV_FILE_PATH;

	__COUT__ << "EPICS PV file: " << filename << __E__;

	std::string previousConfigFileContents;
	{
		std::FILE* fp = std::fopen(filename.c_str(), "rb");
		if(fp)
		{
			std::fseek(fp, 0, SEEK_END);
			previousConfigFileContents.resize(std::ftell(fp));
			std::rewind(fp);
			std::fread(&previousConfigFileContents[0], 1, previousConfigFileContents.size(), fp);
			std::fclose(fp);
		}
		else
			__COUT_WARN__ << "Could not open EPICS IOC config file at " << filename << __E__;

	}  // done reading

	/////////////////////////
	// generate xdaq run parameter file

	std::stringstream out;

	std::string tabStr     = "";
	std::string commentStr = "";

	// loop through ARTDAQ EventBuilder records starting at ARTDAQSupervisorTable
	std::vector<std::pair<std::string, ConfigurationTree>> artdaqRecords =
		configManager->getNode("ARTDAQSupervisorTable").getChildren();

	unsigned int numberOfEventBuilders = 0;
	unsigned int numberOfEventBuiderMetricParameters = 0;

	for(auto& artdaqPair : artdaqRecords)  // start main artdaq record loop
	{
		if(artdaqPair.second.getNode(colARTDAQSupervisor_.colLinkToEventBuilders_).isDisconnected())
			continue;

		std::vector<std::pair<std::string, ConfigurationTree>> eventBuilderRecords =
		    artdaqPair.second.getNode(colARTDAQSupervisor_.colLinkToEventBuilders_).getChildren();

		for(auto& eventBuilderPair : eventBuilderRecords)  // start main eventBuilder record loop
		{
			if(!eventBuilderPair.second.status())
				continue;

			numberOfEventBuilders++;

			try
			{
				if(eventBuilderPair.second.getNode("daqLink").isDisconnected())
					continue;

				auto daqLink = eventBuilderPair.second.getNode("daqLink");

				if(daqLink.getNode("daqMetricsLink").isDisconnected())
					continue;

				auto daqMetricsLinks = daqLink.getNode("daqMetricsLink").getChildren();
				for(auto& daqMetricsLink : daqMetricsLinks)  // start daqMetricsLinks record loop
				{
					if(!daqMetricsLink.second.status())
						continue;

					if(daqMetricsLink.second.getNode("metricParametersLink").isDisconnected())
						continue;

					//ConfigurationTree slowControlsLink = daqMetricsLink.second.getNode("ARTDAQMetricAlarmThresholdsTable");
					ConfigurationTree slowControlsLink = configManager->getNode("ARTDAQMetricAlarmThresholdsTable");

					auto metricParametersLinks = daqMetricsLink.second.getNode("metricParametersLink").getChildren();
					for(auto& metricParametersLink : metricParametersLinks)  // start daq MetricParametersLinks record loop
					{
						if(!metricParametersLink.second.status())
							continue;

						std::string subsystem = metricParametersLink.second.getNode("metricParameterValue")
								.getValueWithDefault<std::string>(std::string("TDAQ_") + __ENV__("MU2E_OWNER"));
						if(subsystem.find("Mu2e:") != std::string::npos)
							subsystem = subsystem.replace(subsystem.find("Mu2e:"), 5, "");
						while(subsystem.find("\"") != std::string::npos)
							subsystem = subsystem.replace(subsystem.find("\""), 1, "");

						numberOfEventBuiderMetricParameters =
						slowControlsHandler(
							  out
							, tabStr
							, commentStr
							, subsystem
							, eventBuilderPair.first
							, slowControlsLink
							, channelList);

						__COUT__ << "EventBuilder '" << eventBuilderPair.first
								<< "' number of metrics for slow controls: "
								<< numberOfEventBuiderMetricParameters << __E__;
					}
				}
			}
			catch(const std::runtime_error& e)
			{
				__COUT_ERR__ << "Ignoring EventBuilder error: " << e.what() << __E__;
			}
		}
	}

	// check if need to restart EPICS ioc
	//	if dbg string has changed, then mark ioc configuration dirty
	if(previousConfigFileContents != out.str())
	{
		__COUT__ << "Configuration has changed! Marking dirty flag..." << __E__;

		// only write files if first app in context AND channelList is not passed, i.e. init() is only time we write!
		// if(isFirstAppInContext_ && channelList == nullptr)
		if(channelList == nullptr)
		{
			std::fstream fout;
			fout.open(filename, std::fstream::out | std::fstream::trunc);
			if(fout.fail())
			{
				__SS__ << "Failed to open EPICS PV file: " << filename << __E__;
				__SS_THROW__;
			}

			fout << out.str();
			fout.close();

			std::FILE* fp = fopen(EPICS_DIRTY_FILE_PATH.c_str(), "w");
			if(fp)
			{
				fprintf(fp, "1");  // set dirty flag
				fclose(fp);
			}
			else
				__COUT_WARN__ << "Could not open dirty file: " << EPICS_DIRTY_FILE_PATH << __E__;
		}

		// Indicate that PV list has changed
		//	if otsdaq_epics plugin is listening, then write PV data to archive db:	SQL insert or modify of ROW for PV
		__COUT__ << "outputEpicsPVFile() return true" << __E__;
		return true;
	}  // end handling of previous contents

	__COUT__ << "outputEpicsPVFile() return false" << __E__;
	return false;
}  // end outputEpicsPVFile()

DEFINE_OTS_TABLE(ARTDAQEventBuilderTable)
