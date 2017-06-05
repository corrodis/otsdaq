#include "otsdaq-core/CoreSupervisors/FESupervisor.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(FESupervisor)

//========================================================================================================================
FESupervisor::FESupervisor(xdaq::ApplicationStub * s) throw (xdaq::exception::Exception)
: CoreSupervisorBase(s)

{
	CoreSupervisorBase::theStateMachineImplementation_.push_back(
			new FEVInterfacesManager (
					CoreSupervisorBase::theConfigurationManager_->getNode(CoreSupervisorBase::XDAQContextConfigurationName_),
					CoreSupervisorBase::supervisorConfigurationPath_
			)
	);

}

//========================================================================================================================
FESupervisor::~FESupervisor(void)
{
	//theStateMachineImplementation_ is reset and the object it points to deleted in ~CoreSupervisorBase()0
}
