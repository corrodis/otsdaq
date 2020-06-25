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

	const ITRACEController::HostTraceLevelMap& getTraceLevels() final;
	void setTraceLevelMask(const std::string& trace_name, TraceMasks const& lvl, const std::string& host = "localhost") final;

	void setSupervisorPtr(ARTDAQSupervisor* ptr) { theSupervisor_ = ptr; }


	// These functions are defaulted because ARTDAQSupervisorTRACEController doesn't have direct access to the ARTDAQ TRACE Buffers
	virtual bool getIsTriggered() { return false; }
	virtual void setTriggerEnable(size_t) {}

	virtual void                   resetTraceBuffer() {}
	virtual void                   enableTrace() {}

  private:
	ARTDAQSupervisor* theSupervisor_;
};
}  // namespace ots

#endif  // OTSDAQ_ARTDAQSUPERVISOR_ARTDAQSUPERVISORTRACECONTROLLER_H
