#ifndef _ots_ARTDAQSupervisorr_h
#define _ots_ARTDAQSupervisor_h

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#include <Python.h>
#pragma GCC diagnostic pop

#include "artdaq/ExternalComms/CommanderInterface.hh"
#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

namespace ots
{
// class ConfigurationManager;
// class TableGroupKey;

// ARTDAQSupervisor
//	This class provides the otsdaq Supervisor interface to a single artdaq Data Logger.
class ARTDAQSupervisor : public CoreSupervisorBase
// public xdaq::Application,
// public SOAPMessenger,
// public RunControlStateMachine
{
  public:
	XDAQ_INSTANTIATOR();

	ARTDAQSupervisor(xdaq::ApplicationStub* s);
	virtual ~ARTDAQSupervisor(void);

	void init(void);
	void destroy(void);

	virtual void transitionConfiguring(toolbox::Event::Reference e) override;
	virtual void transitionHalting(toolbox::Event::Reference e) override;
	virtual void transitionInitializing(toolbox::Event::Reference e) override;
	virtual void transitionPausing(toolbox::Event::Reference e) override;
	virtual void transitionResuming(toolbox::Event::Reference e) override;
	virtual void transitionStarting(toolbox::Event::Reference e) override;
	virtual void transitionStopping(toolbox::Event::Reference e) override;
	virtual void enteringError(toolbox::Event::Reference e);

  private:
	PyObject* daqinterface_ptr_;
	int       partition_;
	};

}  // namespace ots

#endif
