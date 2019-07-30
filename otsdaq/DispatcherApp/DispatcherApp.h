#ifndef _ots_ARTDAQDispatcherSupervisor_h
#define _ots_ARTDAQDispatcherSupervisor_h

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"
#include "artdaq/Application/DispatcherApp.hh"

namespace ots
{
// DispatcherApp
//	This class provides the otsdaq interface to a single artdaq Dispatcher.
class DispatcherApp : public CoreSupervisorBase
{
  public:
	XDAQ_INSTANTIATOR();

	DispatcherApp(xdaq::ApplicationStub* stub);
	virtual ~DispatcherApp(void);

	void init(void);
	void destroy(void);

	virtual void request(const std::string&               requestType,
	                     cgicc::Cgicc&                    cgiIn,
	                     HttpXmlDocument&                 xmlOut,
	                     const WebUsers::RequestUserInfo& userInfo) override;

	virtual void transitionConfiguring(toolbox::Event::Reference e) override;
	virtual void transitionHalting(toolbox::Event::Reference e) override;
	virtual void transitionInitializing(toolbox::Event::Reference e) override;
	virtual void transitionPausing(toolbox::Event::Reference e) override;
	virtual void transitionResuming(toolbox::Event::Reference e) override;
	virtual void transitionStarting(toolbox::Event::Reference e) override;
	virtual void transitionStopping(toolbox::Event::Reference e) override;
	//	void enteringError(toolbox::Event::Reference e);


  private:

	void registerMonitor(const std::string& fcl);
	void unregisterMonitor(const std::string& label);

	std::unique_ptr<artdaq::DispatcherApp> theDispatcherInterface_;
	std::map<std::string /* monitor label */, std::string /* monitor fcl */> monitors_;
};

}  // namespace ots

#endif
