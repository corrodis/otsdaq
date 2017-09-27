#include "otsdaq-core/DataManager/RawDataSaverConsumerBase.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
//#include "otsdaq-core/Macros/CoutHeaderMacros.h"

#include <iostream>
#include <cassert>
#include <unistd.h>
//#include <string.h> //memcpy
#include <sstream>

using namespace ots;


//========================================================================================================================
RawDataSaverConsumerBase::RawDataSaverConsumerBase(std::string supervisorApplicationUID, std::string bufferUID, std::string processorUID, const ConfigurationTree& theXDAQContextConfigTree, const std::string& configurationPath)
: WorkLoop            (processorUID)
, DataConsumer        (supervisorApplicationUID, bufferUID, processorUID, HighConsumerPriority)
, Configurable        (theXDAQContextConfigTree, configurationPath)
, filePath_           (theXDAQContextConfigTree.getNode(configurationPath).getNode("FilePath").getValue<std::string>())
, fileRadix_          (theXDAQContextConfigTree.getNode(configurationPath).getNode("RadixFileName").getValue<std::string>())
, maxFileSize_        (theXDAQContextConfigTree.getNode(configurationPath).getNode("MaxFileSize").getValue<long>()*1000000)//Instead of 2^6=1048576
, currentSubRunNumber_(0)

{
}

//========================================================================================================================
RawDataSaverConsumerBase::~RawDataSaverConsumerBase(void)
{}

//========================================================================================================================
void RawDataSaverConsumerBase::startProcessingData(std::string runNumber)
{
	openFile(runNumber);
	DataConsumer::startProcessingData(runNumber);
}

//========================================================================================================================
void RawDataSaverConsumerBase::stopProcessingData(void)
{
	DataConsumer::stopProcessingData();
	closeFile();
}

//========================================================================================================================
void RawDataSaverConsumerBase::openFile(std::string runNumber)
{
	currentRunNumber_    = runNumber;
//	std::string fileName =   "Run" + runNumber + "_" + processorUID_ + "_Raw.dat";
	std::stringstream fileName;
	fileName << filePath_ << "/" << fileRadix_ << "_Run" << runNumber;
	//if split file is there then subrunnumber must be set!
	if(maxFileSize_ > 0)
		fileName << "_" << currentSubRunNumber_;
	fileName << "_Raw.dat";
	std::cout << __COUT_HDR_FL__ << "Saving file: " << fileName.str() << std::endl;
	outFile_.open(fileName.str().c_str(), std::ios::out | std::ios::binary);
	if(!outFile_.is_open())
	{
		__SS__ << "Can't open file " << fileName.str() << std::endl;
		__COUT_ERR__ << "\n" << ss.str();
		throw std::runtime_error(ss.str());
	}

	writeHeader(); //write start of file header
}

//========================================================================================================================
void RawDataSaverConsumerBase::closeFile(void)
{
	if(outFile_.is_open())
	{
		writeFooter(); //write end of file footer
		outFile_.close();
	}
}

//========================================================================================================================
void RawDataSaverConsumerBase::save(const std::string& data)
{
	if(maxFileSize_ > 0)
	{
		long length = outFile_.tellp();
		if(length >= maxFileSize_)
		{
			closeFile();
			++currentSubRunNumber_;
			openFile(currentRunNumber_);
		}
	}

	writePacketHeader(data); //write start of packet header
	outFile_.write( (char*)&data[0], data.length());
	writePacketFooter(data); //write start of packet footer
}

//========================================================================================================================
bool RawDataSaverConsumerBase::workLoopThread(toolbox::task::WorkLoop* workLoop)
{
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << DataProcessor::processorUID_ << " running, because workloop: " << WorkLoop::continueWorkLoop_ << std::endl;
	fastRead();
	return WorkLoop::continueWorkLoop_;
}

//========================================================================================================================
void RawDataSaverConsumerBase::fastRead(void)
{
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " running!" << std::endl;
	if(DataConsumer::read(dataP_, headerP_) < 0)
	{
		usleep(100);
		return;
	}
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " Buffer: " << buffer << std::endl;
	//std::cout << __COUT_HDR_FL__ << dataP_->length() << std::endl;
	save(*dataP_);
	DataConsumer::setReadSubBuffer<std::string, std::map<std::string, std::string>>();
}

//========================================================================================================================
void RawDataSaverConsumerBase::slowRead(void)
{
	//std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << processorUID_ << " running!" << std::endl;
	//This is making a copy!!!
	if(DataConsumer::read(data_, header_) < 0)
	{
		usleep(1000);
		return;
	}
	save(data_);
}
