#ifndef OTSDAQ_MESSAGEFACILITY_ITRACECONTROLLER_H
#define OTSDAQ_MESSAGEFACILITY_ITRACECONTROLLER_H

#include <limits.h>
#include <unistd.h>
#include <deque>
#include <string>
#include <unordered_map>

namespace ots
{
class ITRACEController
{
  public:
	struct TraceMasks
	{
		uint64_t M;
		uint64_t S;
		uint64_t T;
	};
	typedef std::unordered_map<std::string, TraceMasks>    TraceLevelMap;
	typedef std::unordered_map<std::string, TraceLevelMap> HostTraceLevelMap;

	ITRACEController() {}
	virtual ~ITRACEController() = default;

	virtual HostTraceLevelMap GetTraceLevels()                                                                                 = 0;
	virtual void              SetTraceLevelMask(std::string trace_name, TraceMasks const& lvl, std::string host = "localhost") = 0;

  protected:
	std::string GetHostnameString()
	{
		char hostname_c[HOST_NAME_MAX];
		gethostname(hostname_c, HOST_NAME_MAX);
		return std::string(hostname_c);
	}
};

class NullTRACEController : public ITRACEController
{
  public:
	NullTRACEController() {}
	virtual ~NullTRACEController() = default;

	HostTraceLevelMap GetTraceLevels() final { return HostTraceLevelMap(); }
	void              SetTraceLevelMask(std::string, TraceMasks const&, std::string) final {}
};

}  // namespace ots

#endif  // OTSDAQ_MESSAGEFACILITY_ITRACECONTROLLER_H