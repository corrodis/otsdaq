#include "otsdaq/MessageFacility/TRACEController.h"

const ots::ITRACEController::HostTraceLevelMap& ots::TRACEController::getTraceLevels()
{
	TLOG(TLVL_DEBUG) << "Getting TRACE levels";
	traceLevelsMap_.clear(); //reset
	ots::ITRACEController::addTraceLevelsForThisHost();
	return traceLevelsMap_;
} //end getTraceLevels()

void ots::TRACEController::setTraceLevelMask(const std::string& name, TraceMasks const& lvl, const std::string& hostname)
{
	if(hostname != "localhost" && hostname != getHostnameString())
	{
		TLOG(TLVL_WARNING) << "TRACEController asked to set TRACE levels for host " <<
				hostname << ", but this is " << getHostnameString() << "!";
		return;
	}
	TLOG(TLVL_DEBUG) << "Setting levels for name " << name << " to " << std::hex <<
			std::showbase << lvl.M << " " << lvl.S << " " << lvl.T;
	if(name != "ALL")
	{
		TRACE_CNTL("lvlmsknM", name.c_str(), lvl.M);
		TRACE_CNTL("lvlmsknS", name.c_str(), lvl.S);
		TRACE_CNTL("lvlmsknT", name.c_str(), lvl.T);
	}
	else
	{
		TRACE_CNTL("lvlmskMg", lvl.M);
		TRACE_CNTL("lvlmskSg", lvl.S);
		TRACE_CNTL("lvlmskTg", lvl.T);
	}
} //end setTraceLevelMask()
