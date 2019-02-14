#include "otsdaq-core/CoreSupervisors/DataManagerSupervisor.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(DataManagerSupervisor)

//========================================================================================================================
DataManagerSupervisor::DataManagerSupervisor(xdaq::ApplicationStub* s)
    : CoreSupervisorBase(s)
{
	CoreSupervisorBase::theStateMachineImplementation_.push_back(
	    DataManagerSingleton::getInstance<DataManager>(
		CorePropertySupervisorBase::getContextTreeNode(),
		CorePropertySupervisorBase::supervisorConfigurationPath_,
		CorePropertySupervisorBase::supervisorApplicationUID_));
}

//========================================================================================================================
DataManagerSupervisor::~DataManagerSupervisor(void)
{
	__SUP_COUT__ << "Destroying..." << std::endl;

	DataManagerSingleton::deleteInstance(CorePropertySupervisorBase::supervisorApplicationUID_);
	theStateMachineImplementation_.pop_back();
}
