#ifndef _ots_FEVInterface_h_
#define _ots_FEVInterface_h_

#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"
#include "otsdaq-core/WorkLoopManager/WorkLoop.h"
#include "otsdaq-core/Configurable/Configurable.h"
#include "otsdaq-core/FECore/FESlowControlsWorkLoop.h"



#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h" //to send errors to Gateway, e.g.


#include "otsdaq-core/FECore/FESlowControlsChannel.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h" 	//for xdaq::ApplicationDescriptor communication

#include <string>
#include <iostream>
#include <vector>
#include <array>

#define __ARGS__				FEVInterface::frontEndMacroInArgs_t argsIn, FEVInterface::frontEndMacroOutArgs_t argsOut
#define __GET_ARG_IN__(X,Y) 	getFEMacroInputArgumentValue<Y> 				(argsIn ,X)
#define __GET_ARG_OUT__(X) 		FEVInterface::getFEMacroOutputArgument   		(argsOut,X)
#define __SET_ARG_OUT__(X,Y) 	FEVInterface::setFEMacroOutputArgumentValue   	(argsOut,X,Y)

namespace ots
{

class FrontEndHardwareBase;
class FrontEndFirmwareBase;
class FEInterfaceConfigurationBase;
//class SlowControlsChannelInfo;

//FEVInterface
//	This class is a virtual class defining the features of front-end interface plugin class.
//	The features include configuration hooks, finite state machine handlers, Front-end Macros for web accessible C++ handlers, slow controls hooks, as well as universal write and read for
//	Macro Maker compatibility.
class FEVInterface : public VStateMachine, public WorkLoop, public Configurable
{
public:

	FEVInterface (const std::string& interfaceUID,
			const ConfigurationTree& theXDAQContextConfigTree,
			const std::string& configurationPath);

	virtual 				~FEVInterface  					(void) {__CFG_COUT__ << "Destructed." << __E__;}

			/////////===========================
			//start OLD - but keeping around for a while, in case we realize we need it
			//
			//virtual void 			initLocalGroup					(int local_group_comm_) {std::cout << __PRETTY_FUNCTION__ << std::endl;}
			//void 					setConfigurationManager(ConfigurationManager* configurationManager){theConfigurationManager_ = configurationManager;}
			//virtual void 			configureDetector(const DACStream& theDACStream) = 0;
			//virtual void resetDetector() = 0;
			//virtual void configureFEW     (void) = 0;
			//
			//end OLD
			/////////

	const std::string&		getInterfaceUID     			(void) const {return interfaceUID_;}
	const std::string&		getDaqHardwareType  			(void) const {return daqHardwareType_;}
	const std::string&		getFirmwareType     			(void) const {return firmwareType_;}
	virtual std::string		getInterfaceType    			(void) const {return theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode("FEInterfacePluginName").getValue<std::string>();}//interfaceType_;}

	virtual int				universalRead	        		(char* address, char* returnValue) = 0;
	virtual void 			universalWrite	        		(char* address, char* writeValue)  = 0;
	const unsigned int&		getUniversalAddressSize			(void) {return universalAddressSize_;}
	const unsigned int&		getUniversalDataSize   			(void) {return universalDataSize_;}

	FrontEndHardwareBase* 	getHardwareP					(void) const {return theFrontEndHardware_;}
	FrontEndFirmwareBase*	getFirmwareP					(void) const {return theFrontEndFirmware_;}

	void 					runSequenceOfCommands			(const std::string &treeLinkName);

	/////////===========================
	//start State Machine handlers
	void 					configure						(void) { __COUT__ << "\t Configure" << std::endl;  runSequenceOfCommands("LinkToConfigureSequence"); /*Run Configure Sequence Commands*/}
	void 					start							(std::string runNumber) { __COUT__ << "\t Start" << std::endl;  runSequenceOfCommands("LinkToStartSequence"); /*Run Start Sequence Commands*/}
	void 					stop							(void) { __COUT__ << "\t Stop" << std::endl;  runSequenceOfCommands("LinkToStopSequence"); /*Run Stop Sequence Commands*/}
	void 					halt							(void) { stop();}
	void 					pause							(void) { stop();}
	void 					resume							(void) { start("");}
	bool 					running   		  				(void) { /*while(WorkLoop::continueWorkLoop_){;}*/ return false;}
	//end State Machine handlers
	/////////

	/////////===========================
	//start Slow Controls
	void 					configureSlowControls			(void);
	bool 					slowControlsRunning				(void);
	void					startSlowControlsWorkLooop		(void) {slowControlsWorkLoop_.startWorkLoop();}
	void					stopSlowControlsWorkLooop		(void) {slowControlsWorkLoop_.stopWorkLoop();}
	//end Slow Controls
	/////////


	/////////===========================
	//start FE Macros

	//public types and functions for map of FE macros
	using	frontEndMacroInArg_t	= std::pair<const std::string /* input arg name */ , const std::string /* arg input value */ >;
	using	frontEndMacroInArgs_t	= const std::vector<frontEndMacroInArg_t> &;
	using	frontEndMacroOutArg_t	= std::pair<const std::string /* output arg name */, std::string 		/* arg return value */>;
	using	frontEndMacroOutArgs_t	= std::vector<frontEndMacroOutArg_t> &;
	using	frontEndMacroFunction_t = void (ots::FEVInterface::* )(frontEndMacroInArgs_t, frontEndMacroOutArgs_t); //void function (vector-of-inputs, vector-of-outputs)
	struct	frontEndMacroStruct_t	//members fully define a front-end macro function
	{
		frontEndMacroStruct_t(const frontEndMacroFunction_t &feMacroFunction,
				const std::vector<std::string> &namesOfInputArgs,
				const std::vector<std::string> &namesOfOutputArgs,
				const uint8_t requiredUserPermissions)
		:macroFunction_(feMacroFunction)
		,namesOfInputArguments_(namesOfInputArgs)
		,namesOfOutputArguments_(namesOfOutputArgs)
		,requiredUserPermissions_(requiredUserPermissions)
		{}

		const frontEndMacroFunction_t	macroFunction_; 		//Note: must be called using this instance
		const std::vector<std::string> 	namesOfInputArguments_, namesOfOutputArguments_;
		const uint8_t					requiredUserPermissions_;
	};
	const std::map<std::string, frontEndMacroStruct_t>&	getMapOfFEMacroFunctions(void) {return mapOfFEMacroFunctions_;}
	//end FE Macros
	/////////

protected:
	bool 					workLoopThread(toolbox::task::WorkLoop* workLoop);
	std::string             interfaceUID_;
	//std::string             interfaceType_;

	std::string  			daqHardwareType_;
	std::string  			firmwareType_;

	FrontEndHardwareBase* 	theFrontEndHardware_ = nullptr;
	FrontEndFirmwareBase* 	theFrontEndFirmware_ = nullptr;

	unsigned int 			universalAddressSize_ = 0;
	unsigned int 			universalDataSize_ = 0;

	//Controls members
	std::map<std::string, FESlowControlsChannel> 	mapOfSlowControlsChannels_;
	FESlowControlsWorkLoop							slowControlsWorkLoop_;


	//FE Macro Function members and helper functions:

	std::map<std::string, frontEndMacroStruct_t>	mapOfFEMacroFunctions_; //Map of FE Macro functions members
	void											registerFEMacroFunction(const std::string &feMacroName, frontEndMacroFunction_t feMacroFunction, const std::vector<std::string> &namesOfInputArgs, const std::vector<std::string> &namesOfOutputArgs, uint8_t requiredUserPermissions);

public: //for external specialized template access
	static const std::string&						getFEMacroInputArgument			(frontEndMacroInArgs_t &argsIn, const std::string &argName);
protected:
	static std::string&								getFEMacroOutputArgument 		(frontEndMacroOutArgs_t &argsOut, const std::string& argName);


	template<class T>
	std::string& 									setFEMacroOutputArgumentValue(
			frontEndMacroOutArgs_t &argsOut,
			const std::string &argName, const T& value) const
	{
		std::string& argOut = getFEMacroOutputArgument(argsOut,argName);
		std::stringstream ss; ss << value;
		argOut = ss.str();
		return argOut;
	}

};


template<class T>
T 												getFEMacroInputArgumentValue(
		FEVInterface::frontEndMacroInArgs_t &argsIn, const std::string &argName)
{
	//stolen from ConfigurationView
	//	only handles number types (strings are handled in non-template function override

	const std::string& data = FEVInterface::getFEMacroInputArgument(argsIn, argName);


	T retValue;

	if(!StringMacros::getNumber(data,retValue))
	{
		__SS__ << "Error extracting value for argument named '" <<
				argName << ".' The value '" << data << "' is not a number!" << std::endl;
		__COUT__ << "\n" << ss.str();
		__SS_THROW__;
	}

	return retValue;
}
//specialized template version of getFEMacroInputArgumentValue for string
template<>
std::string										getFEMacroInputArgumentValue<std::string>	(FEVInterface::frontEndMacroInArgs_t &argsIn, const std::string &argName);


}

#endif
