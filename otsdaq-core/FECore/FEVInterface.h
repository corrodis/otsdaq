#ifndef _ots_FEVInterface_h_
#define _ots_FEVInterface_h_

#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"
#include "otsdaq-core/WorkLoopManager/WorkLoop.h"
#include "otsdaq-core/ConfigurationInterface/Configurable.h"
#include "otsdaq-core/FECore/FESlowControlsWorkLoop.h"


//#include "otsdaq-core/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq-core/FECore/FESlowControlsChannel.h"

#include <string>
#include <iostream>
#include <vector>
#include <array>

namespace ots
{
class FrontEndHardwareBase;
class FrontEndFirmwareBase;
class FEInterfaceConfigurationBase;
//class SlowControlsChannelInfo;

class FEVInterface : public VStateMachine, public WorkLoop, public Configurable
{
public:

	FEVInterface (const std::string& interfaceUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& configurationPath)
: WorkLoop                    	(interfaceUID)
, Configurable                	(theXDAQContextConfigTree, configurationPath)
, interfaceUID_	              	(interfaceUID)
, interfaceType_				(theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode("FEInterfacePluginName").getValue<std::string>())
, daqHardwareType_            	("NOT SET")
, firmwareType_               	("NOT SET")
, slowControlsWorkLoop_			(interfaceUID + "-SlowControls", this)
{}

	virtual 				~FEVInterface  					(void) {;}

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
	const std::string&		getInterfaceType    			(void) const {return interfaceType_;}

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
	const std::map<std::string, frontEndMacroStruct_t>&	getMapOfFEMacroFunctions() {return mapOfFEMacroFunctions_;}
	//end FE Macros
	/////////

protected:
	bool 					workLoopThread(toolbox::task::WorkLoop* workLoop){continueWorkLoop_ = running(); /* in case users return false, without using continueWorkLoop_*/ return continueWorkLoop_;}
	std::string             interfaceUID_;
	std::string             interfaceType_;


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
	std::array<std::string, 3> testarr_;
	//void											registerFEMacroFunction(const std::string &feMacroName, frontEndMacroFunction_t feMacroFunction, unsigned int numOfInputArgs, unsigned int numOfOutputArgs, uint8_t requiredUserPermissions);
	void											registerFEMacroFunction(const std::string &feMacroName, frontEndMacroFunction_t feMacroFunction, const std::vector<std::string> &namesOfInputArgs, const std::vector<std::string> &namesOfOutputArgs,uint8_t requiredUserPermissions);
	const std::string&								getFEMacroInputArgument(frontEndMacroInArgs_t &argsIn, const std::string &argName);
	std::string&									getFEMacroOutputArgument(frontEndMacroOutArgs_t &argsOut, const std::string& argName);
};

}

#endif
