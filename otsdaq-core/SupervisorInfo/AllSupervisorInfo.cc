#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"

#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/ConfigurationPluginDataFormats/XDAQContextConfiguration.h"


#include <iostream>



using namespace ots;



//========================================================================================================================
AllSupervisorInfo::AllSupervisorInfo(void)
: theSupervisorInfo_		(0)
, theWizardInfo_			(0)
{
}

//========================================================================================================================
AllSupervisorInfo::AllSupervisorInfo(xdaq::ApplicationContext* applicationContext)
: AllSupervisorInfo()
{
	init(applicationContext);
}

//========================================================================================================================
AllSupervisorInfo::~AllSupervisorInfo(void)
{
	destroy();
}

//========================================================================================================================
void AllSupervisorInfo::destroy(void)
{
	allSupervisorInfo_.clear();
	allFETypeSupervisorInfo_.clear();
	allDMTypeSupervisorInfo_.clear();

	theSupervisorInfo_ = 0;
	theWizardInfo_ = 0;

	SupervisorDescriptorInfoBase::destroy();
}

//========================================================================================================================
void AllSupervisorInfo::init(xdaq::ApplicationContext* applicationContext)
{
	AllSupervisorInfo::destroy();
	SupervisorDescriptorInfoBase::init(applicationContext);

	//ready.. loop through all descriptors, and organize

	ConfigurationManager cfgMgr;
	const XDAQContextConfiguration* contextConfig =
			cfgMgr.__GET_CONFIG__(XDAQContextConfiguration);

	auto allDescriptors = SupervisorDescriptorInfoBase::getAllDescriptors();
	for(const auto& descriptor:allDescriptors)
	{
		auto /*<iterator,bool>*/ emplacePair = allSupervisorInfo_.emplace(std::pair<unsigned int, SupervisorInfo>(
				descriptor.second->getLocalId(),//descriptor.first,
				SupervisorInfo(
						descriptor.second /* descriptor */,
						contextConfig->getApplicationUID
						(
								descriptor.second->getContextDescriptor()->getURL(),
								descriptor.second->getLocalId()
						) /* name */,
						contextConfig->getContextUID(
								descriptor.second->getContextDescriptor()->getURL()) /* xdaq parent context */
				)));
		if(!emplacePair.second)
		{
			__SS__ << "Error! Duplicate Application IDs are not allowed. ID =" <<
					descriptor.second->getLocalId() << __E__;
			__SS_THROW__;
		}

		/////////////////////////////////////////////
		// now organize new descriptor by class...

		//check for gateway supervisor
		// note: necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isGatewaySupervisor())
		{
			if(theSupervisorInfo_)
			{
				__SS__ << "Error! Multiple Gateway Supervisors of class " << XDAQContextConfiguration::GATEWAY_SUPERVISOR_CLASS <<
						" found. There can only be one. ID =" <<
						descriptor.second->getLocalId() << __E__;
				__SS_THROW__;
			}
			//copy and erase from map
			theSupervisorInfo_ = &(emplacePair.first->second);
			continue;
		}

		//check for wizard supervisor
		// note: necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isWizardSupervisor())
		{
			if(theWizardInfo_)
			{
				__SS__ << "Error! Multiple Wizard Supervisors of class " << XDAQContextConfiguration::WIZARD_SUPERVISOR_CLASS <<
						" found. There can only be one. ID =" <<
						descriptor.second->getLocalId() << __E__;
				__SS_THROW__;
			}
			//copy and erase from map
			theWizardInfo_ = &(emplacePair.first->second);
			continue;
		}


		//check for FE type, then add to FE group
		// note: not necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isTypeFESupervisor())
		{
			allFETypeSupervisorInfo_.emplace(std::pair<unsigned int, const SupervisorInfo&>(
					emplacePair.first->second.getId(),
					emplacePair.first->second));
		}

		//check for DM type, then add to DM group
		// note: not necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isTypeDMSupervisor())
		{
			allDMTypeSupervisorInfo_.emplace(std::pair<unsigned int, const SupervisorInfo&>(
					emplacePair.first->second.getId(),
					emplacePair.first->second));
		}

		//check for Logbook type, then add to Logbook group
		// note: not necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isTypeLogbookSupervisor())
		{
			allLogbookTypeSupervisorInfo_.emplace(std::pair<unsigned int, const SupervisorInfo&>(
					emplacePair.first->second.getId(),
					emplacePair.first->second));
		}

	} //end main extraction loop


	if((!theWizardInfo_ && !theSupervisorInfo_) ||
			(theWizardInfo_ && theSupervisorInfo_))
	{
		__SS__ << "Error! Must have one " << XDAQContextConfiguration::GATEWAY_SUPERVISOR_CLASS <<
				" OR one " << XDAQContextConfiguration::WIZARD_SUPERVISOR_CLASS <<
				" as part of the context configuration! " <<
				"Neither were found." << __E__;
		__SS_THROW__;
	}


	SupervisorDescriptorInfoBase::destroy();

	__COUT__ << "Init" << __E__;

	//for debugging
	//getOrderedSupervisorDescriptors("Configure");
}

//========================================================================================================================
const SupervisorInfo& AllSupervisorInfo::getSupervisorInfo(xdaq::Application* app) const
{
	auto it = allSupervisorInfo_.find(app->getApplicationDescriptor()->getLocalId());
	if(it == allSupervisorInfo_.end())
	{
		__SS__ << "Could not find: " << app->getApplicationDescriptor()->getLocalId() << std::endl;
		__SS_THROW__;
	}
	return it->second;
}

//========================================================================================================================
void AllSupervisorInfo::setSupervisorStatus(xdaq::Application* app,
		const std::string& status)
{
	setSupervisorStatus(app->getApplicationDescriptor()->getLocalId(), status);
}
//========================================================================================================================
void AllSupervisorInfo::setSupervisorStatus(const SupervisorInfo& appInfo,
		const std::string& status)
{
	setSupervisorStatus(appInfo.getId(), status);
}
//========================================================================================================================
void AllSupervisorInfo::setSupervisorStatus(const unsigned int& id,
		const std::string& status)
{
	auto it = allSupervisorInfo_.find(id);
	if(it == allSupervisorInfo_.end())
	{
		__SS__ << "Could not find: " << id << std::endl;
		__SS_THROW__;
	}
	it->second.setStatus(status);
}

//========================================================================================================================
const SupervisorInfo& AllSupervisorInfo::getGatewayInfo(void) const
{
	if(!theSupervisorInfo_)
	{
		__SS__ << "AllSupervisorInfo was not initialized or no Application of type " <<
				XDAQContextConfiguration::GATEWAY_SUPERVISOR_CLASS << " found!" << __E__;
		__SS_THROW__;
	}
	return *theSupervisorInfo_;
}
//========================================================================================================================
XDAQ_CONST_CALL xdaq::ApplicationDescriptor* AllSupervisorInfo::getGatewayDescriptor(void) const
{
	return getGatewayInfo().getDescriptor();
}

//========================================================================================================================
const SupervisorInfo& AllSupervisorInfo::getWizardInfo(void) const
{
	if(!theWizardInfo_)
	{
		__SS__ << "AllSupervisorInfo was not initialized or no Application of type " <<
				XDAQContextConfiguration::WIZARD_SUPERVISOR_CLASS << "  found!" << __E__;
		__SS_THROW__;
	}
	return *theWizardInfo_;
}
//========================================================================================================================
XDAQ_CONST_CALL xdaq::ApplicationDescriptor* AllSupervisorInfo::getWizardDescriptor(void) const
{
	return getWizardInfo().getDescriptor();
}


//========================================================================================================================
std::vector<const SupervisorInfo*> AllSupervisorInfo::getOrderedSupervisorDescriptors(
		const std::string& stateMachineCommand) const
{
	__COUT__ << "getOrderedSupervisorDescriptors" << __E__;

	std::map<uint8_t      /*priority*/, std::vector< unsigned int /*appId*/> > orderedByPriority;

	try
	{
		ConfigurationManager cfgMgr;
		const std::vector<XDAQContextConfiguration::XDAQContext>& contexts =
				cfgMgr.__GET_CONFIG__(XDAQContextConfiguration)->getContexts();

		for (const auto& context : contexts)
			if(context.status_)
				for (const auto& app : context.applications_)
				{
					if(!app.status_) continue; //skip disabled apps

					auto it = app.stateMachineCommandPriority_.find(stateMachineCommand);
					if(it == app.stateMachineCommandPriority_.end())
						orderedByPriority[100].push_back(app.id_);
					else
						orderedByPriority[it->second?it->second:100].push_back(app.id_);

					//__COUT__ << "app.id_ " << app.id_ << __E__;
				}
	}
	catch(...)
	{
		__COUT_ERR__ << "SupervisorDescriptorInfoBase could not access the XDAQ Context and Application configuration through the Configuration Context Group." << __E__;
		throw;
	}


	__COUT__ << "Here is the order supervisors will be " << stateMachineCommand << "'d:" << __E__;


	//return ordered set of supervisor infos
	//	skip over Gateway Supervisor
	std::vector<const SupervisorInfo*> retVec;
	for (const auto& priorityAppVector : orderedByPriority)
		for (const auto& priorityApp : priorityAppVector.second)
	{
		auto it = allSupervisorInfo_.find(priorityApp);
		if(it == allSupervisorInfo_.end())
		{
			__SS__ << "Error! Was AllSupervisorInfo properly initialized? The app.id_ " << priorityApp << " priority " <<
							(unsigned int)priorityAppVector.first << " could not be found in AllSupervisorInfo." << __E__;
			__SS_THROW__;
		}

		//__COUT__ << it->second.getName() << " [" << it->second.getId() << "]: " << " priority? " <<
		//				(unsigned int)priorityAppVector.first << __E__;

		if(it->second.isGatewaySupervisor()) continue; //skip gateway supervisor
		if(it->second.isTypeLogbookSupervisor()) continue; //skip logbook supervisor(s)
		if(it->second.isTypeMacroMakerSupervisor()) continue; //skip macromaker supervisor(s)
		if(it->second.isTypeConfigurationGUISupervisor()) continue; //skip configurationGUI supervisor(s)
		if(it->second.isTypeChatSupervisor()) continue; //skip chat supervisor(s)
		if(it->second.isTypeConsoleSupervisor()) continue; //skip console supervisor(s)

		retVec.push_back(&(it->second));
		__COUT__ << it->second.getName() << " [LID=" << it->second.getId() << "]: " << " priority " <<
				(unsigned int)priorityAppVector.first << __E__;
	}
	return retVec;
}










