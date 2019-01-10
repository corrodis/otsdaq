#include "otsdaq-core/FECore/FEVInterfacesManager.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/PluginMakers/MakeInterface.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

#include "messagefacility/MessageLogger/MessageLogger.h"
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "fhiclcpp/make_ParameterSet.h"

#include <iostream>
#include <sstream>

using namespace ots;

//========================================================================================================================
FEVInterfacesManager::FEVInterfacesManager(const ConfigurationTree& theXDAQContextConfigTree, const std::string& supervisorConfigurationPath)
: Configurable(theXDAQContextConfigTree, supervisorConfigurationPath)
{
	init();
}

//========================================================================================================================
FEVInterfacesManager::~FEVInterfacesManager(void)
{
	destroy();
}

//========================================================================================================================
void FEVInterfacesManager::init(void)
{
}

//========================================================================================================================
void FEVInterfacesManager::destroy(void)
{
	for(auto& it : theFEInterfaces_)
		it.second.reset();

	theFEInterfaces_.clear();
	theFENamesByPriority_.clear();
}

//========================================================================================================================
void FEVInterfacesManager::createInterfaces(void)
{
	destroy();

	__CFG_COUT__ << "Path: "<< theConfigurationPath_+"/LinkToFEInterfaceConfiguration" << std::endl;

	std::vector<std::pair<std::string,ConfigurationTree> > feChildren =
			Configurable::getSelfNode().getNode(
					"LinkToFEInterfaceConfiguration").getChildren();

	//acquire names by priority
	theFENamesByPriority_ =
			Configurable::getSelfNode().getNode(
					"LinkToFEInterfaceConfiguration").getChildrenNames(
							true /*byPriority*/, true /*onlyStatusTrue*/);
	__CFG_COUTV__(StringMacros::vectorToString(theFENamesByPriority_));

	for(const auto& interface: feChildren)
	{
		try
		{
			if(!interface.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>()) continue;
		}
		catch(...) //if Status column not there ignore (for backwards compatibility)
		{
			__CFG_COUT_INFO__ << "Ignoring FE Status since Status column is missing!" << std::endl;
		}

		__CFG_COUT__ << "Interface Plugin Name: "<< interface.second.getNode("FEInterfacePluginName").getValue<std::string>() << std::endl;
		__CFG_COUT__ << "Interface Name: "<< interface.first << std::endl;
		__CFG_COUT__ << "XDAQContext Node: "<< theXDAQContextConfigTree_ << std::endl;
		__CFG_COUT__ << "Path to configuration: "<< (theConfigurationPath_ + "/LinkToFEInterfaceConfiguration/" + interface.first + "/LinkToFETypeConfiguration") << std::endl;

		try
		{
			theFEInterfaces_[interface.first] = makeInterface(
					interface.second.getNode("FEInterfacePluginName").getValue<std::string>(),
					interface.first,
					theXDAQContextConfigTree_,
					(theConfigurationPath_ + "/LinkToFEInterfaceConfiguration/" + interface.first + "/LinkToFETypeConfiguration")
					);

			//setup parent supervisor and interface manager
			//	of FEVinterface (for backwards compatibility, left out of constructor)
			theFEInterfaces_[interface.first]->VStateMachine::parentSupervisor_ =
					VStateMachine::parentSupervisor_;
			theFEInterfaces_[interface.first]->parentInterfaceManager_ = this;
		}
		catch(const cet::exception& e)
		{
			__CFG_SS__ << "Failed to instantiate plugin named '" <<
					interface.first << "' of type '" <<
					interface.second.getNode("FEInterfacePluginName").getValue<std::string>()
					<< "' due to the following error: \n" << e.what() << __E__;
			__MOUT_ERR__ << ss.str();
			__CFG_SS_THROW__;
		}
	}
	__CFG_COUT__ << "Done creating interfaces" << std::endl;
}
//========================================================================================================================
//getFEInterfaceP
FEVInterface* FEVInterfacesManager::getFEInterfaceP(const std::string& interfaceID)
{
	try
	{
		return theFEInterfaces_.at(interfaceID).get();
	}
	catch(...)
	{
		__CFG_SS__ << "Interface ID '" << interfaceID << "' not found in configured interfaces." << __E__;
		__CFG_SS_THROW__;
	}
} //end getFEInterfaceP()

//========================================================================================================================
//getFEInterface
const FEVInterface&	FEVInterfacesManager::getFEInterface(const std::string& interfaceID) const
{
	try
	{
		return *(theFEInterfaces_.at(interfaceID));
	}
	catch(...)
	{
		__CFG_SS__ << "Interface ID '" << interfaceID << "' not found in configured interfaces." << __E__;
		__CFG_SS_THROW__;
	}
} //end getFEInterface()

//========================================================================================================================
//universalRead
//	used by MacroMaker
//	throw std::runtime_error on error/timeout
void FEVInterfacesManager::universalRead(const std::string &interfaceID, char* address, char* returnValue)
{
	getFEInterfaceP(interfaceID)->universalRead(address, returnValue);
} //end universalRead()

//========================================================================================================================
//getInterfaceUniversalAddressSize
//	used by MacroMaker
unsigned int FEVInterfacesManager::getInterfaceUniversalAddressSize(const std::string &interfaceID)
{
	return getFEInterfaceP(interfaceID)->getUniversalAddressSize();
} //end getInterfaceUniversalAddressSize()

//========================================================================================================================
//getInterfaceUniversalDataSize
//	used by MacroMaker
unsigned int FEVInterfacesManager::getInterfaceUniversalDataSize(const std::string &interfaceID)
{
	return getFEInterfaceP(interfaceID)->getUniversalDataSize();
} //end getInterfaceUniversalDataSize()

//========================================================================================================================
//universalWrite
//	used by MacroMaker
void FEVInterfacesManager::universalWrite(const std::string &interfaceID, char* address, char* writeValue)
{
	getFEInterfaceP(interfaceID)->universalWrite(address, writeValue);
} //end universalWrite()

//========================================================================================================================
//getFEListString
//	returns string with each new line for each FE
//	each line:
//		<interface type>:<parent supervisor lid>:<interface UID>
std::string FEVInterfacesManager::getFEListString(
		const std::string &supervisorLid)
{
	std::string retList = "";

	for(const auto& it : theFEInterfaces_)
	{
	  __CFG_COUT__ << "FE name = " << it.first << std::endl;

	  retList += it.second->getInterfaceType() +
			  ":" + supervisorLid +
			  ":" + it.second->getInterfaceUID() + "\n";
	}
	return retList;
} //end getFEListString()

//========================================================================================================================
//runFEMacroByFE
//	Runs the FE Macro in the specified FE interface. Called by another FE.
//
//	inputs:
//		- inputArgs: colon-separated name/value pairs, and then comma-separated
//		- outputArgs: comma-separated (Note: resolved for FE, allowing FE to not know output arguments)
//
//	outputs:
//		- throws exception on failure
//		- outputArgs: colon-separate name/value pairs, and then comma-separated
void FEVInterfacesManager::runFEMacroByFE(const std::string &callingInterfaceID, const std::string &interfaceID,
		const std::string &feMacroName, const std::string &inputArgs,
		std::string &outputArgs)
{
	__CFG_COUTV__(callingInterfaceID);

	//check for interfaceID
	FEVInterface* fe = getFEInterfaceP(interfaceID);

	//have pointer to virtual FEInterface, find Macro structure
	auto FEMacroIt = fe->getMapOfFEMacroFunctions().find(feMacroName);
	if(FEMacroIt == fe->getMapOfFEMacroFunctions().end())
	{
		__CFG_SS__ << "FE Macro '" << feMacroName << "' of interfaceID '" <<
				interfaceID << "' was not found!" << std::endl;
		__CFG_COUT_ERR__ << "\n" << ss.str();
		__CFG_SS_THROW__;
	}

	auto& feMacro = FEMacroIt->second;

	std::set<std::string> allowedFEsSet;
	StringMacros::getSetFromString(
						feMacro.allowedCallingFrontEnds_,
						allowedFEsSet);

	//check if calling interface is allowed to call macro
	if(!StringMacros::inWildCardSet(callingInterfaceID,allowedFEsSet))
	{
		__CFG_SS__ << "FE Macro '" << feMacroName << "' of interfaceID '" <<
				interfaceID << "' does not allow access to calling interfaceID '" <<
				callingInterfaceID << "!' Did the interface add the calling interfaceID " <<
				"to the access list when registering the front-end macro." << __E__;
		__CFG_COUT_ERR__ << "\n" << ss.str();
		__CFG_SS_THROW__;
	}

	//if here, then access allowed
	//build output args list

	outputArgs = "";


	for(unsigned int i=0;i<feMacro.namesOfOutputArguments_.size();++i)
		outputArgs += (i?",":"") + feMacro.namesOfOutputArguments_[i];

	__CFG_COUTV__(outputArgs);

	runFEMacro(interfaceID,feMacro,inputArgs,outputArgs);

	__CFG_COUTV__(outputArgs);

} //end runFEMacroByFE()

//========================================================================================================================
//runFEMacro
//	Runs the FE Macro in the specified FE interface.
//
//	inputs:
//		- inputArgs: colon-separated name/value pairs, and then comma-separated
//		- outputArgs: comma-separated
//
//	outputs:
//		- throws exception on failure
//		- outputArgs: colon-separate name/value pairs, and then comma-separated
void FEVInterfacesManager::runFEMacro(const std::string &interfaceID,
		const std::string &feMacroName, const std::string &inputArgs, std::string &outputArgs)
{

	//check for interfaceID
	FEVInterface* fe = getFEInterfaceP(interfaceID);

	//have pointer to virtual FEInterface, find Macro structure
	auto FEMacroIt = fe->getMapOfFEMacroFunctions().find(feMacroName);
	if(FEMacroIt == fe->getMapOfFEMacroFunctions().end())
	{
		__CFG_SS__ << "FE Macro '" << feMacroName << "' of interfaceID '" <<
				interfaceID << "' was not found!" << std::endl;
		__CFG_COUT_ERR__ << "\n" << ss.str();
		__CFG_SS_THROW__;
	}

	runFEMacro(interfaceID,FEMacroIt->second,inputArgs,outputArgs);

} //end runFEMacro()

//========================================================================================================================
//runFEMacro
//	Runs the FE Macro in the specified FE interface.
//
//	inputs:
//		- inputArgs: semicolon-separated name/value pairs, and then comma-separated
//		- outputArgs: comma-separated
//
//	outputs:
//		- throws exception on failure
//		- outputArgs: colon-separate name/value pairs, and then comma-separated
void FEVInterfacesManager::runFEMacro(const std::string &interfaceID,
		const FEVInterface::frontEndMacroStruct_t& feMacro, const std::string &inputArgs,
		std::string &outputArgs)
{
	//build input arguments
	//	parse args, semicolon-separated pairs, and then comma-separated
	std::vector<FEVInterface::frontEndMacroConstArg_t> argsIn;
	{
		std::istringstream inputStream(inputArgs);
		std::string splitVal, argName, argValue;
		while (getline(inputStream, splitVal, ';'))
		{
			std::istringstream pairInputStream(splitVal);
			getline(pairInputStream, argName, ',');
			getline(pairInputStream, argValue, ',');
			argsIn.push_back(std::pair<std::string,std::string>(argName,argValue));
		}
	}

	//check namesOfInputArguments_
	if(feMacro.namesOfInputArguments_.size() != argsIn.size())
	{
		__CFG_SS__ << "FE Macro '" << feMacro.feMacroName_ << "' of interfaceID '" <<
				interfaceID << "' was attempted with a mismatch in" <<
						" number of input arguments. " << argsIn.size() <<
						" were given. " << feMacro.namesOfInputArguments_.size() <<
						" expected." << std::endl;
		__CFG_COUT_ERR__ << "\n" << ss.str();
		__CFG_SS_THROW__;
	}
	for(unsigned int i=0;i<argsIn.size();++i)
		if(argsIn[i].first != feMacro.namesOfInputArguments_[i])
		{
			__CFG_SS__ << "FE Macro '" << feMacro.feMacroName_ << "' of interfaceID '" <<
					interfaceID << "' was attempted with a mismatch in" <<
							" a name of an input argument. " <<
							argsIn[i].first << " were given. " <<
							feMacro.namesOfInputArguments_[i] <<
							" expected." << std::endl;
			__CFG_COUT_ERR__ << "\n" << ss.str();
			__CFG_SS_THROW__;
		}



	//build output arguments
	std::vector<std::string> returnStrings;
	std::vector<FEVInterface::frontEndMacroArg_t> argsOut;

	{
		std::istringstream inputStream(outputArgs);
		std::string argName;
		while (getline(inputStream, argName, ','))
		{
			__CFG_COUT__ << "argName " << argName << std::endl;

			returnStrings.push_back( "DEFAULT" );//std::string());
			argsOut.push_back(FEVInterface::frontEndMacroArg_t(
					argName,
					returnStrings[returnStrings.size()-1]));
			//
			//			__CFG_COUT__ << argsOut[argsOut.size()-1].first << std::endl;
			__CFG_COUT__ << (uint64_t) &(returnStrings[returnStrings.size()-1]) << std::endl;
		}
	}

	//check namesOfOutputArguments_
	if(feMacro.namesOfOutputArguments_.size() != argsOut.size())
	{
		__CFG_SS__ << "FE Macro '" << feMacro.feMacroName_ << "' of interfaceID '" <<
				interfaceID << "' was attempted with a mismatch in" <<
						" number of output arguments. " << argsOut.size() <<
						" were given. " << feMacro.namesOfOutputArguments_.size() <<
						" expected." << std::endl;
		__CFG_COUT_ERR__ << "\n" << ss.str();
		__CFG_SS_THROW__;
	}
	for(unsigned int i=0;i<argsOut.size();++i)
		if(argsOut[i].first != feMacro.namesOfOutputArguments_[i])
		{
			__CFG_SS__ << "FE Macro '" << feMacro.feMacroName_ << "' of interfaceID '" <<
					interfaceID << "' was attempted with a mismatch in" <<
							" a name of an output argument. " <<
							argsOut[i].first << " were given. " <<
							feMacro.namesOfOutputArguments_[i] <<
							" expected." << std::endl;
			__CFG_COUT_ERR__ << "\n" << ss.str();
			__CFG_SS_THROW__;
		}






	__CFG_COUT__ << "# of input args = " << argsIn.size() << std::endl;
	for(auto &argIn:argsIn)
		__CFG_COUT__ << argIn.first << ": " << argIn.second << std::endl;

	//	__CFG_COUT__ << "# of output args = " << argsOut.size() << std::endl;
	//	for(unsigned int i=0;i<argsOut.size();++i)
	//		__CFG_COUT__ << i << ": " << argsOut[i].first << std::endl;
	//	for(unsigned int i=0;i<returnStrings.size();++i)
	//		__CFG_COUT__ << i << ": " << returnStrings[i] << std::endl;




	__MOUT__ << "Launching FE Macro '" << feMacro.feMacroName_ << "' ..." << std::endl;
	__CFG_COUT__ << "Launching FE Macro '" << feMacro.feMacroName_ << "' ..." << std::endl;

	//have pointer to Macro structure, so run it
	//(FEVInterfaceIt->second.get()->*(feMacro.macroFunction_))(argsIn,argsOut);
	(getFEInterfaceP(interfaceID)->*(feMacro.macroFunction_))(argsIn,argsOut);

	__CFG_COUT__ << "FE Macro complete!" << std::endl;

	__CFG_COUT__ << "# of output args = " << argsOut.size() << std::endl;
	for(const auto &arg:argsOut)
		__CFG_COUT__ << arg.first << ": " << arg.second << std::endl;


	//check namesOfOutputArguments_ size
	if(feMacro.namesOfOutputArguments_.size() != argsOut.size())
	{
		__CFG_SS__ << "FE Macro '" << feMacro.feMacroName_ << "' of interfaceID '" <<
				interfaceID << "' was attempted but the FE macro "
						"manipulated the output arguments vector. It is illegal "
						"to add or remove output vector name/value pairs." << std::endl;
		__CFG_COUT_ERR__ << "\n" << ss.str();
		__CFG_SS_THROW__;
	}



	//Success! at this point so return the output string
	outputArgs = "";
	for(unsigned int i=0; i<argsOut.size(); ++i)
	{
		if(i) outputArgs += ";";
		outputArgs += argsOut[i].first + "," + argsOut[i].second;
	}

	__CFG_COUT__ << "outputArgs = " << outputArgs << std::endl;

} //end runFEMacro()

//========================================================================================================================
//getFEMacrosString
//	returns string with each new line indicating the macros for a FE
//	each line:
//		<parent supervisor name>:<parent supervisor lid>:<interface type>:<interface UID>
//		:<macro name>:<macro permissions req>:<macro num of inputs>:...<input names : separated>...
//		:<macro num of outputs>:...<output names : separated>...
std::string FEVInterfacesManager::getFEMacrosString(
		const std::string &supervisorName,
		const std::string &supervisorLid)
{
	std::string retList = "";

	for(const auto& it : theFEInterfaces_)
	{
		  __CFG_COUT__ << "FE interface UID = " << it.first << std::endl;

	  retList += supervisorName +
			  ":" + supervisorLid +
			  ":" + it.second->getInterfaceType() +
			  ":" + it.second->getInterfaceUID();

	  for(const auto& macroPair : it.second->getMapOfFEMacroFunctions())
	  {
		  __CFG_COUT__ << "FE Macro name = " << macroPair.first << std::endl;
		  retList +=
				  ":" + macroPair.first +
				  ":" + std::to_string(macroPair.second.requiredUserPermissions_) +
				  ":" + std::to_string(macroPair.second.namesOfInputArguments_.size());
		  for(const auto& name:macroPair.second.namesOfInputArguments_)
			  retList += ":" + name;

		  retList +=
				  ":" + std::to_string(macroPair.second.namesOfOutputArguments_.size());
		  for(const auto& name:macroPair.second.namesOfOutputArguments_)
		  			  retList += ":" + name;
	  }

	  retList += "\n";
	}
	return retList;
}

//========================================================================================================================
bool FEVInterfacesManager::allFEWorkloopsAreDone(void)
{
	bool allFEWorkloopsAreDone = true;
	bool isActive;

	for(const auto& FEInterface: theFEInterfaces_)
	{
		isActive = FEInterface.second->WorkLoop::isActive();

		__CFG_COUT__ << FEInterface.second->getInterfaceUID() << " of type " <<
				FEInterface.second->getInterfaceType() << ": \t" <<
				"workLoop_->isActive() " <<
			(isActive?"yes":"no") << std::endl;

		if(isActive) //then not done
		{
			allFEWorkloopsAreDone = false;
			break;
		}
	}

	return allFEWorkloopsAreDone;
} //end allFEWorkloopsAreDone()


//========================================================================================================================
void FEVInterfacesManager::preStateMachineExecutionLoop(void)
{
	VStateMachine::clearIterationWork();
	VStateMachine::clearSubIterationWork();

	stateMachinesIterationWorkCount_ = 0;

	__CFG_COUT__ << "Number of front ends to transition: " <<
			theFENamesByPriority_.size() << __E__;

	if(VStateMachine::getIterationIndex() == 0 &&
			VStateMachine::getSubIterationIndex() == 0)
	{
		//reset map for iterations done on first iteration

		subIterationWorkStateMachineIndex_ = -1; //clear sub iteration work index

		stateMachinesIterationDone_.clear();
		for(const auto& FEPair : theFEInterfaces_)
			stateMachinesIterationDone_[FEPair.first] = false; //init to not done
	}
	else
		__CFG_COUT__ << "Iteration " << VStateMachine::getIterationIndex() <<
			"." << VStateMachine::getSubIterationIndex() <<
			"(" << subIterationWorkStateMachineIndex_ << ")" << __E__;
} //end preStateMachineExecutionLoop()

//========================================================================================================================
void FEVInterfacesManager::preStateMachineExecution(unsigned int i)
{
	if(i >= theFENamesByPriority_.size())
	{
		__CFG_SS__ << "FE Interface " << i << " not found!" << __E__;
		__CFG_SS_THROW__;
	}

	const std::string& name = theFENamesByPriority_[i];

	FEVInterface* fe = getFEInterfaceP(name);

	fe->VStateMachine::setIterationIndex(
			VStateMachine::getIterationIndex());
	fe->VStateMachine::setSubIterationIndex(
			VStateMachine::getSubIterationIndex());

	fe->VStateMachine::clearIterationWork();
	fe->VStateMachine::clearSubIterationWork();

	__CFG_COUT__ << "theStateMachineImplementation Iteration " <<
			fe->VStateMachine::getIterationIndex() <<
		"." << fe->VStateMachine::getSubIterationIndex() << __E__;
} //end preStateMachineExecution()

//========================================================================================================================
//postStateMachineExecution
//	return false to indicate state machine is NOT done with transition
bool FEVInterfacesManager::postStateMachineExecution(unsigned int i)
{
	if(i >= theFENamesByPriority_.size())
	{
		__CFG_SS__ << "FE Interface index " << i << " not found!" << __E__;
		__CFG_SS_THROW__;
	}

	const std::string& name = theFENamesByPriority_[i];

	FEVInterface* fe = getFEInterfaceP(name);

	//sub-iteration has priority
	if(fe->VStateMachine::getSubIterationWork())
	{
		subIterationWorkStateMachineIndex_ = i;
		VStateMachine::indicateSubIterationWork();

		__CFG_COUT__ << "FE Interface '" << name << "' is flagged for another sub-iteration..." << __E__;
		return false; //to indicate state machine is NOT done with transition
	}
	else
	{
		subIterationWorkStateMachineIndex_ = -1; //clear sub iteration work index

		bool& stateMachineDone = stateMachinesIterationDone_[name];
			stateMachineDone =
					!fe->VStateMachine::getIterationWork();


		if(!stateMachineDone)
		{
			__CFG_COUT__ << "FE Interface '" << name << "' is flagged for another iteration..." << __E__;
					VStateMachine::indicateIterationWork(); //mark not done at FEVInterfacesManager level
			++stateMachinesIterationWorkCount_; //increment still working count
			return false; //to indicate state machine is NOT done with transition
		}
	}
	return true; //to indicate state machine is done with transition
} //end postStateMachineExecution()

//========================================================================================================================
void FEVInterfacesManager::postStateMachineExecutionLoop(void)
{
	if(VStateMachine::getSubIterationWork())
		__CFG_COUT__ << "FE Interface state machine implementation " << subIterationWorkStateMachineIndex_ <<
			" is flagged for another sub-iteration..." << __E__;
	else if(VStateMachine::getIterationWork())
		__CFG_COUT__ << stateMachinesIterationWorkCount_ <<
			" FE Interface state machine implementation(s) flagged for another iteration..." << __E__;
	else
		__CFG_COUT__ << "Done configuration all state machine implementations..." << __E__;
} //end postStateMachineExecutionLoop()

//========================================================================================================================
void FEVInterfacesManager::configure(void)
{
	//create interfaces (the first iteration)
	if(VStateMachine::getIterationIndex() == 0 &&
			VStateMachine::getSubIterationIndex() == 0)
		createInterfaces(); //by priority

	const std::string transitionName = "Configuring";
	FEVInterface* fe;

	preStateMachineExecutionLoop();
	for(unsigned int i=0;i<theFENamesByPriority_.size();++i)
	{
		//if one state machine is doing a sub-iteration, then target that one
		if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
				i != subIterationWorkStateMachineIndex_) continue; //skip those not in the sub-iteration

		const std::string& name = theFENamesByPriority_[i];

		//test for front-end existence
		fe = getFEInterfaceP(name);

		if(stateMachinesIterationDone_[name]) continue; //skip state machines already done


		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;

		preStateMachineExecution(i);
		fe->configure();
		postStateMachineExecution(i);

		//configure slow controls and start slow controls workloop
		//	slow controls workloop stays alive through start/stop.. and dies on halt
		fe->configureSlowControls();
		//fe->startSlowControlsWorkLooop();

		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;

	}
	postStateMachineExecutionLoop();

	__CFG_COUT__ << "Done " << transitionName << " all interfaces." << std::endl;
} //end configure()

//========================================================================================================================
void FEVInterfacesManager::halt(void)
{
	const std::string transitionName = "Halting";
	FEVInterface* fe;

	preStateMachineExecutionLoop();
	for(unsigned int i=0;i<theFENamesByPriority_.size();++i)
	{
		//if one state machine is doing a sub-iteration, then target that one
		if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
				i != subIterationWorkStateMachineIndex_) continue; //skip those not in the sub-iteration

		const std::string& name = theFENamesByPriority_[i];

		fe = getFEInterfaceP(name);

		if(stateMachinesIterationDone_[name]) continue; //skip state machines already done

		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;

		preStateMachineExecution(i);
		fe->stopWorkLoop();
		fe->stopSlowControlsWorkLooop();
		fe->halt();
		postStateMachineExecution(i);

		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
	}
	postStateMachineExecutionLoop();


	__CFG_COUT__ << "Done " << transitionName << " all interfaces." << std::endl;

	destroy(); //destroy all FE interfaces on halt, must be configured for FE interfaces to exist
} //end halt()

//========================================================================================================================
void FEVInterfacesManager::pause(void)
{
	const std::string transitionName = "Pausing";
	FEVInterface* fe;

	preStateMachineExecutionLoop();
	for(unsigned int i=0;i<theFENamesByPriority_.size();++i)
	{
		//if one state machine is doing a sub-iteration, then target that one
		if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
				i != subIterationWorkStateMachineIndex_) continue; //skip those not in the sub-iteration

		const std::string& name = theFENamesByPriority_[i];

		fe = getFEInterfaceP(name);

		if(stateMachinesIterationDone_[name]) continue; //skip state machines already done

		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;

		preStateMachineExecution(i);
		fe->stopWorkLoop();
		fe->pause();
		postStateMachineExecution(i);

		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
	}
	postStateMachineExecutionLoop();

	__CFG_COUT__ << "Done " << transitionName << " all interfaces." << std::endl;
} //end pause()

//========================================================================================================================
void FEVInterfacesManager::resume(void)
{
	const std::string transitionName = "Resuming";
	FEVInterface* fe;

	preStateMachineExecutionLoop();
	for(unsigned int i=0;i<theFENamesByPriority_.size();++i)
	{
		//if one state machine is doing a sub-iteration, then target that one
		if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
				i != subIterationWorkStateMachineIndex_) continue; //skip those not in the sub-iteration

		const std::string& name = theFENamesByPriority_[i];

		FEVInterface* fe = getFEInterfaceP(name);

		if(stateMachinesIterationDone_[name]) continue; //skip state machines already done

		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;

		preStateMachineExecution(i);
		fe->resume();
		//only start workloop once transition is done
		if(postStateMachineExecution(i))
			fe->startWorkLoop();

		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
	}
	postStateMachineExecutionLoop();

	__CFG_COUT__ << "Done " << transitionName << " all interfaces." << std::endl;

} //end resume()

//========================================================================================================================
void FEVInterfacesManager::start(std::string runNumber)
{
	const std::string transitionName = "Starting";
	FEVInterface* fe;

	preStateMachineExecutionLoop();
	for(unsigned int i=0;i<theFENamesByPriority_.size();++i)
	{
		//if one state machine is doing a sub-iteration, then target that one
		if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
				i != subIterationWorkStateMachineIndex_) continue; //skip those not in the sub-iteration

		const std::string& name = theFENamesByPriority_[i];

		fe = getFEInterfaceP(name);

		if(stateMachinesIterationDone_[name]) continue; //skip state machines already done

		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;

		preStateMachineExecution(i);
		fe->start(runNumber);
		//only start workloop once transition is done
		if(postStateMachineExecution(i))
			fe->startWorkLoop();


		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
	}
	postStateMachineExecutionLoop();

	__CFG_COUT__ << "Done " << transitionName << " all interfaces." << std::endl;

} //end start()

//========================================================================================================================
void FEVInterfacesManager::stop(void)
{
	const std::string transitionName = "Starting";
	FEVInterface* fe;

	preStateMachineExecutionLoop();
	for(unsigned int i=0;i<theFENamesByPriority_.size();++i)
	{
		//if one state machine is doing a sub-iteration, then target that one
		if(subIterationWorkStateMachineIndex_ != (unsigned int)-1 &&
				i != subIterationWorkStateMachineIndex_) continue; //skip those not in the sub-iteration

		const std::string& name = theFENamesByPriority_[i];

		fe = getFEInterfaceP(name);

		if(stateMachinesIterationDone_[name]) continue; //skip state machines already done

		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << transitionName << " interface " << name << std::endl;

		preStateMachineExecution(i);
		fe->stopWorkLoop();
		fe->stop();
		postStateMachineExecution(i);

		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
		__CFG_COUT__ << "Done " << transitionName << " interface " << name << std::endl;
	}
	postStateMachineExecutionLoop();

	__CFG_COUT__ << "Done " << transitionName << " all interfaces." << std::endl;

} //end stop()
