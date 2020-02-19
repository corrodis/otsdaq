#include "otsdaq/SupervisorInfo/SupervisorInfo.h"

using namespace ots;

const std::string SupervisorInfo::APP_STATUS_UNKNOWN = "Unknown";

//=====================================================================================
void SupervisorInfo::setStatus(const std::string& status, const unsigned int progress, const std::string& detail)
{
	status_   = status;
	progress_ = progress;
	detail_   = detail;
	if(status != SupervisorInfo::APP_STATUS_UNKNOWN)  // if unknown, then do not consider it a status update
		lastStatusTime_ = time(0);
}  // end setStatus()

//=====================================================================================
void SupervisorInfo::clear(void)
{
	descriptor_        = 0;
	progress_          = 0;
	contextDescriptor_ = 0;
	name_              = "";
	id_                = 0;
	contextName_       = "";
	status_            = SupervisorInfo::APP_STATUS_UNKNOWN;
}  // end clear()
