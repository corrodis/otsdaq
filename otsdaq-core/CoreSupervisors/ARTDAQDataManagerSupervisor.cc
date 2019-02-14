#include "otsdaq-core/CoreSupervisors/ARTDAQDataManagerSupervisor.h"
#include "otsdaq-core/ARTDAQDataManager/ARTDAQDataManager.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/DataManager/DataManagerSingleton.h"
#include "otsdaq-core/FECore/FEVInterfacesManager.h"

using namespace ots;

XDAQ_INSTANTIATOR_IMPL (ARTDAQDataManagerSupervisor)

//========================================================================================================================
ARTDAQDataManagerSupervisor::ARTDAQDataManagerSupervisor (xdaq::ApplicationStub* s)
    : CoreSupervisorBase (s)
{
	__SUP_COUT__ << "Constructor." << std::endl;

	CoreSupervisorBase::theStateMachineImplementation_.push_back (
	    DataManagerSingleton::getInstance<ARTDAQDataManager> (
	        CorePropertySupervisorBase::getContextTreeNode (),
	        CorePropertySupervisorBase::supervisorConfigurationPath_,
	        CorePropertySupervisorBase::supervisorApplicationUID_));

	__SUP_COUT__ << "Constructed." << __E__;
}

//========================================================================================================================
ARTDAQDataManagerSupervisor::~ARTDAQDataManagerSupervisor (void)
{
	__SUP_COUT__ << "Destroying..." << std::endl;
	DataManagerSingleton::deleteInstance (CorePropertySupervisorBase::supervisorApplicationUID_);
	theStateMachineImplementation_.pop_back ();
	__SUP_COUT__ << "Destructed." << __E__;
}
