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

	virtual const HostTraceLevelMap& 	getTraceLevels		(void) = 0; // pure virtual
	virtual void              			setTraceLevelMask	(std::string const& name, TraceMasks const& lvl, std::string const& hostname = "localhost", std::string const& mode = "ALL") = 0; // pure virtual

	virtual bool 						getIsTriggered		(void) = 0;	// pure virtual
	virtual void 						setTriggerEnable	(size_t entriesAfterTrigger) = 0; // pure virtual

	virtual void                   		resetTraceBuffer	(void) = 0;	// pure virtual
	virtual void                   		enableTrace			(void) = 0;	// pure virtual

  protected:
	//=====================================
	void addTraceLevelsForThisHost(void)
	{
		auto hostname = getHostnameString();
		TLOG(TLVL_DEBUG) << "Adding TRACE levels [" << hostname << "]";

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
	}  //end addTraceLevelsForThisHost()

	//=====================================
	void setTraceLevelsForThisHost(std::string const& name, TraceMasks const& lvl, std::string const& mode = "ALL")
	{
		auto hostname = getHostnameString();
		TLOG(TLVL_DEBUG) << "Setting TRACE levels [" << hostname << "]";

		bool allMode = mode == "ALL";
		TLOG(TLVL_DEBUG) << "Setting " << mode << " levels for name '" << name << "' to " <<
				std::hex << std::showbase << lvl.M << " " << lvl.S << " " << lvl.T;
		if(name != "ALL")
		{
			if(allMode || mode == "FAST")
				TRACE_CNTL("lvlmsknM", name.c_str(), lvl.M);
			if(allMode || mode == "SLOW")
				TRACE_CNTL("lvlmsknS", name.c_str(), lvl.S);
			if(allMode || mode == "TRIGGER")
				TRACE_CNTL("lvlmsknT", name.c_str(), lvl.T);
		}
		else
		{
			if(allMode || mode == "FAST")
				TRACE_CNTL("lvlmskMg", lvl.M);
			if(allMode || mode == "SLOW")
				TRACE_CNTL("lvlmskSg", lvl.S);
			if(allMode || mode == "TRIGGER")
				TRACE_CNTL("lvlmskTg", lvl.T);
		}
	}  //end setTraceLevelsForThisHost()

	//=====================================
	std::string getHostnameString(void)
	{
		char hostname_c[HOST_NAME_MAX];
		gethostname(hostname_c, HOST_NAME_MAX);
		return std::string(hostname_c);
	} //end getHostnameString()

	HostTraceLevelMap traceLevelsMap_;
};

}  // namespace ots

#endif  // OTSDAQ_MESSAGEFACILITY_ITRACECONTROLLER_H
