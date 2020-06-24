#include "otsdaq/ARTDAQSupervisor/ARTDAQSupervisorTRACEController.h"
#include <string>

ots::ARTDAQSupervisorTRACEController::ARTDAQSupervisorTRACEController()
{}

const ots::ITRACEController::HostTraceLevelMap& ots::ARTDAQSupervisorTRACEController::getTraceLevels()
{
	traceLevelsMap_.clear(); //reset
	if(theSupervisor_)
	{
		auto commanders = theSupervisor_->makeCommandersFromProcessInfo();

		for(auto& comm : commanders)
		{
			if(traceLevelsMap_.count(comm.first.host) == 0)
			{
				auto lvlstring = comm.second->send_trace_get("ALL");
				auto lvls      = ARTDAQSupervisor::tokenize_(lvlstring);

				for(auto& lvl : lvls)
				{
					std::istringstream iss(lvl);
					std::string        name;
					uint64_t           lvlM, lvlS, lvlT;

					iss >> name >> lvlM >> lvlS >> lvlT;

					traceLevelsMap_[comm.first.host][name].M = lvlM;
					traceLevelsMap_[comm.first.host][name].S = lvlS;
					traceLevelsMap_[comm.first.host][name].T = lvlT;
				}
			}
		}
	}
	ots::ITRACEController::addTraceLevelsForThisHost();

	return traceLevelsMap_;
} //end getTraceLevels()

void ots::ARTDAQSupervisorTRACEController::setTraceLevelMask(const std::string& trace_name, TraceMasks const& lvl, const std::string& host)
{
	if(theSupervisor_)
	{
		auto commanders = theSupervisor_->makeCommandersFromProcessInfo();

		for(auto& comm : commanders)
		{
			if(comm.first.host == host)
			{
				comm.second->send_trace_set(trace_name, "M", std::to_string(lvl.M));
				comm.second->send_trace_set(trace_name, "S", std::to_string(lvl.S));
				comm.second->send_trace_set(trace_name, "T", std::to_string(lvl.T));
				break;
			}
		}
	}
} //end setTraceLevelMask()
