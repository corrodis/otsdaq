#include "otsdaq-core/CoreSupervisors/DataManagerSupervisor.h"
#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(DataManagerSupervisor)

//========================================================================================================================
DataManagerSupervisor::DataManagerSupervisor(xdaq::ApplicationStub * s) 
//FIXME WE MUST ADD A MAP OF INSTANCES IN THE SINGLETON
: CoreSupervisorBase(s)
{
	CoreSupervisorBase::theStateMachineImplementation_.push_back(
			DataManagerSingleton::getInstance<DataManager>(
					CorePropertySupervisorBase::getContextTreeNode(),
					CorePropertySupervisorBase::supervisorConfigurationPath_,
					CorePropertySupervisorBase::supervisorApplicationUID_
			)
	);
}

//========================================================================================================================
DataManagerSupervisor::~DataManagerSupervisor(void)
{
	DataManagerSingleton::deleteInstance(CorePropertySupervisorBase::supervisorApplicationUID_);
	theStateMachineImplementation_.pop_back();
}
