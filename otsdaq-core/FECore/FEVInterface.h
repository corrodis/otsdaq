#ifndef _ots_FEVInterface_h_
#define _ots_FEVInterface_h_

#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"
#include "otsdaq-core/WorkLoopManager/WorkLoop.h"
#include "otsdaq-core/Configurable/Configurable.h"
#include "otsdaq-core/FECore/FESlowControlsWorkLoop.h"

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h" //to send errors to Gateway, e.g.

#include "otsdaq-core/CoreSupervisors/FESupervisor.h"

#include "otsdaq-core/FECore/FESlowControlsChannel.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h" 	//for xdaq::ApplicationDescriptor communication

#include <string>
#include <iostream>
#include <vector>
#include <array>

#define __ARGS__				FEVInterface::frontEndMacroConstArgs_t argsIn, FEVInterface::frontEndMacroArgs_t argsOut

#define __GET_ARG_IN__(X,Y) 	getFEMacroConstArgumentValue<Y> 			(argsIn ,X)
#define __GET_ARG_OUT__(X,Y)	getFEMacroArgumentValue<Y> 					(argsOut,X)

#define __SET_ARG_IN__(X,Y) 	FEVInterface::emplaceFEMacroArgumentValue	(argsIn,X,Y)
#define __SET_ARG_OUT__(X,Y) 	FEVInterface::setFEMacroArgumentValue   	(argsOut,X,Y)

namespace ots
{

//class FrontEndHardwareBase;
//class FrontEndFirmwareBase;
//class FEInterfaceConfigurationBase;
//class SlowControlsChannelInfo;
class FEVInterfacesManager;

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

	virtual 				~FEVInterface  					(void);

	FEVInterfacesManager*	parentInterfaceManager_;

	const std::string&		getInterfaceUID     			(void) const {return interfaceUID_;}
	virtual std::string		getInterfaceType    			(void) const {return theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode("FEInterfacePluginName").getValue<std::string>();}//interfaceType_;}

	virtual void			universalRead	        		(char* address, char* returnValue) = 0; //throw std::runtime_error exception on error/timeout
	virtual void 			universalWrite	        		(char* address, char* writeValue)  = 0;
	const unsigned int&		getUniversalAddressSize			(void) {return universalAddressSize_;}
	const unsigned int&		getUniversalDataSize   			(void) {return universalDataSize_;}

	void 					runSequenceOfCommands			(const std::string& treeLinkName);

	static void 			sendAsyncErrorToGateway			(FEVInterface* fe, const std::string& errMsg, bool isSoftError);


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
	using	frontEndMacroConstArg_t	= std::pair<const std::string /* input arg name */ , const std::string /* arg input value */ >;
	using	frontEndMacroConstArgs_t= const std::vector<frontEndMacroConstArg_t>& ;
	using	frontEndMacroArg_t		= std::pair<const std::string /* output arg name */, std::string 		/* arg return value */>;
	using	frontEndMacroArgs_t		= std::vector<frontEndMacroArg_t>& ;
	using	frontEndMacroFunction_t = void (ots::FEVInterface::* )(frontEndMacroConstArgs_t, frontEndMacroArgs_t); //void function (vector-of-inputs, vector-of-outputs)
	struct	frontEndMacroStruct_t	//members fully define a front-end macro function
	{
		frontEndMacroStruct_t(
				const std::string& feMacroName,
				const frontEndMacroFunction_t& feMacroFunction,
				const std::vector<std::string>& namesOfInputArgs,
				const std::vector<std::string>& namesOfOutputArgs,
				const uint8_t requiredUserPermissions = 1 /*1:=user,255:=admin*/,
				const std::string& allowedCallingFrontEnds = "*" /*StringMacros:: wild card set match string (i.e. string-to-set, then wild-card-set match)*/)
		:feMacroName_					(feMacroName)
		,macroFunction_					(feMacroFunction)
		,namesOfInputArguments_			(namesOfInputArgs)
		,namesOfOutputArguments_		(namesOfOutputArgs)
		,requiredUserPermissions_		(requiredUserPermissions)
		,allowedCallingFrontEnds_		(allowedCallingFrontEnds)
		{}

		const std::string				feMacroName_;
		const frontEndMacroFunction_t	macroFunction_; 		//Note: must be called using this instance
		const std::vector<std::string> 	namesOfInputArguments_, namesOfOutputArguments_;
		const uint8_t					requiredUserPermissions_;
		const std::string				allowedCallingFrontEnds_;
	};
	const std::map<std::string, frontEndMacroStruct_t>&	getMapOfFEMacroFunctions(void) {return mapOfFEMacroFunctions_;}
	//end FE Macros
	/////////





	/////////===========================
    //start FE communication helpers

    template<class T>
    void 						sendToFrontEnd						(const std::string& targetInterfaceID, const T& value) const
    {
    	__FE_COUTV__(targetInterfaceID);
    	std::stringstream ss;
    	ss << value;
    	__FE_COUTV__(ss.str());

    	__FE_COUTV__(VStateMachine::parentSupervisor_);
    	
    	xoap::MessageReference message = 
    		SOAPUtilities::makeSOAPMessageReference("FECommunication");
						
		SOAPParameters parameters;
		parameters.addParameter("type", "feSend");
		parameters.addParameter("sourceInterfaceID", FEVInterface::interfaceUID_);
		parameters.addParameter("targetInterfaceID", targetInterfaceID);
		parameters.addParameter("value", ss.str());
		SOAPUtilities::addParameters(message, parameters);
		
		__FE_COUT__ << "Sending FE communication: " <<
				SOAPUtilities::translate(message) << __E__;

		xoap::MessageReference replyMessage = VStateMachine::parentSupervisor_->SOAPMessenger::sendWithSOAPReply(
			VStateMachine::parentSupervisor_->allSupervisorInfo_.getAllMacroMakerTypeSupervisorInfo().
			begin()->second.getDescriptor(), message);

		__FE_COUT__ << "Response received: " <<
				SOAPUtilities::translate(replyMessage) << __E__;

		SOAPParameters rxParameters;
		rxParameters.addParameter("Error");
		SOAPUtilities::receive(replyMessage,rxParameters);

		std::string error = rxParameters.getValue("Error");

		if(error != "")
		{
			//error occurred!
			__FE_SS__ << "Error transmitting request to target interface '" <<
					targetInterfaceID << "' from '" << FEVInterface::interfaceUID_ << ".' " <<
					error << __E__;
			__FE_SS_THROW__;
		}

    } //end sendToFrontEnd()
    void 						runFrontEndMacro					(const std::string& targetInterfaceID, const std::string& feMacroName, const std::vector<frontEndMacroArg_t>& inputArgs, std::vector<frontEndMacroArg_t>& outputArgs) const;

    /////////
    //receiveFromFrontEnd
    //	* can be used for source interface ID to accept a message from any front-end
    // NOTE: can not overload functions based on return type, so T& passed as value
    template<class T>
    void						receiveFromFrontEnd					(const std::string& sourceInterfaceID, T& retValue, unsigned int timeoutInSeconds = 1) const
    {
    	__FE_COUTV__(sourceInterfaceID);
    	__FE_COUTV__(VStateMachine::parentSupervisor_);

    	std::string data;
    	FEVInterface::receiveFromFrontEnd(sourceInterfaceID,data,timeoutInSeconds);

    	if(!StringMacros::getNumber(data,retValue))
    	{
    		__SS__ << (data + " is not a number!") << __E__;
    		__SS_THROW__;
    	}
    } //end receiveFromFrontEnd()
    //	specialized template function for T=std::string
    void						receiveFromFrontEnd					(const std::string& sourceInterfaceID, std::string& retValue, unsigned int timeoutInSeconds = 1) const;
    // NOTE: can not overload functions based on return type, so calls function with T& passed as value
    template<class T>
    T 							receiveFromFrontEnd					(const std::string& sourceInterfaceID = "*", unsigned int timeoutInSeconds = 1) const
    {
    	T retValue;
    	//call receiveFromFrontEnd without <T> so strings are handled well
    	FEVInterface::receiveFromFrontEnd(sourceInterfaceID, retValue, timeoutInSeconds);
    	return retValue;
    } //end receiveFromFrontEnd()
    //	specialized template function for T=std::string
    std::string					receiveFromFrontEnd					(const std::string& sourceInterfaceID = "*", unsigned int timeoutInSeconds = 1) const;

    //end FE Communication helpers
    /////////



protected:
	bool 					workLoopThread(toolbox::task::WorkLoop* workLoop);
	std::string             interfaceUID_;

	unsigned int 			universalAddressSize_ = 0;
	unsigned int 			universalDataSize_ = 0;

	//Controls members
	std::map<std::string, FESlowControlsChannel> 	mapOfSlowControlsChannels_;
	FESlowControlsWorkLoop							slowControlsWorkLoop_;


	//FE Macro Function members and helper functions:

	std::map<std::string, frontEndMacroStruct_t>	mapOfFEMacroFunctions_; //Map of FE Macro functions members
	void											registerFEMacroFunction(const std::string& feMacroName, frontEndMacroFunction_t feMacroFunction, const std::vector<std::string>& namesOfInputArgs, const std::vector<std::string>& namesOfOutputArgs, uint8_t requiredUserPermissions = 1 /*1:=user,255:=admin*/, const std::string& allowedCallingFEs = "*" /*StringMacros:: wild card set match string (i.e. string-to-set, then wild-card-set match)*/);

public: //for external specialized template access
	static const std::string&						getFEMacroConstArgument	(frontEndMacroConstArgs_t args , const std::string& argName);
	static std::string&								getFEMacroArgument 		(frontEndMacroArgs_t args, const std::string& argName);

protected:

	template<class T>
	std::string& 									setFEMacroArgumentValue(
			frontEndMacroArgs_t args,
			const std::string& argName, const T& value) const
	{
		//modify existing pair
		std::string& arg = getFEMacroArgument(args,argName);
		std::stringstream ss; ss << value;
		arg = ss.str();
		return arg;
	}
	template<class T>
	std::string& 									emplaceFEMacroArgumentValue(
			frontEndMacroArgs_t args,
			const std::string& argName, const T& value) const
	{
		//insert new pair
		std::stringstream ss; ss << value;
		args.push_back(frontEndMacroArg_t(argName, ss.str()));
		return args.back().second;
	}

}; //end FEVInterface class


template<class T>
T 												getFEMacroConstArgumentValue(
		FEVInterface::frontEndMacroConstArgs_t args, const std::string& argName)
{
	//stolen from ConfigurationView
	//	only handles number types (strings are handled in non-template function override

	const std::string& data = FEVInterface::getFEMacroConstArgument(args, argName);


	T retValue;

	if(!StringMacros::getNumber(data,retValue))
	{
		__SS__ << "Error extracting value for input argument named '" <<
				argName << ".' The value '" << data << "' is not a number!" << std::endl;
		__COUT__ << "\n" << ss.str();
		__SS_THROW__;
	}

	return retValue;
}
//specialized template version of getFEMacroConstArgumentValue for string
template<>
std::string										getFEMacroConstArgumentValue<std::string>	(FEVInterface::frontEndMacroConstArgs_t args, const std::string& argName);

template<class T>
T 												getFEMacroArgumentValue(
		FEVInterface::frontEndMacroArgs_t args, const std::string& argName)
{
	//stolen from ConfigurationView
	//	only handles number types (strings are handled in non-template function override

	const std::string& data = FEVInterface::getFEMacroArgument(args, argName);


	T retValue;

	if(!StringMacros::getNumber(data,retValue))
	{
		__SS__ << "Error extracting value for output argument named '" <<
				argName << ".' The value '" << data << "' is not a number!" << std::endl;
		__COUT__ << "\n" << ss.str();
		__SS_THROW__;
	}

	return retValue;
}
//specialized template version of getFEMacroArgumentValue for string
template<>
std::string										getFEMacroArgumentValue<std::string>	(FEVInterface::frontEndMacroArgs_t argsIn, const std::string& argName);


}

#endif
