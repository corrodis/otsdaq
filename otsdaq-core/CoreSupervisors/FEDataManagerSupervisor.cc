#include "otsdaq-core/CoreSupervisors/FEDataManagerSupervisor.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"
#include "otsdaq-core/DataManager/DataManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(FEDataManagerSupervisor)

//========================================================================================================================
FEDataManagerSupervisor::FEDataManagerSupervisor(xdaq::ApplicationStub * s) 
: FESupervisor(s)
{

	//WARNING THE ORDER IS IMPORTANT SINCE THE FIRST ELEMENTS WILLL BE CALLED FIRST!!!!!

	//the FE Interfaces Manager is added first, and then the Data Manager
	//	So on FSM transitions, front-ends will transition first.

//
//	CoreSupervisorBase::theStateMachineImplementation_.push_back(
//			new FEVInterfacesManager(
//					CorePropertySupervisorBase::getContextTreeNode(),
//					CorePropertySupervisorBase::supervisorConfigurationPath_
//			)
//	);

	CoreSupervisorBase::theStateMachineImplementation_.push_back(
			DataManagerSingleton::getInstance<DataManager>(
					CorePropertySupervisorBase::getContextTreeNode(),
					CorePropertySupervisorBase::supervisorConfigurationPath_,
					CorePropertySupervisorBase::supervisorApplicationUID_
			)
	);
}

//========================================================================================================================
FEDataManagerSupervisor::~FEDataManagerSupervisor(void)
{
	__SUP_COUT__ << "Destroying..." << std::endl;

	//theStateMachineImplementation_ is reset and the object it points to deleted in ~CoreSupervisorBase()0
	//This destructor must happen before the CoreSupervisor destructor

	DataManagerSingleton::deleteInstance(CoreSupervisorBase::supervisorApplicationUID_);
	theStateMachineImplementation_.pop_back();
}
