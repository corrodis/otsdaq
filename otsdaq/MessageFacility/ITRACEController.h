#ifndef OTSDAQ_MESSAGEFACILITY_ITRACECONTROLLER_H
#define OTSDAQ_MESSAGEFACILITY_ITRACECONTROLLER_H

#include <limits.h>
#include <unistd.h>
#include <deque>
#include <list>
#include <string>
#include <unordered_map>
#include "TRACE/trace.h"

namespace ots
{
class ITRACEController
{
  public:
	struct TraceMasks
	{
		uint64_t             M;
		uint64_t             S;
		uint64_t             T;
		friend std::ostream& operator<<(std::ostream& out, const TraceMasks& traceMask)
		{
			out << "Memory:" << traceMask.M << ",Slow:" << traceMask.S << ",Trigger:" << traceMask.T;
			return out;
		}
	};
	typedef std::unordered_map<std::string, TraceMasks>    TraceLevelMap;
	typedef std::unordered_map<std::string, TraceLevelMap> HostTraceLevelMap;

	ITRACEController() {}
	virtual ~ITRACEController() = default;

	virtual const HostTraceLevelMap& getTraceLevels(void) = 0;  // pure virtual
	virtual void                     setTraceLevelMask(const std::string& trace_name, TraceMasks const& lvl, const std::string& host = "localhost") = 0;

	virtual bool getIsTriggered()                             = 0;
	virtual void setTriggerEnable(size_t entriesAfterTrigger) = 0;

	virtual void                   resetTraceBuffer()                                 = 0;
	virtual void                   enableTrace()                                      = 0;

  protected:
	void addTraceLevelsForThisHost(void)
	{
		TLOG(TLVL_DEBUG) << "Adding TRACE levels";
		auto hostname = getHostnameString();

		unsigned ee = traceControl_p->num_namLvlTblEnts;
		for(unsigned ii = 0; ii < ee; ++ii)
		{
			if(traceNamLvls_p[ii].name[0])
			{
				std::string name                  = std::string(traceNamLvls_p[ii].name);
				traceLevelsMap_[hostname][name].M = traceNamLvls_p[ii].M;
				traceLevelsMap_[hostname][name].S = traceNamLvls_p[ii].S;
				traceLevelsMap_[hostname][name].T = traceNamLvls_p[ii].T;
			}
		}
	}  // end addTraceLevelsForThisHost()

	std::string getHostnameString(void)
	{
		char hostname_c[HOST_NAME_MAX];
		gethostname(hostname_c, HOST_NAME_MAX);
		return std::string(hostname_c);
	}
	HostTraceLevelMap traceLevelsMap_;
};

}  // namespace ots

#endif  // OTSDAQ_MESSAGEFACILITY_ITRACECONTROLLER_H
