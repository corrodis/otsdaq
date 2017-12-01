#include "otsdaq-core/FECore/FEVInterfacesManager.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"
#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-core/PluginMakers/MakeInterface.h"
#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"

#include "messagefacility/MessageLogger/MessageLogger.h"
#include "artdaq/DAQdata/configureMessageFacility.hh"
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
}

//========================================================================================================================
void FEVInterfacesManager::createInterfaces(void)
{
	destroy();

	__COUT__ << "Path: "<< theConfigurationPath_+"/LinkToFEInterfaceConfiguration" << std::endl;
	for(const auto& interface: theXDAQContextConfigTree_.getNode(theConfigurationPath_+"/LinkToFEInterfaceConfiguration").getChildren())
	{
		try
		{
			if(!interface.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>()) continue;
		}
		catch(...) //if Status column not there ignore (for backwards compatibility)
		{
			__COUT_INFO__ << "Ignoring FE Status since Status column is missing!" << std::endl;
		}

		__COUT__ << "Interface Plugin Name: "<< interface.second.getNode("FEInterfacePluginName").getValue<std::string>() << std::endl;
		__COUT__ << "Interface Name: "<< interface.first << std::endl;
		__COUT__ << "XDAQContext Node: "<< theXDAQContextConfigTree_ << std::endl;
		__COUT__ << "Path to configuration: "<< (theConfigurationPath_ + "/LinkToFEInterfaceConfiguration/" + interface.first + "/LinkToFETypeConfiguration") << std::endl;
		theFEInterfaces_[interface.first] = makeInterface(
				interface.second.getNode("FEInterfacePluginName").getValue<std::string>(),
				interface.first,
				theXDAQContextConfigTree_,
				(theConfigurationPath_ + "/LinkToFEInterfaceConfiguration/" + interface.first + "/LinkToFETypeConfiguration")
				);
	}
	__COUT__ << "Done creating interfaces" << std::endl;
}

//========================================================================================================================
//used by MacroMaker
int FEVInterfacesManager::universalRead(const std::string &interfaceID, char* address, char* returnValue)
{
	__COUT__ << "interfaceID: " << interfaceID << " and size: " << theFEInterfaces_.size() << std::endl;

	if (theFEInterfaces_[interfaceID]->universalRead(address, returnValue) < 0) return -1;
	return 0;
}


//========================================================================================================================
//used by MacroMaker
unsigned int FEVInterfacesManager::getInterfaceUniversalAddressSize(const std::string &interfaceID)
{ return theFEInterfaces_[interfaceID]->getUniversalAddressSize(); } //used by MacroMaker

//========================================================================================================================
//used by MacroMaker
unsigned int FEVInterfacesManager::getInterfaceUniversalDataSize(const std::string &interfaceID)
{ return theFEInterfaces_[interfaceID]->getUniversalDataSize(); } //used by MacroMaker

//========================================================================================================================
//used by MacroMaker
void FEVInterfacesManager::universalWrite(const std::string &interfaceID, char* address, char* writeValue)
{

	__COUT__ << "interfaceID: " << interfaceID << " and size: " << theFEInterfaces_.size() << std::endl;

	theFEInterfaces_[interfaceID]->universalWrite(address, writeValue);

//	if(interfaceIndex >= theFEInterfaces_.size())
//	{
//		__COUT__ << "ERROR!!!! Invalid interface index" << std::endl;
//		return; //invalid interface index
//	}


}

//========================================================================================================================
//getFEListString
//	returns string with each new line indicating the macros for a FE
//	each line:
//		<interface type>:<parent supervisor lid>:<interface UID>
std::string FEVInterfacesManager::getFEListString(const std::string &supervisorLid)
{
	std::string retList = "";

	for(const auto& it : theFEInterfaces_)
	{
	  __COUT__ << "Just curious: it.first is " << it.first << std::endl;

	  retList += it.second->getInterfaceType() +
			  ":" + supervisorLid + ":" +
			  it.second->getInterfaceUID() + "\n";
	}
	return retList;
}

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
	auto FEVInterfaceIt = theFEInterfaces_.find(interfaceID);
	if(FEVInterfaceIt == theFEInterfaces_.end())
	{
		__SS__ << "interfaceID '" << interfaceID << "' was not found!" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	//have pointer to virtual FEInterface, find Macro structure
	auto FEMacroIt = FEVInterfaceIt->second->getMapOfFEMacroFunctions().find(feMacroName);
	if(FEMacroIt == FEVInterfaceIt->second->getMapOfFEMacroFunctions().end())
	{
		__SS__ << "FE Macro '" << feMacroName << "' of interfaceID '" <<
				interfaceID << "' was not found!" << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	//build input arguments
	//	parse args, colon-separated pairs, and then comma-separated
	std::vector<FEVInterface::frontEndMacroInArg_t> argsIn;
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
	if(FEMacroIt->second.namesOfInputArguments_.size() != argsIn.size())
	{
		__SS__ << "FE Macro '" << feMacroName << "' of interfaceID '" <<
				interfaceID << "' was attempted with a mismatch in" <<
						" number of input arguments. " << argsIn.size() <<
						" were given. " << FEMacroIt->second.namesOfInputArguments_.size() <<
						" expected." << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}
	for(unsigned int i=0;i<argsIn.size();++i)
		if(argsIn[i].first != FEMacroIt->second.namesOfInputArguments_[i])
		{
			__SS__ << "FE Macro '" << feMacroName << "' of interfaceID '" <<
					interfaceID << "' was attempted with a mismatch in" <<
							" a name of an input argument. " <<
							argsIn[i].first << " were given. " <<
							FEMacroIt->second.namesOfInputArguments_[i] <<
							" expected." << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}



	//build output arguments
	std::vector<std::string> returnStrings;
	std::vector<FEVInterface::frontEndMacroOutArg_t> argsOut;

	{
		std::istringstream inputStream(outputArgs);
		std::string argName;
		while (getline(inputStream, argName, ','))
		{
			__COUT__ << "argName " << argName << std::endl;

			returnStrings.push_back( "valueLore" );//std::string());
			argsOut.push_back(FEVInterface::frontEndMacroOutArg_t(
					argName,
					returnStrings[returnStrings.size()-1]));
			//
			//			__COUT__ << argsOut[argsOut.size()-1].first << std::endl;
			__COUT__ << (uint64_t) &(returnStrings[returnStrings.size()-1]) << std::endl;
		}
	}

	//check namesOfOutputArguments_
	if(FEMacroIt->second.namesOfOutputArguments_.size() != argsOut.size())
	{
		__SS__ << "FE Macro '" << feMacroName << "' of interfaceID '" <<
				interfaceID << "' was attempted with a mismatch in" <<
						" number of output arguments. " << argsOut.size() <<
						" were given. " << FEMacroIt->second.namesOfOutputArguments_.size() <<
						" expected." << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}
	for(unsigned int i=0;i<argsOut.size();++i)
		if(argsOut[i].first != FEMacroIt->second.namesOfOutputArguments_[i])
		{
			__SS__ << "FE Macro '" << feMacroName << "' of interfaceID '" <<
					interfaceID << "' was attempted with a mismatch in" <<
							" a name of an output argument. " <<
							argsOut[i].first << " were given. " <<
							FEMacroIt->second.namesOfOutputArguments_[i] <<
							" expected." << std::endl;
			__COUT_ERR__ << "\n" << ss.str();
			throw std::runtime_error(ss.str());
		}






	__COUT__ << "# of input args = " << argsIn.size() << std::endl;
	for(auto &argIn:argsIn)
		__COUT__ << argIn.first << ": " << argIn.second << std::endl;

	//	__COUT__ << "# of output args = " << argsOut.size() << std::endl;
	//	for(unsigned int i=0;i<argsOut.size();++i)
	//		__COUT__ << i << ": " << argsOut[i].first << std::endl;
	//	for(unsigned int i=0;i<returnStrings.size();++i)
	//		__COUT__ << i << ": " << returnStrings[i] << std::endl;




	__COUT__ << "Trying it " << std::endl;

	//have pointer to Macro structure, so run it
	(FEVInterfaceIt->second.get()->*(FEMacroIt->second.macroFunction_))(argsIn,argsOut);

	__COUT__ << "Made it " << std::endl;

	__COUT__ << "# of output args = " << argsOut.size() << std::endl;
	for(const auto &arg:argsOut)
		__COUT__ << arg.first << ": " << arg.second << std::endl;





	//check namesOfOutputArguments_ size
	if(FEMacroIt->second.namesOfOutputArguments_.size() != argsOut.size())
	{
		__SS__ << "FE Macro '" << feMacroName << "' of interfaceID '" <<
				interfaceID << "' was attempted but the FE macro "
						"manipulated the output arguments vector. It is illegal "
						"to add or remove output vector name/value pairs." << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}



	//Success! at this point so return the output string
	outputArgs = "";
	for(unsigned int i=0; i<argsOut.size(); ++i)
	{
		if(i) outputArgs += ";";
		outputArgs += argsOut[i].first + "," + argsOut[i].second;
	}

	__COUT__ << "outputArgs = " << outputArgs << std::endl;

}

//========================================================================================================================
//getFEMacrosString
//	returns string with each new line indicating the macros for a FE
//	each line:
//		<interface type>:<parent supervisor lid>:<interface UID>
//		:<macro name>:<macro permissions req>:<macro num of inputs>:...<input names : separated>...
//		:<macro num of outputs>:...<output names : separated>...
std::string FEVInterfacesManager::getFEMacrosString(const std::string &supervisorLid)
{
	std::string retList = "";

	for(const auto& it : theFEInterfaces_)
	{
		  __COUT__ << "FE interface UID = " << it.first << std::endl;

	  retList += it.second->getInterfaceType() +
			  ":" + supervisorLid + ":" +
			  it.second->getInterfaceUID();

	  for(const auto& macroPair : it.second->getMapOfFEMacroFunctions())
	  {
		  __COUT__ << "FE Macro name = " << macroPair.first << std::endl;
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
		isActive = FEInterface.second->isActive();
		__COUT__ << FEInterface.second->getInterfaceUID() << " of type " <<
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
}

//========================================================================================================================
void FEVInterfacesManager::configure(void)
{
	createInterfaces();
	for(const auto& it : theFEInterfaces_)
	{
//		if(supervisorType_ == "FER")
//			it.second->initLocalGroup((int)local_group_comm_);

		__COUT__ << "Configuring interface " << it.first << std::endl;
		__COUT__ << "Configuring interface " << it.first << std::endl;
		__COUT__ << "Configuring interface " << it.first << std::endl;
		it.second->configure();

		//configure slow controls and start slow controls workloop
		//	slow controls workloop stays alive through start/stop.. and dies on halt
		//LOREit.second->configureSlowControls();
		//WAS ALREADY COMMENTED OUTit.second->startSlowControlsWorkLooop();

//		if((supervisorType_ == "FEW") || (supervisorType_ == "FEWR") )
//		{

		__COUT__ << "Done configuring interface " << it.first << std::endl;
		__COUT__ << "Done configuring interface " << it.first << std::endl;
		__COUT__ << "Done configuring interface " << it.first << std::endl;
		//throw std::runtime_error(ss.str());
		//	it.second->configureDetector(theConfigurationManager_->getDACStream(it.first));

//		}
	}


	__COUT__ << "Done Configure" << std::endl;
}

//========================================================================================================================
void FEVInterfacesManager::halt(void)
{
	for(const auto& it : theFEInterfaces_)
	{
		it.second->halt();
		it.second->stopWorkLoop();
		//it.second->stopSlowControlsWorkLooop();
	}

	destroy(); //destroy all FE interfaces on halt, must be configured for FE interfaces to exist
}

//========================================================================================================================
void FEVInterfacesManager::pause(void)
{
	for(const auto& it : theFEInterfaces_)
	{
		it.second->pause();
		it.second->stopWorkLoop();
	}
}

//========================================================================================================================
void FEVInterfacesManager::resume(void)
{
	for(const auto& it : theFEInterfaces_)
	{
		it.second->resume();
		it.second->startWorkLoop();
	}
}

//========================================================================================================================
void FEVInterfacesManager::start(std::string runNumber)
{
	for(const auto& it : theFEInterfaces_)
	{
		it.second->start(runNumber);
		it.second->startWorkLoop();
	}
}
//========================================================================================================================
void FEVInterfacesManager::stop(void)
{
	for(const auto& it : theFEInterfaces_)
	{
		it.second->stopWorkLoop();
		it.second->stop();
	}
}
