#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include "otsdaq-core/NetworkUtilities/UDPDataStreamerBase.h"

#include "otsdaq-core/CoreSupervisors/CoreSupervisorBase.h"


#include <iostream>
#include <sstream>

using namespace ots;



//========================================================================================================================
FEVInterface::FEVInterface (const std::string& interfaceUID,
		const ConfigurationTree& theXDAQContextConfigTree,
		const std::string& configurationPath)
: WorkLoop                    	(interfaceUID)
, Configurable                	(theXDAQContextConfigTree, configurationPath)
, interfaceUID_	              	(interfaceUID)
, interfaceType_				(theXDAQContextConfigTree_.getBackNode(theConfigurationPath_).getNode("FEInterfacePluginName").getValue<std::string>())
, daqHardwareType_            	("NOT SET")
, firmwareType_               	("NOT SET")
, slowControlsWorkLoop_			(interfaceUID + "-SlowControls", this)
{ __CFG_COUT__ << "Constructed." << __E__;}

//========================================================================================================================
void FEVInterface::configureSlowControls(void)
{
	ConfigurationTree slowControlsGroupLink =
			theXDAQContextConfigTree_.getBackNode(
					theConfigurationPath_).getNode("LinkToSlowControlChannelsConfiguration");

	if(slowControlsGroupLink.isDisconnected())
	{
		__CFG_COUT__ << "slowControlsGroupLink is disconnected, so done configuring slow controls." <<
				__E__;
		return;
	}
	__CFG_COUT__ << "slowControlsGroupLink is valid! Configuring slow controls..." <<
			__E__;

	mapOfSlowControlsChannels_.clear();
	std::vector<std::pair<std::string,ConfigurationTree> > groupLinkChildren =
			slowControlsGroupLink.getChildren();
	for(auto &groupLinkChild: groupLinkChildren)
	{
		//skip channels that are off
		if(!(groupLinkChild.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())) continue;

		__CFG_COUT__ << "Channel:" << getInterfaceUID() <<
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
	__CFG_COUT__ << "slowControlsRunning" << __E__;
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
		__CFG_COUT__ << "slowControlsInterfaceLink is disconnected, so no socket made." <<
				__E__;
	}
	else
	{
		__CFG_COUT__ << "slowControlsInterfaceLink is valid! Create tx socket..." <<
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

		__CFG_COUT_INFO__ << "Slow Controls Aggregate Saving turned On BinaryFormat=" <<
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
			__CFG_COUT_ERR__ << "Failed to open slow controls channel file: " <<
					saveFullFileName << __E__;
			//continue on, just nothing will be saved
		}
		else
			__CFG_COUT_INFO__ << "Slow controls aggregate file opened: " <<
			saveFullFileName << __E__;
	}
	else
		__CFG_COUT_INFO__ << "Slow Controls Aggregate Saving turned off." << __E__;


	time_t	timeCounter = 0;

	while(slowControlsWorkLoop_.getContinueWorkLoop())
	{
		sleep(1); //seconds
		++timeCounter;

		if(txBuffer.size())
			__CFG_COUT__ << "txBuffer sz=" << txBuffer.size() << __E__;

		txBuffer.resize(0); //clear buffer a la txBuffer = "";

		//__CFG_COUT__ << "timeCounter=" << timeCounter << __E__;
		//__CFG_COUT__ << "txBuffer sz=" << txBuffer.size() << __E__;

		for(auto &slowControlsChannelPair : mapOfSlowControlsChannels_)
		{
			channel = &slowControlsChannelPair.second;

			//skip if no read access
			if(!channel->readAccess_) continue;

			//skip if not a sampling moment in time for channel
			if(timeCounter % channel->delayBetweenSamples_) continue;


			__CFG_COUT__ << "Channel:" << getInterfaceUID() <<
					"/" << slowControlsChannelPair.first << __E__;
			__CFG_COUT__ << "Monitoring..." << __E__;

			universalRead(channel->getUniversalAddress(),
					&readVal[0]);

			//			{ //print
			//				__CFG_SS__ << "0x ";
			//				for(int i=(int)universalAddressSize_-1;i>=0;--i)
			//					ss << std::hex << (int)((readVal[i]>>4)&0xF) <<
			//					(int)((readVal[i])&0xF) << " " << std::dec;
			//				ss << __E__;
			//				__CFG_COUT__ << "Sampled.\n" << ss.str();
			//			}

			//have sample
			channel->handleSample(readVal,txBuffer, fp, aggregateFileIsBinaryFormat);
			if(txBuffer.size())
				__CFG_COUT__ << "txBuffer sz=" << txBuffer.size() << __E__;

			//make sure buffer hasn't exploded somehow
			if(txBuffer.size() > txBufferSz)
			{
				__CFG_SS__ << "This should never happen hopefully!" << __E__;
				__CFG_COUT_ERR__ << "\n" << ss.str();
				__CFG_SS_THROW__;
			}

			//send early if threshold reached
			if(txSocket &&
					txBuffer.size() > txBufferFullThreshold)
			{
				__CFG_COUT__ << "Sending now! txBufferFullThreshold=" << txBufferFullThreshold << __E__;
				txSocket->send(txBuffer);
				txBuffer.resize(0); //clear buffer a la txBuffer = "";
			}


		}

		if(txBuffer.size())
			__CFG_COUT__ << "txBuffer sz=" << txBuffer.size() << __E__;

		//send anything left
		if(txSocket &&
				txBuffer.size())
		{
			__CFG_COUT__ << "Sending now!" << __E__;
			txSocket->send(txBuffer);
		}

		if(fp) fflush(fp); //flush anything in aggregate file for reading ease
	}
	if(fp) fclose(fp);
	return false;
}

//========================================================================================================================
//workLoopThread
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
		__CFG_SS__;

		try
		{
			throw;
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

		__CFG_COUT_ERR__ << ss.str();

		//send error out to Gateway
		XDAQ_CONST_CALL xdaq::ApplicationDescriptor* gatewaySupervisor =
				VStateMachine::parentSupervisor_->allSupervisorInfo_.getGatewayInfo().getDescriptor();

		__CFG_COUT__ << "Sending FERunningError... " << __E__;

		SOAPParameters               parameters;
		parameters.addParameter("ErrorMessage",ss.str());

		xoap::MessageReference       retMsg =
				VStateMachine::parentSupervisor_->
				SOAPMessenger::sendWithSOAPReply(gatewaySupervisor,
				"AsyncError",parameters);

		std::stringstream retMsgSs;
		retMsgSs << SOAPUtilities::translate(retMsg);
		__CFG_COUT__ << "Received... " << retMsgSs.str() << std::endl;

		if(retMsgSs.str().find("Fault") != std::string::npos)
		{
			__CFG_COUT_ERR__ << "Failure to indicate fault to system... so crashing..." << __E__;
			throw;
		}

		return false;
	}

	return continueWorkLoop_;
}

//========================================================================================================================
//registerFEMacroFunction
//	used by user-defined front-end interface implementations of this
//	virtual interface class to register their macro functions.
//
//	Front-end Macro Functions are then made accessible through the ots Control System
//	web interfaces. The menu consisting of all enabled FEs macros is assembled
//	by the FE Supervisor (and its FE Interface Manager).
void FEVInterface::registerFEMacroFunction(
		const std::string &feMacroName,
		frontEndMacroFunction_t feMacroFunction,
		const std::vector<std::string> &namesOfInputArgs,
		const std::vector<std::string> &namesOfOutputArgs,
		uint8_t requiredUserPermissions)
{
	if(mapOfFEMacroFunctions_.find(feMacroName) !=
			mapOfFEMacroFunctions_.end())
	{
		__CFG_SS__ << "feMacroName '" << feMacroName << "' already exists! Not allowed." << __E__;
		__CFG_COUT_ERR__ << "\n" << ss.str();
		__CFG_SS_THROW__;
	}

	mapOfFEMacroFunctions_.insert(
			std::pair<std::string, frontEndMacroStruct_t> (
					feMacroName,
					frontEndMacroStruct_t(
							feMacroFunction,
							namesOfInputArgs,
							namesOfOutputArgs,
							requiredUserPermissions
					)));
}


//========================================================================================================================
//getFEMacroInputArgument
//	helper function for getting the value of an argument
//
//	Note: static function
const std::string& FEVInterface::getFEMacroInputArgument(frontEndMacroInArgs_t& argsIn,
		const std::string& argName)
{
	for(const std::pair<const std::string /* input arg name */ , const std::string /* arg input value */ >&
			pair : argsIn)
	{
		if(pair.first == argName)
		{

			__COUT__ << "argName : " << pair.second << __E__;
			return pair.second;
		}
	}
	__SS__ << "Requested input argument not found with name '" << argName << "'" << __E__;
	__COUT_ERR__ << "\n" << ss.str();
	__SS_THROW__;
}
//========================================================================================================================
//getFEMacroInputArgumentValue
//	helper function for getting the copy of the value of an argument
template<>
std::string	getFEMacroInputArgumentValue<std::string>(FEVInterface::frontEndMacroInArgs_t &argsIn,
		const std::string &argName)
{
	return FEVInterface::getFEMacroInputArgument(argsIn,argName);
}

//========================================================================================================================
//getFEMacroOutputArgument
//	helper function for getting the value of an argument
//
//	Note: static function
std::string& FEVInterface::getFEMacroOutputArgument(frontEndMacroOutArgs_t& argsOut,
		const std::string& argName)
{

	for(std::pair<const std::string /* output arg name */ , std::string /* arg output value */ >&
			pair : argsOut)
	{
		if(pair.first == argName)
			return pair.second;
	}
	__SS__ << "Requested output argument not found with name '" << argName << "'" << __E__;
	__COUT_ERR__ << "\n" << ss.str();
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
				__CFG_COUT__ << "Disconnected configure sequence" << __E__;
			else
			{
				__CFG_COUT__ << "Handling configure sequence." << __E__;
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

					__CFG_COUT__ << msg << __E__;

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
		__CFG_COUT__ << "Unable to access sequence of commands through configuration tree. " <<
				"Assuming no sequence. " << __E__;
	}
}
























