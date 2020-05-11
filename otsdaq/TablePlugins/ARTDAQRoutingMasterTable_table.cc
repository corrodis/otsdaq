#include "otsdaq/Macros/TablePluginMacros.h"
#include "otsdaq/TablePlugins/ARTDAQRoutingMasterTable.h"
#include "otsdaq/TablePlugins/XDAQContextTable.h"

using namespace ots;

// clang-format off

#define SLOWCONTROL_PV_FILE_PATH \
		std::string( \
			getenv("OTSDAQ_EPICS_DATA")? \
				(std::string(getenv("OTSDAQ_EPICS_DATA")) + "/" + __ENV__("MU2E_OWNER") + "_otsdaq_artdaqRoutingMaster-ai.dbg"): \
				(EPICS_CONFIG_PATH + "/_otsdaq_artdaqRoutingMaster-ai.dbg")  )

// clang-format on

//==============================================================================
ARTDAQRoutingMasterTable::ARTDAQRoutingMasterTable(void) 
	: TableBase("ARTDAQRoutingMasterTable")
	, ARTDAQTableBase("ARTDAQRoutingMasterTable")
	, SlowControlsTableBase("ARTDAQRoutingMasterTable")
{
	//////////////////////////////////////////////////////////////////////
	// WARNING: the names used in C++ MUST match the Table INFO 		//
	//////////////////////////////////////////////////////////////////////
	__COUT__ << "ARTDAQRoutingMasterTable Constructed." << __E__;
}  // end constructor()

//==============================================================================
ARTDAQRoutingMasterTable::~ARTDAQRoutingMasterTable(void) {}

//==============================================================================
void ARTDAQRoutingMasterTable::init(ConfigurationManager* configManager)
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

	auto routingMasters = lastConfigManager_->__SELF_NODE__.getChildren(
	    /*default filterMap*/ std::map<std::string /*relative-path*/, std::string /*value*/>(),
	    /*default byPriority*/ false,
	    /*TRUE! onlyStatusTrue*/ true);

	for(auto& routingMaster : routingMasters)
	{
		ARTDAQTableBase::outputRoutingMasterFHICL(routingMaster.second);
		ARTDAQTableBase::flattenFHICL(ARTDAQAppType::RoutingMaster, routingMaster.second.getValue());
	}

}  // end init()

//==============================================================================
unsigned int ARTDAQRoutingMasterTable::slowControlsHandlerConfig(
    std::stringstream&                                                             out,
    ConfigurationManager*                                                          configManager,
    std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>* channelList /*= 0*/
) const
{
	/////////////////////////
	// generate xdaq run parameter file

	std::string tabStr     = "";
	std::string commentStr = "";

	// loop through ARTDAQ RoutingMaster records starting at ARTDAQSupervisorTable
	std::vector<std::pair<std::string, ConfigurationTree>> artdaqRecords = configManager->getNode("ARTDAQSupervisorTable").getChildren();

	unsigned int numberOfRoutingMasters                = 0;
	unsigned int numberOfRoutingMasterMetricParameters = 0;

	for(auto& artdaqPair : artdaqRecords)  // start main artdaq record loop
	{
		if(artdaqPair.second.getNode(colARTDAQSupervisor_.colLinkToRoutingMasters_).isDisconnected())
			continue;

		std::vector<std::pair<std::string, ConfigurationTree>> routingMasterRecords =
		    artdaqPair.second.getNode(colARTDAQSupervisor_.colLinkToRoutingMasters_).getChildren();

		for(auto& routingMasterPair : routingMasterRecords)  // start main routingMaster record loop
		{
			if(!routingMasterPair.second.status())
				continue;

			numberOfRoutingMasters++;

			try
			{
				if(routingMasterPair.second.getNode("daqMetricsLink").isDisconnected())
					continue;

				auto daqMetricsLinks = routingMasterPair.second.getNode("daqMetricsLink").getChildren();
				for(auto& daqMetricsLink : daqMetricsLinks)  // start daqMetricsLinks record loop
				{
					if(!daqMetricsLink.second.status())
						continue;

					if(daqMetricsLink.second.getNode("metricParametersLink").isDisconnected())
						continue;

					// ConfigurationTree slowControlsLink = configManager->getNode("ARTDAQMetricAlarmThresholdsTable");
					ConfigurationTree slowControlsLink = routingMasterPair.second.getNode("MetricAlarmThresholdsLink");

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

						numberOfRoutingMasterMetricParameters =
						    slowControlsHandler(out, tabStr, commentStr, subsystem, routingMasterPair.first, slowControlsLink, channelList);

						__COUT__ << "RoutingMaster '" << routingMasterPair.first << "' number of metrics for slow controls: " << numberOfRoutingMasterMetricParameters
						         << __E__;
					}
				}
			}
			catch(const std::runtime_error& e)
			{
				__COUT_ERR__ << "Ignoring RoutingMaster error: " << e.what() << __E__;
			}
		}
	}

	return numberOfRoutingMasterMetricParameters;
}  // end slowControlsHandlerConfig()

//==============================================================================
// return out file path
std::string ARTDAQRoutingMasterTable::setFilePath() const { return SLOWCONTROL_PV_FILE_PATH; }

DEFINE_OTS_TABLE(ARTDAQRoutingMasterTable)
