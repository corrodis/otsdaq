#include "otsdaq-core/CoreSupervisors/ARTDAQFEDataManagerSupervisor.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"
#include "otsdaq-core/ARTDAQDataManager/ARTDAQDataManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(ARTDAQFEDataManagerSupervisor)

//========================================================================================================================
ARTDAQFEDataManagerSupervisor::ARTDAQFEDataManagerSupervisor(xdaq::ApplicationStub * s) 
//FIXME WE MUST ADD A MAP OF INSTANCES IN THE SINGLETON
: CoreSupervisorBase(s)
{
	//WARNING THE ORDER IS IMPORTANT SINCE THE FIRST ELEMENTS WILLL BE CALLED FIRST!!!!!
	CoreSupervisorBase::theStateMachineImplementation_.push_back(
			new FEVInterfacesManager(
					CorePropertySupervisorBase::getContextTreeNode(),
					CorePropertySupervisorBase::supervisorConfigurationPath_//FIXME Was ARTADFE
			)
	);

	CoreSupervisorBase::theStateMachineImplementation_.push_back(
			DataManagerSingleton::getInstance<ARTDAQDataManager>(
					CorePropertySupervisorBase::getContextTreeNode(),
					CorePropertySupervisorBase::supervisorConfigurationPath_,
					CorePropertySupervisorBase::supervisorApplicationUID_
			)
	);
}

//========================================================================================================================
ARTDAQFEDataManagerSupervisor::~ARTDAQFEDataManagerSupervisor(void)
{
	DataManagerSingleton::deleteInstance(CorePropertySupervisorBase::supervisorApplicationUID_);
	theStateMachineImplementation_.pop_back();
}
