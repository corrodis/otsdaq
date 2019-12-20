#include "otsdaq/MessageFacility/ITRACEController.h"
#include "TRACE/trace.h"

std::unordered_map<std::string, std::deque<TraceLevel>> ots::TRACEController::GetTraceLevels()
{
	return std::unordered_map<std::string, std::deque<TraceLevel>>();
}

void ots::TRACEController::SetTraceLevelMask(TraceLevel const& lvl)
{
}