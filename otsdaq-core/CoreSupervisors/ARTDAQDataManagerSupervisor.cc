#include "otsdaq-core/CoreSupervisors/ARTDAQDataManagerSupervisor.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"
#include "otsdaq-core/ARTDAQDataManager/ARTDAQDataManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(ARTDAQDataManagerSupervisor)

//========================================================================================================================
ARTDAQDataManagerSupervisor::ARTDAQDataManagerSupervisor(xdaq::ApplicationStub * s) throw (xdaq::exception::Exception)
//FIXME WE MUST ADD A MAP OF INSTANCES IN THE SINGLETON
: CoreSupervisorBase(s)
{
	__COUT__ << "Begin!" << std::endl;
	__COUT__ << "Begin!" << std::endl;
	__COUT__ << "Begin!" << std::endl;
	__COUT__ << "Begin!" << std::endl;
	__COUT__ << "Begin!" << std::endl;

	CoreSupervisorBase::theStateMachineImplementation_.push_back(
			DataManagerSingleton::getInstance<ARTDAQDataManager>(
					CoreSupervisorBase::theConfigurationManager_->getNode(CoreSupervisorBase::XDAQContextConfigurationName_),
					CoreSupervisorBase::supervisorConfigurationPath_,
					CoreSupervisorBase::supervisorApplicationUID_)
	);

	__COUT__ << "Initialized!" << std::endl;
	__COUT__ << "Initialized!" << std::endl;
	__COUT__ << "Initialized!" << std::endl;
	__COUT__ << "Initialized!" << std::endl;
	__COUT__ << "Initialized!" << std::endl;

}

//========================================================================================================================
ARTDAQDataManagerSupervisor::~ARTDAQDataManagerSupervisor(void)
{
	DataManagerSingleton::deleteInstance(CoreSupervisorBase::supervisorApplicationUID_);
	theStateMachineImplementation_.pop_back();
}
