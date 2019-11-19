#include "otsdaq/DataProcessorPlugins/TCPDataListenerProducer.h"
#include "otsdaq/Macros/CoutMacros.h"
#include "otsdaq/Macros/ProcessorPluginMacros.h"
#include "otsdaq/MessageFacility/MessageFacility.h"
#include "otsdaq/NetworkUtilities/NetworkConverters.h"

#include <string.h>
#include <unistd.h>
#include <cassert>
#include <iostream>

using namespace ots;

//========================================================================================================================
TCPDataListenerProducer::TCPDataListenerProducer(std::string              supervisorApplicationUID,
                                                 std::string              bufferUID,
                                                 std::string              processorUID,
                                                 const ConfigurationTree& theXDAQContextConfigTree,
                                                 const std::string&       configurationPath)
    : WorkLoop(processorUID)
    //, Socket       ("192.168.133.100", 40000)
    , DataProducer(
          supervisorApplicationUID, bufferUID, processorUID, theXDAQContextConfigTree.getNode(configurationPath).getNode("BufferSize").getValue<unsigned int>())
    //, DataProducer (supervisorApplicationUID, bufferUID, processorUID, 100)
    , Configurable(theXDAQContextConfigTree, configurationPath)
    , TCPSubscribeClient(theXDAQContextConfigTree.getNode(configurationPath)
                             .getNode("HostIPAddress")  // THIS IS ACTUALLY THE SERVER IP
                             .getValue<std::string>(),
                         theXDAQContextConfigTree.getNode(configurationPath)
                             .getNode("HostPort")  // THIS IS ACTUALLY THE SERVER PORT
                             .getValue<unsigned int>())
    , dataP_(nullptr)
    , headerP_(nullptr)
    , ipAddress_(theXDAQContextConfigTree.getNode(configurationPath).getNode("HostIPAddress").getValue<std::string>())
    , port_(theXDAQContextConfigTree.getNode(configurationPath).getNode("HostPort").getValue<unsigned int>())
{
}

//========================================================================================================================
TCPDataListenerProducer::~TCPDataListenerProducer(void) {}

//========================================================================================================================
void TCPDataListenerProducer::startProcessingData(std::string runNumber)
{
	TCPSubscribeClient::connect();
	DataProducer::startProcessingData(runNumber);
}

//========================================================================================================================
void TCPDataListenerProducer::stopProcessingData(void)
{
	TCPSubscribeClient::close();
	DataProducer::stopProcessingData();
}

//========================================================================================================================
bool TCPDataListenerProducer::workLoopThread(toolbox::task::WorkLoop* workLoop)
// bool TCPDataListenerProducer::getNextFragment(void)
{
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << DataProcessor::processorUID_
	// << " running, because workloop: " << WorkLoop::continueWorkLoop_ << std::endl;
	fastWrite();
	return WorkLoop::continueWorkLoop_;
}

//========================================================================================================================
void TCPDataListenerProducer::slowWrite(void)
{
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << name_ << " running!" <<
	// std::endl;

	data_                = TCPSubscribeClient::receive<std::string>();  // Throws an exception if it fails
	header_["IPAddress"] = ipAddress_;
	header_["Port"]      = std::to_string(port_);

	while(DataProducer::write(data_, header_) < 0)
	{
		__COUT__ << "There are no available buffers! Retrying...after waiting 10 "
		            "milliseconds!"
		         << std::endl;
		usleep(10000);
		return;
	}
}

//========================================================================================================================
void TCPDataListenerProducer::fastWrite(void)
{
	// std::cout << __COUT_HDR_FL__ << __PRETTY_FUNCTION__ << name_ << " running!" <<
	// std::endl;

	if(DataProducer::attachToEmptySubBuffer(dataP_, headerP_) < 0)
	{
		__COUT__ << "There are no available buffers! Retrying...after waiting 10 milliseconds!" << std::endl;
		usleep(10000);
		return;
	}

	// IT IS A BLOCKING CALL! SO NEEDS TO BE CHANGED
	*dataP_                  = TCPSubscribeClient::receive<std::string>();  // Throws an exception if it fails
	(*headerP_)["IPAddress"] = ipAddress_;
	(*headerP_)["Port"]      = std::to_string(port_);

	// if (port_ == 40005)
	//{
	//	__COUT__ << "Got data: " << dataP_->length() << std::endl;
	//}

	DataProducer::setWrittenSubBuffer<std::string, std::map<std::string, std::string>>();
}

DEFINE_OTS_PROCESSOR(TCPDataListenerProducer)
