#include "otsdaq-core/FECore/FEVInterface.h"


#include "otsdaq-core/NetworkUtilities/UDPDataStreamerBase.h"

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"

#include "otsdaq-core/FECore/FEVInterfacesManager.h"


#include <iostream>
#include <sstream>
#include <thread>       //for std::thread

using namespace ots;

//========================================================================================================================
FEVInterface::FEVInterface (const std::string& interfaceUID,
		const ConfigurationTree& theXDAQContextConfigTree,
		const std::string& configurationPath)
: WorkLoop                    	(interfaceUID)
, Configurable                	(theXDAQContextConfigTree, configurationPath)
, interfaceUID_	              	(interfaceUID)
//, interfaceType_				(theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode("FEInterfacePluginName").getValue<std::string>())
//, daqHardwareType_            	("NOT SET")
//, firmwareType_               	("NOT SET")
, slowControlsWorkLoop_			(interfaceUID + "-SlowControls", this)
{
	//NOTE!! be careful to not decorate with __FE_COUT__ because in the constructor the base class versions of function (e.g. getInterfaceType) are called because the derived class has not been instantiate yet!
	__COUT__ << "'" << interfaceUID << "' Constructed." << __E__;
}

//========================================================================================================================
FEVInterface::~FEVInterface(void)
{
	//NOTE:: be careful not to call __FE_COUT__ decoration because it uses the tree and it may already be destructed partially
	__COUT__ << FEVInterface::interfaceUID_ << " Destructor" << __E__;
}

//========================================================================================================================
void FEVInterface::configureSlowControls(void)
{
	ConfigurationTree slowControlsGroupLink =
			theXDAQContextConfigTree_.getBackNode(
					theConfigurationPath_).getNode("LinkToSlowControlChannelsConfiguration");

	if(slowControlsGroupLink.isDisconnected())
	{
		__FE_COUT__ << "slowControlsGroupLink is disconnected, so done configuring slow controls." <<
				__E__;
		return;
	}
	__FE_COUT__ << "slowControlsGroupLink is valid! Configuring slow controls..." <<
			__E__;

	mapOfSlowControlsChannels_.clear();
	std::vector<std::pair<std::string,ConfigurationTree> > groupLinkChildren =
			slowControlsGroupLink.getChildren();
	for(auto &groupLinkChild: groupLinkChildren)
	{
		//skip channels that are off
		if(!(groupLinkChild.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())) continue;

		__FE_COUT__ << "Channel:" << getInterfaceUID() <<
				"/" <<  groupLinkChild.first << "\t Type:" <<
				groupLinkChild.second.getNode("ChannelDataType") <<
				__E__;

		mapOfSlowControlsChannels_.insert(
				std::pair<std::string,FESlowControlsChannel>(
						groupLinkChild.first,
						FESlowControlsChannel(
								getInterfaceUID(),
								groupLinkChild.first,
								groupLinkChild.second.getNode("ChannelDataType").getValue<std::string>(),
								universalDataSize_,
								universalAddressSize_,
								groupLinkChild.second.getNode("UniversalInterfaceAddress").getValue		<std::string>(),
								groupLinkChild.second.getNode("UniversalDataBitOffset").getValue		<unsigned int>(),
								groupLinkChild.second.getNode("ReadAccess").getValue					<bool>(),
								groupLinkChild.second.getNode("WriteAccess").getValue					<bool>(),
								groupLinkChild.second.getNode("MonitoringEnabled").getValue				<bool>(),
								groupLinkChild.second.getNode("RecordChangesOnly").getValue				<bool>(),
								groupLinkChild.second.getNode("DelayBetweenSamplesInSeconds").getValue	<time_t>(),
								groupLinkChild.second.getNode("LocalSavingEnabled").getValue			<bool>(),
								groupLinkChild.second.getNode("LocalFilePath").getValue					<std::string>(),
								groupLinkChild.second.getNode("RadixFileName").getValue					<std::string>(),
								groupLinkChild.second.getNode("SaveBinaryFile").getValue				<bool>(),
								groupLinkChild.second.getNode("AlarmsEnabled").getValue					<bool>(),
								groupLinkChild.second.getNode("LatchAlarms").getValue					<bool>(),
								groupLinkChild.second.getNode("LowLowThreshold").getValue				<std::string>(),
								groupLinkChild.second.getNode("LowThreshold").getValue					<std::string>(),
								groupLinkChild.second.getNode("HighThreshold").getValue					<std::string>(),
								groupLinkChild.second.getNode("HighHighThreshold").getValue				<std::string>()
						)));
	}

}

//========================================================================================================================
bool FEVInterface::slowControlsRunning(void)
{
	__FE_COUT__ << "slowControlsRunning" << __E__;
	std::string readVal;
	readVal.resize(universalDataSize_); //size to data in advance

	FESlowControlsChannel *channel;

	const unsigned int txBufferSz = 1500;
	const unsigned int txBufferFullThreshold = 750;
	std::string txBuffer;
	txBuffer.reserve(txBufferSz);

	ConfigurationTree FEInterfaceNode = theXDAQContextConfigTree_.getBackNode(
			theConfigurationPath_);

	ConfigurationTree slowControlsInterfaceLink =
			FEInterfaceNode.getNode("LinkToSlowControlsMonitorConfiguration");

	std::unique_ptr<UDPDataStreamerBase> txSocket;

	if(slowControlsInterfaceLink.isDisconnected())
	{
		__FE_COUT__ << "slowControlsInterfaceLink is disconnected, so no socket made." <<
				__E__;
	}
	else
	{
		__FE_COUT__ << "slowControlsInterfaceLink is valid! Create tx socket..." <<
				__E__;
		txSocket.reset(new UDPDataStreamerBase(
				FEInterfaceNode.getNode("SlowControlsTxSocketIPAddress").getValue	<std::string>(),
				FEInterfaceNode.getNode("SlowControlsTxSocketPort").getValue		<int>(),
				slowControlsInterfaceLink.getNode("IPAddress").getValue				<std::string>(),
				slowControlsInterfaceLink.getNode("Port").getValue					<int>()
		));
	}


	//check if aggregate saving


	FILE *fp = 0;
	bool aggregateFileIsBinaryFormat = false;
	if(FEInterfaceNode.getNode("SlowControlsLocalAggregateSavingEnabled").getValue<bool>())
	{
		aggregateFileIsBinaryFormat =
				FEInterfaceNode.getNode("SlowControlsSaveBinaryFile").getValue<bool>();

		__FE_COUT_INFO__ << "Slow Controls Aggregate Saving turned On BinaryFormat=" <<
				aggregateFileIsBinaryFormat << __E__;

		std::string saveFullFileName =
				FEInterfaceNode.getNode("SlowControlsLocalFilePath").getValue<std::string>() +
				"/" +
				FEInterfaceNode.getNode("SlowControlsRadixFileName").getValue<std::string>() +
				"-" +
				FESlowControlsChannel::underscoreString(getInterfaceUID()) +
				"-" + std::to_string(time(0)) +
				(aggregateFileIsBinaryFormat?".dat":".txt");


		fp = fopen(saveFullFileName.c_str(),
				aggregateFileIsBinaryFormat?
						"ab":"a");
		if(!fp)
		{
			__FE_COUT_ERR__ << "Failed to open slow controls channel file: " <<
					saveFullFileName << __E__;
			//continue on, just nothing will be saved
		}
		else
			__FE_COUT_INFO__ << "Slow controls aggregate file opened: " <<
			saveFullFileName << __E__;
	}
	else
		__FE_COUT_INFO__ << "Slow Controls Aggregate Saving turned off." << __E__;


	time_t	timeCounter = 0;

	while(slowControlsWorkLoop_.getContinueWorkLoop())
	{
		sleep(1); //seconds
		++timeCounter;

		if(txBuffer.size())
			__FE_COUT__ << "txBuffer sz=" << txBuffer.size() << __E__;

		txBuffer.resize(0); //clear buffer a la txBuffer = "";

		//__FE_COUT__ << "timeCounter=" << timeCounter << __E__;
		//__FE_COUT__ << "txBuffer sz=" << txBuffer.size() << __E__;

		for(auto &slowControlsChannelPair : mapOfSlowControlsChannels_)
		{
			channel = &slowControlsChannelPair.second;

			//skip if no read access
			if(!channel->readAccess_) continue;

			//skip if not a sampling moment in time for channel
			if(timeCounter % channel->delayBetweenSamples_) continue;


			__FE_COUT__ << "Channel:" << getInterfaceUID() <<
					"/" << slowControlsChannelPair.first << __E__;
			__FE_COUT__ << "Monitoring..." << __E__;

			universalRead(channel->getUniversalAddress(),
					&readVal[0]);

			//			{ //print
			//				__FE_SS__ << "0x ";
			//				for(int i=(int)universalAddressSize_-1;i>=0;--i)
			//					ss << std::hex << (int)((readVal[i]>>4)&0xF) <<
			//					(int)((readVal[i])&0xF) << " " << std::dec;
			//				ss << __E__;
			//				__FE_COUT__ << "Sampled.\n" << ss.str();
			//			}

			//have sample
			channel->handleSample(readVal,txBuffer, fp, aggregateFileIsBinaryFormat);
			if(txBuffer.size())
				__FE_COUT__ << "txBuffer sz=" << txBuffer.size() << __E__;

			//make sure buffer hasn't exploded somehow
			if(txBuffer.size() > txBufferSz)
			{
				__FE_SS__ << "This should never happen hopefully!" << __E__;
				__FE_COUT_ERR__ << "\n" << ss.str();
				__FE_SS_THROW__;
			}

			//send early if threshold reached
			if(txSocket &&
					txBuffer.size() > txBufferFullThreshold)
			{
				__FE_COUT__ << "Sending now! txBufferFullThreshold=" << txBufferFullThreshold << __E__;
				txSocket->send(txBuffer);
				txBuffer.resize(0); //clear buffer a la txBuffer = "";
			}


		}

		if(txBuffer.size())
			__FE_COUT__ << "txBuffer sz=" << txBuffer.size() << __E__;

		//send anything left
		if(txSocket &&
				txBuffer.size())
		{
			__FE_COUT__ << "Sending now!" << __E__;
			txSocket->send(txBuffer);
		}

		if(fp) fflush(fp); //flush anything in aggregate file for reading ease
	}
	if(fp) fclose(fp);
	return false;
} //end slowControlsRunning()

//========================================================================================================================
//SendAsyncErrorToGateway
//	Static -- thread
//	Send async error or soft error to gateway
//	Do this as thread so that workloop can end
void FEVInterface::sendAsyncErrorToGateway(FEVInterface* fe, const std::string& errorMessage, bool isSoftError)
try
{
	if(isSoftError)
		__COUT_ERR__ << ":FE:" << fe->getInterfaceType() << ":" <<
		fe->getInterfaceUID() << ":" << fe->theConfigurationRecordName_ << ":" <<
			"Sending FE Async SOFT Running Error... \n" << errorMessage << __E__;
	else
		__COUT_ERR__ << ":FE:" << fe->getInterfaceType() << ":" <<
		fe->getInterfaceUID() << ":" << fe->theConfigurationRecordName_ << ":" <<
			"Sending FE Async Running Error... \n" << errorMessage << __E__;

	XDAQ_CONST_CALL xdaq::ApplicationDescriptor* gatewaySupervisor =
			fe->VStateMachine::parentSupervisor_->allSupervisorInfo_.getGatewayInfo().getDescriptor();

	SOAPParameters parameters;
	parameters.addParameter("ErrorMessage",errorMessage);

	xoap::MessageReference replyMessage =
			fe->VStateMachine::parentSupervisor_->
			SOAPMessenger::sendWithSOAPReply(
					gatewaySupervisor,
					isSoftError?"AsyncSoftError":"AsyncError",
							parameters);

	std::stringstream replyMessageSStream;
	replyMessageSStream << SOAPUtilities::translate(replyMessage);
	__COUT__ << ":FE:" << fe->getInterfaceType() << ":" << fe->getInterfaceUID() << ":" << fe->theConfigurationRecordName_ << ":" <<
			"Received... " << replyMessageSStream.str() << std::endl;

	if(replyMessageSStream.str().find("Fault") != std::string::npos)
	{
		__COUT_ERR__ << ":FE:" << fe->getInterfaceType() << ":" <<
				fe->getInterfaceUID() << ":" << fe->theConfigurationRecordName_ << ":" <<
				"Failure to indicate fault to Gateway..." << __E__;
		throw;
	}
}
catch(const xdaq::exception::Exception& e)
{
	if(isSoftError)
		__COUT__ << "SOAP message failure indicating front-end asynchronous running SOFT error back to Gateway: " <<
		e.what() << __E__;
	else
		__COUT__ << "SOAP message failure indicating front-end asynchronous running error back to Gateway: " <<
		e.what() << __E__;
}
catch(...)
{
	if(isSoftError)
		__COUT__ << "Unknown error encounter indicating front-end asynchronous running SOFT error back to Gateway." << __E__;
	else
		__COUT__ << "Unknown error encounter indicating front-end asynchronous running error back to Gateway." << __E__;
} //end SendAsyncErrorToGateway()

//========================================================================================================================
//override WorkLoop::workLoopThread
//	return false to stop the workloop from calling the thread again
bool FEVInterface::workLoopThread(toolbox::task::WorkLoop* workLoop)
{
	try
	{
		continueWorkLoop_ = running(); /* in case users return false, without using continueWorkLoop_*/
	}
	catch(...) //
	{
		//catch all, then rethrow with local variables needed
		__FE_SS__;

		bool isSoftError = false;

		try
		{
			throw;
		}
		catch(const __OTS_SOFT_EXCEPTION__& e)
		{
			ss << "SOFT Error was caught while configuring: " << e.what() << std::endl;
			isSoftError = true;
		}
		catch(const std::runtime_error& e)
		{
			ss << "Caught an error during running at FE Interface '" <<
					Configurable::theConfigurationRecordName_ <<
					"': " << e.what() << __E__;
		}
		catch(...)
		{
			ss << "Caught an unknown error during running." << __E__;
		}

		//At this point, an asynchronous error has occurred
		//	during front-end running...
		//Send async error to Gateway

		__FE_COUT_ERR__ << ss.str();

		std::thread([](FEVInterface* fe, const std::string errorMessage, bool isSoftError)
				{
			FEVInterface::sendAsyncErrorToGateway(fe,errorMessage,isSoftError);
				},
			//pass the values
			this,
			ss.str(),
			isSoftError).detach();

		return false;
	}

	return continueWorkLoop_;
} //end workLoopThread()

//========================================================================================================================
//registerFEMacroFunction
//	used by user-defined front-end interface implementations of this
//	virtual interface class to register their macro functions.
//
//	Front-end Macro Functions are then made accessible through the ots Control System
//	web interfaces. The menu consisting of all enabled FEs macros is assembled
//	by the FE Supervisor (and its FE Interface Manager).
void FEVInterface::registerFEMacroFunction(
		const std::string& feMacroName,
		frontEndMacroFunction_t feMacroFunction,
		const std::vector<std::string> &namesOfInputArgs,
		const std::vector<std::string> &namesOfOutputArgs,
		uint8_t requiredUserPermissions,
		const std::string& allowedCallingFEs)
{
	if(mapOfFEMacroFunctions_.find(feMacroName) !=
			mapOfFEMacroFunctions_.end())
	{
		__FE_SS__ << "feMacroName '" << feMacroName << "' already exists! Not allowed." << __E__;
		__FE_COUT_ERR__ << "\n" << ss.str();
		__FE_SS_THROW__;
	}

	mapOfFEMacroFunctions_.insert(
			std::pair<std::string, frontEndMacroStruct_t> (
					feMacroName,
					frontEndMacroStruct_t(
							feMacroName,
							feMacroFunction,
							namesOfInputArgs,
							namesOfOutputArgs,
							requiredUserPermissions,
							allowedCallingFEs
					)));
}


//========================================================================================================================
//getFEMacroConstArgument
//	helper function for getting the value of an argument
//
//	Note: static function
const std::string& FEVInterface::getFEMacroConstArgument(frontEndMacroConstArgs_t& args,
		const std::string& argName)
{
	for(const std::pair<const std::string /* input arg name */ ,
			const std::string /* arg input value */ >&
			pair : args)
	{
		if(pair.first == argName)
		{

			__COUT__ << "argName : " << pair.second << __E__;
			return pair.second;
		}
	}
	__SS__ << "Requested input argument not found with name '" <<
			argName << "'" << __E__;
	__SS_THROW__;
}

//========================================================================================================================
//getFEMacroConstArgumentValue
//	helper function for getting the copy of the value of an argument
template<>
std::string	getFEMacroConstArgumentValue<std::string>(
		FEVInterface::frontEndMacroConstArgs_t &args,
		const std::string &argName)
{
	return FEVInterface::getFEMacroConstArgument(args,argName);
}

//========================================================================================================================
//getFEMacroArgumentValue
//	helper function for getting the copy of the value of an argument
template<>
std::string	getFEMacroArgumentValue<std::string>(
		FEVInterface::frontEndMacroArgs_t &args,
		const std::string &argName)
{
	return FEVInterface::getFEMacroArgument(args,argName);
}

//========================================================================================================================
//getFEMacroOutputArgument
//	helper function for getting the value of an argument
//
//	Note: static function
std::string& FEVInterface::getFEMacroArgument(
		frontEndMacroArgs_t& args,
		const std::string& argName)
{

	for(std::pair<const std::string /* output arg name */ , std::string /* arg output value */ >&
			pair : args)
	{
		if(pair.first == argName)
			return pair.second;
	}
	__SS__ << "Requested argument not found with name '" <<
			argName << "'" << __E__;
	__SS_THROW__;
}

//========================================================================================================================
//runSequenceOfCommands
//	runs a sequence of write commands from a linked section of the configuration tree
//		based on these fields:
//			- WriteAddress,  WriteValue, StartingBitPosition, BitFieldSize
void FEVInterface::runSequenceOfCommands(const std::string &treeLinkName)
{
	std::map<uint64_t,uint64_t> writeHistory;
	uint64_t writeAddress, writeValue, bitMask;
	uint8_t bitPosition;

	std::string writeBuffer;
	std::string readBuffer;
	char msg[1000];
	bool ignoreError = true;

	//ignore errors getting sequence of commands through tree (since it is optional)
	try
	{
		ConfigurationTree configSeqLink = theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode(
				treeLinkName);


		//but throw errors if problems executing the sequence of commands
		try
		{
			if(configSeqLink.isDisconnected())
				__FE_COUT__ << "Disconnected configure sequence" << __E__;
			else
			{
				__FE_COUT__ << "Handling configure sequence." << __E__;
				auto childrenMap = configSeqLink.getChildrenMap();
				for(const auto &child:childrenMap)
				{
					//WriteAddress and WriteValue fields

					writeAddress = child.second.getNode("WriteAddress").getValue<uint64_t>();
					writeValue = child.second.getNode("WriteValue").getValue<uint64_t>();
					bitPosition = child.second.getNode("StartingBitPosition").getValue<uint8_t>();
					bitMask = (1 << child.second.getNode("BitFieldSize").getValue<uint8_t>())-1;

					writeValue &= bitMask;
					writeValue <<= bitPosition;
					bitMask = ~(bitMask<<bitPosition);

					//place into write history
					if(writeHistory.find(writeAddress) == writeHistory.end())
						writeHistory[writeAddress] = 0;//init to 0

					writeHistory[writeAddress] &= bitMask; //clear incoming bits
					writeHistory[writeAddress] |= writeValue; //add incoming bits

					sprintf(msg,"\t Writing %s: \t %ld(0x%lX) \t %ld(0x%lX)", child.first.c_str(),
							writeAddress, writeAddress,
							writeHistory[writeAddress], writeHistory[writeAddress]);

					__FE_COUT__ << msg << __E__;

					universalWrite((char *)&writeAddress,(char *)&(writeHistory[writeAddress]));
				}
			}
		}
		catch(...)
		{
			ignoreError = false;
			throw;
		}
	}
	catch(...)
	{
		if(!ignoreError) throw;
		//else ignoring error
		__FE_COUT__ << "Unable to access sequence of commands through configuration tree. " <<
				"Assuming no sequence. " << __E__;
	}
}


//========================================================================================================================
//runFrontEndMacro
//	run a front-end macro in the target interface plug-in and gets the output arguments back
void FEVInterface::runFrontEndMacro(const std::string& targetInterfaceID,
		const std::string& feMacroName,
		const std::vector<frontEndMacroArg_t>& inputArgs,
		std::vector<frontEndMacroArg_t>& outputArgs) const
{

	__FE_COUTV__(targetInterfaceID);
	__FE_COUTV__(VStateMachine::parentSupervisor_);


	std::string inputArgsStr =
			StringMacros::vectorToString(inputArgs,";" /*primaryDelimeter*/,","/*secondaryDelimeter*/);

	__FE_COUTV__(inputArgsStr);

	xoap::MessageReference message =
		SOAPUtilities::makeSOAPMessageReference("FECommunication");

	SOAPParameters parameters;
	parameters.addParameter("type", "feMacro");
	parameters.addParameter("sourceInterfaceID", FEVInterface::interfaceUID_);
	parameters.addParameter("targetInterfaceID", targetInterfaceID);
	parameters.addParameter("feMacroName", feMacroName);
	parameters.addParameter("inputArgs", inputArgsStr);
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

	//extract output args
	SOAPParameters argsOutParameter;
	argsOutParameter.addParameter("outputArgs");
	SOAPUtilities::receive(replyMessage,argsOutParameter);

	std::string outputArgsStr = argsOutParameter.getValue("outputArgs");
	std::set<char> pairDelimiter({';'}), nameValueDelimiter({','});

	std::map<std::string,std::string> mapToReturn;
	StringMacros::getMapFromString(
			outputArgsStr,
			mapToReturn,
			pairDelimiter,
			nameValueDelimiter);

	outputArgs.clear();
	for(auto& mapPair: mapToReturn)
		outputArgs.push_back(mapPair);

} //end runFrontEndMacro()

//========================================================================================================================
//receiveFromFrontEnd
//	specialized template function for T=std::string
//
//	Note: sourceInterfaceID can be a wildcard string as defined in StringMacros
void FEVInterface::receiveFromFrontEnd(const std::string& sourceInterfaceID, std::string& retValue, unsigned int timeoutInSeconds) const
{
	__FE_COUTV__(sourceInterfaceID);
	__FE_COUTV__(parentSupervisor_);

	std::string data = "0";
	bool found = false;
	while(1)
	{
		//mutex scope
		{
			std::lock_guard<std::mutex> lock(
					parentInterfaceManager_->frontEndCommunicationReceiveMutex_);

			auto receiveBuffersForTargetIt =
					parentInterfaceManager_->frontEndCommunicationReceiveBuffer_.find(
							FEVInterface::interfaceUID_);
			if(receiveBuffersForTargetIt !=
					parentInterfaceManager_->frontEndCommunicationReceiveBuffer_.end())
			{
				__FE_COUT__ << "Number of source buffers found for front-end '" <<
						FEVInterface::interfaceUID_ << "': " <<
						receiveBuffersForTargetIt->second.size() << __E__;

				for(auto& buffPair:receiveBuffersForTargetIt->second)
					__FE_COUTV__(buffPair.first);

				//match sourceInterfaceID to map of buffers
				std::string sourceBufferId = "";
				std::queue<std::string /*value*/>& sourceBuffer =
									StringMacros::getWildCardMatchFromMap(sourceInterfaceID,
						receiveBuffersForTargetIt->second,
						&sourceBufferId);

				__FE_COUT__ << "Found source buffer '" << sourceBufferId <<
						"' with size " << sourceBuffer.size() << __E__;

				if(sourceBuffer.size())
				{
					__FE_COUT__ << "Found a value in queue of size " <<
							sourceBuffer.size() << __E__;

					//remove from receive buffer
					retValue = sourceBuffer.front();
					sourceBuffer.pop();
					return;
				}
				else
					__FE_COUT__ << "Source buffer empty for '" <<
						sourceInterfaceID << "'" << __E__;
			}

			//else, not found...

			//if out of time, throw error
			if(!timeoutInSeconds)
			{
				__FE_SS__ << "Timeout (" << timeoutInSeconds <<
						" s) waiting for front-end communication from " <<
						sourceInterfaceID << "." << __E__;
				__FE_SS_THROW__;
			}
			//else, there is still hope

		} //end mutex scope

		//else try again in a sec
		__FE_COUT__ << "Waiting for front-end communication from " <<
				sourceInterfaceID << " for " <<
				timeoutInSeconds << " more seconds..." << __E__;

		--timeoutInSeconds;
		sleep(1); //wait a sec
	} //end timeout loop

	//should never get here
} //end receiveFromFrontEnd()

//========================================================================================================================
//receiveFromFrontEnd
//	specialized template function for T=std::string
//	Note: if called without template <T> syntax, necessary because types of std::basic_string<char> cause compiler problems if no string specific function
std::string FEVInterface::receiveFromFrontEnd(const std::string& sourceInterfaceID, unsigned int timeoutInSeconds) const
{
	std::string retValue;
	FEVInterface::receiveFromFrontEnd(sourceInterfaceID,retValue,timeoutInSeconds);
	return retValue;
} //end receiveFromFrontEnd()























