#ifndef _ots_ARTDAQSupervisorr_h
#define _ots_ARTDAQSupervisor_h

#if __cplusplus > 201402L
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#include <Python.h>
#pragma GCC diagnostic pop
#else
#include <Python.h>
#endif
#include <thread>
#include <mutex>

#include "artdaq/ExternalComms/CommanderInterface.hh"
#include "otsdaq/CoreSupervisors/CoreSupervisorBase.h"

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

	struct SubsystemInfo
	{
		int           destination;
		std::set<int> sources;

		SubsystemInfo() : destination(0), sources() {}
	};

	struct ProcessInfo
	{
		std::string label;
		std::string hostname;
		int         subsystem;

		ProcessInfo(std::string l, std::string h, int s)
		    : label(l), hostname(h), subsystem(s)
		{
		}
	};

	PyObject* daqinterface_ptr_;
	std::mutex  daqinterface_mutex_;
	int       partition_;
	std::string daqinterface_state_;
	std::unique_ptr<std::thread> runner_thread_;
	std::atomic<bool> runner_running_;

	void getDAQState_();
	void daqinterfaceRunner_();
	void stop_runner_();
	void start_runner_();
};

}  // namespace ots

#endif
