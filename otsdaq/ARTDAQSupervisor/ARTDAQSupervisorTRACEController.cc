#include "otsdaq/ARTDAQSupervisor/ARTDAQSupervisorTRACEController.h"

ots::ARTDAQSupervisorTRACEController::ARTDAQSupervisorTRACEController() {}

ots::ITRACEController::HostTraceLevelMap ots::ARTDAQSupervisorTRACEController::GetTraceLevels()
{
	HostTraceLevelMap output;
	if(theSupervisor_)
	{
		auto ret = theSupervisor_->GetTraceLevels();
		if(ret == "NOT IMPLEMENTED")
			return output;
	}

	return output;
}

void ots::ARTDAQSupervisorTRACEController::SetTraceLevelMask(std::string trace_name, TraceMasks const& lvl, std::string host)
{
	if(theSupervisor_)
	{
		theSupervisor_->SetTraceLevel(trace_name, lvl.M, lvl.S, lvl.T, host);
	}
}