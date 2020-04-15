#include "otsdaq/TablePlugins/SlowControlsTableBase/SlowControlsTableBase.h"
#include "otsdaq/TablePlugins/XDAQContextTable.h"

using namespace ots;

#undef __MF_SUBJECT__
#define __MF_SUBJECT__ "SlowControlsTableBase"


//==============================================================================
// TableBase
//	If a valid string pointer is passed in accumulatedExceptions
//	then allowIllegalColumns is set for InfoReader
//	If accumulatedExceptions pointer = 0, then illegal columns throw std::runtime_error
// exception
SlowControlsTableBase::SlowControlsTableBase(std::string tableName, std::string* accumulatedExceptions /* =0 */)
: TableBase(tableName,accumulatedExceptions)
{
}  // end constuctor()

//==============================================================================
// SlowControlsTableBase
//	Default constructor should never be used because table type is lost
SlowControlsTableBase::SlowControlsTableBase(void) 
: TableBase("SlowControlsTableBase")
{
	__SS__ << "Should not call void constructor, table type is lost!" << __E__;
	__SS_THROW__;
}  // end illegal default constructor()

//==============================================================================
SlowControlsTableBase::~SlowControlsTableBase(void) {}  // end destructor()

//==============================================================================
void SlowControlsTableBase::getSlowControlsChannelList(
    std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>&
        channelList) const
{	
	outputEpicsPVFile(lastConfigManager_,&channelList);
} //end getSlowControlsChannelList()

//==============================================================================
bool SlowControlsTableBase::slowControlsChannelListHasChanged(void) const
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
} //end slowControlsChannelListHasChanged()

//==============================================================================
unsigned int SlowControlsTableBase::slowControlsHandler(std::stringstream&                                                             out,
                                                        std::string&                                                                   tabStr,
                                                        std::string&                                                                   commentStr,
                                                        std::string&                                                                   subsystem,
                                                        std::string&                                                                   location,
                                                        ConfigurationTree                                                              slowControlsLink,
                                                        std::vector<std::pair<std::string /*channelName*/, std::vector<std::string>>>* channelList /*= 0*/
) const
{
	unsigned int numberOfChannels = 0;
	__COUT__ << "slowControlsHandler" << __E__;

	//if(!slowControlsLink.isDisconnected())
	if(1)
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
				channelList->push_back(std::make_pair("Mu2e:" + subsystem + ":" + location + ":" + pvName, pvSettings));
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
}  // end localSlowControlsHandler