#include "otsdaq/ARTDAQSupervisor/ARTDAQSupervisorTRACEController.h"

ots::ARTDAQSupervisorTRACEController::ARTDAQSupervisorTRACEController() {}

ots::ITRACEController::HostTraceLevelMap ots::ARTDAQSupervisorTRACEController::GetTraceLevels()
{
	HostTraceLevelMap output;
	if(theSupervisor_)
	{
		auto commanders = theSupervisor_->makeCommandersFromProcessInfo();

		for(auto& comm : commanders)
		{
			if(output.count(comm.first.host) == 0)
			{
				auto lvlstring = comm.second->send_trace_get("ALL");
				auto lvls      = ARTDAQSupervisor::tokenize_(lvlstring);

				for(auto& lvl : lvls)
				{
					std::istringstream iss(lvl);
					std::string        name;
					uint64_t           lvlM, lvlS, lvlT;

					iss >> name >> lvlM >> lvlS >> lvlT;

					output[comm.first.host][name].M = lvlM;
					output[comm.first.host][name].S = lvlS;
					output[comm.first.host][name].T = lvlT;
				}
			}
		}
	}

	return output;
}

void ots::ARTDAQSupervisorTRACEController::SetTraceLevelMask(std::string trace_name, TraceMasks const& lvl, std::string host)
{
	if(theSupervisor_)
	{
		auto commanders = theSupervisor_->makeCommandersFromProcessInfo();

		for(auto& comm : commanders)
		{
			if(comm.first.host == host)
			{
				comm.second->send_trace_set(trace_name, "M", lvl.M);
				comm.second->send_trace_set(trace_name, "S", lvl.S);
				comm.second->send_trace_set(trace_name, "T", lvl.T);
				break;
			}
		}
	}
}
