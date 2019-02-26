#ifndef _ots_FEVInterface_h_
#define _ots_FEVInterface_h_

#include "otsdaq-core/Configurable/Configurable.h"
#include "otsdaq-core/FECore/FESlowControlsWorkLoop.h"
#include "otsdaq-core/FiniteStateMachine/VStateMachine.h"
#include "otsdaq-core/WorkLoopManager/WorkLoop.h"

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

#include "otsdaq-core/SupervisorInfo/AllSupervisorInfo.h"  //to send errors to Gateway, e.g.

#include "otsdaq-core/CoreSupervisors/FESupervisor.h"

#include "otsdaq-core/FECore/FESlowControlsChannel.h"
#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"  //for xdaq::ApplicationDescriptor communication

#include <array>
#include <iostream>
#include <string>
#include <vector>

#include "otsdaq-core/Macros/CoutMacros.h"

#define __ARGS__                                   \
	frontEndMacroStruct_t feMacroStruct,		   \
	FEVInterface::frontEndMacroConstArgs_t argsIn, \
	    FEVInterface::frontEndMacroArgs_t  argsOut

#define __GET_ARG_IN__(X, Y) getFEMacroConstArgumentValue<Y>(argsIn, X)
#define __GET_ARG_OUT__(X, Y) getFEMacroArgumentValue<Y>(argsOut, X)

#define __SET_ARG_IN__(X, Y) FEVInterface::emplaceFEMacroArgumentValue(argsIn, X, Y)
#define __SET_ARG_OUT__(X, Y) FEVInterface::setFEMacroArgumentValue(argsOut, X, Y)

namespace ots
{
// class FrontEndHardwareBase;
// class FrontEndFirmwareBase;
// class FEInterfaceTableBase;
// class SlowControlsChannelInfo;
class FEVInterfacesManager;

// FEVInterface
//	This class is a virtual class defining the features of front-end interface plugin
// class. 	The features include configuration hooks, finite state machine handlers,
// Front-end Macros for web accessible C++ handlers, slow controls hooks, as well as
// universal write and read for 	Macro Maker compatibility.
//
//	It inherits workloop as 'public virtual' for the case that other classes like
// DataProducer 	 will also be inherited by child class and only one workloop is
// desired.
class FEVInterface : public VStateMachine, public WorkLoop, public Configurable
{
  public:
	FEVInterface(const std::string&       interfaceUID,
	             const ConfigurationTree& theXDAQContextConfigTree,
	             const std::string&       configurationPath);

	virtual ~FEVInterface(void);

	FEVInterfacesManager* parentInterfaceManager_;

	const std::string&  getInterfaceUID(void) const { return interfaceUID_; }
	virtual std::string getInterfaceType(void) const
	{
		return theXDAQContextConfigTree_.getBackNode(theConfigurationPath_)
		    .getNode("FEInterfacePluginName")
		    .getValue<std::string>();
	}  // interfaceType_;}

	virtual void universalRead(
	    char* address,
	    char* returnValue) = 0;  // throw std::runtime_error exception on error/timeout
	virtual void        universalWrite(char* address, char* writeValue) = 0;
	const unsigned int& getUniversalAddressSize(void) { return universalAddressSize_; }
	const unsigned int& getUniversalDataSize(void) { return universalDataSize_; }

	void runSequenceOfCommands(const std::string& treeLinkName);

	static void sendAsyncErrorToGateway(FEVInterface*      fe,
	                                    const std::string& errMsg,
	                                    bool               isSoftError);

	/////////===========================
	// start State Machine handlers
	void configure(void)
	{
		__COUT__ << "\t Configure" << std::endl;
		runSequenceOfCommands(
		    "LinkToConfigureSequence"); /*Run Configure Sequence Commands*/
	}
	void start(std::string runNumber)
	{
		__COUT__ << "\t Start" << std::endl;
		runSequenceOfCommands("LinkToStartSequence"); /*Run Start Sequence Commands*/
	}
	void stop(void)
	{
		__COUT__ << "\t Stop" << std::endl;
		runSequenceOfCommands("LinkToStopSequence"); /*Run Stop Sequence Commands*/
	}
	void halt(void) { stop(); }
	void pause(void) { stop(); }
	void resume(void) { start(""); }
	bool running(void) { /*while(WorkLoop::continueWorkLoop_){;}*/ return false; }
	// end State Machine handlers
	/////////

	/////////===========================
	// start Slow Controls
	void configureSlowControls(void);
	bool slowControlsRunning(void);
	void startSlowControlsWorkLooop(void) { slowControlsWorkLoop_.startWorkLoop(); }
	void stopSlowControlsWorkLooop(void) { slowControlsWorkLoop_.stopWorkLoop(); }
	// end Slow Controls
	/////////

	/////////===========================
	// start FE Macros

	// public types and functions for map of FE macros
	using frontEndMacroArg_t =
	    std::pair<const std::string /* arg name */, std::string /* arg return value */>;
	using frontEndMacroArgs_t = std::vector<frontEndMacroArg_t>&;
	// using	frontEndMacroConstArg_t	= std::pair<const std::string /* input arg name */
	// , const std::string /* arg input value */ >;
	using frontEndMacroConstArgs_t = const std::vector<frontEndMacroArg_t>&;
	struct frontEndMacroStruct_t; //declare name for __ARGS__
	using frontEndMacroFunction_t  = void (ots::FEVInterface::*)(__ARGS__);    // void function (vector-of-inputs, vector-of-outputs)
	struct frontEndMacroStruct_t  // members fully define a front-end macro function
	{
		frontEndMacroStruct_t(
		    const std::string&              feMacroName,
		    const frontEndMacroFunction_t&  feMacroFunction,
		    const std::vector<std::string>& namesOfInputArgs,
		    const std::vector<std::string>& namesOfOutputArgs,
		    const uint8_t      requiredUserPermissions = 1 /*1:=user,255:=admin*/,
		    const std::string& allowedCallingFrontEnds =
		        "*" /*StringMacros:: wild card set match string (i.e. string-to-set, then wild-card-set match)*/)
		    : feMacroName_(feMacroName)
		    , macroFunction_(feMacroFunction)
		    , namesOfInputArguments_(namesOfInputArgs)
		    , namesOfOutputArguments_(namesOfOutputArgs)
		    , requiredUserPermissions_(requiredUserPermissions)
		    , allowedCallingFrontEnds_(allowedCallingFrontEnds)
		{
		}

		const std::string feMacroName_;
		const frontEndMacroFunction_t
		                               macroFunction_;  // Note: must be called using this instance
		const std::vector<std::string> namesOfInputArguments_, namesOfOutputArguments_;
		const uint8_t                  requiredUserPermissions_;
		const std::string              allowedCallingFrontEnds_;
	};
	const std::map<std::string, frontEndMacroStruct_t>& getMapOfFEMacroFunctions(void)
	{
		return mapOfFEMacroFunctions_;
	}
	// end FE Macros
	/////////

	/////////===========================
	// start Macros
	struct macroStruct_t
	{
		macroStruct_t(const std::string& macroString);

		enum
		{
			OP_TYPE_READ,
			OP_TYPE_WRITE,
			OP_TYPE_DELAY,
		};

		struct readOp_t
		{
			uint64_t    address_;
			bool        addressIsVar_;
			std::string addressVarName_;
			bool        dataIsVar_;
			std::string dataVarName_;
		};  // end macroStruct_t::writeOp_t declaration

		struct writeOp_t
		{
			uint64_t    address_;
			bool        addressIsVar_;
			std::string addressVarName_;
			uint64_t    data_;
			bool        dataIsVar_;
			std::string dataVarName_;
		};  // end macroStruct_t::writeOp_t declaration

		struct delayOp_t
		{
			uint64_t    delay_;  // milliseconds
			bool        delayIsVar_;
			std::string delayVarName_;
		};  // end macroStruct_t::writeOp_t declaration

		std::string macroName_;
		std::vector<std::pair<unsigned int /*op type*/,
		                      unsigned int /*index in specific type vector*/> >
		                                      operations_;
		std::vector<macroStruct_t::readOp_t>  readOps_;
		std::vector<macroStruct_t::writeOp_t> writeOps_;
		std::vector<macroStruct_t::delayOp_t> delayOps_;
		std::set<std::string> namesOfInputArguments_, namesOfOutputArguments_;
		bool                  lsbf_;  // least significant byte first
	};                                // end macroStruct_t declaration
	void runMacro(FEVInterface::macroStruct_t&                        macro,
	              std::map<std::string /*name*/, uint64_t /*value*/>& variableMap);
	// end FE Macros
	/////////

	/////////===========================
	// start FE communication helpers

	template<class T>
	void sendToFrontEnd(const std::string& targetInterfaceID, const T& value) const;
	void runFrontEndMacro(
	    const std::string&                                   targetInterfaceID,
	    const std::string&                                   feMacroName,
	    const std::vector<FEVInterface::frontEndMacroArg_t>& inputArgs,
	    std::vector<FEVInterface::frontEndMacroArg_t>&       outputArgs) const;
	void runSelfFrontEndMacro(
	    const std::string&                                   feMacroName,
	    const std::vector<FEVInterface::frontEndMacroArg_t>& inputArgs,
	    std::vector<FEVInterface::frontEndMacroArg_t>&       outputArgs);

	/////////
	// receiveFromFrontEnd
	//	* can be used for source interface ID to accept a message from any front-end
	// NOTE: can not overload functions based on return type, so T& passed as value
	template<class T>
	void receiveFromFrontEnd(const std::string& requester,
	                         T&                 retValue,
	                         unsigned int       timeoutInSeconds = 1) const;
	//	specialized template function for T=std::string
	void receiveFromFrontEnd(const std::string& requester,
	                         std::string&       retValue,
	                         unsigned int       timeoutInSeconds = 1) const;
	// NOTE: can not overload functions based on return type, so calls function with T&
	// passed as value
	template<class T>
	T receiveFromFrontEnd(const std::string& requester        = "*",
	                      unsigned int       timeoutInSeconds = 1) const;
	//	specialized template function for T=std::string
	std::string receiveFromFrontEnd(const std::string& requester        = "*",
	                                unsigned int       timeoutInSeconds = 1) const;

	// end FE Communication helpers
	/////////

  protected:
	bool        workLoopThread(toolbox::task::WorkLoop* workLoop);
	std::string interfaceUID_;

	unsigned int universalAddressSize_ = 0;
	unsigned int universalDataSize_    = 0;

	// Controls members
	std::map<std::string, FESlowControlsChannel> mapOfSlowControlsChannels_;
	FESlowControlsWorkLoop                       slowControlsWorkLoop_;

	// FE Macro Function members and helper functions:

	std::map<std::string, frontEndMacroStruct_t>
	     mapOfFEMacroFunctions_;  // Map of FE Macro functions members
	void registerFEMacroFunction(
	    const std::string&              feMacroName,
	    frontEndMacroFunction_t         feMacroFunction,
	    const std::vector<std::string>& namesOfInputArgs,
	    const std::vector<std::string>& namesOfOutputArgs,
	    uint8_t            requiredUserPermissions = 1 /*1:=user,255:=admin*/,
	    const std::string& allowedCallingFEs =
	        "*" /*StringMacros:: wild card set match string (i.e. string-to-set, then wild-card-set match)*/);

  public:  // for external specialized template access
	static const std::string& getFEMacroConstArgument(frontEndMacroConstArgs_t args,
	                                                  const std::string&       argName);
	static std::string&       getFEMacroArgument(frontEndMacroArgs_t args,
	                                             const std::string&  argName);

  protected:
	template<class T>
	std::string& setFEMacroArgumentValue(frontEndMacroArgs_t args,
	                                     const std::string&  argName,
	                                     const T&            value) const;
	//	{
	//		//modify existing pair
	//		std::string& arg = getFEMacroArgument(args,argName);
	//		std::stringstream ss; ss << value;
	//		arg = ss.str();
	//		return arg;
	//	}
	template<class T>
	std::string& emplaceFEMacroArgumentValue(frontEndMacroArgs_t args,
	                                         const std::string&  argName,
	                                         const T&            value) const;
	//	{
	//		//insert new pair
	//		std::stringstream ss; ss << value;
	//		args.push_back(frontEndMacroArg_t(argName, ss.str()));
	//		return args.back().second;
	//	}

};  // end FEVInterface class

template<class T>
T getFEMacroConstArgumentValue(FEVInterface::frontEndMacroConstArgs_t args,
                               const std::string&                     argName);
// specialized template version of getFEMacroConstArgumentValue for string
template<>
std::string getFEMacroConstArgumentValue<std::string>(
    FEVInterface::frontEndMacroConstArgs_t args, const std::string& argName);

template<class T>
T getFEMacroArgumentValue(FEVInterface::frontEndMacroArgs_t args,
                          const std::string&                argName);

// specialized template version of getFEMacroArgumentValue for string
template<>
std::string getFEMacroArgumentValue<std::string>(FEVInterface::frontEndMacroArgs_t argsIn,
                                                 const std::string& argName);

// include template definitions required at include level for compiler
#include "otsdaq-core/FECore/FEVInterface.icc"

}  // namespace ots

#endif
