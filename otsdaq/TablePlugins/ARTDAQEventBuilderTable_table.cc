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

// helpers
#define OUT out << tabStr << commentStr
#define PUSHTAB tabStr += "\t"
#define POPTAB tabStr.resize(tabStr.size() - 1)
#define PUSHCOMMENT commentStr += "# "
#define POPCOMMENT commentStr.resize(commentStr.size() - 2)

// // Column names
struct ColsChannel
{
	std::string const colStatus_            = "Status";
	std::string const colUnits_             = "Units";
	std::string const colChannelDataType_   = "ChannelDataType";
	std::string const colLowLowThreshold_   = "LowLowThreshold";
	std::string const colLowThreshold_      = "LowThreshold";
	std::string const colHighThreshold_     = "HighThreshold";
	std::string const colHighHighThreshold_ = "HighHighThreshold";
} channelColNames_;

//==============================================================================
ARTDAQEventBuilderTable::ARTDAQEventBuilderTable(void)
    : TableBase("ARTDAQEventBuilderTable")
    , ARTDAQTableBase("ARTDAQEventBuilderTable")
    , SlowControlsTableBase("ARTDAQEventBuilderTable")
    , isFirstAppInContext_(false)
    , channelListHasChanged_(false)
    , lastConfigManager_(nullptr)
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
bool ARTDAQEventBuilderTable::slowControlsChannelListHasChanged (void) const
{
	__COUT__ << "channelListHasChanged()" << __E__;
	if(isFirstAppInContext_)
		return channelListHasChanged_;
				
	if(lastConfigManager_ == nullptr)	
	{
		__SS__ << "Illegal call to get status of channel list, no config manager has been initialized!" << __E__;
		__SS_THROW__;
	}

	//if here, lastConfigManager_ pointer is defined
	bool changed = outputEpicsPVFile(lastConfigManager_);
	__COUT__ << "slowControlsChannelListHasChanged(): return " << std::boolalpha << std::to_string(changed) << __E__;
	return changed;
} // end slowControlsChannelListHasChanged()


//==============================================================================
void ARTDAQEventBuilderTable::getSlowControlsChannelList(
    std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>&
        channelList) const
{	
	outputEpicsPVFile(lastConfigManager_,&channelList);
} // end getSlowControlsChannelList()

//==============================================================================
unsigned int slowControlsHandler(
									  std::stringstream& out
									, std::string& tabStr
									, std::string& commentStr
									, std::string& subsystem
									, std::string& location
									, ConfigurationTree slowControlsLink
									, std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>* channelList /*= 0*/
								 )
{
	unsigned int numberOfChannels = 0;
	__COUT__ << "localSlowControlsHandler" << __E__;

	if(!slowControlsLink.isDisconnected())
	{
		std::vector<std::pair<std::string, ConfigurationTree>> channelChildren = slowControlsLink.getChildren();

		// first do single bit binary fields
		bool first = true;
		for(auto& channel : channelChildren)
		{
			if(channel.second.getNode(channelColNames_.colChannelDataType_).getValue<std::string>() != "1b")
				continue;  // skip non-binary fields

			if(first)  // if first, output header
			{
				first = false;
				OUT << "file \"dbt/soft_bi.dbt\" {" << __E__;
				PUSHTAB;
				OUT << "pattern  { Subsystem, loc, pvar, ZNAM, ONAM, ZSV, OSV, "
				       "COSV, DESC  }"
				    << __E__;
				PUSHTAB;
			}

			++numberOfChannels;

			std::string pvName    = channel.first;
			std::string comment   = channel.second.getNode(TableViewColumnInfo::COL_NAME_COMMENT).getValue<std::string>();

			// output channel
			OUT << "{ \"" << subsystem << "\", \"" << location << "\", \"" << pvName << "\", \""
			    << "NOT_OK"
			    << "\", \""
			    << "OK"
			    << "\", \""
			    << "MAJOR"
			    << "\", \""
			    << "NO_ALARM"
			    << "\", \""
			    << ""
			    << "\", \"" << comment << "\"  }" << __E__;

		}           // end binary channel loop
		if(!first)  // if there was data, then pop tabs
		{
			POPTAB;
			POPTAB;
			out << "}" << __E__;
		}

		// then do 'analog' fields
		first = true;
		for(auto& channel : channelChildren)
		{
			if(channel.second.getNode(channelColNames_.colChannelDataType_).getValue<std::string>() == "1b")
				continue;  // skip non-binary fields

			if(first)  // if first, output header
			{
				first = false;
				OUT << "file \"dbt/subst_ai.dbt\" {" << __E__;
				PUSHTAB;
				OUT << "pattern  { Subsystem, loc, pvar, PREC, EGU, LOLO, LOW, "
				       "HIGH, HIHI, MDEL, ADEL, INP, SCAN, DTYP, DESC }"
				    << __E__;
				PUSHTAB;
			}

			++numberOfChannels;

			std::string pvName         = channel.first;
			std::string comment        =
				channel.second.getNode(TableViewColumnInfo::COL_NAME_COMMENT)
					.getValue<std::string>();
			std::string precision      = "0";
			std::string units          =
				channel.second.getNode(channelColNames_.colUnits_)
					.getValue<std::string>();
				// channel.second.getNode(channelColNames_.colChannelDataType_)
				//     .getValue<std::string>();
			std::string low_alarm_lmt  =
				channel.second.getNode(channelColNames_.colLowLowThreshold_)
					.getValueWithDefault<std::string>("-1000");
			std::string low_warn_lmt   =
				channel.second.getNode(channelColNames_.colLowThreshold_)
					.getValueWithDefault<std::string>("-100");
			std::string high_warn_lmt  =
				channel.second.getNode(channelColNames_.colHighThreshold_)
					.getValueWithDefault<std::string>("100");
			std::string high_alarm_lmt =
				channel.second.getNode(channelColNames_.colHighHighThreshold_)
					.getValueWithDefault<std::string>("1000");
			if(channelList != nullptr)
			{
				std::vector<std::string> pvSettings;
				pvSettings.push_back(comment);
				pvSettings.push_back(low_warn_lmt);
				pvSettings.push_back(high_warn_lmt);
				pvSettings.push_back(low_alarm_lmt);
				pvSettings.push_back(high_alarm_lmt);
				pvSettings.push_back(precision);
				pvSettings.push_back(units);
				channelList->push_back(std::make_pair("Mu2e_" + subsystem + "_" + location + "/" + pvName, pvSettings));
			}

			// output channel
			OUT << "{ \"" << subsystem << "\", \"" << location << "\", \""
				<< pvName << "\", \""
				<< precision // PREC
				<< "\", \""
				<< units
				<< "\", \""
				<< low_alarm_lmt
				<< "\", \""
				<< low_warn_lmt
				<< "\", \""
				<< high_warn_lmt
				<< "\", \""
				<< high_alarm_lmt
				<< "\", \""
				<< ""
				<< "\", \"" <<  // MDEL
				""
				<< "\", \"" <<  // ADEL
				""
				<< "\", \"" <<  // INP
				""
				<< "\", \"" <<  // SCAN
				""
				<< "\", \"" <<  // DTYP
				comment << "\"  }" << __E__;

		}           // end binary channel loop
		if(!first)  // if there was data, then pop tabs
		{
			POPTAB;
			POPTAB;
			out << "}" << __E__;
		}
	}
	else
		__COUT__ << "Disconnected EventBuilder Slow Controls metric channels link, so assuming "
		            "no slow controls channels."
		         << __E__;

	return numberOfChannels;
} // end localSlowControlsHandler

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

	// create lambda function to handle slow controls link
	std::function<unsigned int(
	    std::string&,
	    std::string&,
	    ConfigurationTree,
	    std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>*)>
		localSlowControlsHandler = [this, &out, &tabStr, &commentStr](
	                                 std::string & subsystem,
	                                 std::string & location,
	                                 ConfigurationTree slowControlsLink,
	                                 std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>> * channelList /*= 0*/
	)
	{
		unsigned int numberOfChannels = 0;
		__COUT__ << "localSlowControlsHandler" << __E__;

		if(!slowControlsLink.isDisconnected())
		{
			std::vector<std::pair<std::string, ConfigurationTree>> channelChildren = slowControlsLink.getChildren();

			// first do single bit binary fields
			bool first = true;
			for(auto& channel : channelChildren)
			{
				if(channel.second.getNode(channelColNames_.colChannelDataType_).getValue<std::string>() != "1b")
					continue;  // skip non-binary fields

				if(first)  // if first, output header
				{
					first = false;
					OUT << "file \"dbt/soft_bi.dbt\" {" << __E__;
					PUSHTAB;
					OUT << "pattern  { Subsystem, loc, pvar, ZNAM, ONAM, ZSV, OSV, "
					       "COSV, DESC  }"
					    << __E__;
					PUSHTAB;
				}

				++numberOfChannels;

				std::string pvName  = channel.first;
				std::string comment = channel.second.getNode(TableViewColumnInfo::COL_NAME_COMMENT).getValue<std::string>();

				// output channel
				OUT << "{ \"" << subsystem << "\", \"" << location << "\", \"" << pvName << "\", \""
				    << "NOT_OK"
				    << "\", \""
				    << "OK"
				    << "\", \""
				    << "MAJOR"
				    << "\", \""
				    << "NO_ALARM"
				    << "\", \""
				    << ""
				    << "\", \"" << comment << "\"  }" << __E__;

			}           // end binary channel loop
			if(!first)  // if there was data, then pop tabs
			{
				POPTAB;
				POPTAB;
				out << "}" << __E__;
			}

			// then do 'analog' fields
			first = true;
			for(auto& channel : channelChildren)
			{
				if(channel.second.getNode(channelColNames_.colChannelDataType_).getValue<std::string>() == "1b")
					continue;  // skip non-binary fields

				if(first)  // if first, output header
				{
					first = false;
					OUT << "file \"dbt/subst_ai.dbt\" {" << __E__;
					PUSHTAB;
					OUT << "pattern  { Subsystem, loc, pvar, PREC, EGU, LOLO, LOW, "
					       "HIGH, HIHI, MDEL, ADEL, INP, SCAN, DTYP, DESC }"
					    << __E__;
					PUSHTAB;
				}

				++numberOfChannels;

				std::string pvName    = channel.first;
				std::string comment   = channel.second.getNode(TableViewColumnInfo::COL_NAME_COMMENT).getValue<std::string>();
				std::string precision = "0";
				std::string units     = channel.second.getNode(channelColNames_.colUnits_).getValue<std::string>();
				// channel.second.getNode(channelColNames_.colChannelDataType_)
				//     .getValue<std::string>();
				std::string low_alarm_lmt  = channel.second.getNode(channelColNames_.colLowLowThreshold_).getValueWithDefault<std::string>("-1000");
				std::string low_warn_lmt   = channel.second.getNode(channelColNames_.colLowThreshold_).getValueWithDefault<std::string>("-100");
				std::string high_warn_lmt  = channel.second.getNode(channelColNames_.colHighThreshold_).getValueWithDefault<std::string>("100");
				std::string high_alarm_lmt = channel.second.getNode(channelColNames_.colHighHighThreshold_).getValueWithDefault<std::string>("1000");
				if(channelList != nullptr)
				{
					std::vector<std::string> pvSettings;
					pvSettings.push_back(comment);
					pvSettings.push_back(low_warn_lmt);
					pvSettings.push_back(high_warn_lmt);
					pvSettings.push_back(low_alarm_lmt);
					pvSettings.push_back(high_alarm_lmt);
					pvSettings.push_back(precision);
					pvSettings.push_back(units);
					channelList->push_back(std::make_pair("Mu2e_" + subsystem + "_" + location + "/" + pvName, pvSettings));
				}

				// output channel
				OUT << "{ \"" << subsystem << "\", \"" << location << "\", \"" << pvName << "\", \"" << precision  // PREC
				    << "\", \"" << units << "\", \"" << low_alarm_lmt << "\", \"" << low_warn_lmt << "\", \"" << high_warn_lmt << "\", \"" << high_alarm_lmt
				    << "\", \""
				    << ""
				    << "\", \"" <<  // MDEL
				    ""
				    << "\", \"" <<  // ADEL
				    ""
				    << "\", \"" <<  // INP
				    ""
				    << "\", \"" <<  // SCAN
				    ""
				    << "\", \"" <<  // DTYP
				    comment << "\"  }" << __E__;

			}           // end binary channel loop
			if(!first)  // if there was data, then pop tabs
			{
				POPTAB;
				POPTAB;
				out << "}" << __E__;
			}
		}
		else
			__COUT__ << "Disconnected EventBuilder Slow Controls metric channels link, so assuming "
			            "no slow controls channels."
			         << __E__;

		return numberOfChannels;
	};  // end localSlowControlsHandler lambda function

	// loop through ARTDAQ EventBuilder records starting at ARTDAQSupervisorTable
	std::vector<std::pair<std::string, ConfigurationTree>> artdaqRecords =
		configManager->getNode("ARTDAQSupervisorTable").getChildren();

	unsigned int numberOfEventBuilders = 0;
	unsigned int numberOfEventBuiderMetricParameters = 0;

	for(auto& artdaqPair : artdaqRecords)  // start main artdaq record loop
	{
		if(!artdaqPair.second.status())
			continue;

		if(artdaqPair.second.getNode("EventBuildersLink").isDisconnected())
			continue;

		std::vector<std::pair<std::string, ConfigurationTree>> eventBuilderRecords = 
		artdaqPair.second.getNode("EventBuildersLink").getChildren();

		for(auto& eventBuilderPair : eventBuilderRecords)  // start main eventBuilder record loop
		{
			if(!eventBuilderPair.second.status())
				continue;

			numberOfEventBuilders++;

			try
			{
				if(eventBuilderPair.second.getNode("metricAlarmThresholdsLink").isDisconnected())
					continue;

				ConfigurationTree slowControlsLink =eventBuilderPair.second.getNode("metricAlarmThresholdsLink");

				if(eventBuilderPair.second.getNode("daqLink").isDisconnected())
					continue;

				auto daqLinks = eventBuilderPair.second.getNode("daqLink").getChildren();
				for(auto& daqLink : daqLinks)  // start daqLinks record loop
				{
					if(daqLink.second.getNode("daqMetricsLink").isDisconnected())
						continue;

					auto daqMetricsLinks = daqLink.second.getNode("daqMetricsLink").getChildren();
					for(auto& daqMetricsLink : daqMetricsLinks)  // start daqMetricsLinks record loop
					{
						if(!daqMetricsLink.second.status())
							continue;

						if(daqMetricsLink.second.getNode("metricParametersLink").isDisconnected())
							continue;

						auto metricParametersLinks = daqMetricsLink.second.getNode("metricParametersLink").getChildren();
						for(auto& metricParametersLink : metricParametersLinks)  // start daqMetricsLinks record loop
						{
							if(!metricParametersLink.second.status())
								continue;

							std::string subsystem = metricParametersLink.second.getNode("metricParameterValue")
								  .getValueWithDefault<std::string>(std::string("TDAQ_") + __ENV__("MU2E_OWNER"));

							numberOfEventBuiderMetricParameters =
							localSlowControlsHandler(
								  subsystem
								, eventBuilderPair.first
								, slowControlsLink
								, channelList);

							__COUT__ << "EventBuilder '" << eventBuilderPair.first
									<< "' number of metrics fro slow controls: "
									<< numberOfEventBuiderMetricParameters << __E__;
						}
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
