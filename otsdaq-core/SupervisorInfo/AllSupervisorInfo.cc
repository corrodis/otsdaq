#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"

#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/TablePluginDataFormats/XDAQContextTable.h"

#include <iostream>

using namespace ots;

//========================================================================================================================
AllSupervisorInfo::AllSupervisorInfo(void) : theSupervisorInfo_(0), theWizardInfo_(0) {}

//========================================================================================================================
AllSupervisorInfo::AllSupervisorInfo(xdaq::ApplicationContext* applicationContext)
    : AllSupervisorInfo()
{
	init(applicationContext);
}

//========================================================================================================================
AllSupervisorInfo::~AllSupervisorInfo(void) { destroy(); }

//========================================================================================================================
void AllSupervisorInfo::destroy(void)
{
	allSupervisorInfo_.clear();
	allFETypeSupervisorInfo_.clear();
	allDMTypeSupervisorInfo_.clear();

	theSupervisorInfo_ = 0;
	theWizardInfo_     = 0;

	SupervisorDescriptorInfoBase::destroy();
}

//========================================================================================================================
void AllSupervisorInfo::init(xdaq::ApplicationContext* applicationContext)
{
	__COUT__ << "Initializing info based on XDAQ context..." << __E__;

	AllSupervisorInfo::destroy();
	SupervisorDescriptorInfoBase::init(applicationContext);

	auto allDescriptors = SupervisorDescriptorInfoBase::getAllDescriptors();
	// ready.. loop through all descriptors, and organize

	//	for(const auto& descriptor:allDescriptors)
	//	{
	//		SupervisorInfo tempSupervisorInfo(
	//						descriptor.second /* descriptor */,
	//						"" /* config app name */,"" /* config parent context name */
	////skip  configuration info
	//						);
	//
	//		__COUT__ << "id " << descriptor.second->getLocalId() << " url " <<
	// descriptor.second->getContextDescriptor()->getURL() << __E__;
	//
	//	}
	//	__COUTV__(XDAQContextTable::GATEWAY_SUPERVISOR_CLASS);

	// Steps:
	//	1. first pass, identify Wiz mode or not
	//	2. second pass, organize supervisors

	bool isWizardMode = false;

	// first pass, identify Wiz mode or not
	//	accept first encountered (wizard or gateway) as the mode
	for(const auto& descriptor : allDescriptors)
	{
		SupervisorInfo tempSupervisorInfo(
		    descriptor.second /* descriptor */,
		    "" /* config app name */,
		    "" /* config parent context name */  // skip configuration info
		);

		// check for gateway supervisor
		if(tempSupervisorInfo.isGatewaySupervisor())
		{
			// found normal mode, done with first pass
			isWizardMode = false;
			break;
		}
		else if(tempSupervisorInfo.isWizardSupervisor())
		{
			// found wiz mode, done with first pass
			isWizardMode = true;
			break;
		}
	}

	if(isWizardMode)
		__COUT__ << "Initializing info for Wiz mode XDAQ context..." << __E__;
	else
		__COUT__ << "Initializing info for Normal mode XDAQ context..." << __E__;
	std::unique_ptr<ConfigurationManager> cfgMgr(
	    isWizardMode ? 0 : new ConfigurationManager());
	const XDAQContextTable* contextConfig =
	    isWizardMode ? 0 : cfgMgr->__GET_CONFIG__(XDAQContextTable);

	// do not involve the Configuration Manager
	//	as it adds no valid information to the supervisors
	//	present in wiz mode
	for(const auto& descriptor : allDescriptors)
	{
		auto /*<iterator,bool>*/ emplacePair =
		    allSupervisorInfo_.emplace(std::pair<unsigned int, SupervisorInfo>(
		        descriptor.second->getLocalId(),  // descriptor.first,
		        SupervisorInfo(
		            descriptor.second /* descriptor */,
		            contextConfig
		                ? contextConfig->getApplicationUID(
		                      descriptor.second->getContextDescriptor()->getURL(),
		                      descriptor.second->getLocalId())
		                : "" /* config app name */,
		            contextConfig
		                ? contextConfig->getContextUID(
		                      descriptor.second->getContextDescriptor()->getURL())
		                : "" /* config parent context name */
		            )));
		if(!emplacePair.second)
		{
			__SS__ << "Error! Duplicate Application IDs are not allowed. ID ="
			       << descriptor.second->getLocalId() << __E__;
			__SS_THROW__;
		}

		/////////////////////////////////////////////
		// now organize new descriptor by class...

		// check for gateway supervisor
		// note: necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isGatewaySupervisor())
		{
			if(theSupervisorInfo_)
			{
				__SS__ << "Error! Multiple Gateway Supervisors of class "
				       << XDAQContextTable::GATEWAY_SUPERVISOR_CLASS
				       << " found. There can only be one. ID ="
				       << descriptor.second->getLocalId() << __E__;
				__SS_THROW__;
			}
			// copy and erase from map
			theSupervisorInfo_ = &(emplacePair.first->second);
			continue;
		}

		// check for wizard supervisor
		// note: necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isWizardSupervisor())
		{
			if(theWizardInfo_)
			{
				__SS__ << "Error! Multiple Wizard Supervisors of class "
				       << XDAQContextTable::WIZARD_SUPERVISOR_CLASS
				       << " found. There can only be one. ID ="
				       << descriptor.second->getLocalId() << __E__;
				__SS_THROW__;
			}
			// copy and erase from map
			theWizardInfo_ = &(emplacePair.first->second);
			continue;
		}

		// check for FE type, then add to FE group
		// note: not necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isTypeFESupervisor())
		{
			allFETypeSupervisorInfo_.emplace(
			    std::pair<unsigned int, const SupervisorInfo&>(
			        emplacePair.first->second.getId(), emplacePair.first->second));
		}

		// check for DM type, then add to DM group
		// note: not necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isTypeDMSupervisor())
		{
			allDMTypeSupervisorInfo_.emplace(
			    std::pair<unsigned int, const SupervisorInfo&>(
			        emplacePair.first->second.getId(), emplacePair.first->second));
		}

		// check for Logbook type, then add to Logbook group
		// note: not necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isTypeLogbookSupervisor())
		{
			allLogbookTypeSupervisorInfo_.emplace(
			    std::pair<unsigned int, const SupervisorInfo&>(
			        emplacePair.first->second.getId(), emplacePair.first->second));
		}

		// check for MacroMaker type, then add to MacroMaker group
		// note: not necessarily exclusive to other Supervisor types
		if(emplacePair.first->second.isTypeMacroMakerSupervisor())
		{
			allMacroMakerTypeSupervisorInfo_.emplace(
			    std::pair<unsigned int, const SupervisorInfo&>(
			        emplacePair.first->second.getId(), emplacePair.first->second));
		}

	}  // end main extraction loop

	//} //end wiz mode extraction
	// else
	//	{
	//		__COUT__ << "Initializing info for Normal mode XDAQ context..." << __E__;
	//
	//
	//		ConfigurationManager cfgMgr;
	//		const XDAQContextTable* contextConfig =
	//				cfgMgr.__GET_CONFIG__(XDAQContextTable);
	//
	//
	//		//second pass, organize supervisors
	//		auto allDescriptors = SupervisorDescriptorInfoBase::getAllDescriptors();
	//		for(const auto& descriptor:allDescriptors)
	//		{
	//			auto /*<iterator,bool>*/ emplacePair =
	// allSupervisorInfo_.emplace(std::pair<unsigned int, SupervisorInfo>(
	//					descriptor.second->getLocalId(),//descriptor.first,
	//					SupervisorInfo(
	//							descriptor.second /* descriptor */,
	//							contextConfig->getApplicationUID
	//							(
	//									descriptor.second->getContextDescriptor()->getURL(),
	//									descriptor.second->getLocalId()
	//							) /* name */,
	//							contextConfig->getContextUID(
	//									descriptor.second->getContextDescriptor()->getURL())
	///*  xdaq  parent context */
	//					)));
	//			if(!emplacePair.second)
	//			{
	//				__SS__ << "Error! Duplicate Application IDs are not allowed. ID =" <<
	//						descriptor.second->getLocalId() << __E__;
	//				__SS_THROW__;
	//			}
	//
	//			/////////////////////////////////////////////
	//			// now organize new descriptor by class...
	//
	//			//check for gateway supervisor
	//			// note: necessarily exclusive to other Supervisor types
	//			if(emplacePair.first->second.isGatewaySupervisor())
	//			{
	//				if(theSupervisorInfo_)
	//				{
	//					__SS__ << "Error! Multiple Gateway Supervisors of class " <<
	// XDAQContextTable::GATEWAY_SUPERVISOR_CLASS << 							"
	// found. There can only be one. ID =" <<
	//							descriptor.second->getLocalId() << __E__;
	//					__SS_THROW__;
	//				}
	//				//copy and erase from map
	//				theSupervisorInfo_ = &(emplacePair.first->second);
	//				continue;
	//			}
	//
	//			//check for wizard supervisor
	//			// note: necessarily exclusive to other Supervisor types
	//			if(emplacePair.first->second.isWizardSupervisor())
	//			{
	//				if(theWizardInfo_)
	//				{
	//					__SS__ << "Error! Multiple Wizard Supervisors of class " <<
	// XDAQContextTable::WIZARD_SUPERVISOR_CLASS <<
	//							" found. There can only be one. ID =" <<
	//							descriptor.second->getLocalId() << __E__;
	//					__SS_THROW__;
	//				}
	//				//copy and erase from map
	//				theWizardInfo_ = &(emplacePair.first->second);
	//				continue;
	//			}
	//
	//
	//			//check for FE type, then add to FE group
	//			// note: not necessarily exclusive to other Supervisor types
	//			if(emplacePair.first->second.isTypeFESupervisor())
	//			{
	//				allFETypeSupervisorInfo_.emplace(std::pair<unsigned int, const
	// SupervisorInfo&>( 						emplacePair.first->second.getId(),
	//						emplacePair.first->second));
	//			}
	//
	//			//check for DM type, then add to DM group
	//			// note: not necessarily exclusive to other Supervisor types
	//			if(emplacePair.first->second.isTypeDMSupervisor())
	//			{
	//				allDMTypeSupervisorInfo_.emplace(std::pair<unsigned int, const
	// SupervisorInfo&>( 						emplacePair.first->second.getId(),
	//						emplacePair.first->second));
	//			}
	//
	//			//check for Logbook type, then add to Logbook group
	//			// note: not necessarily exclusive to other Supervisor types
	//			if(emplacePair.first->second.isTypeLogbookSupervisor())
	//			{
	//				allLogbookTypeSupervisorInfo_.emplace(std::pair<unsigned int, const
	// SupervisorInfo&>( 						emplacePair.first->second.getId(),
	//						emplacePair.first->second));
	//			}
	//
	//			//check for MacroMaker type, then add to MacroMaker group
	//			// note: not necessarily exclusive to other Supervisor types
	//			if(emplacePair.first->second.isTypeMacroMakerSupervisor())
	//			{
	//				allMacroMakerTypeSupervisorInfo_.emplace(std::pair<unsigned int, const
	// SupervisorInfo&>( 						emplacePair.first->second.getId(),
	//						emplacePair.first->second));
	//			}
	//
	//		} //end main extraction loop
	//	} //end normal mode extraction

	if((!theWizardInfo_ && !theSupervisorInfo_) || (theWizardInfo_ && theSupervisorInfo_))
	{
		__SS__ << "Error! Must have one "
		       << XDAQContextTable::GATEWAY_SUPERVISOR_CLASS << " OR one "
		       << XDAQContextTable::WIZARD_SUPERVISOR_CLASS
		       << " as part of the context configuration! "
		       << "Neither were found." << __E__;
		__SS_THROW__;
	}

	SupervisorDescriptorInfoBase::destroy();

	__COUT__ << "Init complete" << __E__;

	// for debugging
	// getOrderedSupervisorDescriptors("Configure");
}

//========================================================================================================================
const SupervisorInfo& AllSupervisorInfo::getSupervisorInfo(xdaq::Application* app) const
{
	auto it = allSupervisorInfo_.find(app->getApplicationDescriptor()->getLocalId());
	if(it == allSupervisorInfo_.end())
	{
		__SS__ << "Could not find: " << app->getApplicationDescriptor()->getLocalId()
		       << std::endl;
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
                                            const std::string&    status)
{
	setSupervisorStatus(appInfo.getId(), status);
}
//========================================================================================================================
void AllSupervisorInfo::setSupervisorStatus(const unsigned int& id,
                                            const std::string&  status)
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
		__SS__ << "AllSupervisorInfo was not initialized or no Application of type "
		       << XDAQContextTable::GATEWAY_SUPERVISOR_CLASS << " found!"
		       << __E__;
		__SS_THROW__;
	}
	return *theSupervisorInfo_;
}
//========================================================================================================================
XDAQ_CONST_CALL xdaq::ApplicationDescriptor* AllSupervisorInfo::getGatewayDescriptor(
    void) const
{
	return getGatewayInfo().getDescriptor();
}

//========================================================================================================================
const SupervisorInfo& AllSupervisorInfo::getWizardInfo(void) const
{
	if(!theWizardInfo_)
	{
		__SS__ << "AllSupervisorInfo was not initialized or no Application of type "
		       << XDAQContextTable::WIZARD_SUPERVISOR_CLASS << "  found!"
		       << __E__;
		__SS_THROW__;
	}
	return *theWizardInfo_;
}
//========================================================================================================================
XDAQ_CONST_CALL xdaq::ApplicationDescriptor* AllSupervisorInfo::getWizardDescriptor(
    void) const
{
	return getWizardInfo().getDescriptor();
}

//========================================================================================================================
std::vector<std::vector<const SupervisorInfo*>>
AllSupervisorInfo::getOrderedSupervisorDescriptors(
    const std::string& stateMachineCommand) const
{
	__COUT__ << "getOrderedSupervisorDescriptors" << __E__;

	std::map<uint64_t /*priority*/, std::vector<unsigned int /*appId*/>>
	    orderedByPriority;

	try
	{
		ConfigurationManager                                      cfgMgr;
		const std::vector<XDAQContextTable::XDAQContext>& contexts =
		    cfgMgr.__GET_CONFIG__(XDAQContextTable)->getContexts();

		for(const auto& context : contexts)
			if(context.status_)
				for(const auto& app : context.applications_)
				{
					if(!app.status_)
						continue;  // skip disabled apps

					auto it = app.stateMachineCommandPriority_.find(stateMachineCommand);
					if(it == app.stateMachineCommandPriority_.end())
						orderedByPriority[XDAQContextTable::XDAQApplication::
						                      DEFAULT_PRIORITY]
						    .push_back(
						        app.id_);  // if no priority, then default to
						                   // XDAQContextTable::XDAQApplication::DEFAULT_PRIORITY
					else  // take value, and do not allow DEFAULT value of 0 -> force to
					      // XDAQContextTable::XDAQApplication::DEFAULT_PRIORITY
						orderedByPriority[it->second
						                      ? it->second
						                      : XDAQContextTable::
						                            XDAQApplication::DEFAULT_PRIORITY]
						    .push_back(app.id_);

					//__COUT__ << "app.id_ " << app.id_ << __E__;
				}
	}
	catch(...)
	{
		__COUT_ERR__ << "SupervisorDescriptorInfoBase could not access the XDAQ Context "
		                "and Application configuration through the Configuration Context "
		                "Group."
		             << __E__;
		throw;
	}

	__COUT__ << "Here is the order supervisors will be " << stateMachineCommand
	         << "'d:" << __E__;

	// return ordered set of supervisor infos
	//	skip over Gateway Supervisor,
	//	and other supervisors that do not need state transitions.
	std::vector<std::vector<const SupervisorInfo*>> retVec;
	bool                                            createContainer;
	for(const auto& priorityAppVector : orderedByPriority)
	{
		createContainer = true;

		for(const auto& priorityApp : priorityAppVector.second)
		{
			auto it = allSupervisorInfo_.find(priorityApp);
			if(it == allSupervisorInfo_.end())
			{
				__SS__
				    << "Error! Was AllSupervisorInfo properly initialized? The app.id_ "
				    << priorityApp << " priority "
				    << (unsigned int)priorityAppVector.first
				    << " could not be found in AllSupervisorInfo." << __E__;
				__SS_THROW__;
			}

			//__COUT__ << it->second.getName() << " [" << it->second.getId() << "]: " << "
			// priority? " << 				(unsigned int)priorityAppVector.first <<
			//__E__;

			if(it->second.isGatewaySupervisor())
				continue;  // skip gateway supervisor
			if(it->second.isTypeLogbookSupervisor())
				continue;  // skip logbook supervisor(s)
			if(it->second.isTypeMacroMakerSupervisor())
				continue;  // skip macromaker supervisor(s)
			if(it->second.isTypeConfigurationGUISupervisor())
				continue;  // skip configurationGUI supervisor(s)
			if(it->second.isTypeChatSupervisor())
				continue;  // skip chat supervisor(s)
			if(it->second.isTypeConsoleSupervisor())
				continue;  // skip console supervisor(s)

			if(createContainer)  // create container first time
			{
				retVec.push_back(std::vector<const SupervisorInfo*>());

				// if default priority, create a new vector container for each entry
				//	so they happen in sequence by default
				// if(priorityAppVector.first !=
				//		XDAQContextTable::XDAQApplication::DEFAULT_PRIORITY)
				// createContainer = false;

				createContainer = false;
			}
			retVec[retVec.size() - 1].push_back(&(it->second));

			__COUT__ << it->second.getName() << " [LID=" << it->second.getId() << "]: "
			         << " priority " << (unsigned int)priorityAppVector.first << " count "
			         << retVec[retVec.size() - 1].size() << __E__;
		}
	}  // end equal priority loop
	return retVec;
}
