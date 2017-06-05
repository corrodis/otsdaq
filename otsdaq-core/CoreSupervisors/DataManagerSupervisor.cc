#include "otsdaq-core/CoreSupervisors/DataManagerSupervisor.h"
#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(DataManagerSupervisor)

//========================================================================================================================
DataManagerSupervisor::DataManagerSupervisor(xdaq::ApplicationStub * s) throw (xdaq::exception::Exception)
//FIXME WE MUST ADD A MAP OF INSTANCES IN THE SINGLETON
: CoreSupervisorBase(s)
{
	CoreSupervisorBase::theStateMachineImplementation_.push_back(
			DataManagerSingleton::getInstance<DataManager>(
					CoreSupervisorBase::theConfigurationManager_->getNode(CoreSupervisorBase::XDAQContextConfigurationName_),
					CoreSupervisorBase::supervisorConfigurationPath_,
					CoreSupervisorBase::supervisorApplicationUID_
			)
	);
}

//========================================================================================================================
DataManagerSupervisor::~DataManagerSupervisor(void)
{
	DataManagerSingleton::deleteInstance(CoreSupervisorBase::supervisorApplicationUID_);
	theStateMachineImplementation_.pop_back();
}
