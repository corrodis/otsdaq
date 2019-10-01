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

#include <mutex>
#include <thread>

#include "artdaq/ExternalComms/CommanderInterface.hh"
#include "otsdaq/CoreSupervisors/CoreSupervisorBase.h"
#include "otsdaq/TablePlugins/ARTDAQTableBase/ARTDAQTableBase.h"

namespace ots
{
// clang-format off


// ARTDAQSupervisor
//	This class provides the otsdaq Supervisor interface to a single artdaq Data Logger.
class ARTDAQSupervisor : public CoreSupervisorBase
{
  public:
	XDAQ_INSTANTIATOR();

	ARTDAQSupervisor							(xdaq::ApplicationStub* s);
	virtual ~ARTDAQSupervisor					(void);



	void 			init						(void);
	void 			destroy						(void);

	virtual void 	transitionConfiguring		(toolbox::Event::Reference event) override;
	virtual void 	transitionHalting			(toolbox::Event::Reference event) override;
	virtual void 	transitionInitializing		(toolbox::Event::Reference event) override;
	virtual void 	transitionPausing			(toolbox::Event::Reference event) override;
	virtual void 	transitionResuming			(toolbox::Event::Reference event) override;
	virtual void 	transitionStarting			(toolbox::Event::Reference event) override;
	virtual void 	transitionStopping			(toolbox::Event::Reference event) override;
	virtual void 	enteringError				(toolbox::Event::Reference event);

  private:
	static void 	configuringThread(ARTDAQSupervisor* cs);

	PyObject* 						daqinterface_ptr_;
	std::mutex  					daqinterface_mutex_;
	int       						partition_;
	std::string 					daqinterface_state_;
	std::unique_ptr<std::thread> 	runner_thread_;
	std::atomic<bool> 				runner_running_;


	void 			getDAQState_				(void);
	void 			daqinterfaceRunner_			(void);
	void 			stop_runner_				(void);
	void 			start_runner_				(void);
};

}  // namespace ots

// clang-format on
#endif
