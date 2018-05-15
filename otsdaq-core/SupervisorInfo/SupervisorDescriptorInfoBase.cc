#include "otsdaq-core/SupervisorInfo/SupervisorDescriptorInfoBase.h"

#include <xdaq/ContextDescriptor.h>
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include <cassert>

using namespace ots;



//========================================================================================================================
SupervisorDescriptorInfoBase::SupervisorDescriptorInfoBase(void)
{
}

//========================================================================================================================
SupervisorDescriptorInfoBase::SupervisorDescriptorInfoBase(xdaq::ApplicationContext* applicationContext)
{
	init(applicationContext);
}

//========================================================================================================================
SupervisorDescriptorInfoBase::~SupervisorDescriptorInfoBase()
{}

//========================================================================================================================
void SupervisorDescriptorInfoBase::destroy()
{
	allSupervisors_.clear();
}

//========================================================================================================================
void SupervisorDescriptorInfoBase::init(xdaq::ApplicationContext* applicationContext)
{
	if(applicationContext->getDefaultZone()->getApplicationGroup("daq") == 0)
	{
		__SS__ << "Could not find xdaq application group \"daq\" (Must not be present in the xdaq context configuration)" << __E__;
		__SS_THROW__;
	}

	__COUT__ << "Init" << __E__;

//	//There is only one and only Supervisor! (Or Config Wizard!!)
//	theSupervisor_ = *(applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::Supervisor").begin());
//	if(0)//theSupervisor_ == 0)
//		__COUT__ << "Note: Could not find xdaq application descriptor \"ots::Supervisor\" (Must not be present in the xdaq context configuration)" << __E__;
//
//	theWizard_ = *(applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::OtsConfigurationWizardSupervisor").begin());
//	if(0)//theWizard_ == 0)
//		__COUT__ << "Note: Could not find xdaq application descriptor \"ots::OtsConfigurationWizardSupervisor\" (Must not be present in the xdaq context configuration)" << __E__;
//
//	if(theWizard_ == 0 && theSupervisor_ == 0)
//	{
//		__SS__ << "Must have THE ots::Supervisor (or THE ots::OtsConfigurationWizardSupervisor) as part of the context configuration!" << __E__;
//		__COUT_ERR__ << "\n" << ss.str();
//		throw std::runtime_error(ss.str());
//	}

	std::set<XDAQ_CONST_CALL xdaq::ApplicationDescriptor*> appDescriptors;

	//get allSupervisors_

	//allFETypeSupervisors_.clear();
	allSupervisors_.clear();
	appDescriptors = applicationContext->getDefaultZone()->getApplicationGroup(
			"daq")->getApplicationDescriptors();
	for(auto& it: appDescriptors)
	{
		auto /*<it,bool*/ retPair = allSupervisors_.emplace(std::pair
				<xdata::UnsignedIntegerT, XDAQ_CONST_CALL xdaq::ApplicationDescriptor*>(
				it->getLocalId(),
				it));
		if(!retPair.second)
		{
			__SS__ << "Error! Duplicate Application IDs are not allowed. ID =" <<
					it->getLocalId() << __E__;
			__SS_THROW__;
		}
	}

//
//	theLogbookSupervisor_ = *(applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::LogbookSupervisor").begin());
//	if(theWizard_ == 0 && theLogbookSupervisor_ == 0)
//		__COUT__ << "Note: Could not find xdaq application descriptor \"ots::LogbookSupervisor\" (Must not be present in the xdaq context configuration)" << __E__;

//	theDataManagerSupervisors_.clear();
//	appDescriptors = applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::DataManagerSupervisor");
//	for(auto& it: appDescriptors)
//		theDataManagerSupervisors_[it->getLocalId()] = it;
//	if(theWizard_ == 0 && theDataManagerSupervisors_.size() == 0)
//		__COUT__ << "Note: Could not find any xdaq application descriptor \"ots::DataManagerSupervisor\" (Must not be present in the xdaq context configuration)" << __E__;


//	//theFESupervisors_.clear();
//	appDescriptors = applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::FESupervisor");
//	for(auto& it: appDescriptors)
//	{
//		//theFESupervisors_[it->getLocalId()] = it;
//		allFETypeSupervisors_[it->getLocalId()] = it;
//	}

//	if(theWizard_ == 0 && theFESupervisors_.size() == 0)
	//	__COUT__ << "Note: Could not find any xdaq application descriptor \"ots::FESupervisor\" (Must not be present in the xdaq context configuration)" << __E__;

	//RAR: no need for this theDTCSupervisors_.. state machine should use 'all supervisors'
	//    theDTCSupervisors_.clear();
	//    appDescriptors = applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("Ph2TkDAQ::DTCSupervisor");
	//    for(auto& it: appDescriptors)
	//    	theDTCSupervisors_[it->getLocalId()] = it;
	////
	//    if(theDTCSupervisors_.size() == 0)
	//    {
	//        __COUT__ << "Note: Could not find any xdaq application descriptor \"ots::DTCSupervisor\" (Must not be present in the xdaq context configuration)" << __E__;
	//        //assert(0);
	//    }

	//theFEDataManagerSupervisors_.clear();
//	appDescriptors = applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::FEDataManagerSupervisor");
//	for(auto& it: appDescriptors)
//	{
//		//theFEDataManagerSupervisors_[it->getLocalId()] = it;
//		allFETypeSupervisors_[it->getLocalId()] = it;
//	}
//	if(theWizard_ == 0 && theFEDataManagerSupervisors_.size() == 0)
	//	__COUT__ << "Note: Could not find any xdaq application descriptor \"ots::FEDataManagerSupervisor\" (Must not be present in the xdaq context configuration)" << __E__;


	//    theARTDAQFESupervisors_.clear();
	//    for(auto& it: applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::ARTDAQFESupervisor"))
	//    	theARTDAQFESupervisors_[it->getLocalId()] = it;
	//    if(theARTDAQFESupervisors_.size() == 0)
	//    {
	//        __COUT__ << "Note: Could not find xdaq application descriptor \"ots::ARTDAQFESupervisor\" (Must not be present in the xdaq context configuration)" << __E__;
	//        //assert(0);
	//    }
//
//	theARTDAQFEDataManagerSupervisors_.clear();
//			for(auto& it: applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::ARTDAQFEDataManagerSupervisor"))
//			{
//				//theARTDAQFEDataManagerSupervisors_[it->getLocalId()] = it;
//				allFETypeSupervisors_[it->getLocalId()] = it;
//			}
//	if(0)//theARTDAQFEDataManagerSupervisors_.size() == 0)
	//	__COUT__ << "Note: Could not find any xdaq application descriptor \"ots::ARTDAQFEDataManagerSupervisor\" (Must not be present in the xdaq context configuration)" << __E__;
//
//	theARTDAQDataManagerSupervisors_.clear();
//	for(auto& it: applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::ARTDAQDataManagerSupervisor"))
//		theARTDAQDataManagerSupervisors_[it->getLocalId()] = it;
//	if(0)//theARTDAQDataManagerSupervisors_.size() == 0)
//		__COUT__ << "Note: Could not find any xdaq application descriptor \"ots::ARTDAQDataManagerSupervisor\" (Must not be present in the xdaq context configuration)" << __E__;
//
//
//	theARTDAQBuilderSupervisors_.clear();
//	for(auto& it: applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::EventBuilderApp"))
//		theARTDAQBuilderSupervisors_[it->getLocalId()] = it;
//	if(0)//theARTDAQBuilderSupervisors_.size() == 0)
//		__COUT__ << "Note: Could not find any xdaq application descriptor \"ots::EventBuilderApp\" (Must not be present in the xdaq context configuration)" << __E__;
//
//
//	theARTDAQAggregatorSupervisors_.clear();
//	for(auto& it: applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::AggregatorApp"))
//		theARTDAQAggregatorSupervisors_[it->getLocalId()] = it;
//	if(0)//theARTDAQAggregatorSupervisors_.size() == 0)
//		__COUT__ << "Note: Could not find any xdaq application descriptor \"ots::AggregatorApp\" (Must not be present in the xdaq context configuration)" << __E__;
//
//
//	theVisualSupervisors_.clear();
//	for(auto& it: applicationContext->getDefaultZone()->getApplicationGroup("daq")->getApplicationDescriptors("ots::VisualSupervisor"))
//		theVisualSupervisors_[it->getLocalId()] = it;
//	if(0)//theVisualSupervisors_.size() == 0)
//		__COUT__ << "Note: Could not find any xdaq application descriptor \"ots::VisualSupervisor\" (Must not be present in the xdaq context configuration)" << __E__;

}

//
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getDataManagerDescriptors(void) const
//{
//	return theDataManagerSupervisors_;
//}
//
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getFEDescriptors(void) const
//{
//	return theFESupervisors_;
//}
//
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getDTCDescriptors(void) const
//{
//	return theDTCSupervisors_;
//}
//
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getFEDataManagerDescriptors(void) const
//{
//	return theFEDataManagerSupervisors_;
//}

//========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getARTDAQFEDescriptors(void) const
//{
//    return theARTDAQFESupervisors_;
//}
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getARTDAQFEDataManagerDescriptors(void) const
//{
//	return theARTDAQFEDataManagerSupervisors_;
//}
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getARTDAQDataManagerDescriptors  (void) const
//{
//	return theARTDAQDataManagerSupervisors_;
//}
//
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getARTDAQBuilderDescriptors(void) const
//{
//	return theARTDAQBuilderSupervisors_;
//}
//
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getARTDAQAggregatorDescriptors(void) const
//{
//	return theARTDAQAggregatorSupervisors_;
//}
//
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getVisualDescriptors(void) const
//{
//	return theVisualSupervisors_;
//}


//========================================================================================================================
const SupervisorDescriptors& SupervisorDescriptorInfoBase::getAllDescriptors(void) const
{
	return allSupervisors_;
}
//
////========================================================================================================================
//const SupervisorDescriptors& SupervisorDescriptorInfoBase::getAllFEDescriptors(void) const
//{
//	return allFETypeSupervisors_;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getSupervisorDescriptor(void) const
//{
//	return theSupervisor_;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getWizardDescriptor(void) const
//{
//	return theWizard_;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getLogbookDescriptor(void) const
//{
//	return theLogbookSupervisor_;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getVisualDescriptor(xdata::UnsignedIntegerT instance) const
//{
//	if(theVisualSupervisors_.find(instance) == theVisualSupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//	return theVisualSupervisors_.find(instance)->second;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getDataManagerDescriptor(xdata::UnsignedIntegerT instance) const
//{
//	if(theDataManagerSupervisors_.find(instance) == theDataManagerSupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//	return  theDataManagerSupervisors_.find(instance)->second;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getFEDescriptor(xdata::UnsignedIntegerT instance) const
//{
//	if(theFESupervisors_.find(instance) == theFESupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//	return  theFESupervisors_.find(instance)->second;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getDTCDescriptor(xdata::UnsignedIntegerT instance) const
//{
//	if(theDTCSupervisors_.find(instance) == theDTCSupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//	return  theDTCSupervisors_.find(instance)->second;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getFEDataManagerDescriptor(xdata::UnsignedIntegerT instance) const
//{
//	if(theFEDataManagerSupervisors_.find(instance) == theFEDataManagerSupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//	return  theFEDataManagerSupervisors_.find(instance)->second;
//}

//========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getARTDAQFEDescriptor(xdata::UnsignedIntegerT instance) const
//{
//	if(theARTDAQFESupervisors_.find(instance) == theARTDAQFESupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//    return  theARTDAQFESupervisors_.find(instance)->second;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getARTDAQFEDataManagerDescriptor(xdata::UnsignedIntegerT instance) const
//{
//	if(theARTDAQFEDataManagerSupervisors_.find(instance) == theARTDAQFEDataManagerSupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//	return  theARTDAQFEDataManagerSupervisors_.find(instance)->second;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getARTDAQDataManagerDescriptor  (xdata::UnsignedIntegerT instance) const
//{
//	if(theARTDAQDataManagerSupervisors_.find(instance) == theARTDAQDataManagerSupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//	return  theARTDAQDataManagerSupervisors_.find(instance)->second;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getARTDAQBuilderDescriptor(xdata::UnsignedIntegerT instance) const
//{
//	if(theARTDAQBuilderSupervisors_.find(instance) == theARTDAQBuilderSupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//	return  theARTDAQBuilderSupervisors_.find(instance)->second;
//}
//
////========================================================================================================================
//XDAQ_CONST_CALL xdaq::ApplicationDescriptor* SupervisorDescriptorInfoBase::getARTDAQAggregatorDescriptor(xdata::UnsignedIntegerT instance) const
//{
//	if(theARTDAQAggregatorSupervisors_.find(instance) == theARTDAQAggregatorSupervisors_.end())
//		__COUT__ << "Could not find: " << instance << __E__;
//	return  theARTDAQAggregatorSupervisors_.find(instance)->second;
//}
//
////========================================================================================================================
//std::string SupervisorDescriptorInfoBase::getFEURL(xdata::UnsignedIntegerT instance) const
//{
//	if(allFETypeSupervisors_.find(instance) == allFETypeSupervisors_.end())
//			__COUT__ << "Could not find: " << instance << __E__;
//	return  allFETypeSupervisors_.find(instance)->second->getContextDescriptor()->getURL();
//	//getFEDescriptor(instance)->getContextDescriptor()->getURL();
//}

/*
//========================================================================================================================
std::string SupervisorDescriptorInfoBase::getARTDAQFEURL(xdata::UnsignedIntegerT instance) const
{
    return  getARTDAQFEDescriptor(instance)->getContextDescriptor()->getURL();
}

//========================================================================================================================
std::string SupervisorDescriptorInfoBase::getARTDAQBuilderURL(xdata::UnsignedIntegerT instance) const
{
    return  getARTDAQBuilderDescriptor(instance)->getContextDescriptor()->getURL();
}

//========================================================================================================================
std::string SupervisorDescriptorInfoBase::getARTDAQAggregatorURL(xdata::UnsignedIntegerT instance) const
{
    return  getARTDAQAggregatorDescriptor(instance)->getContextDescriptor()->getURL();
}
 */
