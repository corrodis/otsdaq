#include "otsdaq-core/CoreSupervisors/FEDataManagerSupervisor.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"
#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(FEDataManagerSupervisor)

//========================================================================================================================
FEDataManagerSupervisor::FEDataManagerSupervisor(xdaq::ApplicationStub * s) throw (xdaq::exception::Exception)
//FIXME WE MUST ADD A MAP OF INSTANCES IN THE SINGLETON
: CoreSupervisorBase(s)
{
	//WARNING THE ORDER IS IMPORTANT SINCE THE FIRST ELEMENTS WILLL BE CALLED FIRST!!!!!
	CoreSupervisorBase::theStateMachineImplementation_.push_back(
			new FEVInterfacesManager(
					CoreSupervisorBase::theConfigurationManager_->getNode(CoreSupervisorBase::XDAQContextConfigurationName_),
					CoreSupervisorBase::supervisorConfigurationPath_
			)
	);

	CoreSupervisorBase::theStateMachineImplementation_.push_back(
			DataManagerSingleton::getInstance<DataManager>(
					CoreSupervisorBase::theConfigurationManager_->getNode(CoreSupervisorBase::XDAQContextConfigurationName_),
					CoreSupervisorBase::supervisorConfigurationPath_,
					CoreSupervisorBase::supervisorApplicationUID_
			)
	);
}

//========================================================================================================================
FEDataManagerSupervisor::~FEDataManagerSupervisor(void)
{
	DataManagerSingleton::deleteInstance(CoreSupervisorBase::supervisorApplicationUID_);
	theStateMachineImplementation_.pop_back();
}
