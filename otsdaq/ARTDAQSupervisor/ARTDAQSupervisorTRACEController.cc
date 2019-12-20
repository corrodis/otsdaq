#include "otsdaq/ARTDAQSupervisor/ARTDAQSupervisorTRACEController.h"

ots::ARTDAQSupervisorTRACEController::ARTDAQSupervisorTRACEController()
{
}

std::unordered_map<std::string, std::deque<TraceLevel>> ots::ARTDAQSupervisorTRACEController::GetTraceLevels()
{
	return std::unordered_map<std::string, std::deque<TraceLevel>>();
}

void ots::ARTDAQSupervisorTRACEController::SetTraceLevelMask(TraceLevel const& lvl)
{
}