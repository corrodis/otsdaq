#include "otsdaq/MessageFacility/TRACEController.h"

const ots::ITRACEController::HostTraceLevelMap& ots::TRACEController::getTraceLevels()
{
	TLOG(TLVL_DEBUG) << "Getting TRACE levels";
	traceLevelsMap_.clear();  // reset
	ots::ITRACEController::addTraceLevelsForThisHost();
	return traceLevelsMap_;
}  // end getTraceLevels()

void ots::TRACEController::setTraceLevelMask(std::string const& name, TraceMasks const& lvl, std::string const& hostname)
{
	if(hostname != "localhost" && hostname != getHostnameString())
	{
		TLOG(TLVL_WARNING) << "TRACEController asked to set TRACE levels for host " << hostname << ", but this is " << getHostnameString() << "!";
		return;
	}
	TLOG(TLVL_DEBUG) << "Setting levels for name " << name << " to " << std::hex << std::showbase << lvl.M << " " << lvl.S << " " << lvl.T;
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
}  // end setTraceLevelMask()

bool ots::TRACEController::getIsTriggered()
{
	traceInit(NULL, 1);
	return static_cast<bool>(traceControl_rwp->triggered);
}

void ots::TRACEController::setTriggerEnable(size_t entriesAfterTrigger) { 
	TRACE_CNTL("trig", entriesAfterTrigger + 1);
}

void ots::TRACEController::resetTraceBuffer()
{
	TRACE_CNTL("reset");
}

void ots::TRACEController::enableTrace()
{
	TRACE_CNTL("modeM", 1);
	TRACE_CNTL("modeS", 1);
}
