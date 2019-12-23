#ifndef OTSDAQ_ARTDAQSUPERVISOR_ARTDAQSUPERVISORTRACECONTROLLER_H
#define OTSDAQ_ARTDAQSUPERVISOR_ARTDAQSUPERVISORTRACECONTROLLER_H

#include "otsdaq/ARTDAQSupervisor/ARTDAQSupervisor.hh"
#include "otsdaq/MessageFacility/ITRACEController.h"

namespace ots
{
class ARTDAQSupervisorTRACEController : public ITRACEController
{
  public:
	ARTDAQSupervisorTRACEController();
	virtual ~ARTDAQSupervisorTRACEController() { theSupervisor_ = nullptr; }

	HostTraceLevelMap GetTraceLevels() final;
	void              SetTraceLevelMask(std::string trace_name, TraceMasks const& lvl, std::string host = "localhost") final;

	void SetSupervisorPtr(ARTDAQSupervisor* ptr) { theSupervisor_ = ptr; }

  private:

	ARTDAQSupervisor* theSupervisor_;
};
}  // namespace ots

#endif  // OTSDAQ_ARTDAQSUPERVISOR_ARTDAQSUPERVISORTRACECONTROLLER_H