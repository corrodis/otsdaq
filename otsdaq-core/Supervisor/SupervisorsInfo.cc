#include "otsdaq-core/Supervisor/SupervisorsInfo.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/SupervisorDescriptorInfo/SupervisorDescriptorInfoBase.h"

#include <iostream>


using namespace ots;

//========================================================================================================================
SupervisorsInfo::SupervisorsInfo(void)
{
}

//========================================================================================================================
SupervisorsInfo::~SupervisorsInfo(void)
{
}

//========================================================================================================================
void SupervisorsInfo::init(const SupervisorDescriptorInfoBase& supervisorDescriptorInfo)
{
	SupervisorDescriptors::const_iterator it;
	it = supervisorDescriptorInfo.getFEDescriptors().begin();
	for(; it!=supervisorDescriptorInfo.getFEDescriptors().end(); it++)
		theFESupervisorsInfo_[it->first] = SupervisorInfo();

//	it = supervisorsConfiguration.getARTDAQFEDescriptors().begin();
//	for(; it!=supervisorsConfiguration.getARTDAQFEDescriptors().end(); it++)
//		theARTDAQFESupervisorsInfo_[it->first] = SupervisorInfo();
	for(auto it : supervisorDescriptorInfo.getARTDAQFEDataManagerDescriptors())
		theARTDAQFEDataManagerSupervisorsInfo_[it.first] = SupervisorInfo();

	for(auto it : supervisorDescriptorInfo.getARTDAQDataManagerDescriptors())
		theARTDAQDataManagerSupervisorsInfo_[it.first] = SupervisorInfo();
}

//========================================================================================================================
SupervisorInfo& SupervisorsInfo::getSupervisorInfo(void)
{
	return theSupervisorInfo_;
}

//========================================================================================================================
SupervisorInfo& SupervisorsInfo::getFESupervisorInfo(xdata::UnsignedIntegerT instance)
{
	if(theFESupervisorsInfo_.find(instance) == theFESupervisorsInfo_.end())
		std::cout << __COUT_HDR_FL__ << "Couldn't find: " << instance << std::endl;
	return theFESupervisorsInfo_.find(instance)->second;
}

//========================================================================================================================
SupervisorInfo& SupervisorsInfo::getARTDAQFEDataManagerSupervisorInfo(xdata::UnsignedIntegerT instance)
{
	if(theARTDAQFEDataManagerSupervisorsInfo_.find(instance) == theARTDAQFEDataManagerSupervisorsInfo_.end())
		std::cout << __COUT_HDR_FL__ << "Couldn't find: " << instance << std::endl;
	return theARTDAQFEDataManagerSupervisorsInfo_.find(instance)->second;
}
//========================================================================================================================
SupervisorInfo& SupervisorsInfo::getARTDAQDataManagerSupervisorInfo(xdata::UnsignedIntegerT instance)
{
	if(theARTDAQDataManagerSupervisorsInfo_.find(instance) == theARTDAQDataManagerSupervisorsInfo_.end())
		std::cout << __COUT_HDR_FL__ << "Couldn't find: " << instance << std::endl;
	return theARTDAQDataManagerSupervisorsInfo_.find(instance)->second;
}
//========================================================================================================================
SupervisorInfo& SupervisorsInfo::getVisualSupervisorInfo(void)
{
	return theVisualSupervisorInfo_;
}

