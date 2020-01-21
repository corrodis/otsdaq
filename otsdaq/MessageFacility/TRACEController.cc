#include "otsdaq/MessageFacility/TRACEController.h"
#include "TRACE/trace.h"

ots::ITRACEController::HostTraceLevelMap ots::TRACEController::GetTraceLevels()
{
	TLOG(TLVL_DEBUG) << "Getting TRACE levels";

	HostTraceLevelMap output;
	auto              hostname = GetHostnameString();

	unsigned ee = traceControl_p->num_namLvlTblEnts;
	for(unsigned ii = 0; ii < ee; ++ii)
	{
		if(traceNamLvls_p[ii].name[0])
		{
			std::string name         = std::string(traceNamLvls_p[ii].name);
			output[hostname][name].M = traceNamLvls_p[ii].M;
			output[hostname][name].S = traceNamLvls_p[ii].S;
			output[hostname][name].T = traceNamLvls_p[ii].T;
		}
	}
	return output;
}

void ots::TRACEController::SetTraceLevelMask(std::string name, TraceMasks const& lvl, std::string hostname)
{
	if(hostname != "localhost" && hostname != GetHostnameString())
	{
		TLOG(TLVL_WARNING) << "TRACEController asked to set TRACE levels for host " << hostname << ", but this is " << GetHostnameString() << "!";
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
}