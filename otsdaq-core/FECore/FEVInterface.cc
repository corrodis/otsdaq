#include "otsdaq-core/FECore/FEVInterface.h"
#include "otsdaq-core/Macros/CoutMacros.h"

#include "otsdaq-core/NetworkUtilities/UDPDataStreamerBase.h"


#include <iostream>
#include <sstream>

using namespace ots;

//========================================================================================================================
void FEVInterface::configureSlowControls(void)
{
	ConfigurationTree slowControlsGroupLink =
			theXDAQContextConfigTree_.getBackNode(
					theConfigurationPath_).getNode("LinkToSlowControlChannelsConfiguration");

	if(slowControlsGroupLink.isDisconnected())
	{
		__COUT__ << "slowControlsGroupLink is disconnected, so done configuring slow controls." <<
				std::endl;
		return;
	}
	__COUT__ << "slowControlsGroupLink is valid! Configuring slow controls..." <<
			std::endl;

	mapOfSlowControlsChannels_.clear();
	std::vector<std::pair<std::string,ConfigurationTree> > groupLinkChildren =
			slowControlsGroupLink.getChildren();
	for(auto &groupLinkChild: groupLinkChildren)
	{
		//skip channels that are off
		if(!(groupLinkChild.second.getNode(ViewColumnInfo::COL_NAME_STATUS).getValue<bool>())) continue;

		__COUT__ << "Channel:" << getInterfaceUID() <<
				"/" <<  groupLinkChild.first << "\t Type:" <<
				groupLinkChild.second.getNode("ChannelDataType") <<
				std::endl;

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
	__COUT__ << "slowControlsRunning" << std::endl;
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
		__COUT__ << "slowControlsInterfaceLink is disconnected, so no socket made." <<
				std::endl;
	}
	else
	{
		__COUT__ << "slowControlsInterfaceLink is valid! Create tx socket..." <<
				std::endl;
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

		__COUT_INFO__ << "Slow Controls Aggregate Saving turned On BinaryFormat=" <<
				aggregateFileIsBinaryFormat << std::endl;

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
			__COUT_ERR__ << "Failed to open slow controls channel file: " <<
					saveFullFileName << std::endl;
			//continue on, just nothing will be saved
		}
		else
			__COUT_INFO__ << "Slow controls aggregate file opened: " <<
			saveFullFileName << std::endl;
	}
	else
		__COUT_INFO__ << "Slow Controls Aggregate Saving turned off." << std::endl;


	time_t	timeCounter = 0;

	while(slowControlsWorkLoop_.getContinueWorkLoop())
	{
		sleep(1); //seconds
		++timeCounter;

		if(txBuffer.size())
			__COUT__ << "txBuffer sz=" << txBuffer.size() << std::endl;

		txBuffer.resize(0); //clear buffer a la txBuffer = "";

		//__COUT__ << "timeCounter=" << timeCounter << std::endl;
		//__COUT__ << "txBuffer sz=" << txBuffer.size() << std::endl;

		for(auto &slowControlsChannelPair : mapOfSlowControlsChannels_)
		{
			channel = &slowControlsChannelPair.second;

			//skip if no read access
			if(!channel->readAccess_) continue;

			//skip if not a sampling moment in time for channel
			if(timeCounter % channel->delayBetweenSamples_) continue;


			__COUT__ << "Channel:" << getInterfaceUID() <<
					"/" << slowControlsChannelPair.first << std::endl;
			__COUT__ << "Monitoring..." << std::endl;

			universalRead(channel->getUniversalAddress(),
					&readVal[0]);

			//			{ //print
			//				__SS__ << "0x ";
			//				for(int i=(int)universalAddressSize_-1;i>=0;--i)
			//					ss << std::hex << (int)((readVal[i]>>4)&0xF) <<
			//					(int)((readVal[i])&0xF) << " " << std::dec;
			//				ss << std::endl;
			//				__COUT__ << "Sampled.\n" << ss.str();
			//			}

			//have sample
			channel->handleSample(readVal,txBuffer, fp, aggregateFileIsBinaryFormat);
			if(txBuffer.size())
				__COUT__ << "txBuffer sz=" << txBuffer.size() << std::endl;

			//make sure buffer hasn't exploded somehow
			if(txBuffer.size() > txBufferSz)
			{
				__SS__ << "This should never happen hopefully!" << std::endl;
				__COUT_ERR__ << "\n" << ss.str();
				__SS_THROW__;
			}

			//send early if threshold reached
			if(txSocket &&
					txBuffer.size() > txBufferFullThreshold)
			{
				__COUT__ << "Sending now! txBufferFullThreshold=" << txBufferFullThreshold << std::endl;
				txSocket->send(txBuffer);
				txBuffer.resize(0); //clear buffer a la txBuffer = "";
			}


		}

		if(txBuffer.size())
			__COUT__ << "txBuffer sz=" << txBuffer.size() << std::endl;

		//send anything left
		if(txSocket &&
				txBuffer.size())
		{
			__COUT__ << "Sending now!" << std::endl;
			txSocket->send(txBuffer);
		}

		if(fp) fflush(fp); //flush anything in aggregate file for reading ease
	}
	if(fp) fclose(fp);
	return false;
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
		const std::string &feMacroName, frontEndMacroFunction_t feMacroFunction,
		const std::vector<std::string> &namesOfInputArgs,
		const std::vector<std::string> &namesOfOutputArgs,
		uint8_t requiredUserPermissions)
{
	if(mapOfFEMacroFunctions_.find(feMacroName) !=
			mapOfFEMacroFunctions_.end())
	{
		__SS__ << "feMacroName '" << feMacroName << "' already exists! Not allowed." << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		__SS_THROW__;
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
	__SS__ << "Requested input argument not found with name '" << argName << "'" << std::endl;
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
std::string& FEVInterface::getFEMacroOutputArgument(frontEndMacroOutArgs_t& argsOut,
		const std::string& argName)
{

	for(std::pair<const std::string /* output arg name */ , std::string /* arg output value */ >&
			pair : argsOut)
	{
		if(pair.first == argName)
			return pair.second;
	}
	__SS__ << "Requested output argument not found with name '" << argName << "'" << std::endl;
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
	try
	{
		auto configSeqLink = theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode(
				treeLinkName);

		if(configSeqLink.isDisconnected())
			__COUT__ << "Disconnected configure sequence" << std::endl;
		else
		{
			__COUT__ << "Handling configure sequence." << std::endl;
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

				__COUT__ << msg << std::endl;

				universalWrite((char *)&writeAddress,(char *)&(writeHistory[writeAddress]));
			}
		}
	}
	catch(const std::runtime_error &e)
	{
		__COUT__ << "Error accessing sequence, so giving up:\n" << e.what() << std::endl;
	}
	catch(...)
	{
		__COUT__ << "Unknown Error accessing sequence, so giving up." << std::endl;
	}
}
























